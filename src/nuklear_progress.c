#include "nuklear.h"
#include "nuklear_internal.h"

/* ===============================================================
 *
 *                          PROGRESS
 *
 * ===============================================================*/
NK_LIB nk_size
nk_progress_behavior(nk_flags *state, struct nk_input *in,
    struct nk_rect r, struct nk_rect cursor, nk_size max, nk_size value, int modifiable)
{
    int left_mouse_down = 0;
    int left_mouse_click_in_cursor = 0;

    nk_widget_state_reset(state);
    if (!in || !modifiable) return value;
    left_mouse_down = in && in->mouse.buttons[NK_BUTTON_LEFT].down;
    left_mouse_click_in_cursor = in && nk_input_has_mouse_click_down_in_rect(in,
            NK_BUTTON_LEFT, cursor, nk_true);
    if (nk_input_is_mouse_hovering_rect(in, r))
        *state = NK_WIDGET_STATE_HOVERED;

    if (in && left_mouse_down && left_mouse_click_in_cursor) {
        if (left_mouse_down && left_mouse_click_in_cursor) {
            float ratio = NK_MAX(0, (float)(in->mouse.pos.x - cursor.x)) / (float)cursor.w;
            value = (nk_size)NK_CLAMP(0, (float)max * ratio, (float)max);
            in->mouse.buttons[NK_BUTTON_LEFT].clicked_pos.x = cursor.x + cursor.w/2.0f;
            *state |= NK_WIDGET_STATE_ACTIVE;
        }
    }
    /* set progressbar widget state */
    if (*state & NK_WIDGET_STATE_HOVER && !nk_input_is_mouse_prev_hovering_rect(in, r))
        *state |= NK_WIDGET_STATE_ENTERED;
    else if (nk_input_is_mouse_prev_hovering_rect(in, r))
        *state |= NK_WIDGET_STATE_LEFT;
    return value;
}
NK_LIB void
nk_draw_progress(struct nk_command_buffer *out, nk_flags state,
    const struct nk_style_progress *style, const struct nk_rect *bounds,
    const struct nk_rect *scursor, nk_size value, nk_size max)
{
    const struct nk_style_item *background;
    const struct nk_style_item *cursor;

    NK_UNUSED(max);
    NK_UNUSED(value);

    /* select correct colors/images to draw */
    if (state & NK_WIDGET_STATE_ACTIVED) {
        background = &style->active;
        cursor = &style->cursor_active;
    } else if (state & NK_WIDGET_STATE_HOVER){
        background = &style->hover;
        cursor = &style->cursor_hover;
    } else {
        background = &style->normal;
        cursor = &style->cursor_normal;
    }

    /* draw background */
    if (background->type == NK_STYLE_ITEM_COLOR) {
        nk_fill_rect(out, *bounds, style->rounding, background->data.color);
        nk_stroke_rect(out, *bounds, style->rounding, style->border, style->border_color);
    } else nk_draw_image(out, *bounds, &background->data.image, nk_white);

    /* draw cursor */
    if (cursor->type == NK_STYLE_ITEM_COLOR) {
        nk_fill_rect(out, *scursor, style->rounding, cursor->data.color);
        nk_stroke_rect(out, *scursor, style->rounding, style->border, style->border_color);
    } else nk_draw_image(out, *scursor, &cursor->data.image, nk_white);
}
NK_LIB nk_size
nk_do_progress(nk_flags *state,
    struct nk_command_buffer *out, struct nk_rect bounds,
    nk_size value, nk_size max, int modifiable,
    const struct nk_style_progress *style, struct nk_input *in)
{
    float prog_scale;
    nk_size prog_value;
    struct nk_rect cursor;

    NK_ASSERT(style);
    NK_ASSERT(out);
    if (!out || !style) return 0;

    /* calculate progressbar cursor */
    cursor.w = NK_MAX(bounds.w, 2 * style->padding.x + 2 * style->border);
    cursor.h = NK_MAX(bounds.h, 2 * style->padding.y + 2 * style->border);
    cursor = nk_pad_rect(bounds, nk_vec2(style->padding.x + style->border, style->padding.y + style->border));
    prog_scale = (float)value / (float)max;

    /* update progressbar */
    prog_value = NK_MIN(value, max);
    prog_value = nk_progress_behavior(state, in, bounds, cursor,max, prog_value, modifiable);
    cursor.w = cursor.w * prog_scale;

    /* draw progressbar */
    if (style->draw_begin) style->draw_begin(out, style->userdata);
    nk_draw_progress(out, *state, style, &bounds, &cursor, value, max);
    if (style->draw_end) style->draw_end(out, style->userdata);
    return prog_value;
}
NK_API int
nk_progress(struct nk_context *ctx, nk_size *cur, nk_size max, int is_modifyable)
{
    struct nk_window *win;
    struct nk_panel *layout;
    const struct nk_style *style;
    struct nk_input *in;

    struct nk_rect bounds;
    enum nk_widget_layout_states state;
    nk_size old_value;

    NK_ASSERT(ctx);
    NK_ASSERT(cur);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout || !cur)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;
    state = nk_widget(&bounds, ctx);
    if (!state) return 0;

    in = (state == NK_WIDGET_ROM || layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;
    old_value = *cur;
    *cur = nk_do_progress(&ctx->last_widget_state, &win->buffer, bounds,
            *cur, max, is_modifyable, &style->progress, in);
    return (*cur != old_value);
}
NK_API nk_size
nk_prog(struct nk_context *ctx, nk_size cur, nk_size max, int modifyable)
{
    nk_progress(ctx, &cur, max, modifyable);
    return cur;
}

