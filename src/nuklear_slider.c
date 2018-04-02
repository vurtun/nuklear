#include "nuklear.h"
#include "nuklear_internal.h"

/* ===============================================================
 *
 *                              SLIDER
 *
 * ===============================================================*/
NK_LIB float
nk_slider_behavior(nk_flags *state, struct nk_rect *logical_cursor,
    struct nk_rect *visual_cursor, struct nk_input *in,
    struct nk_rect bounds, float slider_min, float slider_max, float slider_value,
    float slider_step, float slider_steps)
{
    int left_mouse_down;
    int left_mouse_click_in_cursor;

    /* check if visual cursor is being dragged */
    nk_widget_state_reset(state);
    left_mouse_down = in && in->mouse.buttons[NK_BUTTON_LEFT].down;
    left_mouse_click_in_cursor = in && nk_input_has_mouse_click_down_in_rect(in,
            NK_BUTTON_LEFT, *visual_cursor, nk_true);

    if (left_mouse_down && left_mouse_click_in_cursor) {
        float ratio = 0;
        const float d = in->mouse.pos.x - (visual_cursor->x+visual_cursor->w*0.5f);
        const float pxstep = bounds.w / slider_steps;

        /* only update value if the next slider step is reached */
        *state = NK_WIDGET_STATE_ACTIVE;
        if (NK_ABS(d) >= pxstep) {
            const float steps = (float)((int)(NK_ABS(d) / pxstep));
            slider_value += (d > 0) ? (slider_step*steps) : -(slider_step*steps);
            slider_value = NK_CLAMP(slider_min, slider_value, slider_max);
            ratio = (slider_value - slider_min)/slider_step;
            logical_cursor->x = bounds.x + (logical_cursor->w * ratio);
            in->mouse.buttons[NK_BUTTON_LEFT].clicked_pos.x = logical_cursor->x;
        }
    }

    /* slider widget state */
    if (nk_input_is_mouse_hovering_rect(in, bounds))
        *state = NK_WIDGET_STATE_HOVERED;
    if (*state & NK_WIDGET_STATE_HOVER &&
        !nk_input_is_mouse_prev_hovering_rect(in, bounds))
        *state |= NK_WIDGET_STATE_ENTERED;
    else if (nk_input_is_mouse_prev_hovering_rect(in, bounds))
        *state |= NK_WIDGET_STATE_LEFT;
    return slider_value;
}
NK_LIB void
nk_draw_slider(struct nk_command_buffer *out, nk_flags state,
    const struct nk_style_slider *style, const struct nk_rect *bounds,
    const struct nk_rect *visual_cursor, float min, float value, float max)
{
    struct nk_rect fill;
    struct nk_rect bar;
    const struct nk_style_item *background;

    /* select correct slider images/colors */
    struct nk_color bar_color;
    const struct nk_style_item *cursor;

    NK_UNUSED(min);
    NK_UNUSED(max);
    NK_UNUSED(value);

    if (state & NK_WIDGET_STATE_ACTIVED) {
        background = &style->active;
        bar_color = style->bar_active;
        cursor = &style->cursor_active;
    } else if (state & NK_WIDGET_STATE_HOVER) {
        background = &style->hover;
        bar_color = style->bar_hover;
        cursor = &style->cursor_hover;
    } else {
        background = &style->normal;
        bar_color = style->bar_normal;
        cursor = &style->cursor_normal;
    }
    /* calculate slider background bar */
    bar.x = bounds->x;
    bar.y = (visual_cursor->y + visual_cursor->h/2) - bounds->h/12;
    bar.w = bounds->w;
    bar.h = bounds->h/6;

    /* filled background bar style */
    fill.w = (visual_cursor->x + (visual_cursor->w/2.0f)) - bar.x;
    fill.x = bar.x;
    fill.y = bar.y;
    fill.h = bar.h;

    /* draw background */
    if (background->type == NK_STYLE_ITEM_IMAGE) {
        nk_draw_image(out, *bounds, &background->data.image, nk_white);
    } else {
        nk_fill_rect(out, *bounds, style->rounding, background->data.color);
        nk_stroke_rect(out, *bounds, style->rounding, style->border, style->border_color);
    }

    /* draw slider bar */
    nk_fill_rect(out, bar, style->rounding, bar_color);
    nk_fill_rect(out, fill, style->rounding, style->bar_filled);

    /* draw cursor */
    if (cursor->type == NK_STYLE_ITEM_IMAGE)
        nk_draw_image(out, *visual_cursor, &cursor->data.image, nk_white);
    else nk_fill_circle(out, *visual_cursor, cursor->data.color);
}
NK_LIB float
nk_do_slider(nk_flags *state,
    struct nk_command_buffer *out, struct nk_rect bounds,
    float min, float val, float max, float step,
    const struct nk_style_slider *style, struct nk_input *in,
    const struct nk_user_font *font)
{
    float slider_range;
    float slider_min;
    float slider_max;
    float slider_value;
    float slider_steps;
    float cursor_offset;

    struct nk_rect visual_cursor;
    struct nk_rect logical_cursor;

    NK_ASSERT(style);
    NK_ASSERT(out);
    if (!out || !style)
        return 0;

    /* remove padding from slider bounds */
    bounds.x = bounds.x + style->padding.x;
    bounds.y = bounds.y + style->padding.y;
    bounds.h = NK_MAX(bounds.h, 2*style->padding.y);
    bounds.w = NK_MAX(bounds.w, 2*style->padding.x + style->cursor_size.x);
    bounds.w -= 2 * style->padding.x;
    bounds.h -= 2 * style->padding.y;

    /* optional buttons */
    if (style->show_buttons) {
        nk_flags ws;
        struct nk_rect button;
        button.y = bounds.y;
        button.w = bounds.h;
        button.h = bounds.h;

        /* decrement button */
        button.x = bounds.x;
        if (nk_do_button_symbol(&ws, out, button, style->dec_symbol, NK_BUTTON_DEFAULT,
            &style->dec_button, in, font))
            val -= step;

        /* increment button */
        button.x = (bounds.x + bounds.w) - button.w;
        if (nk_do_button_symbol(&ws, out, button, style->inc_symbol, NK_BUTTON_DEFAULT,
            &style->inc_button, in, font))
            val += step;

        bounds.x = bounds.x + button.w + style->spacing.x;
        bounds.w = bounds.w - (2*button.w + 2*style->spacing.x);
    }

    /* remove one cursor size to support visual cursor */
    bounds.x += style->cursor_size.x*0.5f;
    bounds.w -= style->cursor_size.x;

    /* make sure the provided values are correct */
    slider_max = NK_MAX(min, max);
    slider_min = NK_MIN(min, max);
    slider_value = NK_CLAMP(slider_min, val, slider_max);
    slider_range = slider_max - slider_min;
    slider_steps = slider_range / step;
    cursor_offset = (slider_value - slider_min) / step;

    /* calculate cursor
    Basically you have two cursors. One for visual representation and interaction
    and one for updating the actual cursor value. */
    logical_cursor.h = bounds.h;
    logical_cursor.w = bounds.w / slider_steps;
    logical_cursor.x = bounds.x + (logical_cursor.w * cursor_offset);
    logical_cursor.y = bounds.y;

    visual_cursor.h = style->cursor_size.y;
    visual_cursor.w = style->cursor_size.x;
    visual_cursor.y = (bounds.y + bounds.h*0.5f) - visual_cursor.h*0.5f;
    visual_cursor.x = logical_cursor.x - visual_cursor.w*0.5f;

    slider_value = nk_slider_behavior(state, &logical_cursor, &visual_cursor,
        in, bounds, slider_min, slider_max, slider_value, step, slider_steps);
    visual_cursor.x = logical_cursor.x - visual_cursor.w*0.5f;

    /* draw slider */
    if (style->draw_begin) style->draw_begin(out, style->userdata);
    nk_draw_slider(out, *state, style, &bounds, &visual_cursor, slider_min, slider_value, slider_max);
    if (style->draw_end) style->draw_end(out, style->userdata);
    return slider_value;
}
NK_API int
nk_slider_float(struct nk_context *ctx, float min_value, float *value, float max_value,
    float value_step)
{
    struct nk_window *win;
    struct nk_panel *layout;
    struct nk_input *in;
    const struct nk_style *style;

    int ret = 0;
    float old_value;
    struct nk_rect bounds;
    enum nk_widget_layout_states state;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    NK_ASSERT(value);
    if (!ctx || !ctx->current || !ctx->current->layout || !value)
        return ret;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;

    state = nk_widget(&bounds, ctx);
    if (!state) return ret;
    in = (/*state == NK_WIDGET_ROM || */ layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;

    old_value = *value;
    *value = nk_do_slider(&ctx->last_widget_state, &win->buffer, bounds, min_value,
                old_value, max_value, value_step, &style->slider, in, style->font);
    return (old_value > *value || old_value < *value);
}
NK_API float
nk_slide_float(struct nk_context *ctx, float min, float val, float max, float step)
{
    nk_slider_float(ctx, min, &val, max, step); return val;
}
NK_API int
nk_slide_int(struct nk_context *ctx, int min, int val, int max, int step)
{
    float value = (float)val;
    nk_slider_float(ctx, (float)min, &value, (float)max, (float)step);
    return (int)value;
}
NK_API int
nk_slider_int(struct nk_context *ctx, int min, int *val, int max, int step)
{
    int ret;
    float value = (float)*val;
    ret = nk_slider_float(ctx, (float)min, &value, (float)max, (float)step);
    *val =  (int)value;
    return ret;
}

