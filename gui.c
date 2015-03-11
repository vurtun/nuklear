/*
    Copyright (c) 2015
    vurtun <polygone@gmx.net>
    MIT licence
*/
#include "gui.h"

#define NULL (void*)0
#define UTF_INVALID 0xFFFD
#define PASTE(a,b) a##b
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#define LEN(a) (sizeof(a)/sizeof(a)[0])
#define ABS(a) (((a) < 0) ? -(a) : (a))
#define UNUSED(a) ((void)(a))
#define BETWEEN(x, a, b) ((a) <= (x) && (x) <= (b))
#define INBOX(x, y, x0, y0, x1, y1) (BETWEEN(x, x0, x1) && BETWEEN(y, y0, y1))

#define ASSERT_LINE(p, l, f) \
    typedef char PASTE(assertion_failed_##f##_,l)[2*!!(p)-1];
#define ASSERT(predicate) ASSERT_LINE(predicate,__LINE__,__FILE__)

#define vec2_load(v,a,b) (v).x = (a), (v).y = (b)
#define vec2_mov(to,from) (to).x = (from).x, (to).y = (from).y
#define vec2_len(v) ((float)fsqrt((v).x*(v).x+(v).y*(v).y))
#define vec2_sub(r,a,b) do {(r).x=(a).x-(b).x; (r).y=(a).y-(b).y;} while(0)
#define vec2_muls(r, v, s) do {(r).x=(v).x*(s); (r).y=(v).y*(s);} while(0)

static const gui_texture null_tex = (void*)0;
static const struct gui_rect null_rect = {-9999, -9999, 9999 * 2, 9999 * 2};
static gui_char utfbyte[GUI_UTF_SIZE+1] = {0x80, 0, 0xC0, 0xE0, 0xF0};
static gui_char utfmask[GUI_UTF_SIZE+1] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
static long utfmin[GUI_UTF_SIZE+1] = {0, 0, 0x80, 0x800, 0x100000};
static long utfmax[GUI_UTF_SIZE+1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};

static gui_float
fsqrt(float x)
{
    float xhalf = 0.5f*x;
    union {float f; int i;} val;
    ASSERT(sizeof(int) == sizeof(float));

    val.f = x;
    val.i = 0x5f375a86 - (val.i>>1);
    val.f = val.f*(1.5f-xhalf*val.f*val.f);
    return 1.0f/val.f;
}

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

static gui_size
utf_validate(long *u, gui_size i)
{
    if (!u) return 0;
    if (!BETWEEN(*u, utfmin[i], utfmax[i]) ||
         BETWEEN(*u, 0xD800, 0xDFFF))
            *u = UTF_INVALID;
    for (i = 1; *u > utfmax[i]; ++i);
    return i;
}

static gui_long
utf_decode_byte(gui_char c, gui_size *i)
{
    if (!i) return 0;
    for(*i = 0; *i < LEN(utfmask); ++(*i)) {
        if ((c & utfmask[*i]) == utfbyte[*i])
            return c & ~utfmask[*i];
    }
    return 0;
}

static gui_size
utf_decode(const gui_char *c, gui_long *u, gui_size clen)
{
    gui_size i, j, len, type;
    gui_long udecoded;

    *u = UTF_INVALID;
    if (!c || !u) return 0;
    if (!clen) return 0;
    udecoded = utf_decode_byte(c[0], &len);
    if (!BETWEEN(len, 1, GUI_UTF_SIZE))
        return 1;

    for (i = 1, j = 1; i < clen && j < len; ++i, ++j) {
        udecoded = (udecoded << 6) | utf_decode_byte(c[i], &type);
        if (type != 0)
            return j;
    }
    if (j < len)
        return 0;
    *u = udecoded;
    utf_validate(u, len);
    return len;
}

static gui_char
utf_encode_byte(gui_long u, gui_size i)
{
    return (gui_char)(utfbyte[i] | (u & ~utfmask[i]));
}

static gui_size
utf_encode(gui_long u, gui_char *c, gui_size clen)
{
    gui_size len, i;
    len = utf_validate(&u, 0);
    if (clen < len || !len)
        return 0;
    for (i = len - 1; i != 0; --i) {
        c[i] = utf_encode_byte(u, 0);
        u >>= 6;
    }
    c[0] = utf_encode_byte(u, len);
    return len;
}

struct gui_color
gui_make_color(gui_byte r, gui_byte g, gui_byte b, gui_byte a)
{
    struct gui_color col;
    col.r = r; col.g = g;
    col.b = b; col.a = a;
    return col;
}

struct gui_vec2
gui_make_vec2(gui_float x, gui_float y)
{
    struct gui_vec2 vec;
    vec.x = x; vec.y = y;
    return vec;
}

void
gui_default_config(struct gui_config *config)
{
    if (!config) return;
    config->global_alpha = 1.0f;
    config->header_height = 50.0f;
    config->scrollbar_width = 16;
    config->scroll_factor = 2;
    config->panel_padding = gui_make_vec2(10.0f, 10.0f);
    config->panel_min_size = gui_make_vec2(32.0f, 32.0f);
    config->item_spacing = gui_make_vec2(8.0f, 4.0f);
    config->item_padding = gui_make_vec2(4.0f, 4.0f);
    config->colors[GUI_COLOR_TEXT] = gui_make_color(255, 255, 255, 255);
    config->colors[GUI_COLOR_PANEL] = gui_make_color(45, 45, 45, 255);
    config->colors[GUI_COLOR_BORDER] = gui_make_color(0, 0, 0, 255);
    config->colors[GUI_COLOR_TITLEBAR] = gui_make_color(45, 45, 45, 255);
    config->colors[GUI_COLOR_BUTTON] = gui_make_color(45, 45, 45, 255);
    config->colors[GUI_COLOR_BUTTON_HOVER] = gui_make_color(100, 100, 100, 255);
    config->colors[GUI_COLOR_BUTTON_BORDER] = gui_make_color(100, 100, 100, 255);
    config->colors[GUI_COLOR_TOGGLE] = gui_make_color(100, 100, 100, 255);
    config->colors[GUI_COLOR_TOGGLE_ACTIVE] = gui_make_color(45, 45, 45, 255);
    config->colors[GUI_COLOR_SCROLL] = gui_make_color(100, 100, 100, 255);
    config->colors[GUI_COLOR_SCROLL_CURSOR] = gui_make_color(45, 45, 45, 255);
    config->colors[GUI_COLOR_SLIDER] = gui_make_color(100, 100, 100, 255);
    config->colors[GUI_COLOR_SLIDER_CURSOR] = gui_make_color(45, 45, 45, 255);
    config->colors[GUI_COLOR_PROGRESS] = gui_make_color(100, 100, 100, 255);
    config->colors[GUI_COLOR_PROGRESS_CURSOR] = gui_make_color(45, 45, 45, 255);
    config->colors[GUI_COLOR_INPUT] = gui_make_color(100, 100, 100, 100);
    config->colors[GUI_COLOR_INPUT_BORDER] = gui_make_color(45, 45, 45, 255);
    config->colors[GUI_COLOR_HISTO] = gui_make_color(100, 100, 100, 255);
    config->colors[GUI_COLOR_HISTO_BARS] = gui_make_color(45, 45, 45, 255);
    config->colors[GUI_COLOR_HISTO_NEGATIVE] = gui_make_color(255, 255, 255, 255);
    config->colors[GUI_COLOR_HISTO_HIGHLIGHT] = gui_make_color(255, 0, 0, 255);
    config->colors[GUI_COLOR_PLOT] = gui_make_color(100, 100, 100, 255);
    config->colors[GUI_COLOR_PLOT_LINES] = gui_make_color(45, 45, 45, 255);
}

void
gui_input_begin(struct gui_input *in)
{
    gui_size i;
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
    vec2_load(in->mouse_pos, (gui_float)x, (gui_float)y);
}

void
gui_input_key(struct gui_input *in, enum gui_keys key, gui_int down)
{
    if (!in) return;
    if (in->keys[key].down == down) return;
    in->keys[key].down = down;
    in->keys[key].clicked++;
}

void
gui_input_button(struct gui_input *in, gui_int x, gui_int y, gui_int down)
{
    if (!in) return;
    if (in->mouse_down == down) return;
    vec2_load(in->mouse_clicked_pos, (gui_float)x, (gui_float)y);
    in->mouse_down = down;
    in->mouse_clicked++;
}

void
gui_input_char(struct gui_input *in, gui_glyph glyph)
{
    gui_size len = 0;
    gui_long unicode;
    if (!in) return;
    len = utf_decode(glyph, &unicode, GUI_UTF_SIZE);
    if (len && ((in->text_len + len) < GUI_INPUT_MAX)) {
        utf_encode(unicode, &in->text[in->text_len], GUI_INPUT_MAX - in->text_len);
        in->text_len += len;
    }
}

void
gui_input_end(struct gui_input *in)
{
    if (!in) return;
    vec2_sub(in->mouse_delta, in->mouse_pos, in->mouse_prev);
}

static gui_size
gui_font_text_width(const struct gui_font *font, const gui_char *t, gui_size l)
{
    gui_long unicode;
    gui_size len = 0;
    const struct gui_font_glyph *g;
    gui_size text_len;

    if (!t || !l) return 0;
    text_len = utf_decode(t, &unicode, l);
    while (text_len < l) {
        if (unicode == UTF_INVALID) return 0;
        g = (unicode < font->glyph_count) ? &font->glyphes[unicode] : font->fallback;
        g = (g->code == 0) ? font->fallback : g;
        len += (gui_size)(g->xadvance * font->scale);
        text_len += utf_decode(t + text_len, &unicode, l - text_len);
    }
    return len;
}

void
gui_begin(struct gui_draw_buffer *buffer, gui_byte *memory, gui_size size)
{
    if (!buffer || !memory || !size) return;
    buffer->begin = memory + size - sizeof(struct gui_draw_command);
    buffer->vertexes = memory;
    buffer->end = buffer->begin;
    buffer->memory = memory;
    buffer->size = size;
    buffer->vertex_write = 0;
    buffer->vertex_count = 0;
    buffer->command_count = 0;
    buffer->allocated = 0;
}

gui_size
gui_end(struct gui_draw_buffer *buffer)
{
    gui_size allocated;
    if (!buffer) return 0;
    allocated = buffer->allocated;
    buffer->allocated = 0;
    buffer->vertex_count = 0;
    return allocated;
}

static gui_int
gui_push(struct gui_draw_buffer *buffer, gui_size count,
    const struct gui_rect *rect, gui_texture tex)
{
    gui_size i;
    gui_byte *end;
    gui_size cmd_memory = 0;
    gui_size vertex_size;
    gui_size current;
    struct gui_draw_command cmd;
    if (!buffer || !rect) return gui_false;
    if (!buffer->end || !buffer->begin || buffer->end > buffer->begin)
        return gui_false;
    vertex_size = sizeof(struct gui_vertex) * (buffer->vertex_count + count);
    if (buffer->vertexes + vertex_size >= buffer->end)
        return gui_false;

    cmd.vertex_count = count;
    cmd.clip_rect = *rect;
    cmd.texture = tex;
    memcopy(buffer->end, &cmd, sizeof(struct gui_draw_command));

    buffer->end -= sizeof(struct gui_draw_command);
    buffer->vertex_count += count;
    buffer->allocated += count * sizeof(struct gui_vertex);
    buffer->allocated += sizeof(struct gui_draw_command);
    buffer->command_count++;
    return gui_true;
}

gui_int
gui_get_command(struct gui_draw_command *cmd, const struct gui_draw_buffer *buffer,
    gui_size index)
{
    gui_byte *iter;
    gui_size size = 0;
    if (!buffer || !buffer->memory || !buffer->begin || !buffer->end || !buffer->size)
        return gui_false;
    if (!cmd || buffer->begin == buffer->end)
        return gui_false;
    if (index >= buffer->command_count)
        return gui_false;
    iter = buffer->begin - (index * sizeof(struct gui_draw_command));
    memcopy(cmd, iter, sizeof(struct gui_draw_command));
    return gui_true;
}

static void
gui_vertex(struct gui_draw_buffer *buffer, gui_float x, gui_float y,
    struct gui_color col, gui_float u, gui_float v)
{
    gui_size i;
    gui_byte *dst;
    struct gui_vertex vertex;
    if (!buffer) return;
    i = buffer->vertex_write;
    if (i >= buffer->vertex_count) return;

    vertex.pos.x = x;
    vertex.pos.y = y;
    vertex.color = col;
    vertex.uv.u = u;
    vertex.uv.v = v;

    dst = buffer->vertexes + i * sizeof(struct gui_vertex);
    memcopy(dst, &vertex, sizeof(struct gui_vertex));
    buffer->vertex_write++;
}

static void
gui_vertex_line(struct gui_draw_buffer* buffer, gui_float x0, gui_float y0,
    gui_float x1, gui_float y1, struct gui_color col)
{
    gui_float len;
    struct gui_vec2 a, b, d;
    struct gui_vec2 hn, hp0, hp1;
    if (!buffer) return;

    vec2_load(a, x0, y0);
    vec2_load(b, x1, y1);
    vec2_sub(d, b, a);

    len = 0.5f / vec2_len(d);
    vec2_muls(hn, d, len);
    vec2_load(hp0, +hn.y, -hn.x);
    vec2_load(hp1, -hn.y, +hn.x);

    gui_vertex(buffer, a.x + hp0.x, a.y + hp0.y, col, 0.0f, 0.0f);
    gui_vertex(buffer, b.x + hp0.x, b.y + hp0.y, col, 0.0f, 0.0f);
    gui_vertex(buffer, a.x + hp1.x, a.y + hp1.y, col, 0.0f, 0.0f);
    gui_vertex(buffer, b.x + hp0.x, b.y + hp0.y, col, 0.0f, 0.0f);
    gui_vertex(buffer, b.x + hp1.x, b.y + hp1.y, col, 0.0f, 0.0f);
    gui_vertex(buffer, a.x + hp1.x, a.y + hp1.y, col, 0.0f, 0.0f);
}

static void
gui_line(struct gui_draw_buffer *buffer, gui_float x0, gui_float y0,
    gui_float x1, gui_float y1, struct gui_color col)
{
    struct gui_draw_command *cmd;
    if (!buffer) return;
    if (col.a == 0) return;
    if (!gui_push(buffer, 6, &null_rect, null_tex)) return;
    gui_vertex_line(buffer, x0, y0, x1, y1, col);
}

static void
gui_trianglef(struct gui_draw_buffer *buffer, gui_float x0, gui_float y0,
    gui_float x1, gui_float y1, gui_float x2, gui_float y2, struct gui_color c)
{
    struct gui_draw_command *cmd;
    if (!buffer) return;
    if (c.a == 0) return;
    if (!gui_push(buffer, 3, &null_rect, null_tex)) return;
    gui_vertex(buffer, x0, y0, c, 0.0f, 0.0f);
    gui_vertex(buffer, x1, y1, c, 0.0f, 0.0f);
    gui_vertex(buffer, x2, y2, c, 0.0f, 0.0f);
}

static void
gui_rect(struct gui_draw_buffer *buffer, gui_float x, gui_float y,
    gui_float w, gui_float h, struct gui_color col)
{
    struct gui_draw_command *cmd;
    if (!buffer) return;
    if (col.a == 0) return;
    if (!gui_push(buffer, 6 * 4, &null_rect, null_tex)) return;
    gui_vertex_line(buffer, x, y, x + w, y, col);
    gui_vertex_line(buffer, x + w, y, x + w, y + h, col);
    gui_vertex_line(buffer, x + w, y + h, x, y + h, col);
    gui_vertex_line(buffer, x, y + h, x, y, col);
}

static void
gui_rectf(struct gui_draw_buffer *buffer, gui_float x, gui_float y,
    gui_float w, gui_float h, struct gui_color col)
{
    struct gui_draw_command *cmd;
    if (!buffer) return;
    if (col.a == 0) return;
    if (!gui_push(buffer, 6, &null_rect, null_tex)) return;

    gui_vertex(buffer, x, y, col, 0.0f, 0.0f);
    gui_vertex(buffer, x + w, y, col, 0.0f, 0.0f);
    gui_vertex(buffer, x + w, y + h, col, 0.0f, 0.0f);
    gui_vertex(buffer, x, y, col, 0.0f, 0.0f);
    gui_vertex(buffer, x + w, y + h, col, 0.0f, 0.0f);
    gui_vertex(buffer, x, y + h, col, 0.0f, 0.0f);
}

static void
gui_string(struct gui_draw_buffer *buffer, const struct gui_font *font, gui_float x,
        gui_float y, gui_float w, gui_float h,
        struct gui_color col, const gui_char *t, gui_size len)
{
    struct gui_draw_command *cmd;
    struct gui_rect clip;
    gui_size text_len;
    gui_long unicode;
    const struct gui_font_glyph *g;

    if (!buffer || !t || !font || !len) return;
    clip.x = x; clip.y = y;
    clip.w = w; clip.h = h;
    if (!gui_push(buffer, 6 * len, &clip, font->texture)) return;

    text_len = utf_decode(t, &unicode, len);
    while (text_len <= len) {
        gui_float x1, y1, x2, y2, char_width = 0;
        if (unicode == UTF_INVALID) break;
        g = (unicode < font->glyph_count) ?
            &font->glyphes[unicode] :
            font->fallback;
        g = (g->code == 0) ? font->fallback : g;

        y1 = (gui_float)(y + (g->yoff * font->scale));
        y2 = (gui_float)(y1 + (gui_float)g->height * font->scale);
        x1 = (gui_float)(x + g->xoff * font->scale);
        x2 = (gui_float)(x1 + (gui_float)g->width * font->scale);
        char_width = g->xadvance * font->scale;

        gui_vertex(buffer, x1, y1, col, g->uv[0].u, g->uv[0].v);
        gui_vertex(buffer, x2, y1, col, g->uv[1].u, g->uv[0].v);
        gui_vertex(buffer, x2, y2, col, g->uv[1].u, g->uv[1].v);
        gui_vertex(buffer, x1, y1, col, g->uv[0].u, g->uv[0].v);
        gui_vertex(buffer, x2, y2, col, g->uv[1].u, g->uv[1].v);
        gui_vertex(buffer, x1, y2, col, g->uv[0].u, g->uv[1].v);
        text_len += utf_decode(t + text_len, &unicode, len - text_len);
        x += char_width;
    }
}

gui_int
gui_button(struct gui_draw_buffer *buffer, const struct gui_button *button,
    const struct gui_font *font, const struct gui_input *in)
{
    gui_size text_width;
    gui_float label_x, label_y;
    gui_float label_w, label_h;
    gui_int ret = gui_false;
    gui_float mouse_x, mouse_y;
    gui_float clicked_x, clicked_y;
    struct gui_color fc, bg, hc;
    gui_float x, y, w, h;
    const gui_char *t;

    if (!buffer || !in || !button)
        return gui_false;

    x = button->x, y = button->y;
    w = button->w, h = button->h;
    t = (const gui_char*)button->text;
    mouse_x = in->mouse_pos.x;
    mouse_y = in->mouse_pos.y;
    clicked_x = in->mouse_clicked_pos.x;
    clicked_y = in->mouse_clicked_pos.y;
    fc = button->font;
    bg = button->background;
    hc = button->highlight;

    if (INBOX(mouse_x, mouse_y, x, y, x+w, y+h)) {
        if (INBOX(clicked_x, clicked_y, x, y, x+w, y+h))
            ret = (in->mouse_down && in->mouse_clicked);
        fc = bg; bg = hc;
    }

    gui_rectf(buffer, x, y, w, h, bg);
    gui_rect(buffer, x+1, y+1, w-1, h-1, button->foreground);
    if (!font || !button->text || !button->length) return ret;

    text_width = gui_font_text_width(font, t, button->length);
    if (text_width > (w - 2.0f * button->pad_x)) {
        label_x = x + button->pad_x;
        label_y = y + button->pad_y;
        label_w = w - 2 * button->pad_x;
        label_h = h - 2 * button->pad_y;
    } else {
        label_w = w - 2 * button->pad_x;
        label_h = h - 2 * button->pad_y;
        label_x = x + (label_w - (gui_float)text_width) / 2;
        label_y = y + (label_h - font->height) / 2;
    }
    gui_string(buffer, font, label_x, label_y, label_w, label_h, fc, t, button->length);
    return ret;
}

gui_int
gui_toggle(struct gui_draw_buffer *buffer, const struct gui_toggle *toggle,
    const struct gui_font *font, const struct gui_input *in)
{
    gui_float select_size;
    gui_float x, y, w, h;
    gui_int active;
    gui_float select_x, select_y;
    gui_float cursor_x, cursor_y;
    gui_float cursor_pad, cursor_size;
    gui_float label_x, label_w;
    struct gui_color fg, bg;
    const gui_char *t;

    if (!buffer || !toggle || !font || !in) return 0;
    active = toggle->active;
    t = (const gui_char*)toggle->text;
    x = toggle->x, y = toggle->y;
    w = toggle->w, h = toggle->h;
    w = MAX(w, font->height + 2 * toggle->pad_x);
    h = MAX(h, font->height + 2 * toggle->pad_y);
    fg = toggle->foreground;
    bg = toggle->background;

    select_x = x + toggle->pad_x;
    select_y = y + toggle->pad_y;
    select_size = font->height;

    cursor_pad = select_size / 8;
    cursor_x = select_x + cursor_pad;
    cursor_y = select_y + cursor_pad;
    cursor_size = select_size - 2 * cursor_pad;

    label_x = x + select_size + toggle->pad_x * 2;
    label_w = w - select_size + 3 * toggle->pad_x;

    if (!in->mouse_down && in->mouse_clicked) {
        const gui_float clicked_x = in->mouse_clicked_pos.x;
        const gui_float clicked_y = in->mouse_clicked_pos.y;
        const gui_float cursor_px = cursor_x + cursor_size;
        const gui_float cursor_py = cursor_y + cursor_size;
        if (INBOX(clicked_x, clicked_y, cursor_x, cursor_y, cursor_px, cursor_py))
            active = !active;
    }

    gui_rectf(buffer, select_x, select_y, select_size, select_size, bg);
    if (active) gui_rectf(buffer, cursor_x, cursor_y, cursor_size, cursor_size, fg);
    gui_string(buffer, font, label_x, y + toggle->pad_y, label_w,
        h - 2 * toggle->pad_y, toggle->font, t, toggle->length);
    return active;
}

gui_float
gui_slider(struct gui_draw_buffer *buffer, const struct gui_slider *slider,
    const struct gui_input *in)
{
    gui_float min, max, value, step;
    gui_float x, y, w, h;
    gui_float mouse_x, mouse_y;
    gui_float clicked_x, clicked_y;
    gui_float cursor_x, cursor_y, cursor_h;
    gui_float cursor_w;

    if (!buffer || !slider || !in) return 0;
    x = slider->x; y = slider->y;
    w = MAX(slider->w, 2 * slider->pad_x);
    h = MAX(slider->h, 2 * slider->pad_y);
    max = MAX(slider->min, slider->max);
    min = MIN(slider->min, slider->max);
    value = CLAMP(min, slider->value, max);
    step = slider->step;

    cursor_w = (w - 2 * slider->pad_x) / (((max - min) + step) / step);
    cursor_h = h - 2 * slider->pad_y;
    cursor_x = x + slider->pad_x + (cursor_w * (value - min));
    cursor_y = y + slider->pad_y;

    if (!buffer || !in) return 0;
    mouse_x = in->mouse_pos.x;
    mouse_y = in->mouse_pos.y;
    clicked_x = in->mouse_clicked_pos.x;
    clicked_y = in->mouse_clicked_pos.y;

    if (in->mouse_down &&
        INBOX(clicked_x, clicked_y, x, y, x + w, y + h) &&
        INBOX(mouse_x, mouse_y, x, y, x + w, y + h))
    {
        gui_float cursor_next_x;
        const gui_float tmp = mouse_x - cursor_x + (cursor_w / 2);
        gui_float next_value = value + step * ((tmp < 0) ? -1.0f : 1.0f);
        next_value = CLAMP(min, next_value, max);
        cursor_next_x = x + slider->pad_x + (cursor_w * (next_value - min));
        if (INBOX(mouse_x, mouse_y, cursor_next_x, cursor_y,
                cursor_next_x + cursor_w, cursor_y + cursor_h))
        {
            value = next_value;
            cursor_x = cursor_next_x;
        }
    }
    gui_rectf(buffer, x, y, w, h, slider->background);
    gui_rectf(buffer, cursor_x, cursor_y, cursor_w, cursor_h, slider->foreground);
    return value;
}

gui_size
gui_progress(struct gui_draw_buffer *buffer, const struct gui_progress *prog,
    const struct gui_input *in)
{
    gui_float scale;
    gui_float mouse_x, mouse_y;
    gui_float cursor_x, cursor_y;
    gui_float cursor_w, cursor_h;
    gui_float x, y, w, h;
    gui_size value;

    if (!buffer || !in || !prog) return 0;
    mouse_x = in->mouse_pos.x;
    mouse_y = in->mouse_pos.y;

    x = prog->x; y = prog->y;
    w = MAX(prog->w, 2 * prog->pad_x + 1);
    h = MAX(prog->h, 2 * prog->pad_y + 1);
    value = MIN(prog->current, prog->max);

    if (prog->modifyable && in->mouse_down && INBOX(mouse_x, mouse_y, x, y, x+w, y+h)) {
        gui_float ratio = (gui_float)(mouse_x - x) / (gui_float)w;
        value = (gui_size)((gui_float)prog->max * ratio);
    }

    if (!prog->max) return value;
    value = MIN(value, prog->max);
    scale = (gui_float)value / (gui_float)prog->max;
    cursor_h = h - 2 * prog->pad_y;
    cursor_w = (w - 2 * prog->pad_x) * scale;
    cursor_x = x + prog->pad_x;
    cursor_y = y + prog->pad_y;
    gui_rectf(buffer, x, y, w, h, prog->background);
    gui_rectf(buffer, cursor_x, cursor_y, cursor_w, cursor_h, prog->foreground);
    return value;
}

gui_int
gui_input(struct gui_draw_buffer *buffer, const struct gui_input_field *input,
    const struct gui_font *font, const struct gui_input *in)
{
    gui_size offset = 0;
    gui_float label_x, label_y;
    gui_float label_w, label_h;
    gui_float x, y, w, h;
    gui_int active;
    const gui_char *t;
    gui_float mouse_x, mouse_y;
    gui_size text_width;

    if (!buffer || !in || !font || !input)
        return 0;

    mouse_x = in->mouse_clicked_pos.x;
    mouse_y = in->mouse_clicked_pos.y;
    x = input->x, y = input->y;
    w = MAX(input->w, 2 * input->pad_x);
    h = MAX(input->h, font->height);
    active = input->active;
    t = input->buffer;

    if (in->mouse_clicked && in->mouse_down)
        active = INBOX(mouse_x, mouse_y, x, y, x + w, y + h);
    if (active) {
        const struct gui_key *del = &in->keys[GUI_KEY_DEL];
        const struct gui_key *bs = &in->keys[GUI_KEY_BACKSPACE];
        const struct gui_key *enter = &in->keys[GUI_KEY_ENTER];
        const struct gui_key *esc = &in->keys[GUI_KEY_ESCAPE];
        if (in->text_len && *input->length < input->max) {
            gui_long unicode;
            gui_size i = 0, l = 0;
            gui_size ulen = utf_decode(in->text, &unicode, in->text_len);
            while (ulen && (l + ulen) <= in->text_len && *input->length < input->max) {
                for (i = 0; i < ulen; i++)
                    input->buffer[(*input->length)++] = in->text[l + i];
                l = l + ulen;
                ulen = utf_decode(in->text + l, &unicode, in->text_len - l);
            }
        }
        if ((del->down && del->clicked) || (bs->down && bs->clicked))
            if (*input->length > 0) *input->length = *input->length - 1;
        if ((enter->down && enter->clicked) || (esc->down && esc->clicked))
            active = gui_false;
    }

    label_x = x + input->pad_x;
    label_y = y + input->pad_y;
    label_w = w - 2 * input->pad_x;
    label_h = h - 2 * input->pad_y;
    text_width = gui_font_text_width(font, t, *input->length);
    while (text_width > label_w) {
        offset += 1;
        text_width = gui_font_text_width(font, &t[offset], *input->length - offset);
    }

    gui_rectf(buffer, x, y, w, h, input->background);
    gui_rect(buffer, x + 1, y, w - 1, h, input->foreground);
    gui_string(buffer, font, label_x, label_y, label_w, label_h, input->font,
        &t[offset], *input->length);
    return active;
}

void
gui_plot(struct gui_draw_buffer *buffer, const struct gui_plot *plot)
{
    gui_size i;
    gui_float last_x;
    gui_float last_y;
    gui_float max, min;
    gui_float range, ratio;
    gui_size step;
    gui_float canvas_x, canvas_y;
    gui_float canvas_w, canvas_h;
    gui_float x, y, w, h;

    if (!buffer || !plot) return;

    x = plot->x, y = plot->y;
    w = plot->w, h = plot->h;
    gui_rectf(buffer, x, y, w, h, plot->background);
    if (!plot->value_count) return;

    max = plot->values[0], min = plot->values[0];
    for (i = 0; i < plot->value_count; ++i) {
        if (plot->values[i] > max)
            max = plot->values[i];
        if (plot->values[i] < min)
            min = plot->values[i];
    }
    range = max - min;

    canvas_x = x + plot->pad_x;
    canvas_y = y + plot->pad_y;
    canvas_w = MAX(1 + 2 * plot->pad_x, w - 2 * plot->pad_x);
    canvas_h = MAX(1 + 2 * plot->pad_y, h - 2 * plot->pad_y);
    step = (gui_size)canvas_w / plot->value_count;

    ratio = (plot->values[0] - min) / range;
    last_x = canvas_x;
    last_y = (canvas_y + canvas_h) - ratio * (gui_float)canvas_h;
    gui_rectf(buffer, last_x - 3, last_y - 3, 6, 6, plot->foreground);
    for (i = 1; i < plot->value_count; i++) {
        gui_float cur_x, cur_y;
        ratio = (plot->values[i] - min) / range;
        cur_x = canvas_x + (gui_float)(step * i);
        cur_y = (canvas_y + canvas_h) - (ratio * (gui_float)canvas_h);
        gui_line(buffer, last_x, last_y, cur_x, cur_y, plot->foreground);
        gui_rectf(buffer, cur_x - 3, cur_y - 3, 6, 6, plot->foreground);
        last_x = cur_x;
        last_y = cur_y;
    }
}

gui_int
gui_histo(struct gui_draw_buffer *buffer, const struct gui_histo *histo,
    const struct gui_input *in)
{
    gui_size i;
    gui_float max;
    gui_float canvas_x, canvas_y;
    gui_float canvas_w, canvas_h;
    gui_float item_w = 0.0f;
    gui_int selected = -1;
    gui_float x, y, w, h;
    struct gui_color fg, nc;

    if (!buffer || !histo || !in)
        return selected;

    x = histo->x, y = histo->y;
    w = histo->w, h = histo->h;
    fg = histo->foreground;
    nc = histo->negative;
    max = histo->values[0];
    for (i = 0; i < histo->value_count; ++i) {
        if (ABS(histo->values[i]) > max)
            max = histo->values[i];
    }

    gui_rectf(buffer, x, y, w, h, histo->background);
    canvas_x = x + histo->pad_x;
    canvas_y = y + histo->pad_y;
    canvas_w = MAX(0, w - 2 * histo->pad_x);
    canvas_h = MAX(0, h - 2 * histo->pad_y);
    if (histo->value_count) {
        gui_float padding = (gui_float)(histo->value_count-1) * histo->pad_x;
        item_w = (canvas_w - padding) / (gui_float)(histo->value_count);
    }

    for (i = 0; i < histo->value_count; i++) {
        const gui_float j = (gui_float)i;
        const gui_float mouse_x = in->mouse_pos.x;
        const gui_float mouse_y = in->mouse_pos.y;
        const gui_float ratio = ABS(histo->values[i]) / max;
        const gui_float item_h = canvas_h * ratio;
        const gui_float item_x = canvas_x + (j * item_w) + (j * histo->pad_y);
        const gui_float item_y = (canvas_y + canvas_h) - item_h;
        struct gui_color col = (histo->values[i] < 0) ? nc: fg;
        if (INBOX(mouse_x, mouse_y, item_x, item_y, item_x + item_w, item_y + item_h)) {
            selected = (in->mouse_down && in->mouse_clicked) ? (gui_int)i : -1;
            col = histo->highlight;
        }
        gui_rectf(buffer, item_x, item_y, item_w, item_h, col);
    }
    return selected;
}

gui_size
gui_scroll(struct gui_draw_buffer *buffer, const struct gui_scroll *scroll,
    const struct gui_input *in)
{
    gui_float mouse_x, mouse_y;
    gui_float prev_x, prev_y;
    gui_float x, y, w, h;
    gui_size step, offset;

    gui_bool u, d;
    gui_float button_size, button_half, button_y;
    gui_float bar_h, bar_y;
    struct gui_button up, down;
    struct gui_color fg, bg;
    gui_size target;

    gui_float ratio;
    gui_float off;
    gui_float cursor_x, cursor_y;
    gui_float cursor_w, cursor_h;
    gui_float cursor_px, cursor_py;
    gui_bool inscroll, incursor;

    gui_float pad;
    gui_float xoff, yoff, boff;
    gui_float xpad, ypad, xmid;

    if (!buffer || !in) return 0;
    x = scroll->x, y = scroll->y;
    w = scroll->w, h = scroll->h;
    target = scroll->target;
    fg = scroll->foreground;
    bg = scroll->background;
    gui_rectf(buffer, x, y, w, h, scroll->background);
    if (scroll->target <= h) return 0;

    mouse_x = in->mouse_pos.x;
    mouse_y = in->mouse_pos.y;
    prev_x = in->mouse_prev.x;
    prev_y = in->mouse_prev.y;

    w = MAX(w, 0);
    h = MAX(h, 2 * w);
    step = MIN(scroll->step, (gui_size)h);

    pad = w / 4;
    button_size = w;
    button_half = button_size / 2;
    button_y = y + h - button_size;
    bar_h = h - 2 * button_size;
    bar_y = y + button_size;
    offset = MIN(scroll->offset, (gui_size)(scroll->target - (gui_size)bar_h));

    ratio = (gui_float)bar_h/(gui_float)scroll->target;
    off = (gui_float)offset/(gui_float)scroll->target;
    cursor_h = ratio * bar_h;
    cursor_y = bar_y + (off * bar_h);
    cursor_w = w;
    cursor_x = x;

    up.x = x, up.y = y;
    up.w = button_size, up.h = button_size;
    up.pad_y = 0, up.pad_x = 0;
    up.text = NULL, up.length = 0;
    up.font = fg, up.background = fg;
    up.foreground = bg, up.highlight = fg;

    down.x = x, down.y = y + h - button_size;
    down.w = button_size, down.h = button_size;
    down.pad_y = 0, down.pad_x = 0;
    down.text = NULL, down.length = 0;
    down.font = fg, down.background = fg;
    down.foreground = bg, down.highlight = fg;

    u = gui_button(buffer, &up, NULL, in);
    d = gui_button(buffer, &down, NULL, in);

    xpad = x + pad;
    ypad = button_y + pad;
    xmid = x + button_half;
    xoff = x + (button_size - pad);
    yoff = y + (button_size - pad);
    boff = button_y + (button_size - pad);

    gui_trianglef(buffer, xmid, y + pad, xoff, yoff, xpad, yoff, scroll->background);
    gui_trianglef(buffer, xpad, ypad, xoff, ypad, xmid, boff, scroll->background);

    cursor_px = cursor_x + cursor_w;
    cursor_py = cursor_y + cursor_h;
    inscroll = INBOX(mouse_x, mouse_y, x, y, x + w, y + h);
    incursor = INBOX(prev_x, prev_y, cursor_x, cursor_y, cursor_px, cursor_py);

    if (in->mouse_down && inscroll && incursor) {
        const gui_float pixel = in->mouse_delta.y;
        const gui_float delta =  (pixel / (gui_float)bar_h) * (gui_float)target;
        offset = MIN(offset + (gui_size)delta, target - (gui_size)bar_h);
        cursor_y += pixel;
    } else if (u || d) {
        offset = (d) ? MIN(offset + step, target - (gui_size)bar_h): MAX(0, offset - step);
        off = (gui_float)offset / (gui_float)scroll->target;
        cursor_y = bar_y + (off * bar_h);
    }

    gui_rectf(buffer, cursor_x, cursor_y, cursor_w, cursor_h, fg);
    gui_rect(buffer, cursor_x+1, cursor_y, cursor_w-1, cursor_h, bg);
    return offset;
}

void
gui_panel_init(struct gui_panel *panel, const struct gui_config *config,
    const struct gui_font *font, const struct gui_input *input)
{
    panel->config = config;
    panel->font = font;
    panel->in = input;
    panel->x = 0;
    panel->y = 0;
    panel->at_y = 0;
    panel->width = 0;
    panel->flags = 0;
    panel->index = 0;
    panel->row_columns = 0;
    panel->row_height = 0;
    panel->out = NULL;
}

gui_int
gui_panel_begin(struct gui_panel *panel, struct gui_draw_buffer *out,
    const char *t, gui_flags f, gui_float x, gui_float y, gui_float w, gui_float h)
{
    const struct gui_config *config = panel->config;
    const struct gui_color *header = &config->colors[GUI_COLOR_TITLEBAR];
    gui_rectf(out, x, y, w, config->header_height, *header);
    if (panel->flags & GUI_PANEL_SCROLLBAR) panel->height = h;

    UNUSED(t);
    panel->out = out;
    panel->x = x;
    panel->y = y;
    panel->width = w;
    panel->at_y = y;
    panel->flags = f;
    panel->index = 0;
    panel->row_columns = 0;
    panel->row_height = config->header_height;
    return 0;
}

void
gui_panel_row(struct gui_panel *panel, gui_float height, gui_size cols)
{
    const struct gui_config *config = panel->config;
    const struct gui_color *color = &config->colors[GUI_COLOR_PANEL];
    panel->at_y += panel->row_height;
    panel->index = 0;
    panel->row_columns = cols;
    panel->row_height = height;
    gui_rectf(panel->out, panel->x, panel->at_y, panel->width, height, *color);
}

static void
gui_panel_alloc(struct gui_rect *bounds, struct gui_panel *panel)
{
    const struct gui_config *config = panel->config;
    gui_float padding, spacing, space;
    gui_float item_offset, item_width, item_spacing;
    if (panel->index >= panel->row_columns)
        gui_panel_row(panel, panel->row_height, panel->row_columns);

    padding = 2.0f * config->panel_padding.x;
    spacing = (gui_float)(panel->row_columns - 1) * config->item_spacing.x;
    space  = panel->width - padding - spacing;

    item_width = space / (gui_float)panel->row_columns;
    item_offset = (gui_float)panel->index * item_width;
    item_spacing = (gui_float)panel->index * config->item_spacing.x;

    bounds->x = panel->x + item_offset + item_spacing + config->panel_padding.x;
    bounds->y = panel->at_y;
    bounds->w = item_width;
    bounds->h = panel->row_height - config->item_spacing.y;
    panel->index++;
}

gui_int
gui_panel_button(struct gui_panel *panel, const char *str, gui_size len)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config = panel->config;
    gui_panel_alloc(&bounds, panel);

    button.text = str;
    button.length = len;
    button.x = bounds.x;
    button.y = bounds.y;
    button.w = bounds.w;
    button.h = bounds.h;
    button.pad_x = config->item_padding.x;
    button.pad_y = config->item_padding.y;
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
    button.font = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
    return gui_button(panel->out, &button, panel->font, panel->in);
}

gui_int
gui_panel_toggle(struct gui_panel *panel, const char *text, gui_size length,
    gui_int is_active)
{
    struct gui_rect bounds;
    struct gui_toggle toggle;
    const struct gui_config *config = panel->config;
    gui_panel_alloc(&bounds, panel);

    toggle.x = bounds.x;
    toggle.y = bounds.y;
    toggle.w = bounds.w;
    toggle.h = bounds.h;
    toggle.pad_x = config->item_padding.x;
    toggle.pad_y = config->item_padding.y;
    toggle.active = is_active;
    toggle.text = text;
    toggle.length = length;
    toggle.font = config->colors[GUI_COLOR_TEXT];
    toggle.background = config->colors[GUI_COLOR_TOGGLE];
    toggle.foreground = config->colors[GUI_COLOR_TOGGLE_ACTIVE];
    return gui_toggle(panel->out, &toggle, panel->font, panel->in);
}

gui_float
gui_panel_slider(struct gui_panel *panel, gui_float min_value, gui_float value,
    gui_float max_value, gui_float value_step)
{
    struct gui_rect bounds;
    struct gui_slider slider;
    const struct gui_config *config = panel->config;
    gui_panel_alloc(&bounds, panel);

    slider.x = bounds.x;
    slider.y = bounds.y;
    slider.w = bounds.w;
    slider.h = bounds.h;
    slider.pad_x = config->item_padding.x;
    slider.pad_y = config->item_padding.y;
    slider.value = value;
    slider.min = min_value;
    slider.max = max_value;
    slider.step = value_step;
    slider.background = config->colors[GUI_COLOR_SLIDER];
    slider.foreground = config->colors[GUI_COLOR_SLIDER_CURSOR];
    return gui_slider(panel->out, &slider, panel->in);
}

gui_size
gui_panel_progress(struct gui_panel *panel, gui_size cur_value, gui_size max_value,
    gui_bool is_modifyable)
{
    struct gui_rect bounds;
    struct gui_progress prog;
    const struct gui_config *config = panel->config;
    gui_panel_alloc(&bounds, panel);

    prog.x = bounds.x;
    prog.y = bounds.y;
    prog.w = bounds.w;
    prog.h = bounds.h;
    prog.pad_x = config->item_padding.x;
    prog.pad_y = config->item_padding.y;
    prog.current = cur_value;
    prog.max = max_value;
    prog.modifyable = is_modifyable;
    prog.background = config->colors[GUI_COLOR_PROGRESS];
    prog.foreground = config->colors[GUI_COLOR_PROGRESS_CURSOR];
    return gui_progress(panel->out, &prog, panel->in);
}

gui_int
gui_panel_input(struct gui_panel *panel, gui_char *buffer, gui_size *length,
    gui_size max_length, gui_bool is_active)
{
    struct gui_rect bounds;
    struct gui_input_field field;
    const struct gui_config *config = panel->config;
    gui_panel_alloc(&bounds, panel);

    field.x = bounds.x;
    field.y = bounds.y;
    field.w = bounds.w;
    field.h = bounds.h;
    field.pad_x = config->item_padding.x;
    field.pad_y = config->item_padding.y;
    field.buffer = buffer;
    field.length = length;
    field.max  = max_length;
    field.active = is_active;
    field.font = config->colors[GUI_COLOR_TEXT];
    field.background = config->colors[GUI_COLOR_INPUT];
    field.foreground = config->colors[GUI_COLOR_BORDER];
    return gui_input(panel->out, &field, panel->font, panel->in);
}

void
gui_panel_plot(struct gui_panel *panel, const gui_float *values, gui_size count)
{
    struct gui_rect bounds;
    struct gui_plot plot;
    const struct gui_config *config = panel->config;
    gui_panel_alloc(&bounds, panel);

    plot.x = bounds.x;
    plot.y = bounds.y;
    plot.w = bounds.w;
    plot.h = bounds.h;
    plot.pad_x = config->item_padding.x;
    plot.pad_y = config->item_padding.y;
    plot.value_count = count;
    plot.values = values;
    plot.background = config->colors[GUI_COLOR_PLOT];
    plot.foreground = config->colors[GUI_COLOR_PLOT_LINES];
    gui_plot(panel->out, &plot);
}

gui_int
gui_panel_histo(struct gui_panel *panel, const gui_float *values, gui_size count)
{
    struct gui_rect bounds;
    struct gui_histo histo;
    const struct gui_config *config = panel->config;
    gui_panel_alloc(&bounds, panel);

    histo.x = bounds.x;
    histo.y = bounds.y;
    histo.w = bounds.w;
    histo.h = bounds.h;
    histo.pad_x = config->item_padding.x;
    histo.pad_y = config->item_padding.y;
    histo.value_count = count;
    histo.values = values;
    histo.background = config->colors[GUI_COLOR_HISTO];
    histo.foreground = config->colors[GUI_COLOR_HISTO_BARS];
    histo.negative = config->colors[GUI_COLOR_HISTO_NEGATIVE];
    histo.highlight = config->colors[GUI_COLOR_HISTO_HIGHLIGHT];
    return gui_histo(panel->out, &histo, panel->in);
}

void gui_panel_end(struct gui_panel *panel)
{
    if (!(panel->flags & GUI_PANEL_SCROLLBAR))
        panel->height = panel->at_y - panel->y;
}

