#include "nuklear.h"
#include "nuklear_internal.h"

/* ===============================================================
 *
 *                          LIST VIEW
 *
 * ===============================================================*/
NK_API int
nk_list_view_begin(struct nk_context *ctx, struct nk_list_view *view,
    const char *title, nk_flags flags, int row_height, int row_count)
{
    int title_len;
    nk_hash title_hash;
    nk_uint *x_offset;
    nk_uint *y_offset;

    int result;
    struct nk_window *win;
    struct nk_panel *layout;
    const struct nk_style *style;
    struct nk_vec2 item_spacing;

    NK_ASSERT(ctx);
    NK_ASSERT(view);
    NK_ASSERT(title);
    if (!ctx || !view || !title) return 0;

    win = ctx->current;
    style = &ctx->style;
    item_spacing = style->window.spacing;
    row_height += NK_MAX(0, (int)item_spacing.y);

    /* find persistent list view scrollbar offset */
    title_len = (int)nk_strlen(title);
    title_hash = nk_murmur_hash(title, (int)title_len, NK_PANEL_GROUP);
    x_offset = nk_find_value(win, title_hash);
    if (!x_offset) {
        x_offset = nk_add_value(ctx, win, title_hash, 0);
        y_offset = nk_add_value(ctx, win, title_hash+1, 0);

        NK_ASSERT(x_offset);
        NK_ASSERT(y_offset);
        if (!x_offset || !y_offset) return 0;
        *x_offset = *y_offset = 0;
    } else y_offset = nk_find_value(win, title_hash+1);
    view->scroll_value = *y_offset;
    view->scroll_pointer = y_offset;

    *y_offset = 0;
    result = nk_group_scrolled_offset_begin(ctx, x_offset, y_offset, title, flags);
    win = ctx->current;
    layout = win->layout;

    view->total_height = row_height * NK_MAX(row_count,1);
    view->begin = (int)NK_MAX(((float)view->scroll_value / (float)row_height), 0.0f);
    view->count = (int)NK_MAX(nk_iceilf((layout->clip.h)/(float)row_height),0);
    view->count = NK_MIN(view->count, row_count - view->begin);
    view->end = view->begin + view->count;
    view->ctx = ctx;
    return result;
}
NK_API void
nk_list_view_end(struct nk_list_view *view)
{
    struct nk_context *ctx;
    struct nk_window *win;
    struct nk_panel *layout;

    NK_ASSERT(view);
    NK_ASSERT(view->ctx);
    NK_ASSERT(view->scroll_pointer);
    if (!view || !view->ctx) return;

    ctx = view->ctx;
    win = ctx->current;
    layout = win->layout;
    layout->at_y = layout->bounds.y + (float)view->total_height;
    *view->scroll_pointer = *view->scroll_pointer + view->scroll_value;
    nk_group_end(view->ctx);
}

