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

#define MIN(a,b)((a) < (b) ? (a) : (b))
#define MAX(a,b)((a) < (b) ? (b) : (a))
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#define LEN(a)(sizeof(a)/sizeof(a)[0])
#define UNUSED(a)((void)(a))
#define glerror() glerror_(__FILE__, __LINE__)

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
    struct gui_draw_list out;
    struct gui_input input;
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

static struct gui_font *ldfont(const char*, unsigned char);
static void delfont(struct gui_font *font);
static void draw(struct GUI*, int, int , const struct gui_draw_list*);

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
kpress(struct GUI *con, XEvent* e)
{
    int ret;
    struct XWindow *xw = con->win;
    KeySym *keysym = XGetKeyboardMapping(xw->dpy, e->xkey.keycode, 1, &ret);
    if (*keysym == XK_Escape) xw->running = 0;
    else if (*keysym == XK_Shift_L || *keysym == XK_Shift_R)
        gui_input_key(&con->input, GUI_KEY_SHIFT, gui_true);
    else if (*keysym == XK_Control_L || *keysym == XK_Control_L)
        gui_input_key(&con->input, GUI_KEY_CTRL, gui_true);
    else if (*keysym == XK_Delete)
        gui_input_key(&con->input, GUI_KEY_DEL, gui_true);
    else if (*keysym == XK_Return)
        gui_input_key(&con->input, GUI_KEY_ENTER, gui_true);
    else if (*keysym == XK_BackSpace)
        gui_input_key(&con->input, GUI_KEY_BACKSPACE, gui_true);
}

static void
krelease(struct GUI *con, XEvent* e)
{
    int ret;
    struct XWindow *xw = con->win;
    KeySym *keysym = XGetKeyboardMapping(xw->dpy, e->xkey.keycode, 1, &ret);
    if (*keysym == XK_Shift_L || *keysym == XK_Shift_R)
        gui_input_key(&con->input, GUI_KEY_SHIFT, gui_false);
    else if (*keysym == XK_Control_L || *keysym == XK_Control_L)
        gui_input_key(&con->input, GUI_KEY_CTRL, gui_false);
    else if (*keysym == XK_Delete)
        gui_input_key(&con->input, GUI_KEY_DEL, gui_false);
    else if (*keysym == XK_Return)
        gui_input_key(&con->input, GUI_KEY_ENTER, gui_false);
    else if (*keysym == XK_BackSpace)
        gui_input_key(&con->input, GUI_KEY_BACKSPACE, gui_false);
}

static void
bpress(struct GUI *con, XEvent *evt)
{
    const float x = evt->xbutton.x;
    const float y = evt->xbutton.y;
    if (evt->xbutton.button == Button1)
        gui_input_button(&con->input, x, y, gui_true);
}

static void
brelease(struct GUI *con, XEvent *evt)
{
    const float x = evt->xbutton.x;
    const float y = evt->xbutton.y;
    struct XWindow *xw = con->win;
    if (evt->xbutton.button == Button1)
        gui_input_button(&con->input, x, y, gui_false);
}

static void
bmotion(struct GUI *con, XEvent *evt)
{
    const gui_int x = evt->xbutton.x;
    const gui_int y = evt->xbutton.y;
    struct XWindow *xw = con->win;
    gui_input_motion(&con->input, x, y);
}

static void
resize(struct GUI *con, XEvent* evt)
{
    struct XWindow *xw = con->win;
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

static void
glerror_(const char *file, int line)
{
    const GLenum code = glGetError();
    if (code == GL_INVALID_ENUM)
        fprintf(stdout, "[GL] Error: (%s:%d) invalid value!\n", file, line);
    else if (code == GL_INVALID_OPERATION)
        fprintf(stdout, "[GL] Error: (%s:%d) invalid operation!\n", file, line);
    else if (code == GL_INVALID_FRAMEBUFFER_OPERATION)
        fprintf(stdout, "[GL] Error: (%s:%d) invalid frame op!\n", file, line);
    else if (code == GL_OUT_OF_MEMORY)
        fprintf(stdout, "[GL] Error: (%s:%d) out of memory!\n", file, line);
    else if (code == GL_STACK_UNDERFLOW)
        fprintf(stdout, "[GL] Error: (%s:%d) stack underflow!\n", file, line);
    else if (code == GL_STACK_OVERFLOW)
        fprintf(stdout, "[GL] Error: (%s:%d) stack overflow!\n", file, line);
}

static struct gui_font*
ldfont(const char *name, unsigned char height)
{
    union conversion {
        gui_texture handle;
        uintptr_t ptr;
    } convert;
    GLuint texture;
    size_t size;
    struct gui_font *font;
    short i = 0;
    uint32_t bpp;
    short max_height = 0;
    uint32_t ioff;

    /* header */
    unsigned char *buffer = (unsigned char*)ldfile(name, O_RDONLY, &size);
    uint16_t num = *(uint16_t*)buffer;
    uint16_t indexes = *(uint16_t*)&buffer[0x02] + 1;
    uint16_t tex_width = *(uint16_t*)&buffer[0x04];
    uint16_t tex_height = *(uint16_t*)&buffer[0x06];

    /* glyphes */
    unsigned char *header;
    unsigned char *data;
    unsigned char *iter = &buffer[0x08];
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
        glyphes[id].uv[0].u = (gui_float)x/(gui_float)tex_width;
        glyphes[id].uv[0].v = (gui_float)(y+h)/(gui_float)tex_height;
        glyphes[id].uv[1].u = (gui_float)(x+w)/(gui_float)tex_width;
        glyphes[id].uv[1].v = (gui_float)y/(gui_float)tex_height;
        if (glyphes[id].height > max_height) max_height = glyphes[id].height;
        iter += 22;
    }

    /* texture */
    header = iter;
    assert(header[0] == 'B');
    assert(header[1] == 'M');
    ioff = *(uint32_t*)(&header[0x0A]);

    data = iter + ioff;
    glGenTextures(1, &texture);
    convert.ptr = texture;
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, tex_width, tex_height, 0,
                GL_BGRA, GL_UNSIGNED_BYTE, data);
    glerror();
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

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
draw(struct GUI *con, int width, int height, const struct gui_draw_list *list)
{
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

    cmd = list->begin;
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
        cmd = gui_next(list, cmd);
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
    gui_size buffer_size = MAX_VERTEX_BUFFER;
    const struct gui_color colorA = {100, 100, 100, 255};
    const struct gui_color colorB = {45, 45, 45, 255};
    const struct gui_color colorC = {0, 0, 0, 255};
    static GLint att[] = {GLX_RGBA, GLX_DEPTH_SIZE,24, GLX_DOUBLEBUFFER, None};
    GLenum err;

    gui_float slider = 5.0f;
    gui_float prog = 60.0f;
    gui_float offset = 300;

    /* x11 */
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
    buffer = xcalloc(MAX_VERTEX_BUFFER, 1);

    xw.running = 1;
    gui.win = &xw;
    gui.font = ldfont("mono.font", 12);
    while (xw.running) {
        XEvent ev;
        started = timestamp();
        gui_input_begin(&gui.input);
        while (XCheckWindowEvent(xw.dpy, xw.win, xw.swa.event_mask, &ev)) {
            if (ev.type == Expose||ev.type == ConfigureNotify) resize(&gui, &ev);
            else if (ev.type == MotionNotify) bmotion(&gui, &ev);
            else if (ev.type == ButtonPress) bpress(&gui, &ev);
            else if (ev.type == ButtonRelease) brelease(&gui, &ev);
            else if (ev.type == KeyPress) kpress(&gui, &ev);
            else if (ev.type == KeyRelease) krelease(&gui, &ev);
        }
        gui_input_end(&gui.input);

        /* ------------------------- GUI --------------------------*/
        gui_begin(&gui.out, buffer, MAX_VERTEX_BUFFER);
        if (gui_button(&gui.out, &gui.input, gui.font, colorA, colorC, 50,50,150,30,5,"button",6))
            fprintf(stdout, "Button pressed!\n");
        slider = gui_slider(&gui.out, &gui.input, colorA, colorB,
                            50, 100, 150, 30, 2, 0.0f, slider, 10.0f, 1.0f);
        prog = gui_progress(&gui.out, &gui.input, colorA, colorB,
                            50, 150, 150, 30, 2, prog, 100.0f, gui_true);
        offset = gui_scroll(&gui.out, &gui.input, colorA, colorB,
                            250, 50, 16, 300, offset, 600);
        gui_end(&gui.out);
        /* ---------------------------------------------------------*/

        /* Draw */
        glClearColor(45.0f/255.0f,45.0f/255.0f,45.0f/255.0f,1);
        glClear(GL_COLOR_BUFFER_BIT);
        draw(&gui, xw.gwa.width, xw.gwa.height, &gui.out);
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

