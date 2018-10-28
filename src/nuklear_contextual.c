#include "nuklear.h"
#include "nuklear_internal.h"

/* ==============================================================
 *
 *                          CONTEXTUAL
 *
 * ===============================================================*/
NK_API int
nk_contextual_begin(struct nk_context *ctx, nk_flags flags, struct nk_vec2 size,
    struct nk_rect trigger_bounds)
{
    struct nk_window *win;
    struct nk_window *popup;
    struct nk_rect body;

    NK_STORAGE const struct nk_rect null_rect = {-1,-1,0,0};
    int is_clicked = 0;
    int is_open = 0;
    int ret = 0;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    ++win->popup.con_count;
    if (ctx->current != ctx->active)
        return 0;

    /* check if currently active contextual is active */
    popup = win->popup.win;
    is_open = (popup && win->popup.type == NK_PANEL_CONTEXTUAL);
    is_clicked = nk_input_mouse_clicked(&ctx->input, NK_BUTTON_RIGHT, trigger_bounds);
    if (win->popup.active_con && win->popup.con_count != win->popup.active_con)
        return 0;
    if (!is_open && win->popup.active_con)
        win->popup.active_con = 0;
    if ((!is_open && !is_clicked))
        return 0;

    /* calculate contextual position on click */
    win->popup.active_con = win->popup.con_count;
    if (is_clicked) {
        body.x = ctx->input.mouse.pos.x;
        body.y = ctx->input.mouse.pos.y;
    } else {
        body.x = popup->bounds.x;
        body.y = popup->bounds.y;
    }
    body.w = size.x;
    body.h = size.y;

    /* start nonblocking contextual popup */
    ret = nk_nonblock_begin(ctx, flags|NK_WINDOW_NO_SCROLLBAR, body,
            null_rect, NK_PANEL_CONTEXTUAL);
    if (ret) win->popup.type = NK_PANEL_CONTEXTUAL;
    else {
        win->popup.active_con = 0;
        win->popup.type = NK_PANEL_NONE;
        if (win->popup.win)
            win->popup.win->flags = 0;
    }
    return ret;
}
NK_API int
nk_contextual_item_text(struct nk_context *ctx, const char *text, int len,
    nk_flags alignment)
{
    struct nk_window *win;
    const struct nk_input *in;
    const struct nk_style *style;

    struct nk_rect bounds;
    enum nk_widget_layout_states state;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    state = nk_widget_fitting(&bounds, ctx, style->contextual_button.padding);
    if (!state) return nk_false;

    in = (state == NK_WIDGET_ROM || win->layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;
    if (nk_do_button_text(&ctx->last_widget_state, &win->buffer, bounds,
        text, len, alignment, NK_BUTTON_DEFAULT, &style->contextual_button, in, style->font)) {
        nk_contextual_close(ctx);
        return nk_true;
    }
    return nk_false;
}
NK_API int
nk_contextual_item_label(struct nk_context *ctx, const char *label, nk_flags align)
{
    return nk_contextual_item_text(ctx, label, nk_strlen(label), align);
}
NK_API int
nk_contextual_item_image_text(struct nk_context *ctx, struct nk_image img,
    const char *text, int len, nk_flags align)
{
    struct nk_window *win;
    const struct nk_input *in;
    const struct nk_style *style;

    struct nk_rect bounds;
    enum nk_widget_layout_states state;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    state = nk_widget_fitting(&bounds, ctx, style->contextual_button.padding);
    if (!state) return nk_false;

    in = (state == NK_WIDGET_ROM || win->layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;
    if (nk_do_button_text_image(&ctx->last_widget_state, &win->buffer, bounds,
        img, text, len, align, NK_BUTTON_DEFAULT, &style->contextual_button, style->font, in)){
        nk_contextual_close(ctx);
        return nk_true;
    }
    return nk_false;
}
NK_API int
nk_contextual_item_image_label(struct nk_context *ctx, struct nk_image img,
    const char *label, nk_flags align)
{
    return nk_contextual_item_image_text(ctx, img, label, nk_strlen(label), align);
}
NK_API int
nk_contextual_item_symbol_text(struct nk_context *ctx, enum nk_symbol_type symbol,
    const char *text, int len, nk_flags align)
{
    struct nk_window *win;
    const struct nk_input *in;
    const struct nk_style *style;

    struct nk_rect bounds;
    enum nk_widget_layout_states state;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    state = nk_widget_fitting(&bounds, ctx, style->contextual_button.padding);
    if (!state) return nk_false;

    in = (state == NK_WIDGET_ROM || win->layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;
    if (nk_do_button_text_symbol(&ctx->last_widget_state, &win->buffer, bounds,
        symbol, text, len, align, NK_BUTTON_DEFAULT, &style->contextual_button, style->font, in)) {
        nk_contextual_close(ctx);
        return nk_true;
    }
    return nk_false;
}
NK_API int
nk_contextual_item_symbol_label(struct nk_context *ctx, enum nk_symbol_type symbol,
    const char *text, nk_flags align)
{
    return nk_contextual_item_symbol_text(ctx, symbol, text, nk_strlen(text), align);
}
NK_API void
nk_contextual_close(struct nk_context *ctx)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout) return;
    nk_popup_close(ctx);
}
NK_API void
nk_contextual_end(struct nk_context *ctx)
{
    struct nk_window *popup;
    struct nk_panel *panel;
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return;

    popup = ctx->current;
    panel = popup->layout;
    NK_ASSERT(popup->parent);
    NK_ASSERT(panel->type & NK_PANEL_SET_POPUP);
    if (panel->flags & NK_WINDOW_DYNAMIC) {
        /* Close behavior
        This is a bit of a hack solution since we do not know before we end our popup
        how big it will be. We therefore do not directly know when a
        click outside the non-blocking popup must close it at that direct frame.
        Instead it will be closed in the next frame.*/
        struct nk_rect body = {0,0,0,0};
        if (panel->at_y < (panel->bounds.y + panel->bounds.h)) {
            struct nk_vec2 padding = nk_panel_get_padding(&ctx->style, panel->type);
            body = panel->bounds;
            body.y = (panel->at_y + panel->footer_height + panel->border + padding.y + panel->row.height);
            body.h = (panel->bounds.y + panel->bounds.h) - body.y;
        }
        {int pressed = nk_input_is_mouse_pressed(&ctx->input, NK_BUTTON_LEFT);
        int in_body = nk_input_is_mouse_hovering_rect(&ctx->input, body);
        if (pressed && in_body)
            popup->flags |= NK_WINDOW_HIDDEN;
        }
    }
    if (popup->flags & NK_WINDOW_HIDDEN)
        popup->seq = 0;
    nk_popup_end(ctx);
    return;
}

