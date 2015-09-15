/*
    Copyright (c) 2015
    vurtun <polygone@gmx.net>
    MIT licence
*/
#include "gui.h"

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#endif
#ifndef CLAMP
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#endif

#define GUI_PI 3.141592654f
#define GUI_PI2 (GUI_PI * 2.0f)
#define GUI_4_DIV_PI (1.27323954f)
#define GUI_4_DIV_PI_SQRT (0.405284735f)

#define GUI_UTF_INVALID 0xFFFD
#define GUI_MAX_NUMBER_BUFFER 64

#define GUI_UNUSED(x) ((void)(x))
#define GUI_LERP(a, b, t) ((a) + ((b) - (a)) * (t))
#define GUI_SATURATE(x) (MAX(0, MIN(1.0f, x)))
#define GUI_LEN(a) (sizeof(a)/sizeof(a)[0])
#define GUI_ABS(a) (((a) < 0) ? -(a) : (a))
#define GUI_BETWEEN(x, a, b) ((a) <= (x) && (x) <= (b))
#define GUI_INBOX(px, py, x, y, w, h) (GUI_BETWEEN(px,x,x+w) && GUI_BETWEEN(py,y,y+h))
#define GUI_INTERSECT(x0, y0, w0, h0, x1, y1, w1, h1) \
    (!(((x1 > (x0 + w0)) || ((x1 + w1) < x0) || (y1 > (y0 + h0)) || (y1 + h1) < y0)))
#define GUI_CONTAINS(x, y, w, h, bx, by, bw, bh)\
    (GUI_INBOX(x,y, bx, by, bw, bh) && GUI_INBOX(x+w,y+h, bx, by, bw, bh))

#define gui_vec2_mov(to,from) (to).x = (from).x, (to).y = (from).y
#define gui_vec2_sub(a, b) gui_vec2((a).x - (b).x, (a).y - (b).y)
#define gui_vec2_add(a, b) gui_vec2((a).x + (b).x, (a).y + (b).y)
#define gui_vec2_len_sqr(a) ((a).x*(a).x+(a).y*(a).y)
#define gui_vec2_inv_len(a) gui_inv_sqrt(gui_vec2_len_sqr(a))
#define gui_vec2_muls(a, t) gui_vec2((a).x * (t), (a).y * (t))
#define gui_vec2_lerp(a, b, t) gui_vec2(GUI_LERP((a).x, (b).x, t), GUI_LERP((a).y, (b).y, t))

#define GUI_ALIGN_PTR(x, mask) (void*)((gui_ptr)((gui_byte*)(x) + (mask-1)) & ~(mask-1))
#define GUI_ALIGN_PTR_BACK(x, mask)(void*)((gui_ptr)((gui_byte*)(x)) & ~(mask-1));
#define GUI_ALIGN(x, mask) ((x) + (mask-1)) & ~(mask-1)
#define GUI_OFFSETOF(st, m) ((gui_size)(&((st *)0)->m))
#define GUI_CONTAINER_OF_CONST(ptr, type, member)\
    ((const type*)((const void*)((const gui_byte*)ptr - GUI_OFFSETOF(type, member))))
#define GUI_CONTAINER_OF(ptr, type, member)\
    ((type*)((void*)((gui_byte*)ptr - GUI_OFFSETOF(type, member))))

#ifdef __cplusplus
template<typename T> struct gui_alignof;
template<typename T, int size_diff> struct gui_helper{enum {value = size_diff};};
template<typename T> struct gui_helper<T,0>{enum {value = gui_alignof<T>::value};};
template<typename T> struct gui_alignof{struct Big {T x; char c;}; enum {
    diff = sizeof(Big) - sizeof(T), value = gui_helper<Big, diff>::value};};
#define GUI_ALIGNOF(t) (gui_alignof<t>::value);
#else
#define GUI_ALIGNOF(t) ((char*)(&((struct {char c; t _h;}*)0)->_h) - (char*)0)
#endif

enum gui_tree_node_symbol {GUI_TREE_NODE_BULLET, GUI_TREE_NODE_TRIANGLE};
static const struct gui_rect gui_null_rect = {-8192.0f, -8192.0f, 16384, 16384};
static const gui_byte gui_utfbyte[GUI_UTF_SIZE+1] = {0x80, 0, 0xC0, 0xE0, 0xF0};
static const gui_byte gui_utfmask[GUI_UTF_SIZE+1] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
static const long gui_utfmin[GUI_UTF_SIZE+1] = {0, 0, 0x80, 0x800, 0x100000};
static const long gui_utfmax[GUI_UTF_SIZE+1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};
/*
 * ==============================================================
 *
 *                          Utility
 *
 * ===============================================================
 */
static gui_uint
gui_round_up_pow2(gui_uint v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

static gui_float
gui_inv_sqrt(gui_float number)
{
    gui_float x2;
    const gui_float threehalfs = 1.5f;
    union {gui_uint i; gui_float f;} conv;
    conv.f = number;
    x2 = number * 0.5f;
    conv.i = 0x5f375A84 - (conv.i >> 1);
    conv.f = conv.f * (threehalfs - (x2 * conv.f * conv.f));
    return conv.f;
}

struct gui_rect
gui_get_null_rect(void)
{
    return gui_null_rect;
}

struct gui_rect
gui_rect(gui_float x, gui_float y, gui_float w, gui_float h)
{
    struct gui_rect r;
    r.x = x, r.y = y;
    r.w = w, r.h = h;
    return r;
}

static struct gui_rect
gui_shrink_rect(struct gui_rect r, gui_float amount)
{
    struct gui_rect res;
    r.w = MAX(r.w, 2 * amount);
    r.h = MAX(r.h, 2 * amount);
    res.x = r.x + amount;
    res.y = r.y + amount;
    res.w = r.w - 2 * amount;
    res.h = r.h - 2 * amount;
    return res;
}

static struct gui_rect
gui_pad_rect(struct gui_rect r, struct gui_vec2 pad)
{
    r.w = MAX(r.w, 2 * pad.x);
    r.h = MAX(r.h, 2 * pad.y);
    r.x += pad.x; r.y += pad.y;
    r.w -= 2 * pad.x;
    r.h -= 2 * pad.y;
    return r;
}

struct gui_vec2
gui_vec2(gui_float x, gui_float y)
{
    struct gui_vec2 ret;
    ret.x = x; ret.y = y;
    return ret;
}

static void*
gui_memcopy(void *dst, const void *src, gui_size size)
{
    gui_size i = 0;
    char *d = (char*)dst;
    const char *s = (const char*)src;
    for (i = 0; i < size; ++i)
        d[i] = s[i];
    return dst;
}

struct gui_color
gui_rgba(gui_byte r, gui_byte g, gui_byte b, gui_byte a)
{
    struct gui_color ret;
    ret.r = r; ret.g = g;
    ret.b = b; ret.a = a;
    return ret;
}

struct gui_color
gui_rgb(gui_byte r, gui_byte g, gui_byte b)
{
    struct gui_color ret;
    ret.r = r; ret.g = g;
    ret.b = b; ret.a = 255;
    return ret;
}

gui_handle
gui_handle_ptr(void *ptr)
{
    gui_handle handle;
    handle.ptr = ptr;
    return handle;
}

gui_handle
gui_handle_id(gui_int id)
{
    gui_handle handle;
    handle.id = id;
    return handle;
}

struct gui_image
gui_subimage_ptr(void *ptr, gui_ushort w, gui_ushort h, struct gui_rect r)
{
    struct gui_image s;
    s.handle.ptr = ptr;
    s.w = w; s.h = h;
    s.region[0] = (gui_ushort)r.x;
    s.region[1] = (gui_ushort)r.y;
    s.region[2] = (gui_ushort)r.w;
    s.region[3] = (gui_ushort)r.h;
    return s;
}

struct gui_image
gui_subimage_id(gui_int id, gui_ushort w, gui_ushort h, struct gui_rect r)
{
    struct gui_image s;
    s.handle.id = id;
    s.w = w; s.h = h;
    s.region[0] = (gui_ushort)r.x;
    s.region[1] = (gui_ushort)r.y;
    s.region[2] = (gui_ushort)r.w;
    s.region[3] = (gui_ushort)r.h;
    return s;
}

struct gui_image
gui_image_ptr(void *ptr)
{
    struct gui_image s;
    GUI_ASSERT(ptr);
    s.handle.ptr = ptr;
    s.w = 0; s.h = 0;
    s.region[0] = 0;
    s.region[1] = 0;
    s.region[2] = 0;
    s.region[3] = 0;
    return s;
}

struct gui_image
gui_image_id(gui_int id)
{
    struct gui_image s;
    s.handle.id = id;
    s.w = 0; s.h = 0;
    s.region[0] = 0;
    s.region[1] = 0;
    s.region[2] = 0;
    s.region[3] = 0;
    return s;
}

gui_bool
gui_image_is_subimage(const struct gui_image* img)
{
    GUI_ASSERT(img);
    return (img->w == 0 && img->h == 0);
}

static void
gui_zero(void *dst, gui_size size)
{
    gui_size i;
    char *d = (char*)dst;
    GUI_ASSERT(dst);
    for (i = 0; i < size; ++i) d[i] = 0;
}

static gui_size
gui_strsiz(const char *str)
{
    gui_size siz = 0;
    GUI_ASSERT(str);
    while (str && *str++ != '\0') siz++;
    return siz;
}

static gui_int
gui_strtoi(gui_int *number, const char *buffer, gui_size len)
{
    gui_size i;
    GUI_ASSERT(number);
    GUI_ASSERT(buffer);
    if (!number || !buffer) return 0;
    *number = 0;
    for (i = 0; i < len; ++i)
        *number = *number * 10 + (buffer[i] - '0');
    return 1;
}

static gui_size
gui_itos(char *buffer, gui_int num)
{
    static const char digit[] = "0123456789";
    gui_int shifter;
    gui_size len = 0;
    char *p = buffer;

    GUI_ASSERT(buffer);
    if (!buffer)
        return 0;

    if (num < 0) {
        num = GUI_ABS(num);
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
gui_unify(struct gui_rect *clip, const struct gui_rect *a, gui_float x0, gui_float y0,
    gui_float x1, gui_float y1)
{
    GUI_ASSERT(a);
    GUI_ASSERT(clip);
    clip->x = MAX(a->x, x0) - 1;
    clip->y = MAX(a->y, y0) - 1;
    clip->w = MIN(a->x + a->w, x1) - clip->x+ 2;
    clip->h = MIN(a->y + a->h, y1) - clip->y + 2;
    clip->w = MAX(0, clip->w);
    clip->h = MAX(0, clip->h);
}

static void
gui_triangle_from_direction(struct gui_vec2 *result, struct gui_rect r,
    gui_float pad_x, gui_float pad_y, enum gui_heading direction)
{
    gui_float w_half, h_half;
    GUI_ASSERT(result);

    r.w = MAX(2 * pad_x, r.w);
    r.h = MAX(2 * pad_y, r.h);
    r.w = r.w - 2 * pad_x;
    r.h = r.h - 2 * pad_y;

    r.x = r.x + pad_x;
    r.y = r.y + pad_y;

    w_half = r.w / 2.0f;
    h_half = r.h / 2.0f;

    if (direction == GUI_UP) {
        result[0] = gui_vec2(r.x + w_half, r.y);
        result[1] = gui_vec2(r.x + r.w, r.y + r.h);
        result[2] = gui_vec2(r.x, r.y + r.h);
    } else if (direction == GUI_RIGHT) {
        result[0] = gui_vec2(r.x, r.y);
        result[1] = gui_vec2(r.x + r.w, r.y + h_half);
        result[2] = gui_vec2(r.x, r.y + r.h);
    } else if (direction == GUI_DOWN) {
        result[0] = gui_vec2(r.x, r.y);
        result[1] = gui_vec2(r.x + r.w, r.y);
        result[2] = gui_vec2(r.x + w_half, r.y + r.h);
    } else {
        result[0] = gui_vec2(r.x, r.y + h_half);
        result[1] = gui_vec2(r.x + r.w, r.y);
        result[2] = gui_vec2(r.x + r.w, r.y + r.h);
    }
}

/*
 * ==============================================================
 *
 *                          UTF-8
 *
 * ===============================================================
 */
static gui_size
gui_utf_validate(gui_long *u, gui_size i)
{
    GUI_ASSERT(u);
    if (!u) return 0;
    if (!GUI_BETWEEN(*u, gui_utfmin[i], gui_utfmax[i]) ||
         GUI_BETWEEN(*u, 0xD800, 0xDFFF))
            *u = GUI_UTF_INVALID;
    for (i = 1; *u > gui_utfmax[i]; ++i);
    return i;
}

static gui_long
gui_utf_decode_byte(gui_char c, gui_size *i)
{
    GUI_ASSERT(i);
    if (!i) return 0;
    for(*i = 0; *i < GUI_LEN(gui_utfmask); ++(*i)) {
        if ((c & gui_utfmask[*i]) == gui_utfbyte[*i])
            return c & ~gui_utfmask[*i];
    }
    return 0;
}

gui_size
gui_utf_decode(const gui_char *c, gui_long *u, gui_size clen)
{
    gui_size i, j, len, type=0;
    gui_long udecoded;

    GUI_ASSERT(c);
    GUI_ASSERT(u);

    if (!c || !u) return 0;
    if (!clen) return 0;
    *u = GUI_UTF_INVALID;

    udecoded = gui_utf_decode_byte(c[0], &len);
    if (!GUI_BETWEEN(len, 1, GUI_UTF_SIZE))
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
    return (gui_char)(gui_utfbyte[i] | (u & ~gui_utfmask[i]));
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

gui_size
gui_utf_len(const gui_char *str, gui_size len)
{
    const gui_char *text;
    gui_size text_len;
    gui_size glyph_len;
    gui_size src_len = 0;
    gui_long unicode;

    GUI_ASSERT(str);
    if (!str || !len) return 0;

    text = str;
    text_len = len;
    glyph_len = gui_utf_decode(text, &unicode, text_len);
    while (glyph_len && src_len < len) {
        src_len = src_len + glyph_len;
        glyph_len = gui_utf_decode(text + src_len, &unicode, text_len - src_len);
    }
    return src_len;
}

/*
 * ==============================================================
 *
 *                          Input
 *
 * ===============================================================
 */
void
gui_input_begin(struct gui_input *in)
{
    gui_size i;
    GUI_ASSERT(in);
    if (!in) return;

    for (i = 0; i < GUI_BUTTON_MAX; ++i)
        in->mouse.buttons[i].clicked = 0;
    in->keyboard.text_len = 0;
    in->mouse.scroll_delta = 0;
    gui_vec2_mov(in->mouse.prev, in->mouse.pos);
    for (i = 0; i < GUI_KEY_MAX; i++)
        in->keyboard.keys[i].clicked = 0;
}

void
gui_input_motion(struct gui_input *in, gui_int x, gui_int y)
{
    GUI_ASSERT(in);
    if (!in) return;
    in->mouse.pos.x = (gui_float)x;
    in->mouse.pos.y = (gui_float)y;
}

void
gui_input_key(struct gui_input *in, enum gui_keys key, gui_bool down)
{
    GUI_ASSERT(in);
    if (!in) return;
    if (in->keyboard.keys[key].down == down) return;
    in->keyboard.keys[key].down = down;
    in->keyboard.keys[key].clicked++;
}

void
gui_input_button(struct gui_input *in, enum gui_buttons id, gui_int x, gui_int y, gui_bool down)
{
    struct gui_mouse_button *btn;
    GUI_ASSERT(in);
    if (!in) return;
    if (in->mouse.buttons[id].down == down) return;
    btn = &in->mouse.buttons[id];
    btn->clicked_pos.x = (gui_float)x;
    btn->clicked_pos.y = (gui_float)y;
    btn->down = down;
    btn->clicked++;
}

void
gui_input_scroll(struct gui_input *in, gui_float y)
{
    GUI_ASSERT(in);
    if (!in) return;
    in->mouse.scroll_delta += y;
}

void
gui_input_glyph(struct gui_input *in, const gui_glyph glyph)
{
    gui_size len = 0;
    gui_long unicode;
    GUI_ASSERT(in);
    if (!in) return;

    len = gui_utf_decode(glyph, &unicode, GUI_UTF_SIZE);
    if (len && ((in->keyboard.text_len + len) < GUI_INPUT_MAX)) {
        gui_utf_encode(unicode, &in->keyboard.text[in->keyboard.text_len],
            GUI_INPUT_MAX - in->keyboard.text_len);
        in->keyboard.text_len += len;
    }
}

void
gui_input_char(struct gui_input *in, char c)
{
    gui_glyph glyph;
    GUI_ASSERT(in);
    glyph[0] = c;
    gui_input_glyph(in, glyph);
}

void
gui_input_end(struct gui_input *in)
{
    GUI_ASSERT(in);
    if (!in) return;
    in->mouse.delta = gui_vec2_sub(in->mouse.pos, in->mouse.prev);
}

gui_bool
gui_input_has_mouse_click_in_rect(const struct gui_input *i, enum gui_buttons id,
    struct gui_rect b)
{
    const struct gui_mouse_button *btn;
    if (!i) return gui_false;
    btn = &i->mouse.buttons[id];
    if (!GUI_INBOX(btn->clicked_pos.x,btn->clicked_pos.y,b.x,b.y,b.w,b.h))
        return gui_false;
    return gui_true;
}

gui_bool
gui_input_has_mouse_click_down_in_rect(const struct gui_input *i, enum gui_buttons id,
    struct gui_rect b, gui_bool down)
{
    const struct gui_mouse_button *btn;
    if (!i) return gui_false;
    btn = &i->mouse.buttons[id];
    return gui_input_has_mouse_click_in_rect(i, id, b) && (btn->down == down);
}

gui_bool
gui_input_is_mouse_click_in_rect(const struct gui_input *i, enum gui_buttons id,
    struct gui_rect b)
{
    const struct gui_mouse_button *btn;
    if (!i) return gui_false;
    btn = &i->mouse.buttons[id];
    return (gui_input_has_mouse_click_down_in_rect(i, id, b, gui_true) &&
            btn->clicked) ? gui_true : gui_false;
}

gui_bool
gui_input_is_mouse_hovering_rect(const struct gui_input *i, struct gui_rect rect)
{
    if (!i) return gui_false;
    return GUI_INBOX(i->mouse.pos.x, i->mouse.pos.y, rect.x, rect.y, rect.w, rect.h);
}

gui_bool
gui_input_is_mouse_prev_hovering_rect(const struct gui_input *i, struct gui_rect rect)
{
    if (!i) return gui_false;
    return GUI_INBOX(i->mouse.prev.x, i->mouse.prev.y, rect.x, rect.y, rect.w, rect.h);
}

gui_bool
gui_input_mouse_clicked(const struct gui_input *i, enum gui_buttons id, struct gui_rect rect)
{
    if (!i) return gui_false;
    if (!gui_input_is_mouse_hovering_rect(i, rect)) return gui_false;
    return gui_input_is_mouse_click_in_rect(i, id, rect);
}

gui_bool
gui_input_is_mouse_down(const struct gui_input *i, enum gui_buttons id)
{
    if (!i) return gui_false;
    return i->mouse.buttons[id].down;
}

gui_bool
gui_input_is_mouse_pressed(const struct gui_input *i, enum gui_buttons id)
{
    const struct gui_mouse_button *b;
    if (!i) return gui_false;
    b = &i->mouse.buttons[id];
    if (b->down && b->clicked)
        return gui_true;
    return gui_false;
}

gui_bool
gui_input_is_mouse_released(const struct gui_input *i, enum gui_buttons id)
{
    if (!i) return gui_false;
    return (!i->mouse.buttons[id].down && i->mouse.buttons[id].clicked) ? gui_true : gui_false;
}

gui_bool
gui_input_is_key_pressed(const struct gui_input *i, enum gui_keys key)
{
    const struct gui_key *k;
    if (!i) return gui_false;
    k = &i->keyboard.keys[key];
    if (k->down && k->clicked)
        return gui_true;
    return gui_false;
}

gui_bool
gui_input_is_key_released(const struct gui_input *i, enum gui_keys key)
{
    const struct gui_key *k;
    if (!i) return gui_false;
    k = &i->keyboard.keys[key];
    if (!k->down && k->clicked)
        return gui_true;
    return gui_false;
}

gui_bool
gui_input_is_key_down(const struct gui_input *i, enum gui_keys key)
{
    const struct gui_key *k;
    if (!i) return gui_false;
    k = &i->keyboard.keys[key];
    if (k->down) return gui_true;
    return gui_false;
}

/*
 * ==============================================================
 *
 *                          Buffer
 *
 * ===============================================================
 */
void
gui_buffer_init(struct gui_buffer *b, const struct gui_allocator *a,
    gui_size initial_size, gui_float grow_factor)
{
    GUI_ASSERT(b);
    GUI_ASSERT(a);
    GUI_ASSERT(initial_size);
    if (!b || !a || !initial_size) return;

    gui_zero(b, sizeof(*b));
    b->type = GUI_BUFFER_DYNAMIC;
    b->memory.ptr = a->alloc(a->userdata, initial_size);
    b->memory.size = initial_size;
    b->grow_factor = grow_factor;
    b->pool = *a;
}

void
gui_buffer_init_fixed(struct gui_buffer *b, void *m, gui_size size)
{
    GUI_ASSERT(b);
    GUI_ASSERT(m);
    GUI_ASSERT(size);
    if (!b || !m || !size) return;

    gui_zero(b, sizeof(*b));
    b->type = GUI_BUFFER_FIXED;
    b->memory.ptr = m;
    b->memory.size = size;
    b->size = size;
}

static void*
gui_buffer_align(void *unaligned, gui_size align, gui_size *alignment,
    enum gui_buffer_allocation_type type)
{
    void *memory = 0;
    if (type == GUI_BUFFER_FRONT) {
        if (align) {
            memory = GUI_ALIGN_PTR(unaligned, align);
            *alignment = (gui_size)((gui_byte*)memory - (gui_byte*)unaligned);
        } else {
            memory = unaligned;
            *alignment = 0;
        }
    } else {
        if (align) {
            memory = GUI_ALIGN_PTR_BACK(unaligned, align);
            *alignment = (gui_size)((gui_byte*)unaligned - (gui_byte*)memory);
        } else {
            memory = unaligned;
            *alignment = 0;
        }
    }
    return memory;
}

static void*
gui_buffer_realloc(struct gui_buffer *b, gui_size capacity, gui_size *size)
{
    void *temp;
    gui_size buffer_size;

    GUI_ASSERT(b);
    GUI_ASSERT(size);
    if (!b || !size || !b->pool.realloc)
        return 0;

    buffer_size = b->memory.size;
    temp = b->pool.realloc(b->pool.userdata, b->memory.ptr, capacity);
    GUI_ASSERT(temp);
    if (!temp) return 0;
    *size = capacity;

    if (b->size == buffer_size) {
        /* no back buffer so just set correct size */
        b->size = capacity;
        return temp;
    } else {
        /* copy back buffer to the end of the new buffer */
        void *dst, *src;
        gui_size back_size;
        back_size = buffer_size - b->size;
        dst = gui_ptr_add(void, temp, capacity - back_size);
        src = gui_ptr_add(void, temp, b->size);
        gui_memcopy(dst, src, back_size);
        b->size = capacity - back_size;
    }
    return temp;
}

void*
gui_buffer_alloc(struct gui_buffer *b, enum gui_buffer_allocation_type type,
    gui_size size, gui_size align)
{
    gui_bool full;
    gui_size cap;
    gui_size alignment;
    void *unaligned;
    void *memory;

    GUI_ASSERT(b);
    if (!b || !size) return 0;
    b->needed += size;

    /* calculate total size with needed alignment + size */
    if (type == GUI_BUFFER_FRONT)
        unaligned = gui_ptr_add(void, b->memory.ptr, b->allocated);
    else unaligned = gui_ptr_add(void, b->memory.ptr, b->size - size);
    memory = gui_buffer_align(unaligned, align, &alignment, type);

    /* check if buffer has enough memory*/
    if (type == GUI_BUFFER_FRONT)
        full = ((b->allocated + size + alignment) > b->size);
    else full = ((b->size - (size + alignment)) <= b->allocated);

    if (full) {
        /* buffer is full so allocate bigger buffer if dynamic */
        if (b->type != GUI_BUFFER_DYNAMIC || !b->pool.realloc) return 0;
        cap = (gui_size)((gui_float)b->memory.size * b->grow_factor);
        b->memory.ptr = gui_buffer_realloc(b, cap, &b->memory.size);
        if (!b->memory.ptr) return 0;

        /* align newly allocated pointer to memory */
        if (type == GUI_BUFFER_FRONT)
            unaligned = gui_ptr_add(void, b->memory.ptr, b->allocated);
        else unaligned = gui_ptr_add(void, b->memory.ptr, b->size);
        memory = gui_buffer_align(unaligned, align, &alignment, type);
    }

    if (type == GUI_BUFFER_FRONT)
        b->allocated += size + alignment;
    else b->size -= (size + alignment);
    b->needed += alignment;
    b->calls++;
    return memory;
}

void
gui_buffer_mark(struct gui_buffer *buffer, enum gui_buffer_allocation_type type)
{
    GUI_ASSERT(buffer);
    if (!buffer) return;
    buffer->marker[type].active = gui_true;
    if (type == GUI_BUFFER_BACK)
        buffer->marker[type].offset = buffer->size;
    else buffer->marker[type].offset = buffer->allocated;
}

void
gui_buffer_reset(struct gui_buffer *buffer, enum gui_buffer_allocation_type type)
{
    GUI_ASSERT(buffer);
    if (!buffer) return;
    if (type == GUI_BUFFER_BACK) {
        /* reset back buffer either back to back marker or empty */
        if (buffer->marker[type].active)
            buffer->size = buffer->marker[type].offset;
        else buffer->size = buffer->memory.size;
    } else {
        /* reset front buffer either back to back marker or empty */
        if (buffer->marker[type].active)
            buffer->allocated = buffer->marker[type].offset;
        else buffer->allocated = 0;
    }
}

void
gui_buffer_clear(struct gui_buffer *b)
{
    GUI_ASSERT(b);
    if (!b) return;
    b->allocated = 0;
    b->size = b->memory.size;
    b->calls = 0;
    b->needed = 0;
}

void
gui_buffer_free(struct gui_buffer *b)
{
    GUI_ASSERT(b);
    if (!b || !b->memory.ptr) return;
    if (b->type == GUI_BUFFER_FIXED) return;
    if (!b->pool.free) return;
    GUI_ASSERT(b->pool.free);
    b->pool.free(b->pool.userdata, b->memory.ptr);
}

void
gui_buffer_info(struct gui_memory_status *s, struct gui_buffer *b)
{
    GUI_ASSERT(b);
    GUI_ASSERT(s);
    if (!s || !b) return;
    s->allocated = b->allocated;
    s->size =  b->memory.size;
    s->needed = b->needed;
    s->memory = b->memory.ptr;
    s->calls = b->calls;
}

void*
gui_buffer_memory(struct gui_buffer *buffer)
{
    GUI_ASSERT(buffer);
    if (!buffer) return 0;
    return buffer->memory.ptr;
}

gui_size
gui_buffer_total(struct gui_buffer *buffer)
{
    GUI_ASSERT(buffer);
    if (!buffer) return 0;
    return buffer->memory.size;
}
/*
 * ==============================================================
 *
 *                      Command buffer
 *
 * ===============================================================
 */
void
gui_command_buffer_init(struct gui_command_buffer *cmdbuf,
    struct gui_buffer *buffer, enum gui_command_clipping clip)
{
    GUI_ASSERT(cmdbuf);
    GUI_ASSERT(buffer);
    if (!cmdbuf || !buffer) return;
    cmdbuf->queue = 0;
    cmdbuf->base = buffer;
    cmdbuf->use_clipping = clip;
    cmdbuf->begin = buffer->allocated;
    cmdbuf->end = buffer->allocated;
    cmdbuf->last = buffer->allocated;
}

void
gui_command_buffer_reset(struct gui_command_buffer *buffer)
{
    GUI_ASSERT(buffer);
    if (!buffer) return;
    gui_zero(&buffer->stats, sizeof(buffer->stats));
    buffer->begin = 0;
    buffer->end = 0;
    buffer->last = 0;
}

void
gui_command_buffer_clear(struct gui_command_buffer *buffer)
{
    GUI_ASSERT(buffer);
    if (!buffer) return;
    gui_command_buffer_reset(buffer);
    gui_buffer_clear(buffer->base);
}

void*
gui_command_buffer_push(struct gui_command_buffer* b,
    enum gui_command_type t, gui_size size)
{
    static const gui_size align = GUI_ALIGNOF(struct gui_command);
    struct gui_command *cmd;
    gui_size alignment;
    void *unaligned;
    void *memory;

    GUI_ASSERT(b);
    GUI_ASSERT(b->base);
    if (!b) return 0;

    cmd = (struct gui_command*)gui_buffer_alloc(b->base, GUI_BUFFER_FRONT, size, align);
    if (!cmd) return 0;

    /* make sure the offset to the next command is aligned */
    b->last = (gui_size)((gui_byte*)cmd - (gui_byte*)b->base->memory.ptr);
    unaligned = (gui_byte*)cmd + size;
    memory = GUI_ALIGN_PTR(unaligned, align);
    alignment = (gui_size)((gui_byte*)memory - (gui_byte*)unaligned);

    cmd->type = t;
    cmd->next = b->base->allocated + alignment;
    b->end = cmd->next;
    return cmd;
}

void
gui_command_buffer_push_scissor(struct gui_command_buffer *b, struct gui_rect r)
{
    struct gui_command_scissor *cmd;
    GUI_ASSERT(b);
    if (!b) return;

    b->clip.x = r.x;
    b->clip.y = r.y;
    b->clip.w = r.w;
    b->clip.h = r.h;
    cmd = (struct gui_command_scissor*)
        gui_command_buffer_push(b, GUI_COMMAND_SCISSOR, sizeof(*cmd));
    if (!cmd) return;
    cmd->x = (gui_short)r.x;
    cmd->y = (gui_short)r.y;
    cmd->w = (gui_ushort)MAX(0, r.w);
    cmd->h = (gui_ushort)MAX(0, r.h);
}

void
gui_command_buffer_push_line(struct gui_command_buffer *b, gui_float x0, gui_float y0,
    gui_float x1, gui_float y1, struct gui_color c)
{
    struct gui_command_line *cmd;
    GUI_ASSERT(b);
    if (!b) return;
    cmd = (struct gui_command_line*)
        gui_command_buffer_push(b, GUI_COMMAND_LINE, sizeof(*cmd));
    if (!cmd) return;
    cmd->begin.x = (gui_short)x0;
    cmd->begin.y = (gui_short)y0;
    cmd->end.x = (gui_short)x1;
    cmd->end.y = (gui_short)y1;
    cmd->color = c;
    b->stats.lines++;
}

void
gui_command_buffer_push_curve(struct gui_command_buffer *b, gui_float ax, gui_float ay,
    gui_float ctrl0x, gui_float ctrl0y, gui_float ctrl1x, gui_float ctrl1y,
    gui_float bx, gui_float by, struct gui_color col)
{
    struct gui_command_curve *cmd;
    GUI_ASSERT(b);
    if (!b) return;

    cmd = (struct gui_command_curve*)
        gui_command_buffer_push(b, GUI_COMMAND_CURVE, sizeof(*cmd));
    if (!cmd) return;
    cmd->begin.x = (gui_short)ax;
    cmd->begin.y = (gui_short)ay;
    cmd->ctrl[0].x = (gui_short)ctrl0x;
    cmd->ctrl[0].y = (gui_short)ctrl0y;
    cmd->ctrl[1].x = (gui_short)ctrl1x;
    cmd->ctrl[1].y = (gui_short)ctrl1y;
    cmd->end.x = (gui_short)bx;
    cmd->end.y = (gui_short)by;
    cmd->color = col;
    b->stats.lines++;
}

void
gui_command_buffer_push_rect(struct gui_command_buffer *b, struct gui_rect rect,
    gui_float rounding, struct gui_color c)
{
    struct gui_command_rect *cmd;
    GUI_ASSERT(b);
    if (!b) return;
    if (b->use_clipping) {
        const struct gui_rect *clip = &b->clip;
        if (!GUI_INTERSECT(rect.x, rect.y, rect.w, rect.h,
            clip->x, clip->y, clip->w, clip->h)) return;
    }

    cmd = (struct gui_command_rect*)
        gui_command_buffer_push(b, GUI_COMMAND_RECT, sizeof(*cmd));
    if (!cmd) return;
    cmd->rounding = (gui_uint)rounding;
    cmd->x = (gui_short)rect.x;
    cmd->y = (gui_short)rect.y;
    cmd->w = (gui_ushort)MAX(0, rect.w);
    cmd->h = (gui_ushort)MAX(0, rect.h);
    cmd->color = c;
    b->stats.rectangles++;
}

void
gui_command_buffer_push_circle(struct gui_command_buffer *b, struct gui_rect r,
    struct gui_color c)
{
    struct gui_command_circle *cmd;
    GUI_ASSERT(b);
    if (!b) return;
    if (b->use_clipping) {
        const struct gui_rect *clip = &b->clip;
        if (!GUI_INTERSECT(r.x, r.y, r.w, r.h, clip->x, clip->y, clip->w, clip->h))
            return;
    }

    cmd = (struct gui_command_circle*)
        gui_command_buffer_push(b, GUI_COMMAND_CIRCLE, sizeof(*cmd));
    if (!cmd) return;
    cmd->x = (gui_short)r.x;
    cmd->y = (gui_short)r.y;
    cmd->w = (gui_ushort)MAX(r.w, 0);
    cmd->h = (gui_ushort)MAX(r.h, 0);
    cmd->color = c;
    b->stats.circles++;
}

void
gui_command_buffer_push_triangle(struct gui_command_buffer *b,gui_float x0,gui_float y0,
    gui_float x1, gui_float y1, gui_float x2, gui_float y2, struct gui_color c)
{
    struct gui_command_triangle *cmd;
    GUI_ASSERT(b);
    if (!b) return;
    if (b->use_clipping) {
        const struct gui_rect *clip = &b->clip;
        if (!GUI_INBOX(x0, y0, clip->x, clip->y, clip->w, clip->h) ||
            !GUI_INBOX(x1, y1, clip->x, clip->y, clip->w, clip->h) ||
            !GUI_INBOX(x2, y2, clip->x, clip->y, clip->w, clip->h))
            return;
    }

    cmd = (struct gui_command_triangle*)
        gui_command_buffer_push(b, GUI_COMMAND_TRIANGLE, sizeof(*cmd));
    if (!cmd) return;
    cmd->a.x = (gui_short)x0;
    cmd->a.y = (gui_short)y0;
    cmd->b.x = (gui_short)x1;
    cmd->b.y = (gui_short)y1;
    cmd->c.x = (gui_short)x2;
    cmd->c.y = (gui_short)y2;
    cmd->color = c;
    b->stats.triangles++;
}

void
gui_command_buffer_push_image(struct gui_command_buffer *b, struct gui_rect r,
    struct gui_image *img)
{
    struct gui_command_image *cmd;
    GUI_ASSERT(b);
    if (!b) return;
    if (b->use_clipping) {
        const struct gui_rect *c = &b->clip;
        if (!GUI_INTERSECT(r.x, r.y, r.w, r.h, c->x, c->y, c->w, c->h))
            return;
    }

    cmd = (struct gui_command_image*)
        gui_command_buffer_push(b, GUI_COMMAND_IMAGE, sizeof(*cmd));
    if (!cmd) return;
    cmd->x = (gui_short)r.x;
    cmd->y = (gui_short)r.y;
    cmd->w = (gui_ushort)MAX(0, r.w);
    cmd->h = (gui_ushort)MAX(0, r.h);
    cmd->img = *img;
    b->stats.images++;
}

void
gui_command_buffer_push_text(struct gui_command_buffer *b, struct gui_rect r,
    const gui_char *string, gui_size length, const struct gui_user_font *font,
    struct gui_color bg, struct gui_color fg)
{
    struct gui_command_text *cmd;
    GUI_ASSERT(b);
    GUI_ASSERT(font);
    if (!b || !string || !length) return;
    if (b->use_clipping) {
        const struct gui_rect *c = &b->clip;
        if (!GUI_INTERSECT(r.x, r.y, r.w, r.h, c->x, c->y, c->w, c->h))
            return;
    }

    cmd = (struct gui_command_text*)
        gui_command_buffer_push(b, GUI_COMMAND_TEXT, sizeof(*cmd) + length + 1);
    if (!cmd) return;
    cmd->x = (gui_short)r.x;
    cmd->y = (gui_short)r.y;
    cmd->w = (gui_ushort)r.w;
    cmd->h = (gui_ushort)r.h;
    cmd->background = bg;
    cmd->foreground = fg;
    cmd->font = font;
    cmd->length = length;
    gui_memcopy(cmd->string, string, length);
    cmd->string[length] = '\0';
    b->stats.text++;
    b->stats.glyphes += (gui_uint)length;
}

const struct gui_command*
gui_command_buffer_begin(struct gui_command_buffer *buffer)
{
    gui_byte *memory;
    GUI_ASSERT(buffer);
    if (!buffer || !buffer->base) return 0;
    if (buffer->last == buffer->begin) return 0;
    memory = (gui_byte*)buffer->base->memory.ptr;
    return gui_ptr_add_const(struct gui_command, memory, buffer->begin);
}

const struct gui_command*
gui_command_buffer_next(struct gui_command_buffer *buffer,
    struct gui_command *cmd)
{
    gui_byte *memory;
    GUI_ASSERT(buffer);
    if (!buffer) return 0;
    if (!cmd) return 0;
    if (cmd->next > buffer->last) return 0;
    if (cmd->next > buffer->base->allocated) return 0;
    memory = (gui_byte*)buffer->base->memory.ptr;
    return gui_ptr_add_const(struct gui_command, memory, cmd->next);
}
/*
 * ==============================================================
 *
 *                      Command Queue
 *
 * ===============================================================
 */
void
gui_command_queue_init(struct gui_command_queue *queue,
    const struct gui_allocator *alloc, gui_size initial_size, gui_float grow_factor)
{
    GUI_ASSERT(alloc);
    GUI_ASSERT(queue);
    if (!alloc || !queue) return;
    gui_zero(queue, sizeof(*queue));
    gui_buffer_init(&queue->buffer, alloc, initial_size, grow_factor);
}

void
gui_command_queue_init_fixed(struct gui_command_queue *queue,
    void *memory, gui_size size)
{
    GUI_ASSERT(size);
    GUI_ASSERT(memory);
    GUI_ASSERT(queue);
    if (!memory || !size || !queue) return;
    gui_zero(queue, sizeof(*queue));
    gui_buffer_init_fixed(&queue->buffer, memory, size);
}

void
gui_command_queue_insert_back(struct gui_command_queue *queue,
    struct gui_command_buffer *buffer)
{
    struct gui_command_buffer_list *list;
    GUI_ASSERT(queue);
    GUI_ASSERT(buffer);
    if (!queue || !buffer) return;

    list = &queue->list;
    if (!list->begin) {
        buffer->next = 0;
        buffer->prev = 0;
        list->begin = buffer;
        list->end = buffer;
        list->count = 1;
        buffer->queue = queue;
        return;
    }

    list->end->next = buffer;
    buffer->prev = list->end;
    buffer->next = 0;
    list->end = buffer;
    list->count++;
    buffer->queue = queue;
}

void
gui_command_queue_insert_front(struct gui_command_queue *queue,
    struct gui_command_buffer *buffer)
{
    struct gui_command_buffer_list *list;
    GUI_ASSERT(queue);
    GUI_ASSERT(buffer);
    if (!queue || !buffer) return;

    list = &queue->list;
    if (!list->begin) {
        gui_command_queue_insert_back(queue, buffer);
        return;
    }

    list->begin->prev = buffer;
    buffer->next = list->begin;
    buffer->prev = 0;
    list->begin = buffer;
    list->count++;
    buffer->queue = queue;
}

void
gui_command_queue_remove(struct gui_command_queue *queue,
    struct gui_command_buffer *buffer)
{
    struct gui_command_buffer_list *list;
    GUI_ASSERT(queue);
    GUI_ASSERT(buffer);
    if (!queue || !buffer) return;

    list = &queue->list;
    if (buffer->prev)
        buffer->prev->next = buffer->next;
    if (buffer->next)
        buffer->next->prev = buffer->prev;
    if (list->begin == buffer)
        list->begin = buffer->next;
    if (list->end == buffer)
        list->end = buffer->prev;

    buffer->next = 0;
    buffer->prev = 0;
    list->count--;
}

gui_bool
gui_command_queue_start_child(struct gui_command_queue *queue,
    struct gui_command_buffer *buffer)
{
    static const gui_size buf_size = sizeof(struct gui_command_sub_buffer);
    static const gui_size buf_align = GUI_ALIGNOF(struct gui_command_sub_buffer);
    struct gui_command_sub_buffer_stack *stack;
    struct gui_command_sub_buffer *buf;
    struct gui_command_sub_buffer *end;
    gui_size offset;
    void *memory;

    GUI_ASSERT(queue);
    GUI_ASSERT(buffer);
    if (!queue || !buffer) return gui_false;

    /* allocate header space from the buffer */
    memory = queue->buffer.memory.ptr;
    buf = (struct gui_command_sub_buffer*)
        gui_buffer_alloc(&queue->buffer, GUI_BUFFER_BACK, buf_size, buf_align);
    if (!buf) return gui_false;
    offset = (gui_size)(((gui_byte*)memory + queue->buffer.memory.size) - (gui_byte*)buf);

    /* setup sub buffer */
    buf->begin = buffer->end;
    buf->end = buffer->end;
    buf->parent_last = buffer->last;
    buf->last = buf->begin;

    /* add first sub-buffer into the stack */
    stack = &queue->stack;
    if (!stack->begin) {
        buf->next = 0;
        stack->begin = offset;
        stack->end = offset;
        stack->count = 1;
        return gui_true;
    }

    /* add sub-buffer at the end of the stack */
    end = gui_ptr_add(struct gui_command_sub_buffer, memory, stack->end);
    end->next = offset;
    buf->next = offset;
    stack->count++;
    return gui_true;
}

void
gui_command_queue_finish_child(struct gui_command_queue *queue,
    struct gui_command_buffer *buffer)
{
    struct gui_command_sub_buffer *buf;
    struct gui_command_sub_buffer_stack *stack;
    gui_size offset;

    GUI_ASSERT(queue);
    GUI_ASSERT(buffer);
    if (!queue || !buffer) return;
    if (!queue->stack.count) return;

    stack = &queue->stack;
    offset = queue->buffer.memory.size - stack->end;
    buf = gui_ptr_add(struct gui_command_sub_buffer, queue->buffer.memory.ptr, offset);
    buf->last = buffer->last;
    buf->end = buffer->end;
}

void
gui_command_queue_start(struct gui_command_queue *queue,
    struct gui_command_buffer *buffer)
{
    GUI_ASSERT(queue);
    GUI_ASSERT(buffer);
    if (!queue || !buffer) return;
    gui_zero(&buffer->stats, sizeof(buffer->stats));
    buffer->begin = queue->buffer.allocated;
    buffer->end = buffer->begin;
    buffer->last = buffer->begin;
    buffer->clip = gui_null_rect;
}

void
gui_command_queue_finish(struct gui_command_queue *queue,
    struct gui_command_buffer *buffer)
{
    void *memory;
    gui_size size;
    gui_size i = 0;
    struct gui_command_sub_buffer *iter;
    struct gui_command_sub_buffer_stack *stack;

    GUI_ASSERT(queue);
    GUI_ASSERT(buffer);

    if (!queue || !buffer) return;
    stack = &queue->stack;
    buffer->end = queue->buffer.allocated;
    if (!stack->count) return;

    memory = queue->buffer.memory.ptr;
    size = queue->buffer.memory.size;
    iter = gui_ptr_add(struct gui_command_sub_buffer, memory, (size - stack->begin));

    /* fix buffer command list for subbuffers  */
    for (i = 0; i < stack->count; ++i) {
        struct gui_command *parent_last, *sublast, *last;
        parent_last = gui_ptr_add(struct gui_command, memory, iter->parent_last);
        sublast = gui_ptr_add(struct gui_command, memory, iter->last);
        last = gui_ptr_add(struct gui_command, memory, buffer->last);

        /* redirect the subbuffer to the end of the current command buffer */
        parent_last->next = iter->end;
        if (i == (stack->count - 1))
            sublast->next = last->next;
        last->next = iter->begin;
        buffer->last = iter->last;
        buffer->end = iter->end;
        iter = gui_ptr_add(struct gui_command_sub_buffer, memory, size - iter->next);
    }
    queue->stack.count = 0;
    gui_buffer_reset(&queue->buffer, GUI_BUFFER_BACK);
}

void
gui_command_queue_free(struct gui_command_queue *queue)
{
    GUI_ASSERT(queue);
    if (!queue) return;
    gui_buffer_clear(&queue->buffer);
}

void
gui_command_queue_clear(struct gui_command_queue *queue)
{
    GUI_ASSERT(queue);
    if (!queue) return;
    gui_buffer_clear(&queue->buffer);
    queue->build = gui_false;
    gui_zero(&queue->stack, sizeof(queue->stack));
}

void
gui_command_queue_build(struct gui_command_queue *queue)
{
    struct gui_command_buffer *iter;
    struct gui_command_buffer *next;
    struct gui_command *cmd;
    gui_byte *buffer;

    iter = queue->list.begin;
    buffer = (gui_byte*)queue->buffer.memory.ptr;
    while (iter != 0) {
        next = iter->next;
        if (iter->last != iter->begin) {
            cmd = gui_ptr_add(struct gui_command, buffer, iter->last);
            while (next && next->last == next->begin)
                next = next->next; /* skip empty command buffers */
            if (next) {
                cmd->next = next->begin;
            } else cmd->next = queue->buffer.allocated;
        }
        iter = next;
    }
    queue->build = gui_true;
}

const struct gui_command*
gui_command_queue_begin(struct gui_command_queue *queue)
{
    struct gui_command_buffer *iter;
    gui_byte *buffer;
    GUI_ASSERT(queue);
    if (!queue) return 0;
    if (!queue->list.count) return 0;

    /* build one command list out of all command buffers */
    buffer = (gui_byte*)queue->buffer.memory.ptr;
    if (!queue->build)
        gui_command_queue_build(queue);

    iter = queue->list.begin;
    while (iter && iter->begin == iter->end)
        iter = iter->next;
    if (!iter) return 0;
    return gui_ptr_add_const(struct gui_command, buffer, iter->begin);
}

const struct gui_command*
gui_command_queue_next(struct gui_command_queue *queue, const struct gui_command *cmd)
{
    gui_byte *buffer;
    const struct gui_command *next;
    GUI_ASSERT(queue);
    if (!queue || !cmd || !queue->list.count) return 0;
    if (cmd->next >= queue->buffer.allocated)
        return 0;
    buffer = (gui_byte*)queue->buffer.memory.ptr;
    next = gui_ptr_add_const(struct gui_command, buffer, cmd->next);
    return next;
}
/* ==============================================================
 *
 *                      Draw List
 *
 * ===============================================================*/
#if GUI_COMPILE_WITH_VERTEX_BUFFER
void
gui_draw_list_init(struct gui_draw_list *list, struct gui_buffer *cmds,
    struct gui_buffer *vertexes, struct gui_buffer *elements,
    gui_sin_f sine, gui_cos_f cosine, struct gui_draw_null_texture null,
    enum gui_anti_aliasing AA)
{
    gui_zero(list, sizeof(*list));
    list->sin = sine;
    list->cos = cosine;
    list->null = null;
    list->clip_rect = gui_null_rect;
    list->vertexes = vertexes;
    list->elements = elements;
    list->buffer = cmds;
    list->AA = AA;
}

void
gui_draw_list_load(struct gui_draw_list *list, struct gui_command_queue *queue,
    gui_float line_thickness, gui_uint curve_segments)
{
    const struct gui_command *cmd;
    GUI_ASSERT(list);
    GUI_ASSERT(list->vertexes);
    GUI_ASSERT(list->elements);
    GUI_ASSERT(queue);
    line_thickness = MAX(line_thickness, 1.0f);
    if (!list || !queue || !list->vertexes || !list->elements) return;

    gui_foreach_command(cmd, queue) {
        switch (cmd->type) {
        case GUI_COMMAND_NOP: break;
        case GUI_COMMAND_SCISSOR: {
            const struct gui_command_scissor *s = gui_command(scissor, cmd);
            gui_draw_list_add_clip(list, gui_rect(s->x, s->y, s->w, s->h));
        } break;
        case GUI_COMMAND_LINE: {
            const struct gui_command_line *l = gui_command(line, cmd);
            gui_draw_list_add_line(list, gui_vec2(l->begin.x, l->begin.y),
                gui_vec2(l->end.x, l->end.y), l->color, line_thickness);
        } break;
        case GUI_COMMAND_CURVE: {
            const struct gui_command_curve *q = gui_command(curve, cmd);
            gui_draw_list_add_curve(list, gui_vec2(q->begin.x, q->begin.y),
                gui_vec2(q->ctrl[0].x, q->ctrl[0].y), gui_vec2(q->ctrl[1].x, q->ctrl[1].y),
                gui_vec2(q->end.x, q->end.y), q->color, curve_segments, line_thickness);
        } break;
        case GUI_COMMAND_RECT: {
            const struct gui_command_rect *r = gui_command(rect, cmd);
            gui_draw_list_add_rect_filled(list, gui_rect(r->x, r->y, r->w, r->h),
                r->color, (gui_float)r->rounding);
        } break;
        case GUI_COMMAND_CIRCLE: {
            const struct gui_command_circle *c = gui_command(circle, cmd);
            gui_draw_list_add_circle_filled(list, gui_vec2((gui_float)c->x + (gui_float)c->w/2,
                (gui_float)c->y + c->h/2), c->w/2, c->color, curve_segments);
        } break;
        case GUI_COMMAND_TRIANGLE: {
            const struct gui_command_triangle *t = gui_command(triangle, cmd);
            gui_draw_list_add_triangle_filled(list, gui_vec2(t->a.x, t->a.y),
                gui_vec2(t->b.x, t->b.y), gui_vec2(t->c.x, t->c.y), t->color);
        } break;
        case GUI_COMMAND_TEXT: {
            const struct gui_command_text *t = gui_command(text, cmd);
            gui_draw_list_add_text(list, t->font, gui_rect(t->x, t->y, t->w, t->h),
                t->string, t->length, t->background, t->foreground);
        } break;
        case GUI_COMMAND_IMAGE: {
            const struct gui_command_image *i = gui_command(image, cmd);
            gui_draw_list_add_image(list, i->img, gui_rect(i->x, i->y, i->w, i->h),
                gui_rgb(255, 255, 255));
        } break;
        case GUI_COMMAND_MAX:
        default: break;
        }
    }
}

void
gui_draw_list_clear(struct gui_draw_list *list)
{
    GUI_ASSERT(list);
    if (!list) return;
    if (list->buffer)
        gui_buffer_clear(list->buffer);
    if (list->elements)
        gui_buffer_clear(list->vertexes);
    if (list->vertexes)
        gui_buffer_clear(list->elements);
    list->element_count = 0;
    list->vertex_count = 0;
    list->cmd_offset = 0;
    list->cmd_count = 0;
    list->path_count = 0;
    list->vertexes = 0;
    list->elements = 0;
    list->clip_rect = gui_null_rect;
}

gui_bool
gui_draw_list_is_empty(struct gui_draw_list *list)
{
    GUI_ASSERT(list);
    if (!list) return gui_true;
    return (list->cmd_count == 0);
}

const struct gui_draw_command*
gui_draw_list_begin(const struct gui_draw_list *list)
{
    gui_byte *memory;
    gui_size offset;
    const struct gui_draw_command *cmd;
    GUI_ASSERT(list);
    GUI_ASSERT(list->buffer);
    if (!list || !list->buffer || !list->cmd_count) return 0;
    memory = (gui_byte*)list->buffer->memory.ptr;
    offset = list->buffer->memory.size - list->cmd_offset;
    cmd = gui_ptr_add(const struct gui_draw_command, memory, offset);
    return cmd;
}

const struct gui_draw_command*
gui_draw_list_next(const struct gui_draw_list *list, const struct gui_draw_command *cmd)
{
    gui_byte *memory;
    gui_size offset;
    const struct gui_draw_command *end;
    GUI_ASSERT(list);
    if (!list || !list->buffer || !list->cmd_count) return 0;
    memory = (gui_byte*)list->buffer->memory.ptr;
    offset = list->buffer->memory.size - list->cmd_offset;
    end = gui_ptr_add(const struct gui_draw_command, memory, offset);
    end -= (list->cmd_count-1);
    if (cmd <= end) return 0;
    return (cmd - 1);
}

static struct gui_vec2*
gui_draw_list_alloc_path(struct gui_draw_list *list, gui_size count)
{
    void *memory;
    struct gui_vec2 *points;
    static const gui_size point_align = GUI_ALIGNOF(struct gui_vec2);
    static const gui_size point_size = sizeof(struct gui_vec2);
    points = (struct gui_vec2*)
        gui_buffer_alloc(list->buffer, GUI_BUFFER_FRONT, point_size * count, point_align);
    if (!points) return 0;
    if (!list->path_offset) {
        memory = gui_buffer_memory(list->buffer);
        list->path_offset = (gui_uint)((gui_byte*)points - (gui_byte*)memory);
    }
    list->path_count += (gui_uint)count;
    return points;
}

static struct gui_vec2
gui_draw_list_path_last(struct gui_draw_list *list)
{
    void *memory;
    struct gui_vec2 *point;
    GUI_ASSERT(list->path_count);
    memory = gui_buffer_memory(list->buffer);
    point = gui_ptr_add(struct gui_vec2, memory, list->path_offset);
    point += (list->path_count-1);
    return *point;
}

static struct gui_draw_command*
gui_draw_list_push_command(struct gui_draw_list *list, struct gui_rect clip,
    gui_handle texture)
{
    static const gui_size cmd_align = GUI_ALIGNOF(struct gui_draw_command);
    static const gui_size cmd_size = sizeof(struct gui_draw_command);
    struct gui_draw_command *cmd;

    GUI_ASSERT(list);
    cmd = (struct gui_draw_command*)
        gui_buffer_alloc(list->buffer, GUI_BUFFER_BACK, cmd_size, cmd_align);
    if (!cmd) return 0;
    if (!list->cmd_count) {
        gui_byte *memory = (gui_byte*)gui_buffer_memory(list->buffer);
        gui_size total = gui_buffer_total(list->buffer);
        memory = gui_ptr_add(gui_byte, memory, total);
        list->cmd_offset = (gui_size)(memory - (gui_byte*)cmd);
    }

    cmd->elem_count = 0;
    cmd->clip_rect = clip;
    cmd->texture = texture;
    list->cmd_count++;
    list->clip_rect = clip;
    return cmd;
}

static struct gui_draw_command*
gui_draw_list_command_last(struct gui_draw_list *list)
{
    void *memory;
    gui_size size;
    struct gui_draw_command *cmd;
    GUI_ASSERT(list->cmd_count);
    memory = gui_buffer_memory(list->buffer);
    size = gui_buffer_total(list->buffer);
    cmd = gui_ptr_add(struct gui_draw_command, memory, size - list->cmd_offset);
    return (cmd - (list->cmd_count-1));
}

void
gui_draw_list_add_clip(struct gui_draw_list *list, struct gui_rect rect)
{
    GUI_ASSERT(list);
    if (!list) return;
    if (!list->cmd_count) {
        gui_draw_list_push_command(list, rect, list->null.texture);
    } else {
        struct gui_draw_command *prev = gui_draw_list_command_last(list);
        gui_draw_list_push_command(list, rect, prev->texture);
    }
}

static void
gui_draw_list_push_image(struct gui_draw_list *list, gui_handle texture)
{
    GUI_ASSERT(list);
    if (!list) return;
    if (!list->cmd_count) {
        gui_draw_list_push_command(list, gui_null_rect, list->null.texture);
    } else {
        struct gui_draw_command *prev = gui_draw_list_command_last(list);
        if (prev->texture.id != texture.id)
            gui_draw_list_push_command(list, prev->clip_rect, texture);
    }
}

static struct gui_draw_vertex*
gui_draw_list_alloc_vertexes(struct gui_draw_list *list, gui_size count)
{
    struct gui_draw_vertex *vtx;
    static const gui_size vtx_align = GUI_ALIGNOF(struct gui_draw_vertex);
    static const gui_size vtx_size = sizeof(struct gui_draw_vertex);
    GUI_ASSERT(list);
    if (!list) return 0;
    vtx = (struct gui_draw_vertex*)
        gui_buffer_alloc(list->vertexes, GUI_BUFFER_FRONT, vtx_size * count, vtx_align);
    if (!vtx) return 0;
    list->vertex_count += (gui_uint)count;
    return vtx;
}

static gui_draw_index*
gui_draw_list_alloc_elements(struct gui_draw_list *list, gui_size count)
{
    gui_draw_index *ids;
    struct gui_draw_command *cmd;
    static const gui_size elem_align = GUI_ALIGNOF(gui_draw_index);
    static const gui_size elem_size = sizeof(gui_draw_index);
    GUI_ASSERT(list);
    if (!list) return 0;
    ids = (gui_draw_index*)
        gui_buffer_alloc(list->elements, GUI_BUFFER_FRONT, elem_size * count, elem_align);
    if (!ids) return 0;
    cmd = gui_draw_list_command_last(list);
    list->element_count += (gui_uint)count;
    cmd->elem_count += (gui_uint)count;
    return ids;
}

static gui_draw_vertex_color
gui_color32(struct gui_color in)
{
    gui_draw_vertex_color out = (gui_draw_vertex_color)in.r;
    out |= ((gui_draw_vertex_color)in.g << 8);
    out |= ((gui_draw_vertex_color)in.b << 16);
    out |= ((gui_draw_vertex_color)in.a << 24);
    return out;
}

static struct gui_draw_vertex
gui_draw_vertex(struct gui_vec2 pos, struct gui_vec2 uv, gui_draw_vertex_color col)
{
    struct gui_draw_vertex out;
    out.position = pos;
    out.uv = uv;
    out.col = col;
    return out;
}

static void
gui_draw_list_add_poly_line(struct gui_draw_list *list, struct gui_vec2 *points,
    const gui_uint points_count, struct gui_color color, gui_bool closed,
    gui_float thickness, enum gui_anti_aliasing aliasing)
{
    gui_size count;
    gui_bool thick_line;
    gui_draw_vertex_color col = gui_color32(color);
    GUI_ASSERT(list);
    if (!list) return;
    if (!list || points_count < 2) return;

    count = points_count;
    if (!closed) count = points_count-1;
    thick_line = thickness > 1.0f;

    if (aliasing == GUI_ANTI_ALIASING_ON) {
        /* ANTI-ALIASED STROKE */
        const gui_float AA_SIZE = 1.0f;
        static const gui_size pnt_align = GUI_ALIGNOF(struct gui_vec2);
        static const gui_size pnt_size = sizeof(struct gui_vec2);
        const gui_draw_vertex_color col_trans = col & 0x00ffffff;

        /* allocate vertexes and elements  */
        gui_size i1 = 0;
        gui_size index = list->vertex_count;
        const gui_size idx_count = (thick_line) ?  (count * 18) : (count * 12);
        const gui_size vtx_count = (thick_line) ? (points_count * 4): (points_count *3);
        struct gui_draw_vertex *vtx = gui_draw_list_alloc_vertexes(list, vtx_count);
        gui_draw_index *ids = gui_draw_list_alloc_elements(list, idx_count);

        struct gui_vec2 *normals, *temp;
        gui_size size;
        if (!vtx || !ids) return;

        /* temporary allocate normals + points */
        gui_buffer_mark(list->vertexes, GUI_BUFFER_FRONT);
        size = pnt_size * ((thick_line) ? 5 : 3) * points_count;
        normals = (struct gui_vec2*)
            gui_buffer_alloc(list->vertexes, GUI_BUFFER_FRONT, size, pnt_align);
        if (!normals) return;
        temp = normals + points_count;

        /* calculate normals */
        for (i1 = 0; i1 < count; ++i1) {
            const gui_size i2 = ((i1 + 1) == points_count) ? 0 : (i1 + 1);
            struct gui_vec2 diff = gui_vec2_sub(points[i2], points[i1]);
            gui_float len;

            /* vec2 inverted lenth  */
            len = gui_vec2_len_sqr(diff);
            if (len != 0.0f)
                len = gui_inv_sqrt(len);
            else len = 1.0f;

            diff = gui_vec2_muls(diff, len);
            normals[i1].x = diff.y;
            normals[i1].y = -diff.x;
        }

        if (!closed)
            normals[points_count-1] = normals[points_count-2];

        if (!thick_line) {
            gui_size idx1, i;
            if (!closed) {
                struct gui_vec2 d;
                temp[0] = gui_vec2_add(points[0], gui_vec2_muls(normals[0], AA_SIZE));
                temp[1] = gui_vec2_sub(points[0], gui_vec2_muls(normals[0], AA_SIZE));
                d = gui_vec2_muls(normals[points_count-1], AA_SIZE);
                temp[(points_count-1) * 2 + 0] = gui_vec2_add(points[points_count-1], d);
                temp[(points_count-1) * 2 + 1] = gui_vec2_sub(points[points_count-1], d);
            }

            /* fill elements */
            idx1 = index;
            for (i1 = 0; i1 < count; i1++) {
                struct gui_vec2 dm;
                gui_float dmr2;
                gui_size i2 = ((i1 + 1) == points_count) ? 0 : (i1 + 1);
                gui_size idx2 = ((i1+1) == points_count) ? index: (idx1 + 3);

                /* average normals */
                dm = gui_vec2_muls(gui_vec2_add(normals[i1], normals[i2]), 0.5f);
                dmr2 = dm.x * dm.x + dm.y* dm.y;
                if (dmr2 > 0.000001f) {
                    gui_float scale = 1.0f/dmr2;
                    scale = MIN(100.0f, scale);
                    dm = gui_vec2_muls(dm, scale);
                }

                dm = gui_vec2_muls(dm, AA_SIZE);
                temp[i2*2+0] = gui_vec2_add(points[i2], dm);
                temp[i2*2+1] = gui_vec2_sub(points[i2], dm);

                ids[0] = (gui_draw_index)(idx2 + 0); ids[1] = (gui_draw_index)(idx1+0);
                ids[2] = (gui_draw_index)(idx1 + 2); ids[3] = (gui_draw_index)(idx1+2);
                ids[4] = (gui_draw_index)(idx2 + 2); ids[5] = (gui_draw_index)(idx2+0);
                ids[6] = (gui_draw_index)(idx2 + 1); ids[7] = (gui_draw_index)(idx1+1);
                ids[8] = (gui_draw_index)(idx1 + 0); ids[9] = (gui_draw_index)(idx1+0);
                ids[10]= (gui_draw_index)(idx2 + 0); ids[11]= (gui_draw_index)(idx2+1);
                ids += 12;
                idx1 = idx2;
            }

            /* fill vertexes */
            for (i = 0; i < points_count; ++i) {
                const struct gui_vec2 uv = list->null.uv;
                vtx[0] = gui_draw_vertex(points[i], uv, col);
                vtx[1] = gui_draw_vertex(temp[i*2+0], uv, col_trans);
                vtx[2] = gui_draw_vertex(temp[i*2+1], uv, col_trans);
                vtx += 3;
            }
        } else {
            gui_size idx1, i;
            const gui_float half_inner_thickness = (thickness - AA_SIZE) * 0.5f;
            if (!closed) {
                struct gui_vec2 d1, d2;
                d1 = gui_vec2_muls(normals[0], half_inner_thickness + AA_SIZE);
                d2 = gui_vec2_muls(normals[0], half_inner_thickness);

                temp[0] = gui_vec2_add(points[0], d1);
                temp[1] = gui_vec2_add(points[0], d2);
                temp[2] = gui_vec2_sub(points[0], d2);
                temp[3] = gui_vec2_sub(points[0], d1);

                d1 = gui_vec2_muls(normals[points_count-1], half_inner_thickness + AA_SIZE);
                d2 = gui_vec2_muls(normals[points_count-1], half_inner_thickness);

                temp[(points_count-1)*4+0] = gui_vec2_add(points[points_count-1], d1);
                temp[(points_count-1)*4+1] = gui_vec2_add(points[points_count-1], d2);
                temp[(points_count-1)*4+2] = gui_vec2_sub(points[points_count-1], d2);
                temp[(points_count-1)*4+3] = gui_vec2_sub(points[points_count-1], d1);
            }

            /* add all elements */
            idx1 = index;
            for (i1 = 0; i1 < count; ++i1) {
                gui_float dmr2;
                struct gui_vec2 dm_out, dm_in, dm;
                const gui_size i2 = ((i1+1) == points_count) ? 0: (i1 + 1);
                gui_size idx2 = ((i1+1) == points_count) ? index: (idx1 + 4);

                /* average normals */
                dm = gui_vec2_muls(gui_vec2_add(normals[i1], normals[i2]), 0.5f);
                dmr2 = dm.x * dm.x + dm.y* dm.y;
                if (dmr2 > 0.000001f) {
                    gui_float scale = 1.0f/dmr2;
                    scale = MIN(100.0f, scale);
                    dm = gui_vec2_muls(dm, scale);
                }

                dm_out = gui_vec2_muls(dm, ((half_inner_thickness) + AA_SIZE));
                dm_in = gui_vec2_muls(dm, half_inner_thickness);
                temp[i2*4+0] = gui_vec2_add(points[i2], dm_out);
                temp[i2*4+1] = gui_vec2_add(points[i2], dm_in);
                temp[i2*4+2] = gui_vec2_sub(points[i2], dm_in);
                temp[i2*4+3] = gui_vec2_sub(points[i2], dm_out);

                /* add indexes */
                ids[0] = (gui_draw_index)(idx2 + 1); ids[1] = (gui_draw_index)(idx1+1);
                ids[2] = (gui_draw_index)(idx1 + 2); ids[3] = (gui_draw_index)(idx1+2);
                ids[4] = (gui_draw_index)(idx2 + 2); ids[5] = (gui_draw_index)(idx2+1);
                ids[6] = (gui_draw_index)(idx2 + 1); ids[7] = (gui_draw_index)(idx1+1);
                ids[8] = (gui_draw_index)(idx1 + 0); ids[9] = (gui_draw_index)(idx1+0);
                ids[10]= (gui_draw_index)(idx2 + 0); ids[11] = (gui_draw_index)(idx2+1);
                ids[12]= (gui_draw_index)(idx2 + 2); ids[13] = (gui_draw_index)(idx1+2);
                ids[14]= (gui_draw_index)(idx1 + 3); ids[15] = (gui_draw_index)(idx1+3);
                ids[16]= (gui_draw_index)(idx2 + 3); ids[17] = (gui_draw_index)(idx2+2);
                ids += 18;
                idx1 = idx2;
            }

            /* add vertexes */
            for (i = 0; i < points_count; ++i) {
                const struct gui_vec2 uv = list->null.uv;
                vtx[0] = gui_draw_vertex(temp[i*4+0], uv, col_trans);
                vtx[1] = gui_draw_vertex(temp[i*4+1], uv, col);
                vtx[2] = gui_draw_vertex(temp[i*4+2], uv, col);
                vtx[3] = gui_draw_vertex(temp[i*4+3], uv, col_trans);
                vtx += 4;
            }
        }

        /* free temporary normals + points */
        gui_buffer_reset(list->vertexes, GUI_BUFFER_FRONT);
    } else {
        /* NON ANTI-ALIASED STROKE */
        gui_size i1 = 0;
        gui_size idx = list->vertex_count;
        const gui_size idx_count = count * 6;
        const gui_size vtx_count = count * 4;
        struct gui_draw_vertex *vtx = gui_draw_list_alloc_vertexes(list, vtx_count);
        gui_draw_index *ids = gui_draw_list_alloc_elements(list, idx_count);
        if (!vtx || !ids) return;

        for (i1 = 0; i1 < count; ++i1) {
            gui_float dx, dy;
            const struct gui_vec2 uv = list->null.uv;
            const gui_size i2 = ((i1+1) == points_count) ? 0 : i1 + 1;
            const struct gui_vec2 p1 = points[i1];
            const struct gui_vec2 p2 = points[i2];
            struct gui_vec2 diff = gui_vec2_sub(p2, p1);
            gui_float len;

            /* vec2 inverted lenth  */
            len = gui_vec2_len_sqr(diff);
            if (len != 0.0f)
                len = gui_inv_sqrt(len);
            else len = 1.0f;
            diff = gui_vec2_muls(diff, len);

            /* add vertexes */
            dx = diff.x * (thickness * 0.5f);
            dy = diff.y * (thickness * 0.5f);

            vtx[0] = gui_draw_vertex(gui_vec2(p1.x + dy, p1.y - dx), uv, col);
            vtx[1] = gui_draw_vertex(gui_vec2(p2.x + dy, p2.y - dx), uv, col);
            vtx[2] = gui_draw_vertex(gui_vec2(p2.x - dy, p2.y + dx), uv, col);
            vtx[3] = gui_draw_vertex(gui_vec2(p1.x - dy, p1.y + dx), uv, col);
            vtx += 4;

            ids[0] = (gui_draw_index)(idx+0); ids[1] = (gui_draw_index)(idx+1);
            ids[2] = (gui_draw_index)(idx+2); ids[3] = (gui_draw_index)(idx+0);
            ids[4] = (gui_draw_index)(idx+2); ids[5] = (gui_draw_index)(idx+3);
            ids += 6;
            idx += 4;
        }
    }
}

static void
gui_draw_list_add_poly_convex(struct gui_draw_list *list, struct gui_vec2 *points,
    const gui_uint points_count, struct gui_color color, enum gui_anti_aliasing aliasing)
{
    static const gui_size pnt_align = GUI_ALIGNOF(struct gui_vec2);
    static const gui_size pnt_size = sizeof(struct gui_vec2);
    gui_draw_vertex_color col = gui_color32(color);
    GUI_ASSERT(list);
    if (!list || points_count < 3) return;

    if (aliasing == GUI_ANTI_ALIASING_ON) {
        gui_size i = 0;
        gui_size i0 = 0, i1 = 0;

        const gui_float AA_SIZE = 1.0f;
        const gui_draw_vertex_color col_trans = col & 0x00ffffff;
        gui_size index = list->vertex_count;
        const gui_size idx_count = (points_count-2)*3 + points_count*6;
        const gui_size vtx_count = (points_count*2);
        struct gui_draw_vertex *vtx = gui_draw_list_alloc_vertexes(list, vtx_count);
        gui_draw_index *ids = gui_draw_list_alloc_elements(list, idx_count);

        gui_uint vtx_inner_idx = (gui_uint)(index + 0);
        gui_uint vtx_outer_idx = (gui_uint)(index + 1);
        struct gui_vec2 *normals;
        gui_size size;
        if (!vtx || !ids) return;

        /* temporary allocate normals */
        gui_buffer_mark(list->vertexes, GUI_BUFFER_FRONT);
        size = pnt_size * points_count;
        normals = (struct gui_vec2*)
            gui_buffer_alloc(list->vertexes, GUI_BUFFER_FRONT, size, pnt_align);
        if (!normals) return;

        /* add elements */
        for (i = 2; i < points_count; i++) {
            ids[0] = (gui_draw_index)(vtx_inner_idx);
            ids[1] = (gui_draw_index)(vtx_inner_idx + ((i-1) << 1));
            ids[2] = (gui_draw_index)(vtx_inner_idx + (i << 1));
            ids += 3;
        }

        /* compute normals */
        for (i0 = points_count-1, i1 = 0; i1 < points_count; i0 = i1++) {
            struct gui_vec2 p0 = points[i0];
            struct gui_vec2 p1 = points[i1];
            struct gui_vec2 diff = gui_vec2_sub(p1, p0);
            gui_float len;

            /* vec2 inverted lenth  */
            len = gui_vec2_len_sqr(diff);
            if (len != 0.0f)
                len = gui_inv_sqrt(len);
            else len = 1.0f;
            diff = gui_vec2_muls(diff, len);

            normals[i0].x = diff.y;
            normals[i0].y = -diff.x;
        }

        /* add vertexes + indexes */
        for (i0 = points_count-1, i1 = 0; i1 < points_count; i0 = i1++) {
            const struct gui_vec2 uv = list->null.uv;
            struct gui_vec2 n0 = normals[i0];
            struct gui_vec2 n1 = normals[i1];
            struct gui_vec2 dm = gui_vec2_muls(gui_vec2_add(n0, n1), 0.5f);

            gui_float dmr2 = dm.x*dm.x + dm.y*dm.y;
            if (dmr2 > 0.000001f) {
                gui_float scale = 1.0f / dmr2;
                scale = MIN(scale, 100.0f);
                dm = gui_vec2_muls(dm, scale);
            }
            dm = gui_vec2_muls(dm, AA_SIZE * 0.5f);

            /* add vertexes */
            vtx[0] = gui_draw_vertex(gui_vec2_sub(points[i1], dm), uv, col);
            vtx[1] = gui_draw_vertex(gui_vec2_add(points[i1], dm), uv, col_trans);
            vtx += 2;

            /* add indexes */
            ids[0] = (gui_draw_index)(vtx_inner_idx+(i1<<1));
            ids[1] = (gui_draw_index)(vtx_inner_idx+(i0<<1));
            ids[2] = (gui_draw_index)(vtx_outer_idx+(i0<<1));
            ids[3] = (gui_draw_index)(vtx_outer_idx+(i0<<1));
            ids[4] = (gui_draw_index)(vtx_outer_idx+(i1<<1));
            ids[5] = (gui_draw_index)(vtx_inner_idx+(i1<<1));
            ids += 6;
        }
        /* free temporary normals + points */
        gui_buffer_reset(list->vertexes, GUI_BUFFER_FRONT);
    } else {
        gui_size i;
        gui_size index = list->vertex_count;
        const gui_size idx_count = (points_count-2)*3;
        const gui_size vtx_count = points_count;
        struct gui_draw_vertex *vtx = gui_draw_list_alloc_vertexes(list, vtx_count);
        gui_draw_index *ids = gui_draw_list_alloc_elements(list, idx_count);
        if (!vtx || !ids) return;
        for (i = 0; i < vtx_count; ++i) {
            vtx[0] = gui_draw_vertex(points[i], list->null.uv, col);
            vtx++;
        }
        for (i = 2; i < points_count; ++i) {
            ids[0] = (gui_draw_index)index;
            ids[1] = (gui_draw_index)(index+ i - 1);
            ids[2] = (gui_draw_index)(index+i);
            ids += 3;
        }
    }
}

void
gui_draw_list_add_line(struct gui_draw_list *list, struct gui_vec2 a,
    struct gui_vec2 b, struct gui_color col, gui_float thickness)
{
    GUI_ASSERT(list);
    if (!list || !col.a) return;
    gui_draw_list_path_line_to(list, gui_vec2_add(a, gui_vec2(0.5f, 0.5f)));
    gui_draw_list_path_line_to(list, gui_vec2_add(b, gui_vec2(0.5f, 0.5f)));
    gui_draw_list_path_stroke(list,  col, GUI_STROKE_OPEN, thickness);
}

void
gui_draw_list_add_rect(struct gui_draw_list *list, struct gui_rect rect,
    struct gui_color col, gui_float rounding, gui_float line_thickness)
{
    GUI_ASSERT(list);
    if (!list || !col.a) return;
    gui_draw_list_path_rect_to(list, gui_vec2(rect.x + 0.5f, rect.y + 0.5f),
        gui_vec2(rect.x + rect.w + 0.5f, rect.y + rect.h + 0.5f), rounding);
    gui_draw_list_path_stroke(list,  col, GUI_STROKE_CLOSED, line_thickness);
}

void
gui_draw_list_add_rect_filled(struct gui_draw_list *list, struct gui_rect rect,
    struct gui_color col, gui_float rounding)
{
    GUI_ASSERT(list);
    if (!list || !col.a) return;
    gui_draw_list_path_rect_to(list, gui_vec2(rect.x + 0.5f, rect.y + 0.5f),
        gui_vec2(rect.x + rect.w + 0.5f, rect.y + rect.h + 0.5f), rounding);
    gui_draw_list_path_fill(list,  col);
}

void
gui_draw_list_add_triangle(struct gui_draw_list *list, struct gui_vec2 a, struct gui_vec2 b,
    struct gui_vec2 c, struct gui_color col, gui_float line_thickness)
{
    GUI_ASSERT(list);
    if (!list) return;
    if (!col.a) return;
    gui_draw_list_path_line_to(list, a);
    gui_draw_list_path_line_to(list, b);
    gui_draw_list_path_line_to(list, c);
    gui_draw_list_path_stroke(list, col, GUI_STROKE_CLOSED, line_thickness);
}

void
gui_draw_list_add_triangle_filled(struct gui_draw_list *list, struct gui_vec2 a,
    struct gui_vec2 b, struct gui_vec2 c, struct gui_color col)
{
    GUI_ASSERT(list);
    if (!list || !col.a) return;
    gui_draw_list_path_line_to(list, a);
    gui_draw_list_path_line_to(list, b);
    gui_draw_list_path_line_to(list, c);
    gui_draw_list_path_fill(list, col);
}

void
gui_draw_list_add_circle(struct gui_draw_list *list, struct gui_vec2 center,
    gui_float radius, struct gui_color col, gui_uint segs, gui_float line_thickness)
{
    gui_float a_max;
    GUI_ASSERT(list);
    if (!list || !col.a) return;
    a_max = GUI_PI * 2.0f * ((gui_float)segs - 1.0f) / (gui_float)segs;
    gui_draw_list_path_arc_to(list, center, radius, 0.0f, a_max, segs);
    gui_draw_list_path_stroke(list, col, GUI_STROKE_CLOSED, line_thickness);
}

void
gui_draw_list_add_circle_filled(struct gui_draw_list *list, struct gui_vec2 center,
    gui_float radius, struct gui_color col, gui_uint segs)
{
    gui_float a_max;
    GUI_ASSERT(list);
    if (!list || !col.a) return;
    a_max = GUI_PI * 2.0f * ((gui_float)segs - 1.0f) / (gui_float)segs;
    gui_draw_list_path_arc_to(list, center, radius, 0.0f, a_max, segs);
    gui_draw_list_path_fill(list, col);
}

void
gui_draw_list_add_curve(struct gui_draw_list *list, struct gui_vec2 p0,
    struct gui_vec2 cp0, struct gui_vec2 cp1, struct gui_vec2 p1,
    struct gui_color col, gui_uint segments, gui_float thickness)
{
    GUI_ASSERT(list);
    if (!list || !col.a) return;
    gui_draw_list_path_line_to(list, p0);
    gui_draw_list_path_curve_to(list, cp0, cp1, p1, segments);
    gui_draw_list_path_stroke(list, col, GUI_STROKE_OPEN, thickness);
}

static void
gui_draw_list_push_rect_uv(struct gui_draw_list *list, struct gui_vec2 a,
    struct gui_vec2 c, struct gui_vec2 uva, struct gui_vec2 uvc, struct gui_color color)
{
    gui_draw_vertex_color col = gui_color32(color);
    struct gui_draw_vertex *vtx;
    struct gui_vec2 uvb, uvd;
    struct gui_vec2 b,d;
    gui_draw_index *idx;
    gui_draw_index index;
    GUI_ASSERT(list);
    if (!list) return;

    uvb = gui_vec2(uvc.x, uva.y);
    uvd = gui_vec2(uva.x, uvc.y);
    b = gui_vec2(c.x, a.y);
    d = gui_vec2(a.x, c.y);

    index = (gui_draw_index)list->vertex_count;
    vtx = gui_draw_list_alloc_vertexes(list, 4);
    idx = gui_draw_list_alloc_elements(list, 6);
    if (!vtx || !idx) return;

    idx[0] = (gui_draw_index)(index+0); idx[1] = (gui_draw_index)(index+1);
    idx[2] = (gui_draw_index)(index+2); idx[3] = (gui_draw_index)(index+0);
    idx[4] = (gui_draw_index)(index+2); idx[5] = (gui_draw_index)(index+3);

    vtx[0] = gui_draw_vertex(a, uva, col);
    vtx[1] = gui_draw_vertex(b, uvb, col);
    vtx[2] = gui_draw_vertex(c, uvc, col);
    vtx[3] = gui_draw_vertex(d, uvd, col);
}

void
gui_draw_list_add_image(struct gui_draw_list *list, struct gui_image texture,
    struct gui_rect rect, struct gui_color color)
{
    GUI_ASSERT(list);
    if (!list) return;
    /* push new command with given texture */
    gui_draw_list_push_image(list, texture.handle);
    if (gui_image_is_subimage(&texture)) {
        /* add region inside of the texture  */
        struct gui_vec2 uv[2];
        uv[0].x = (gui_float)texture.region[0]/(gui_float)texture.w;
        uv[0].y = (gui_float)texture.region[1]/(gui_float)texture.h;
        uv[1].x = (gui_float)(texture.region[0] + texture.region[2])/(gui_float)texture.w;
        uv[1].y = (gui_float)(texture.region[1] + texture.region[3])/(gui_float)texture.h;
        gui_draw_list_push_rect_uv(list, gui_vec2(rect.x, rect.y),
            gui_vec2(rect.x + rect.w, rect.y + rect.h),  uv[0], uv[1], color);
    } else gui_draw_list_push_rect_uv(list, gui_vec2(rect.x, rect.y),
            gui_vec2(rect.x + rect.w, rect.y + rect.h),
            gui_vec2(0.0f, 0.0f), gui_vec2(1.0f, 1.0f),color);
}

void
gui_draw_list_add_text(struct gui_draw_list *list, const struct gui_user_font *font,
    struct gui_rect rect, const gui_char *text, gui_size len,
    struct gui_color bg, struct gui_color fg)
{
    gui_float x;
    gui_size text_len;
    gui_long unicode, next;
    gui_size glyph_len, next_glyph_len;
    struct gui_user_font_glyph g;
    GUI_ASSERT(list);
    if (!list || !len || !text) return;
    if (rect.x > (list->clip_rect.x + list->clip_rect.w) ||
        rect.y > (list->clip_rect.y + list->clip_rect.h) ||
        rect.x < list->clip_rect.x || rect.y < list->clip_rect.y)
        return;

    /* draw text background */
    gui_draw_list_add_rect_filled(list, rect, bg, 0.0f);
    gui_draw_list_push_image(list, font->texture);

    /* draw every glyph image */
    x = rect.x;
    glyph_len = text_len = gui_utf_decode(text, &unicode, len);
    if (!glyph_len) return;
    while (text_len <= len && glyph_len) {
        gui_float gx, gy, gh, gw;
        gui_float char_width = 0;
        if (unicode == GUI_UTF_INVALID) break;

        /* query currently drawn glyph information */
        next_glyph_len = gui_utf_decode(text + text_len, &next, len - text_len);
        font->query(font->userdata, &g, unicode, (next == GUI_UTF_INVALID) ? '\0' : next);

        /* calculate and draw glyph drawing rectangle and image */
        gx = x + g.offset.x;
        /*gy = rect.y + (font->height/2) + (rect.h/2) - g.offset.y;*/
        gy = rect.y + (rect.h/2) - (font->height/2) + g.offset.y;
        gw = g.width; gh = g.height;
        char_width = g.xadvance;
        gui_draw_list_push_rect_uv(list, gui_vec2(gx,gy), gui_vec2(gx + gw, gy+ gh),
            g.uv[0], g.uv[1], fg);

        /* offset next glyph */
        text_len += glyph_len;
        x += char_width;
        glyph_len = next_glyph_len;
        unicode = next;
    }
}

void
gui_draw_list_path_clear(struct gui_draw_list *list)
{
    GUI_ASSERT(list);
    if (!list) return;
    gui_buffer_reset(list->buffer, GUI_BUFFER_FRONT);
    list->path_count = 0;
    list->path_offset = 0;
}

void
gui_draw_list_path_line_to(struct gui_draw_list *list, struct gui_vec2 pos)
{
    struct gui_vec2 *points;
    struct gui_draw_command *cmd;
    GUI_ASSERT(list);
    if (!list) return;
    if (!list->cmd_count)
        gui_draw_list_add_clip(list, gui_null_rect);
    cmd = gui_draw_list_command_last(list);
    if (cmd->texture.ptr != list->null.texture.ptr)
        gui_draw_list_push_image(list, list->null.texture);

    points = gui_draw_list_alloc_path(list, 1);
    if (!points) return;
    points[0] = pos;
}

static void
gui_draw_list_path_arc_to_fast(struct gui_draw_list *list, struct gui_vec2 center,
    gui_float radius, gui_int a_min, gui_int a_max)
{
    static struct gui_vec2 circle_vtx[12];
    static gui_bool circle_vtx_builds = gui_false;
    static const gui_int circle_vtx_count = GUI_LEN(circle_vtx);

    GUI_ASSERT(list);
    if (!list) return;
    if (!circle_vtx_builds) {
        gui_int i = 0;
        for (i = 0; i < circle_vtx_count; ++i) {
            const gui_float a = ((gui_float)i / (gui_float)circle_vtx_count) * 2 * GUI_PI;
            circle_vtx[i].x = list->cos(a);
            circle_vtx[i].y = list->sin(a);
        }
        circle_vtx_builds = gui_true;
    }

    if (a_min <= a_max) {
        gui_int a = 0;
        for (a = a_min; a <= a_max; a++) {
            const struct gui_vec2 c = circle_vtx[a % circle_vtx_count];
            const gui_float x = center.x + c.x * radius;
            const gui_float y = center.y + c.y * radius;
            gui_draw_list_path_line_to(list, gui_vec2(x, y));
        }
    }
}

void
gui_draw_list_path_arc_to(struct gui_draw_list *list, struct gui_vec2 center,
    gui_float radius, gui_float a_min, gui_float a_max, gui_uint segments)
{
    gui_uint i = 0;
    GUI_ASSERT(list);
    if (!list) return;
    if (radius == 0.0f) return;
    for (i = 0; i <= segments; ++i) {
        const gui_float a = a_min + ((gui_float)i / (gui_float)segments) * (a_max - a_min);
        const gui_float x = center.x + list->cos(a) * radius;
        const gui_float y = center.y + list->sin(a) * radius;
        gui_draw_list_path_line_to(list, gui_vec2(x, y));
    }
}

void
gui_draw_list_path_rect_to(struct gui_draw_list *list, struct gui_vec2 a,
    struct gui_vec2 b, gui_float rounding)
{
    gui_float r;
    GUI_ASSERT(list);
    if (!list) return;
    r = rounding;
    r = MIN(r, ((b.x-a.x) < 0) ? -(b.x-a.x): (b.x-a.x));
    r = MIN(r, ((b.y-a.y) < 0) ? -(b.y-a.y): (b.y-a.y));
    if (r == 0.0f) {
        gui_draw_list_path_line_to(list, a);
        gui_draw_list_path_line_to(list, gui_vec2(b.x,a.y));
        gui_draw_list_path_line_to(list, b);
        gui_draw_list_path_line_to(list, gui_vec2(a.x,b.y));
    } else {
        gui_draw_list_path_arc_to_fast(list, gui_vec2(a.x + r, a.y + r), r, 6, 9);
        gui_draw_list_path_arc_to_fast(list, gui_vec2(b.x - r, a.y + r), r, 9, 12);
        gui_draw_list_path_arc_to_fast(list, gui_vec2(b.x - r, b.y - r), r, 0, 3);
        gui_draw_list_path_arc_to_fast(list, gui_vec2(a.x + r, b.y - r), r, 3, 6);
    }
}

void
gui_draw_list_path_curve_to(struct gui_draw_list *list, struct gui_vec2 p2,
    struct gui_vec2 p3, struct gui_vec2 p4, gui_uint num_segments)
{
    gui_uint i_step;
    gui_float t_step;
    struct gui_vec2 p1;

    GUI_ASSERT(list);
    GUI_ASSERT(list->path_count);
    if (!list || !list->path_count) return;
    num_segments = MAX(num_segments, 1);

    p1 = gui_draw_list_path_last(list);
    t_step = 1.0f/(gui_float)num_segments;
    for (i_step = 1; i_step <= num_segments; ++i_step) {
        gui_float t = t_step * (gui_float)i_step;
        gui_float u = 1.0f - t;
        gui_float w1 = u*u*u;
        gui_float w2 = 3*u*u*t;
        gui_float w3 = 3*u*t*t;
        gui_float w4 = t * t *t;
        gui_float x = w1 * p1.x + w2 * p2.x + w3 * p3.x + w4 * p4.x;
        gui_float y = w1 * p1.y + w2 * p2.y + w3 * p3.y + w4 * p4.y;
        gui_draw_list_path_line_to(list, gui_vec2(x,y));
    }
}

void
gui_draw_list_path_fill(struct gui_draw_list *list, struct gui_color color)
{
    struct gui_vec2 *points;
    GUI_ASSERT(list);
    if (!list) return;
    points = (struct gui_vec2*)gui_buffer_memory(list->buffer);
    gui_draw_list_add_poly_convex(list, points, list->path_count, color, list->AA);
    gui_draw_list_path_clear(list);
}

void
gui_draw_list_path_stroke(struct gui_draw_list *list, struct gui_color color,
    gui_bool closed, gui_float thickness)
{
    struct gui_vec2 *points;
    GUI_ASSERT(list);
    if (!list) return;
    points = (struct gui_vec2*)gui_buffer_memory(list->buffer);
    gui_draw_list_add_poly_line(list, points, list->path_count, color,
        closed, thickness, list->AA);
    gui_draw_list_path_clear(list);
}
#endif
/*
 * ==============================================================
 *
 *                          Font
 *
 * ===============================================================
 */
#ifdef GUI_COMPILE_WITH_FONT

/* this is a bloody mess but 'fixing' both stb libraries would be a pain */
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wfloat-equal"
#pragma clang diagnostic ignored "-Wbad-function-cast"
#pragma clang diagnostic ignored "-Wcast-qual"
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wdeclaration-after-statement"
#pragma clang diagnostic ignored "-Wunused-function"
#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wbad-function-cast"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wswitch-default"
#pragma GCC diagnostic ignored "-Wunused-function"
#elif _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4456)
#endif

#ifndef GUI_DISABLE_STB_RECT_PACK_IMPLEMENTATION
#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#endif
#include "stb_rect_pack.h"

#ifndef GUI_DISABLE_STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#endif
#include "stb_truetype.h"

#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop
#elif _MSC_VER
#pragma warning (pop)
#endif

struct gui_font_bake_data {
    stbtt_fontinfo info;
    stbrp_rect *rects;
    stbtt_pack_range *ranges;
    gui_uint range_count;
};

struct gui_font_baker {
    stbtt_pack_context spc;
    struct gui_font_bake_data *build;
    stbtt_packedchar *packed_chars;
    stbrp_rect *rects;
    stbtt_pack_range *ranges;
};

static const gui_size gui_rect_align = GUI_ALIGNOF(stbrp_rect);
static const gui_size gui_range_align = GUI_ALIGNOF(stbtt_pack_range);
static const gui_size gui_char_align = GUI_ALIGNOF(stbtt_packedchar);
static const gui_size gui_build_align = GUI_ALIGNOF(struct gui_font_bake_data);
static const gui_size gui_baker_align = GUI_ALIGNOF(struct gui_font_baker);

static gui_size
gui_range_count(const gui_long *range)
{
    const gui_long *iter = range;
    GUI_ASSERT(range);
    if (!range) return 0;
    while (*(iter++) != 0);
    return (iter == range) ? 0 : (gui_size)((iter - range)/2);
}

static gui_size
gui_range_glyph_count(const gui_long *range, gui_size count)
{
    gui_size i = 0;
    gui_size total_glyphes = 0;
    for (i = 0; i < count; ++i) {
        gui_size diff;
        gui_long f = range[(i*2)+0];
        gui_long t = range[(i*2)+1];
        GUI_ASSERT(t >= f);
        diff = (gui_size)((t - f) + 1);
        total_glyphes += diff;
    }
    return total_glyphes;
}

const gui_long*
gui_font_default_glyph_ranges(void)
{
    static const gui_long ranges[] = {0x0020, 0x00FF, 0};
    return ranges;
}

const gui_long*
gui_font_chinese_glyph_ranges(void)
{
    static const gui_long ranges[] = {
        0x0020, 0x00FF,
        0x3000, 0x30FF,
        0x31F0, 0x31FF,
        0xFF00, 0xFFEF,
        0x4e00, 0x9FAF,
        0
    };
    return ranges;
}

const gui_long*
gui_font_cyrillic_glyph_ranges(void)
{
    static const gui_long ranges[] = {
        0x0020, 0x00FF,
        0x0400, 0x052F,
        0x2DE0, 0x2DFF,
        0xA640, 0xA69F,
        0
    };
    return ranges;
}

void gui_font_bake_memory(gui_size *temp, gui_size *glyph_count,
    struct gui_font_config *config, gui_size count)
{
    gui_size i;
    gui_size range_count = 0;

    GUI_ASSERT(config);
    GUI_ASSERT(glyph_count);
    if (!config) {
        *temp = 0;
        *glyph_count = 0;
        return;
    }

    *glyph_count = 0;
    if (!config->range)
        config->range = gui_font_default_glyph_ranges();
    for (i = 0; i < count; ++i) {
        range_count += gui_range_count(config[i].range);
        *glyph_count += gui_range_glyph_count(config[i].range, range_count);
    }

    *temp = *glyph_count * sizeof(stbrp_rect);
    *temp += range_count * sizeof(stbtt_pack_range);
    *temp += *glyph_count * sizeof(stbtt_packedchar);
    *temp += count * sizeof(struct gui_font_bake_data);
    *temp += sizeof(struct gui_font_baker);
    *temp += gui_rect_align + gui_range_align + gui_char_align;
    *temp += gui_build_align + gui_baker_align;
}

static struct gui_font_baker*
gui_font_baker(void *memory, gui_size glyph_count, gui_size count)
{
    struct gui_font_baker *baker;
    if (!memory) return 0;
    /* setup baker inside a memory block  */
    baker = (struct gui_font_baker*)GUI_ALIGN_PTR(memory, gui_baker_align);
    baker->build = (struct gui_font_bake_data*)GUI_ALIGN_PTR((baker + 1), gui_build_align);
    baker->packed_chars = (stbtt_packedchar*)GUI_ALIGN_PTR((baker->build + count), gui_char_align);
    baker->rects = (stbrp_rect*)GUI_ALIGN_PTR((baker->packed_chars + glyph_count), gui_rect_align);
    baker->ranges = (stbtt_pack_range*)GUI_ALIGN_PTR((baker->rects + glyph_count), gui_range_align);
    return baker;
}

gui_bool
gui_font_bake_pack(gui_size *image_memory, gui_size *width, gui_size *height,
    struct gui_recti *custom, void *temp, gui_size temp_size,
    const struct gui_font_config *config, gui_size count)
{
    static const gui_size max_height = 1024 * 32;
    struct gui_font_baker* baker;
    gui_size total_glyph_count = 0;
    gui_size total_range_count = 0;
    gui_size i = 0;

    GUI_ASSERT(image_memory);
    GUI_ASSERT(width);
    GUI_ASSERT(height);
    GUI_ASSERT(config);
    GUI_ASSERT(temp);
    GUI_ASSERT(temp_size);
    GUI_ASSERT(count);
    if (!image_memory || !width || !height || !config || !temp ||
        !temp_size || !count) return gui_false;

    for (i = 0; i < count; ++i) {
        total_range_count += gui_range_count(config[i].range);
        total_glyph_count += gui_range_glyph_count(config[i].range, total_range_count);
    }

    /* setup font baker from temporary memory */
    gui_zero(temp, temp_size);
    baker = gui_font_baker(temp, total_glyph_count, count);
    if (!baker) return gui_false;
    for (i = 0; i < count; ++i) {
        const struct gui_font_config *cfg = &config[i];
        if (!stbtt_InitFont(&baker->build[i].info, (const unsigned char*)cfg->ttf_blob, 0))
            return gui_false;
    }

    *height = 0;
    *width = (total_glyph_count > 1000) ? 1024 : 512;
    stbtt_PackBegin(&baker->spc, NULL, (gui_int)*width, (int)max_height, 0, 1, 0);
    {
        gui_size input_i = 0;
        gui_size range_n = 0, rect_n = 0, char_n = 0;

        /* pack custom user data first so it will be in the upper left corner  */
        if (custom) {
            stbrp_rect custom_space;
            gui_zero(&custom_space, sizeof(custom_space));
            custom_space.w = (stbrp_coord)((custom->w * 2) + 1);
            custom_space.h = (stbrp_coord)(custom->h + 1);

            stbtt_PackSetOversampling(&baker->spc, 1, 1);
            stbrp_pack_rects((stbrp_context*)baker->spc.pack_info, &custom_space, 1);
            *height = MAX(*height, (size_t)(custom_space.y + custom_space.h));

            custom->x = custom_space.x;
            custom->y = custom_space.y;
            custom->w = custom_space.w;
            custom->h = custom_space.h;
        }

        /* first font pass: pack all glyphes */
        for (input_i = 0; input_i < count; input_i++) {
            gui_size n = 0;
            const gui_long *in_range;
            const struct gui_font_config *cfg = &config[input_i];
            struct gui_font_bake_data *tmp = &baker->build[input_i];
            gui_size glyph_count, range_count;

            /* count glyphes + ranges in current font */
            glyph_count = 0; range_count = 0;
            for (in_range = cfg->range; in_range[0] && in_range[1]; in_range += 2) {
                glyph_count += (gui_size)(in_range[1] - in_range[0]) + 1;
                range_count++;
            }

            /* setup ranges  */
            tmp->ranges = baker->ranges + range_n;
            tmp->range_count = (gui_uint)range_count;
            range_n += range_count;
            for (i = 0; i < range_count; ++i) {
                /*stbtt_pack_range *range = &tmp->ranges[i];*/
                in_range = &cfg->range[i * 2];
                tmp->ranges[i].font_size = cfg->size;
                tmp->ranges[i].first_unicode_codepoint_in_range = (gui_int)in_range[0];
                tmp->ranges[i].num_chars = (gui_int)(in_range[1]- in_range[0]) + 1;
                tmp->ranges[i].chardata_for_range = baker->packed_chars + char_n;
                char_n += (gui_size)tmp->ranges[i].num_chars;
            }

            /* pack */
            tmp->rects = baker->rects + rect_n;
            rect_n += glyph_count;
            stbtt_PackSetOversampling(&baker->spc, cfg->oversample_h, cfg->oversample_v);
            n = (gui_size)stbtt_PackFontRangesGatherRects(&baker->spc, &tmp->info,
                tmp->ranges, (gui_int)tmp->range_count, tmp->rects);
            stbrp_pack_rects((stbrp_context*)baker->spc.pack_info, tmp->rects, (gui_int)n);

            /* texture height */
            for (i = 0; i < n; ++i) {
                if (tmp->rects[i].was_packed)
                    *height = MAX(*height, (gui_size)(tmp->rects[i].y + tmp->rects[i].h));
            }
        }
        GUI_ASSERT(rect_n == total_glyph_count);
        GUI_ASSERT(char_n == total_glyph_count);
        GUI_ASSERT(range_n == total_range_count);
    }
    *height = gui_round_up_pow2((gui_uint)*height);
    *image_memory = (*width) * (*height);
    return gui_true;
}

void
gui_font_bake(void *image_memory, gui_size width, gui_size height,
    void *temp, gui_size temp_size, struct gui_font_glyph *glyphes,
    gui_size glyphes_count, const struct gui_font_config *config, gui_size font_count)
{
    gui_size input_i = 0;
    struct gui_font_baker* baker;
    gui_long glyph_n = 0;

    GUI_ASSERT(image_memory);
    GUI_ASSERT(width);
    GUI_ASSERT(height);
    GUI_ASSERT(config);
    GUI_ASSERT(temp);
    GUI_ASSERT(temp_size);
    GUI_ASSERT(font_count);
    GUI_ASSERT(glyphes_count);
    if (!image_memory || !width || !height || !config || !temp ||
        !temp_size || !font_count || !glyphes || !glyphes_count)
        return;

    /* second font pass: render glyphes */
    baker = (struct gui_font_baker*)temp;
    gui_zero(image_memory, width * height);
    baker->spc.pixels = (unsigned char*)image_memory;
    baker->spc.height = (gui_int)height;
    for (input_i = 0; input_i < font_count; ++input_i) {
        const struct gui_font_config *cfg = &config[input_i];
        struct gui_font_bake_data *tmp = &baker->build[input_i];
        stbtt_PackSetOversampling(&baker->spc, cfg->oversample_h, cfg->oversample_v);
        stbtt_PackFontRangesRenderIntoRects(&baker->spc, &tmp->info, tmp->ranges,
            (gui_int)tmp->range_count, tmp->rects);
    }
    stbtt_PackEnd(&baker->spc);

    /* third pass: setup font and glyphes */
    for (input_i = 0; input_i < font_count; ++input_i)  {
        gui_size i = 0;
        gui_int char_idx = 0;
        gui_uint glyph_count = 0;
        const struct gui_font_config *cfg = &config[input_i];
        struct gui_font_bake_data *tmp = &baker->build[input_i];
        struct gui_baked_font *dst_font = cfg->font;

        gui_float font_scale = stbtt_ScaleForPixelHeight(&tmp->info, cfg->size);
        gui_int unscaled_ascent, unscaled_descent, unscaled_line_gap;
        stbtt_GetFontVMetrics(&tmp->info, &unscaled_ascent, &unscaled_descent,
                                &unscaled_line_gap);

        /* fill baked font */
        dst_font->ranges = cfg->range;
        dst_font->height = cfg->size;
        dst_font->ascent = ((gui_float)unscaled_ascent * font_scale);
        dst_font->descent = ((gui_float)unscaled_descent * font_scale);
        dst_font->glyph_offset = glyph_n;

        /* fill own baked font glyph array */
        for (i = 0; i < tmp->range_count; ++i) {
            stbtt_pack_range *range = &tmp->ranges[i];
            for (char_idx = 0; char_idx < range->num_chars; char_idx++) {
                gui_int codepoint = 0;
                gui_float dummy_x = 0, dummy_y = 0;
                stbtt_aligned_quad q;
                struct gui_font_glyph *glyph;

                const stbtt_packedchar *pc = &range->chardata_for_range[char_idx];
                glyph_count++;
                if (!pc->x0 && !pc->x1 && !pc->y0 && !pc->y1) continue;
                codepoint = range->first_unicode_codepoint_in_range + char_idx;
                stbtt_GetPackedQuad(range->chardata_for_range, (gui_int)width,
                    (gui_int)height, char_idx, &dummy_x, &dummy_y, &q, 0);

                /* fill own glyph type with data */
                glyph = &glyphes[dst_font->glyph_offset + char_idx];
                glyph->codepoint = (gui_long)codepoint;
                glyph->x0 = q.x0; glyph->y0 = q.y0; glyph->x1 = q.x1; glyph->y1 = q.y1;
                glyph->y0 += (dst_font->ascent + 0.5f);
                glyph->y1 += (dst_font->ascent + 0.5f);
                if (cfg->coord_type == GUI_COORD_PIXEL) {
                    glyph->u0 = q.s0 * (gui_float)width;
                    glyph->v0 = q.t0 * (gui_float)height;
                    glyph->u1 = q.s1 * (gui_float)width;
                    glyph->v1 = q.t1 * (gui_float)height;
                } else {
                    glyph->u0 = q.s0;
                    glyph->v0 = q.t0;
                    glyph->u1 = q.s1;
                    glyph->v1 = q.t1;
                }
                glyph->xadvance = (pc->xadvance + cfg->spacing.x);
                if (cfg->pixel_snap)
                    glyph->xadvance = (gui_float)(gui_int)(glyph->xadvance + 0.5f);
            }
        }
        dst_font->glyph_count = glyph_count;
        glyph_n += dst_font->glyph_count;
    }
}

void
gui_font_bake_custom_data(void *img_memory, gui_size img_width, gui_size img_height,
    struct gui_recti img_dst, const char *texture_data_mask, gui_size tex_width,
    gui_size tex_height, gui_char white, gui_char black)
{
    gui_byte *pixels;
    gui_size y = 0, x = 0, n = 0;
    GUI_ASSERT(img_memory);
    GUI_ASSERT(img_width);
    GUI_ASSERT(img_height);
    GUI_ASSERT(texture_data_mask);
    GUI_UNUSED(tex_height);
    if (!img_memory || !img_width || !img_height || !texture_data_mask)
        return;

    pixels = (gui_byte*)img_memory;
    for (y = 0, n = 0; y < tex_height; ++y) {
        for (x = 0; x < tex_width; ++x, ++n) {
            const gui_size off0 = (img_dst.x + x) + (img_dst.y + y) * img_width;
            const gui_size off1 = off0 + 1 + tex_width;
            pixels[off0] = (texture_data_mask[n] == white) ? 0xFF : 0x00;
            pixels[off1] = (texture_data_mask[n] == black) ? 0xFF : 0x00;
        }
    }
}

void
gui_font_bake_convert(void *out_memory, gui_ushort img_width, gui_ushort img_height,
    const void *in_memory)
{
    gui_int n = 0;
    const gui_byte *src;
    gui_uint *dst;
    GUI_ASSERT(out_memory);
    GUI_ASSERT(in_memory);
    GUI_ASSERT(img_width);
    GUI_ASSERT(img_height);
    if (!out_memory || !in_memory || !img_height || !img_width) return;

    dst = (gui_uint*)out_memory;
    src = (const gui_byte*)in_memory;
    for (n = (gui_int)(img_width * img_height); n > 0; n--)
        *dst++ = ((gui_uint)(*src++) << 24) | 0x00FFFFFF;
}
/*
 * -------------------------------------------------------------
 *
 *                          Font
 *
 * --------------------------------------------------------------
 */
void
gui_font_init(struct gui_font *font, gui_float pixel_height,
    gui_long fallback_codepoint, struct gui_font_glyph *glyphes,
    const struct gui_baked_font *baked_font, gui_handle atlas)
{
    GUI_ASSERT(font);
    GUI_ASSERT(glyphes);
    GUI_ASSERT(baked_font);
    if (!font || !glyphes || !baked_font)
        return;

    gui_zero(font, sizeof(*font));
    font->ascent = baked_font->ascent;
    font->descent = baked_font->descent;
    font->size = baked_font->height;
    font->scale = (gui_float)pixel_height / (gui_float)font->size;
    font->glyphes = &glyphes[baked_font->glyph_offset];
    font->glyph_count = baked_font->glyph_count;
    font->ranges = baked_font->ranges;
    font->atlas = atlas;
    font->fallback_codepoint = fallback_codepoint;
    font->fallback = gui_font_find_glyph(font, fallback_codepoint);
}

const struct gui_font_glyph*
gui_font_find_glyph(struct gui_font *font, gui_long unicode)
{
    gui_size i = 0;
    gui_size count;
    gui_size total_glyphes = 0;
    const struct gui_font_glyph *glyph = 0;
    GUI_ASSERT(font);

    glyph = font->fallback;
    count = gui_range_count(font->ranges);
    for (i = 0; i < count; ++i) {
        gui_size diff;
        gui_long f = font->ranges[(i*2)+0];
        gui_long t = font->ranges[(i*2)+1];
        diff = (gui_size)((t - f) + 1);
        if (unicode >= f && unicode <= t)
            return &font->glyphes[(total_glyphes + (gui_size)(unicode - f))];
        total_glyphes += diff;
    }
    return glyph;
}

static gui_size
gui_font_text_width(gui_handle handle, const gui_char *text, gui_size len)
{
    gui_long unicode;
    const struct gui_font_glyph *glyph;
    struct gui_font *font;
    gui_size text_len  = 0;
    gui_size text_width = 0;
    gui_size glyph_len;
    font = (struct gui_font*)handle.ptr;
    GUI_ASSERT(font);
    if (!font || !text || !len)
        return 0;

    glyph_len = gui_utf_decode(text, &unicode, len);
    while (text_len < len && glyph_len) {
        if (unicode == GUI_UTF_INVALID) return 0;
        glyph = gui_font_find_glyph(font, unicode);
        text_len += glyph_len;
        text_width += (gui_size)((glyph->xadvance * font->scale));
        glyph_len = gui_utf_decode(text + text_len, &unicode, len - text_len);
    }
    return text_width;
}

#if GUI_COMPILE_WITH_VERTEX_BUFFER
static void
gui_font_query_font_glyph(gui_handle handle, struct gui_user_font_glyph *glyph,
        gui_long codepoint, gui_long next_codepoint)
{
    const struct gui_font_glyph *g;
    struct gui_font *font;
    GUI_ASSERT(glyph);
    GUI_UNUSED(next_codepoint);
    font = (struct gui_font*)handle.ptr;
    GUI_ASSERT(font);
    if (!font || !glyph)
        return;

    g = gui_font_find_glyph(font, codepoint);
    glyph->width = (g->x1 - g->x0) * font->scale;
    glyph->height = (g->y1 - g->y0) * font->scale;
    glyph->offset = gui_vec2(g->x0 * font->scale, g->y0 * font->scale);
    glyph->xadvance = (g->xadvance * font->scale);
    glyph->uv[0] = gui_vec2(g->u0, g->v0);
    glyph->uv[1] = gui_vec2(g->u1, g->v1);
}
#endif

struct gui_user_font
gui_font_ref(struct gui_font *font)
{
    struct gui_user_font user_font;
    gui_zero(&user_font, sizeof(user_font));
    user_font.height = font->size * font->scale;
    user_font.width = gui_font_text_width;
    user_font.userdata.ptr = font;
#if GUI_COMPILE_WITH_VERTEX_BUFFER
    user_font.query = gui_font_query_font_glyph;
    user_font.texture = font->atlas;
#endif
    return user_font;
}
#endif
/*
 * ==============================================================
 *
 *                      Edit Box
 *
 * ===============================================================
 */
static void
gui_edit_buffer_append(gui_edit_buffer *buffer, const char *str, gui_size len)
{
    char *mem;
    GUI_ASSERT(buffer);
    GUI_ASSERT(str);
    if (!buffer || !str || !len) return;
    mem = (char*)gui_buffer_alloc(buffer, GUI_BUFFER_FRONT, len * sizeof(char), 0);
    if (!mem) return;
    gui_memcopy(mem, str, len * sizeof(char));
}

static int
gui_edit_buffer_insert(gui_edit_buffer *buffer, gui_size pos,
    const char *str, gui_size len)
{
    void *mem;
    gui_size i;
    char *src, *dst;

    gui_size copylen;
    GUI_ASSERT(buffer);
    if (!buffer || !str || !len || pos > buffer->allocated) return 0;

    copylen = buffer->allocated - pos;
    if (!copylen) {
        gui_edit_buffer_append(buffer, str, len);
        return 1;
    }
    mem = gui_buffer_alloc(buffer, GUI_BUFFER_FRONT, len * sizeof(char), 0);
    if (!mem) return 0;

    /* memmove */
    GUI_ASSERT(((int)pos + (int)len + ((int)copylen - 1)) >= 0);
    GUI_ASSERT(((int)pos + ((int)copylen - 1)) >= 0);
    dst = gui_ptr_add(char, buffer->memory.ptr, pos + len + (copylen - 1));
    src = gui_ptr_add(char, buffer->memory.ptr, pos + (copylen-1));
    for (i = 0; i < copylen; ++i) *dst-- = *src--;
    mem = gui_ptr_add(void, buffer->memory.ptr, pos);
    gui_memcopy(mem, str, len * sizeof(char));
    return 1;
}

static void
gui_edit_buffer_remove(gui_edit_buffer *buffer, gui_size len)
{
    GUI_ASSERT(buffer);
    if (!buffer || len > buffer->allocated) return;
    GUI_ASSERT(((int)buffer->allocated - (int)len) >= 0);
    buffer->allocated -= len;
}

static void
gui_edit_buffer_del(gui_edit_buffer *buffer, gui_size pos, gui_size len)
{
    char *src, *dst;
    GUI_ASSERT(buffer);
    if (!buffer || !len || pos > buffer->allocated ||
        pos + len > buffer->allocated) return;

    if (pos + len < buffer->allocated) {
        /* memmove */
        dst = gui_ptr_add(char, buffer->memory.ptr, pos);
        src = gui_ptr_add(char, buffer->memory.ptr, pos + len);
        gui_memcopy(dst, src, buffer->allocated - (pos + len));
        GUI_ASSERT(((int)buffer->allocated - (int)len) >= 0);
        buffer->allocated -= len;
    } else gui_edit_buffer_remove(buffer, len);
}

static char*
gui_edit_buffer_at_char(gui_edit_buffer *buffer, gui_size pos)
{
    GUI_ASSERT(buffer);
    if (!buffer || pos > buffer->allocated) return 0;
    return gui_ptr_add(char, buffer->memory.ptr, pos);
}

static char*
gui_edit_buffer_at(gui_edit_buffer *buffer, gui_int pos, gui_long *unicode,
    gui_size *len)
{
    gui_int i = 0;
    gui_size src_len = 0;
    gui_size glyph_len = 0;
    gui_char *text;
    gui_size text_len;

    GUI_ASSERT(buffer);
    GUI_ASSERT(unicode);
    GUI_ASSERT(len);
    if (!buffer || !unicode || !len) return 0;
    if (pos < 0) {
        *unicode = 0;
        *len = 0;
        return 0;
    }

    text = (gui_char*)buffer->memory.ptr;
    text_len = buffer->allocated;
    glyph_len = gui_utf_decode(text, unicode, text_len);
    while (glyph_len) {
        if (i == pos) {
            *len = glyph_len;
            break;
        }

        i++;
        src_len = src_len + glyph_len;
        glyph_len = gui_utf_decode(text + src_len, unicode, text_len - src_len);
    }
    if (i != pos) return 0;
    return text + src_len;
}

void
gui_edit_box_init(struct gui_edit_box *eb, struct gui_allocator *a,
    gui_size initial_size, gui_float grow_fac, const struct gui_clipboard *clip,
    gui_filter f)
{
    GUI_ASSERT(eb);
    GUI_ASSERT(a);
    GUI_ASSERT(initial_size);
    if (!eb || !a) return;

    gui_zero(eb, sizeof(*eb));
    gui_buffer_init(&eb->buffer, a, initial_size, grow_fac);
    if (clip) eb->clip = *clip;
    if (f) eb->filter = f;
    else eb->filter = gui_filter_default;
    eb->cursor = 0;
    eb->glyphes = 0;
}

void
gui_edit_box_init_fixed(struct gui_edit_box *eb, void *memory, gui_size size,
                        const struct gui_clipboard *clip, gui_filter f)
{
    GUI_ASSERT(eb);
    if (!eb) return;
    gui_zero(eb, sizeof(*eb));
    gui_buffer_init_fixed(&eb->buffer, memory, size);
    if (clip) eb->clip = *clip;
    if (f) eb->filter = f;
    else eb->filter = gui_filter_default;
    eb->cursor = 0;
    eb->glyphes = 0;
}

void
gui_edit_box_clear(struct gui_edit_box *box)
{
    GUI_ASSERT(box);
    if (!box) return;
    gui_buffer_clear(&box->buffer);
    box->cursor = box->glyphes = 0;
}

void
gui_edit_box_free(struct gui_edit_box *box)
{
    GUI_ASSERT(box);
    if (!box) return;
    gui_buffer_free(&box->buffer);
    box->cursor = box->glyphes = 0;
}

void
gui_edit_box_info(struct gui_memory_status *status, struct gui_edit_box *box)
{
    GUI_ASSERT(box);
    GUI_ASSERT(status);
    if (!box || !status) return;
    gui_buffer_info(status, &box->buffer);
}

void
gui_edit_box_add(struct gui_edit_box *eb, const char *str, gui_size len)
{
    gui_int res = 0;
    GUI_ASSERT(eb);
    if (!eb || !str || !len) return;
    if (eb->cursor == eb->glyphes) {
        res = gui_edit_buffer_insert(&eb->buffer, eb->buffer.allocated, str, len);
    } else {
        gui_size l = 0;
        gui_long unicode;
        gui_int cursor = (eb->cursor) ? (gui_int)(eb->cursor) : 0;
        gui_char *sym = gui_edit_buffer_at(&eb->buffer, cursor, &unicode, &l);
        gui_size offset = (gui_size)(sym - (gui_char*)eb->buffer.memory.ptr);
        res = gui_edit_buffer_insert(&eb->buffer, offset, str, len);
    }
    if (res) {
        gui_size l = gui_utf_len(str, len);
        eb->cursor += l;
        eb->glyphes += l;
    }
}

static gui_size
gui_edit_box_buffer_input(struct gui_edit_box *box, const struct gui_input *i)
{
    gui_long unicode;
    gui_size src_len = 0;
    gui_size text_len = 0, glyph_len = 0;
    gui_size glyphes = 0;

    GUI_ASSERT(box);
    GUI_ASSERT(i);
    if (!box || !i) return 0;

    /* add user provided text to buffer until either no input or buffer space left*/
    glyph_len = gui_utf_decode(i->keyboard.text, &unicode, i->keyboard.text_len);
    while (glyph_len && ((text_len+glyph_len) <= i->keyboard.text_len)) {
        /* filter to make sure the value is correct */
        if (box->filter(unicode)) {
            gui_edit_box_add(box, &i->keyboard.text[text_len], glyph_len);
            text_len += glyph_len;
            glyphes++;
        }
        src_len = src_len + glyph_len;
        glyph_len = gui_utf_decode(i->keyboard.text + src_len, &unicode,
            i->keyboard.text_len - src_len);
    }
    return glyphes;
}

void
gui_edit_box_remove(struct gui_edit_box *box)
{
    gui_size len;
    gui_char *buf;
    gui_size min, maxi, diff;
    GUI_ASSERT(box);
    if (!box) return;
    if (!box->glyphes) return;

    buf = (gui_char*)box->buffer.memory.ptr;
    min = MIN(box->sel.end, box->sel.begin);
    maxi = MAX(box->sel.end, box->sel.begin);
    diff = maxi - min;

    if (diff && box->cursor != box->glyphes) {
        gui_size off;
        gui_long unicode;
        gui_char *begin, *end;

        /* delete text selection */
        begin = gui_edit_buffer_at(&box->buffer, (gui_int)min, &unicode, &len);
        end = gui_edit_buffer_at(&box->buffer, (gui_int)maxi, &unicode, &len);
        len = (gui_size)(end - begin);
        off = (gui_size)(begin - buf);
        gui_edit_buffer_del(&box->buffer, off, len);
        box->glyphes = gui_utf_len(buf, box->buffer.allocated);
    } else {
        gui_long unicode;
        gui_int cursor;
        gui_char *glyph;
        gui_size offset;

        /* remove last glyph */
        cursor = (gui_int)box->cursor - 1;
        glyph = gui_edit_buffer_at(&box->buffer, cursor, &unicode, &len);
        if (!glyph || !len) return;
        offset = (gui_size)(glyph - (gui_char*)box->buffer.memory.ptr);
        gui_edit_buffer_del(&box->buffer, offset, len);
        box->glyphes--;
    }
    if (box->cursor >= box->glyphes) box->cursor = box->glyphes;
    else if (box->cursor > 0) box->cursor--;
}

gui_char*
gui_edit_box_get(struct gui_edit_box *eb)
{
    GUI_ASSERT(eb);
    if (!eb) return 0;
    return (gui_char*)eb->buffer.memory.ptr;
}

const gui_char*
gui_edit_box_get_const(struct gui_edit_box *eb)
{
    GUI_ASSERT(eb);
    if (!eb) return 0;
    return (gui_char*)eb->buffer.memory.ptr;
}

gui_char
gui_edit_box_at_char(struct gui_edit_box *eb, gui_size pos)
{
    gui_char *c;
    GUI_ASSERT(eb);
    if (!eb || pos >= eb->buffer.allocated) return 0;
    c = gui_edit_buffer_at_char(&eb->buffer, pos);
    return c ? *c : 0;
}

void
gui_edit_box_at(struct gui_edit_box *eb, gui_size pos, gui_glyph g, gui_size *len)
{
    gui_char *sym;
    gui_long unicode;

    GUI_ASSERT(eb);
    GUI_ASSERT(g);
    GUI_ASSERT(len);

    if (!eb || !len || !g) return;
    if (pos >= eb->glyphes) {
        *len = 0;
        return;
    }
    sym = gui_edit_buffer_at(&eb->buffer, (gui_int)pos, &unicode, len);
    if (!sym) return;
    gui_memcopy(g, sym, *len);
}

void
gui_edit_box_at_cursor(struct gui_edit_box *eb, gui_glyph g, gui_size *len)
{
    gui_size cursor = 0;
    GUI_ASSERT(eb);
    GUI_ASSERT(g);
    GUI_ASSERT(len);

    if (!eb || !g || !len) return;
    if (!eb->cursor) {
        *len = 0;
        return;
    }
    cursor = (eb->cursor) ? eb->cursor-1 : 0;
    gui_edit_box_at(eb, cursor, g, len);
}

void
gui_edit_box_set_cursor(struct gui_edit_box *eb, gui_size pos)
{
    GUI_ASSERT(eb);
    GUI_ASSERT(eb->glyphes >= pos);
    if (!eb || pos > eb->glyphes) return;
    eb->cursor = pos;
}

gui_size
gui_edit_box_get_cursor(struct gui_edit_box *eb)
{
    GUI_ASSERT(eb);
    if (!eb) return 0;
    return eb->cursor;
}

gui_size
gui_edit_box_len_char(struct gui_edit_box *eb)
{
    GUI_ASSERT(eb);
    return eb->buffer.allocated;
}

gui_size
gui_edit_box_len(struct gui_edit_box *eb)
{
    GUI_ASSERT(eb);
    return eb->glyphes;
}
/*
 * ==============================================================
 *
 *                          Widgets
 *
 * ===============================================================
 */
void
gui_widget_text(struct gui_command_buffer *o, struct gui_rect b,
    const char *string, gui_size len, const struct gui_text *t,
    enum gui_text_align a, const struct gui_user_font *f)
{
    struct gui_rect label;
    gui_size text_width;

    GUI_ASSERT(o);
    GUI_ASSERT(t);
    if (!o || !t) return;

    b.h = MAX(b.h, 2 * t->padding.y);
    label.x = 0; label.w = 0;
    label.y = b.y + t->padding.y;
    label.h = MAX(0, b.h - 2 * t->padding.y);
    text_width = f->width(f->userdata, (const gui_char*)string, len);
    b.w = MAX(b.w, (2 * t->padding.x + (gui_float)text_width));

    if (a == GUI_TEXT_LEFT) {
        label.x = b.x + t->padding.x;
        label.w = MAX(0, b.w - 2 * t->padding.x);
    } else if (a == GUI_TEXT_CENTERED) {
        label.w = MAX(1, 2 * t->padding.x + (gui_float)text_width);
        label.x = (b.x + t->padding.x + ((b.w - 2 * t->padding.x)/2));
        if (label.x > label.w/2) label.x -= (label.w/2);
        label.x = MAX(b.x + t->padding.x, label.x);
        label.w = MIN(b.x + b.w, label.x + label.w);
        if (label.w > label.x) label.w -= label.x;
    } else if (a == GUI_TEXT_RIGHT) {
        label.x = (b.x + b.w) - (2 * t->padding.x + (gui_float)text_width);
        label.w = (gui_float)text_width + 2 * t->padding.x;
    } else return;
    gui_command_buffer_push_text(o, label, (const gui_char*)string,
        len, f, t->background, t->text);
}

static gui_bool
gui_widget_do_button(struct gui_command_buffer *o, struct gui_rect r,
    const struct gui_button *b, const struct gui_input *i,
    enum gui_button_behavior behavior, struct gui_rect *content)
{
    gui_bool ret = gui_false;
    struct gui_color background;
    struct gui_vec2 pad;
    GUI_ASSERT(b);
    if (!o || !b)
        return gui_false;

    /* calculate button content space */
    pad.x = b->padding.x + b->border_width;
    pad.y = b->padding.y + b->border_width;
    *content = gui_pad_rect(r, pad);

    /* general button user input logic */
    background = b->normal;
    if (gui_input_is_mouse_hovering_rect(i, r)) {
        background = b->hover;
        if (gui_input_is_mouse_down(i, GUI_BUTTON_LEFT))
            background = b->active;
        if (gui_input_has_mouse_click_in_rect(i, GUI_BUTTON_LEFT, r)) {
            ret = (behavior != GUI_BUTTON_DEFAULT) ?
                gui_input_is_mouse_down(i, GUI_BUTTON_LEFT):
                gui_input_is_mouse_pressed(i, GUI_BUTTON_LEFT);
        }
    }
    gui_command_buffer_push_rect(o, r, b->rounding, b->border);
    gui_command_buffer_push_rect(o, gui_shrink_rect(r, b->border_width),
                                    b->rounding, background);
    return ret;
}

gui_bool
gui_widget_button_text(struct gui_command_buffer *o, struct gui_rect r,
     const char *string, enum gui_button_behavior behavior,
    const struct gui_button_text *b, const struct gui_input *i,
    const struct gui_user_font *f)
{
    struct gui_text t;
    struct gui_rect content;
    gui_bool ret = gui_false;

    GUI_ASSERT(b);
    GUI_ASSERT(o);
    GUI_ASSERT(string);
    GUI_ASSERT(f);
    if (!o || !b || !f)
        return gui_false;

    t.text = b->normal;
    t.background = b->base.normal;
    t.padding = gui_vec2(0,0);
    ret = gui_widget_do_button(o, r, &b->base, i, behavior, &content);
    if (gui_input_is_mouse_hovering_rect(i, r)) {
        gui_bool is_down = gui_input_is_mouse_down(i, GUI_BUTTON_LEFT);
        t.background =  (is_down) ? b->base.active: b->base.hover;
        t.text = (is_down) ? b->active : b->hover;
    }
    gui_widget_text(o, content, string, gui_strsiz(string), &t, b->alignment, f);
    return ret;
}

static void
gui_draw_symbol(struct gui_command_buffer *out, enum gui_symbol symbol,
    struct gui_rect content, struct gui_color background, struct gui_color foreground,
    gui_float border_width, const struct gui_user_font *font)
{
    switch (symbol) {
    case GUI_SYMBOL_X:
    case GUI_SYMBOL_UNDERSCORE:
    case GUI_SYMBOL_PLUS:
    case GUI_SYMBOL_MINUS: {
        /* single character text symbol */
        const gui_char *X = (symbol == GUI_SYMBOL_X) ? "x":
            (symbol == GUI_SYMBOL_UNDERSCORE) ? "_":
            (symbol == GUI_SYMBOL_PLUS) ? "+": "-";
        struct gui_text text;
        text.padding = gui_vec2(0,0);
        text.background = background;
        text.text = foreground;
        gui_widget_text(out, content, X, 1, &text, GUI_TEXT_CENTERED, font);
    } break;
    case GUI_SYMBOL_CIRCLE:
    case GUI_SYMBOL_CIRCLE_FILLED:
    case GUI_SYMBOL_RECT:
    case GUI_SYMBOL_RECT_FILLED: {
        /* simple empty/filled shapes */
        if (symbol == GUI_SYMBOL_RECT || symbol == GUI_SYMBOL_RECT_FILLED) {
            gui_command_buffer_push_rect(out, content,  0, foreground);
            if (symbol == GUI_SYMBOL_RECT_FILLED)
                gui_command_buffer_push_rect(out, gui_shrink_rect(content,
                    border_width), 0, background);
        } else {
            gui_command_buffer_push_circle(out, content, foreground);
            if (symbol == GUI_SYMBOL_CIRCLE_FILLED)
                gui_command_buffer_push_circle(out, gui_shrink_rect(content, 1),
                    background);
        }
    } break;
    case GUI_SYMBOL_TRIANGLE_UP:
    case GUI_SYMBOL_TRIANGLE_DOWN:
    case GUI_SYMBOL_TRIANGLE_LEFT:
    case GUI_SYMBOL_TRIANGLE_RIGHT: {
        enum gui_heading heading;
        struct gui_vec2 points[3];
        heading = (symbol == GUI_SYMBOL_TRIANGLE_RIGHT) ? GUI_RIGHT :
            (symbol == GUI_SYMBOL_TRIANGLE_LEFT) ? GUI_LEFT:
            (symbol == GUI_SYMBOL_TRIANGLE_UP) ? GUI_UP: GUI_DOWN;
        gui_triangle_from_direction(points, content, 0, 0, heading);
        gui_command_buffer_push_triangle(out,  points[0].x, points[0].y,
            points[1].x, points[1].y, points[2].x, points[2].y, foreground);
    } break;
    case GUI_SYMBOL_MAX:
    default: break;
    }
}

gui_bool
gui_widget_button_symbol(struct gui_command_buffer *out, struct gui_rect r,
    enum gui_symbol symbol, enum gui_button_behavior bh,
    const struct gui_button_symbol *b, const struct gui_input *in,
    const struct gui_user_font *font)
{
    gui_bool ret;
    struct gui_color background;
    struct gui_color color;
    struct gui_rect content;

    GUI_ASSERT(b);
    GUI_ASSERT(out);
    if (!out || !b)
        return gui_false;

    ret = gui_widget_do_button(out, r, &b->base, in, bh, &content);
    if (gui_input_is_mouse_hovering_rect(in, r)) {
        gui_bool is_down = gui_input_is_mouse_down(in, GUI_BUTTON_LEFT);
        background = (is_down) ? b->base.active : b->base.hover;
        color = (is_down) ? b->active : b->hover;
    } else {
        background = b->base.normal;
        color = b->normal;
    }
    gui_draw_symbol(out, symbol, content, background, color, b->base.border_width, font);
    return ret;
}

gui_bool
gui_widget_button_image(struct gui_command_buffer *out, struct gui_rect r,
    struct gui_image img, enum gui_button_behavior b,
    const struct gui_button_icon *button, const struct gui_input *in)
{
    gui_bool pressed;
    struct gui_rect bounds;
    GUI_ASSERT(button);
    GUI_ASSERT(out);
    if (!out || !button)
        return gui_false;

    pressed = gui_widget_do_button(out, r, &button->base, in, b, &bounds);
    gui_command_buffer_push_image(out, bounds, &img);
    return pressed;
}

gui_bool
gui_widget_button_text_symbol(struct gui_command_buffer *out, struct gui_rect r,
    enum gui_symbol symbol, const char *text, enum gui_text_align align,
    enum gui_button_behavior behavior, const struct gui_button_text *button,
    const struct gui_user_font *f, const struct gui_input *i)
{
    gui_bool ret;
    struct gui_rect tri;
    struct gui_color background;
    struct gui_color color;

    GUI_ASSERT(button);
    GUI_ASSERT(out);
    if (!out || !button)
        return gui_false;

    ret = gui_widget_button_text(out, r, text, behavior, button, i, f);
    if (gui_input_is_mouse_hovering_rect(i, r)) {
        gui_bool is_down = gui_input_is_mouse_down(i, GUI_BUTTON_LEFT);
        background = (is_down) ? button->base.active : button->base.hover;
        color = (is_down) ? button->active : button->hover;
    } else {
        background = button->base.normal;
        color = button->normal;
    }

    /* calculate symbol bounds */
    tri.y = r.y + (r.h/2) - f->height/2;
    tri.w = tri.h = f->height;
    if (align == GUI_TEXT_LEFT) {
        tri.x = (r.x + r.w) - (2 * button->base.padding.x + tri.w);
        tri.x = MAX(tri.x, 0);
    } else tri.x = r.x + 2 * button->base.padding.x;
    gui_draw_symbol(out, symbol, tri, background, color, button->base.border_width, f);
    return ret;
}

gui_bool
gui_widget_button_text_image(struct gui_command_buffer *out, struct gui_rect r,
    struct gui_image img, const char* text, enum gui_text_align align,
    enum gui_button_behavior behavior, const struct gui_button_text *button,
    const struct gui_user_font *f, const struct gui_input *i)
{
    gui_bool pressed;
    struct gui_rect icon;
    GUI_ASSERT(button);
    GUI_ASSERT(out);
    if (!out || !button)
        return gui_false;

    pressed = gui_widget_button_text(out, r, text, behavior, button, i, f);
    icon.y = r.y + button->base.padding.y;
    icon.w = icon.h = r.h - 2 * button->base.padding.y;
    if (align == GUI_TEXT_LEFT) {
        icon.x = (r.x + r.w) - (2 * button->base.padding.x + icon.w);
        icon.x = MAX(icon.x, 0);
    } else icon.x = r.x + 2 * button->base.padding.x;
    gui_command_buffer_push_image(out, icon, &img);
    return pressed;
}

void
gui_widget_toggle(struct gui_command_buffer *out, struct gui_rect r,
    gui_bool *active, const char *string, enum gui_toggle_type type,
    const struct gui_toggle *toggle, const struct gui_input *in,
    const struct gui_user_font *font)
{
    gui_bool toggle_active;
    struct gui_rect select;
    struct gui_rect cursor;
    gui_float cursor_pad;
    struct gui_color col;

    GUI_ASSERT(toggle);
    GUI_ASSERT(out);
    GUI_ASSERT(font);
    if (!out || !toggle || !font || !active)
        return;

    /* make sure correct values */
    r.w = MAX(r.w, font->height + 2 * toggle->padding.x);
    r.h = MAX(r.h, font->height + 2 * toggle->padding.y);
    toggle_active = *active;

    /* calculate the size of the complete toggle */
    select.w = MAX(font->height + 2 * toggle->padding.y, 1);
    select.h = select.w;
    select.x = r.x + toggle->padding.x;
    select.y = (r.y + toggle->padding.y + (select.w / 2)) - (font->height / 2);

    /* calculate the bounds of the cursor inside the toggle */
    cursor_pad = (type == GUI_TOGGLE_OPTION) ?
        (gui_float)(gui_int)(select.w / 4):
        (gui_float)(gui_int)(select.h / 8);

    select.h = MAX(select.w, cursor_pad * 2);
    cursor.h = select.w - cursor_pad * 2;
    cursor.w = cursor.h;
    cursor.x = select.x + cursor_pad;
    cursor.y = select.y + cursor_pad;

    /* update toggle state with user input */
    toggle_active = gui_input_mouse_clicked(in, GUI_BUTTON_LEFT, cursor) ?
        !toggle_active : toggle_active;
    if (in && gui_input_is_mouse_hovering_rect(in, cursor))
        col = toggle->hover;
    else col = toggle->normal;

    /* draw radiobutton/checkbox background */
    if (type == GUI_TOGGLE_CHECK)
        gui_command_buffer_push_rect(out, select , toggle->rounding, col);
    else gui_command_buffer_push_circle(out, select, col);

    /* draw radiobutton/checkbox cursor if active */
    if (toggle_active) {
        if (type == GUI_TOGGLE_CHECK)
            gui_command_buffer_push_rect(out, cursor, toggle->rounding, toggle->cursor);
        else gui_command_buffer_push_circle(out, cursor, toggle->cursor);
    }

    /* draw toggle text */
    if (font && string) {
        struct gui_text text;
        struct gui_rect inner;

        /* calculate text bounds */
        inner.x = r.x + select.w + toggle->padding.x * 2;
        inner.y = select.y;
        inner.w = MAX(r.x + r.w, inner.x + toggle->padding.x);
        inner.w -= (inner.x + toggle->padding.x);
        inner.h = select.w;

        /* draw text */
        text.padding.x = 0;
        text.padding.y = 0;
        text.background = toggle->cursor;
        text.text = toggle->font;
        gui_widget_text(out, inner, string, gui_strsiz(string), &text, GUI_TEXT_LEFT, font);
    }
    *active = toggle_active;
}

gui_float
gui_widget_slider(struct gui_command_buffer *out, struct gui_rect slider,
    gui_float min, gui_float val, gui_float max, gui_float step,
    const struct gui_slider *s, const struct gui_input *in)
{
    gui_float slider_range;
    gui_float slider_min, slider_max;
    gui_float slider_value, slider_steps;
    gui_float cursor_offset;
    struct gui_rect cursor;
    struct gui_rect bar;
    struct gui_color col;
    gui_bool inslider;
    gui_bool incursor;

    GUI_ASSERT(s);
    GUI_ASSERT(out);
    if (!out || !s)
        return 0;

    /* make sure the provided values are correct */
    slider.x = slider.x + s->padding.x;
    slider.y = slider.y + s->padding.y;
    slider.h = MAX(slider.h, 2 * s->padding.y);
    slider.w = MAX(slider.w, 1 + slider.h + 2 * s->padding.x);
    slider.h -= 2 * s->padding.y;
    slider.w -= 2 * s->padding.y;
    slider_max = MAX(min, max);
    slider_min = MIN(min, max);
    slider_value = CLAMP(slider_min, val, slider_max);
    slider_range = slider_max - slider_min;
    slider_steps = slider_range / step;

    /* calculate slider virtual cursor bounds */
    cursor_offset = (slider_value - slider_min) / step;
    cursor.h = slider.h;
    cursor.w = slider.w / (slider_steps + 1);
    cursor.x = slider.x + (cursor.w * cursor_offset);
    cursor.y = slider.y;

    /* calculate slider background bar bounds */
    bar.x = slider.x;
    bar.y = (slider.y + cursor.h/2) - cursor.h/8;
    bar.w = slider.w;
    bar.h = slider.h/4;

    /* updated the slider value by user input */
    inslider = in && gui_input_is_mouse_hovering_rect(in, slider);
    incursor = in && gui_input_has_mouse_click_down_in_rect(in, GUI_BUTTON_LEFT, slider, gui_true);
    col = (inslider) ? s->hover: s->normal;

    if (in && inslider && incursor)
    {
        const float d = in->mouse.pos.x - (cursor.x + cursor.w / 2.0f);
        const float pxstep = (slider.w - (2 * s->padding.x)) / slider_steps;
        /* only update value if the next slider step is reached*/
        col = s->active;
        if (GUI_ABS(d) >= pxstep) {
            const gui_float steps = (gui_float)((gui_int)(GUI_ABS(d) / pxstep));
            slider_value += (d > 0) ? (step * steps) : -(step * steps);
            slider_value = CLAMP(slider_min, slider_value, slider_max);
            cursor.x = slider.x + (cursor.w * ((slider_value - slider_min) / step));
        }
    }

    {
        struct gui_rect fill;
        cursor.w = cursor.h;
        cursor.x = (slider_value <= slider_min) ? cursor.x:
            (slider_value >= slider_max) ? ((bar.x + bar.w) - cursor.w) :
            cursor.x - (cursor.w/2);

        fill.x = bar.x;
        fill.y = bar.y;
        fill.w = (cursor.x + (cursor.w/2.0f)) - bar.x;
        fill.h = bar.h;

        /* draw slider with background and circle cursor*/
        gui_command_buffer_push_rect(out, bar, 0, s->bg);
        gui_command_buffer_push_rect(out, fill, 0, col);
        gui_command_buffer_push_circle(out, cursor, col);
    }
    return slider_value;
}

gui_size
gui_widget_progress(struct gui_command_buffer *out, struct gui_rect r,
    gui_size value, gui_size max, gui_bool modifyable,
    const struct gui_progress *prog, const struct gui_input *in)
{
    gui_float prog_scale;
    gui_size prog_value;
    struct gui_color col;

    GUI_ASSERT(prog);
    GUI_ASSERT(out);
    if (!out || !prog) return 0;

    /* make sure given values are correct */
    r.w = MAX(r.w, 2 * prog->padding.x + 5);
    r.h = MAX(r.h, 2 * prog->padding.y + 5);
    r = gui_pad_rect(r, gui_vec2(prog->padding.x, prog->padding.y));
    prog_value = MIN(value, max);

    /* update progress by user input if modifyable */
    if (in && modifyable && gui_input_is_mouse_hovering_rect(in, r)) {
        if (gui_input_is_mouse_down(in, GUI_BUTTON_LEFT)) {
            gui_float ratio = (gui_float)(in->mouse.pos.x - r.x) / (gui_float)r.w;
            prog_value = (gui_size)((gui_float)max * ratio);
            col = prog->active;
        } else col = prog->hover;
    } else col = prog->normal;

    /* make sure calculated values are correct */
    if (!max) return prog_value;
    prog_value = MIN(prog_value, max);
    prog_scale = (gui_float)prog_value / (gui_float)max;

    /* draw progressbar with background and cursor */
    gui_command_buffer_push_rect(out, r, prog->rounding, prog->background);
    r.w = (r.w - 2) * prog_scale;
    gui_command_buffer_push_rect(out, r, prog->rounding, col);
    return prog_value;
}

static gui_size
gui_user_font_glyph_index_at_pos(const struct gui_user_font *font, const char *text,
    gui_size text_len, gui_float xoff)
{
    gui_long unicode;
    gui_size glyph_offset = 0;
    gui_size glyph_len = gui_utf_decode(text, &unicode, text_len);
    gui_size text_width = font->width(font->userdata, text, glyph_len);
    gui_size src_len = glyph_len;

    while (text_len && glyph_len) {
        if (text_width >= xoff)
            return glyph_offset;
        glyph_offset++;
        text_len -= glyph_len;
        glyph_len = gui_utf_decode(text + src_len, &unicode, text_len);
        src_len += glyph_len;
        text_width = font->width(font->userdata, text, src_len);
    }
    return glyph_offset;
}

static gui_size
gui_user_font_glyphes_fitting_in_space(const struct gui_user_font *font, const char *text,
    gui_size text_len, gui_float space, gui_size *glyphes, gui_float *text_width)
{
    gui_size glyph_len;
    gui_float width = 0;
    gui_float last_width = 0;
    gui_long unicode;
    gui_size offset = 0;
    gui_size g = 0;
    gui_size s;

    glyph_len = gui_utf_decode(text, &unicode, text_len);
    s = font->width(font->userdata, text, glyph_len);
    width = last_width = (gui_float)s;
    while ((width < space) && text_len) {
        g++;
        offset += glyph_len;
        text_len -= glyph_len;
        last_width = width;
        glyph_len = gui_utf_decode(&text[offset], &unicode, text_len);
        s = font->width(font->userdata, &text[offset], glyph_len);
        width += (gui_float)s;
    }

    *glyphes = g;
    *text_width = last_width;
    return offset;
}

void
gui_widget_editbox(struct gui_command_buffer *out, struct gui_rect r,
    struct gui_edit_box *box, const struct gui_edit *field,
    const struct gui_input *in, const struct gui_user_font *font)
{
    gui_char *buffer;
    gui_size len;
    GUI_ASSERT(out);
    GUI_ASSERT(font);
    GUI_ASSERT(field);
    if (!out || !box || !field)
        return;

    r.w = MAX(r.w, 2 * field->padding.x + 2 * field->border_size);
    r.h = MAX(r.h, font->height + (2 * field->padding.y + 2 * field->border_size));
    len = gui_edit_box_len(box);
    buffer = gui_edit_box_get(box);

    /* draw editbox background and border */
    gui_command_buffer_push_rect(out, r, field->rounding, field->border);
    gui_command_buffer_push_rect(out, gui_shrink_rect(r, field->border_size),
        field->rounding, field->background);

    /* check if the editbox is activated/deactivated */
    if (in && in->mouse.buttons[GUI_BUTTON_LEFT].clicked &&
        in->mouse.buttons[GUI_BUTTON_LEFT].down)
        box->active = GUI_INBOX(in->mouse.pos.x,in->mouse.pos.y,r.x,r.y,r.w,r.h);

    /* input handling */
    if (box->active && in) {
        gui_size min = MIN(box->sel.end, box->sel.begin);
        gui_size maxi = MAX(box->sel.end, box->sel.begin);
        gui_size diff = maxi - min;

        /* text manipulation */
        if (gui_input_is_key_pressed(in,GUI_KEY_DEL) ||
            gui_input_is_key_pressed(in,GUI_KEY_BACKSPACE))
            gui_edit_box_remove(box);
        if (gui_input_is_key_pressed(in, GUI_KEY_ENTER))
            box->active = gui_false;
        if (gui_input_is_key_pressed(in, GUI_KEY_SPACE)) {
            if (diff && box->cursor != box->glyphes)
                gui_edit_box_remove(box);
            gui_edit_box_add(box, " ", 1);
        }
        if (in->keyboard.text_len) {
            if (diff && box->cursor != box->glyphes) {
                /* replace text selection */
                gui_edit_box_remove(box);
                box->cursor = min;
                gui_edit_box_buffer_input(box, in);
                box->sel.begin = box->cursor;
                box->sel.end = box->cursor;
            } else{
                /* append text into the buffer */
                gui_edit_box_buffer_input(box, in);
                box->sel.begin = box->cursor;
                box->sel.end = box->cursor;
            }
       }

        /* cursor key movement */
        if (gui_input_is_key_pressed(in, GUI_KEY_LEFT)) {
            box->cursor = (gui_size)MAX(0, (gui_int)box->cursor-1);
            box->sel.begin = box->cursor;
            box->sel.end = box->cursor;
        }
        if (gui_input_is_key_pressed(in, GUI_KEY_RIGHT)) {
            box->cursor = MIN((!box->glyphes) ? 0 : box->glyphes, box->cursor+1);
            box->sel.begin = box->cursor;
            box->sel.end = box->cursor;
        }

        /* copy & cut & paste functionatlity */
        if (gui_input_is_key_pressed(in, GUI_KEY_PASTE) && box->clip.paste)
            box->clip.paste(box->clip.userdata, box);
        if ((gui_input_is_key_pressed(in, GUI_KEY_COPY) && box->clip.copy) ||
            (gui_input_is_key_pressed(in, GUI_KEY_CUT) && box->clip.copy)) {
            if (diff && box->cursor != box->glyphes) {
                /* copy or cut text selection */
                gui_size l;
                gui_long unicode;
                gui_char *begin, *end;
                begin = gui_edit_buffer_at(&box->buffer, (gui_int)min, &unicode, &l);
                end = gui_edit_buffer_at(&box->buffer, (gui_int)maxi, &unicode, &l);
                box->clip.copy(box->clip.userdata, begin, (gui_size)(end - begin));
                if (gui_input_is_key_pressed(in, GUI_KEY_CUT))
                    gui_edit_box_remove(box);
            } else {
                /* copy or cut complete buffer */
                box->clip.copy(box->clip.userdata, buffer, len);
                if (gui_input_is_key_pressed(in, GUI_KEY_CUT))
                    gui_edit_box_clear(box);
            }

        }
    }
    {
        /* text management */
        struct gui_rect label;
        gui_size  cursor_w = font->width(font->userdata,(const gui_char*)"X", 1);
        gui_size text_len = len;
        gui_size glyph_off = 0;
        gui_size glyph_cnt = 0;
        gui_size offset = 0;
        gui_float text_width = 0;

        /* calculate text frame */
        label.w = MAX(r.w,  - 2 * field->padding.x - 2 * field->border_size);
        label.w -= 2 * field->padding.x - 2 * field->border_size;
        {
            gui_size frames = 0;
            gui_size glyphes = 0;
            gui_size frame_len = 0;
            gui_float space = MAX(label.w, (gui_float)cursor_w);
            space -= (gui_float)cursor_w;
            while (text_len) {
                frames++;
                offset += frame_len;
                frame_len = gui_user_font_glyphes_fitting_in_space(font,
                    &buffer[offset], text_len, space, &glyphes, &text_width);
                glyph_off += glyphes;
                if (glyph_off > box->cursor)
                    break;
                text_len -= frame_len;
            }
            text_len = frame_len;
            glyph_cnt = glyphes;
            glyph_off = (frames <= 1) ? 0 : (glyph_off - glyphes);
            offset = (frames <= 1) ? 0 : offset;
        }

        /* set cursor by mouse click and handle text selection */
        if (in && field->show_cursor && in->mouse.buttons[GUI_BUTTON_LEFT].down && box->active) {
            const gui_char *visible = &buffer[offset];
            gui_float xoff = in->mouse.pos.x-(r.x+field->padding.x+field->border_size);
            if (GUI_INBOX(in->mouse.pos.x, in->mouse.pos.y, r.x, r.y, r.w, r.h))
            {
                /* text selection in the current text frame */
                gui_size glyph_index;
                gui_size glyph_pos=gui_user_font_glyph_index_at_pos(font,visible,text_len,xoff);
                if (glyph_cnt + glyph_off >= box->glyphes)
                    glyph_index = glyph_off + MIN(glyph_pos, glyph_cnt);
                else glyph_index = glyph_off + MIN(glyph_pos, glyph_cnt-1);

                if (text_len)
                    gui_edit_box_set_cursor(box, glyph_index);
                if (!box->sel.active) {
                    box->sel.active = gui_true;
                    box->sel.begin = glyph_index;
                    box->sel.end = box->sel.begin;
                } else {
                    if (box->sel.begin > glyph_index) {
                        box->sel.end = glyph_index;
                        box->sel.active = gui_true;
                    }
                }
            } else if (!GUI_INBOX(in->mouse.pos.x,in->mouse.pos.y,r.x,r.y,r.w,r.h) &&
                GUI_INBOX(in->mouse.buttons[GUI_BUTTON_LEFT].clicked_pos.x,
                    in->mouse.buttons[GUI_BUTTON_LEFT].clicked_pos.y,r.x,r.y,r.w,r.h)
                && box->cursor != box->glyphes && box->cursor > 0)
            {
                /* text selection out of the current text frame */
                gui_size glyph = ((in->mouse.pos.x > r.x) &&
                    box->cursor+1 < box->glyphes) ?
                    box->cursor+1: box->cursor-1;
                gui_edit_box_set_cursor(box, glyph);
                if (box->sel.active) {
                    box->sel.end = glyph;
                    box->sel.active = gui_true;
                }
            } else box->sel.active = gui_false;
        } else box->sel.active = gui_false;


        /* calculate the text bounds */
        label.x = r.x + field->padding.x + field->border_size;
        label.y = r.y + field->padding.y + field->border_size;
        label.h = r.h - (2 * field->padding.y + 2 * field->border_size);
        gui_command_buffer_push_text(out , label, &buffer[offset], text_len,
            font, field->background, field->text);

        /* draw selected text */
        if (box->active && field->show_cursor) {
            if (box->cursor == box->glyphes) {
                /* draw the cursor at the end of the string */
                gui_command_buffer_push_rect(out, gui_rect(label.x+(gui_float)text_width,
                        label.y, (gui_float)cursor_w, label.h), 0, field->cursor);
            } else {
                /* draw text selection */
                gui_size l, s;
                gui_long unicode;
                gui_char *begin, *end;
                gui_size off_begin, off_end;
                gui_size min = MIN(box->sel.end, box->sel.begin);
                gui_size maxi = MAX(box->sel.end, box->sel.begin);
                struct gui_rect clip = out->clip;

                /* calculate selection text range */
                begin = gui_edit_buffer_at(&box->buffer, (gui_int)min, &unicode, &l);
                end = gui_edit_buffer_at(&box->buffer, (gui_int)maxi, &unicode, &l);
                off_begin = (gui_size)(begin - (gui_char*)box->buffer.memory.ptr);
                off_end = (gui_size)(end - (gui_char*)box->buffer.memory.ptr);

                /* calculate selected text width */
                gui_command_buffer_push_scissor(out, label);
                s = font->width(font->userdata, buffer + offset, off_begin - offset);
                label.x += (gui_float)s;
                s = font->width(font->userdata, begin, MAX(l, off_end - off_begin));
                label.w = (gui_float)s;

                /* draw selected text */
                gui_command_buffer_push_text(out , label, begin,
                    MAX(l, off_end - off_begin), font, field->cursor, field->background);
                gui_command_buffer_push_scissor(out, clip);
            }
        }
    }
}

gui_bool
gui_filter_default(gui_long unicode)
{
    (void)unicode;
    return gui_true;
}

gui_bool
gui_filter_ascii(gui_long unicode)
{
    if (unicode < 0 || unicode > 128)
        return gui_false;
    return gui_true;
}

gui_bool
gui_filter_float(gui_long unicode)
{
    if ((unicode < '0' || unicode > '9') && unicode != '.' && unicode != '-')
        return gui_false;
    return gui_true;
}

gui_bool
gui_filter_decimal(gui_long unicode)
{
    if (unicode < '0' || unicode > '9')
        return gui_false;
    return gui_true;
}

gui_bool
gui_filter_hex(gui_long unicode)
{
    if ((unicode < '0' || unicode > '9') &&
        (unicode < 'a' || unicode > 'f') &&
        (unicode < 'A' || unicode > 'F'))
        return gui_false;
    return gui_true;
}

gui_bool
gui_filter_oct(gui_long unicode)
{
    if (unicode < '0' || unicode > '7')
        return gui_false;
    return gui_true;
}

gui_bool
gui_filter_binary(gui_long unicode)
{
    if (unicode < '0' || unicode > '1')
        return gui_false;
    return gui_true;
}

gui_size
gui_widget_edit_filtered(struct gui_command_buffer *out, struct gui_rect r,
    gui_char *buffer, gui_size len, gui_size max, gui_state *active,
    gui_size *cursor,  const struct gui_edit *field, gui_filter filter,
    const struct gui_input *in, const struct gui_user_font *font)
{
    struct gui_edit_box box;
    gui_edit_box_init_fixed(&box, buffer, max, 0, filter);

    box.buffer.allocated = len;
    box.active = *active;
    box.glyphes = gui_utf_len(buffer, len);
    if (!cursor) box.cursor = box.glyphes;
    else box.cursor = MIN(*cursor, box.glyphes);

    gui_widget_editbox(out, r, &box, field, in, font);
    *active = box.active;
    return gui_edit_box_len(&box);
}

gui_size
gui_widget_edit(struct gui_command_buffer *out, struct gui_rect r,
    gui_char *buffer, gui_size len, gui_size max, gui_state *active,
    gui_size *cursor, const struct gui_edit *field, enum gui_input_filter f,
    const struct gui_input *in, const struct gui_user_font *font)
{
    static const gui_filter filter[] = {
        gui_filter_default,
        gui_filter_ascii,
        gui_filter_float,
        gui_filter_decimal,
        gui_filter_hex,
        gui_filter_oct,
        gui_filter_binary,
    };
    return gui_widget_edit_filtered(out, r, buffer, len, max, active,
            cursor, field, filter[f], in, font);
}

gui_float
gui_widget_scrollbarv(struct gui_command_buffer *out, struct gui_rect scroll,
    gui_float offset, gui_float target, gui_float step, const struct gui_scrollbar *s,
    const struct gui_input *i)
{
    struct gui_rect cursor;
    gui_float scroll_step;
    gui_float scroll_offset;
    gui_float scroll_off, scroll_ratio;
    gui_bool inscroll, incursor;
    struct gui_color col;

    GUI_ASSERT(out);
    GUI_ASSERT(s);
    if (!out || !s) return 0;

    /* scrollbar background */
    scroll.w = MAX(scroll.w, 1);
    scroll.h = MAX(scroll.h, 2 * scroll.w);
    if (target <= scroll.h) return 0;

    /* calculate scrollbar constants */
    scroll.h = scroll.h - 2 * scroll.w;
    scroll.y = scroll.y + scroll.w;
    scroll_step = MIN(step, scroll.h);
    scroll_offset = MIN(offset, target - scroll.h);
    scroll_ratio = scroll.h / target;
    scroll_off = scroll_offset / target;

    /* calculate scrollbar cursor bounds */
    cursor.h = (scroll_ratio * scroll.h - 2);
    cursor.y = scroll.y + (scroll_off * scroll.h) + 1;
    cursor.w = scroll.w - 2;
    cursor.x = scroll.x + 1;

    col = s->normal;
    if (i) {
        inscroll = gui_input_is_mouse_hovering_rect(i, scroll);
        incursor = gui_input_is_mouse_prev_hovering_rect(i, cursor);
        if (gui_input_is_mouse_hovering_rect(i, cursor))
            col = s->hover;
        if (i->mouse.buttons[GUI_BUTTON_LEFT].down && inscroll && incursor) {
            /* update cursor by mouse dragging */
            const gui_float pixel = i->mouse.delta.y;
            const gui_float delta =  (pixel / scroll.h) * target;
            scroll_offset = CLAMP(0, scroll_offset + delta, target - scroll.h);
            col = s->active;
        } else if (s->has_scrolling && ((i->mouse.scroll_delta<0) || (i->mouse.scroll_delta>0))) {
            /* update cursor by mouse scrolling */
            scroll_offset = scroll_offset + scroll_step * (-i->mouse.scroll_delta);
            scroll_offset = CLAMP(0, scroll_offset, target - scroll.h);
        }
        scroll_off = scroll_offset / target;
        cursor.y = scroll.y + (scroll_off * scroll.h);
    }

    /* draw scrollbar cursor */
    gui_command_buffer_push_rect(out, gui_shrink_rect(scroll,1), s->rounding, s->border);
    gui_command_buffer_push_rect(out, scroll, s->rounding, s->background);
    gui_command_buffer_push_rect(out, cursor, s->rounding, col);
    return scroll_offset;
}

gui_float
gui_widget_scrollbarh(struct gui_command_buffer *out, struct gui_rect scroll,
    gui_float offset, gui_float target, gui_float step, const struct gui_scrollbar *s,
    const struct gui_input *i)
{
    struct gui_rect cursor;
    gui_float scroll_step;
    gui_float scroll_offset;
    gui_float scroll_off, scroll_ratio;
    gui_bool inscroll, incursor;
    struct gui_color col;

    GUI_ASSERT(out);
    GUI_ASSERT(s);
    if (!out || !s) return 0;

    /* scrollbar background */
    scroll.h = MAX(scroll.h, 1);
    scroll.w = MAX(scroll.w, 2 * scroll.h);
    if (target <= scroll.w) return 0;
    gui_command_buffer_push_rect(out, scroll, s->rounding,s->background);

    /* calculate scrollbar constants */
    scroll.w = scroll.w - 2 * scroll.h;
    scroll.x = scroll.x + scroll.h;
    scroll_step = MIN(step, scroll.w);
    scroll_offset = MIN(offset, target - scroll.h);
    scroll_ratio = scroll.w / target;
    scroll_off = scroll_offset / target;

    /* calculate scrollbar cursor bounds */
    cursor.w = scroll_ratio * scroll.w;
    cursor.x = scroll.x + (scroll_off * scroll.w);
    cursor.h = scroll.h;
    cursor.y = scroll.y;

    col = s->normal;
    if (i) {
        inscroll = gui_input_is_mouse_hovering_rect(i, scroll);
        incursor = gui_input_is_mouse_prev_hovering_rect(i, cursor);
        if (gui_input_is_mouse_hovering_rect(i, cursor))
            col = s->hover;

        if (i->mouse.buttons[GUI_BUTTON_LEFT].down && inscroll && incursor) {
            /* update cursor by mouse dragging */
            const gui_float pixel = i->mouse.delta.x;
            const gui_float delta =  (pixel / scroll.w) * target;
            scroll_offset = CLAMP(0, scroll_offset + delta, target - scroll.w);
            col = s->active;
        } else if (s->has_scrolling && ((i->mouse.scroll_delta<0) || (i->mouse.scroll_delta>0))) {
            /* update cursor by mouse scrolling */
            scroll_offset = scroll_offset + scroll_step * (-i->mouse.scroll_delta);
            scroll_offset = CLAMP(0, scroll_offset, target - scroll.w);
        }
        scroll_off = scroll_offset / target;
        cursor.x = scroll.x + (scroll_off * scroll.w);
    }

    /* draw scrollbar cursor */
    gui_command_buffer_push_rect(out, cursor, s->rounding, col);
    return scroll_offset;

}

static gui_int
gui_widget_spinner_base(struct gui_command_buffer *out, struct gui_rect r,
    const struct gui_spinner *s, gui_char *buffer, gui_size *len,
    enum gui_input_filter filter, gui_state *active,
    const struct gui_input *in, const struct gui_user_font *font)
{
    gui_int ret = 0;
    struct gui_rect bounds;
    struct gui_edit field;
    gui_bool button_up_clicked, button_down_clicked;
    gui_state is_active = (active) ? *active : gui_false;

    r.h = MAX(r.h, font->height + 2 * s->padding.x);
    r.w = MAX(r.w, r.h - (s->padding.x + (gui_float)s->button.base.border_width * 2));

    /* up/down button setup and execution */
    bounds.y = r.y;
    bounds.h = r.h / 2;
    bounds.w = r.h - s->padding.x;
    bounds.x = r.x + r.w - bounds.w;
    button_up_clicked = gui_widget_button_symbol(out, bounds, GUI_SYMBOL_TRIANGLE_UP,
        GUI_BUTTON_DEFAULT, &s->button, in, font);
    if (button_up_clicked) ret = 1;

    bounds.y = r.y + bounds.h;
    button_down_clicked = gui_widget_button_symbol(out, bounds, GUI_SYMBOL_TRIANGLE_DOWN,
        GUI_BUTTON_DEFAULT, &s->button, in, font);
    if (button_down_clicked) ret = -1;

    /* editbox setup and execution */
    bounds.x = r.x;
    bounds.y = r.y;
    bounds.h = r.h;
    bounds.w = r.w - (r.h - s->padding.x);

    field.border_size = 1;
    field.rounding = 0;
    field.padding.x = s->padding.x;
    field.padding.y = s->padding.y;
    field.show_cursor = s->show_cursor;
    field.background = s->color;
    field.border = s->border;
    field.text = s->text;
    *len = gui_widget_edit(out, bounds, buffer, *len, GUI_MAX_NUMBER_BUFFER,
        &is_active, 0, &field, filter, in, font);
    if (active) *active = is_active;
    return ret;
}

gui_int
gui_widget_spinner(struct gui_command_buffer *out, struct gui_rect r,
    const struct gui_spinner *s, gui_int min, gui_int value,
    gui_int max, gui_int step, gui_state *active, const struct gui_input *in,
    const struct gui_user_font *font)
{
    gui_int res;
    gui_char string[GUI_MAX_NUMBER_BUFFER];
    gui_size len, old_len;
    gui_state is_active;

    GUI_ASSERT(out);
    GUI_ASSERT(s);
    GUI_ASSERT(font);
    if (!out || !s || !font) return value;

    /* make sure given values are correct */
    value = CLAMP(min, value, max);
    len = gui_itos(string, value);
    is_active = (active) ? *active : gui_false;
    old_len = len;

    res = gui_widget_spinner_base(out, r, s, string, &len, GUI_INPUT_DEC, active, in, font);
    if (res) {
        value += (res > 0) ? step : -step;
        value = CLAMP(min, value, max);
    }

    if (old_len != len)
        gui_strtoi(&value, string, len);
    if (active) *active = is_active;
    return value;
}

/*
 * ==============================================================
 *
 *                          Config
 *
 * ===============================================================
 */
static void
gui_style_default_properties(struct gui_style *style)
{
    style->properties[GUI_PROPERTY_SCROLLBAR_SIZE] = gui_vec2(14, 14);
    style->properties[GUI_PROPERTY_PADDING] = gui_vec2(15.0f, 10.0f);
    style->properties[GUI_PROPERTY_SIZE] = gui_vec2(64.0f, 64.0f);
    style->properties[GUI_PROPERTY_ITEM_SPACING] = gui_vec2(4.0f, 4.0f);
    style->properties[GUI_PROPERTY_ITEM_PADDING] = gui_vec2(4.0f, 4.0f);
    style->properties[GUI_PROPERTY_SCALER_SIZE] = gui_vec2(16.0f, 16.0f);
}

static void
gui_style_default_rounding(struct gui_style *style)
{
    style->rounding[GUI_ROUNDING_BUTTON] = 4.0f;
    style->rounding[GUI_ROUNDING_SLIDER] = 8.0f;
    style->rounding[GUI_ROUNDING_PROGRESS] = 4.0f;
    style->rounding[GUI_ROUNDING_CHECK] = 0.0f;
    style->rounding[GUI_ROUNDING_INPUT] = 0.0f;
    style->rounding[GUI_ROUNDING_GRAPH] = 4.0f;
    style->rounding[GUI_ROUNDING_SCROLLBAR] = 5.0f;
}

static void
gui_style_default_color(struct gui_style *style)
{
    style->colors[GUI_COLOR_TEXT] = gui_rgba(135, 135, 135, 255);
    style->colors[GUI_COLOR_TEXT_HOVERING] = gui_rgba(120, 120, 120, 255);
    style->colors[GUI_COLOR_TEXT_ACTIVE] = gui_rgba(100, 100, 100, 255);
    style->colors[GUI_COLOR_WINDOW] = gui_rgba(45, 45, 45, 255);
    style->colors[GUI_COLOR_HEADER] = gui_rgba(40, 40, 40, 255);
    style->colors[GUI_COLOR_BORDER] = gui_rgba(38, 38, 38, 255);
    style->colors[GUI_COLOR_BUTTON] = gui_rgba(50, 50, 50, 255);
    style->colors[GUI_COLOR_BUTTON_HOVER] = gui_rgba(35, 35, 35, 255);
    style->colors[GUI_COLOR_BUTTON_ACTIVE] = gui_rgba(40, 40, 40, 255);
    style->colors[GUI_COLOR_TOGGLE] = gui_rgba(100, 100, 100, 255);
    style->colors[GUI_COLOR_TOGGLE_HOVER] = gui_rgba(120, 120, 120, 255);
    style->colors[GUI_COLOR_TOGGLE_CURSOR] = gui_rgba(45, 45, 45, 255);
    style->colors[GUI_COLOR_SLIDER] = gui_rgba(38, 38, 38, 255);
    style->colors[GUI_COLOR_SLIDER_CURSOR] = gui_rgba(100, 100, 100, 255);
    style->colors[GUI_COLOR_SLIDER_CURSOR_HOVER] = gui_rgba(120, 120, 120, 255);
    style->colors[GUI_COLOR_SLIDER_CURSOR_ACTIVE] = gui_rgba(150, 150, 150, 255);
    style->colors[GUI_COLOR_PROGRESS] = gui_rgba(38, 38, 38, 255);
    style->colors[GUI_COLOR_PROGRESS_CURSOR] = gui_rgba(100, 100, 100, 255);
    style->colors[GUI_COLOR_PROGRESS_CURSOR_HOVER] = gui_rgba(120, 120, 120, 255);
    style->colors[GUI_COLOR_PROGRESS_CURSOR_ACTIVE] = gui_rgba(150, 150, 150, 255);
    style->colors[GUI_COLOR_INPUT] = gui_rgba(45, 45, 45, 255);
    style->colors[GUI_COLOR_INPUT_CURSOR] = gui_rgba(100, 100, 100, 255);
    style->colors[GUI_COLOR_INPUT_TEXT] = gui_rgba(135, 135, 135, 255);
    style->colors[GUI_COLOR_SPINNER] = gui_rgba(45, 45, 45, 255);
    style->colors[GUI_COLOR_SPINNER_TRIANGLE] = gui_rgba(100, 100, 100, 255);
    style->colors[GUI_COLOR_HISTO] = gui_rgba(120, 120, 120, 255);
    style->colors[GUI_COLOR_HISTO_BARS] = gui_rgba(45, 45, 45, 255);
    style->colors[GUI_COLOR_HISTO_NEGATIVE] = gui_rgba(255, 255, 255, 255);
    style->colors[GUI_COLOR_HISTO_HIGHLIGHT] = gui_rgba( 255, 0, 0, 255);
    style->colors[GUI_COLOR_PLOT] = gui_rgba(100, 100, 100, 255);
    style->colors[GUI_COLOR_PLOT_LINES] = gui_rgba(45, 45, 45, 255);
    style->colors[GUI_COLOR_PLOT_HIGHLIGHT] = gui_rgba(255, 0, 0, 255);
    style->colors[GUI_COLOR_SCROLLBAR] = gui_rgba(40, 40, 40, 255);
    style->colors[GUI_COLOR_SCROLLBAR_CURSOR] = gui_rgba(100, 100, 100, 255);
    style->colors[GUI_COLOR_SCROLLBAR_CURSOR_HOVER] = gui_rgba(120, 120, 120, 255);
    style->colors[GUI_COLOR_SCROLLBAR_CURSOR_ACTIVE] = gui_rgba(150, 150, 150, 255);
    style->colors[GUI_COLOR_TABLE_LINES] = gui_rgba(100, 100, 100, 255);
    style->colors[GUI_COLOR_TAB_HEADER] = gui_rgba(40, 40, 40, 255);
    style->colors[GUI_COLOR_SHELF] = gui_rgba(40, 40, 40, 255);
    style->colors[GUI_COLOR_SHELF_TEXT] = gui_rgba(150, 150, 150, 255);
    style->colors[GUI_COLOR_SHELF_ACTIVE] = gui_rgba(30, 30, 30, 255);
    style->colors[GUI_COLOR_SHELF_ACTIVE_TEXT] = gui_rgba(150, 150, 150, 255);
    style->colors[GUI_COLOR_SCALER] = gui_rgba(100, 100, 100, 255);
}

void
gui_style_default(struct gui_style *style, gui_flags flags,
    const struct gui_user_font *font)
{
    GUI_ASSERT(style);
    if (!style) return;
    gui_zero(style, sizeof(*style));
    style->font = *font;

    if (flags & GUI_DEFAULT_COLOR)
        gui_style_default_color(style);
    if (flags & GUI_DEFAULT_PROPERTIES)
        gui_style_default_properties(style);
    if (flags & GUI_DEFAULT_ROUNDING)
        gui_style_default_rounding(style);
}

void
gui_style_set_font(struct gui_style *style, const struct gui_user_font *font)
{
    GUI_ASSERT(style);
    if (!style) return;
    style->font = *font;
}

struct gui_vec2
gui_style_property(const struct gui_style *style, enum gui_style_properties index)
{
    static struct gui_vec2 zero;
    GUI_ASSERT(style);
    if (!style) return zero;
    return style->properties[index];
}

struct gui_color
gui_style_color(const struct gui_style *style, enum gui_style_colors index)
{
    static struct gui_color zero;
    GUI_ASSERT(style);
    if (!style) return zero;
    return style->colors[index];
}

void
gui_style_push_color(struct gui_style *style, enum gui_style_colors index,
    struct gui_color col)
{
    struct gui_saved_color *c;
    GUI_ASSERT(style);
    if (!style) return;
    if (style->stack.color >= GUI_MAX_COLOR_STACK) return;

    c = &style->stack.colors[style->stack.color++];
    c->value = style->colors[index];
    c->type = index;
    style->colors[index] = col;
}

void
gui_style_push_property(struct gui_style *style, enum gui_style_properties index,
    struct gui_vec2 v)
{
    struct gui_saved_property *p;
    GUI_ASSERT(style);
    if (!style) return;
    if (style->stack.property >= GUI_MAX_ATTRIB_STACK) return;

    p = &style->stack.properties[style->stack.property++];
    p->value = style->properties[index];
    p->type = index;
    style->properties[index] = v;
}

void
gui_style_pop_color(struct gui_style *style)
{
    struct gui_saved_color *c;
    GUI_ASSERT(style);
    if (!style) return;
    if (!style->stack.color) return;
    c = &style->stack.colors[--style->stack.color];
    style->colors[c->type] = c->value;
}

void
gui_style_pop_property(struct gui_style *style)
{
    struct gui_saved_property *p;
    GUI_ASSERT(style);
    if (!style) return;
    if (!style->stack.property) return;
    p = &style->stack.properties[--style->stack.property];
    style->properties[p->type] = p->value;
}

void
gui_style_reset_colors(struct gui_style *style)
{
    GUI_ASSERT(style);
    if (!style) return;
    while (style->stack.color)
        gui_style_pop_color(style);
}

void
gui_style_reset_properties(struct gui_style *style)
{
    GUI_ASSERT(style);
    if (!style) return;
    while (style->stack.property)
        gui_style_pop_property(style);
}

void
gui_style_reset(struct gui_style *style)
{
    GUI_ASSERT(style);
    if (!style) return;
    gui_style_reset_colors(style);
    gui_style_reset_properties(style);
}
/*
 * ==============================================================
 *
 *                          Tiling
 *
 * ===============================================================
 */
void
gui_tiled_begin(struct gui_tiled_layout *layout, enum gui_layout_format fmt,
    gui_float width, gui_float height)
{
    GUI_ASSERT(layout);
    if (!layout) return;
    gui_zero(layout->slots, sizeof(layout->slots));
    layout->fmt = fmt;
    layout->width = width;
    layout->height = height;
}

void
gui_tiled_slot(struct gui_tiled_layout *layout,
    enum gui_tiled_layout_slot_index slot, gui_float ratio,
    enum gui_tiled_slot_format fmt, gui_uint widget_count)
{
    GUI_ASSERT(layout);
    if (!layout) return;
    layout->slots[slot].capacity = widget_count;
    layout->slots[slot].format = fmt;
    layout->slots[slot].value = ratio;
}

void
gui_tiled_slot_bounds(struct gui_rect *bounds,
    const struct gui_tiled_layout *layout, enum gui_tiled_layout_slot_index slot)
{
    const struct gui_tiled_slot *s;
    GUI_ASSERT(layout);
    if (!layout) return;

    s = &layout->slots[slot];
    if (layout->fmt == GUI_DYNAMIC) {
        bounds->x = s->pos.x * (gui_float)layout->width;
        bounds->y = s->pos.y * (gui_float)layout->height;
        bounds->w = s->size.x * (gui_float)layout->width;
        bounds->h = s->size.y * (gui_float)layout->height;
    } else {
        bounds->x = s->pos.x;
        bounds->y = s->pos.y;
        bounds->w = s->size.x;
        bounds->h = s->size.y;
    }
}

void
gui_tiled_bounds(struct gui_rect *bounds, const struct gui_tiled_layout *layout,
    enum gui_tiled_layout_slot_index slot, gui_uint index)
{
    struct gui_rect slot_bounds;
    const struct gui_tiled_slot *s;
    GUI_ASSERT(layout);
    if (!layout) return;

    GUI_ASSERT(slot < GUI_SLOT_MAX);
    s = &layout->slots[slot];
    GUI_ASSERT(index < s->capacity);
    gui_tiled_slot_bounds(&slot_bounds, layout, slot);
    if (s->format == GUI_SLOT_HORIZONTAL) {
        bounds->h = slot_bounds.h;
        bounds->y = slot_bounds.y;
        bounds->w = slot_bounds.w / (gui_float)s->capacity;
        bounds->x = slot_bounds.x + (gui_float)index * bounds->w;
    } else {
        bounds->x = slot_bounds.x;
        bounds->w = slot_bounds.w;
        bounds->h = slot_bounds.h/(gui_float)s->capacity;
        bounds->y = slot_bounds.y + (gui_float)index * bounds->h;
    }
}

void
gui_tiled_end(struct gui_tiled_layout *layout)
{
    gui_float centerh, centerv;
    const struct gui_tiled_slot *top, *bottom;
    const struct gui_tiled_slot *left, *right;
    GUI_ASSERT(layout);
    if (!layout) return;

    top = &layout->slots[GUI_SLOT_TOP];
    bottom = &layout->slots[GUI_SLOT_BOTTOM];
    left = &layout->slots[GUI_SLOT_LEFT];
    right = &layout->slots[GUI_SLOT_RIGHT];

    if (layout->fmt == GUI_DYNAMIC) {
        layout->width = 1.0f;
        centerh = MAX(0.0f, 1.0f - (left->value + right->value));
        centerv = MAX(0.0f, 1.0f - (top->value + bottom->value));
    } else {
        centerh = MAX(0.0f, layout->width - (left->value + right->value));
        centerv = MAX(0.0f, layout->height - (top->value + bottom->value));
    }

    /* calculate the slot size */
    layout->slots[GUI_SLOT_CENTER].size = gui_vec2(centerh, centerv);
    layout->slots[GUI_SLOT_TOP].size = gui_vec2(layout->width, top->value);
    layout->slots[GUI_SLOT_LEFT].size = gui_vec2(left->value, centerv);
    layout->slots[GUI_SLOT_BOTTOM].size = gui_vec2(layout->width, bottom->value);
    layout->slots[GUI_SLOT_RIGHT].size = gui_vec2(right->value, centerv);

    /* calculate the slot window position */
    layout->slots[GUI_SLOT_TOP].pos = gui_vec2(0.0f, 0.0f);
    layout->slots[GUI_SLOT_LEFT].pos = gui_vec2(0.0f, top->value);
    layout->slots[GUI_SLOT_BOTTOM].pos = gui_vec2(0.0f, top->value + centerv);
    layout->slots[GUI_SLOT_RIGHT].pos = gui_vec2(left->value + centerh, top->value);
    layout->slots[GUI_SLOT_CENTER].pos = gui_vec2(left->value, top->value);
}
/*
 * ==============================================================
 *
 *                          Window
 *
 * ===============================================================
 */
void
gui_window_init(struct gui_window *window, struct gui_rect bounds,
    gui_flags flags, struct gui_command_queue *queue,
    const struct gui_style *style, const struct gui_input *input)
{
    GUI_ASSERT(window);
    GUI_ASSERT(style);
    GUI_ASSERT(input);
    if (!window || !style || !input)
        return;

    window->bounds = bounds;
    window->flags = flags;
    window->style = style;
    window->offset.x = 0;
    window->offset.y = 0;
    window->queue = queue;
    window->input = input;
    if (queue) {
        gui_command_buffer_init(&window->buffer, &queue->buffer, GUI_CLIP);
        gui_command_queue_insert_back(queue, &window->buffer);
    }
}
void
gui_window_set_config(struct gui_window *panel, const struct gui_style *config)
{
    GUI_ASSERT(panel);
    GUI_ASSERT(config);
    if (!panel || !config) return;
    panel->style = config;
}

void
gui_window_unlink(struct gui_window *window)
{
    GUI_ASSERT(window);
    GUI_ASSERT(window->queue);
    if (!window || !window->queue) return;
    gui_command_queue_remove(window->queue, &window->buffer);
    window->queue = 0;
}

void
gui_window_set_queue(struct gui_window *panel, struct gui_command_queue *queue)
{
    GUI_ASSERT(panel);
    GUI_ASSERT(queue);
    if (!panel || !queue) return;
    if (panel->queue)
        gui_window_unlink(panel);
    gui_command_queue_insert_back(queue, &panel->buffer);
    panel->queue = queue;
}

void
gui_window_set_input(struct gui_window *window, const struct gui_input *in)
{
    GUI_ASSERT(window);
    GUI_ASSERT(in);
    if (!window || !in) return;
    window->input = in;
}

struct gui_vec2
gui_window_get_scrollbar(struct gui_window *window)
{GUI_ASSERT(window); return window->offset;}

void
gui_window_set_scrollbar(struct gui_window *window, struct gui_vec2 offset)
{GUI_ASSERT(window); if (!window) return; window->offset = offset;}

void
gui_window_set_position(struct gui_window *window, struct gui_vec2 offset)
{
    GUI_ASSERT(window);
    if (!window) return;
    window->bounds.x = offset.x;
    window->bounds.y = offset.y;
}

struct gui_vec2
gui_window_get_position(struct gui_window *window)
{GUI_ASSERT(window);return gui_vec2(window->bounds.x, window->bounds.y);}

void
gui_window_set_size(struct gui_window *window, struct gui_vec2 size)
{
    GUI_ASSERT(window);
    if (!window) return;
    window->bounds.w = size.x;
    window->bounds.h = size.y;
}

struct gui_vec2
gui_window_get_size(struct gui_window *window)
{GUI_ASSERT(window);return gui_vec2(window->bounds.w, window->bounds.h);}

void
gui_window_set_bounds(struct gui_window *window, struct gui_rect bounds)
{GUI_ASSERT(window); if (!window) return; window->bounds = bounds;}

struct gui_rect
gui_window_get_bounds(struct gui_window *window)
{GUI_ASSERT(window);return window->bounds;}

void
gui_window_add_flag(struct gui_window *panel, gui_flags f)
{panel->flags |= f;}

void
gui_window_remove_flag(struct gui_window *panel, gui_flags f)
{panel->flags &= (gui_flags)~f;}

gui_bool
gui_window_has_flag(struct gui_window *panel, gui_flags f)
{return (panel->flags & f) ? gui_true: gui_false;}

gui_bool
gui_window_is_minimized(struct gui_window *panel)
{return panel->flags & GUI_WINDOW_MINIMIZED;}

/*
 * -------------------------------------------------------------
 *
 *                          Context
 *
 * --------------------------------------------------------------
 */
void
gui_begin(struct gui_context *context, struct gui_window *window)
{
    const struct gui_style *c;
    gui_float scrollbar_size;
    struct gui_vec2 item_padding;
    struct gui_vec2 item_spacing;
    struct gui_vec2 window_padding;
    struct gui_vec2 scaler_size;
    struct gui_command_buffer *out;
    const struct gui_input *in;

    GUI_ASSERT(context);
    GUI_ASSERT(window);
    GUI_ASSERT(window->style);

    /* cache configuration data */
    c = window->style;
    in = window->input;
    scrollbar_size = gui_style_property(c, GUI_PROPERTY_SCROLLBAR_SIZE).x;
    window_padding = gui_style_property(c, GUI_PROPERTY_PADDING);
    item_padding = gui_style_property(c, GUI_PROPERTY_ITEM_PADDING);
    item_spacing = gui_style_property(c, GUI_PROPERTY_ITEM_SPACING);
    scaler_size = gui_style_property(c, GUI_PROPERTY_SCALER_SIZE);

    /* check arguments */
    if (!window || !context) return;
    gui_zero(context, sizeof(*context));
    if (window->flags & GUI_WINDOW_HIDDEN) {
        context->flags = window->flags;
        context->valid = gui_false;
        context->style = window->style;
        context->buffer = &window->buffer;
        return;
    }

    /* overlapping panels */
    if (window->queue && !(window->flags & GUI_WINDOW_TAB))
    {
        context->queue = window->queue;
        gui_command_queue_start(window->queue, &window->buffer);
        {
            gui_bool inpanel;
            struct gui_command_buffer_list *s = &window->queue->list;
            inpanel = gui_input_mouse_clicked(in, GUI_BUTTON_LEFT, window->bounds);
            if (inpanel && (&window->buffer != s->end)) {
                const struct gui_command_buffer *iter = window->buffer.next;
                while (iter) {
                    /* try to find a panel with higher priorty in the same position */
                    const struct gui_window *cur;
                    cur = GUI_CONTAINER_OF_CONST(iter, struct gui_window, buffer);
                    if (GUI_INBOX(in->mouse.prev.x, in->mouse.prev.y, cur->bounds.x,
                        cur->bounds.y, cur->bounds.w, cur->bounds.h) &&
                      !(cur->flags & GUI_WINDOW_MINIMIZED) && !(cur->flags & GUI_WINDOW_HIDDEN))
                        break;
                    iter = iter->next;
                }
                /* current panel is active panel in that position so transfer to top
                 * at the highest priority in stack */
                if (!iter) {
                    gui_command_queue_remove(window->queue, &window->buffer);
                    gui_command_queue_insert_back(window->queue, &window->buffer);
                    window->flags &= ~(gui_flags)GUI_WINDOW_ROM;
                }
            }
            if (s->end != &window->buffer)
                window->flags |= GUI_WINDOW_ROM;
        }
    }

    /* move panel position if requested */
    context->header.h = c->font.height + 4 * item_padding.y;
    context->header.h += window_padding.y;
    if ((window->flags & GUI_WINDOW_MOVEABLE) && !(window->flags & GUI_WINDOW_ROM)) {
        gui_bool incursor;
        struct gui_rect move;
        move.x = window->bounds.x;
        move.y = window->bounds.y;
        move.w = window->bounds.w;
        move.h = context->header.h;
        incursor = gui_input_is_mouse_prev_hovering_rect(in, move);
        if (gui_input_is_mouse_down(in, GUI_BUTTON_LEFT) && incursor) {
            window->bounds.x = MAX(0, window->bounds.x + in->mouse.delta.x);
            window->bounds.y = MAX(0, window->bounds.y + in->mouse.delta.y);
        }
    }

    /* setup panel context */
    context->input = in;
    context->bounds = window->bounds;
    context->at_x = window->bounds.x;
    context->at_y = window->bounds.y;
    context->width = window->bounds.w;
    context->height = window->bounds.h;
    context->style = window->style;
    context->buffer = &window->buffer;
    context->row.index = 0;
    context->row.columns = 0;
    context->row.height = 0;
    context->row.ratio = 0;
    context->row.item_width = 0;
    context->offset = window->offset;
    context->header.h = 1;
    context->row.height = context->header.h + 2 * item_spacing.y;
    out = &window->buffer;

    /* panel activation by clicks inside of the panel */
    if (!(window->flags & GUI_WINDOW_TAB) && !(window->flags & GUI_WINDOW_ROM)) {
        gui_float clicked_x = in->mouse.buttons[GUI_BUTTON_LEFT].clicked_pos.x;
        gui_float clicked_y = in->mouse.buttons[GUI_BUTTON_LEFT].clicked_pos.y;
        if (gui_input_is_mouse_down(in, GUI_BUTTON_LEFT)) {
            if (GUI_INBOX(clicked_x, clicked_y, window->bounds.x, window->bounds.y,
                window->bounds.w, window->bounds.h))
                window->flags |= GUI_WINDOW_ACTIVE;
            else window->flags &= (gui_flag)~GUI_WINDOW_ACTIVE;
        }
    }

    context->flags = window->flags;
    context->valid = !(window->flags & GUI_WINDOW_HIDDEN) &&
        !(window->flags & GUI_WINDOW_MINIMIZED);
    context->footer_h = scaler_size.y + item_padding.y;

    /* calculate the window size and window footer height */
    if (!(window->flags & GUI_WINDOW_NO_SCROLLBAR))
        context->width = window->bounds.w - scrollbar_size;
    context->height = window->bounds.h - (context->header.h + 2 * item_spacing.y);
    if (context->flags & GUI_WINDOW_SCALEABLE)
        context->height -= context->footer_h;

    /* draw window background if not a dynamic window */
    if (!(context->flags & GUI_WINDOW_DYNAMIC) && context->valid) {
        gui_command_buffer_push_rect(out, context->bounds, 0, c->colors[GUI_COLOR_WINDOW]);
    } else{
        context->footer_h = scaler_size.y + item_padding.y;
        gui_command_buffer_push_rect(out, gui_rect(context->bounds.x, context->bounds.y,
            context->bounds.w, context->row.height), 0, c->colors[GUI_COLOR_WINDOW]);
    }

    /* draw top border line */
    if (context->flags & GUI_WINDOW_BORDER) {
        gui_command_buffer_push_line(out, context->bounds.x, context->bounds.y,
            context->bounds.x + context->bounds.w, context->bounds.y,
            c->colors[GUI_COLOR_BORDER]);
    }

    /* calculate and set the window clipping rectangle*/
    context->clip.x = window->bounds.x;
    context->clip.y = window->bounds.y;
    context->clip.w = window->bounds.w;
    context->clip.h = window->bounds.h - (context->footer_h + context->header.h);
    context->clip.h -= (window_padding.y + item_padding.y);
    gui_command_buffer_push_scissor(out, context->clip);
}

void
gui_begin_tiled(struct gui_context *context, struct gui_window *window,
    struct gui_tiled_layout *tiled, enum gui_tiled_layout_slot_index slot,
    gui_uint index)
{
    GUI_ASSERT(context);
    GUI_ASSERT(window);
    GUI_ASSERT(tiled);
    GUI_ASSERT(slot < GUI_SLOT_MAX);
    GUI_ASSERT(index < tiled->slots[slot].capacity);
    if (!context || !window || !tiled) return;

    /* make sure that correct flags are set */
    window->flags &= (gui_flags)~GUI_WINDOW_MOVEABLE;
    window->flags &= (gui_flags)~GUI_WINDOW_SCALEABLE;
    window->flags &= (gui_flags)~GUI_WINDOW_DYNAMIC;

    /* place window inside layout and set window to background  */
    gui_tiled_bounds(&window->bounds, tiled, slot, index);
    gui_begin(context, window);
    gui_command_queue_remove(window->queue, &window->buffer);
    gui_command_queue_insert_front(window->queue, &window->buffer);
}

void
gui_end(struct gui_context *layout, struct gui_window *window)
{
    const struct gui_input *in;
    const struct gui_style *config;
    struct gui_command_buffer *out;
    gui_float scrollbar_size;
    struct gui_vec2 item_padding;
    struct gui_vec2 item_spacing;
    struct gui_vec2 window_padding;
    struct gui_vec2 scaler_size;
    struct gui_rect footer = {0,0,0,0};

    GUI_ASSERT(layout);
    GUI_ASSERT(window);
    if (!window || !layout) return;

    config = layout->style;
    out = layout->buffer;
    in = (layout->flags & GUI_WINDOW_ROM) ? 0 :layout->input;
    if (!(layout->flags & GUI_WINDOW_TAB)) {
        struct gui_rect clip;
        clip.x = MAX(0, (layout->bounds.x - 1));
        clip.y = MAX(0, (layout->bounds.y - 1));
        clip.w = layout->bounds.w + 1;
        clip.h = layout->bounds.h + 1;
        gui_command_buffer_push_scissor(out, clip);
    }

    /* cache configuration data */
    item_padding = gui_style_property(config, GUI_PROPERTY_ITEM_PADDING);
    item_spacing = gui_style_property(config, GUI_PROPERTY_ITEM_SPACING);
    window_padding = gui_style_property(config, GUI_PROPERTY_PADDING);
    scrollbar_size = gui_style_property(config, GUI_PROPERTY_SCROLLBAR_SIZE).x;
    scaler_size = gui_style_property(config, GUI_PROPERTY_SCALER_SIZE);

    /* update the current Y-position to point over the last added widget */
    layout->at_y += layout->row.height;

    /* draw footer and fill empty spaces inside a dynamically growing panel */
    if (layout->valid && (layout->flags & GUI_WINDOW_DYNAMIC) &&
        !(layout->valid & GUI_WINDOW_NO_SCROLLBAR)) {
        /* calculate the dynamic window footer bounds */
        layout->height = MIN(layout->at_y - layout->bounds.y, layout->bounds.h);

        /* draw the correct footer */
        footer.x = window->bounds.x;
        footer.w = window->bounds.w + scrollbar_size;
        footer.h = layout->footer_h;
        if (layout->flags & GUI_WINDOW_COMBO_MENU)
            footer.y = window->bounds.y + layout->height;
        else footer.y = window->bounds.y + layout->height + layout->footer_h;
        gui_command_buffer_push_rect(out, footer, 0, config->colors[GUI_COLOR_WINDOW]);

        if (!(layout->flags & GUI_WINDOW_COMBO_MENU)) {
            struct gui_rect bounds;
            bounds.x = layout->bounds.x;
            bounds.y = window->bounds.y + layout->height;
            bounds.w = layout->width;
            bounds.h = layout->row.height;
            gui_command_buffer_push_rect(out, bounds, 0, config->colors[GUI_COLOR_WINDOW]);
        }
    }

    /* scrollbars */
    if (layout->valid && !(layout->flags & GUI_WINDOW_NO_SCROLLBAR)) {
        struct gui_rect bounds;
        gui_float scroll_target, scroll_offset, scroll_step;

        struct gui_scrollbar scroll;
        scroll.rounding = config->rounding[GUI_ROUNDING_SCROLLBAR];
        scroll.background = config->colors[GUI_COLOR_SCROLLBAR];
        scroll.normal = config->colors[GUI_COLOR_SCROLLBAR_CURSOR];
        scroll.hover = config->colors[GUI_COLOR_SCROLLBAR_CURSOR_HOVER];
        scroll.active = config->colors[GUI_COLOR_SCROLLBAR_CURSOR_ACTIVE];
        scroll.border = config->colors[GUI_COLOR_BORDER];
        {
            /* vertical scollbar */
            bounds.x = layout->bounds.x + layout->width;
            bounds.y = (layout->flags & GUI_WINDOW_BORDER) ? layout->bounds.y+1:layout->bounds.y;
            bounds.y += layout->header.h + layout->menu.h;
            bounds.w = scrollbar_size;
            bounds.h = layout->height;
            if (layout->flags & GUI_WINDOW_BORDER) bounds.h -= 1;

            scroll_offset = layout->offset.y;
            scroll_step = layout->height * 0.10f;
            scroll_target = (layout->at_y-layout->bounds.y)-(layout->header.h+2*item_spacing.y);
            scroll.has_scrolling = (layout->flags & GUI_WINDOW_ACTIVE);
            window->offset.y = gui_widget_scrollbarv(out, bounds, scroll_offset,
                                scroll_target, scroll_step, &scroll, in);
        }
        {
            /* horizontal scrollbar */
            bounds.x = layout->bounds.x + window_padding.x;
            if (layout->flags & GUI_WINDOW_TAB) {
                bounds.h = scrollbar_size;
                bounds.y = (layout->flags & GUI_WINDOW_BORDER) ? layout->bounds.y + 1 : layout->bounds.y;
                bounds.y += layout->header.h + layout->menu.h + layout->height;
                bounds.w = layout->width - scrollbar_size;
            } else if (layout->flags & GUI_WINDOW_DYNAMIC) {
                bounds.h = MIN(scrollbar_size, layout->footer_h);
                bounds.w = layout->width - 2 * window_padding.x;
                bounds.y = footer.y;
            } else {
                bounds.h = MIN(scrollbar_size, layout->footer_h);
                bounds.y = layout->bounds.y + window->bounds.h - MAX(layout->footer_h, scrollbar_size);
                bounds.w = layout->width - 2 * window_padding.x;
            }

            scroll_offset = layout->offset.x;
            scroll_step = layout->max_x * 0.05f;
            scroll_target = (layout->max_x - layout->at_x) - 2 * window_padding.x;
            scroll.has_scrolling = gui_false;
            window->offset.x = gui_widget_scrollbarh(out, bounds, scroll_offset,
                                    scroll_target, scroll_step, &scroll, in);
        }
    };

    /* draw the panel scaler into the right corner of the panel footer and
     * update panel size if user drags the scaler */
    if ((layout->flags & GUI_WINDOW_SCALEABLE) && layout->valid && in) {
        gui_float scaler_y;
        struct gui_color col = config->colors[GUI_COLOR_SCALER];
        gui_float scaler_w = MAX(0, scaler_size.x - item_padding.x);
        gui_float scaler_h = MAX(0, scaler_size.y - item_padding.y);
        gui_float scaler_x = (layout->bounds.x + layout->bounds.w) - (item_padding.x + scaler_w);

        if (layout->flags & GUI_WINDOW_DYNAMIC)
            scaler_y = footer.y + layout->footer_h - scaler_size.y;
        else scaler_y = layout->bounds.y + layout->bounds.h - scaler_size.y;
        gui_command_buffer_push_triangle(out, scaler_x + scaler_w, scaler_y,
            scaler_x + scaler_w, scaler_y + scaler_h, scaler_x, scaler_y + scaler_h, col);

        if (!(window->flags & GUI_WINDOW_ROM)) {
            gui_bool incursor;
            gui_float prev_x = in->mouse.prev.x;
            gui_float prev_y = in->mouse.prev.y;
            struct gui_vec2 window_size = gui_style_property(config, GUI_PROPERTY_SIZE);
            incursor = GUI_INBOX(prev_x,prev_y,scaler_x,scaler_y,scaler_w,scaler_h);

            if (gui_input_is_mouse_down(in, GUI_BUTTON_LEFT) && incursor) {
                window->bounds.w = MAX(window_size.x, window->bounds.w + in->mouse.delta.x);
                /* draging in y-direction is only possible if static window */
                if (!(layout->flags & GUI_WINDOW_DYNAMIC))
                    window->bounds.h = MAX(window_size.y, window->bounds.h + in->mouse.delta.y);
            }
        }
    }

    if (layout->flags & GUI_WINDOW_BORDER) {
        /* draw the border around the complete panel */
        const gui_float width = (layout->flags & GUI_WINDOW_NO_SCROLLBAR) ?
            layout->width: layout->width + scrollbar_size;
        const gui_float padding_y = (!layout->valid) ?
                window->bounds.y + layout->header.h:
                (layout->flags & GUI_WINDOW_DYNAMIC) ?
                layout->footer_h + footer.y:
                layout->bounds.y + layout->bounds.h;

        if (window->flags & GUI_WINDOW_BORDER_HEADER)
            gui_command_buffer_push_line(out, window->bounds.x, window->bounds.y + layout->header.h,
                window->bounds.x + window->bounds.w, window->bounds.y + layout->header.h,
                config->colors[GUI_COLOR_BORDER]);
        gui_command_buffer_push_line(out, window->bounds.x, padding_y, window->bounds.x + width,
                padding_y, config->colors[GUI_COLOR_BORDER]);
        gui_command_buffer_push_line(out, window->bounds.x, window->bounds.y, window->bounds.x,
                padding_y, config->colors[GUI_COLOR_BORDER]);
        gui_command_buffer_push_line(out, window->bounds.x + width, window->bounds.y,
                window->bounds.x + width, padding_y, config->colors[GUI_COLOR_BORDER]);
    }

    gui_command_buffer_push_scissor(out, gui_rect(0, 0, gui_null_rect.w, gui_null_rect.h));
    if (!(window->flags & GUI_WINDOW_TAB)) {
        /* window is hidden so clear command buffer  */
        if (layout->flags & GUI_WINDOW_HIDDEN)
            gui_command_buffer_reset(&window->buffer);
        /* window is visible and not tab */
        else gui_command_queue_finish(window->queue, &window->buffer);
    }

    /* the remove ROM flag was set so remove ROM FLAG*/
    if (layout->flags & GUI_WINDOW_REMOVE_ROM) {
        layout->flags &= ~(gui_flags)GUI_WINDOW_ROM;
        layout->flags &= ~(gui_flags)GUI_WINDOW_REMOVE_ROM;
    }
    window->flags = layout->flags;
}

struct gui_command_buffer*
gui_canvas(struct gui_context *layout)
{return layout->buffer;}

const struct gui_input*
gui_input(struct gui_context *layout)
{return layout->input;}

struct gui_command_queue*
gui_queue(struct gui_context *layout)
{return layout->queue;}
/*
 * -------------------------------------------------------------
 *
 *                          Header
 *
 * --------------------------------------------------------------
 */
void
gui_header_begin(struct gui_context *layout)
{
    const struct gui_style *c;
    struct gui_vec2 item_padding;
    struct gui_vec2 item_spacing;
    struct gui_vec2 panel_padding;
    struct gui_command_buffer *out;

    GUI_ASSERT(layout);
    if (!layout) return;
    if (layout->flags & GUI_WINDOW_HIDDEN)
        return;

    c = layout->style;
    out = layout->buffer;

    /* cache some configuration data */
    panel_padding = gui_style_property(c, GUI_PROPERTY_PADDING);
    item_padding = gui_style_property(c, GUI_PROPERTY_ITEM_PADDING);
    item_spacing = gui_style_property(c, GUI_PROPERTY_ITEM_SPACING);

    /* update the header height and first row height */
    layout->header.h = c->font.height + 4 * item_padding.y;
    layout->header.h += panel_padding.y;
    layout->row.height += layout->header.h;
    if (layout->valid)
        gui_command_buffer_push_rect(out,
            gui_rect(layout->bounds.x, layout->bounds.y + layout->header.h,
            layout->bounds.w, layout->row.height), 0, c->colors[GUI_COLOR_WINDOW]);

    /* setup header bounds and growable icon space */
    layout->header.x = layout->bounds.x + panel_padding.x;
    layout->header.y = layout->bounds.y + panel_padding.y;
    layout->header.w = MAX(layout->bounds.w, 2 * panel_padding.x);
    layout->header.w -= 2 * panel_padding.x;
    layout->header.front = layout->header.x;
    layout->header.back = layout->header.x + layout->header.w;
    layout->height = layout->bounds.h - (layout->header.h + 2 * item_spacing.y);
    layout->height -= layout->footer_h;
    gui_command_buffer_push_rect(out, gui_rect(layout->bounds.x, layout->bounds.y,
        layout->bounds.w, layout->header.h), 0, c->colors[GUI_COLOR_HEADER]);
}

gui_bool
gui_header_button(struct gui_context *layout,
    enum gui_symbol symbol, enum gui_header_align align)
{
    /* calculate the position of the close icon position and draw it */
    struct gui_rect sym = {0,0,0,0};
    gui_float sym_bw = 0;
    gui_bool ret = gui_false;

    const struct gui_style *c;
    struct gui_command_buffer *out;
    struct gui_vec2 item_padding;

    GUI_ASSERT(layout);
    GUI_ASSERT(layout->buffer);
    GUI_ASSERT(layout->style);
    if (!layout || layout->flags & GUI_WINDOW_HIDDEN)
        return gui_false;

    /* cache configuration data */
    c = layout->style;
    out = layout->buffer;
    item_padding = gui_style_property(c, GUI_PROPERTY_ITEM_PADDING);

    sym.x = layout->header.front;
    sym.y = layout->header.y;

    switch (symbol) {
    case GUI_SYMBOL_MINUS:
    case GUI_SYMBOL_PLUS:
    case GUI_SYMBOL_UNDERSCORE:
    case GUI_SYMBOL_X: {
        /* single character text icon */
        const gui_char *X = (symbol == GUI_SYMBOL_X) ? "x":
            (symbol == GUI_SYMBOL_UNDERSCORE) ? "_":
            (symbol == GUI_SYMBOL_PLUS) ? "+": "-";
        const gui_size t = c->font.width(c->font.userdata, X, 1);
        const gui_float text_width = (gui_float)t;

        /* calculate bounds of the icon */
        sym_bw = text_width;
        sym.w = (gui_float)text_width + 2 * item_padding.x;
        sym.h = c->font.height + 2 * item_padding.y;
        if (align == GUI_HEADER_RIGHT)
            sym.x = layout->header.back - sym.w;
        gui_command_buffer_push_text(out, sym, X, 1, &c->font, c->colors[GUI_COLOR_HEADER],
                                        c->colors[GUI_COLOR_TEXT]);
        } break;
    case GUI_SYMBOL_CIRCLE_FILLED:
    case GUI_SYMBOL_CIRCLE:
    case GUI_SYMBOL_RECT_FILLED:
    case GUI_SYMBOL_RECT: {
        /* simple empty/filled shapes */
        sym_bw = sym.w = c->font.height;
        sym.h = c->font.height;
        sym.y = sym.y + c->font.height/2;
        if (align == GUI_HEADER_RIGHT)
            sym.x = layout->header.back - (c->font.height + 2 * item_padding.x);

        if (symbol == GUI_SYMBOL_RECT || symbol == GUI_SYMBOL_RECT_FILLED) {
            /* rectangle shape  */
            gui_command_buffer_push_rect(out, sym,  0, c->colors[GUI_COLOR_TEXT]);
            if (symbol == GUI_SYMBOL_RECT_FILLED)
                gui_command_buffer_push_rect(out, gui_shrink_rect(sym, 1),
                    0, c->colors[GUI_COLOR_HEADER]);
        } else {
            /* circle shape  */
            gui_command_buffer_push_circle(out, sym, c->colors[GUI_COLOR_TEXT]);
            if (symbol == GUI_SYMBOL_CIRCLE_FILLED)
                gui_command_buffer_push_circle(out, gui_shrink_rect(sym, 1),
                    c->colors[GUI_COLOR_HEADER]);
        }

        /* calculate the space the icon occupied */
        sym.w = c->font.height + 2 * item_padding.x;
        } break;
    case GUI_SYMBOL_TRIANGLE_UP:
    case GUI_SYMBOL_TRIANGLE_DOWN:
    case GUI_SYMBOL_TRIANGLE_LEFT:
    case GUI_SYMBOL_TRIANGLE_RIGHT: {
        /* triangle icon with direction */
        enum gui_heading heading;
        struct gui_vec2 points[3];
        heading = (symbol == GUI_SYMBOL_TRIANGLE_RIGHT) ? GUI_RIGHT :
            (symbol == GUI_SYMBOL_TRIANGLE_LEFT) ? GUI_LEFT:
            (symbol == GUI_SYMBOL_TRIANGLE_UP) ? GUI_UP: GUI_DOWN;

        /* calculate bounds of the icon */
        sym_bw = sym.w = c->font.height;
        sym.h = c->font.height;
        sym.y = sym.y + c->font.height/2;
        if (align == GUI_HEADER_RIGHT)
            sym.x = layout->header.back - (c->font.height + 2 * item_padding.x);

        /* calculate the triangle point positions and draw triangle */
        gui_triangle_from_direction(points, sym, 0, 0, heading);
        gui_command_buffer_push_triangle(layout->buffer,  points[0].x, points[0].y,
            points[1].x, points[1].y, points[2].x, points[2].y, c->colors[GUI_COLOR_TEXT]);

        /* calculate the space the icon occupied */
        sym.w = c->font.height + 2 * item_padding.x;
        } break;
    case GUI_SYMBOL_MAX:
    default: return ret;
    }

    /* check if the icon has been pressed */
    if (!(layout->flags & GUI_WINDOW_ROM)) {
        gui_float mouse_x = layout->input->mouse.pos.x;
        gui_float mouse_y = layout->input->mouse.pos.y;
        gui_float clicked_x = layout->input->mouse.buttons[GUI_BUTTON_LEFT].clicked_pos.x;
        gui_float clicked_y = layout->input->mouse.buttons[GUI_BUTTON_LEFT].clicked_pos.y;
        if (GUI_INBOX(mouse_x, mouse_y, sym.x, sym.y, sym_bw, sym.h)) {
            if (GUI_INBOX(clicked_x, clicked_y, sym.x, sym.y, sym_bw, sym.h))
                ret = (layout->input->mouse.buttons[GUI_BUTTON_LEFT].down &&
                        layout->input->mouse.buttons[GUI_BUTTON_LEFT].clicked);
        }
    }

    /* update the header space */
    if (align == GUI_HEADER_RIGHT)
        layout->header.back -= (sym.w + item_padding.x);
    else layout->header.front += sym.w + item_padding.x;
    return ret;
}

gui_bool
gui_header_toggle(struct gui_context *layout,
    enum gui_symbol inactive, enum gui_symbol active,
    enum gui_header_align align, gui_bool state)
{
    gui_bool ret = gui_header_button(layout, (state) ? active : inactive, align);
    if (ret) return !state;
    return state;
}

gui_bool
gui_header_flag(struct gui_context *layout, enum gui_symbol inactive,
    enum gui_symbol active, enum gui_header_align align,
    enum gui_window_flags flag)
{
    gui_flags flags = layout->flags;
    gui_bool state = (flags & flag) ? gui_true : gui_false;
    gui_bool ret = gui_header_toggle(layout, inactive, active, align, state);
    if (ret != ((flags & flag) ? gui_true : gui_false)) {
        /* the state of the toggle icon has been changed  */
        if (!ret) layout->flags &= ~flag;
        else layout->flags |= flag;
        /* update the state of the panel since the flag have changed */
        layout->valid = !(layout->flags & GUI_WINDOW_HIDDEN) &&
                        !(layout->flags & GUI_WINDOW_MINIMIZED);
        return gui_true;
    }
    return gui_false;
}

void
gui_header_title(struct gui_context *layout, const char *title,
    enum gui_header_align align)
{
    struct gui_rect label = {0,0,0,0};
    const struct gui_style *c;
    struct gui_command_buffer *out;
    struct gui_vec2 item_padding;
    gui_size text_len;
    gui_size t;

    /* make sure correct values and layout state */
    GUI_ASSERT(layout);
    if (!layout || !title) return;
    if (layout->flags & GUI_WINDOW_HIDDEN)
        return;

    /* cache configuration and title length */
    c = layout->style;
    out = layout->buffer;
    item_padding = gui_style_property(c, GUI_PROPERTY_ITEM_PADDING);
    text_len = gui_strsiz(title);

    /* calculate and allocate space from the header */
    t = c->font.width(c->font.userdata, title, text_len);
    if (align == GUI_HEADER_RIGHT) {
        layout->header.back = layout->header.back - (3 * item_padding.x + (gui_float)t);
        label.x = layout->header.back;
    } else {
        label.x = layout->header.front;
        layout->header.front += 3 * item_padding.x + (gui_float)t;
    }

    /* calculate label bounds and draw text */
    label.y = layout->header.y;
    label.h = c->font.height + 2 * item_padding.y;
    if (align == GUI_HEADER_LEFT)
        label.w = MAX((gui_float)t + 2 * item_padding.x, 3 * item_padding.x);
    else label.w = MAX((gui_float)t + 2 * item_padding.x, 3 * item_padding.x);
    label.w -= (3 * item_padding.x);
    gui_command_buffer_push_text(out, label, (const gui_char*)title, text_len,
        &c->font, c->colors[GUI_COLOR_HEADER], c->colors[GUI_COLOR_TEXT]);
}

void
gui_header_end(struct gui_context *layout)
{
    const struct gui_style *c;
    struct gui_command_buffer *out;
    struct gui_vec2 item_padding;
    struct gui_vec2 panel_padding;

    GUI_ASSERT(layout);
    if (!layout) return;
    if (layout->flags & GUI_WINDOW_HIDDEN)
        return;

    /* cache configuration data */
    c = layout->style;
    out = layout->buffer;
    panel_padding = gui_style_property(c, GUI_PROPERTY_PADDING);
    item_padding = gui_style_property(c, GUI_PROPERTY_ITEM_PADDING);

    /* draw panel header border */
    if (layout->flags & GUI_WINDOW_BORDER) {
        gui_float scrollbar_width = gui_style_property(c, GUI_PROPERTY_SCROLLBAR_SIZE).x;
        const gui_float width = layout->width + scrollbar_width;

        /* draw the header border lines */
        gui_command_buffer_push_line(out, layout->bounds.x, layout->bounds.y, layout->bounds.x,
                layout->bounds.y + layout->header.h, c->colors[GUI_COLOR_BORDER]);
        gui_command_buffer_push_line(out, layout->bounds.x + width, layout->bounds.y,
                layout->bounds.x + width, layout->bounds.y + layout->header.h,
                c->colors[GUI_COLOR_BORDER]);
        if (layout->flags & GUI_WINDOW_BORDER_HEADER)
            gui_command_buffer_push_line(out, layout->bounds.x, layout->bounds.y + layout->header.h,
                layout->bounds.x + layout->bounds.w, layout->bounds.y + layout->header.h,
                c->colors[GUI_COLOR_BORDER]);
    }

    /* update the panel clipping rect to include the header */
    layout->clip.x = layout->bounds.x;
    layout->clip.w = layout->bounds.w;
    layout->clip.y = layout->bounds.y + layout->header.h;
    layout->clip.h = layout->bounds.h - (layout->footer_h + layout->header.h);
    layout->clip.h -= (panel_padding.y + item_padding.y);
    gui_command_buffer_push_scissor(out, layout->clip);
}

gui_flags
gui_header(struct gui_context *layout, const char *title,
    gui_flags flags, gui_flags notify, enum gui_header_align align)
{
    gui_flags ret = 0;
    gui_flags old = layout->flags;
    GUI_ASSERT(layout);
    if (!layout || layout->flags & GUI_WINDOW_HIDDEN)
        return gui_false;

    /* basic standart header with fixed icon/title sequence */
    gui_header_begin(layout);
    {
        if (flags & GUI_CLOSEABLE)
            gui_header_flag(layout, GUI_SYMBOL_X, GUI_SYMBOL_X,
                align, GUI_WINDOW_HIDDEN);
        if (flags & GUI_MINIMIZABLE)
            gui_header_flag(layout, GUI_SYMBOL_MINUS, GUI_SYMBOL_PLUS,
                align, GUI_WINDOW_MINIMIZED);
        if (flags & GUI_SCALEABLE)
            gui_header_flag(layout, GUI_SYMBOL_RECT, GUI_SYMBOL_RECT_FILLED,
                align, GUI_WINDOW_SCALEABLE);
        if (flags & GUI_MOVEABLE)
            gui_header_flag(layout, GUI_SYMBOL_CIRCLE,
                GUI_SYMBOL_CIRCLE_FILLED, align, GUI_WINDOW_MOVEABLE);
        if (title) gui_header_title(layout, title, GUI_HEADER_LEFT);
    }
    gui_header_end(layout);

    /* notifcation if one if the icon buttons has been pressed */
    if ((notify & GUI_CLOSEABLE) && ((old & GUI_CLOSEABLE) ^ (layout->flags & GUI_CLOSEABLE)))
        ret |= GUI_CLOSEABLE;
    if ((notify & GUI_MINIMIZABLE) && ((old & GUI_MINIMIZABLE)^(layout->flags&GUI_MINIMIZABLE)))
        ret |= GUI_MINIMIZABLE;
    if ((notify & GUI_SCALEABLE) && ((old & GUI_SCALEABLE) ^ (layout->flags & GUI_SCALEABLE)))
        ret |= GUI_SCALEABLE;
    if ((notify & GUI_MOVEABLE) && ((old & GUI_MOVEABLE) ^ (layout->flags & GUI_MOVEABLE)))
        ret |= GUI_MOVEABLE;
    return ret;
}

void
gui_menubar_begin(struct gui_context *layout)
{
    GUI_ASSERT(layout);
    if (!layout || layout->flags & GUI_WINDOW_HIDDEN || layout->flags & GUI_WINDOW_MINIMIZED)
        return;
    layout->menu.x = layout->at_x;
    layout->menu.y = layout->bounds.y + layout->header.h;
    layout->menu.w = layout->width;
    layout->menu.offset = layout->offset;
    layout->offset.y = 0;
}

void
gui_menubar_end(struct gui_context *layout)
{
    const struct gui_style *c;
    struct gui_command_buffer *out;
    struct gui_vec2 item_padding;
    struct gui_vec2 panel_padding;

    GUI_ASSERT(layout);
    if (!layout || layout->flags & GUI_WINDOW_HIDDEN || layout->flags & GUI_WINDOW_MINIMIZED)
        return;

    c = layout->style;
    out = layout->buffer;
    panel_padding = gui_style_property(c, GUI_PROPERTY_PADDING);
    item_padding = gui_style_property(c, GUI_PROPERTY_ITEM_PADDING);

    layout->menu.h = (layout->at_y + layout->row.height-1) - layout->menu.y;
    layout->clip.y = layout->bounds.y + layout->header.h + layout->menu.h;
    layout->height -= layout->menu.h;
    layout->offset = layout->menu.offset;
    layout->clip.h = layout->bounds.h - (layout->footer_h + layout->header.h + layout->menu.h);
    layout->clip.h -= (panel_padding.y + item_padding.y);
    gui_command_buffer_push_scissor(out, layout->clip);
}
/*
 * -------------------------------------------------------------
 *
 *                          LAYOUT
 *
 * --------------------------------------------------------------
 */
static void
gui_panel_layout(struct gui_context *layout, gui_float height, gui_size cols)
{
    const struct gui_style *config;
    const struct gui_color *color;
    struct gui_command_buffer *out;
    struct gui_vec2 item_spacing;
    struct gui_vec2 panel_padding;

    GUI_ASSERT(layout);
    GUI_ASSERT(layout->style);
    GUI_ASSERT(layout->buffer);

    if (!layout) return;
    if (!layout->valid) return;

    /* prefetch some configuration data */
    config = layout->style;
    out = layout->buffer;
    color = &config->colors[GUI_COLOR_WINDOW];
    item_spacing = gui_style_property(config, GUI_PROPERTY_ITEM_SPACING);
    panel_padding = gui_style_property(config, GUI_PROPERTY_PADDING);

    /* draw the current row and set the current row layout */
    layout->row.index = 0;
    layout->at_y += layout->row.height;
    layout->row.columns = cols;
    layout->row.height = height + item_spacing.y;
    layout->row.item_offset = 0;
    if (layout->flags & GUI_WINDOW_DYNAMIC)
        gui_command_buffer_push_rect(out,  gui_rect(layout->bounds.x, layout->at_y,
            layout->bounds.w, height + panel_padding.y), 0, *color);
}

static void
gui_row_layout(struct gui_context *layout,
    enum gui_layout_format fmt, gui_float height, gui_size cols,
    gui_size width)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(layout->style);
    GUI_ASSERT(layout->buffer);

    if (!layout) return;
    if (!layout->valid) return;

    /* draw the current row and set the current row layout */
    gui_panel_layout(layout, height, cols);
    if (fmt == GUI_DYNAMIC)
        layout->row.type = GUI_LAYOUT_DYNAMIC_FIXED;
    else layout->row.type = GUI_LAYOUT_STATIC_FIXED;

    layout->row.item_width = (gui_float)width;
    layout->row.ratio = 0;
    layout->row.item_offset = 0;
    layout->row.filled = 0;
}

void
gui_layout_row_dynamic(struct gui_context *layout, gui_float height, gui_size cols)
{gui_row_layout(layout, GUI_DYNAMIC, height, cols, 0);}

void
gui_layout_row_static(struct gui_context *layout, gui_float height,
    gui_size item_width, gui_size cols)
{gui_row_layout(layout, GUI_STATIC, height, cols, item_width);}

void
gui_layout_row_begin(struct gui_context *layout,
    enum gui_layout_format fmt, gui_float row_height, gui_size cols)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(layout->style);
    GUI_ASSERT(layout->buffer);

    if (!layout) return;
    if (!layout->valid) return;

    gui_panel_layout(layout, row_height, cols);
    if (fmt == GUI_DYNAMIC)
        layout->row.type = GUI_LAYOUT_DYNAMIC_ROW;
    else layout->row.type = GUI_LAYOUT_STATIC_ROW;
    layout->row.ratio = 0;
    layout->row.item_width = 0;
    layout->row.item_offset = 0;
    layout->row.filled = 0;
    layout->row.columns = cols;
}

void
gui_layout_row_push(struct gui_context *layout, gui_float ratio_or_width)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(layout->style);
    if (!layout || !layout->valid) return;

    if (layout->row.type == GUI_LAYOUT_DYNAMIC_ROW) {
        gui_float ratio = ratio_or_width;
        if ((ratio + layout->row.filled) > 1.0f) return;
        if (ratio > 0.0f)
            layout->row.item_width = GUI_SATURATE(ratio);
        else layout->row.item_width = 1.0f - layout->row.filled;
    } else layout->row.item_width = ratio_or_width;
}

void
gui_layout_row_end(struct gui_context *layout)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(layout->style);
    if (!layout) return;
    if (!layout->valid) return;
    layout->row.item_width = 0;
    layout->row.item_offset = 0;
}

void
gui_layout_row(struct gui_context *layout, enum gui_layout_format fmt,
    gui_float height, gui_size cols, const gui_float *ratio)
{
    gui_size i;
    gui_size n_undef = 0;
    gui_float r = 0;

    GUI_ASSERT(layout);
    GUI_ASSERT(layout->style);
    GUI_ASSERT(layout->buffer);

    if (!layout) return;
    if (!layout->valid) return;

    gui_panel_layout(layout, height, cols);
    if (fmt == GUI_DYNAMIC) {
        /* calculate width of undefined widget ratios */
        layout->row.ratio = ratio;
        for (i = 0; i < cols; ++i) {
            if (ratio[i] < 0.0f)
                n_undef++;
            else r += ratio[i];
        }
        r = GUI_SATURATE(1.0f - r);
        layout->row.type = GUI_LAYOUT_DYNAMIC;
        layout->row.item_width = (r > 0 && n_undef > 0) ? (r / (gui_float)n_undef):0;
    } else {
        layout->row.ratio = ratio;
        layout->row.type = GUI_LAYOUT_STATIC;
        layout->row.item_width = 0;
        layout->row.item_offset = 0;
    }

    layout->row.item_offset = 0;
    layout->row.filled = 0;
}

void
gui_layout_row_space_begin(struct gui_context *layout,
    enum gui_layout_format fmt, gui_float height, gui_size widget_count)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(layout->style);
    GUI_ASSERT(layout->buffer);

    if (!layout) return;
    if (!layout->valid) return;

    gui_panel_layout(layout, height, widget_count);
    if (fmt == GUI_STATIC) {
        /* calculate bounds of the free to use panel space */
        struct gui_rect clip, space;
        space.x = layout->at_x;
        space.y = layout->at_y;
        space.w = layout->width;
        space.h = layout->row.height;

        /* setup clipping rect for the free space to prevent overdraw  */
        gui_unify(&clip, &layout->clip, space.x, space.y, space.x + space.w, space.y + space.h);
        gui_command_buffer_push_scissor(layout->buffer, clip);

        layout->row.type = GUI_LAYOUT_STATIC_FREE;
        layout->row.clip = layout->clip;
        layout->clip = clip;
    } else layout->row.type = GUI_LAYOUT_DYNAMIC_FREE;

    layout->row.ratio = 0;
    layout->row.item_width = 0;
    layout->row.item_offset = 0;
    layout->row.filled = 0;
}

void
gui_layout_row_space_push(struct gui_context *layout, struct gui_rect rect)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(layout->style);
    GUI_ASSERT(layout->buffer);
    if (!layout) return;
    if (!layout->valid) return;
    layout->row.item = rect;
}

struct gui_rect
gui_layout_row_space_bounds(struct gui_context *ctx)
{
    struct gui_rect ret;
    ret.x = ctx->clip.x;
    ret.y = ctx->clip.y;
    ret.w = ctx->clip.w;
    ret.h = ctx->row.height;
    return ret;
}

struct gui_vec2
gui_layout_row_space_to_screen(struct gui_context *layout, struct gui_vec2 ret)
{
    GUI_ASSERT(layout);
    ret.x = ret.x + layout->clip.x + layout->offset.x;
    ret.y = ret.y + layout->clip.y + layout->offset.y;
    return ret;
}

struct gui_vec2
gui_layout_row_space_to_local(struct gui_context *layout, struct gui_vec2 ret)
{
    GUI_ASSERT(layout);
    ret.x = ret.x - (layout->clip.x + layout->offset.x);
    ret.y = ret.y - (layout->clip.y + layout->offset.y);
    return ret;
}

struct gui_rect
gui_layout_row_space_rect_to_screen(struct gui_context *layout, struct gui_rect ret)
{
    GUI_ASSERT(layout);
    ret.x = ret.x + layout->clip.x + layout->offset.x;
    ret.y = ret.y + layout->clip.y + layout->offset.y;
    return ret;
}

struct gui_rect
gui_layout_row_space_rect_to_local(struct gui_context *layout, struct gui_rect ret)
{
    GUI_ASSERT(layout);
    ret.x = ret.x - (layout->clip.x + layout->offset.x);
    ret.y = ret.y - (layout->clip.y + layout->offset.y);
    return ret;
}

void
gui_layout_row_space_end(struct gui_context *layout)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(layout->style);
    if (!layout) return;
    if (!layout->valid) return;

    layout->row.item_width = 0;
    layout->row.item_height = 0;
    layout->row.item_offset = 0;
    gui_zero(&layout->row.item, sizeof(layout->row.item));
    if (layout->row.type == GUI_LAYOUT_STATIC_FREE)
        gui_command_buffer_push_scissor(layout->buffer, layout->clip);
}

void
gui_layout_row_tiled_begin(struct gui_context *layout,
    struct gui_tiled_layout *tiled)
{
    GUI_ASSERT(layout);
    if (!layout) return;
    if (!layout->valid) return;

    layout->row.tiled = tiled;
    gui_panel_layout(layout, tiled->height, 2);
    layout->row.type = (tiled->fmt == GUI_STATIC) ?
        GUI_LAYOUT_STATIC_TILED:
        GUI_LAYOUT_DYNAMIC_TILED;
}

void
gui_layout_row_tiled_push(struct gui_context *layout,
    enum gui_tiled_layout_slot_index slot, gui_uint index)
{
    struct gui_rect slot_bounds;
    const struct gui_tiled_slot *s;
    struct gui_vec2 spacing;
    struct gui_vec2 padding;
    const struct gui_style *config;
    struct gui_tiled_layout *tiled;
    gui_float temp;

    GUI_ASSERT(layout);
    GUI_ASSERT(layout->style);
    GUI_ASSERT(layout->buffer);
    GUI_ASSERT(layout->row.tiled);
    if (!layout) return;
    if (!layout->valid) return;

    GUI_ASSERT(layout);
    if (!layout) return;

    tiled = layout->row.tiled;
    s = &tiled->slots[slot];
    GUI_ASSERT(index < s->capacity);

    config = layout->style;
    spacing = config->properties[GUI_PROPERTY_ITEM_SPACING];
    padding = config->properties[GUI_PROPERTY_PADDING];

    temp = tiled->width;
    if (tiled->fmt == GUI_DYNAMIC)
        tiled->width = layout->width - (2 * padding.x);
    gui_tiled_slot_bounds(&slot_bounds, tiled, slot);
    tiled->width = temp;

    if (s->format == GUI_SLOT_HORIZONTAL) {
        slot_bounds.h -= (2 * spacing.y);
        slot_bounds.w -= (gui_float)s->capacity * spacing.x;

        layout->row.item.h = slot_bounds.h;
        layout->row.item.y = slot_bounds.y;
        layout->row.item.w = slot_bounds.w / (gui_float)s->capacity;
        layout->row.item.x = slot_bounds.x + (gui_float)index * layout->row.item.w;
    } else {
        layout->row.item.x = slot_bounds.x + spacing.x;
        layout->row.item.w = slot_bounds.w - (2 * spacing.x);
        layout->row.item.h = (slot_bounds.h - (gui_float)s->capacity * spacing.y);
        layout->row.item.h /= (gui_float)s->capacity;
        layout->row.item.y = slot_bounds.y + (gui_float)index * layout->row.item.h;
        layout->row.item.y += ((gui_float)index * spacing.y);
    }
}

void
gui_layout_row_tiled_end(struct gui_context *layout)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(layout->style);
    if (!layout) return;
    if (!layout->valid) return;

    gui_zero(&layout->row.item, sizeof(layout->row.item));
    layout->row.tiled = 0;
    layout->row.columns = 0;
}

static void
gui_panel_alloc_row(struct gui_context *layout)
{
    const struct gui_style *c = layout->style;
    struct gui_vec2 spacing = gui_style_property(c, GUI_PROPERTY_ITEM_SPACING);
    const gui_float row_height = layout->row.height - spacing.y;
    gui_panel_layout(layout, row_height, layout->row.columns);
}

static void
gui_panel_alloc_space(struct gui_rect *bounds, struct gui_context *layout)
{
    const struct gui_style *config;
    gui_float panel_padding, panel_spacing, panel_space;
    gui_float item_offset = 0, item_width = 0, item_spacing = 0;
    struct gui_vec2 spacing;
    struct gui_vec2 padding;

    GUI_ASSERT(layout);
    GUI_ASSERT(layout->style);
    GUI_ASSERT(bounds);
    if (!layout || !layout->style || !bounds)
        return;

    /* cache some configuration data */
    config = layout->style;
    spacing = gui_style_property(config, GUI_PROPERTY_ITEM_SPACING);
    padding = gui_style_property(config, GUI_PROPERTY_PADDING);

    /* check if the end of the row has been hit and begin new row if so */
    if (layout->row.index >= layout->row.columns)
        gui_panel_alloc_row(layout);

    /* calculate the useable panel space */
    panel_padding = 2 * padding.x;
    panel_spacing = (gui_float)(layout->row.columns - 1) * spacing.x;
    panel_space  = layout->width - panel_padding - panel_spacing;

    /* calculate the width of one item inside the panel row */
    switch (layout->row.type) {
    case GUI_LAYOUT_DYNAMIC_FIXED: {
        /* scaling fixed size widgets item width */
        item_width = panel_space / (gui_float)layout->row.columns;
        item_offset = (gui_float)layout->row.index * item_width;
        item_spacing = (gui_float)layout->row.index * spacing.x;
    } break;
    case GUI_LAYOUT_DYNAMIC_ROW: {
        /* scaling single ratio widget width */
        item_width = layout->row.item_width * panel_space;
        item_offset = layout->row.item_offset;
        item_spacing = (gui_float)layout->row.index * spacing.x;

        layout->row.item_offset += item_width + spacing.x;
        layout->row.filled += layout->row.item_width;
        layout->row.index = 0;
    } break;
    case GUI_LAYOUT_STATIC_TILED:
    case GUI_LAYOUT_DYNAMIC_TILED: {
        /* dynamic tiled layout widget placing */
        bounds->x = layout->at_x + layout->row.item.x + padding.x;
        bounds->x -= layout->offset.x;
        bounds->y = layout->at_y + layout->row.item.y;
        bounds->y -= layout->offset.y;
        bounds->w = layout->row.item.w;
        bounds->h = layout->row.item.h;
        layout->row.index = 0;
        if ((bounds->x + bounds->w) > layout->max_x)
            layout->max_x = bounds->x + bounds->w;
        return;
    } break;
    case GUI_LAYOUT_DYNAMIC_FREE: {
        /* panel width depended free widget placing */
        bounds->x = layout->at_x + (layout->width * layout->row.item.x);
        bounds->x -= layout->offset.x;
        bounds->y = layout->at_y + (layout->row.height * layout->row.item.y);
        bounds->w = layout->width  * layout->row.item.w;
        bounds->h = layout->row.height * layout->row.item.h;
        return;
    } break;
    case GUI_LAYOUT_DYNAMIC: {
        /* scaling arrays of panel width ratios for every widget */
        gui_float ratio;
        GUI_ASSERT(layout->row.ratio);
        ratio = (layout->row.ratio[layout->row.index] < 0) ?
            layout->row.item_width : layout->row.ratio[layout->row.index];

        item_spacing = (gui_float)layout->row.index * spacing.x;
        if (layout->row.index < layout->row.columns-1)
            item_width = (ratio * panel_space) - spacing.x;
        else item_width = (ratio * panel_space);

        item_offset = layout->row.item_offset;
        layout->row.item_offset += item_width + spacing.x;
        layout->row.filled += ratio;
    } break;
    case GUI_LAYOUT_STATIC_FIXED: {
        /* non-scaling fixed widgets item width */
        item_width = layout->row.item_width;
        item_offset = (gui_float)layout->row.index * item_width;
        item_spacing = (gui_float)layout->row.index * spacing.x;
    } break;
    case GUI_LAYOUT_STATIC_ROW: {
        /* scaling single ratio widget width */
        item_width = layout->row.item_width;
        item_offset = layout->row.item_offset;
        item_spacing = (gui_float)layout->row.index * spacing.x;
        layout->row.item_offset += item_width + spacing.x;
        layout->row.index = 0;
    } break;
    case GUI_LAYOUT_STATIC_FREE: {
        /* free widget placing */
        bounds->x = layout->clip.x + layout->row.item.x;
        bounds->x -= layout->offset.x;
        bounds->y = layout->clip.y + layout->row.item.y;
        bounds->y -= layout->offset.y;
        bounds->w = layout->row.item.w;
        bounds->h = layout->row.item.h;
        if ((bounds->x + bounds->w) > layout->max_x)
            layout->max_x = bounds->x + bounds->w;
        return;
    } break;
    case GUI_LAYOUT_STATIC: {
        /* non-scaling array of panel pixel width for every widget */
        item_spacing = (gui_float)layout->row.index * spacing.x;
        item_width = layout->row.ratio[layout->row.index];
        item_offset = layout->row.item_offset;
        layout->row.item_offset += item_width + spacing.x;
        } break;
    default: break;
    };

    /* set the bounds of the newly allocated widget */
    bounds->x = layout->at_x + item_offset + item_spacing + padding.x - layout->offset.x;
    bounds->y = layout->at_y - layout->offset.y;
    bounds->w = item_width;
    bounds->h = layout->row.height - spacing.y;

    layout->row.index++;
    if ((bounds->x + bounds->w) > layout->max_x)
        layout->max_x = bounds->x + bounds->w;
}

gui_bool
gui_layout_push(struct gui_context *layout,
    enum gui_layout_node_type type,
    const char *title, gui_state *state)
{
    const struct gui_style *config;
    struct gui_command_buffer *out;
    struct gui_vec2 item_spacing;
    struct gui_vec2 item_padding;
    struct gui_vec2 panel_padding;
    struct gui_rect header = {0,0,0,0};
    struct gui_rect sym = {0,0,0,0};

    GUI_ASSERT(layout);
    GUI_ASSERT(layout->style);
    GUI_ASSERT(layout->buffer);

    if (!layout) return gui_false;
    if (!layout->valid) return gui_false;

    /* cache some data */
    out = layout->buffer;
    config = layout->style;

    item_spacing = gui_style_property(config, GUI_PROPERTY_ITEM_SPACING);
    item_padding = gui_style_property(config, GUI_PROPERTY_ITEM_PADDING);
    panel_padding = gui_style_property(config, GUI_PROPERTY_PADDING);

    /* calculate header bounds and draw background */
    gui_layout_row_dynamic(layout, config->font.height + 4 * item_padding.y, 1);
    gui_panel_alloc_space(&header, layout);
    if (type == GUI_LAYOUT_TAB)
        gui_command_buffer_push_rect(out, header, 0, config->colors[GUI_COLOR_TAB_HEADER]);

    {
        /* and draw closing/open icon */
        enum gui_heading heading;
        struct gui_vec2 points[3];
        heading = (*state == GUI_MAXIMIZED) ? GUI_DOWN : GUI_RIGHT;

        /* calculate the triangle bounds */
        sym.w = sym.h = config->font.height;
        sym.y = header.y + item_padding.y + config->font.height/2;
        sym.x = header.x + panel_padding.x;

        /* calculate the triangle points and draw triangle */
        gui_triangle_from_direction(points, sym, 0, 0, heading);
        gui_command_buffer_push_triangle(layout->buffer,  points[0].x, points[0].y,
            points[1].x, points[1].y, points[2].x, points[2].y, config->colors[GUI_COLOR_TEXT]);

        /* calculate the space the icon occupied */
        sym.w = config->font.height + 2 * item_padding.x;
    }
    {
        /* draw node label */
        struct gui_color color;
        struct gui_rect label;
        gui_size text_len;

        header.w = MAX(header.w, sym.w + item_spacing.y + panel_padding.x);
        label.x = sym.x + sym.w + item_spacing.x;
        label.y = sym.y;
        label.w = header.w - (sym.w + item_spacing.y + panel_padding.x);
        label.h = config->font.height;
        text_len = gui_strsiz(title);
        color = (type == GUI_LAYOUT_TAB) ? config->colors[GUI_COLOR_TAB_HEADER]:
            config->colors[GUI_COLOR_WINDOW];
        gui_command_buffer_push_text(out, label, (const gui_char*)title, text_len,
            &config->font, color, config->colors[GUI_COLOR_TEXT]);
    }

    /* update node state */
    if (!(layout->flags & GUI_WINDOW_ROM)) {
        gui_float mouse_x = layout->input->mouse.pos.x;
        gui_float mouse_y = layout->input->mouse.pos.y;
        gui_float clicked_x = layout->input->mouse.buttons[GUI_BUTTON_LEFT].clicked_pos.x;
        gui_float clicked_y = layout->input->mouse.buttons[GUI_BUTTON_LEFT].clicked_pos.y;
        if (GUI_INBOX(mouse_x, mouse_y, sym.x, sym.y, sym.w, sym.h)) {
            if (GUI_INBOX(clicked_x, clicked_y, sym.x, sym.y, sym.w, sym.h)) {
                if (layout->input->mouse.buttons[GUI_BUTTON_LEFT].down &&
                    layout->input->mouse.buttons[GUI_BUTTON_LEFT].clicked)
                    *state = (*state == GUI_MAXIMIZED) ? GUI_MINIMIZED : GUI_MAXIMIZED;
            }
        }
    }

    if (type == GUI_LAYOUT_TAB) {
        /* special node with border around the header */
        gui_command_buffer_push_line(out, header.x, header.y,
            header.x + header.w, header.y, config->colors[GUI_COLOR_BORDER]);
        gui_command_buffer_push_line(out, header.x, header.y,
            header.x, header.y + header.h, config->colors[GUI_COLOR_BORDER]);
        gui_command_buffer_push_line(out, header.x + header.w, header.y,
            header.x + header.w, header.y + header.h, config->colors[GUI_COLOR_BORDER]);
        gui_command_buffer_push_line(out, header.x, header.y + header.h,
            header.x + header.w, header.y + header.h, config->colors[GUI_COLOR_BORDER]);
    }

    if (*state == GUI_MAXIMIZED) {
        layout->at_x = header.x;
        layout->width = MAX(layout->width, 2 * panel_padding.x);
        layout->width -= 2 * panel_padding.x;
        return gui_true;
    } else return gui_false;
}

void
gui_layout_pop(struct gui_context *layout)
{
    const struct gui_style *config;
    struct gui_vec2 panel_padding;

    GUI_ASSERT(layout);
    GUI_ASSERT(layout->style);
    GUI_ASSERT(layout->buffer);

    if (!layout) return;
    if (!layout->valid) return;

    config = layout->style;
    panel_padding = gui_style_property(config, GUI_PROPERTY_PADDING);
    layout->at_x -= panel_padding.x;
    layout->width += 2 * panel_padding.x;
}
/*
 * -------------------------------------------------------------
 *
 *                      PANEL WIDGETS
 *
 * --------------------------------------------------------------
 */
void
gui_spacing(struct gui_context *l, gui_size cols)
{
    gui_size i, n;
    gui_size index;
    struct gui_rect nil;

    GUI_ASSERT(l);
    GUI_ASSERT(l->style);
    GUI_ASSERT(l->buffer);
    if (!l) return;
    if (!l->valid) return;

    index = (l->row.index + cols) % l->row.columns;
    n = index - l->row.index;

    /* spacing over the row boundries */
    if (l->row.index + cols > l->row.columns) {
        gui_size rows = (l->row.index + cols) / l->row.columns;
        for (i = 0; i < rows; ++i)
            gui_panel_alloc_row(l);
    }

    /* non table layout need to allocate space */
    if (l->row.type != GUI_LAYOUT_DYNAMIC_FIXED &&
        l->row.type != GUI_LAYOUT_STATIC_FIXED) {
        for (i = 0; i < n; ++i)
            gui_panel_alloc_space(&nil, l);
    }
    l->row.index = index;
}

void
gui_seperator(struct gui_context *layout)
{
    struct gui_command_buffer *out;
    const struct gui_style *config;
    struct gui_vec2 item_padding;
    struct gui_vec2 item_spacing;
    struct gui_rect bounds;
    GUI_ASSERT(layout);
    if (!layout) return;

    out = layout->buffer;
    config = layout->style;
    item_padding = gui_style_property(config, GUI_PROPERTY_ITEM_PADDING);
    item_spacing = gui_style_property(config, GUI_PROPERTY_ITEM_SPACING);

    bounds.w = MAX(layout->width, 2 * item_spacing.x + 2 * item_padding.x);
    bounds.y = (layout->at_y + layout->row.height + item_padding.y) - layout->offset.y;
    bounds.x = layout->at_x + item_spacing.x + item_padding.x - layout->offset.x;
    bounds.w = bounds.w - (2 * item_spacing.x + 2 * item_padding.x);
    gui_command_buffer_push_line(out, bounds.x, bounds.y, bounds.x + bounds.w,
        bounds.y + bounds.h, config->colors[GUI_COLOR_BORDER]);
}

enum gui_widget_state
gui_widget(struct gui_rect *bounds, struct gui_context *layout)
{
    struct gui_rect *c = 0;
    GUI_ASSERT(layout);
    GUI_ASSERT(layout->style);
    GUI_ASSERT(layout->buffer);

    if (!layout) return GUI_WIDGET_INVALID;
    if (!layout->valid || !layout->style || !layout->buffer) return GUI_WIDGET_INVALID;

    /* allocated space for the panel and check if the widget needs to be updated */
    gui_panel_alloc_space(bounds, layout);
    c = &layout->clip;
    if (!GUI_INTERSECT(c->x, c->y, c->w, c->h, bounds->x, bounds->y, bounds->w, bounds->h))
        return GUI_WIDGET_INVALID;
    if (!GUI_CONTAINS(bounds->x, bounds->y, bounds->w, bounds->h, c->x, c->y, c->w, c->h))
        return GUI_WIDGET_ROM;
    return GUI_WIDGET_VALID;
}

void
gui_text_colored(struct gui_context *layout, const char *str, gui_size len,
    enum gui_text_align alignment, struct gui_color color)
{
    struct gui_rect bounds;
    struct gui_text text;
    const struct gui_style *config;
    struct gui_vec2 item_padding;

    GUI_ASSERT(layout);
    GUI_ASSERT(layout->style);
    GUI_ASSERT(layout->buffer);

    if (!layout) return;
    if (!layout->valid || !layout->style || !layout->buffer) return;
    gui_panel_alloc_space(&bounds, layout);
    config = layout->style;
    item_padding = gui_style_property(config, GUI_PROPERTY_ITEM_PADDING);

    text.padding.x = item_padding.x;
    text.padding.y = item_padding.y;
    text.background = config->colors[GUI_COLOR_WINDOW];
    text.text = color;
    gui_widget_text(layout->buffer, bounds, str, len, &text, alignment, &config->font);
}

void
gui_text(struct gui_context *l, const char *str, gui_size len,
    enum gui_text_align alignment)
{gui_text_colored(l, str, len, alignment,l->style->colors[GUI_COLOR_TEXT]);}

void
gui_label_colored(struct gui_context *layout, const char *text,
    enum gui_text_align align, struct gui_color color)
{gui_text_colored(layout, text, gui_strsiz(text), align, color);}

void
gui_label(struct gui_context *layout, const char *text,
    enum gui_text_align align)
{gui_text(layout, text, gui_strsiz(text), align);}

void
gui_image(struct gui_context *layout, struct gui_image img)
{
    const struct gui_style *config;
    struct gui_vec2 item_padding;
    struct gui_rect bounds;
    GUI_ASSERT(layout);
    if (!gui_widget(&bounds, layout))
        return;

    config = layout->style;
    item_padding = gui_style_property(config, GUI_PROPERTY_ITEM_PADDING);
    bounds.x += item_padding.x;
    bounds.y += item_padding.y;
    bounds.w -= 2 * item_padding.x;
    bounds.h -= 2 * item_padding.y;
    gui_command_buffer_push_image(layout->buffer, bounds, &img);
}

static void
gui_fill_button(const struct gui_style *config, struct gui_button *button)
{
    struct gui_vec2 item_padding;
    item_padding = gui_style_property(config, GUI_PROPERTY_ITEM_PADDING);
    button->rounding = config->rounding[GUI_ROUNDING_BUTTON];
    button->normal = config->colors[GUI_COLOR_BUTTON];
    button->hover = config->colors[GUI_COLOR_BUTTON_HOVER];
    button->active = config->colors[GUI_COLOR_BUTTON_ACTIVE];
    button->border = config->colors[GUI_COLOR_BORDER];
    button->border_width = 1;
    button->padding.x = item_padding.x;
    button->padding.y = item_padding.y;
}

static enum gui_widget_state
gui_button(struct gui_button *button, struct gui_rect *bounds,
    struct gui_context *layout)
{
    const struct gui_style *config;
    enum gui_widget_state state;
    state = gui_widget(bounds, layout);
    if (!state) return state;
    config = layout->style;
    gui_fill_button(config, button);
    return state;
}

gui_bool
gui_button_text(struct gui_context *layout, const char *str,
    enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button_text button;
    const struct gui_style *config;

    const struct gui_input *i;
    enum gui_widget_state state;
    state = gui_button(&button.base, &bounds, layout);
    if (!state) return gui_false;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_WINDOW_ROM) ? 0 : layout->input;

    config = layout->style;
    button.alignment = GUI_TEXT_CENTERED;
    button.normal = config->colors[GUI_COLOR_TEXT];
    button.hover = config->colors[GUI_COLOR_TEXT_HOVERING];
    button.active = config->colors[GUI_COLOR_TEXT_ACTIVE];
    return gui_widget_button_text(layout->buffer, bounds, str, behavior,
            &button, i, &config->font);
}

gui_bool
gui_button_color(struct gui_context *layout,
   struct gui_color color, enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_input *i;

    enum gui_widget_state state;
    state = gui_button(&button, &bounds, layout);
    if (!state) return gui_false;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_WINDOW_ROM) ? 0 : layout->input;

    button.normal = color;
    button.hover = color;
    button.active = color;
    return gui_widget_do_button(layout->buffer, bounds, &button, i, behavior, &bounds);
}

gui_bool
gui_button_symbol(struct gui_context *layout, enum gui_symbol symbol,
    enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button_symbol button;
    const struct gui_style *config;

    const struct gui_input *i;
    enum gui_widget_state state;
    state = gui_button(&button.base, &bounds, layout);
    if (!state) return gui_false;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_WINDOW_ROM) ? 0 : layout->input;

    config = layout->style;
    button.normal = config->colors[GUI_COLOR_TEXT];
    button.hover = config->colors[GUI_COLOR_TEXT_HOVERING];
    button.active = config->colors[GUI_COLOR_TEXT_ACTIVE];
    return gui_widget_button_symbol(layout->buffer, bounds, symbol,
            behavior, &button, i, &config->font);
}

gui_bool
gui_button_image(struct gui_context *layout, struct gui_image image,
    enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button_icon button;

    const struct gui_input *i;
    enum gui_widget_state state;
    state = gui_button(&button.base, &bounds, layout);
    if (!state) return gui_false;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_WINDOW_ROM) ? 0 : layout->input;
    button.padding = gui_vec2(0,0);
    return gui_widget_button_image(layout->buffer, bounds, image, behavior, &button, i);
}

gui_bool
gui_button_text_symbol(struct gui_context *layout, enum gui_symbol symbol,
    const char *text, enum gui_text_align align, enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button_text button;
    const struct gui_style *config;

    const struct gui_input *i;
    enum gui_widget_state state;
    state = gui_button(&button.base, &bounds, layout);
    if (!state) return gui_false;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_WINDOW_ROM) ? 0 : layout->input;

    config = layout->style;
    button.normal = config->colors[GUI_COLOR_TEXT];
    button.hover = config->colors[GUI_COLOR_TEXT_HOVERING];
    button.active = config->colors[GUI_COLOR_TEXT_ACTIVE];
    return gui_widget_button_text_symbol(layout->buffer, bounds, symbol, text, align,
            behavior, &button, &config->font, i);
}

gui_bool
gui_button_text_image(struct gui_context *layout, struct gui_image img,
    const char *text, enum gui_text_align align, enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button_text button;
    const struct gui_style *config;

    const struct gui_input *i;
    enum gui_widget_state state;
    state = gui_button(&button.base, &bounds, layout);
    if (!state) return gui_false;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_WINDOW_ROM) ? 0 : layout->input;

    config = layout->style;
    button.normal = config->colors[GUI_COLOR_TEXT];
    button.hover = config->colors[GUI_COLOR_TEXT_HOVERING];
    button.active = config->colors[GUI_COLOR_TEXT_ACTIVE];
    return gui_widget_button_text_image(layout->buffer, bounds, img, text, align,
                                behavior, &button, &config->font, i);
}

gui_bool
gui_button_fitting(struct gui_context *layout,  const char *text,
    enum gui_text_align align, enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button_text button;
    const struct gui_style *config;

    const struct gui_input *i;
    enum gui_widget_state state;
    state = gui_button(&button.base, &bounds, layout);
    if (!state) return gui_false;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_WINDOW_ROM) ? 0 : layout->input;

    /* update the bounds to stand without padding  */
    if (layout->row.index == 1) {
        bounds.w += layout->style->properties[GUI_PROPERTY_PADDING].x;
        bounds.x -= layout->style->properties[GUI_PROPERTY_PADDING].x;
    } else bounds.x -= layout->style->properties[GUI_PROPERTY_ITEM_PADDING].x;
    if (layout->row.index == layout->row.columns)
        bounds.w += layout->style->properties[GUI_PROPERTY_PADDING].x;
    else bounds.w += layout->style->properties[GUI_PROPERTY_ITEM_PADDING].x;

    config = layout->style;
    button.base.border_width = 0;
    button.base.normal = config->colors[GUI_COLOR_WINDOW];
    button.normal = config->colors[GUI_COLOR_TEXT];
    button.hover = config->colors[GUI_COLOR_TEXT_HOVERING];
    button.active = config->colors[GUI_COLOR_TEXT_ACTIVE];
    button.alignment = align;
    return gui_widget_button_text(layout->buffer, bounds, text,  behavior,
            &button, i, &config->font);
}

gui_bool
gui_button_toggle(struct gui_context *layout, const char *str, gui_bool value)
{
    struct gui_rect bounds;
    struct gui_button_text button;
    const struct gui_style *config;

    const struct gui_input *i;
    enum gui_widget_state state;
    state = gui_button(&button.base, &bounds, layout);
    if (!state) return value;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_WINDOW_ROM) ? 0 : layout->input;

    config = layout->style;
    button.base.border = config->colors[GUI_COLOR_BORDER];
    button.alignment = GUI_TEXT_CENTERED;
    if (!value) {
        button.base.normal = config->colors[GUI_COLOR_BUTTON];
        button.base.hover = config->colors[GUI_COLOR_BUTTON_HOVER];
        button.base.active = config->colors[GUI_COLOR_BUTTON_ACTIVE];
        button.normal = config->colors[GUI_COLOR_TEXT];
        button.hover = config->colors[GUI_COLOR_TEXT_HOVERING];
        button.active = config->colors[GUI_COLOR_TEXT_ACTIVE];
    } else {
        button.base.normal = config->colors[GUI_COLOR_BUTTON_ACTIVE];
        button.base.hover = config->colors[GUI_COLOR_BUTTON_HOVER];
        button.base.active = config->colors[GUI_COLOR_BUTTON];
        button.normal = config->colors[GUI_COLOR_TEXT_ACTIVE];
        button.hover = config->colors[GUI_COLOR_TEXT_HOVERING];
        button.active = config->colors[GUI_COLOR_TEXT];
    }
    if (gui_widget_button_text(layout->buffer, bounds, str, GUI_BUTTON_DEFAULT,
        &button, i, &config->font)) value = !value;
    return value;
}

static enum gui_widget_state
gui_toggle_base(struct gui_toggle *toggle, struct gui_rect *bounds,
    struct gui_context *layout)
{
    const struct gui_style *config;
    struct gui_vec2 item_padding;
    enum gui_widget_state state;
    state = gui_widget(bounds, layout);
    if (!state) return state;

    config = layout->style;
    item_padding = gui_style_property(config, GUI_PROPERTY_ITEM_PADDING);
    toggle->rounding = 0;
    toggle->padding.x = item_padding.x;
    toggle->padding.y = item_padding.y;
    toggle->font = config->colors[GUI_COLOR_TEXT];
    return state;
}

gui_bool
gui_check(struct gui_context *layout, const char *text, gui_bool is_active)
{
    struct gui_rect bounds;
    struct gui_toggle toggle;
    const struct gui_style *config;

    const struct gui_input *i;
    enum gui_widget_state state;
    state = gui_toggle_base(&toggle, &bounds, layout);
    if (!state) return is_active;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_WINDOW_ROM) ? 0 : layout->input;

    config = layout->style;
    toggle.rounding = config->rounding[GUI_ROUNDING_CHECK];
    toggle.cursor = config->colors[GUI_COLOR_TOGGLE_CURSOR];
    toggle.normal = config->colors[GUI_COLOR_TOGGLE];
    toggle.hover = config->colors[GUI_COLOR_TOGGLE_HOVER];
    gui_widget_toggle(layout->buffer, bounds, &is_active, text, GUI_TOGGLE_CHECK,
                        &toggle, i, &config->font);
    return is_active;
}

gui_bool
gui_option(struct gui_context *layout, const char *text, gui_bool is_active)
{
    struct gui_rect bounds;
    struct gui_toggle toggle;
    const struct gui_style *config;

    const struct gui_input *i;
    enum gui_widget_state state;
    state = gui_toggle_base(&toggle, &bounds, layout);
    if (!state) return is_active;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_WINDOW_ROM) ? 0 : layout->input;

    config = layout->style;
    toggle.cursor = config->colors[GUI_COLOR_TOGGLE_CURSOR];
    toggle.normal = config->colors[GUI_COLOR_TOGGLE];
    toggle.hover = config->colors[GUI_COLOR_TOGGLE_HOVER];
    gui_widget_toggle(layout->buffer, bounds, &is_active, text, GUI_TOGGLE_OPTION,
                        &toggle, i, &config->font);
    return is_active;
}

gui_size
gui_option_group(struct gui_context *layout, const char **options,
    gui_size count, gui_size current)
{
    gui_size i;
    GUI_ASSERT(layout && options && count);
    if (!layout || !options || !count) return current;
    for (i = 0; i < count; ++i) {
        if (gui_option(layout, options[i], i == current))
            current = i;
    }
    return current;
}

gui_float
gui_slider(struct gui_context *layout, gui_float min_value, gui_float value,
    gui_float max_value, gui_float value_step)
{
    struct gui_rect bounds;
    struct gui_slider slider;
    const struct gui_style *config;
    struct gui_vec2 item_padding;

    const struct gui_input *i;
    enum gui_widget_state state;
    state = gui_widget(&bounds, layout);
    if (!state) return value;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_WINDOW_ROM) ? 0 : layout->input;

    config = layout->style;
    item_padding = gui_style_property(config, GUI_PROPERTY_ITEM_PADDING);
    slider.padding.x = item_padding.x;
    slider.padding.y = item_padding.y;
    slider.bg = config->colors[GUI_COLOR_SLIDER];
    slider.normal = config->colors[GUI_COLOR_SLIDER_CURSOR];
    slider.hover = config->colors[GUI_COLOR_SLIDER_CURSOR_HOVER];
    slider.active = config->colors[GUI_COLOR_SLIDER_CURSOR_ACTIVE];
    slider.border = config->colors[GUI_COLOR_BORDER];
    slider.rounding = config->rounding[GUI_ROUNDING_SLIDER];
    return gui_widget_slider(layout->buffer, bounds, min_value, value, max_value,
                        value_step, &slider, i);
}

gui_size
gui_progress(struct gui_context *layout, gui_size cur_value, gui_size max_value,
    gui_bool is_modifyable)
{
    struct gui_rect bounds;
    struct gui_progress prog;
    const struct gui_style *config;
    struct gui_vec2 item_padding;

    const struct gui_input *i;
    enum gui_widget_state state;
    state = gui_widget(&bounds, layout);
    if (!state) return cur_value;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_WINDOW_ROM) ? 0 : layout->input;

    config = layout->style;
    item_padding = gui_style_property(config, GUI_PROPERTY_ITEM_PADDING);
    prog.rounding = config->rounding[GUI_ROUNDING_PROGRESS];
    prog.padding.x = item_padding.x;
    prog.padding.y = item_padding.y;
    prog.border = config->colors[GUI_COLOR_BORDER];
    prog.background = config->colors[GUI_COLOR_PROGRESS];
    prog.normal = config->colors[GUI_COLOR_PROGRESS_CURSOR];
    prog.hover = config->colors[GUI_COLOR_PROGRESS_CURSOR_HOVER];
    prog.active = config->colors[GUI_COLOR_PROGRESS_CURSOR_ACTIVE];
    prog.rounding = config->rounding[GUI_ROUNDING_PROGRESS];
    return gui_widget_progress(layout->buffer, bounds, cur_value, max_value,
                        is_modifyable, &prog, i);
}

static enum gui_widget_state
gui_edit_base(struct gui_rect *bounds, struct gui_edit *field,
    struct gui_context *layout)
{
    const struct gui_style *config;
    struct gui_vec2 item_padding;
    enum gui_widget_state state = gui_widget(bounds, layout);
    if (!state) return state;

    config = layout->style;
    item_padding = gui_style_property(config, GUI_PROPERTY_ITEM_PADDING);
    field->border_size = 1;
    field->rounding = config->rounding[GUI_ROUNDING_INPUT];
    field->padding.x = item_padding.x;
    field->padding.y = item_padding.y;
    field->show_cursor = gui_true;
    field->background = config->colors[GUI_COLOR_INPUT];
    field->border = config->colors[GUI_COLOR_BORDER];
    field->cursor = config->colors[GUI_COLOR_INPUT_CURSOR];
    field->text = config->colors[GUI_COLOR_INPUT_TEXT];
    return state;
}

void
gui_editbox(struct gui_context *layout, struct gui_edit_box *box)
{
    struct gui_rect bounds;
    struct gui_edit field;
    const struct gui_style *config = layout->style;
    const struct gui_input *i;
    enum gui_widget_state state;

    state = gui_edit_base(&bounds, &field, layout);
    if (!state) return;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_WINDOW_ROM) ? 0 : layout->input;
    gui_widget_editbox(layout->buffer, bounds, box, &field, i, &config->font);
}

gui_size
gui_edit(struct gui_context *layout, gui_char *buffer, gui_size len,
    gui_size max, gui_state *active, gui_size *cursor, enum gui_input_filter filter)
{
    struct gui_rect bounds;
    struct gui_edit field;
    const struct gui_style *config = layout->style;
    const struct gui_input *i;
    enum gui_widget_state state;

    state = gui_edit_base(&bounds, &field, layout);
    if (!state) return len;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_WINDOW_ROM) ? 0 : layout->input;
    return gui_widget_edit(layout->buffer, bounds,  buffer, len, max, active, cursor,
                    &field, filter, i, &config->font);
}

gui_size
gui_edit_filtered(struct gui_context *layout, gui_char *buffer, gui_size len,
    gui_size max, gui_state *active, gui_size *cursor, gui_filter filter)
{
    struct gui_rect bounds;
    struct gui_edit field;
    const struct gui_style *config = layout->style;
    const struct gui_input *i;
    enum gui_widget_state state;

    state = gui_edit_base(&bounds, &field, layout);
    if (!state) return len;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_WINDOW_ROM) ? 0 : layout->input;
    return gui_widget_edit_filtered(layout->buffer, bounds, buffer, len, max, active,
                            cursor, &field, filter, i, &config->font);
}

static enum gui_widget_state
gui_spinner_base(struct gui_context *layout, struct gui_rect *bounds,
        struct gui_spinner *spinner)
{
    struct gui_vec2 item_padding;
    enum gui_widget_state state;
    const struct gui_style *config;

    GUI_ASSERT(layout);
    GUI_ASSERT(bounds);
    GUI_ASSERT(spinner);
    if (!layout || !bounds || !spinner)
        return GUI_WIDGET_INVALID;

    state = gui_widget(bounds, layout);
    if (!state) return state;
    config = layout->style;
    item_padding = gui_style_property(config, GUI_PROPERTY_ITEM_PADDING);

    gui_fill_button(config, &spinner->button.base);
    spinner->button.base.rounding = 0;
    spinner->button.normal = config->colors[GUI_COLOR_TEXT];
    spinner->button.hover = config->colors[GUI_COLOR_TEXT_HOVERING];
    spinner->button.active = config->colors[GUI_COLOR_TEXT_ACTIVE];
    spinner->button.base.padding = item_padding;
    spinner->padding.x = item_padding.x;
    spinner->padding.y = item_padding.y;
    spinner->color = config->colors[GUI_COLOR_SPINNER];
    spinner->border = config->colors[GUI_COLOR_BORDER];
    spinner->text = config->colors[GUI_COLOR_TEXT];
    spinner->show_cursor = gui_false;
    return state;
}

gui_int
gui_spinner(struct gui_context *layout, gui_int min, gui_int value,
    gui_int max, gui_int step, gui_state *active)
{
    struct gui_rect bounds;
    struct gui_spinner spinner;
    struct gui_command_buffer *out;
    const struct gui_input *i;
    enum gui_widget_state state;

    state = gui_spinner_base(layout, &bounds, &spinner);
    if (!state) return value;
    out = layout->buffer;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_WINDOW_ROM) ? 0 : layout->input;
    return gui_widget_spinner(out, bounds, &spinner, min, value, max, step, active,
                        i, &layout->style->font);
}

/*
 * -------------------------------------------------------------
 *
 *                          GRAPH
 *
 * --------------------------------------------------------------
 */
void
gui_graph_begin(struct gui_context *layout, struct gui_graph *graph,
    enum gui_graph_type type, gui_size count, gui_float min_value, gui_float max_value)
{
    struct gui_rect bounds = {0, 0, 0, 0};
    const struct gui_style *config;
    struct gui_command_buffer *out;
    struct gui_color color;
    struct gui_vec2 item_padding;
    if (!gui_widget(&bounds, layout)) {
        gui_zero(graph, sizeof(*graph));
        return;
    }

    /* draw graph background */
    config = layout->style;
    out = layout->buffer;
    item_padding = gui_style_property(config, GUI_PROPERTY_ITEM_PADDING);
    color = (type == GUI_GRAPH_LINES) ?
        config->colors[GUI_COLOR_PLOT]: config->colors[GUI_COLOR_HISTO];
    gui_command_buffer_push_rect(out, bounds, config->rounding[GUI_ROUNDING_GRAPH], color);

    /* setup basic generic graph  */
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
gui_graph_push_line(struct gui_context *layout,
    struct gui_graph *g, gui_float value)
{
    struct gui_command_buffer *out = layout->buffer;
    const struct gui_style *config = layout->style;
    const struct gui_input *i = layout->input;
    struct gui_color color = config->colors[GUI_COLOR_PLOT_LINES];
    gui_bool selected = gui_false;
    gui_float step, range, ratio;
    struct gui_vec2 cur;

    GUI_ASSERT(g);
    GUI_ASSERT(layout);
    GUI_ASSERT(out);
    if (!g || !layout || !g->valid || g->index >= g->count)
        return gui_false;

    step = g->w / (gui_float)g->count;
    range = g->max - g->min;
    ratio = (value - g->min) / range;

    if (g->index == 0) {
        /* special case for the first data point since it does not have a connection */
        g->last.x = g->x;
        g->last.y = (g->y + g->h) - ratio * (gui_float)g->h;
        if (!(layout->flags & GUI_WINDOW_ROM) &&
            GUI_INBOX(i->mouse.pos.x,i->mouse.pos.y,g->last.x-3,g->last.y-3,6,6)){
            selected = (i->mouse.buttons[GUI_BUTTON_LEFT].down &&
                i->mouse.buttons[GUI_BUTTON_LEFT].clicked) ? gui_true: gui_false;
            color = config->colors[GUI_COLOR_PLOT_HIGHLIGHT];
        }
        gui_command_buffer_push_rect(out,
            gui_rect(g->last.x - 3, g->last.y - 3, 6, 6), 0, color);
        g->index++;
        return selected;
    }

    /* draw a line between the last data point and the new one */
    cur.x = g->x + (gui_float)(step * (gui_float)g->index);
    cur.y = (g->y + g->h) - (ratio * (gui_float)g->h);
    gui_command_buffer_push_line(out, g->last.x, g->last.y, cur.x, cur.y,
        config->colors[GUI_COLOR_PLOT_LINES]);

    /* user selection of the current data point */
    if (!(layout->flags & GUI_WINDOW_ROM) &&
        GUI_INBOX(i->mouse.pos.x, i->mouse.pos.y, cur.x-3, cur.y-3, 6, 6)) {
        selected = (i->mouse.buttons[GUI_BUTTON_LEFT].down &&
            i->mouse.buttons[GUI_BUTTON_LEFT].clicked) ? gui_true: gui_false;
        color = config->colors[GUI_COLOR_PLOT_HIGHLIGHT];
    } else color = config->colors[GUI_COLOR_PLOT_LINES];
    gui_command_buffer_push_rect(out, gui_rect(cur.x - 3, cur.y - 3, 6, 6), 0, color);

    /* save current data point position */
    g->last.x = cur.x;
    g->last.y = cur.y;
    g->index++;
    return selected;
}

static gui_bool
gui_graph_push_column(struct gui_context *layout,
    struct gui_graph *graph, gui_float value)
{
    struct gui_command_buffer *out = layout->buffer;
    const struct gui_style *config = layout->style;
    const struct gui_input *in = layout->input;
    struct gui_vec2 item_padding;
    struct gui_color color;

    gui_float ratio;
    gui_bool selected = gui_false;
    struct gui_rect item = {0,0,0,0};
    item_padding = gui_style_property(config, GUI_PROPERTY_ITEM_PADDING);

    if (!graph->valid || graph->index >= graph->count)
        return gui_false;
    if (graph->count) {
        gui_float padding = (gui_float)(graph->count-1) * item_padding.x;
        item.w = (graph->w - padding) / (gui_float)(graph->count);
    }

    ratio = GUI_ABS(value) / graph->max;
    color = (value < 0) ? config->colors[GUI_COLOR_HISTO_NEGATIVE]:
        config->colors[GUI_COLOR_HISTO_BARS];

    /* calculate bounds of the current bar graph entry */
    item.h = graph->h * ratio;
    item.y = (graph->y + graph->h) - item.h;
    item.x = graph->x + ((gui_float)graph->index * item.w);
    item.x = item.x + ((gui_float)graph->index * item_padding.x);

    /* user graph bar selection */
    if (!(layout->flags & GUI_WINDOW_ROM) &&
        GUI_INBOX(in->mouse.pos.x,in->mouse.pos.y,item.x,item.y,item.w,item.h)) {
        selected = (in->mouse.buttons[GUI_BUTTON_LEFT].down &&
                in->mouse.buttons[GUI_BUTTON_LEFT].clicked) ? (gui_int)graph->index: selected;
        color = config->colors[GUI_COLOR_HISTO_HIGHLIGHT];
    }
    gui_command_buffer_push_rect(out, item, 0, color);
    graph->index++;
    return selected;
}

gui_bool
gui_graph_push(struct gui_context *layout, struct gui_graph *graph,
    gui_float value)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(graph);
    if (!layout || !graph || !layout->valid) return gui_false;
    switch (graph->type) {
    case GUI_GRAPH_LINES:
        return gui_graph_push_line(layout, graph, value);
    case GUI_GRAPH_COLUMN:
        return gui_graph_push_column(layout, graph, value);
    case GUI_GRAPH_MAX:
    default: return gui_false;
    }
}

void
gui_graph_end(struct gui_context *layout, struct gui_graph *graph)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(graph);
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
gui_graph(struct gui_context *layout, enum gui_graph_type type,
    const gui_float *values, gui_size count, gui_size offset)
{
    gui_size i;
    gui_int index = -1;
    gui_float min_value;
    gui_float max_value;
    struct gui_graph graph;

    GUI_ASSERT(layout);
    GUI_ASSERT(values);
    GUI_ASSERT(count);
    if (!layout || !layout->valid || !values || !count)
        return -1;

    /* find min and max graph value */
    max_value = values[0];
    min_value = values[0];
    for (i = offset; i < count; ++i) {
        if (values[i] > max_value)
            max_value = values[i];
        if (values[i] < min_value)
            min_value = values[i];
    }

    /* execute graph */
    gui_graph_begin(layout, &graph, type, count, min_value, max_value);
    for (i = offset; i < count; ++i) {
        if (gui_graph_push(layout, &graph, values[i]))
            index = (gui_int)i;
    }
    gui_graph_end(layout, &graph);
    return index;
}

gui_int
gui_graph_callback(struct gui_context *layout, enum gui_graph_type type,
    gui_size count, gui_float(*get_value)(void*, gui_size), void *userdata)
{
    gui_size i;
    gui_int index = -1;
    gui_float min_value;
    gui_float max_value;
    struct gui_graph graph;

    GUI_ASSERT(layout);
    GUI_ASSERT(get_value);
    GUI_ASSERT(count);
    if (!layout || !layout->valid || !get_value || !count)
        return -1;

    /* find min and max graph value */
    max_value = get_value(userdata, 0);
    min_value = max_value;
    for (i = 1; i < count; ++i) {
        gui_float value = get_value(userdata, i);
        if (value > max_value)
            max_value = value;
        if (value < min_value)
            min_value = value;
    }

    /* execute graph */
    gui_graph_begin(layout, &graph, type, count, min_value, max_value);
    for (i = 0; i < count; ++i) {
        gui_float value = get_value(userdata, i);
        if (gui_graph_push(layout, &graph, value))
            index = (gui_int)i;
    }
    gui_graph_end(layout, &graph);
    return index;
}
/*
 * -------------------------------------------------------------
 *
 *                          POPUP
 *
 * --------------------------------------------------------------
 */
gui_flags
gui_popup_begin(struct gui_context *parent, struct gui_context *popup,
    enum gui_popup_type type, gui_flags flags,
    struct gui_rect rect, struct gui_vec2 scrollbar)
{
    struct gui_window panel;
    GUI_ASSERT(parent);
    GUI_ASSERT(popup);
    if (!parent || !popup || !parent->valid) {
        popup->valid = gui_false;
        popup->style = parent->style;
        popup->buffer = parent->buffer;
        popup->input = parent->input;
        return 0;
    }

    /* calculate bounds of the panel */
    gui_zero(popup, sizeof(*popup));
    rect.x += parent->clip.x;
    rect.y += parent->clip.y;
    parent->flags |= GUI_WINDOW_ROM;

    /* initialize a fake panel */
    flags |= GUI_WINDOW_BORDER|GUI_WINDOW_TAB;
    if (type == GUI_POPUP_DYNAMIC)
        flags |= GUI_WINDOW_DYNAMIC;
    gui_window_init(&panel, gui_rect(rect.x, rect.y, rect.w, rect.h),flags, 0,
        parent->style, parent->input);

    /* begin sub-buffer and create panel layout  */
    gui_command_queue_start_child(parent->queue, parent->buffer);
    panel.buffer = *parent->buffer;
    gui_begin(popup, &panel);
    *parent->buffer = panel.buffer;
    parent->flags |= GUI_WINDOW_ROM;

    popup->buffer = parent->buffer;
    popup->offset = scrollbar;
    popup->queue = parent->queue;
    return 1;
}

void
gui_popup_close(struct gui_context *popup)
{
    GUI_ASSERT(popup);
    if (!popup || !popup->valid) return;
    popup->flags |= GUI_WINDOW_HIDDEN;
    popup->valid = gui_false;
}

struct gui_vec2
gui_popup_end(struct gui_context *parent, struct gui_context *popup)
{
    struct gui_window pan;
    struct gui_command_buffer *out;

    GUI_ASSERT(parent);
    GUI_ASSERT(popup);
    if (!parent || !popup) return gui_vec2(0,0);
    if (!parent->valid) return gui_vec2(0,0);

    gui_zero(&pan, sizeof(pan));
    if (popup->flags & GUI_WINDOW_HIDDEN) {
        parent->flags |= GUI_WINDOW_REMOVE_ROM;
        popup->flags &= ~(gui_flags)~GUI_WINDOW_HIDDEN;
        popup->valid = gui_true;
    }

    out = parent->buffer;
    pan.bounds.x = popup->bounds.x;
    pan.bounds.y = popup->bounds.y;
    pan.bounds.w = popup->width;
    pan.bounds.h = popup->height;
    pan.flags = GUI_WINDOW_BORDER|GUI_WINDOW_TAB;

    /* end popup and reset clipping rect back to parent panel */
    gui_command_buffer_push_scissor(out, parent->clip);
    gui_end(popup, &pan);
    gui_command_queue_finish_child(parent->queue, parent->buffer);
    gui_command_buffer_push_scissor(out, parent->clip);
    return pan.offset;
}

static gui_bool
gui_popup_nonblocking_begin(struct gui_context *parent,
    struct gui_context *popup, gui_flags flags, gui_state *active, gui_state is_active,
    struct gui_rect body)
{
    /* deactivate popup if user clicked outside the popup*/
    const struct gui_input *in = parent->input;
    if (in && *active) {
        gui_bool inbody = gui_input_is_mouse_click_in_rect(in, GUI_BUTTON_LEFT, body);
        gui_bool inpanel = gui_input_is_mouse_click_in_rect(in, GUI_BUTTON_LEFT, parent->bounds);
        if (!inbody && inpanel)
            is_active = gui_false;
    }

    /* recalculate body bounds into local panel position */
    body.x -= parent->clip.x;
    body.y -= parent->clip.y;

    /* if active create popup otherwise deactive the panel layout  */
    if (!is_active && *active) {
        gui_popup_begin(parent, popup, GUI_POPUP_DYNAMIC, flags, body, gui_vec2(0,0));
        gui_popup_close(popup);
        popup->flags &= ~(gui_flags)GUI_WINDOW_MINIMIZED;
        parent->flags &= ~(gui_flags)GUI_WINDOW_ROM;
    } else if (!is_active && !*active) {
        *active = is_active;
        popup->flags |= GUI_WINDOW_MINIMIZED;
        return gui_false;
    } else {
        gui_popup_begin(parent, popup, GUI_POPUP_DYNAMIC, flags, body, gui_vec2(0,0));
        popup->flags &= ~(gui_flags)GUI_WINDOW_MINIMIZED;
    }
    *active = is_active;
    return gui_true;
}

static void
gui_popup_nonblocking_end(struct gui_context *parent,
    struct gui_context *popup)
{
    GUI_ASSERT(parent);
    GUI_ASSERT(popup);
    if (!parent || !popup) return;
    if (!parent->valid) return;
    if (!(popup->flags & GUI_WINDOW_MINIMIZED))
        gui_popup_end(parent, popup);
}

void
gui_popup_nonblock_begin(struct gui_context *parent, struct gui_context *popup,
    gui_flags flags, gui_state *active, struct gui_rect body)
{
    GUI_ASSERT(parent);
    GUI_ASSERT(popup);
    GUI_ASSERT(active);
    if (!parent || !popup || !active) return;
    gui_popup_nonblocking_begin(parent, popup, flags, active, *active, body);
}

gui_state
gui_popup_nonblock_close(struct gui_context *popup)
{
    GUI_ASSERT(popup);
    if (!popup) return GUI_INACTIVE;
    gui_popup_close(popup);
    popup->flags |= GUI_WINDOW_HIDDEN;
    return GUI_INACTIVE;
}

void
gui_popup_nonblock_end(struct gui_context *parent, struct gui_context *popup)
{
    GUI_ASSERT(parent);
    GUI_ASSERT(popup);
    if (!parent || !popup) return;
    if ((!parent->valid || !popup->valid) && !(popup->flags & GUI_WINDOW_HIDDEN))
        return;
    gui_popup_nonblocking_end(parent, popup);
    return;
}
/*
 * -------------------------------------------------------------
 *
 *                          COMBO
 *
 * --------------------------------------------------------------
 */
void
gui_combo_begin(struct gui_context *parent, struct gui_context *combo,
    const char *selected, gui_state *active)
{
    struct gui_rect bounds = {0,0,0,0};
    const struct gui_input *in;
    const struct gui_style *config;
    struct gui_command_buffer *out;
    struct gui_vec2 item_padding;
    struct gui_rect header;
    gui_state is_active;

    GUI_ASSERT(parent);
    GUI_ASSERT(combo);
    GUI_ASSERT(selected);
    GUI_ASSERT(active);
    if (!parent || !combo || !selected || !active) return;
    if (!parent->valid || !gui_widget(&header, parent))
        goto failed;

    gui_zero(combo, sizeof(*combo));
    in = (parent->flags & GUI_WINDOW_ROM) ? 0 : parent->input;
    is_active = *active;
    out = parent->buffer;
    config = parent->style;
    item_padding = gui_style_property(config, GUI_PROPERTY_ITEM_PADDING);

    /* draw combo box header background and border */
    gui_command_buffer_push_rect(out, header, 0, config->colors[GUI_COLOR_BORDER]);
    gui_command_buffer_push_rect(out, gui_shrink_rect(header, 1), 0,
        config->colors[GUI_COLOR_SPINNER]);

    {
        /* print currently selected item */
        gui_size text_len;
        struct gui_rect clip;
        struct gui_rect label;
        label.x = header.x + item_padding.x;
        label.y = header.y + item_padding.y;
        label.w = header.w - (header.h + 2 * item_padding.x);
        label.h = header.h - 2 * item_padding.y;
        text_len = gui_strsiz(selected);

        /* set correct clipping rectangle and draw title */
        gui_unify(&clip, &parent->clip, label.x, label.y, label.x + label.w,
            label.y + label.h);
        gui_command_buffer_push_scissor(out, clip);
        gui_command_buffer_push_text(out, label, selected, text_len, &config->font,
                config->colors[GUI_COLOR_WINDOW], config->colors[GUI_COLOR_TEXT]);
        gui_command_buffer_push_scissor(out, parent->clip);
    }
    {
        /* button setup and execution */
        struct gui_button_symbol button;
        bounds.y = header.y + 1;
        bounds.h = MAX(2, header.h);
        bounds.h = bounds.h - 2;
        bounds.w = bounds.h - 2;
        bounds.x = (header.x + header.w) - (bounds.w+2);

        button.base.rounding = 0;
        button.base.border_width = 0;
        button.base.padding.x = bounds.w/4.0f;
        button.base.padding.y = bounds.h/4.0f;
        button.base.border = config->colors[GUI_COLOR_SPINNER];
        button.base.normal = config->colors[GUI_COLOR_SPINNER];
        button.base.hover = config->colors[GUI_COLOR_SPINNER];
        button.base.active = config->colors[GUI_COLOR_SPINNER];
        button.normal = config->colors[GUI_COLOR_TEXT];
        button.hover = config->colors[GUI_COLOR_TEXT_HOVERING];
        button.active = config->colors[GUI_COLOR_TEXT_ACTIVE];
        if (gui_widget_button_symbol(out, bounds, GUI_SYMBOL_TRIANGLE_DOWN,
            GUI_BUTTON_DEFAULT, &button, in, &config->font))
            is_active = !is_active;
    }
    {
        /* calculate the maximum height of the combo box*/
        struct gui_rect body;
        body.x = header.x;
        body.w = header.w;
        body.y = header.y + header.h;
        body.h = gui_null_rect.h;
        if (!gui_popup_nonblocking_begin(parent, combo, GUI_WINDOW_NO_SCROLLBAR,
                active, is_active, body)) goto failed;
        combo->flags |= GUI_WINDOW_COMBO_MENU;
    }
    return;

failed:
    combo->valid = gui_false;
    combo->style = parent->style;
    combo->buffer = parent->buffer;
    combo->input = parent->input;
    combo->queue = parent->queue;
}

void
gui_combo_end(struct gui_context *parent, struct gui_context *combo)
{
    GUI_ASSERT(parent);
    GUI_ASSERT(combo);
    if (!parent || !combo) return;
    if ((!parent->valid || !combo->valid) && !(combo->flags & GUI_WINDOW_HIDDEN))
        return;
    gui_popup_nonblocking_end(parent, combo);
    return;
}

void
gui_combo_close(struct gui_context *combo)
{
    GUI_ASSERT(combo);
    if (!combo) return;
    gui_popup_close(combo);
    combo->flags |= GUI_WINDOW_HIDDEN;
}

void
gui_combo(struct gui_context *layout, const char **entries,
    gui_size count, gui_size *current, gui_size row_height,
    gui_state *active)
{
    gui_size i;
    struct gui_context combo;
    GUI_ASSERT(layout);
    GUI_ASSERT(entries);
    GUI_ASSERT(current);
    GUI_ASSERT(active);
    if (!layout || !layout->valid || !entries || !current || !active) return;
    if (!count) return;

    gui_zero(&combo, sizeof(combo));
    gui_combo_begin(layout, &combo, entries[*current], active);
    gui_layout_row_dynamic(&combo, (gui_float)row_height, 1);
    for (i = 0; i < count; ++i) {
        if (i == *current) continue;
        if (gui_button_fitting(&combo,entries[i], GUI_TEXT_LEFT, GUI_BUTTON_DEFAULT)) {
            gui_combo_close(&combo);
            *active = gui_false;
            *current = i;
        }
    }
    gui_combo_end(layout, &combo);
}
/*
 * -------------------------------------------------------------
 *
 *                          MENU
 *
 * --------------------------------------------------------------
 */
void
gui_menu_begin(struct gui_context *parent, struct gui_context *menu,
    const char *title, gui_float width, gui_state *active)
{
    const struct gui_input *in;
    const struct gui_style *config;
    struct gui_rect header;
    gui_state is_active;

    GUI_ASSERT(parent);
    GUI_ASSERT(menu);
    GUI_ASSERT(title);
    GUI_ASSERT(active);
    if (!parent || !menu || !title || !active) return;
    if (!parent->valid) goto failed;

    is_active = *active;
    in = parent->input;
    config = parent->style;
    gui_zero(menu, sizeof(*menu));
    {
        /* exeucte menu button for open/closing the popup */
        struct gui_button_text button;
        gui_button(&button.base, &header, parent);
        button.alignment = GUI_TEXT_CENTERED;
        button.base.rounding = 0;
        button.base.border = config->colors[GUI_COLOR_WINDOW];
        button.base.normal = (is_active) ? config->colors[GUI_COLOR_BUTTON_HOVER]:
            config->colors[GUI_COLOR_WINDOW];
        button.base.active = config->colors[GUI_COLOR_WINDOW];
        button.normal = config->colors[GUI_COLOR_TEXT];
        button.active = config->colors[GUI_COLOR_TEXT];
        button.hover = config->colors[GUI_COLOR_TEXT];
        button.base.rounding = config->rounding[GUI_ROUNDING_BUTTON];
        if (gui_widget_button_text(parent->buffer, header, title, GUI_BUTTON_DEFAULT,
                &button, in, &config->font)) is_active = !is_active;
    }
    {
        /* calculate the maximum height of the menu */
        struct gui_rect body;
        body.x = header.x;
        body.w = width;
        body.y = header.y + header.h;
        body.h = (parent->bounds.y + parent->bounds.h) - body.y;
        if (!gui_popup_nonblocking_begin(parent, menu, GUI_WINDOW_NO_SCROLLBAR, active,
            is_active, body)) goto failed;
        menu->flags |= GUI_WINDOW_COMBO_MENU;
    }
    return;

failed:
    menu->valid = gui_false;
    menu->style = parent->style;
    menu->buffer = parent->buffer;
    menu->input = parent->input;
    menu->queue = parent->queue;
}

void
gui_menu_close(struct gui_context *menu)
{
    GUI_ASSERT(menu);
    if (!menu) return;
    gui_popup_close(menu);
}

struct gui_vec2
gui_menu_end(struct gui_context *parent, struct gui_context *menu)
{
    GUI_ASSERT(parent);
    GUI_ASSERT(menu);
    if (!parent || !menu) return gui_vec2(0,0);
    if (!parent->valid)
        return menu->offset;
    gui_popup_nonblocking_end(parent, menu);
    return menu->offset;
}

gui_int
gui_menu(struct gui_context *layout, const char *title,
    const char **entries, gui_size count, gui_size row_height,
    gui_float width, gui_state *active)
{
    gui_size i;
    gui_int sel = -1;
    struct gui_context menu;

    GUI_ASSERT(layout);
    GUI_ASSERT(entries);
    GUI_ASSERT(title);
    GUI_ASSERT(active);
    if (!layout || !layout->valid || !entries || !title || !active) return -1;
    if (!count) return -1;

    gui_menu_begin(layout, &menu, title, width, active);
    gui_layout_row_dynamic(&menu, (gui_float)row_height, 1);
    for (i = 0; i < count; ++i) {
        if (gui_button_fitting(&menu, entries[i], GUI_TEXT_LEFT, GUI_BUTTON_DEFAULT)) {
            gui_combo_close(&menu);
            *active = gui_false;
            sel = (gui_int)i;
        }
    }
    gui_menu_end(layout, &menu);
    return sel;
}

/*
 * -------------------------------------------------------------
 *
 *                          TREE
 *
 * --------------------------------------------------------------
 */
void
gui_tree_begin(struct gui_context *p, struct gui_tree *tree,
    const char *title, gui_float height, struct gui_vec2 offset)
{
    struct gui_vec2 padding;
    const struct gui_style *config;
    GUI_ASSERT(p);
    GUI_ASSERT(tree);
    GUI_ASSERT(title);
    if (!p || !tree || !title) return;

    gui_group_begin(p, &tree->group, title, 0, offset);
    gui_panel_layout(&tree->group, height, 1);

    config = tree->group.style;
    padding = gui_style_property(config, GUI_PROPERTY_ITEM_PADDING);

    tree->at_x = 0;
    tree->skip = -1;
    tree->depth = 0;
    tree->x_off = tree->group.style->font.height + 2 * padding.x;
}

static enum gui_tree_node_operation
gui_tree_node(struct gui_tree *tree, enum gui_tree_node_symbol symbol,
    const char *title, struct gui_image *img, gui_state *state)
{
    struct gui_text text;
    struct gui_rect bounds = {0,0,0,0};
    struct gui_rect sym, label, icon;
    struct gui_context *layout;
    enum gui_tree_node_operation op = GUI_NODE_NOP;

    const struct gui_input *i;
    const struct gui_style *config;
    struct gui_vec2 item_padding;
    struct gui_color col;

    GUI_ASSERT(tree);
    GUI_ASSERT(state);
    GUI_ASSERT(title);
    if (!tree || !state || !title)
        return GUI_NODE_NOP;

    layout = &tree->group;
    if (tree->skip >= 0 || !gui_widget(&bounds, layout)) {
        if (!tree->depth) tree->at_x = bounds.x;
        return op;
    }

    if (tree->depth){
        bounds.w = (bounds.x + bounds.w) - tree->at_x;
        bounds.x = tree->at_x;
    } else tree->at_x = bounds.x;

    /* fetch some configuration constants */
    i = layout->input;
    config = layout->style;
    item_padding = gui_style_property(config, GUI_PROPERTY_ITEM_PADDING);
    col = gui_style_color(config, GUI_COLOR_TEXT);

    /* calculate symbol bounds */
    sym.x = bounds.x;
    sym.y = bounds.y + (bounds.h/2) - (config->font.height/2);
    sym.w = config->font.height;
    sym.h = config->font.height;

    if (img) {
        icon.x = sym.x + sym.w + item_padding.x;
        icon.y = sym.y;
        icon.w = bounds.h;
        icon.h = bounds.h;
        label.x = icon.x + icon.w + item_padding.x;
    } else label.x = sym.x + sym.w + item_padding.x;

    /* calculate text bounds */
    label.y = bounds.y;
    label.h = bounds.h;
    label.w = bounds.w - (sym.w + 2 * item_padding.x);

    /* output symbol */
    if (symbol == GUI_TREE_NODE_TRIANGLE) {
        /* parent node */
        struct gui_vec2 points[3];
        enum gui_heading heading;
        if (gui_input_mouse_clicked(i, GUI_BUTTON_LEFT, sym)) {
            if (*state & GUI_NODE_ACTIVE)
                *state &= ~(gui_flags)GUI_NODE_ACTIVE;
            else *state |= GUI_NODE_ACTIVE;
        }

        heading = (*state & GUI_NODE_ACTIVE) ? GUI_DOWN : GUI_RIGHT;
        gui_triangle_from_direction(points, sym, 0, 0, heading);
        gui_command_buffer_push_triangle(layout->buffer,  points[0].x, points[0].y,
            points[1].x, points[1].y, points[2].x, points[2].y, col);
    } else {
        /* leaf node */
        gui_command_buffer_push_circle(layout->buffer, sym, col);
    }

    if (!(layout->flags & GUI_WINDOW_ROM)) {
        /* node selection */
        if (gui_input_mouse_clicked(i, GUI_BUTTON_LEFT, label)) {
            if (*state & GUI_NODE_SELECTED)
                *state &= ~(gui_flags)GUI_NODE_SELECTED;
            else *state |= GUI_NODE_SELECTED;
        }

        /* tree node operations */
        if (gui_input_is_key_pressed(i, GUI_KEY_DEL) && (*state & GUI_NODE_SELECTED)) {
            *state &= ~(gui_flags)GUI_NODE_SELECTED;
            op = GUI_NODE_DELETE;
        } else if (gui_input_is_key_pressed(i, GUI_KEY_COPY) &&
            (*state & GUI_NODE_SELECTED)) {
            *state &= ~(gui_flags)GUI_NODE_SELECTED;
            op = GUI_NODE_CLONE;
        } else if (gui_input_is_key_pressed(i, GUI_KEY_CUT) &&
            (*state & GUI_NODE_SELECTED)) {
            *state &= ~(gui_flags)GUI_NODE_SELECTED;
            op = GUI_NODE_CUT;
        } else if (gui_input_is_key_pressed(i, GUI_KEY_PASTE) &&
            (*state & GUI_NODE_SELECTED)) {
            *state &= ~(gui_flags)GUI_NODE_SELECTED;
            op = GUI_NODE_PASTE;
        }
    }

    /* output label */
    text.padding.x = item_padding.x;
    text.padding.y = item_padding.y;
    text.text = config->colors[GUI_COLOR_TEXT];
    text.background = (*state & GUI_NODE_SELECTED) ?
        config->colors[GUI_COLOR_BUTTON_HOVER]:
        config->colors[GUI_COLOR_WINDOW];
    gui_widget_text(layout->buffer, label, title, gui_strsiz(title), &text,
            GUI_TEXT_LEFT, &config->font);
    return op;
}

enum gui_tree_node_operation
gui_tree_begin_node(struct gui_tree *tree, const char *title,
    gui_state *state)
{
    enum gui_tree_node_operation op;
    GUI_ASSERT(tree);
    GUI_ASSERT(state);
    GUI_ASSERT(title);
    if (!tree || !state || !title)
        return GUI_NODE_NOP;

    op = gui_tree_node(tree, GUI_TREE_NODE_TRIANGLE, title, 0, state);
    tree->at_x += tree->x_off;
    if (tree->skip < 0 && !(*state & GUI_NODE_ACTIVE))
        tree->skip = tree->depth;
    tree->depth++;
    return op;
}

enum gui_tree_node_operation
gui_tree_begin_node_icon(struct gui_tree *tree, const char *title,
    struct gui_image img, gui_state *state)
{
    enum gui_tree_node_operation op;
    GUI_ASSERT(tree);
    GUI_ASSERT(state);
    GUI_ASSERT(title);
    if (!tree || !state || !title)
        return GUI_NODE_NOP;

    op = gui_tree_node(tree, GUI_TREE_NODE_TRIANGLE, title, &img, state);
    tree->at_x += tree->x_off;
    if (tree->skip < 0 && !(*state & GUI_NODE_ACTIVE))
        tree->skip = tree->depth;
    tree->depth++;
    return op;
}

enum gui_tree_node_operation
gui_tree_leaf(struct gui_tree *tree, const char *title, gui_state *state)
{return gui_tree_node(tree, GUI_TREE_NODE_BULLET, title, 0, state);}

enum gui_tree_node_operation
gui_tree_leaf_icon(struct gui_tree *tree, const char *title, struct gui_image img,
    gui_state *state)
{return gui_tree_node(tree, GUI_TREE_NODE_BULLET, title, &img, state);}

void
gui_tree_end_node(struct gui_tree *tree)
{
    GUI_ASSERT(tree->depth);
    tree->depth--;
    tree->at_x -= tree->x_off;
    if (tree->skip == tree->depth)
        tree->skip = -1;
}

struct gui_vec2
gui_tree_end(struct gui_context *p, struct gui_tree* tree)
{return gui_group_end(p, &tree->group);}

/*
 * -------------------------------------------------------------
 *
 *                          GROUPS
 *
 * --------------------------------------------------------------
 */
void
gui_group_begin(struct gui_context *p, struct gui_context *g,
    const char *title, gui_flags flags, struct gui_vec2 offset)
{
    struct gui_rect bounds;
    struct gui_window panel;
    const struct gui_rect *c;

    GUI_ASSERT(p);
    GUI_ASSERT(g);
    if (!p || !g) return;
    if (!p->valid)
        goto failed;

    /* allocate space for the group panel inside the panel */
    c = &p->clip;
    gui_panel_alloc_space(&bounds, p);
    gui_zero(g, sizeof(*g));
    if (!GUI_INTERSECT(c->x, c->y, c->w, c->h, bounds.x, bounds.y, bounds.w, bounds.h))
        goto failed;

    /* initialize a fake panel to create the layout from */
    flags |= GUI_WINDOW_BORDER|GUI_WINDOW_TAB;
    if (p->flags & GUI_WINDOW_ROM)
        flags |= GUI_WINDOW_ROM;

    gui_window_init(&panel, gui_rect(bounds.x, bounds.y,bounds.w,bounds.h),
        flags, 0, p->style, p->input);
    panel.buffer = *p->buffer;
    gui_begin(g, &panel);
    *p->buffer = panel.buffer;
    g->buffer = p->buffer;
    g->offset = offset;
    g->queue = p->queue;

    {
        /* setup correct clipping rectangle for the group */
        struct gui_rect clip;
        struct gui_command_buffer *out = p->buffer;
        gui_unify(&clip, &p->clip, g->clip.x, g->clip.y, g->clip.x + g->clip.w,
            g->clip.y + g->clip.h);
        gui_command_buffer_push_scissor(out, clip);

        /* calculate the group clipping rect */
        if (title) gui_header(g, title, 0, 0, GUI_HEADER_LEFT);
        gui_unify(&clip, &p->clip, g->clip.x, g->clip.y, g->clip.x + g->clip.w,
            g->clip.y + g->clip.h);

        gui_command_buffer_push_scissor(out, clip);
        g->clip = clip;
    }
    return;

failed:
    /* invalid panels still need correct data */
    g->valid = gui_false;
    g->style = p->style;
    g->buffer = p->buffer;
    g->input = p->input;
    g->queue = p->queue;
}

struct gui_vec2
gui_group_end(struct gui_context *p, struct gui_context *g)
{
    struct gui_window pan;
    struct gui_command_buffer *out;
    struct gui_rect clip;

    GUI_ASSERT(p);
    GUI_ASSERT(g);
    if (!p || !g) return gui_vec2(0,0);
    if (!p->valid) return gui_vec2(0,0);
    gui_zero(&pan, sizeof(pan));

    out = p->buffer;
    pan.bounds.x = g->bounds.x;
    pan.bounds.y = g->bounds.y;
    pan.bounds.w = g->width;
    pan.bounds.h = g->height;
    pan.flags = g->flags|GUI_WINDOW_TAB;

    /* setup clipping rect to finalize group panel drawing back to parent */
    gui_unify(&clip, &p->clip, g->clip.x, g->clip.y, g->bounds.x + g->bounds.w,
        g->bounds.y + g->bounds.h);
    gui_command_buffer_push_scissor(out, clip);
    gui_end(g, &pan);
    gui_command_buffer_push_scissor(out, p->clip);
    return pan.offset;
}

/*
 * -------------------------------------------------------------
 *
 *                          SHELF
 *
 * --------------------------------------------------------------
 */
gui_size
gui_shelf_begin(struct gui_context *parent, struct gui_context *shelf,
    const char *tabs[], gui_size size, gui_size active, struct gui_vec2 offset)
{
    const struct gui_style *config;
    struct gui_command_buffer *out;
    const struct gui_user_font *font;
    struct gui_vec2 item_padding;
    struct gui_vec2 panel_padding;

    struct gui_rect bounds;
    struct gui_rect *c;
    struct gui_window panel;

    gui_float header_x, header_y;
    gui_float header_w, header_h;

    GUI_ASSERT(parent);
    GUI_ASSERT(tabs);
    GUI_ASSERT(shelf);
    GUI_ASSERT(active < size);
    if (!parent || !shelf || !tabs || active >= size)
        return active;
    if (!parent->valid)
        goto failed;

    config = parent->style;
    out = parent->buffer;
    font = &config->font;
    item_padding = gui_style_property(config, GUI_PROPERTY_ITEM_PADDING);
    panel_padding = gui_style_property(config, GUI_PROPERTY_PADDING);

    /* allocate space for the shelf */
    gui_panel_alloc_space(&bounds, parent);
    gui_zero(shelf, sizeof(*shelf));
    c = &parent->clip;
    if (!GUI_INTERSECT(c->x, c->y, c->w, c->h, bounds.x, bounds.y, bounds.w, bounds.h))
        goto failed;

    /* calculate the header height for the tabs */
    header_x = bounds.x;
    header_y = bounds.y;
    header_w = bounds.w;
    header_h = panel_padding.y + 3 * item_padding.y + config->font.height;

    {
        /* basic button setup valid for every tab */
        const struct gui_input *input;
        struct gui_button_text button;
        gui_float item_width;
        gui_size i;

        input = (parent->flags & GUI_WINDOW_ROM) ? 0 : parent->input;
        item_width = (header_w - (gui_float)size) / (gui_float)size;
        gui_fill_button(config, &button.base);
        button.base.rounding = 0;
        button.alignment = GUI_TEXT_CENTERED;
        for (i = 0; i < size; i++) {
            struct gui_rect b = {0,0,0,0};
            /* calculate the needed space for the tab text */
            gui_size text_width = font->width(font->userdata,
                (const gui_char*)tabs[i], gui_strsiz(tabs[i]));
            text_width = text_width + (gui_size)(4 * item_padding.x);

            /* calculate current tab button bounds */
            b.y = header_y;
            b.h = header_h;
            b.x = header_x;
            b.w = MIN(item_width, (gui_float)text_width);
            header_x += MIN(item_width, (gui_float)text_width);

            /* set different color for active or inactive tab */
            if ((b.x + b.w) >= (bounds.x + bounds.w)) break;
            if (active != i) {
                b.y += item_padding.y;
                b.h -= item_padding.y;
                button.base.normal = config->colors[GUI_COLOR_SHELF];
                button.base.hover = config->colors[GUI_COLOR_SHELF];
                button.base.active = config->colors[GUI_COLOR_SHELF];

                button.normal = config->colors[GUI_COLOR_SHELF_TEXT];
                button.hover = config->colors[GUI_COLOR_SHELF_TEXT];
                button.active = config->colors[GUI_COLOR_SHELF_TEXT];
            } else {
                button.base.normal = config->colors[GUI_COLOR_SHELF_ACTIVE];
                button.base.hover = config->colors[GUI_COLOR_SHELF_ACTIVE];
                button.base.active = config->colors[GUI_COLOR_SHELF_ACTIVE];

                button.normal = config->colors[GUI_COLOR_SHELF_ACTIVE_TEXT];
                button.hover = config->colors[GUI_COLOR_SHELF_ACTIVE_TEXT];
                button.active = config->colors[GUI_COLOR_SHELF_ACTIVE_TEXT];
            }
            if (gui_widget_button_text(out, b, tabs[i], GUI_BUTTON_DEFAULT, &button,
                input, &config->font)) active = i;
        }
    }
    bounds.y += header_h;
    bounds.h -= header_h;
    {
        /* setup fake panel to create a panel layout */
        gui_flags flags;
        flags = GUI_WINDOW_BORDER|GUI_WINDOW_TAB;
        if (parent->flags & GUI_WINDOW_ROM)
            flags |= GUI_WINDOW_ROM;
        gui_window_init(&panel, gui_rect(bounds.x, bounds.y, bounds.w, bounds.h),
            flags, 0, config, parent->input);

        panel.buffer = *parent->buffer;
        gui_begin(shelf, &panel);
        *parent->buffer = panel.buffer;
        shelf->buffer = parent->buffer;
        shelf->offset = offset;
        shelf->height -= header_h;
        shelf->queue = parent->queue;
    }
    {
        /* setup clip rect for the shelf panel */
        struct gui_rect clip;
        gui_unify(&clip, &parent->clip, shelf->clip.x, shelf->clip.y,
            shelf->clip.x + shelf->clip.w, shelf->clip.y + shelf->clip.h);
        gui_command_buffer_push_scissor(out, clip);
        shelf->clip = clip;
    }
    return active;

failed:
    /* invalid panels still need correct data */
    shelf->valid = gui_false;
    shelf->style = parent->style;
    shelf->buffer = parent->buffer;
    shelf->input = parent->input;
    shelf->queue = parent->queue;
    return active;
}

struct gui_vec2
gui_shelf_end(struct gui_context *p, struct gui_context *s)
{
    struct gui_command_buffer *out;
    struct gui_rect clip;
    struct gui_window pan;

    GUI_ASSERT(p);
    GUI_ASSERT(s);
    if (!p || !s) return gui_vec2(0,0);
    if (!p->valid) return gui_vec2(0,0);
    gui_zero(&pan, sizeof(pan));

    out = p->buffer;
    pan.bounds = s->bounds;
    pan.flags = s->flags|GUI_WINDOW_TAB;

    gui_unify(&clip, &p->clip, s->clip.x, s->clip.y,
        s->bounds.x + s->bounds.w, s->bounds.y + s->bounds.h);
    gui_command_buffer_push_scissor(out, clip);
    gui_end(s, &pan);
    gui_command_buffer_push_scissor(out, p->clip);
    return pan.offset;
}


