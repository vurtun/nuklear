#ifndef NK_XLIB_H_
#define NK_XLIB_H_

#include "../../nuklear.h"

typedef struct XFont XFont;
NK_API struct nk_context* nk_xlib_init(XFont *font, Display *dpy, int screen, Window root, unsigned int w, unsigned int h);
NK_API void nk_xlib_set_font(XFont *font);
NK_API void nk_xlib_handle_event(Display *dpy, int screen, Window win, XEvent *evt);
NK_API void nk_xlib_render(Drawable screen, struct nk_color clear);
NK_API void nk_xlib_shutdown(void);

/* font */
NK_API XFont* nk_xfont_create(Display *dpy, const char *name);
NK_API void nk_xfont_del(Display *dpy, XFont *font);

#endif
