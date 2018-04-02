#include "nuklear.h"
#include "nuklear_internal.h"

/* ===============================================================
 *
 *                              MENU
 *
 * ===============================================================*/
NK_API void
nk_menubar_begin(struct nk_context *ctx)
{
    struct nk_panel *layout;
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    layout = ctx->current->layout;
    NK_ASSERT(layout->at_y == layout->bounds.y);
    /* if this assert triggers you allocated space between nk_begin and nk_menubar_begin.
    If you want a menubar the first nuklear function after `nk_begin` has to be a
    `nk_menubar_begin` call. Inside the menubar you then have to allocate space for
    widgets (also supports multiple rows).
    Example:
        if (nk_begin(...)) {
            nk_menubar_begin(...);
                nk_layout_xxxx(...);
                nk_button(...);
                nk_layout_xxxx(...);
                nk_button(...);
            nk_menubar_end(...);
        }
        nk_end(...);
    */
    if (layout->flags & NK_WINDOW_HIDDEN || layout->flags & NK_WINDOW_MINIMIZED)
        return;

    layout->menu.x = layout->at_x;
    layout->menu.y = layout->at_y + layout->row.height;
    layout->menu.w = layout->bounds.w;
    layout->menu.offset.x = *layout->offset_x;
    layout->menu.offset.y = *layout->offset_y;
    *layout->offset_y = 0;
}
NK_API void
nk_menubar_end(struct nk_context *ctx)
{
    struct nk_window *win;
    struct nk_panel *layout;
    struct nk_command_buffer *out;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    out = &win->buffer;
    layout = win->layout;
    if (layout->flags & NK_WINDOW_HIDDEN || layout->flags & NK_WINDOW_MINIMIZED)
        return;

    layout->menu.h = layout->at_y - layout->menu.y;
    layout->bounds.y += layout->menu.h + ctx->style.window.spacing.y + layout->row.height;
    layout->bounds.h -= layout->menu.h + ctx->style.window.spacing.y + layout->row.height;

    *layout->offset_x = layout->menu.offset.x;
    *layout->offset_y = layout->menu.offset.y;
    layout->at_y = layout->bounds.y - layout->row.height;

    layout->clip.y = layout->bounds.y;
    layout->clip.h = layout->bounds.h;
    nk_push_scissor(out, layout->clip);
}
NK_INTERN int
nk_menu_begin(struct nk_context *ctx, struct nk_window *win,
    const char *id, int is_clicked, struct nk_rect header, struct nk_vec2 size)
{
    int is_open = 0;
    int is_active = 0;
    struct nk_rect body;
    struct nk_window *popup;
    nk_hash hash = nk_murmur_hash(id, (int)nk_strlen(id), NK_PANEL_MENU);

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    body.x = header.x;
    body.w = size.x;
    body.y = header.y + header.h;
    body.h = size.y;

    popup = win->popup.win;
    is_open = popup ? nk_true : nk_false;
    is_active = (popup && (win->popup.name == hash) && win->popup.type == NK_PANEL_MENU);
    if ((is_clicked && is_open && !is_active) || (is_open && !is_active) ||
        (!is_open && !is_active && !is_clicked)) return 0;
    if (!nk_nonblock_begin(ctx, NK_WINDOW_NO_SCROLLBAR, body, header, NK_PANEL_MENU))
        return 0;

    win->popup.type = NK_PANEL_MENU;
    win->popup.name = hash;
    return 1;
}
NK_API int
nk_menu_begin_text(struct nk_context *ctx, const char *title, int len,
    nk_flags align, struct nk_vec2 size)
{
    struct nk_window *win;
    const struct nk_input *in;
    struct nk_rect header;
    int is_clicked = nk_false;
    nk_flags state;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    state = nk_widget(&header, ctx);
    if (!state) return 0;
    in = (state == NK_WIDGET_ROM || win->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;
    if (nk_do_button_text(&ctx->last_widget_state, &win->buffer, header,
        title, len, align, NK_BUTTON_DEFAULT, &ctx->style.menu_button, in, ctx->style.font))
        is_clicked = nk_true;
    return nk_menu_begin(ctx, win, title, is_clicked, header, size);
}
NK_API int nk_menu_begin_label(struct nk_context *ctx,
    const char *text, nk_flags align, struct nk_vec2 size)
{
    return nk_menu_begin_text(ctx, text, nk_strlen(text), align, size);
}
NK_API int
nk_menu_begin_image(struct nk_context *ctx, const char *id, struct nk_image img,
    struct nk_vec2 size)
{
    struct nk_window *win;
    struct nk_rect header;
    const struct nk_input *in;
    int is_clicked = nk_false;
    nk_flags state;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    state = nk_widget(&header, ctx);
    if (!state) return 0;
    in = (state == NK_WIDGET_ROM || win->layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;
    if (nk_do_button_image(&ctx->last_widget_state, &win->buffer, header,
        img, NK_BUTTON_DEFAULT, &ctx->style.menu_button, in))
        is_clicked = nk_true;
    return nk_menu_begin(ctx, win, id, is_clicked, header, size);
}
NK_API int
nk_menu_begin_symbol(struct nk_context *ctx, const char *id,
    enum nk_symbol_type sym, struct nk_vec2 size)
{
    struct nk_window *win;
    const struct nk_input *in;
    struct nk_rect header;
    int is_clicked = nk_false;
    nk_flags state;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    state = nk_widget(&header, ctx);
    if (!state) return 0;
    in = (state == NK_WIDGET_ROM || win->layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;
    if (nk_do_button_symbol(&ctx->last_widget_state,  &win->buffer, header,
        sym, NK_BUTTON_DEFAULT, &ctx->style.menu_button, in, ctx->style.font))
        is_clicked = nk_true;
    return nk_menu_begin(ctx, win, id, is_clicked, header, size);
}
NK_API int
nk_menu_begin_image_text(struct nk_context *ctx, const char *title, int len,
    nk_flags align, struct nk_image img, struct nk_vec2 size)
{
    struct nk_window *win;
    struct nk_rect header;
    const struct nk_input *in;
    int is_clicked = nk_false;
    nk_flags state;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    state = nk_widget(&header, ctx);
    if (!state) return 0;
    in = (state == NK_WIDGET_ROM || win->layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;
    if (nk_do_button_text_image(&ctx->last_widget_state, &win->buffer,
        header, img, title, len, align, NK_BUTTON_DEFAULT, &ctx->style.menu_button,
        ctx->style.font, in))
        is_clicked = nk_true;
    return nk_menu_begin(ctx, win, title, is_clicked, header, size);
}
NK_API int
nk_menu_begin_image_label(struct nk_context *ctx,
    const char *title, nk_flags align, struct nk_image img, struct nk_vec2 size)
{
    return nk_menu_begin_image_text(ctx, title, nk_strlen(title), align, img, size);
}
NK_API int
nk_menu_begin_symbol_text(struct nk_context *ctx, const char *title, int len,
    nk_flags align, enum nk_symbol_type sym, struct nk_vec2 size)
{
    struct nk_window *win;
    struct nk_rect header;
    const struct nk_input *in;
    int is_clicked = nk_false;
    nk_flags state;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    state = nk_widget(&header, ctx);
    if (!state) return 0;

    in = (state == NK_WIDGET_ROM || win->layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;
    if (nk_do_button_text_symbol(&ctx->last_widget_state, &win->buffer,
        header, sym, title, len, align, NK_BUTTON_DEFAULT, &ctx->style.menu_button,
        ctx->style.font, in)) is_clicked = nk_true;
    return nk_menu_begin(ctx, win, title, is_clicked, header, size);
}
NK_API int
nk_menu_begin_symbol_label(struct nk_context *ctx,
    const char *title, nk_flags align, enum nk_symbol_type sym, struct nk_vec2 size )
{
    return nk_menu_begin_symbol_text(ctx, title, nk_strlen(title), align,sym,size);
}
NK_API int
nk_menu_item_text(struct nk_context *ctx, const char *title, int len, nk_flags align)
{
    return nk_contextual_item_text(ctx, title, len, align);
}
NK_API int
nk_menu_item_label(struct nk_context *ctx, const char *label, nk_flags align)
{
    return nk_contextual_item_label(ctx, label, align);
}
NK_API int
nk_menu_item_image_label(struct nk_context *ctx, struct nk_image img,
    const char *label, nk_flags align)
{
    return nk_contextual_item_image_label(ctx, img, label, align);
}
NK_API int
nk_menu_item_image_text(struct nk_context *ctx, struct nk_image img,
    const char *text, int len, nk_flags align)
{
    return nk_contextual_item_image_text(ctx, img, text, len, align);
}
NK_API int nk_menu_item_symbol_text(struct nk_context *ctx, enum nk_symbol_type sym,
    const char *text, int len, nk_flags align)
{
    return nk_contextual_item_symbol_text(ctx, sym, text, len, align);
}
NK_API int nk_menu_item_symbol_label(struct nk_context *ctx, enum nk_symbol_type sym,
    const char *label, nk_flags align)
{
    return nk_contextual_item_symbol_label(ctx, sym, label, align);
}
NK_API void nk_menu_close(struct nk_context *ctx)
{
    nk_contextual_close(ctx);
}
NK_API void
nk_menu_end(struct nk_context *ctx)
{
    nk_contextual_end(ctx);
}

