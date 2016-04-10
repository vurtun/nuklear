#ifndef NK_ALLEGRO_H_
#define NK_ALLEGRO_H_

#include "../../nuklear.h"

NK_API struct nk_context* nk_allegro_init(ALLEGRO_DISPLAY *win, int max_vertex_memory, int max_element_memory);
NK_API void nk_allegro_font_stash_begin(struct nk_font_atlas **atlas);
NK_API void nk_allegro_font_stash_end(void);

NK_API void nk_allegro_handle_event(ALLEGRO_EVENT *evt);
NK_API void nk_allegro_render(enum nk_anti_aliasing);
NK_API void nk_allegro_shutdown(void);

NK_API void nk_allegro_device_destroy(void);
NK_API void nk_allegro_device_create(int max_vertex_memory, int max_elemnt_memory);

#endif
