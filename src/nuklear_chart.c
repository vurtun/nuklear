#include "nuklear.h"
#include "nuklear_internal.h"

/* ==============================================================
 *
 *                          CHART
 *
 * ===============================================================*/
NK_API int
nk_chart_begin_colored(struct nk_context *ctx, enum nk_chart_type type,
    struct nk_color color, struct nk_color highlight,
    int count, float min_value, float max_value)
{
    struct nk_window *win;
    struct nk_chart *chart;
    const struct nk_style *config;
    const struct nk_style_chart *style;

    const struct nk_style_item *background;
    struct nk_rect bounds = {0, 0, 0, 0};

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);

    if (!ctx || !ctx->current || !ctx->current->layout) return 0;
    if (!nk_widget(&bounds, ctx)) {
        chart = &ctx->current->layout->chart;
        nk_zero(chart, sizeof(*chart));
        return 0;
    }

    win = ctx->current;
    config = &ctx->style;
    chart = &win->layout->chart;
    style = &config->chart;

    /* setup basic generic chart  */
    nk_zero(chart, sizeof(*chart));
    chart->x = bounds.x + style->padding.x;
    chart->y = bounds.y + style->padding.y;
    chart->w = bounds.w - 2 * style->padding.x;
    chart->h = bounds.h - 2 * style->padding.y;
    chart->w = NK_MAX(chart->w, 2 * style->padding.x);
    chart->h = NK_MAX(chart->h, 2 * style->padding.y);

    /* add first slot into chart */
    {struct nk_chart_slot *slot = &chart->slots[chart->slot++];
    slot->type = type;
    slot->count = count;
    slot->color = color;
    slot->highlight = highlight;
    slot->min = NK_MIN(min_value, max_value);
    slot->max = NK_MAX(min_value, max_value);
    slot->range = slot->max - slot->min;}

    /* draw chart background */
    background = &style->background;
    if (background->type == NK_STYLE_ITEM_IMAGE) {
        nk_draw_image(&win->buffer, bounds, &background->data.image, nk_white);
    } else {
        nk_fill_rect(&win->buffer, bounds, style->rounding, style->border_color);
        nk_fill_rect(&win->buffer, nk_shrink_rect(bounds, style->border),
            style->rounding, style->background.data.color);
    }
    return 1;
}
NK_API int
nk_chart_begin(struct nk_context *ctx, const enum nk_chart_type type,
    int count, float min_value, float max_value)
{
    return nk_chart_begin_colored(ctx, type, ctx->style.chart.color,
                ctx->style.chart.selected_color, count, min_value, max_value);
}
NK_API void
nk_chart_add_slot_colored(struct nk_context *ctx, const enum nk_chart_type type,
    struct nk_color color, struct nk_color highlight,
    int count, float min_value, float max_value)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    NK_ASSERT(ctx->current->layout->chart.slot < NK_CHART_MAX_SLOT);
    if (!ctx || !ctx->current || !ctx->current->layout) return;
    if (ctx->current->layout->chart.slot >= NK_CHART_MAX_SLOT) return;

    /* add another slot into the graph */
    {struct nk_chart *chart = &ctx->current->layout->chart;
    struct nk_chart_slot *slot = &chart->slots[chart->slot++];
    slot->type = type;
    slot->count = count;
    slot->color = color;
    slot->highlight = highlight;
    slot->min = NK_MIN(min_value, max_value);
    slot->max = NK_MAX(min_value, max_value);
    slot->range = slot->max - slot->min;}
}
NK_API void
nk_chart_add_slot(struct nk_context *ctx, const enum nk_chart_type type,
    int count, float min_value, float max_value)
{
    nk_chart_add_slot_colored(ctx, type, ctx->style.chart.color,
        ctx->style.chart.selected_color, count, min_value, max_value);
}
NK_INTERN nk_flags
nk_chart_push_line(struct nk_context *ctx, struct nk_window *win,
    struct nk_chart *g, float value, int slot)
{
    struct nk_panel *layout = win->layout;
    const struct nk_input *i = &ctx->input;
    struct nk_command_buffer *out = &win->buffer;

    nk_flags ret = 0;
    struct nk_vec2 cur;
    struct nk_rect bounds;
    struct nk_color color;
    float step;
    float range;
    float ratio;

    NK_ASSERT(slot >= 0 && slot < NK_CHART_MAX_SLOT);
    step = g->w / (float)g->slots[slot].count;
    range = g->slots[slot].max - g->slots[slot].min;
    ratio = (value - g->slots[slot].min) / range;

    if (g->slots[slot].index == 0) {
        /* first data point does not have a connection */
        g->slots[slot].last.x = g->x;
        g->slots[slot].last.y = (g->y + g->h) - ratio * (float)g->h;

        bounds.x = g->slots[slot].last.x - 2;
        bounds.y = g->slots[slot].last.y - 2;
        bounds.w = bounds.h = 4;

        color = g->slots[slot].color;
        if (!(layout->flags & NK_WINDOW_ROM) &&
            NK_INBOX(i->mouse.pos.x,i->mouse.pos.y, g->slots[slot].last.x-3, g->slots[slot].last.y-3, 6, 6)){
            ret = nk_input_is_mouse_hovering_rect(i, bounds) ? NK_CHART_HOVERING : 0;
            ret |= (i->mouse.buttons[NK_BUTTON_LEFT].down &&
                i->mouse.buttons[NK_BUTTON_LEFT].clicked) ? NK_CHART_CLICKED: 0;
            color = g->slots[slot].highlight;
        }
        nk_fill_rect(out, bounds, 0, color);
        g->slots[slot].index += 1;
        return ret;
    }

    /* draw a line between the last data point and the new one */
    color = g->slots[slot].color;
    cur.x = g->x + (float)(step * (float)g->slots[slot].index);
    cur.y = (g->y + g->h) - (ratio * (float)g->h);
    nk_stroke_line(out, g->slots[slot].last.x, g->slots[slot].last.y, cur.x, cur.y, 1.0f, color);

    bounds.x = cur.x - 3;
    bounds.y = cur.y - 3;
    bounds.w = bounds.h = 6;

    /* user selection of current data point */
    if (!(layout->flags & NK_WINDOW_ROM)) {
        if (nk_input_is_mouse_hovering_rect(i, bounds)) {
            ret = NK_CHART_HOVERING;
            ret |= (!i->mouse.buttons[NK_BUTTON_LEFT].down &&
                i->mouse.buttons[NK_BUTTON_LEFT].clicked) ? NK_CHART_CLICKED: 0;
            color = g->slots[slot].highlight;
        }
    }
    nk_fill_rect(out, nk_rect(cur.x - 2, cur.y - 2, 4, 4), 0, color);

    /* save current data point position */
    g->slots[slot].last.x = cur.x;
    g->slots[slot].last.y = cur.y;
    g->slots[slot].index  += 1;
    return ret;
}
NK_INTERN nk_flags
nk_chart_push_column(const struct nk_context *ctx, struct nk_window *win,
    struct nk_chart *chart, float value, int slot)
{
    struct nk_command_buffer *out = &win->buffer;
    const struct nk_input *in = &ctx->input;
    struct nk_panel *layout = win->layout;

    float ratio;
    nk_flags ret = 0;
    struct nk_color color;
    struct nk_rect item = {0,0,0,0};

    NK_ASSERT(slot >= 0 && slot < NK_CHART_MAX_SLOT);
    if (chart->slots[slot].index  >= chart->slots[slot].count)
        return nk_false;
    if (chart->slots[slot].count) {
        float padding = (float)(chart->slots[slot].count-1);
        item.w = (chart->w - padding) / (float)(chart->slots[slot].count);
    }

    /* calculate bounds of current bar chart entry */
    color = chart->slots[slot].color;;
    item.h = chart->h * NK_ABS((value/chart->slots[slot].range));
    if (value >= 0) {
        ratio = (value + NK_ABS(chart->slots[slot].min)) / NK_ABS(chart->slots[slot].range);
        item.y = (chart->y + chart->h) - chart->h * ratio;
    } else {
        ratio = (value - chart->slots[slot].max) / chart->slots[slot].range;
        item.y = chart->y + (chart->h * NK_ABS(ratio)) - item.h;
    }
    item.x = chart->x + ((float)chart->slots[slot].index * item.w);
    item.x = item.x + ((float)chart->slots[slot].index);

    /* user chart bar selection */
    if (!(layout->flags & NK_WINDOW_ROM) &&
        NK_INBOX(in->mouse.pos.x,in->mouse.pos.y,item.x,item.y,item.w,item.h)) {
        ret = NK_CHART_HOVERING;
        ret |= (!in->mouse.buttons[NK_BUTTON_LEFT].down &&
                in->mouse.buttons[NK_BUTTON_LEFT].clicked) ? NK_CHART_CLICKED: 0;
        color = chart->slots[slot].highlight;
    }
    nk_fill_rect(out, item, 0, color);
    chart->slots[slot].index += 1;
    return ret;
}
NK_API nk_flags
nk_chart_push_slot(struct nk_context *ctx, float value, int slot)
{
    nk_flags flags;
    struct nk_window *win;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(slot >= 0 && slot < NK_CHART_MAX_SLOT);
    NK_ASSERT(slot < ctx->current->layout->chart.slot);
    if (!ctx || !ctx->current || slot >= NK_CHART_MAX_SLOT) return nk_false;
    if (slot >= ctx->current->layout->chart.slot) return nk_false;

    win = ctx->current;
    if (win->layout->chart.slot < slot) return nk_false;
    switch (win->layout->chart.slots[slot].type) {
    case NK_CHART_LINES:
        flags = nk_chart_push_line(ctx, win, &win->layout->chart, value, slot); break;
    case NK_CHART_COLUMN:
        flags = nk_chart_push_column(ctx, win, &win->layout->chart, value, slot); break;
    default:
    case NK_CHART_MAX:
        flags = 0;
    }
    return flags;
}
NK_API nk_flags
nk_chart_push(struct nk_context *ctx, float value)
{
    return nk_chart_push_slot(ctx, value, 0);
}
NK_API void
nk_chart_end(struct nk_context *ctx)
{
    struct nk_window *win;
    struct nk_chart *chart;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current)
        return;

    win = ctx->current;
    chart = &win->layout->chart;
    NK_MEMSET(chart, 0, sizeof(*chart));
    return;
}
NK_API void
nk_plot(struct nk_context *ctx, enum nk_chart_type type, const float *values,
    int count, int offset)
{
    int i = 0;
    float min_value;
    float max_value;

    NK_ASSERT(ctx);
    NK_ASSERT(values);
    if (!ctx || !values || !count) return;

    min_value = values[offset];
    max_value = values[offset];
    for (i = 0; i < count; ++i) {
        min_value = NK_MIN(values[i + offset], min_value);
        max_value = NK_MAX(values[i + offset], max_value);
    }

    if (nk_chart_begin(ctx, type, count, min_value, max_value)) {
        for (i = 0; i < count; ++i)
            nk_chart_push(ctx, values[i + offset]);
        nk_chart_end(ctx);
    }
}
NK_API void
nk_plot_function(struct nk_context *ctx, enum nk_chart_type type, void *userdata,
    float(*value_getter)(void* user, int index), int count, int offset)
{
    int i = 0;
    float min_value;
    float max_value;

    NK_ASSERT(ctx);
    NK_ASSERT(value_getter);
    if (!ctx || !value_getter || !count) return;

    max_value = min_value = value_getter(userdata, offset);
    for (i = 0; i < count; ++i) {
        float value = value_getter(userdata, i + offset);
        min_value = NK_MIN(value, min_value);
        max_value = NK_MAX(value, max_value);
    }

    if (nk_chart_begin(ctx, type, count, min_value, max_value)) {
        for (i = 0; i < count; ++i)
            nk_chart_push(ctx, value_getter(userdata, i + offset));
        nk_chart_end(ctx);
    }
}

