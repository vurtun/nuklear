#ifndef NK_GDI_H_
#define NK_GDI_H_

#include "../../nuklear.h"

typedef struct GdipFont GdipFont;

NK_API struct nk_context* nk_gdip_init(HWND hwnd, unsigned int width, unsigned int height);
NK_API void nk_gdip_set_font(GdipFont *font);
NK_API int nk_gdip_handle_event(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
NK_API void nk_gdip_render(enum nk_anti_aliasing AA, struct nk_color clear);
NK_API void nk_gdip_shutdown(void);

/* font */
NK_API GdipFont* nk_gdipfont_create(const char *name, int size);
NK_API void nk_gdipfont_del(GdipFont *font);

#endif
