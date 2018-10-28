#include "nuklear.h"
#include "nuklear_internal.h"

/* ===============================================================
 *
 *                              TOOLTIP
 *
 * ===============================================================*/
NK_API int
nk_tooltip_begin(struct nk_context *ctx, float width)
{
    int x,y,w,h;
    struct nk_window *win;
    const struct nk_input *in;
    struct nk_rect bounds;
    int ret;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    /* make sure that no nonblocking popup is currently active */
    win = ctx->current;
    in = &ctx->input;
    if (win->popup.win && (win->popup.type & NK_PANEL_SET_NONBLOCK))
        return 0;

    w = nk_iceilf(width);
    h = nk_iceilf(nk_null_rect.h);
    x = nk_ifloorf(in->mouse.pos.x + 1) - (int)win->layout->clip.x;
    y = nk_ifloorf(in->mouse.pos.y + 1) - (int)win->layout->clip.y;

    bounds.x = (float)x;
    bounds.y = (float)y;
    bounds.w = (float)w;
    bounds.h = (float)h;

    ret = nk_popup_begin(ctx, NK_POPUP_DYNAMIC,
        "__##Tooltip##__", NK_WINDOW_NO_SCROLLBAR|NK_WINDOW_BORDER, bounds);
    if (ret) win->layout->flags &= ~(nk_flags)NK_WINDOW_ROM;
    win->popup.type = NK_PANEL_TOOLTIP;
    ctx->current->layout->type = NK_PANEL_TOOLTIP;
    return ret;
}

NK_API void
nk_tooltip_end(struct nk_context *ctx)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return;
    ctx->current->seq--;
    nk_popup_close(ctx);
    nk_popup_end(ctx);
}
NK_API void
nk_tooltip(struct nk_context *ctx, const char *text)
{
    const struct nk_style *style;
    struct nk_vec2 padding;

    int text_len;
    float text_width;
    float text_height;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    NK_ASSERT(text);
    if (!ctx || !ctx->current || !ctx->current->layout || !text)
        return;

    /* fetch configuration data */
    style = &ctx->style;
    padding = style->window.padding;

    /* calculate size of the text and tooltip */
    text_len = nk_strlen(text);
    text_width = style->font->width(style->font->userdata,
                    style->font->height, text, text_len);
    text_width += (4 * padding.x);
    text_height = (style->font->height + 2 * padding.y);

    /* execute tooltip and fill with text */
    if (nk_tooltip_begin(ctx, (float)text_width)) {
        nk_layout_row_dynamic(ctx, (float)text_height, 1);
        nk_text(ctx, text, text_len, NK_TEXT_LEFT);
        nk_tooltip_end(ctx);
    }
}
#ifdef NK_INCLUDE_STANDARD_VARARGS
NK_API void
nk_tooltipf(struct nk_context *ctx, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    nk_tooltipfv(ctx, fmt, args);
    va_end(args);
}
NK_API void
nk_tooltipfv(struct nk_context *ctx, const char *fmt, va_list args)
{
    char buf[256];
    nk_strfmt(buf, NK_LEN(buf), fmt, args);
    nk_tooltip(ctx, buf);
}
#endif


