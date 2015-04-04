/*
    Copyright (c) 2015
    vurtun <polygone@gmx.net>
    MIT licence
*/
#include "gui.h"

#ifndef NDEBUG
#include <assert.h>
#else
#define assert(expr)
#endif

#define NULL (void*)0
#define UTF_INVALID 0xFFFD
#define MAX_NUMBER_BUFFER 64
#define PASTE(a,b) a##b
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#define SATURATE(x) (MAX(0, MIN(1.0f, x)))
#define LEN(a) (sizeof(a)/sizeof(a)[0])
#define ABS(a) (((a) < 0) ? -(a) : (a))
#define BETWEEN(x, a, b) ((a) <= (x) && (x) <= (b))
#define INBOX(px, py, x, y, w, h) (BETWEEN(px, x, x+w) && BETWEEN(py, y, y+h))
#define ALIGNOF(t) ((char*)(&((struct {char c; t _h;}*)0)->_h) - (char*)0)
#define ALIGN(x, mask) (void*)((gui_size)((gui_byte*)(x) + (mask-1)) & ~(mask-1))

#define col_load(c,j,k,l,m) (c).r = (j), (c).g = (k), (c).b = (l), (c).a = (m)
#define vec2_load(v,a,b) (v).x = (a), (v).y = (b)
#define vec2_mov(to,from) (to).x = (from).x, (to).y = (from).y
#define vec2_len(v) ((float)fsqrt((v).x*(v).x+(v).y*(v).y))
#define vec2_sub(r,a,b) do {(r).x=(a).x-(b).x; (r).y=(a).y-(b).y;} while(0)
#define vec2_muls(r, v, s) do {(r).x=(v).x*(s); (r).y=(v).y*(s);} while(0)

struct gui_context_panel {
    struct gui_panel panel;
    gui_float x, y, w, h;
    struct gui_draw_call_list list;
    struct gui_context_panel *next;
    struct gui_context_panel *prev;
};

struct gui_context {
    gui_float width, height;
    struct gui_draw_buffer global_buffer;
    struct gui_draw_buffer current_buffer;
    const struct gui_input *input;
    struct gui_context_panel *active;
    struct gui_context_panel *panel_pool;
    struct gui_context_panel *free_list;
    struct gui_context_panel *stack_begin;
    struct gui_context_panel *stack_end;
    struct gui_draw_call_list **output_list;
    gui_size panel_capacity;
    gui_size panel_size;
};

static const gui_texture null_tex;
static const struct gui_rect null_rect = {0.0f, 0.0f, 9999.0f, 9999.0f};
static const gui_char utfbyte[GUI_UTF_SIZE+1] = {0x80, 0, 0xC0, 0xE0, 0xF0};
static const gui_char utfmask[GUI_UTF_SIZE+1] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
static const long utfmin[GUI_UTF_SIZE+1] = {0, 0, 0x80, 0x800, 0x100000};
static const long utfmax[GUI_UTF_SIZE+1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};

static gui_float
fsqrt(float x)
{
    float xhalf = 0.5f*x;
    union {float f; long i;} val;
    val.i = 0;
    val.f = ABS(x);
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

static struct gui_rect
unify(const struct gui_rect *a, const struct gui_rect *b)
{
    struct gui_rect clip;
    clip.x = MAX(a->x, b->x);
    clip.y = MAX(a->y, b->y);
    clip.w = MIN(a->x + a->w, b->x + b->w) - clip.x;
    clip.h = MIN(a->y + a->h, b->y + b->h) - clip.y;
    clip.w = MAX(0, clip.w);
    clip.h = MAX(0, clip.h);
    return clip;
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

static void
gui_triangle_from_direction(struct gui_vec2 *result, gui_float x, gui_float y,
    gui_float w, gui_float h, gui_float pad_x, gui_float pad_y,
    enum gui_heading direction)
{
    gui_float w_half, h_half;
    assert(result);
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
    assert(in);
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
    assert(in);
    in->mouse_pos.x = (gui_float)x;
    in->mouse_pos.y = (gui_float)y;
}

void
gui_input_key(struct gui_input *in, enum gui_keys key, gui_bool down)
{
    assert(in);
    if (!in) return;
    if (in->keys[key].down == down) return;
    in->keys[key].down = down;
    in->keys[key].clicked++;
}

void
gui_input_button(struct gui_input *in, gui_int x, gui_int y, gui_bool down)
{
    assert(in);
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
    assert(in);
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
    assert(in);
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

    assert(font);
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
    const struct gui_font_glyph *glyph;
    gui_size chars = 0;
    gui_long unicode;
    gui_size text_width = 0;
    gui_size text_len = 0;
    gui_size glyph_len;

    assert(font);
    assert(text);
    assert(len);
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
gui_output_begin(struct gui_draw_buffer *buffer, const struct gui_memory *memory)
{
    void *cmds;
    void *clips;
    void *aligned;
    gui_size vertex_size;
    gui_size command_size;
    gui_size clip_size;
    gui_long alignment;
    static const gui_size align_cmd = ALIGNOF(struct gui_draw_command);
    static const gui_size align_clip = ALIGNOF(struct gui_rect);

    assert(buffer);
    assert(memory);
    assert((memory->vertex_percentage+memory->command_percentage+memory->clip_percentage) <= 1.0f);
    if (!buffer || !memory) return;
    if ((memory->vertex_percentage + memory->command_percentage +
        memory->clip_percentage) > 1.0f) return;

    vertex_size = (gui_size)((gui_float)memory->size * SATURATE(memory->vertex_percentage));
    command_size = (gui_size)((gui_float)memory->size * SATURATE(memory->command_percentage));
    clip_size = (gui_size)((gui_float)memory->size * SATURATE(memory->clip_percentage));

    cmds = (gui_byte*)memory->memory + vertex_size;
    aligned = ALIGN(cmds, align_cmd);
    alignment = (gui_byte*)aligned - (gui_byte*)cmds;
    cmds = aligned;

    clips = (gui_byte*)memory->memory + vertex_size + command_size + alignment;
    clips = ALIGN(clips, align_clip);

    buffer->vertex_capacity = vertex_size / sizeof(struct gui_vertex);
    buffer->command_capacity = command_size / sizeof(struct gui_draw_command);
    buffer->clip_capacity = clip_size / sizeof(struct gui_rect);
    buffer->vertex_capacity = (gui_size)MAX(0, (gui_int)vertex_size - 1);
    buffer->command_capacity = (gui_size)MAX(0, (gui_int)command_size - 1);
    buffer->clip_capacity = (gui_size)MAX(0, (gui_int)clip_size - 1);
    buffer->vertexes = memory->memory;
    buffer->commands = cmds;
    buffer->clips = clips;
    buffer->command_size = 0;
    buffer->command_needed = 0;
    buffer->vertex_size = 0;
    buffer->vertex_needed = 0;
    buffer->clip_size = 0;
    buffer->clip_needed = 0;
}

void
gui_output_end(struct gui_draw_buffer *buffer, struct gui_draw_call_list *list,
    struct gui_memory_status* status)
{
    assert(buffer);
    if (!buffer) return;
    if (status) {
        static const gui_size vertsize = sizeof(struct gui_vertex);
        static const gui_size cmdsize = sizeof(struct gui_draw_command);
        static const gui_size recsize = sizeof(struct gui_rect);

        status->vertexes_allocated = buffer->vertex_size * vertsize;
        status->vertexes_needed = buffer->vertex_needed * vertsize;
        status->commands_allocated = buffer->command_size * cmdsize;
        status->commands_needed = buffer->command_needed *cmdsize;
        status->clips_allocated = buffer->clip_size * recsize;
        status->clips_needed = buffer->clip_needed * recsize;

        status->allocated = status->vertexes_allocated;
        status->allocated += status->commands_allocated;
        status->allocated += status->clips_allocated;

        status->needed = status->vertexes_needed;
        status->needed += status->commands_needed;
        status->needed += status->clips_needed;
    }

    if (list) {
        list->vertexes = buffer->vertexes;
        list->vertex_size = buffer->vertex_size;
        list->commands = buffer->commands;
        list->command_size = buffer->command_size;
    }

    buffer->vertex_size = 0;
    buffer->vertex_needed = 0;
    buffer->command_size = 0;
    buffer->command_needed = 0;
    buffer->clip_size = 0;
    buffer->clip_needed = 0;
}

static gui_int
gui_push_clip(struct gui_draw_buffer *buffer, const struct gui_rect *rect)
{
    struct gui_rect clip;
    assert(buffer);
    assert(rect);

    buffer->clip_needed += sizeof(struct gui_rect);
    if (!buffer || !rect || buffer->clip_size >= buffer->clip_capacity)
        return gui_false;

    clip = unify(rect, (!buffer->clip_size) ? &null_rect : &buffer->clips[buffer->clip_size-1]);
    buffer->clips[buffer->clip_size] = clip;
    buffer->clip_size++;
    return gui_true;
}

static void
gui_pop_clip(struct gui_draw_buffer *buffer)
{
    assert(buffer);
    if (!buffer || !buffer->clip_capacity) return;
    if (buffer->clip_size == 0) {
        buffer->clips[buffer->clip_size] = null_rect;
        buffer->clip_size = 1;
    } else {
        buffer->clip_size--;
    }
}

static gui_int
gui_push_command(struct gui_draw_buffer *buffer, gui_size count, gui_texture tex)
{
    struct gui_draw_command *cmd;
    const struct gui_rect *clip;
    assert(buffer);
    assert(count);
    if (!buffer || !count)
        return gui_false;

    buffer->vertex_needed += count * sizeof(struct gui_vertex);
    buffer->command_needed += sizeof(struct gui_draw_command);
    if (!buffer->commands || buffer->command_size >= buffer->command_capacity ||
        !buffer->command_capacity)
        return gui_false;
    if (!buffer->vertexes || buffer->vertex_size >= buffer->vertex_capacity ||
        !buffer->vertex_capacity)
        return gui_false;

    cmd = &buffer->commands[buffer->command_size++];
    clip = (buffer->clip_size) ? &buffer->clips[buffer->clip_size-1] : &null_rect;
    cmd->vertex_count = count;
    cmd->clip_rect = *clip;
    cmd->texture = tex;
    return gui_true;
}

static void
gui_push_vertex(struct gui_draw_buffer *buffer, gui_float x, gui_float y,
    struct gui_color col, gui_float u, gui_float v)
{
    struct gui_vertex *vertex;
    assert(buffer);
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
    assert(buffer);
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
    assert(buffer);
    if (!buffer) return;
    if (col.a == 0) return;
    if (!gui_push_command(buffer, 6, null_tex)) return;
    gui_vertex_line(buffer, x0, y0, x1, y1, col);
}

static void
gui_draw_trianglef(struct gui_draw_buffer *buffer, gui_float x0, gui_float y0,
    gui_float x1, gui_float y1, gui_float x2, gui_float y2, struct gui_color c)
{
    assert(buffer);
    if (!buffer) return;
    if (c.a == 0) return;
    if (!gui_push_command(buffer, 3, null_tex))
        return;

    gui_push_vertex(buffer, x0, y0, c, 0.0f, 0.0f);
    gui_push_vertex(buffer, x1, y1, c, 0.0f, 0.0f);
    gui_push_vertex(buffer, x2, y2, c, 0.0f, 0.0f);
}

static void
gui_draw_rect(struct gui_draw_buffer *buffer, gui_float x, gui_float y,
    gui_float w, gui_float h, struct gui_color col)
{
    assert(buffer);
    if (!buffer) return;
    if (col.a == 0) return;
    if (!gui_push_command(buffer, 6 * 4, null_tex))
        return;

    gui_vertex_line(buffer, x, y, x + w, y, col);
    gui_vertex_line(buffer, x + w, y, x + w, y + h, col);
    gui_vertex_line(buffer, x + w, y + h, x, y + h, col);
    gui_vertex_line(buffer, x, y + h, x, y, col);
}

static void
gui_draw_rectf(struct gui_draw_buffer *buffer, gui_float x, gui_float y,
    gui_float w, gui_float h, struct gui_color col)
{
    assert(buffer);
    if (!buffer) return;
    if (col.a == 0) return;
    if (!gui_push_command(buffer, 6, null_tex))
        return;

    gui_push_vertex(buffer, x, y, col, 0.0f, 0.0f);
    gui_push_vertex(buffer, x + w, y, col, 0.0f, 0.0f);
    gui_push_vertex(buffer, x + w, y + h, col, 0.0f, 0.0f);
    gui_push_vertex(buffer, x, y, col, 0.0f, 0.0f);
    gui_push_vertex(buffer, x + w, y + h, col, 0.0f, 0.0f);
    gui_push_vertex(buffer, x, y + h, col, 0.0f, 0.0f);
}

static void
gui_draw_diamondf(struct gui_draw_buffer *buffer, gui_float x, gui_float y,
    gui_float w, gui_float h, struct gui_color col)
{
    gui_float wh;
    gui_float hh;

    assert(buffer);
    if (!buffer) return;
    if (col.a == 0) return;
    if (!gui_push_command(buffer, 6, null_tex))
        return;

    wh = w * 0.5f;
    hh = h * 0.5f;
    gui_push_vertex(buffer, x+wh, y, col, 0.0f, 0.0f);
    gui_push_vertex(buffer, x+w, y+hh, col, 0.0f, 0.0f);
    gui_push_vertex(buffer, x+wh, y+h, col, 0.0f, 0.0f);
    gui_push_vertex(buffer, x+wh, y, col, 0.0f, 0.0f);
    gui_push_vertex(buffer, x+wh, y+h, col, 0.0f, 0.0f);
    gui_push_vertex(buffer, x, y + hh, col, 0.0f, 0.0f);
}

static void
gui_draw_string(struct gui_draw_buffer *buffer, const struct gui_font *font, gui_float x,
        gui_float y, gui_float w, gui_float h, struct gui_color col,
        const gui_char *text, gui_size len)
{
    struct gui_rect clip;
    gui_size text_len;
    gui_long unicode;
    const struct gui_font_glyph *g;

    assert(buffer);
    assert(text);
    assert(font);
    if (!buffer || !text || !font || !len)
        return;

    clip.x = x; clip.y = y;
    clip.w = w; clip.h = h;
    if (!gui_push_clip(buffer, &clip)) return;
    if (!gui_push_command(buffer, 6 * len, font->texture)) {
        gui_pop_clip(buffer);
        return;
    }

    text_len = utf_decode(text, &unicode, len);
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
        text_len += utf_decode(text + text_len, &unicode, len - text_len);
        x += char_width;
    }
    gui_pop_clip(buffer);
}

static void
gui_draw_image(struct gui_draw_buffer *buffer, gui_float x, gui_float y,
    gui_float w, gui_float h, gui_texture texture, struct gui_texCoord from,
    struct gui_texCoord to, struct gui_color col)
{
    assert(buffer);
    if (!gui_push_command(buffer, 6, texture)) return;
    gui_push_vertex(buffer, x, y, col, from.u, from.v);
    gui_push_vertex(buffer, x + w, y, col, to.u, from.v);
    gui_push_vertex(buffer, x + w, y + h, col, to.u, to.v);
    gui_push_vertex(buffer, x, y, col, from.u, from.v);
    gui_push_vertex(buffer, x + w, y + h, col, to.u, to.v);
    gui_push_vertex(buffer, x, y + h, col, from.u, to.v);
}

void
gui_widget_text(struct gui_draw_buffer *buffer, const struct gui_text *text,
    const struct gui_font *font)
{
    gui_float label_x;
    gui_float label_y;
    gui_float label_w;
    gui_float label_h;

    assert(buffer);
    assert(text);
    assert(font);
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
gui_widget_text_wrap(struct gui_draw_buffer *buffer, const struct gui_text *text,
    const struct gui_font *font)
{
    gui_size len = 0;
    gui_size lines = 0;
    gui_size chars = 0;

    gui_float label_x, label_y;
    gui_float label_w, label_h;
    gui_float space;
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
gui_widget_image(struct gui_draw_buffer *buffer, const struct gui_image *image)
{
    gui_float image_x;
    gui_float image_y;
    gui_float image_w;
    gui_float image_h;

    assert(buffer);
    assert(image);
    if (!buffer || !image) return;

    image_x = image->x + image->pad_x;
    image_y = image->y + image->pad_y;
    image_w = MAX(0, image->w - 2 * image->pad_x);
    image_h = MAX(0, image->h - 2 * image->pad_y);
    gui_draw_rectf(buffer, image->x, image->y, image->w, image->h, image->background);
    gui_draw_image(buffer, image_x, image_y, image_w, image_h,
        image->texture, image->uv[0], image->uv[1], image->color);
}

static gui_bool
gui_widget_button(struct gui_draw_buffer *buffer, const struct gui_button *button,
        const struct gui_input *in)
{
    gui_bool ret = gui_false;
    struct gui_color background, highlight;

    assert(buffer);
    assert(button);
    if (!buffer || !button)
        return gui_false;

    background = button->background;
    if (in && INBOX(in->mouse_pos.x,in->mouse_pos.y,button->x,button->y,button->w,button->h)) {
        background = button->highlight;
        if (INBOX(in->mouse_clicked_pos.x, in->mouse_clicked_pos.y,
                 button->x, button->y, button->w, button->h)) {
            if (button->behavior == GUI_BUTTON_SWITCH)
                ret = (in->mouse_down && in->mouse_clicked);
            else
                ret = in->mouse_down;
        }
    }
    gui_draw_rectf(buffer, button->x, button->y, button->w, button->h, background);
    gui_draw_rect(buffer, button->x+1, button->y+1, button->w-1, button->h-1, button->foreground);
    return ret;
}

gui_bool
gui_widget_button_text(struct gui_draw_buffer *buffer, const struct gui_button *button,
    const char *text, gui_size length, const struct gui_font *font,
    const struct gui_input *in)
{
    gui_bool ret = gui_false;
    gui_float label_x, label_y, label_w, label_h;
    struct gui_color font_color, background, highlight;
    gui_size text_width;
    gui_float button_w, button_h;

    assert(buffer);
    assert(button);
    assert(text);
    assert(font);
    if (!buffer || !button)
        return gui_false;

    font_color = button->content;
    button_w = MAX(button->w, font->height + 2 * button->pad_x);
    button_h = MAX(button->h, font->height + 2 * button->pad_y);
    if (in && INBOX(in->mouse_pos.x, in->mouse_pos.y, button->x, button->y, button_w, button_h))
        font_color = button->highlight_content;

    ret = gui_widget_button(buffer, button, in);
    if (!font || !text || !length) return ret;

    text_width = gui_font_text_width(font, (const gui_char*)text, length);
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
    return ret;
}

gui_bool
gui_widget_button_triangle(struct gui_draw_buffer *buffer, struct gui_button* button,
    enum gui_heading heading, const struct gui_input *in)
{
    gui_bool pressed;
    struct gui_color col;
    struct gui_vec2 points[3];
    pressed = gui_widget_button(buffer, button, in);
    gui_triangle_from_direction(points, button->x, button->y, button->w, button->h,
        button->pad_x, button->pad_y, heading);
    col = (in && INBOX(in->mouse_pos.x,in->mouse_pos.y,button->x,button->y,button->w,button->h)) ?
        button->highlight_content : button->foreground;
    gui_draw_trianglef(buffer, points[0].x, points[0].y, points[1].x, points[1].y,
        points[2].x, points[2].y, col);
    return pressed;
}

gui_bool
gui_widget_button_icon(struct gui_draw_buffer *buffer, struct gui_button* button,
    gui_texture tex, struct gui_texCoord from, struct gui_texCoord to,
    const struct gui_input *in)
{
    gui_bool pressed;
    struct gui_image image;
    struct gui_color col;
    const struct gui_color color = {255,255,255,255};

    assert(buffer);
    assert(button);
    if (!buffer || !button)
        return gui_false;

    pressed = gui_widget_button(buffer, button, in);
    image.x = button->x + button->pad_x;
    image.y = button->y + button->pad_y;
    image.w = button->w - 2 * button->pad_x;
    image.h = button->h - 2 * button->pad_y;
    image.pad_x = button->pad_x;
    image.pad_y = button->pad_y;
    image.texture = tex;
    image.uv[0] = from;
    image.uv[1] = to;
    col = (in && INBOX(in->mouse_pos.x,in->mouse_pos.y,button->x,button->y,button->w,button->h)) ?
        button->highlight: button->background;
    image.background = col;
    image.color = color;
    gui_widget_image(buffer, &image);
    return pressed;
}

gui_bool
gui_widget_toggle(struct gui_draw_buffer *buffer, const struct gui_toggle *toggle,
    const struct gui_font *font, const struct gui_input *in)
{
    typedef void(*draw_f)(struct gui_draw_buffer*,gui_float,gui_float,
        gui_float, gui_float, struct gui_color);
    static const draw_f gui_draw_function[] = {gui_draw_rectf, gui_draw_diamondf};
    gui_bool toggle_active;
    gui_float select_size;
    gui_float toggle_w, toggle_h;
    gui_float select_x, select_y;
    gui_float cursor_x, cursor_y;
    gui_float cursor_pad, cursor_size;

    assert(buffer);
    assert(toggle);
    assert(font);
    if (!buffer || !toggle || !font)
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
    if (in && !in->mouse_down && in->mouse_clicked) {
        if (INBOX(in->mouse_clicked_pos.x, in->mouse_clicked_pos.y,
                  cursor_x, cursor_y, cursor_size, cursor_size))
            toggle_active = !toggle_active;
    }

    gui_draw_function[toggle->type](buffer, select_x, select_y,
        select_size, select_size, toggle->background);
    if (toggle_active)
        gui_draw_function[toggle->type](buffer, cursor_x, cursor_y, cursor_size,
            cursor_size, toggle->foreground);

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
gui_widget_slider(struct gui_draw_buffer *buffer, const struct gui_slider *slider,
    const struct gui_input *in)
{
    gui_float slider_min, slider_max;
    gui_float slider_value, slider_steps;
    gui_float slider_w, slider_h;
    gui_float cursor_x, cursor_y;
    gui_float cursor_w, cursor_h;

    assert(buffer);
    assert(slider);
    if (!buffer || !slider)
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

    if (in && in->mouse_down &&
        INBOX(in->mouse_pos.x, in->mouse_pos.y, slider->x, slider->y, slider_w, slider_h) &&
        INBOX(in->mouse_clicked_pos.x,in->mouse_clicked_pos.y, slider->x, slider->y,
                slider_w, slider_h))
    {
        const float d = in->mouse_pos.x - (cursor_x + cursor_w / 2.0f);
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

gui_float
gui_widget_slider_vertical(struct gui_draw_buffer *buffer,
    const struct gui_slider *slider, const struct gui_input *in)
{
    gui_float slider_min, slider_max;
    gui_float slider_value, slider_steps;
    gui_float slider_w, slider_h;
    gui_float cursor_x, cursor_y;
    gui_float cursor_w, cursor_h;

    assert(buffer);
    assert(slider);
    if (!buffer || !slider)
        return 0;

    slider_w = MAX(slider->w, 2 * slider->pad_x);
    slider_h = MAX(slider->h, 2 * slider->pad_y);
    slider_max = MAX(slider->min, slider->max);
    slider_min = MIN(slider->min, slider->max);
    slider_value = CLAMP(slider_min, slider->value, slider_max);
    slider_steps = (slider_max - slider_min) / slider->step;

    cursor_w = (slider_w - 2 * slider->pad_x);
    cursor_h = slider_h - 2 * slider->pad_y;
    cursor_h = cursor_h / (((slider_max - slider_min) + slider->step) / slider->step);
    cursor_y = slider->y + slider_h - slider->pad_y - (cursor_h * (slider_value - slider_min));
    cursor_x = slider->x + slider->pad_x;

    if (in && in->mouse_down &&
        INBOX(in->mouse_pos.x, in->mouse_pos.y, slider->x, slider->y, slider_w, slider_h) &&
        INBOX(in->mouse_clicked_pos.x,in->mouse_clicked_pos.y, slider->x, slider->y,
                slider_w, slider_h))
    {
        const float d = in->mouse_pos.y - (cursor_y + cursor_h / 2.0f);
        const float pxstep = (slider_h - 2 * slider->pad_y) / slider_steps;
        if (ABS(d) >= pxstep) {
            slider_value += (d > 0) ? -slider->step : slider->step;
            slider_value = CLAMP(slider_min, slider_value, slider_max);
            cursor_y = slider->y + slider_h - slider->pad_y;
            cursor_y -= (cursor_h * (slider_value - slider_min));
        }
    }
    gui_draw_rectf(buffer, slider->x, slider->y, slider_w, slider_h, slider->background);
    gui_draw_rectf(buffer, cursor_x, cursor_y, cursor_w, cursor_h, slider->foreground);
    return slider_value;
}

gui_size
gui_widget_progress(struct gui_draw_buffer *buffer, const struct gui_progress *prog,
    const struct gui_input *in)
{
    gui_float prog_scale;
    gui_float cursor_x, cursor_y;
    gui_float cursor_w, cursor_h;
    gui_float prog_w, prog_h;
    gui_size prog_value;

    assert(buffer);
    assert(prog);

    if (!buffer || !prog) return 0;
    prog_w = MAX(prog->w, 2 * prog->pad_x + 1);
    prog_h = MAX(prog->h, 2 * prog->pad_y + 1);
    prog_value = MIN(prog->current, prog->max);

    if (in && prog->modifyable && in->mouse_down &&
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

gui_size
gui_widget_progress_vertical(struct gui_draw_buffer *buffer, const struct gui_progress *prog,
    const struct gui_input *in)
{
    gui_float prog_scale;
    gui_float cursor_x, cursor_y;
    gui_float cursor_w, cursor_h;
    gui_float prog_w, prog_h;
    gui_size prog_value;

    assert(buffer);
    assert(prog);

    if (!buffer || !prog) return 0;
    prog_w = MAX(prog->w, 2 * prog->pad_x + 1);
    prog_h = MAX(prog->h, 2 * prog->pad_y + 1);
    prog_value = MIN(prog->current, prog->max);

    if (in && prog->modifyable && in->mouse_down &&
        INBOX(in->mouse_pos.x, in->mouse_pos.y, prog->x, prog->y, prog_w, prog_h)){
        gui_float ratio = (gui_float)(prog->y + prog->h - in->mouse_pos.y) / (gui_float)prog_h;
        prog_value = (gui_size)((gui_float)prog->max * ratio);
    }

    if (!prog->max) return prog_value;
    prog_value = MIN(prog_value, prog->max);
    prog_scale = (gui_float)prog_value / (gui_float)prog->max;

    cursor_w = prog_w - 2 * prog->pad_x;
    cursor_h = (prog_h - 2 * prog->pad_y) * prog_scale;
    cursor_x = prog->x + prog->pad_x;
    cursor_y = prog->y + prog->h - prog->pad_y - cursor_h;

    gui_draw_rectf(buffer, prog->x, prog->y, prog_w, prog_h, prog->background);
    gui_draw_rectf(buffer, cursor_x, cursor_y, cursor_w, cursor_h, prog->foreground);
    return prog_value;
}

static gui_bool
gui_filter_input(gui_long unicode, gui_size len, enum gui_input_filter filter)
{
    if (filter == GUI_INPUT_DEFAULT) {
        return gui_true;
    } else if (filter == GUI_INPUT_FLOAT) {
        if (len > 1)
            return gui_false;
        if ((unicode < '0' || unicode > '9') && unicode != '.' && unicode != '-')
            return gui_false;
        return gui_true;
    } else if (filter == GUI_INPUT_DEC) {
        if (len > 1)
            return gui_false;
        if (unicode < '0' || unicode > '9')
            return gui_false;
        return gui_true;
    } else if (filter == GUI_INPUT_HEX) {
        if (len > 1)
            return gui_false;
        if ((unicode < '0' || unicode > '9') &&
            (unicode < 'a' || unicode > 'f') &&
            (unicode < 'A' || unicode > 'B'))
            return gui_false;
        return gui_true;
    } else if (filter == GUI_INPUT_OCT) {
        if (len > 1)
            return gui_false;
        if (unicode < '0' || unicode > '7')
            return gui_false;
        return gui_true;
    } else if (filter == GUI_INPUT_BIN) {
        if (len > 1)
            return gui_false;
        if (unicode < '0' || unicode > '1')
            return gui_false;
        return gui_true;
    }
    return gui_false;
}

static gui_size
gui_buffer_input(gui_char *buffer, gui_size length, gui_size max,
    enum gui_input_filter filter, const struct gui_input *in)
{
    gui_long unicode;
    gui_size text_len = 0, glyph_len = 0;
    assert(buffer);
    assert(in);

    glyph_len = utf_decode(in->text, &unicode, in->text_len);
    while (glyph_len && (text_len + glyph_len) <= in->text_len && (length + text_len) < max) {
        if (gui_filter_input(unicode, glyph_len, filter)) {
            gui_size i = 0;
            for (i = 0; i < glyph_len; i++)
                buffer[length++] = in->text[text_len + i];
        }
        text_len = text_len + glyph_len;
        glyph_len = utf_decode(in->text + text_len, &unicode, in->text_len - text_len);
    }
    return text_len;
}

gui_bool
gui_widget_input(struct gui_draw_buffer *buf,  gui_char *buffer, gui_size *length,
    const struct gui_input_field *input,
    const struct gui_font *font, const struct gui_input *in)
{
    gui_float input_w, input_h;
    gui_bool input_active;

    assert(buf);
    assert(font);
    assert(input);
    assert(length);
    if (!buffer || !font || !input)
        return 0;

    input_w = MAX(input->w, 2 * input->pad_x);
    input_h = MAX(input->h, font->height);
    input_active = input->active;
    gui_draw_rectf(buf, input->x, input->y, input_w, input_h, input->background);
    gui_draw_rect(buf, input->x + 1, input->y, input_w - 1, input_h, input->foreground);
    if (in && in->mouse_clicked && in->mouse_down) {
        input_active = INBOX(in->mouse_pos.x, in->mouse_pos.y,
                            input->x, input->y, input_w, input_h);
    }

    if (input_active && in) {
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
        if (in->text_len && *length < input->max)
            *length += gui_buffer_input(buffer, *length, input->max, input->filter, in);
    }

    if (font && buffer && length) {
        gui_size offset = 0;
        gui_float label_x, label_y, label_h;
        gui_float label_w = input_w - 2 * input->pad_x;
        gui_size cursor_width = (gui_size)font->glyphes['x'].width;

        gui_size text_len = *length;
        gui_size text_width = gui_font_text_width(font, buffer, text_len);
        while (text_len && (text_width + cursor_width) > (gui_size)label_w) {
            gui_long unicode;
            offset += utf_decode(&buffer[offset], &unicode, text_len);
            text_len -= offset;
            text_width = gui_font_text_width(font, &buffer[offset], text_len);
        }

        label_x = input->x + input->pad_x;
        label_y = input->y + input->pad_y;
        label_h = input_h - 2 * input->pad_y;
        gui_draw_string(buf, font, label_x, label_y, label_w, label_h, input->font,
            &buffer[offset], text_len);
        if (input_active && input->show_cursor) {
            gui_draw_rectf(buf, label_x + (gui_float)text_width, label_y,
                (gui_float)cursor_width, label_h, input->foreground);
        }
    }
    return input_active;
}

gui_int
gui_widget_plot(struct gui_draw_buffer *buffer, const struct gui_plot *plot,
    const struct gui_input *in)
{
    gui_size i;
    gui_size plot_step;
    gui_float plot_w, plot_h;
    gui_int plot_selected = -1;
    gui_float plot_max_value, plot_min_value;
    gui_float plot_value_range, plot_value_ratio;
    gui_float canvas_x, canvas_y;
    gui_float canvas_w, canvas_h;
    gui_float plot_last_x;
    gui_float plot_last_y;
    struct gui_color col;

    assert(buffer);
    assert(plot);
    if (!buffer || !plot)
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

    plot_last_x = canvas_x;
    plot_last_y = (canvas_y + canvas_h) - plot_value_ratio * (gui_float)canvas_h;
    if (in && INBOX(in->mouse_pos.x, in->mouse_pos.y, plot_last_x-3, plot_last_y-3, 6, 6)) {
        plot_selected = (in->mouse_down && in->mouse_clicked) ? (gui_int)i : -1;
        col = plot->highlight;
    }
    gui_draw_rectf(buffer, plot_last_x - 3, plot_last_y - 3, 6, 6, col);

    for (i = 1; i < plot->value_count; i++) {
        gui_float plot_cur_x, plot_cur_y;
        plot_value_ratio = (plot->values[i] - plot_min_value) / plot_value_range;
        plot_cur_x = canvas_x + (gui_float)(plot_step * i);
        plot_cur_y = (canvas_y + canvas_h) - (plot_value_ratio * (gui_float)canvas_h);
        gui_draw_line(buffer, plot_last_x, plot_last_y, plot_cur_x, plot_cur_y, plot->foreground);

        if (in && INBOX(in->mouse_pos.x, in->mouse_pos.y, plot_cur_x-3, plot_cur_y-3, 6, 6)) {
            plot_selected = (in->mouse_down && in->mouse_clicked) ? (gui_int)i : plot_selected;
            col = plot->highlight;
        } else col = plot->foreground;

        gui_draw_rectf(buffer, plot_cur_x - 3, plot_cur_y - 3, 6, 6, col);
        plot_last_x = plot_cur_x, plot_last_y = plot_cur_y;
    }
    return plot_selected;
}

gui_int
gui_widget_histo(struct gui_draw_buffer *buffer, const struct gui_histo *histo,
    const struct gui_input *in)
{
    gui_size i;
    gui_int selected = -1;
    gui_float canvas_x, canvas_y;
    gui_float canvas_w, canvas_h;
    gui_float histo_max_value;
    gui_float histo_w, histo_h;
    gui_float item_w = 0.0f;

    assert(buffer);
    assert(histo);
    if (!buffer || !histo)
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

        if (in && INBOX(in->mouse_pos.x, in->mouse_pos.y, item_x, item_y, item_w, item_h)) {
            selected = (in->mouse_down && in->mouse_clicked) ? (gui_int)i: selected;
            item_color = histo->highlight;
        }
        gui_draw_rectf(buffer, item_x, item_y, item_w, item_h, item_color);
    }
    return selected;
}

gui_float
gui_widget_scroll(struct gui_draw_buffer *buffer, const struct gui_scroll *scroll,
    const struct gui_input *in)
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

    assert(buffer);
    assert(scroll);
    if (!buffer || !scroll) return 0;
    scroll_w = MAX(scroll->w, 0);
    scroll_h = MAX(scroll->h, 2 * scroll_w);
    gui_draw_rectf(buffer, scroll->x, scroll->y, scroll_w, scroll_h, scroll->background);
    gui_draw_rect(buffer, scroll->x, scroll->y, scroll_w, scroll_h, scroll->border);
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
    button.behavior = GUI_BUTTON_SWITCH;
    button_up_pressed = gui_widget_button_triangle(buffer, &button, GUI_UP, in);
    button.y = scroll->y + scroll_h - button.h;
    button_down_pressed = gui_widget_button_triangle(buffer, &button, GUI_DOWN, in);

    scroll_h = scroll_h - 2 * button.h;
    scroll_y = scroll->y + button.h;
    scroll_step = MIN(scroll->step, scroll_h);
    scroll_offset = MIN(scroll->offset, scroll->target - scroll_h);
    scroll_ratio = scroll_h / scroll->target;
    scroll_off = scroll_offset / scroll->target;

    cursor_h = scroll_ratio * scroll_h;
    cursor_y = scroll_y + (scroll_off * scroll_h);
    cursor_w = scroll_w;
    cursor_x = scroll->x;

    if (in) {
        inscroll = INBOX(in->mouse_pos.x,in->mouse_pos.y,scroll->x,scroll->y,scroll_w,scroll_h);
        incursor = INBOX(in->mouse_prev.x,in->mouse_prev.y,cursor_x,cursor_y,cursor_w, cursor_h);
        if (in->mouse_down && inscroll && incursor) {
            const gui_float pixel = in->mouse_delta.y;
            const gui_float delta =  (pixel / scroll_h) * scroll->target;
            scroll_offset = CLAMP(0, scroll_offset + delta, scroll->target - scroll_h);
            cursor_y += pixel;
        } else if (button_up_pressed || button_down_pressed) {
            scroll_offset = (button_down_pressed) ?
                MIN(scroll_offset + scroll_step, scroll->target - scroll_h):
                MAX(0, scroll_offset - scroll_step);
            scroll_off = scroll_offset / scroll->target;
            cursor_y = scroll_y + (scroll_off * scroll_h);
        }
    }
    gui_draw_rectf(buffer, cursor_x, cursor_y, cursor_w, cursor_h, scroll->foreground);
    gui_draw_rect(buffer, cursor_x+1, cursor_y, cursor_w-1, cursor_h, scroll->background);
    return scroll_offset;
}

void
gui_default_config(struct gui_config *config)
{
    if (!config) return;
    config->scrollbar_width = 16;
    vec2_load(config->panel_padding, 15.0f, 10.0f);
    vec2_load(config->panel_min_size, 64.0f, 64.0f);
    vec2_load(config->item_spacing, 8.0f, 8.0f);
    vec2_load(config->item_padding, 4.0f, 4.0f);
    vec2_load(config->scaler_size, 16.0f, 16.0f);
    col_load(config->colors[GUI_COLOR_TEXT], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_PANEL], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_BORDER], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_TITLEBAR], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_BUTTON], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_BUTTON_HOVER], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_BUTTON_HOVER_FONT], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_BUTTON_BORDER], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_CHECK], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_CHECK_ACTIVE], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_OPTION], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_OPTION_ACTIVE], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_SCROLL], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_SLIDER], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_SLIDER_CURSOR], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_PROGRESS], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_PROGRESS_CURSOR], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_INPUT], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_INPUT_BORDER], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_SPINNER], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_SPINNER_BORDER], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_SELECTOR], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_SELECTOR_BORDER], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_HISTO], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_HISTO_BARS], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_HISTO_NEGATIVE], 255, 255, 255, 255);
    col_load(config->colors[GUI_COLOR_HISTO_HIGHLIGHT], 255, 0, 0, 255);
    col_load(config->colors[GUI_COLOR_PLOT], 100, 100, 100, 255);
    col_load(config->colors[GUI_COLOR_PLOT_LINES], 45, 45, 45, 255);
    col_load(config->colors[GUI_COLOR_PLOT_HIGHLIGHT], 255, 0, 0, 255);
    col_load(config->colors[GUI_COLOR_SCROLLBAR], 41, 41, 41, 255);
    col_load(config->colors[GUI_COLOR_SCROLLBAR_CURSOR], 70, 70, 70, 255);
    col_load(config->colors[GUI_COLOR_SCROLLBAR_BORDER], 45, 45, 45, 255);
}

void
gui_panel_init(struct gui_panel *panel, const struct gui_config *config,
    const struct gui_font *font)
{
    panel->flags = 0;
    panel->x = 0;
    panel->y = 0;
    panel->at_y = 0;
    panel->width = 0;
    panel->height = 0;
    panel->index = 0;
    panel->header_height = 0;
    panel->row_height = 0;
    panel->row_columns = 0;
    panel->offset = 0;
    panel->minimized = gui_false;
    panel->out = NULL;
    panel->in = NULL;
    panel->font = font;
    panel->config = config;
}

gui_bool
gui_panel_begin(struct gui_panel *panel, struct gui_draw_buffer *out,
    const struct gui_input *in,  const char *text,  gui_float x, gui_float y,
    gui_float w, gui_float h, gui_flags f)
{
    const struct gui_config *config;
    const struct gui_color *header;
    struct gui_rect clip;

    gui_float mouse_x, mouse_y;
    gui_float clicked_x, clicked_y;
    gui_float header_x, header_w;
    gui_bool ret = gui_true;

    assert(panel);
    assert(out);
    if (!panel || !out)
        return gui_false;
    if (panel->flags & GUI_PANEL_HIDDEN)
        return gui_false;

    panel->out = out;
    panel->in = in;
    panel->x = x;
    panel->y = y;
    panel->width = w;
    panel->at_y = y;
    panel->flags = f;
    panel->index = 0;
    panel->row_columns = 0;

    config = panel->config;
    header = &config->colors[GUI_COLOR_TITLEBAR];
    mouse_x = (panel->in) ? panel->in->mouse_pos.x : -1;
    mouse_y = (panel->in) ? panel->in->mouse_pos.y: -1;
    clicked_x = (panel->in) ? panel->in->mouse_clicked_pos.x: - 1;
    clicked_y = (panel->in) ? panel->in->mouse_clicked_pos.y: - 1;
    header_x = x + config->panel_padding.x;
    header_w = w - 2 * config->panel_padding.x;

    if (!(panel->flags & GUI_PANEL_TAB))
        panel->flags |= GUI_PANEL_SCROLLBAR;
    if (panel->flags & GUI_PANEL_HEADER) {
        panel->header_height = panel->font->height + 3 * config->item_padding.y;
        panel->header_height += config->panel_padding.y;
        gui_draw_rectf(out, x, y, w, panel->header_height, *header);

        clip.x = x; clip.w = w;
        clip.y = y + panel->header_height;
        clip.h = h - panel->header_height;
        if (panel->flags & GUI_PANEL_SCROLLBAR)
            clip.h -= (config->panel_padding.y + config->item_padding.y);
        else clip.h = null_rect.h;
    } else {
        panel->header_height = config->panel_padding.y + config->item_padding.y;
        clip.x = x; clip.y = y;
        clip.w = w; clip.h = h;
        if (panel->flags & GUI_PANEL_SCROLLBAR)
            clip.h -= panel->header_height;
        else clip.h = null_rect.h;
    }

    if (panel->flags & GUI_PANEL_CLOSEABLE && panel->flags & GUI_PANEL_HEADER) {
        const gui_char *X = (const gui_char*)"x";
        const gui_size text_width = gui_font_text_width(panel->font, X, 1);
        const gui_float close_x = header_x + config->item_padding.x;
        const gui_float close_y = y + config->panel_padding.y;
        const gui_float close_w = (gui_float)text_width + config->item_padding.x;
        const gui_float close_h = panel->font->height + 2 * config->item_padding.y;
        gui_draw_string(panel->out, panel->font, close_x, close_y, close_w, close_h,
                        config->colors[GUI_COLOR_TEXT], X, 1);

        header_w -= close_w;
        header_x += close_h - config->item_padding.x;
        if (INBOX(mouse_x, mouse_y, close_x, close_y, close_w, close_h)) {
            if (INBOX(clicked_x, clicked_y, close_x, close_y, close_w, close_h)) {
                ret = !(panel->in->mouse_down && panel->in->mouse_clicked);
                if (ret) panel->flags |= GUI_PANEL_HIDDEN;
            }
        }
    }

    if (panel->flags & GUI_PANEL_MINIMIZABLE && panel->flags & GUI_PANEL_HEADER) {
        gui_size text_width;
        gui_float min_x, min_y, min_w, min_h;
        const gui_char *score = (panel->minimized) ?
            (const gui_char*)"+":
            (const gui_char*)"-";

        text_width = gui_font_text_width(panel->font, score, 1);
        min_x = header_x + config->item_padding.x;
        min_y = y + config->panel_padding.y;
        min_w = (gui_float)text_width + 2 * config->item_padding.x;
        min_h = panel->font->height + 2 * config->item_padding.y;
        gui_draw_string(panel->out, panel->font, min_x, min_y, min_w, min_h,
                        config->colors[GUI_COLOR_TEXT], score, 1);

        header_w -= min_w;
        header_x += min_w - config->item_padding.x;
        if (INBOX(mouse_x, mouse_y, min_x, min_y, min_w, min_h)) {
            if (INBOX(clicked_x, clicked_y, min_x, min_y, min_w, min_h))
                if (panel->in->mouse_down && panel->in->mouse_clicked)
                    panel->minimized = !panel->minimized;
        }
    }

    if (panel->flags & GUI_PANEL_HEADER && text) {
        const gui_size text_len = strsiz(text);
        const gui_float label_x = header_x + config->item_padding.x;
        const gui_float label_y = y + config->panel_padding.y;
        const gui_float label_w = header_w - (2 * config->item_padding.x);
        const gui_float label_h = panel->font->height + 2 * config->item_padding.y;
        gui_draw_string(panel->out, panel->font, label_x, label_y, label_w, label_h,
            config->colors[GUI_COLOR_TEXT], (const gui_char*)text, text_len);
    }

    panel->row_height = panel->header_height;
    if (panel->flags & GUI_PANEL_SCROLLBAR) {
        const struct gui_color *color = &config->colors[GUI_COLOR_PANEL];
        panel->width -= config->scrollbar_width;
        panel->height = (panel->flags & GUI_PANEL_HEADER) ? (h - panel->header_height) : h;
        if (!(panel->flags & GUI_PANEL_HEADER))
            panel->height -= (config->panel_padding.y + config->item_padding.y);
        if (!panel->minimized)
            gui_draw_rectf(panel->out, x, y + panel->header_height, w,
                            h - panel->header_height, *color);
    }

    if (panel->flags & GUI_PANEL_BORDER) {
        const struct gui_color *color;
        color = &config->colors[GUI_COLOR_BORDER];
        gui_draw_line(panel->out, x, y, x + w, y, *color);
    }
    gui_push_clip(out, &clip);
    return ret;
}

void
gui_panel_layout(struct gui_panel *panel, gui_float height, gui_size cols)
{
    const struct gui_config *config;
    const struct gui_color *color;

    assert(panel);
    if (!panel) return;
    if (panel->minimized || (panel->flags & GUI_PANEL_HIDDEN)) return;

    assert(panel->config);
    assert(panel->out);
    config = panel->config;
    color = &config->colors[GUI_COLOR_PANEL];

    panel->index = 0;
    panel->at_y += panel->row_height;
    panel->row_columns = cols;
    panel->row_height = height + config->item_spacing.y;
    gui_draw_rectf(panel->out, panel->x, panel->at_y, panel->width,
                    height + config->panel_padding.y, *color);
}

void
gui_panel_seperator(struct gui_panel *panel, gui_size cols)
{
    const struct gui_config *config;

    assert(panel);
    assert(panel->config);
    assert(panel->out);

    if (!panel) return;
    if (panel->minimized || (panel->flags & GUI_PANEL_HIDDEN)) return;
    config = panel->config;
    cols = MIN(cols, panel->row_columns - panel->index);
    panel->index += cols;
    if (panel->index >= panel->row_columns) {
        const gui_float row_height = panel->row_height - config->item_spacing.y;
        gui_panel_layout(panel, row_height, panel->row_columns);
    }
}

static void
gui_panel_alloc_space(struct gui_rect *bounds, struct gui_panel *panel)
{
    const struct gui_config *config;
    gui_float panel_padding, panel_spacing, panel_space;
    gui_float item_offset, item_width, item_spacing;

    assert(panel);
    assert(panel->config);
    assert(bounds);

    config = panel->config;
    if (panel->index >= panel->row_columns) {
        const gui_float row_height = panel->row_height - config->item_spacing.y;
        gui_panel_layout(panel, row_height, panel->row_columns);
    }

    panel_padding = 2 * config->panel_padding.x;
    panel_spacing = (gui_float)(panel->row_columns - 1) * config->item_spacing.x;
    panel_space  = panel->width - panel_padding - panel_spacing;

    item_width = panel_space / (gui_float)panel->row_columns;
    item_offset = (gui_float)panel->index * item_width;
    item_spacing = (gui_float)panel->index * config->item_spacing.x;

    bounds->x = panel->x + item_offset + item_spacing + config->panel_padding.x;
    bounds->y = panel->at_y - panel->offset;
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

    assert(panel);
    assert(panel->config);
    assert(panel->out);
    assert(panel->font);
    assert(str && len);

    if (!panel || !panel->config || !panel->out || !panel->font) return;
    if (panel->minimized || (panel->flags & GUI_PANEL_HIDDEN)) return;
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
    gui_widget_text(panel->out, &text, panel->font);
}

gui_bool
gui_panel_button_text(struct gui_panel *panel, const char *str,
    enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;
    gui_size len;

    assert(panel);
    assert(panel->config);
    assert(panel->out);
    assert(panel->font);

    if (!panel || !panel->config || !panel->out || !panel->font) return 0;
    if (panel->minimized || (panel->flags & GUI_PANEL_HIDDEN)) return 0;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;
    len = strsiz(str);

    button.x = bounds.x;
    button.y = bounds.y;
    button.w = bounds.w;
    button.h = bounds.h;
    button.behavior = behavior;
    button.pad_x = config->item_padding.x;
    button.pad_y = config->item_padding.y;
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
    button.content = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
    button.highlight_content = config->colors[GUI_COLOR_BUTTON_HOVER_FONT];
    return gui_widget_button_text(panel->out, &button, str, len, panel->font, panel->in);
}

gui_bool gui_panel_button_color(struct gui_panel *panel, const struct gui_color color,
    enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;

    assert(panel);
    assert(panel->config);
    assert(panel->out);

    if (!panel || !panel->config || !panel->out) return 0;
    if (panel->minimized || (panel->flags & GUI_PANEL_HIDDEN)) return 0;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;

    button.x = bounds.x;
    button.y = bounds.y;
    button.w = bounds.w;
    button.h = bounds.h;
    button.behavior = behavior;
    button.pad_x = config->item_padding.x;
    button.pad_y = config->item_padding.y;
    button.background = color;
    button.foreground = color;
    button.highlight = color;
    button.highlight_content = config->colors[GUI_COLOR_BUTTON_HOVER_FONT];
    return gui_widget_button(panel->out, &button, panel->in);
}

gui_bool
gui_panel_button_triangle(struct gui_panel *panel, enum gui_heading heading,
    enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;

    assert(panel);
    assert(panel->config);
    assert(panel->out);

    if (!panel || !panel->config || !panel->out) return 0;
    if (panel->minimized || (panel->flags & GUI_PANEL_HIDDEN)) return 0;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;

    button.x = bounds.x;
    button.y = bounds.y;
    button.w = bounds.w;
    button.h = bounds.h;
    button.behavior = behavior;
    button.pad_x = config->item_padding.x;
    button.pad_y = config->item_padding.y;
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
    button.content = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
    button.highlight_content = config->colors[GUI_COLOR_BUTTON_HOVER_FONT];
    return gui_widget_button_triangle(panel->out, &button, heading, panel->in);
}

gui_bool
gui_panel_button_icon(struct gui_panel *panel, gui_texture tex,
    struct gui_texCoord from, struct gui_texCoord to,
    enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;

    assert(panel);
    assert(panel->config);
    assert(panel->out);

    if (!panel || !panel->config || !panel->out) return 0;
    if (panel->minimized || (panel->flags & GUI_PANEL_HIDDEN)) return 0;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;
    button.x = bounds.x;
    button.y = bounds.y;
    button.w = bounds.w;
    button.h = bounds.h;
    button.behavior = behavior;
    button.pad_x = config->item_padding.x;
    button.pad_y = config->item_padding.y;
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
    button.content = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
    button.highlight_content = config->colors[GUI_COLOR_BUTTON_HOVER];
    return gui_widget_button_icon(panel->out, &button, tex, from, to, panel->in);
}

gui_bool
gui_panel_button_toggle(struct gui_panel *panel, const char *str, gui_bool value)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;
    gui_size len;

    assert(panel);
    assert(panel->config);
    assert(panel->out);
    assert(str);

    if (!panel || !panel->config || !panel->out) return 0;
    if (panel->minimized || (panel->flags & GUI_PANEL_HIDDEN)) return 0;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;
    len = strsiz(str);

    button.x = bounds.x;
    button.y = bounds.y;
    button.w = bounds.w;
    button.h = bounds.h;
    button.behavior = GUI_BUTTON_SWITCH;
    button.pad_x = config->item_padding.x;
    button.pad_y = config->item_padding.y;
    if (!value) {
        button.background = config->colors[GUI_COLOR_BUTTON];
        button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
        button.content = config->colors[GUI_COLOR_TEXT];
        button.highlight = config->colors[GUI_COLOR_BUTTON];
        button.highlight_content = config->colors[GUI_COLOR_TEXT];
    } else {
        button.background = config->colors[GUI_COLOR_BUTTON_HOVER];
        button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
        button.content = config->colors[GUI_COLOR_BUTTON];
        button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
        button.highlight_content = config->colors[GUI_COLOR_BUTTON];
    }
    if (gui_widget_button_text(panel->out, &button, str, len, panel->font, panel->in))
        value = !value;
    return value;
}

gui_bool
gui_panel_check(struct gui_panel *panel, const char *text, gui_bool is_active)
{
    struct gui_rect bounds;
    struct gui_toggle toggle;
    const struct gui_config *config;
    gui_size length;

    assert(panel);
    assert(panel->config);
    assert(panel->out);
    assert(text);

    if (!panel || !panel->config || !panel->out) return is_active;
    if (panel->minimized || (panel->flags & GUI_PANEL_HIDDEN)) return is_active;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;
    length = strsiz(text);

    toggle.x = bounds.x;
    toggle.y = bounds.y;
    toggle.w = bounds.w;
    toggle.h = bounds.h;
    toggle.pad_x = config->item_padding.x;
    toggle.pad_y = config->item_padding.y;
    toggle.active = is_active;
    toggle.text = (const gui_char*)text;
    toggle.length = length;
    toggle.type = GUI_TOGGLE_CHECK;
    toggle.font = config->colors[GUI_COLOR_TEXT];
    toggle.background = config->colors[GUI_COLOR_CHECK];
    toggle.foreground = config->colors[GUI_COLOR_CHECK_ACTIVE];
    return gui_widget_toggle(panel->out, &toggle, panel->font, panel->in);
}

gui_bool
gui_panel_option(struct gui_panel *panel, const char *text, gui_bool is_active)
{
    struct gui_rect bounds;
    struct gui_toggle toggle;
    const struct gui_config *config;
    gui_size length;

    assert(panel);
    assert(panel->config);
    assert(panel->out);
    assert(text);

    if (!panel || !panel->config || !panel->out) return is_active;
    if (panel->minimized || (panel->flags & GUI_PANEL_HIDDEN)) return is_active;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;
    length = strsiz(text);

    toggle.x = bounds.x;
    toggle.y = bounds.y;
    toggle.w = bounds.w;
    toggle.h = bounds.h;
    toggle.pad_x = config->item_padding.x;
    toggle.pad_y = config->item_padding.y;
    toggle.active = is_active;
    toggle.text = (const gui_char*)text;
    toggle.length = length;
    toggle.type = GUI_TOGGLE_OPTION;
    toggle.font = config->colors[GUI_COLOR_TEXT];
    toggle.background = config->colors[GUI_COLOR_CHECK];
    toggle.foreground = config->colors[GUI_COLOR_CHECK_ACTIVE];
    return gui_widget_toggle(panel->out, &toggle, panel->font, panel->in);
}

gui_float
gui_panel_slider(struct gui_panel *panel, gui_float min_value, gui_float value,
    gui_float max_value, gui_float value_step, enum gui_direction direction)
{
    struct gui_rect bounds;
    struct gui_slider slider;
    const struct gui_config *config;

    assert(panel);
    assert(panel->config);
    assert(panel->out);

    if (!panel || !panel->config || !panel->out) return value;
    if (panel->minimized || (panel->flags & GUI_PANEL_HIDDEN)) return value;
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
    if (direction == GUI_HORIZONTAL)
        return gui_widget_slider(panel->out, &slider, panel->in);
    return gui_widget_slider_vertical(panel->out, &slider, panel->in);
}

gui_size
gui_panel_progress(struct gui_panel *panel, gui_size cur_value, gui_size max_value,
    gui_bool is_modifyable, enum gui_direction direction)
{
    struct gui_rect bounds;
    struct gui_progress prog;
    const struct gui_config *config;

    assert(panel);
    assert(panel->config);
    assert(panel->out);

    if (!panel || !panel->config || !panel->out) return cur_value;
    if (panel->minimized || (panel->flags & GUI_PANEL_HIDDEN)) return cur_value;
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
    if (direction == GUI_HORIZONTAL)
        return gui_widget_progress(panel->out, &prog, panel->in);
    return gui_widget_progress_vertical(panel->out, &prog, panel->in);
}

gui_bool
gui_panel_input(struct gui_panel *panel, gui_char *buffer, gui_size *length,
    gui_size max_length, enum gui_input_filter filter, gui_bool is_active)
{
    struct gui_rect bounds;
    struct gui_input_field field;
    const struct gui_config *config;

    assert(panel);
    assert(panel->config);
    assert(panel->out);
    assert(panel->font);
    assert(buffer);
    assert(length);

    if (!panel || !panel->config || !panel->out || !panel->font) return 0;
    if (panel->minimized || (panel->flags & GUI_PANEL_HIDDEN)) return 0;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;

    field.x = bounds.x;
    field.y = bounds.y;
    field.w = bounds.w;
    field.h = bounds.h;
    field.pad_x = config->item_padding.x;
    field.pad_y = config->item_padding.y;
    field.max  = max_length;
    field.filter = filter;
    field.active = is_active;
    field.show_cursor = gui_true;
    field.font = config->colors[GUI_COLOR_TEXT];
    field.background = config->colors[GUI_COLOR_INPUT];
    field.foreground = config->colors[GUI_COLOR_INPUT_BORDER];
    return gui_widget_input(panel->out, buffer, length, &field, panel->font, panel->in);
}

gui_size
gui_panel_shell(struct gui_panel *panel, gui_char *buffer, gui_size *length,
    gui_size max, gui_bool *active)
{
    struct gui_rect bounds;
    gui_size submit = 0;
    gui_size space = 0;
    struct gui_button button;
    struct gui_input_field field;
    const struct gui_config *config;

    assert(panel);
    assert(panel->config);
    assert(panel->out);
    assert(panel->font);
    assert(buffer);
    assert(length);
    assert(active);

    if (!panel || !panel->config || !panel->out || !panel->font) return 0;
    if (panel->minimized || (panel->flags & GUI_PANEL_HIDDEN)) return 0;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;

    field.x = bounds.x;
    field.y = bounds.y;
    field.w = bounds.w;
    field.h = bounds.h;
    field.pad_x = config->item_padding.x;
    field.pad_y = config->item_padding.y;
    field.max  = max;
    field.filter = GUI_INPUT_DEFAULT;
    field.active = *active;
    field.font = config->colors[GUI_COLOR_TEXT];
    field.background = config->colors[GUI_COLOR_INPUT];
    field.foreground = config->colors[GUI_COLOR_INPUT_BORDER];

    space = gui_font_text_width(panel->font, (const gui_char*)"Submit", 6);
    button.y = field.y;
    button.h = field.h;
    button.w = (gui_float)space + 2 * field.pad_x - 1;
    button.x = field.x + field.w - button.w + 1;
    button.pad_x = field.pad_x;
    button.pad_y = field.pad_y;
    button.behavior = GUI_BUTTON_SWITCH;
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
    button.content = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
    button.highlight_content = config->colors[GUI_COLOR_BUTTON_HOVER_FONT];
    if (gui_widget_button_text(panel->out, &button, "Submit", 6, panel->font, panel->in )) {
        submit = *length;
        *length = 0;
    }

    field.w = field.w - button.w;
    *active = gui_widget_input(panel->out, (gui_char*)buffer, length, &field,
                                panel->font, panel->in);
    if (!submit && active && panel->in) {
        const struct gui_key *enter = &panel->in->keys[GUI_KEY_ENTER];
        if ((enter->down && enter->clicked)) {
            submit = *length;
            *length = 0;
        }
    }
    return submit;
}

gui_bool
gui_panel_spinner(struct gui_panel *panel, gui_int min, gui_int *value,
    gui_int max, gui_int step, gui_bool active)
{
    struct gui_rect bounds;
    const struct gui_config *config;
    struct gui_input_field field;
    char string[MAX_NUMBER_BUFFER];
    gui_size len, old_len;

    struct gui_button button;
    gui_bool is_active, updated = gui_false;
    gui_bool button_up_clicked, button_down_clicked;

    assert(panel);
    assert(panel->config);
    assert(panel->out);
    assert(panel->font);
    assert(value);

    if (!panel || !panel->config || !panel->out || !panel->font) return 0;
    if (panel->minimized || (panel->flags & GUI_PANEL_HIDDEN)) return 0;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;

    *value = CLAMP(min, *value, max);
    len = itos(string, *value);
    is_active = active;
    old_len = len;

    button.y = bounds.y;
    button.h = bounds.h / 2;
    button.w = bounds.h - config->item_padding.x + 1;
    button.x = bounds.x + bounds.w - button.w - 1;
    button.pad_x = MAX(3, button.h - panel->font->height);
    button.pad_y = MAX(3, button.h - panel->font->height);
    button.behavior = GUI_BUTTON_SWITCH;
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
    button.content = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON];
    button.highlight_content = config->colors[GUI_COLOR_TEXT];
    button_up_clicked = gui_widget_button_triangle(panel->out, &button, GUI_UP, panel->in);
    button.y = bounds.y + button.h;
    button_down_clicked = gui_widget_button_triangle(panel->out, &button, GUI_DOWN, panel->in);
    if (button_up_clicked || button_down_clicked) {
        *value += (button_up_clicked) ? step : -step;
        *value = CLAMP(min, *value, max);
    }

    field.x = bounds.x;
    field.y = bounds.y;
    field.w = bounds.w - button.w;
    field.h = bounds.h;
    field.pad_x = config->item_padding.x;
    field.pad_y = config->item_padding.y;
    field.max  = MAX_NUMBER_BUFFER;
    field.filter = GUI_INPUT_FLOAT;
    field.active = is_active;
    field.show_cursor = gui_false;
    field.font = config->colors[GUI_COLOR_TEXT];
    field.background = config->colors[GUI_COLOR_SPINNER];
    field.foreground = config->colors[GUI_COLOR_SPINNER_BORDER];
    is_active = gui_widget_input(panel->out, (gui_char*)string, &len, &field,
                                panel->font, panel->in);
    if (old_len != len)
        strtoi(value, string, len);
    return is_active;
}

gui_size
gui_panel_selector(struct gui_panel *panel, const char *items[],
    gui_size item_count, gui_size item_current)
{
    gui_size text_len;
    gui_float label_x, label_y;
    gui_float label_w, label_h;

    struct gui_color color;
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;
    gui_bool button_up_clicked, button_down_clicked;

    assert(panel);
    assert(panel->config);
    assert(panel->out);
    assert(panel->font);
    assert(items);
    assert(item_count);
    assert(item_current < item_count);

    if (!panel || !panel->config || !panel->out || !panel->font) return 0;
    if (panel->minimized || (panel->flags & GUI_PANEL_HIDDEN)) return 0;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;

    gui_draw_rectf(panel->out, bounds.x, bounds.y, bounds.w, bounds.h,
            config->colors[GUI_COLOR_SELECTOR]);
    gui_draw_rect(panel->out, bounds.x, bounds.y, bounds.w, bounds.h,
            config->colors[GUI_COLOR_SELECTOR_BORDER]);

    button.y = bounds.y;
    button.h = bounds.h / 2;
    button.w = bounds.h - config->item_padding.x + 1;
    button.x = bounds.x + bounds.w - button.w - 1;
    button.pad_x = MAX(3, button.h - panel->font->height);
    button.pad_y = MAX(3, button.h - panel->font->height);
    button.behavior = GUI_BUTTON_SWITCH;
    button.behavior = GUI_BUTTON_SWITCH;
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
    button.content = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON];
    button.highlight_content = config->colors[GUI_COLOR_TEXT];
    button_up_clicked = gui_widget_button_triangle(panel->out, &button, GUI_UP, panel->in);
    button.y = bounds.y + button.h;
    button_down_clicked = gui_widget_button_triangle(panel->out, &button, GUI_DOWN, panel->in);
    item_current = (button_down_clicked && item_current < item_count-1) ? item_current+1:
        (button_up_clicked && item_current > 0) ? item_current-1 : item_current;

    label_x = bounds.x + config->item_padding.x;
    label_y = bounds.y + config->item_padding.y;
    label_w = bounds.w - (button.w + 2 * config->item_padding.x);
    label_h = bounds.h - 2 * config->item_padding.y;
    text_len = strsiz(items[item_current]);
    color = config->colors[GUI_COLOR_TEXT];
    gui_draw_string(panel->out, panel->font, label_x, label_y, label_w, label_h,
                    color, (const gui_char*)items[item_current], text_len);
    return item_current;
}

gui_int
gui_panel_plot(struct gui_panel *panel, const gui_float *values, gui_size count)
{
    struct gui_rect bounds;
    struct gui_plot plot;
    const struct gui_config *config;

    assert(panel);
    assert(panel->config);
    assert(panel->out);
    assert(values);
    assert(count);

    if (!panel || !panel->config || !panel->out) return -1;
    if (panel->minimized || (panel->flags & GUI_PANEL_HIDDEN)) return -1;
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
    return gui_widget_plot(panel->out, &plot, panel->in);
}

gui_int
gui_panel_histo(struct gui_panel *panel, const gui_float *values, gui_size count)
{
    struct gui_rect bounds;
    struct gui_histo histo;
    const struct gui_config *config;

    assert(panel);
    assert(panel->config);
    assert(panel->out);
    assert(values);
    assert(count);

    if (!panel || !panel->config || !panel->out) return -1;
    if (panel->minimized || (panel->flags & GUI_PANEL_HIDDEN)) return -1;
    gui_panel_alloc_space(&bounds, panel);
    config = panel->config;

    histo.x = bounds.x;
    histo.y = bounds.y;
    histo.w = bounds.w;
    histo.h = bounds.h;
    histo.pad_x = config->item_padding.x;
    histo.pad_y = config->item_padding.y;
    histo.values = values;
    histo.value_count = count;
    histo.background = config->colors[GUI_COLOR_HISTO];
    histo.foreground = config->colors[GUI_COLOR_HISTO_BARS];
    histo.negative = config->colors[GUI_COLOR_HISTO_NEGATIVE];
    histo.highlight = config->colors[GUI_COLOR_HISTO_HIGHLIGHT];
    return gui_widget_histo(panel->out, &histo, panel->in);
}

gui_bool
gui_panel_tab_begin(struct gui_panel *panel, gui_tab *tab, const char *title)
{
    struct gui_rect bounds;
    gui_float old_height;
    gui_size old_cols;
    gui_flags flags;
    gui_bool min;

    assert(panel);
    assert(tab);
    if (!panel || !tab) return gui_true;
    if (panel->minimized || (panel->flags & GUI_PANEL_HIDDEN))
        return gui_false;

    old_height = panel->row_height;
    old_cols = panel->row_columns;
    panel->index = 0;
    panel->row_columns = 1;
    panel->row_height = 0;
    gui_panel_alloc_space(&bounds, panel);
    panel->row_columns = old_cols;
    panel->row_height = old_height;
    min = tab->minimized;

    gui_panel_init(tab, panel->config, panel->font);
    tab->minimized = min;
        flags = GUI_PANEL_BORDER|GUI_PANEL_MINIMIZABLE|GUI_PANEL_HEADER|GUI_PANEL_TAB;
    gui_panel_begin(tab, panel->out, panel->in, title,
        bounds.x, bounds.y + 1, bounds.w, null_rect.h, flags);
    return tab->minimized;
}

void
gui_panel_tab_end(struct gui_panel *panel, struct gui_panel *tab)
{
    assert(panel);
    assert(tab);
    if (!panel || !tab) return;
    if (panel->minimized || (panel->flags & GUI_PANEL_HIDDEN)) return;
    gui_panel_end(tab);
    panel->at_y -= panel->row_height;
    panel->at_y += tab->height + tab->config->item_spacing.y;
}

void
gui_panel_group_begin(struct gui_panel *panel, gui_group *group, const char *title)
{
    gui_flags flags;
    struct gui_rect bounds;
    gui_float offset;

    assert(panel);
    assert(group);
    if (!panel || !group) return;
    if ((panel->flags & GUI_PANEL_HIDDEN) || panel->minimized) return;
    offset = group->offset;
    gui_panel_alloc_space(&bounds, panel);
    gui_panel_init(group, panel->config, panel->font);
    flags = GUI_PANEL_BORDER|GUI_PANEL_HEADER|GUI_PANEL_SCROLLBAR|GUI_PANEL_TAB;
    group->offset = offset;
    gui_panel_begin(group, panel->out, panel->in, title,
        bounds.x, bounds.y, bounds.w, bounds.h, flags);
}

void
gui_panel_group_end(struct gui_panel *panel, gui_group* group)
{
    assert(panel);
    assert(group);
    if (!panel || !group) return;
    if (panel->minimized || (panel->flags & GUI_PANEL_HIDDEN)) return;
    gui_panel_end(group);
}

gui_size
gui_panel_shelf_begin(struct gui_panel *panel, gui_shelf *shelf,
    const char *tabs[], gui_size size, gui_size active)
{
    gui_size i;
    gui_flags flags;
    struct gui_rect bounds;
    const struct gui_config *config;
    gui_float header_x, header_y;
    gui_float header_w, header_h;
    gui_float item_width;
    gui_float offset;

    assert(panel);
    assert(tabs);
    assert(shelf);
    assert(active < size);
    if (!panel || !shelf || !tabs || active >= size)
        return active;
    if ((panel->flags & GUI_PANEL_HIDDEN) || panel->minimized)
        return active;

    config = panel->config;
    gui_panel_alloc_space(&bounds, panel);

    header_x = bounds.x;
    header_y = bounds.y;
    header_w = bounds.w;
    header_h = config->panel_padding.y + 3 * config->item_padding.y + panel->font->height;
    item_width = (header_w - (gui_float)size) / (gui_float)size;
    for (i = 0; i < size; i++) {
        struct gui_button button;
        button.y = header_y;
        button.h = header_h;
        button.x = header_x + ((gui_float)i * (item_width + 1));
        button.w = item_width;
        button.pad_x = config->item_padding.x;
        button.pad_y = config->item_padding.y;
        button.behavior = GUI_BUTTON_SWITCH;
        if (active == i) {
            button.background = config->colors[GUI_COLOR_BUTTON];
            button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
            button.content = config->colors[GUI_COLOR_TEXT];
            button.highlight = config->colors[GUI_COLOR_BUTTON];
            button.highlight_content = config->colors[GUI_COLOR_TEXT];
        } else {
            button.y += config->item_padding.y;
            button.h -= config->item_padding.y;
            button.background = config->colors[GUI_COLOR_BUTTON];
            button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
            button.content = config->colors[GUI_COLOR_TEXT];
            button.highlight = config->colors[GUI_COLOR_BUTTON];
            button.highlight_content = config->colors[GUI_COLOR_TEXT];
        }
        if (gui_widget_button_text(panel->out, &button, tabs[i], strsiz(tabs[i]),
                panel->font, panel->in)) active = i;
    }

    bounds.y += header_h;
    bounds.h -= header_h;
    offset = shelf->offset;
    gui_panel_init(shelf, panel->config, panel->font);
    shelf->offset = offset;
    flags = GUI_PANEL_BORDER|GUI_PANEL_SCROLLBAR|GUI_PANEL_TAB;
    gui_panel_begin(shelf, panel->out, panel->in, NULL,
        bounds.x, bounds.y, bounds.w, bounds.h, flags);
    return active;
}

void
gui_panel_shelf_end(struct gui_panel *panel, gui_shelf *shelf)
{
    assert(panel);
    assert(shelf);
    if (!panel || !shelf) return;
    if (panel->minimized || (panel->flags & GUI_PANEL_HIDDEN)) return;
    gui_panel_end(shelf);
}

void
gui_panel_end(struct gui_panel *panel)
{
    const struct gui_config *config;
    assert(panel);
    if (!panel) return;
    if (panel->flags & GUI_PANEL_HIDDEN) return;
    gui_pop_clip(panel->out);
    panel->at_y += panel->row_height;

    config = panel->config;
    if (panel->flags & GUI_PANEL_SCROLLBAR && !panel->minimized) {
        gui_float panel_y;
        struct gui_scroll scroll;
        scroll.x = panel->x + panel->width;
        scroll.y = (panel->flags & GUI_PANEL_BORDER) ? panel->y + 1 : panel->y;
        scroll.w = config->scrollbar_width;
        scroll.offset = panel->offset;
        scroll.step = panel->height * 0.25f;
        scroll.background = config->colors[GUI_COLOR_SCROLLBAR];
        scroll.foreground = config->colors[GUI_COLOR_SCROLLBAR_CURSOR];
        scroll.border = config->colors[GUI_COLOR_SCROLLBAR_BORDER];
        scroll.h = panel->height;
        if (panel->flags & GUI_PANEL_HEADER) scroll.y += panel->header_height;
        else scroll.h += panel->header_height;
        if (panel->flags & GUI_PANEL_BORDER) scroll.h -= 1;
        scroll.target = panel->at_y - panel->y;
        if (panel->flags & GUI_PANEL_HEADER)
            scroll.target -= panel->header_height;

        panel->offset = (gui_float)gui_widget_scroll(panel->out, &scroll, panel->in);
        panel_y = panel->y + panel->height + panel->header_height - config->panel_padding.y;
        gui_draw_rectf(panel->out, panel->x, panel_y, panel->width, config->panel_padding.y,
                        config->colors[GUI_COLOR_PANEL]);
    } else panel->height = panel->at_y - panel->y;

    if (panel->flags & GUI_PANEL_BORDER) {
        const gui_float width = (panel->flags & GUI_PANEL_SCROLLBAR) ?
                panel->width + config->scrollbar_width : panel->width;
        const gui_float padding_y = (panel->minimized) ?
                panel->y + panel->header_height:
                (panel->flags & GUI_PANEL_SCROLLBAR) ?
                panel->y + panel->height + panel->header_height:
                panel->y + panel->height + config->item_padding.y;

        gui_draw_line(panel->out, panel->x, padding_y, panel->x + width,
                padding_y, config->colors[GUI_COLOR_BORDER]);
        gui_draw_line(panel->out, panel->x, panel->y, panel->x,
                padding_y, config->colors[GUI_COLOR_BORDER]);
        gui_draw_line(panel->out, panel->x + width, panel->y, panel->x + width,
                padding_y, config->colors[GUI_COLOR_BORDER]);
    }
}

struct gui_context*
gui_new(const struct gui_memory *memory, const struct gui_input *input)
{
    void *ptr;
    gui_size size;
    struct gui_memory buffer;
    struct gui_context *ctx;
    static const gui_size align_panels = ALIGNOF(struct gui_context_panel);
    static const gui_size align_list = ALIGNOF(struct gui_draw_call_list**);

    assert(memory);
    assert(input);
    if (!memory  || !input)
        return NULL;

    ctx = memory->memory;
    ctx->width = 0;
    ctx->height = 0;
    ctx->input = input;
    ctx->active = NULL;
    ctx->free_list = NULL;
    ctx->stack_begin = NULL;
    ctx->stack_end = NULL;

    ptr = (gui_byte*)memory->memory + sizeof(struct gui_context);
    ctx->panel_pool = ALIGN(ptr, align_panels);
    ctx->panel_capacity = memory->max_panels;
    ctx->panel_size = 0;

    size = sizeof(struct gui_context_panel) * memory->max_panels;
    ptr = (gui_byte*)ctx->panel_pool + size;
    ctx->output_list = ALIGN(ptr, align_list);

    size = sizeof(struct gui_draw_call_list*) * memory->max_panels;
    buffer.memory = (gui_byte*)ctx->output_list + size;
    buffer.size = memory->size - (gui_size)((gui_byte*)buffer.memory - (gui_byte*)memory->memory);
    buffer.vertex_percentage = memory->vertex_percentage;
    buffer.command_percentage = memory->command_percentage;
    buffer.clip_percentage = memory->clip_percentage;
    gui_output_begin(&ctx->global_buffer, &buffer);
    return ctx;
}

static struct gui_context_panel*
gui_alloc_panel(struct gui_context *ctx)
{
    assert(ctx);
    if (ctx->free_list) {
        struct gui_context_panel *panel;
        panel = ctx->free_list;
        ctx->free_list = panel->next;
        panel->next = NULL;
        panel->prev = NULL;
        return panel;
    } else if (ctx->panel_capacity) {
        ctx->panel_capacity--;
        return &ctx->panel_pool[ctx->panel_capacity];
    }
    return NULL;
}

static void
gui_free_panel(struct gui_context *ctx, struct gui_context_panel *panel)
{
    assert(ctx);
    assert(panel);
    panel->next = ctx->free_list;
    ctx->free_list = panel;
}

static void
gui_stack_push(struct gui_context *ctx, struct gui_context_panel *panel)
{
    if (!ctx->stack_begin) {
        ctx->stack_begin = panel;
        ctx->stack_end = panel;
        return;
    }
    ctx->stack_end->next = panel;
    panel->prev = ctx->stack_end;
    panel->next = NULL;
    ctx->stack_end = panel;
}

static void
gui_stack_remove(struct gui_context *ctx, struct gui_context_panel *panel)
{
    if (panel->prev)
        panel->prev->next = panel->next;
    if (panel->next)
        panel->next->prev = panel->prev;
    if (ctx->stack_begin == panel)
        ctx->stack_begin = panel->next;
    if (ctx->stack_end == panel)
        ctx->stack_end = panel->prev;
    panel->next = NULL;
    panel->prev = NULL;
}

struct gui_panel*
gui_panel_new(struct gui_context *ctx,
    gui_float x, gui_float y, gui_float w, gui_float h,
    const struct gui_config *config, const struct gui_font *font)
{
    struct gui_context_panel *cpanel;
    assert(ctx);
    assert(config);
    assert(font);
    if (!ctx || !config || !font)
        return NULL;

    cpanel = gui_alloc_panel(ctx);
    if (!cpanel) return NULL;
    cpanel->x = x, cpanel->y = y;
    cpanel->w = w, cpanel->h = h;
    cpanel->next = NULL; cpanel->prev = NULL;
    gui_panel_init(&cpanel->panel, config, font);
    gui_stack_push(ctx, cpanel);
    ctx->panel_size++;
    return &cpanel->panel;
}

void
gui_panel_del(struct gui_context *ctx, struct gui_panel *panel)
{
    struct gui_context_panel *cpanel;
    assert(ctx);
    assert(panel);
    if (!ctx || !panel) return;
    cpanel = (struct gui_context_panel*)panel;
    gui_stack_remove(ctx, cpanel);
    gui_free_panel(ctx, cpanel);
    ctx->panel_size--;
}

void
gui_begin(struct gui_context *ctx, gui_float w, gui_float h)
{
    struct gui_context_panel *iter;
    assert(ctx);
    if (!ctx) return;
    ctx->width = w;
    ctx->height = h;
}

gui_bool
gui_begin_panel(struct gui_context *ctx, struct gui_panel *panel,
    const char *title, gui_flags flags)
{
    gui_bool active, inpanel;
    const struct gui_input *in;
    struct gui_context_panel *cpanel;
    struct gui_draw_buffer *out;
    struct gui_draw_buffer *global;

    assert(ctx);
    assert(panel);
    assert(title);

    if (!ctx || !panel || !title)
        return gui_false;
    if (panel->flags & GUI_PANEL_HIDDEN)
        return gui_false;

    in = ctx->input;
    cpanel = (struct gui_context_panel*)panel;
    inpanel = INBOX(in->mouse_prev.x,in->mouse_prev.y, cpanel->x, cpanel->y, cpanel->w, cpanel->h);
    if (in->mouse_down && in->mouse_clicked && inpanel && cpanel != ctx->active) {
        struct gui_context_panel *iter = cpanel->next;
        while (iter) {
            if (INBOX(in->mouse_prev.x, in->mouse_prev.y, iter->x, iter->y, iter->w, iter->h))
                break;
            iter = iter->next;
        }
        if (!iter) {
            gui_stack_remove(ctx, cpanel);
            gui_stack_push(ctx, cpanel);
            ctx->active = cpanel;
        }
    }

    if (!(flags & GUI_PANEL_TAB))
        flags |= GUI_PANEL_SCROLLBAR;

    if (ctx->active == cpanel && (flags & GUI_PANEL_MOVEABLE) && (flags & GUI_PANEL_HEADER)) {
        gui_bool incursor;
        const gui_float header_x = cpanel->x;
        const gui_float header_y = cpanel->y;
        const gui_float header_w = cpanel->w;
        const gui_float header_h = cpanel->panel.header_height;
        incursor = INBOX(in->mouse_prev.x,in->mouse_prev.y,header_x, header_y, header_w, header_h);
        if (in->mouse_down && incursor) {
            cpanel->x = CLAMP(0, cpanel->x + in->mouse_delta.x, ctx->width - cpanel->w);
            cpanel->y = CLAMP(0, cpanel->y + in->mouse_delta.y, ctx->height - cpanel->h);
        }
    }

    if (ctx->active == cpanel && (flags & GUI_PANEL_SCALEABLE) && (flags & GUI_PANEL_SCROLLBAR)) {
        const struct gui_config *config = cpanel->panel.config;
        gui_bool incursor;
        const gui_float scaler_x = cpanel->x;
        const gui_float scaler_y = (cpanel->y + cpanel->h) - config->scaler_size.y;
        const gui_float scaler_w = config->scaler_size.x;
        const gui_float scaler_h = config->scaler_size.y;
        incursor = INBOX(in->mouse_prev.x,in->mouse_prev.y,scaler_x, scaler_y, scaler_w, scaler_h);
        if (in->mouse_down && incursor) {
            cpanel->x = CLAMP(0, cpanel->x + in->mouse_delta.x, ctx->width - cpanel->w);
            cpanel->w = CLAMP(0, cpanel->w - in->mouse_delta.x, ctx->width - cpanel->x);
            cpanel->h = CLAMP(0, cpanel->h + in->mouse_delta.y, ctx->height - cpanel->y);
        }
    }


    global = &ctx->global_buffer;
    out = &ctx->current_buffer;
    out->vertex_size = 0;
    out->vertex_needed = 0;
    out->command_size = 0;
    out->command_needed = 0;
    out->clip_size = 0;
    out->clip_needed = 0;
    out->vertexes = &global->vertexes[global->vertex_size];
    out->vertex_capacity = global->vertex_capacity - global->vertex_size;
    out->commands = &global->commands[global->command_size];
    out->command_capacity = global->command_capacity - global->command_size;
    out->clips = global->clips;
    out->clip_capacity = global->clip_capacity;
    in = (ctx->active == cpanel) ? ctx->input : NULL;
    return gui_panel_begin(panel, out, in, title, cpanel->x, cpanel->y,
                            cpanel->w, cpanel->h, flags);
}

void
gui_end_panel(struct gui_context *ctx, struct gui_panel *panel,
    struct gui_memory_status *status)
{
    struct gui_context_panel *cpanel;
    struct gui_draw_buffer *global;
    assert(ctx);
    assert(panel);
    if (!ctx || !panel) return;

    cpanel = (struct gui_context_panel*)panel;
    gui_panel_end(panel);
    global = &ctx->global_buffer;
    global->vertex_size += ctx->current_buffer.vertex_size;
    global->command_size += ctx->current_buffer.command_size;
    gui_output_end(&ctx->current_buffer, &cpanel->list, status);
}

void
gui_end(struct gui_context *ctx, struct gui_output *output,
    struct gui_memory_status *status)
{
    assert(ctx);
    if (!ctx) return;
    if (output) {
        gui_size n = 0;
        struct gui_context_panel *iter = ctx->stack_begin;
        while (iter) {
            if (!(iter->panel.flags & GUI_PANEL_HIDDEN))
                ctx->output_list[n++] = &iter->list;
            iter = iter->next;
        }
        output->list_size = n;
        output->list = ctx->output_list;
    }
    gui_output_end(&ctx->global_buffer, NULL, status);
}

