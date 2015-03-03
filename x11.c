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
#define internal static
#define global static
#define persistent static

#define WIN_WIDTH   800
#define WIN_HEIGHT  600
#define DTIME       33
#define MAX_VERTEX_BUFFER (16 * 1024)

#define CONSOLE_MAX_CMDLINE 1024
#define CONSOLE_MAX_TEXT (16 * 1024)

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

struct Console {
    struct XWindow *win;
    struct gui_draw_list gui;
    struct gui_input input;
    struct gui_font font;
    /* text */
    int output_len;
    char output[CONSOLE_MAX_TEXT];
    char line[CONSOLE_MAX_CMDLINE];
    int line_len;
};

/* functions */
internal void die(const char*,...);
internal long timestamp(void);
internal void sleep_for(long ms);

internal void kpress(struct Console*, XEvent*);
internal void bpress(struct Console*, XEvent*);
internal void brelease(struct Console*, XEvent*);
internal void bmotion(struct Console*, XEvent*);
internal void resize(struct Console*, XEvent*);

internal void
die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

internal void*
xcalloc(size_t nmemb, size_t size)
{
    void *p = calloc(nmemb, size);
    if (!p)
        die("out of memory\n");
    return p;
}

internal long
timestamp(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0) return 0;
    return (long)((long)tv.tv_sec * 1000 + (long)tv.tv_usec/1000);
}

internal void
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
kpress(struct Console *con, XEvent* e)
{
    int ret;
    struct XWindow *xw = con->win;
    KeySym *keysym = XGetKeyboardMapping(xw->dpy, e->xkey.keycode, 1, &ret);
    if (*keysym == XK_Escape) xw->running = 0;
    else if (*keysym == XK_Shift_L || *keysym == XK_Shift_R) gui_input_key(&con->input, GUI_KEY_SHIFT, gui_true);
    else if (*keysym == XK_Control_L || *keysym == XK_Control_L) gui_input_key(&con->input, GUI_KEY_CTRL, gui_true);
    else if (*keysym == XK_Delete) gui_input_key(&con->input, GUI_KEY_DEL, gui_true);
    else if (*keysym == XK_Return) gui_input_key(&con->input, GUI_KEY_ENTER, gui_true);
    else if (*keysym == XK_BackSpace) gui_input_key(&con->input, GUI_KEY_BACKSPACE, gui_true);
}

static void
krelease(struct Console *con, XEvent* e)
{
    int ret;
    struct XWindow *xw = con->win;
    KeySym *keysym = XGetKeyboardMapping(xw->dpy, e->xkey.keycode, 1, &ret);
    if (*keysym == XK_Shift_L || *keysym == XK_Shift_R) gui_input_key(&con->input, GUI_KEY_SHIFT, gui_false);
    else if (*keysym == XK_Control_L || *keysym == XK_Control_L) gui_input_key(&con->input, GUI_KEY_CTRL, gui_false);
    else if (*keysym == XK_Delete) gui_input_key(&con->input, GUI_KEY_DEL, gui_false);
    else if (*keysym == XK_Return) gui_input_key(&con->input, GUI_KEY_ENTER, gui_false);
    else if (*keysym == XK_BackSpace) gui_input_key(&con->input, GUI_KEY_BACKSPACE, gui_false);
}

static void
bpress(struct Console *con, XEvent *evt)
{
    const float x = evt->xbutton.x;
    const float y = evt->xbutton.y;
    if (evt->xbutton.button == Button1)
        gui_input_button(&con->input, x, y, gui_true);
}

static void
brelease(struct Console *con, XEvent *evt)
{
    const float x = evt->xbutton.x;
    const float y = evt->xbutton.y;
    struct XWindow *xw = con->win;
    if (evt->xbutton.button == Button1)
        gui_input_button(&con->input, x, y, gui_false);
}

static void
bmotion(struct Console *con, XEvent *evt)
{
    const float x = evt->xbutton.x;
    const float y = evt->xbutton.y;
    struct XWindow *xw = con->win;
    gui_input_motion(&con->input, x, y);
}

static void
resize(struct Console *con, XEvent* evt)
{
    struct XWindow *xw = con->win;
    XGetWindowAttributes(xw->dpy, xw->win, &xw->gwa);
    glViewport(0, 0, xw->gwa.width, xw->gwa.height);
}

static void
draw(int width, int height, const struct gui_draw_list *list)
{
    const struct gui_draw_command *cmd;
    persistent const size_t v = sizeof(struct gui_vertex);
    persistent const size_t p = offsetof(struct gui_vertex, pos);
    persistent const size_t t = offsetof(struct gui_vertex, uv);
    persistent const size_t c = offsetof(struct gui_vertex, color);

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
    struct Console con;
    long dt, started;
    gui_byte *buffer;
    gui_size buffer_size = MAX_VERTEX_BUFFER;
    const struct gui_color colorA = {100, 100, 100, 255};
    const struct gui_color colorB = {45, 45, 45, 255};
    const struct gui_color colorC = {0, 0, 0, 255};
    static GLint att[] = {GLX_RGBA, GLX_DEPTH_SIZE,24, GLX_DOUBLEBUFFER, None};

    gui_float slider = 5.0f;
    gui_float prog = 60.0f;
    gui_float offset = 300;

    /* x11 */
    UNUSED(argc); UNUSED(argv);
    memset(&xw, 0, sizeof xw);
    memset(&con, 0, sizeof con);
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
        ButtonReleaseMask | ButtonMotionMask |
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
    con.win = &xw;

    /* OpenGL */
    xw.glc = glXCreateContext(xw.dpy, xw.vi, NULL, GL_TRUE);
    glXMakeCurrent(xw.dpy, xw.win, xw.glc);
    buffer = xcalloc(MAX_VERTEX_BUFFER, 1);

    xw.running = 1;
    while (xw.running) {
        /* Input */
        XEvent ev;
        started = timestamp();
        gui_input_begin(&con.input);
        while (XCheckWindowEvent(xw.dpy, xw.win, xw.swa.event_mask, &ev)) {
            if (ev.type == Expose || ev.type == ConfigureNotify) resize(&con, &ev);
            else if (ev.type == MotionNotify) bmotion(&con, &ev);
            else if (ev.type == ButtonPress) bpress(&con, &ev);
            else if (ev.type == ButtonRelease) brelease(&con, &ev);
            else if (ev.type == KeyPress) kpress(&con, &ev);
            else if (ev.type == KeyRelease) kpress(&con, &ev);
        }
        gui_input_end(&con.input);

        /* ------------------------- GUI --------------------------*/
        gui_begin(&con.gui, buffer, MAX_VERTEX_BUFFER);
        if (gui_button(&con.gui, &con.input, colorA, colorC, 50,50,150,30,5,"",0))
            fprintf(stdout, "Button pressed!\n");
        slider = gui_slider(&con.gui, &con.input, colorA, colorB, 50,100,150,30,2, 0.0f, slider, 10.0f, 1.0f);
        prog = gui_progress(&con.gui, &con.input, colorA, colorB, 50,150,150,30,2, prog, 100.0f, gui_true);
        offset = gui_scroll(&con.gui, &con.input, colorA, colorB, 250,50,16,332, offset, 600);
        gui_end(&con.gui);
        /* ---------------------------------------------------------*/

        /* Draw */
        glClearColor(45.0f/255.0f,45.0f/255.0f,45.0f/255.0f,1);
        glClear(GL_COLOR_BUFFER_BIT);
        draw(xw.gwa.width, xw.gwa.height, &con.gui);
        glXSwapBuffers(xw.dpy, xw.win);

        /* Timing */
        dt = timestamp() - started;
        if (dt < DTIME) {
            sleep_for(DTIME - dt);
            dt = DTIME;
        }
    }

    /* Cleanup */
    glXMakeCurrent(xw.dpy, None, NULL);
    glXDestroyContext(xw.dpy, xw.glc);
    XDestroyWindow(xw.dpy, xw.win);
    XCloseDisplay(xw.dpy);
    return 0;
}

