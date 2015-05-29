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
#define NULL ((void*)0)
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
#define PTR_ADD(t, p, i) ((t*)((void*)((gui_size)(p) + (i))))
#define PTR_SUB(t, p, i) ((t*)((void*)((gui_size)(p) - (i))))
#define ALIGNOF(t) ((char*)(&((struct {char c; t _h;}*)0)->_h) - (char*)0)
#define ALIGN_PTR(x, mask) (void*)((gui_size)((gui_byte*)(x) + (mask-1)) & ~(mask-1))
#define ALIGN(x, mask) ((x) + (mask-1)) & ~(mask-1)
#define OFFSETOF(st, m) ((gui_size)(&((st *)0)->m))

static const struct gui_rect null_rect = {-9999.0f, 9999.0f, 9999.0f, 9999.0f};
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

static void
triangle_from_direction(struct gui_vec2 *result, gui_float x, gui_float y,
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

/*
 * ==============================================================
 *
 *                          Input
 *
 * ===============================================================
 */
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

/*
 * ==============================================================
 *
 *                          Widgets
 *
 * ===============================================================
 */
void
gui_text(gui_command_buffer *out, gui_float x, gui_float y, gui_float w, gui_float h,
    const char *string, gui_size len, const struct gui_text *text,
    enum gui_text_align align, const struct gui_font *font)
{
    gui_float label_x;
    gui_float label_y;
    gui_float label_w;
    gui_float label_h;
    gui_size text_width;

    ASSERT(text);
    ASSERT(out);
    if (!text) return;

    label_y = y + text->padding.y;
    label_h = MAX(0, h - 2 * text->padding.y);

    text_width = font->width(font->userdata, (const gui_char*)string, len);
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

    gui_command_buffer_push_rect(out, x, y, w, h, text->background);
    gui_command_buffer_push_text(out, label_x, label_y, label_w, label_h,
        (const gui_char*)string, len, font, text->background, text->foreground);
}

static gui_bool
gui_do_button(gui_command_buffer *out, gui_float x, gui_float y, gui_float w,
    gui_float h, const struct gui_button *button, const struct gui_input *in,
    enum gui_button_behavior behavior)
{
    gui_bool ret = gui_false;
    struct gui_color background;

    ASSERT(button);
    ASSERT(out);
    if (!button || !out)
        return gui_false;

    background = button->background;
    if (in && INBOX(in->mouse_pos.x,in->mouse_pos.y, x, y, w, h)) {
        background = button->highlight;
        if (INBOX(in->mouse_clicked_pos.x, in->mouse_clicked_pos.y, x, y, w, h)) {
            ret = (behavior != GUI_BUTTON_DEFAULT) ? in->mouse_down:
                (in->mouse_down && in->mouse_clicked);
        }
    }

    gui_command_buffer_push_rect(out, x, y, w, h, button->foreground);
    gui_command_buffer_push_rect(out, x + button->border, y + button->border,
        w - 2 * button->border, h - 2 * button->border, background);
    return ret;
}

gui_bool
gui_button_text(gui_command_buffer *out, gui_float x, gui_float y,
    gui_float w, gui_float h, const char *string, enum gui_button_behavior b,
    const struct gui_button *button, const struct gui_input *in,
    const struct gui_font *font)
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
    ASSERT(out);
    if (!out || !button)
        return gui_false;

    font_color = button->content;
    bg_color = button->background;
    button_w = MAX(w, 2 * button->padding.x);
    button_h = MAX(h, font->height + 2 * button->padding.y);
    if (in && INBOX(in->mouse_pos.x, in->mouse_pos.y, x, y, button_w, button_h)) {
        font_color = button->highlight_content;
        bg_color = button->highlight;
    }
    ret = gui_do_button(out, x, y, button_w, button_h, button, in, b);

    inner_x = x + button->border;
    inner_y = y + button->border;
    inner_w = button_w - 2 * button->border;
    inner_h = button_h - 2 * button->border;

    text.padding.x = button->padding.x;
    text.padding.y = button->padding.y;
    text.background = bg_color;
    text.foreground = font_color;
    gui_text(out, inner_x, inner_y, inner_w, inner_h, string, strsiz(string),
        &text, GUI_TEXT_CENTERED, font);
    return ret;
}

gui_bool
gui_button_triangle(gui_command_buffer *out, gui_float x, gui_float y,
    gui_float w, gui_float h, enum gui_heading heading, enum gui_button_behavior bh,
    const struct gui_button *b, const struct gui_input *in)
{
    gui_bool pressed;
    struct gui_color col;
    struct gui_vec2 points[3];

    ASSERT(b);
    ASSERT(out);
    if (!out || !b)
        return gui_false;

    pressed = gui_do_button(out, x, y, w, h, b, in, bh);
    triangle_from_direction(points, x, y, w, h, b->padding.x, b->padding.y, heading);
    col = (in && INBOX(in->mouse_pos.x, in->mouse_pos.y, x, y, w, h)) ?
        b->highlight_content : b->content;
    gui_command_buffer_push_triangle(out,  points[0].x, points[0].y,
        points[1].x, points[1].y, points[2].x, points[2].y, col);
    return pressed;
}

gui_bool
gui_button_image(gui_command_buffer *out, gui_float x, gui_float y,
    gui_float w, gui_float h, gui_image img, enum gui_button_behavior b,
    const struct gui_button *button, const struct gui_input *in)
{
    gui_bool pressed;
    gui_float img_x, img_y, img_w, img_h;

    ASSERT(button);
    ASSERT(out);
    if (!out || !button)
        return gui_false;

    pressed = gui_do_button(out, x, y, w, h, button, in, b);
    img_x = x + button->padding.x;
    img_y = y + button->padding.y;
    img_w = w - 2 * button->padding.x;
    img_h = h - 2 * button->padding.y;
    gui_command_buffer_push_image(out, img_x, img_y, img_w, img_h, img);
    return pressed;
}

gui_bool
gui_toggle(gui_command_buffer *out, gui_float x, gui_float y, gui_float w,
    gui_float h, gui_bool active, const char *string, enum gui_toggle_type type,
    const struct gui_toggle *toggle, const struct gui_input *in,
    const struct gui_font *font)
{
    gui_bool toggle_active;
    gui_float select_size;
    gui_float toggle_w, toggle_h;
    gui_float select_x, select_y;
    gui_float cursor_x, cursor_y;
    gui_float cursor_pad, cursor_size;

    ASSERT(toggle);
    ASSERT(out);
    if (!out || !toggle)
        return 0;

    toggle_w = MAX(w, font->height + 2 * toggle->padding.x);
    toggle_h = MAX(h, font->height + 2 * toggle->padding.y);
    toggle_active = active;

    select_x = x + toggle->padding.x;
    select_y = y + toggle->padding.y;
    select_size = font->height + 2 * toggle->padding.y;

    cursor_pad = (type == GUI_TOGGLE_OPTION) ?
        (gui_float)(gui_int)(select_size / 4):
        (gui_float)(gui_int)(select_size / 8);
    cursor_size = select_size - cursor_pad * 2;
    cursor_x = select_x + cursor_pad;
    cursor_y = select_y + cursor_pad;

    if (in && !in->mouse_down && in->mouse_clicked)
        if (INBOX(in->mouse_clicked_pos.x, in->mouse_clicked_pos.y,
                cursor_x, cursor_y, cursor_size, cursor_size))
                toggle_active = !toggle_active;

    if (type == GUI_TOGGLE_CHECK)
        gui_command_buffer_push_rect(out, select_x, select_y, select_size,
           select_size, toggle->foreground);
    else gui_command_buffer_push_circle(out, select_x, select_y, select_size,
            select_size, toggle->foreground);

    if (toggle_active) {
        if (type == GUI_TOGGLE_CHECK)
            gui_command_buffer_push_rect(out, cursor_x, cursor_y, cursor_size,
                cursor_size, toggle->cursor);
        else gui_command_buffer_push_circle(out, cursor_x, cursor_y,
                cursor_size, cursor_size, toggle->cursor);
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
        gui_text(out, inner_x, inner_y, inner_w, inner_h, string, strsiz(string),
            &text, GUI_TEXT_LEFT, font);
    }
    return toggle_active;
}

gui_float
gui_slider(gui_command_buffer *out, gui_float x, gui_float y, gui_float w, gui_float h,
    gui_float min, gui_float val, gui_float max, gui_float step,
    const struct gui_slider *s, const struct gui_input *in)
{
    gui_float slider_range;
    gui_float slider_min, slider_max;
    gui_float slider_value, slider_steps;
    gui_float slider_w, slider_h;
    gui_float cursor_offset;
    struct gui_rect cursor;
    struct gui_rect bar;

    ASSERT(s);
    ASSERT(out);
    if (!out || !s)
        return 0;

    slider_w = MAX(w, 2 * s->padding.x);
    slider_h = MAX(h, 2 * s->padding.y);
    slider_max = MAX(min, max);
    slider_min = MIN(min, max);
    slider_value = CLAMP(slider_min, val, slider_max);
    slider_range = slider_max - slider_min;
    slider_steps = slider_range / step;

    cursor_offset = (slider_value - slider_min) / step;
    cursor.w = (slider_w - 2 * s->padding.x) / (slider_steps + 1);
    cursor.h = slider_h - 2 * s->padding.y;
    cursor.x = x + s->padding.x + (cursor.w * cursor_offset);
    cursor.y = y + s->padding.y;

    bar.x = x + s->padding.x;
    bar.y = (cursor.y + cursor.h/2) - cursor.h/8;
    bar.w = slider_w - 2 * s->padding.x;
    bar.h = cursor.h/4;

    if (in && in->mouse_down &&
        INBOX(in->mouse_pos.x, in->mouse_pos.y, x, y, slider_w, slider_h) &&
        INBOX(in->mouse_clicked_pos.x,in->mouse_clicked_pos.y, x, y, slider_w, slider_h))
    {
        const float d = in->mouse_pos.x - (cursor.x + cursor.w / 2.0f);
        const float pxstep = (slider_w - 2 * s->padding.x) / slider_steps;
        if (ABS(d) >= pxstep) {
            const gui_float steps = (gui_float)((gui_int)(ABS(d) / pxstep));
            slider_value += (d > 0) ? (step * steps) : -(step * steps);
            slider_value = CLAMP(slider_min, slider_value, slider_max);
            cursor.x = x + s->padding.x + (cursor.w * (slider_value - slider_min));
        }
    }

    gui_command_buffer_push_rect(out, x, y, slider_w, slider_h, s->bg);
    gui_command_buffer_push_rect(out, bar.x, bar.y, bar.w, bar.h, s->bar);
    gui_command_buffer_push_rect(out,cursor.x, cursor.y, cursor.w, cursor.h, s->border);
    gui_command_buffer_push_rect(out,cursor.x+1,cursor.y+1,cursor.w-2,cursor.h-2, s->fg);
    return slider_value;
}

gui_size
gui_progress(gui_command_buffer *out, gui_float x, gui_float y,
    gui_float w, gui_float h, gui_size value, gui_size max, gui_bool modifyable,
    const struct gui_progress *prog, const struct gui_input *in)
{
    gui_float cursor_x, cursor_y;
    gui_float cursor_w, cursor_h;
    gui_float prog_w, prog_h;
    gui_float prog_scale;
    gui_size prog_value;

    ASSERT(prog);
    ASSERT(out);
    if (!out || !prog) return 0;

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

    gui_command_buffer_push_rect(out, x, y, prog_w, prog_h, prog->background);
    gui_command_buffer_push_rect(out, cursor_x, cursor_y, cursor_w, cursor_h,
        prog->foreground);
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
    gui_filter filter, const struct gui_input *i)
{
    gui_long unicode;
    gui_size src_len = 0;
    gui_size text_len = 0, glyph_len = 0;
    ASSERT(buffer);
    ASSERT(i);

    glyph_len = gui_utf_decode(i->text, &unicode, i->text_len);
    while (glyph_len && ((text_len+glyph_len) <= i->text_len) && (length+src_len)<max) {
        if (filter(unicode)) {
            gui_size n = 0;
            for (n = 0; n < glyph_len; n++)
                buffer[length++] = i->text[text_len + n];
            text_len += glyph_len;
        }
        src_len = src_len + glyph_len;
        glyph_len = gui_utf_decode(i->text + src_len, &unicode, i->text_len - src_len);
    }
    return text_len;
}

gui_size
gui_edit_filtered(gui_command_buffer *out, gui_float x, gui_float y, gui_float w,
    gui_float h, gui_char *buffer, gui_size len, gui_size max, gui_bool *active,
    const struct gui_edit *field, gui_filter filter, const struct gui_input *in,
    const struct gui_font *font)
{
    gui_float input_w, input_h;
    gui_bool input_active;

    ASSERT(out);
    ASSERT(buffer);
    ASSERT(field);
    if (!out || !buffer || !field)
        return 0;

    input_w = MAX(w, 2 * field->padding.x);
    input_h = MAX(h, font->height);
    input_active = *active;

    gui_command_buffer_push_rect(out, x, y, input_w, input_h, field->background);
    gui_command_buffer_push_rect(out, x + 1, y, input_w - 1, input_h, field->foreground);
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
        gui_size cursor_w =(gui_size)font->width(font->userdata,(const gui_char*)"X", 1);

        gui_size text_len = len;
        gui_size text_width = font->width(font->userdata, buffer, text_len);
        while (text_len && (text_width + cursor_w) > (gui_size)label_w) {
            gui_long unicode;
            offset += gui_utf_decode(&buffer[offset], &unicode, text_len);
            text_len = len - offset;
            text_width = font->width(font->userdata, &buffer[offset], text_len);
        }

        label_x = x + field->padding.x;
        label_y = y + field->padding.y;
        label_h = input_h - 2 * field->padding.y;
        gui_command_buffer_push_text(out , label_x, label_y, label_w, label_h,
            &buffer[offset], text_len, font, field->foreground, field->background);

        if (input_active && field->show_cursor) {
            gui_command_buffer_push_rect(out, label_x + (gui_float)text_width, label_y,
                (gui_float)cursor_w, label_h, field->cursor);
        }
    }
    *active = input_active;
    return len;

}

gui_size
gui_edit(gui_command_buffer *out, gui_float x, gui_float y, gui_float w,
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
    return gui_edit_filtered(out, x, y, w, h, buffer, len, max, active,
                field,filter[f], in, font);
}

gui_float
gui_scroll(gui_command_buffer *out, gui_float x, gui_float y,
    gui_float w, gui_float h, gui_float offset, gui_float target,
    gui_float step, const struct gui_scroll *s, const struct gui_input *i)
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

    ASSERT(out);
    ASSERT(s);
    if (!out || !s) return 0;

    scroll_w = MAX(w, 0);
    scroll_h = MAX(h, 2 * scroll_w);
    gui_command_buffer_push_rect(out , x, y, scroll_w, scroll_h, s->background);
    if (target <= scroll_h) return 0;

    button.border = 1;
    button.padding.x = scroll_w / 4;
    button.padding.y = scroll_w / 4;
    button.background = s->background;
    button.foreground =  s->foreground;
    button.content = s->foreground;
    button.highlight = s->background;
    button.highlight_content = s->foreground;

    button_up_pressed = gui_button_triangle(out, x, y, scroll_w, scroll_w,
         GUI_UP, GUI_BUTTON_DEFAULT, &button, i);
    button_down_pressed = gui_button_triangle(out, x, y + scroll_h - scroll_w,
        scroll_w, scroll_w, GUI_DOWN, GUI_BUTTON_DEFAULT, &button, i);

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

    if (i) {
        const struct gui_vec2 mouse_pos = i->mouse_pos;
        const struct gui_vec2 mouse_prev = i->mouse_prev;
        inscroll = INBOX(mouse_pos.x,mouse_pos.y, x, y, scroll_w, scroll_h);
        incursor = INBOX(mouse_prev.x,mouse_prev.y,cursor_x,cursor_y,cursor_w, cursor_h);
        if (i->mouse_down && inscroll && incursor) {
            /* update cursor over mouse dragging */
            const gui_float pixel = i->mouse_delta.y;
            const gui_float delta =  (pixel / scroll_h) * target;
            scroll_offset = CLAMP(0, scroll_offset + delta, target - scroll_h);
            cursor_y += pixel;
        } else if (button_up_pressed || button_down_pressed) {
            /* update cursor over up/down button */
            scroll_offset = (button_down_pressed) ?
                MIN(scroll_offset + scroll_step, target - scroll_h):
                MAX(0, scroll_offset - scroll_step);
            scroll_off = scroll_offset / target;
            cursor_y = scroll_y + (scroll_off * scroll_h);
        }
    }

    gui_command_buffer_push_rect(out, cursor_x, cursor_y, cursor_w,
        cursor_h, s->foreground);
    return scroll_offset;
}

/*
 * ==============================================================
 *
 *                          Buffer
 *
 * ===============================================================
 */
void
gui_buffer_init(struct gui_buffer *buffer, const struct gui_allocator *memory,
    gui_size initial_size, gui_float grow_factor)
{
    ASSERT(buffer);
    ASSERT(memory);
    ASSERT(initial_size);
    if (!buffer || !memory || !initial_size) return;

    zero(buffer, sizeof(*buffer));
    buffer->type = GUI_BUFFER_DYNAMIC;
    buffer->memory.ptr = memory->alloc(memory->userdata, initial_size);
    buffer->memory.size = initial_size;
    buffer->grow_factor = grow_factor;
    buffer->pool = *memory;
}

void
gui_buffer_init_fixed(struct gui_buffer *buffer, void *memory, gui_size size)
{
    ASSERT(buffer);
    ASSERT(memory);
    ASSERT(size);
    if (!buffer || !memory || !size) return;

    zero(buffer, sizeof(*buffer));
    buffer->type = GUI_BUFFER_FIXED;
    buffer->memory.ptr = memory;
    buffer->memory.size = size;
}

void*
gui_buffer_alloc(struct gui_buffer *b, gui_size size, gui_size align)
{
    gui_size cap;
    gui_size alignment;
    void *unaligned;
    void *memory;

    ASSERT(b);
    if (!b || !size) return NULL;
    b->needed += size;

    unaligned = PTR_ADD(void, b->memory.ptr, b->allocated);
    memory = ALIGN_PTR(unaligned, align);
    alignment = (gui_size)((gui_byte*)memory - (gui_byte*)unaligned);

    if ((b->allocated + size + alignment) >= b->memory.size) {
        if (b->type != GUI_BUFFER_DYNAMIC || !b->pool.realloc)
            return NULL;

        cap = (gui_size)((gui_float)b->memory.size * b->grow_factor);
        b->memory.ptr = b->pool.realloc(b->pool.userdata, b->memory.ptr, cap);
        if (!b->memory.ptr) return NULL;

        b->memory.size = cap;
        unaligned = PTR_ADD(gui_byte, b->memory.ptr, b->allocated);
        memory = ALIGN_PTR(unaligned, align);
        alignment = (gui_size)((gui_byte*)memory - (gui_byte*)unaligned);
    }

    b->allocated += size + alignment;
    b->needed += alignment;
    b->calls++;
    return memory;
}

void
gui_buffer_reset(struct gui_buffer *buffer)
{
    ASSERT(buffer);
    if (!buffer) return;
    buffer->allocated = 0;
    buffer->calls = 0;
}

void
gui_buffer_clear(struct gui_buffer *buffer)
{
    ASSERT(buffer);
    if (!buffer || !buffer->memory.ptr) return;
    if (buffer->type == GUI_BUFFER_FIXED) return;
    if (!buffer->pool.free) return;
    buffer->pool.free(buffer->pool.userdata, buffer->memory.ptr);
}

void
gui_buffer_info(struct gui_memory_status *status, struct gui_buffer *buffer)
{
    ASSERT(buffer);
    ASSERT(status);
    if (!status || !buffer) return;
    status->allocated = buffer->allocated;
    status->size =  buffer->memory.size;
    status->needed = buffer->needed;
    status->memory = buffer->memory.ptr;
    status->calls = buffer->calls;
}

/*
 * ==============================================================
 *
 *                          Command buffer
 *
 * ===============================================================
 */
void*
gui_command_buffer_push(gui_command_buffer* buffer,
    enum gui_command_type type, gui_size size)
{
    static const gui_size align = ALIGNOF(struct gui_command);
    struct gui_command *cmd;
    gui_size alignment;
    void *unaligned;
    void *memory;
    ASSERT(buffer);
    if (!buffer) return NULL;

    cmd = gui_buffer_alloc(buffer, size, align);
    if (!cmd) return NULL;

    unaligned = (gui_byte*)cmd + size;
    memory = ALIGN_PTR(unaligned, align);
    alignment = (gui_size)((gui_byte*)memory - (gui_byte*)unaligned);

    cmd->type = type;
    cmd->offset = size + alignment;
    return cmd;
}

void
gui_command_buffer_push_scissor(gui_command_buffer *buffer, gui_float x, gui_float y,
    gui_float w, gui_float h)
{
    struct gui_command_scissor *cmd;
    ASSERT(buffer);
    if (!buffer) return;

    cmd = gui_command_buffer_push(buffer, GUI_COMMAND_SCISSOR, sizeof(*cmd));
    if (!cmd) return;
    cmd->x = (gui_short)x;
    cmd->y = (gui_short)y;
    cmd->w = (gui_ushort)MAX(0, w);
    cmd->h = (gui_ushort)MAX(0, h);
}

void
gui_command_buffer_push_line(gui_command_buffer *buffer, gui_float x0, gui_float y0,
    gui_float x1, gui_float y1, struct gui_color c)
{
    struct gui_command_line *cmd;
    ASSERT(buffer);
    if (!buffer) return;

    cmd = gui_command_buffer_push(buffer, GUI_COMMAND_LINE, sizeof(*cmd));
    if (!cmd) return;
    cmd->begin[0] = (gui_short)x0;
    cmd->begin[1] = (gui_short)y0;
    cmd->end[0] = (gui_short)x1;
    cmd->end[1] = (gui_short)y1;
    cmd->color = c;
}

void
gui_command_buffer_push_rect(gui_command_buffer *buffer, gui_float x, gui_float y,
    gui_float w, gui_float h, struct gui_color c)
{
    struct gui_command_rect *cmd;
    ASSERT(buffer);
    if (!buffer) return;

    cmd = gui_command_buffer_push(buffer, GUI_COMMAND_RECT, sizeof(*cmd));
    if (!cmd) return;
    cmd->x = (gui_short)x;
    cmd->y = (gui_short)y;
    cmd->w = (gui_ushort)MAX(0, w);
    cmd->h = (gui_ushort)MAX(0, h);
    cmd->color = c;
}

void
gui_command_buffer_push_circle(gui_command_buffer *buffer, gui_float x, gui_float y,
    gui_float w, gui_float h, struct gui_color c)
{
    struct gui_command_circle *cmd;
    ASSERT(buffer);
    if (!buffer) return;

    cmd = gui_command_buffer_push(buffer, GUI_COMMAND_CIRCLE, sizeof(*cmd));
    if (!cmd) return;
    cmd->x = (gui_short)x;
    cmd->y = (gui_short)y;
    cmd->w = (gui_ushort)MAX(w, 0);
    cmd->h = (gui_ushort)MAX(h, 0);
    cmd->color = c;
}

void
gui_command_buffer_push_triangle(gui_command_buffer *buffer, gui_float x0, gui_float y0,
    gui_float x1, gui_float y1, gui_float x2, gui_float y2, struct gui_color c)
{
    struct gui_command_triangle *cmd;
    ASSERT(buffer);
    if (!buffer) return;

    cmd = gui_command_buffer_push(buffer, GUI_COMMAND_TRIANGLE, sizeof(*cmd));
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
gui_command_buffer_push_image(gui_command_buffer *buffer, gui_float x, gui_float y,
    gui_float w, gui_float h, gui_image img)
{
    struct gui_command_image *cmd;
    ASSERT(buffer);
    if (!buffer) return;

    cmd = gui_command_buffer_push(buffer, GUI_COMMAND_IMAGE, sizeof(*cmd));
    if (!cmd) return;
    cmd->x = (gui_short)x;
    cmd->y = (gui_short)y;
    cmd->w = (gui_ushort)MAX(0, w);
    cmd->h = (gui_ushort)MAX(0, h);
    cmd->img = img;
}

void
gui_command_buffer_push_text(gui_command_buffer *buffer, gui_float x, gui_float y,
    gui_float w, gui_float h, const gui_char *string, gui_size length,
    const struct gui_font *font, struct gui_color bg, struct gui_color fg)
{
    struct gui_command_text *cmd;
    ASSERT(buffer);
    ASSERT(font);
    if (!buffer || !string || !length) return;

    cmd = gui_command_buffer_push(buffer, GUI_COMMAND_TEXT, sizeof(*cmd) + length + 1);
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

/*
 * ==============================================================
 *
 *                          Config
 *
 * ===============================================================
 */
void
gui_config_default(struct gui_config *config, const struct gui_font *font)
{
    ASSERT(config);
    if (!config) return;
    zero(config, sizeof(*config));
    config->font = *font;
    vec2_load(config->properties[GUI_PROPERTY_SCROLLBAR_WIDTH], 16, 16);
    vec2_load(config->properties[GUI_PROPERTY_PADDING], 15.0f, 10.0f);
    vec2_load(config->properties[GUI_PROPERTY_SIZE], 64.0f, 64.0f);
    vec2_load(config->properties[GUI_PROPERTY_ITEM_SPACING], 10.0f, 4.0f);
    vec2_load(config->properties[GUI_PROPERTY_ITEM_PADDING], 4.0f, 4.0f);
    vec2_load(config->properties[GUI_PROPERTY_SCALER_SIZE], 16.0f, 16.0f);
    col_load(config->colors[GUI_COLOR_TEXT], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_PANEL], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_HEADER], 40, 40, 40, 255);
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
    col_load(config->colors[GUI_COLOR_SLIDER], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_SLIDER_BAR], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_SLIDER_BORDER], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_SLIDER_CURSOR], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_PROGRESS], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_PROGRESS_CURSOR], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_INPUT], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_INPUT_CURSOR], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_INPUT_BORDER], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_SPINNER], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_SPINNER_BORDER], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_SPINNER_TRIANGLE], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_SELECTOR], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_SELECTOR_BORDER], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_SELECTOR_TRIANGLE], 100, 100, 100, 255);
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
    col_load(config->colors[GUI_COLOR_SHELF], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_SHELF_TEXT], 150, 150, 150, 255);
    col_load(config->colors[GUI_COLOR_SHELF_ACTIVE], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_SHELF_ACTIVE_TEXT], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_SCALER], 100, 100, 100, 255);
}

struct gui_vec2
gui_config_property(const struct gui_config *config, enum gui_config_properties index)
{
    static struct gui_vec2 zero;
    ASSERT(config);
    if (!config) return zero;
    return config->properties[index];
}

struct gui_color
gui_config_color(const struct gui_config *config, enum gui_config_colors index)
{
    static struct gui_color zero;
    ASSERT(config);
    if (!config) return zero;
    return config->colors[index];
}

void
gui_config_push_color(struct gui_config *config, enum gui_config_colors index,
    gui_byte r, gui_byte g, gui_byte b, gui_byte a)
{
    struct gui_saved_color *c;
    ASSERT(config);
    if (!config) return;
    if (config->color >= GUI_MAX_COLOR_STACK) return;
    c = &config->color_stack[config->color++];
    c->value = config->colors[index];
    c->type = index;
    config->colors[index].r = r;
    config->colors[index].g = g;
    config->colors[index].b = b;
    config->colors[index].a = a;
}

void
gui_config_push_property(struct gui_config *config, enum gui_config_properties index,
    gui_float x, gui_float y)
{
    struct gui_saved_property *p;
    ASSERT(config);
    if (!config) return;
    if (config->property >= GUI_MAX_ATTRIB_STACK) return;
    p = &config->property_stack[config->property++];
    p->value = config->properties[index];
    p->type = index;
    config->properties[index].x = x;
    config->properties[index].y = y;
}

void
gui_config_pop_color(struct gui_config *config)
{
    struct gui_saved_color *c;
    ASSERT(config);
    if (!config) return;
    if (!config->color) return;
    c = &config->color_stack[--config->color];
    config->colors[c->type] = c->value;
}

void
gui_config_pop_property(struct gui_config *config)
{
    struct gui_saved_property *p;
    ASSERT(config);
    if (!config) return;
    if (!config->property) return;
    p = &config->property_stack[--config->property];
    config->properties[p->type] = p->value;
}

void
gui_config_reset_colors(struct gui_config *config)
{
    ASSERT(config);
    if (!config) return;
    while (config->color)
    gui_config_pop_color(config);
}

void
gui_config_reset_properties(struct gui_config *config)
{
    ASSERT(config);
    if (!config) return;
    while (config->property)
    gui_config_pop_property(config);
}

void
gui_config_reset(struct gui_config *config)
{
    ASSERT(config);
    if (!config) return;
    gui_config_reset_colors(config);
    gui_config_reset_properties(config);
}

/*
 * ==============================================================
 *
 *                          Panel
 *
 * ===============================================================
 */
void
gui_panel_init(struct gui_panel *panel, gui_float x, gui_float y, gui_float w,
    gui_float h, gui_flags flags, gui_command_buffer *buffer,
    const struct gui_config *config)
{
    ASSERT(panel);
    ASSERT(config);
    if (!panel || !config)
        return;

    panel->x = x;
    panel->y = y;
    panel->w = w;
    panel->h = h;
    panel->flags = flags;
    panel->config = config;
    panel->buffer = buffer;
    panel->offset = 0;
    panel->minimized = gui_false;
}

gui_bool
gui_panel_begin(struct gui_panel_layout *l, struct gui_panel *p,
    const char *text, const struct gui_input *i)
{
    const struct gui_config *c;
    gui_command_buffer *out;
    const struct gui_color *header;
    gui_float mouse_x, mouse_y;
    gui_float prev_x, prev_y;
    gui_float clicked_x, clicked_y;
    gui_float header_x = 0, header_w = 0;
    gui_float footer_h;
    gui_bool ret = gui_true;

    float scrollbar_width;
    struct gui_vec2 panel_size;
    struct gui_vec2 item_padding;
    struct gui_vec2 item_spacing;
    struct gui_vec2 panel_padding;
    struct gui_vec2 scaler_size;

    ASSERT(p);
    ASSERT(l);
    ASSERT(p->buffer);

    /* check arguments */
    if (!p || !p->buffer || !l)
        return gui_false;
    if (!(p->flags & GUI_PANEL_TAB))
        gui_command_buffer_reset(p->buffer);
    if (p->flags & GUI_PANEL_HIDDEN) {
        zero(l, sizeof(*l));
        l->valid = gui_false;
        l->config = p->config;
        l->buffer = p->buffer;
        return gui_false;
    }

    c = p->config;
    scrollbar_width = gui_config_property(c, GUI_PROPERTY_SCROLLBAR_WIDTH).x;
    panel_size = gui_config_property(c, GUI_PROPERTY_SIZE);
    panel_padding = gui_config_property(c, GUI_PROPERTY_PADDING);
    item_padding = gui_config_property(c, GUI_PROPERTY_ITEM_PADDING);
    item_spacing = gui_config_property(c, GUI_PROPERTY_ITEM_SPACING);
    scaler_size = gui_config_property(c, GUI_PROPERTY_SCALER_SIZE);

    l->header_height = c->font.height + 3 * item_padding.y;
    l->header_height += panel_padding.y;

    mouse_x = (i) ? i->mouse_pos.x : -1;
    mouse_y = (i) ? i->mouse_pos.y : -1;
    prev_x = (i) ? i->mouse_prev.x : -1;
    prev_y = (i) ? i->mouse_prev.y : -1;
    clicked_x = (i) ? i->mouse_clicked_pos.x : -1;
    clicked_y = (i) ? i->mouse_clicked_pos.y : -1;

    if (p->flags & GUI_PANEL_MOVEABLE) {
        gui_bool incursor;
        const gui_float move_x = p->x;
        const gui_float move_y = p->y;
        const gui_float move_w = p->w;
        const gui_float move_h = l->header_height;

        incursor = i && INBOX(i->mouse_prev.x,i->mouse_prev.y,move_x,move_y,move_w,move_h);
        if (i && i->mouse_down && incursor) {
            p->x = MAX(0, p->x + i->mouse_delta.x);
            p->y = MAX(0, p->y + i->mouse_delta.y);
        }
    }

    if (p->flags & GUI_PANEL_SCALEABLE) {
        gui_bool incursor;
        gui_float scaler_w = MAX(0, scaler_size.x - item_padding.x);
        gui_float scaler_h = MAX(0, scaler_size.y - item_padding.y);
        gui_float scaler_x = (p->x + p->w) - (item_padding.x + scaler_w);
        gui_float scaler_y = p->y + p->h - scaler_size.y;

        incursor = i && INBOX(prev_x, prev_y, scaler_x, scaler_y, scaler_w, scaler_h);
        if (i && i->mouse_down && incursor) {
            gui_float min_x = panel_size.x;
            gui_float min_y = panel_size.y;
            p->w = MAX(min_x, p->w + i->mouse_delta.x);
            p->h = MAX(min_y, p->h + i->mouse_delta.y);
        }
    }

    out = p->buffer;
    l->x = p->x;
    l->y = p->y;
    l->w = p->w;
    l->h = p->h;
    l->at_x = p->x;
    l->at_y = p->y;
    l->width = p->w;
    l->height = p->h;
    l->config = p->config;
    l->buffer = p->buffer;
    l->input = i;
    l->index = 0;
    l->row_columns = 0;
    l->row_height = 0;
    l->offset = p->offset;

    if (!(p->flags & GUI_PANEL_NO_HEADER)) {
        header = &c->colors[GUI_COLOR_HEADER];
        header_x = p->x + panel_padding.x;
        header_w = p->w - 2 * panel_padding.x;
        gui_command_buffer_push_rect(out, p->x, p->y, p->w, l->header_height, *header);
    } else l->header_height = 1;
    l->row_height = l->header_height + 2 * item_spacing.y;

    footer_h = scaler_size.y + item_padding.y;
    if ((p->flags & GUI_PANEL_SCALEABLE) &&
        (p->flags & GUI_PANEL_SCROLLBAR) &&
        !p->minimized) {
        gui_float footer_x, footer_y, footer_w;
        footer_x = p->x;
        footer_w = p->w;
        footer_y = p->y + p->h - footer_h;
        gui_command_buffer_push_rect(out, footer_x, footer_y, footer_w, footer_h,
            c->colors[GUI_COLOR_PANEL]);
    }

    if (!(p->flags & GUI_PANEL_TAB)) {
        p->flags |= GUI_PANEL_SCROLLBAR;
        if (i && i->mouse_down) {
            if (!INBOX(clicked_x, clicked_y, p->x, p->y, p->w, p->h))
                p->flags &= (gui_flag)~GUI_PANEL_ACTIVE;
            else p->flags |= GUI_PANEL_ACTIVE;
        }
    }

    l->clip.x = p->x;
    l->clip.w = p->w;
    l->clip.y = p->y + l->header_height - 1;
    if (p->flags & GUI_PANEL_SCROLLBAR) {
        if (p->flags & GUI_PANEL_SCALEABLE)
            l->clip.h = p->h - (footer_h + l->header_height);
        else l->clip.h = p->h - l->header_height;
            l->clip.h -= (panel_padding.y + item_padding.y);
    } else l->clip.h = null_rect.h;

    if ((p->flags & GUI_PANEL_CLOSEABLE) && (!(p->flags & GUI_PANEL_NO_HEADER))) {
        const gui_char *X = (const gui_char*)"x";
        const gui_size text_width = c->font.width(c->font.userdata, X, 1);
        const gui_float close_x = header_x;
        const gui_float close_y = p->y + panel_padding.y;
        const gui_float close_w = (gui_float)text_width + 2 * item_padding.x;
        const gui_float close_h = c->font.height + 2 * item_padding.y;
        gui_command_buffer_push_text(out, close_x, close_y, close_w, close_h,
            X, 1, &c->font, c->colors[GUI_COLOR_HEADER], c->colors[GUI_COLOR_TEXT]);

        header_w -= close_w;
        header_x += close_h - item_padding.x;
        if (i && INBOX(mouse_x, mouse_y, close_x, close_y, close_w, close_h)) {
            if (INBOX(clicked_x, clicked_y, close_x, close_y, close_w, close_h)) {
                ret = !(i->mouse_down && i->mouse_clicked);
                if (!ret) p->flags |= GUI_PANEL_HIDDEN;
            }
        }
    }

    if ((p->flags & GUI_PANEL_MINIMIZABLE) && (!(p->flags & GUI_PANEL_NO_HEADER))) {
        gui_size text_width;
        gui_float min_x, min_y, min_w, min_h;
        const gui_char *score = (p->minimized) ?
            (const gui_char*)"+":
            (const gui_char*)"-";

        text_width = c->font.width(c->font.userdata, score, 1);
        min_x = header_x;
        min_y = p->y + panel_padding.y;
        min_w = (gui_float)text_width + 3 * item_padding.x;
        min_h = c->font.height + 2 * item_padding.y;
        gui_command_buffer_push_text(out, min_x, min_y, min_w, min_h,
            score, 1, &c->font, c->colors[GUI_COLOR_HEADER],
            c->colors[GUI_COLOR_TEXT]);

        header_w -= min_w;
        header_x += min_w - item_padding.x;
        if (i && INBOX(mouse_x, mouse_y, min_x, min_y, min_w, min_h)) {
            if (INBOX(clicked_x, clicked_y, min_x, min_y, min_w, min_h))
                if (i->mouse_down && i->mouse_clicked)
                    p->minimized = !p->minimized;
        }
    }
    l->valid = !(p->minimized || (p->flags & GUI_PANEL_HIDDEN));

    if (text && !(p->flags & GUI_PANEL_NO_HEADER)) {
        const gui_size text_len = strsiz(text);
        const gui_float label_x = header_x + item_padding.x;
        const gui_float label_y = p->y + panel_padding.y;
        const gui_float label_w = header_w - (3 * item_padding.x);
        const gui_float label_h = c->font.height + 2 * item_padding.y;
        gui_command_buffer_push_text(out, label_x, label_y, label_w, label_h,
            (const gui_char*)text, text_len, &c->font, c->colors[GUI_COLOR_HEADER],
            c->colors[GUI_COLOR_TEXT]);
    }

    if (p->flags & GUI_PANEL_SCROLLBAR) {
        const struct gui_color *color = &c->colors[GUI_COLOR_PANEL];
        l->width = p->w - scrollbar_width;
        l->height = p->h - (l->header_height + 2 * item_spacing.y);
        if (p->flags & GUI_PANEL_SCALEABLE) l->height -= footer_h;
        if (l->valid)
            gui_command_buffer_push_rect(out, p->x, p->y + l->header_height,
                p->w, p->h - l->header_height, *color);
    }

    if (p->flags & GUI_PANEL_BORDER) {
        const struct gui_color *color = &c->colors[GUI_COLOR_BORDER];
        const gui_float width = (p->flags & GUI_PANEL_SCROLLBAR) ?
                l->width + scrollbar_width : l->width;

        gui_command_buffer_push_line(out, p->x, p->y,
                p->x + p->w, p->y, *color);
        gui_command_buffer_push_line(out, p->x, p->y, p->x,
                p->y + l->header_height, c->colors[GUI_COLOR_BORDER]);
        gui_command_buffer_push_line(out, p->x + width, p->y, p->x + width,
                p->y + l->header_height, c->colors[GUI_COLOR_BORDER]);
        if (p->flags & GUI_PANEL_BORDER_HEADER)
            gui_command_buffer_push_line(out, p->x, p->y + l->header_height,
                p->x + p->w, p->y + l->header_height, c->colors[GUI_COLOR_BORDER]);
    }
    gui_command_buffer_push_scissor(out, l->clip.x, l->clip.y, l->clip.w, l->clip.h);
    return ret;
}

gui_bool
gui_panel_begin_stacked(struct gui_panel_layout *l, struct gui_panel* p,
    struct gui_stack *s, const char *title, const struct gui_input *i)
{
    gui_bool inpanel;
    ASSERT(l);
    ASSERT(s);
    if (!l || !s) return gui_false;

    inpanel = INBOX(i->mouse_prev.x, i->mouse_prev.y, p->x, p->y, p->w, p->h);
    if (i->mouse_down && i->mouse_clicked && inpanel && p != s->end) {
        const struct gui_panel *iter = p->next;
        while (iter) {
            const struct gui_panel *cur = iter;
            if (INBOX(i->mouse_prev.x,i->mouse_prev.y,cur->x,cur->y,cur->w,cur->h) &&
              !cur->minimized) break;
            iter = iter->next;
        }

        if (!iter) {
            gui_stack_pop(s, p);
            gui_stack_push(s, p);
        }
    }
    return gui_panel_begin(l, p, title, (s->end == p) ? i : NULL);
}


gui_bool
gui_panel_begin_tiled(struct gui_panel_layout *tile, struct gui_panel *panel,
    struct gui_layout *layout, enum gui_layout_slot_index slot, gui_size index,
    const char *title, const struct gui_input *in)
{
    struct gui_rect bounds;
    struct gui_layout_slot *s;

    ASSERT(panel);
    ASSERT(tile);
    ASSERT(layout);

    if (!layout || !panel || !tile) return gui_false;
    if (slot >= GUI_SLOT_MAX) return gui_false;
    if (index >= layout->slots[slot].capacity) return gui_false;

    panel->flags &= (gui_flags)~GUI_PANEL_MINIMIZABLE;
    panel->flags &= (gui_flags)~GUI_PANEL_CLOSEABLE;
    panel->flags &= (gui_flags)~GUI_PANEL_MOVEABLE;
    panel->flags &= (gui_flags)~GUI_PANEL_SCALEABLE;

    s = &layout->slots[slot];
    bounds.x = s->offset.x * (gui_float)layout->width;
    bounds.y = s->offset.y * (gui_float)layout->height;
    bounds.w = s->ratio.x * (gui_float)layout->width;
    bounds.h = s->ratio.y * (gui_float)layout->height;

    if (s->format == GUI_LAYOUT_HORIZONTAL) {
        panel->h = bounds.h;
        panel->y = bounds.y;
        panel->x = bounds.x + (gui_float)index * panel->w;
        panel->w = bounds.w / (gui_float)s->capacity;
    } else {
        panel->x = bounds.x;
        panel->w = bounds.w;
        panel->h = bounds.h / (gui_float)s->capacity;
        panel->y = bounds.y + (gui_float)index * panel->h;
    }

    gui_stack_push(&layout->stack, panel);
    return gui_panel_begin(tile, panel, title, (layout->state) ? in : NULL);
}

void
gui_panel_row(struct gui_panel_layout *layout, gui_float height, gui_size cols)
{
    const struct gui_config *config;
    const struct gui_color *color;
    gui_command_buffer *out;
    struct gui_vec2 item_spacing;
    struct gui_vec2 panel_padding;

    ASSERT(layout);
    ASSERT(layout->config);
    ASSERT(layout->buffer);
    if (!layout) return;
    if (!layout->valid) return;

    config = layout->config;
    out = layout->buffer;
    color = &config->colors[GUI_COLOR_PANEL];
    item_spacing = gui_config_property(config, GUI_PROPERTY_ITEM_SPACING);
    panel_padding = gui_config_property(config, GUI_PROPERTY_PADDING);

    layout->index = 0;
    layout->at_y += layout->row_height;
    layout->row_columns = cols;
    layout->row_height = height + item_spacing.y;
    gui_command_buffer_push_rect(out,  layout->at_x, layout->at_y,
        layout->width, height + panel_padding.y, *color);
}

gui_size
gui_panel_row_columns(const struct gui_panel_layout *l, gui_size widget_size)
{
    struct gui_vec2 spacing;
    struct gui_vec2 padding;
    gui_size cols = 0, size;
    ASSERT(l);
    ASSERT(widget_size);
    if (!l || !widget_size)
        return 0;

    spacing = gui_config_property(l->config, GUI_PROPERTY_ITEM_SPACING);
    padding = gui_config_property(l->config, GUI_PROPERTY_PADDING);
    cols = (gui_size)(l->width) / widget_size;
    size = (cols * (gui_size)spacing.x) + 2 * (gui_size)padding.x + widget_size * cols;
    while ((size > l->width) && --cols)
        size = (cols*(gui_size)spacing.x) + 2*(gui_size)padding.x + widget_size * cols;
    return cols;
}

void
gui_panel_spacing(struct gui_panel_layout *l, gui_size cols)
{
    gui_size add;
    ASSERT(l);
    ASSERT(l->config);
    ASSERT(l->buffer);
    if (!l) return;
    if (!l->valid) return;

    add = (l->index + cols) % l->row_columns;
    if (l->index + cols > l->row_columns) {
        gui_size i;
        const struct gui_config *c = l->config;
        struct gui_vec2 item_spacing = gui_config_property(c, GUI_PROPERTY_ITEM_SPACING);
        const gui_float row_height = l->row_height - item_spacing.y;
        gui_size rows = (l->index + cols) / l->row_columns;
        for (i = 0; i < rows; ++i)
            gui_panel_row(l, row_height, l->row_columns);
    }
    l->index += add;
}

static void
gui_panel_alloc_space(struct gui_rect *bounds, struct gui_panel_layout *layout)
{
    const struct gui_config *config;
    gui_float panel_padding, panel_spacing, panel_space;
    gui_float item_offset, item_width, item_spacing;
    struct gui_vec2 spacing;
    struct gui_vec2 padding;

    ASSERT(layout);
    ASSERT(layout->config);
    ASSERT(bounds);
    if (!layout || !layout->config || !bounds)
        return;

    config = layout->config;
    spacing = gui_config_property(config, GUI_PROPERTY_ITEM_SPACING);
    padding = gui_config_property(config, GUI_PROPERTY_PADDING);

    if (layout->index >= layout->row_columns) {
        const gui_float row_height = layout->row_height - spacing.y;
        gui_panel_row(layout, row_height, layout->row_columns);
    }

    panel_padding = 2 * padding.x;
    panel_spacing = (gui_float)(layout->row_columns - 1) * spacing.x;
    panel_space  = layout->width - panel_padding - panel_spacing;

    item_width = panel_space / (gui_float)layout->row_columns;
    item_offset = (gui_float)layout->index * item_width;
    item_spacing = (gui_float)layout->index * spacing.x;

    bounds->x = layout->at_x + item_offset + item_spacing + padding.x;
    bounds->y = layout->at_y - layout->offset;
    bounds->w = item_width;
    bounds->h = layout->row_height - spacing.y;
    layout->index++;
}

gui_bool
gui_panel_widget(struct gui_rect *bounds, struct gui_panel_layout *layout)
{
    struct gui_rect *c;
    ASSERT(layout);
    ASSERT(layout->config);
    ASSERT(layout->buffer);
    if (!layout || !layout->config || !layout->buffer) return gui_false;
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
    struct gui_vec2 item_padding;

    ASSERT(layout);
    ASSERT(layout->config);
    ASSERT(layout->buffer);
    ASSERT(str && len);

    if (!layout || !layout->config || !layout->buffer) return;
    if (!layout->valid) return;
    gui_panel_alloc_space(&bounds, layout);
    config = layout->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);

    text.padding.x = item_padding.x;
    text.padding.y = item_padding.y;
    text.foreground = color;
    text.background = config->colors[GUI_COLOR_PANEL];
    gui_text(layout->buffer,bounds.x,bounds.y,bounds.w,bounds.h, str, len,
        &text, alignment, &config->font);
}

void
gui_panel_text(struct gui_panel_layout *l, const char *str, gui_size len,
    enum gui_text_align alignment)
{
    gui_panel_text_colored(l, str, len, alignment, l->config->colors[GUI_COLOR_TEXT]);
}

void
gui_panel_label_colored(struct gui_panel_layout *layout, const char *text,
    enum gui_text_align align, struct gui_color color)
{
    gui_size len = strsiz(text);
    gui_panel_text_colored(layout, text, len, align, color);
}

void
gui_panel_label(struct gui_panel_layout *layout, const char *text,
    enum gui_text_align align)
{
    gui_size len = strsiz(text);
    gui_panel_text(layout, text, len, align);
}

static gui_bool
gui_panel_button(struct gui_button *button, struct gui_rect *bounds,
    struct gui_panel_layout *layout)
{
    const struct gui_config *config;
    struct gui_vec2 item_padding;
    if (!gui_panel_widget(bounds, layout)) return gui_false;
    config = layout->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);

    button->border = 1;
    button->padding.x = item_padding.x;
    button->padding.y = item_padding.y;
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
    return gui_button_text(layout->buffer, bounds.x, bounds.y, bounds.w, bounds.h,
            str, behavior, &button, layout->input, &config->font);
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
    return gui_do_button(layout->buffer, bounds.x, bounds.y, bounds.w, bounds.h,
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
    return gui_button_triangle(layout->buffer, bounds.x, bounds.y, bounds.w,
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
    return gui_button_image(layout->buffer, bounds.x, bounds.y, bounds.w,
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
        return value;

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
    if (gui_button_text(layout->buffer, bounds.x, bounds.y, bounds.w, bounds.h,
        str, GUI_BUTTON_DEFAULT, &button, layout->input, &config->font)) value = !value;
    return value;
}

static gui_bool
gui_panel_toggle_base(struct gui_toggle *toggle, struct gui_rect *bounds,
    struct gui_panel_layout *layout)
{
    const struct gui_config *config;
    struct gui_vec2 item_padding;
    if (!gui_panel_widget(bounds, layout))
        return gui_false;

    config = layout->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    toggle->padding.x = item_padding.x;
    toggle->padding.y = item_padding.y;
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
    return gui_toggle(layout->buffer, bounds.x, bounds.y, bounds.w, bounds.h,
                is_active, text, GUI_TOGGLE_CHECK, &toggle, layout->input, &config->font);
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
    return gui_toggle(layout->buffer, bounds.x, bounds.y, bounds.w, bounds.h,
        is_active, text, GUI_TOGGLE_OPTION, &toggle, layout->input, &config->font);
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
    struct gui_vec2 item_padding;
    if (!gui_panel_widget(&bounds, layout))
        return value;

    config = layout->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    slider.padding.x = item_padding.x;
    slider.padding.y = item_padding.y;
    slider.bg = config->colors[GUI_COLOR_SLIDER];
    slider.fg = config->colors[GUI_COLOR_SLIDER_CURSOR];
    slider.bar = config->colors[GUI_COLOR_SLIDER_BAR];
    slider.border = config->colors[GUI_COLOR_SLIDER_BORDER];
    return gui_slider(layout->buffer, bounds.x, bounds.y, bounds.w, bounds.h,
                min_value, value, max_value, value_step, &slider, layout->input);
}

gui_size
gui_panel_progress(struct gui_panel_layout *layout, gui_size cur_value, gui_size max_value,
    gui_bool is_modifyable)
{
    struct gui_rect bounds;
    struct gui_progress prog;
    const struct gui_config *config;
    struct gui_vec2 item_padding;
    if (!gui_panel_widget(&bounds, layout))
        return cur_value;

    config = layout->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    prog.padding.x = item_padding.x;
    prog.padding.y = item_padding.y;
    prog.background = config->colors[GUI_COLOR_PROGRESS];
    prog.foreground = config->colors[GUI_COLOR_PROGRESS_CURSOR];
    return gui_progress(layout->buffer, bounds.x, bounds.y, bounds.w, bounds.h,
            cur_value, max_value, is_modifyable, &prog, layout->input);
}

static gui_bool
gui_panel_edit_base(struct gui_rect *bounds, struct gui_edit *field,
    struct gui_panel_layout *layout)
{
    const struct gui_config *config;
    struct gui_vec2 item_padding;
    if (!gui_panel_widget(bounds, layout))
        return gui_false;

    config = layout->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    field->padding.x = item_padding.x;
    field->padding.y = item_padding.y;
    field->show_cursor = gui_true;
    field->background = config->colors[GUI_COLOR_INPUT];
    field->foreground = config->colors[GUI_COLOR_INPUT_BORDER];
    field->cursor = config->colors[GUI_COLOR_INPUT_CURSOR];
    return gui_true;
}

gui_size
gui_panel_edit(struct gui_panel_layout *layout, gui_char *buffer, gui_size len,
    gui_size max, gui_bool *active, enum gui_input_filter filter)
{
    struct gui_rect bounds;
    struct gui_edit field;
    const struct gui_config *config = layout->config;
    if (!gui_panel_edit_base(&bounds, &field, layout)) return len;
    return gui_edit(layout->buffer, bounds.x, bounds.y, bounds.w, bounds.h,
        buffer, len, max, active, &field, filter, layout->input, &config->font);
}

gui_size
gui_panel_edit_filtered(struct gui_panel_layout *layout, gui_char *buffer, gui_size len,
    gui_size max, gui_bool *active, gui_filter filter)
{
    struct gui_rect bounds;
    struct gui_edit field;
    const struct gui_config *config = layout->config;
    if (!gui_panel_edit_base(&bounds, &field, layout)) return len;
    return gui_edit_filtered(layout->buffer, bounds.x, bounds.y, bounds.w, bounds.h,
        buffer, len, max, active, &field, filter, layout->input, &config->font);
}

gui_bool
gui_panel_shell(struct gui_panel_layout *layout, gui_char *buffer, gui_size *len,
    gui_size max, gui_bool *active)
{
    struct gui_rect bounds;
    struct gui_edit field;
    struct gui_button button;
    struct gui_vec2 item_padding;
    struct gui_vec2 item_spacing;
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
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    item_spacing = gui_config_property(config, GUI_PROPERTY_ITEM_SPACING);

    width = config->font.width(config->font.userdata, (const gui_char*)"submit", 6);
    button.border = 1;
    button_y = bounds.y;
    button_h = bounds.h;
    button_w = (gui_float)width + item_padding.x * 2;
    button_w += button.border * 2;
    button_x = bounds.x + bounds.w - button_w;
    button.padding.x = item_padding.x;
    button.padding.y = item_padding.y;
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
    button.content = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
    button.highlight_content = config->colors[GUI_COLOR_BUTTON_HOVER_FONT];
    clicked = gui_button_text(layout->buffer, button_x, button_y, button_w, button_h,
                "submit", GUI_BUTTON_DEFAULT, &button, layout->input, &config->font);

    field_x = bounds.x;
    field_y = bounds.y;
    field_w = bounds.w - button_w - item_spacing.x;
    field_h = bounds.h;
    field.padding.x = item_padding.x;
    field.padding.y = item_padding.y;
    field.show_cursor = gui_true;
    field.cursor = config->colors[GUI_COLOR_INPUT_CURSOR];
    field.background = config->colors[GUI_COLOR_INPUT];
    field.foreground = config->colors[GUI_COLOR_INPUT_BORDER];
    *len = gui_edit(layout->buffer, field_x, field_y, field_w, field_h, buffer,
            *len, max, active, &field, GUI_INPUT_DEFAULT, layout->input, &config->font);
    return clicked;
}

gui_int
gui_panel_spinner(struct gui_panel_layout *layout, gui_int min, gui_int value,
    gui_int max, gui_int step, gui_bool *active)
{
    struct gui_rect bounds;
    const struct gui_config *config;
    gui_command_buffer *out;
    struct gui_edit field;
    char string[MAX_NUMBER_BUFFER];
    gui_size len, old_len;
    struct gui_vec2 item_padding;

    struct gui_button button;
    gui_float button_x, button_y;
    gui_float button_w, button_h;
    gui_float field_x, field_y;
    gui_float field_w, field_h;
    gui_bool is_active, updated = gui_false;
    gui_bool button_up_clicked, button_down_clicked;
    if (!gui_panel_widget(&bounds, layout))
        return value;

    config = layout->config;
    out = layout->buffer;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);

    value = CLAMP(min, value, max);
    len = itos(string, value);
    is_active = (active) ? *active : gui_false;
    old_len = len;

    button.border = 1;
    button_y = bounds.y;
    button_h = bounds.h / 2;
    button_w = bounds.h - item_padding.x;
    button_x = bounds.x + bounds.w - button_w;
    button.padding.x = MAX(3, (button_h - config->font.height) / 2);
    button.padding.y = MAX(3, (button_h - config->font.height) / 2);
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
    button.content = config->colors[GUI_COLOR_SPINNER_TRIANGLE];
    button.highlight = config->colors[GUI_COLOR_BUTTON];
    button.highlight_content = config->colors[GUI_COLOR_SPINNER_TRIANGLE];
    button_up_clicked = gui_button_triangle(out, button_x, button_y, button_w, button_h,
        GUI_UP, GUI_BUTTON_DEFAULT, &button, layout->input);

    button_y = bounds.y + button_h;
    button_down_clicked = gui_button_triangle(out, button_x, button_y, button_w, button_h,
        GUI_DOWN, GUI_BUTTON_DEFAULT, &button, layout->input);
    if (button_up_clicked || button_down_clicked) {
        value += (button_up_clicked) ? step : -step;
        value = CLAMP(min, value, max);
    }

    field_x = bounds.x;
    field_y = bounds.y;
    field_w = bounds.w - button_w;
    field_h = bounds.h;
    field.padding.x = item_padding.x;
    field.padding.y = item_padding.y;
    field.show_cursor = gui_false;
    field.background = config->colors[GUI_COLOR_SPINNER];
    field.foreground = config->colors[GUI_COLOR_SPINNER_BORDER];
    len = gui_edit(out, field_x, field_y, field_w, field_h, (gui_char*)string,
            len, MAX_NUMBER_BUFFER, &is_active, &field,GUI_INPUT_FLOAT,
            layout->input, &config->font);

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
    gui_command_buffer *out;
    struct gui_vec2 item_padding;

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
    out = layout->buffer;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    gui_command_buffer_push_rect(out, bounds.x, bounds.y, bounds.w, bounds.h,
            config->colors[GUI_COLOR_SELECTOR_BORDER]);
    gui_command_buffer_push_rect(out, bounds.x+1, bounds.y+1, bounds.w-2, bounds.h-2,
            config->colors[GUI_COLOR_SELECTOR]);

    button.border = 1;
    button_y = bounds.y;
    button_h = bounds.h / 2;
    button_w = bounds.h - item_padding.x;
    button_x = bounds.x + bounds.w - button_w;
    button.padding.x = MAX(3, (button_h - config->font.height) / 2);
    button.padding.y = MAX(3, (button_h - config->font.height) / 2);
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
    button.content = config->colors[GUI_COLOR_SELECTOR_TRIANGLE];
    button.highlight = config->colors[GUI_COLOR_BUTTON];
    button.highlight_content = config->colors[GUI_COLOR_SELECTOR_TRIANGLE];
    button_down_clicked = gui_button_triangle(out, button_x, button_y, button_w,
        button_h, GUI_UP, GUI_BUTTON_DEFAULT, &button, layout->input);

    button_y = bounds.y + button_h;
    button_up_clicked = gui_button_triangle(out, button_x, button_y, button_w,
        button_h, GUI_DOWN, GUI_BUTTON_DEFAULT, &button,  layout->input);
    item_current = (button_down_clicked && item_current < item_count-1) ?
        item_current+1 : (button_up_clicked && item_current > 0) ?
        item_current-1 : item_current;

    label_x = bounds.x + item_padding.x;
    label_y = bounds.y + item_padding.y;
    label_w = bounds.w - (button_w + 2 * item_padding.x);
    label_h = bounds.h - 2 * item_padding.y;
    text_len = strsiz(items[item_current]);
    gui_command_buffer_push_text(out, label_x, label_y, label_w, label_h,
        (const gui_char*)items[item_current], text_len, &config->font,
        config->colors[GUI_COLOR_PANEL], config->colors[GUI_COLOR_TEXT]);
    return item_current;
}

void
gui_panel_graph_begin(struct gui_panel_layout *layout, struct gui_graph *graph,
    enum gui_graph_type type, gui_size count, gui_float min_value, gui_float max_value)
{
    struct gui_rect bounds;
    const struct gui_config *config;
    gui_command_buffer *out;
    struct gui_color color;
    struct gui_vec2 item_padding;
    if (!gui_panel_widget(&bounds, layout)) {
        zero(graph, sizeof(*graph));
        return;
    }

    config = layout->config;
    out = layout->buffer;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    color = (type == GUI_GRAPH_LINES) ?
        config->colors[GUI_COLOR_PLOT]: config->colors[GUI_COLOR_HISTO];
    gui_command_buffer_push_rect(out, bounds.x, bounds.y, bounds.w, bounds.h,color);

    graph->valid = gui_true;
    graph->type = type;
    graph->index = 0;
    graph->count = count;
    graph->min = min_value;
    graph->max = max_value;
    graph->x = bounds.x + item_padding.x;
    graph->y = bounds.y + item_padding.y;
    graph->w = bounds.w - 2 * item_padding.x;
    graph->h = bounds.h - 2 * item_padding.y;
    graph->w = MAX(graph->w, 2 * item_padding.x);
    graph->h = MAX(graph->h, 2 * item_padding.y);
    graph->last.x = 0; graph->last.y = 0;
}

static gui_bool
gui_panel_graph_push_line(struct gui_panel_layout *l,
    struct gui_graph *g, gui_float value)
{
    gui_command_buffer *out = l->buffer;
    const struct gui_config *config = l->config;
    const struct gui_input *i = l->input;
    struct gui_color color = config->colors[GUI_COLOR_PLOT_LINES];
    gui_bool selected = gui_false;
    gui_float step, range, ratio;
    struct gui_vec2 cur;

    ASSERT(g);
    ASSERT(l);
    ASSERT(out);
    if (!g || !l || !g->valid || g->index >= g->count)
        return gui_false;

    step = g->w / (gui_float)g->count;
    range = g->max - g->min;
    ratio = (value - g->min) / range;

    if (g->index == 0) {
        g->last.x = g->x;
        g->last.y = (g->y + g->h) - ratio * (gui_float)g->h;
        if (i && INBOX(i->mouse_pos.x, i->mouse_pos.y, g->last.x-3, g->last.y-3, 6, 6)){
            selected = (i->mouse_down && i->mouse_clicked) ? gui_true: gui_false;
            color = config->colors[GUI_COLOR_PLOT_HIGHLIGHT];
        }
        gui_command_buffer_push_rect(out, g->last.x - 3, g->last.y - 3, 6, 6, color);
        g->index++;
        return selected;
    }

    cur.x = g->x + (gui_float)(step * (gui_float)g->index);
    cur.y = (g->y + g->h) - (ratio * (gui_float)g->h);
    gui_command_buffer_push_line(out, g->last.x, g->last.y, cur.x, cur.y,
        config->colors[GUI_COLOR_PLOT_LINES]);

    if (i && INBOX(i->mouse_pos.x, i->mouse_pos.y, cur.x-3, cur.y-3, 6, 6)) {
        selected = (i->mouse_down && i->mouse_clicked) ? gui_true: gui_false;
        color = config->colors[GUI_COLOR_PLOT_HIGHLIGHT];
    } else color = config->colors[GUI_COLOR_PLOT_LINES];
    gui_command_buffer_push_rect(out, cur.x - 3, cur.y - 3, 6, 6, color);

    g->last.x = cur.x;
    g->last.y = cur.y;
    g->index++;
    return selected;
}

static gui_bool
gui_panel_graph_push_column(struct gui_panel_layout *layout,
    struct gui_graph *graph, gui_float value)
{
    gui_command_buffer *out = layout->buffer;
    const struct gui_config *config = layout->config;
    const struct gui_input *in = layout->input;
    struct gui_vec2 item_padding;
    struct gui_color color;

    gui_float ratio;
    gui_bool selected = gui_false;
    gui_float item_x, item_y;
    gui_float item_w = 0.0f, item_h = 0.0f;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);

    if (!graph->valid || graph->index >= graph->count)
        return gui_false;
    if (graph->count) {
        gui_float padding = (gui_float)(graph->count-1) * item_padding.x;
        item_w = (graph->w - padding) / (gui_float)(graph->count);
    }

    ratio = ABS(value) / graph->max;
    color = (value < 0) ? config->colors[GUI_COLOR_HISTO_NEGATIVE]:
        config->colors[GUI_COLOR_HISTO_BARS];

    item_h = graph->h * ratio;
    item_y = (graph->y + graph->h) - item_h;
    item_x = graph->x + ((gui_float)graph->index * item_w);
    item_x = item_x + ((gui_float)graph->index * item_padding.x);

    if (in && INBOX(in->mouse_pos.x, in->mouse_pos.y, item_x, item_y, item_w, item_h)) {
        selected = (in->mouse_down && in->mouse_clicked) ? (gui_int)graph->index: selected;
        color = config->colors[GUI_COLOR_HISTO_HIGHLIGHT];
    }
    gui_command_buffer_push_rect(out, item_x, item_y, item_w, item_h, color);
    graph->index++;
    return selected;
}

gui_bool
gui_panel_graph_push(struct gui_panel_layout *layout, struct gui_graph *graph,
    gui_float value)
{
    ASSERT(layout);
    ASSERT(graph);
    if (!layout || !graph || !layout->valid) return gui_false;
    switch (graph->type) {
    case GUI_GRAPH_LINES:
        return gui_panel_graph_push_line(layout, graph, value);
    case GUI_GRAPH_COLUMN:
        return gui_panel_graph_push_column(layout, graph, value);
    case GUI_GRAPH_MAX:
    default: return gui_false;
    }
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
    const gui_float *values, gui_size count, gui_size offset)
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
    for (i = offset; i < count; ++i) {
        if (values[i] > max_value)
            max_value = values[i];
        if (values[i] < min_value)
            min_value = values[i];
    }

    gui_panel_graph_begin(layout, &graph, type, count, min_value, max_value);
    for (i = offset; i < count; ++i) {
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
gui_panel_table_hline(struct gui_panel_layout *l, gui_size row_height)
{
    gui_command_buffer *out = l->buffer;
    const struct gui_config *c = l->config;
    const struct gui_vec2 item_padding = gui_config_property(c, GUI_PROPERTY_ITEM_PADDING);
    const struct gui_vec2 item_spacing = gui_config_property(c, GUI_PROPERTY_ITEM_SPACING);

    gui_float y = l->at_y + (gui_float)row_height - l->offset;
    gui_float x = l->at_x + item_spacing.x + item_padding.x;
    gui_float w = l->width - (2 * item_spacing.x + 2 * item_padding.x);
    gui_command_buffer_push_line(out, x, y, x + w, y, c->colors[GUI_COLOR_TABLE_LINES]);
}

static void
gui_panel_table_vline(struct gui_panel_layout *layout, gui_size cols)
{
    gui_size i;
    gui_command_buffer *out = layout->buffer;
    const struct gui_config *config = layout->config;
    for (i = 0; i < cols - 1; ++i) {
        gui_float y, h;
        struct gui_rect bounds;

        gui_panel_alloc_space(&bounds, layout);
        y = layout->at_y + layout->row_height;
        h = layout->row_height;
        gui_command_buffer_push_line(out,  bounds.x + bounds.w, y,
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
    struct gui_vec2 item_spacing;
    ASSERT(layout);
    if (!layout) return;

    config = layout->config;
    item_spacing = gui_config_property(config, GUI_PROPERTY_ITEM_SPACING);
    gui_panel_row(layout, layout->row_height - item_spacing.y, layout->row_columns);
    if (layout->tbl_flags & GUI_TABLE_HBODY)
        gui_panel_table_hline(layout, (gui_size)(layout->row_height - item_spacing.y));
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
    gui_command_buffer *out;
    struct gui_panel panel;
    struct gui_rect clip;
    gui_flags flags;

    ASSERT(parent);
    ASSERT(tab);
    if (!parent || !tab) return gui_true;
    out = parent->buffer;
    zero(tab, sizeof(*tab));
    tab->valid = !minimized;
    if (!parent->valid) {
        tab->valid = gui_false;
        tab->config = parent->config;
        tab->buffer = parent->buffer;
        tab->input = parent->input;
        return minimized;
    }

    parent->index = 0;
    gui_panel_row(parent, 0, 1);
    gui_panel_alloc_space(&bounds, parent);

    flags = GUI_PANEL_BORDER|GUI_PANEL_MINIMIZABLE|GUI_PANEL_TAB|GUI_PANEL_BORDER_HEADER;
    gui_panel_init(&panel,bounds.x,bounds.y,bounds.w,null_rect.h,flags,out,parent->config);
    panel.minimized = minimized;

    gui_panel_begin(tab, &panel, title, parent->input);
    unify(&clip, &parent->clip, tab->clip.x, tab->clip.y, tab->clip.x + tab->clip.w,
        tab->clip.y + tab->clip.h);
    gui_command_buffer_push_scissor(out, clip.x, clip.y, clip.w, clip.h);
    return panel.minimized;
}

void
gui_panel_tab_end(struct gui_panel_layout *p, struct gui_panel_layout *t)
{
    struct gui_panel panel;
    gui_command_buffer *out;
    struct gui_vec2 item_spacing;
    ASSERT(t);
    ASSERT(p);
    if (!p || !t || !p->valid)
        return;

    panel.x = t->at_x;
    panel.y = t->y;
    panel.flags = GUI_PANEL_BORDER|GUI_PANEL_MINIMIZABLE|GUI_PANEL_TAB;
    gui_panel_end(t, &panel);

    item_spacing = gui_config_property(p->config, GUI_PROPERTY_ITEM_SPACING);
    p->at_y += t->height + item_spacing.y;
    out = p->buffer;
    gui_command_buffer_push_scissor(out, p->clip.x, p->clip.y, p->clip.w, p->clip.h);
}

void
gui_panel_group_begin(struct gui_panel_layout *p, struct gui_panel_layout *g,
    const char *title, gui_float offset)
{
    gui_flags flags;
    struct gui_rect bounds;
    gui_command_buffer *out;
    struct gui_rect clip;
    struct gui_panel panel;
    const struct gui_rect *c;

    ASSERT(p);
    ASSERT(g);
    if (!p || !g) return;
    if (!p->valid)
        goto failed;

    gui_panel_alloc_space(&bounds, p);
    zero(g, sizeof(*g));
    c = &p->clip;
    if (!INTERSECT(c->x, c->y, c->w, c->h, bounds.x, bounds.y, bounds.w, bounds.h))
        goto failed;

    out = p->buffer;
    flags = GUI_PANEL_BORDER|GUI_PANEL_SCROLLBAR|GUI_PANEL_TAB;
    gui_panel_init(&panel, bounds.x,bounds.y,bounds.w,bounds.h,flags, out, p->config);

    gui_panel_begin(g, &panel, title, p->input);
    g->offset = offset;
    unify(&clip, &p->clip, g->clip.x, g->clip.y, g->clip.x + g->clip.w, 
        g->clip.y + g->clip.h);
    gui_command_buffer_push_scissor(out, clip.x, clip.y, clip.w, clip.h);
    return;

failed:
    g->valid = gui_false;
    g->config = p->config;
    g->buffer = p->buffer;
    g->input = p->input;
}

gui_float
gui_panel_group_end(struct gui_panel_layout *p, struct gui_panel_layout *g)
{
    gui_command_buffer *out;
    struct gui_rect clip;
    struct gui_panel pan;

    ASSERT(p);
    ASSERT(g);
    if (!p || !g) return 0;
    if (!p->valid) return 0;

    out = p->buffer;
    pan.x = g->at_x;
    pan.y = g->y;
    pan.flags = GUI_PANEL_BORDER|GUI_PANEL_MINIMIZABLE|GUI_PANEL_TAB|GUI_PANEL_SCROLLBAR;

    unify(&clip, &p->clip, g->clip.x, g->clip.y, g->x + g->w, g->y + g->h);
    gui_command_buffer_push_scissor(out, clip.x, clip.y, clip.w, clip.h);
    gui_panel_end(g, &pan);
    gui_command_buffer_push_scissor(out, p->clip.x, p->clip.y, p->clip.w, p->clip.h);
    return pan.offset;
}

gui_size
gui_panel_shelf_begin(struct gui_panel_layout *parent, struct gui_panel_layout *shelf,
    const char *tabs[], gui_size size, gui_size active, gui_float offset)
{
    const struct gui_config *config;
    gui_command_buffer *out;
    const struct gui_font *font;
    struct gui_vec2 item_spacing;
    struct gui_vec2 item_padding;
    struct gui_vec2 panel_padding;

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
    out = parent->buffer;
    font = &config->font;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    item_spacing = gui_config_property(config, GUI_PROPERTY_ITEM_SPACING);
    panel_padding = gui_config_property(config, GUI_PROPERTY_PADDING);

    gui_panel_alloc_space(&bounds, parent);
    zero(shelf, sizeof(*shelf));
    c = &parent->clip;
    if (!INTERSECT(c->x, c->y, c->w, c->h, bounds.x, bounds.y, bounds.w, bounds.h))
        goto failed;

    header_x = bounds.x;
    header_y = bounds.y;
    header_w = bounds.w;
    header_h = panel_padding.y + 3 * item_padding.y + config->font.height;
    item_width = (header_w - (gui_float)size) / (gui_float)size;

    for (i = 0; i < size; i++) {
        struct gui_button button;
        gui_float button_x, button_y;
        gui_float button_w, button_h;
        gui_size text_width = font->width(font->userdata,
            (const gui_char*)tabs[i], strsiz(tabs[i]));
        text_width = text_width + (gui_size)(2 * item_spacing.x);

        button_y = header_y;
        button_h = header_h;
        button_x = header_x;
        button_w = MIN(item_width, (gui_float)text_width);
        button.border = 1;
        button.padding.x = item_padding.x;
        button.padding.y = item_padding.y;
        button.foreground = config->colors[GUI_COLOR_BORDER];
        header_x += MIN(item_width, (gui_float)text_width);

        if ((button_x + button_w) >= (bounds.x + bounds.w)) break;
        if (active != i) {
            button_y += item_padding.y;
            button_h -= item_padding.y;
            button.background = config->colors[GUI_COLOR_SHELF];
            button.content = config->colors[GUI_COLOR_SHELF_TEXT];
            button.highlight = config->colors[GUI_COLOR_SHELF];
            button.highlight_content = config->colors[GUI_COLOR_SHELF_TEXT];
        } else {
            button.background = config->colors[GUI_COLOR_SHELF_ACTIVE];
            button.content = config->colors[GUI_COLOR_SHELF_ACTIVE_TEXT];
            button.highlight = config->colors[GUI_COLOR_SHELF_ACTIVE];
            button.highlight_content = config->colors[GUI_COLOR_SHELF_ACTIVE_TEXT];
        }

        if (gui_button_text(out, button_x, button_y, button_w, button_h,
            tabs[i], GUI_BUTTON_DEFAULT, &button, parent->input, &config->font))
            active = i;
    }

    bounds.y += header_h;
    bounds.h -= header_h;

    flags = GUI_PANEL_BORDER|GUI_PANEL_SCROLLBAR|GUI_PANEL_TAB|GUI_PANEL_NO_HEADER;
    gui_panel_init(&panel, bounds.x, bounds.y, bounds.w, bounds.h, flags, out, config);
    gui_panel_begin(shelf, &panel, NULL, parent->input);
    shelf->offset = offset;

    unify(&clip, &parent->clip, shelf->clip.x, shelf->clip.y,
        shelf->clip.x + shelf->clip.w, shelf->clip.y + shelf->clip.h);
    gui_command_buffer_push_scissor(out, clip.x, clip.y, clip.w, clip.h);
    return active;

failed:
    shelf->valid = gui_false;
    shelf->config = parent->config;
    shelf->buffer = parent->buffer;
    shelf->input = parent->input;
    return active;
}

gui_float
gui_panel_shelf_end(struct gui_panel_layout *p, struct gui_panel_layout *s)
{
    gui_command_buffer *out;
    struct gui_rect clip;
    struct gui_panel pan;

    ASSERT(p);
    ASSERT(s);
    if (!p || !s) return 0;
    if (!p->valid) return 0;

    out = p->buffer;
    pan.x = s->at_x; pan.y = s->y;
    pan.flags = GUI_PANEL_BORDER|GUI_PANEL_MINIMIZABLE|GUI_PANEL_TAB|GUI_PANEL_SCROLLBAR;

    unify(&clip, &p->clip, s->clip.x, s->clip.y, s->x + s->w, s->y + s->h);
    gui_command_buffer_push_scissor(out, clip.x, clip.y, clip.w, clip.h);
    gui_panel_end(s, &pan);
    gui_command_buffer_push_scissor(out, p->clip.x, p->clip.y, p->clip.w, p->clip.h);
    return pan.offset;
}

void
gui_panel_end(struct gui_panel_layout *layout, struct gui_panel *panel)
{
    const struct gui_config *config;
    gui_command_buffer *out;
    gui_float scrollbar_width;
    struct gui_vec2 item_padding;
    struct gui_vec2 item_spacing;
    struct gui_vec2 panel_padding;
    struct gui_vec2 scaler_size;

    ASSERT(layout);
    ASSERT(panel);
    if (!panel || !layout) return;
    layout->at_y += layout->row_height;

    config = layout->config;
    out = layout->buffer;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    item_spacing = gui_config_property(config, GUI_PROPERTY_ITEM_SPACING);
    panel_padding = gui_config_property(config, GUI_PROPERTY_PADDING);
    scrollbar_width = gui_config_property(config, GUI_PROPERTY_SCROLLBAR_WIDTH).x;
    scaler_size = gui_config_property(config, GUI_PROPERTY_SCALER_SIZE);
    if (!(panel->flags & GUI_PANEL_TAB))
        gui_command_buffer_push_scissor(out, layout->x,layout->y,layout->w+1,layout->h+1);

    if (panel->flags & GUI_PANEL_SCROLLBAR && layout->valid) {
        struct gui_scroll scroll;
        gui_float panel_y;
        gui_float scroll_x, scroll_y;
        gui_float scroll_w, scroll_h;
        gui_float scroll_target, scroll_offset, scroll_step;

        scroll_x = layout->at_x + layout->width;
        scroll_y = (panel->flags & GUI_PANEL_BORDER) ? layout->y + 1 : layout->y;
        scroll_y += layout->header_height;
        scroll_w = scrollbar_width;
        scroll_h = layout->height;
        scroll_offset = layout->offset;
        scroll_step = layout->height * 0.25f;
        scroll.background = config->colors[GUI_COLOR_SCROLLBAR];
        scroll.foreground = config->colors[GUI_COLOR_SCROLLBAR_CURSOR];
        scroll.border = config->colors[GUI_COLOR_SCROLLBAR_BORDER];
        if (panel->flags & GUI_PANEL_BORDER) scroll_h -= 1;
        scroll_target = (layout->at_y-layout->y)-(layout->header_height+2*item_spacing.y);
        panel->offset = gui_scroll(out, scroll_x, scroll_y, scroll_w, scroll_h,
                                    scroll_offset, scroll_target, scroll_step,
                                    &scroll, layout->input);

        panel_y = layout->y + layout->height + layout->header_height - panel_padding.y;
        gui_command_buffer_push_rect(out, layout->x,panel_y,layout->width,panel_padding.y,
                        config->colors[GUI_COLOR_PANEL]);
    } else layout->height = layout->at_y - layout->y;

    if ((panel->flags & GUI_PANEL_SCALEABLE) && layout->valid) {
        struct gui_color col = config->colors[GUI_COLOR_SCALER];
        gui_float scaler_w = MAX(0, scaler_size.x - item_padding.x);
        gui_float scaler_h = MAX(0, scaler_size.y - item_padding.y);
        gui_float scaler_x = (layout->x + layout->w) - (item_padding.x + scaler_w);
        gui_float scaler_y = layout->y + layout->h - scaler_size.y;
        gui_command_buffer_push_triangle(out, scaler_x + scaler_w, scaler_y,
            scaler_x + scaler_w, scaler_y + scaler_h, scaler_x, scaler_y + scaler_h, col);
    }

    if (panel->flags & GUI_PANEL_BORDER) {
        const gui_float width = (panel->flags & GUI_PANEL_SCROLLBAR) ?
                layout->width + scrollbar_width : layout->width;
        const gui_float padding_y = (!layout->valid) ?
                panel->y + layout->header_height:
                (panel->flags & GUI_PANEL_SCROLLBAR) ?
                panel->y + layout->h :
                panel->y + layout->height + item_padding.y;

        gui_command_buffer_push_line(out, panel->x, padding_y, panel->x + width,
                padding_y, config->colors[GUI_COLOR_BORDER]);
        gui_command_buffer_push_line(out, panel->x, panel->y, panel->x,
                padding_y, config->colors[GUI_COLOR_BORDER]);
        gui_command_buffer_push_line(out, panel->x + width, panel->y, panel->x + width,
                padding_y, config->colors[GUI_COLOR_BORDER]);
    }
    gui_command_buffer_push_scissor(out, 0, 0, null_rect.w, null_rect.h);
}

/*
 * ==============================================================
 *
 *                          Stack
 *
 * ===============================================================
 */
void
gui_stack_clear(struct gui_stack *stack)
{
    stack->begin = NULL;
    stack->end = NULL;
    stack->count = 0;
}

void
gui_stack_push(struct gui_stack *stack, struct gui_panel *panel)
{
    struct gui_panel *iter;
    ASSERT(stack);
    ASSERT(panel);
    if (!stack || !panel) return;
    gui_foreach_panel(iter, stack)
        if (iter == panel) return;

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
gui_stack_pop(struct gui_stack *stack, struct gui_panel*panel)
{
    ASSERT(stack);
    ASSERT(panel);
    if (!stack || !panel) return;

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

/*
 * ==============================================================
 *
 *                          Border Layout
 *
 * ===============================================================
 */
void
gui_layout_init(struct gui_layout *layout, const struct gui_layout_config *config)
{
    gui_float left, right;
    gui_float centerh, centerv;
    gui_float bottom, top;

    ASSERT(layout);
    ASSERT(config);
    if (!layout || !config) return;
    zero(layout, sizeof(*layout));
    layout->state = GUI_LAYOUT_ACTIVE;

    left = SATURATE(config->left);
    right = SATURATE(config->right);
    centerh = SATURATE(config->centerh);
    centerv = SATURATE(config->centerv);
    bottom = SATURATE(config->bottom);
    top = SATURATE(config->top);

    vec2_load(layout->slots[GUI_SLOT_TOP].ratio, 1.0f, top);
    vec2_load(layout->slots[GUI_SLOT_LEFT].ratio, left, centerv);
    vec2_load(layout->slots[GUI_SLOT_BOTTOM].ratio, 1.0f, bottom);
    vec2_load(layout->slots[GUI_SLOT_CENTER].ratio, centerh, centerv);
    vec2_load(layout->slots[GUI_SLOT_RIGHT].ratio, right, centerv);

    vec2_load(layout->slots[GUI_SLOT_TOP].offset, 0.0f, 0.0f);
    vec2_load(layout->slots[GUI_SLOT_LEFT].offset, 0.0f, top);
    vec2_load(layout->slots[GUI_SLOT_BOTTOM].offset, 0.0f, top + centerv);
    vec2_load(layout->slots[GUI_SLOT_CENTER].offset, left, top);
    vec2_load(layout->slots[GUI_SLOT_RIGHT].offset, left + centerh, top);
}

void
gui_layout_slot(struct gui_layout *layout, enum gui_layout_slot_index slot,
    enum gui_layout_format format, gui_size count)
{
    ASSERT(layout);
    ASSERT(count);
    ASSERT(slot >= GUI_SLOT_TOP && slot < GUI_SLOT_MAX);
    if (!layout || !count) return;
    layout->slots[slot].capacity = count;
    layout->slots[slot].format = format;
}

