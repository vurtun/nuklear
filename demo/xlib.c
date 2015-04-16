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
#define MAX_BUFFER 64
#define MAX_MEMORY (16 * 1024)
#define MAX_DEPTH   8
#define MAX_PANEL   4
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
    Colormap cmap;
    XWindowAttributes attr;
    XSetWindowAttributes swa;
    Window win;
    GC gc;
    int screen;
    unsigned int width;
    unsigned int height;
    XSurface surface;
};

struct demo {
    gui_char in_buf[MAX_BUFFER];
    gui_size in_len;
    gui_bool in_act;
    gui_char cmd_buf[MAX_BUFFER];
    gui_size cmd_len;
    gui_bool cmd_act;
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
    if (gettimeofday(&tv, NULL) < 0)
        return 0;
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
    surf->w = w;
    surf->h = h;
    if(surf->drawable)
        XFreePixmap(surf->dpy, surf->drawable);
    surf->drawable = XCreatePixmap(surf->dpy, surf->root, w, h,
        (unsigned int)DefaultDepth(surf->dpy, surf->screen));
}

static void
surface_scissor(XSurface *surf, int x, int y, unsigned int w, unsigned int h)
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
surface_draw_line(XSurface *surf, int x0, int y0, int x1, int y1, unsigned long c)
{
    XSetForeground(surf->dpy, surf->gc, c);
    XDrawLine(surf->dpy, surf->drawable, surf->gc, x0, y0, x1, y1);
}

static void
surface_draw_rect(XSurface *surf, int x, int y, unsigned int w, unsigned int h, unsigned long c)
{
    XSetForeground(surf->dpy, surf->gc, c);
    XFillRectangle(surf->dpy, surf->drawable, surf->gc, x, y, w, h);
}

static void
surface_draw_triangle(XSurface *surf, int x0, int y0, int x1, int y1, int x2, int y2,
    unsigned long c)
{
    XPoint pnts[3];
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
surface_draw_circle(XSurface *surf, int x, int y, unsigned int radius, unsigned long c)
{
    unsigned int d = radius * 2;
    XSetForeground(surf->dpy, surf->gc, c);
    x -= (int)radius;
    y -= (int)radius;
    XFillArc(surf->dpy, surf->drawable, surf->gc, x, y, d, d, 0, 360 * 64);
}

static void
surface_draw_text(XSurface *surf, XFont *font, int x, int y, unsigned int w, unsigned int h,
    const char *text, unsigned int len, unsigned long bg, unsigned long fg)
{
    int i, tx, ty, th, olen;
    XSetForeground(surf->dpy, surf->gc, bg);
    XFillRectangle(surf->dpy, surf->drawable, surf->gc, x, y, w, h);
    if(!text || !font || !len) return;

    th = font->ascent + font->descent;
    ty = y + ((int)h / 2) - (th / 2) + font->ascent;
    tx = x;/* + ((int)h / 2);*/
    XSetForeground(surf->dpy, surf->gc, fg);
    if(font->set)
        XmbDrawString(surf->dpy, surf->drawable, font->set, surf->gc, tx, ty, text, (int)len);
    else
        XDrawString(surf->dpy, surf->drawable, surf->gc, tx, ty, text, (int)len);
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
button(struct gui_input *in, XEvent *evt, gui_bool down)
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

static unsigned long
color_from_byte(struct gui_color col)
{
    unsigned long res = 0;
    res |= (unsigned long)col.r << 16;
    res |= (unsigned long)col.g << 8;
    res |= (unsigned long)col.b << 0;
    return (res);
}

static void
execute(XSurface *surf, const struct gui_command_list *list)
{
    const struct gui_command *iter = NULL;
    if (!list->count) return;
    iter = list->begin;
    while (iter != list->end) {
        if (iter->type == GUI_COMMAND_CLIP) {
            const struct gui_command_clip *clip = (const struct gui_command_clip*)iter;
            surface_scissor(surf, (int)clip->x, (int)clip->y,
                (unsigned int)clip->w, (unsigned int)clip->h);
        } else if (iter->type == GUI_COMMAND_LINE) {
            const struct gui_command_line *line = (const struct gui_command_line*)iter;
            const unsigned long color = color_from_byte(line->color);
            surface_draw_line(surf, (int)line->from.x, (int)line->from.y,
                (int)line->to.x, (int)line->to.y, color);
        } else if (iter->type == GUI_COMMAND_RECT) {
            const struct gui_command_rect *rect = (const struct gui_command_rect*)iter;
            const unsigned long color = color_from_byte(rect->color);
            surface_draw_rect(surf, (int)rect->x, (int)rect->y,
                (unsigned int)rect->w, (unsigned int)rect->h, color);
        } else if (iter->type == GUI_COMMAND_CIRCLE) {
            const struct gui_command_circle *circle = (const struct gui_command_circle*)iter;
            const unsigned long color = color_from_byte(circle->color);
            surface_draw_circle(surf, (int)circle->x, (int)circle->y,
                (unsigned int)circle->radius, color);
        } else if (iter->type == GUI_COMMAND_BITMAP) {
            const struct gui_command_bitmap *bitmap = (const struct gui_command_bitmap*)iter;
        } else if (iter->type == GUI_COMMAND_TRIANGLE) {
            const struct gui_command_triangle *triangle = (const struct gui_command_triangle*)iter;
            const unsigned long color = color_from_byte(triangle->color);
            surface_draw_triangle(surf, (int)triangle->pnt[0].x, (int)triangle->pnt[0].y,
                (int)triangle->pnt[1].x, (int)triangle->pnt[1].y, (int)triangle->pnt[2].x,
                (int)triangle->pnt[2].y, color);
        } else if (iter->type == GUI_COMMAND_TEXT) {
            const struct gui_command_text *text = (const struct gui_command_text*)iter;
            const unsigned long bg = color_from_byte(text->background);
            const unsigned long fg = color_from_byte(text->foreground);
            surface_draw_text(surf, text->font, (int)text->x, (int)text->y,
                (unsigned int)text->w, (unsigned int)text->h,
                (const char*)text->string, (unsigned int)text->length, bg, fg);
        }
        iter = iter->next;
    }
}

static void
gui_draw(XSurface *surf, const struct gui_output *out)
{
    gui_size i = 0;
    if (!out->list_size) return;
    for (i = 0; i < out->list_size; i++)
        execute(surf, out->list[i]);
}

static gui_bool
demo_panel(struct gui_context *ctx, struct gui_panel *panel, struct demo *demo)
{
    enum {PLOT, HISTO};
    const char *shelfs[] = {"Histogram", "Lines"};
    const gui_float values[] = {8.0f, 15.0f, 20.0f, 12.0f, 30.0f};
    const char *items[] = {"Fist", "Pistol", "Shotgun", "Railgun", "BFG"};
    gui_bool running;

    running = gui_begin_panel(ctx, panel, "Demo",
        GUI_PANEL_CLOSEABLE|GUI_PANEL_MINIMIZABLE|GUI_PANEL_SCALEABLE|
        GUI_PANEL_MOVEABLE|GUI_PANEL_BORDER);

    /* Tabs */
    gui_panel_layout(panel, 100, 1);
    gui_panel_tab_begin(panel, &demo->tab, "Difficulty");
    gui_panel_layout(&demo->tab, 30, 2);
    if (gui_panel_option(&demo->tab, "easy", demo->option == 0)) demo->option = 0;
    if (gui_panel_option(&demo->tab, "hard", demo->option == 1)) demo->option = 1;
    if (gui_panel_option(&demo->tab, "normal", demo->option == 2)) demo->option = 2;
    if (gui_panel_option(&demo->tab, "godlike", demo->option == 3)) demo->option = 3;
    gui_panel_tab_end(panel, &demo->tab);

    /* Shelf */
    gui_panel_layout(panel, 200, 2);
    demo->current = gui_panel_shelf_begin(panel, &demo->shelf, shelfs, LEN(shelfs), demo->current);
    gui_panel_layout(&demo->shelf, 100, 1);
    if (demo->current == PLOT) {
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
    demo->in_act = gui_panel_input(&demo->group, demo->in_buf, &demo->in_len,
                        MAX_BUFFER, GUI_INPUT_DEFAULT, demo->in_act);
    if (gui_panel_shell(&demo->group, demo->cmd_buf, &demo->cmd_len, MAX_BUFFER, &demo->cmd_act))
        fprintf(stdout, "shell executed!\n");
    gui_panel_group_end(panel, &demo->group);

    gui_end_panel(ctx, panel, NULL);
    return running;
}

int
main(int argc, char *argv[])
{
    long dt;
    long started;
    gui_bool running = gui_true;

    /* GUI */
    struct demo demo;
    struct gui_input in;
    struct gui_config config;
    struct gui_memory memory;
    struct gui_font font;
    struct gui_panel *panel;
    struct gui_context *ctx;
    struct gui_output output;

    /* Window */
    XWindow xw;
    XSurface *surf;
    XFont *xfont;

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
    xfont = font_create(xw.dpy, "fixed");
    surf = surface_create(xw.dpy, xw.screen, xw.win, xw.width, xw.height);

    /* GUI */
    memory.size = MAX_MEMORY;
    memory.memory = calloc(1, memory.size);
    memory.max_depth = MAX_DEPTH;
    memory.max_panels = MAX_PANEL;
    ctx = gui_new(&memory, &in);

    font.user = xfont;
    font.height = (gui_float)xfont->height;
    font.width = font_get_text_width;
    gui_default_config(&config);
    panel = gui_new_panel(ctx, 50, 50, 500, 320, &config, &font);

    memset(&demo, 0, sizeof(demo));
    demo.tab.minimized = gui_false;
    demo.spinner = 250;
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
            else if (evt.type == ButtonPress) button(&in, &evt, gui_true);
            else if (evt.type == ButtonRelease) button(&in, &evt, gui_false);
            else if (evt.type == MotionNotify) motion(&in, &evt);
            else if (evt.type == Expose || evt.type == ConfigureNotify)
                resize(&xw, surf);
        }
        gui_input_end(&in);

        /* GUI */
        gui_begin(ctx, (gui_float)xw.width, (gui_float)xw.height);
        running = demo_panel(ctx, panel, &demo);
        gui_end(ctx, &output, NULL);

        /* Draw */
        XClearWindow(xw.dpy, xw.win);
        surface_clear(surf, 0x00646464);
        gui_draw(surf, &output);
        surface_blit(xw.win, surf, xw.width, xw.height);
        XFlush(xw.dpy);

        /* Timing */
        dt = timestamp() - started;
        if (dt < DTIME)
            sleep_for(DTIME - dt);
    }

    free(memory.memory);
    font_del(xw.dpy, xfont);
    surface_del(surf);
    XDestroyWindow(xw.dpy, xw.win);
    XCloseDisplay(xw.dpy);
    return 0;
}

