/*
 * Nuklear - v1.32.0 - public domain
 * no warrenty implied; use at your own risk.
 * authored from 2015-2016 by Micha Mettke
 */
/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */
#ifndef NK_XLIB_H_
#define NK_XLIB_H_

#include <X11/Xlib.h>
/* Font */
typedef struct XFont XFont;
NK_API XFont*               nk_xfont_create(Display *dpy, const char *name);
NK_API void                 nk_xfont_del(Display *dpy, XFont *font);

NK_API struct nk_context*   nk_xlib_init(XFont *font, Display *dpy, int screen, Window root, unsigned int w, unsigned int h);
NK_API int                  nk_xlib_handle_event(Display *dpy, int screen, Window win, XEvent *evt);
NK_API void                 nk_xlib_render(Drawable screen, struct nk_color clear);
NK_API void                 nk_xlib_shutdown(void);
NK_API void                 nk_xlib_set_font(XFont *font);

#endif
/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#ifdef NK_XLIB_IMPLEMENTATION
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xlocale.h>

typedef struct XSurface XSurface;
struct XFont {
    int ascent;
    int descent;
    int height;
    XFontSet set;
    XFontStruct *xfont;
    struct nk_user_font handle;
};
struct XSurface {
    GC gc;
    Display *dpy;
    int screen;
    Window root;
    Drawable drawable;
    unsigned int w, h;
};
static struct  {
    struct nk_context ctx;
    struct XSurface *surf;
    Cursor cursor;
    Display *dpy;
    Window root;
} xlib;

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#endif

static unsigned long
nk_color_from_byte(const nk_byte *c)
{
    unsigned long res = 0;
    res |= (unsigned long)c[0] << 16;
    res |= (unsigned long)c[1] << 8;
    res |= (unsigned long)c[2] << 0;
    return (res);
}

static XSurface*
nk_xsurf_create(int screen, unsigned int w, unsigned int h)
{
    XSurface *surface = (XSurface*)calloc(1, sizeof(XSurface));
    surface->w = w;
    surface->h = h;
    surface->dpy = xlib.dpy;
    surface->screen = screen;
    surface->root = xlib.root;
    surface->gc = XCreateGC(xlib.dpy, xlib.root, 0, NULL);
    XSetLineAttributes(xlib.dpy, surface->gc, 1, LineSolid, CapButt, JoinMiter);
    surface->drawable = XCreatePixmap(xlib.dpy, xlib.root, w, h,
        (unsigned int)DefaultDepth(xlib.dpy, screen));
    return surface;
}

static void
nk_xsurf_resize(XSurface *surf, unsigned int w, unsigned int h)
{
    if(!surf) return;
    if (surf->w == w && surf->h == h) return;
    surf->w = w; surf->h = h;
    if(surf->drawable) XFreePixmap(surf->dpy, surf->drawable);
    surf->drawable = XCreatePixmap(surf->dpy, surf->root, w, h,
        (unsigned int)DefaultDepth(surf->dpy, surf->screen));
}

static void
nk_xsurf_scissor(XSurface *surf, float x, float y, float w, float h)
{
    XRectangle clip_rect;
    clip_rect.x = (short)(x-1);
    clip_rect.y = (short)(y-1);
    clip_rect.width = (unsigned short)(w+2);
    clip_rect.height = (unsigned short)(h+2);
    XSetClipRectangles(surf->dpy, surf->gc, 0, 0, &clip_rect, 1, Unsorted);
}

static void
nk_xsurf_stroke_line(XSurface *surf, short x0, short y0, short x1,
    short y1, unsigned int line_thickness, struct nk_color col)
{
    unsigned long c = nk_color_from_byte(&col.r);
    XSetForeground(surf->dpy, surf->gc, c);
    XSetLineAttributes(surf->dpy, surf->gc, line_thickness, LineSolid, CapButt, JoinMiter);
    XDrawLine(surf->dpy, surf->drawable, surf->gc, (int)x0, (int)y0, (int)x1, (int)y1);
    XSetLineAttributes(surf->dpy, surf->gc, 1, LineSolid, CapButt, JoinMiter);
}

static void
nk_xsurf_stroke_rect(XSurface* surf, short x, short y, unsigned short w,
    unsigned short h, unsigned short r, unsigned short line_thickness, struct nk_color col)
{
    unsigned long c = nk_color_from_byte(&col.r);
    XSetForeground(surf->dpy, surf->gc, c);
    XSetLineAttributes(surf->dpy, surf->gc, line_thickness, LineSolid, CapButt, JoinMiter);
    if (r == 0) {
        XDrawRectangle(surf->dpy, surf->drawable, surf->gc, x, y, w, h);
    } else {
        short xc = x + r;
        short yc = y + r;
        short wc = (short)(w - 2 * r);
        short hc = (short)(h - 2 * r);

        XDrawLine(surf->dpy, surf->drawable, surf->gc, xc, y, xc+wc, y);
        XDrawLine(surf->dpy, surf->drawable, surf->gc, x+w, yc, x+w, yc+hc);
        XDrawLine(surf->dpy, surf->drawable, surf->gc, xc, y+h, xc+wc, y+h);
        XDrawLine(surf->dpy, surf->drawable, surf->gc, x, yc, x, yc+hc);

        XDrawArc(surf->dpy, surf->drawable, surf->gc, xc + wc - r, y,
            (unsigned)r*2, (unsigned)r*2, 0 * 64, 90 * 64);
        XDrawArc(surf->dpy, surf->drawable, surf->gc, x, y,
            (unsigned)r*2, (unsigned)r*2, 90 * 64, 90 * 64);
        XDrawArc(surf->dpy, surf->drawable, surf->gc, x, yc + hc - r,
            (unsigned)r*2, (unsigned)2*r, 180 * 64, 90 * 64);
        XDrawArc(surf->dpy, surf->drawable, surf->gc, xc + wc - r, yc + hc - r,
            (unsigned)r*2, (unsigned)2*r, -90 * 64, 90 * 64);
    }
    XSetLineAttributes(surf->dpy, surf->gc, 1, LineSolid, CapButt, JoinMiter);
}

static void
nk_xsurf_fill_rect(XSurface* surf, short x, short y, unsigned short w,
    unsigned short h, unsigned short r, struct nk_color col)
{
    unsigned long c = nk_color_from_byte(&col.r);
    XSetForeground(surf->dpy, surf->gc, c);
    if (r == 0) {
        XFillRectangle(surf->dpy, surf->drawable, surf->gc, x, y, w, h);
    } else {
        short xc = x + r;
        short yc = y + r;
        short wc = (short)(w - 2 * r);
        short hc = (short)(h - 2 * r);

        XPoint pnts[12];
        pnts[0].x = x;
        pnts[0].y = yc;
        pnts[1].x = xc;
        pnts[1].y = yc;
        pnts[2].x = xc;
        pnts[2].y = y;

        pnts[3].x = xc + wc;
        pnts[3].y = y;
        pnts[4].x = xc + wc;
        pnts[4].y = yc;
        pnts[5].x = x + w;
        pnts[5].y = yc;

        pnts[6].x = x + w;
        pnts[6].y = yc + hc;
        pnts[7].x = xc + wc;
        pnts[7].y = yc + hc;
        pnts[8].x = xc + wc;
        pnts[8].y = y + h;

        pnts[9].x = xc;
        pnts[9].y = y + h;
        pnts[10].x = xc;
        pnts[10].y = yc + hc;
        pnts[11].x = x;
        pnts[11].y = yc + hc;

        XFillPolygon(surf->dpy, surf->drawable, surf->gc, pnts, 12, Convex, CoordModeOrigin);
        XFillArc(surf->dpy, surf->drawable, surf->gc, xc + wc - r, y,
            (unsigned)r*2, (unsigned)r*2, 0 * 64, 90 * 64);
        XFillArc(surf->dpy, surf->drawable, surf->gc, x, y,
            (unsigned)r*2, (unsigned)r*2, 90 * 64, 90 * 64);
        XFillArc(surf->dpy, surf->drawable, surf->gc, x, yc + hc - r,
            (unsigned)r*2, (unsigned)2*r, 180 * 64, 90 * 64);
        XFillArc(surf->dpy, surf->drawable, surf->gc, xc + wc - r, yc + hc - r,
            (unsigned)r*2, (unsigned)2*r, -90 * 64, 90 * 64);
    }
}

static void
nk_xsurf_fill_triangle(XSurface *surf, short x0, short y0, short x1,
    short y1, short x2, short y2, struct nk_color col)
{
    XPoint pnts[3];
    unsigned long c = nk_color_from_byte(&col.r);
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
nk_xsurf_stroke_triangle(XSurface *surf, short x0, short y0, short x1,
    short y1, short x2, short y2, unsigned short line_thickness, struct nk_color col)
{
    XPoint pnts[3];
    unsigned long c = nk_color_from_byte(&col.r);
    XSetForeground(surf->dpy, surf->gc, c);
    XSetLineAttributes(surf->dpy, surf->gc, line_thickness, LineSolid, CapButt, JoinMiter);
    XDrawLine(surf->dpy, surf->drawable, surf->gc, x0, y0, x1, y1);
    XDrawLine(surf->dpy, surf->drawable, surf->gc, x1, y1, x2, y2);
    XDrawLine(surf->dpy, surf->drawable, surf->gc, x2, y2, x0, y0);
    XSetLineAttributes(surf->dpy, surf->gc, 1, LineSolid, CapButt, JoinMiter);
}

static void
nk_xsurf_fill_polygon(XSurface *surf,  const struct nk_vec2i *pnts, int count,
    struct nk_color col)
{
    int i = 0;
    #define MAX_POINTS 64
    XPoint xpnts[MAX_POINTS];
    unsigned long c = nk_color_from_byte(&col.r);
    XSetForeground(surf->dpy, surf->gc, c);
    for (i = 0; i < count && i < MAX_POINTS; ++i) {
        xpnts[i].x = pnts[i].x;
        xpnts[i].y = pnts[i].y;
    }
    XFillPolygon(surf->dpy, surf->drawable, surf->gc, xpnts, count, Convex, CoordModeOrigin);
    #undef MAX_POINTS
}

static void
nk_xsurf_stroke_polygon(XSurface *surf, const struct nk_vec2i *pnts, int count,
    unsigned short line_thickness, struct nk_color col)
{
    int i = 0;
    unsigned long c = nk_color_from_byte(&col.r);
    XSetForeground(surf->dpy, surf->gc, c);
    XSetLineAttributes(surf->dpy, surf->gc, line_thickness, LineSolid, CapButt, JoinMiter);
    for (i = 1; i < count; ++i)
        XDrawLine(surf->dpy, surf->drawable, surf->gc, pnts[i-1].x, pnts[i-1].y, pnts[i].x, pnts[i].y);
    XDrawLine(surf->dpy, surf->drawable, surf->gc, pnts[count-1].x, pnts[count-1].y, pnts[0].x, pnts[0].y);
    XSetLineAttributes(surf->dpy, surf->gc, 1, LineSolid, CapButt, JoinMiter);
}

static void
nk_xsurf_stroke_polyline(XSurface *surf, const struct nk_vec2i *pnts,
    int count, unsigned short line_thickness, struct nk_color col)
{
    int i = 0;
    unsigned long c = nk_color_from_byte(&col.r);
    XSetLineAttributes(surf->dpy, surf->gc, line_thickness, LineSolid, CapButt, JoinMiter);
    XSetForeground(surf->dpy, surf->gc, c);
    for (i = 0; i < count-1; ++i)
        XDrawLine(surf->dpy, surf->drawable, surf->gc, pnts[i].x, pnts[i].y, pnts[i+1].x, pnts[i+1].y);
    XSetLineAttributes(surf->dpy, surf->gc, 1, LineSolid, CapButt, JoinMiter);
}

static void
nk_xsurf_fill_circle(XSurface *surf, short x, short y, unsigned short w,
    unsigned short h, struct nk_color col)
{
    unsigned long c = nk_color_from_byte(&col.r);
    XSetForeground(surf->dpy, surf->gc, c);
    XFillArc(surf->dpy, surf->drawable, surf->gc, (int)x, (int)y,
        (unsigned)w, (unsigned)h, 0, 360 * 64);
}

static void
nk_xsurf_stroke_circle(XSurface *surf, short x, short y, unsigned short w,
    unsigned short h, unsigned short line_thickness, struct nk_color col)
{
    unsigned long c = nk_color_from_byte(&col.r);
    XSetLineAttributes(surf->dpy, surf->gc, line_thickness, LineSolid, CapButt, JoinMiter);
    XSetForeground(surf->dpy, surf->gc, c);
    XDrawArc(surf->dpy, surf->drawable, surf->gc, (int)x, (int)y,
        (unsigned)w, (unsigned)h, 0, 360 * 64);
    XSetLineAttributes(surf->dpy, surf->gc, 1, LineSolid, CapButt, JoinMiter);
}

static void
nk_xsurf_stroke_curve(XSurface *surf, struct nk_vec2i p1,
    struct nk_vec2i p2, struct nk_vec2i p3, struct nk_vec2i p4,
    unsigned int num_segments, unsigned short line_thickness, struct nk_color col)
{
    unsigned int i_step;
    float t_step;
    struct nk_vec2i last = p1;

    XSetLineAttributes(surf->dpy, surf->gc, line_thickness, LineSolid, CapButt, JoinMiter);
    num_segments = MAX(num_segments, 1);
    t_step = 1.0f/(float)num_segments;
    for (i_step = 1; i_step <= num_segments; ++i_step) {
        float t = t_step * (float)i_step;
        float u = 1.0f - t;
        float w1 = u*u*u;
        float w2 = 3*u*u*t;
        float w3 = 3*u*t*t;
        float w4 = t * t *t;
        float x = w1 * p1.x + w2 * p2.x + w3 * p3.x + w4 * p4.x;
        float y = w1 * p1.y + w2 * p2.y + w3 * p3.y + w4 * p4.y;
        nk_xsurf_stroke_line(surf, last.x, last.y, (short)x, (short)y, line_thickness,col);
        last.x = (short)x; last.y = (short)y;
    }
    XSetLineAttributes(surf->dpy, surf->gc, 1, LineSolid, CapButt, JoinMiter);
}

static void
nk_xsurf_draw_text(XSurface *surf, short x, short y, unsigned short w, unsigned short h,
    const char *text, int len, XFont *font, struct nk_color cbg, struct nk_color cfg)
{
    int tx, ty;
    unsigned long bg = nk_color_from_byte(&cbg.r);
    unsigned long fg = nk_color_from_byte(&cfg.r);

    XSetForeground(surf->dpy, surf->gc, bg);
    XFillRectangle(surf->dpy, surf->drawable, surf->gc, (int)x, (int)y, (unsigned)w, (unsigned)h);
    if(!text || !font || !len) return;

    tx = (int)x;
    ty = (int)y + font->ascent;
    XSetForeground(surf->dpy, surf->gc, fg);
    if(font->set)
        XmbDrawString(surf->dpy,surf->drawable,font->set,surf->gc,tx,ty,(const char*)text,(int)len);
    else
        XDrawString(surf->dpy, surf->drawable, surf->gc, tx, ty, (const char*)text, (int)len);
}

static void
nk_xsurf_clear(XSurface *surf, unsigned long color)
{
    XSetForeground(surf->dpy, surf->gc, color);
    XFillRectangle(surf->dpy, surf->drawable, surf->gc, 0, 0, surf->w, surf->h);
}

static void
nk_xsurf_blit(Drawable target, XSurface *surf, unsigned int w, unsigned int h)
{
    XCopyArea(surf->dpy, surf->drawable, target, surf->gc, 0, 0, w, h, 0, 0);
}

static void
nk_xsurf_del(XSurface *surf)
{
    XFreePixmap(surf->dpy, surf->drawable);
    XFreeGC(surf->dpy, surf->gc);
    free(surf);
}

XFont*
nk_xfont_create(Display *dpy, const char *name)
{
    int n;
    char *def, **missing;
    XFont *font = (XFont*)calloc(1, sizeof(XFont));
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
        && !(font->xfont = XLoadQueryFont(dpy, "fixed"))) {
            free(font);
            return 0;
        }
        font->ascent = font->xfont->ascent;
        font->descent = font->xfont->descent;
    }
    font->height = font->ascent + font->descent;
    return font;
}

static float
nk_xfont_get_text_width(nk_handle handle, float height, const char *text, int len)
{
    XFont *font = (XFont*)handle.ptr;
    XRectangle r;
    if(!font || !text)
        return 0;

    if(font->set) {
        XmbTextExtents(font->set, (const char*)text, len, NULL, &r);
        return (float)r.width;
    } else{
        int w = XTextWidth(font->xfont, (const char*)text, len);
        return (float)w;
    }
}

void
nk_xfont_del(Display *dpy, XFont *font)
{
    if(!font) return;
    if(font->set)
        XFreeFontSet(dpy, font->set);
    else
        XFreeFont(dpy, font->xfont);
    free(font);
}

NK_API struct nk_context*
nk_xlib_init(XFont *xfont, Display *dpy, int screen, Window root,
    unsigned int w, unsigned int h)
{
    struct nk_user_font *font = &xfont->handle;
    font->userdata = nk_handle_ptr(xfont);
    font->height = (float)xfont->height;
    font->width = nk_xfont_get_text_width;
    xlib.dpy = dpy;
    xlib.root = root;

    if (!setlocale(LC_ALL,"")) return 0;
    if (!XSupportsLocale()) return 0;
    if (!XSetLocaleModifiers("@im=none")) return 0;

    /* create invisible cursor */
    {static XColor dummy; char data[1] = {0};
    Pixmap blank = XCreateBitmapFromData(dpy, root, data, 1, 1);
    if (blank == None) return 0;
    xlib.cursor = XCreatePixmapCursor(dpy, blank, blank, &dummy, &dummy, 0, 0);
    XFreePixmap(dpy, blank);}

    xlib.surf = nk_xsurf_create(screen, w, h);
    nk_init_default(&xlib.ctx, font);
    return &xlib.ctx;
}

NK_API void
nk_xlib_set_font(XFont *xfont)
{
    struct nk_user_font *font = &xfont->handle;
    font->userdata = nk_handle_ptr(xfont);
    font->height = (float)xfont->height;
    font->width = nk_xfont_get_text_width;
    nk_style_set_font(&xlib.ctx, font);
}

NK_API int
nk_xlib_handle_event(Display *dpy, int screen, Window win, XEvent *evt)
{
    struct nk_context *ctx = &xlib.ctx;

    /* optional grabbing behavior */
    if (ctx->input.mouse.grab) {
        XDefineCursor(xlib.dpy, xlib.root, xlib.cursor);
        ctx->input.mouse.grab = 0;
    } else if (ctx->input.mouse.ungrab) {
        XWarpPointer(xlib.dpy, None, xlib.root, 0, 0, 0, 0,
            (int)ctx->input.mouse.prev.x, (int)ctx->input.mouse.prev.y);
        XUndefineCursor(xlib.dpy, xlib.root);
        ctx->input.mouse.ungrab = 0;
    }

    if (evt->type == KeyPress || evt->type == KeyRelease)
    {
        /* Key handler */
        int ret, down = (evt->type == KeyPress);
        KeySym *code = XGetKeyboardMapping(xlib.surf->dpy, (KeyCode)evt->xkey.keycode, 1, &ret);
        if (*code == XK_Shift_L || *code == XK_Shift_R) nk_input_key(ctx, NK_KEY_SHIFT, down);
        else if (*code == XK_Delete)    nk_input_key(ctx, NK_KEY_DEL, down);
        else if (*code == XK_Return)    nk_input_key(ctx, NK_KEY_ENTER, down);
        else if (*code == XK_Tab)       nk_input_key(ctx, NK_KEY_TAB, down);
        else if (*code == XK_Left)      nk_input_key(ctx, NK_KEY_LEFT, down);
        else if (*code == XK_Right)     nk_input_key(ctx, NK_KEY_RIGHT, down);
        else if (*code == XK_Up)        nk_input_key(ctx, NK_KEY_UP, down);
        else if (*code == XK_Down)      nk_input_key(ctx, NK_KEY_DOWN, down);
        else if (*code == XK_BackSpace) nk_input_key(ctx, NK_KEY_BACKSPACE, down);
        else if (*code == XK_Escape)    nk_input_key(ctx, NK_KEY_TEXT_RESET_MODE, down);
        else if (*code == XK_Page_Up)   nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
        else if (*code == XK_Page_Down) nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
        else if (*code == XK_Home) {
            nk_input_key(ctx, NK_KEY_TEXT_START, down);
            nk_input_key(ctx, NK_KEY_SCROLL_START, down);
        } else if (*code == XK_End) {
            nk_input_key(ctx, NK_KEY_TEXT_END, down);
            nk_input_key(ctx, NK_KEY_SCROLL_END, down);
        } else {
            if (*code == 'c' && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_COPY, down);
            else if (*code == 'v' && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_PASTE, down);
            else if (*code == 'x' && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_CUT, down);
            else if (*code == 'z' && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_TEXT_UNDO, down);
            else if (*code == 'r' && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_TEXT_REDO, down);
            else if (*code == XK_Left && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
            else if (*code == XK_Right && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
            else if (*code == 'b' && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down);
            else if (*code == 'e' && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down);
            else {
                if (*code == 'i')
                    nk_input_key(ctx, NK_KEY_TEXT_INSERT_MODE, down);
                else if (*code == 'r')
                    nk_input_key(ctx, NK_KEY_TEXT_REPLACE_MODE, down);
                if (down) {
                    char buf[32];
                    KeySym keysym = 0;
                    if (XLookupString((XKeyEvent*)evt, buf, 32, &keysym, NULL) != NoSymbol)
                        nk_input_glyph(ctx, buf);
                }
            }
        }
        XFree(code);
        return 1;
    } else if (evt->type == ButtonPress || evt->type == ButtonRelease) {
        /* Button handler */
        int down = (evt->type == ButtonPress);
        const int x = evt->xbutton.x, y = evt->xbutton.y;
        if (evt->xbutton.button == Button1)
            nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
        if (evt->xbutton.button == Button2)
            nk_input_button(ctx, NK_BUTTON_MIDDLE, x, y, down);
        else if (evt->xbutton.button == Button3)
            nk_input_button(ctx, NK_BUTTON_RIGHT, x, y, down);
        else if (evt->xbutton.button == Button4)
            nk_input_scroll(ctx, 1.0f);
        else if (evt->xbutton.button == Button5)
            nk_input_scroll(ctx, -1.0f);
        else return 0;
        return 1;
    } else if (evt->type == MotionNotify) {
        /* Mouse motion handler */
        const int x = evt->xmotion.x, y = evt->xmotion.y;
        nk_input_motion(ctx, x, y);
        if (ctx->input.mouse.grabbed) {
            ctx->input.mouse.pos.x = ctx->input.mouse.prev.x;
            ctx->input.mouse.pos.y = ctx->input.mouse.prev.y;
            XWarpPointer(xlib.dpy, None, xlib.surf->root, 0, 0, 0, 0, (int)ctx->input.mouse.pos.x, (int)ctx->input.mouse.pos.y);
        }
        return 1;
    } else if (evt->type == Expose || evt->type == ConfigureNotify) {
        /* Window resize handler */
        unsigned int width, height;
        XWindowAttributes attr;
        XGetWindowAttributes(dpy, win, &attr);
        width = (unsigned int)attr.width;
        height = (unsigned int)attr.height;
        nk_xsurf_resize(xlib.surf, width, height);
        return 1;
    } else if (evt->type == KeymapNotify) {
        XRefreshKeyboardMapping(&evt->xmapping);
        return 1;
    }
    return 0;
}

NK_API void
nk_xlib_shutdown(void)
{
    nk_xsurf_del(xlib.surf);
    nk_free(&xlib.ctx);
    XFreeCursor(xlib.dpy, xlib.cursor);
    nk_memset(&xlib, 0, sizeof(xlib));
}

NK_API void
nk_xlib_render(Drawable screen, struct nk_color clear)
{
    const struct nk_command *cmd;
    struct nk_context *ctx = &xlib.ctx;
    XSurface *surf = xlib.surf;

    nk_xsurf_clear(xlib.surf, nk_color_from_byte(&clear.r));
    nk_foreach(cmd, &xlib.ctx)
    {
        switch (cmd->type) {
        case NK_COMMAND_NOP: break;
        case NK_COMMAND_SCISSOR: {
            const struct nk_command_scissor *s =(const struct nk_command_scissor*)cmd;
            nk_xsurf_scissor(surf, s->x, s->y, s->w, s->h);
        } break;
        case NK_COMMAND_LINE: {
            const struct nk_command_line *l = (const struct nk_command_line *)cmd;
            nk_xsurf_stroke_line(surf, l->begin.x, l->begin.y, l->end.x,
                l->end.y, l->line_thickness, l->color);
        } break;
        case NK_COMMAND_RECT: {
            const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
            nk_xsurf_stroke_rect(surf, r->x, r->y, r->w, r->h,
                (unsigned short)r->rounding, r->line_thickness, r->color);
        } break;
        case NK_COMMAND_RECT_FILLED: {
            const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled *)cmd;
            nk_xsurf_fill_rect(surf, r->x, r->y, r->w, r->h,
                (unsigned short)r->rounding, r->color);
        } break;
        case NK_COMMAND_CIRCLE: {
            const struct nk_command_circle *c = (const struct nk_command_circle *)cmd;
            nk_xsurf_stroke_circle(surf, c->x, c->y, c->w, c->h, c->line_thickness, c->color);
        } break;
        case NK_COMMAND_CIRCLE_FILLED: {
            const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;
            nk_xsurf_fill_circle(surf, c->x, c->y, c->w, c->h, c->color);
        } break;
        case NK_COMMAND_TRIANGLE: {
            const struct nk_command_triangle*t = (const struct nk_command_triangle*)cmd;
            nk_xsurf_stroke_triangle(surf, t->a.x, t->a.y, t->b.x, t->b.y,
                t->c.x, t->c.y, t->line_thickness, t->color);
        } break;
        case NK_COMMAND_TRIANGLE_FILLED: {
            const struct nk_command_triangle_filled *t = (const struct nk_command_triangle_filled *)cmd;
            nk_xsurf_fill_triangle(surf, t->a.x, t->a.y, t->b.x, t->b.y,
                t->c.x, t->c.y, t->color);
        } break;
        case NK_COMMAND_POLYGON: {
            const struct nk_command_polygon *p =(const struct nk_command_polygon*)cmd;
            nk_xsurf_stroke_polygon(surf, p->points, p->point_count, p->line_thickness,p->color);
        } break;
        case NK_COMMAND_POLYGON_FILLED: {
            const struct nk_command_polygon_filled *p = (const struct nk_command_polygon_filled *)cmd;
            nk_xsurf_fill_polygon(surf, p->points, p->point_count, p->color);
        } break;
        case NK_COMMAND_POLYLINE: {
            const struct nk_command_polyline *p = (const struct nk_command_polyline *)cmd;
            nk_xsurf_stroke_polyline(surf, p->points, p->point_count, p->line_thickness, p->color);
        } break;
        case NK_COMMAND_TEXT: {
            const struct nk_command_text *t = (const struct nk_command_text*)cmd;
            nk_xsurf_draw_text(surf, t->x, t->y, t->w, t->h,
                (const char*)t->string, t->length,
                (XFont*)t->font->userdata.ptr,
                t->background, t->foreground);
        } break;
        case NK_COMMAND_CURVE: {
            const struct nk_command_curve *q = (const struct nk_command_curve *)cmd;
            nk_xsurf_stroke_curve(surf, q->begin, q->ctrl[0], q->ctrl[1],
                q->end, 22, q->line_thickness, q->color);
        } break;
        case NK_COMMAND_RECT_MULTI_COLOR:
        case NK_COMMAND_IMAGE:
        case NK_COMMAND_ARC:
        case NK_COMMAND_ARC_FILLED:
        default: break;
        }
    }
    nk_clear(ctx);
    nk_xsurf_blit(screen, surf, surf->w, surf->h);
}
#endif
