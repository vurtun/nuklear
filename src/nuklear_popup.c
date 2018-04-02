#include "nuklear.h"
#include "nuklear_internal.h"

/* ===============================================================
 *
 *                              POPUP
 *
 * ===============================================================*/
NK_API int
nk_popup_begin(struct nk_context *ctx, enum nk_popup_type type,
    const char *title, nk_flags flags, struct nk_rect rect)
{
    struct nk_window *popup;
    struct nk_window *win;
    struct nk_panel *panel;

    int title_len;
    nk_hash title_hash;
    nk_size allocated;

    NK_ASSERT(ctx);
    NK_ASSERT(title);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    panel = win->layout;
    NK_ASSERT(!(panel->type & NK_PANEL_SET_POPUP) && "popups are not allowed to have popups");
    (void)panel;
    title_len = (int)nk_strlen(title);
    title_hash = nk_murmur_hash(title, (int)title_len, NK_PANEL_POPUP);

    popup = win->popup.win;
    if (!popup) {
        popup = (struct nk_window*)nk_create_window(ctx);
        popup->parent = win;
        win->popup.win = popup;
        win->popup.active = 0;
        win->popup.type = NK_PANEL_POPUP;
    }

    /* make sure we have correct popup */
    if (win->popup.name != title_hash) {
        if (!win->popup.active) {
            nk_zero(popup, sizeof(*popup));
            win->popup.name = title_hash;
            win->popup.active = 1;
            win->popup.type = NK_PANEL_POPUP;
        } else return 0;
    }

    /* popup position is local to window */
    ctx->current = popup;
    rect.x += win->layout->clip.x;
    rect.y += win->layout->clip.y;

    /* setup popup data */
    popup->parent = win;
    popup->bounds = rect;
    popup->seq = ctx->seq;
    popup->layout = (struct nk_panel*)nk_create_panel(ctx);
    popup->flags = flags;
    popup->flags |= NK_WINDOW_BORDER;
    if (type == NK_POPUP_DYNAMIC)
        popup->flags |= NK_WINDOW_DYNAMIC;

    popup->buffer = win->buffer;
    nk_start_popup(ctx, win);
    allocated = ctx->memory.allocated;
    nk_push_scissor(&popup->buffer, nk_null_rect);

    if (nk_panel_begin(ctx, title, NK_PANEL_POPUP)) {
        /* popup is running therefore invalidate parent panels */
        struct nk_panel *root;
        root = win->layout;
        while (root) {
            root->flags |= NK_WINDOW_ROM;
            root->flags &= ~(nk_flags)NK_WINDOW_REMOVE_ROM;
            root = root->parent;
        }
        win->popup.active = 1;
        popup->layout->offset_x = &popup->scrollbar.x;
        popup->layout->offset_y = &popup->scrollbar.y;
        popup->layout->parent = win->layout;
        return 1;
    } else {
        /* popup was closed/is invalid so cleanup */
        struct nk_panel *root;
        root = win->layout;
        while (root) {
            root->flags |= NK_WINDOW_REMOVE_ROM;
            root = root->parent;
        }
        win->popup.buf.active = 0;
        win->popup.active = 0;
        ctx->memory.allocated = allocated;
        ctx->current = win;
        nk_free_panel(ctx, popup->layout);
        popup->layout = 0;
        return 0;
    }
}
NK_LIB int
nk_nonblock_begin(struct nk_context *ctx,
    nk_flags flags, struct nk_rect body, struct nk_rect header,
    enum nk_panel_type panel_type)
{
    struct nk_window *popup;
    struct nk_window *win;
    struct nk_panel *panel;
    int is_active = nk_true;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    /* popups cannot have popups */
    win = ctx->current;
    panel = win->layout;
    NK_ASSERT(!(panel->type & NK_PANEL_SET_POPUP));
    (void)panel;
    popup = win->popup.win;
    if (!popup) {
        /* create window for nonblocking popup */
        popup = (struct nk_window*)nk_create_window(ctx);
        popup->parent = win;
        win->popup.win = popup;
        win->popup.type = panel_type;
        nk_command_buffer_init(&popup->buffer, &ctx->memory, NK_CLIPPING_ON);
    } else {
        /* close the popup if user pressed outside or in the header */
        int pressed, in_body, in_header;
        pressed = nk_input_is_mouse_pressed(&ctx->input, NK_BUTTON_LEFT);
        in_body = nk_input_is_mouse_hovering_rect(&ctx->input, body);
        in_header = nk_input_is_mouse_hovering_rect(&ctx->input, header);
        if (pressed && (!in_body || in_header))
            is_active = nk_false;
    }
    win->popup.header = header;

    if (!is_active) {
        /* remove read only mode from all parent panels */
        struct nk_panel *root = win->layout;
        while (root) {
            root->flags |= NK_WINDOW_REMOVE_ROM;
            root = root->parent;
        }
        return is_active;
    }
    popup->bounds = body;
    popup->parent = win;
    popup->layout = (struct nk_panel*)nk_create_panel(ctx);
    popup->flags = flags;
    popup->flags |= NK_WINDOW_BORDER;
    popup->flags |= NK_WINDOW_DYNAMIC;
    popup->seq = ctx->seq;
    win->popup.active = 1;
    NK_ASSERT(popup->layout);

    nk_start_popup(ctx, win);
    popup->buffer = win->buffer;
    nk_push_scissor(&popup->buffer, nk_null_rect);
    ctx->current = popup;

    nk_panel_begin(ctx, 0, panel_type);
    win->buffer = popup->buffer;
    popup->layout->parent = win->layout;
    popup->layout->offset_x = &popup->scrollbar.x;
    popup->layout->offset_y = &popup->scrollbar.y;

    /* set read only mode to all parent panels */
    {struct nk_panel *root;
    root = win->layout;
    while (root) {
        root->flags |= NK_WINDOW_ROM;
        root = root->parent;
    }}
    return is_active;
}
NK_API void
nk_popup_close(struct nk_context *ctx)
{
    struct nk_window *popup;
    NK_ASSERT(ctx);
    if (!ctx || !ctx->current) return;

    popup = ctx->current;
    NK_ASSERT(popup->parent);
    NK_ASSERT(popup->layout->type & NK_PANEL_SET_POPUP);
    popup->flags |= NK_WINDOW_HIDDEN;
}
NK_API void
nk_popup_end(struct nk_context *ctx)
{
    struct nk_window *win;
    struct nk_window *popup;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    popup = ctx->current;
    if (!popup->parent) return;
    win = popup->parent;
    if (popup->flags & NK_WINDOW_HIDDEN) {
        struct nk_panel *root;
        root = win->layout;
        while (root) {
            root->flags |= NK_WINDOW_REMOVE_ROM;
            root = root->parent;
        }
        win->popup.active = 0;
    }
    nk_push_scissor(&popup->buffer, nk_null_rect);
    nk_end(ctx);

    win->buffer = popup->buffer;
    nk_finish_popup(ctx, win);
    ctx->current = win;
    nk_push_scissor(&win->buffer, win->layout->clip);
}

