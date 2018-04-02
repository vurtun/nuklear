#include "nuklear.h"
#include "nuklear_internal.h"

/* ==============================================================
 *
 *                          BUTTON
 *
 * ===============================================================*/
NK_LIB void
nk_draw_symbol(struct nk_command_buffer *out, enum nk_symbol_type type,
    struct nk_rect content, struct nk_color background, struct nk_color foreground,
    float border_width, const struct nk_user_font *font)
{
    switch (type) {
    case NK_SYMBOL_X:
    case NK_SYMBOL_UNDERSCORE:
    case NK_SYMBOL_PLUS:
    case NK_SYMBOL_MINUS: {
        /* single character text symbol */
        const char *X = (type == NK_SYMBOL_X) ? "x":
            (type == NK_SYMBOL_UNDERSCORE) ? "_":
            (type == NK_SYMBOL_PLUS) ? "+": "-";
        struct nk_text text;
        text.padding = nk_vec2(0,0);
        text.background = background;
        text.text = foreground;
        nk_widget_text(out, content, X, 1, &text, NK_TEXT_CENTERED, font);
    } break;
    case NK_SYMBOL_CIRCLE_SOLID:
    case NK_SYMBOL_CIRCLE_OUTLINE:
    case NK_SYMBOL_RECT_SOLID:
    case NK_SYMBOL_RECT_OUTLINE: {
        /* simple empty/filled shapes */
        if (type == NK_SYMBOL_RECT_SOLID || type == NK_SYMBOL_RECT_OUTLINE) {
            nk_fill_rect(out, content,  0, foreground);
            if (type == NK_SYMBOL_RECT_OUTLINE)
                nk_fill_rect(out, nk_shrink_rect(content, border_width), 0, background);
        } else {
            nk_fill_circle(out, content, foreground);
            if (type == NK_SYMBOL_CIRCLE_OUTLINE)
                nk_fill_circle(out, nk_shrink_rect(content, 1), background);
        }
    } break;
    case NK_SYMBOL_TRIANGLE_UP:
    case NK_SYMBOL_TRIANGLE_DOWN:
    case NK_SYMBOL_TRIANGLE_LEFT:
    case NK_SYMBOL_TRIANGLE_RIGHT: {
        enum nk_heading heading;
        struct nk_vec2 points[3];
        heading = (type == NK_SYMBOL_TRIANGLE_RIGHT) ? NK_RIGHT :
            (type == NK_SYMBOL_TRIANGLE_LEFT) ? NK_LEFT:
            (type == NK_SYMBOL_TRIANGLE_UP) ? NK_UP: NK_DOWN;
        nk_triangle_from_direction(points, content, 0, 0, heading);
        nk_fill_triangle(out, points[0].x, points[0].y, points[1].x, points[1].y,
            points[2].x, points[2].y, foreground);
    } break;
    default:
    case NK_SYMBOL_NONE:
    case NK_SYMBOL_MAX: break;
    }
}
NK_LIB int
nk_button_behavior(nk_flags *state, struct nk_rect r,
    const struct nk_input *i, enum nk_button_behavior behavior)
{
    int ret = 0;
    nk_widget_state_reset(state);
    if (!i) return 0;
    if (nk_input_is_mouse_hovering_rect(i, r)) {
        *state = NK_WIDGET_STATE_HOVERED;
        if (nk_input_is_mouse_down(i, NK_BUTTON_LEFT))
            *state = NK_WIDGET_STATE_ACTIVE;
        if (nk_input_has_mouse_click_in_rect(i, NK_BUTTON_LEFT, r)) {
            ret = (behavior != NK_BUTTON_DEFAULT) ?
                nk_input_is_mouse_down(i, NK_BUTTON_LEFT):
#ifdef NK_BUTTON_TRIGGER_ON_RELEASE
                nk_input_is_mouse_released(i, NK_BUTTON_LEFT);
#else
                nk_input_is_mouse_pressed(i, NK_BUTTON_LEFT);
#endif
        }
    }
    if (*state & NK_WIDGET_STATE_HOVER && !nk_input_is_mouse_prev_hovering_rect(i, r))
        *state |= NK_WIDGET_STATE_ENTERED;
    else if (nk_input_is_mouse_prev_hovering_rect(i, r))
        *state |= NK_WIDGET_STATE_LEFT;
    return ret;
}
NK_LIB const struct nk_style_item*
nk_draw_button(struct nk_command_buffer *out,
    const struct nk_rect *bounds, nk_flags state,
    const struct nk_style_button *style)
{
    const struct nk_style_item *background;
    if (state & NK_WIDGET_STATE_HOVER)
        background = &style->hover;
    else if (state & NK_WIDGET_STATE_ACTIVED)
        background = &style->active;
    else background = &style->normal;

    if (background->type == NK_STYLE_ITEM_IMAGE) {
        nk_draw_image(out, *bounds, &background->data.image, nk_white);
    } else {
        nk_fill_rect(out, *bounds, style->rounding, background->data.color);
        nk_stroke_rect(out, *bounds, style->rounding, style->border, style->border_color);
    }
    return background;
}
NK_LIB int
nk_do_button(nk_flags *state, struct nk_command_buffer *out, struct nk_rect r,
    const struct nk_style_button *style, const struct nk_input *in,
    enum nk_button_behavior behavior, struct nk_rect *content)
{
    struct nk_rect bounds;
    NK_ASSERT(style);
    NK_ASSERT(state);
    NK_ASSERT(out);
    if (!out || !style)
        return nk_false;

    /* calculate button content space */
    content->x = r.x + style->padding.x + style->border + style->rounding;
    content->y = r.y + style->padding.y + style->border + style->rounding;
    content->w = r.w - (2 * style->padding.x + style->border + style->rounding*2);
    content->h = r.h - (2 * style->padding.y + style->border + style->rounding*2);

    /* execute button behavior */
    bounds.x = r.x - style->touch_padding.x;
    bounds.y = r.y - style->touch_padding.y;
    bounds.w = r.w + 2 * style->touch_padding.x;
    bounds.h = r.h + 2 * style->touch_padding.y;
    return nk_button_behavior(state, bounds, in, behavior);
}
NK_LIB void
nk_draw_button_text(struct nk_command_buffer *out,
    const struct nk_rect *bounds, const struct nk_rect *content, nk_flags state,
    const struct nk_style_button *style, const char *txt, int len,
    nk_flags text_alignment, const struct nk_user_font *font)
{
    struct nk_text text;
    const struct nk_style_item *background;
    background = nk_draw_button(out, bounds, state, style);

    /* select correct colors/images */
    if (background->type == NK_STYLE_ITEM_COLOR)
        text.background = background->data.color;
    else text.background = style->text_background;
    if (state & NK_WIDGET_STATE_HOVER)
        text.text = style->text_hover;
    else if (state & NK_WIDGET_STATE_ACTIVED)
        text.text = style->text_active;
    else text.text = style->text_normal;

    text.padding = nk_vec2(0,0);
    nk_widget_text(out, *content, txt, len, &text, text_alignment, font);
}
NK_LIB int
nk_do_button_text(nk_flags *state,
    struct nk_command_buffer *out, struct nk_rect bounds,
    const char *string, int len, nk_flags align, enum nk_button_behavior behavior,
    const struct nk_style_button *style, const struct nk_input *in,
    const struct nk_user_font *font)
{
    struct nk_rect content;
    int ret = nk_false;

    NK_ASSERT(state);
    NK_ASSERT(style);
    NK_ASSERT(out);
    NK_ASSERT(string);
    NK_ASSERT(font);
    if (!out || !style || !font || !string)
        return nk_false;

    ret = nk_do_button(state, out, bounds, style, in, behavior, &content);
    if (style->draw_begin) style->draw_begin(out, style->userdata);
    nk_draw_button_text(out, &bounds, &content, *state, style, string, len, align, font);
    if (style->draw_end) style->draw_end(out, style->userdata);
    return ret;
}
NK_LIB void
nk_draw_button_symbol(struct nk_command_buffer *out,
    const struct nk_rect *bounds, const struct nk_rect *content,
    nk_flags state, const struct nk_style_button *style,
    enum nk_symbol_type type, const struct nk_user_font *font)
{
    struct nk_color sym, bg;
    const struct nk_style_item *background;

    /* select correct colors/images */
    background = nk_draw_button(out, bounds, state, style);
    if (background->type == NK_STYLE_ITEM_COLOR)
        bg = background->data.color;
    else bg = style->text_background;

    if (state & NK_WIDGET_STATE_HOVER)
        sym = style->text_hover;
    else if (state & NK_WIDGET_STATE_ACTIVED)
        sym = style->text_active;
    else sym = style->text_normal;
    nk_draw_symbol(out, type, *content, bg, sym, 1, font);
}
NK_LIB int
nk_do_button_symbol(nk_flags *state,
    struct nk_command_buffer *out, struct nk_rect bounds,
    enum nk_symbol_type symbol, enum nk_button_behavior behavior,
    const struct nk_style_button *style, const struct nk_input *in,
    const struct nk_user_font *font)
{
    int ret;
    struct nk_rect content;

    NK_ASSERT(state);
    NK_ASSERT(style);
    NK_ASSERT(font);
    NK_ASSERT(out);
    if (!out || !style || !font || !state)
        return nk_false;

    ret = nk_do_button(state, out, bounds, style, in, behavior, &content);
    if (style->draw_begin) style->draw_begin(out, style->userdata);
    nk_draw_button_symbol(out, &bounds, &content, *state, style, symbol, font);
    if (style->draw_end) style->draw_end(out, style->userdata);
    return ret;
}
NK_LIB void
nk_draw_button_image(struct nk_command_buffer *out,
    const struct nk_rect *bounds, const struct nk_rect *content,
    nk_flags state, const struct nk_style_button *style, const struct nk_image *img)
{
    nk_draw_button(out, bounds, state, style);
    nk_draw_image(out, *content, img, nk_white);
}
NK_LIB int
nk_do_button_image(nk_flags *state,
    struct nk_command_buffer *out, struct nk_rect bounds,
    struct nk_image img, enum nk_button_behavior b,
    const struct nk_style_button *style, const struct nk_input *in)
{
    int ret;
    struct nk_rect content;

    NK_ASSERT(state);
    NK_ASSERT(style);
    NK_ASSERT(out);
    if (!out || !style || !state)
        return nk_false;

    ret = nk_do_button(state, out, bounds, style, in, b, &content);
    content.x += style->image_padding.x;
    content.y += style->image_padding.y;
    content.w -= 2 * style->image_padding.x;
    content.h -= 2 * style->image_padding.y;

    if (style->draw_begin) style->draw_begin(out, style->userdata);
    nk_draw_button_image(out, &bounds, &content, *state, style, &img);
    if (style->draw_end) style->draw_end(out, style->userdata);
    return ret;
}
NK_LIB void
nk_draw_button_text_symbol(struct nk_command_buffer *out,
    const struct nk_rect *bounds, const struct nk_rect *label,
    const struct nk_rect *symbol, nk_flags state, const struct nk_style_button *style,
    const char *str, int len, enum nk_symbol_type type,
    const struct nk_user_font *font)
{
    struct nk_color sym;
    struct nk_text text;
    const struct nk_style_item *background;

    /* select correct background colors/images */
    background = nk_draw_button(out, bounds, state, style);
    if (background->type == NK_STYLE_ITEM_COLOR)
        text.background = background->data.color;
    else text.background = style->text_background;

    /* select correct text colors */
    if (state & NK_WIDGET_STATE_HOVER) {
        sym = style->text_hover;
        text.text = style->text_hover;
    } else if (state & NK_WIDGET_STATE_ACTIVED) {
        sym = style->text_active;
        text.text = style->text_active;
    } else {
        sym = style->text_normal;
        text.text = style->text_normal;
    }

    text.padding = nk_vec2(0,0);
    nk_draw_symbol(out, type, *symbol, style->text_background, sym, 0, font);
    nk_widget_text(out, *label, str, len, &text, NK_TEXT_CENTERED, font);
}
NK_LIB int
nk_do_button_text_symbol(nk_flags *state,
    struct nk_command_buffer *out, struct nk_rect bounds,
    enum nk_symbol_type symbol, const char *str, int len, nk_flags align,
    enum nk_button_behavior behavior, const struct nk_style_button *style,
    const struct nk_user_font *font, const struct nk_input *in)
{
    int ret;
    struct nk_rect tri = {0,0,0,0};
    struct nk_rect content;

    NK_ASSERT(style);
    NK_ASSERT(out);
    NK_ASSERT(font);
    if (!out || !style || !font)
        return nk_false;

    ret = nk_do_button(state, out, bounds, style, in, behavior, &content);
    tri.y = content.y + (content.h/2) - font->height/2;
    tri.w = font->height; tri.h = font->height;
    if (align & NK_TEXT_ALIGN_LEFT) {
        tri.x = (content.x + content.w) - (2 * style->padding.x + tri.w);
        tri.x = NK_MAX(tri.x, 0);
    } else tri.x = content.x + 2 * style->padding.x;

    /* draw button */
    if (style->draw_begin) style->draw_begin(out, style->userdata);
    nk_draw_button_text_symbol(out, &bounds, &content, &tri,
        *state, style, str, len, symbol, font);
    if (style->draw_end) style->draw_end(out, style->userdata);
    return ret;
}
NK_LIB void
nk_draw_button_text_image(struct nk_command_buffer *out,
    const struct nk_rect *bounds, const struct nk_rect *label,
    const struct nk_rect *image, nk_flags state, const struct nk_style_button *style,
    const char *str, int len, const struct nk_user_font *font,
    const struct nk_image *img)
{
    struct nk_text text;
    const struct nk_style_item *background;
    background = nk_draw_button(out, bounds, state, style);

    /* select correct colors */
    if (background->type == NK_STYLE_ITEM_COLOR)
        text.background = background->data.color;
    else text.background = style->text_background;
    if (state & NK_WIDGET_STATE_HOVER)
        text.text = style->text_hover;
    else if (state & NK_WIDGET_STATE_ACTIVED)
        text.text = style->text_active;
    else text.text = style->text_normal;

    text.padding = nk_vec2(0,0);
    nk_widget_text(out, *label, str, len, &text, NK_TEXT_CENTERED, font);
    nk_draw_image(out, *image, img, nk_white);
}
NK_LIB int
nk_do_button_text_image(nk_flags *state,
    struct nk_command_buffer *out, struct nk_rect bounds,
    struct nk_image img, const char* str, int len, nk_flags align,
    enum nk_button_behavior behavior, const struct nk_style_button *style,
    const struct nk_user_font *font, const struct nk_input *in)
{
    int ret;
    struct nk_rect icon;
    struct nk_rect content;

    NK_ASSERT(style);
    NK_ASSERT(state);
    NK_ASSERT(font);
    NK_ASSERT(out);
    if (!out || !font || !style || !str)
        return nk_false;

    ret = nk_do_button(state, out, bounds, style, in, behavior, &content);
    icon.y = bounds.y + style->padding.y;
    icon.w = icon.h = bounds.h - 2 * style->padding.y;
    if (align & NK_TEXT_ALIGN_LEFT) {
        icon.x = (bounds.x + bounds.w) - (2 * style->padding.x + icon.w);
        icon.x = NK_MAX(icon.x, 0);
    } else icon.x = bounds.x + 2 * style->padding.x;

    icon.x += style->image_padding.x;
    icon.y += style->image_padding.y;
    icon.w -= 2 * style->image_padding.x;
    icon.h -= 2 * style->image_padding.y;

    if (style->draw_begin) style->draw_begin(out, style->userdata);
    nk_draw_button_text_image(out, &bounds, &content, &icon, *state, style, str, len, font, &img);
    if (style->draw_end) style->draw_end(out, style->userdata);
    return ret;
}
NK_API void
nk_button_set_behavior(struct nk_context *ctx, enum nk_button_behavior behavior)
{
    NK_ASSERT(ctx);
    if (!ctx) return;
    ctx->button_behavior = behavior;
}
NK_API int
nk_button_push_behavior(struct nk_context *ctx, enum nk_button_behavior behavior)
{
    struct nk_config_stack_button_behavior *button_stack;
    struct nk_config_stack_button_behavior_element *element;

    NK_ASSERT(ctx);
    if (!ctx) return 0;

    button_stack = &ctx->stacks.button_behaviors;
    NK_ASSERT(button_stack->head < (int)NK_LEN(button_stack->elements));
    if (button_stack->head >= (int)NK_LEN(button_stack->elements))
        return 0;

    element = &button_stack->elements[button_stack->head++];
    element->address = &ctx->button_behavior;
    element->old_value = ctx->button_behavior;
    ctx->button_behavior = behavior;
    return 1;
}
NK_API int
nk_button_pop_behavior(struct nk_context *ctx)
{
    struct nk_config_stack_button_behavior *button_stack;
    struct nk_config_stack_button_behavior_element *element;

    NK_ASSERT(ctx);
    if (!ctx) return 0;

    button_stack = &ctx->stacks.button_behaviors;
    NK_ASSERT(button_stack->head > 0);
    if (button_stack->head < 1)
        return 0;

    element = &button_stack->elements[--button_stack->head];
    *element->address = element->old_value;
    return 1;
}
NK_API int
nk_button_text_styled(struct nk_context *ctx,
    const struct nk_style_button *style, const char *title, int len)
{
    struct nk_window *win;
    struct nk_panel *layout;
    const struct nk_input *in;

    struct nk_rect bounds;
    enum nk_widget_layout_states state;

    NK_ASSERT(ctx);
    NK_ASSERT(style);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!style || !ctx || !ctx->current || !ctx->current->layout) return 0;

    win = ctx->current;
    layout = win->layout;
    state = nk_widget(&bounds, ctx);

    if (!state) return 0;
    in = (state == NK_WIDGET_ROM || layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;
    return nk_do_button_text(&ctx->last_widget_state, &win->buffer, bounds,
                    title, len, style->text_alignment, ctx->button_behavior,
                    style, in, ctx->style.font);
}
NK_API int
nk_button_text(struct nk_context *ctx, const char *title, int len)
{
    NK_ASSERT(ctx);
    if (!ctx) return 0;
    return nk_button_text_styled(ctx, &ctx->style.button, title, len);
}
NK_API int nk_button_label_styled(struct nk_context *ctx,
    const struct nk_style_button *style, const char *title)
{
    return nk_button_text_styled(ctx, style, title, nk_strlen(title));
}
NK_API int nk_button_label(struct nk_context *ctx, const char *title)
{
    return nk_button_text(ctx, title, nk_strlen(title));
}
NK_API int
nk_button_color(struct nk_context *ctx, struct nk_color color)
{
    struct nk_window *win;
    struct nk_panel *layout;
    const struct nk_input *in;
    struct nk_style_button button;

    int ret = 0;
    struct nk_rect bounds;
    struct nk_rect content;
    enum nk_widget_layout_states state;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    layout = win->layout;

    state = nk_widget(&bounds, ctx);
    if (!state) return 0;
    in = (state == NK_WIDGET_ROM || layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;

    button = ctx->style.button;
    button.normal = nk_style_item_color(color);
    button.hover = nk_style_item_color(color);
    button.active = nk_style_item_color(color);
    ret = nk_do_button(&ctx->last_widget_state, &win->buffer, bounds,
                &button, in, ctx->button_behavior, &content);
    nk_draw_button(&win->buffer, &bounds, ctx->last_widget_state, &button);
    return ret;
}
NK_API int
nk_button_symbol_styled(struct nk_context *ctx,
    const struct nk_style_button *style, enum nk_symbol_type symbol)
{
    struct nk_window *win;
    struct nk_panel *layout;
    const struct nk_input *in;

    struct nk_rect bounds;
    enum nk_widget_layout_states state;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    layout = win->layout;
    state = nk_widget(&bounds, ctx);
    if (!state) return 0;
    in = (state == NK_WIDGET_ROM || layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;
    return nk_do_button_symbol(&ctx->last_widget_state, &win->buffer, bounds,
            symbol, ctx->button_behavior, style, in, ctx->style.font);
}
NK_API int
nk_button_symbol(struct nk_context *ctx, enum nk_symbol_type symbol)
{
    NK_ASSERT(ctx);
    if (!ctx) return 0;
    return nk_button_symbol_styled(ctx, &ctx->style.button, symbol);
}
NK_API int
nk_button_image_styled(struct nk_context *ctx, const struct nk_style_button *style,
    struct nk_image img)
{
    struct nk_window *win;
    struct nk_panel *layout;
    const struct nk_input *in;

    struct nk_rect bounds;
    enum nk_widget_layout_states state;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    layout = win->layout;

    state = nk_widget(&bounds, ctx);
    if (!state) return 0;
    in = (state == NK_WIDGET_ROM || layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;
    return nk_do_button_image(&ctx->last_widget_state, &win->buffer, bounds,
                img, ctx->button_behavior, style, in);
}
NK_API int
nk_button_image(struct nk_context *ctx, struct nk_image img)
{
    NK_ASSERT(ctx);
    if (!ctx) return 0;
    return nk_button_image_styled(ctx, &ctx->style.button, img);
}
NK_API int
nk_button_symbol_text_styled(struct nk_context *ctx,
    const struct nk_style_button *style, enum nk_symbol_type symbol,
    const char *text, int len, nk_flags align)
{
    struct nk_window *win;
    struct nk_panel *layout;
    const struct nk_input *in;

    struct nk_rect bounds;
    enum nk_widget_layout_states state;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    layout = win->layout;

    state = nk_widget(&bounds, ctx);
    if (!state) return 0;
    in = (state == NK_WIDGET_ROM || layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;
    return nk_do_button_text_symbol(&ctx->last_widget_state, &win->buffer, bounds,
                symbol, text, len, align, ctx->button_behavior,
                style, ctx->style.font, in);
}
NK_API int
nk_button_symbol_text(struct nk_context *ctx, enum nk_symbol_type symbol,
    const char* text, int len, nk_flags align)
{
    NK_ASSERT(ctx);
    if (!ctx) return 0;
    return nk_button_symbol_text_styled(ctx, &ctx->style.button, symbol, text, len, align);
}
NK_API int nk_button_symbol_label(struct nk_context *ctx, enum nk_symbol_type symbol,
    const char *label, nk_flags align)
{
    return nk_button_symbol_text(ctx, symbol, label, nk_strlen(label), align);
}
NK_API int nk_button_symbol_label_styled(struct nk_context *ctx,
    const struct nk_style_button *style, enum nk_symbol_type symbol,
    const char *title, nk_flags align)
{
    return nk_button_symbol_text_styled(ctx, style, symbol, title, nk_strlen(title), align);
}
NK_API int
nk_button_image_text_styled(struct nk_context *ctx,
    const struct nk_style_button *style, struct nk_image img, const char *text,
    int len, nk_flags align)
{
    struct nk_window *win;
    struct nk_panel *layout;
    const struct nk_input *in;

    struct nk_rect bounds;
    enum nk_widget_layout_states state;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    layout = win->layout;

    state = nk_widget(&bounds, ctx);
    if (!state) return 0;
    in = (state == NK_WIDGET_ROM || layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;
    return nk_do_button_text_image(&ctx->last_widget_state, &win->buffer,
            bounds, img, text, len, align, ctx->button_behavior,
            style, ctx->style.font, in);
}
NK_API int
nk_button_image_text(struct nk_context *ctx, struct nk_image img,
    const char *text, int len, nk_flags align)
{
    return nk_button_image_text_styled(ctx, &ctx->style.button,img, text, len, align);
}
NK_API int nk_button_image_label(struct nk_context *ctx, struct nk_image img,
    const char *label, nk_flags align)
{
    return nk_button_image_text(ctx, img, label, nk_strlen(label), align);
}
NK_API int nk_button_image_label_styled(struct nk_context *ctx,
    const struct nk_style_button *style, struct nk_image img,
    const char *label, nk_flags text_alignment)
{
    return nk_button_image_text_styled(ctx, style, img, label, nk_strlen(label), text_alignment);
}

