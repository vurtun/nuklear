/*
    Copyright (c) 2015
    vurtun <polygone@gmx.net>
    MIT licence
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <assert.h>
#include <time.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>

#include "gui.h"

/* macros */
#define WIN_WIDTH   800
#define WIN_HEIGHT  600
#define DTIME       33
#define MAX_BUFFER (64 * 1024)
#define INPUT_MAX 64

#define LEN(a)(sizeof(a)/sizeof(a)[0])
#define UNUSED(a)((void)(a))

/* types  */
struct XWindow {
    Window root;
    Display *dpy;
    XVisualInfo *vi;
    Colormap cmap;
    XSetWindowAttributes swa;
    XWindowAttributes gwa;
    Window win;
    GLXContext glc;
    gui_bool running;
};

struct GUI {
    struct XWindow *win;
    struct gui_memory main;
    struct gui_memory combo;
    struct gui_memory_status status;
    struct gui_draw_buffer out;
    struct gui_draw_call_list draw_list;
    struct gui_input in;
    struct gui_font *font;
    struct gui_config config;
    struct gui_panel panel;
    struct gui_panel tab;
    gui_bool selection[6];
    gui_float seloff;

    /* State */
    gui_char input_text[INPUT_MAX];
    gui_char cmd_input[INPUT_MAX];
    gui_char box_input[INPUT_MAX];
    gui_size input_len;
    gui_size cmd_len;
    gui_size box_len;
    gui_float box_off;
    gui_bool box_act;
    gui_bool typing;
    gui_float slider;
    gui_size prog;
    gui_size vprog;
    gui_size vprog2;
    gui_size vprog3;
    gui_int spinner;
    gui_bool check;
    gui_bool option;
    gui_bool spinning;
    gui_size submit;
    gui_bool submitting;
    gui_size select;
};

/* functions */
static void die(const char*,...);
static void* xcalloc(size_t nmemb, size_t size);
static long timestamp(void);
static void sleep_for(long ms);
static char* ldfile(const char*, int, size_t*);

static void kpress(struct GUI*, XEvent*);
static void bpress(struct GUI*, XEvent*);
static void brelease(struct GUI*, XEvent*);
static void bmotion(struct GUI*, XEvent*);
static void resize(struct GUI*, XEvent*);

static GLuint ldbmp(gui_byte*, uint32_t*, uint32_t*);
static struct gui_font *ldfont(const char*, unsigned char);
static void delfont(struct gui_font*);

/* gobals */
static void
die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

static void*
xcalloc(size_t nmemb, size_t size)
{
    void *p = calloc(nmemb, size);
    if (!p)
        die("out of memory\n");
    return p;
}

static long
timestamp(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0) return 0;
    return (long)((long)tv.tv_sec * 1000 + (long)tv.tv_usec/1000);
}

static void
sleep_for(long t)
{
    const time_t sec = (int)(t/1000);
    const long ms = t - (sec * 1000);
    struct timespec req;
    req.tv_sec = sec;
    req.tv_nsec = ms * 1000000L;
    while(-1 == nanosleep(&req, &req));
}

static void
kpress(struct GUI *gui, XEvent* e)
{
    int ret;
    struct XWindow *xw = gui->win;
    KeySym *keysym = XGetKeyboardMapping(xw->dpy, (KeyCode)e->xkey.keycode, 1, &ret);
    if (*keysym == XK_Shift_L || *keysym == XK_Shift_R)
        gui_input_key(&gui->in, GUI_KEY_SHIFT, gui_true);
    else if (*keysym == XK_Control_L || *keysym == XK_Control_L)
        gui_input_key(&gui->in, GUI_KEY_CTRL, gui_true);
    else if (*keysym == XK_Delete)
        gui_input_key(&gui->in, GUI_KEY_DEL, gui_true);
    else if (*keysym == XK_Return)
        gui_input_key(&gui->in, GUI_KEY_ENTER, gui_true);
    else if (*keysym == XK_space)
        gui_input_key(&gui->in, GUI_KEY_SPACE, gui_true);
    else if (*keysym == XK_BackSpace)
        gui_input_key(&gui->in, GUI_KEY_BACKSPACE, gui_true);
    else if ((*keysym >= 'a' && *keysym <= 'z') ||
            (*keysym >= '0' && *keysym <= '9'))
        gui_input_char(&gui->in, (unsigned char*)keysym);
    XFree(keysym);
}

static void
krelease(struct GUI *gui, XEvent* e)
{
    int ret;
    struct XWindow *xw = gui->win;
    KeySym *keysym = XGetKeyboardMapping(xw->dpy, (KeyCode)e->xkey.keycode, 1, &ret);
    if (*keysym == XK_Shift_L || *keysym == XK_Shift_R)
        gui_input_key(&gui->in, GUI_KEY_SHIFT, gui_false);
    else if (*keysym == XK_Control_L || *keysym == XK_Control_L)
        gui_input_key(&gui->in, GUI_KEY_CTRL, gui_false);
    else if (*keysym == XK_Delete)
        gui_input_key(&gui->in, GUI_KEY_DEL, gui_false);
    else if (*keysym == XK_Return)
        gui_input_key(&gui->in, GUI_KEY_ENTER, gui_false);
    else if (*keysym == XK_space)
        gui_input_key(&gui->in, GUI_KEY_SPACE, gui_false);
    else if (*keysym == XK_BackSpace)
        gui_input_key(&gui->in, GUI_KEY_BACKSPACE, gui_false);
    XFree(keysym);
}

static void
bpress(struct GUI *gui, XEvent *evt)
{
    const int x = evt->xbutton.x;
    const int y = evt->xbutton.y;
    if (evt->xbutton.button == Button1)
        gui_input_button(&gui->in, x, y, gui_true);
}

static void
brelease(struct GUI *con, XEvent *evt)
{
    const int x = evt->xbutton.x;
    const int y = evt->xbutton.y;
    UNUSED(evt);
    if (evt->xbutton.button == Button1)
        gui_input_button(&con->in, x, y, gui_false);
}

static void
bmotion(struct GUI *gui, XEvent *evt)
{
    const gui_int x = evt->xbutton.x;
    const gui_int y = evt->xbutton.y;
    UNUSED(evt);
    gui_input_motion(&gui->in, x, y);
}

static void
resize(struct GUI *con, XEvent* evt)
{
    struct XWindow *xw = con->win;
    UNUSED(evt);
    XGetWindowAttributes(xw->dpy, xw->win, &xw->gwa);
    glViewport(0, 0, xw->gwa.width, xw->gwa.height);
}

static char*
ldfile(const char* path, int flags, size_t* siz)
{
    char *buf;
    int fd = open(path, flags);
    struct stat status;
    if (fd == -1)
        die("Failed to open file: %s (%s)\n",
            path, strerror(errno));
    if (fstat(fd, &status) < 0)
        die("Failed to call fstat: %s",strerror(errno));
    *siz = (size_t)status.st_size;
    buf = xcalloc(*siz, 1);
    if (read(fd, buf, *siz) < 0)
        die("Failed to call read: %s",strerror(errno));
    close(fd);
    return buf;
}

static GLuint
ldbmp(gui_byte *data, uint32_t *width, uint32_t *height)
{
    /* texture */
    GLuint texture;
    gui_byte *header;
    gui_byte *target;
    gui_byte *writer;
    gui_byte *reader;
    size_t mem;
    uint32_t ioff;
    uint32_t j;
    int32_t i;

    header = data;
    if (!width || !height)
        die("[BMP]: width or height is NULL!");
    if (header[0] != 'B' || header[1] != 'M')
        die("[BMP]: invalid file");

    memcpy(width, &header[0x12], sizeof(uint32_t));
    memcpy(height, &header[0x16], sizeof(uint32_t));
    memcpy(&ioff, &header[0x0A], sizeof(uint32_t));
    if (*width <= 0 || *height <= 0)
        die("[BMP]: invalid image size");

    data = data + ioff;
    reader = data;
    mem = *width * *height * 4;
    target = xcalloc(mem, 1);
    for (i = (int32_t)*height-1; i >= 0; i--) {
        writer = target + (i * (int32_t)*width * 4);
        for (j = 0; j < *width; j++) {
            *writer++ = *(reader + (j * 4) + 1);
            *writer++ = *(reader + (j * 4) + 2);
            *writer++ = *(reader + (j * 4) + 3);
            *writer++ = *(reader + (j * 4) + 0);
        }
        reader += *width * 4;
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)*width, (GLsizei)*height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, target);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    free(target);
    return texture;
}

static struct gui_font*
ldfont(const char *name, unsigned char height)
{
    union conversion {
        gui_texture handle;
        uintptr_t ptr;
    } convert;
    size_t size;
    struct gui_font *font;
    uint32_t img_width, img_height;
    short max_height = 0;
    size_t i = 0;
    size_t mem;
    gui_byte *iter;
    struct gui_font_glyph *glyphes;

    uint16_t num;
    uint16_t indexes;
    uint16_t tex_width;
    uint16_t tex_height;

    /* header */
    gui_byte *buffer = (gui_byte*)ldfile(name, O_RDONLY, &size);
    memcpy(&num, buffer, sizeof(uint16_t));
    memcpy(&indexes, &buffer[0x02], sizeof(uint16_t));
    memcpy(&tex_width, &buffer[0x04], sizeof(uint16_t));
    memcpy(&tex_height, &buffer[0x06], sizeof(uint16_t));

    /* glyphes */
    iter = &buffer[0x08];
    mem = sizeof(struct gui_font_glyph) * ((size_t)indexes + 1);
    glyphes = xcalloc(mem, 1);
    for(i = 0; i < num; ++i) {
        uint16_t id, x, y, w, h;
        float xoff, yoff, xadv;

        memcpy(&id, iter, sizeof(uint16_t));
        memcpy(&x, &iter[0x02], sizeof(uint16_t));
        memcpy(&y, &iter[0x04], sizeof(uint16_t));
        memcpy(&w, &iter[0x06], sizeof(uint16_t));
        memcpy(&h, &iter[0x08], sizeof(uint16_t));
        memcpy(&xoff, &iter[10], sizeof(float));
        memcpy(&yoff, &iter[14], sizeof(float));
        memcpy(&xadv, &iter[18], sizeof(float));

        assert(id <= indexes);
        glyphes[id].code = id;
        glyphes[id].width = (short)w;
        glyphes[id].height = (short)h;
        glyphes[id].xoff  = xoff;
        glyphes[id].yoff = yoff;
        glyphes[id].xadvance = xadv;
        glyphes[id].uv[0].u = (float)x/(float)tex_width;
        glyphes[id].uv[0].v = (float)y/(float)tex_height;
        glyphes[id].uv[1].u = (float)(x+w)/(float)tex_width;
        glyphes[id].uv[1].v = (float)(y+h)/(float)tex_height;
        if (glyphes[id].height > max_height) max_height = glyphes[id].height;
        iter += 22;
    }

    /* texture */
    convert.ptr = ldbmp(iter, &img_width, &img_height);
    assert(img_width == tex_width && img_height == tex_height);

    /* font */
    font = xcalloc(sizeof(struct gui_font), 1);
    font->height = height;
    font->scale = (float)height/(float)max_height;
    font->texture = convert.handle;
    font->tex_size.x = tex_width;
    font->tex_size.y = tex_height;
    font->fallback = &glyphes['?'];
    font->glyphes = glyphes;
    font->glyph_count = indexes + 1;
    free(buffer);
    return font;
}

static void
delfont(struct gui_font *font)
{
    if (!font) return;
    if (font->glyphes)
        free(font->glyphes);
    free(font);
}

static void
draw(int width, int height, const struct gui_draw_call_list **list, gui_size count)
{
    gui_size i = 0;
    gui_size n = 0;
    GLint offset = 0;
    const gui_byte *vertexes;
    const struct gui_draw_command *cmd;
    static const size_t v = sizeof(struct gui_vertex);
    static const size_t p = offsetof(struct gui_vertex, pos);
    static const size_t t = offsetof(struct gui_vertex, uv);
    static const size_t c = offsetof(struct gui_vertex, color);

    if (!list) return;
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0f, width, height, 0.0f, 0.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    for (n = 0; n < count; ++n) {
        vertexes = (const gui_char*)list[n]->vertexes;
        glVertexPointer(2, GL_FLOAT, (GLsizei)v, (const void*)(vertexes + p));
        glTexCoordPointer(2, GL_FLOAT, (GLsizei)v, (const void*)(vertexes + t));
        glColorPointer(4, GL_UNSIGNED_BYTE, (GLsizei)v, (const void*)(vertexes + c));

        for (i = 0; i < list[n]->command_size; ++i) {
            int x,y,w,h;
            cmd = &list[n]->commands[i];
            x = (int)cmd->clip_rect.x;
            y = height - (int)(cmd->clip_rect.y + cmd->clip_rect.h);
            w = (int)cmd->clip_rect.w;
            h = (int)cmd->clip_rect.h;
            glScissor(x, y, w, h);
            glBindTexture(GL_TEXTURE_2D, (GLuint)(unsigned long)cmd->texture);
            glDrawArrays(GL_TRIANGLES, offset, (GLsizei)cmd->vertex_count);
            offset += (GLint)cmd->vertex_count;
        }
    }

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

int
main(int argc, char *argv[])
{
    struct XWindow xw;
    struct GUI gui;
    long dt, started;
    const struct gui_draw_call_list *list;
    static const gui_float values[] = {10.0f, 12.5f, 18.0f, 15.0f, 25.0f, 30.0f};
    static GLint att[] = {GLX_RGBA, GLX_DEPTH_SIZE,24, GLX_DOUBLEBUFFER, None};
    static const char *sel[] = {"item0", "item1", "item2", "item3", "item4", "item5"};
    static const char *select[] = {"Fist", "Pistol", "Shotgun", "Railgun", "BFG"};

    /* Window */
    UNUSED(argc); UNUSED(argv);
    memset(&xw, 0, sizeof xw);
    memset(&gui, 0, sizeof gui);
    xw.dpy = XOpenDisplay(NULL);
    if (!xw.dpy) die("XOpenDisplay failed\n");
    xw.root = DefaultRootWindow(xw.dpy);
    xw.vi = glXChooseVisual(xw.dpy, 0, att);
    if (!xw.vi) die("Failed to find appropriate visual\n");
    xw.cmap = XCreateColormap(xw.dpy,xw.root,xw.vi->visual,AllocNone);
    xw.swa.colormap = xw.cmap;
    xw.swa.event_mask =
        ExposureMask | KeyPressMask |KeyReleaseMask | ButtonPress |
        ButtonReleaseMask | ButtonMotionMask | PointerMotionMask |
        Button1MotionMask | Button2MotionMask | Button3MotionMask;
    xw.win = XCreateWindow(
        xw.dpy, xw.root, 0, 0,WIN_WIDTH,WIN_HEIGHT, 0,
        xw.vi->depth, InputOutput, xw.vi->visual,
        CWColormap | CWEventMask, &xw.swa);
    XGetWindowAttributes(xw.dpy, xw.win, &xw.gwa);
    XStoreName(xw.dpy, xw.win, "Demo");
    XMapWindow(xw.dpy, xw.win);
    XFlush(xw.dpy);
    XSync(xw.dpy, False);

    /* OpenGL */
    xw.glc = glXCreateContext(xw.dpy, xw.vi, NULL, GL_TRUE);
    glXMakeCurrent(xw.dpy, xw.win, xw.glc);

    gui.win = &xw;
    gui.option = 0;
    gui.select = 0;
    gui.prog = 60;
    gui.vprog = 70;
    gui.vprog2 = 30;
    gui.vprog3 = 50;
    gui.spinner = 100;
    gui.slider = 5.0f;

    /* GUI */
    gui.font = ldfont("mono.sdf", 16);
    gui.main.memory = xcalloc(MAX_BUFFER, 1);
    gui.main.clip_size = MAX_BUFFER / 32;
    gui.main.command_size = MAX_BUFFER / 6;
    gui.main.vertex_size = MAX_BUFFER - gui.main.clip_size - gui.main.command_size;
    gui_default_config(&gui.config);
    gui_panel_init(&gui.panel, &gui.config, gui.font, &gui.in);

    xw.running = 1;
    while (xw.running) {
        XEvent ev;
        started = timestamp();
        gui_input_begin(&gui.in);
        while (XCheckWindowEvent(xw.dpy, xw.win, xw.swa.event_mask, &ev)) {
            if (ev.type == Expose||ev.type == ConfigureNotify) resize(&gui, &ev);
            else if (ev.type == MotionNotify) bmotion(&gui, &ev);
            else if (ev.type == ButtonPress) bpress(&gui, &ev);
            else if (ev.type == ButtonRelease) brelease(&gui, &ev);
            else if (ev.type == KeyPress) kpress(&gui, &ev);
            else if (ev.type == KeyRelease) krelease(&gui, &ev);
        }
        gui_input_end(&gui.in);

        /* ------------------------- GUI --------------------------*/
        gui_begin(&gui.out, &gui.main);
        xw.running = gui_panel_begin(&gui.panel, &gui.out, "Demo", 20, 20, 200, 400,
            GUI_PANEL_HEADER|GUI_PANEL_CLOSEABLE|GUI_PANEL_MINIMIZABLE|GUI_PANEL_SCROLLBAR);
        gui_panel_layout(&gui.panel, 30, 1);
        if (gui_panel_button_text(&gui.panel, "button", 6, GUI_BUTTON_SWITCH))
            fprintf(stdout, "button pressed!\n");
        gui.slider = gui_panel_slider(&gui.panel, 0.0f, gui.slider, 10.0f, 1.0f, GUI_HORIZONTAL);
        gui.prog = gui_panel_progress(&gui.panel, gui.prog, 100, gui_true, GUI_HORIZONTAL);
        gui.check = gui_panel_check(&gui.panel, "advanced", 8, gui.check);
        gui_panel_layout(&gui.panel, 30, 2);
        if (gui_panel_option(&gui.panel, "easy", 4, gui.option == 0)) gui.option = 0;
        if (gui_panel_option(&gui.panel, "hard", 4, gui.option == 1)) gui.option = 1;
        gui_panel_layout(&gui.panel, 30, 1);
        gui.typing = gui_panel_input(&gui.panel, gui.input_text, &gui.input_len, INPUT_MAX,
                                        GUI_INPUT_DEFAULT, gui.typing);
        gui.submit = gui_panel_command(&gui.panel, gui.cmd_input, &gui.cmd_len, INPUT_MAX,
                                        &gui.submitting);
        gui.spinning = gui_panel_spinner(&gui.panel, 0, &gui.spinner, 250, 10, gui.spinning);
        gui.select = gui_panel_selector(&gui.panel, select, LEN(select), gui.select);
        gui_panel_layout(&gui.panel, 100, 3);
        gui.vprog = gui_panel_progress(&gui.panel, gui.vprog, 100, gui_true, GUI_VERTICAL);
        gui.vprog2 = gui_panel_progress(&gui.panel, gui.vprog2, 100, gui_true, GUI_VERTICAL);
        gui.vprog3 = gui_panel_progress(&gui.panel, gui.vprog3, 100, gui_true, GUI_VERTICAL);
        gui_panel_layout(&gui.panel, 100, 1);
        gui.seloff = gui_panel_list(&gui.panel, gui.selection, sel, LEN(sel), gui.seloff, 30);
        gui_panel_histo(&gui.panel, values, LEN(values));
        gui_panel_plot(&gui.panel, values, LEN(values));
        gui_panel_text_box(&gui.panel, gui.box_input, &gui.box_len, &gui.box_off);
        gui_panel_end(&gui.panel);
        gui_end(&gui.out, &gui.draw_list, &gui.status);
        /* ---------------------------------------------------------*/

        /* Draw */
        glClearColor(120.0f/255.0f, 120.0f/255.0f, 120.0f/255.0f, 120.0f/255.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        list = &gui.draw_list;
        draw(xw.gwa.width, xw.gwa.height, &list, 1);
        glXSwapBuffers(xw.dpy, xw.win);

        /* Timing */
        dt = timestamp() - started;
        if (dt < DTIME)
            sleep_for(DTIME - dt);
    }

    /* Cleanup */
    free(gui.main.memory);
    delfont(gui.font);
    XFree(xw.vi);
    glXMakeCurrent(xw.dpy, None, NULL);
    glXDestroyContext(xw.dpy, xw.glc);
    XDestroyWindow(xw.dpy, xw.win);
    XCloseDisplay(xw.dpy);
    return 0;
}

