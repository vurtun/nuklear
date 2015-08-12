/*
    Copyright (c) 2015
    vurtun <polygone@gmx.net>
    MIT licence
 */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/* macros */
#define DTIME       16
#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#define MAX(a,b)    ((a) < (b) ? (b) : (a))
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#define LEN(a)      (sizeof(a)/sizeof(a)[0])
#define UNUSED(a)   ((void)(a))

#include "../gui.h"

static void clipboard_set(const char *text){UNUSED(text);}
static gui_bool clipboard_is_filled(void){return gui_false;}
static const char* clipboard_get(void) {return NULL;}

#include "demo.c"

typedef struct XFont XFont;
typedef struct XSurface XSurface;
typedef struct XWindow XWindow;

struct XFont {
    int ascent;
    int descent;
    int height;
    XFontSet set;
    XFontStruct *xfont;
};

struct XSurface {
    GC gc;
    Display *dpy;
    int screen;
    Window root;
    Drawable drawable;
    unsigned int w, h;
};

struct XWindow {
    Display *dpy;
    Window root;
    Visual *vis;
    XFont *font;
    XSurface *surf;
    Colormap cmap;
    XWindowAttributes attr;
    XSetWindowAttributes swa;
    Window win;
    int screen;
    unsigned int width;
    unsigned int height;
};

static void
die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputs("\n", stderr);
    exit(EXIT_FAILURE);
}

static void*
xcalloc(size_t siz, size_t n)
{
    void *ptr = calloc(siz, n);
    if (!ptr) die("Out of memory\n");
    return ptr;
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
    struct timespec req;
    const time_t sec = (int)(t/1000);
    const long ms = t - (sec * 1000);
    req.tv_sec = sec;
    req.tv_nsec = ms * 1000000L;
    while(-1 == nanosleep(&req, &req));
}

static XFont*
font_create(Display *dpy, const char *name)
{
    int n;
    char *def, **missing;
    XFont *font = (XFont*)xcalloc(1, sizeof(XFont));
    font->set = XCreateFontSet(dpy, name, &missing, &n, &def);
    if(missing) {
        while(n--)
            fprintf(stderr, "missing fontset: %s\n", missing[n]);
        XFreeStringList(missing);
    }

    if(font->set) {
        XFontStruct **xfonts;
        char **font_names;
        XExtentsOfFontSet(font->set);
        n = XFontsOfFontSet(font->set, &xfonts, &font_names);
        while(n--) {
            font->ascent = MAX(font->ascent, (*xfonts)->ascent);
            font->descent = MAX(font->descent,(*xfonts)->descent);
            xfonts++;
        }
    } else {
        if(!(font->xfont = XLoadQueryFont(dpy, name))
        && !(font->xfont = XLoadQueryFont(dpy, "fixed")))
            die("error, cannot load font: '%s'\n", name);
        font->ascent = font->xfont->ascent;
        font->descent = font->xfont->descent;
    }
    font->height = font->ascent + font->descent;
    return font;
}

static gui_size
font_get_text_width(gui_handle handle, const gui_char *text, gui_size len)
{
    XFont *font = (XFont*)handle.ptr;
    XRectangle r;
    gui_size width;
    if(!font || !text)
        return 0;

    if(font->set) {
        XmbTextExtents(font->set, (const char*)text, (int)len, NULL, &r);
        return r.width;
    }
    else {
        return (gui_size)XTextWidth(font->xfont, (const char*)text, (int)len);
    }
    return width;
}

static void
font_del(Display *dpy, XFont *font)
{
    if(!font) return;
    if(font->set)
        XFreeFontSet(dpy, font->set);
    else
        XFreeFont(dpy, font->xfont);
    free(font);
}

static unsigned long
color_from_byte(const gui_byte *c)
{
    /* NOTE(vurtun): this only works for little-endian */
    unsigned long res = 0;
    res |= (unsigned long)c[0] << 16;
    res |= (unsigned long)c[1] << 8;
    res |= (unsigned long)c[2] << 0;
    return (res);
}

static XSurface*
surface_create(Display *dpy,  int screen, Window root, unsigned int w, unsigned int h)
{
    XSurface *surface = (XSurface*)xcalloc(1, sizeof(XSurface));
    surface->w = w;
    surface->h = h;
    surface->dpy = dpy;
    surface->screen = screen;
    surface->root = root;
    surface->gc = XCreateGC(dpy, root, 0, NULL);
    XSetLineAttributes(dpy, surface->gc, 1, LineSolid, CapButt, JoinMiter);
    surface->drawable = XCreatePixmap(dpy, root, w, h, (unsigned int)DefaultDepth(dpy, screen));
    return surface;
}

static void
surface_resize(XSurface *surf, unsigned int w, unsigned int h) {
    if(!surf) return;
    if (surf->w == w && surf->h == h) return;
    surf->w = w; surf->h = h;
    if(surf->drawable) XFreePixmap(surf->dpy, surf->drawable);
    surf->drawable = XCreatePixmap(surf->dpy, surf->root, w, h,
        (unsigned int)DefaultDepth(surf->dpy, surf->screen));
}

static void
surface_scissor(XSurface *surf, float x, float y, float w, float h)
{
    XRectangle clip_rect;
    clip_rect.x = (short)x;
    clip_rect.y = (short)y;
    clip_rect.width = (unsigned short)w;
    clip_rect.height = (unsigned short)h;
    clip_rect.width = (unsigned short)MIN(surf->w, clip_rect.width);
    clip_rect.height = (unsigned short)MIN(surf->h, clip_rect.height);
    XSetClipRectangles(surf->dpy, surf->gc, 0, 0, &clip_rect, 1, Unsorted);
}

static void
surface_draw_line(XSurface *surf, gui_short x0, gui_short y0, gui_short x1,
    gui_short y1, struct gui_color col)
{
    unsigned long c = color_from_byte(&col.r);
    XSetForeground(surf->dpy, surf->gc, c);
    XDrawLine(surf->dpy, surf->drawable, surf->gc, (int)x0, (int)y0, (int)x1, (int)y1);
}

static void
surface_draw_round_rect(XSurface* surf, gui_short x, gui_short y, gui_ushort w,
    gui_ushort h, gui_ushort r, struct gui_color col)
{
    unsigned long c = color_from_byte(&col.r);
    gui_int mx, my;
    gui_int mw, mh;
    mx = x + r; my = y + r;
    mw = w - (r * 2);
    mh = h - (r * 2);

    XSetForeground(surf->dpy, surf->gc, c);
    XFillRectangle(surf->dpy, surf->drawable, surf->gc, mx, my, (gui_ushort)mw, (gui_ushort)mh);
    XFillRectangle(surf->dpy, surf->drawable, surf->gc, mx, y, (gui_ushort)mw, r);
    XFillRectangle(surf->dpy, surf->drawable, surf->gc, mx+mw, my, r, (gui_ushort)mh);
    XFillRectangle(surf->dpy, surf->drawable, surf->gc, mx, my+mh, (gui_ushort)mw, r);
    XFillRectangle(surf->dpy, surf->drawable, surf->gc, x, my, r, (gui_ushort)mh);

    XFillArc(surf->dpy, surf->drawable, surf->gc, x, y, 2*r, 2*r, 90 * 64, 90 * 64);
    XFillArc(surf->dpy, surf->drawable, surf->gc, mx+mw-r, y, 2*r, 2*r, 90 * 64, -90 * 64);
    XFillArc(surf->dpy, surf->drawable, surf->gc, mx+mw-r, my+mh-r, r*2, r*2, 0 * 64, -90 * 64);
    XFillArc(surf->dpy, surf->drawable, surf->gc, x, my+mh-r, r*2, r*2, -90 * 64, -90 * 64);
}

static void
surface_draw_rect(XSurface* surf, gui_short x, gui_short y, gui_ushort w,
    gui_ushort h, struct gui_color col)
{
    unsigned long c = color_from_byte(&col.r);
    XSetForeground(surf->dpy, surf->gc, c);
    XFillRectangle(surf->dpy, surf->drawable, surf->gc, x, y, w, h);
}

static void
surface_draw_triangle(XSurface *surf, gui_short x0, gui_short y0, gui_short x1,
    gui_short y1, gui_short x2, gui_short y2, struct gui_color col)
{
    XPoint pnts[3];
    unsigned long c = color_from_byte(&col.r);
    pnts[0].x = (short)x0;
    pnts[0].y = (short)y0;
    pnts[1].x = (short)x1;
    pnts[1].y = (short)y1;
    pnts[2].x = (short)x2;
    pnts[2].y = (short)y2;
    XSetForeground(surf->dpy, surf->gc, c);
    XFillPolygon(surf->dpy, surf->drawable, surf->gc, pnts, 3, Convex, CoordModeOrigin);
}

static void
surface_draw_circle(XSurface *surf, gui_short x, gui_short y, gui_ushort w,
    gui_ushort h, struct gui_color col)
{
    unsigned long c = color_from_byte(&col.r);
    XSetForeground(surf->dpy, surf->gc, c);
    XFillArc(surf->dpy, surf->drawable, surf->gc, (int)x, (int)y,
        (unsigned)w, (unsigned)h, 0, 360 * 64);
}

static void
surface_draw_text(XSurface *surf, gui_short x, gui_short y, gui_ushort w, gui_ushort h,
    const char *text, size_t len, XFont *font, struct gui_color cbg, struct gui_color cfg)
{
    int tx, ty, th;
    unsigned long bg = color_from_byte(&cbg.r);
    unsigned long fg = color_from_byte(&cfg.r);

    XSetForeground(surf->dpy, surf->gc, bg);
    XFillRectangle(surf->dpy, surf->drawable, surf->gc, (int)x, (int)y, (unsigned)w, (unsigned)h);
    if(!text || !font || !len) return;

    tx = (int)x;
    th = font->ascent + font->descent;
    ty = (int)y + ((int)h / 2) - (th / 2) + font->ascent;
    XSetForeground(surf->dpy, surf->gc, fg);
    if(font->set)
        XmbDrawString(surf->dpy,surf->drawable,font->set,surf->gc,tx,ty,(const char*)text,(int)len);
    else
        XDrawString(surf->dpy, surf->drawable, surf->gc, tx, ty, (const char*)text, (int)len);
}

static void
surface_clear(XSurface *surf, unsigned long color)
{
    XSetForeground(surf->dpy, surf->gc, color);
    XFillRectangle(surf->dpy, surf->drawable, surf->gc, 0, 0, surf->w, surf->h);
}

static void
surface_blit(Drawable target, XSurface *surf, unsigned int width, unsigned int height)
{
    XCopyArea(surf->dpy, surf->drawable, target, surf->gc, 0, 0, width, height, 0, 0);
}

static void
surface_del(XSurface *surf)
{
    XFreePixmap(surf->dpy, surf->drawable);
    XFreeGC(surf->dpy, surf->gc);
    free(surf);
}

static void
draw(XSurface *surf, struct gui_command_queue *queue)
{
    const struct gui_command *cmd;
    gui_foreach_command(cmd, queue) {
        switch (cmd->type) {
        case GUI_COMMAND_NOP: break;
        case GUI_COMMAND_SCISSOR: {
            const struct gui_command_scissor *s = gui_command(scissor, cmd);
            surface_scissor(surf, s->x, s->y, s->w, s->h);
        } break;
        case GUI_COMMAND_LINE: {
            const struct gui_command_line *l = gui_command(line, cmd);
            surface_draw_line(surf, l->begin.x, l->begin.y, l->end.x,
                l->end.y, l->color);
        } break;
        case GUI_COMMAND_RECT: {
            const struct gui_command_rect *r = gui_command(rect, cmd);
            if (r->rounding)
                surface_draw_round_rect(surf, r->x, r->y, r->w, r->h, (gui_ushort)r->rounding, r->color);
            else
                surface_draw_rect(surf, r->x, r->y, r->w, r->h, r->color);
        } break;
        case GUI_COMMAND_CIRCLE: {
            const struct gui_command_circle *c = gui_command(circle, cmd);
            surface_draw_circle(surf, c->x, c->y, c->w, c->h, c->color);
        } break;
        case GUI_COMMAND_TRIANGLE: {
            const struct gui_command_triangle *t = gui_command(triangle, cmd);
            surface_draw_triangle(surf, t->a.x, t->a.y, t->b.x, t->b.y,
                t->c.x, t->c.y, t->color);
        } break;
        case GUI_COMMAND_TEXT: {
            const struct gui_command_text *t = gui_command(text, cmd);
            surface_draw_text(surf, t->x, t->y, t->w, t->h, (const char*)t->string,
                    t->length, (XFont*)t->font.ptr, t->background, t->foreground);
        } break;
        case GUI_COMMAND_IMAGE:
        case GUI_COMMAND_MAX:
        default: break;
        }
    }
    gui_command_queue_clear(queue);
}

static void
key(struct XWindow *xw, struct gui_input *in, XEvent *evt, gui_bool down)
{
    int ret;
    KeySym *code = XGetKeyboardMapping(xw->dpy, (KeyCode)evt->xkey.keycode, 1, &ret);
    if (*code == XK_Shift_L || *code == XK_Shift_R)
        gui_input_key(in, GUI_KEY_SHIFT, down);
    else if (*code == XK_Delete)
        gui_input_key(in, GUI_KEY_DEL, down);
    else if (*code == XK_Return)
        gui_input_key(in, GUI_KEY_ENTER, down);
    else if (*code == XK_space)
        gui_input_key(in, GUI_KEY_SPACE, down);
    else if (*code == XK_Left)
        gui_input_key(in, GUI_KEY_LEFT, down);
    else if (*code == XK_Right)
        gui_input_key(in, GUI_KEY_RIGHT, down);
    else if (*code == XK_BackSpace)
        gui_input_key(in, GUI_KEY_BACKSPACE, down);
    else if (*code > 32 && *code < 128) {
        if (*code == 'c')
            gui_input_key(in, GUI_KEY_COPY, down && (evt->xkey.state & ControlMask));
        else if (*code == 'v')
            gui_input_key(in, GUI_KEY_PASTE, down && (evt->xkey.state & ControlMask));
        else if (*code == 'x')
            gui_input_key(in, GUI_KEY_CUT, down && (evt->xkey.state & ControlMask));
        if (!down) gui_input_char(in, (char)*code);
    }
    XFree(code);
}

static void
motion(struct gui_input *in, XEvent *evt)
{
    const gui_int x = evt->xmotion.x;
    const gui_int y = evt->xmotion.y;
    gui_input_motion(in, x, y);
}

static void
btn(struct gui_input *in, XEvent *evt, gui_bool down)
{
    const gui_int x = evt->xbutton.x;
    const gui_int y = evt->xbutton.y;
    if (evt->xbutton.button == Button1)
        gui_input_button(in, x, y, down);
}

static void
resize(struct XWindow *xw, XSurface *surf)
{
    XGetWindowAttributes(xw->dpy, xw->win, &xw->attr);
    xw->width = (unsigned int)xw->attr.width;
    xw->height = (unsigned int)xw->attr.height;
    surface_resize(surf, xw->width, xw->height);
}

int
main(int argc, char *argv[])
{
    long dt;
    long started;
    XWindow xw;

    struct gui_input in;
    struct gui_font font;
    struct demo_gui gui;

    /* Platform */
    UNUSED(argc); UNUSED(argv);
    memset(&xw, 0, sizeof xw);
    xw.dpy = XOpenDisplay(NULL);
    xw.root = DefaultRootWindow(xw.dpy);
    xw.screen = XDefaultScreen(xw.dpy);
    xw.vis = XDefaultVisual(xw.dpy, xw.screen);
    xw.cmap = XCreateColormap(xw.dpy,xw.root,xw.vis,AllocNone);
    xw.swa.colormap = xw.cmap;
    xw.swa.event_mask =
        ExposureMask | KeyPressMask | KeyReleaseMask | ButtonPress |
        ButtonReleaseMask | ButtonMotionMask | Button1MotionMask | PointerMotionMask;
    xw.win = XCreateWindow(xw.dpy, xw.root, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, 0,
        XDefaultDepth(xw.dpy, xw.screen), InputOutput,
        xw.vis, CWEventMask | CWColormap, &xw.swa);
    XStoreName(xw.dpy, xw.win, "X11");
    XMapWindow(xw.dpy, xw.win);
    XGetWindowAttributes(xw.dpy, xw.win, &xw.attr);
    xw.width = (unsigned int)xw.attr.width;
    xw.height = (unsigned int)xw.attr.height;
    xw.surf = surface_create(xw.dpy, xw.screen, xw.win, xw.width, xw.height);
    xw.font = font_create(xw.dpy, "fixed");

    /* GUI */
    memset(&in, 0, sizeof in);
    memset(&gui, 0, sizeof gui);
    font.userdata.ptr = xw.font;
    font.height = (gui_float)xw.font->height;
    font.width = font_get_text_width;
    gui.memory = calloc(MAX_MEMORY, 1);
    gui.input = &in;
    init_demo(&gui, &font);

    while (gui.running) {
        /* Input */
        XEvent evt;
        started = timestamp();
        gui_input_begin(&in);
        while (XCheckWindowEvent(xw.dpy, xw.win, xw.swa.event_mask, &evt)) {
            if (evt.type == KeyPress)
                key(&xw, &in, &evt, gui_true);
            else if (evt.type == KeyRelease) key(&xw, &in, &evt, gui_false);
            else if (evt.type == ButtonPress) btn(&in, &evt, gui_true);
            else if (evt.type == ButtonRelease) btn(&in, &evt, gui_false);
            else if (evt.type == MotionNotify) motion(&in, &evt);
            else if (evt.type == Expose || evt.type == ConfigureNotify)
                resize(&xw, xw.surf);
        }
        gui_input_end(&in);

        /* GUI */
        run_demo(&gui);

        /* Draw */
        XClearWindow(xw.dpy, xw.win);
        surface_clear(xw.surf, 0x00646464);
        draw(xw.surf, &gui.queue);
        surface_blit(xw.win, xw.surf, xw.width, xw.height);
        XFlush(xw.dpy);

        /* Timing */
        dt = timestamp() - started;
        if (dt < DTIME)
            sleep_for(DTIME - dt);
    }

    free(gui.memory);
    font_del(xw.dpy, xw.font);
    surface_del(xw.surf);
    XUnmapWindow(xw.dpy, xw.win);
    XFreeColormap(xw.dpy, xw.cmap);
    XDestroyWindow(xw.dpy, xw.win);
    XCloseDisplay(xw.dpy);
    return 0;
}

