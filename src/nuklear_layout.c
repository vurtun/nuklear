#include "nuklear.h"
#include "nuklear_internal.h"

/* ===============================================================
 *
 *                          LAYOUT
 *
 * ===============================================================*/
NK_API void
nk_layout_set_min_row_height(struct nk_context *ctx, float height)
{
    struct nk_window *win;
    struct nk_panel *layout;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    layout->row.min_height = height;
}
NK_API void
nk_layout_reset_min_row_height(struct nk_context *ctx)
{
    struct nk_window *win;
    struct nk_panel *layout;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    layout->row.min_height = ctx->style.font->height;
    layout->row.min_height += ctx->style.text.padding.y*2;
    layout->row.min_height += ctx->style.window.min_row_height_padding*2;
}
NK_LIB float
nk_layout_row_calculate_usable_space(const struct nk_style *style, enum nk_panel_type type,
    float total_space, int columns)
{
    float panel_padding;
    float panel_spacing;
    float panel_space;

    struct nk_vec2 spacing;
    struct nk_vec2 padding;

    spacing = style->window.spacing;
    padding = nk_panel_get_padding(style, type);

    /* calculate the usable panel space */
    panel_padding = 2 * padding.x;
    panel_spacing = (float)NK_MAX(columns - 1, 0) * spacing.x;
    panel_space  = total_space - panel_padding - panel_spacing;
    return panel_space;
}
NK_LIB void
nk_panel_layout(const struct nk_context *ctx, struct nk_window *win,
    float height, int cols)
{
    struct nk_panel *layout;
    const struct nk_style *style;
    struct nk_command_buffer *out;

    struct nk_vec2 item_spacing;
    struct nk_color color;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    /* prefetch some configuration data */
    layout = win->layout;
    style = &ctx->style;
    out = &win->buffer;
    color = style->window.background;
    item_spacing = style->window.spacing;

    /*  if one of these triggers you forgot to add an `if` condition around either
        a window, group, popup, combobox or contextual menu `begin` and `end` block.
        Example:
            if (nk_begin(...) {...} nk_end(...); or
            if (nk_group_begin(...) { nk_group_end(...);} */
    NK_ASSERT(!(layout->flags & NK_WINDOW_MINIMIZED));
    NK_ASSERT(!(layout->flags & NK_WINDOW_HIDDEN));
    NK_ASSERT(!(layout->flags & NK_WINDOW_CLOSED));

    /* update the current row and set the current row layout */
    layout->row.index = 0;
    layout->at_y += layout->row.height;
    layout->row.columns = cols;
    if (height == 0.0f)
        layout->row.height = NK_MAX(height, layout->row.min_height) + item_spacing.y;
    else layout->row.height = height + item_spacing.y;

    layout->row.item_offset = 0;
    if (layout->flags & NK_WINDOW_DYNAMIC) {
        /* draw background for dynamic panels */
        struct nk_rect background;
        background.x = win->bounds.x;
        background.w = win->bounds.w;
        background.y = layout->at_y - 1.0f;
        background.h = layout->row.height + 1.0f;
        nk_fill_rect(out, background, 0, color);
    }
}
NK_LIB void
nk_row_layout(struct nk_context *ctx, enum nk_layout_format fmt,
    float height, int cols, int width)
{
    /* update the current row and set the current row layout */
    struct nk_window *win;
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    nk_panel_layout(ctx, win, height, cols);
    if (fmt == NK_DYNAMIC)
        win->layout->row.type = NK_LAYOUT_DYNAMIC_FIXED;
    else win->layout->row.type = NK_LAYOUT_STATIC_FIXED;

    win->layout->row.ratio = 0;
    win->layout->row.filled = 0;
    win->layout->row.item_offset = 0;
    win->layout->row.item_width = (float)width;
}
NK_API float
nk_layout_ratio_from_pixel(struct nk_context *ctx, float pixel_width)
{
    struct nk_window *win;
    NK_ASSERT(ctx);
    NK_ASSERT(pixel_width);
    if (!ctx || !ctx->current || !ctx->current->layout) return 0;
    win = ctx->current;
    return NK_CLAMP(0.0f, pixel_width/win->bounds.x, 1.0f);
}
NK_API void
nk_layout_row_dynamic(struct nk_context *ctx, float height, int cols)
{
    nk_row_layout(ctx, NK_DYNAMIC, height, cols, 0);
}
NK_API void
nk_layout_row_static(struct nk_context *ctx, float height, int item_width, int cols)
{
    nk_row_layout(ctx, NK_STATIC, height, cols, item_width);
}
NK_API void
nk_layout_row_begin(struct nk_context *ctx, enum nk_layout_format fmt,
    float row_height, int cols)
{
    struct nk_window *win;
    struct nk_panel *layout;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    nk_panel_layout(ctx, win, row_height, cols);
    if (fmt == NK_DYNAMIC)
        layout->row.type = NK_LAYOUT_DYNAMIC_ROW;
    else layout->row.type = NK_LAYOUT_STATIC_ROW;

    layout->row.ratio = 0;
    layout->row.filled = 0;
    layout->row.item_width = 0;
    layout->row.item_offset = 0;
    layout->row.columns = cols;
}
NK_API void
nk_layout_row_push(struct nk_context *ctx, float ratio_or_width)
{
    struct nk_window *win;
    struct nk_panel *layout;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    NK_ASSERT(layout->row.type == NK_LAYOUT_STATIC_ROW || layout->row.type == NK_LAYOUT_DYNAMIC_ROW);
    if (layout->row.type != NK_LAYOUT_STATIC_ROW && layout->row.type != NK_LAYOUT_DYNAMIC_ROW)
        return;

    if (layout->row.type == NK_LAYOUT_DYNAMIC_ROW) {
        float ratio = ratio_or_width;
        if ((ratio + layout->row.filled) > 1.0f) return;
        if (ratio > 0.0f)
            layout->row.item_width = NK_SATURATE(ratio);
        else layout->row.item_width = 1.0f - layout->row.filled;
    } else layout->row.item_width = ratio_or_width;
}
NK_API void
nk_layout_row_end(struct nk_context *ctx)
{
    struct nk_window *win;
    struct nk_panel *layout;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    NK_ASSERT(layout->row.type == NK_LAYOUT_STATIC_ROW || layout->row.type == NK_LAYOUT_DYNAMIC_ROW);
    if (layout->row.type != NK_LAYOUT_STATIC_ROW && layout->row.type != NK_LAYOUT_DYNAMIC_ROW)
        return;
    layout->row.item_width = 0;
    layout->row.item_offset = 0;
}
NK_API void
nk_layout_row(struct nk_context *ctx, enum nk_layout_format fmt,
    float height, int cols, const float *ratio)
{
    int i;
    int n_undef = 0;
    struct nk_window *win;
    struct nk_panel *layout;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    nk_panel_layout(ctx, win, height, cols);
    if (fmt == NK_DYNAMIC) {
        /* calculate width of undefined widget ratios */
        float r = 0;
        layout->row.ratio = ratio;
        for (i = 0; i < cols; ++i) {
            if (ratio[i] < 0.0f)
                n_undef++;
            else r += ratio[i];
        }
        r = NK_SATURATE(1.0f - r);
        layout->row.type = NK_LAYOUT_DYNAMIC;
        layout->row.item_width = (r > 0 && n_undef > 0) ? (r / (float)n_undef):0;
    } else {
        layout->row.ratio = ratio;
        layout->row.type = NK_LAYOUT_STATIC;
        layout->row.item_width = 0;
        layout->row.item_offset = 0;
    }
    layout->row.item_offset = 0;
    layout->row.filled = 0;
}
NK_API void
nk_layout_row_template_begin(struct nk_context *ctx, float height)
{
    struct nk_window *win;
    struct nk_panel *layout;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    nk_panel_layout(ctx, win, height, 1);
    layout->row.type = NK_LAYOUT_TEMPLATE;
    layout->row.columns = 0;
    layout->row.ratio = 0;
    layout->row.item_width = 0;
    layout->row.item_height = 0;
    layout->row.item_offset = 0;
    layout->row.filled = 0;
    layout->row.item.x = 0;
    layout->row.item.y = 0;
    layout->row.item.w = 0;
    layout->row.item.h = 0;
}
NK_API void
nk_layout_row_template_push_dynamic(struct nk_context *ctx)
{
    struct nk_window *win;
    struct nk_panel *layout;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    NK_ASSERT(layout->row.type == NK_LAYOUT_TEMPLATE);
    NK_ASSERT(layout->row.columns < NK_MAX_LAYOUT_ROW_TEMPLATE_COLUMNS);
    if (layout->row.type != NK_LAYOUT_TEMPLATE) return;
    if (layout->row.columns >= NK_MAX_LAYOUT_ROW_TEMPLATE_COLUMNS) return;
    layout->row.templates[layout->row.columns++] = -1.0f;
}
NK_API void
nk_layout_row_template_push_variable(struct nk_context *ctx, float min_width)
{
    struct nk_window *win;
    struct nk_panel *layout;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    NK_ASSERT(layout->row.type == NK_LAYOUT_TEMPLATE);
    NK_ASSERT(layout->row.columns < NK_MAX_LAYOUT_ROW_TEMPLATE_COLUMNS);
    if (layout->row.type != NK_LAYOUT_TEMPLATE) return;
    if (layout->row.columns >= NK_MAX_LAYOUT_ROW_TEMPLATE_COLUMNS) return;
    layout->row.templates[layout->row.columns++] = -min_width;
}
NK_API void
nk_layout_row_template_push_static(struct nk_context *ctx, float width)
{
    struct nk_window *win;
    struct nk_panel *layout;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    NK_ASSERT(layout->row.type == NK_LAYOUT_TEMPLATE);
    NK_ASSERT(layout->row.columns < NK_MAX_LAYOUT_ROW_TEMPLATE_COLUMNS);
    if (layout->row.type != NK_LAYOUT_TEMPLATE) return;
    if (layout->row.columns >= NK_MAX_LAYOUT_ROW_TEMPLATE_COLUMNS) return;
    layout->row.templates[layout->row.columns++] = width;
}
NK_API void
nk_layout_row_template_end(struct nk_context *ctx)
{
    struct nk_window *win;
    struct nk_panel *layout;

    int i = 0;
    int variable_count = 0;
    int min_variable_count = 0;
    float min_fixed_width = 0.0f;
    float total_fixed_width = 0.0f;
    float max_variable_width = 0.0f;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    NK_ASSERT(layout->row.type == NK_LAYOUT_TEMPLATE);
    if (layout->row.type != NK_LAYOUT_TEMPLATE) return;
    for (i = 0; i < layout->row.columns; ++i) {
        float width = layout->row.templates[i];
        if (width >= 0.0f) {
            total_fixed_width += width;
            min_fixed_width += width;
        } else if (width < -1.0f) {
            width = -width;
            total_fixed_width += width;
            max_variable_width = NK_MAX(max_variable_width, width);
            variable_count++;
        } else {
            min_variable_count++;
            variable_count++;
        }
    }
    if (variable_count) {
        float space = nk_layout_row_calculate_usable_space(&ctx->style, layout->type,
                            layout->bounds.w, layout->row.columns);
        float var_width = (NK_MAX(space-min_fixed_width,0.0f)) / (float)variable_count;
        int enough_space = var_width >= max_variable_width;
        if (!enough_space)
            var_width = (NK_MAX(space-total_fixed_width,0)) / (float)min_variable_count;
        for (i = 0; i < layout->row.columns; ++i) {
            float *width = &layout->row.templates[i];
            *width = (*width >= 0.0f)? *width: (*width < -1.0f && !enough_space)? -(*width): var_width;
        }
    }
}
NK_API void
nk_layout_space_begin(struct nk_context *ctx, enum nk_layout_format fmt,
    float height, int widget_count)
{
    struct nk_window *win;
    struct nk_panel *layout;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    nk_panel_layout(ctx, win, height, widget_count);
    if (fmt == NK_STATIC)
        layout->row.type = NK_LAYOUT_STATIC_FREE;
    else layout->row.type = NK_LAYOUT_DYNAMIC_FREE;

    layout->row.ratio = 0;
    layout->row.filled = 0;
    layout->row.item_width = 0;
    layout->row.item_offset = 0;
}
NK_API void
nk_layout_space_end(struct nk_context *ctx)
{
    struct nk_window *win;
    struct nk_panel *layout;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    layout->row.item_width = 0;
    layout->row.item_height = 0;
    layout->row.item_offset = 0;
    nk_zero(&layout->row.item, sizeof(layout->row.item));
}
NK_API void
nk_layout_space_push(struct nk_context *ctx, struct nk_rect rect)
{
    struct nk_window *win;
    struct nk_panel *layout;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    layout->row.item = rect;
}
NK_API struct nk_rect
nk_layout_space_bounds(struct nk_context *ctx)
{
    struct nk_rect ret;
    struct nk_window *win;
    struct nk_panel *layout;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    win = ctx->current;
    layout = win->layout;

    ret.x = layout->clip.x;
    ret.y = layout->clip.y;
    ret.w = layout->clip.w;
    ret.h = layout->row.height;
    return ret;
}
NK_API struct nk_rect
nk_layout_widget_bounds(struct nk_context *ctx)
{
    struct nk_rect ret;
    struct nk_window *win;
    struct nk_panel *layout;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    win = ctx->current;
    layout = win->layout;

    ret.x = layout->at_x;
    ret.y = layout->at_y;
    ret.w = layout->bounds.w - NK_MAX(layout->at_x - layout->bounds.x,0);
    ret.h = layout->row.height;
    return ret;
}
NK_API struct nk_vec2
nk_layout_space_to_screen(struct nk_context *ctx, struct nk_vec2 ret)
{
    struct nk_window *win;
    struct nk_panel *layout;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    win = ctx->current;
    layout = win->layout;

    ret.x += layout->at_x - (float)*layout->offset_x;
    ret.y += layout->at_y - (float)*layout->offset_y;
    return ret;
}
NK_API struct nk_vec2
nk_layout_space_to_local(struct nk_context *ctx, struct nk_vec2 ret)
{
    struct nk_window *win;
    struct nk_panel *layout;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    win = ctx->current;
    layout = win->layout;

    ret.x += -layout->at_x + (float)*layout->offset_x;
    ret.y += -layout->at_y + (float)*layout->offset_y;
    return ret;
}
NK_API struct nk_rect
nk_layout_space_rect_to_screen(struct nk_context *ctx, struct nk_rect ret)
{
    struct nk_window *win;
    struct nk_panel *layout;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    win = ctx->current;
    layout = win->layout;

    ret.x += layout->at_x - (float)*layout->offset_x;
    ret.y += layout->at_y - (float)*layout->offset_y;
    return ret;
}
NK_API struct nk_rect
nk_layout_space_rect_to_local(struct nk_context *ctx, struct nk_rect ret)
{
    struct nk_window *win;
    struct nk_panel *layout;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    win = ctx->current;
    layout = win->layout;

    ret.x += -layout->at_x + (float)*layout->offset_x;
    ret.y += -layout->at_y + (float)*layout->offset_y;
    return ret;
}
NK_LIB void
nk_panel_alloc_row(const struct nk_context *ctx, struct nk_window *win)
{
    struct nk_panel *layout = win->layout;
    struct nk_vec2 spacing = ctx->style.window.spacing;
    const float row_height = layout->row.height - spacing.y;
    nk_panel_layout(ctx, win, row_height, layout->row.columns);
}
NK_LIB void
nk_layout_widget_space(struct nk_rect *bounds, const struct nk_context *ctx,
    struct nk_window *win, int modify)
{
    struct nk_panel *layout;
    const struct nk_style *style;

    struct nk_vec2 spacing;
    struct nk_vec2 padding;

    float item_offset = 0;
    float item_width = 0;
    float item_spacing = 0;
    float panel_space = 0;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    style = &ctx->style;
    NK_ASSERT(bounds);

    spacing = style->window.spacing;
    padding = nk_panel_get_padding(style, layout->type);
    panel_space = nk_layout_row_calculate_usable_space(&ctx->style, layout->type,
                                            layout->bounds.w, layout->row.columns);

    /* calculate the width of one item inside the current layout space */
    switch (layout->row.type) {
    case NK_LAYOUT_DYNAMIC_FIXED: {
        /* scaling fixed size widgets item width */
        item_width = NK_MAX(1.0f,panel_space) / (float)layout->row.columns;
        item_offset = (float)layout->row.index * item_width;
        item_spacing = (float)layout->row.index * spacing.x;
    } break;
    case NK_LAYOUT_DYNAMIC_ROW: {
        /* scaling single ratio widget width */
        item_width = layout->row.item_width * panel_space;
        item_offset = layout->row.item_offset;
        item_spacing = 0;

        if (modify) {
            layout->row.item_offset += item_width + spacing.x;
            layout->row.filled += layout->row.item_width;
            layout->row.index = 0;
        }
    } break;
    case NK_LAYOUT_DYNAMIC_FREE: {
        /* panel width depended free widget placing */
        bounds->x = layout->at_x + (layout->bounds.w * layout->row.item.x);
        bounds->x -= (float)*layout->offset_x;
        bounds->y = layout->at_y + (layout->row.height * layout->row.item.y);
        bounds->y -= (float)*layout->offset_y;
        bounds->w = layout->bounds.w  * layout->row.item.w;
        bounds->h = layout->row.height * layout->row.item.h;
        return;
    }
    case NK_LAYOUT_DYNAMIC: {
        /* scaling arrays of panel width ratios for every widget */
        float ratio;
        NK_ASSERT(layout->row.ratio);
        ratio = (layout->row.ratio[layout->row.index] < 0) ?
            layout->row.item_width : layout->row.ratio[layout->row.index];

        item_spacing = (float)layout->row.index * spacing.x;
        item_width = (ratio * panel_space);
        item_offset = layout->row.item_offset;

        if (modify) {
            layout->row.item_offset += item_width;
            layout->row.filled += ratio;
        }
    } break;
    case NK_LAYOUT_STATIC_FIXED: {
        /* non-scaling fixed widgets item width */
        item_width = layout->row.item_width;
        item_offset = (float)layout->row.index * item_width;
        item_spacing = (float)layout->row.index * spacing.x;
    } break;
    case NK_LAYOUT_STATIC_ROW: {
        /* scaling single ratio widget width */
        item_width = layout->row.item_width;
        item_offset = layout->row.item_offset;
        item_spacing = (float)layout->row.index * spacing.x;
        if (modify) layout->row.item_offset += item_width;
    } break;
    case NK_LAYOUT_STATIC_FREE: {
        /* free widget placing */
        bounds->x = layout->at_x + layout->row.item.x;
        bounds->w = layout->row.item.w;
        if (((bounds->x + bounds->w) > layout->max_x) && modify)
            layout->max_x = (bounds->x + bounds->w);
        bounds->x -= (float)*layout->offset_x;
        bounds->y = layout->at_y + layout->row.item.y;
        bounds->y -= (float)*layout->offset_y;
        bounds->h = layout->row.item.h;
        return;
    }
    case NK_LAYOUT_STATIC: {
        /* non-scaling array of panel pixel width for every widget */
        item_spacing = (float)layout->row.index * spacing.x;
        item_width = layout->row.ratio[layout->row.index];
        item_offset = layout->row.item_offset;
        if (modify) layout->row.item_offset += item_width;
    } break;
    case NK_LAYOUT_TEMPLATE: {
        /* stretchy row layout with combined dynamic/static widget width*/
        NK_ASSERT(layout->row.index < layout->row.columns);
        NK_ASSERT(layout->row.index < NK_MAX_LAYOUT_ROW_TEMPLATE_COLUMNS);
        item_width = layout->row.templates[layout->row.index];
        item_offset = layout->row.item_offset;
        item_spacing = (float)layout->row.index * spacing.x;
        if (modify) layout->row.item_offset += item_width;
    } break;
    default: NK_ASSERT(0); break;
    };

    /* set the bounds of the newly allocated widget */
    bounds->w = item_width;
    bounds->h = layout->row.height - spacing.y;
    bounds->y = layout->at_y - (float)*layout->offset_y;
    bounds->x = layout->at_x + item_offset + item_spacing + padding.x;
    if (((bounds->x + bounds->w) > layout->max_x) && modify)
        layout->max_x = bounds->x + bounds->w;
    bounds->x -= (float)*layout->offset_x;
}
NK_LIB void
nk_panel_alloc_space(struct nk_rect *bounds, const struct nk_context *ctx)
{
    struct nk_window *win;
    struct nk_panel *layout;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    /* check if the end of the row has been hit and begin new row if so */
    win = ctx->current;
    layout = win->layout;
    if (layout->row.index >= layout->row.columns)
        nk_panel_alloc_row(ctx, win);

    /* calculate widget position and size */
    nk_layout_widget_space(bounds, ctx, win, nk_true);
    layout->row.index++;
}
NK_LIB void
nk_layout_peek(struct nk_rect *bounds, struct nk_context *ctx)
{
    float y;
    int index;
    struct nk_window *win;
    struct nk_panel *layout;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    y = layout->at_y;
    index = layout->row.index;
    if (layout->row.index >= layout->row.columns) {
        layout->at_y += layout->row.height;
        layout->row.index = 0;
    }
    nk_layout_widget_space(bounds, ctx, win, nk_false);
    if (!layout->row.index) {
        bounds->x -= layout->row.item_offset;
    }
    layout->at_y = y;
    layout->row.index = index;
}

