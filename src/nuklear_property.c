#include "nuklear.h"
#include "nuklear_internal.h"

/* ===============================================================
 *
 *                              PROPERTY
 *
 * ===============================================================*/
NK_LIB void
nk_drag_behavior(nk_flags *state, const struct nk_input *in,
    struct nk_rect drag, struct nk_property_variant *variant,
    float inc_per_pixel)
{
    int left_mouse_down = in && in->mouse.buttons[NK_BUTTON_LEFT].down;
    int left_mouse_click_in_cursor = in &&
        nk_input_has_mouse_click_down_in_rect(in, NK_BUTTON_LEFT, drag, nk_true);

    nk_widget_state_reset(state);
    if (nk_input_is_mouse_hovering_rect(in, drag))
        *state = NK_WIDGET_STATE_HOVERED;

    if (left_mouse_down && left_mouse_click_in_cursor) {
        float delta, pixels;
        pixels = in->mouse.delta.x;
        delta = pixels * inc_per_pixel;
        switch (variant->kind) {
        default: break;
        case NK_PROPERTY_INT:
            variant->value.i = variant->value.i + (int)delta;
            variant->value.i = NK_CLAMP(variant->min_value.i, variant->value.i, variant->max_value.i);
            break;
        case NK_PROPERTY_FLOAT:
            variant->value.f = variant->value.f + (float)delta;
            variant->value.f = NK_CLAMP(variant->min_value.f, variant->value.f, variant->max_value.f);
            break;
        case NK_PROPERTY_DOUBLE:
            variant->value.d = variant->value.d + (double)delta;
            variant->value.d = NK_CLAMP(variant->min_value.d, variant->value.d, variant->max_value.d);
            break;
        }
        *state = NK_WIDGET_STATE_ACTIVE;
    }
    if (*state & NK_WIDGET_STATE_HOVER && !nk_input_is_mouse_prev_hovering_rect(in, drag))
        *state |= NK_WIDGET_STATE_ENTERED;
    else if (nk_input_is_mouse_prev_hovering_rect(in, drag))
        *state |= NK_WIDGET_STATE_LEFT;
}
NK_LIB void
nk_property_behavior(nk_flags *ws, const struct nk_input *in,
    struct nk_rect property,  struct nk_rect label, struct nk_rect edit,
    struct nk_rect empty, int *state, struct nk_property_variant *variant,
    float inc_per_pixel)
{
    if (in && *state == NK_PROPERTY_DEFAULT) {
        if (nk_button_behavior(ws, edit, in, NK_BUTTON_DEFAULT))
            *state = NK_PROPERTY_EDIT;
        else if (nk_input_is_mouse_click_down_in_rect(in, NK_BUTTON_LEFT, label, nk_true))
            *state = NK_PROPERTY_DRAG;
        else if (nk_input_is_mouse_click_down_in_rect(in, NK_BUTTON_LEFT, empty, nk_true))
            *state = NK_PROPERTY_DRAG;
    }
    if (*state == NK_PROPERTY_DRAG) {
        nk_drag_behavior(ws, in, property, variant, inc_per_pixel);
        if (!(*ws & NK_WIDGET_STATE_ACTIVED)) *state = NK_PROPERTY_DEFAULT;
    }
}
NK_LIB void
nk_draw_property(struct nk_command_buffer *out, const struct nk_style_property *style,
    const struct nk_rect *bounds, const struct nk_rect *label, nk_flags state,
    const char *name, int len, const struct nk_user_font *font)
{
    struct nk_text text;
    const struct nk_style_item *background;

    /* select correct background and text color */
    if (state & NK_WIDGET_STATE_ACTIVED) {
        background = &style->active;
        text.text = style->label_active;
    } else if (state & NK_WIDGET_STATE_HOVER) {
        background = &style->hover;
        text.text = style->label_hover;
    } else {
        background = &style->normal;
        text.text = style->label_normal;
    }

    /* draw background */
    if (background->type == NK_STYLE_ITEM_IMAGE) {
        nk_draw_image(out, *bounds, &background->data.image, nk_white);
        text.background = nk_rgba(0,0,0,0);
    } else {
        text.background = background->data.color;
        nk_fill_rect(out, *bounds, style->rounding, background->data.color);
        nk_stroke_rect(out, *bounds, style->rounding, style->border, background->data.color);
    }

    /* draw label */
    text.padding = nk_vec2(0,0);
    nk_widget_text(out, *label, name, len, &text, NK_TEXT_CENTERED, font);
}
NK_LIB void
nk_do_property(nk_flags *ws,
    struct nk_command_buffer *out, struct nk_rect property,
    const char *name, struct nk_property_variant *variant,
    float inc_per_pixel, char *buffer, int *len,
    int *state, int *cursor, int *select_begin, int *select_end,
    const struct nk_style_property *style,
    enum nk_property_filter filter, struct nk_input *in,
    const struct nk_user_font *font, struct nk_text_edit *text_edit,
    enum nk_button_behavior behavior)
{
    const nk_plugin_filter filters[] = {
        nk_filter_decimal,
        nk_filter_float
    };
    int active, old;
    int num_len, name_len;
    char string[NK_MAX_NUMBER_BUFFER];
    float size;

    char *dst = 0;
    int *length;

    struct nk_rect left;
    struct nk_rect right;
    struct nk_rect label;
    struct nk_rect edit;
    struct nk_rect empty;

    /* left decrement button */
    left.h = font->height/2;
    left.w = left.h;
    left.x = property.x + style->border + style->padding.x;
    left.y = property.y + style->border + property.h/2.0f - left.h/2;

    /* text label */
    name_len = nk_strlen(name);
    size = font->width(font->userdata, font->height, name, name_len);
    label.x = left.x + left.w + style->padding.x;
    label.w = (float)size + 2 * style->padding.x;
    label.y = property.y + style->border + style->padding.y;
    label.h = property.h - (2 * style->border + 2 * style->padding.y);

    /* right increment button */
    right.y = left.y;
    right.w = left.w;
    right.h = left.h;
    right.x = property.x + property.w - (right.w + style->padding.x);

    /* edit */
    if (*state == NK_PROPERTY_EDIT) {
        size = font->width(font->userdata, font->height, buffer, *len);
        size += style->edit.cursor_size;
        length = len;
        dst = buffer;
    } else {
        switch (variant->kind) {
        default: break;
        case NK_PROPERTY_INT:
            nk_itoa(string, variant->value.i);
            num_len = nk_strlen(string);
            break;
        case NK_PROPERTY_FLOAT:
            NK_DTOA(string, (double)variant->value.f);
            num_len = nk_string_float_limit(string, NK_MAX_FLOAT_PRECISION);
            break;
        case NK_PROPERTY_DOUBLE:
            NK_DTOA(string, variant->value.d);
            num_len = nk_string_float_limit(string, NK_MAX_FLOAT_PRECISION);
            break;
        }
        size = font->width(font->userdata, font->height, string, num_len);
        dst = string;
        length = &num_len;
    }

    edit.w =  (float)size + 2 * style->padding.x;
    edit.w = NK_MIN(edit.w, right.x - (label.x + label.w));
    edit.x = right.x - (edit.w + style->padding.x);
    edit.y = property.y + style->border;
    edit.h = property.h - (2 * style->border);

    /* empty left space activator */
    empty.w = edit.x - (label.x + label.w);
    empty.x = label.x + label.w;
    empty.y = property.y;
    empty.h = property.h;

    /* update property */
    old = (*state == NK_PROPERTY_EDIT);
    nk_property_behavior(ws, in, property, label, edit, empty, state, variant, inc_per_pixel);

    /* draw property */
    if (style->draw_begin) style->draw_begin(out, style->userdata);
    nk_draw_property(out, style, &property, &label, *ws, name, name_len, font);
    if (style->draw_end) style->draw_end(out, style->userdata);

    /* execute right button  */
    if (nk_do_button_symbol(ws, out, left, style->sym_left, behavior, &style->dec_button, in, font)) {
        switch (variant->kind) {
        default: break;
        case NK_PROPERTY_INT:
            variant->value.i = NK_CLAMP(variant->min_value.i, variant->value.i - variant->step.i, variant->max_value.i); break;
        case NK_PROPERTY_FLOAT:
            variant->value.f = NK_CLAMP(variant->min_value.f, variant->value.f - variant->step.f, variant->max_value.f); break;
        case NK_PROPERTY_DOUBLE:
            variant->value.d = NK_CLAMP(variant->min_value.d, variant->value.d - variant->step.d, variant->max_value.d); break;
        }
    }
    /* execute left button  */
    if (nk_do_button_symbol(ws, out, right, style->sym_right, behavior, &style->inc_button, in, font)) {
        switch (variant->kind) {
        default: break;
        case NK_PROPERTY_INT:
            variant->value.i = NK_CLAMP(variant->min_value.i, variant->value.i + variant->step.i, variant->max_value.i); break;
        case NK_PROPERTY_FLOAT:
            variant->value.f = NK_CLAMP(variant->min_value.f, variant->value.f + variant->step.f, variant->max_value.f); break;
        case NK_PROPERTY_DOUBLE:
            variant->value.d = NK_CLAMP(variant->min_value.d, variant->value.d + variant->step.d, variant->max_value.d); break;
        }
    }
    if (old != NK_PROPERTY_EDIT && (*state == NK_PROPERTY_EDIT)) {
        /* property has been activated so setup buffer */
        NK_MEMCPY(buffer, dst, (nk_size)*length);
        *cursor = nk_utf_len(buffer, *length);
        *len = *length;
        length = len;
        dst = buffer;
        active = 0;
    } else active = (*state == NK_PROPERTY_EDIT);

    /* execute and run text edit field */
    nk_textedit_clear_state(text_edit, NK_TEXT_EDIT_SINGLE_LINE, filters[filter]);
    text_edit->active = (unsigned char)active;
    text_edit->string.len = *length;
    text_edit->cursor = NK_CLAMP(0, *cursor, *length);
    text_edit->select_start = NK_CLAMP(0,*select_begin, *length);
    text_edit->select_end = NK_CLAMP(0,*select_end, *length);
    text_edit->string.buffer.allocated = (nk_size)*length;
    text_edit->string.buffer.memory.size = NK_MAX_NUMBER_BUFFER;
    text_edit->string.buffer.memory.ptr = dst;
    text_edit->string.buffer.size = NK_MAX_NUMBER_BUFFER;
    text_edit->mode = NK_TEXT_EDIT_MODE_INSERT;
    nk_do_edit(ws, out, edit, NK_EDIT_FIELD|NK_EDIT_AUTO_SELECT,
        filters[filter], text_edit, &style->edit, (*state == NK_PROPERTY_EDIT) ? in: 0, font);

    *length = text_edit->string.len;
    *cursor = text_edit->cursor;
    *select_begin = text_edit->select_start;
    *select_end = text_edit->select_end;
    if (text_edit->active && nk_input_is_key_pressed(in, NK_KEY_ENTER))
        text_edit->active = nk_false;

    if (active && !text_edit->active) {
        /* property is now not active so convert edit text to value*/
        *state = NK_PROPERTY_DEFAULT;
        buffer[*len] = '\0';
        switch (variant->kind) {
        default: break;
        case NK_PROPERTY_INT:
            variant->value.i = nk_strtoi(buffer, 0);
            variant->value.i = NK_CLAMP(variant->min_value.i, variant->value.i, variant->max_value.i);
            break;
        case NK_PROPERTY_FLOAT:
            nk_string_float_limit(buffer, NK_MAX_FLOAT_PRECISION);
            variant->value.f = nk_strtof(buffer, 0);
            variant->value.f = NK_CLAMP(variant->min_value.f, variant->value.f, variant->max_value.f);
            break;
        case NK_PROPERTY_DOUBLE:
            nk_string_float_limit(buffer, NK_MAX_FLOAT_PRECISION);
            variant->value.d = nk_strtod(buffer, 0);
            variant->value.d = NK_CLAMP(variant->min_value.d, variant->value.d, variant->max_value.d);
            break;
        }
    }
}
NK_LIB struct nk_property_variant
nk_property_variant_int(int value, int min_value, int max_value, int step)
{
    struct nk_property_variant result;
    result.kind = NK_PROPERTY_INT;
    result.value.i = value;
    result.min_value.i = min_value;
    result.max_value.i = max_value;
    result.step.i = step;
    return result;
}
NK_LIB struct nk_property_variant
nk_property_variant_float(float value, float min_value, float max_value, float step)
{
    struct nk_property_variant result;
    result.kind = NK_PROPERTY_FLOAT;
    result.value.f = value;
    result.min_value.f = min_value;
    result.max_value.f = max_value;
    result.step.f = step;
    return result;
}
NK_LIB struct nk_property_variant
nk_property_variant_double(double value, double min_value, double max_value,
    double step)
{
    struct nk_property_variant result;
    result.kind = NK_PROPERTY_DOUBLE;
    result.value.d = value;
    result.min_value.d = min_value;
    result.max_value.d = max_value;
    result.step.d = step;
    return result;
}
NK_LIB void
nk_property(struct nk_context *ctx, const char *name, struct nk_property_variant *variant,
    float inc_per_pixel, const enum nk_property_filter filter)
{
    struct nk_window *win;
    struct nk_panel *layout;
    struct nk_input *in;
    const struct nk_style *style;

    struct nk_rect bounds;
    enum nk_widget_layout_states s;

    int *state = 0;
    nk_hash hash = 0;
    char *buffer = 0;
    int *len = 0;
    int *cursor = 0;
    int *select_begin = 0;
    int *select_end = 0;
    int old_state;

    char dummy_buffer[NK_MAX_NUMBER_BUFFER];
    int dummy_state = NK_PROPERTY_DEFAULT;
    int dummy_length = 0;
    int dummy_cursor = 0;
    int dummy_select_begin = 0;
    int dummy_select_end = 0;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    style = &ctx->style;
    s = nk_widget(&bounds, ctx);
    if (!s) return;

    /* calculate hash from name */
    if (name[0] == '#') {
        hash = nk_murmur_hash(name, (int)nk_strlen(name), win->property.seq++);
        name++; /* special number hash */
    } else hash = nk_murmur_hash(name, (int)nk_strlen(name), 42);

    /* check if property is currently hot item */
    if (win->property.active && hash == win->property.name) {
        buffer = win->property.buffer;
        len = &win->property.length;
        cursor = &win->property.cursor;
        state = &win->property.state;
        select_begin = &win->property.select_start;
        select_end = &win->property.select_end;
    } else {
        buffer = dummy_buffer;
        len = &dummy_length;
        cursor = &dummy_cursor;
        state = &dummy_state;
        select_begin =  &dummy_select_begin;
        select_end = &dummy_select_end;
    }

    /* execute property widget */
    old_state = *state;
    ctx->text_edit.clip = ctx->clip;
    in = ((s == NK_WIDGET_ROM && !win->property.active) ||
        layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;
    nk_do_property(&ctx->last_widget_state, &win->buffer, bounds, name,
        variant, inc_per_pixel, buffer, len, state, cursor, select_begin,
        select_end, &style->property, filter, in, style->font, &ctx->text_edit,
        ctx->button_behavior);

    if (in && *state != NK_PROPERTY_DEFAULT && !win->property.active) {
        /* current property is now hot */
        win->property.active = 1;
        NK_MEMCPY(win->property.buffer, buffer, (nk_size)*len);
        win->property.length = *len;
        win->property.cursor = *cursor;
        win->property.state = *state;
        win->property.name = hash;
        win->property.select_start = *select_begin;
        win->property.select_end = *select_end;
        if (*state == NK_PROPERTY_DRAG) {
            ctx->input.mouse.grab = nk_true;
            ctx->input.mouse.grabbed = nk_true;
        }
    }
    /* check if previously active property is now inactive */
    if (*state == NK_PROPERTY_DEFAULT && old_state != NK_PROPERTY_DEFAULT) {
        if (old_state == NK_PROPERTY_DRAG) {
            ctx->input.mouse.grab = nk_false;
            ctx->input.mouse.grabbed = nk_false;
            ctx->input.mouse.ungrab = nk_true;
        }
        win->property.select_start = 0;
        win->property.select_end = 0;
        win->property.active = 0;
    }
}
NK_API void
nk_property_int(struct nk_context *ctx, const char *name,
    int min, int *val, int max, int step, float inc_per_pixel)
{
    struct nk_property_variant variant;
    NK_ASSERT(ctx);
    NK_ASSERT(name);
    NK_ASSERT(val);

    if (!ctx || !ctx->current || !name || !val) return;
    variant = nk_property_variant_int(*val, min, max, step);
    nk_property(ctx, name, &variant, inc_per_pixel, NK_FILTER_INT);
    *val = variant.value.i;
}
NK_API void
nk_property_float(struct nk_context *ctx, const char *name,
    float min, float *val, float max, float step, float inc_per_pixel)
{
    struct nk_property_variant variant;
    NK_ASSERT(ctx);
    NK_ASSERT(name);
    NK_ASSERT(val);

    if (!ctx || !ctx->current || !name || !val) return;
    variant = nk_property_variant_float(*val, min, max, step);
    nk_property(ctx, name, &variant, inc_per_pixel, NK_FILTER_FLOAT);
    *val = variant.value.f;
}
NK_API void
nk_property_double(struct nk_context *ctx, const char *name,
    double min, double *val, double max, double step, float inc_per_pixel)
{
    struct nk_property_variant variant;
    NK_ASSERT(ctx);
    NK_ASSERT(name);
    NK_ASSERT(val);

    if (!ctx || !ctx->current || !name || !val) return;
    variant = nk_property_variant_double(*val, min, max, step);
    nk_property(ctx, name, &variant, inc_per_pixel, NK_FILTER_FLOAT);
    *val = variant.value.d;
}
NK_API int
nk_propertyi(struct nk_context *ctx, const char *name, int min, int val,
    int max, int step, float inc_per_pixel)
{
    struct nk_property_variant variant;
    NK_ASSERT(ctx);
    NK_ASSERT(name);

    if (!ctx || !ctx->current || !name) return val;
    variant = nk_property_variant_int(val, min, max, step);
    nk_property(ctx, name, &variant, inc_per_pixel, NK_FILTER_INT);
    val = variant.value.i;
    return val;
}
NK_API float
nk_propertyf(struct nk_context *ctx, const char *name, float min,
    float val, float max, float step, float inc_per_pixel)
{
    struct nk_property_variant variant;
    NK_ASSERT(ctx);
    NK_ASSERT(name);

    if (!ctx || !ctx->current || !name) return val;
    variant = nk_property_variant_float(val, min, max, step);
    nk_property(ctx, name, &variant, inc_per_pixel, NK_FILTER_FLOAT);
    val = variant.value.f;
    return val;
}
NK_API double
nk_propertyd(struct nk_context *ctx, const char *name, double min,
    double val, double max, double step, float inc_per_pixel)
{
    struct nk_property_variant variant;
    NK_ASSERT(ctx);
    NK_ASSERT(name);

    if (!ctx || !ctx->current || !name) return val;
    variant = nk_property_variant_double(val, min, max, step);
    nk_property(ctx, name, &variant, inc_per_pixel, NK_FILTER_FLOAT);
    val = variant.value.d;
    return val;
}

