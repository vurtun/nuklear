#ifndef NK_GDI_H_
#define NK_GDI_H_

#include "../../nuklear.h"

typedef struct GdiFont GdiFont;

NK_API struct nk_context* nk_gdi_init(GdiFont *font, HDC window_dc, unsigned int width, unsigned int height);
NK_API void nk_gdi_set_font(GdiFont *font);
NK_API int nk_gdi_handle_event(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
NK_API void nk_gdi_render(struct nk_color clear);
NK_API void nk_gdi_shutdown(void);

/* font */
NK_API GdiFont* nk_gdifont_create(const char *name, int size);
NK_API void nk_gdifont_del(GdiFont *font);

#endif
