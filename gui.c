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
#define INBOX(px, py, x, y, w, h) (BETWEEN(px, x, x+w) && BETWEEN(py, y, y+h))
#define ALIGNOF(t) ((char*)(&((struct {char c; t _h;}*)0)->_h) - (char*)0)
#define ALIGN(x, mask) (void*)((unsigned long)((gui_byte*)(x) + (mask-1)) & ~(mask-1))

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
    val.i = 0x5f375a86 - (val.i >> 1);
    val.f = val.f * (1.5f - xhalf * val.f * val.f);
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

static struct gui_color
gui_make_color(gui_byte r, gui_byte g, gui_byte b, gui_byte a)
{
    struct gui_color col;
    col.r = r; col.g = g;
    col.b = b; col.a = a;
    return col;
}

static struct gui_vec2
gui_make_vec2(gui_float x, gui_float y)
{
    struct gui_vec2 vec;
    vec.x = x; vec.y = y;
    return vec;
}

static void
gui_triangle_from_direction(struct gui_vec2 *result, gui_float x, gui_float y,
    gui_float w, gui_float h, gui_float pad_x, gui_float pad_y,
    enum gui_direction direction)
{
    gui_float w_half, h_half;
    w = MAX(4 * pad_x, w);
    h = MAX(4 * pad_y, h);
    w = w - 2 * pad_x;
    h = h - 2 * pad_y;
    w_half = w / 2.0f;
    h_half = h / 2.0f;
    x = x + pad_x;
    y = y + pad_y;
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
    gui_size text_width = 0;
    const struct gui_font_glyph *glyph;
    gui_size text_len = 0;
    gui_size glyph_len;

    if (!t || !l) return 0;
    glyph_len = utf_decode(t, &unicode, l);
    while (text_len <= l && glyph_len) {
        if (unicode == UTF_INVALID) return 0;
        glyph = (unicode < font->glyph_count) ? &font->glyphes[unicode] : font->fallback;
        glyph = (glyph->code == 0) ? font->fallback : glyph;
        text_width += (gui_size)(glyph->xadvance * font->scale);
        glyph_len = utf_decode(t + text_len, &unicode, l - text_len);
        text_len += glyph_len;
    }
    return text_width;
}

static gui_size
gui_font_chars_in_space(const struct gui_font *font, const gui_char *text, gui_size len,
    gui_float space)
{
    gui_size chars = 0;
    gui_long unicode;
    gui_size text_width = 0;
    const struct gui_font_glyph *glyph;
    gui_size text_len = 0;
    gui_size glyph_len;

    if (!text || !len) return 0;
    glyph_len = utf_decode(text, &unicode, len);
    while (text_len <= len && glyph_len) {
        if (unicode == UTF_INVALID) return 0;
        glyph = (unicode < font->glyph_count) ? &font->glyphes[unicode] : font->fallback;
        glyph = (glyph->code == 0) ? font->fallback : glyph;
        text_width += (gui_size)(glyph->xadvance * font->scale);
        if (text_width >= space) break;
        glyph_len = utf_decode(text + text_len, &unicode, len - text_len);
        text_len += glyph_len;
        chars += glyph_len;
    }
    return chars;
}

void
gui_begin(struct gui_draw_buffer *buffer, void *memory, gui_size size)
{
    void *vertexes, *aligned;
    gui_size cmd_siz, aligning;
    static const gui_size align = ALIGNOF(struct gui_vertex);
    if (!buffer || !memory || !size)
        return;

    cmd_siz = size / 6;
    vertexes = (gui_byte*)memory + cmd_siz;
    aligned = ALIGN(vertexes, align);
    aligning = (gui_size)((gui_byte*)aligned - (gui_byte*)vertexes);
    buffer->command_capacity = cmd_siz / sizeof(struct gui_draw_command);
    buffer->vertex_capacity = (size - cmd_siz - aligning) / sizeof(struct gui_vertex);
    buffer->commands = memory;
    buffer->vertexes = aligned;
    buffer->command_size = 0;
    buffer->vertex_size = 0;
    buffer->allocated = 0;
    buffer->needed = 0;
}

gui_size
gui_end(struct gui_draw_buffer *buffer)
{
    gui_size needed;
    if (!buffer) return 0;
    needed = buffer->needed;
    buffer->allocated = 0;
    buffer->command_capacity = 0;
    buffer->vertex_capacity = 0;
    return needed;
}

static gui_int
gui_push_command(struct gui_draw_buffer *buffer, gui_size count,
    const struct gui_rect *rect, gui_texture tex)
{
    gui_size i;
    gui_byte *end;
    gui_size cmd_memory = 0;
    gui_size vertex_size;
    gui_size current;
    struct gui_draw_command *cmd;

    buffer->needed += count * sizeof(struct gui_vertex);
    buffer->needed += sizeof(struct gui_draw_command);
    if (!buffer || !rect) return gui_false;
    if (!buffer->commands || !buffer->command_capacity ||
        buffer->command_size >= buffer->command_capacity)
        return gui_false;
    if (!buffer->vertexes || !buffer->vertex_capacity ||
        (buffer->vertex_size + count) >= buffer->vertex_capacity)
        return gui_false;

    cmd = &buffer->commands[buffer->command_size++];
    cmd->vertex_count = count;
    cmd->clip_rect = *rect;
    cmd->texture = tex;
    buffer->allocated += count * sizeof(struct gui_vertex);
    buffer->allocated += sizeof(struct gui_draw_command);
    return gui_true;
}

static void
gui_push_vertex(struct gui_draw_buffer *buffer, gui_float x, gui_float y,
    struct gui_color col, gui_float u, gui_float v)
{
    struct gui_vertex *vertex;
    if (!buffer) return;
    if (!buffer->vertexes || !buffer->vertex_capacity ||
        buffer->vertex_size  >= buffer->vertex_capacity)
        return;

    vertex = &buffer->vertexes[buffer->vertex_size++];
    vertex->color = col;
    vertex->pos.x = x;
    vertex->pos.y = y;
    vertex->uv.u = u;
    vertex->uv.v = v;
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

    gui_push_vertex(buffer, a.x + hp0.x, a.y + hp0.y, col, 0.0f, 0.0f);
    gui_push_vertex(buffer, b.x + hp0.x, b.y + hp0.y, col, 0.0f, 0.0f);
    gui_push_vertex(buffer, a.x + hp1.x, a.y + hp1.y, col, 0.0f, 0.0f);
    gui_push_vertex(buffer, b.x + hp0.x, b.y + hp0.y, col, 0.0f, 0.0f);
    gui_push_vertex(buffer, b.x + hp1.x, b.y + hp1.y, col, 0.0f, 0.0f);
    gui_push_vertex(buffer, a.x + hp1.x, a.y + hp1.y, col, 0.0f, 0.0f);
}

static void
gui_draw_line(struct gui_draw_buffer *buffer, gui_float x0, gui_float y0,
    gui_float x1, gui_float y1, struct gui_color col)
{
    struct gui_draw_command *cmd;
    if (!buffer) return;
    if (col.a == 0) return;
    if (!gui_push_command(buffer, 6, &null_rect, null_tex)) return;
    gui_vertex_line(buffer, x0, y0, x1, y1, col);
}

static void
gui_draw_trianglef(struct gui_draw_buffer *buffer, gui_float x0, gui_float y0,
    gui_float x1, gui_float y1, gui_float x2, gui_float y2, struct gui_color c)
{
    struct gui_draw_command *cmd;
    if (!buffer) return;
    if (c.a == 0) return;
    if (!gui_push_command(buffer, 3, &null_rect, null_tex)) return;
    gui_push_vertex(buffer, x0, y0, c, 0.0f, 0.0f);
    gui_push_vertex(buffer, x1, y1, c, 0.0f, 0.0f);
    gui_push_vertex(buffer, x2, y2, c, 0.0f, 0.0f);
}

static void
gui_draw_rect(struct gui_draw_buffer *buffer, gui_float x, gui_float y,
    gui_float w, gui_float h, struct gui_color col)
{
    struct gui_draw_command *cmd;
    if (!buffer) return;
    if (col.a == 0) return;
    if (!gui_push_command(buffer, 6 * 4, &null_rect, null_tex)) return;
    gui_vertex_line(buffer, x, y, x + w, y, col);
    gui_vertex_line(buffer, x + w, y, x + w, y + h, col);
    gui_vertex_line(buffer, x + w, y + h, x, y + h, col);
    gui_vertex_line(buffer, x, y + h, x, y, col);
}

static void
gui_draw_rectf(struct gui_draw_buffer *buffer, gui_float x, gui_float y,
    gui_float w, gui_float h, struct gui_color col)
{
    struct gui_draw_command *cmd;
    if (!buffer) return;
    if (col.a == 0) return;
    if (!gui_push_command(buffer, 6, &null_rect, null_tex))
        return;

    gui_push_vertex(buffer, x, y, col, 0.0f, 0.0f);
    gui_push_vertex(buffer, x + w, y, col, 0.0f, 0.0f);
    gui_push_vertex(buffer, x + w, y + h, col, 0.0f, 0.0f);
    gui_push_vertex(buffer, x, y, col, 0.0f, 0.0f);
    gui_push_vertex(buffer, x + w, y + h, col, 0.0f, 0.0f);
    gui_push_vertex(buffer, x, y + h, col, 0.0f, 0.0f);
}

static void
gui_draw_string(struct gui_draw_buffer *buffer, const struct gui_font *font, gui_float x,
        gui_float y, gui_float w, gui_float h,
        struct gui_color col, const gui_char *t, gui_size len)
{
    struct gui_rect clip;
    gui_size text_len;
    gui_long unicode;
    const struct gui_font_glyph *g;

    if (!buffer || !t || !font || !len) return;
    clip.x = x; clip.y = y;
    clip.w = w; clip.h = h;
    if (!gui_push_command(buffer, 6 * len, &clip, font->texture))
        return;

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

        gui_push_vertex(buffer, x1, y1, col, g->uv[0].u, g->uv[0].v);
        gui_push_vertex(buffer, x2, y1, col, g->uv[1].u, g->uv[0].v);
        gui_push_vertex(buffer, x2, y2, col, g->uv[1].u, g->uv[1].v);
        gui_push_vertex(buffer, x1, y1, col, g->uv[0].u, g->uv[0].v);
        gui_push_vertex(buffer, x2, y2, col, g->uv[1].u, g->uv[1].v);
        gui_push_vertex(buffer, x1, y2, col, g->uv[0].u, g->uv[1].v);
        text_len += utf_decode(t + text_len, &unicode, len - text_len);
        x += char_width;
    }
}

static void
gui_draw_image(struct gui_draw_buffer *buffer, gui_float x, gui_float y,
    gui_float w, gui_float h, gui_texture texture, struct gui_texCoord from,
    struct gui_texCoord to)
{
    const struct gui_color col = {0,0,0,0};
    struct gui_rect clip;
    clip.x = x; clip.y = y;
    clip.w = w; clip.h = h;
    if (!gui_push_command(buffer, 6, &clip, texture))
        return;

    gui_push_vertex(buffer, x, y, col, from.u, from.v);
    gui_push_vertex(buffer, x + w, y, col, to.u, from.v);
    gui_push_vertex(buffer, x + w, y + h, col, to.u, to.v);
    gui_push_vertex(buffer, x, y, col, from.u, to.v);
    gui_push_vertex(buffer, x + w, y + h, col, to.u, to.v);
    gui_push_vertex(buffer, x, y + h, col, from.u, to.v);
}

void
gui_text(struct gui_draw_buffer *buffer, const struct gui_text *text,
    const struct gui_font *font)
{
    gui_float label_x;
    gui_float label_y;
    gui_float label_w;
    gui_float label_h;
    if (!buffer || !text || !font)
        return;

    label_x = text->x + text->pad_x;
    label_y = text->y + text->pad_y;
    label_w = MAX(0, text->w - 2 * text->pad_x);
    label_h = MAX(0, text->h - 2 * text->pad_y);
    gui_draw_rectf(buffer, text->x, text->y, text->w, text->h, text->background);
    gui_draw_string(buffer, font, label_x, label_y, label_w, label_h,
                    text->font, (const gui_char*)text->text, text->length);
}

gui_size
gui_text_wrap(struct gui_draw_buffer *buffer, const struct gui_text *text,
    const struct gui_font *font)
{
    gui_float label_x;
    gui_float label_y;
    gui_float label_w;
    gui_float label_h;
    gui_float space;
    gui_size lines = 0;
    gui_size len = 0;
    gui_size chars = 0;
    if (!buffer || !text || !font)
        return 0;

    label_x = text->x + text->pad_x;
    label_y = text->y + text->pad_y;
    label_w = MAX(0, text->w - 2 * text->pad_x);
    label_h = MAX(0, text->h - 2 * text->pad_y);
    space = font->height + 2 * text->pad_y;
    chars = gui_font_chars_in_space(font, (const gui_char*)text->text, text->length, label_w);
    while (chars && label_h >= space) {
        lines++;
        gui_draw_string(buffer, font, label_x, label_y, label_w, space,
            text->font, (const gui_char*)text->text + len, chars);

        len += chars;
        label_h -= space;
        label_y += space;
        chars = gui_font_chars_in_space(font, (const gui_char*)text->text + chars,
            text->length - chars, label_w);
    }
    return lines;
}

void
gui_image(struct gui_draw_buffer *buffer, const struct gui_image *image)
{
    gui_float image_x;
    gui_float image_y;
    gui_float image_w;
    gui_float image_h;
    if (!buffer || !image)
        return;

    image_x = image->x + image->pad_x;
    image_y = image->y + image->pad_y;
    image_w = MAX(0, image->w - 2 * image->pad_x);
    image_h = MAX(0, image->h - 2 * image->pad_y);
    gui_draw_rectf(buffer, image->x, image->y, image->w, image->h, image->background);
    gui_draw_image(buffer, image_x, image_y, image_w, image_h,
        image->texture, image->uv[0], image->uv[1]);
}

gui_int
gui_button_text(struct gui_draw_buffer *buffer, const struct gui_button *button,
    const char *text, gui_size length, const struct gui_font *font,
    const struct gui_input *in)
{
    gui_int ret = gui_false;
    struct gui_color font_color, background, highlight;
    gui_float button_w, button_h;
    if (!buffer || !in || !button)
        return gui_false;

    font_color = button->font;
    background = button->background;
    highlight = button->highlight;
    button_w = MAX(button->w, 2 * button->pad_x);
    button_h = MAX(button->h, 2 * button->pad_x);

    if (INBOX(in->mouse_pos.x, in->mouse_pos.y, button->x, button->y, button_w, button_h)) {
        font_color = button->background;
        background = button->highlight;
        if (INBOX(in->mouse_clicked_pos.x, in->mouse_clicked_pos.y,
                 button->x, button->y, button_w, button_h))
            ret = (in->mouse_down && in->mouse_clicked);
    }
    gui_draw_rectf(buffer, button->x, button->y, button_w, button_h, background);
    gui_draw_rect(buffer, button->x+1, button->y+1, button_w-1, button_h-1, button->foreground);

    if (font && text && length) {
        gui_float label_x, label_y, label_w, label_h;
        gui_size text_width = gui_font_text_width(font, (const gui_char*)text, length);
        if (text_width > (button_w - 2.0f * button->pad_x)) {
            label_x = button->x + button->pad_x;
            label_y = button->y + button->pad_y;
            label_w = button_w - 2 * button->pad_x;
            label_h = button_h - 2 * button->pad_y;
        } else {
            label_w = button_w - 2 * button->pad_x;
            label_h = button_h - 2 * button->pad_y;
            label_x = button->x + (label_w - (gui_float)text_width) / 2;
            label_y = button->y + (label_h - font->height) / 2;
        }
        gui_draw_string(buffer, font, label_x, label_y, label_w, label_h, font_color,
                    (const gui_char*)text, length);
    }
    return ret;
}

gui_int
gui_button_triangle(struct gui_draw_buffer *buffer, struct gui_button* button,
    enum gui_direction direction, const struct gui_input *in)
{
    gui_bool pressed;
    struct gui_vec2 points[3];
    pressed = gui_button_text(buffer, button, NULL, 0, NULL, in);
    gui_triangle_from_direction(points, button->x, button->y, button->w, button->h,
        button->pad_x, button->pad_y, direction);
    gui_draw_trianglef(buffer, points[0].x, points[0].y, points[1].x, points[1].y,
        points[2].x, points[2].y, button->foreground);
    return pressed;
}

gui_int
gui_button_image(struct gui_draw_buffer *buffer, struct gui_button* button,
    gui_texture tex, struct gui_texCoord from, struct gui_texCoord to,
    const struct gui_input *in)
{
    gui_bool pressed;
    struct gui_image image;
    pressed = gui_button_text(buffer, button, NULL, 0, NULL, in);
    image.x = button->x;
    image.y = button->y;
    image.w = button->y;
    image.h = button->y;
    image.pad_x = button->pad_x;
    image.pad_y = button->pad_y;
    image.texture = tex;
    image.uv[0] = from;
    image.uv[1] = to;
    image.background = button->background;
    gui_image(buffer, &image);
    return pressed;
}

gui_int
gui_toggle(struct gui_draw_buffer *buffer, const struct gui_toggle *toggle,
    const struct gui_font *font, const struct gui_input *in)
{
    gui_int toggle_active;
    gui_float select_size;
    gui_float toggle_w, toggle_h;
    gui_float select_x, select_y;
    gui_float cursor_x, cursor_y;
    gui_float cursor_pad, cursor_size;
    if (!buffer || !toggle || !in)
        return 0;

    toggle_w = MAX(toggle->w, font->height + 2 * toggle->pad_x);
    toggle_h = MAX(toggle->h, font->height + 2 * toggle->pad_y);
    toggle_active = toggle->active;

    select_x = toggle->x + toggle->pad_x;
    select_y = toggle->y + toggle->pad_y;
    select_size = font->height;

    cursor_pad = select_size / 8;
    cursor_x = select_x + cursor_pad;
    cursor_y = select_y + cursor_pad;
    cursor_size = select_size - 2 * cursor_pad;
    if (!in->mouse_down && in->mouse_clicked) {
        if (INBOX(in->mouse_clicked_pos.x, in->mouse_clicked_pos.y,
                  cursor_x, cursor_y, cursor_size, cursor_size))
            toggle_active = !toggle_active;
    }

    gui_draw_rectf(buffer, select_x, select_y, select_size, select_size, toggle->background);
    if (toggle_active)
        gui_draw_rectf(buffer, cursor_x, cursor_y, cursor_size, cursor_size, toggle->foreground);

    if (font && toggle->text && toggle->length) {
        gui_float label_x, label_w;
        label_x = toggle->x + select_size + toggle->pad_x * 2;
        label_w = toggle_w - select_size + 3 * toggle->pad_x;
        gui_draw_string(buffer, font, label_x, toggle->y + toggle->pad_y, label_w,
            toggle_h - 2 * toggle->pad_y, toggle->font, toggle->text, toggle->length);
    }
    return toggle_active;
}

gui_float
gui_slider(struct gui_draw_buffer *buffer, const struct gui_slider *slider,
    const struct gui_input *in)
{
    gui_float slider_min, slider_max;
    gui_float slider_value, slider_steps;
    gui_float slider_w, slider_h;
    gui_float mouse_x, mouse_y;
    gui_float clicked_x, clicked_y;
    gui_float cursor_x, cursor_y;
    gui_float cursor_w, cursor_h;
    if (!buffer || !slider || !in)
        return 0;

    slider_w = MAX(slider->w, 2 * slider->pad_x);
    slider_h = MAX(slider->h, 2 * slider->pad_y);
    slider_max = MAX(slider->min, slider->max);
    slider_min = MIN(slider->min, slider->max);
    slider_value = CLAMP(slider_min, slider->value, slider_max);
    slider_steps = (slider_max - slider_min) / slider->step;

    cursor_w = (slider_w - 2 * slider->pad_x);
    cursor_w = cursor_w / (((slider_max - slider_min) + slider->step) / slider->step);
    cursor_h = slider_h - 2 * slider->pad_y;
    cursor_x = slider->x + slider->pad_x + (cursor_w * (slider_value - slider_min));
    cursor_y = slider->y + slider->pad_y;

    mouse_x = in->mouse_pos.x;
    mouse_y = in->mouse_pos.y;
    clicked_x = in->mouse_clicked_pos.x;
    clicked_y = in->mouse_clicked_pos.y;

    if (in->mouse_down &&
        INBOX(clicked_x, clicked_y, slider->x, slider->y, slider_w, slider_h) &&
        INBOX(mouse_x, mouse_y, slider->x, slider->y, slider_w, slider_h))
    {
        const float d = mouse_x - (cursor_x + cursor_w / 2.0f);
        const float pxstep = (slider_w - 2 * slider->pad_x) / slider_steps;
        if (ABS(d) >= pxstep) {
            slider_value += (d < 0) ? -slider->step : slider->step;
            slider_value = CLAMP(slider_min, slider_value, slider_max);
            cursor_x = slider->x + slider->pad_x + (cursor_w * (slider_value - slider_min));
        }
    }
    gui_draw_rectf(buffer, slider->x, slider->y, slider_w, slider_h, slider->background);
    gui_draw_rectf(buffer, cursor_x, cursor_y, cursor_w, cursor_h, slider->foreground);
    return slider_value;
}

gui_size
gui_progress(struct gui_draw_buffer *buffer, const struct gui_progress *prog,
    const struct gui_input *in)
{
    gui_float prog_scale;
    gui_float cursor_x, cursor_y;
    gui_float cursor_w, cursor_h;
    gui_float prog_w, prog_h;
    gui_size prog_value;

    if (!buffer || !in || !prog) return 0;
    prog_w = MAX(prog->w, 2 * prog->pad_x + 1);
    prog_h = MAX(prog->h, 2 * prog->pad_y + 1);
    prog_value = MIN(prog->current, prog->max);

    if (prog->modifyable && in->mouse_down &&
        INBOX(in->mouse_pos.x, in->mouse_pos.y, prog->x, prog->y, prog_w, prog_h)){
        gui_float ratio = (gui_float)(in->mouse_pos.x - prog->x) / (gui_float)prog_w;
        prog_value = (gui_size)((gui_float)prog->max * ratio);
    }

    if (!prog->max) return prog_value;
    prog_value = MIN(prog_value, prog->max);
    prog_scale = (gui_float)prog_value / (gui_float)prog->max;

    cursor_h = prog_h - 2 * prog->pad_y;
    cursor_w = (prog_w - 2 * prog->pad_x) * prog_scale;
    cursor_x = prog->x + prog->pad_x;
    cursor_y = prog->y + prog->pad_y;

    gui_draw_rectf(buffer, prog->x, prog->y, prog_w, prog_h, prog->background);
    gui_draw_rectf(buffer, cursor_x, cursor_y, cursor_w, cursor_h, prog->foreground);
    return prog_value;
}

gui_int
gui_input(struct gui_draw_buffer *buf,  gui_char *buffer, gui_size *length,
    const struct gui_input_field *input,
    const struct gui_font *font, const struct gui_input *in)
{
    gui_size offset = 0;
    gui_float input_w, input_h;
    gui_int input_active;
    if (!buffer || !in || !font || !input)
        return 0;

    input_w = MAX(input->w, 2 * input->pad_x);
    input_h = MAX(input->h, font->height);
    input_active = input->active;
    if (in->mouse_clicked && in->mouse_down) {
        input_active = INBOX(in->mouse_pos.x, in->mouse_pos.y,
                            input->x, input->y, input_w, input_h);
    }
    gui_draw_rectf(buf, input->x, input->y, input_w, input_h, input->background);
    gui_draw_rect(buf, input->x + 1, input->y, input_w - 1, input_h, input->foreground);

    if (input_active) {
        const struct gui_key *del = &in->keys[GUI_KEY_DEL];
        const struct gui_key *bs = &in->keys[GUI_KEY_BACKSPACE];
        const struct gui_key *enter = &in->keys[GUI_KEY_ENTER];
        const struct gui_key *esc = &in->keys[GUI_KEY_ESCAPE];
        const struct gui_key *space = &in->keys[GUI_KEY_SPACE];

        if ((del->down && del->clicked) || (bs->down && bs->clicked))
            if (*length > 0) *length = *length - 1;
        if ((enter->down && enter->clicked) || (esc->down && esc->clicked))
            input_active = gui_false;
        if ((space->down && space->clicked) && (*length < input->max))
            buffer[(*length)++] = ' ';

        if (in->text_len && *length < input->max) {
            gui_size text_len = 0, glyph_len = 0;
            gui_long unicode;
            gui_size i = 0;
            glyph_len = utf_decode(in->text, &unicode, in->text_len);
            while (glyph_len && (text_len + glyph_len) <= in->text_len && *length < input->max) {
                for (i = 0; i < glyph_len; i++)
                    buffer[(*length)++] = in->text[text_len + i];
                text_len = text_len + glyph_len;
                glyph_len = utf_decode(in->text + text_len, &unicode, in->text_len - text_len);
            }
        }
    }

    if (font && buffer && length && *length) {
        gui_float label_x = input->x + input->pad_x;
        gui_float label_y = input->y + input->pad_y;
        gui_float label_w = input_w - 2 * input->pad_x;
        gui_float label_h = input_h - 2 * input->pad_y;
        gui_size text_width = gui_font_text_width(font, buffer, *length);
        while (text_width > label_w) {
            offset += 1;
            text_width = gui_font_text_width(font, &buffer[offset], *length - offset);
        }
        gui_draw_string(buf, font, label_x, label_y, label_w, label_h, input->font,
                    &buffer[offset], *length);
    }
    return input_active;
}

gui_int
gui_plot(struct gui_draw_buffer *buffer, const struct gui_plot *plot,
    const struct gui_input *in)
{
    gui_size i;
    gui_int plot_selected = -1;
    gui_float plot_max_value, plot_min_value;
    gui_float plot_value_range, plot_value_ratio;
    gui_size plot_step;
    gui_float plot_w, plot_h;
    gui_float canvas_x, canvas_y;
    gui_float canvas_w, canvas_h;
    gui_float last_x;
    gui_float last_y;
    struct gui_color col;
    if (!buffer || !plot || !in)
        return plot_selected;

    col = plot->foreground;
    plot_w = MAX(plot->w, 2 * plot->pad_x);
    plot_h = MAX(plot->h, 2 * plot->pad_y);
    gui_draw_rectf(buffer, plot->x, plot->y, plot_w, plot_h, plot->background);
    if (!plot->value_count)
        return plot_selected;

    plot_max_value = plot->values[0];
    plot_min_value = plot->values[0];
    for (i = 0; i < plot->value_count; ++i) {
        if (plot->values[i] > plot_max_value)
            plot_max_value = plot->values[i];
        if (plot->values[i] < plot_min_value)
            plot_min_value = plot->values[i];
    }

    canvas_x = plot->x + plot->pad_x;
    canvas_y = plot->y + plot->pad_y;
    canvas_w = MAX(1 + 2 * plot->pad_x, plot_w - 2 * plot->pad_x);
    canvas_h = MAX(1 + 2 * plot->pad_y, plot_h - 2 * plot->pad_y);

    plot_step = (gui_size)canvas_w / plot->value_count;
    plot_value_range = plot_max_value - plot_min_value;
    plot_value_ratio = (plot->values[0] - plot_min_value) / plot_value_range;

    last_x = canvas_x;
    last_y = (canvas_y + canvas_h) - plot_value_ratio * (gui_float)canvas_h;
    if (INBOX(in->mouse_pos.x, in->mouse_pos.y, last_x-3, last_y-3, 6, 6)) {
        plot_selected = (in->mouse_down && in->mouse_clicked) ? (gui_int)i : -1;
        col = plot->highlight;
    }
    gui_draw_rectf(buffer, last_x - 3, last_y - 3, 6, 6, col);

    for (i = 1; i < plot->value_count; i++) {
        gui_float cur_x, cur_y;
        plot_value_ratio = (plot->values[i] - plot_min_value) / plot_value_range;
        cur_x = canvas_x + (gui_float)(plot_step * i);
        cur_y = (canvas_y + canvas_h) - (plot_value_ratio * (gui_float)canvas_h);
        gui_draw_line(buffer, last_x, last_y, cur_x, cur_y, plot->foreground);
        if (INBOX(in->mouse_pos.x, in->mouse_pos.y, cur_x-3, cur_y-3, 6, 6)) {
            plot_selected = (in->mouse_down && in->mouse_clicked) ? (gui_int)i : plot_selected;
            col = plot->highlight;
        } else col = plot->foreground;
        gui_draw_rectf(buffer, cur_x - 3, cur_y - 3, 6, 6, col);
        last_x = cur_x, last_y = cur_y;
    }
    return plot_selected;
}

gui_int
gui_histo(struct gui_draw_buffer *buffer, const struct gui_histo *histo,
    const struct gui_input *in)
{
    gui_size i;
    gui_int selected = -1;
    gui_float canvas_x, canvas_y;
    gui_float canvas_w, canvas_h;
    gui_float histo_max_value;
    gui_float histo_w, histo_h;
    gui_float item_w = 0.0f;
    if (!buffer || !histo || !in)
        return selected;

    histo_w = MAX(histo->w, 2 * histo->pad_x);
    histo_h = MAX(histo->h, 2 * histo->pad_y);
    gui_draw_rectf(buffer, histo->x, histo->y, histo_w, histo_h, histo->background);

    histo_max_value = histo->values[0];
    for (i = 0; i < histo->value_count; ++i) {
        if (ABS(histo->values[i]) > histo_max_value)
            histo_max_value = histo->values[i];
    }

    canvas_x = histo->x + histo->pad_x;
    canvas_y = histo->y + histo->pad_y;
    canvas_w = histo_w - 2 * histo->pad_x;
    canvas_h = histo_h - 2 * histo->pad_y;
    if (histo->value_count) {
        gui_float padding = (gui_float)(histo->value_count-1) * histo->pad_x;
        item_w = (canvas_w - padding) / (gui_float)(histo->value_count);
    }

    for (i = 0; i < histo->value_count; i++) {
        const gui_float histo_ratio = ABS(histo->values[i]) / histo_max_value;
        struct gui_color item_color = (histo->values[i] < 0) ? histo->negative: histo->foreground;
        const gui_float item_h = canvas_h * histo_ratio;
        const gui_float item_y = (canvas_y + canvas_h) - item_h;
        gui_float item_x = canvas_x + ((gui_float)i * item_w);
        item_x = item_x + ((gui_float)i * histo->pad_y);
        if (INBOX(in->mouse_pos.x, in->mouse_pos.y, item_x, item_y, item_w, item_h)) {
            selected = (in->mouse_down && in->mouse_clicked) ? (gui_int)i: selected;
            item_color = histo->highlight;
        }
        gui_draw_rectf(buffer, item_x, item_y, item_w, item_h, item_color);
    }
    return selected;
}

gui_size
gui_scroll(struct gui_draw_buffer *buffer, const struct gui_scroll *scroll,
    const struct gui_input *in)
{
    gui_bool button_up_pressed;
    gui_bool button_down_pressed;
    struct gui_button button;

    gui_float scroll_y, scroll_w, scroll_h;
    gui_size scroll_offset;
    gui_size scroll_step;
    gui_float scroll_off, scroll_ratio;

    gui_float cursor_x, cursor_y;
    gui_float cursor_w, cursor_h;
    gui_float cursor_px, cursor_py;
    gui_bool inscroll, incursor;

    if (!buffer || !in || !scroll) return 0;
    scroll_y = scroll->y;
    scroll_w = MAX(scroll->w, 0);
    scroll_h = MAX(scroll->h, 2 * scroll_w);
    gui_draw_rectf(buffer, scroll->x, scroll->y, scroll_w, scroll_h, scroll->background);
    if (scroll->target <= scroll_h) return 0;

    button.x = scroll->x;
    button.y = scroll->y;
    button.w = scroll_w;
    button.h = scroll_w;
    button.pad_x = scroll_w / 4;
    button.pad_y = scroll_w / 4;
    button.background = scroll->foreground;
    button.foreground =  scroll->background;
    button.highlight = scroll->foreground;
    button_up_pressed = gui_button_triangle(buffer, &button, GUI_UP, in);
    button.y = scroll->y + scroll_h - button.h;
    button_down_pressed = gui_button_triangle(buffer, &button, GUI_DOWN, in);

    scroll_h = scroll_h - 2 * button.h;
    scroll_y = scroll->y + button.h;
    scroll_step = MIN(scroll->step, (gui_size)scroll_h);
    scroll_offset = MIN(scroll->offset, (gui_size)(scroll->target - (gui_size)scroll_h));
    scroll_ratio = (gui_float)scroll_h/(gui_float)scroll->target;
    scroll_off = (gui_float)scroll_offset/(gui_float)scroll->target;

    cursor_h = scroll_ratio * scroll_h;
    cursor_y = scroll_y + (scroll_off * scroll_h);
    cursor_w = scroll_w;
    cursor_x = scroll->x;

    inscroll = INBOX(in->mouse_pos.x, in->mouse_pos.y, scroll->x, scroll->y, scroll_w, scroll_h);
    incursor = INBOX(in->mouse_prev.x, in->mouse_prev.y, cursor_x, cursor_y, cursor_w, cursor_h);
    if (in->mouse_down && inscroll && incursor) {
        const gui_float pixel = in->mouse_delta.y;
        const gui_float delta =  (pixel / (gui_float)scroll_h) * (gui_float)scroll->target;
        scroll_offset = MIN(scroll_offset + (gui_size)delta, scroll->target - (gui_size)scroll_h);
        cursor_y += pixel;
    } else if (button_up_pressed || button_down_pressed) {
        scroll_offset = (button_down_pressed) ?
            MIN(scroll_offset + scroll_step, scroll->target - (gui_size)scroll_h):
            MAX(0, scroll_offset - scroll_step);
        scroll_off = (gui_float)scroll_offset / (gui_float)scroll->target;
        cursor_y = scroll_y + (scroll_off * scroll_h);
    }

    gui_draw_rectf(buffer, cursor_x, cursor_y, cursor_w, cursor_h, scroll->foreground);
    gui_draw_rect(buffer, cursor_x+1, cursor_y, cursor_w-1, cursor_h, scroll->background);
    return scroll_offset;
}

void
gui_default_config(struct gui_config *config)
{
    if (!config) return;
    config->global_alpha = 1.0f;
    config->scrollbar_width = 16;
    config->scroll_factor = 2;
    config->panel_padding = gui_make_vec2(15.0f, 10.0f);
    config->panel_min_size = gui_make_vec2(32.0f, 32.0f);
    config->item_spacing = gui_make_vec2(8.0f, 8.0f);
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
    config->colors[GUI_COLOR_INPUT] = gui_make_color(45, 45, 45, 45);
    config->colors[GUI_COLOR_INPUT_BORDER] = gui_make_color(100, 100, 100, 100);
    config->colors[GUI_COLOR_HISTO] = gui_make_color(100, 100, 100, 255);
    config->colors[GUI_COLOR_HISTO_BARS] = gui_make_color(45, 45, 45, 255);
    config->colors[GUI_COLOR_HISTO_NEGATIVE] = gui_make_color(255, 255, 255, 255);
    config->colors[GUI_COLOR_HISTO_HIGHLIGHT] = gui_make_color(255, 0, 0, 255);
    config->colors[GUI_COLOR_PLOT] = gui_make_color(100, 100, 100, 255);
    config->colors[GUI_COLOR_PLOT_LINES] = gui_make_color(45, 45, 45, 255);
    config->colors[GUI_COLOR_PLOT_HIGHLIGHT] = gui_make_color(255, 0, 0, 255);
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
    panel->minimized = gui_false;
    panel->out = NULL;
}

gui_int
gui_panel_begin(struct gui_panel *panel, struct gui_draw_buffer *out,
    const char *text, gui_flags f, gui_float x, gui_float y, gui_float w, gui_float h)
{
    const char *i;
    gui_int ret = 1;
    gui_size text_len = 0;
    gui_float header_height = 0;
    gui_float label_x, label_y, label_w, label_h;

    const struct gui_config *config = panel->config;
    const struct gui_color *header = &config->colors[GUI_COLOR_TITLEBAR];
    const gui_float mouse_x = panel->in->mouse_pos.x;
    const gui_float mouse_y = panel->in->mouse_pos.y;
    const gui_float clicked_x = panel->in->mouse_clicked_pos.x;
    const gui_float clicked_y = panel->in->mouse_clicked_pos.y;

    header_height = panel->font->height + 3 * config->item_padding.y;
    header_height += config->panel_padding.y;
    gui_draw_rectf(out, x, y, w, header_height, *header);

    panel->out = out;
    panel->x = x;
    panel->y = y;
    panel->width = w;
    panel->at_y = y;
    panel->flags = f;
    panel->index = 0;
    panel->row_columns = 0;
    panel->row_height = header_height;
    if (panel->flags & GUI_PANEL_SCROLLBAR)
        panel->height = h;

    if (panel->flags & (GUI_PANEL_CLOSEABLE|GUI_PANEL_HEADER)) {
        gui_size text_width;
        gui_float close_x, close_y, close_w, close_h;
        const gui_char *X = (const gui_char*)"x";

        text_width = gui_font_text_width(panel->font, X, 1);
        close_x = (x + w) - ((gui_float)text_width + config->panel_padding.x);
        close_y = y + config->panel_padding.y;
        close_w = (gui_float)text_width + config->panel_padding.x;
        close_h = panel->font->height + 2 * config->item_padding.y;
        w -= ((gui_float)text_width + config->panel_padding.x);

        gui_draw_string(panel->out, panel->font, close_x, close_y, close_w, close_h,
            config->colors[GUI_COLOR_TEXT], X, 1);
        if (INBOX(mouse_x, mouse_y, close_x, close_y, close_w, close_h)) {
            if (INBOX(clicked_x, clicked_y, close_x, close_y, close_w, close_h))
                ret = !(panel->in->mouse_down && panel->in->mouse_clicked);
        }
    }

    if (panel->flags & (GUI_PANEL_MINIMIZABLE|GUI_PANEL_HEADER)) {
        gui_size text_width;
        gui_float min_x, min_y, min_w, min_h;
        const gui_char *score = (panel->minimized) ?
            (const gui_char*)"+":
            (const gui_char*)"-";

        text_width = gui_font_text_width(panel->font, score, 1);
        min_x = (x + w) - ((gui_float)text_width + config->item_padding.y);
        min_y = y + config->panel_padding.y;
        min_w = (gui_float)text_width;
        min_h = panel->font->height + 2 * config->item_padding.y;
        w -= (gui_float)text_width;

        gui_draw_string(panel->out, panel->font, min_x, min_y, min_w, min_h,
            config->colors[GUI_COLOR_TEXT], score, 1);
        if (INBOX(mouse_x, mouse_y, min_x, min_y, min_w, min_h)) {
            if (INBOX(clicked_x, clicked_y, min_x, min_y, min_w, min_h))
                if (panel->in->mouse_down && panel->in->mouse_clicked)
                    panel->minimized = !panel->minimized;
        }
    }

    i = text;
    label_x = x + config->panel_padding.x + config->item_padding.x;
    label_y = y + config->panel_padding.y;
    label_w = w - 2 * config->panel_padding.x - 2 * config->item_padding.x;
    label_h = panel->font->height + 2 * config->item_padding.y;
    while (*i++ != '\0') text_len++;
    gui_draw_string(panel->out, panel->font, label_x, label_y, label_w, label_h,
        config->colors[GUI_COLOR_TEXT], (const gui_char*)text, text_len);
    return ret;
}

void
gui_panel_row(struct gui_panel *panel, gui_float height, gui_size cols)
{
    const struct gui_config *config = panel->config;
    const struct gui_color *color = &config->colors[GUI_COLOR_PANEL];
    if (panel->minimized) return;
    panel->at_y += panel->row_height;
    panel->index = 0;
    panel->row_columns = cols;
    panel->row_height = height + config->item_spacing.y;
    gui_draw_rectf(panel->out, panel->x, panel->at_y, panel->width,
                    height + config->panel_padding.y, *color);
}

void
gui_panel_seperator(struct gui_panel *panel, gui_size cols)
{
    const struct gui_config *config;
    if (!panel) return;
    config = panel->config;
    cols = MIN(cols, panel->row_columns - panel->index);
    panel->index += cols;
    if (panel->index >= panel->row_columns) {
        const gui_float row_height = panel->row_height - config->item_spacing.y;
        gui_panel_row(panel, row_height, panel->row_columns);
    }
}

static void
gui_panel_alloc_space(struct gui_rect *bounds, struct gui_panel *panel)
{
    const struct gui_config *config = panel->config;
    gui_float padding, spacing, space;
    gui_float item_offset, item_width, item_spacing;
    if (panel->index >= panel->row_columns) {
        const gui_float row_height = panel->row_height - config->item_spacing.y;
        gui_panel_row(panel, row_height, panel->row_columns);
    }

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

void
gui_panel_text(struct gui_panel *panel, const char *str, gui_size len)
{
    struct gui_rect bounds;
    struct gui_text text;
    const struct gui_config *config;

    if (!panel || !panel->config || !panel->in || !panel->out) return;
    if (!panel->font || panel->minimized) return;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;

    text.x = bounds.x;
    text.y = bounds.y;
    text.w = bounds.w;
    text.h = bounds.h;
    text.pad_x = config->item_padding.x;
    text.pad_y = config->item_padding.y;
    text.text = str;
    text.length = len;
    text.font = config->colors[GUI_COLOR_TEXT];
    text.background = config->colors[GUI_COLOR_PANEL];
    return gui_text(panel->out, &text, panel->font);
}

gui_size
gui_panel_text_wrap(struct gui_panel *panel, const char *str, gui_size len)
{
    struct gui_rect bounds;
    struct gui_text text;
    const struct gui_config *config;

    if (!panel || !panel->config || !panel->in || !panel->out) return 0;
    if (!panel->font || panel->minimized) return 0;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;

    text.x = bounds.x;
    text.y = bounds.y;
    text.w = bounds.w;
    text.h = bounds.h;
    text.pad_x = config->item_padding.x;
    text.pad_y = config->item_padding.y;
    text.text = str;
    text.length = len;
    text.font = config->colors[GUI_COLOR_TEXT];
    text.background = config->colors[GUI_COLOR_PANEL];
    return gui_text_wrap(panel->out, &text, panel->font);
}

gui_int
gui_panel_button_text(struct gui_panel *panel, const char *str, gui_size len)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;

    if (!panel || !panel->config || !panel->in || !panel->out) return 0;
    if (!panel->font || panel->minimized) return 0;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;

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
    return gui_button_text(panel->out, &button, str, len, panel->font, panel->in);
}

gui_int
gui_panel_button_triangle(struct gui_panel *panel, enum gui_direction d)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;

    if (!panel || !panel->config || !panel->in || !panel->out) return 0;
    if (!panel->font || panel->minimized) return 0;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;

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
    return gui_button_triangle(panel->out, &button, d, panel->in);
}

gui_int
gui_panel_button_image(struct gui_panel *panel, gui_texture tex,
    struct gui_texCoord from, struct gui_texCoord to)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;

    if (!panel || !panel->config || !panel->in || !panel->out) return 0;
    if (!panel->font || panel->minimized) return 0;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;
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
    return gui_button_image(panel->out, &button, tex, from, to, panel->in);
}

gui_int
gui_panel_toggle(struct gui_panel *panel, const char *text, gui_size length,
    gui_int is_active)
{
    struct gui_rect bounds;
    struct gui_toggle toggle;
    const struct gui_config *config;

    if (!panel || !panel->config || !panel->in || !panel->out) return is_active;
    if (!panel->font || panel->minimized) return is_active;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;

    toggle.x = bounds.x;
    toggle.y = bounds.y;
    toggle.w = bounds.w;
    toggle.h = bounds.h;
    toggle.pad_x = config->item_padding.x;
    toggle.pad_y = config->item_padding.y;
    toggle.active = is_active;
    toggle.text = (const gui_char*)text;
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
    const struct gui_config *config;

    if (!panel || !panel->config || !panel->in || !panel->out) return value;
    if (panel->minimized) return value;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;

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
    const struct gui_config *config;

    if (!panel || !panel->config || !panel->in || !panel->out) return cur_value;
    if (panel->minimized) return cur_value;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;

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
    const struct gui_config *config;

    if (!panel || !panel->config || !panel->in || !panel->out) return is_active;
    if (!panel->font || panel->minimized) return is_active;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;

    field.x = bounds.x;
    field.y = bounds.y;
    field.w = bounds.w;
    field.h = bounds.h;
    field.pad_x = config->item_padding.x;
    field.pad_y = config->item_padding.y;
    field.max  = max_length;
    field.active = is_active;
    field.font = config->colors[GUI_COLOR_TEXT];
    field.background = config->colors[GUI_COLOR_INPUT];
    field.foreground = config->colors[GUI_COLOR_INPUT_BORDER];
    return gui_input(panel->out, buffer, length, &field, panel->font, panel->in);
}

gui_int
gui_panel_plot(struct gui_panel *panel, const gui_float *values, gui_size count)
{
    struct gui_rect bounds;
    struct gui_plot plot;
    const struct gui_config *config;

    if (!panel || !panel->config || !panel->in || !panel->out) return -1;
    if (panel->minimized) return -1;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;

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
    plot.highlight = config->colors[GUI_COLOR_PLOT_HIGHLIGHT];
    return gui_plot(panel->out, &plot, panel->in);
}

gui_int
gui_panel_histo(struct gui_panel *panel, const gui_float *values, gui_size count)
{
    struct gui_rect bounds;
    struct gui_histo histo;
    const struct gui_config *config;

    if (!panel || !panel->config || !panel->in || !panel->out) return -1;
    if (panel->minimized) return -1;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;

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

