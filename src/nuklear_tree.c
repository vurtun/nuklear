#include "nuklear.h"
#include "nuklear_internal.h"

/* ===============================================================
 *
 *                              TREE
 *
 * ===============================================================*/
NK_INTERN int
nk_tree_state_base(struct nk_context *ctx, enum nk_tree_type type,
    struct nk_image *img, const char *title, enum nk_collapse_states *state)
{
    struct nk_window *win;
    struct nk_panel *layout;
    const struct nk_style *style;
    struct nk_command_buffer *out;
    const struct nk_input *in;
    const struct nk_style_button *button;
    enum nk_symbol_type symbol;
    float row_height;

    struct nk_vec2 item_spacing;
    struct nk_rect header = {0,0,0,0};
    struct nk_rect sym = {0,0,0,0};
    struct nk_text text;

    nk_flags ws = 0;
    enum nk_widget_layout_states widget_state;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    /* cache some data */
    win = ctx->current;
    layout = win->layout;
    out = &win->buffer;
    style = &ctx->style;
    item_spacing = style->window.spacing;

    /* calculate header bounds and draw background */
    row_height = style->font->height + 2 * style->tab.padding.y;
    nk_layout_set_min_row_height(ctx, row_height);
    nk_layout_row_dynamic(ctx, row_height, 1);
    nk_layout_reset_min_row_height(ctx);

    widget_state = nk_widget(&header, ctx);
    if (type == NK_TREE_TAB) {
        const struct nk_style_item *background = &style->tab.background;
        if (background->type == NK_STYLE_ITEM_IMAGE) {
            nk_draw_image(out, header, &background->data.image, nk_white);
            text.background = nk_rgba(0,0,0,0);
        } else {
            text.background = background->data.color;
            nk_fill_rect(out, header, 0, style->tab.border_color);
            nk_fill_rect(out, nk_shrink_rect(header, style->tab.border),
                style->tab.rounding, background->data.color);
        }
    } else text.background = style->window.background;

    /* update node state */
    in = (!(layout->flags & NK_WINDOW_ROM)) ? &ctx->input: 0;
    in = (in && widget_state == NK_WIDGET_VALID) ? &ctx->input : 0;
    if (nk_button_behavior(&ws, header, in, NK_BUTTON_DEFAULT))
        *state = (*state == NK_MAXIMIZED) ? NK_MINIMIZED : NK_MAXIMIZED;

    /* select correct button style */
    if (*state == NK_MAXIMIZED) {
        symbol = style->tab.sym_maximize;
        if (type == NK_TREE_TAB)
            button = &style->tab.tab_maximize_button;
        else button = &style->tab.node_maximize_button;
    } else {
        symbol = style->tab.sym_minimize;
        if (type == NK_TREE_TAB)
            button = &style->tab.tab_minimize_button;
        else button = &style->tab.node_minimize_button;
    }

    {/* draw triangle button */
    sym.w = sym.h = style->font->height;
    sym.y = header.y + style->tab.padding.y;
    sym.x = header.x + style->tab.padding.x;
    nk_do_button_symbol(&ws, &win->buffer, sym, symbol, NK_BUTTON_DEFAULT,
        button, 0, style->font);

    if (img) {
        /* draw optional image icon */
        sym.x = sym.x + sym.w + 4 * item_spacing.x;
        nk_draw_image(&win->buffer, sym, img, nk_white);
        sym.w = style->font->height + style->tab.spacing.x;}
    }

    {/* draw label */
    struct nk_rect label;
    header.w = NK_MAX(header.w, sym.w + item_spacing.x);
    label.x = sym.x + sym.w + item_spacing.x;
    label.y = sym.y;
    label.w = header.w - (sym.w + item_spacing.y + style->tab.indent);
    label.h = style->font->height;
    text.text = style->tab.text;
    text.padding = nk_vec2(0,0);
    nk_widget_text(out, label, title, nk_strlen(title), &text,
        NK_TEXT_LEFT, style->font);}

    /* increase x-axis cursor widget position pointer */
    if (*state == NK_MAXIMIZED) {
        layout->at_x = header.x + (float)*layout->offset_x + style->tab.indent;
        layout->bounds.w = NK_MAX(layout->bounds.w, style->tab.indent);
        layout->bounds.w -= (style->tab.indent + style->window.padding.x);
        layout->row.tree_depth++;
        return nk_true;
    } else return nk_false;
}
NK_INTERN int
nk_tree_base(struct nk_context *ctx, enum nk_tree_type type,
    struct nk_image *img, const char *title, enum nk_collapse_states initial_state,
    const char *hash, int len, int line)
{
    struct nk_window *win = ctx->current;
    int title_len = 0;
    nk_hash tree_hash = 0;
    nk_uint *state = 0;

    /* retrieve tree state from internal widget state tables */
    if (!hash) {
        title_len = (int)nk_strlen(title);
        tree_hash = nk_murmur_hash(title, (int)title_len, (nk_hash)line);
    } else tree_hash = nk_murmur_hash(hash, len, (nk_hash)line);
    state = nk_find_value(win, tree_hash);
    if (!state) {
        state = nk_add_value(ctx, win, tree_hash, 0);
        *state = initial_state;
    }
    return nk_tree_state_base(ctx, type, img, title, (enum nk_collapse_states*)state);
}
NK_API int
nk_tree_state_push(struct nk_context *ctx, enum nk_tree_type type,
    const char *title, enum nk_collapse_states *state)
{
    return nk_tree_state_base(ctx, type, 0, title, state);
}
NK_API int
nk_tree_state_image_push(struct nk_context *ctx, enum nk_tree_type type,
    struct nk_image img, const char *title, enum nk_collapse_states *state)
{
    return nk_tree_state_base(ctx, type, &img, title, state);
}
NK_API void
nk_tree_state_pop(struct nk_context *ctx)
{
    struct nk_window *win = 0;
    struct nk_panel *layout = 0;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    layout->at_x -= ctx->style.tab.indent + ctx->style.window.padding.x;
    layout->bounds.w += ctx->style.tab.indent + ctx->style.window.padding.x;
    NK_ASSERT(layout->row.tree_depth);
    layout->row.tree_depth--;
}
NK_API int
nk_tree_push_hashed(struct nk_context *ctx, enum nk_tree_type type,
    const char *title, enum nk_collapse_states initial_state,
    const char *hash, int len, int line)
{
    return nk_tree_base(ctx, type, 0, title, initial_state, hash, len, line);
}
NK_API int
nk_tree_image_push_hashed(struct nk_context *ctx, enum nk_tree_type type,
    struct nk_image img, const char *title, enum nk_collapse_states initial_state,
    const char *hash, int len,int seed)
{
    return nk_tree_base(ctx, type, &img, title, initial_state, hash, len, seed);
}
NK_API void
nk_tree_pop(struct nk_context *ctx)
{
    nk_tree_state_pop(ctx);
}
NK_INTERN int
nk_tree_element_image_push_hashed_base(struct nk_context *ctx, enum nk_tree_type type,
    struct nk_image *img, const char *title, int title_len,
    enum nk_collapse_states *state, int *selected)
{
    struct nk_window *win;
    struct nk_panel *layout;
    const struct nk_style *style;
    struct nk_command_buffer *out;
    const struct nk_input *in;
    const struct nk_style_button *button;
    enum nk_symbol_type symbol;
    float row_height;
    struct nk_vec2 padding;

    int text_len;
    float text_width;

    struct nk_vec2 item_spacing;
    struct nk_rect header = {0,0,0,0};
    struct nk_rect sym = {0,0,0,0};
    struct nk_text text;

    nk_flags ws = 0;
    enum nk_widget_layout_states widget_state;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    /* cache some data */
    win = ctx->current;
    layout = win->layout;
    out = &win->buffer;
    style = &ctx->style;
    item_spacing = style->window.spacing;
    padding = style->selectable.padding;

    /* calculate header bounds and draw background */
    row_height = style->font->height + 2 * style->tab.padding.y;
    nk_layout_set_min_row_height(ctx, row_height);
    nk_layout_row_dynamic(ctx, row_height, 1);
    nk_layout_reset_min_row_height(ctx);

    widget_state = nk_widget(&header, ctx);
    if (type == NK_TREE_TAB) {
        const struct nk_style_item *background = &style->tab.background;
        if (background->type == NK_STYLE_ITEM_IMAGE) {
            nk_draw_image(out, header, &background->data.image, nk_white);
            text.background = nk_rgba(0,0,0,0);
        } else {
            text.background = background->data.color;
            nk_fill_rect(out, header, 0, style->tab.border_color);
            nk_fill_rect(out, nk_shrink_rect(header, style->tab.border),
                style->tab.rounding, background->data.color);
        }
    } else text.background = style->window.background;

    in = (!(layout->flags & NK_WINDOW_ROM)) ? &ctx->input: 0;
    in = (in && widget_state == NK_WIDGET_VALID) ? &ctx->input : 0;

    /* select correct button style */
    if (*state == NK_MAXIMIZED) {
        symbol = style->tab.sym_maximize;
        if (type == NK_TREE_TAB)
            button = &style->tab.tab_maximize_button;
        else button = &style->tab.node_maximize_button;
    } else {
        symbol = style->tab.sym_minimize;
        if (type == NK_TREE_TAB)
            button = &style->tab.tab_minimize_button;
        else button = &style->tab.node_minimize_button;
    }
    {/* draw triangle button */
    sym.w = sym.h = style->font->height;
    sym.y = header.y + style->tab.padding.y;
    sym.x = header.x + style->tab.padding.x;
    if (nk_do_button_symbol(&ws, &win->buffer, sym, symbol, NK_BUTTON_DEFAULT, button, in, style->font))
        *state = (*state == NK_MAXIMIZED) ? NK_MINIMIZED : NK_MAXIMIZED;}

    /* draw label */
    {nk_flags dummy = 0;
    struct nk_rect label;
    /* calculate size of the text and tooltip */
    text_len = nk_strlen(title);
    text_width = style->font->width(style->font->userdata, style->font->height, title, text_len);
    text_width += (4 * padding.x);

    header.w = NK_MAX(header.w, sym.w + item_spacing.x);
    label.x = sym.x + sym.w + item_spacing.x;
    label.y = sym.y;
    label.w = NK_MIN(header.w - (sym.w + item_spacing.y + style->tab.indent), text_width);
    label.h = style->font->height;

    if (img) {
        nk_do_selectable_image(&dummy, &win->buffer, label, title, title_len, NK_TEXT_LEFT,
            selected, img, &style->selectable, in, style->font);
    } else nk_do_selectable(&dummy, &win->buffer, label, title, title_len, NK_TEXT_LEFT,
            selected, &style->selectable, in, style->font);
    }
    /* increase x-axis cursor widget position pointer */
    if (*state == NK_MAXIMIZED) {
        layout->at_x = header.x + (float)*layout->offset_x + style->tab.indent;
        layout->bounds.w = NK_MAX(layout->bounds.w, style->tab.indent);
        layout->bounds.w -= (style->tab.indent + style->window.padding.x);
        layout->row.tree_depth++;
        return nk_true;
    } else return nk_false;
}
NK_INTERN int
nk_tree_element_base(struct nk_context *ctx, enum nk_tree_type type,
    struct nk_image *img, const char *title, enum nk_collapse_states initial_state,
    int *selected, const char *hash, int len, int line)
{
    struct nk_window *win = ctx->current;
    int title_len = 0;
    nk_hash tree_hash = 0;
    nk_uint *state = 0;

    /* retrieve tree state from internal widget state tables */
    if (!hash) {
        title_len = (int)nk_strlen(title);
        tree_hash = nk_murmur_hash(title, (int)title_len, (nk_hash)line);
    } else tree_hash = nk_murmur_hash(hash, len, (nk_hash)line);
    state = nk_find_value(win, tree_hash);
    if (!state) {
        state = nk_add_value(ctx, win, tree_hash, 0);
        *state = initial_state;
    } return nk_tree_element_image_push_hashed_base(ctx, type, img, title,
        nk_strlen(title), (enum nk_collapse_states*)state, selected);
}
NK_API int
nk_tree_element_push_hashed(struct nk_context *ctx, enum nk_tree_type type,
    const char *title, enum nk_collapse_states initial_state,
    int *selected, const char *hash, int len, int seed)
{
    return nk_tree_element_base(ctx, type, 0, title, initial_state, selected, hash, len, seed);
}
NK_API int
nk_tree_element_image_push_hashed(struct nk_context *ctx, enum nk_tree_type type,
    struct nk_image img, const char *title, enum nk_collapse_states initial_state,
    int *selected, const char *hash, int len,int seed)
{
    return nk_tree_element_base(ctx, type, &img, title, initial_state, selected, hash, len, seed);
}
NK_API void
nk_tree_element_pop(struct nk_context *ctx)
{
    nk_tree_state_pop(ctx);
}

