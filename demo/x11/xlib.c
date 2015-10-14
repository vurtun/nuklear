/*
    Copyright (c) 2015 Micha Mettke

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1.  The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software
        in a product, an acknowledgment in the product documentation would be
        appreciated but is not required.
    2.  Altered source versions must be plainly marked as such, and must not be
        misrepresented as being the original software.
    3.  This notice may not be removed or altered from any source distribution.
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
#define CURVE_STEPS 22
#include "../../zahnrad.h"

#define UNUSED(a)   ((void)(a))
static void clipboard_set(const char *text){UNUSED(text);}
static zr_bool clipboard_is_filled(void){return zr_false;}
static const char* clipboard_get(void) {return NULL;}

#include "../demo.c"

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

static zr_size
font_get_text_width(zr_handle handle, const zr_char *text, zr_size len)
{
    XFont *font = (XFont*)handle.ptr;
    XRectangle r;
    zr_size width;
    if(!font || !text)
        return 0;

    if(font->set) {
        XmbTextExtents(font->set, (const char*)text, (int)len, NULL, &r);
        return r.width;
    }
    else {
        return (zr_size)XTextWidth(font->xfont, (const char*)text, (int)len);
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
color_from_byte(const zr_byte *c)
{
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
    clip_rect.x = (short)(x - 1);
    clip_rect.y = (short)(y - 1);
    clip_rect.width = (unsigned short)(w + 2);
    clip_rect.height = (unsigned short)(h + 2);
    clip_rect.width = (unsigned short)MIN(surf->w, clip_rect.width);
    clip_rect.height = (unsigned short)MIN(surf->h, clip_rect.height);
    XSetClipRectangles(surf->dpy, surf->gc, 0, 0, &clip_rect, 1, Unsorted);
}

static void
surface_draw_line(XSurface *surf, zr_short x0, zr_short y0, zr_short x1,
    zr_short y1, struct zr_color col)
{
    unsigned long c = color_from_byte(&col.r);
    XSetForeground(surf->dpy, surf->gc, c);
    XDrawLine(surf->dpy, surf->drawable, surf->gc, (int)x0, (int)y0, (int)x1, (int)y1);
}

static void
surface_draw_rect(XSurface* surf, zr_short x, zr_short y, zr_ushort w,
    zr_ushort h, struct zr_color col)
{
    unsigned long c = color_from_byte(&col.r);
    XSetForeground(surf->dpy, surf->gc, c);
    XFillRectangle(surf->dpy, surf->drawable, surf->gc, x, y, w, h);
}

static void
surface_draw_triangle(XSurface *surf, zr_short x0, zr_short y0, zr_short x1,
    zr_short y1, zr_short x2, zr_short y2, struct zr_color col)
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
surface_draw_circle(XSurface *surf, zr_short x, zr_short y, zr_ushort w,
    zr_ushort h, struct zr_color col)
{
    unsigned long c = color_from_byte(&col.r);
    XSetForeground(surf->dpy, surf->gc, c);
    XFillArc(surf->dpy, surf->drawable, surf->gc, (int)x, (int)y,
        (unsigned)w, (unsigned)h, 0, 360 * 64);
}

static void
surface_draw_text(XSurface *surf, zr_short x, zr_short y, zr_ushort w, zr_ushort h,
    const char *text, size_t len, XFont *font, struct zr_color cbg, struct zr_color cfg)
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
draw(XSurface *surf, struct zr_command_queue *queue)
{
    const struct zr_command *cmd;
    zr_foreach_command(cmd, queue) {
        switch (cmd->type) {
        case ZR_COMMAND_NOP: break;
        case ZR_COMMAND_SCISSOR: {
            const struct zr_command_scissor *s = zr_command(scissor, cmd);
            surface_scissor(surf, s->x, s->y, s->w, s->h);
        } break;
        case ZR_COMMAND_LINE: {
            const struct zr_command_line *l = zr_command(line, cmd);
            surface_draw_line(surf, l->begin.x, l->begin.y, l->end.x,
                l->end.y, l->color);
        } break;
        case ZR_COMMAND_RECT: {
            const struct zr_command_rect *r = zr_command(rect, cmd);
            surface_draw_rect(surf, r->x, r->y, r->w, r->h, r->color);
        } break;
        case ZR_COMMAND_CIRCLE: {
            const struct zr_command_circle *c = zr_command(circle, cmd);
            surface_draw_circle(surf, c->x, c->y, c->w, c->h, c->color);
        } break;
        case ZR_COMMAND_TRIANGLE: {
            const struct zr_command_triangle *t = zr_command(triangle, cmd);
            surface_draw_triangle(surf, t->a.x, t->a.y, t->b.x, t->b.y,
                t->c.x, t->c.y, t->color);
        } break;
        case ZR_COMMAND_TEXT: {
            const struct zr_command_text *t = zr_command(text, cmd);
            surface_draw_text(surf, t->x, t->y, t->w, t->h, (const char*)t->string,
                t->length, (XFont*)t->font->userdata.ptr, t->background, t->foreground);
        } break;
        case ZR_COMMAND_CURVE:
        case ZR_COMMAND_IMAGE:
        case ZR_COMMAND_ARC:
        default: break;
        }
    }
    zr_command_queue_clear(queue);
}

static void
input_key(struct XWindow *xw, struct zr_input *in, XEvent *evt, zr_bool down)
{
    int ret;
    KeySym *code = XGetKeyboardMapping(xw->dpy, (KeyCode)evt->xkey.keycode, 1, &ret);
    if (*code == XK_Shift_L || *code == XK_Shift_R)
        zr_input_key(in, ZR_KEY_SHIFT, down);
    else if (*code == XK_Delete)
        zr_input_key(in, ZR_KEY_DEL, down);
    else if (*code == XK_Return)
        zr_input_key(in, ZR_KEY_ENTER, down);
    else if (*code == XK_space && !down)
        zr_input_char(in, ' ');
    else if (*code == XK_Left)
        zr_input_key(in, ZR_KEY_LEFT, down);
    else if (*code == XK_Right)
        zr_input_key(in, ZR_KEY_RIGHT, down);
    else if (*code == XK_BackSpace)
        zr_input_key(in, ZR_KEY_BACKSPACE, down);
    else if (*code > 32 && *code < 128) {
        if (*code == 'c')
            zr_input_key(in, ZR_KEY_COPY, down && (evt->xkey.state & ControlMask));
        else if (*code == 'v')
            zr_input_key(in, ZR_KEY_PASTE, down && (evt->xkey.state & ControlMask));
        else if (*code == 'x')
            zr_input_key(in, ZR_KEY_CUT, down && (evt->xkey.state & ControlMask));
        if (!down)
            zr_input_unicode(in, (zr_uint)*code);
    }
    XFree(code);
}

static void
input_motion(struct zr_input *in, XEvent *evt)
{
    const zr_int x = evt->xmotion.x;
    const zr_int y = evt->xmotion.y;
    zr_input_motion(in, x, y);
}

static void
input_button(struct zr_input *in, XEvent *evt, zr_bool down)
{
    const zr_int x = evt->xbutton.x;
    const zr_int y = evt->xbutton.y;
    if (evt->xbutton.button == Button1)
        zr_input_button(in, ZR_BUTTON_LEFT, x, y, down);
    else if (evt->xbutton.button == Button3)
        zr_input_button(in, ZR_BUTTON_RIGHT, x, y, down);
    else if (evt->xbutton.button == Button4)
        zr_input_scroll(in, 1.0f);
    else if (evt->xbutton.button == Button5)
        zr_input_scroll(in, -1.0f);
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
        ExposureMask | KeyPressMask | KeyReleaseMask |
        ButtonPress | ButtonReleaseMask| ButtonMotionMask |
        Button1MotionMask | Button3MotionMask | Button4MotionMask | Button5MotionMask|
        PointerMotionMask;
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
    memset(&gui, 0, sizeof gui);
    zr_command_queue_init_fixed(&gui.queue, calloc(MAX_MEMORY, 1), MAX_MEMORY);
    gui.font.userdata = zr_handle_ptr(xw.font);
    gui.font.height = (zr_float)xw.font->height;
    gui.font.width = font_get_text_width;
    init_demo(&gui);

    while (gui.running) {
        /* Input */
        XEvent evt;
        started = timestamp();
        zr_input_begin(&gui.input);
        while (XCheckWindowEvent(xw.dpy, xw.win, xw.swa.event_mask, &evt)) {
            if (evt.type == KeyPress)
                input_key(&xw, &gui.input, &evt, zr_true);
            else if (evt.type == KeyRelease)
                input_key(&xw, &gui.input, &evt, zr_false);
            else if (evt.type == ButtonPress)
                input_button(&gui.input, &evt, zr_true);
            else if (evt.type == ButtonRelease)
                input_button(&gui.input, &evt, zr_false);
            else if (evt.type == MotionNotify)
                input_motion(&gui.input, &evt);
            else if (evt.type == Expose || evt.type == ConfigureNotify)
                resize(&xw, xw.surf);
        }
        zr_input_end(&gui.input);

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

    free(zr_buffer_memory(&gui.queue.buffer));
    font_del(xw.dpy, xw.font);
    surface_del(xw.surf);
    XUnmapWindow(xw.dpy, xw.win);
    XFreeColormap(xw.dpy, xw.cmap);
    XDestroyWindow(xw.dpy, xw.win);
    XCloseDisplay(xw.dpy);
    return 0;
}

