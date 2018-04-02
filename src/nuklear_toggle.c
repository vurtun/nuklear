#include "nuklear.h"
#include "nuklear_internal.h"

/* ===============================================================
 *
 *                              TOGGLE
 *
 * ===============================================================*/
NK_LIB int
nk_toggle_behavior(const struct nk_input *in, struct nk_rect select,
    nk_flags *state, int active)
{
    nk_widget_state_reset(state);
    if (nk_button_behavior(state, select, in, NK_BUTTON_DEFAULT)) {
        *state = NK_WIDGET_STATE_ACTIVE;
        active = !active;
    }
    if (*state & NK_WIDGET_STATE_HOVER && !nk_input_is_mouse_prev_hovering_rect(in, select))
        *state |= NK_WIDGET_STATE_ENTERED;
    else if (nk_input_is_mouse_prev_hovering_rect(in, select))
        *state |= NK_WIDGET_STATE_LEFT;
    return active;
}
NK_LIB void
nk_draw_checkbox(struct nk_command_buffer *out,
    nk_flags state, const struct nk_style_toggle *style, int active,
    const struct nk_rect *label, const struct nk_rect *selector,
    const struct nk_rect *cursors, const char *string, int len,
    const struct nk_user_font *font)
{
    const struct nk_style_item *background;
    const struct nk_style_item *cursor;
    struct nk_text text;

    /* select correct colors/images */
    if (state & NK_WIDGET_STATE_HOVER) {
        background = &style->hover;
        cursor = &style->cursor_hover;
        text.text = style->text_hover;
    } else if (state & NK_WIDGET_STATE_ACTIVED) {
        background = &style->hover;
        cursor = &style->cursor_hover;
        text.text = style->text_active;
    } else {
        background = &style->normal;
        cursor = &style->cursor_normal;
        text.text = style->text_normal;
    }

    /* draw background and cursor */
    if (background->type == NK_STYLE_ITEM_COLOR) {
        nk_fill_rect(out, *selector, 0, style->border_color);
        nk_fill_rect(out, nk_shrink_rect(*selector, style->border), 0, background->data.color);
    } else nk_draw_image(out, *selector, &background->data.image, nk_white);
    if (active) {
        if (cursor->type == NK_STYLE_ITEM_IMAGE)
            nk_draw_image(out, *cursors, &cursor->data.image, nk_white);
        else nk_fill_rect(out, *cursors, 0, cursor->data.color);
    }

    text.padding.x = 0;
    text.padding.y = 0;
    text.background = style->text_background;
    nk_widget_text(out, *label, string, len, &text, NK_TEXT_LEFT, font);
}
NK_LIB void
nk_draw_option(struct nk_command_buffer *out,
    nk_flags state, const struct nk_style_toggle *style, int active,
    const struct nk_rect *label, const struct nk_rect *selector,
    const struct nk_rect *cursors, const char *string, int len,
    const struct nk_user_font *font)
{
    const struct nk_style_item *background;
    const struct nk_style_item *cursor;
    struct nk_text text;

    /* select correct colors/images */
    if (state & NK_WIDGET_STATE_HOVER) {
        background = &style->hover;
        cursor = &style->cursor_hover;
        text.text = style->text_hover;
    } else if (state & NK_WIDGET_STATE_ACTIVED) {
        background = &style->hover;
        cursor = &style->cursor_hover;
        text.text = style->text_active;
    } else {
        background = &style->normal;
        cursor = &style->cursor_normal;
        text.text = style->text_normal;
    }

    /* draw background and cursor */
    if (background->type == NK_STYLE_ITEM_COLOR) {
        nk_fill_circle(out, *selector, style->border_color);
        nk_fill_circle(out, nk_shrink_rect(*selector, style->border), background->data.color);
    } else nk_draw_image(out, *selector, &background->data.image, nk_white);
    if (active) {
        if (cursor->type == NK_STYLE_ITEM_IMAGE)
            nk_draw_image(out, *cursors, &cursor->data.image, nk_white);
        else nk_fill_circle(out, *cursors, cursor->data.color);
    }

    text.padding.x = 0;
    text.padding.y = 0;
    text.background = style->text_background;
    nk_widget_text(out, *label, string, len, &text, NK_TEXT_LEFT, font);
}
NK_LIB int
nk_do_toggle(nk_flags *state,
    struct nk_command_buffer *out, struct nk_rect r,
    int *active, const char *str, int len, enum nk_toggle_type type,
    const struct nk_style_toggle *style, const struct nk_input *in,
    const struct nk_user_font *font)
{
    int was_active;
    struct nk_rect bounds;
    struct nk_rect select;
    struct nk_rect cursor;
    struct nk_rect label;

    NK_ASSERT(style);
    NK_ASSERT(out);
    NK_ASSERT(font);
    if (!out || !style || !font || !active)
        return 0;

    r.w = NK_MAX(r.w, font->height + 2 * style->padding.x);
    r.h = NK_MAX(r.h, font->height + 2 * style->padding.y);

    /* add additional touch padding for touch screen devices */
    bounds.x = r.x - style->touch_padding.x;
    bounds.y = r.y - style->touch_padding.y;
    bounds.w = r.w + 2 * style->touch_padding.x;
    bounds.h = r.h + 2 * style->touch_padding.y;

    /* calculate the selector space */
    select.w = font->height;
    select.h = select.w;
    select.y = r.y + r.h/2.0f - select.h/2.0f;
    select.x = r.x;

    /* calculate the bounds of the cursor inside the selector */
    cursor.x = select.x + style->padding.x + style->border;
    cursor.y = select.y + style->padding.y + style->border;
    cursor.w = select.w - (2 * style->padding.x + 2 * style->border);
    cursor.h = select.h - (2 * style->padding.y + 2 * style->border);

    /* label behind the selector */
    label.x = select.x + select.w + style->spacing;
    label.y = select.y;
    label.w = NK_MAX(r.x + r.w, label.x) - label.x;
    label.h = select.w;

    /* update selector */
    was_active = *active;
    *active = nk_toggle_behavior(in, bounds, state, *active);

    /* draw selector */
    if (style->draw_begin)
        style->draw_begin(out, style->userdata);
    if (type == NK_TOGGLE_CHECK) {
        nk_draw_checkbox(out, *state, style, *active, &label, &select, &cursor, str, len, font);
    } else {
        nk_draw_option(out, *state, style, *active, &label, &select, &cursor, str, len, font);
    }
    if (style->draw_end)
        style->draw_end(out, style->userdata);
    return (was_active != *active);
}
/*----------------------------------------------------------------
 *
 *                          CHECKBOX
 *
 * --------------------------------------------------------------*/
NK_API int
nk_check_text(struct nk_context *ctx, const char *text, int len, int active)
{
    struct nk_window *win;
    struct nk_panel *layout;
    const struct nk_input *in;
    const struct nk_style *style;

    struct nk_rect bounds;
    enum nk_widget_layout_states state;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return active;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;

    state = nk_widget(&bounds, ctx);
    if (!state) return active;
    in = (state == NK_WIDGET_ROM || layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;
    nk_do_toggle(&ctx->last_widget_state, &win->buffer, bounds, &active,
        text, len, NK_TOGGLE_CHECK, &style->checkbox, in, style->font);
    return active;
}
NK_API unsigned int
nk_check_flags_text(struct nk_context *ctx, const char *text, int len,
    unsigned int flags, unsigned int value)
{
    int old_active;
    NK_ASSERT(ctx);
    NK_ASSERT(text);
    if (!ctx || !text) return flags;
    old_active = (int)((flags & value) & value);
    if (nk_check_text(ctx, text, len, old_active))
        flags |= value;
    else flags &= ~value;
    return flags;
}
NK_API int
nk_checkbox_text(struct nk_context *ctx, const char *text, int len, int *active)
{
    int old_val;
    NK_ASSERT(ctx);
    NK_ASSERT(text);
    NK_ASSERT(active);
    if (!ctx || !text || !active) return 0;
    old_val = *active;
    *active = nk_check_text(ctx, text, len, *active);
    return old_val != *active;
}
NK_API int
nk_checkbox_flags_text(struct nk_context *ctx, const char *text, int len,
    unsigned int *flags, unsigned int value)
{
    int active;
    NK_ASSERT(ctx);
    NK_ASSERT(text);
    NK_ASSERT(flags);
    if (!ctx || !text || !flags) return 0;

    active = (int)((*flags & value) & value);
    if (nk_checkbox_text(ctx, text, len, &active)) {
        if (active) *flags |= value;
        else *flags &= ~value;
        return 1;
    }
    return 0;
}
NK_API int nk_check_label(struct nk_context *ctx, const char *label, int active)
{
    return nk_check_text(ctx, label, nk_strlen(label), active);
}
NK_API unsigned int nk_check_flags_label(struct nk_context *ctx, const char *label,
    unsigned int flags, unsigned int value)
{
    return nk_check_flags_text(ctx, label, nk_strlen(label), flags, value);
}
NK_API int nk_checkbox_label(struct nk_context *ctx, const char *label, int *active)
{
    return nk_checkbox_text(ctx, label, nk_strlen(label), active);
}
NK_API int nk_checkbox_flags_label(struct nk_context *ctx, const char *label,
    unsigned int *flags, unsigned int value)
{
    return nk_checkbox_flags_text(ctx, label, nk_strlen(label), flags, value);
}
/*----------------------------------------------------------------
 *
 *                          OPTION
 *
 * --------------------------------------------------------------*/
NK_API int
nk_option_text(struct nk_context *ctx, const char *text, int len, int is_active)
{
    struct nk_window *win;
    struct nk_panel *layout;
    const struct nk_input *in;
    const struct nk_style *style;

    struct nk_rect bounds;
    enum nk_widget_layout_states state;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return is_active;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;

    state = nk_widget(&bounds, ctx);
    if (!state) return (int)state;
    in = (state == NK_WIDGET_ROM || layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;
    nk_do_toggle(&ctx->last_widget_state, &win->buffer, bounds, &is_active,
        text, len, NK_TOGGLE_OPTION, &style->option, in, style->font);
    return is_active;
}
NK_API int
nk_radio_text(struct nk_context *ctx, const char *text, int len, int *active)
{
    int old_value;
    NK_ASSERT(ctx);
    NK_ASSERT(text);
    NK_ASSERT(active);
    if (!ctx || !text || !active) return 0;
    old_value = *active;
    *active = nk_option_text(ctx, text, len, old_value);
    return old_value != *active;
}
NK_API int
nk_option_label(struct nk_context *ctx, const char *label, int active)
{
    return nk_option_text(ctx, label, nk_strlen(label), active);
}
NK_API int
nk_radio_label(struct nk_context *ctx, const char *label, int *active)
{
    return nk_radio_text(ctx, label, nk_strlen(label), active);
}

