#ifndef NK_SDL_H_
#define NK_SDL_H_

#include "../../nuklear.h"

#include <SDL/SDL.h>

typedef struct nk_sdl_Font nk_sdl_Font;

NK_API struct nk_context *nk_sdl_init(SDL_Surface *screen_surface);
NK_API void nk_sdl_handle_event(SDL_Event *evt);
NK_API void nk_gdi_render(struct nk_color clear);
NK_API void nk_sdl_shutdown(void);

#endif
