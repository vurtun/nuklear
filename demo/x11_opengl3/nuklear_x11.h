#ifndef NK_X11_OPENGL_H_
#define NK_X11_OPENGL_H_

#include "../../nuklear.h"

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>

/* Define this to 0 if you already load all required OpenGL extensions yourself
 * otherwise define to 1 to let nuklear load its required extensions. */
#define NK_X11_LOAD_OPENGL_EXTENSIONS 1

NK_API struct nk_context *nk_x11_init(Display *dpy, Window win);
NK_API void nk_x11_font_stash_begin(struct nk_font_atlas **atlas);
NK_API void nk_x11_font_stash_end(void);
NK_API void nk_x11_handle_event(XEvent *evt);
NK_API void nk_x11_render(enum nk_anti_aliasing, int max_vertex_buffer, int max_element_buffer);
NK_API void nk_x11_shutdown(void);

NK_API int nk_x11_device_create(void);
NK_API void nk_x11_device_destroy(void);


#endif
