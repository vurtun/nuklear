/*
    Copyright (c) 2015
    vurtun <polygone@gmx.net>
    MIT licence
*/
#include "gui.h"

#ifdef GUI_USE_DEBUG_BUILD
#include <assert.h>
#define ASSERT assert
#else
#define ASSERT(expr)
#endif

#ifndef NULL
#define NULL (void*)0
#endif
#define MAX_NUMBER_BUFFER 64
#define UNUSED(a)   ((void)(a))
#define PASTE(a,b) a##b
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#define SATURATE(x) (MAX(0, MIN(1.0f, x)))
#define LEN(a) (sizeof(a)/sizeof(a)[0])
#define ABS(a) (((a) < 0) ? -(a) : (a))
#define BETWEEN(x, a, b) ((a) <= (x) && (x) <= (b))
#define INBOX(px, py, x, y, w, h) (BETWEEN(px, x, x+w) && BETWEEN(py, y, y+h))
#define INTERSECT(x0, y0, w0, h0, x1, y1, w1, h1) \
    (!(((x1 > (x0 + w0)) || ((x1 + w1) < x0) || (y1 > (y0 + h0)) || (y1 + h1) < y0)))

#define col_load(c,j,k,l,m) (c).r = (j), (c).g = (k), (c).b = (l), (c).a = (m)
#define vec2_load(v,a,b) (v).x = (a), (v).y = (b)
#define vec2_mov(to,from) (to).x = (from).x, (to).y = (from).y
#define vec2_len(v) ((float)fsqrt((v).x*(v).x+(v).y*(v).y))
#define vec2_sub(r,a,b) do {(r).x=(a).x-(b).x; (r).y=(a).y-(b).y;} while(0)
#define vec2_muls(r, v, s) do {(r).x=(v).x*(s); (r).y=(v).y*(s);} while(0)
#define ALIGNOF(t) ((char*)(&((struct {char c; t _h;}*)0)->_h) - (char*)0)
#define ALIGN_PTR(x, mask) (void*)((gui_size)((gui_byte*)(x) + (mask-1)) & ~(mask-1))
#define ALIGN(x, mask) ((x) + (mask-1)) & ~(mask-1)
#define OFFSETOF(st, m) ((gui_size)(&((st *)0)->m))

static const struct gui_rect null_rect = {0.0f, 0.0f, 9999.0f, 9999.0f};
static const gui_char utfbyte[GUI_UTF_SIZE+1] = {0x80, 0, 0xC0, 0xE0, 0xF0};
static const gui_char utfmask[GUI_UTF_SIZE+1] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
static const long utfmin[GUI_UTF_SIZE+1] = {0, 0, 0x80, 0x800, 0x100000};
static const long utfmax[GUI_UTF_SIZE+1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};

static void*
memcopy(void *dst, const void *src, gui_size size)
{
    gui_size i = 0;
    char *d = dst;
    const char *s = src;
    for (i = 0; i < size; ++i)
        d[i] = s[i];
    return dst;
}

static void
zero(void *dst, gui_size size)
{
    gui_size i;
    char *d = dst;
    for (i = 0; i < size; ++i) d[i] = 0;
}

static gui_size
strsiz(const char *str)
{
    gui_size siz = 0;
    while (str && *str++ != '\0') siz++;
    return siz;
}

static gui_int
strtoi(gui_int *number, const char *buffer, gui_size len)
{
    gui_size i;
    if (!number || !buffer)
        return 0;
    *number = 0;
    for (i = 0; i < len; ++i)
        *number = *number * 10 + (buffer[i] - '0');
    return 1;
}

static gui_size
itos(char *buffer, gui_int num)
{
    static const char digit[] = "0123456789";
    gui_int shifter;
    gui_size len = 0;
    char *p = buffer;
    if (!buffer)
        return 0;

    if (num < 0) {
        num = ABS(num);
        *p++ = '-';
    }
    shifter = num;

    do {
        ++p;
        shifter = shifter/10;
    } while (shifter);
    *p = '\0';

    len = (gui_size)(p - buffer);
    do {
        *--p = digit[num % 10];
        num = num / 10;
    } while (num);
    return len;
}

static void
unify(struct gui_rect *clip, const struct gui_rect *a, gui_float x0, gui_float y0,
    gui_float x1, gui_float y1)
{
    clip->x = MAX(a->x, x0) - 1;
    clip->y = MAX(a->y, y0) - 1;
    clip->w = MIN(a->x + a->w, x1) - clip->x+ 2;
    clip->h = MIN(a->y + a->h, y1) - clip->y + 2;
    clip->w = MAX(0, clip->w);
    clip->h = MAX(0, clip->h);
}

static gui_size
gui_utf_validate(long *u, gui_size i)
{
    if (!u) return 0;
    if (!BETWEEN(*u, utfmin[i], utfmax[i]) ||
         BETWEEN(*u, 0xD800, 0xDFFF))
            *u = GUI_UTF_INVALID;
    for (i = 1; *u > utfmax[i]; ++i);
    return i;
}

static gui_long
gui_utf_decode_byte(gui_char c, gui_size *i)
{
    if (!i) return 0;
    for(*i = 0; *i < LEN(utfmask); ++(*i)) {
        if ((c & utfmask[*i]) == utfbyte[*i])
            return c & ~utfmask[*i];
    }
    return 0;
}

gui_size
gui_utf_decode(const gui_char *c, gui_long *u, gui_size clen)
{
    gui_size i, j, len, type;
    gui_long udecoded;

    *u = GUI_UTF_INVALID;
    if (!c || !u) return 0;
    if (!clen) return 0;
    udecoded = gui_utf_decode_byte(c[0], &len);
    if (!BETWEEN(len, 1, GUI_UTF_SIZE))
        return 1;

    for (i = 1, j = 1; i < clen && j < len; ++i, ++j) {
        udecoded = (udecoded << 6) | gui_utf_decode_byte(c[i], &type);
        if (type != 0)
            return j;
    }
    if (j < len)
        return 0;
    *u = udecoded;
    gui_utf_validate(u, len);
    return len;
}

static gui_char
gui_utf_encode_byte(gui_long u, gui_size i)
{
    return (gui_char)(utfbyte[i] | (u & ~utfmask[i]));
}

gui_size
gui_utf_encode(gui_long u, gui_char *c, gui_size clen)
{
    gui_size len, i;
    len = gui_utf_validate(&u, 0);
    if (clen < len || !len)
        return 0;
    for (i = len - 1; i != 0; --i) {
        c[i] = gui_utf_encode_byte(u, 0);
        u >>= 6;
    }
    c[0] = gui_utf_encode_byte(u, len);
    return len;
}

static void
gui_triangle_from_direction(struct gui_vec2 *result, gui_float x, gui_float y,
    gui_float w, gui_float h, gui_float pad_x, gui_float pad_y,
    enum gui_heading direction)
{
    gui_float w_half, h_half;
    ASSERT(result);

    w = MAX(4 * pad_x, w);
    h = MAX(4 * pad_y, h);
    w = w - 2 * pad_x;
    h = h - 2 * pad_y;
    x = x + pad_x;
    y = y + pad_y;
    w_half = w / 2.0f;
    h_half = h / 2.0f;

    if (direction == GUI_UP) {
        result[0].x = x + w_half;
        result[0].y = y;
        result[1].x = x;
        result[1].y = y + h;
        result[2].x = x + w;
        result[2].y = y + h;
    } else if (direction == GUI_RIGHT) {
        result[0].x = x;
        result[0].y = y;
        result[1].x = x;
        result[1].y = y + h;
        result[2].x = x + w;
        result[2].y = y + h_half;
    } else if (direction == GUI_DOWN) {
        result[0].x = x;
        result[0].y = y;
        result[1].x = x + w_half;
        result[1].y = y + h;
        result[2].x = x + w;
        result[2].y = y;
    } else {
        result[0].x = x;
        result[0].y = y + h_half;
        result[1].x = x + w;
        result[1].y = y + h;
        result[2].x = x + w;
        result[2].y = y;
    }
}

void
gui_input_begin(struct gui_input *in)
{
    gui_size i;
    ASSERT(in);
    if (!in) return;
    in->mouse_clicked = 0;
    in->text_len = 0;
    vec2_mov(in->mouse_prev, in->mouse_pos);
    for (i = 0; i < GUI_KEY_MAX; i++)
        in->keys[i].clicked = 0;
}

void
gui_input_motion(struct gui_input *in, gui_int x, gui_int y)
{
    ASSERT(in);
    if (!in) return;
    in->mouse_pos.x = (gui_float)x;
    in->mouse_pos.y = (gui_float)y;
}

void
gui_input_key(struct gui_input *in, enum gui_keys key, gui_bool down)
{
    ASSERT(in);
    if (!in) return;
    if (in->keys[key].down == down) return;
    in->keys[key].down = down;
    in->keys[key].clicked++;
}

void
gui_input_button(struct gui_input *in, gui_int x, gui_int y, gui_bool down)
{
    ASSERT(in);
    if (!in) return;
    if (in->mouse_down == down) return;
    in->mouse_clicked_pos.x = (gui_float)x;
    in->mouse_clicked_pos.y = (gui_float)y;
    in->mouse_down = down;
    in->mouse_clicked++;
}

void
gui_input_char(struct gui_input *in, const gui_glyph glyph)
{
    gui_size len = 0;
    gui_long unicode;
    ASSERT(in);
    if (!in) return;
    len = gui_utf_decode(glyph, &unicode, GUI_UTF_SIZE);
    if (len && ((in->text_len + len) < GUI_INPUT_MAX)) {
        gui_utf_encode(unicode, &in->text[in->text_len], GUI_INPUT_MAX - in->text_len);
        in->text_len += len;
    }
}

void
gui_input_end(struct gui_input *in)
{
    ASSERT(in);
    if (!in) return;
    vec2_sub(in->mouse_delta, in->mouse_pos, in->mouse_prev);
}

void
gui_text(const struct gui_canvas *canvas, gui_float x, gui_float y, gui_float w, gui_float h,
    const char *string, gui_size len, const struct gui_text *text, enum gui_text_align align,
    const struct gui_font *font)
{
    gui_float label_x;
    gui_float label_y;
    gui_float label_w;
    gui_float label_h;
    gui_size text_width;

    ASSERT(text);
    if (!text) return;

    text_width = font->width(font->userdata, (const gui_char*)string, len);
    label_y = y + text->padding.y;
    label_h = MAX(0, h - 2 * text->padding.y);
    if (align == GUI_TEXT_LEFT) {
        label_x = x + text->padding.x;
        label_w = MAX(0, w - 2 * text->padding.x);
    } else if (align == GUI_TEXT_CENTERED) {
        label_w = 2 * text->padding.x + (gui_float)text_width;
        label_x = (x + text->padding.x + ((w - 2 * text->padding.x)/2)) - (label_w/2);
        label_x = MAX(x + text->padding.x, label_x);
        label_w = MIN(x + w, label_x + label_w) - label_x;
    } else if (align == GUI_TEXT_RIGHT) {
        label_x = MAX(x, (x + w) - (2 * text->padding.x + (gui_float)text_width));
        label_w = (gui_float)text_width + 2 * text->padding.x;
    } else return;

    canvas->draw_rect(canvas->userdata, x, y, w, h, text->background);
    canvas->draw_text(canvas->userdata, label_x, label_y, label_w, label_h,
        (const gui_char*)string, len, font, text->background, text->foreground);
}

static gui_bool
gui_do_button(const struct gui_canvas *canvas, gui_float x, gui_float y, gui_float w,
    gui_float h, const struct gui_button *button, const struct gui_input *in,
    enum gui_button_behavior behavior)
{
    gui_bool ret = gui_false;
    struct gui_color background;

    ASSERT(button);
    if (!button)
        return gui_false;

    background = button->background;
    if (in && INBOX(in->mouse_pos.x,in->mouse_pos.y, x, y, w, h)) {
        background = button->highlight;
        if (INBOX(in->mouse_clicked_pos.x, in->mouse_clicked_pos.y, x, y, w, h)) {
            ret = (behavior != GUI_BUTTON_DEFAULT) ? in->mouse_down:
                (in->mouse_down && in->mouse_clicked);
        }
    }

    canvas->draw_rect(canvas->userdata, x, y, w, h, button->foreground);
    canvas->draw_rect(canvas->userdata, x + button->border, y + button->border,
        w - 2 * button->border, h - 2 * button->border, background);
    return ret;
}

gui_bool
gui_button_text(const struct gui_canvas *canvas, gui_float x, gui_float y,
    gui_float w, gui_float h, const char *string, enum gui_button_behavior b,
    const struct gui_button *button, const struct gui_input *in, const struct gui_font *font)
{
    gui_bool ret = gui_false;
    gui_float button_w, button_h;
    struct gui_text text;
    struct gui_color font_color;
    struct gui_color bg_color;
    gui_float inner_x, inner_y;
    gui_float inner_w, inner_h;

    ASSERT(button);
    ASSERT(string);
    ASSERT(canvas);
    if (!canvas || !button)
        return gui_false;

    font_color = button->content;
    bg_color = button->background;
    button_w = MAX(w, 2 * button->padding.x);
    button_h = MAX(h, font->height + 2 * button->padding.y);
    if (in && INBOX(in->mouse_pos.x, in->mouse_pos.y, x, y, button_w, button_h)) {
        font_color = button->highlight_content;
        bg_color = button->highlight;
    }
    ret = gui_do_button(canvas, x, y, button_w, button_h, button, in, b);

    inner_x = x + button->border;
    inner_y = y + button->border;
    inner_w = button_w - 2 * button->border;
    inner_h = button_h - 2 * button->border;

    text.padding.x = button->padding.x;
    text.padding.y = button->padding.y;
    text.background = bg_color;
    text.foreground = font_color;
    gui_text(canvas, inner_x, inner_y, inner_w, inner_h, string, strsiz(string),
        &text, GUI_TEXT_CENTERED, font);
    return ret;
}

gui_bool
gui_button_triangle(const struct gui_canvas *canvas, gui_float x, gui_float y,
    gui_float w, gui_float h, enum gui_heading heading, enum gui_button_behavior b,
    const struct gui_button *button, const struct gui_input *in)
{
    gui_bool pressed;
    struct gui_color col;
    struct gui_vec2 points[3];

    ASSERT(button);
    ASSERT(canvas);
    if (!canvas || !button)
        return gui_false;

    pressed = gui_do_button(canvas, x, y, w, h, button, in, b);
    gui_triangle_from_direction(points, x, y, w, h, button->padding.x, button->padding.y, heading);
    col = (in && INBOX(in->mouse_pos.x, in->mouse_pos.y, x, y, w, h)) ?
        button->highlight_content : button->content;
    canvas->draw_triangle(canvas->userdata, points[0].x, points[0].y,
        points[1].x, points[1].y, points[2].x, points[2].y, col);
    return pressed;
}

gui_bool
gui_button_image(const struct gui_canvas *canvas, gui_float x, gui_float y,
    gui_float w, gui_float h, gui_image img, enum gui_button_behavior b,
    const struct gui_button *button, const struct gui_input *in)
{
    gui_bool pressed;
    gui_float img_x, img_y, img_w, img_h;

    ASSERT(button);
    ASSERT(canvas);
    if (!canvas || !button)
        return gui_false;

    pressed = gui_do_button(canvas, x, y, w, h, button, in, b);
    img_x = x + button->padding.x;
    img_y = y + button->padding.y;
    img_w = w - 2 * button->padding.x;
    img_h = h - 2 * button->padding.y;
    canvas->draw_image(canvas->userdata, img_x, img_y, img_w, img_h, img);
    return pressed;
}

gui_bool
gui_toggle(const struct gui_canvas *canvas, gui_float x, gui_float y, gui_float w,
    gui_float h, gui_bool active, const char *string, enum gui_toggle_type type,
    const struct gui_toggle *toggle, const struct gui_input *in, const struct gui_font *font)
{
    gui_bool toggle_active;
    gui_float select_size;
    gui_float toggle_w, toggle_h;
    gui_float select_x, select_y;
    gui_float cursor_x, cursor_y;
    gui_float cursor_pad, cursor_size;

    ASSERT(toggle);
    ASSERT(canvas);
    if (!canvas || !toggle)
        return 0;

    toggle_w = MAX(w, font->height + 2 * toggle->padding.x);
    toggle_h = MAX(h, font->height + 2 * toggle->padding.y);
    toggle_active = active;

    select_x = x + toggle->padding.x;
    select_y = y + toggle->padding.y;
    select_size = font->height + 2 * toggle->padding.y;

    cursor_pad = (gui_float)(gui_int)(select_size / 8);
    cursor_size = select_size - cursor_pad * 2;
    cursor_x = select_x + cursor_pad;
    cursor_y = select_y + cursor_pad;

    if (in && !in->mouse_down && in->mouse_clicked)
        if (INBOX(in->mouse_clicked_pos.x, in->mouse_clicked_pos.y,
                cursor_x, cursor_y, cursor_size, cursor_size))
                toggle_active = !toggle_active;

    if (type == GUI_TOGGLE_CHECK)
        canvas->draw_rect(canvas->userdata, select_x, select_y, select_size,
           select_size, toggle->foreground);
    else canvas->draw_circle(canvas->userdata, select_x, select_y, select_size,
            select_size, toggle->foreground);

    if (toggle_active) {
        if (type == GUI_TOGGLE_CHECK)
            canvas->draw_rect(canvas->userdata,cursor_x,cursor_y,cursor_size,
                cursor_size, toggle->cursor);
        else canvas->draw_circle(canvas->userdata, cursor_x, cursor_y, cursor_size,
                cursor_size, toggle->cursor);
    }

    if (font && string) {
        struct gui_text text;
        gui_float inner_x, inner_y;
        gui_float inner_w, inner_h;

        inner_x = x + select_size + toggle->padding.x * 2;
        inner_y = (y + (select_size / 2)) - (font->height / 2);
        inner_w = (x + toggle_w) - (inner_x + toggle->padding.x);
        inner_h = (y + toggle_h) - (inner_y + toggle->padding.y);

        text.padding.x = 0;
        text.padding.y = 0;
        text.background = toggle->background;
        text.foreground = toggle->font;
        gui_text(canvas, inner_x, inner_y, inner_w, inner_h, string, strsiz(string),
            &text, GUI_TEXT_LEFT, font);
    }
    return toggle_active;
}

gui_float
gui_slider(const struct gui_canvas *canvas, gui_float x, gui_float y, gui_float w, gui_float h,
    gui_float min, gui_float val, gui_float max, gui_float step,
    const struct gui_slider *slider, const struct gui_input *in)
{
    gui_float slider_range;
    gui_float slider_min, slider_max;
    gui_float slider_value, slider_steps;
    gui_float slider_w, slider_h;

    gui_float cursor_offset;
    gui_float cursor_x, cursor_y;
    gui_float cursor_w, cursor_h;

    ASSERT(slider);
    ASSERT(canvas);
    if (!canvas || !slider)
        return 0;

    slider_w = MAX(w, 2 * slider->padding.x);
    slider_h = MAX(h, 2 * slider->padding.y);
    slider_max = MAX(min, max);
    slider_min = MIN(min, max);
    slider_value = CLAMP(slider_min, val, slider_max);
    slider_range = slider_max - slider_min;
    slider_steps = slider_range / step;

    cursor_offset = (slider_value - slider_min) / step;
    cursor_w = (slider_w - 2 * slider->padding.x) / (slider_steps + 1);
    cursor_h = slider_h - 2 * slider->padding.y;
    cursor_x = x + slider->padding.x + (cursor_w * cursor_offset);
    cursor_y = y + slider->padding.y;

    if (in && in->mouse_down &&
        INBOX(in->mouse_pos.x, in->mouse_pos.y, x, y, slider_w, slider_h) &&
        INBOX(in->mouse_clicked_pos.x,in->mouse_clicked_pos.y, x, y, slider_w, slider_h))
    {
        const float d = in->mouse_pos.x - (cursor_x + cursor_w / 2.0f);
        const float pxstep = (slider_w - 2 * slider->padding.x) / slider_steps;
        if (ABS(d) >= pxstep) {
            const gui_float steps = (gui_float)((gui_int)(ABS(d) / pxstep));
            slider_value += (d > 0) ? (step * steps) : -(step * steps);
            slider_value = CLAMP(slider_min, slider_value, slider_max);
            cursor_x = x + slider->padding.x + (cursor_w * (slider_value - slider_min));
        }
    }
    canvas->draw_rect(canvas->userdata, x, y, slider_w, slider_h, slider->background);
    canvas->draw_rect(canvas->userdata,cursor_x,cursor_y,cursor_w,cursor_h,slider->foreground);
    return slider_value;
}

gui_size
gui_progress(const struct gui_canvas *canvas, gui_float x, gui_float y,
    gui_float w, gui_float h, gui_size value, gui_size max, gui_bool modifyable,
    const struct gui_slider *prog, const struct gui_input *in)
{
    gui_float cursor_x, cursor_y;
    gui_float cursor_w, cursor_h;
    gui_float prog_w, prog_h;
    gui_float prog_scale;
    gui_size prog_value;

    ASSERT(prog);
    ASSERT(canvas);
    if (!canvas || !prog) return 0;

    prog_w = MAX(w, 2 * prog->padding.x + 1);
    prog_h = MAX(h, 2 * prog->padding.y + 1);
    prog_value = MIN(value, max);

    if (in && modifyable && in->mouse_down &&
        INBOX(in->mouse_pos.x, in->mouse_pos.y, x, y, prog_w, prog_h)){
        gui_float ratio = (gui_float)(in->mouse_pos.x - x) / (gui_float)prog_w;
        prog_value = (gui_size)((gui_float)max * ratio);
    }

    if (!max) return prog_value;
    prog_value = MIN(prog_value, max);
    prog_scale = (gui_float)prog_value / (gui_float)max;

    cursor_h = prog_h - 2 * prog->padding.y;
    cursor_w = (prog_w - 2 * prog->padding.x) * prog_scale;
    cursor_x = x + prog->padding.x;
    cursor_y = y + prog->padding.y;

    canvas->draw_rect(canvas->userdata, x, y, prog_w, prog_h, prog->background);
    canvas->draw_rect(canvas->userdata, cursor_x, cursor_y, cursor_w, cursor_h, prog->foreground);
    return prog_value;
}

static gui_bool
gui_filter_input_default(gui_long unicode)
{
    UNUSED(unicode);
    return gui_true;
}

static gui_bool
gui_filter_input_ascii(gui_long unicode)
{
    if (unicode < 0 || unicode > 128)
        return gui_false;
    return gui_true;
}

static gui_bool
gui_filter_input_float(gui_long unicode)
{
    if ((unicode < '0' || unicode > '9') && unicode != '.' && unicode != '-')
        return gui_false;
    return gui_true;
}

static gui_bool
gui_filter_input_decimal(gui_long unicode)
{
    if (unicode < '0' || unicode > '9')
        return gui_false;
    return gui_true;
}

static gui_bool
gui_filter_input_hex(gui_long unicode)
{
    if ((unicode < '0' || unicode > '9') &&
        (unicode < 'a' || unicode > 'f') &&
        (unicode < 'A' || unicode > 'B'))
        return gui_false;
    return gui_true;
}

static gui_bool
gui_filter_input_oct(gui_long unicode)
{
    if (unicode < '0' || unicode > '7')
        return gui_false;
    return gui_true;
}

static gui_bool
gui_filter_input_binary(gui_long unicode)
{
    if (unicode < '0' || unicode > '1')
        return gui_false;
    return gui_true;
}

static gui_size
gui_buffer_input(gui_char *buffer, gui_size length, gui_size max,
    gui_filter filter, const struct gui_input *in)
{
    gui_long unicode;
    gui_size src_len = 0;
    gui_size text_len = 0, glyph_len = 0;
    ASSERT(buffer);
    ASSERT(in);

    glyph_len = gui_utf_decode(in->text, &unicode, in->text_len);
    while (glyph_len && ((text_len + glyph_len) <= in->text_len) && (length + src_len) < max) {
        if (filter(unicode)) {
            gui_size i = 0;
            for (i = 0; i < glyph_len; i++)
                buffer[length++] = in->text[text_len + i];
            text_len += glyph_len;
        }
        src_len = src_len + glyph_len;
        glyph_len = gui_utf_decode(in->text + src_len, &unicode, in->text_len - src_len);
    }
    return text_len;
}

gui_size
gui_edit_filtered(const struct gui_canvas *canvas, gui_float x, gui_float y, gui_float w,
    gui_float h, gui_char *buffer, gui_size len, gui_size max, gui_bool *active,
    const struct gui_edit *field, gui_filter filter, const struct gui_input *in,
    const struct gui_font *font)
{
    gui_float input_w, input_h;
    gui_bool input_active;

    ASSERT(canvas);
    ASSERT(buffer);
    ASSERT(field);
    if (!canvas || !buffer || !field)
        return 0;

    input_w = MAX(w, 2 * field->padding.x);
    input_h = MAX(h, font->height);
    input_active = *active;
    canvas->draw_rect(canvas->userdata, x, y, input_w, input_h, field->background);
    canvas->draw_rect(canvas->userdata, x + 1, y, input_w - 1, input_h, field->foreground);
    if (in && in->mouse_clicked && in->mouse_down)
        input_active = INBOX(in->mouse_pos.x, in->mouse_pos.y, x, y, input_w, input_h);

    if (input_active && in) {
        const struct gui_key *bs = &in->keys[GUI_KEY_BACKSPACE];
        const struct gui_key *del = &in->keys[GUI_KEY_DEL];
        const struct gui_key *esc = &in->keys[GUI_KEY_ESCAPE];
        const struct gui_key *enter = &in->keys[GUI_KEY_ENTER];
        const struct gui_key *space = &in->keys[GUI_KEY_SPACE];

        if ((del->down && del->clicked) || (bs->down && bs->clicked))
            if (len > 0) len = len - 1;
        if ((enter->down && enter->clicked) || (esc->down && esc->clicked))
            input_active = gui_false;
        if ((space->down && space->clicked) && (len < max))
            buffer[len++] = ' ';
        if (in->text_len && len < max)
            len += gui_buffer_input(buffer, len, max, filter, in);
    }

    if (font && buffer) {
        gui_size offset = 0;
        gui_float label_x, label_y, label_h;
        gui_float label_w = input_w - 2 * field->padding.x;
        gui_size cursor_width = (gui_size)font->width(font->userdata, (const gui_char*)"X", 1);

        gui_size text_len = len;
        gui_size text_width = font->width(font->userdata, buffer, text_len);
        while (text_len && (text_width + cursor_width) > (gui_size)label_w) {
            gui_long unicode;
            offset += gui_utf_decode(&buffer[offset], &unicode, text_len);
            text_len = len - offset;
            text_width = font->width(font->userdata, &buffer[offset], text_len);
        }

        label_x = x + field->padding.x;
        label_y = y + field->padding.y;
        label_h = input_h - 2 * field->padding.y;
        canvas->draw_text(canvas->userdata, label_x, label_y, label_w, label_h,
            &buffer[offset], text_len, font, field->foreground, field->background);
        if (input_active && field->show_cursor) {
            canvas->draw_rect(canvas->userdata,  label_x + (gui_float)text_width, label_y,
                (gui_float)cursor_width, label_h, field->cursor);
        }
    }
    *active = input_active;
    return len;

}

gui_size
gui_edit(const struct gui_canvas *canvas, gui_float x, gui_float y, gui_float w,
    gui_float h, gui_char *buffer, gui_size len, gui_size max, gui_bool *active,
    const struct gui_edit *field, enum gui_input_filter f,
    const struct gui_input *in, const struct gui_font *font)
{
    static const gui_filter filter[] = {
        gui_filter_input_default,
        gui_filter_input_ascii,
        gui_filter_input_float,
        gui_filter_input_decimal,
        gui_filter_input_hex,
        gui_filter_input_oct,
        gui_filter_input_binary,
    };
    return gui_edit_filtered(canvas, x, y, w, h, buffer, len, max, active,
                field,filter[f], in, font);
}

gui_float
gui_scroll(const struct gui_canvas *canvas, gui_float x, gui_float y,
    gui_float w, gui_float h, gui_float offset, gui_float target,
    gui_float step, const struct gui_scroll *scroll, const struct gui_input *in)
{
    gui_bool button_up_pressed;
    gui_bool button_down_pressed;
    struct gui_button button;

    gui_float scroll_step;
    gui_float scroll_offset;
    gui_float scroll_off, scroll_ratio;
    gui_float scroll_y, scroll_w, scroll_h;

    gui_float cursor_x, cursor_y;
    gui_float cursor_w, cursor_h;
    gui_bool inscroll, incursor;

    ASSERT(canvas);
    ASSERT(scroll);
    if (!canvas || !scroll) return 0;

    scroll_w = MAX(w, 0);
    scroll_h = MAX(h, 2 * scroll_w);
    canvas->draw_rect(canvas->userdata, x, y, scroll_w, scroll_h, scroll->background);
    if (target <= scroll_h) return 0;

    button.border = 1;
    button.padding.x = scroll_w / 4;
    button.padding.y = scroll_w / 4;
    button.background = scroll->background;
    button.foreground =  scroll->foreground;
    button.content = scroll->foreground;
    button.highlight = scroll->background;
    button.highlight_content = scroll->foreground;

    button_up_pressed = gui_button_triangle(canvas, x, y, scroll_w, scroll_w,
         GUI_UP, GUI_BUTTON_DEFAULT, &button, in);
    button_down_pressed = gui_button_triangle(canvas, x, y + scroll_h - scroll_w,
        scroll_w, scroll_w, GUI_DOWN, GUI_BUTTON_DEFAULT, &button, in);

    scroll_h = scroll_h - 2 * scroll_w;
    scroll_y = y + scroll_w;
    scroll_step = MIN(step, scroll_h);
    scroll_offset = MIN(offset, target - scroll_h);
    scroll_ratio = scroll_h / target;
    scroll_off = scroll_offset / target;

    cursor_h = scroll_ratio * scroll_h;
    cursor_y = scroll_y + (scroll_off * scroll_h);
    cursor_w = scroll_w;
    cursor_x = x;

    if (in) {
        inscroll = INBOX(in->mouse_pos.x,in->mouse_pos.y, x, y, scroll_w, scroll_h);
        incursor = INBOX(in->mouse_prev.x,in->mouse_prev.y,cursor_x,cursor_y,cursor_w, cursor_h);
        if (in->mouse_down && inscroll && incursor) {
            const gui_float pixel = in->mouse_delta.y;
            const gui_float delta =  (pixel / scroll_h) * target;
            scroll_offset = CLAMP(0, scroll_offset + delta, target - scroll_h);
            cursor_y += pixel;
        } else if (button_up_pressed || button_down_pressed) {
            scroll_offset = (button_down_pressed) ?
                MIN(scroll_offset + scroll_step, target - scroll_h):
                MAX(0, scroll_offset - scroll_step);
            scroll_off = scroll_offset / target;
            cursor_y = scroll_y + (scroll_off * scroll_h);
        }
    }
    canvas->draw_rect(canvas->userdata,cursor_x,cursor_y,cursor_w,cursor_h,scroll->foreground);
    return scroll_offset;
}

void*
gui_buffer_push(struct gui_command_buffer* buffer,
    enum gui_command_type type, gui_size size)
{
    static const gui_size align = ALIGNOF(struct gui_command);
    struct gui_command *cmd;
    gui_size alignment;
    void *unaligned;
    void *tail;

    ASSERT(buffer);
    ASSERT(!(buffer->flags & GUI_BUFFER_LOCKED));
    if (!buffer || buffer->flags & GUI_BUFFER_LOCKED) return NULL;
    buffer->needed += size;

    unaligned = (gui_byte*)buffer->end + size;
    tail = ALIGN_PTR(unaligned, align);
    alignment = (gui_size)((gui_byte*)tail - (gui_byte*)unaligned);

    if ((buffer->allocated + size + alignment) >= buffer->capacity) {
        gui_size cap;
        if (!buffer->allocator.realloc) return NULL;
        cap = (gui_size)((gui_float)buffer->capacity * buffer->grow_factor);
        buffer->memory = buffer->allocator.realloc(buffer->allocator.userdata,buffer->memory, cap);
        if (!buffer->memory) return NULL;

        buffer->capacity = cap;
        buffer->begin = buffer->memory;
        buffer->end = (void*)((gui_byte*)buffer->begin + buffer->allocated);

        unaligned = (gui_byte*)buffer->end + size;
        tail = ALIGN_PTR(unaligned, align);
        alignment = (gui_size)((gui_byte*)tail - (gui_byte*)unaligned);
    }

    cmd = buffer->end;
    cmd->type = type;
    cmd->offset = size + alignment;
    cmd->offset = ALIGN(cmd->offset, align);

    buffer->end = tail;
    buffer->allocated += size + alignment;
    buffer->needed += alignment;
    buffer->count++;
    return cmd;
}

void
gui_buffer_push_scissor(struct gui_command_buffer *buffer, gui_float x, gui_float y,
    gui_float w, gui_float h)
{
    struct gui_command_scissor *cmd;
    ASSERT(buffer);
    ASSERT(!(buffer->flags & GUI_BUFFER_LOCKED));
    if (!buffer || buffer->flags & GUI_BUFFER_LOCKED) return;

    cmd = gui_buffer_push(buffer, GUI_COMMAND_SCISSOR, sizeof(*cmd));
    if (!cmd) return;

    buffer->clip.x = x;
    buffer->clip.y = y;
    buffer->clip.w = w;
    buffer->clip.h = h;

    cmd->x = (gui_short)x;
    cmd->y = (gui_short)y;
    cmd->w = (gui_ushort)w;
    cmd->h = (gui_ushort)h;
}

void
gui_buffer_push_line(struct gui_command_buffer *buffer, gui_float x0, gui_float y0,
    gui_float x1, gui_float y1, struct gui_color c)
{
    struct gui_command_line *cmd;
    ASSERT(buffer);
    ASSERT(!(buffer->flags & GUI_BUFFER_LOCKED));
    if (!buffer || buffer->flags & GUI_BUFFER_LOCKED) return;

    cmd = gui_buffer_push(buffer, GUI_COMMAND_LINE, sizeof(*cmd));
    if (!cmd) return;
    cmd->begin[0] = (gui_short)x0;
    cmd->begin[1] = (gui_short)y0;
    cmd->end[0] = (gui_short)x1;
    cmd->end[1] = (gui_short)y1;
    cmd->color = c;
}

void
gui_buffer_push_rect(struct gui_command_buffer *buffer, gui_float x, gui_float y,
    gui_float w, gui_float h, struct gui_color c)
{
    struct gui_command_rect *cmd;
    ASSERT(buffer);
    ASSERT(!(buffer->flags & GUI_BUFFER_LOCKED));
    if (!buffer || buffer->flags & GUI_BUFFER_LOCKED) return;

    if (buffer->flags & GUI_BUFFER_CLIPPING) {
        const struct gui_rect *r = &buffer->clip;
        if (!INTERSECT(r->x, r->y, r->w, r->h, x, y, w, h)) {
            buffer->clipped_memory += sizeof(*cmd);
            buffer->clipped_cmds++;
            return;
        }
    }

    cmd = gui_buffer_push(buffer, GUI_COMMAND_RECT, sizeof(*cmd));
    if (!cmd) return;
    cmd->x = (gui_short)x;
    cmd->y = (gui_short)y;
    cmd->w = (gui_ushort)w;
    cmd->h = (gui_ushort)h;
    cmd->color = c;
}

void
gui_buffer_push_circle(struct gui_command_buffer *buffer, gui_float x, gui_float y,
    gui_float w, gui_float h, struct gui_color c)
{
    struct gui_command_circle *cmd;
    ASSERT(buffer);
    ASSERT(!(buffer->flags & GUI_BUFFER_LOCKED));
    if (!buffer || buffer->flags & GUI_BUFFER_LOCKED) return;

    if (buffer->flags & GUI_BUFFER_CLIPPING) {
        const struct gui_rect *r = &buffer->clip;
        if (!INTERSECT(r->x, r->y, r->w, r->h, x, y, w, h)) {
            buffer->clipped_memory += sizeof(*cmd);
            buffer->clipped_cmds++;
            return;
        }
    }

    cmd = gui_buffer_push(buffer, GUI_COMMAND_CIRCLE, sizeof(*cmd));
    if (!cmd) return;
    cmd->x = (gui_short)x;
    cmd->y = (gui_short)y;
    cmd->w = (gui_ushort)w;
    cmd->h = (gui_ushort)h;
    cmd->color = c;
}

void
gui_buffer_push_triangle(struct gui_command_buffer *buffer, gui_float x0, gui_float y0,
    gui_float x1, gui_float y1, gui_float x2, gui_float y2, struct gui_color c)
{
    struct gui_command_triangle *cmd;
    ASSERT(buffer);
    ASSERT(!(buffer->flags & GUI_BUFFER_LOCKED));
    if (!buffer || buffer->flags & GUI_BUFFER_LOCKED) return;

    if (buffer->flags & GUI_BUFFER_CLIPPING) {
        const struct gui_rect *r = &buffer->clip;
        if (!INBOX(x0, y0, r->x, r->y, r->w, r->h) &&
            !INBOX(x1, y1, r->x, r->y, r->w, r->h) &&
            !INBOX(x2, y2, r->x, r->y, r->w, r->h)) {
            buffer->clipped_memory += sizeof(*cmd);
            buffer->clipped_cmds++;
            return;
        }
    }
    cmd = gui_buffer_push(buffer, GUI_COMMAND_TRIANGLE, sizeof(*cmd));
    if (!cmd) return;
    cmd->a[0] = (gui_short)x0;
    cmd->a[1] = (gui_short)y0;
    cmd->b[0] = (gui_short)x1;
    cmd->b[1] = (gui_short)y1;
    cmd->c[0] = (gui_short)x2;
    cmd->c[1] = (gui_short)y2;
    cmd->color = c;
}

void
gui_buffer_push_image(struct gui_command_buffer *buffer, gui_float x, gui_float y,
    gui_float w, gui_float h, gui_image img)
{
    struct gui_command_image *cmd;
    ASSERT(buffer);
    ASSERT(!(buffer->flags & GUI_BUFFER_LOCKED));
    if (!buffer || buffer->flags & GUI_BUFFER_LOCKED) return;

    if (buffer->flags == GUI_BUFFER_CLIPPING) {
        const struct gui_rect *r = &buffer->clip;
        if (!INTERSECT(r->x, r->y, r->w, r->h, x, y, w, h)) {
            buffer->clipped_memory += sizeof(*cmd);
            buffer->clipped_cmds++;
            return;
        }
    }

    cmd = gui_buffer_push(buffer, GUI_COMMAND_IMAGE, sizeof(*cmd));
    if (!cmd) return;
    cmd->x = (gui_short)x;
    cmd->y = (gui_short)y;
    cmd->w = (gui_ushort)w;
    cmd->h = (gui_ushort)h;
    cmd->img = img;
}

void
gui_buffer_push_text(struct gui_command_buffer *buffer, gui_float x, gui_float y,
    gui_float w, gui_float h, const gui_char *string, gui_size length,
    const struct gui_font *font, struct gui_color bg, struct gui_color fg)
{
    struct gui_command_text *cmd;
    ASSERT(buffer);
    ASSERT(font);
    ASSERT(!(buffer->flags & GUI_BUFFER_LOCKED));
    if (!buffer || !string || !length) return;
    if (buffer->flags & GUI_BUFFER_LOCKED) return;

    if (buffer->flags & GUI_BUFFER_CLIPPING) {
        const struct gui_rect *r = &buffer->clip;
        if (!INTERSECT(r->x, r->y, r->w, r->h, x, y, w, h)) {
            buffer->clipped_memory += sizeof(*cmd);
            buffer->clipped_cmds++;
            return;
        }
    }

    cmd = gui_buffer_push(buffer, GUI_COMMAND_TEXT, sizeof(*cmd) + length + 1);
    if (!cmd) return;
    cmd->x = (gui_short)x;
    cmd->y = (gui_short)y;
    cmd->w = (gui_ushort)w;
    cmd->h = (gui_ushort)h;
    cmd->bg = bg;
    cmd->fg = fg;
    cmd->font = font->userdata;
    cmd->length = length;
    memcopy(cmd->string, string, length);
    cmd->string[length] = '\0';
}

void
gui_buffer_init_fixed(struct gui_command_buffer *buffer, const struct gui_memory *memory,
    gui_flags clipping)
{
    ASSERT(buffer);
    ASSERT(memory);
    if (!buffer || !memory) return;

    zero(buffer, sizeof(*buffer));
    buffer->flags = GUI_BUFFER_OWNER;
    if (clipping) buffer->flags |= GUI_BUFFER_CLIPPING;
    buffer->memory = memory->memory;
    buffer->capacity = memory->size;
    buffer->begin = buffer->memory;
    buffer->end = buffer->begin;
    buffer->clip = null_rect;
    buffer->grow_factor = 0;
}

void
gui_buffer_init(struct gui_command_buffer *buffer, const struct gui_allocator *memory,
    gui_size initial_size, gui_float grow_factor, gui_flags clipping)
{
    ASSERT(buffer);
    ASSERT(memory);
    ASSERT(initial_size);
    if (!buffer || !memory || !initial_size) return;

    zero(buffer, sizeof(*buffer));
    buffer->flags = GUI_BUFFER_OWNER;
    if (clipping) buffer->flags |= GUI_BUFFER_CLIPPING;
    buffer->memory = memory->alloc(memory->userdata, initial_size);
    buffer->allocator = *memory;
    buffer->capacity = initial_size;
    buffer->begin = buffer->memory;
    buffer->end = buffer->begin;
    buffer->grow_factor = grow_factor;
    buffer->clip = null_rect;
}

void
gui_buffer_lock(struct gui_canvas *canvas, struct gui_command_buffer *buffer,
    struct gui_command_buffer *sub, gui_flags clipping, gui_size width, gui_size height)
{
    static const gui_size align = ALIGNOF(struct gui_command);
    ASSERT(buffer);
    ASSERT(sub);
    ASSERT(!(buffer->flags & GUI_BUFFER_LOCKED));
    if (!buffer || !sub || (buffer->flags & GUI_BUFFER_LOCKED)) return;

    buffer->flags |= GUI_BUFFER_LOCKED;
    sub->flags = GUI_BUFFER_DEFAULT;
    if (clipping) sub->flags |= GUI_BUFFER_CLIPPING;
    sub->memory = buffer->memory;
    sub->allocator = buffer->allocator;
    sub->begin = buffer->end;
    sub->end = sub->begin;
    sub->grow_factor = buffer->grow_factor;
    sub->allocated = buffer->allocated;
    sub->capacity = buffer->capacity;
    sub->sub_size = sub->allocated;
    sub->sub_cap = sub->capacity;
    sub->needed = 0;
    sub->count = 0;
    gui_buffer_begin(canvas, sub, width, height);
}

void
gui_buffer_unlock(struct gui_command_list *list, struct gui_command_buffer *buffer,
    struct gui_command_buffer *sub, struct gui_canvas *canvas,
    struct gui_memory_status *status)
{
    ASSERT(buffer);
    ASSERT(sub);
    if (!buffer || !sub) return;

    buffer->flags &= (gui_flags)~GUI_BUFFER_LOCKED;
    buffer->end = sub->end;
    buffer->allocated = sub->allocated;
    buffer->capacity = sub->capacity;
    buffer->count += sub->count;
    buffer->needed += sub->needed;
    sub->flags = GUI_BUFFER_LOCKED;
    gui_buffer_end(list, sub, canvas, status);
    zero(sub, sizeof(*sub));
}

void
gui_buffer_begin(struct gui_canvas *canvas, struct gui_command_buffer *buffer,
    gui_size width, gui_size height)
{
    ASSERT(buffer);
    ASSERT(buffer->memory);
    ASSERT(buffer->begin == buffer->end);
    if (!canvas || !buffer) return;

    canvas->userdata = buffer;
    canvas->width = width;
    canvas->height = height;
    canvas->scissor = (gui_scissor)gui_buffer_push_scissor;
    canvas->draw_line = (gui_draw_line)gui_buffer_push_line;
    canvas->draw_rect = (gui_draw_rect)gui_buffer_push_rect;
    canvas->draw_circle = (gui_draw_circle)gui_buffer_push_circle;
    canvas->draw_triangle = (gui_draw_triangle)gui_buffer_push_triangle;
    canvas->draw_text = (gui_draw_text)gui_buffer_push_text;
}

void
gui_buffer_end(struct gui_command_list *list, struct gui_command_buffer *buffer,
    struct gui_canvas *canvas, struct gui_memory_status *status)
{
    ASSERT(buffer);
    if (!buffer) return;
    if (status) {
        status->allocated = (!(buffer->flags & GUI_BUFFER_OWNER)) ?
            buffer->allocated - buffer->sub_size: buffer->allocated;
        status->size = (!(buffer->flags & GUI_BUFFER_OWNER)) ?
            buffer->capacity - buffer->sub_cap: buffer->capacity;
        status->needed = buffer->needed;
        status->clipped_commands = buffer->clipped_cmds;
        status->clipped_memory = buffer->clipped_memory;
    }

    if (list) {
        list->count = buffer->count;
        list->begin = buffer->begin;
        list->end = buffer->end;
    }

    buffer->begin = buffer->memory;
    buffer->end = buffer->begin;
    buffer->needed = 0;
    buffer->allocated = 0;
    buffer->count = 0;
    buffer->clip = null_rect;
    buffer->clipped_cmds = 0;
    buffer->clipped_memory = 0;
    if (canvas) zero(canvas, sizeof(*canvas));
}

void
gui_buffer_clear(struct gui_command_buffer *buffer)
{
    ASSERT(buffer);
    if (!buffer || !buffer->memory || !buffer->allocator.free) return;
    if (!(buffer->flags & GUI_BUFFER_OWNER)) return;
    if (buffer->flags & GUI_BUFFER_LOCKED) return;
    buffer->allocator.free(buffer->allocator.userdata, buffer->memory);
}

const struct gui_command*
gui_list_begin(const struct gui_command_list *list)
{
    const struct gui_command *cmd;
    ASSERT(list);
    if (!list || !list->count) return NULL;
    cmd = list->begin;
    return cmd;
}

const struct gui_command*
gui_list_next(const struct gui_command_list *list, const struct gui_command *cmd)
{
    ASSERT(list);
    ASSERT(cmd);
    if (!list || !list->count || !cmd) return NULL;
    cmd = (const void*)((const gui_byte*)cmd + cmd->offset);
    if (cmd >= list->end) return NULL;
    return cmd;
}

void
gui_default_config(struct gui_config *config)
{
    ASSERT(config);
    if (!config) return;
    config->scrollbar_width = 16;
    vec2_load(config->panel_padding, 15.0f, 10.0f);
    vec2_load(config->panel_min_size, 64.0f, 64.0f);
    vec2_load(config->item_spacing, 10.0f, 4.0f);
    vec2_load(config->item_padding, 4.0f, 4.0f);
    vec2_load(config->scaler_size, 16.0f, 16.0f);
    col_load(config->colors[GUI_COLOR_TEXT], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_PANEL], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_HEADER], 76, 88, 68, 255);
    col_load(config->colors[GUI_COLOR_HEADER], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_BORDER], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_BUTTON], 50, 50, 50, 255);
    col_load(config->colors[GUI_COLOR_BUTTON_HOVER], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_BUTTON_TOGGLE], 75, 75, 75, 255);
    col_load(config->colors[GUI_COLOR_BUTTON_HOVER_FONT], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_BUTTON_BORDER], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_CHECK], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_CHECK_BACKGROUND], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_CHECK_ACTIVE], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_OPTION], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_OPTION_BACKGROUND], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_OPTION_ACTIVE], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_SLIDER], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_SLIDER_CURSOR], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_PROGRESS], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_PROGRESS_CURSOR], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_INPUT], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_INPUT_CURSOR], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_INPUT_BORDER], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_SPINNER], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_SPINNER_BORDER], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_SELECTOR], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_SELECTOR_BORDER], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_HISTO], 120, 120, 120, 255);
    col_load(config->colors[GUI_COLOR_HISTO_BARS], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_HISTO_NEGATIVE], 255, 255, 255, 255);
    col_load(config->colors[GUI_COLOR_HISTO_HIGHLIGHT], 255, 0, 0, 255);
    col_load(config->colors[GUI_COLOR_PLOT], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_PLOT_LINES], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_PLOT_HIGHLIGHT], 255, 0, 0, 255);
    col_load(config->colors[GUI_COLOR_SCROLLBAR], 40, 40, 40, 255);
    col_load(config->colors[GUI_COLOR_SCROLLBAR_CURSOR], 70, 70, 70, 255);
    col_load(config->colors[GUI_COLOR_SCROLLBAR_BORDER], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_TABLE_LINES], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_SCALER], 100, 100, 100, 255);
}

void
gui_panel_init(struct gui_panel *panel, gui_float x, gui_float y, gui_float w,
    gui_float h, gui_flags flags, const struct gui_config *config, const struct gui_font *font)
{
    ASSERT(panel);
    ASSERT(config);
    ASSERT(font);
    if (!panel || !config || !font)
        return;

    panel->x = x;
    panel->y = y;
    panel->w = w;
    panel->h = h;
    panel->flags = flags;
    panel->config = config;
    panel->font = *font;
    panel->offset = 0;
    panel->minimized = gui_false;
}

gui_bool
gui_panel_begin(struct gui_panel_layout *layout, struct gui_panel *panel,
    const char *text, const struct gui_canvas *canvas, const struct gui_input *in)
{
    const struct gui_config *config;
    const struct gui_color *header;
    gui_float mouse_x, mouse_y;
    gui_float prev_x, prev_y;
    gui_float clicked_x, clicked_y;
    gui_float header_x = 0, header_w = 0;
    gui_float footer_h;
    gui_bool ret = gui_true;

    ASSERT(panel);
    ASSERT(layout);
    ASSERT(canvas);

    if (!panel || !canvas || !layout)
        return gui_false;
    if (panel->flags & GUI_PANEL_HIDDEN) {
        zero(layout, sizeof(*layout));
        layout->valid = gui_false;
        layout->config = panel->config;
        layout->canvas = canvas;
        return gui_false;
    }

    config = panel->config;
    layout->header_height = panel->font.height + 3 * config->item_padding.y;
    layout->header_height += config->panel_padding.y;

    mouse_x = (in) ? in->mouse_pos.x : -1;
    mouse_y = (in) ? in->mouse_pos.y : -1;
    prev_x = (in) ? in->mouse_prev.x : -1;
    prev_y = (in) ? in->mouse_prev.y : -1;
    clicked_x = (in) ? in->mouse_clicked_pos.x : -1;
    clicked_y = (in) ? in->mouse_clicked_pos.y : -1;

    if (panel->flags & GUI_PANEL_MOVEABLE) {
        gui_bool incursor;
        const gui_float move_x = panel->x;
        const gui_float move_y = panel->y;
        const gui_float move_w = panel->w;
        const gui_float move_h = layout->header_height;

        incursor = in && INBOX(in->mouse_prev.x,in->mouse_prev.y, move_x, move_y, move_w, move_h);
        if (in && in->mouse_down && incursor) {
            panel->x = CLAMP(0, panel->x+in->mouse_delta.x, (gui_float)canvas->width-panel->w);
            panel->y = CLAMP(0, panel->y+in->mouse_delta.y, (gui_float)canvas->height-panel->h);
        }
    }

    if (panel->flags & GUI_PANEL_SCALEABLE) {
        gui_bool incursor;
        gui_float scaler_w = MAX(0, config->scaler_size.x - config->item_padding.x);
        gui_float scaler_h = MAX(0, config->scaler_size.y - config->item_padding.y);
        gui_float scaler_x = (panel->x + panel->w) - (config->item_padding.x + scaler_w);
        gui_float scaler_y = panel->y + panel->h - config->scaler_size.y;

        incursor = in && INBOX(prev_x, prev_y, scaler_x, scaler_y, scaler_w, scaler_h);
        if (in && in->mouse_down && incursor) {
            gui_float min_x = config->panel_min_size.x;
            gui_float min_y = config->panel_min_size.y;
            panel->w = CLAMP(min_x, panel->w+in->mouse_delta.x, (gui_float)canvas->width-panel->x);
            panel->h = CLAMP(min_y,panel->h+in->mouse_delta.y,(gui_float)canvas->height-panel->y);
        }
    }

    layout->x = panel->x;
    layout->y = panel->y;
    layout->w = panel->w;
    layout->h = panel->h;
    layout->at_x = panel->x;
    layout->at_y = panel->y;
    layout->width = panel->w;
    layout->height = panel->h;
    layout->font = panel->font;
    layout->config = panel->config;
    layout->canvas = canvas;
    layout->input = in;
    layout->index = 0;
    layout->row_columns = 0;
    layout->row_height = 0;
    layout->offset = panel->offset;

    if (!(panel->flags & GUI_PANEL_NO_HEADER)) {
        header = &config->colors[GUI_COLOR_HEADER];
        header_x = panel->x + config->panel_padding.x;
        header_w = panel->w - 2 * config->panel_padding.x;
        canvas->draw_rect(canvas->userdata, panel->x, panel->y, panel->w,
            layout->header_height, *header);
    } else layout->header_height = 1;
    layout->row_height = layout->header_height + 2 * config->item_spacing.y;

    footer_h = config->scaler_size.y + config->item_padding.y;
    if ((panel->flags & GUI_PANEL_SCROLLBAR) && !panel->minimized) {
        gui_float footer_x, footer_y, footer_w;
        footer_x = panel->x;
        footer_w = panel->w;
        footer_y = panel->y + panel->h - footer_h;
        canvas->draw_rect(canvas->userdata, footer_x, footer_y, footer_w, footer_h,
            config->colors[GUI_COLOR_PANEL]);
    }

    if (!(panel->flags & GUI_PANEL_TAB)) {
        panel->flags |= GUI_PANEL_SCROLLBAR;
        if (in && in->mouse_down) {
            if (!INBOX(clicked_x, clicked_y, panel->x, panel->y, panel->w, panel->h))
                panel->flags &= (gui_flag)~GUI_PANEL_ACTIVE;
            else panel->flags |= GUI_PANEL_ACTIVE;
        }
    }

    layout->clip.x = panel->x;
    layout->clip.w = panel->w;
    layout->clip.y = panel->y + layout->header_height - 1;
    if (panel->flags & GUI_PANEL_SCROLLBAR) {
        layout->clip.h = panel->h - (footer_h + layout->header_height);
        layout->clip.h -= (config->panel_padding.y + config->item_padding.y);
    }
    else layout->clip.h = null_rect.h;

    if ((panel->flags & GUI_PANEL_CLOSEABLE) && (!(panel->flags & GUI_PANEL_NO_HEADER))) {
        const gui_char *X = (const gui_char*)"x";
        const gui_size text_width = panel->font.width(panel->font.userdata, X, 1);
        const gui_float close_x = header_x;
        const gui_float close_y = panel->y + config->panel_padding.y;
        const gui_float close_w = (gui_float)text_width + 2 * config->item_padding.x;
        const gui_float close_h = panel->font.height + 2 * config->item_padding.y;
        canvas->draw_text(canvas->userdata, close_x, close_y, close_w, close_h,
            X, 1, &panel->font, config->colors[GUI_COLOR_HEADER], config->colors[GUI_COLOR_TEXT]);

        header_w -= close_w;
        header_x += close_h - config->item_padding.x;
        if (in && INBOX(mouse_x, mouse_y, close_x, close_y, close_w, close_h)) {
            if (INBOX(clicked_x, clicked_y, close_x, close_y, close_w, close_h)) {
                ret = !(in->mouse_down && in->mouse_clicked);
                if (!ret) panel->flags |= GUI_PANEL_HIDDEN;
            }
        }
    }

    if ((panel->flags & GUI_PANEL_MINIMIZABLE) && (!(panel->flags & GUI_PANEL_NO_HEADER))) {
        gui_size text_width;
        gui_float min_x, min_y, min_w, min_h;
        const gui_char *score = (panel->minimized) ?
            (const gui_char*)"+":
            (const gui_char*)"-";

        text_width = panel->font.width(panel->font.userdata, score, 1);
        min_x = header_x;
        min_y = panel->y + config->panel_padding.y;
        min_w = (gui_float)text_width + 3 * config->item_padding.x;
        min_h = panel->font.height + 2 * config->item_padding.y;
        canvas->draw_text(canvas->userdata, min_x, min_y, min_w, min_h,
            score, 1, &panel->font, config->colors[GUI_COLOR_HEADER],
            config->colors[GUI_COLOR_TEXT]);

        header_w -= min_w;
        header_x += min_w - config->item_padding.x;
        if (in && INBOX(mouse_x, mouse_y, min_x, min_y, min_w, min_h)) {
            if (INBOX(clicked_x, clicked_y, min_x, min_y, min_w, min_h))
                if (in->mouse_down && in->mouse_clicked)
                    panel->minimized = !panel->minimized;
        }
    }
    layout->valid = !(panel->minimized || (panel->flags & GUI_PANEL_HIDDEN));

    if (text && !(panel->flags & GUI_PANEL_NO_HEADER)) {
        const gui_size text_len = strsiz(text);
        const gui_float label_x = header_x + config->item_padding.x;
        const gui_float label_y = panel->y + config->panel_padding.y;
        const gui_float label_w = header_w - (3 * config->item_padding.x);
        const gui_float label_h = panel->font.height + 2 * config->item_padding.y;
        canvas->draw_text(canvas->userdata, label_x, label_y, label_w, label_h,
            (const gui_char*)text, text_len, &panel->font, config->colors[GUI_COLOR_HEADER],
            config->colors[GUI_COLOR_TEXT]);
    }

    if (panel->flags & GUI_PANEL_SCROLLBAR) {
        const struct gui_color *color = &config->colors[GUI_COLOR_PANEL];
        layout->width = panel->w - config->scrollbar_width;
        layout->height = panel->h - (layout->header_height + 2 * config->item_spacing.y);
        layout->height -= footer_h;
        if (layout->valid)
            canvas->draw_rect(canvas->userdata, panel->x, panel->y + layout->header_height,
                panel->w, panel->h - layout->header_height, *color);
    }

    if (panel->flags & GUI_PANEL_BORDER) {
        const struct gui_color *color = &config->colors[GUI_COLOR_BORDER];
        const gui_float width = (panel->flags & GUI_PANEL_SCROLLBAR) ?
                layout->width + config->scrollbar_width : layout->width;

        canvas->draw_line(canvas->userdata, panel->x, panel->y,
                panel->x + panel->w, panel->y, *color);
        canvas->draw_line(canvas->userdata, panel->x, panel->y, panel->x,
                panel->y + layout->header_height, config->colors[GUI_COLOR_BORDER]);
        canvas->draw_line(canvas->userdata, panel->x + width, panel->y, panel->x + width,
                panel->y + layout->header_height, config->colors[GUI_COLOR_BORDER]);
    }
    canvas->scissor(canvas->userdata,layout->clip.x,layout->clip.y,layout->clip.w,layout->clip.h);
    return ret;
}

gui_bool
gui_panel_begin_stacked(struct gui_panel_layout *layout, struct gui_panel *panel,
    struct gui_panel_stack *stack, const char *title, const struct gui_canvas *canvas,
    const struct gui_input *in)
{
    gui_bool inpanel;
    struct gui_panel_hook *hook;
    ASSERT(layout);
    ASSERT(panel);
    ASSERT(stack);
    ASSERT(canvas);
    if (!layout || !panel || !stack || !canvas)
        return gui_false;

    hook = (struct gui_panel_hook*)panel;
    inpanel = INBOX(in->mouse_prev.x, in->mouse_prev.y, panel->x, panel->y, panel->w, panel->h);
    if (in->mouse_down && in->mouse_clicked && inpanel && hook != stack->end) {
        const struct gui_panel_hook *iter = hook->next;
        while (iter) {
            const struct gui_panel *cur = gui_hook_panel(iter);
            if (INBOX(in->mouse_prev.x, in->mouse_prev.y, cur->x, cur->y, cur->w, cur->h) &&
              !cur->minimized) break;
            iter = iter->next;
        }
        if (!iter) {
            gui_stack_pop(stack, hook);
            gui_stack_push(stack, hook);
        }
    }
    return gui_panel_begin(layout, panel, title, canvas, (stack->end == hook) ? in : NULL);
}

void
gui_panel_row(struct gui_panel_layout *layout, gui_float height, gui_size cols)
{
    const struct gui_config *config;
    const struct gui_color *color;

    ASSERT(layout);
    if (!layout) return;
    if (!layout->valid) return;

    ASSERT(layout->config);
    ASSERT(layout->canvas);
    config = layout->config;
    color = &config->colors[GUI_COLOR_PANEL];

    layout->index = 0;
    layout->at_y += layout->row_height;
    layout->row_columns = cols;
    layout->row_height = height + config->item_spacing.y;
    layout->canvas->draw_rect(layout->canvas->userdata, layout->at_x, layout->at_y,
        layout->width, height + config->panel_padding.y, *color);
}

void
gui_panel_seperator(struct gui_panel_layout *layout, gui_size cols)
{
    const struct gui_config *config;
    ASSERT(layout);
    ASSERT(layout->config);
    ASSERT(layout->canvas);
    if (!layout) return;
    if (!layout->valid) return;

    config = layout->config;
    cols = MIN(cols, layout->row_columns - layout->index);
    layout->index += cols;
    if (layout->index >= layout->row_columns) {
        const gui_float row_height = layout->row_height - config->item_spacing.y;
        gui_panel_row(layout, row_height, layout->row_columns);
    }
}

void
gui_panel_alloc_space(struct gui_rect *bounds, struct gui_panel_layout *layout)
{
    const struct gui_config *config;
    gui_float panel_padding, panel_spacing, panel_space;
    gui_float item_offset, item_width, item_spacing;

    ASSERT(layout);
    ASSERT(layout->config);
    ASSERT(bounds);
    if (!layout || !layout->config || !bounds)
        return;

    config = layout->config;
    if (layout->index >= layout->row_columns) {
        const gui_float row_height = layout->row_height - config->item_spacing.y;
        gui_panel_row(layout, row_height, layout->row_columns);
    }

    panel_padding = 2 * config->panel_padding.x;
    panel_spacing = (gui_float)(layout->row_columns - 1) * config->item_spacing.x;
    panel_space  = layout->width - panel_padding - panel_spacing;

    item_width = panel_space / (gui_float)layout->row_columns;
    item_offset = (gui_float)layout->index * item_width;
    item_spacing = (gui_float)layout->index * config->item_spacing.x;

    bounds->x = layout->at_x + item_offset + item_spacing + config->panel_padding.x;
    bounds->y = layout->at_y - layout->offset;
    bounds->w = item_width;
    bounds->h = layout->row_height - config->item_spacing.y;
    layout->index++;
}

static gui_bool
gui_panel_widget(struct gui_rect *bounds, struct gui_panel_layout *layout)
{
    struct gui_rect *c;
    ASSERT(layout);
    ASSERT(layout->config);
    ASSERT(layout->canvas);
    if (!layout || !layout->config || !layout->canvas) return gui_false;
    if (!layout->valid) return gui_false;

    gui_panel_alloc_space(bounds, layout);
    c = &layout->clip;
    if (!INTERSECT(c->x, c->y, c->w, c->h, bounds->x, bounds->y, bounds->w, bounds->h))
        return gui_false;
    return gui_true;
}

void
gui_panel_text_colored(struct gui_panel_layout *layout, const char *str, gui_size len,
    enum gui_text_align alignment, struct gui_color color)
{
    struct gui_rect bounds;
    struct gui_text text;
    const struct gui_config *config;

    ASSERT(layout);
    ASSERT(layout->config);
    ASSERT(layout->canvas);
    ASSERT(str && len);

    if (!layout || !layout->config || !layout->canvas) return;
    if (!layout->valid) return;
    gui_panel_alloc_space(&bounds, layout);
    config = layout->config;

    text.padding.x = config->item_padding.x;
    text.padding.y = config->item_padding.y;
    text.foreground = color;
    text.background = config->colors[GUI_COLOR_PANEL];
    gui_text(layout->canvas,bounds.x,bounds.y,bounds.w,bounds.h, str, len,
        &text, alignment, &layout->font);
}

void
gui_panel_text(struct gui_panel_layout *layout, const char *str, gui_size len,
    enum gui_text_align alignment)
{
    gui_panel_text_colored(layout, str, len, alignment, layout->config->colors[GUI_COLOR_TEXT]);
}

void
gui_panel_label_colored(struct gui_panel_layout *layout, const char *text, enum gui_text_align align,
    struct gui_color color)
{
    gui_size len = strsiz(text);
    gui_panel_text_colored(layout, text, len, align, color);
}

void
gui_panel_label(struct gui_panel_layout *layout, const char *text, enum gui_text_align align)
{
    gui_size len = strsiz(text);
    gui_panel_text(layout, text, len, align);
}

static gui_bool
gui_panel_button(struct gui_button *button, struct gui_rect *bounds,
    struct gui_panel_layout *layout)
{
    const struct gui_config *config;
    if (!gui_panel_widget(bounds, layout)) return gui_false;
    config = layout->config;
    button->border = 1;
    button->padding.x = config->item_padding.x;
    button->padding.y = config->item_padding.y;
    return gui_true;
}

gui_bool
gui_panel_button_text(struct gui_panel_layout *layout, const char *str,
    enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;
    if (!gui_panel_button(&button, &bounds, layout))
        return gui_false;

    config = layout->config;
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
    button.content = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
    button.highlight_content = config->colors[GUI_COLOR_BUTTON_HOVER_FONT];
    return gui_button_text(layout->canvas, bounds.x, bounds.y, bounds.w, bounds.h,
            str, behavior, &button, layout->input, &layout->font);
}

gui_bool gui_panel_button_color(struct gui_panel_layout *layout,
    const struct gui_color color, enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button button;
    if (!gui_panel_button(&button, &bounds, layout))
        return gui_false;

    button.background = color;
    button.foreground = color;
    button.highlight = color;
    button.highlight_content = color;
    return gui_do_button(layout->canvas, bounds.x, bounds.y, bounds.w, bounds.h,
                &button, layout->input, behavior);
}

gui_bool
gui_panel_button_triangle(struct gui_panel_layout *layout, enum gui_heading heading,
    enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;
    if (!gui_panel_button(&button, &bounds, layout))
        return gui_false;

    config = layout->config;
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
    button.content = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
    button.highlight_content = config->colors[GUI_COLOR_BUTTON_HOVER_FONT];
    return gui_button_triangle(layout->canvas, bounds.x, bounds.y, bounds.w,
            bounds.h, heading, behavior, &button, layout->input);
}

gui_bool
gui_panel_button_image(struct gui_panel_layout *layout, gui_image image,
    enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;
    if (!gui_panel_button(&button, &bounds, layout))
        return gui_false;

    config = layout->config;
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
    button.content = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
    button.highlight_content = config->colors[GUI_COLOR_BUTTON_HOVER_FONT];
    return gui_button_image(layout->canvas, bounds.x, bounds.y, bounds.w,
            bounds.h, image, behavior, &button, layout->input);
}

gui_bool
gui_panel_button_toggle(struct gui_panel_layout *layout, const char *str, gui_bool value)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;
    ASSERT(str);
    if (!gui_panel_button(&button, &bounds, layout))
        return gui_false;

    config = layout->config;
    if (!value) {
        button.background = config->colors[GUI_COLOR_BUTTON];
        button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
        button.content = config->colors[GUI_COLOR_TEXT];
        button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
        button.highlight_content = config->colors[GUI_COLOR_BUTTON];
    } else {
        button.background = config->colors[GUI_COLOR_BUTTON_TOGGLE];
        button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
        button.content = config->colors[GUI_COLOR_TEXT];
        button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
        button.highlight_content = config->colors[GUI_COLOR_BUTTON];
    }
    if (gui_button_text(layout->canvas, bounds.x, bounds.y, bounds.w, bounds.h,
            str, GUI_BUTTON_DEFAULT, &button, layout->input, &layout->font)) value = !value;
    return value;
}

static gui_bool
gui_panel_toggle_base(struct gui_toggle *toggle, struct gui_rect *bounds,
    struct gui_panel_layout *layout)
{
    const struct gui_config *config;
    if (!gui_panel_widget(bounds, layout))
        return gui_false;

    config = layout->config;
    toggle->padding.x = config->item_padding.x;
    toggle->padding.y = config->item_padding.y;
    toggle->font = config->colors[GUI_COLOR_TEXT];
    return gui_true;
}

gui_bool
gui_panel_check(struct gui_panel_layout *layout, const char *text, gui_bool is_active)
{
    struct gui_rect bounds;
    struct gui_toggle toggle;
    const struct gui_config *config;
    if (!text || !gui_panel_toggle_base(&toggle, &bounds, layout))
        return is_active;

    config = layout->config;
    toggle.cursor = config->colors[GUI_COLOR_CHECK_ACTIVE];
    toggle.background = config->colors[GUI_COLOR_CHECK_BACKGROUND];
    toggle.foreground = config->colors[GUI_COLOR_CHECK];
    return gui_toggle(layout->canvas, bounds.x, bounds.y, bounds.w, bounds.h,
                is_active, text, GUI_TOGGLE_CHECK, &toggle, layout->input, &layout->font);
}

gui_bool
gui_panel_option(struct gui_panel_layout *layout, const char *text, gui_bool is_active)
{
    struct gui_rect bounds;
    struct gui_toggle toggle;
    const struct gui_config *config;
    if (!text || !gui_panel_toggle_base(&toggle, &bounds, layout))
        return is_active;

    config = layout->config;
    toggle.cursor = config->colors[GUI_COLOR_OPTION_ACTIVE];
    toggle.background = config->colors[GUI_COLOR_OPTION_BACKGROUND];
    toggle.foreground = config->colors[GUI_COLOR_OPTION];
    return gui_toggle(layout->canvas, bounds.x, bounds.y, bounds.w, bounds.h,
        is_active, text, GUI_TOGGLE_OPTION, &toggle, layout->input, &layout->font);
}

gui_size
gui_panel_option_group(struct gui_panel_layout *layout, const char **options,
    gui_size count, gui_size current)
{
    gui_size i;
    ASSERT(layout && options && count);
    if (!layout || !options || !count) return current;
    for (i = 0; i < count; ++i) {
        if (gui_panel_option(layout, options[i], i == current))
            current = i;
    }
    return current;
}

gui_float
gui_panel_slider(struct gui_panel_layout *layout, gui_float min_value, gui_float value,
    gui_float max_value, gui_float value_step)
{
    struct gui_rect bounds;
    struct gui_slider slider;
    const struct gui_config *config;
    if (!gui_panel_widget(&bounds, layout))
        return value;

    config = layout->config;
    slider.padding.x = config->item_padding.x;
    slider.padding.y = config->item_padding.y;
    slider.background = config->colors[GUI_COLOR_SLIDER];
    slider.foreground = config->colors[GUI_COLOR_SLIDER_CURSOR];
    return gui_slider(layout->canvas, bounds.x, bounds.y, bounds.w, bounds.h,
                min_value, value, max_value, value_step, &slider, layout->input);
}

gui_size
gui_panel_progress(struct gui_panel_layout *layout, gui_size cur_value, gui_size max_value,
    gui_bool is_modifyable)
{
    struct gui_rect bounds;
    struct gui_slider prog;
    const struct gui_config *config;
    if (!gui_panel_widget(&bounds, layout))
        return cur_value;

    config = layout->config;
    prog.padding.x = config->item_padding.x;
    prog.padding.y = config->item_padding.y;
    prog.background = config->colors[GUI_COLOR_PROGRESS];
    prog.foreground = config->colors[GUI_COLOR_PROGRESS_CURSOR];
    return gui_progress(layout->canvas, bounds.x, bounds.y, bounds.w, bounds.h,
            cur_value, max_value, is_modifyable, &prog, layout->input);
}

static gui_bool
gui_panel_edit_base(struct gui_rect *bounds, struct gui_edit *field,
    struct gui_panel_layout *layout)
{
    const struct gui_config *config;
    if (!gui_panel_widget(bounds, layout))
        return gui_false;

    config = layout->config;
    field->padding.x = config->item_padding.x;
    field->padding.y = config->item_padding.y;
    field->show_cursor = gui_true;
    field->background = config->colors[GUI_COLOR_INPUT];
    field->foreground = config->colors[GUI_COLOR_INPUT_BORDER];
    return gui_true;
}

gui_size
gui_panel_edit(struct gui_panel_layout *layout, gui_char *buffer, gui_size len,
    gui_size max, gui_bool *active, enum gui_input_filter filter)
{
    struct gui_rect bounds;
    struct gui_edit field;
    if (!gui_panel_edit_base(&bounds, &field, layout)) return len;
    return gui_edit(layout->canvas, bounds.x, bounds.y, bounds.w, bounds.h,
                    buffer, len, max, active, &field, filter, layout->input, &layout->font);
}

gui_size
gui_panel_edit_filtered(struct gui_panel_layout *layout, gui_char *buffer, gui_size len,
    gui_size max, gui_bool *active, gui_filter filter)
{
    struct gui_rect bounds;
    struct gui_edit field;
    if (!gui_panel_edit_base(&bounds, &field, layout)) return len;
    return gui_edit_filtered(layout->canvas, bounds.x, bounds.y, bounds.w, bounds.h,
                    buffer, len, max, active, &field, filter, layout->input, &layout->font);
}

gui_bool
gui_panel_shell(struct gui_panel_layout *layout, gui_char *buffer, gui_size *len,
    gui_size max, gui_bool *active)
{
    struct gui_rect bounds;
    struct gui_edit field;
    struct gui_button button;
    gui_float button_x, button_y;
    gui_float button_w, button_h;
    gui_float field_x, field_y;
    gui_float field_w, field_h;
    const struct gui_config *config;
    gui_bool clicked = gui_false;
    gui_size width;
    if (!gui_panel_widget(&bounds, layout))
        return *active;

    config = layout->config;
    width = layout->font.width(layout->font.userdata, (const gui_char*)"submit", 6);
    button.border = 1;
    button_y = bounds.y;
    button_h = bounds.h;
    button_w = (gui_float)width + config->item_padding.x * 2;
    button_w += button.border * 2;
    button_x = bounds.x + bounds.w - button_w;
    button.padding.x = config->item_padding.x;
    button.padding.y = config->item_padding.y;
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
    button.content = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
    button.highlight_content = config->colors[GUI_COLOR_BUTTON_HOVER_FONT];
    clicked = gui_button_text(layout->canvas, button_x, button_y, button_w, button_h,
                "submit", GUI_BUTTON_DEFAULT, &button, layout->input, &layout->font);

    field_x = bounds.x;
    field_y = bounds.y;
    field_w = bounds.w - button_w - config->item_spacing.x;
    field_h = bounds.h;
    field.padding.x = config->item_padding.x;
    field.padding.y = config->item_padding.y;
    field.show_cursor = gui_true;
    field.background = config->colors[GUI_COLOR_INPUT];
    field.foreground = config->colors[GUI_COLOR_INPUT_BORDER];
    *len = gui_edit(layout->canvas, field_x, field_y, field_w, field_h, buffer,
            *len, max, active, &field, GUI_INPUT_DEFAULT, layout->input, &layout->font);
    return clicked;
}

gui_int
gui_panel_spinner(struct gui_panel_layout *layout, gui_int min, gui_int value,
    gui_int max, gui_int step, gui_bool *active)
{
    struct gui_rect bounds;
    const struct gui_config *config;
    const struct gui_canvas *canvas;
    struct gui_edit field;
    char string[MAX_NUMBER_BUFFER];
    gui_size len, old_len;

    struct gui_button button;
    gui_float button_x, button_y;
    gui_float button_w, button_h;
    gui_float field_x, field_y;
    gui_float field_w, field_h;
    gui_bool is_active, updated = gui_false;
    gui_bool button_up_clicked, button_down_clicked;
    if (!gui_panel_widget(&bounds, layout))
        return value;

    value = CLAMP(min, value, max);
    len = itos(string, value);
    is_active = (active) ? *active : gui_false;
    old_len = len;

    config = layout->config;
    canvas = layout->canvas;

    button.border = 1;
    button_y = bounds.y;
    button_h = bounds.h / 2;
    button_w = bounds.h - config->item_padding.x;
    button_x = bounds.x + bounds.w - button_w;
    button.padding.x = MAX(3, (button_h - layout->font.height) / 2);
    button.padding.y = MAX(3, (button_h - layout->font.height) / 2);
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
    button.content = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON];
    button.highlight_content = config->colors[GUI_COLOR_TEXT];
    button_up_clicked = gui_button_triangle(canvas, button_x, button_y, button_w, button_h,
        GUI_UP, GUI_BUTTON_DEFAULT, &button, layout->input);
    button_y = bounds.y + button_h;
    button_down_clicked = gui_button_triangle(canvas, button_x, button_y, button_w, button_h,
        GUI_DOWN, GUI_BUTTON_DEFAULT, &button, layout->input);
    if (button_up_clicked || button_down_clicked) {
        value += (button_up_clicked) ? step : -step;
        value = CLAMP(min, value, max);
    }

    field_x = bounds.x;
    field_y = bounds.y;
    field_w = bounds.w - button_w;
    field_h = bounds.h;
    field.padding.x = config->item_padding.x;
    field.padding.y = config->item_padding.y;
    field.show_cursor = gui_false;
    field.background = config->colors[GUI_COLOR_SPINNER];
    field.foreground = config->colors[GUI_COLOR_SPINNER_BORDER];
    len = gui_edit(canvas, field_x, field_y, field_w, field_h, (gui_char*)string,
            len, MAX_NUMBER_BUFFER, &is_active, &field,GUI_INPUT_FLOAT,
            layout->input, &layout->font);
    if (old_len != len)
        strtoi(&value, string, len);
    if (active) *active = is_active;
    return value;
}

gui_size
gui_panel_selector(struct gui_panel_layout *layout, const char *items[],
    gui_size item_count, gui_size item_current)
{
    gui_size text_len;
    gui_float label_x, label_y;
    gui_float label_w, label_h;

    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;
    const struct gui_canvas *canvas;
    gui_bool button_up_clicked;
    gui_bool button_down_clicked;
    gui_float button_x, button_y;
    gui_float button_w, button_h;

    ASSERT(items);
    ASSERT(item_count);
    ASSERT(item_current < item_count);
    if (!gui_panel_widget(&bounds, layout))
        return item_current;

    config = layout->config;
    canvas = layout->canvas;
    canvas->draw_rect(canvas->userdata, bounds.x, bounds.y, bounds.w, bounds.h,
            config->colors[GUI_COLOR_SELECTOR_BORDER]);
    canvas->draw_rect(canvas->userdata, bounds.x + 1, bounds.y + 1, bounds.w - 2, bounds.h - 2,
            config->colors[GUI_COLOR_SELECTOR]);

    button.border = 1;
    button_y = bounds.y;
    button_h = bounds.h / 2;
    button_w = bounds.h - config->item_padding.x;
    button_x = bounds.x + bounds.w - button_w;
    button.padding.x = MAX(3, (button_h - layout->font.height) / 2);
    button.padding.y = MAX(3, (button_h - layout->font.height) / 2);
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
    button.content = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON];
    button.highlight_content = config->colors[GUI_COLOR_TEXT];
    button_down_clicked = gui_button_triangle(canvas, button_x, button_y, button_w,
        button_h, GUI_UP, GUI_BUTTON_DEFAULT, &button, layout->input);

    button_y = bounds.y + button_h;
    button_up_clicked = gui_button_triangle(canvas, button_x, button_y, button_w,
        button_h, GUI_DOWN, GUI_BUTTON_DEFAULT, &button,  layout->input);
    item_current = (button_down_clicked && item_current < item_count-1) ?
        item_current+1 : (button_up_clicked && item_current > 0) ? item_current-1 : item_current;

    label_x = bounds.x + config->item_padding.x;
    label_y = bounds.y + config->item_padding.y;
    label_w = bounds.w - (button_w + 2 * config->item_padding.x);
    label_h = bounds.h - 2 * config->item_padding.y;
    text_len = strsiz(items[item_current]);
    canvas->draw_text(canvas->userdata, label_x, label_y, label_w, label_h,
        (const gui_char*)items[item_current], text_len, &layout->font,
        config->colors[GUI_COLOR_PANEL], config->colors[GUI_COLOR_TEXT]);
    return item_current;
}

void
gui_panel_graph_begin(struct gui_panel_layout *layout, struct gui_graph *graph,
    enum gui_graph_type type, gui_size count, gui_float min_value, gui_float max_value)
{
    struct gui_rect bounds;
    const struct gui_config *config;
    const struct gui_canvas *canvas;
    struct gui_color color;
    if (!gui_panel_widget(&bounds, layout)) {
        zero(graph, sizeof(*graph));
        return;
    }

    config = layout->config;
    canvas = layout->canvas;
    color = (type == GUI_GRAPH_LINES) ?
        config->colors[GUI_COLOR_PLOT]: config->colors[GUI_COLOR_HISTO];
    canvas->draw_rect(canvas->userdata, bounds.x, bounds.y, bounds.w, bounds.h,color);

    graph->valid = gui_true;
    graph->type = type;
    graph->index = 0;
    graph->count = count;
    graph->min = min_value;
    graph->max = max_value;
    graph->x = bounds.x + config->item_padding.x;
    graph->y = bounds.y + config->item_padding.y;
    graph->w = bounds.w - 2 * config->item_padding.x;
    graph->h = bounds.h - 2 * config->item_padding.y;
    graph->w = MAX(graph->w, 2 * config->item_padding.x);
    graph->h = MAX(graph->h, 2 * config->item_padding.y);
    graph->last.x = 0; graph->last.y = 0;
}

static gui_bool
gui_panel_graph_push_line(struct gui_panel_layout *layout,
    struct gui_graph *graph, gui_float value)
{
    const struct gui_canvas *canvas = layout->canvas;
    const struct gui_config *config = layout->config;
    const struct gui_input *in = layout->input;
    struct gui_color color = config->colors[GUI_COLOR_PLOT_LINES];
    gui_bool selected = gui_false;
    gui_float step, range, ratio;
    struct gui_vec2 cur;

    ASSERT(graph);
    ASSERT(layout);
    if (!graph || !layout || !graph->valid || graph->index >= graph->count)
        return gui_false;

    step = graph->w / (gui_float)graph->count;
    range = graph->max - graph->min;
    ratio = (value - graph->min) / range;

    if (graph->index == 0) {
        graph->last.x = graph->x;
        graph->last.y = (graph->y + graph->h) - ratio * (gui_float)graph->h;
        if (in && INBOX(in->mouse_pos.x, in->mouse_pos.y, graph->last.x-3, graph->last.y-3, 6, 6)) {
            selected = (in->mouse_down && in->mouse_clicked) ? gui_true: gui_false;
            color = config->colors[GUI_COLOR_PLOT_HIGHLIGHT];
        }
        canvas->draw_rect(canvas->userdata, graph->last.x - 3, graph->last.y - 3, 6, 6, color);
        graph->index++;
        return selected;
    }

    cur.x = graph->x + (gui_float)(step * (gui_float)graph->index);
    cur.y = (graph->y + graph->h) - (ratio * (gui_float)graph->h);
    canvas->draw_line(canvas->userdata, graph->last.x, graph->last.y, cur.x, cur.y,
        config->colors[GUI_COLOR_PLOT_LINES]);

    if (in && INBOX(in->mouse_pos.x, in->mouse_pos.y, cur.x-3, cur.y-3, 6, 6)) {
        selected = (in->mouse_down && in->mouse_clicked) ? gui_true: gui_false;
        color = config->colors[GUI_COLOR_PLOT_HIGHLIGHT];
    } else color = config->colors[GUI_COLOR_PLOT_LINES];
    canvas->draw_rect(canvas->userdata, cur.x - 3, cur.y - 3, 6, 6, color);

    graph->last.x = cur.x;
    graph->last.y = cur.y;
    graph->index++;
    return selected;
}

static gui_bool
gui_panel_graph_push_histo(struct gui_panel_layout *layout,
    struct gui_graph *graph, gui_float value)
{
    const struct gui_canvas *canvas = layout->canvas;
    const struct gui_config *config = layout->config;
    const struct gui_input *in = layout->input;
    struct gui_color color;

    gui_float ratio;
    gui_bool selected = gui_false;
    gui_float item_x, item_y;
    gui_float item_w = 0.0f, item_h = 0.0f;
    if (!graph->valid || graph->index >= graph->count)
        return gui_false;
    if (graph->count) {
        gui_float padding = (gui_float)(graph->count-1) * config->item_padding.x;
        item_w = (graph->w - padding) / (gui_float)(graph->count);
    }

    ratio = ABS(value) / graph->max;
    color = (value < 0) ? config->colors[GUI_COLOR_HISTO_NEGATIVE]:
        config->colors[GUI_COLOR_HISTO_BARS];

    item_h = graph->h * ratio;
    item_y = (graph->y + graph->h) - item_h;
    item_x = graph->x + ((gui_float)graph->index * item_w);
    item_x = item_x + ((gui_float)graph->index * config->item_padding.y);

    if (in && INBOX(in->mouse_pos.x, in->mouse_pos.y, item_x, item_y, item_w, item_h)) {
        selected = (in->mouse_down && in->mouse_clicked) ? (gui_int)graph->index: selected;
        color = config->colors[GUI_COLOR_HISTO_HIGHLIGHT];
    }
    canvas->draw_rect(canvas->userdata, item_x, item_y, item_w, item_h, color);
    graph->index++;
    return selected;
}

gui_bool
gui_panel_graph_push(struct gui_panel_layout *layout, struct gui_graph *graph, gui_float value)
{
    ASSERT(layout);
    ASSERT(graph);
    if (!layout || !graph || !layout->valid) return gui_false;
    if (graph->type == GUI_GRAPH_LINES)
        return gui_panel_graph_push_line(layout, graph, value);
    else if (graph->type == GUI_GRAPH_HISTO)
        return gui_panel_graph_push_histo(layout, graph, value);
    else return gui_false;
}

void
gui_panel_graph_end(struct gui_panel_layout *layout, struct gui_graph *graph)
{
    ASSERT(layout);
    ASSERT(graph);
    if (!layout || !graph) return;
    graph->type = GUI_GRAPH_MAX;
    graph->index = 0;
    graph->count = 0;
    graph->min = 0;
    graph->max = 0;
    graph->x = 0;
    graph->y = 0;
    graph->w = 0;
    graph->h = 0;
}

gui_int
gui_panel_graph(struct gui_panel_layout *layout, enum gui_graph_type type,
    const gui_float *values, gui_size count)
{
    gui_size i;
    gui_int index = -1;
    gui_float min_value;
    gui_float max_value;
    struct gui_graph graph;

    ASSERT(layout);
    ASSERT(values);
    ASSERT(count);
    if (!layout || !layout->valid || !values || !count)
        return -1;

    max_value = values[0];
    min_value = values[0];
    for (i = 0; i < count; ++i) {
        if (values[i] > max_value)
            max_value = values[i];
        if (values[i] < min_value)
            min_value = values[i];
    }

    gui_panel_graph_begin(layout, &graph, type, count, min_value, max_value);
    for (i = 0; i <  count; ++i) {
        if (gui_panel_graph_push(layout, &graph, values[i]))
            index = (gui_int)i;
    }
    gui_panel_graph_end(layout, &graph);
    return index;
}

gui_int
gui_panel_graph_ex(struct gui_panel_layout *layout, enum gui_graph_type type,
    gui_size count, gui_float(*get_value)(void*, gui_size), void *userdata)
{
    gui_size i;
    gui_int index = -1;
    gui_float min_value;
    gui_float max_value;
    struct gui_graph graph;

    ASSERT(layout);
    ASSERT(get_value);
    ASSERT(count);
    if (!layout || !layout->valid || !get_value || !count)
        return -1;

    max_value = get_value(userdata, 0);
    min_value = max_value;
    for (i = 1; i < count; ++i) {
        gui_float value = get_value(userdata, i);
        if (value > max_value)
            max_value = value;
        if (value < min_value)
            min_value = value;
    }

    gui_panel_graph_begin(layout, &graph, type, count, min_value, max_value);
    for (i = 0; i < count; ++i) {
        gui_float value = get_value(userdata, i);
        if (gui_panel_graph_push(layout, &graph, value))
            index = (gui_int)i;
    }
    gui_panel_graph_end(layout, &graph);
    return index;
}

static void
gui_panel_table_hline(struct gui_panel_layout *layout, gui_size row_height)
{
    const struct gui_canvas *canvas = layout->canvas;
    const struct gui_config *config = layout->config;
    gui_float y = layout->at_y + (gui_float)row_height - layout->offset;
    gui_float x = layout->at_x + config->item_spacing.x + config->item_padding.x;
    gui_float w = layout->width - (2 * config->item_spacing.x + 2 * config->item_padding.x);
    canvas->draw_line(canvas->userdata, x, y, x + w,
        y, config->colors[GUI_COLOR_TABLE_LINES]);
}

static void
gui_panel_table_vline(struct gui_panel_layout *layout, gui_size cols)
{
    gui_size i;
    const struct gui_canvas *canvas = layout->canvas;
    const struct gui_config *config = layout->config;
    for (i = 0; i < cols - 1; ++i) {
        gui_float y, h;
        struct gui_rect bounds;

        gui_panel_alloc_space(&bounds, layout);
        y = layout->at_y + layout->row_height;
        h = layout->row_height;
        canvas->draw_line(canvas->userdata, bounds.x + bounds.w, y,
            bounds.x + bounds.w, h, config->colors[GUI_COLOR_TABLE_LINES]);
    }
    layout->index -= i;
}

void
gui_panel_table_begin(struct gui_panel_layout *layout, gui_flags flags,
    gui_size row_height, gui_size cols)
{
    ASSERT(layout);
    if (!layout || !layout->valid) return;

    layout->is_table = gui_true;
    layout->tbl_flags = flags;
    gui_panel_row(layout, (gui_float)row_height, cols);
    if (layout->tbl_flags & GUI_TABLE_HHEADER)
        gui_panel_table_hline(layout, row_height);
    if (layout->tbl_flags & GUI_TABLE_VHEADER)
        gui_panel_table_vline(layout, cols);
}

void
gui_panel_table_row(struct gui_panel_layout *layout)
{
    const struct gui_config *config;
    ASSERT(layout);
    if (!layout) return;

    config = layout->config;
    gui_panel_row(layout, layout->row_height - config->item_spacing.y, layout->row_columns);
    if (layout->tbl_flags & GUI_TABLE_HBODY)
        gui_panel_table_hline(layout, (gui_size)(layout->row_height - config->item_spacing.y));
    if (layout->tbl_flags & GUI_TABLE_VBODY)
        gui_panel_table_vline(layout, layout->row_columns);
}

void
gui_panel_table_end(struct gui_panel_layout *layout)
{
    layout->is_table = gui_false;
}

gui_bool
gui_panel_tab_begin(struct gui_panel_layout *parent, struct gui_panel_layout *tab,
    const char *title, gui_bool minimized)
{
    struct gui_rect bounds;
    const struct gui_canvas *canvas;
    struct gui_panel panel;
    struct gui_rect clip;
    gui_flags flags;

    ASSERT(parent);
    ASSERT(tab);
    if (!parent || !tab) return gui_true;
    canvas = parent->canvas;
    zero(tab, sizeof(*tab));
    tab->valid = !minimized;
    if (!parent->valid) {
        tab->valid = gui_false;
        tab->config = parent->config;
        tab->canvas = parent->canvas;
        tab->input = parent->input;
        return minimized;
    }

    parent->index = 0;
    gui_panel_row(parent, 0, 1);
    gui_panel_alloc_space(&bounds, parent);

    flags = GUI_PANEL_BORDER|GUI_PANEL_MINIMIZABLE|GUI_PANEL_TAB;
    gui_panel_init(&panel,bounds.x,bounds.y,bounds.w,null_rect.h,flags,parent->config,&parent->font);
    panel.minimized = minimized;

    gui_panel_begin(tab, &panel, title, canvas, parent->input);
    unify(&clip, &parent->clip, tab->clip.x, tab->clip.y, tab->clip.x + tab->clip.w,
        tab->clip.y + tab->clip.h);
    canvas->scissor(canvas->userdata, clip.x, clip.y, clip.w, clip.h);
    return panel.minimized;
}

void
gui_panel_tab_end(struct gui_panel_layout *parent, struct gui_panel_layout *tab)
{
    struct gui_panel panel;
    const struct gui_canvas *canvas;
    ASSERT(tab);
    ASSERT(parent);
    if (!parent || !tab || !parent->valid)
        return;

    panel.x = tab->at_x;
    panel.y = tab->y;
    panel.flags = GUI_PANEL_BORDER|GUI_PANEL_MINIMIZABLE|GUI_PANEL_TAB;
    gui_panel_end(tab, &panel);

    parent->at_y += tab->height + tab->config->item_spacing.y;
    canvas = parent->canvas;
    canvas->scissor(canvas->userdata, parent->clip.x,parent->clip.y,parent->clip.w,parent->clip.h);
}

void
gui_panel_group_begin(struct gui_panel_layout *parent, struct gui_panel_layout *group,
    const char *title, gui_float offset)
{
    gui_flags flags;
    struct gui_rect bounds;
    const struct gui_canvas *canvas;
    struct gui_rect clip;
    struct gui_panel panel;
    const struct gui_rect *c;

    ASSERT(parent);
    ASSERT(group);
    if (!parent || !group) return;
    if (!parent->valid)
        goto failed;

    gui_panel_alloc_space(&bounds, parent);
    zero(group, sizeof(*group));
    c = &parent->clip;
    if (!INTERSECT(c->x, c->y, c->w, c->h, bounds.x, bounds.y, bounds.w, bounds.h))
        goto failed;

    canvas = parent->canvas;
    flags = GUI_PANEL_BORDER|GUI_PANEL_SCROLLBAR|GUI_PANEL_TAB;
    gui_panel_init(&panel, bounds.x,bounds.y,bounds.w,bounds.h,flags,parent->config,&parent->font);

    gui_panel_begin(group, &panel, title, parent->canvas, parent->input);
    group->offset = offset;
    unify(&clip, &parent->clip, group->clip.x, group->clip.y, group->clip.x + group->clip.w,
        group->clip.y + group->clip.h);
    canvas->scissor(canvas->userdata, clip.x, clip.y, clip.w, clip.h);
    return;

failed:
    group->valid = gui_false;
    group->config = parent->config;
    group->canvas = parent->canvas;
    group->input = parent->input;
}

gui_float
gui_panel_group_end(struct gui_panel_layout *parent, struct gui_panel_layout *group)
{
    const struct gui_canvas *canvas;
    struct gui_rect clip;
    struct gui_panel panel;

    ASSERT(parent);
    ASSERT(group);
    if (!parent || !group) return 0;
    if (!parent->valid) return 0;

    canvas = parent->canvas;
    panel.x = group->at_x;
    panel.y = group->y;
    panel.flags = GUI_PANEL_BORDER|GUI_PANEL_MINIMIZABLE|GUI_PANEL_TAB|GUI_PANEL_SCROLLBAR;

    unify(&clip, &parent->clip, group->clip.x, group->clip.y, group->x+group->w, group->y+group->h);
    canvas->scissor(canvas->userdata, clip.x, clip.y, clip.w, clip.h);
    gui_panel_end(group, &panel);
    canvas->scissor(canvas->userdata, parent->clip.x,parent->clip.y,parent->clip.w,parent->clip.h);
    return panel.offset;
}

gui_size
gui_panel_shelf_begin(struct gui_panel_layout *parent, struct gui_panel_layout *shelf,
    const char *tabs[], gui_size size, gui_size active, gui_float offset)
{
    const struct gui_config *config;
    const struct gui_canvas *canvas;

    struct gui_rect bounds;
    struct gui_rect clip;
    struct gui_rect *c;
    struct gui_panel panel;

    gui_float header_x, header_y;
    gui_float header_w, header_h;
    gui_float item_width;
    gui_flags flags;
    gui_size i;

    ASSERT(parent);
    ASSERT(tabs);
    ASSERT(shelf);
    ASSERT(active < size);
    if (!parent || !shelf || !tabs || active >= size)
        return active;

    if (!parent->valid)
        goto failed;

    config = parent->config;
    canvas = parent->canvas;
    gui_panel_alloc_space(&bounds, parent);
    zero(shelf, sizeof(*shelf));
    c = &parent->clip;
    if (!INTERSECT(c->x, c->y, c->w, c->h, bounds.x, bounds.y, bounds.w, bounds.h))
        goto failed;

    header_x = bounds.x;
    header_y = bounds.y;
    header_w = bounds.w;
    header_h = config->panel_padding.y + 3 * config->item_padding.y + parent->font.height;
    item_width = (header_w - (gui_float)size) / (gui_float)size;

    for (i = 0; i < size; i++) {
        struct gui_button button;
        gui_float button_x, button_y;
        gui_float button_w, button_h;
        button_y = header_y;
        button_h = header_h;
        button_x = header_x + ((gui_float)i * (item_width + 1));
        button_w = item_width;
        button.border = 1;
        button.padding.x = config->item_padding.x;
        button.padding.y = config->item_padding.y;
        button.background = config->colors[GUI_COLOR_HEADER];
        button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
        button.content = config->colors[GUI_COLOR_TEXT];
        button.highlight = config->colors[GUI_COLOR_HEADER];
        button.highlight_content = config->colors[GUI_COLOR_TEXT];
        if (active != i) {
            button_y += config->item_padding.y;
            button_h -= config->item_padding.y;
        }
        if (gui_button_text(canvas, button_x, button_y, button_w, button_h,
            tabs[i], GUI_BUTTON_DEFAULT, &button, parent->input, &parent->font))
            active = i;
    }

    bounds.y += header_h;
    bounds.h -= header_h;

    flags = GUI_PANEL_BORDER|GUI_PANEL_SCROLLBAR|GUI_PANEL_TAB|GUI_PANEL_NO_HEADER;
    gui_panel_init(&panel, bounds.x, bounds.y, bounds.w, bounds.h, flags, config, &parent->font);
    gui_panel_begin(shelf, &panel, NULL, canvas, parent->input);
    shelf->offset = offset;

    unify(&clip, &parent->clip, shelf->clip.x, shelf->clip.y,
        shelf->clip.x + shelf->clip.w, shelf->clip.y + shelf->clip.h);
    canvas->scissor(canvas->userdata, clip.x, clip.y, clip.w, clip.h);
    return active;

failed:
    shelf->valid = gui_false;
    shelf->config = parent->config;
    shelf->canvas = parent->canvas;
    shelf->input = parent->input;
    return active;
}

gui_float
gui_panel_shelf_end(struct gui_panel_layout *parent, struct gui_panel_layout *shelf)
{
    const struct gui_canvas *canvas;
    struct gui_rect clip;
    struct gui_panel panel;

    ASSERT(parent);
    ASSERT(shelf);
    if (!parent || !shelf) return 0;
    if (!parent->valid) return 0;

    canvas = parent->canvas;
    panel.x = shelf->at_x;
    panel.y = shelf->y;
    panel.flags = GUI_PANEL_BORDER|GUI_PANEL_MINIMIZABLE|GUI_PANEL_TAB|GUI_PANEL_SCROLLBAR;

    unify(&clip, &parent->clip, shelf->clip.x, shelf->clip.y, shelf->x+shelf->w, shelf->y+shelf->h);
    canvas->scissor(canvas->userdata, clip.x, clip.y, clip.w, clip.h);
    gui_panel_end(shelf, &panel);
    canvas->scissor(canvas->userdata, parent->clip.x,parent->clip.y,parent->clip.w,parent->clip.h);
    return panel.offset;
}

void
gui_panel_end(struct gui_panel_layout *layout, struct gui_panel *panel)
{
    const struct gui_config *config;
    const struct gui_canvas *canvas;
    ASSERT(layout);
    ASSERT(panel);
    if (!panel || !layout) return;
    layout->at_y += layout->row_height;

    config = layout->config;
    canvas = layout->canvas;
    if (!(panel->flags & GUI_PANEL_TAB))
        canvas->scissor(canvas->userdata, layout->x, layout->y, layout->w + 1, layout->h + 1);

    if (panel->flags & GUI_PANEL_SCROLLBAR && layout->valid) {
        struct gui_scroll scroll;
        gui_float panel_y;
        gui_float scroll_x, scroll_y;
        gui_float scroll_w, scroll_h;
        gui_float scroll_target, scroll_offset, scroll_step;

        scroll_x = layout->at_x + layout->width;
        scroll_y = (panel->flags & GUI_PANEL_BORDER) ? layout->y + 1 : layout->y;
        scroll_y += layout->header_height;
        scroll_w = config->scrollbar_width;
        scroll_h = layout->height;
        scroll_offset = layout->offset;
        scroll_step = layout->height * 0.25f;
        scroll.background = config->colors[GUI_COLOR_SCROLLBAR];
        scroll.foreground = config->colors[GUI_COLOR_SCROLLBAR_CURSOR];
        scroll.border = config->colors[GUI_COLOR_SCROLLBAR_BORDER];
        if (panel->flags & GUI_PANEL_BORDER) scroll_h -= 1;
        scroll_target = (layout->at_y-layout->y)-(layout->header_height+2*config->item_spacing.y);
        panel->offset = gui_scroll(canvas, scroll_x, scroll_y, scroll_w, scroll_h,
                                    scroll_offset, scroll_target, scroll_step,
                                    &scroll, layout->input);

        panel_y = layout->y + layout->height + layout->header_height - config->panel_padding.y;
        canvas->draw_rect(canvas->userdata,layout->x,panel_y,layout->width,config->panel_padding.y,
                        config->colors[GUI_COLOR_PANEL]);
    } else layout->height = layout->at_y - layout->y;

    if ((panel->flags & GUI_PANEL_SCALEABLE) && layout->valid) {
        struct gui_color col = config->colors[GUI_COLOR_SCALER];
        gui_float scaler_w = MAX(0, config->scaler_size.x - config->item_padding.x);
        gui_float scaler_h = MAX(0, config->scaler_size.y - config->item_padding.y);
        gui_float scaler_x = (layout->x + layout->w) - (config->item_padding.x + scaler_w);
        gui_float scaler_y = layout->y + layout->h - config->scaler_size.y;
        canvas->draw_triangle(canvas->userdata, scaler_x + scaler_w, scaler_y,
            scaler_x + scaler_w, scaler_y + scaler_h, scaler_x, scaler_y + scaler_h, col);
    }

    if (panel->flags & GUI_PANEL_BORDER) {
        const gui_float width = (panel->flags & GUI_PANEL_SCROLLBAR) ?
                layout->width + config->scrollbar_width : layout->width;
        const gui_float padding_y = (!layout->valid) ?
                panel->y + layout->header_height:
                (panel->flags & GUI_PANEL_SCROLLBAR) ?
                panel->y + layout->h :
                panel->y + layout->height + config->item_padding.y;

        canvas->draw_line(canvas->userdata, panel->x, padding_y, panel->x + width,
                padding_y, config->colors[GUI_COLOR_BORDER]);
        canvas->draw_line(canvas->userdata, panel->x, panel->y, panel->x,
                padding_y, config->colors[GUI_COLOR_BORDER]);
        canvas->draw_line(canvas->userdata, panel->x + width, panel->y, panel->x + width,
                padding_y, config->colors[GUI_COLOR_BORDER]);
    }
    canvas->scissor(canvas->userdata, 0, 0, (gui_float)canvas->width, (gui_float)canvas->height);
}

void
gui_stack_clear(struct gui_panel_stack *stack)
{
    stack->begin = NULL;
    stack->end = NULL;
    stack->count = 0;
}

void
gui_stack_push(struct gui_panel_stack *stack, struct gui_panel_hook *panel)
{
    if (!stack->begin) {
        panel->next = NULL;
        panel->prev = NULL;
        stack->begin = panel;
        stack->end = panel;
        stack->count = 1;
        return;
    }

    stack->end->next = panel;
    panel->prev = stack->end;
    panel->next = NULL;
    stack->end = panel;
    stack->count++;
}

void
gui_stack_pop(struct gui_panel_stack *stack, struct gui_panel_hook *panel)
{
    if (panel->prev)
        panel->prev->next = panel->next;
    if (panel->next)
        panel->next->prev = panel->prev;
    if (stack->begin == panel)
        stack->begin = panel->next;
    if (stack->end == panel)
        stack->end = panel->prev;
    panel->next = NULL;
    panel->prev = NULL;
}

