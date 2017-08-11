/*
 * Nuklear - 1.32.0 - public domain
 * no warrenty implied; use at your own risk.
 * authored from 2017 by Shevtcov Vyacheslav
 */
/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */
#ifndef NK_SDL_SOFT_H_
#define NK_SDL_SOFT_H_
#include <SDL.h>
#include <SDL_image.h>

#include <SDL_ttf.h>

NK_API struct nk_context *  nk_sdl_soft_init(SDL_Renderer *ren); //1
NK_API int                  nk_sdl_handle_event(SDL_Event *evt); //1
NK_API void                 nk_sdl_soft_render();//1
NK_API void                 nk_sdl_soft_shutdown(void);//1
NK_API void                 nk_sdl_soft_destroy(void);//1
NK_API void                 nk_sdl_font_stash_end(void);
/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#endif
#ifdef NK_SDL_SOFT_IMPLEMENTATION

#include <string.h>


static struct {
	SDL_Renderer * ren;
	SDL_Texture * font_tex;
	struct nk_draw_null_texture null;
	struct nk_user_font *font;
	struct nk_buffer cmds;
    struct nk_context ctx;
	struct nk_font_atlas atlas;
} sdl_soft;

NK_API void
nk_sdl_soft_destroy(void)
{
	nk_buffer_free(&sdl_soft.cmds);
};


NK_API void
nk_sdl_soft_render()
{
	/* convert from command queue into draw list and draw to screen */
	const struct nk_command *cmd;
	nk_foreach(cmd, &sdl_soft.ctx)
	{
		switch (cmd->type) {
		case NK_COMMAND_NOP: break;
		case NK_COMMAND_SCISSOR: {
			const struct nk_command_scissor *s = (const struct nk_command_scissor*)cmd;
		//need procedure
		} break;
		case NK_COMMAND_LINE: {
			const struct nk_command_line *l = (const struct nk_command_line *)cmd;
			SDL_SetRenderDrawColor(sdl_soft.ren, l->color.r, l->color.g, l->color.b, l->color.a);
			SDL_RenderDrawLine(sdl_soft.ren, l->begin.x, l->begin.y, l->end.x, l->end.y);
			if (l->begin.x == l->end.x) {
				for (short i = 1; i < l->line_thickness; ++i) {
					SDL_RenderDrawLine(sdl_soft.ren, l->begin.x + i, l->begin.y, l->end.x + i, l->end.y);
					SDL_RenderDrawLine(sdl_soft.ren, l->begin.x - i, l->begin.y, l->end.x - i, l->end.y);
				}
			}
			else {
				for (short i = 1; i < l->line_thickness; ++i) {
					SDL_RenderDrawLine(sdl_soft.ren, l->begin.x, l->begin.y -+ i, l->end.x, l->end.y - i);
					SDL_RenderDrawLine(sdl_soft.ren, l->begin.x, l->begin.y+i , l->end.x , l->end.y+i);
				}
			}
		} break;
		case NK_COMMAND_RECT: {
			const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
			SDL_Rect rect;
			rect.x = r->x;
			rect.	y = r->y;
			rect.w = r->w;
			rect.h = r->h;
			SDL_SetRenderDrawColor(sdl_soft.ren, r->color.r, r->color.g, r->color.b, r->color.a);
			SDL_RenderDrawRect(sdl_soft.ren, &rect);
		} break;
		case NK_COMMAND_RECT_FILLED: {
			const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled *)cmd;
			SDL_Rect rect;
			rect.x = r->x;
			rect.y = r->y;
			rect.w = r->w;
			rect.h = r->h;
			SDL_SetRenderDrawColor(sdl_soft.ren, r->color.r, r->color.g, r->color.b, r->color.a);
			SDL_RenderFillRect(sdl_soft.ren, &rect);
		} break;
		case NK_COMMAND_CIRCLE: {
			const struct nk_command_circle *c = (const struct nk_command_circle *)cmd;
			int x = c->x+ c->w / 2;
			int y = c->y + c->h / 2;
			int rx = c->w/2;
			int ry = c->h / 2;
			int ix, iy;
			int h, i, j, k;
			int oh, oi, oj, ok;
			int xmh, xph, ypk, ymk;
			int xmi, xpi, ymj, ypj;
			int xmj, xpj, ymi, ypi;
			int xmk, xpk, ymh, yph;
			SDL_SetRenderDrawColor(sdl_soft.ren, c->color.r, c->color.g, c->color.b, c->color.a);

			/*
			* Sanity check radii
			*/
			if ((rx < 0) || (ry < 0)) {
				break;
			}
			/*
			* Special case for rx=0 - draw a vline
			*/
			if (rx == 0) {
				SDL_RenderDrawLine(sdl_soft.ren, x, y - ry, x, y + ry);
			}
			/*
			* Special case for ry=0 - draw a hline
			*/
			if (ry == 0) {
				SDL_RenderDrawLine(sdl_soft.ren, x - rx, y, x + rx, y);
			}
			/*
			* Init vars
			*/
			oh = oi = oj = ok = 0xFFFF;
			/*
			* Draw
			*/
			if (rx > ry) {
				ix = 0;
				iy = rx * 64;
				do {
					h = (ix + 32) >> 6;
					i = (iy + 32) >> 6;
					j = (h * ry) / rx;
					k = (i * ry) / rx;
					if (((ok != k) && (oj != k)) || ((oj != j) && (ok != j)) || (k != j)) {
						xph = x + h;
						xmh = x - h;
						if (k > 0) {
							ypk = y + k;
							ymk = y - k;
							SDL_RenderDrawPoint(sdl_soft.ren, xmh, ypk);
							SDL_RenderDrawPoint(sdl_soft.ren, xph, ypk);
							SDL_RenderDrawPoint(sdl_soft.ren, xmh, ymk);
							SDL_RenderDrawPoint(sdl_soft.ren, xph, ymk);

						}
						else {
							SDL_RenderDrawPoint(sdl_soft.ren, xmh, y);
							SDL_RenderDrawPoint(sdl_soft.ren, xph, y);
						}
						ok = k;
						xpi = x + i;
						xmi = x - i;
						if (j > 0) {
							ypj = y + j;
							ymj = y - j;
							SDL_RenderDrawPoint(sdl_soft.ren, xmi, ypj);
							SDL_RenderDrawPoint(sdl_soft.ren, xpi, ypj);
							SDL_RenderDrawPoint(sdl_soft.ren, xmi, ymj);
							SDL_RenderDrawPoint(sdl_soft.ren, xpi, ymj);
						}
						else {
							SDL_RenderDrawPoint(sdl_soft.ren, xmi, y);
							SDL_RenderDrawPoint(sdl_soft.ren, xpi, y);
						}
						oj = j;
					}
					ix = ix + iy / rx;
					iy = iy - ix / rx;
				} while (i > h);

			}
			else {
				ix = 0;
				iy = ry * 64;

				do {
					h = (ix + 32) >> 6;
					i = (iy + 32) >> 6;
					j = (h * rx) / ry;
					k = (i * rx) / ry;

					if (((oi != i) && (oh != i)) || ((oh != h) && (oi != h) && (i != h))) {
						xmj = x - j;
						xpj = x + j;
						if (i > 0) {
							ypi = y + i;
							ymi = y - i;
							SDL_RenderDrawPoint(sdl_soft.ren, xmj, ypi);
							SDL_RenderDrawPoint(sdl_soft.ren, xpj, ypi);
							SDL_RenderDrawPoint(sdl_soft.ren, xmj, ymi);
							SDL_RenderDrawPoint(sdl_soft.ren, xpj, ymi);

						}
						else {
							SDL_RenderDrawPoint(sdl_soft.ren, xmj, y);
							SDL_RenderDrawPoint(sdl_soft.ren, xpj, y);
						}
						oi = i;
						xmk = x - k;
						xpk = x + k;
						if (h > 0) {
							yph = y + h;
							ymh = y - h;
							SDL_RenderDrawPoint(sdl_soft.ren, xmk, yph);
							SDL_RenderDrawPoint(sdl_soft.ren, xpk, yph);
							SDL_RenderDrawPoint(sdl_soft.ren, xmk, ymh);
							SDL_RenderDrawPoint(sdl_soft.ren, xpk, ymh);
						}
						else {
							SDL_RenderDrawPoint(sdl_soft.ren, xmk, y);
							SDL_RenderDrawPoint(sdl_soft.ren, xpk, y);
						}
						oh = h;
					}
					ix = ix + iy / ry;
					iy = iy - ix / ry;

				} while (i > h);

			}

		} break;
		case NK_COMMAND_CIRCLE_FILLED: {
			const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;

			int x = c->x + c->w / 2;
			int y = c->y + c->h / 2;
			int rx = c->w / 2;
			int ry = c->h / 2;
			int ix, iy;
			int h, i, j, k;
			int oh, oi, oj, ok;
			int xmh, xph;
			int xmi, xpi;
			int xmj, xpj;
			int xmk, xpk;

			SDL_SetRenderDrawColor(sdl_soft.ren, c->color.r, c->color.g, c->color.b, c->color.a);
			/*
			* Sanity check radii
			*/
			if ((rx < 0) || (ry < 0)) {
				break;
			}

			/*
			* Special case for rx=0 - draw a vline
			*/
			if (rx == 0) {
				SDL_RenderDrawLine(sdl_soft.ren, x, y - ry, x, y + ry);
				break;
			}
			/*
			* Special case for ry=0 - draw a hline
			*/
			if (ry == 0) {
				SDL_RenderDrawLine(sdl_soft.ren, x - rx, y, x + rx, y);
				break;

			}


			/*
			* Init vars
			*/
			oh = oi = oj = ok = 0xFFFF;

			/*
			* Draw
			*/
			if (rx > ry) {
				ix = 0;
				iy = rx * 64;

				do {
					h = (ix + 32) >> 6;
					i = (iy + 32) >> 6;
					j = (h * ry) / rx;
					k = (i * ry) / rx;

					if ((ok != k) && (oj != k)) {
						xph = x + h;
						xmh = x - h;
						if (k > 0) {
							SDL_RenderDrawLine(sdl_soft.ren, xmh, y + k, xph, y + k);
							SDL_RenderDrawLine(sdl_soft.ren, xmh, y - k, xph, y - k);
						}
						else {
							SDL_RenderDrawLine(sdl_soft.ren, xmh, y, xph, y);
						}
						ok = k;
					}
					if ((oj != j) && (ok != j) && (k != j)) {
						xmi = x - i;
						xpi = x + i;
						if (j > 0) {
							SDL_RenderDrawLine(sdl_soft.ren, xmi, y + j, xpi, y + j);
							SDL_RenderDrawLine(sdl_soft.ren, xmi, y - j, xpi, y - j);
						}
						else {
							SDL_RenderDrawLine(sdl_soft.ren, xmi, y, xpi, y);
						}
						oj = j;
					}

					ix = ix + iy / rx;
					iy = iy - ix / rx;

				} while (i > h);
			}
			else {
				ix = 0;
				iy = ry * 64;

				do {
					h = (ix + 32) >> 6;
					i = (iy + 32) >> 6;
					j = (h * rx) / ry;
					k = (i * rx) / ry;

					if ((oi != i) && (oh != i)) {
						xmj = x - j;
						xpj = x + j;
						if (i > 0) {
							SDL_RenderDrawLine(sdl_soft.ren, xmj, y + i, xpj, y + i);
							SDL_RenderDrawLine(sdl_soft.ren, xmj, y - i, xpj, y - i);
						}
						else {
							SDL_RenderDrawLine(sdl_soft.ren, xmj, y, xpj, y);
						}
						oi = i;
					}
					if ((oh != h) && (oi != h) && (i != h)) {
						xmk = x - k;
						xpk = x + k;
						if (h > 0) {
							SDL_RenderDrawLine(sdl_soft.ren, xmk, y + h, xpk, y + h);
							SDL_RenderDrawLine(sdl_soft.ren, xmk, y - h, xpk, y - h);
						}
						else {
							SDL_RenderDrawLine(sdl_soft.ren, xmk, y, xpk, y);
						}
						oh = h;
					}

					ix = ix + iy / ry;
					iy = iy - ix / ry;

				} while (i > h);
			}
		} break;
		case NK_COMMAND_TRIANGLE: {
			const struct nk_command_triangle*t = (const struct nk_command_triangle*)cmd;			
			SDL_SetRenderDrawColor(sdl_soft.ren, t->color.r, t->color.g, t->color.b, t->color.a);
			SDL_RenderDrawLine(sdl_soft.ren, t->a.x, t->a.y, t->b.x, t->b.y);
			SDL_RenderDrawLine(sdl_soft.ren, t->b.x, t->b.y, t->c.x, t->c.y);
			SDL_RenderDrawLine(sdl_soft.ren, t->c.x, t->c.y, t->a.x, t->a.y);
		} break;
		case NK_COMMAND_TRIANGLE_FILLED: {
			const struct nk_command_triangle_filled *t = (const struct nk_command_triangle_filled *)cmd;
			//Replacement is required for FILL TRIANGLE
			SDL_SetRenderDrawColor(sdl_soft.ren, t->color.r, t->color.g, t->color.b, t->color.a);
			SDL_RenderDrawLine(sdl_soft.ren, t->a.x, t->a.y, t->b.x, t->b.y);
			SDL_RenderDrawLine(sdl_soft.ren, t->b.x, t->b.y, t->c.x, t->c.y);
			SDL_RenderDrawLine(sdl_soft.ren, t->c.x, t->c.y, t->a.x, t->a.y);
		} break;
		case NK_COMMAND_POLYGON: {
			const struct nk_command_polygon *p = (const struct nk_command_polygon*)cmd;
			//need procedure
		} break;
		case NK_COMMAND_POLYGON_FILLED: {
			const struct nk_command_polygon_filled *p = (const struct nk_command_polygon_filled *)cmd;
			//need procedure
		} break;
		case NK_COMMAND_POLYLINE: {
			const struct nk_command_polyline *p = (const struct nk_command_polyline *)cmd;
			//need procedure
		} break;
		case NK_COMMAND_TEXT: {
			const struct nk_command_text *t = (const struct nk_command_text*)cmd;
			//need help 
		} break;
		case NK_COMMAND_CURVE: {
			const struct nk_command_curve *q = (const struct nk_command_curve *)cmd;
			//need procedure

		} break;
		case NK_COMMAND_RECT_MULTI_COLOR: {
			//need procedure

		} break;

		case NK_COMMAND_IMAGE: {
			const struct nk_command_image *i = (const struct nk_command_image *)cmd;
			//need procedure
		} break;
		case NK_COMMAND_ARC: {
			//need procedure

		} break;

		case NK_COMMAND_ARC_FILLED: {
			//need procedure

		} break;
		default: break;
		}
	}


	nk_clear(&sdl_soft.ctx);
};

static void 
nk_sdl_clipbard_paste(nk_handle usr, struct nk_text_edit *edit)
{
    const char *text = SDL_GetClipboardText();
    if (text) nk_textedit_paste(edit, text, nk_strlen(text));
    (void)usr;
}

static void //1
nk_sdl_clipbard_copy(nk_handle usr, const char *text, int len)
{
    char *str = 0;
    (void)usr;
    if (!len) return;
    str = (char*)malloc((size_t)len+1);
    if (!str) return;
    memcpy(str, text, (size_t)len);
    str[len] = '\0';
    SDL_SetClipboardText(str);
    free(str);
}

NK_API struct nk_context*
nk_sdl_soft_init(SDL_Renderer *ren)
{
	SDL_Surface *surface;
	Uint32 rmask, gmask, bmask, amask;

	sdl_soft.ren = ren;
	nk_init_default(&sdl_soft.ctx, 0);
	nk_buffer_init_default(&sdl_soft.cmds);
	sdl_soft.ctx.clip.copy = nk_sdl_clipbard_copy;
	sdl_soft.ctx.clip.paste = nk_sdl_clipbard_paste;
	sdl_soft.ctx.clip.userdata = nk_handle_ptr(0);

	nk_font_atlas_init_default(&sdl_soft.atlas);
	nk_font_atlas_begin(&sdl_soft.atlas);
	//	nk_style_set_font(&sdl_soft.ctx, &sdl_soft.atlas.default_font->handle);
	sdl_soft.atlas.default_font = nk_font_atlas_add_default(&sdl_soft.atlas, 13.f, 0);


	// Step 2: bake default font
	
	int w, h;
	const void* image = nk_font_atlas_bake(&sdl_soft.atlas, &w, &h, NK_FONT_ATLAS_RGBA32);

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
		rmask = 0xff000000;
		gmask = 0x00ff0000;
		bmask = 0x0000ff00;
		amask = 0x000000ff;
#else
		rmask = 0x000000ff;
		gmask = 0x0000ff00;
		bmask = 0x00ff0000;
		amask = 0xff000000;
#endif
		surface = SDL_CreateRGBSurface(SDL_SWSURFACE, w, h, 32,
			rmask, gmask, bmask, amask);
		//		if (_atlas.default_font)
		//			nk_style_set_font(&_nk, &_atlas.default_font->handle);
		sdl_soft.font_tex = SDL_CreateTextureFromSurface(sdl_soft.ren, surface);
		nk_font_atlas_end(&sdl_soft.atlas, nk_handle_ptr(&sdl_soft.font_tex), &sdl_soft.null);
		nk_style_set_font(&sdl_soft.ctx, &sdl_soft.atlas.default_font->handle);
		

		return &sdl_soft.ctx;
	
}
NK_API void
nk_sdl_font_stash_begin(struct nk_font_atlas **atlas)
{
	nk_style_set_font(&sdl_soft.ctx, &sdl_soft.atlas.default_font->handle);

}


NK_API void
nk_sdl_font_stash_end(void)
{
//	const void *image; int w, h;
//	image = nk_font_atlas_bake(&sdl_soft.atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
//	SDL_Surface * surface =		SDL_CreateRGBSurfaceFrom(image,	w,	h,24,1, 0xff000000, 0xff000000, 0xff000000,0);
	
	//nk_sdl_device_upload_atlas(image, w, h);
//	nk_font_atlas_end(&sdl_soft.atlas, nk_handle_id((int)sdl_soft.font_tex), &sdl_soft.null);
//	nk_style_set_font(&sdl_soft.ctx, &sdl_soft.atlas.default_font->handle);

}



NK_API int
nk_sdl_handle_event(SDL_Event *evt)  //1
{
    struct nk_context *ctx = &sdl_soft.ctx;
    if (evt->type == SDL_KEYUP || evt->type == SDL_KEYDOWN) {
        /* key events */
        int down = evt->type == SDL_KEYDOWN;
        const Uint8* state = SDL_GetKeyboardState(0);
        SDL_Keycode sym = evt->key.keysym.sym;
        if (sym == SDLK_RSHIFT || sym == SDLK_LSHIFT)
            nk_input_key(ctx, NK_KEY_SHIFT, down);
        else if (sym == SDLK_DELETE)
            nk_input_key(ctx, NK_KEY_DEL, down);
        else if (sym == SDLK_RETURN)
            nk_input_key(ctx, NK_KEY_ENTER, down);
        else if (sym == SDLK_TAB)
            nk_input_key(ctx, NK_KEY_TAB, down);
        else if (sym == SDLK_BACKSPACE)
            nk_input_key(ctx, NK_KEY_BACKSPACE, down);
        else if (sym == SDLK_HOME) {
            nk_input_key(ctx, NK_KEY_TEXT_START, down);
            nk_input_key(ctx, NK_KEY_SCROLL_START, down);
        } else if (sym == SDLK_END) {
            nk_input_key(ctx, NK_KEY_TEXT_END, down);
            nk_input_key(ctx, NK_KEY_SCROLL_END, down);
        } else if (sym == SDLK_PAGEDOWN) {
            nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
        } else if (sym == SDLK_PAGEUP) {
            nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
        } else if (sym == SDLK_z)
            nk_input_key(ctx, NK_KEY_TEXT_UNDO, down && state[SDL_SCANCODE_LCTRL]);
        else if (sym == SDLK_r)
            nk_input_key(ctx, NK_KEY_TEXT_REDO, down && state[SDL_SCANCODE_LCTRL]);
        else if (sym == SDLK_c)
            nk_input_key(ctx, NK_KEY_COPY, down && state[SDL_SCANCODE_LCTRL]);
        else if (sym == SDLK_v)
            nk_input_key(ctx, NK_KEY_PASTE, down && state[SDL_SCANCODE_LCTRL]);
        else if (sym == SDLK_x)
            nk_input_key(ctx, NK_KEY_CUT, down && state[SDL_SCANCODE_LCTRL]);
        else if (sym == SDLK_b)
            nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down && state[SDL_SCANCODE_LCTRL]);
        else if (sym == SDLK_e)
            nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down && state[SDL_SCANCODE_LCTRL]);
        else if (sym == SDLK_UP)
            nk_input_key(ctx, NK_KEY_UP, down);
        else if (sym == SDLK_DOWN)
            nk_input_key(ctx, NK_KEY_DOWN, down);
        else if (sym == SDLK_LEFT) {
            if (state[SDL_SCANCODE_LCTRL])
                nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
            else nk_input_key(ctx, NK_KEY_LEFT, down);
        } else if (sym == SDLK_RIGHT) {
            if (state[SDL_SCANCODE_LCTRL])
                nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
            else nk_input_key(ctx, NK_KEY_RIGHT, down);
        } else return 0;
        return 1;
    } else if (evt->type == SDL_MOUSEBUTTONDOWN || evt->type == SDL_MOUSEBUTTONUP) {
        /* mouse button */
        int down = evt->type == SDL_MOUSEBUTTONDOWN;
        const int x = evt->button.x, y = evt->button.y;
        if (evt->button.button == SDL_BUTTON_LEFT) {
            if (evt->button.clicks > 1)
                nk_input_button(ctx, NK_BUTTON_DOUBLE, x, y, down);
            nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
        } else if (evt->button.button == SDL_BUTTON_MIDDLE)
            nk_input_button(ctx, NK_BUTTON_MIDDLE, x, y, down);
        else if (evt->button.button == SDL_BUTTON_RIGHT)
            nk_input_button(ctx, NK_BUTTON_RIGHT, x, y, down);
        return 1;
    } else if (evt->type == SDL_MOUSEMOTION) {
        /* mouse motion */
        if (ctx->input.mouse.grabbed) {
            int x = (int)ctx->input.mouse.prev.x, y = (int)ctx->input.mouse.prev.y;
            nk_input_motion(ctx, x + evt->motion.xrel, y + evt->motion.yrel);
        } else nk_input_motion(ctx, evt->motion.x, evt->motion.y);
        return 1;
    } else if (evt->type == SDL_TEXTINPUT) {
        /* text input */
        nk_glyph glyph;
        memcpy(glyph, evt->text.text, NK_UTF_SIZE);
        nk_input_glyph(ctx, glyph);
        return 1;
    } else if (evt->type == SDL_MOUSEWHEEL) {
        /* mouse wheel */
        nk_input_scroll(ctx,nk_vec2((float)evt->wheel.x,(float)evt->wheel.y));
        return 1;
    }
    return 0;
}

NK_API

void nk_sdl_soft_shutdown(void)//1
{
    nk_free(&sdl_soft.ctx);
    nk_sdl_soft_destroy();
    memset(&sdl_soft, 0, sizeof(sdl_soft));
}

#endif
