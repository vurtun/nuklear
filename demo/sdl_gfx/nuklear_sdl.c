#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL2_gfxPrimitives.h>

#define NK_IMPLEMENTATION
#include "nuklear_sdl.h"
//#include "../../nuklear.h"

#ifndef MAX
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#endif

#define NK_SDL_MAX_POINTS 128

struct nk_sdl_Font {
	int width;
	int height;
	int handle;
};

static struct nk_sdl {
	SDL_Window* window;
	SDL_Renderer* renderer;
	struct nk_context ctx;
} sdl;

static nk_sdl_Font *sdl_font;
static struct nk_user_font font;
static SDL_Rect sdl_clip_rect;

static void
nk_sdl_scissor(SDL_Renderer* renderer, float x, float y, float w, float h)
{
	sdl_clip_rect.x = x;
	sdl_clip_rect.y = y;
	sdl_clip_rect.w = w  + 1;
	sdl_clip_rect.h = h;
	SDL_RenderSetClipRect(renderer, &sdl_clip_rect);
}

static void
nk_sdl_stroke_line(SDL_Renderer* renderer, short x0, short y0, short x1,
	short y1, unsigned int line_thickness, struct nk_color col)
{
	thickLineRGBA(renderer, x0, y0, x1, y1, line_thickness, col.r, col.g, col.b, col.a);
}

static void
nk_sdl_stroke_rect(SDL_Renderer* renderer, short x, short y, unsigned short w,
	unsigned short h, unsigned short r, unsigned short line_thickness, struct nk_color col)
{
	/* TODO Add line thickness support */
	if (r == 0) {
		rectangleRGBA(renderer, x, y, x + w, y + h, col.r, col.g, col.b, col.a);
	} else {
		roundedRectangleRGBA(renderer, x, y, x + w, y + h, r, col.r, col.g, col.b, col.a);
	}
}

static void
nk_sdl_fill_rect(SDL_Renderer* renderer, short x, short y, unsigned short w,
	unsigned short h, unsigned short r, struct nk_color col)
{
	if (r == 0) {
		boxRGBA(renderer, x, y, x + w, y + h, col.r, col.g, col.b, col.a);
	} else {
		roundedBoxRGBA(renderer, x, y, x + w, y + h, r, col.r, col.g, col.b, col.a);
	}
}

static void
nk_sdl_fill_triangle(SDL_Renderer* renderer, short x0, short y0, short x1, short y1, short x2, short y2, struct nk_color col)
{
	filledTrigonRGBA(renderer, x0, y0, x1, y1, x2, y2, col.r, col.g, col.b, col.a);
}

static void
nk_sdl_stroke_triangle(SDL_Renderer* renderer, short x0, short y0, short x1,
	short y1, short x2, short y2, unsigned short line_thickness, struct nk_color col)
{
	/* TODO Add line_thickness support */
	aatrigonRGBA(renderer, x0, y0, x1, y1, x2, y2, col.r, col.g, col.b, col.a);
}

static void
nk_sdl_fill_polygon(SDL_Renderer* renderer, const struct nk_vec2i *pnts, int count, struct nk_color col)
{
	Sint16 p_x[NK_SDL_MAX_POINTS];
	Sint16 p_y[NK_SDL_MAX_POINTS];
	int i;
	for (i = 0; (i < count) && (i < NK_SDL_MAX_POINTS); ++i) {
		p_x[i] = pnts[i].x;
		p_y[i] = pnts[i].y;
	}
	filledPolygonRGBA (renderer, (Sint16 *)p_x, (Sint16 *)p_y, count, col.r, col.g, col.b, col.a);
}

static void
nk_sdl_stroke_polygon(SDL_Renderer* renderer, const struct nk_vec2i *pnts, int count,
	unsigned short line_thickness, struct nk_color col)
{
	/*  TODO Add line thickness support */
	Sint16 p_x[NK_SDL_MAX_POINTS];
	Sint16 p_y[NK_SDL_MAX_POINTS];
	int i;
	for (i = 0; (i < count) && (i < NK_SDL_MAX_POINTS); ++i) {
		p_x[i] = pnts[i].x;
		p_y[i] = pnts[i].y;
	}
	aapolygonRGBA(renderer, (Sint16 *)p_x, (Sint16 *)p_y, count, col.r, col.g, col.b, col.a);
}

static void
nk_sdl_stroke_polyline(SDL_Renderer* renderer, const struct nk_vec2i *pnts,
	int count, unsigned short line_thickness, struct nk_color col)
{
	int x0, y0, x1, y1;
	if (count == 1) {
		x0 = pnts[0].x;
		y0 = pnts[0].y;
		x1 = x0;
		y1 = y0;
		thickLineRGBA(renderer, x0, y0, x1, y1, line_thickness, col.r, col.g, col.b, col.a);
	} else if (count >= 2) {
		int i;
		for (i = 0; i < (count - 1); i++) {
			x0 = pnts[i].x;
			y0 = pnts[i].y;
			x1 = pnts[i + 1].x;
			y1 = pnts[i + 1].y;
			thickLineRGBA(renderer, x0, y0, x1, y1, line_thickness, col.r, col.g, col.b, col.a);
		}
	}
}

static void
nk_sdl_fill_circle(SDL_Renderer* renderer, short x, short y, unsigned short w,
	unsigned short h, struct nk_color col)
{
	filledEllipseRGBA(renderer,  x + w /2, y + h /2, w / 2, h / 2, col.r, col.g, col.b, col.a);
}

static void
nk_sdl_stroke_circle(SDL_Renderer* renderer, short x, short y, unsigned short w,
	unsigned short h, unsigned short line_thickness, struct nk_color col)
{
	/* TODO  Add line_thickness support */
	aaellipseRGBA (renderer,  x + w /2, y + h /2, w / 2, h / 2, col.r, col.g, col.b, col.a);
}

static void
nk_sdl_stroke_curve(SDL_Renderer* renderer, struct nk_vec2i p1,
	struct nk_vec2i p2, struct nk_vec2i p3, struct nk_vec2i p4, unsigned int num_segments,
	unsigned short line_thickness, struct nk_color col)
{
	unsigned int i_step;
	float t_step;
	struct nk_vec2i last = p1;

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
		nk_sdl_stroke_line(renderer, last.x, last.y, (short)x, (short)y, line_thickness, col);
		last.x = (short)x; last.y = (short)y;
	}
}

static void
nk_sdl_draw_text(SDL_Renderer* renderer, short x, short y, unsigned short w, unsigned short h,
	const char *text, int len, nk_sdl_Font *font, struct nk_color cbg, struct nk_color cfg)
{
	int i;

	nk_sdl_fill_rect(renderer, x, y, len * font->width, font->height, 0, cbg);
	for (i = 0; i < len; i++) {
		characterRGBA(renderer, x, y, text[i], cfg.r, cfg.g, cfg.b, cfg.a);
		x += font->width;
	}
}

static void interpolate_color(struct nk_color c1, struct nk_color c2, struct nk_color *result, float fraction) {
	float r = c1.r + (c2.r - c1.r) * fraction;
	float g = c1.g + (c2.g - c1.g) * fraction;
	float b = c1.b + (c2.b - c1.b) * fraction;
	float a = c1.a + (c2.a - c1.a) * fraction;
	result->r = (nk_byte)NK_CLAMP(0, r, 255);
	result->g = (nk_byte)NK_CLAMP(0, g, 255);
	result->b = (nk_byte)NK_CLAMP(0, b, 255);
	result->a = (nk_byte)NK_CLAMP(0, a, 255);
}

static void
nk_sdl_fill_rect_multi_color(SDL_Renderer* renderer, short x, short y, unsigned short w, unsigned short h,
	struct nk_color left, struct nk_color top,  struct nk_color right, struct nk_color bottom)
{
	struct nk_color X1, X2, Y;
	float fraction_x, fraction_y;
	int i,j;

	for (j = 0; j < h; j++) {
		fraction_y = ((float)j) / h;
		for (i = 0; i < w; i++) {
			fraction_x = ((float)i) / w;
			interpolate_color(left, top, &X1, fraction_x);
			interpolate_color(right, bottom, &X2, fraction_x);
			interpolate_color(X1, X2, &Y, fraction_y);
			pixelRGBA(renderer, x + i, y + j, Y.r, Y.g, Y.b, Y.a);
		}
	}
}


static void
nk_sdl_clear(SDL_Renderer* renderer, struct nk_color col)
{
	int w, h;
	SDL_GetWindowSize(sdl.window, &w, &h);
	nk_sdl_fill_rect(renderer, 0, 0, w, h, 0, col);
}

static void
nk_sdl_blit(SDL_Renderer* renderer)
{
	SDL_RenderPresent(renderer);
}

NK_API void
nk_sdl_render(struct nk_color clear)
{
	const struct nk_command *cmd;

	SDL_Renderer* ren = sdl.renderer;
	nk_sdl_clear(ren, clear);

	nk_foreach(cmd, &sdl.ctx)
	{
		switch (cmd->type) {
		case NK_COMMAND_NOP: break;
		case NK_COMMAND_SCISSOR: {
			const struct nk_command_scissor *s =(const struct nk_command_scissor*)cmd;
			nk_sdl_scissor(ren, s->x, s->y, s->w, s->h);
		} break;
		case NK_COMMAND_LINE: {
			const struct nk_command_line *l = (const struct nk_command_line *)cmd;
			nk_sdl_stroke_line(ren, l->begin.x, l->begin.y, l->end.x,
				l->end.y, l->line_thickness, l->color);
		} break;
		case NK_COMMAND_RECT: {
			const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
			nk_sdl_stroke_rect(ren, r->x, r->y, r->w, r->h,
				(unsigned short)r->rounding, r->line_thickness, r->color);
		} break;
		case NK_COMMAND_RECT_FILLED: {
			const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled *)cmd;
			nk_sdl_fill_rect(ren, r->x, r->y, r->w, r->h,
				(unsigned short)r->rounding, r->color);
		} break;
		case NK_COMMAND_CIRCLE: {
			const struct nk_command_circle *c = (const struct nk_command_circle *)cmd;
			nk_sdl_stroke_circle(ren, c->x, c->y, c->w, c->h, c->line_thickness, c->color);
		} break;
		case NK_COMMAND_CIRCLE_FILLED: {
			const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;
			nk_sdl_fill_circle(ren, c->x, c->y, c->w, c->h, c->color);
		} break;
		case NK_COMMAND_TRIANGLE: {
			const struct nk_command_triangle*t = (const struct nk_command_triangle*)cmd;
			nk_sdl_stroke_triangle(ren, t->a.x, t->a.y, t->b.x, t->b.y,
				t->c.x, t->c.y, t->line_thickness, t->color);
		} break;
		case NK_COMMAND_TRIANGLE_FILLED: {
			const struct nk_command_triangle_filled *t = (const struct nk_command_triangle_filled *)cmd;
			nk_sdl_fill_triangle(ren, t->a.x, t->a.y, t->b.x, t->b.y, t->c.x, t->c.y, t->color);
		} break;
		case NK_COMMAND_POLYGON: {
			const struct nk_command_polygon *p =(const struct nk_command_polygon*)cmd;
			nk_sdl_stroke_polygon(ren, p->points, p->point_count, p->line_thickness,p->color);
		} break;
		case NK_COMMAND_POLYGON_FILLED: {
			const struct nk_command_polygon_filled *p = (const struct nk_command_polygon_filled *)cmd;
			nk_sdl_fill_polygon(ren, p->points, p->point_count, p->color);
		} break;
		case NK_COMMAND_POLYLINE: {
			const struct nk_command_polyline *p = (const struct nk_command_polyline *)cmd;
			nk_sdl_stroke_polyline(ren, p->points, p->point_count, p->line_thickness, p->color);
		} break;
		case NK_COMMAND_TEXT: {
			const struct nk_command_text *t = (const struct nk_command_text*)cmd;
			nk_sdl_draw_text(ren, t->x, t->y, t->w, t->h,
				(const char*)t->string, t->length,
				(nk_sdl_Font*)t->font->userdata.ptr,
				t->background, t->foreground);
		} break;
		case NK_COMMAND_CURVE: {
			const struct nk_command_curve *q = (const struct nk_command_curve *)cmd;
			nk_sdl_stroke_curve(ren, q->begin, q->ctrl[0], q->ctrl[1],
				q->end, 22, q->line_thickness, q->color);
		} break;
		case NK_COMMAND_RECT_MULTI_COLOR: {
			const struct nk_command_rect_multi_color *r = (const struct nk_command_rect_multi_color *)cmd;
			nk_sdl_fill_rect_multi_color(ren, r->x, r->y, r->w, r->h, r->left, r->top, r->right, r->bottom);
		} break;
		case NK_COMMAND_IMAGE:
		case NK_COMMAND_ARC:
		case NK_COMMAND_ARC_FILLED:
		default: break;
		}
	}

	nk_sdl_blit(ren);
	nk_clear(&sdl.ctx);

}

static void
nk_sdl_clipbard_paste(nk_handle usr, struct nk_text_edit *edit)
{
    const char *text = SDL_GetClipboardText();
    if (text) nk_textedit_paste(edit, text, nk_strlen(text));
    (void)usr;
}

static void
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

static float
nk_sdl_get_text_width(nk_handle handle, float height, const char *text, int len)
{
	return len * sdl_font->width;
}

NK_API struct nk_context*
nk_sdl_init(SDL_Window* window, SDL_Renderer* renderer)
{
	sdl_font = (nk_sdl_Font*)calloc(1, sizeof(nk_sdl_Font));
	sdl_font->width = 8; /* Default in the SDL_gfx library */
	sdl_font->height = 8; /* Default in the SDL_gfx library */
	if (!sdl_font)
		return NULL;

	font.userdata = nk_handle_ptr(sdl_font);
	font.height = (float)sdl_font->height;
	font.width = nk_sdl_get_text_width;

	sdl.window = window;
	sdl.renderer = renderer;
	nk_init_default(&sdl.ctx, &font);
	sdl.ctx.clip.copy = nk_sdl_clipbard_copy;
	sdl.ctx.clip.paste = nk_sdl_clipbard_paste;
	sdl.ctx.clip.userdata = nk_handle_ptr(0);
	return &sdl.ctx;
}

NK_API void
nk_sdl_handle_event(SDL_Event *evt)
{

	struct nk_context *ctx = &sdl.ctx;
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
		else if (sym == SDLK_HOME)
			nk_input_key(ctx, NK_KEY_TEXT_START, down);
		else if (sym == SDLK_END)
			nk_input_key(ctx, NK_KEY_TEXT_END, down);
		else if (sym == SDLK_z)
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
		else if (sym == SDLK_LEFT) {
			if (state[KMOD_LCTRL])
				nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
			else nk_input_key(ctx, NK_KEY_LEFT, down);
		} else if (sym == SDLK_RIGHT) {
			if (state[KMOD_LCTRL])
				nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
			else nk_input_key(ctx, NK_KEY_RIGHT, down);
		}
	} else if (evt->type == SDL_MOUSEBUTTONDOWN || evt->type == SDL_MOUSEBUTTONUP) {
		/* mouse button */
		int down = evt->type == SDL_MOUSEBUTTONDOWN;
		const int x = evt->button.x, y = evt->button.y;
		if (evt->button.button == SDL_BUTTON_LEFT)
			nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
		if (evt->button.button == SDL_BUTTON_MIDDLE)
			nk_input_button(ctx, NK_BUTTON_MIDDLE, x, y, down);
		if (evt->button.button == SDL_BUTTON_RIGHT)
			nk_input_button(ctx, NK_BUTTON_RIGHT, x, y, down);
	} else if (evt->type == SDL_MOUSEWHEEL) {
        nk_input_scroll(ctx,nk_vec2((float)evt->wheel.x,(float)evt->wheel.y));
	} else if (evt->type == SDL_MOUSEMOTION) {
		nk_input_motion(ctx, evt->motion.x, evt->motion.y);
	}
}

NK_API void
nk_sdl_shutdown(void)
{
	free(sdl_font);
	nk_free(&sdl.ctx);
}
