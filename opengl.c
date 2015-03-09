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
#define MAX_VERTEX_BUFFER (16 * 1024)
#define INPUT_MAX 64

#define MIN(a,b)((a) < (b) ? (a) : (b))
#define MAX(a,b)((a) < (b) ? (b) : (a))
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#define LEN(a)(sizeof(a)/sizeof(a)[0])
#define UNUSED(a)((void)(a))

/* types  */
struct XWindow {
    Display *dpy;
    Window root;
    XVisualInfo *vi;
    Colormap cmap;
    XSetWindowAttributes swa;
    XWindowAttributes gwa;
    Window win;
    GLXContext glc;
    int running;
};

struct GUI {
    struct XWindow *win;
    struct gui_draw_queue out;
    struct gui_input in;
    struct gui_font *font;
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
static void delfont(struct gui_font *font);
static void draw(int, int, const struct gui_draw_queue*);

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
    KeySym *keysym = XGetKeyboardMapping(xw->dpy, e->xkey.keycode, 1, &ret);
    if (*keysym == XK_Escape) xw->running = 0;
    else if (*keysym == XK_Shift_L || *keysym == XK_Shift_R)
        gui_input_key(&gui->in, GUI_KEY_SHIFT, gui_true);
    else if (*keysym == XK_Control_L || *keysym == XK_Control_L)
        gui_input_key(&gui->in, GUI_KEY_CTRL, gui_true);
    else if (*keysym == XK_Delete)
        gui_input_key(&gui->in, GUI_KEY_DEL, gui_true);
    else if (*keysym == XK_Return)
        gui_input_key(&gui->in, GUI_KEY_ENTER, gui_true);
    else if (*keysym == XK_BackSpace)
        gui_input_key(&gui->in, GUI_KEY_BACKSPACE, gui_true);
    else if ((*keysym >= 'a' && *keysym <= 'z') ||
            (*keysym >= '0' && *keysym <= '9'))
        gui_input_char(&gui->in, (unsigned char*)keysym);
}

static void
krelease(struct GUI *gui, XEvent* e)
{
    int ret;
    struct XWindow *xw = gui->win;
    KeySym *keysym = XGetKeyboardMapping(xw->dpy, e->xkey.keycode, 1, &ret);
    if (*keysym == XK_Shift_L || *keysym == XK_Shift_R)
        gui_input_key(&gui->in, GUI_KEY_SHIFT, gui_false);
    else if (*keysym == XK_Control_L || *keysym == XK_Control_L)
        gui_input_key(&gui->in, GUI_KEY_CTRL, gui_false);
    else if (*keysym == XK_Delete)
        gui_input_key(&gui->in, GUI_KEY_DEL, gui_false);
    else if (*keysym == XK_Return)
        gui_input_key(&gui->in, GUI_KEY_ENTER, gui_false);
    else if (*keysym == XK_BackSpace)
        gui_input_key(&gui->in, GUI_KEY_BACKSPACE, gui_false);
}

static void
bpress(struct GUI *gui, XEvent *evt)
{
    const float x = evt->xbutton.x;
    const float y = evt->xbutton.y;
    if (evt->xbutton.button == Button1)
        gui_input_button(&gui->in, x, y, gui_true);
}

static void
brelease(struct GUI *con, XEvent *evt)
{
    const float x = evt->xbutton.x;
    const float y = evt->xbutton.y;
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
    *siz = status.st_size;
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

    *width = *(uint32_t*)&(header[0x12]);
    *height = *(uint32_t*)&(header[0x12]);
    ioff = *(uint32_t*)(&header[0x0A]);
    if (*width <= 0 || *height <= 0)
        die("[BMP]: invalid image size");

    data = data + ioff;
    reader = data;
    mem = *width * *height * 4;
    target = xcalloc(mem, 1);
    for (i = *height-1; i >= 0; i--) {
        writer = target + (i * *width * 4);
        for (j = 0; j < *width; j++) {
            *writer++ = *(reader + (j * 4) + 1);
            *writer++ = *(reader + (j * 4) + 2);
            *writer++ = *(reader + (j * 4) + 3);
            *writer++ = *(reader + (j * 4) + 0);
            *writer += 4;
        }
        reader += *width * 4;
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, *width, *height, 0,
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

    /* header */
    gui_byte *buffer = (gui_byte*)ldfile(name, O_RDONLY, &size);
    uint16_t num = *(uint16_t*)buffer;
    uint16_t indexes = *(uint16_t*)&buffer[0x02] + 1;
    uint16_t tex_width = *(uint16_t*)&buffer[0x04];
    uint16_t tex_height = *(uint16_t*)&buffer[0x06];

    /* glyphes */
    gui_byte *iter = &buffer[0x08];
    size_t mem = sizeof(struct gui_font_glyph) * indexes;
    struct gui_font_glyph *glyphes = xcalloc(mem, 1);
    for(i = 0; i < num; ++i) {
        short id = *(uint16_t*)&iter[0];
        short x = *(uint16_t*)&iter[2];
        short y = *(uint16_t*)&iter[4];
        short w = *(uint16_t*)&iter[6];
        short h = *(uint16_t*)&iter[8];

        glyphes[id].code = id;
        glyphes[id].width = w;
        glyphes[id].height = h;
        glyphes[id].xoff  = *(float*)&iter[10];
        glyphes[id].yoff = *(float*)&iter[14];
        glyphes[id].xadvance =  *(float*)&iter[18];
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
    font->glyph_count = indexes;
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
draw(int width, int height, const struct gui_draw_queue *que)
{
    const struct gui_draw_command *cmd;
    static const size_t v = sizeof(struct gui_vertex);
    static const size_t p = offsetof(struct gui_vertex, pos);
    static const size_t t = offsetof(struct gui_vertex, uv);
    static const size_t c = offsetof(struct gui_vertex, color);

    if (!que) return;
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

    cmd = que->begin;
    while (cmd) {
        const int x = (int)cmd->clip_rect.x;
        const int y = height - (int)(cmd->clip_rect.y + cmd->clip_rect.h);
        const int w = (int)cmd->clip_rect.w;
        const int h = (int)cmd->clip_rect.h;
        gui_byte *buffer = (gui_byte*)cmd->vertexes;
        glVertexPointer(2, GL_FLOAT, v, (void*)(buffer + p));
        glTexCoordPointer(2, GL_FLOAT, v, (void*)(buffer + t));
        glColorPointer(4, GL_UNSIGNED_BYTE, v, (void*)(buffer + c));
        glBindTexture(GL_TEXTURE_2D, (unsigned long)cmd->texture);
        glScissor(x, y, w, h);
        glDrawArrays(GL_TRIANGLES, 0, cmd->vertex_count);
        cmd = gui_next(que, cmd);
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
    gui_byte *buffer;
    const gui_size buffer_size = MAX_VERTEX_BUFFER;
    const struct gui_color colorA = {100, 100, 100, 255};
    const struct gui_color colorB = {45, 45, 45, 255};
    const struct gui_color colorC = {255, 255, 255, 255};
    const struct gui_color colorD = {255, 0, 0, 255};
    static GLint att[] = {GLX_RGBA, GLX_DEPTH_SIZE,24, GLX_DOUBLEBUFFER, None};

    struct gui_button button;
    struct gui_slider slider;
    struct gui_progress prog;
    struct gui_toggle toggle;
    struct gui_input_field field;
    struct gui_plot plot;
    struct gui_histo histo;
    struct gui_scroll scroll;

    const char *selection[] = {"Inactive", "Active"};
    gui_char input[INPUT_MAX];
    gui_int input_len = 0;
    const gui_float values[] = {10.0f, 12.0f, 25.0f, 15.0f, 18.0f, 30.0f, 32.0f, 15.0f};
    strcpy((char*)input, "input");
    input_len = strlen((char*)input);

    button.x = 50, button.y = 50;
    button.w = 150, button.h = 30;
    button.pad_x = 5, button.pad_y = 5;
    button.text = "button";
    button.length = 6;
    button.background = colorB;
    button.foreground = colorA;
    button.font = colorC;
    button.highlight = colorA;

    slider.x = 50, slider.y = 100;
    slider.w = 150, slider.h = 30;
    slider.pad_x = 2, slider.pad_y = 2;
    slider.min = 0.0f;
    slider.value = 5.0f;
    slider.max = 10.0f;
    slider.step = 1.0f;
    slider.foreground = colorB;
    slider.background = colorA;

    prog.x = 50, prog.y = 150;
    prog.w = 150, prog.h = 30;
    prog.pad_x = 2, prog.pad_y = 2;
    prog.current = 60.0f;
    prog.max = 100.0f;
    prog.modifyable = gui_true;
    prog.foreground = colorB;
    prog.background = colorA;

    toggle.x = 50, toggle.y = 200;
    toggle.w = 150, toggle.h = 30;
    toggle.pad_x = 2, toggle.pad_y = 2;
    toggle.active = gui_false;
    toggle.text = selection[toggle.active];
    toggle.length = strlen(selection[toggle.active]);
    toggle.foreground = colorB;
    toggle.background = colorA;
    toggle.font = colorC;
    toggle.highlight = colorA;

    field.x = 50, field.y = 250;
    field.w = 150, field.h = 30;
    field.pad_x = 5, field.pad_y = 5;
    field.buffer = input;
    field.length = &input_len;
    field.max = INPUT_MAX;
    field.active = gui_false;
    field.foreground = colorA;
    field.background = colorB;
    field.font = colorC;

    plot.x = 50, plot.y = 300;
    plot.w = 150, plot.h = 100;
    plot.pad_x = 5, plot.pad_y = 5;
    plot.values = values;
    plot.value_count = LEN(values);
    plot.foreground = colorB;
    plot.background = colorA;

    histo.x = 50, histo.y = 430;
    histo.w = 150, histo.h = 100;
    histo.pad_x = 5, histo.pad_y = 5;
    histo.values = values;
    histo.value_count = LEN(values);
    histo.foreground = colorB;
    histo.background = colorA;
    histo.negative = colorC;
    histo.highlight = colorD;

    scroll.x = 250, scroll.y = 50;
    scroll.w = 16, scroll.h = 300;
    scroll.offset = 300;
    scroll.target = 600;
    scroll.step = 150;
    scroll.foreground = colorB;
    scroll.background = colorA;

    /* Window */
    UNUSED(argc); UNUSED(argv);
    memset(&xw, 0, sizeof xw);
    memset(&gui, 0, sizeof gui);
    xw.dpy = XOpenDisplay(NULL);
    if (!xw.dpy)
        die("XOpenDisplay failed\n");
    xw.root = DefaultRootWindow(xw.dpy);
    xw.vi = glXChooseVisual(xw.dpy, 0, att);
    if (!xw.vi)
        die("Failed to find appropriate visual\n");
    xw.cmap = XCreateColormap(xw.dpy,xw.root,xw.vi->visual,AllocNone);
    xw.swa.colormap = xw.cmap;
    xw.swa.event_mask =
        ExposureMask | KeyPressMask | ButtonPress |
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
    buffer = xcalloc(buffer_size, 1);

    xw.running = 1;
    gui.win = &xw;
    gui.font = ldfont("mono.font", 16);
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
        gui_begin(&gui.out, buffer, MAX_VERTEX_BUFFER);
        if (gui_button(&gui.out, &button, gui.font, &gui.in))
            fprintf(stdout, "button pressed!\n");
        slider.value = gui_slider(&gui.out, &slider, &gui.in);
        prog.current = gui_progress(&gui.out, &prog, &gui.in);
        toggle.active = gui_toggle(&gui.out, &toggle, gui.font, &gui.in);
        toggle.text = selection[toggle.active];
        field.active = gui_input(&gui.out, &field, gui.font, &gui.in);
        gui_plot(&gui.out, &plot);
        gui_histo(&gui.out, &histo, &gui.in);
        scroll.offset = gui_scroll(&gui.out, &scroll, &gui.in);
        gui_end(&gui.out);
        /* ---------------------------------------------------------*/

        /* Draw */
        glClearColor(45.0f/255.0f,45.0f/255.0f,45.0f/255.0f,1);
        glClear(GL_COLOR_BUFFER_BIT);
        draw(xw.gwa.width, xw.gwa.height, &gui.out);
        glXSwapBuffers(xw.dpy, xw.win);

        /* Timing */
        dt = timestamp() - started;
        if (dt < DTIME) {
            sleep_for(DTIME - dt);
            dt = DTIME;
        }
    }

    /* Cleanup */
    delfont(gui.font);
    glXMakeCurrent(xw.dpy, None, NULL);
    glXDestroyContext(xw.dpy, xw.glc);
    XDestroyWindow(xw.dpy, xw.win);
    XCloseDisplay(xw.dpy);
    return 0;
}

