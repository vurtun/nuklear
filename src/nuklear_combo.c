#include "nuklear.h"
#include "nuklear_internal.h"

/* ==============================================================
 *
 *                          COMBO
 *
 * ===============================================================*/
NK_INTERN int
nk_combo_begin(struct nk_context *ctx, struct nk_window *win,
    struct nk_vec2 size, int is_clicked, struct nk_rect header)
{
    struct nk_window *popup;
    int is_open = 0;
    int is_active = 0;
    struct nk_rect body;
    nk_hash hash;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    popup = win->popup.win;
    body.x = header.x;
    body.w = size.x;
    body.y = header.y + header.h-ctx->style.window.combo_border;
    body.h = size.y;

    hash = win->popup.combo_count++;
    is_open = (popup) ? nk_true:nk_false;
    is_active = (popup && (win->popup.name == hash) && win->popup.type == NK_PANEL_COMBO);
    if ((is_clicked && is_open && !is_active) || (is_open && !is_active) ||
        (!is_open && !is_active && !is_clicked)) return 0;
    if (!nk_nonblock_begin(ctx, 0, body,
        (is_clicked && is_open)?nk_rect(0,0,0,0):header, NK_PANEL_COMBO)) return 0;

    win->popup.type = NK_PANEL_COMBO;
    win->popup.name = hash;
    return 1;
}
NK_API int
nk_combo_begin_text(struct nk_context *ctx, const char *selected, int len,
    struct nk_vec2 size)
{
    const struct nk_input *in;
    struct nk_window *win;
    struct nk_style *style;

    enum nk_widget_layout_states s;
    int is_clicked = nk_false;
    struct nk_rect header;
    const struct nk_style_item *background;
    struct nk_text text;

    NK_ASSERT(ctx);
    NK_ASSERT(selected);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout || !selected)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    s = nk_widget(&header, ctx);
    if (s == NK_WIDGET_INVALID)
        return 0;

    in = (win->layout->flags & NK_WINDOW_ROM || s == NK_WIDGET_ROM)? 0: &ctx->input;
    if (nk_button_behavior(&ctx->last_widget_state, header, in, NK_BUTTON_DEFAULT))
        is_clicked = nk_true;

    /* draw combo box header background and border */
    if (ctx->last_widget_state & NK_WIDGET_STATE_ACTIVED) {
        background = &style->combo.active;
        text.text = style->combo.label_active;
    } else if (ctx->last_widget_state & NK_WIDGET_STATE_HOVER) {
        background = &style->combo.hover;
        text.text = style->combo.label_hover;
    } else {
        background = &style->combo.normal;
        text.text = style->combo.label_normal;
    }
    if (background->type == NK_STYLE_ITEM_IMAGE) {
        text.background = nk_rgba(0,0,0,0);
        nk_draw_image(&win->buffer, header, &background->data.image, nk_white);
    } else {
        text.background = background->data.color;
        nk_fill_rect(&win->buffer, header, style->combo.rounding, background->data.color);
        nk_stroke_rect(&win->buffer, header, style->combo.rounding, style->combo.border, style->combo.border_color);
    }
    {
        /* print currently selected text item */
        struct nk_rect label;
        struct nk_rect button;
        struct nk_rect content;

        enum nk_symbol_type sym;
        if (ctx->last_widget_state & NK_WIDGET_STATE_HOVER)
            sym = style->combo.sym_hover;
        else if (is_clicked)
            sym = style->combo.sym_active;
        else sym = style->combo.sym_normal;

        /* calculate button */
        button.w = header.h - 2 * style->combo.button_padding.y;
        button.x = (header.x + header.w - header.h) - style->combo.button_padding.x;
        button.y = header.y + style->combo.button_padding.y;
        button.h = button.w;

        content.x = button.x + style->combo.button.padding.x;
        content.y = button.y + style->combo.button.padding.y;
        content.w = button.w - 2 * style->combo.button.padding.x;
        content.h = button.h - 2 * style->combo.button.padding.y;

        /* draw selected label */
        text.padding = nk_vec2(0,0);
        label.x = header.x + style->combo.content_padding.x;
        label.y = header.y + style->combo.content_padding.y;
        label.w = button.x - (style->combo.content_padding.x + style->combo.spacing.x) - label.x;;
        label.h = header.h - 2 * style->combo.content_padding.y;
        nk_widget_text(&win->buffer, label, selected, len, &text,
            NK_TEXT_LEFT, ctx->style.font);

        /* draw open/close button */
        nk_draw_button_symbol(&win->buffer, &button, &content, ctx->last_widget_state,
            &ctx->style.combo.button, sym, style->font);
    }
    return nk_combo_begin(ctx, win, size, is_clicked, header);
}
NK_API int
nk_combo_begin_label(struct nk_context *ctx, const char *selected, struct nk_vec2 size)
{
    return nk_combo_begin_text(ctx, selected, nk_strlen(selected), size);
}
NK_API int
nk_combo_begin_color(struct nk_context *ctx, struct nk_color color, struct nk_vec2 size)
{
    struct nk_window *win;
    struct nk_style *style;
    const struct nk_input *in;

    struct nk_rect header;
    int is_clicked = nk_false;
    enum nk_widget_layout_states s;
    const struct nk_style_item *background;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    s = nk_widget(&header, ctx);
    if (s == NK_WIDGET_INVALID)
        return 0;

    in = (win->layout->flags & NK_WINDOW_ROM || s == NK_WIDGET_ROM)? 0: &ctx->input;
    if (nk_button_behavior(&ctx->last_widget_state, header, in, NK_BUTTON_DEFAULT))
        is_clicked = nk_true;

    /* draw combo box header background and border */
    if (ctx->last_widget_state & NK_WIDGET_STATE_ACTIVED)
        background = &style->combo.active;
    else if (ctx->last_widget_state & NK_WIDGET_STATE_HOVER)
        background = &style->combo.hover;
    else background = &style->combo.normal;

    if (background->type == NK_STYLE_ITEM_IMAGE) {
        nk_draw_image(&win->buffer, header, &background->data.image,nk_white);
    } else {
        nk_fill_rect(&win->buffer, header, style->combo.rounding, background->data.color);
        nk_stroke_rect(&win->buffer, header, style->combo.rounding, style->combo.border, style->combo.border_color);
    }
    {
        struct nk_rect content;
        struct nk_rect button;
        struct nk_rect bounds;

        enum nk_symbol_type sym;
        if (ctx->last_widget_state & NK_WIDGET_STATE_HOVER)
            sym = style->combo.sym_hover;
        else if (is_clicked)
            sym = style->combo.sym_active;
        else sym = style->combo.sym_normal;

        /* calculate button */
        button.w = header.h - 2 * style->combo.button_padding.y;
        button.x = (header.x + header.w - header.h) - style->combo.button_padding.x;
        button.y = header.y + style->combo.button_padding.y;
        button.h = button.w;

        content.x = button.x + style->combo.button.padding.x;
        content.y = button.y + style->combo.button.padding.y;
        content.w = button.w - 2 * style->combo.button.padding.x;
        content.h = button.h - 2 * style->combo.button.padding.y;

        /* draw color */
        bounds.h = header.h - 4 * style->combo.content_padding.y;
        bounds.y = header.y + 2 * style->combo.content_padding.y;
        bounds.x = header.x + 2 * style->combo.content_padding.x;
        bounds.w = (button.x - (style->combo.content_padding.x + style->combo.spacing.x)) - bounds.x;
        nk_fill_rect(&win->buffer, bounds, 0, color);

        /* draw open/close button */
        nk_draw_button_symbol(&win->buffer, &button, &content, ctx->last_widget_state,
            &ctx->style.combo.button, sym, style->font);
    }
    return nk_combo_begin(ctx, win, size, is_clicked, header);
}
NK_API int
nk_combo_begin_symbol(struct nk_context *ctx, enum nk_symbol_type symbol, struct nk_vec2 size)
{
    struct nk_window *win;
    struct nk_style *style;
    const struct nk_input *in;

    struct nk_rect header;
    int is_clicked = nk_false;
    enum nk_widget_layout_states s;
    const struct nk_style_item *background;
    struct nk_color sym_background;
    struct nk_color symbol_color;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    s = nk_widget(&header, ctx);
    if (s == NK_WIDGET_INVALID)
        return 0;

    in = (win->layout->flags & NK_WINDOW_ROM || s == NK_WIDGET_ROM)? 0: &ctx->input;
    if (nk_button_behavior(&ctx->last_widget_state, header, in, NK_BUTTON_DEFAULT))
        is_clicked = nk_true;

    /* draw combo box header background and border */
    if (ctx->last_widget_state & NK_WIDGET_STATE_ACTIVED) {
        background = &style->combo.active;
        symbol_color = style->combo.symbol_active;
    } else if (ctx->last_widget_state & NK_WIDGET_STATE_HOVER) {
        background = &style->combo.hover;
        symbol_color = style->combo.symbol_hover;
    } else {
        background = &style->combo.normal;
        symbol_color = style->combo.symbol_hover;
    }

    if (background->type == NK_STYLE_ITEM_IMAGE) {
        sym_background = nk_rgba(0,0,0,0);
        nk_draw_image(&win->buffer, header, &background->data.image, nk_white);
    } else {
        sym_background = background->data.color;
        nk_fill_rect(&win->buffer, header, style->combo.rounding, background->data.color);
        nk_stroke_rect(&win->buffer, header, style->combo.rounding, style->combo.border, style->combo.border_color);
    }
    {
        struct nk_rect bounds = {0,0,0,0};
        struct nk_rect content;
        struct nk_rect button;

        enum nk_symbol_type sym;
        if (ctx->last_widget_state & NK_WIDGET_STATE_HOVER)
            sym = style->combo.sym_hover;
        else if (is_clicked)
            sym = style->combo.sym_active;
        else sym = style->combo.sym_normal;

        /* calculate button */
        button.w = header.h - 2 * style->combo.button_padding.y;
        button.x = (header.x + header.w - header.h) - style->combo.button_padding.y;
        button.y = header.y + style->combo.button_padding.y;
        button.h = button.w;

        content.x = button.x + style->combo.button.padding.x;
        content.y = button.y + style->combo.button.padding.y;
        content.w = button.w - 2 * style->combo.button.padding.x;
        content.h = button.h - 2 * style->combo.button.padding.y;

        /* draw symbol */
        bounds.h = header.h - 2 * style->combo.content_padding.y;
        bounds.y = header.y + style->combo.content_padding.y;
        bounds.x = header.x + style->combo.content_padding.x;
        bounds.w = (button.x - style->combo.content_padding.y) - bounds.x;
        nk_draw_symbol(&win->buffer, symbol, bounds, sym_background, symbol_color,
            1.0f, style->font);

        /* draw open/close button */
        nk_draw_button_symbol(&win->buffer, &bounds, &content, ctx->last_widget_state,
            &ctx->style.combo.button, sym, style->font);
    }
    return nk_combo_begin(ctx, win, size, is_clicked, header);
}
NK_API int
nk_combo_begin_symbol_text(struct nk_context *ctx, const char *selected, int len,
    enum nk_symbol_type symbol, struct nk_vec2 size)
{
    struct nk_window *win;
    struct nk_style *style;
    struct nk_input *in;

    struct nk_rect header;
    int is_clicked = nk_false;
    enum nk_widget_layout_states s;
    const struct nk_style_item *background;
    struct nk_color symbol_color;
    struct nk_text text;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    s = nk_widget(&header, ctx);
    if (!s) return 0;

    in = (win->layout->flags & NK_WINDOW_ROM || s == NK_WIDGET_ROM)? 0: &ctx->input;
    if (nk_button_behavior(&ctx->last_widget_state, header, in, NK_BUTTON_DEFAULT))
        is_clicked = nk_true;

    /* draw combo box header background and border */
    if (ctx->last_widget_state & NK_WIDGET_STATE_ACTIVED) {
        background = &style->combo.active;
        symbol_color = style->combo.symbol_active;
        text.text = style->combo.label_active;
    } else if (ctx->last_widget_state & NK_WIDGET_STATE_HOVER) {
        background = &style->combo.hover;
        symbol_color = style->combo.symbol_hover;
        text.text = style->combo.label_hover;
    } else {
        background = &style->combo.normal;
        symbol_color = style->combo.symbol_normal;
        text.text = style->combo.label_normal;
    }
    if (background->type == NK_STYLE_ITEM_IMAGE) {
        text.background = nk_rgba(0,0,0,0);
        nk_draw_image(&win->buffer, header, &background->data.image, nk_white);
    } else {
        text.background = background->data.color;
        nk_fill_rect(&win->buffer, header, style->combo.rounding, background->data.color);
        nk_stroke_rect(&win->buffer, header, style->combo.rounding, style->combo.border, style->combo.border_color);
    }
    {
        struct nk_rect content;
        struct nk_rect button;
        struct nk_rect label;
        struct nk_rect image;

        enum nk_symbol_type sym;
        if (ctx->last_widget_state & NK_WIDGET_STATE_HOVER)
            sym = style->combo.sym_hover;
        else if (is_clicked)
            sym = style->combo.sym_active;
        else sym = style->combo.sym_normal;

        /* calculate button */
        button.w = header.h - 2 * style->combo.button_padding.y;
        button.x = (header.x + header.w - header.h) - style->combo.button_padding.x;
        button.y = header.y + style->combo.button_padding.y;
        button.h = button.w;

        content.x = button.x + style->combo.button.padding.x;
        content.y = button.y + style->combo.button.padding.y;
        content.w = button.w - 2 * style->combo.button.padding.x;
        content.h = button.h - 2 * style->combo.button.padding.y;
        nk_draw_button_symbol(&win->buffer, &button, &content, ctx->last_widget_state,
            &ctx->style.combo.button, sym, style->font);

        /* draw symbol */
        image.x = header.x + style->combo.content_padding.x;
        image.y = header.y + style->combo.content_padding.y;
        image.h = header.h - 2 * style->combo.content_padding.y;
        image.w = image.h;
        nk_draw_symbol(&win->buffer, symbol, image, text.background, symbol_color,
            1.0f, style->font);

        /* draw label */
        text.padding = nk_vec2(0,0);
        label.x = image.x + image.w + style->combo.spacing.x + style->combo.content_padding.x;
        label.y = header.y + style->combo.content_padding.y;
        label.w = (button.x - style->combo.content_padding.x) - label.x;
        label.h = header.h - 2 * style->combo.content_padding.y;
        nk_widget_text(&win->buffer, label, selected, len, &text, NK_TEXT_LEFT, style->font);
    }
    return nk_combo_begin(ctx, win, size, is_clicked, header);
}
NK_API int
nk_combo_begin_image(struct nk_context *ctx, struct nk_image img, struct nk_vec2 size)
{
    struct nk_window *win;
    struct nk_style *style;
    const struct nk_input *in;

    struct nk_rect header;
    int is_clicked = nk_false;
    enum nk_widget_layout_states s;
    const struct nk_style_item *background;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    s = nk_widget(&header, ctx);
    if (s == NK_WIDGET_INVALID)
        return 0;

    in = (win->layout->flags & NK_WINDOW_ROM || s == NK_WIDGET_ROM)? 0: &ctx->input;
    if (nk_button_behavior(&ctx->last_widget_state, header, in, NK_BUTTON_DEFAULT))
        is_clicked = nk_true;

    /* draw combo box header background and border */
    if (ctx->last_widget_state & NK_WIDGET_STATE_ACTIVED)
        background = &style->combo.active;
    else if (ctx->last_widget_state & NK_WIDGET_STATE_HOVER)
        background = &style->combo.hover;
    else background = &style->combo.normal;

    if (background->type == NK_STYLE_ITEM_IMAGE) {
        nk_draw_image(&win->buffer, header, &background->data.image, nk_white);
    } else {
        nk_fill_rect(&win->buffer, header, style->combo.rounding, background->data.color);
        nk_stroke_rect(&win->buffer, header, style->combo.rounding, style->combo.border, style->combo.border_color);
    }
    {
        struct nk_rect bounds = {0,0,0,0};
        struct nk_rect content;
        struct nk_rect button;

        enum nk_symbol_type sym;
        if (ctx->last_widget_state & NK_WIDGET_STATE_HOVER)
            sym = style->combo.sym_hover;
        else if (is_clicked)
            sym = style->combo.sym_active;
        else sym = style->combo.sym_normal;

        /* calculate button */
        button.w = header.h - 2 * style->combo.button_padding.y;
        button.x = (header.x + header.w - header.h) - style->combo.button_padding.y;
        button.y = header.y + style->combo.button_padding.y;
        button.h = button.w;

        content.x = button.x + style->combo.button.padding.x;
        content.y = button.y + style->combo.button.padding.y;
        content.w = button.w - 2 * style->combo.button.padding.x;
        content.h = button.h - 2 * style->combo.button.padding.y;

        /* draw image */
        bounds.h = header.h - 2 * style->combo.content_padding.y;
        bounds.y = header.y + style->combo.content_padding.y;
        bounds.x = header.x + style->combo.content_padding.x;
        bounds.w = (button.x - style->combo.content_padding.y) - bounds.x;
        nk_draw_image(&win->buffer, bounds, &img, nk_white);

        /* draw open/close button */
        nk_draw_button_symbol(&win->buffer, &bounds, &content, ctx->last_widget_state,
            &ctx->style.combo.button, sym, style->font);
    }
    return nk_combo_begin(ctx, win, size, is_clicked, header);
}
NK_API int
nk_combo_begin_image_text(struct nk_context *ctx, const char *selected, int len,
    struct nk_image img, struct nk_vec2 size)
{
    struct nk_window *win;
    struct nk_style *style;
    struct nk_input *in;

    struct nk_rect header;
    int is_clicked = nk_false;
    enum nk_widget_layout_states s;
    const struct nk_style_item *background;
    struct nk_text text;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    s = nk_widget(&header, ctx);
    if (!s) return 0;

    in = (win->layout->flags & NK_WINDOW_ROM || s == NK_WIDGET_ROM)? 0: &ctx->input;
    if (nk_button_behavior(&ctx->last_widget_state, header, in, NK_BUTTON_DEFAULT))
        is_clicked = nk_true;

    /* draw combo box header background and border */
    if (ctx->last_widget_state & NK_WIDGET_STATE_ACTIVED) {
        background = &style->combo.active;
        text.text = style->combo.label_active;
    } else if (ctx->last_widget_state & NK_WIDGET_STATE_HOVER) {
        background = &style->combo.hover;
        text.text = style->combo.label_hover;
    } else {
        background = &style->combo.normal;
        text.text = style->combo.label_normal;
    }
    if (background->type == NK_STYLE_ITEM_IMAGE) {
        text.background = nk_rgba(0,0,0,0);
        nk_draw_image(&win->buffer, header, &background->data.image, nk_white);
    } else {
        text.background = background->data.color;
        nk_fill_rect(&win->buffer, header, style->combo.rounding, background->data.color);
        nk_stroke_rect(&win->buffer, header, style->combo.rounding, style->combo.border, style->combo.border_color);
    }
    {
        struct nk_rect content;
        struct nk_rect button;
        struct nk_rect label;
        struct nk_rect image;

        enum nk_symbol_type sym;
        if (ctx->last_widget_state & NK_WIDGET_STATE_HOVER)
            sym = style->combo.sym_hover;
        else if (is_clicked)
            sym = style->combo.sym_active;
        else sym = style->combo.sym_normal;

        /* calculate button */
        button.w = header.h - 2 * style->combo.button_padding.y;
        button.x = (header.x + header.w - header.h) - style->combo.button_padding.x;
        button.y = header.y + style->combo.button_padding.y;
        button.h = button.w;

        content.x = button.x + style->combo.button.padding.x;
        content.y = button.y + style->combo.button.padding.y;
        content.w = button.w - 2 * style->combo.button.padding.x;
        content.h = button.h - 2 * style->combo.button.padding.y;
        nk_draw_button_symbol(&win->buffer, &button, &content, ctx->last_widget_state,
            &ctx->style.combo.button, sym, style->font);

        /* draw image */
        image.x = header.x + style->combo.content_padding.x;
        image.y = header.y + style->combo.content_padding.y;
        image.h = header.h - 2 * style->combo.content_padding.y;
        image.w = image.h;
        nk_draw_image(&win->buffer, image, &img, nk_white);

        /* draw label */
        text.padding = nk_vec2(0,0);
        label.x = image.x + image.w + style->combo.spacing.x + style->combo.content_padding.x;
        label.y = header.y + style->combo.content_padding.y;
        label.w = (button.x - style->combo.content_padding.x) - label.x;
        label.h = header.h - 2 * style->combo.content_padding.y;
        nk_widget_text(&win->buffer, label, selected, len, &text, NK_TEXT_LEFT, style->font);
    }
    return nk_combo_begin(ctx, win, size, is_clicked, header);
}
NK_API int
nk_combo_begin_symbol_label(struct nk_context *ctx,
    const char *selected, enum nk_symbol_type type, struct nk_vec2 size)
{
    return nk_combo_begin_symbol_text(ctx, selected, nk_strlen(selected), type, size);
}
NK_API int
nk_combo_begin_image_label(struct nk_context *ctx,
    const char *selected, struct nk_image img, struct nk_vec2 size)
{
    return nk_combo_begin_image_text(ctx, selected, nk_strlen(selected), img, size);
}
NK_API int
nk_combo_item_text(struct nk_context *ctx, const char *text, int len,nk_flags align)
{
    return nk_contextual_item_text(ctx, text, len, align);
}
NK_API int
nk_combo_item_label(struct nk_context *ctx, const char *label, nk_flags align)
{
    return nk_contextual_item_label(ctx, label, align);
}
NK_API int
nk_combo_item_image_text(struct nk_context *ctx, struct nk_image img, const char *text,
    int len, nk_flags alignment)
{
    return nk_contextual_item_image_text(ctx, img, text, len, alignment);
}
NK_API int
nk_combo_item_image_label(struct nk_context *ctx, struct nk_image img,
    const char *text, nk_flags alignment)
{
    return nk_contextual_item_image_label(ctx, img, text, alignment);
}
NK_API int
nk_combo_item_symbol_text(struct nk_context *ctx, enum nk_symbol_type sym,
    const char *text, int len, nk_flags alignment)
{
    return nk_contextual_item_symbol_text(ctx, sym, text, len, alignment);
}
NK_API int
nk_combo_item_symbol_label(struct nk_context *ctx, enum nk_symbol_type sym,
    const char *label, nk_flags alignment)
{
    return nk_contextual_item_symbol_label(ctx, sym, label, alignment);
}
NK_API void nk_combo_end(struct nk_context *ctx)
{
    nk_contextual_end(ctx);
}
NK_API void nk_combo_close(struct nk_context *ctx)
{
    nk_contextual_close(ctx);
}
NK_API int
nk_combo(struct nk_context *ctx, const char **items, int count,
    int selected, int item_height, struct nk_vec2 size)
{
    int i = 0;
    int max_height;
    struct nk_vec2 item_spacing;
    struct nk_vec2 window_padding;

    NK_ASSERT(ctx);
    NK_ASSERT(items);
    NK_ASSERT(ctx->current);
    if (!ctx || !items ||!count)
        return selected;

    item_spacing = ctx->style.window.spacing;
    window_padding = nk_panel_get_padding(&ctx->style, ctx->current->layout->type);
    max_height = count * item_height + count * (int)item_spacing.y;
    max_height += (int)item_spacing.y * 2 + (int)window_padding.y * 2;
    size.y = NK_MIN(size.y, (float)max_height);
    if (nk_combo_begin_label(ctx, items[selected], size)) {
        nk_layout_row_dynamic(ctx, (float)item_height, 1);
        for (i = 0; i < count; ++i) {
            if (nk_combo_item_label(ctx, items[i], NK_TEXT_LEFT))
                selected = i;
        }
        nk_combo_end(ctx);
    }
    return selected;
}
NK_API int
nk_combo_separator(struct nk_context *ctx, const char *items_separated_by_separator,
    int separator, int selected, int count, int item_height, struct nk_vec2 size)
{
    int i;
    int max_height;
    struct nk_vec2 item_spacing;
    struct nk_vec2 window_padding;
    const char *current_item;
    const char *iter;
    int length = 0;

    NK_ASSERT(ctx);
    NK_ASSERT(items_separated_by_separator);
    if (!ctx || !items_separated_by_separator)
        return selected;

    /* calculate popup window */
    item_spacing = ctx->style.window.spacing;
    window_padding = nk_panel_get_padding(&ctx->style, ctx->current->layout->type);
    max_height = count * item_height + count * (int)item_spacing.y;
    max_height += (int)item_spacing.y * 2 + (int)window_padding.y * 2;
    size.y = NK_MIN(size.y, (float)max_height);

    /* find selected item */
    current_item = items_separated_by_separator;
    for (i = 0; i < count; ++i) {
        iter = current_item;
        while (*iter && *iter != separator) iter++;
        length = (int)(iter - current_item);
        if (i == selected) break;
        current_item = iter + 1;
    }

    if (nk_combo_begin_text(ctx, current_item, length, size)) {
        current_item = items_separated_by_separator;
        nk_layout_row_dynamic(ctx, (float)item_height, 1);
        for (i = 0; i < count; ++i) {
            iter = current_item;
            while (*iter && *iter != separator) iter++;
            length = (int)(iter - current_item);
            if (nk_combo_item_text(ctx, current_item, length, NK_TEXT_LEFT))
                selected = i;
            current_item = current_item + length + 1;
        }
        nk_combo_end(ctx);
    }
    return selected;
}
NK_API int
nk_combo_string(struct nk_context *ctx, const char *items_separated_by_zeros,
    int selected, int count, int item_height, struct nk_vec2 size)
{
    return nk_combo_separator(ctx, items_separated_by_zeros, '\0', selected, count, item_height, size);
}
NK_API int
nk_combo_callback(struct nk_context *ctx, void(*item_getter)(void*, int, const char**),
    void *userdata, int selected, int count, int item_height, struct nk_vec2 size)
{
    int i;
    int max_height;
    struct nk_vec2 item_spacing;
    struct nk_vec2 window_padding;
    const char *item;

    NK_ASSERT(ctx);
    NK_ASSERT(item_getter);
    if (!ctx || !item_getter)
        return selected;

    /* calculate popup window */
    item_spacing = ctx->style.window.spacing;
    window_padding = nk_panel_get_padding(&ctx->style, ctx->current->layout->type);
    max_height = count * item_height + count * (int)item_spacing.y;
    max_height += (int)item_spacing.y * 2 + (int)window_padding.y * 2;
    size.y = NK_MIN(size.y, (float)max_height);

    item_getter(userdata, selected, &item);
    if (nk_combo_begin_label(ctx, item, size)) {
        nk_layout_row_dynamic(ctx, (float)item_height, 1);
        for (i = 0; i < count; ++i) {
            item_getter(userdata, i, &item);
            if (nk_combo_item_label(ctx, item, NK_TEXT_LEFT))
                selected = i;
        }
        nk_combo_end(ctx);
    } return selected;
}
NK_API void
nk_combobox(struct nk_context *ctx, const char **items, int count,
    int *selected, int item_height, struct nk_vec2 size)
{
    *selected = nk_combo(ctx, items, count, *selected, item_height, size);
}
NK_API void
nk_combobox_string(struct nk_context *ctx, const char *items_separated_by_zeros,
    int *selected, int count, int item_height, struct nk_vec2 size)
{
    *selected = nk_combo_string(ctx, items_separated_by_zeros, *selected, count, item_height, size);
}
NK_API void
nk_combobox_separator(struct nk_context *ctx, const char *items_separated_by_separator,
    int separator,int *selected, int count, int item_height, struct nk_vec2 size)
{
    *selected = nk_combo_separator(ctx, items_separated_by_separator, separator,
                                    *selected, count, item_height, size);
}
NK_API void
nk_combobox_callback(struct nk_context *ctx,
    void(*item_getter)(void* data, int id, const char **out_text),
    void *userdata, int *selected, int count, int item_height, struct nk_vec2 size)
{
    *selected = nk_combo_callback(ctx, item_getter, userdata,  *selected, count, item_height, size);
}

