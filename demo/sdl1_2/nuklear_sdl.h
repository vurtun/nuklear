/*
 * Nuklear - v1.00 - public domain
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
#ifndef NK_SDL_H_
#define NK_SDL_H_

#include <SDL/SDL.h>
typedef struct nk_sdl_Font nk_sdl_Font;
NK_API struct nk_context*   nk_sdl_init(SDL_Surface *screen_surface);
NK_API void                 nk_sdl_handle_event(SDL_Event *evt);
NK_API void                 nk_sdl_render(struct nk_color clear);
NK_API void                 nk_sdl_shutdown(void);

#endif
/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#ifdef NK_SDL_IMPLEMENTATION

#include <string.h>
#include <SDL/SDL.h>
#include <SDL/SDL_gfxPrimitives.h>

#ifndef MAX
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#endif

#ifndef NK_SDL_MAX_POINTS
#define NK_SDL_MAX_POINTS 128
#endif

struct nk_sdl_Font {
    int width;
    int height;
    int handle;
};

static struct nk_sdl {
    SDL_Surface *screen_surface;
    struct nk_context ctx;
} sdl;

static nk_sdl_Font *sdl_font;
static SDL_Rect sdl_clip_rect;

static void
nk_sdl_scissor(SDL_Surface *surface, float x, float y, float w, float h)
{
    sdl_clip_rect.x = x;
    sdl_clip_rect.y = y;
    sdl_clip_rect.w = w  + 1; 
    sdl_clip_rect.h = h;
    SDL_SetClipRect(surface, &sdl_clip_rect);
}

static void
nk_sdl_stroke_line(SDL_Surface *surface, short x0, short y0, short x1,
    short y1, unsigned int line_thickness, struct nk_color col)
{
    thickLineRGBA(surface, x0, y0, x1, y1, line_thickness, col.r, col.g, col.b, col.a);
}

static void
nk_sdl_stroke_rect(SDL_Surface *surface, short x, short y, unsigned short w,
    unsigned short h, unsigned short r, unsigned short line_thickness, struct nk_color col)
{
    /* Note: thickness is not used by default */
    if (r == 0) {
        rectangleRGBA(surface, x, y, x + w, y + h, col.r, col.g, col.b, col.a); 
    } else {
        roundedRectangleRGBA(surface, x, y, x + w, y + h, r, col.r, col.g, col.b, col.a);
    }
}

static void
nk_sdl_fill_rect(SDL_Surface *surface, short x, short y, unsigned short w,
    unsigned short h, unsigned short r, struct nk_color col)
{
    if (r == 0) {
        boxRGBA(surface, x, y, x + w, y + h, col.r, col.g, col.b, col.a); 
    } else {
        roundedBoxRGBA(surface, x, y, x + w, y + h, r, col.r, col.g, col.b, col.a);
    }
}

static void 
nk_sdl_fill_triangle(SDL_Surface *surface, short x0, short y0, short x1, short y1, short x2, short y2, struct nk_color col)
{
    filledTrigonRGBA(surface, x0, y0, x1, y1, x2, y2, col.r, col.g, col.b, col.a);
}

static void
nk_sdl_stroke_triangle(SDL_Surface *surface, short x0, short y0, short x1,
    short y1, short x2, short y2, unsigned short line_thickness, struct nk_color col)
{
    /* Note: thickness is not used by default */
    aatrigonRGBA(surface, x0, y0, x1, y1, x2, y2, col.r, col.g, col.b, col.a); 
}

static void
nk_sdl_fill_polygon(SDL_Surface *surface, const struct nk_vec2i *pnts, int count, struct nk_color col)
{
    Sint16 p_x[NK_SDL_MAX_POINTS];
    Sint16 p_y[NK_SDL_MAX_POINTS];
    int i;
    for (i = 0; (i < count) && (i < NK_SDL_MAX_POINTS); ++i) {
        p_x[i] = pnts[i].x;
        p_y[i] = pnts[i].y;
    }
    filledPolygonRGBA (surface, (Sint16 *)p_x, (Sint16 *)p_y, count, col.r, col.g, col.b, col.a);
}

static void
nk_sdl_stroke_polygon(SDL_Surface *surface, const struct nk_vec2i *pnts, int count,
    unsigned short line_thickness, struct nk_color col)
{
    /* Note: thickness is not used by default */
    Sint16 p_x[NK_SDL_MAX_POINTS];
    Sint16 p_y[NK_SDL_MAX_POINTS];
    int i;
    for (i = 0; (i < count) && (i < NK_SDL_MAX_POINTS); ++i) {
        p_x[i] = pnts[i].x;
        p_y[i] = pnts[i].y;
    }
    aapolygonRGBA(surface, (Sint16 *)p_x, (Sint16 *)p_y, count, col.r, col.g, col.b, col.a); 
}

static void
nk_sdl_stroke_polyline(SDL_Surface *surface, const struct nk_vec2i *pnts,
    int count, unsigned short line_thickness, struct nk_color col)
{
    int x0, y0, x1, y1;
    if (count == 1) {
        x0 = pnts[0].x;
        y0 = pnts[0].y;
        x1 = x0;
        y1 = y0;
        thickLineRGBA(surface, x0, y0, x1, y1, line_thickness, col.r, col.g, col.b, col.a);
    } else if (count >= 2) {
        int i;
        for (i = 0; i < (count - 1); i++) {
            x0 = pnts[i].x;
            y0 = pnts[i].y;
            x1 = pnts[i + 1].x;
            y1 = pnts[i + 1].y;
            thickLineRGBA(surface, x0, y0, x1, y1, line_thickness, col.r, col.g, col.b, col.a);
        }
    }
}

static void
nk_sdl_fill_circle(SDL_Surface *surface, short x, short y, unsigned short w,
    unsigned short h, struct nk_color col)
{
    filledEllipseRGBA(surface,  x + w /2, y + h /2, w / 2, h / 2, col.r, col.g, col.b, col.a); 
}

static void
nk_sdl_stroke_circle(SDL_Surface *surface, short x, short y, unsigned short w,
    unsigned short h, unsigned short line_thickness, struct nk_color col)
{
    /* Note: thickness is not used by default */
    aaellipseRGBA (surface,  x + w /2, y + h /2, w / 2, h / 2, col.r, col.g, col.b, col.a); 
}

static void
nk_sdl_stroke_curve(SDL_Surface *surface, struct nk_vec2i p1,
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
        nk_sdl_stroke_line(surface, last.x, last.y, (short)x, (short)y, line_thickness, col);
        last.x = (short)x; last.y = (short)y;
    }
}

static void
nk_sdl_draw_text(SDL_Surface *surface, short x, short y, unsigned short w, unsigned short h,
    const char *text, int len, nk_sdl_Font *font, struct nk_color cbg, struct nk_color cfg)
{
    int i;
    nk_sdl_fill_rect(surface, x, y, len * font->width, font->height, 0, cbg);
    for (i = 0; i < len; i++) {
        characterRGBA(surface, x, y, text[i], cfg.r, cfg.g, cfg.b, cfg.a);
        x += font->width;
    }
}

static void
interpolate_color(struct nk_color c1, struct nk_color c2, struct nk_color *result, float fraction)
{
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
nk_sdl_fill_rect_multi_color(SDL_Surface *surface, short x, short y, unsigned short w, unsigned short h,
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
            pixelRGBA(surface, x + i, y + j, Y.r, Y.g, Y.b, Y.a); 
        }
    }
}

static void
nk_sdl_clear(SDL_Surface *surface, struct nk_color col)
{
    nk_sdl_fill_rect(surface, 0, 0, surface->w, surface->h, 0, col);
}

static void
nk_sdl_blit(SDL_Surface *surface)
{
    SDL_UpdateRect(surface, 0, 0, 0, 0);
}

NK_API void
nk_sdl_render(struct nk_color clear)
{
    const struct nk_command *cmd;

    SDL_Surface *screen_surface = sdl.screen_surface;
    nk_sdl_clear(screen_surface, clear);

    nk_foreach(cmd, &sdl.ctx)
    {
        switch (cmd->type) {
        case NK_COMMAND_NOP: break;
        case NK_COMMAND_SCISSOR: {
            const struct nk_command_scissor *s =(const struct nk_command_scissor*)cmd;
            nk_sdl_scissor(screen_surface, s->x, s->y, s->w, s->h);
        } break;
        case NK_COMMAND_LINE: {
            const struct nk_command_line *l = (const struct nk_command_line *)cmd;
            nk_sdl_stroke_line(screen_surface, l->begin.x, l->begin.y, l->end.x,
                l->end.y, l->line_thickness, l->color);
        } break;
        case NK_COMMAND_RECT: {
            const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
            nk_sdl_stroke_rect(screen_surface, r->x, r->y, r->w, r->h,
                (unsigned short)r->rounding, r->line_thickness, r->color);
        } break;
        case NK_COMMAND_RECT_FILLED: {
            const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled *)cmd;
            nk_sdl_fill_rect(screen_surface, r->x, r->y, r->w, r->h,
                (unsigned short)r->rounding, r->color);
        } break;
        case NK_COMMAND_CIRCLE: {
            const struct nk_command_circle *c = (const struct nk_command_circle *)cmd;
            nk_sdl_stroke_circle(screen_surface, c->x, c->y, c->w, c->h, c->line_thickness, c->color);
        } break;
        case NK_COMMAND_CIRCLE_FILLED: {
            const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;
            nk_sdl_fill_circle(screen_surface, c->x, c->y, c->w, c->h, c->color);
        } break;
        case NK_COMMAND_TRIANGLE: {
            const struct nk_command_triangle*t = (const struct nk_command_triangle*)cmd;
            nk_sdl_stroke_triangle(screen_surface, t->a.x, t->a.y, t->b.x, t->b.y,
                t->c.x, t->c.y, t->line_thickness, t->color);
        } break;
        case NK_COMMAND_TRIANGLE_FILLED: {
            const struct nk_command_triangle_filled *t = (const struct nk_command_triangle_filled *)cmd;
            nk_sdl_fill_triangle(screen_surface, t->a.x, t->a.y, t->b.x, t->b.y, t->c.x, t->c.y, t->color);
        } break;
        case NK_COMMAND_POLYGON: {
            const struct nk_command_polygon *p =(const struct nk_command_polygon*)cmd;
            nk_sdl_stroke_polygon(screen_surface, p->points, p->point_count, p->line_thickness,p->color);
        } break;
        case NK_COMMAND_POLYGON_FILLED: {
            const struct nk_command_polygon_filled *p = (const struct nk_command_polygon_filled *)cmd;
            nk_sdl_fill_polygon(screen_surface, p->points, p->point_count, p->color);
        } break;
        case NK_COMMAND_POLYLINE: {
            const struct nk_command_polyline *p = (const struct nk_command_polyline *)cmd;
            nk_sdl_stroke_polyline(screen_surface, p->points, p->point_count, p->line_thickness, p->color);
        } break;
        case NK_COMMAND_TEXT: {
            const struct nk_command_text *t = (const struct nk_command_text*)cmd;
            nk_sdl_draw_text(screen_surface, t->x, t->y, t->w, t->h,
                (const char*)t->string, t->length,
                (nk_sdl_Font*)t->font->userdata.ptr,
                t->background, t->foreground);
        } break;
        case NK_COMMAND_CURVE: {
            const struct nk_command_curve *q = (const struct nk_command_curve *)cmd;
            nk_sdl_stroke_curve(screen_surface, q->begin, q->ctrl[0], q->ctrl[1],
                q->end, 22, q->line_thickness, q->color);
        } break;
        case NK_COMMAND_RECT_MULTI_COLOR: {
            const struct nk_command_rect_multi_color *r = (const struct nk_command_rect_multi_color *)cmd;
            nk_sdl_fill_rect_multi_color(screen_surface, r->x, r->y, r->w, r->h, r->left, r->top, r->right, r->bottom);
        } break;
        case NK_COMMAND_IMAGE:
        case NK_COMMAND_ARC:
        case NK_COMMAND_ARC_FILLED:
        default: break;
        }
    }
    nk_sdl_blit(sdl.screen_surface);
    nk_clear(&sdl.ctx);

}

static void
nk_sdl_clipbard_paste(nk_handle usr, struct nk_text_edit *edit)
{
    /* Not supported in SDL 1.2. Use platform specific code.  */
}

static void
nk_sdl_clipbard_copy(nk_handle usr, const char *text, int len)
{
    /* Not supported in SDL 1.2. Use platform specific code.  */
}

static float
nk_sdl_get_text_width(nk_handle handle, float height, const char *text, int len)
{
    return len * sdl_font->width;
}

NK_API struct nk_context*
nk_sdl_init(SDL_Surface *screen_surface)
{
    struct nk_user_font font;
    sdl_font = (nk_sdl_Font*)calloc(1, sizeof(nk_sdl_Font));
    sdl_font->width = 8; /* Default in  the SDL_gfx library */
    sdl_font->height = 8; /* Default in  the SDL_gfx library */
    if (!sdl_font)
        return NULL;

    font.userdata = nk_handle_ptr(sdl_font);
    font.height = (float)sdl_font->height;
    font.width = nk_sdl_get_text_width;

    sdl.screen_surface = screen_surface;
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
    if (evt->type == SDL_VIDEORESIZE) {
        /* Do nothing */
    } else if (evt->type == SDL_KEYUP || evt->type == SDL_KEYDOWN) {
        /* key events */
        int down = evt->type == SDL_KEYDOWN;
        SDLMod state = SDL_GetModState();
        SDLKey sym = evt->key.keysym.sym;

        if (sym == SDLK_RSHIFT || sym == SDLK_LSHIFT) nk_input_key(ctx, NK_KEY_SHIFT, down);
        else if (sym == SDLK_DELETE)    nk_input_key(ctx, NK_KEY_DEL, down);
        else if (sym == SDLK_RETURN)    nk_input_key(ctx, NK_KEY_ENTER, down);
        else if (sym == SDLK_TAB)       nk_input_key(ctx, NK_KEY_TAB, down);
        else if (sym == SDLK_LEFT)      nk_input_key(ctx, NK_KEY_LEFT, down);
        else if (sym == SDLK_RIGHT)     nk_input_key(ctx, NK_KEY_RIGHT, down);
        else if (sym == SDLK_BACKSPACE) nk_input_key(ctx, NK_KEY_BACKSPACE, down);
        else if (sym == SDLK_HOME)      nk_input_key(ctx, NK_KEY_TEXT_START, down);
        else if (sym == SDLK_END)       nk_input_key(ctx, NK_KEY_TEXT_END, down);
        else if (sym == SDLK_SPACE && !down) nk_input_char(ctx, ' ');
        else {
            if (sym == SDLK_c && state == SDLK_LCTRL)
                nk_input_key(ctx, NK_KEY_COPY, down);
            else if (sym == SDLK_v && state == SDLK_LCTRL)
                nk_input_key(ctx, NK_KEY_PASTE, down);
            else if (sym == SDLK_x && state == SDLK_LCTRL)
                nk_input_key(ctx, NK_KEY_CUT, down);
            else if (sym == SDLK_z && state == SDLK_LCTRL)
                nk_input_key(ctx, NK_KEY_TEXT_UNDO, down);
            else if (sym == SDLK_r && state == SDLK_LCTRL)
                nk_input_key(ctx, NK_KEY_TEXT_REDO, down);
            else if (sym == SDLK_LEFT && state == SDLK_LCTRL)
                nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
            else if (sym == SDLK_RIGHT && state == SDLK_LCTRL)
                nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
            else if (sym == SDLK_b && state == SDLK_LCTRL)
                nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down);
            else if (sym == SDLK_e && state == SDLK_LCTRL)
                nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down);
            else if (!down) {
                /* This demo does not provide full unicode support since the default
                 * sdl1.2 font only allows runes in range 0-255. But this demo
                 * already is quite limited and not really meant for full blown Apps
                 * anyway. So I think ASCII support for Debugging Tools should be enough */
                if (sym >= SDLK_0 && sym <= SDLK_9) {
                    nk_rune rune = '0' + sym - SDLK_0;
                    nk_input_unicode(ctx, rune);
                } else if (sym >= SDLK_a && sym <= SDLK_z) {
                    nk_rune rune = 'a' + sym - SDLK_a;
                    rune = ((state == KMOD_LSHIFT) ? (nk_rune)nk_to_upper((int)rune):rune);
                    nk_input_unicode(ctx, rune);
                }
            }
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
        if (evt->button.button == SDL_BUTTON_WHEELUP)
            nk_input_scroll(ctx, 1.0f);
        if (evt->button.button == SDL_BUTTON_WHEELDOWN)
            nk_input_scroll(ctx, -1.0f);
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


#endif

