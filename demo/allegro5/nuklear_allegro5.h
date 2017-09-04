/*
 * Nuklear - 1.32.0 - public domain
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
#ifndef NK_ALLEGRO5_H_
#define NK_ALLEGRO5_H_

#include <string.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

typedef struct NkAllegro5Font NkAllegro5Font;
NK_API struct nk_context*     nk_allegro5_init(NkAllegro5Font *font, ALLEGRO_DISPLAY *dsp,
                                  unsigned int width, unsigned int height);
NK_API void                   nk_allegro5_handle_event(ALLEGRO_EVENT *ev);
NK_API void                   nk_allegro5_shutdown(void);
NK_API void                   nk_allegro5_render(void);

/* Fonts. We wrap normal allegro fonts in some nuklear book keeping */
NK_API NkAllegro5Font*        nk_allegro5_font_create_from_file(const char *file_name, int font_size, int flags);
NK_API void                   nk_allegro5_font_del(NkAllegro5Font *font);
NK_API void                   nk_allegro5_font_set_font(NkAllegro5Font *font);

#endif
/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#ifdef NK_ALLEGRO5_IMPLEMENTATION

#ifndef NK_ALLEGRO5_TEXT_MAX
#define NK_ALLEGRO5_TEXT_MAX 256
#endif


struct NkAllegro5Font {
    struct nk_user_font nk;
    int height;
    ALLEGRO_FONT *font;
};

static struct nk_allegro5 {
    ALLEGRO_DISPLAY *dsp;
    unsigned int width;
    unsigned int height;
    int is_touch_down;
    int touch_down_id;
    struct nk_context ctx;
    struct nk_buffer cmds;
} allegro5;


/* Flags are identical to al_load_font() flags argument */
NK_API NkAllegro5Font*
nk_allegro5_font_create_from_file(const char *file_name, int font_size, int flags)
{
    if (!al_init_image_addon()) {
        fprintf(stdout, "Unable to initialize required allegro5 image addon\n");
        exit(1);
    }
    if (!al_init_font_addon()) {
        fprintf(stdout, "Unable to initialize required allegro5 font addon\n");
        exit(1);
    }
    if (!al_init_ttf_addon()) {
        fprintf(stdout, "Unable to initialize required allegro5 TTF font addon\n");
        exit(1);
    }
    NkAllegro5Font *font = (NkAllegro5Font*)calloc(1, sizeof(NkAllegro5Font));

    font->font = al_load_font(file_name, font_size, flags);
    if (font->font == NULL) {
        fprintf(stdout, "Unable to load font file: %s\n", file_name);
        return NULL;
    }
    font->height = al_get_font_line_height(font->font);
    return font;
}

static float
nk_allegro5_font_get_text_width(nk_handle handle, float height, const char *text, int len)
{
    NkAllegro5Font *font = (NkAllegro5Font*)handle.ptr;
    if (!font || !text) {
        return 0;
    }
    /* We must copy into a new buffer with exact length null-terminated
       as nuklear uses variable size buffers and al_get_text_width doesn't
       accept a length, it infers length from null-termination
       (which is unsafe API design by allegro devs!) */
    char strcpy[len+1];
    strncpy((char*)&strcpy, text, len);
    strcpy[len] = '\0';
    return al_get_text_width(font->font, strcpy);
}

NK_API void
nk_allegro5_font_set_font(NkAllegro5Font *allegro5font)
{
    struct nk_user_font *font = &allegro5font->nk;
    font->userdata = nk_handle_ptr(allegro5font);
    font->height = (float)allegro5font->height;
    font->width = nk_allegro5_font_get_text_width;
    nk_style_set_font(&allegro5.ctx, font);
}

NK_API void
nk_allegro5_font_del(NkAllegro5Font *font)
{
    if(!font) return;
    al_destroy_font(font->font);
    free(font);
}

static ALLEGRO_COLOR
nk_color_to_allegro_color(struct nk_color color)
{
    return al_map_rgba((unsigned char)color.r, (unsigned char)color.g,
                (unsigned char)color.b, (unsigned char)color.a);
}

NK_API void
nk_allegro5_render()
{
    const struct nk_command *cmd;

    al_set_target_backbuffer(allegro5.dsp);

    nk_foreach(cmd, &allegro5.ctx)
    {
        ALLEGRO_COLOR color;
        switch (cmd->type) {
        case NK_COMMAND_NOP: break;
        case NK_COMMAND_SCISSOR: {
            const struct nk_command_scissor *s =(const struct nk_command_scissor*)cmd;
            al_set_clipping_rectangle((int)s->x, (int)s->y, (int)s->w, (int)s->h);
        } break;
        case NK_COMMAND_LINE: {
            const struct nk_command_line *l = (const struct nk_command_line *)cmd;
            color = nk_color_to_allegro_color(l->color);
            al_draw_line((float)l->begin.x, (float)l->begin.y, (float)l->end.x,
                (float)l->end.y, color, (float)l->line_thickness);
        } break;
        case NK_COMMAND_RECT: {
            const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
            color = nk_color_to_allegro_color(r->color);
            al_draw_rounded_rectangle((float)r->x, (float)r->y, (float)(r->x + r->w),
                (float)(r->y + r->h), (float)r->rounding, (float)r->rounding, color,
                (float)r->line_thickness);
        } break;
        case NK_COMMAND_RECT_FILLED: {
            const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled *)cmd;
            color = nk_color_to_allegro_color(r->color);
            al_draw_filled_rounded_rectangle((float)r->x, (float)r->y,
                (float)(r->x + r->w), (float)(r->y + r->h), (float)r->rounding,
                (float)r->rounding, color);
        } break;
        case NK_COMMAND_CIRCLE: {
            const struct nk_command_circle *c = (const struct nk_command_circle *)cmd;
            color = nk_color_to_allegro_color(c->color);
            float xr, yr;
            xr = (float)c->w/2;
            yr = (float)c->h/2;
            al_draw_ellipse(((float)(c->x)) + xr, ((float)c->y) + yr,
                xr, yr, color, (float)c->line_thickness);
        } break;
        case NK_COMMAND_CIRCLE_FILLED: {
            const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;
            color = nk_color_to_allegro_color(c->color);
            float xr, yr;
            xr = (float)c->w/2;
            yr = (float)c->h/2;
            al_draw_filled_ellipse(((float)(c->x)) + xr, ((float)c->y) + yr,
                xr, yr, color);
        } break;
        case NK_COMMAND_TRIANGLE: {
            const struct nk_command_triangle*t = (const struct nk_command_triangle*)cmd;
            color = nk_color_to_allegro_color(t->color);
            al_draw_triangle((float)t->a.x, (float)t->a.y, (float)t->b.x, (float)t->b.y,
                (float)t->c.x, (float)t->c.y, color, (float)t->line_thickness);
        } break;
        case NK_COMMAND_TRIANGLE_FILLED: {
            const struct nk_command_triangle_filled *t = (const struct nk_command_triangle_filled *)cmd;
            color = nk_color_to_allegro_color(t->color);
            al_draw_filled_triangle((float)t->a.x, (float)t->a.y, (float)t->b.x,
                (float)t->b.y, (float)t->c.x, (float)t->c.y, color);
        } break;
        case NK_COMMAND_POLYGON: {
            const struct nk_command_polygon *p = (const struct nk_command_polygon*)cmd;
            color = nk_color_to_allegro_color(p->color);
            int i;
            float vertices[p->point_count * 2];
            for (i = 0; i < p->point_count; i++) {
                vertices[i*2] = p->points[i].x;
                vertices[(i*2) + 1] = p->points[i].y;
            }
            al_draw_polyline((const float*)&vertices, (2 * sizeof(float)),
                (int)p->point_count, ALLEGRO_LINE_JOIN_ROUND, ALLEGRO_LINE_CAP_CLOSED,
                color, (float)p->line_thickness, 0.0);
        } break;
        case NK_COMMAND_POLYGON_FILLED: {
            const struct nk_command_polygon_filled *p = (const struct nk_command_polygon_filled *)cmd;
            color = nk_color_to_allegro_color(p->color);
            int i;
            float vertices[p->point_count * 2];
            for (i = 0; i < p->point_count; i++) {
                vertices[i*2] = p->points[i].x;
                vertices[(i*2) + 1] = p->points[i].y;
            }
            al_draw_filled_polygon((const float*)&vertices, (int)p->point_count, color);
        } break;
        case NK_COMMAND_POLYLINE: {
            const struct nk_command_polyline *p = (const struct nk_command_polyline *)cmd;
            color = nk_color_to_allegro_color(p->color);
            int i;
            float vertices[p->point_count * 2];
            for (i = 0; i < p->point_count; i++) {
                vertices[i*2] = p->points[i].x;
                vertices[(i*2) + 1] = p->points[i].y;
            }
            al_draw_polyline((const float*)&vertices, (2 * sizeof(float)),
                (int)p->point_count, ALLEGRO_LINE_JOIN_ROUND, ALLEGRO_LINE_CAP_ROUND,
                color, (float)p->line_thickness, 0.0);
        } break;
        case NK_COMMAND_TEXT: {
            const struct nk_command_text *t = (const struct nk_command_text*)cmd;
            color = nk_color_to_allegro_color(t->foreground);
            NkAllegro5Font *font = (NkAllegro5Font*)t->font->userdata.ptr;
            al_draw_text(font->font,
                color, (float)t->x, (float)t->y, 0,
                (const char*)t->string);
        } break;
        case NK_COMMAND_CURVE: {
            const struct nk_command_curve *q = (const struct nk_command_curve *)cmd;
            color = nk_color_to_allegro_color(q->color);
            float points[8];
            points[0] = (float)q->begin.x;
            points[1] = (float)q->begin.y;
            points[2] = (float)q->ctrl[0].x;
            points[3] = (float)q->ctrl[0].y;
            points[4] = (float)q->ctrl[1].x;
            points[5] = (float)q->ctrl[1].y;
            points[6] = (float)q->end.x;
            points[7] = (float)q->end.y;
            al_draw_spline(points, color, (float)q->line_thickness);
        } break;
        case NK_COMMAND_ARC: {
            const struct nk_command_arc *a = (const struct nk_command_arc *)cmd;
            color = nk_color_to_allegro_color(a->color);
            al_draw_arc((float)a->cx, (float)a->cy, (float)a->r, a->a[0],
                a->a[1], color, (float)a->line_thickness);
        } break;
        case NK_COMMAND_RECT_MULTI_COLOR:
        case NK_COMMAND_IMAGE:
        case NK_COMMAND_ARC_FILLED:
        default: break;
        }
    }
    nk_clear(&allegro5.ctx);
}

NK_API void
nk_allegro5_handle_event(ALLEGRO_EVENT *ev)
{
    struct nk_context *ctx = &allegro5.ctx;
    switch (ev->type) {
        case ALLEGRO_EVENT_DISPLAY_RESIZE: {
            allegro5.width = (unsigned int)ev->display.width;
            allegro5.height = (unsigned int)ev->display.height;
            al_acknowledge_resize(ev->display.source);
        } break;
        case ALLEGRO_EVENT_MOUSE_AXES: {
            nk_input_motion(ctx, ev->mouse.x, ev->mouse.y);
            if (ev->mouse.dz != 0) {
                nk_input_scroll(ctx, nk_vec2(0,(float)ev->mouse.dz / al_get_mouse_wheel_precision()));
            }
        } break;
        case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
        case ALLEGRO_EVENT_MOUSE_BUTTON_UP: {
            int button = NK_BUTTON_LEFT;
            if (ev->mouse.button == 2) {
                button = NK_BUTTON_RIGHT;
            }
            else if (ev->mouse.button == 3) {
                button = NK_BUTTON_MIDDLE;
            }
            nk_input_button(ctx, button, ev->mouse.x, ev->mouse.y, ev->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN);
        } break;
        /* This essentially converts touch events to mouse events */
        case ALLEGRO_EVENT_TOUCH_BEGIN:
        case ALLEGRO_EVENT_TOUCH_END: {
            /* We only acknowledge one touch at a time. Otherwise, each touch
               would be manipulating multiple nuklear elements, as if there
               were multiple mouse cursors */
            if (allegro5.is_touch_down && allegro5.touch_down_id != ev->touch.id) {
                return;
            }
            if (ev->type == ALLEGRO_EVENT_TOUCH_BEGIN) {
                allegro5.is_touch_down = 1;
                allegro5.touch_down_id = ev->touch.id;
                /* FIXME: This is a hack to properly simulate
                   touches as a mouse with nuklear. If you instantly jump
                   from one place to another without an nk_input_end(), it
                   confuses the nuklear state. nuklear expects smooth mouse
                   movements, which is unlike a touch screen */
                nk_input_motion(ctx, (int)ev->touch.x, (int)ev->touch.y);
                nk_input_end(ctx);
                nk_input_begin(ctx);
            }
            else {
                allegro5.is_touch_down = 0;
                allegro5.touch_down_id = -1;
            }
            nk_input_button(ctx, NK_BUTTON_LEFT, (int)ev->touch.x, (int)ev->touch.y, ev->type == ALLEGRO_EVENT_TOUCH_BEGIN);
        } break;
        case ALLEGRO_EVENT_TOUCH_MOVE: {
            /* Only acknowledge movements of a single touch, we are
               simulating a mouse cursor */
            if (!allegro5.is_touch_down || allegro5.touch_down_id != ev->touch.id) {
                return;
            }
            nk_input_motion(ctx, (int)ev->touch.x, (int)ev->touch.y);
        } break;
        case ALLEGRO_EVENT_KEY_DOWN:
        case ALLEGRO_EVENT_KEY_UP: {
            int kc = ev->keyboard.keycode;
            int down = ev->type == ALLEGRO_EVENT_KEY_DOWN;

            if (kc == ALLEGRO_KEY_LSHIFT || kc == ALLEGRO_KEY_RSHIFT) nk_input_key(ctx, NK_KEY_SHIFT, down);
            else if (kc == ALLEGRO_KEY_DELETE)    nk_input_key(ctx, NK_KEY_DEL, down);
            else if (kc == ALLEGRO_KEY_ENTER)     nk_input_key(ctx, NK_KEY_ENTER, down);
            else if (kc == ALLEGRO_KEY_TAB)       nk_input_key(ctx, NK_KEY_TAB, down);
            else if (kc == ALLEGRO_KEY_LEFT)      nk_input_key(ctx, NK_KEY_LEFT, down);
            else if (kc == ALLEGRO_KEY_RIGHT)     nk_input_key(ctx, NK_KEY_RIGHT, down);
            else if (kc == ALLEGRO_KEY_UP)        nk_input_key(ctx, NK_KEY_UP, down);
            else if (kc == ALLEGRO_KEY_DOWN)      nk_input_key(ctx, NK_KEY_DOWN, down);
            else if (kc == ALLEGRO_KEY_BACKSPACE) nk_input_key(ctx, NK_KEY_BACKSPACE, down);
            else if (kc == ALLEGRO_KEY_ESCAPE)    nk_input_key(ctx, NK_KEY_TEXT_RESET_MODE, down);
            else if (kc == ALLEGRO_KEY_PGUP)      nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
            else if (kc == ALLEGRO_KEY_PGDN)      nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
            else if (kc == ALLEGRO_KEY_HOME) {
                nk_input_key(ctx, NK_KEY_TEXT_START, down);
                nk_input_key(ctx, NK_KEY_SCROLL_START, down);
            } else if (kc == ALLEGRO_KEY_END) {
                nk_input_key(ctx, NK_KEY_TEXT_END, down);
                nk_input_key(ctx, NK_KEY_SCROLL_END, down);
            }
        } break;
        case ALLEGRO_EVENT_KEY_CHAR: {
            int kc = ev->keyboard.keycode;
            int control_mask = (ev->keyboard.modifiers & ALLEGRO_KEYMOD_CTRL) ||
                               (ev->keyboard.modifiers & ALLEGRO_KEYMOD_COMMAND);

            if (kc == ALLEGRO_KEY_C && control_mask) {
                nk_input_key(ctx, NK_KEY_COPY, 1);
            } else if (kc == ALLEGRO_KEY_V && control_mask) {
                nk_input_key(ctx, NK_KEY_PASTE, 1);
            } else if (kc == ALLEGRO_KEY_X && control_mask) {
                nk_input_key(ctx, NK_KEY_CUT, 1);
            } else if (kc == ALLEGRO_KEY_Z && control_mask) {
                nk_input_key(ctx, NK_KEY_TEXT_UNDO, 1);
            } else if (kc == ALLEGRO_KEY_R && control_mask) {
                nk_input_key(ctx, NK_KEY_TEXT_REDO, 1);
            } else if (kc == ALLEGRO_KEY_A && control_mask) {
                nk_input_key(ctx, NK_KEY_TEXT_SELECT_ALL, 1);
            } else {
                if (kc != ALLEGRO_KEY_BACKSPACE &&
                    kc != ALLEGRO_KEY_LEFT &&
                    kc != ALLEGRO_KEY_RIGHT &&
                    kc != ALLEGRO_KEY_UP &&
                    kc != ALLEGRO_KEY_DOWN &&
                    kc != ALLEGRO_KEY_HOME &&
                    kc != ALLEGRO_KEY_DELETE &&
                    kc != ALLEGRO_KEY_ENTER &&
                    kc != ALLEGRO_KEY_END &&
                    kc != ALLEGRO_KEY_ESCAPE &&
                    kc != ALLEGRO_KEY_PGDN &&
                    kc != ALLEGRO_KEY_PGUP) {
                    nk_input_unicode(ctx, ev->keyboard.unichar);
                }
            }
        } break;
        default: break;
    }
}

NK_INTERN void
nk_allegro5_clipboard_paste(nk_handle usr, struct nk_text_edit *edit)
{
    char *text = al_get_clipboard_text(allegro5.dsp);
    if (text) nk_textedit_paste(edit, text, nk_strlen(text));
    (void)usr;
    al_free(text);
}

NK_INTERN void
nk_allegro5_clipboard_copy(nk_handle usr, const char *text, int len)
{
    char *str = 0;
    (void)usr;
    if (!len) return;
    str = (char*)malloc((size_t)len+1);
    if (!str) return;
    memcpy(str, text, (size_t)len);
    str[len] = '\0';
    al_set_clipboard_text(allegro5.dsp, str);
    free(str);
}

NK_API struct nk_context*
nk_allegro5_init(NkAllegro5Font *allegro5font, ALLEGRO_DISPLAY *dsp,
    unsigned int width, unsigned int height)
{
    if (!al_init_primitives_addon()) {
        fprintf(stdout, "Unable to initialize required allegro5 primitives addon\n");
        exit(1);
    }

    struct nk_user_font *font = &allegro5font->nk;
    font->userdata = nk_handle_ptr(allegro5font);
    font->height = (float)allegro5font->height;
    font->width = nk_allegro5_font_get_text_width;

    allegro5.dsp = dsp;
    allegro5.width = width;
    allegro5.height = height;
    allegro5.is_touch_down = 0;
    allegro5.touch_down_id = -1;

    nk_init_default(&allegro5.ctx, font);
    allegro5.ctx.clip.copy = nk_allegro5_clipboard_copy;
    allegro5.ctx.clip.paste = nk_allegro5_clipboard_paste;
    allegro5.ctx.clip.userdata = nk_handle_ptr(0);
    return &allegro5.ctx;
}

NK_API
void nk_allegro5_shutdown(void)
{
    nk_free(&allegro5.ctx);
    memset(&allegro5, 0, sizeof(allegro5));
}

#endif /* NK_ALLEGRO5_IMPLEMENTATION */

