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

#include "../gui.h"

/* macros */
#define MAX_BUFFER  64
#define MAX_DEPTH   4
#define WIN_WIDTH   800
#define WIN_HEIGHT  600
#define DTIME       16

#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#define MAX(a,b)    ((a) < (b) ? (b) : (a))
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#define LEN(a)      (sizeof(a)/sizeof(a)[0])
#define UNUSED(a)   ((void)(a))

typedef struct XFont XFont;
typedef struct XSurface XSurface;
typedef struct XWindow XWindow;

struct demo {
    gui_char in_buf[MAX_BUFFER];
    gui_size in_len;
    gui_bool in_act;
    gui_bool check;
    gui_int option;
    gui_float slider;
    gui_size prog;
    gui_int spinner;
    gui_bool spin_act;
    gui_size item_cur;
    gui_tab tab;
    gui_group group;
    gui_shelf shelf;
    gui_size current;
};

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
    gui_size clip_depth;
    struct gui_rect clips[MAX_DEPTH];
};

struct XWindow {
    Display *dpy;
    Window root;
    Visual *vis;
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
    XFont *font = xcalloc(1, sizeof(XFont));
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
font_get_text_width(void *handle, const gui_char *text, gui_size len)
{
    XFont *font = handle;
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
color_from_byte(struct gui_color col)
{
    unsigned long res = 0;
    res |= (unsigned long)col.r << 16;
    res |= (unsigned long)col.g << 8;
    res |= (unsigned long)col.b << 0;
    return (res);
}

static XSurface*
surface_create(Display *dpy,  int screen, Window root, unsigned int w, unsigned int h)
{
    XSurface *surface = xcalloc(1, sizeof(XSurface));
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
unify(struct gui_rect *clip, const struct gui_rect *a, const struct gui_rect *b)
{
    clip->x = MAX(a->x, b->x);
    clip->y = MAX(a->y, b->y);
    clip->w = MIN(a->x + a->w, b->x + b->w) - clip->x;
    clip->h = MIN(a->y + a->h, b->y + b->h) - clip->y;
}

static void
surface_scissor(void *handle, gui_float x, gui_float y, gui_float w, gui_float h)
{
    XSurface *surf = handle;
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
surface_draw_line(void *handle, gui_float x0, gui_float y0, gui_float x1, gui_float y1, struct gui_color col)
{
    XSurface *surf = handle;
    unsigned long c = color_from_byte(col);
    XSetForeground(surf->dpy, surf->gc, c);
    XDrawLine(surf->dpy, surf->drawable, surf->gc, (int)x0, (int)y0, (int)x1, (int)y1);
}

static void
surface_draw_rect(void *handle, gui_float x, gui_float y, gui_float w, gui_float h, struct gui_color col)
{
    XSurface *surf = handle;
    unsigned long c = color_from_byte(col);
    XSetForeground(surf->dpy, surf->gc, c);
    XFillRectangle(surf->dpy, surf->drawable, surf->gc, (int)x, (int)y, (unsigned)w, (unsigned)h);
}

static void
surface_draw_triangle(void *handle, const struct gui_vec2 *src, struct gui_color col)
{
    XPoint pnts[3];
    XSurface *surf = handle;
    unsigned long c = color_from_byte(col);
    pnts[0].x = (short)src[0].x;
    pnts[0].y = (short)src[0].y;
    pnts[1].x = (short)src[1].x;
    pnts[1].y = (short)src[1].y;
    pnts[2].x = (short)src[2].x;
    pnts[2].y = (short)src[2].y;
    XSetForeground(surf->dpy, surf->gc, c);
    XFillPolygon(surf->dpy, surf->drawable, surf->gc, pnts, 3, Convex, CoordModeOrigin);
}

static void
surface_draw_circle(void *handle, gui_float x, gui_float y, gui_float w, gui_float h, struct gui_color col)
{
    XSurface *surf = handle;
    unsigned long c = color_from_byte(col);
    XSetForeground(surf->dpy, surf->gc, c);
    XFillArc(surf->dpy, surf->drawable, surf->gc, (int)x, (int)y,
        (unsigned)w, (unsigned)h, 0, 360 * 64);
}

static void
surface_draw_text(void *handle, gui_float x, gui_float y, gui_float w, gui_float h, const gui_char *text,
    gui_size len, const struct gui_font *f, struct gui_color cbg, struct gui_color cfg)
{
    int i, tx, ty, th, olen;
    XSurface *surf = handle;
    XFont *font = f->userdata;
    unsigned long bg = color_from_byte(cbg);
    unsigned long fg = color_from_byte(cfg);

    XSetForeground(surf->dpy, surf->gc, bg);
    XFillRectangle(surf->dpy, surf->drawable, surf->gc, (int)x, (int)y, (unsigned)w, (unsigned)h);
    if(!text || !font || !len) return;

    tx = (int)x;
    th = font->ascent + font->descent;
    ty = (int)y + ((int)h / 2) - (th / 2) + font->ascent;
    XSetForeground(surf->dpy, surf->gc, fg);
    if(font->set)
        XmbDrawString(surf->dpy, surf->drawable, font->set, surf->gc, tx, ty, (const char*)text, (int)len);
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
key(struct XWindow *xw, struct gui_input *in, XEvent *evt, gui_bool down)
{
    int ret;
    KeySym *code = XGetKeyboardMapping(xw->dpy, (KeyCode)evt->xkey.keycode, 1, &ret);
    if (*code == XK_Control_L || *code == XK_Control_R)
        gui_input_key(in, GUI_KEY_CTRL, down);
    else if (*code == XK_Shift_L || *code == XK_Shift_R)
        gui_input_key(in, GUI_KEY_SHIFT, down);
    else if (*code == XK_Delete)
        gui_input_key(in, GUI_KEY_DEL, down);
    else if (*code == XK_Return)
        gui_input_key(in, GUI_KEY_ENTER, down);
    else if (*code == XK_space)
        gui_input_key(in, GUI_KEY_SPACE, down);
    else if (*code == XK_BackSpace)
        gui_input_key(in, GUI_KEY_BACKSPACE, down);
    else if (*code > 32 && *code < 128 && !down) {
        gui_glyph glyph;
        glyph[0] = (gui_char)*code;
        gui_input_char(in, glyph);
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

static void
demo_panel(struct gui_panel *panel, struct demo *demo)
{
    enum {HISTO, PLOT};
    const char *shelfs[] = {"Histogram", "Lines"};
    const gui_float values[] = {8.0f, 15.0f, 20.0f, 12.0f, 30.0f};
    const char *items[] = {"Fist", "Pistol", "Shotgun", "Railgun", "BFG"};

    /* Tabs */
    gui_panel_layout(panel, 100, 1);
    gui_panel_tab_begin(panel, &demo->tab, "Difficulty");
    gui_panel_layout(&demo->tab, 30, 3);
    if (gui_panel_option(&demo->tab, "easy", demo->option == 0)) demo->option = 0;
    if (gui_panel_option(&demo->tab, "normal", demo->option == 1)) demo->option = 1;
    if (gui_panel_option(&demo->tab, "hard", demo->option == 2)) demo->option = 2;
    if (gui_panel_option(&demo->tab, "hell", demo->option == 3)) demo->option = 3;
    if (gui_panel_option(&demo->tab, "doom", demo->option == 4)) demo->option = 4;
    if (gui_panel_option(&demo->tab, "godlike", demo->option == 5)) demo->option = 5;
    gui_panel_tab_end(panel, &demo->tab);

    /* Shelf */
    gui_panel_layout(panel, 200, 2);
    demo->current = gui_panel_shelf_begin(panel, &demo->shelf, shelfs, LEN(shelfs), demo->current);
    gui_panel_layout(&demo->shelf, 100, 1);
    if (demo->current == HISTO) {
        gui_panel_histo(&demo->shelf, values, LEN(values));
    } else {
        gui_panel_plot(&demo->shelf, values, LEN(values));
    }
    gui_panel_shelf_end(panel, &demo->shelf);

    /* Group */
    gui_panel_group_begin(panel, &demo->group, "Options");
    gui_panel_layout(&demo->group, 30, 1);
    if (gui_panel_button_text(&demo->group, "button", GUI_BUTTON_DEFAULT))
        fprintf(stdout, "button pressed!\n");
    demo->check = gui_panel_check(&demo->group, "advanced", demo->check);
    demo->slider = gui_panel_slider(&demo->group, 0, demo->slider, 10, 1.0f);
    demo->prog = gui_panel_progress(&demo->group, demo->prog, 100, gui_true);
    demo->item_cur = gui_panel_selector(&demo->group, items, LEN(items), demo->item_cur);
    demo->spin_act = gui_panel_spinner(&demo->group, 0, &demo->spinner, 250, 10, demo->spin_act);
    gui_panel_group_end(panel, &demo->group);
}

int
main(int argc, char *argv[])
{
    long dt;
    long started;
    gui_bool running = gui_true;
    XWindow xw;

    /* GUI */
    gui_bool checked = gui_false;
    gui_float value = 5.0f;
    gui_size done = 20;
    struct gui_input in;
    struct gui_config config;
    struct gui_canvas canvas;
    struct gui_panel panel;
    struct demo demo;

    /* Window */
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
    xw.win = XCreateWindow(xw.dpy, xw.root, 0, 0, WIN_WIDTH, WIN_HEIGHT, 0,
        XDefaultDepth(xw.dpy, xw.screen), InputOutput,
        xw.vis, CWEventMask | CWColormap, &xw.swa);
    XStoreName(xw.dpy, xw.win, "X11");
    XMapWindow(xw.dpy, xw.win);
    XGetWindowAttributes(xw.dpy, xw.win, &xw.attr);
    xw.width = (unsigned int)xw.attr.width;
    xw.height = (unsigned int)xw.attr.height;

    /* GUI */
    canvas.userdata = surface_create(xw.dpy, xw.screen, xw.win, xw.width, xw.height);
    canvas.width = xw.width;
    canvas.height = xw.height;
    canvas.scissor = surface_scissor;
    canvas.draw_line = surface_draw_line;
    canvas.draw_rect = surface_draw_rect;
    canvas.draw_circle = surface_draw_circle;
    canvas.draw_triangle = surface_draw_triangle;
    canvas.draw_text = surface_draw_text;
    canvas.font.userdata = font_create(xw.dpy, "fixed");
    canvas.font.height = (gui_float)((XFont*)canvas.font.userdata)->height;
    canvas.font.width = font_get_text_width;
    gui_default_config(&config);
    memset(&in, 0, sizeof in);
    memset(&panel, 0, sizeof panel);
    panel.x = 50; panel.y = 50;
    panel.w = 420; panel.h = 300;

    memset(&demo, 0, sizeof(demo));
    demo.tab.minimized = gui_true;
    demo.spinner = 100;
    demo.slider = 2.0f;
    demo.prog = 60;
    demo.current = 1;

    while (running) {
        /* Input */
        XEvent evt;
        started = timestamp();
        gui_input_begin(&in);
        while (XCheckWindowEvent(xw.dpy, xw.win, xw.swa.event_mask, &evt)) {
            if (evt.type == KeyPress) key(&xw, &in, &evt, gui_true);
            else if (evt.type == KeyRelease) key(&xw, &in, &evt, gui_false);
            else if (evt.type == ButtonPress) btn(&in, &evt, gui_true);
            else if (evt.type == ButtonRelease) btn(&in, &evt, gui_false);
            else if (evt.type == MotionNotify) motion(&in, &evt);
            else if (evt.type == Expose || evt.type == ConfigureNotify)
                resize(&xw, canvas.userdata);
        }
        gui_input_end(&in);

        /* GUI */
        XClearWindow(xw.dpy, xw.win);
        surface_clear(canvas.userdata, 0x002D2D2D);
        canvas.width = xw.width; canvas.height = xw.height;
        running = gui_panel_begin(&panel, "Demo", panel.x, panel.y, panel.w, panel.h,
            GUI_PANEL_CLOSEABLE|GUI_PANEL_MINIMIZABLE|GUI_PANEL_BORDER|
            GUI_PANEL_MOVEABLE|GUI_PANEL_SCALEABLE, &config, &canvas, &in);
        demo_panel(&panel, &demo);
        gui_panel_end(&panel);
        surface_blit(xw.win, canvas.userdata, xw.width, xw.height);
        XFlush(xw.dpy);

        /* Timing */
        dt = timestamp() - started;
        if (dt < DTIME)
            sleep_for(DTIME - dt);
    }

    font_del(xw.dpy, canvas.font.userdata);
    surface_del(canvas.userdata);
    XUnmapWindow(xw.dpy, xw.win);
    XFreeColormap(xw.dpy, xw.cmap);
    XDestroyWindow(xw.dpy, xw.win);
    XCloseDisplay(xw.dpy);
    return 0;
}

