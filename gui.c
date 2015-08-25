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
#define gui_vec2_sub(r,a,b) do {(r).x=(a).x-(b).x; (r).y=(a).y-(b).y;} while(0)
#define gui_color_to_array(ar, c)\
    (ar)[0] = (c).r, (ar)[1] = (c).g, (ar)[2] = (c).b, (ar)[3] = (c).a

#define GUI_ALIGN_PTR(x, mask) (void*)((gui_ptr)((gui_byte*)(x) + (mask-1)) & ~(mask-1))
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
static const struct gui_rect gui_null_rect = {-9999.0f, -9999.0f, 2*9999.0f, 2*9999.0f};
static const gui_byte gui_utfbyte[GUI_UTF_SIZE+1] = {0x80, 0, 0xC0, 0xE0, 0xF0};
static const gui_byte gui_utfmask[GUI_UTF_SIZE+1] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
static const long gui_utfmin[GUI_UTF_SIZE+1] = {0, 0, 0x80, 0x800, 0x100000};
static const long gui_utfmax[GUI_UTF_SIZE+1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};
static const double GUI_DOUBLE_PRECISION = 0.0000001;

/*
 * ==============================================================
 *
 *                          Utility
 *
 * ===============================================================
 */
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
    res.x = r.x + amount;
    res.y = r.y + amount;
    res.w = r.w - 2 * amount;
    res.h = r.h - 2 * amount;
    return res;
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

struct gui_image
gui_subimage_ptr(void *ptr, struct gui_rect r)
{
    struct gui_image s;
    s.handle.ptr = ptr;
    s.region = r;
    return s;
}

struct gui_image
gui_subimage_id(gui_int id, struct gui_rect r)
{
    struct gui_image s;
    s.handle.id = id;
    s.region = r;
    return s;
}

struct gui_image
gui_image_ptr(void *ptr)
{
    struct gui_image s;
    GUI_ASSERT(ptr);
    s.handle.ptr = ptr;
    s.region = gui_rect(-1,-1,-1,-1);
    return s;
}

struct gui_image
gui_image_id(gui_int id)
{
    struct gui_image s;
    s.handle.id = id;
    s.region = gui_rect(-1,-1,-1,-1);
    return s;
}

gui_bool
gui_rect_is_valid(const struct gui_rect r)
{
    if (r.x < 0 || r.y < 0 ||
        r.w < 0 || r.h < 0)
        return gui_false;
    return gui_true;
}

gui_bool
gui_image_is_subimage(const struct gui_image* img)
{
    GUI_ASSERT(img);
    return gui_rect_is_valid(img->region);
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

static gui_int
gui_strtof(gui_float *number, const char *buffer)
{
    gui_int i;
    gui_float m;
    gui_bool div;
    gui_int pow;
    const gui_char *p = buffer;
    gui_float floatvalue = 0;

    GUI_ASSERT(number);
    GUI_ASSERT(buffer);
    if (!number || !buffer) return 0;
    *number = 0;

    while( *p && *p != '.' && *p != 'e' ) {
        floatvalue = floatvalue * 10.0f + (gui_float) (*p - '0');
        p++;
    }

    if ( *p == '.' ) {
        p++;
        for(m = 0.1f; *p && *p != 'e'; p++ ) {
            floatvalue = floatvalue + (gui_float) (*p - '0') * m;
            m *= 0.1f;
        }
    }
    if ( *p == 'e' ) {
        p++;
        if ( *p == '-' ) {
            div = gui_true;
            p++;
        }
        else if ( *p == '+' ) {
            div = gui_false;
            p++;
        }
        else div = gui_false;

        for ( pow = 0; *p; p++ )
            pow = pow * 10 + (int) (*p - '0');

        for ( m = 1.0, i = 0; i < pow; i++ )
            m *= 10.0f;

        if ( div )
            floatvalue /= m;
        else floatvalue *= m;
    }
    *number = floatvalue;
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

static gui_double
gui_pow(double x, int n)
{
    /*  check the sign of n */
    gui_double r = 1;
    int plus = n >= 0;
    n = (plus) ? n : -n;
    while (n > 0) {
        if ((n & 1) == 1)
            r *= x;
        n /= 2;
        x *= x;
    }
    return plus ? r : 1.0 / r;
}

static int
gui_isinf(double x)
{
    union {gui_ulong u; double f;} ieee754;
    ieee754.f = x;
    return ( (unsigned)(ieee754.u >> 32) & 0x7fffffff ) == 0x7ff00000 &&
           ( (unsigned)ieee754.u == 0 );
}

static int
gui_isnan(double x)
{
    union {gui_ulong u; double f;} ieee754;
    ieee754.f = x;
    return ( (unsigned)(ieee754.u >> 32) & 0x7fffffff ) +
           ( (unsigned)ieee754.u != 0 ) > 0x7ff00000;
}

static gui_double
gui_floor(gui_double x)
{
    return (gui_double)((gui_int)x - ((x < 0.0) ? 1 : 0));
}

static gui_int
gui_log10(gui_double n)
{
    gui_int neg;
    gui_int ret;
    gui_int exp = 0;

    neg = (n < 0) ? 1 : 0;
    ret = (neg) ? (gui_int)-n : (gui_int)n;
    while ((ret / 10) > 0) {
        ret /= 10;
        exp++;
    }
    if (neg) exp = -exp;
    return exp;
}

static gui_size
gui_dtos(char *s, gui_double n)
{
    gui_int useExp;
    gui_int digit, m, m1 = 0;
    gui_char *c = s;
    gui_int neg;

    if (gui_isnan(n)) {
        s[0] = 'n'; s[1] = 'a'; s[2] = 'n'; s[3] = '\0';
        return 3;
    } else if (gui_isinf(n)) {
        s[0] = 'i'; s[1] = 'n'; s[2] = 'f'; s[3] = '\0';
        return 3;
    } else if (n == 0.0) {
        s[0] = '0'; s[1] = '\0';
        return 1;
    }

    neg = (n < 0);
    if (neg) n = -n;

    /* calculate magnitude */
    m = gui_log10(n);
    useExp = (m >= 14 || (neg && m >= 9) || m <= -9);
    if (neg) *(c++) = '-';

    /* set up for scientific notation */
    if (useExp) {
        if (m < 0)
           m -= 1;
        n = n / gui_pow(10.0, m);
        m1 = m;
        m = 0;
    }
    if (m < 1.0) {
        m = 0;
    }

    /* convert the number */
    while (n > GUI_DOUBLE_PRECISION || m >= 0) {
        gui_double tmp;
        gui_double weight = gui_pow(10.0, m);
        if (weight > 0 && !gui_isinf(weight)) {
            gui_double t = (gui_double)n / weight;
            tmp = gui_floor(t);
            digit = (gui_int)tmp;
            n -= (digit * weight);
            *(c++) = (gui_char)('0' + (gui_char)digit);
        }
        if (m == 0 && n > 0)
            *(c++) = '.';
        m--;
    }

    if (useExp) {
        /* convert the exponent */
        int i, j;
        *(c++) = 'e';
        if (m1 > 0) {
            *(c++) = '+';
        } else {
            *(c++) = '-';
            m1 = -m1;
        }
        m = 0;
        while (m1 > 0) {
            *(c++) = (gui_char)('0' + (gui_char)(m1 % 10));
            m1 /= 10;
            m++;
        }
        c -= m;
        for (i = 0, j = m-1; i<j; i++, j--) {
            /* swap without temporary */
            c[i] ^= c[j];
            c[j] ^= c[i];
            c[i] ^= c[j];
        }
        c += m;
    }
    *(c) = '\0';
    return (gui_size)(c - s);
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

    r.w = MAX(4 * pad_x, r.w);
    r.h = MAX(4 * pad_y, r.h);
    r.w = r.w - 2 * pad_x;
    r.h = r.h - 2 * pad_y;

    r.x = r.x + pad_x;
    r.y = r.y + pad_y;

    w_half = r.w / 2.0f;
    h_half = r.h / 2.0f;

    if (direction == GUI_UP) {
        result[0] = gui_vec2(r.x + w_half, r.y);
        result[1] = gui_vec2(r.x, r.y + r.h);
        result[2] = gui_vec2(r.x + r.w, r.y + r.h);
    } else if (direction == GUI_RIGHT) {
        result[0] = gui_vec2(r.x, r.y);
        result[1] = gui_vec2(r.x, r.y + r.h);
        result[2] = gui_vec2(r.x + r.w, r.y + h_half);
    } else if (direction == GUI_DOWN) {
        result[0] = gui_vec2(r.x, r.y);
        result[1] = gui_vec2(r.x + w_half, r.y + r.h);
        result[2] = gui_vec2(r.x + r.w, r.y);
    } else {
        result[0] = gui_vec2(r.x, r.y + h_half);
        result[1] = gui_vec2(r.x + r.w, r.y + r.h);
        result[2] = gui_vec2(r.x + r.w, r.y);
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

    in->mouse_clicked = 0;
    in->text_len = 0;
    in->scroll_delta = 0;
    gui_vec2_mov(in->mouse_prev, in->mouse_pos);
    for (i = 0; i < GUI_KEY_MAX; i++)
        in->keys[i].clicked = 0;
}

void
gui_input_motion(struct gui_input *in, gui_int x, gui_int y)
{
    GUI_ASSERT(in);
    if (!in) return;
    in->mouse_pos.x = (gui_float)x;
    in->mouse_pos.y = (gui_float)y;
}

void
gui_input_key(struct gui_input *in, enum gui_keys key, gui_bool down)
{
    GUI_ASSERT(in);
    if (!in) return;
    if (in->keys[key].down == down) return;
    in->keys[key].down = down;
    in->keys[key].clicked++;
}

void
gui_input_button(struct gui_input *in, gui_int x, gui_int y, gui_bool down)
{
    GUI_ASSERT(in);
    if (!in) return;
    if (in->mouse_down == down) return;
    in->mouse_clicked_pos.x = (gui_float)x;
    in->mouse_clicked_pos.y = (gui_float)y;
    in->mouse_down = down;
    in->mouse_clicked++;
}

void
gui_input_scroll(struct gui_input *in, gui_float y)
{
    GUI_ASSERT(in);
    if (!in) return;
    in->scroll_delta += y;
}

void
gui_input_glyph(struct gui_input *in, const gui_glyph glyph)
{
    gui_size len = 0;
    gui_long unicode;
    GUI_ASSERT(in);
    if (!in) return;

    len = gui_utf_decode(glyph, &unicode, GUI_UTF_SIZE);
    if (len && ((in->text_len + len) < GUI_INPUT_MAX)) {
        gui_utf_encode(unicode, &in->text[in->text_len], GUI_INPUT_MAX - in->text_len);
        in->text_len += len;
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
    gui_vec2_sub(in->mouse_delta, in->mouse_pos, in->mouse_prev);
}

gui_bool
gui_input_is_mouse_click_in_rect(const struct gui_input *i, struct gui_rect b)
{
    if (!i) return gui_false;
    if (!GUI_INBOX(i->mouse_clicked_pos.x,i->mouse_clicked_pos.y,b.x,b.y,b.w,b.h))
        return gui_false;
    return (i->mouse_down && i->mouse_clicked) ? gui_true : gui_false;
}

gui_bool
gui_input_is_mouse_hovering_rect(const struct gui_input *i, struct gui_rect rect)
{
    if (!i) return gui_false;
    return GUI_INBOX(i->mouse_pos.x, i->mouse_pos.y, rect.x, rect.y, rect.w, rect.h);
}

gui_bool
gui_input_mouse_clicked(const struct gui_input *i, struct gui_rect rect)
{
    if (!i) return gui_false;
    if (!gui_input_is_mouse_hovering_rect(i, rect)) return gui_false;
    return gui_input_is_mouse_click_in_rect(i, rect);
}

gui_bool
gui_input_is_mouse_down(const struct gui_input *i)
{
    if (!i) return gui_false;
    return i->mouse_down;
}

gui_bool
gui_input_is_mouse_released(const struct gui_input *i)
{
    if (!i) return gui_false;
    return (!i->mouse_down && i->mouse_clicked) ? gui_true : gui_false;
}

gui_bool
gui_input_is_key_pressed(const struct gui_input *i, enum gui_keys key)
{
    const struct gui_key *k;
    if (!i) return gui_false;
    k = &i->keys[key];
    if (k->down && k->clicked)
        return gui_true;
    return gui_false;
}

gui_bool
gui_input_is_key_released(const struct gui_input *i, enum gui_keys key)
{
    const struct gui_key *k;
    if (!i) return gui_false;
    k = &i->keys[key];
    if (!k->down && k->clicked)
        return gui_true;
    return gui_false;
}

gui_bool
gui_input_is_key_down(const struct gui_input *i, enum gui_keys key)
{
    const struct gui_key *k;
    if (!i) return gui_false;
    k = &i->keys[key];
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
}

void*
gui_buffer_alloc(struct gui_buffer *b, gui_size size, gui_size align)
{
    gui_size cap;
    gui_size alignment;
    void *unaligned;
    void *memory;

    GUI_ASSERT(b);
    if (!b || !size) return 0;
    b->needed += size;

    /* calculate total size with needed alignment + size */
    unaligned = gui_ptr_add(void, b->memory.ptr, b->allocated);
    if (align) {
        memory = GUI_ALIGN_PTR(unaligned, align);
        alignment = (gui_size)((gui_byte*)memory - (gui_byte*)unaligned);
    } else {
        memory = unaligned;
        alignment = 0;
    }

    /* check if buffer has enough memory*/
    if ((b->allocated + size + alignment) >= b->memory.size) {
        void *temp;
        if (b->type != GUI_BUFFER_DYNAMIC || !b->pool.realloc)
            return 0;

        /* allocated new bigger block of memory if the buffer is dynamic */
        cap = (gui_size)((gui_float)b->memory.size * b->grow_factor);
        temp = b->pool.realloc(b->pool.userdata, b->memory.ptr, cap);
        GUI_ASSERT(temp);
        if (!temp) return 0;

        b->memory.ptr = temp;
        b->memory.size = cap;

        /* align newly allocated pointer to memory */
        unaligned = gui_ptr_add(gui_byte, b->memory.ptr, b->allocated);
        if (align) {
            memory = GUI_ALIGN_PTR(unaligned, align);
            alignment = (gui_size)((gui_byte*)memory - (gui_byte*)unaligned);
        } else {
            memory = unaligned;
            alignment = 0;
        }
    }

    b->allocated += size + alignment;
    b->needed += alignment;
    b->calls++;
    return memory;
}

void
gui_buffer_clear(struct gui_buffer *b)
{
    GUI_ASSERT(b);
    if (!b) return;
    b->allocated = 0;
    b->calls = 0;
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

    cmd = (struct gui_command*)gui_buffer_alloc(b->base, size, align);
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
}

void
gui_command_buffer_push_text(struct gui_command_buffer *b, struct gui_rect r,
    const gui_char *string, gui_size length, const struct gui_font *font,
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
    cmd->font = font->userdata;
    cmd->length = length;
    gui_memcopy(cmd->string, string, length);
    cmd->string[length] = '\0';
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
    static const gui_size cmd_align = GUI_ALIGNOF(struct gui_command);
    struct gui_command_sub_buffer_stack *stack;
    struct gui_command_sub_buffer *buf;
    struct gui_command_sub_buffer *end;
    gui_size offset;
    void *memory;

    GUI_ASSERT(queue);
    GUI_ASSERT(buffer);
    if (!queue || !buffer) return gui_false;

    /* allocate header space from the buffer */
    stack = &queue->stack;
    memory = queue->buffer.memory.ptr;
    buf = (struct gui_command_sub_buffer*)
        gui_buffer_alloc(&queue->buffer, buf_size, buf_align);
    if (!buf) return gui_false;
    offset = (gui_size)(((gui_byte*)buf) - (gui_byte*)memory);
    buffer->end += buf_size;

    {
        /* fix last command to offset over the header */
        struct gui_command *cmd;
        cmd = gui_ptr_add(struct gui_command, memory, buffer->last);
        cmd->next = buffer->end;
        cmd->next = GUI_ALIGN(cmd->next, cmd_align);
    }

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

    GUI_ASSERT(queue);
    GUI_ASSERT(buffer);
    if (!queue || !buffer) return;
    if (!queue->stack.count) return;

    stack = &queue->stack;
    buf = gui_ptr_add(struct gui_command_sub_buffer, queue->buffer.memory.ptr, stack->end);
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
    gui_size i = 0;
    struct gui_command_sub_buffer *iter;
    struct gui_command_sub_buffer_stack *stack;

    GUI_ASSERT(queue);
    GUI_ASSERT(buffer);

    if (!queue || !buffer) return;
    stack = &queue->stack;
    buffer->end = queue->buffer.allocated;
    if (!stack->count) return;

    /* fix buffer command list for subbuffers  */
    memory = queue->buffer.memory.ptr;
    iter = gui_ptr_add(struct gui_command_sub_buffer, memory, stack->begin);
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
        iter = gui_ptr_add(struct gui_command_sub_buffer, memory, iter->next);
    }
    queue->stack.count = 0;
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
    mem = (char*)gui_buffer_alloc(buffer, len * sizeof(char), 0);
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

    mem = gui_buffer_alloc(buffer, len * sizeof(char), 0);
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
    glyph_len = gui_utf_decode(i->text, &unicode, i->text_len);
    while (glyph_len && ((text_len+glyph_len) <= i->text_len)) {
        /* filter to make sure the value is correct */
        if (box->filter(unicode)) {
            gui_edit_box_add(box, &i->text[text_len], glyph_len);
            text_len += glyph_len;
            glyphes++;
        }
        src_len = src_len + glyph_len;
        glyph_len = gui_utf_decode(i->text + src_len, &unicode, i->text_len - src_len);
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
gui_text(struct gui_command_buffer *o, struct gui_rect b,
    const char *string, gui_size len, const struct gui_text *t,
    enum gui_text_align a, const struct gui_font *f)
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
        len, f, t->background, t->foreground);
}

static gui_bool
gui_do_button(struct gui_command_buffer *o, struct gui_rect r,
    const struct gui_button *b, const struct gui_input *i,
    enum gui_button_behavior behavior)
{
    gui_bool ret = gui_false;
    struct gui_color background;
    GUI_ASSERT(b);
    if (!o || !b)
        return gui_false;

    /* make sure correct values */
    r.w = MAX(r.w, 2 * b->border);
    r.h = MAX(r.h, 2 * b->border);

    /* general button user input logic */
    background = b->background;
    if (gui_input_is_mouse_hovering_rect(i, r)) {
        background = b->highlight;
        if (gui_input_is_mouse_click_in_rect(i, r)) {
            ret = (behavior != GUI_BUTTON_DEFAULT) ? i->mouse_down:
                (i->mouse_down && i->mouse_clicked);
        }
    }
    gui_command_buffer_push_rect(o, r, b->rounding, b->foreground);
    gui_command_buffer_push_rect(o, gui_shrink_rect(r, b->border),
                                    b->rounding, background);
    return ret;
}

gui_bool
gui_button_text(struct gui_command_buffer *o, struct gui_rect r,
     const char *string, enum gui_button_behavior behavior,
    const struct gui_button *b, const struct gui_input *i,
    const struct gui_font *f)
{
    gui_bool ret = gui_false;
    struct gui_text t;
    struct gui_color font_color;
    struct gui_color bg_color;
    struct gui_rect inner;

    GUI_ASSERT(b);
    GUI_ASSERT(o);
    GUI_ASSERT(string);
    GUI_ASSERT(f);
    if (!o || !b || !f)
        return gui_false;

    /* basic button drawing and logic */
    font_color = b->content;
    bg_color = b->background;
    r.w = MAX(r.w, (2 * b->border + 2 * b->rounding));
    r.h = MAX(r.h, f->height + 2 * b->padding.y);
    if (gui_input_is_mouse_hovering_rect(i, r)) {
        font_color = b->highlight_content;
        bg_color = b->highlight;
    }
    ret = gui_do_button(o, r, b, i, behavior);

    /* calculate text bounds */
    inner.x = r.x + b->border + b->rounding;
    inner.y = r.y + b->border;
    inner.w = r.w - (2 * b->border + 2 * b->rounding);
    inner.h = r.h - (2 * b->border);

    /* draw text inside button */
    t.padding.x = b->padding.x;
    t.padding.y = b->padding.y;
    t.background = bg_color;
    t.foreground = font_color;
    gui_text(o, inner, string, gui_strsiz(string), &t, b->alignment, f);
    return ret;
}

gui_bool
gui_button_triangle(struct gui_command_buffer *out, struct gui_rect r,
    enum gui_heading heading, enum gui_button_behavior bh,
    const struct gui_button *b, const struct gui_input *in)
{
    gui_bool pressed;
    struct gui_color col;
    struct gui_vec2 points[3];

    GUI_ASSERT(b);
    GUI_ASSERT(out);
    if (!out || !b)
        return gui_false;

    pressed = gui_do_button(out, r, b, in, bh);
    gui_triangle_from_direction(points, r, b->padding.x, b->padding.y, heading);
    col = (gui_input_is_mouse_hovering_rect(in, r)) ?
        b->highlight_content : b->content;
    gui_command_buffer_push_triangle(out,  points[0].x, points[0].y,
        points[1].x, points[1].y, points[2].x, points[2].y, col);
    return pressed;
}

gui_bool
gui_button_image(struct gui_command_buffer *out, struct gui_rect r,
    struct gui_image img, enum gui_button_behavior b,
    const struct gui_button *button, const struct gui_input *in)
{
    gui_bool pressed;
    struct gui_rect bounds;
    GUI_ASSERT(button);
    GUI_ASSERT(out);
    if (!out || !button)
        return gui_false;

    /* make sure correct values */
    r.w = MAX(r.w, 2 * button->padding.x);
    r.h = MAX(r.h, 2 * button->padding.y);

    /* execute basic button logic/drawing and finally draw image into the button */
    pressed = gui_do_button(out, r, button, in, b);
    bounds.x = r.x + button->padding.x;
    bounds.y = r.y + button->padding.y;
    bounds.w = r.w - 2 * button->padding.x;
    bounds.h = r.h - 2 * button->padding.y;
    gui_command_buffer_push_image(out, bounds, &img);
    return pressed;
}

gui_bool
gui_button_text_triangle(struct gui_command_buffer *out, struct gui_rect r,
    enum gui_heading heading, const char *text, enum gui_text_align align,
    enum gui_button_behavior behavior, const struct gui_button *button,
    const struct gui_font *f, const struct gui_input *i)
{
    gui_bool pressed;
    struct gui_rect tri;
    struct gui_color col;
    struct gui_vec2 points[3];

    GUI_ASSERT(button);
    GUI_ASSERT(out);
    if (!out || !button)
        return gui_false;

    r.h = MAX(1, r.h);
    pressed = gui_button_text(out, r, text, behavior, button, i, f);

    /* calculate triangle bounds */
    tri.y = r.y + (r.h/2) - f->height/2;
    tri.w = tri.h = f->height;
    if (align == GUI_TEXT_LEFT) {
        tri.x = (r.x + r.w) - (2 * button->padding.x + tri.w);
        tri.x = MAX(tri.x, 0);
    } else tri.x = r.x + 2 * button->padding.x;
    col = (gui_input_is_mouse_hovering_rect(i, r)) ?
        button->highlight_content : button->content;

    /* draw triangle */
    gui_triangle_from_direction(points, tri, 0, 0, heading);
    gui_command_buffer_push_triangle(out,  points[0].x, points[0].y,
        points[1].x, points[1].y, points[2].x, points[2].y, col);
    return pressed;
}

gui_bool
gui_button_text_image(struct gui_command_buffer *out, struct gui_rect r,
    struct gui_image img, const char* text, enum gui_text_align align,
    enum gui_button_behavior behavior, const struct gui_button *button,
    const struct gui_font *f, const struct gui_input *i)
{
    gui_bool pressed;
    struct gui_rect icon;
    GUI_ASSERT(button);
    GUI_ASSERT(out);
    if (!out || !button)
        return gui_false;

    pressed = gui_button_text(out, r, text, behavior, button, i, f);
    icon.y = r.y + button->padding.y;
    icon.w = icon.h = r.h - 2 * button->padding.y;
    if (align == GUI_TEXT_LEFT) {
        icon.x = (r.x + r.w) - (2 * button->padding.x + icon.w);
        icon.x = MAX(icon.x, 0);
    } else icon.x = r.x + 2 * button->padding.x;
    gui_command_buffer_push_image(out, icon, &img);
    return pressed;
}

gui_bool
gui_toggle(struct gui_command_buffer *out, struct gui_rect r,
    gui_bool active, const char *string, enum gui_toggle_type type,
    const struct gui_toggle *toggle, const struct gui_input *in,
    const struct gui_font *font)
{
    gui_bool toggle_active;
    struct gui_rect select;
    struct gui_rect cursor;
    gui_float cursor_pad;

    GUI_ASSERT(toggle);
    GUI_ASSERT(out);
    GUI_ASSERT(font);
    if (!out || !toggle || !font)
        return 0;

    /* make sure correct values */
    r.w = MAX(r.w, font->height + 2 * toggle->padding.x);
    r.h = MAX(r.h, font->height + 2 * toggle->padding.y);
    toggle_active = active;

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
    toggle_active = gui_input_mouse_clicked(in, cursor) ? !toggle_active : toggle_active;

    /* draw radiobutton/checkbox background */
    if (type == GUI_TOGGLE_CHECK)
        gui_command_buffer_push_rect(out, select , toggle->rounding, toggle->foreground);
    else gui_command_buffer_push_circle(out, select, toggle->foreground);

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
        text.background = toggle->background;
        text.foreground = toggle->font;
        gui_text(out, inner, string, gui_strsiz(string), &text, GUI_TEXT_LEFT, font);
    }
    return toggle_active;
}

gui_float
gui_slider(struct gui_command_buffer *out, struct gui_rect slider,
    gui_float min, gui_float val, gui_float max, gui_float step,
    const struct gui_slider *s, const struct gui_input *in)
{
    gui_float slider_range;
    gui_float slider_min, slider_max;
    gui_float slider_value, slider_steps;
    gui_float cursor_offset;
    struct gui_rect cursor;
    struct gui_rect bar;

    GUI_ASSERT(s);
    GUI_ASSERT(out);
    if (!out || !s)
        return 0;

    /* make sure the provided values are correct */
    slider.x = slider.x + s->padding.x;
    slider.h = MAX(slider.h, 2 * s->padding.y);
    slider.w = MAX(slider.w, 1 + slider.h + 2 * s->padding.x);
    slider_max = MAX(min, max);
    slider_min = MIN(min, max);
    slider_value = CLAMP(slider_min, val, slider_max);
    slider_range = slider_max - slider_min;
    slider_steps = slider_range / step;

    /* calculate slider virtual cursor bounds */
    cursor_offset = (slider_value - slider_min) / step;
    cursor.h = slider.h - 2 * s->padding.y;
    cursor.w = (slider.w - (2 * s->padding.x)) / (slider_steps + 1);
    cursor.x = slider.x + (cursor.w * cursor_offset);
    cursor.y = slider.y + s->padding.y;

    /* calculate slider background bar bounds */
    bar.x = slider.x;
    bar.y = (cursor.y + cursor.h/2) - cursor.h/8;
    bar.w = (slider.w - (2 * s->padding.x));
    bar.h = cursor.h/4;

    /* updated the slider value by user input */
    if (in && in->mouse_down &&
        GUI_INBOX(in->mouse_pos.x,in->mouse_pos.y,slider.x,slider.y,slider.w,slider.h) &&
        GUI_INBOX(in->mouse_clicked_pos.x,in->mouse_clicked_pos.y,slider.x,
                    slider.y,slider.w,slider.h))
    {
        const float d = in->mouse_pos.x - (cursor.x + cursor.w / 2.0f);
        const float pxstep = (slider.w - (2 * s->padding.x)) / slider_steps;

        /* only update value if the next slider step is reached*/
        if (GUI_ABS(d) >= pxstep) {
            const gui_float steps = (gui_float)((gui_int)(GUI_ABS(d) / pxstep));
            slider_value += (d > 0) ? (step * steps) : -(step * steps);
            slider_value = CLAMP(slider_min, slider_value, slider_max);
            cursor.x = slider.x + (cursor.w * ((slider_value - slider_min) / step));
        }
    }

    {
        /* NOTE: this is a shitty hack since I am to stupid for math */
        cursor.w = cursor.h;
        cursor.x = (slider_value <= slider_min) ? cursor.x:
            (slider_value >= slider_max) ? ((bar.x + bar.w) - cursor.h) :
            cursor.x + (cursor.w/2) - cursor.h/2;

        /* draw slider with background and circle cursor*/
        gui_command_buffer_push_rect(out, bar, 0, s->bar);
        gui_command_buffer_push_circle(out, cursor, s->border);
        gui_command_buffer_push_circle(out, gui_shrink_rect(cursor, 1), s->fg);
    }
    return slider_value;
}

gui_size
gui_progress(struct gui_command_buffer *out, struct gui_rect r,
    gui_size value, gui_size max, gui_bool modifyable,
    const struct gui_progress *prog, const struct gui_input *in)
{
    struct gui_rect cursor;
    gui_float prog_scale;
    gui_size prog_value;

    GUI_ASSERT(prog);
    GUI_ASSERT(out);
    if (!out || !prog) return 0;

    /* make sure given values are correct */
    r.w = MAX(r.w, 2 * prog->padding.x + 1);
    r.h = MAX(r.h, 2 * prog->padding.y + 1);
    prog_value = MIN(value, max);

    /* update progress by from user input if modifyable */
    if (in && modifyable && in->mouse_down &&
        GUI_INBOX(in->mouse_pos.x, in->mouse_pos.y, r.x, r.y, r.w, r.h)){
        gui_float ratio = (gui_float)(in->mouse_pos.x - r.x) / (gui_float)r.w;
        prog_value = (gui_size)((gui_float)max * ratio);
    }

    if (!max) return prog_value;
    /* make sure calculated values are correct */
    prog_value = MIN(prog_value, max);
    prog_scale = (gui_float)prog_value / (gui_float)max;

    /* calculate progress bar cursor */
    cursor.h = r.h - 2 * prog->padding.y;
    cursor.w = (r.w - 2 * prog->padding.x) * prog_scale;
    cursor.x = r.x + prog->padding.x;
    cursor.y = r.y + prog->padding.y;

    /* draw progressbar width background and cursor */
    gui_command_buffer_push_rect(out, r, prog->rounding, prog->background);
    gui_command_buffer_push_rect(out, cursor, prog->rounding, prog->foreground);
    return prog_value;
}

static gui_size
gui_font_glyph_index_at_pos(const struct gui_font *font, const char *text,
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
gui_font_glyphes_fitting_in_space(const struct gui_font *font, const char *text,
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
gui_editbox(struct gui_command_buffer *out, struct gui_rect r,
    struct gui_edit_box *box, const struct gui_edit *field,
    const struct gui_input *in, const struct gui_font *font)
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
    if (in && in->mouse_clicked && in->mouse_down)
        box->active = GUI_INBOX(in->mouse_pos.x,in->mouse_pos.y,r.x,r.y,r.w,r.h);

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
        if (in->text_len) {
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
        label.w = r.w - 2 * field->padding.x - 2 * field->border_size;
        {
            gui_size frames = 0;
            gui_size glyphes = 0;
            gui_size frame_len = 0;
            gui_float space = label.w - (gui_float)cursor_w;
            while (text_len) {
                frames++;
                offset += frame_len;
                frame_len = gui_font_glyphes_fitting_in_space(font,
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
        if (in && field->show_cursor && in->mouse_down && box->active) {
            const gui_char *visible = &buffer[offset];
            gui_float xoff = in->mouse_pos.x-(r.x+field->padding.x+field->border_size);
            if (GUI_INBOX(in->mouse_pos.x, in->mouse_pos.y, r.x, r.y, r.w, r.h))
            {
                /* text selection in the current text frame */
                gui_size glyph_index;
                gui_size glyph_pos=gui_font_glyph_index_at_pos(font,visible,text_len,xoff);
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
            } else if (!GUI_INBOX(in->mouse_pos.x,in->mouse_pos.y,r.x,r.y,r.w,r.h) &&
                GUI_INBOX(in->mouse_clicked_pos.x,in->mouse_clicked_pos.y,r.x,r.y,r.w,r.h)
                && box->cursor != box->glyphes && box->cursor > 0)
            {
                /* text selection out of the current text frame */
                gui_size glyph = ((in->mouse_pos.x > r.x) &&
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
gui_edit_filtered(struct gui_command_buffer *out, struct gui_rect r,
    gui_char *buffer, gui_size len, gui_size max, gui_state *active,
    gui_size *cursor,  const struct gui_edit *field, gui_filter filter,
    const struct gui_input *in, const struct gui_font *font)
{
    struct gui_edit_box box;
    gui_edit_box_init_fixed(&box, buffer, max, 0, filter);
    box.buffer.allocated = len;
    box.active = *active;
    box.glyphes = gui_utf_len(buffer, len);
    if (!cursor) box.cursor = box.glyphes;
    else box.cursor = MIN(*cursor, box.glyphes);
    gui_editbox(out, r, &box, field, in, font);
    *active = box.active;
    return gui_edit_box_len(&box);
}

gui_size
gui_edit(struct gui_command_buffer *out, struct gui_rect r,
    gui_char *buffer, gui_size len, gui_size max, gui_state *active,
    gui_size *cursor, const struct gui_edit *field, enum gui_input_filter f,
    const struct gui_input *in, const struct gui_font *font)
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
    return gui_edit_filtered(out, r, buffer, len, max, active,
            cursor, field, filter[f], in, font);
}

gui_float
gui_scrollbar_vertical(struct gui_command_buffer *out, struct gui_rect scroll,
    gui_float offset, gui_float target, gui_float step,
    const struct gui_scrollbar *s, const struct gui_input *i)
{
    gui_bool button_up_pressed;
    gui_bool button_down_pressed;
    struct gui_button button;
    struct gui_rect cursor;

    gui_float scroll_step;
    gui_float scroll_offset;
    gui_float scroll_off, scroll_ratio;
    gui_bool inscroll, incursor;

    GUI_ASSERT(out);
    GUI_ASSERT(s);
    if (!out || !s) return 0;

    /* scrollbar background */
    scroll.w = MAX(scroll.w, 1);
    scroll.h = MAX(scroll.h, 2 * scroll.w);
    if (target <= scroll.h) return 0;
    gui_command_buffer_push_rect(out, scroll, s->rounding, s->background);

    /* setup and execute up/down button */
    button.border = 1;
    button.rounding = 0;
    button.padding.x = scroll.w / 4;
    button.padding.y = scroll.w / 4;
    button.background = s->background;
    button.foreground =  s->foreground;
    button.content = s->foreground;
    button.highlight = s->background;
    button.highlight_content = s->foreground;

    button_up_pressed = gui_button_triangle(out,
        gui_rect(scroll.x, scroll.y, scroll.w, scroll.w),
         GUI_UP, GUI_BUTTON_DEFAULT, &button, i);
    button_down_pressed = gui_button_triangle(out,
        gui_rect(scroll.x, scroll.y + scroll.h - scroll.w, scroll.w, scroll.w),
        GUI_DOWN, GUI_BUTTON_DEFAULT, &button, i);

    /* calculate scrollbar constants */
    scroll.h = scroll.h - 2 * scroll.w;
    scroll.y = scroll.y + scroll.w;
    scroll_step = MIN(step, scroll.h);
    scroll_offset = MIN(offset, target - scroll.h);
    scroll_ratio = scroll.h / target;
    scroll_off = scroll_offset / target;

    /* calculate scrollbar cursor bounds */
    cursor.h = scroll_ratio * scroll.h;
    cursor.y = scroll.y + (scroll_off * scroll.h);
    cursor.w = scroll.w;
    cursor.x = scroll.x;

    if (i) {
        const struct gui_vec2 mouse_pos = i->mouse_pos;
        const struct gui_vec2 mouse_prev = i->mouse_prev;
        inscroll=GUI_INBOX(mouse_pos.x,mouse_pos.y,scroll.x,scroll.y,scroll.w,scroll.h);
        incursor=GUI_INBOX(mouse_prev.x, mouse_prev.y, cursor.x, cursor.y,
                                cursor.w, cursor.h);

        if (i->mouse_down && inscroll && incursor) {
            /* update cursor by mouse dragging */
            const gui_float pixel = i->mouse_delta.y;
            const gui_float delta =  (pixel / scroll.h) * target;
            scroll_offset = CLAMP(0, scroll_offset + delta, target - scroll.h);
        } else if (button_up_pressed || button_down_pressed) {
            /* update cursor by up/down button */
            scroll_offset = (button_down_pressed) ?
                MIN(scroll_offset + scroll_step, target - scroll.h):
                MAX(0, scroll_offset - scroll_step);
        } else if (s->has_scrolling && ((i->scroll_delta < 0) || (i->scroll_delta>0))) {
            /* update cursor by mouse scrolling */
            scroll_offset = scroll_offset + scroll_step * (-i->scroll_delta);
            scroll_offset = CLAMP(0, scroll_offset, target - scroll.h);
        }
        scroll_off = scroll_offset / target;
        cursor.y = scroll.y + (scroll_off * scroll.h);
    }

    /* draw scrollbar cursor */
    gui_command_buffer_push_rect(out, cursor, s->rounding, s->foreground);
    return scroll_offset;
}

gui_float
gui_scrollbar_horizontal(struct gui_command_buffer *out, struct gui_rect scroll,
    gui_float offset, gui_float target, gui_float step,
    const struct gui_scrollbar *s, const struct gui_input *i)
{
    gui_bool button_left_pressed;
    gui_bool button_right_pressed;
    struct gui_button button;
    struct gui_rect cursor;

    gui_float scroll_step;
    gui_float scroll_offset;
    gui_float scroll_off, scroll_ratio;
    gui_bool inscroll, incursor;

    GUI_ASSERT(out);
    GUI_ASSERT(s);
    if (!out || !s) return 0;

    /* scrollbar background */
    scroll.h = MAX(scroll.h, 1);
    scroll.w = MAX(scroll.w, 2 * scroll.h);
    if (target <= scroll.w) return 0;
    gui_command_buffer_push_rect(out, scroll, s->rounding,s->background);

    /* setup and execute up/down button */
    button.border = 1;
    button.rounding = 0;
    button.padding.x = scroll.h / 4;
    button.padding.y = scroll.h / 4;
    button.background = s->background;
    button.foreground =  s->foreground;
    button.content = s->foreground;
    button.highlight = s->background;
    button.highlight_content = s->foreground;

    button_right_pressed = gui_button_triangle(out, gui_rect(scroll.x, scroll.y,
            scroll.h, scroll.h), GUI_LEFT, GUI_BUTTON_DEFAULT, &button, i);
    button_left_pressed = gui_button_triangle(out,
            gui_rect(scroll.x + scroll.w - scroll.h, scroll.y, scroll.h, scroll.h),
            GUI_RIGHT, GUI_BUTTON_DEFAULT, &button, i);

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

    if (i) {
        const struct gui_vec2 mouse_pos = i->mouse_pos;
        const struct gui_vec2 mouse_prev = i->mouse_prev;
        inscroll=GUI_INBOX(mouse_pos.x,mouse_pos.y,scroll.x,scroll.y,scroll.w,scroll.h);
        incursor = GUI_INBOX(mouse_prev.x, mouse_prev.y, cursor.x, cursor.y,
                                cursor.w, cursor.h);

        if (i->mouse_down && inscroll && incursor) {
            /* update cursor by mouse dragging */
            const gui_float pixel = i->mouse_delta.x;
            const gui_float delta =  (pixel / scroll.w) * target;
            scroll_offset = CLAMP(0, scroll_offset + delta, target - scroll.w);
        } else if (button_left_pressed || button_right_pressed) {
            /* update cursor by up/down button */
            scroll_offset = (button_left_pressed) ?
                MIN(scroll_offset + scroll_step, target - scroll.w):
                MAX(0, scroll_offset - scroll_step);
        } else if (s->has_scrolling && ((i->scroll_delta < 0) || (i->scroll_delta>0))) {
            /* update cursor by mouse scrolling */
            scroll_offset = scroll_offset + scroll_step * (-i->scroll_delta);
            scroll_offset = CLAMP(0, scroll_offset, target - scroll.w);
        }
        scroll_off = scroll_offset / target;
        cursor.x = scroll.x + (scroll_off * scroll.w);
    }

    /* draw scrollbar cursor */
    gui_command_buffer_push_rect(out, cursor, s->rounding, s->foreground);
    return scroll_offset;

}

static gui_int
gui_spinner_base(struct gui_command_buffer *out, struct gui_rect r,
    const struct gui_spinner *s, gui_char *buffer, gui_size *len,
    enum gui_input_filter filter, gui_state *active,
    const struct gui_input *in, const struct gui_font *font)
{
    gui_int ret = 0;
    struct gui_rect bounds;
    struct gui_edit field;
    struct gui_button button;
    gui_bool button_up_clicked, button_down_clicked;
    gui_state is_active = (active) ? *active : gui_false;


    r.h = MAX(r.h, font->height + 2 * s->padding.x);
    r.w = MAX(r.w, r.h - s->padding.x + (gui_float)s->border_button * 2);

    /* up/down button setup and execution */
    bounds.y = r.y;
    bounds.h = r.h / 2;
    bounds.w = r.h - s->padding.x;
    bounds.x = r.x + r.w - bounds.w;
    button.rounding = 0;
    button.border = (gui_float)s->border_button;
    button.padding.x = MAX(3, (bounds.h - font->height) / 2);
    button.padding.y = MAX(3, (bounds.h - font->height) / 2);
    button.background = s->button_color;
    button.foreground = s->button_border;
    button.content = s->button_triangle;
    button.highlight = s->button_color;
    button.highlight_content = s->button_triangle;
    button_up_clicked = gui_button_triangle(out, bounds, GUI_UP,
        GUI_BUTTON_DEFAULT, &button, in);
    if (button_up_clicked) ret = 1;

    bounds.y = r.y + bounds.h;
    button_down_clicked = gui_button_triangle(out, bounds, GUI_DOWN,
        GUI_BUTTON_DEFAULT, &button, in);
    if (button_down_clicked) ret = -1;

    /* editbox setup and execution */
    bounds.x = r.x;
    bounds.y = r.y;
    bounds.h = r.h;
    bounds.w = r.w - (bounds.w - button.border * 2);
    field.border_size = 1;
    field.rounding = 0;
    field.padding.x = s->padding.x;
    field.padding.y = s->padding.y;
    field.show_cursor = s->show_cursor;
    field.background = s->color;
    field.border = s->border;
    field.text = s->text;
    *len = gui_edit(out, bounds, buffer, *len, GUI_MAX_NUMBER_BUFFER, &is_active,
                    0, &field, filter, in, font);
    if (active) *active = is_active;
    return ret;
}

gui_int
gui_spinner_int(struct gui_command_buffer *out, struct gui_rect r,
    const struct gui_spinner *s, gui_int min, gui_int value,
    gui_int max, gui_int step, gui_state *active, const struct gui_input *in,
    const struct gui_font *font)
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

    res = gui_spinner_base(out, r, s, string, &len, GUI_INPUT_DEC, active, in, font);
    if (res) {
        value += (res > 0) ? step : -step;
        value = CLAMP(min, value, max);
    }

    if (old_len != len)
        gui_strtoi(&value, string, len);
    if (active) *active = is_active;
    return value;
}

gui_float
gui_spinner_float(struct gui_command_buffer *out, struct gui_rect bounds,
    const struct gui_spinner *s, gui_float min, gui_float value,
    gui_float max, gui_float step, gui_state *active,
    const struct gui_input *in, const struct gui_font *font)
{
    gui_int res;
    char string[GUI_MAX_NUMBER_BUFFER];
    gui_size len, old_len;
    gui_state is_active;

    GUI_ASSERT(out);
    GUI_ASSERT(s);
    GUI_ASSERT(font);
    if (!out || !s || !font) return value;

    /* make sure given values are correct */
    value = CLAMP(min, value, max);
    len = gui_dtos(string, value);
    is_active = (active) ? *active : gui_false;
    old_len = len;

    res = gui_spinner_base(out, bounds, s, string, &len, GUI_INPUT_FLOAT, active, in, font);
    if (res) {
        value += (res > 0) ? step : -step;
        value = CLAMP(min, value, max);
    }

    if (old_len != len)
        gui_strtof(&value, string);
    if (active) *active = is_active;
    return value;
}

gui_size
gui_selector(struct gui_command_buffer *out, struct gui_rect r,
    const struct gui_selector *s, const char *items[],
    gui_size item_count, gui_size item_current, const struct gui_input *input,
    const struct gui_font *font)
{
    gui_size text_len;
    struct gui_rect label;
    struct gui_rect bounds;
    struct gui_button button;
    gui_bool button_up_clicked;
    gui_bool button_down_clicked;

    GUI_ASSERT(items);
    GUI_ASSERT(item_count);
    GUI_ASSERT(item_current < item_count);

    r.h = MAX(r.h, font->height + 2 * s->padding.x);
    r.w = MAX(r.w, (r.h - s->padding.x) + 2 * s->padding.x);

    /* draw selector background and border */
    gui_command_buffer_push_rect(out, r, 0, s->border);
    gui_command_buffer_push_rect(out, gui_shrink_rect(r, 1), 0, s->color);

    /* up/down button setup and execution */
    bounds.y = r.y;
    bounds.h = r.h / 2;
    bounds.w = r.h - s->padding.x;
    bounds.x = r.x + r.w - bounds.w;
    button.rounding = 0;
    button.border = (gui_float)s->border_button;
    button.padding.x = MAX(3, (bounds.h - font->height) / 2);
    button.padding.y = MAX(3, (bounds.h - font->height) / 2);
    button.background = s->button_color;
    button.foreground = s->button_border;
    button.content = s->button_triangle;
    button.highlight = s->button_color;
    button.highlight_content = s->button_triangle;
    button_up_clicked = gui_button_triangle(out, bounds,
        GUI_UP, GUI_BUTTON_DEFAULT, &button, input);

    bounds.y = r.y + bounds.h;
    button_down_clicked = gui_button_triangle(out, bounds,
        GUI_DOWN, GUI_BUTTON_DEFAULT, &button,  input);
    item_current = (button_down_clicked && item_current > 0) ?
        item_current-1 : (button_up_clicked && item_current < item_count-1) ?
        item_current+1 : item_current;

    /* calculate text bounds and draw current selected selection text */
    label.x = r.x + s->padding.x;
    label.y = r.y + s->padding.y;
    label.w = r.w - (bounds.w + 2 * s->padding.x);
    label.h = r.h - 2 * s->padding.y;
    text_len = gui_strsiz(items[item_current]);
    gui_command_buffer_push_text(out, label, (const gui_char*)items[item_current],
            text_len, font, s->text_bg, s->text);
    return item_current;
}

/*
 * ==============================================================
 *
 *                          Config
 *
 * ===============================================================
 */
static void
gui_config_default_properties(struct gui_config *config)
{
    config->properties[GUI_PROPERTY_SCROLLBAR_SIZE] = gui_vec2(16, 16);
    config->properties[GUI_PROPERTY_PADDING] = gui_vec2(15.0f, 10.0f);
    config->properties[GUI_PROPERTY_SIZE] = gui_vec2(64.0f, 64.0f);
    config->properties[GUI_PROPERTY_ITEM_SPACING] = gui_vec2(4.0f, 4.0f);
    config->properties[GUI_PROPERTY_ITEM_PADDING] = gui_vec2(4.0f, 4.0f);
    config->properties[GUI_PROPERTY_SCALER_SIZE] = gui_vec2(16.0f, 16.0f);
}

static void
gui_config_default_rounding(struct gui_config *config)
{
    config->rounding[GUI_ROUNDING_BUTTON] = 2.0f;
    config->rounding[GUI_ROUNDING_CHECK] = 2.0f;
    config->rounding[GUI_ROUNDING_PROGRESS] = 4.0f;
    config->rounding[GUI_ROUNDING_INPUT] = 2.0f;
    config->rounding[GUI_ROUNDING_GRAPH] = 4.0f;
    config->rounding[GUI_ROUNDING_SCROLLBAR] = 2.0f;
}

static void
gui_config_default_color(struct gui_config *config)
{
    config->colors[GUI_COLOR_TEXT] = gui_rgba(135, 135, 135, 255);
    config->colors[GUI_COLOR_PANEL] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_HEADER] = gui_rgba(40, 40, 40, 255);
    config->colors[GUI_COLOR_BORDER] = gui_rgba(65, 65, 65, 255);
    config->colors[GUI_COLOR_BUTTON] = gui_rgba(50, 50, 50, 255);
    config->colors[GUI_COLOR_BUTTON_HOVER] = gui_rgba(35, 35, 35, 255);
    config->colors[GUI_COLOR_BUTTON_TOGGLE] = gui_rgba(35, 35, 35, 255);
    config->colors[GUI_COLOR_BUTTON_HOVER_FONT] = gui_rgba(135, 135, 135, 255);
    config->colors[GUI_COLOR_CHECK] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_CHECK_BACKGROUND] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_CHECK_ACTIVE] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_OPTION] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_OPTION_BACKGROUND] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_OPTION_ACTIVE] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_SLIDER] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_SLIDER_BAR] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_SLIDER_CURSOR] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_PROGRESS] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_PROGRESS_CURSOR] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_INPUT] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_INPUT_CURSOR] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_INPUT_TEXT] = gui_rgba(135, 135, 135, 255);
    config->colors[GUI_COLOR_SELECTOR] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_SELECTOR_TRIANGLE] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_SELECTOR_TEXT] = gui_rgba(135, 135, 135, 255);
    config->colors[GUI_COLOR_SELECTOR_BUTTON] = gui_rgba(50, 50, 50, 255);
    config->colors[GUI_COLOR_HISTO] = gui_rgba(120, 120, 120, 255);
    config->colors[GUI_COLOR_HISTO_BARS] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_HISTO_NEGATIVE] = gui_rgba(255, 255, 255, 255);
    config->colors[GUI_COLOR_HISTO_HIGHLIGHT] = gui_rgba( 255, 0, 0, 255);
    config->colors[GUI_COLOR_PLOT] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_PLOT_LINES] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_PLOT_HIGHLIGHT] = gui_rgba(255, 0, 0, 255);
    config->colors[GUI_COLOR_SCROLLBAR] = gui_rgba(40, 40, 40, 255);
    config->colors[GUI_COLOR_SCROLLBAR_CURSOR] = gui_rgba(70, 70, 70, 255);
    config->colors[GUI_COLOR_TABLE_LINES] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_TAB_HEADER] = gui_rgba(40, 40, 40, 255);
    config->colors[GUI_COLOR_SHELF] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_SHELF_TEXT] = gui_rgba(150, 150, 150, 255);
    config->colors[GUI_COLOR_SHELF_ACTIVE] = gui_rgba(30, 30, 30, 255);
    config->colors[GUI_COLOR_SHELF_ACTIVE_TEXT] = gui_rgba(150, 150, 150, 255);
    config->colors[GUI_COLOR_SCALER] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_LAYOUT_SCALER] = gui_rgba(25, 25, 25, 255);
}

void
gui_config_default(struct gui_config *config, gui_flags flags,
    const struct gui_font *font)
{
    GUI_ASSERT(config);
    if (!config) return;
    gui_zero(config, sizeof(*config));
    config->font = *font;

    config->scaler_width = 4.0f;
    if (flags & GUI_DEFAULT_COLOR)
        gui_config_default_color(config);
    if (flags & GUI_DEFAULT_PROPERTIES)
        gui_config_default_properties(config);
    if (flags & GUI_DEFAULT_ROUNDING)
        gui_config_default_rounding(config);
}

void
gui_config_set_font(struct gui_config *config, const struct gui_font *font)
{
    GUI_ASSERT(config);
    if (!config) return;
    config->font = *font;
}

struct gui_vec2
gui_config_property(const struct gui_config *config, enum gui_config_properties index)
{
    static struct gui_vec2 zero;
    GUI_ASSERT(config);
    if (!config) return zero;
    return config->properties[index];
}

struct gui_color
gui_config_color(const struct gui_config *config, enum gui_config_colors index)
{
    static struct gui_color zero;
    GUI_ASSERT(config);
    if (!config) return zero;
    return config->colors[index];
}

void
gui_config_push_color(struct gui_config *config, enum gui_config_colors index,
    struct gui_color col)
{
    struct gui_saved_color *c;
    GUI_ASSERT(config);
    if (!config) return;
    if (config->stack.color >= GUI_MAX_COLOR_STACK) return;

    c = &config->stack.colors[config->stack.color++];
    c->value = config->colors[index];
    c->type = index;
    config->colors[index] = col;
}

void
gui_config_push_property(struct gui_config *config, enum gui_config_properties index,
    struct gui_vec2 v)
{
    struct gui_saved_property *p;
    GUI_ASSERT(config);
    if (!config) return;
    if (config->stack.property >= GUI_MAX_ATTRIB_STACK) return;

    p = &config->stack.properties[config->stack.property++];
    p->value = config->properties[index];
    p->type = index;
    config->properties[index] = v;
}

void
gui_config_pop_color(struct gui_config *config)
{
    struct gui_saved_color *c;
    GUI_ASSERT(config);
    if (!config) return;
    if (!config->stack.color) return;
    c = &config->stack.colors[--config->stack.color];
    config->colors[c->type] = c->value;
}

void
gui_config_pop_property(struct gui_config *config)
{
    struct gui_saved_property *p;
    GUI_ASSERT(config);
    if (!config) return;
    if (!config->stack.property) return;
    p = &config->stack.properties[--config->stack.property];
    config->properties[p->type] = p->value;
}

void
gui_config_reset_colors(struct gui_config *config)
{
    GUI_ASSERT(config);
    if (!config) return;
    while (config->stack.color)
        gui_config_pop_color(config);
}

void
gui_config_reset_properties(struct gui_config *config)
{
    GUI_ASSERT(config);
    if (!config) return;
    while (config->stack.property)
        gui_config_pop_property(config);
}

void
gui_config_reset(struct gui_config *config)
{
    GUI_ASSERT(config);
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
    gui_float h, gui_flags flags, struct gui_command_queue *queue,
    const struct gui_config *config, const struct gui_input *input)
{
    GUI_ASSERT(panel);
    GUI_ASSERT(config);
    GUI_ASSERT(input);
    if (!panel || !config || !input)
        return;

    panel->x = x;
    panel->y = y;
    panel->w = w;
    panel->h = h;
    panel->flags = flags;
    panel->config = config;
    panel->offset.x = 0;
    panel->offset.y = 0;
    panel->queue = queue;
    panel->input = input;
    if (queue) {
        gui_command_buffer_init(&panel->buffer, &queue->buffer, GUI_CLIP);
        gui_command_queue_insert_back(queue, &panel->buffer);
    }
}

void
gui_panel_set_config(struct gui_panel *panel, const struct gui_config *config)
{
    GUI_ASSERT(panel);
    GUI_ASSERT(config);
    if (!panel || !config) return;
    panel->config = config;
}

void
gui_panel_add_flag(struct gui_panel *panel, gui_flags f)
{panel->flags |= f;}

void
gui_panel_remove_flag(struct gui_panel *panel, gui_flags f)
{panel->flags &= (gui_flags)~f;}

gui_bool
gui_panel_has_flag(struct gui_panel *panel, gui_flags f)
{return (panel->flags & f) ? gui_true: gui_false;}

gui_bool
gui_panel_is_minimized(struct gui_panel *panel)
{return panel->flags & GUI_PANEL_MINIMIZED;}

void
gui_panel_begin(struct gui_panel_layout *layout, struct gui_panel *panel)
{
    const struct gui_config *c;
    gui_float scrollbar_size;
    struct gui_vec2 item_padding;
    struct gui_vec2 item_spacing;
    struct gui_vec2 panel_padding;
    struct gui_vec2 scaler_size;
    struct gui_command_buffer *out;
    const struct gui_input *in;

    GUI_ASSERT(layout);
    GUI_ASSERT(panel);
    GUI_ASSERT(panel->config);

    /* cache configuration data */
    c = panel->config;
    in = panel->input;
    scrollbar_size = gui_config_property(c, GUI_PROPERTY_SCROLLBAR_SIZE).x;
    panel_padding = gui_config_property(c, GUI_PROPERTY_PADDING);
    item_padding = gui_config_property(c, GUI_PROPERTY_ITEM_PADDING);
    item_spacing = gui_config_property(c, GUI_PROPERTY_ITEM_SPACING);
    scaler_size = gui_config_property(c, GUI_PROPERTY_SCALER_SIZE);

    /* check arguments */
    if (!panel || !layout) return;
    gui_zero(layout, sizeof(*layout));
    if (panel->flags & GUI_PANEL_HIDDEN) {
        layout->flags = panel->flags;
        layout->valid = gui_false;
        layout->config = panel->config;
        layout->buffer = &panel->buffer;
        return;
    }

    /* overlapping panels */
    if (panel->queue && !(panel->flags & GUI_PANEL_TAB))
    {
        layout->queue = panel->queue;
        gui_command_queue_start(panel->queue, &panel->buffer);
        {
            gui_bool inpanel;
            gui_float x, y, w, h;
            struct gui_command_buffer_list *s = &panel->queue->list;
            x = panel->x; y = panel->y; w = panel->w; h = panel->h;
            inpanel = GUI_INBOX(in->mouse_prev.x, in->mouse_prev.y, x, y, w, h);
            if (in->mouse_down&&in->mouse_clicked && inpanel && &panel->buffer != s->end) {
                const struct gui_command_buffer *iter = panel->buffer.next;
                while (iter) {
                    /* try to find a panel with higher priorty in the same position */
                    const struct gui_panel *cur;
                    cur = GUI_CONTAINER_OF_CONST(iter, struct gui_panel, buffer);
                    if (GUI_INBOX(in->mouse_prev.x,in->mouse_prev.y,cur->x,cur->y,cur->w,cur->h) &&
                      !(cur->flags & GUI_PANEL_MINIMIZED) && !(cur->flags & GUI_PANEL_HIDDEN)) break;
                    iter = iter->next;
                }
                /* current panel is active panel in that position so transfer to top
                 * at the highest priority in stack */
                if (!iter) {
                    gui_command_queue_remove(panel->queue, &panel->buffer);
                    gui_command_queue_insert_back(panel->queue, &panel->buffer);
                    panel->flags &= ~(gui_flags)GUI_PANEL_ROM;
                }
            }
            if (s->end != &panel->buffer)
                panel->flags |= GUI_PANEL_ROM;
        }
    }

    /* move panel position if requested */
    layout->header.h = c->font.height + 4 * item_padding.y;
    layout->header.h += panel_padding.y;
    if (panel->flags & GUI_PANEL_MOVEABLE && !(panel->flags & GUI_PANEL_ROM)) {
        gui_bool incursor;
        const gui_float move_x = panel->x;
        const gui_float move_y = panel->y;
        const gui_float move_w = panel->w;
        const gui_float move_h = layout->header.h;
        incursor = GUI_INBOX(in->mouse_prev.x, in->mouse_prev.y,
                    move_x, move_y, move_w, move_h);
        if (in->mouse_down && incursor) {
            panel->x = MAX(0, panel->x + in->mouse_delta.x);
            panel->y = MAX(0, panel->y + in->mouse_delta.y);
        }
    }

    /* setup panel layout */
    layout->input = in;
    layout->x = panel->x;
    layout->y = panel->y;
    layout->w = panel->w;
    layout->h = panel->h;
    layout->at_x = panel->x;
    layout->at_y = panel->y;
    layout->width = panel->w;
    layout->height = panel->h;
    layout->config = panel->config;
    layout->buffer = &panel->buffer;
    layout->row.index = 0;
    layout->row.columns = 0;
    layout->row.height = 0;
    layout->row.ratio = 0;
    layout->row.item_width = 0;
    layout->offset = panel->offset;
    layout->header.h = 1;
    layout->row.height = layout->header.h + 2 * item_spacing.y;
    out = &panel->buffer;

    /* panel activation by clicks inside of the panel */
    if (!(panel->flags & GUI_PANEL_TAB) && !(panel->flags & GUI_PANEL_ROM)) {
        gui_float clicked_x = in->mouse_clicked_pos.x;
        gui_float clicked_y = in->mouse_clicked_pos.y;
        if (in->mouse_down) {
            if (GUI_INBOX(clicked_x, clicked_y, panel->x, panel->y, panel->w, panel->h))
                panel->flags |= GUI_PANEL_ACTIVE;
            else panel->flags &= (gui_flag)~GUI_PANEL_ACTIVE;
        }
    }

    layout->flags = panel->flags;
    layout->valid = !(panel->flags & GUI_PANEL_HIDDEN) && !(panel->flags & GUI_PANEL_MINIMIZED);
    layout->footer_h = scaler_size.y + item_padding.y;

    /* calculate the panel size and panel footer height */
    layout->width = panel->w - scrollbar_size;
    layout->height = panel->h - (layout->header.h + 2 * item_spacing.y);
    if (layout->flags & GUI_PANEL_SCALEABLE)
        layout->height -= layout->footer_h;

    /* draw panel background if not a dynamic panel */
    if (!(layout->flags & GUI_PANEL_DYNAMIC) && layout->valid) {
        gui_command_buffer_push_rect(out, gui_rect(layout->x, layout->y,
            layout->w, layout->h), 0, c->colors[GUI_COLOR_PANEL]);
    } else{
        layout->footer_h = scaler_size.y + item_padding.y;
        gui_command_buffer_push_rect(out, gui_rect(layout->x, layout->y, layout->w,
            layout->row.height), 0, c->colors[GUI_COLOR_PANEL]);
    }

    /* draw top border line */
    if (layout->flags & GUI_PANEL_BORDER) {
        gui_command_buffer_push_line(out, layout->x, layout->y,
            layout->x + layout->w, layout->y, c->colors[GUI_COLOR_BORDER]);
    }

    /* calculate and set the panel clipping rectangle*/
    layout->clip.x = panel->x;
    layout->clip.y = panel->y;
    layout->clip.w = panel->w;
    layout->clip.h = panel->h - (layout->footer_h + layout->header.h);
    layout->clip.h -= (panel_padding.y + item_padding.y);
    gui_command_buffer_push_scissor(out, layout->clip);
}

void
gui_panel_begin_tiled(struct gui_panel_layout *tile, struct gui_panel *panel,
    struct gui_layout *layout, enum gui_layout_slot_index slot, gui_size index)
{
    struct gui_rect bounds;
    struct gui_layout_slot *s;
    const struct gui_config *config;
    struct gui_vec2 mpos;
    struct gui_rect scaler = {0,0,0,0};
    const struct gui_input *in;

    /* debug build argument check */
    GUI_ASSERT(panel);
    GUI_ASSERT(tile);
    GUI_ASSERT(layout);

    /* release build argument check */
    if (!layout || !panel || !tile) return;
    if (slot >= GUI_SLOT_MAX) return;
    if (index >= layout->slots[slot].capacity) return;

    /* make sure the correct flags are set */
    in = panel->input;
    mpos = in->mouse_prev;
    panel->flags &= (gui_flags)~GUI_PANEL_MOVEABLE;
    panel->flags &= (gui_flags)~GUI_PANEL_SCALEABLE;

    /* calculate the bounds of the slot */
    s = &layout->slots[slot];
    bounds.x = layout->bounds.x + s->offset.x * (gui_float)layout->bounds.w;
    bounds.y = layout->bounds.y + s->offset.y * (gui_float)layout->bounds.h;
    bounds.w = s->ratio.x * (gui_float)layout->bounds.w;
    bounds.h = s->ratio.y * (gui_float)layout->bounds.h;

    config = panel->config;
    {
        /* user slot scaling */
        switch (slot) {
        case GUI_SLOT_TOP:
            /* calculate scaler bounds */
            scaler.x = bounds.x;
            scaler.y = (bounds.y + bounds.h) - config->scaler_width;
            scaler.w = bounds.w;
            scaler.h = config->scaler_width;

            /* update top slot bounds by user input */
            if (!(panel->flags & GUI_PANEL_ROM) &&
                s->state!=GUI_LOCKED && !layout->active &&
                GUI_INBOX(mpos.x, mpos.y, scaler.x, scaler.y, scaler.w, scaler.h) &&
                in->mouse_down)
            {
                /* update bounds to the update size */
                gui_float py;
                bounds.h += in->mouse_delta.y;
                bounds.h = MAX(config->properties[GUI_PROPERTY_SIZE].y, bounds.h);
                scaler.y = (bounds.y + bounds.h) - config->scaler_width;

                /* calculate the updated ratios for every influenced neighbour slot */
                py = 1.0f- ((bounds.h / (gui_float)layout->bounds.h) +
                    layout->slots[GUI_SLOT_BOTTOM].ratio.y);
                layout->slots[GUI_SLOT_TOP].ratio.y = bounds.h / (gui_float)layout->bounds.h;
                layout->slots[GUI_SLOT_LEFT].ratio.y = py;
                layout->slots[GUI_SLOT_CENTER].ratio.y = py;
                layout->slots[GUI_SLOT_RIGHT].ratio.y = py;

                /* calculate the updated offset for every influenced neighbour slot */
                layout->slots[GUI_SLOT_LEFT].offset.y = layout->slots[GUI_SLOT_TOP].ratio.y;
                layout->slots[GUI_SLOT_CENTER].offset.y = layout->slots[GUI_SLOT_TOP].ratio.y;
                layout->slots[GUI_SLOT_RIGHT].offset.y = layout->slots[GUI_SLOT_TOP].ratio.y;
            }
            bounds.h -= config->scaler_width;
            break;
        case GUI_SLOT_BOTTOM:
            /* calculate scaler bounds */
            scaler.x = bounds.x;
            scaler.y = bounds.y;
            scaler.w = bounds.w;
            scaler.h = config->scaler_width;

            /* update bottom slot bounds by user input */
            if (!(panel->flags & GUI_PANEL_ROM) &&
                s->state != GUI_LOCKED && layout->active &&
                GUI_INBOX(mpos.x, mpos.y, scaler.x, scaler.y, scaler.w, scaler.h) &&
                in->mouse_down)
            {
                /* update bounds to the update size */
                gui_float py;
                bounds.y += in->mouse_delta.y;
                bounds.h += -in->mouse_delta.y;
                scaler.y = bounds.y;

                /* calculate the updated ratios/offset for every influenced neighbour slot */
                py = 1.0f - ((bounds.h / (gui_float)layout->bounds.h) +
                    layout->slots[GUI_SLOT_TOP].ratio.y);
                layout->slots[GUI_SLOT_BOTTOM].ratio.y = bounds.h / (gui_float)layout->bounds.h;
                layout->slots[GUI_SLOT_LEFT].ratio.y = py;
                layout->slots[GUI_SLOT_CENTER].ratio.y = py;
                layout->slots[GUI_SLOT_RIGHT].ratio.y = py;
                layout->slots[GUI_SLOT_BOTTOM].offset.y = bounds.y/(gui_float)layout->bounds.h;
            }

            bounds.y += config->scaler_width;
            bounds.h -= config->scaler_width;
            break;
        case GUI_SLOT_LEFT:
            /* calculate scaler bounds */
            scaler.x = bounds.x + bounds.w - config->scaler_width;
            scaler.y = bounds.y;
            scaler.w = config->scaler_width;
            scaler.h = bounds.h;

            /* update left slot bounds by user input */
            if (!(panel->flags & GUI_PANEL_ROM) &&
                s->state != GUI_LOCKED && layout->active &&
                GUI_INBOX(mpos.x, mpos.y, scaler.x, scaler.y, scaler.w, scaler.h) &&
                in->mouse_down)
            {
                /* update left slot bounds by user input */
                gui_float cx, rx;
                bounds.w += in->mouse_delta.x;
                bounds.w = MAX(config->properties[GUI_PROPERTY_SIZE].x, bounds.w);
                scaler.x = bounds.x + bounds.w - config->scaler_width;

                /* calculate the updated center ratio and offset */
                cx = 1.0f - ((bounds.w / (gui_float)layout->bounds.w) +
                    layout->slots[GUI_SLOT_RIGHT].ratio.x);
                layout->slots[GUI_SLOT_LEFT].ratio.x = bounds.w / (gui_float)layout->bounds.w;
                layout->slots[GUI_SLOT_CENTER].offset.x = layout->slots[GUI_SLOT_LEFT].ratio.x;
                layout->slots[GUI_SLOT_CENTER].ratio.x = cx;

                /* calculate the updated right ratio and offset */
                rx = 1.0f - ((bounds.w / (gui_float)layout->bounds.w) +
                    layout->slots[GUI_SLOT_CENTER].ratio.x);
                layout->slots[GUI_SLOT_RIGHT].ratio.x = rx;
                layout->slots[GUI_SLOT_RIGHT].offset.x =
                    layout->slots[GUI_SLOT_CENTER].offset.x +
                    layout->slots[GUI_SLOT_CENTER].ratio.x;
            }
            bounds.w -= config->scaler_width;
            break;
        case GUI_SLOT_RIGHT:
            /* calculate scaler bounds */
            scaler.x = bounds.x;
            scaler.y = bounds.y;
            scaler.w = config->scaler_width;
            scaler.h = bounds.h;

            /* update right slot bounds by user input */
            if (!(panel->flags & GUI_PANEL_ROM) &&
                s->state != GUI_LOCKED && layout->active &&
                GUI_INBOX(mpos.x, mpos.y, scaler.x, scaler.y, scaler.w, scaler.h) &&
                in->mouse_down)
            {
                /* update left slot bounds by user input */
                gui_float cx, lx;
                bounds.w -= in->mouse_delta.x;
                bounds.x += in->mouse_delta.x;
                bounds.w = MAX(config->properties[GUI_PROPERTY_SIZE].x, bounds.w);
                scaler.x = bounds.x;

                /* calculate the updated center ratio and offset */
                cx = 1.0f - ((bounds.w / (gui_float)layout->bounds.w) +
                    layout->slots[GUI_SLOT_LEFT].ratio.x);
                layout->slots[GUI_SLOT_RIGHT].ratio.x = bounds.w / (gui_float)layout->bounds.w;
                layout->slots[GUI_SLOT_CENTER].ratio.x = cx;

                /* calculate the updated left ratio and offset */
                lx = 1.0f - ((bounds.w / (gui_float)layout->bounds.w) +
                    layout->slots[GUI_SLOT_CENTER].ratio.x);
                layout->slots[GUI_SLOT_LEFT].ratio.x = lx;
                layout->slots[GUI_SLOT_RIGHT].offset.x =
                    layout->slots[GUI_SLOT_CENTER].offset.x +
                    layout->slots[GUI_SLOT_CENTER].ratio.x;
            }

            bounds.x += config->scaler_width;
            bounds.w -= config->scaler_width;
            break;
        case GUI_SLOT_CENTER:
        case GUI_SLOT_MAX:
        default: break;
        }
    }

    /* calculate the bounds of the panel */
    if (s->format == GUI_LAYOUT_HORIZONTAL) {
        panel->h = bounds.h;
        panel->y = bounds.y;
        panel->w = bounds.w / (gui_float)s->capacity;
        panel->x = bounds.x + (gui_float)index * panel->w;
    } else {
        panel->x = bounds.x;
        panel->w = bounds.w;
        panel->h = bounds.h / (gui_float)s->capacity;
        panel->y = bounds.y + (gui_float)index * panel->h;
    }

    {
        /* setup panel and make sure tiled panels are always in the background */
        struct gui_command_queue *queue = panel->queue;
        struct gui_command_buffer *out = &panel->buffer;
        gui_command_queue_start(panel->queue, &panel->buffer);
        if (slot != GUI_SLOT_CENTER && !index) {
            /* draw the scaler of the slot */
            gui_command_buffer_push_rect(out, scaler, 0,
                    config->colors[GUI_COLOR_LAYOUT_SCALER]);
        }
        panel->queue = 0;
        if (!layout->active) panel->flags |= GUI_PANEL_ROM;
        else panel->flags &= ~(gui_flags)GUI_PANEL_ROM;
        gui_panel_begin(tile, panel);
        panel->queue = queue;
        gui_command_queue_remove(panel->queue, &panel->buffer);
        gui_command_queue_insert_front(panel->queue, &panel->buffer);
        tile->queue = panel->queue;
    }
}

void
gui_panel_end(struct gui_panel_layout *layout, struct gui_panel *panel)
{
    const struct gui_input *in;
    const struct gui_config *config;
    struct gui_command_buffer *out;
    gui_float scrollbar_size;
    struct gui_vec2 item_padding;
    struct gui_vec2 item_spacing;
    struct gui_vec2 panel_padding;
    struct gui_vec2 scaler_size;
    struct gui_rect footer = {0,0,0,0};

    GUI_ASSERT(layout);
    GUI_ASSERT(panel);
    if (!panel || !layout) return;

    config = layout->config;
    out = layout->buffer;
    in = (layout->flags & GUI_PANEL_ROM) ? 0 :layout->input;
    if (!(layout->flags & GUI_PANEL_TAB)) {
        struct gui_rect clip;
        clip.x = MAX(0, (layout->x - 1));
        clip.y = MAX(0, (layout->y - 1));
        clip.w = layout->w + 1;
        clip.h = layout->h + 1;
        gui_command_buffer_push_scissor(out, clip);
    }

    /* cache configuration data */
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    item_spacing = gui_config_property(config, GUI_PROPERTY_ITEM_SPACING);
    panel_padding = gui_config_property(config, GUI_PROPERTY_PADDING);
    scrollbar_size = gui_config_property(config, GUI_PROPERTY_SCROLLBAR_SIZE).x;
    scaler_size = gui_config_property(config, GUI_PROPERTY_SCALER_SIZE);

    /* update the current Y-position to point over the last added widget */
    layout->at_y += layout->row.height;
    if (layout->valid && layout->flags & GUI_PANEL_DYNAMIC) {
        /* calculate the dynamic panel footer bounds */
        layout->height = MIN(layout->at_y - layout->y, layout->h);

        /* draw the correct footer */
        footer.x = panel->x;
        footer.w = panel->w + scrollbar_size;
        footer.h = layout->footer_h;
        if (layout->flags & GUI_PANEL_COMBO_MENU)
            footer.y = panel->y + layout->height;
        else footer.y = panel->y + layout->height + layout->footer_h;
        gui_command_buffer_push_rect(out, footer, 0, config->colors[GUI_COLOR_PANEL]);

        if (!(layout->flags & GUI_PANEL_COMBO_MENU)) {
            struct gui_rect bounds;
            bounds.x = layout->x;
            bounds.y = panel->y + layout->height;
            bounds.w = layout->width;
            bounds.h = layout->row.height;
            gui_command_buffer_push_rect(out, bounds, 0, config->colors[GUI_COLOR_PANEL]);
        }
    }

    /* scrollbars */
    if (layout->valid) {
        struct gui_rect bounds;
        gui_float scroll_target, scroll_offset, scroll_step;

        struct gui_scrollbar scroll;
        scroll.rounding = config->rounding[GUI_ROUNDING_SCROLLBAR];
        scroll.background = config->colors[GUI_COLOR_SCROLLBAR];
        scroll.foreground = config->colors[GUI_COLOR_SCROLLBAR_CURSOR];
        scroll.border = config->colors[GUI_COLOR_BORDER];
        {
            /* vertical scollbar */
            bounds.x = layout->x + layout->width;
            bounds.y = (layout->flags & GUI_PANEL_BORDER) ? layout->y + 1 : layout->y;
            bounds.y += layout->header.h + layout->menu.h;
            bounds.w = scrollbar_size;
            bounds.h = layout->height;
            if (layout->flags & GUI_PANEL_BORDER) bounds.h -= 1;

            scroll_offset = layout->offset.y;
            scroll_step = layout->height * 0.10f;
            scroll_target = (layout->at_y-layout->y)-(layout->header.h+2*item_spacing.y);
            scroll.has_scrolling = (layout->flags & GUI_PANEL_ACTIVE);
            panel->offset.y = gui_scrollbar_vertical(out, bounds, scroll_offset,
                                scroll_target, scroll_step, &scroll, in);
        }
        {
            /* horizontal scrollbar */
            bounds.x = layout->x + panel_padding.x;
            if (layout->flags & GUI_PANEL_TAB) {
                bounds.h = scrollbar_size;
                bounds.y = (layout->flags & GUI_PANEL_BORDER) ? layout->y + 1 : layout->y;
                bounds.y += layout->header.h + layout->menu.h + layout->height;
                bounds.w = layout->width - scrollbar_size;
            } else if (layout->flags & GUI_PANEL_DYNAMIC) {
                bounds.h = MIN(scrollbar_size, layout->footer_h);
                bounds.w = layout->width - 2 * panel_padding.x;
                bounds.y = footer.y;
            } else {
                bounds.h = MIN(scrollbar_size, layout->footer_h);
                bounds.y = layout->y + panel->h - MAX(layout->footer_h, scrollbar_size);
                bounds.w = layout->width - 2 * panel_padding.x;
            }

            scroll_offset = layout->offset.x;
            scroll_step = layout->max_x * 0.05f;
            scroll_target = (layout->max_x - layout->at_x) - 2 * panel_padding.x;
            scroll.has_scrolling = gui_false;
            panel->offset.x = gui_scrollbar_horizontal(out, bounds, scroll_offset,
                                    scroll_target, scroll_step, &scroll, in);
        }
    };

    /* draw the panel scaler into the right corner of the panel footer and
     * update panel size if user drags the scaler */
    if ((layout->flags & GUI_PANEL_SCALEABLE) && layout->valid && in) {
        gui_float scaler_y;
        struct gui_color col = config->colors[GUI_COLOR_SCALER];
        gui_float scaler_w = MAX(0, scaler_size.x - item_padding.x);
        gui_float scaler_h = MAX(0, scaler_size.y - item_padding.y);
        gui_float scaler_x = (layout->x + layout->w) - (item_padding.x + scaler_w);

        if (layout->flags & GUI_PANEL_DYNAMIC)
            scaler_y = footer.y + layout->footer_h - scaler_size.y;
        else scaler_y = layout->y + layout->h - scaler_size.y;
        gui_command_buffer_push_triangle(out, scaler_x + scaler_w, scaler_y,
            scaler_x + scaler_w, scaler_y + scaler_h, scaler_x, scaler_y + scaler_h, col);

        if (!(panel->flags & GUI_PANEL_ROM)) {
            gui_bool incursor;
            gui_float prev_x = in->mouse_prev.x;
            gui_float prev_y = in->mouse_prev.y;
            struct gui_vec2 panel_size = gui_config_property(config, GUI_PROPERTY_SIZE);
            incursor = GUI_INBOX(prev_x,prev_y,scaler_x,scaler_y,scaler_w,scaler_h);
            if (in->mouse_down && incursor) {
                panel->w = MAX(panel_size.x, panel->w + in->mouse_delta.x);
                /* draging in y-direction is only possible if static panel */
                if (!(layout->flags & GUI_PANEL_DYNAMIC))
                    panel->h = MAX(panel_size.y, panel->h + in->mouse_delta.y);
            }
        }
    }

    if (layout->flags & GUI_PANEL_BORDER) {
        /* draw the border around the complete panel */
        const gui_float width = layout->width + scrollbar_size;
        const gui_float padding_y = (!layout->valid) ?
                panel->y + layout->header.h:
                (layout->flags & GUI_PANEL_DYNAMIC) ?
                layout->footer_h + footer.y:
                layout->y + layout->h;

        if (panel->flags & GUI_PANEL_BORDER_HEADER)
            gui_command_buffer_push_line(out, panel->x, panel->y + layout->header.h,
                panel->x + panel->w, panel->y + layout->header.h,
                config->colors[GUI_COLOR_BORDER]);
        gui_command_buffer_push_line(out, panel->x, padding_y, panel->x + width,
                padding_y, config->colors[GUI_COLOR_BORDER]);
        gui_command_buffer_push_line(out, panel->x, panel->y, panel->x,
                padding_y, config->colors[GUI_COLOR_BORDER]);
        gui_command_buffer_push_line(out, panel->x + width, panel->y, panel->x + width,
                padding_y, config->colors[GUI_COLOR_BORDER]);
    }

    gui_command_buffer_push_scissor(out, gui_rect(0, 0, gui_null_rect.w, gui_null_rect.h));
    if (!(panel->flags & GUI_PANEL_TAB)) {
        /* panel is hidden so clear command buffer  */
        if (layout->flags & GUI_PANEL_HIDDEN)
            gui_command_buffer_reset(&panel->buffer);
        /* panel is visible and not tab */
        else gui_command_queue_finish(panel->queue, &panel->buffer);
    }
    if (layout->flags & GUI_PANEL_REMOVE_ROM) {
        layout->flags &= ~(gui_flags)GUI_PANEL_ROM;
        layout->flags &= ~(gui_flags)GUI_PANEL_REMOVE_ROM;
    }
    panel->flags = layout->flags;
}

/*
 * -------------------------------------------------------------
 *
 *                          Header
 *
 * --------------------------------------------------------------
 */
void
gui_panel_header_begin(struct gui_panel_layout *layout)
{
    const struct gui_config *c;
    struct gui_vec2 item_padding;
    struct gui_vec2 item_spacing;
    struct gui_vec2 panel_padding;
    struct gui_command_buffer *out;

    GUI_ASSERT(layout);
    if (!layout) return;
    if (layout->flags & GUI_PANEL_HIDDEN)
        return;

    c = layout->config;
    out = layout->buffer;

    /* cache some configuration data */
    panel_padding = gui_config_property(c, GUI_PROPERTY_PADDING);
    item_padding = gui_config_property(c, GUI_PROPERTY_ITEM_PADDING);
    item_spacing = gui_config_property(c, GUI_PROPERTY_ITEM_SPACING);

    /* update the header height and first row height */
    layout->header.h = c->font.height + 4 * item_padding.y;
    layout->header.h += panel_padding.y;
    layout->row.height += layout->header.h;
    if (layout->valid)
        gui_command_buffer_push_rect(out,
            gui_rect(layout->x, layout->y + layout->header.h,
            layout->w, layout->row.height), 0, c->colors[GUI_COLOR_PANEL]);

    /* setup header bounds and growable icon space */
    layout->header.x = layout->x + panel_padding.x;
    layout->header.y = layout->y + panel_padding.y;
    layout->header.w = MAX(layout->w, 2 * panel_padding.x);
    layout->header.w -= 2 * panel_padding.x;
    layout->header.front = layout->header.x;
    layout->header.back = layout->header.x + layout->header.w;
    layout->height = layout->h - (layout->header.h + 2 * item_spacing.y);
    layout->height -= layout->footer_h;
    gui_command_buffer_push_rect(out, gui_rect(layout->x, layout->y, layout->w,
        layout->header.h), 0, c->colors[GUI_COLOR_HEADER]);
}

static gui_bool
gui_panel_header_icon(struct gui_panel_layout *layout,
    enum gui_panel_header_symbol symbol, struct gui_image *img,
    enum gui_panel_header_align align)
{
    /* calculate the position of the close icon position and draw it */
    struct gui_rect sym = {0,0,0,0};
    gui_float sym_bw = 0;
    gui_bool ret = gui_false;

    const struct gui_config *c;
    struct gui_command_buffer *out;
    struct gui_vec2 item_padding;

    GUI_ASSERT(layout);
    GUI_ASSERT(layout->buffer);
    GUI_ASSERT(layout->config);
    if (!layout || layout->flags & GUI_PANEL_HIDDEN)
        return gui_false;

    /* cache configuration data */
    c = layout->config;
    out = layout->buffer;
    item_padding = gui_config_property(c, GUI_PROPERTY_ITEM_PADDING);

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

        /* draw icon */
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
    case GUI_SYMBOL_IMAGE: {
        /* image icon */
        sym_bw = sym.w = c->font.height;
        sym.h = c->font.height;
        sym.y = sym.y + c->font.height/2;
        if (align == GUI_HEADER_RIGHT)
            sym.x = layout->header.back - (c->font.height + 2 * item_padding.x);

        /* draw image and calculate the occuppied icon space */
        gui_command_buffer_push_image(out, sym, img);
        sym.w = c->font.height + 2 * item_padding.x;
        } break;
    case GUI_SYMBOL_MAX:
    default: return ret;
    }

    /* check if the icon has been pressed */
    if (!(layout->flags & GUI_PANEL_ROM)) {
        gui_float clicked_x = layout->input->mouse_clicked_pos.x;
        gui_float clicked_y = layout->input->mouse_clicked_pos.y;
        gui_float mouse_x = layout->input->mouse_pos.x;
        gui_float mouse_y = layout->input->mouse_pos.y;
        if (GUI_INBOX(mouse_x, mouse_y, sym.x, sym.y, sym_bw, sym.h)) {
            if (GUI_INBOX(clicked_x, clicked_y, sym.x, sym.y, sym_bw, sym.h))
                ret = (layout->input->mouse_down && layout->input->mouse_clicked);
        }
    }

    /* update the header space */
    if (align == GUI_HEADER_RIGHT)
        layout->header.back -= (sym.w + item_padding.x);
    else layout->header.front += sym.w + item_padding.x;
    return ret;
}

gui_bool
gui_panel_header_button(struct gui_panel_layout *layout,
    enum gui_panel_header_symbol symbol, enum gui_panel_header_align align)
{return gui_panel_header_icon(layout, symbol, 0, align);}

gui_bool
gui_panel_header_button_icon(struct gui_panel_layout *layout, struct gui_image img,
    enum gui_panel_header_align align)
{return gui_panel_header_icon(layout, GUI_SYMBOL_IMAGE, &img, align);}

gui_bool
gui_panel_header_toggle(struct gui_panel_layout *layout,
    enum gui_panel_header_symbol inactive, enum gui_panel_header_symbol active,
    enum gui_panel_header_align align, gui_bool state)
{
    gui_bool ret = gui_panel_header_button(layout, (state) ? active : inactive, align);
    if (ret) return !state;
    return state;
}

gui_bool
gui_panel_header_flag(struct gui_panel_layout *layout, enum gui_panel_header_symbol inactive,
    enum gui_panel_header_symbol active, enum gui_panel_header_align align,
    enum gui_panel_flags flag)
{
    gui_flags flags = layout->flags;
    gui_bool state = (flags & flag) ? gui_true : gui_false;
    gui_bool ret = gui_panel_header_toggle(layout, inactive, active, align, state);
    if (ret != ((flags & flag) ? gui_true : gui_false)) {
        /* the state of the toggle icon has been changed  */
        if (!ret) layout->flags &= ~flag;
        else layout->flags |= flag;
        /* update the state of the panel since the flag have changed */
        layout->valid = !(layout->flags & GUI_PANEL_HIDDEN) &&
                        !(layout->flags & GUI_PANEL_MINIMIZED);
        return gui_true;
    }
    return gui_false;
}

void
gui_panel_header_title(struct gui_panel_layout *layout, const char *title,
    enum gui_panel_header_align align)
{
    struct gui_rect label = {0,0,0,0};
    const struct gui_config *c;
    struct gui_command_buffer *out;
    struct gui_vec2 item_padding;
    gui_size text_len;
    gui_size t;

    /* make sure correct values and layout state */
    GUI_ASSERT(layout);
    if (!layout || !title) return;
    if (layout->flags & GUI_PANEL_HIDDEN)
        return;

    /* cache configuration and title length */
    c = layout->config;
    out = layout->buffer;
    item_padding = gui_config_property(c, GUI_PROPERTY_ITEM_PADDING);
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
gui_panel_header_end(struct gui_panel_layout *layout)
{
    const struct gui_config *c;
    struct gui_command_buffer *out;
    struct gui_vec2 item_padding;
    struct gui_vec2 panel_padding;

    GUI_ASSERT(layout);
    if (!layout) return;
    if (layout->flags & GUI_PANEL_HIDDEN)
        return;

    /* cache configuration data */
    c = layout->config;
    out = layout->buffer;
    panel_padding = gui_config_property(c, GUI_PROPERTY_PADDING);
    item_padding = gui_config_property(c, GUI_PROPERTY_ITEM_PADDING);

    /* draw panel header border */
    if (layout->flags & GUI_PANEL_BORDER) {
        gui_float scrollbar_width = gui_config_property(c, GUI_PROPERTY_SCROLLBAR_SIZE).x;
        const gui_float width = layout->width + scrollbar_width;

        /* draw the header border lines */
        gui_command_buffer_push_line(out, layout->x, layout->y, layout->x,
                layout->y + layout->header.h, c->colors[GUI_COLOR_BORDER]);
        gui_command_buffer_push_line(out, layout->x + width, layout->y, layout->x + width,
                layout->y + layout->header.h, c->colors[GUI_COLOR_BORDER]);
        if (layout->flags & GUI_PANEL_BORDER_HEADER)
            gui_command_buffer_push_line(out, layout->x, layout->y + layout->header.h,
                layout->x + layout->w, layout->y + layout->header.h,
                c->colors[GUI_COLOR_BORDER]);
    }

    /* update the panel clipping rect to include the header */
    layout->clip.x = layout->x;
    layout->clip.w = layout->w;
    layout->clip.y = layout->y + layout->header.h;
    layout->clip.h = layout->h - (layout->footer_h + layout->header.h);
    layout->clip.h -= (panel_padding.y + item_padding.y);
    gui_command_buffer_push_scissor(out, layout->clip);
}

gui_flags
gui_panel_header(struct gui_panel_layout *layout, const char *title,
    gui_flags flags, gui_flags notify, enum gui_panel_header_align align)
{
    gui_flags ret = 0;
    gui_flags old = layout->flags;

    GUI_ASSERT(layout);
    if (!layout || layout->flags & GUI_PANEL_HIDDEN)
        return gui_false;

    /* basic standart header with fixed icon/title sequence */
    gui_panel_header_begin(layout);
    {
        if (flags & GUI_CLOSEABLE)
            gui_panel_header_flag(layout, GUI_SYMBOL_X, GUI_SYMBOL_X,
                align, GUI_PANEL_HIDDEN);
        if (flags & GUI_MINIMIZABLE)
            gui_panel_header_flag(layout, GUI_SYMBOL_MINUS, GUI_SYMBOL_PLUS,
                align, GUI_PANEL_MINIMIZED);
        if (flags & GUI_SCALEABLE)
            gui_panel_header_flag(layout, GUI_SYMBOL_RECT, GUI_SYMBOL_RECT_FILLED,
                align, GUI_PANEL_SCALEABLE);
        if (flags & GUI_MOVEABLE)
            gui_panel_header_flag(layout, GUI_SYMBOL_CIRCLE,
                GUI_SYMBOL_CIRCLE_FILLED, align, GUI_PANEL_MOVEABLE);
        if (title) gui_panel_header_title(layout, title, GUI_HEADER_LEFT);
    }
    gui_panel_header_end(layout);

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
gui_panel_menubar_begin(struct gui_panel_layout *layout)
{
    GUI_ASSERT(layout);
    if (!layout || layout->flags & GUI_PANEL_HIDDEN || layout->flags & GUI_PANEL_MINIMIZED)
        return;
    layout->menu.x = layout->at_x;
    layout->menu.y = layout->y + layout->header.h;
    layout->menu.w = layout->width;
    layout->menu.offset = layout->offset;
    layout->offset.y = 0;
}

void
gui_panel_menubar_end(struct gui_panel_layout *layout)
{
    const struct gui_config *c;
    struct gui_command_buffer *out;
    struct gui_vec2 item_padding;
    struct gui_vec2 panel_padding;

    GUI_ASSERT(layout);
    if (!layout || layout->flags & GUI_PANEL_HIDDEN || layout->flags & GUI_PANEL_MINIMIZED)
        return;

    c = layout->config;
    out = layout->buffer;
    panel_padding = gui_config_property(c, GUI_PROPERTY_PADDING);
    item_padding = gui_config_property(c, GUI_PROPERTY_ITEM_PADDING);

    layout->menu.h = (layout->at_y + layout->row.height-1) - layout->menu.y;
    layout->clip.y = layout->y + layout->header.h + layout->menu.h;
    layout->height -= layout->menu.h;
    layout->offset = layout->menu.offset;
    layout->clip.h = layout->h - (layout->footer_h + layout->header.h + layout->menu.h);
    layout->clip.h -= (panel_padding.y + item_padding.y);
    gui_command_buffer_push_scissor(out, layout->clip);
}

/*
 * -------------------------------------------------------------
 *
 *                      PANEL ROW LAYOUT
 *
 * --------------------------------------------------------------
 */
static void
gui_panel_layout(struct gui_panel_layout *layout, gui_float height, gui_size cols)
{
    const struct gui_config *config;
    const struct gui_color *color;
    struct gui_command_buffer *out;
    struct gui_vec2 item_spacing;
    struct gui_vec2 panel_padding;

    GUI_ASSERT(layout);
    GUI_ASSERT(layout->config);
    GUI_ASSERT(layout->buffer);

    if (!layout) return;
    if (!layout->valid) return;

    /* prefetch some configuration data */
    config = layout->config;
    out = layout->buffer;
    color = &config->colors[GUI_COLOR_PANEL];
    item_spacing = gui_config_property(config, GUI_PROPERTY_ITEM_SPACING);
    panel_padding = gui_config_property(config, GUI_PROPERTY_PADDING);

    /* draw the current row and set the current row layout */
    layout->row.index = 0;
    layout->at_y += layout->row.height;
    layout->row.columns = cols;
    layout->row.height = height + item_spacing.y;
    layout->row.item_offset = 0;
    if (layout->flags & GUI_PANEL_DYNAMIC)
        gui_command_buffer_push_rect(out,  gui_rect(layout->x, layout->at_y,
            layout->w, height + panel_padding.y), 0, *color);
}

static void
gui_panel_row_layout(struct gui_panel_layout *layout,
    enum gui_panel_row_layout_format fmt, gui_float height, gui_size cols,
    gui_size width)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(layout->config);
    GUI_ASSERT(layout->buffer);

    if (!layout) return;
    if (!layout->valid) return;

    /* draw the current row and set the current row layout */
    gui_panel_layout(layout, height, cols);
    if (fmt == GUI_DYNAMIC)
        layout->row.type = GUI_PANEL_LAYOUT_DYNAMIC_FIXED;
    else layout->row.type = GUI_PANEL_LAYOUT_STATIC_FIXED;

    layout->row.item_width = (gui_float)width;
    layout->row.ratio = 0;
    layout->row.item_offset = 0;
    layout->row.filled = 0;
}

void
gui_panel_row_dynamic(struct gui_panel_layout *layout, gui_float height, gui_size cols)
{gui_panel_row_layout(layout, GUI_DYNAMIC, height, cols, 0);}

void
gui_panel_row_static(struct gui_panel_layout *layout, gui_float height,
    gui_size item_width, gui_size cols)
{gui_panel_row_layout(layout, GUI_STATIC, height, cols, item_width);}

void
gui_panel_row_begin(struct gui_panel_layout *layout,
    enum gui_panel_row_layout_format fmt, gui_float row_height, gui_size cols)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(layout->config);
    GUI_ASSERT(layout->buffer);

    if (!layout) return;
    if (!layout->valid) return;

    gui_panel_layout(layout, row_height, cols);
    if (fmt == GUI_DYNAMIC)
        layout->row.type = GUI_PANEL_LAYOUT_DYNAMIC_ROW;
    else layout->row.type = GUI_PANEL_LAYOUT_STATIC_ROW;
    layout->row.ratio = 0;
    layout->row.item_width = 0;
    layout->row.item_offset = 0;
    layout->row.filled = 0;
    layout->row.columns = cols;
}

void
gui_panel_row_push(struct gui_panel_layout *layout, gui_float ratio_or_width)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(layout->config);
    if (!layout || !layout->valid) return;

    if (layout->row.type == GUI_PANEL_LAYOUT_DYNAMIC_ROW) {
        gui_float ratio = ratio_or_width;
        if ((ratio + layout->row.filled) > 1.0f) return;
        if (ratio > 0.0f)
            layout->row.item_width = GUI_SATURATE(ratio);
        else layout->row.item_width = 1.0f - layout->row.filled;
    } else layout->row.item_width = ratio_or_width;
}

void
gui_panel_row_end(struct gui_panel_layout *layout)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(layout->config);
    if (!layout) return;
    if (!layout->valid) return;
    layout->row.item_width = 0;
    layout->row.item_offset = 0;
}

void
gui_panel_row(struct gui_panel_layout *layout, enum gui_panel_row_layout_format fmt,
    gui_float height, gui_size cols, const gui_float *ratio)
{
    gui_size i;
    gui_size n_undef = 0;
    gui_float r = 0;

    GUI_ASSERT(layout);
    GUI_ASSERT(layout->config);
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
        layout->row.type = GUI_PANEL_LAYOUT_DYNAMIC;
        layout->row.item_width = (r > 0 && n_undef > 0) ? (r / (gui_float)n_undef):0;
    } else {
        layout->row.ratio = ratio;
        layout->row.type = GUI_PANEL_LAYOUT_STATIC;
        layout->row.item_width = 0;
        layout->row.item_offset = 0;
    }

    layout->row.item_offset = 0;
    layout->row.filled = 0;
}

void
gui_panel_row_space_begin(struct gui_panel_layout *layout,
    enum gui_panel_row_layout_format fmt, gui_float height, gui_size widget_count)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(layout->config);
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

        layout->row.type = GUI_PANEL_LAYOUT_STATIC_FREE;
        layout->row.clip = layout->clip;
        layout->clip = clip;
    } else layout->row.type = GUI_PANEL_LAYOUT_DYNAMIC_FREE;

    layout->row.ratio = 0;
    layout->row.item_width = 0;
    layout->row.item_offset = 0;
    layout->row.filled = 0;
}

void
gui_panel_row_space_push(struct gui_panel_layout *layout, struct gui_rect rect)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(layout->config);
    GUI_ASSERT(layout->buffer);
    if (!layout) return;
    if (!layout->valid) return;
    layout->row.item = rect;
}

void
gui_panel_row_space_end(struct gui_panel_layout *layout)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(layout->config);
    if (!layout) return;
    if (!layout->valid) return;

    layout->row.item_width = 0;
    layout->row.item_height = 0;
    layout->row.item_offset = 0;
    if (layout->row.type == GUI_PANEL_LAYOUT_STATIC_FREE)
        gui_command_buffer_push_scissor(layout->buffer, layout->clip);
}

static void
gui_panel_alloc_row(struct gui_panel_layout *layout)
{
    const struct gui_config *c = layout->config;
    struct gui_vec2 spacing = gui_config_property(c, GUI_PROPERTY_ITEM_SPACING);
    const gui_float row_height = layout->row.height - spacing.y;
    gui_panel_layout(layout, row_height, layout->row.columns);
}

static void
gui_panel_alloc_space(struct gui_rect *bounds, struct gui_panel_layout *layout)
{
    const struct gui_config *config;
    gui_float panel_padding, panel_spacing, panel_space;
    gui_float item_offset = 0, item_width = 0, item_spacing = 0;
    struct gui_vec2 spacing;
    struct gui_vec2 padding;

    GUI_ASSERT(layout);
    GUI_ASSERT(layout->config);
    GUI_ASSERT(bounds);
    if (!layout || !layout->config || !bounds)
        return;

    /* cache some configuration data */
    config = layout->config;
    spacing = gui_config_property(config, GUI_PROPERTY_ITEM_SPACING);
    padding = gui_config_property(config, GUI_PROPERTY_PADDING);

    /* check if the end of the row was hit and begin new row if so */
    if (layout->row.index >= layout->row.columns)
        gui_panel_alloc_row(layout);

    /* calculate the useable panel space */
    panel_padding = 2 * padding.x;
    panel_spacing = (gui_float)(layout->row.columns - 1) * spacing.x;
    panel_space  = layout->width - panel_padding - panel_spacing;

    /* calculate the width of one item inside the panel row */
    switch (layout->row.type) {
    case GUI_PANEL_LAYOUT_DYNAMIC_FIXED: {
        /* scaling fixed size widgets item width */
        item_width = panel_space / (gui_float)layout->row.columns;
        item_offset = (gui_float)layout->row.index * item_width;
        item_spacing = (gui_float)layout->row.index * spacing.x;
    } break;
    case GUI_PANEL_LAYOUT_DYNAMIC_ROW: {
        /* scaling single ratio widget width */
        item_width = layout->row.item_width * panel_space;
        item_offset = layout->row.item_offset;
        item_spacing = (gui_float)layout->row.index * spacing.x;

        layout->row.item_offset += item_width + spacing.x;
        layout->row.filled += layout->row.item_width;
        layout->row.index = 0;
    } break;
    case GUI_PANEL_LAYOUT_DYNAMIC_FREE: {
        /*panel width depended free widget placing */
        bounds->x = layout->at_x + (layout->width * layout->row.item.x);
        bounds->x -= layout->offset.x;
        bounds->y = layout->at_y + (layout->row.height * layout->row.item.y);
        bounds->w = layout->width  * layout->row.item.w;
        bounds->h = layout->row.height * layout->row.item.h;
        return;
    } break;
    case GUI_PANEL_LAYOUT_DYNAMIC: {
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
    case GUI_PANEL_LAYOUT_STATIC_FIXED: {
        /* non-scaling fixed widgets item width */
        item_width = layout->row.item_width;
        item_offset = (gui_float)layout->row.index * item_width;
        item_spacing = (gui_float)layout->row.index * spacing.x;
    } break;
    case GUI_PANEL_LAYOUT_STATIC_ROW: {
        /* scaling single ratio widget width */
        item_width = layout->row.item_width;
        item_offset = layout->row.item_offset;
        item_spacing = (gui_float)layout->row.index * spacing.x;
        layout->row.item_offset += item_width + spacing.x;
        layout->row.index = 0;
    } break;
    case GUI_PANEL_LAYOUT_STATIC_FREE: {
        /* free widget placing */
        bounds->x = layout->at_x + padding.x + layout->row.item.x;
        bounds->x -= layout->offset.x;
        bounds->y = layout->at_y + layout->row.item.y;
        bounds->w = layout->row.item.w;
        bounds->h = layout->row.item.h;
        if ((bounds->x + bounds->w) > layout->max_x)
            layout->max_x = bounds->x + bounds->w;
        return;
    } break;
    case GUI_PANEL_LAYOUT_STATIC: {
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
gui_panel_layout_push(struct gui_panel_layout *layout,
    enum gui_panel_layout_node_type type,
    const char *title, gui_state *state)
{
    const struct gui_config *config;
    struct gui_command_buffer *out;
    struct gui_vec2 item_spacing;
    struct gui_vec2 item_padding;
    struct gui_vec2 panel_padding;
    struct gui_rect header = {0,0,0,0};
    struct gui_rect sym = {0,0,0,0};

    GUI_ASSERT(layout);
    GUI_ASSERT(layout->config);
    GUI_ASSERT(layout->buffer);

    if (!layout) return gui_false;
    if (!layout->valid) return gui_false;

    /* cache some data */
    out = layout->buffer;
    config = layout->config;

    item_spacing = gui_config_property(config, GUI_PROPERTY_ITEM_SPACING);
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    panel_padding = gui_config_property(config, GUI_PROPERTY_PADDING);

    /* calculate header bounds and draw background */
    gui_panel_row_dynamic(layout, config->font.height + 4 * item_padding.y, 1);
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

        /* calculate the triangle vertexes and draw triangle */
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
            config->colors[GUI_COLOR_PANEL];
        gui_command_buffer_push_text(out, label, (const gui_char*)title, text_len,
            &config->font, color, config->colors[GUI_COLOR_TEXT]);
    }

    /* update node state */
    if (!(layout->flags & GUI_PANEL_ROM)) {
        gui_float clicked_x = layout->input->mouse_clicked_pos.x;
        gui_float clicked_y = layout->input->mouse_clicked_pos.y;
        gui_float mouse_x = layout->input->mouse_pos.x;
        gui_float mouse_y = layout->input->mouse_pos.y;
        if (GUI_INBOX(mouse_x, mouse_y, sym.x, sym.y, sym.w, sym.h)) {
            if (GUI_INBOX(clicked_x, clicked_y, sym.x, sym.y, sym.w, sym.h)) {
                if (layout->input->mouse_down && layout->input->mouse_clicked)
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
gui_panel_layout_pop(struct gui_panel_layout *layout)
{
    const struct gui_config *config;
    struct gui_vec2 panel_padding;

    GUI_ASSERT(layout);
    GUI_ASSERT(layout->config);
    GUI_ASSERT(layout->buffer);

    if (!layout) return;
    if (!layout->valid) return;

    config = layout->config;
    panel_padding = gui_config_property(config, GUI_PROPERTY_PADDING);
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
gui_panel_spacing(struct gui_panel_layout *l, gui_size cols)
{
    gui_size i, n;
    gui_size index;
    struct gui_rect nil;

    GUI_ASSERT(l);
    GUI_ASSERT(l->config);
    GUI_ASSERT(l->buffer);
    if (!l) return;
    if (!l->valid) return;

    index = (l->row.index + cols) % l->row.columns;
    n = index - l->row.index;

    /* spacing goes over the row boundries */
    if (l->row.index + cols > l->row.columns) {
        gui_size rows = (l->row.index + cols) / l->row.columns;
        for (i = 0; i < rows; ++i)
            gui_panel_alloc_row(l);
    }

    /* non table layout need to allocate space */
    if (l->row.type != GUI_PANEL_LAYOUT_DYNAMIC_FIXED &&
        l->row.type != GUI_PANEL_LAYOUT_STATIC_FIXED) {
        for (i = 0; i < n; ++i)
            gui_panel_alloc_space(&nil, l);
    }
    l->row.index = index;
}

enum gui_widget_state
gui_panel_widget(struct gui_rect *bounds, struct gui_panel_layout *layout)
{
    struct gui_rect *c = 0;
    GUI_ASSERT(layout);
    GUI_ASSERT(layout->config);
    GUI_ASSERT(layout->buffer);

    if (!layout) return GUI_WIDGET_INVALID;
    if (!layout->valid || !layout->config || !layout->buffer) return GUI_WIDGET_INVALID;

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
gui_panel_text_colored(struct gui_panel_layout *layout, const char *str, gui_size len,
    enum gui_text_align alignment, struct gui_color color)
{
    struct gui_rect bounds;
    struct gui_text text;
    const struct gui_config *config;
    struct gui_vec2 item_padding;

    GUI_ASSERT(layout);
    GUI_ASSERT(layout->config);
    GUI_ASSERT(layout->buffer);

    if (!layout) return;
    if (!layout->valid || !layout->config || !layout->buffer) return;
    gui_panel_alloc_space(&bounds, layout);
    config = layout->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);

    text.padding.x = item_padding.x;
    text.padding.y = item_padding.y;
    text.foreground = color;
    text.background = config->colors[GUI_COLOR_PANEL];
    gui_text(layout->buffer, bounds, str, len, &text, alignment, &config->font);
}

void
gui_panel_text(struct gui_panel_layout *l, const char *str, gui_size len,
    enum gui_text_align alignment)
{gui_panel_text_colored(l, str, len, alignment,l->config->colors[GUI_COLOR_TEXT]);}

void
gui_panel_label_colored(struct gui_panel_layout *layout, const char *text,
    enum gui_text_align align, struct gui_color color)
{gui_panel_text_colored(layout, text, gui_strsiz(text), align, color);}

void
gui_panel_label(struct gui_panel_layout *layout, const char *text,
    enum gui_text_align align)
{gui_panel_text(layout, text, gui_strsiz(text), align);}

void
gui_panel_image(struct gui_panel_layout *layout, struct gui_image img)
{
    const struct gui_config *config;
    struct gui_vec2 item_padding;
    struct gui_rect bounds;
    GUI_ASSERT(layout);
    if (!gui_panel_widget(&bounds, layout))
        return;

    config = layout->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    bounds.x += item_padding.x;
    bounds.y += item_padding.y;
    bounds.w -= 2 * item_padding.x;
    bounds.h -= 2 * item_padding.y;
    gui_command_buffer_push_image(layout->buffer, bounds, &img);
}

static enum gui_widget_state
gui_panel_button(struct gui_button *button, struct gui_rect *bounds,
    struct gui_panel_layout *layout)
{
    const struct gui_config *config;
    struct gui_vec2 item_padding;
    enum gui_widget_state state;
    state = gui_panel_widget(bounds, layout);
    if (!state) return state;

    config = layout->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    button->border = 1;
    button->alignment = GUI_TEXT_CENTERED;
    button->rounding = config->rounding[GUI_ROUNDING_BUTTON];
    button->padding.x = item_padding.x;
    button->padding.y = item_padding.y;
    return state;
}

gui_bool
gui_panel_button_text(struct gui_panel_layout *layout, const char *str,
    enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;

    const struct gui_input *i;
    enum gui_widget_state state;
    state = gui_panel_button(&button, &bounds, layout);
    if (!state) return gui_false;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_PANEL_ROM) ? 0 : layout->input;

    config = layout->config;
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BORDER];
    button.content = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
    button.highlight_content = config->colors[GUI_COLOR_BUTTON_HOVER_FONT];
    button.rounding = config->rounding[GUI_ROUNDING_BUTTON];
    return gui_button_text(layout->buffer, bounds, str, behavior, &button, i, &config->font);
}

static gui_bool
gui_panel_button_text_fitting(struct gui_panel_layout *layout, const char *str,
    enum gui_text_align align, enum gui_button_behavior behavior)
{
    struct gui_button button;
    struct gui_rect bounds;
    const struct gui_config *config;
    const struct gui_input *i;
    enum gui_widget_state state;
    struct gui_vec2 item_padding;
    struct gui_vec2 panel_padding;
    state = gui_panel_widget(&bounds, layout);
    if (!state) return gui_false;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_PANEL_ROM) ? 0 : layout->input;

    config = layout->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    bounds.x -= item_padding.x;
    bounds.w += 2 * item_padding.x;
    if (layout->row.index > 1 && layout->row.index == layout->row.columns)
    {
        panel_padding = gui_config_property(config, GUI_PROPERTY_PADDING);
        bounds.w += panel_padding.x;
    }

    button.border = 0;
    button.padding.y = 0;
    button.padding.x = 4;
    button.alignment = align;
    button.background = config->colors[GUI_COLOR_PANEL];
    button.foreground = config->colors[GUI_COLOR_PANEL];
    button.content = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
    button.highlight_content = config->colors[GUI_COLOR_BUTTON_HOVER_FONT];
    button.rounding = config->rounding[GUI_ROUNDING_BUTTON];
    return gui_button_text(layout->buffer, bounds, str, behavior, &button, i, &config->font);
}

gui_bool
gui_panel_button_color(struct gui_panel_layout *layout,
   struct gui_color color, enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_input *i;

    enum gui_widget_state state;
    state = gui_panel_button(&button, &bounds, layout);
    if (!state) return gui_false;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_PANEL_ROM) ? 0 : layout->input;

    button.background = color;
    button.foreground = color;
    button.highlight = color;
    button.highlight_content = color;
    button.rounding = layout->config->rounding[GUI_ROUNDING_BUTTON];
    return gui_do_button(layout->buffer, bounds, &button, i, behavior);
}

gui_bool
gui_panel_button_triangle(struct gui_panel_layout *layout, enum gui_heading heading,
    enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;

    const struct gui_input *i;
    enum gui_widget_state state;
    state = gui_panel_button(&button, &bounds, layout);
    if (!state) return gui_false;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_PANEL_ROM) ? 0 : layout->input;

    config = layout->config;
    button.rounding = config->rounding[GUI_ROUNDING_BUTTON];
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BORDER];
    button.content = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
    button.highlight_content = config->colors[GUI_COLOR_BUTTON_HOVER_FONT];
    return gui_button_triangle(layout->buffer, bounds, heading, behavior, &button, i);
}

gui_bool
gui_panel_button_image(struct gui_panel_layout *layout, struct gui_image image,
    enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;

    const struct gui_input *i;
    enum gui_widget_state state;
    state = gui_panel_button(&button, &bounds, layout);
    if (!state) return gui_false;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_PANEL_ROM) ? 0 : layout->input;

    config = layout->config;
    button.rounding = config->rounding[GUI_ROUNDING_BUTTON];
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BORDER];
    button.content = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
    button.highlight_content = config->colors[GUI_COLOR_BUTTON_HOVER_FONT];
    return gui_button_image(layout->buffer, bounds, image, behavior, &button, i);
}

gui_bool
gui_panel_button_toggle(struct gui_panel_layout *layout, const char *str, gui_bool value)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;

    const struct gui_input *i;
    enum gui_widget_state state;
    state = gui_panel_button(&button, &bounds, layout);
    if (!state) return value;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_PANEL_ROM) ? 0 : layout->input;

    config = layout->config;
    button.rounding = config->rounding[GUI_ROUNDING_BUTTON];
    if (!value) {
        button.background = config->colors[GUI_COLOR_BUTTON];
        button.foreground = config->colors[GUI_COLOR_BORDER];
        button.content = config->colors[GUI_COLOR_TEXT];
        button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
        button.highlight_content = config->colors[GUI_COLOR_BUTTON_HOVER_FONT];
    } else {
        button.background = config->colors[GUI_COLOR_BUTTON_TOGGLE];
        button.foreground = config->colors[GUI_COLOR_BORDER];
        button.content = config->colors[GUI_COLOR_BUTTON_HOVER_FONT];
        button.highlight = config->colors[GUI_COLOR_BUTTON];
        button.highlight_content = config->colors[GUI_COLOR_TEXT];
    }
    if (gui_button_text(layout->buffer, bounds, str, GUI_BUTTON_DEFAULT,
        &button, i, &config->font)) value = !value;
    return value;
}

gui_bool
gui_panel_button_text_triangle(struct gui_panel_layout *layout, enum gui_heading heading,
    const char *text, enum gui_text_align align, enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;

    const struct gui_input *i;
    enum gui_widget_state state;
    state = gui_panel_button(&button, &bounds, layout);
    if (!state) return gui_false;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_PANEL_ROM) ? 0 : layout->input;

    config = layout->config;
    button.rounding = config->rounding[GUI_ROUNDING_BUTTON];
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BORDER];
    button.content = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
    button.highlight_content = config->colors[GUI_COLOR_BUTTON_HOVER_FONT];
    return gui_button_text_triangle(layout->buffer, bounds, heading, text, align,
            behavior, &button, &config->font, i);
}

gui_bool
gui_panel_button_text_image(struct gui_panel_layout *layout, struct gui_image img,
    const char *text, enum gui_text_align align, enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;

    const struct gui_input *i;
    enum gui_widget_state state;
    state = gui_panel_button(&button, &bounds, layout);
    if (!state) return gui_false;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_PANEL_ROM) ? 0 : layout->input;

    config = layout->config;
    button.rounding = config->rounding[GUI_ROUNDING_BUTTON];
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BORDER];
    button.content = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
    button.highlight_content = config->colors[GUI_COLOR_BUTTON_HOVER_FONT];
    return gui_button_text_image(layout->buffer, bounds, img, text, align,
                                behavior, &button, &config->font, i);
}

static enum gui_widget_state
gui_panel_toggle_base(struct gui_toggle *toggle, struct gui_rect *bounds,
    struct gui_panel_layout *layout)
{
    const struct gui_config *config;
    struct gui_vec2 item_padding;
    enum gui_widget_state state;
    state = gui_panel_widget(bounds, layout);
    if (!state) return state;

    config = layout->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    toggle->rounding = 0;
    toggle->padding.x = item_padding.x;
    toggle->padding.y = item_padding.y;
    toggle->font = config->colors[GUI_COLOR_TEXT];
    return state;
}

gui_bool
gui_panel_check(struct gui_panel_layout *layout, const char *text, gui_bool is_active)
{
    struct gui_rect bounds;
    struct gui_toggle toggle;
    const struct gui_config *config;

    const struct gui_input *i;
    enum gui_widget_state state;
    state = gui_panel_toggle_base(&toggle, &bounds, layout);
    if (!state) return is_active;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_PANEL_ROM) ? 0 : layout->input;

    config = layout->config;
    toggle.rounding = config->rounding[GUI_ROUNDING_CHECK];
    toggle.cursor = config->colors[GUI_COLOR_CHECK_ACTIVE];
    toggle.background = config->colors[GUI_COLOR_CHECK_BACKGROUND];
    toggle.foreground = config->colors[GUI_COLOR_CHECK];
    return gui_toggle(layout->buffer, bounds, is_active, text, GUI_TOGGLE_CHECK,
                        &toggle, i, &config->font);
}

gui_bool
gui_panel_option(struct gui_panel_layout *layout, const char *text, gui_bool is_active)
{
    struct gui_rect bounds;
    struct gui_toggle toggle;
    const struct gui_config *config;

    const struct gui_input *i;
    enum gui_widget_state state;
    state = gui_panel_toggle_base(&toggle, &bounds, layout);
    if (!state) return is_active;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_PANEL_ROM) ? 0 : layout->input;

    config = layout->config;
    toggle.cursor = config->colors[GUI_COLOR_OPTION_ACTIVE];
    toggle.background = config->colors[GUI_COLOR_OPTION_BACKGROUND];
    toggle.foreground = config->colors[GUI_COLOR_OPTION];
    return gui_toggle(layout->buffer, bounds, is_active, text, GUI_TOGGLE_OPTION,
                        &toggle, i, &config->font);
}

gui_size
gui_panel_option_group(struct gui_panel_layout *layout, const char **options,
    gui_size count, gui_size current)
{
    gui_size i;
    GUI_ASSERT(layout && options && count);
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

    const struct gui_input *i;
    enum gui_widget_state state;
    state = gui_panel_widget(&bounds, layout);
    if (!state) return value;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_PANEL_ROM) ? 0 : layout->input;

    config = layout->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    slider.padding.x = item_padding.x;
    slider.padding.y = item_padding.y;
    slider.bg = config->colors[GUI_COLOR_SLIDER];
    slider.fg = config->colors[GUI_COLOR_SLIDER_CURSOR];
    slider.bar = config->colors[GUI_COLOR_SLIDER_BAR];
    slider.border = config->colors[GUI_COLOR_BORDER];
    return gui_slider(layout->buffer, bounds, min_value, value, max_value,
                        value_step, &slider, i);
}

gui_size
gui_panel_progress(struct gui_panel_layout *layout, gui_size cur_value, gui_size max_value,
    gui_bool is_modifyable)
{
    struct gui_rect bounds;
    struct gui_progress prog;
    const struct gui_config *config;
    struct gui_vec2 item_padding;

    const struct gui_input *i;
    enum gui_widget_state state;
    state = gui_panel_widget(&bounds, layout);
    if (!state) return cur_value;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_PANEL_ROM) ? 0 : layout->input;

    config = layout->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    prog.rounding = config->rounding[GUI_ROUNDING_PROGRESS];
    prog.padding.x = item_padding.x;
    prog.padding.y = item_padding.y;
    prog.background = config->colors[GUI_COLOR_PROGRESS];
    prog.foreground = config->colors[GUI_COLOR_PROGRESS_CURSOR];
    return gui_progress(layout->buffer, bounds, cur_value, max_value,
                        is_modifyable, &prog, i);
}

static enum gui_widget_state
gui_panel_edit_base(struct gui_rect *bounds, struct gui_edit *field,
    struct gui_panel_layout *layout)
{
    const struct gui_config *config;
    struct gui_vec2 item_padding;
    enum gui_widget_state state = gui_panel_widget(bounds, layout);
    if (!state) return state;

    config = layout->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
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
gui_panel_editbox(struct gui_panel_layout *layout, struct gui_edit_box *box)
{
    struct gui_rect bounds;
    struct gui_edit field;
    const struct gui_config *config = layout->config;
    const struct gui_input *i;
    enum gui_widget_state state;

    state = gui_panel_edit_base(&bounds, &field, layout);
    if (!state) return;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_PANEL_ROM) ? 0 : layout->input;
    gui_editbox(layout->buffer, bounds, box, &field, i, &config->font);
}

gui_size
gui_panel_edit(struct gui_panel_layout *layout, gui_char *buffer, gui_size len,
    gui_size max, gui_state *active, gui_size *cursor, enum gui_input_filter filter)
{
    struct gui_rect bounds;
    struct gui_edit field;
    const struct gui_config *config = layout->config;
    const struct gui_input *i;
    enum gui_widget_state state;

    state = gui_panel_edit_base(&bounds, &field, layout);
    if (!state) return len;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_PANEL_ROM) ? 0 : layout->input;
    return gui_edit(layout->buffer, bounds,  buffer, len, max, active, cursor,
                    &field, filter, i, &config->font);
}

gui_size
gui_panel_edit_filtered(struct gui_panel_layout *layout, gui_char *buffer, gui_size len,
    gui_size max, gui_state *active, gui_size *cursor, gui_filter filter)
{
    struct gui_rect bounds;
    struct gui_edit field;
    const struct gui_config *config = layout->config;
    const struct gui_input *i;
    enum gui_widget_state state;

    state = gui_panel_edit_base(&bounds, &field, layout);
    if (!state) return len;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_PANEL_ROM) ? 0 : layout->input;
    return gui_edit_filtered(layout->buffer, bounds, buffer, len, max, active,
                            cursor, &field, filter, i, &config->font);
}

static enum gui_widget_state
gui_panel_spinner(struct gui_panel_layout *layout, struct gui_rect *bounds,
        struct gui_spinner *spinner)
{
    struct gui_vec2 item_padding;
    enum gui_widget_state state;
    const struct gui_config *config;

    GUI_ASSERT(layout);
    GUI_ASSERT(bounds);
    GUI_ASSERT(spinner);
    if (!layout || !bounds || !spinner)
        return GUI_WIDGET_INVALID;

    state = gui_panel_widget(bounds, layout);
    if (!state) return state;
    config = layout->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);

    spinner->border_button = 1;
    spinner->button_color = config->colors[GUI_COLOR_SELECTOR_BUTTON];
    spinner->button_border = config->colors[GUI_COLOR_BORDER];
    spinner->button_triangle = config->colors[GUI_COLOR_SELECTOR_TRIANGLE];
    spinner->padding.x = item_padding.x;
    spinner->padding.y = item_padding.y;
    spinner->color = config->colors[GUI_COLOR_SELECTOR];
    spinner->border = config->colors[GUI_COLOR_BORDER];
    spinner->text = config->colors[GUI_COLOR_SELECTOR_TEXT];
    spinner->show_cursor = gui_false;
    return state;
}

gui_int
gui_panel_spinner_int(struct gui_panel_layout *layout, gui_int min, gui_int value,
    gui_int max, gui_int step, gui_state *active)
{
    struct gui_rect bounds;
    struct gui_spinner spinner;
    struct gui_command_buffer *out;
    const struct gui_input *i;
    enum gui_widget_state state;

    state = gui_panel_spinner(layout, &bounds, &spinner);
    if (!state) return value;
    out = layout->buffer;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_PANEL_ROM) ? 0 : layout->input;
    return gui_spinner_int(out, bounds, &spinner, min, value, max, step, active,
                        i, &layout->config->font);
}

gui_float
gui_panel_spinner_float(struct gui_panel_layout *layout, gui_float min, gui_float value,
    gui_float max, gui_float step, gui_state *active)
{
    struct gui_rect bounds;
    struct gui_spinner spinner;
    struct gui_command_buffer *out;
    const struct gui_input *i;
    enum gui_widget_state state;

    state = gui_panel_spinner(layout, &bounds, &spinner);
    if (!state) return value;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_PANEL_ROM) ? 0 : layout->input;
    out = layout->buffer;
    return gui_spinner_float(out, bounds, &spinner, min, value, max, step, active,
                        i, &layout->config->font);
}

gui_size
gui_panel_selector(struct gui_panel_layout *layout, const char *items[],
    gui_size item_count, gui_size item_current)
{
    struct gui_rect bounds;
    struct gui_selector selector;
    const struct gui_config *config;
    struct gui_command_buffer *out;

    const struct gui_input *i;
    enum gui_widget_state state;
    state = gui_panel_widget(&bounds, layout);
    if (!state) return item_current;
    i = (state == GUI_WIDGET_ROM || layout->flags & GUI_PANEL_ROM) ? 0 : layout->input;

    GUI_ASSERT(items);
    GUI_ASSERT(item_count);
    GUI_ASSERT(item_current < item_count);

    out = layout->buffer;
    config = layout->config;

    selector.border_button = 1;
    selector.button_color = config->colors[GUI_COLOR_SELECTOR_BUTTON];
    selector.button_border = config->colors[GUI_COLOR_BORDER];
    selector.button_triangle = config->colors[GUI_COLOR_SELECTOR_TRIANGLE];
    selector.color = config->colors[GUI_COLOR_SELECTOR];
    selector.border = config->colors[GUI_COLOR_BORDER];
    selector.text = config->colors[GUI_COLOR_SELECTOR_TEXT];
    selector.text_bg = config->colors[GUI_COLOR_SELECTOR];
    selector.padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    return gui_selector(out, bounds, &selector, items, item_count, item_current,
                        i, &layout->config->font);
}

/*
 * -------------------------------------------------------------
 *
 *                          GRAPH
 *
 * --------------------------------------------------------------
 */
void
gui_panel_graph_begin(struct gui_panel_layout *layout, struct gui_graph *graph,
    enum gui_graph_type type, gui_size count, gui_float min_value, gui_float max_value)
{
    struct gui_rect bounds = {0, 0, 0, 0};
    const struct gui_config *config;
    struct gui_command_buffer *out;
    struct gui_color color;
    struct gui_vec2 item_padding;
    if (!gui_panel_widget(&bounds, layout)) {
        gui_zero(graph, sizeof(*graph));
        return;
    }

    /* draw graph background */
    config = layout->config;
    out = layout->buffer;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
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
gui_panel_graph_push_line(struct gui_panel_layout *layout,
    struct gui_graph *g, gui_float value)
{
    struct gui_command_buffer *out = layout->buffer;
    const struct gui_config *config = layout->config;
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
        if (!(layout->flags & GUI_PANEL_ROM) &&
            GUI_INBOX(i->mouse_pos.x,i->mouse_pos.y,g->last.x-3,g->last.y-3,6,6)){
            selected = (i->mouse_down && i->mouse_clicked) ? gui_true: gui_false;
            color = config->colors[GUI_COLOR_PLOT_HIGHLIGHT];
        }
        gui_command_buffer_push_rect(out, gui_rect(g->last.x - 3, g->last.y - 3, 6, 6), 0, color);
        g->index++;
        return selected;
    }

    /* draw a line between the last data point and the new one */
    cur.x = g->x + (gui_float)(step * (gui_float)g->index);
    cur.y = (g->y + g->h) - (ratio * (gui_float)g->h);
    gui_command_buffer_push_line(out, g->last.x, g->last.y, cur.x, cur.y,
        config->colors[GUI_COLOR_PLOT_LINES]);

    /* user selection of the current data point */
    if (!(layout->flags & GUI_PANEL_ROM) &&
        GUI_INBOX(i->mouse_pos.x, i->mouse_pos.y, cur.x-3, cur.y-3, 6, 6)) {
        selected = (i->mouse_down && i->mouse_clicked) ? gui_true: gui_false;
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
gui_panel_graph_push_column(struct gui_panel_layout *layout,
    struct gui_graph *graph, gui_float value)
{
    struct gui_command_buffer *out = layout->buffer;
    const struct gui_config *config = layout->config;
    const struct gui_input *in = layout->input;
    struct gui_vec2 item_padding;
    struct gui_color color;

    gui_float ratio;
    gui_bool selected = gui_false;
    struct gui_rect item = {0,0,0,0};
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);

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
    if (!(layout->flags & GUI_PANEL_ROM) &&
        GUI_INBOX(in->mouse_pos.x,in->mouse_pos.y,item.x,item.y,item.w,item.h)) {
        selected = (in->mouse_down && in->mouse_clicked) ? (gui_int)graph->index: selected;
        color = config->colors[GUI_COLOR_HISTO_HIGHLIGHT];
    }
    gui_command_buffer_push_rect(out, item, 0, color);
    graph->index++;
    return selected;
}

gui_bool
gui_panel_graph_push(struct gui_panel_layout *layout, struct gui_graph *graph,
    gui_float value)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(graph);
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
gui_panel_graph(struct gui_panel_layout *layout, enum gui_graph_type type,
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
    gui_panel_graph_begin(layout, &graph, type, count, min_value, max_value);
    for (i = offset; i < count; ++i) {
        if (gui_panel_graph_push(layout, &graph, values[i]))
            index = (gui_int)i;
    }
    gui_panel_graph_end(layout, &graph);
    return index;
}

gui_int
gui_panel_graph_callback(struct gui_panel_layout *layout, enum gui_graph_type type,
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
    gui_panel_graph_begin(layout, &graph, type, count, min_value, max_value);
    for (i = 0; i < count; ++i) {
        gui_float value = get_value(userdata, i);
        if (gui_panel_graph_push(layout, &graph, value))
            index = (gui_int)i;
    }
    gui_panel_graph_end(layout, &graph);
    return index;
}

/*
 * -------------------------------------------------------------
 *
 *                          TABLE
 *
 * --------------------------------------------------------------
 */
static void
gui_panel_table_horizontal_line(struct gui_panel_layout *l, gui_size row_height)
{
    gui_float x, y, w;
    struct gui_command_buffer *out = l->buffer;
    const struct gui_config *c = l->config;
    const struct gui_vec2 item_padding = gui_config_property(c, GUI_PROPERTY_ITEM_PADDING);
    const struct gui_vec2 item_spacing = gui_config_property(c, GUI_PROPERTY_ITEM_SPACING);
    if (!l->valid) return;

    /* draws a horizontal line under the current item */
    w = MAX(l->width, (2 * item_spacing.x + 2 * item_padding.x));
    y = (l->at_y + (gui_float)row_height + item_padding.y) - l->offset.y;
    x = l->at_x + item_spacing.x + item_padding.x - l->offset.x;
    w = w - (2 * item_spacing.x + 2 * item_padding.x);
    gui_command_buffer_push_line(out, x, y, x + w, y, c->colors[GUI_COLOR_TABLE_LINES]);
}

static void
gui_panel_table_vertical_line(struct gui_panel_layout *layout, gui_size cols)
{
    gui_size i;
    struct gui_command_buffer *out;
    const struct gui_config *config;

    GUI_ASSERT(layout);
    GUI_ASSERT(cols);
    if (!layout || !cols) return;
    if (!layout->valid) return;

    out = layout->buffer;
    config = layout->config;
    for (i = 0; i < cols - 1; ++i) {
        gui_float y, h;
        struct gui_rect bounds;

        /* draw a line between every item in the current row */
        gui_panel_alloc_space(&bounds, layout);
        y = layout->at_y + layout->row.height;
        h = layout->row.height;
        gui_command_buffer_push_line(out,  bounds.x + bounds.w, y,
            bounds.x + bounds.w, h, config->colors[GUI_COLOR_TABLE_LINES]);
    }
    layout->row.index -= i;
}

void
gui_panel_table_begin(struct gui_panel_layout *layout, gui_flags flags,
    gui_size row_height, gui_size cols)
{
    GUI_ASSERT(layout);
    if (!layout || !layout->valid) return;

    layout->is_table = gui_true;
    layout->tbl_flags = flags;
    gui_panel_row_dynamic(layout,  (gui_float)row_height, cols);
    if (layout->tbl_flags & GUI_TABLE_HHEADER)
        gui_panel_table_horizontal_line(layout, row_height);
    if (layout->tbl_flags & GUI_TABLE_VHEADER)
        gui_panel_table_vertical_line(layout, cols);
}

void
gui_panel_table_row(struct gui_panel_layout *layout)
{
    const struct gui_config *config;
    struct gui_vec2 item_spacing;
    GUI_ASSERT(layout);
    if (!layout) return;
    if (!layout->valid) return;

    config = layout->config;
    item_spacing = gui_config_property(config, GUI_PROPERTY_ITEM_SPACING);
    gui_panel_row_dynamic(layout, layout->row.height-item_spacing.y,layout->row.columns);
    if (layout->tbl_flags & GUI_TABLE_HBODY)
        gui_panel_table_horizontal_line(layout, (gui_size)(layout->row.height - item_spacing.y));
    if (layout->tbl_flags & GUI_TABLE_VBODY)
        gui_panel_table_vertical_line(layout, layout->row.columns);
}

void
gui_panel_table_end(struct gui_panel_layout *layout)
{
    /* I personally don't like the flag but it is the easiest way to achieve a table */
    layout->is_table = gui_false;
}

/*
 * -------------------------------------------------------------
 *
 *                          POPUP
 *
 * --------------------------------------------------------------
 */
gui_flags
gui_panel_popup_begin(struct gui_panel_layout *parent, struct gui_panel_layout *popup,
    enum gui_popup_type type, struct gui_rect rect, struct gui_vec2 scrollbar)
{
    gui_flags flags = 0;
    struct gui_panel panel;
    struct gui_rect bounds;

    GUI_ASSERT(parent);
    GUI_ASSERT(popup);
    if (!parent || !popup || !parent->valid) {
        popup->valid = gui_false;
        popup->config = parent->config;
        popup->buffer = parent->buffer;
        popup->input = parent->input;
        return 0;
    }

    /* calculate bounds of the panel */
    gui_zero(popup, sizeof(*popup));
    rect.x += parent->at_x;
    rect.y += parent->clip.y;
    gui_unify(&bounds, &parent->clip, rect.x, rect.y, rect.x + rect.w, rect.y + rect.h);
    parent->flags |= GUI_PANEL_ROM;

    /* initialize a fake panel */
    flags = GUI_PANEL_BORDER|GUI_PANEL_TAB;
    if (type == GUI_POPUP_DYNAMIC)
        flags |= GUI_PANEL_DYNAMIC;
    gui_panel_init(&panel, bounds.x, bounds.y, bounds.w,bounds.h,flags, 0,
        parent->config, parent->input);

    /* begin sub-buffer and create panel layout  */
    gui_command_queue_start_child(parent->queue, parent->buffer);
    panel.buffer = *parent->buffer;
    gui_panel_begin(popup, &panel);
    *parent->buffer = panel.buffer;
    parent->flags |= GUI_PANEL_ROM;

    popup->buffer = parent->buffer;
    popup->offset = scrollbar;
    popup->queue = parent->queue;
    return 1;
}

void
gui_panel_popup_close(struct gui_panel_layout *popup)
{
    GUI_ASSERT(popup);
    if (!popup || !popup->valid) return;
    popup->flags |= GUI_PANEL_HIDDEN;
    popup->valid = gui_false;
}

struct gui_vec2
gui_panel_popup_end(struct gui_panel_layout *parent, struct gui_panel_layout *popup)
{
    struct gui_panel pan;
    struct gui_command_buffer *out;

    GUI_ASSERT(parent);
    GUI_ASSERT(popup);
    if (!parent || !popup) return gui_vec2(0,0);
    if (!parent->valid) return gui_vec2(0,0);

    gui_zero(&pan, sizeof(pan));
    if (popup->flags & GUI_PANEL_HIDDEN) {
        parent->flags |= GUI_PANEL_REMOVE_ROM;
        popup->flags &= ~(gui_flags)~GUI_PANEL_HIDDEN;
        popup->valid = gui_true;
    }

    out = parent->buffer;
    pan.x = popup->x;
    pan.y = popup->y;
    pan.w = popup->width;
    pan.h = popup->height;
    pan.flags = GUI_PANEL_BORDER|GUI_PANEL_TAB;

    /* end popup and reset clipping rect back to parent panel */
    gui_command_buffer_push_scissor(out, parent->clip);
    gui_panel_end(popup, &pan);
    gui_command_queue_finish_child(parent->queue, parent->buffer);
    gui_command_buffer_push_scissor(out, parent->clip);
    return pan.offset;
}

static gui_bool
gui_panel_popup_nonblock_begin(struct gui_panel_layout *parent,
    struct gui_panel_layout *popup, gui_state *active, gui_state is_active,
    struct gui_rect body, struct gui_vec2 offset)
{
    /* deactivate popup if user clicked outside the popup*/
    const struct gui_input *in = parent->input;
    if (in && *active) {
        gui_bool inbody = GUI_INBOX(in->mouse_clicked_pos.x,
            in->mouse_clicked_pos.y, body.x, body.y, body.w, body.h);
        gui_bool inpanel = GUI_INBOX(in->mouse_clicked_pos.x,
            in->mouse_clicked_pos.y, parent->x, parent->y, parent->w, parent->h);
        if ((in->mouse_down && in->mouse_clicked) && !inbody && inpanel)
            is_active = gui_false;
    }

    /* recalculate body bounds into local panel position */
    body.x -= (parent->at_x - 2);
    body.y -= parent->clip.y;
    body.w -= 4;

    /* if active create popup otherwise deactive the panel layout  */
    if (!is_active && *active) {
        gui_panel_popup_begin(parent, popup, GUI_POPUP_DYNAMIC, body, offset);
        gui_panel_popup_close(popup);
        popup->flags &= ~(gui_flags)GUI_PANEL_MINIMIZED;
        parent->flags &= ~(gui_flags)GUI_PANEL_ROM;
    } else if (!is_active && !*active) {
        *active = is_active;
        popup->flags |= GUI_PANEL_MINIMIZED;
        return gui_false;
    } else {
        gui_panel_popup_begin(parent, popup, GUI_POPUP_DYNAMIC, body, offset);
        popup->flags &= ~(gui_flags)GUI_PANEL_MINIMIZED;
    }
    *active = is_active;
    return gui_true;
}

static void
gui_panel_popup_nonblock_end(struct gui_panel_layout *parent, struct gui_panel_layout *popup)
{
    GUI_ASSERT(parent);
    GUI_ASSERT(popup);
    if (!parent || !popup) return;
    if (!parent->valid) return;
    if (!(popup->flags & GUI_PANEL_MINIMIZED))
        gui_panel_popup_end(parent, popup);
}

/*
 * -------------------------------------------------------------
 *
 *                          COMBO
 *
 * --------------------------------------------------------------
 */
void
gui_panel_combo_begin(struct gui_panel_layout *parent, struct gui_panel_layout *combo,
    const char *selected, gui_state *active, struct gui_vec2 offset)
{
    struct gui_rect bounds = {0,0,0,0};
    const struct gui_input *in;
    const struct gui_config *config;
    struct gui_command_buffer *out;
    struct gui_vec2 item_padding;
    struct gui_rect header;
    gui_state is_active;

    GUI_ASSERT(parent);
    GUI_ASSERT(combo);
    GUI_ASSERT(selected);
    GUI_ASSERT(active);
    if (!parent || !combo || !selected || !active) return;
    if (!parent->valid || !gui_panel_widget(&header, parent))
        goto failed;

    gui_zero(combo, sizeof(*combo));
    in = (parent->flags & GUI_PANEL_ROM) ? 0 : parent->input;
    is_active = *active;
    out = parent->buffer;
    config = parent->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);

    /* draw combo box header background and border */
    gui_command_buffer_push_rect(out, header, 0, config->colors[GUI_COLOR_BORDER]);
    gui_command_buffer_push_rect(out, gui_shrink_rect(header, 1), 0,
        config->colors[GUI_COLOR_SELECTOR]);

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
                config->colors[GUI_COLOR_PANEL], config->colors[GUI_COLOR_TEXT]);
        gui_command_buffer_push_scissor(out, parent->clip);
    }
    {
        /* button setup and execution */
        struct gui_button button;
        bounds.y = header.y;
        bounds.h = header.h;
        bounds.w = header.h;
        bounds.x = header.x + header.w - bounds.w;

        button.rounding = 0;
        button.border = 1;
        button.padding.x = bounds.w/4.0f;
        button.padding.y = bounds.h/4.0f;
        button.background = config->colors[GUI_COLOR_SELECTOR_BUTTON];
        button.foreground = config->colors[GUI_COLOR_BORDER];
        button.content = config->colors[GUI_COLOR_SELECTOR_TRIANGLE];
        button.highlight = config->colors[GUI_COLOR_SELECTOR_BUTTON];
        button.highlight_content = config->colors[GUI_COLOR_SELECTOR_TRIANGLE];
        button.rounding = config->rounding[GUI_ROUNDING_BUTTON];
        if (gui_button_triangle(out, bounds, GUI_DOWN, GUI_BUTTON_DEFAULT, &button, in))
            is_active = !is_active;
    }
    {
        /* calculate the maximum height of the combo box*/
        struct gui_rect body;
        body.x = header.x;
        body.w = header.w;
        body.y = header.y + header.h;
        body.h = (parent->y + parent->h) - body.y;
        if (!gui_panel_popup_nonblock_begin(parent, combo, active, is_active, body, offset))
            goto failed;
        combo->flags |= GUI_PANEL_COMBO_MENU;
    }
    return;

failed:
    combo->valid = gui_false;
    combo->config = parent->config;
    combo->buffer = parent->buffer;
    combo->input = parent->input;
    combo->queue = parent->queue;
}

struct gui_vec2
gui_panel_combo_end(struct gui_panel_layout *parent, struct gui_panel_layout *combo)
{
    GUI_ASSERT(parent);
    GUI_ASSERT(combo);
    if (!parent || !combo) return gui_vec2(0,0);
    if ((!parent->valid || !combo->valid) && !(combo->flags & GUI_PANEL_HIDDEN))
        return combo->offset;
    gui_panel_popup_nonblock_end(parent, combo);
    return combo->offset;
}

void
gui_panel_combo_close(struct gui_panel_layout *combo)
{
    GUI_ASSERT(combo);
    if (!combo) return;
    gui_panel_popup_close(combo);
    combo->flags |= GUI_PANEL_HIDDEN;
}

void
gui_panel_combo(struct gui_panel_layout *layout, const char **entries,
    gui_size count, gui_size *current, gui_size row_height,
    gui_state *active, struct gui_vec2 *scrollbar)
{
    gui_size i;
    struct gui_panel_layout combo;
    GUI_ASSERT(layout);
    GUI_ASSERT(entries);
    GUI_ASSERT(current);
    GUI_ASSERT(active);
    if (!layout || !layout->valid || !entries || !current || !active) return;
    if (!count) return;

    gui_zero(&combo, sizeof(combo));
    gui_panel_combo_begin(layout, &combo, entries[*current], active, *scrollbar);
    gui_panel_row_dynamic(&combo, (gui_float)row_height, 1);
    for (i = 0; i < count; ++i) {
        if (i == *current) continue;
        if (gui_panel_button_text_fitting(&combo, entries[i], GUI_TEXT_LEFT, GUI_BUTTON_DEFAULT)) {
            gui_panel_combo_close(&combo);
            *active = gui_false;
            *current = i;
        }
    }
    *scrollbar = gui_panel_combo_end(layout, &combo);
}
/*
 * -------------------------------------------------------------
 *
 *                          MENU
 *
 * --------------------------------------------------------------
 */
void
gui_panel_menu_begin(struct gui_panel_layout *parent, struct gui_panel_layout *menu,
    const char *title, gui_float width, gui_state *active, struct gui_vec2 offset)
{
    const struct gui_input *in;
    const struct gui_config *config;
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
    config = parent->config;
    gui_zero(menu, sizeof(*menu));
    {
        /* exeucte menu button for open/closing the popup */
        struct gui_button button;
        gui_panel_button(&button, &header, parent);
        button.background = (is_active) ? config->colors[GUI_COLOR_BUTTON_HOVER]:
            config->colors[GUI_COLOR_PANEL];
        button.foreground = (is_active) ? config->colors[GUI_COLOR_BORDER]:
            config->colors[GUI_COLOR_PANEL];
        button.content = config->colors[GUI_COLOR_TEXT];
        button.highlight = (is_active) ? config->colors[GUI_COLOR_BUTTON_HOVER]:
            config->colors[GUI_COLOR_PANEL];
        button.highlight_content = config->colors[GUI_COLOR_TEXT];
        button.rounding = config->rounding[GUI_ROUNDING_BUTTON];
        if (gui_button_text(parent->buffer, header, title, GUI_BUTTON_DEFAULT,
                &button, in, &config->font))
            is_active = !is_active;
    }
    {
        /* calculate the maximum height of the menu */
        struct gui_rect body;
        body.x = header.x;
        body.w = width;
        body.y = header.y + header.h;
        body.h = (parent->y + parent->h) - body.y;
        if (!gui_panel_popup_nonblock_begin(parent, menu, active,
            is_active, body, offset)) goto failed;
        menu->flags |= GUI_PANEL_COMBO_MENU;
    }
    return;

failed:
    menu->valid = gui_false;
    menu->config = parent->config;
    menu->buffer = parent->buffer;
    menu->input = parent->input;
    menu->queue = parent->queue;
}

void
gui_panel_menu_close(struct gui_panel_layout *menu)
{
    GUI_ASSERT(menu);
    if (!menu) return;
    gui_panel_popup_close(menu);
}

struct gui_vec2
gui_panel_menu_end(struct gui_panel_layout *parent, struct gui_panel_layout *menu)
{
    GUI_ASSERT(parent);
    GUI_ASSERT(menu);
    if (!parent || !menu) return gui_vec2(0,0);
    if (!parent->valid)
        return menu->offset;
    gui_panel_popup_nonblock_end(parent, menu);
    return menu->offset;
}

gui_int
gui_panel_menu(struct gui_panel_layout *layout, const char *title,
    const char **entries, gui_size count, gui_size row_height,
    gui_float width, gui_state *active, struct gui_vec2 scrollbar)
{
    gui_size i;
    gui_int sel = -1;
    struct gui_panel_layout menu;

    GUI_ASSERT(layout);
    GUI_ASSERT(entries);
    GUI_ASSERT(title);
    GUI_ASSERT(active);
    if (!layout || !layout->valid || !entries || !title || !active) return -1;
    if (!count) return -1;

    gui_panel_menu_begin(layout, &menu, title, width, active, scrollbar);
    gui_panel_row_dynamic(&menu, (gui_float)row_height, 1);
    for (i = 0; i < count; ++i) {
        if (gui_panel_button_text_fitting(&menu, entries[i], GUI_TEXT_CENTERED, GUI_BUTTON_DEFAULT)) {
            gui_panel_combo_close(&menu);
            *active = gui_false;
            sel = (gui_int)i;
        }
    }
    gui_panel_menu_end(layout, &menu);
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
gui_panel_tree_begin(struct gui_panel_layout *p, struct gui_tree *tree,
    const char *title, gui_float height, struct gui_vec2 offset)
{
    struct gui_vec2 padding;
    const struct gui_config *config;
    GUI_ASSERT(p);
    GUI_ASSERT(tree);
    GUI_ASSERT(title);
    if (!p || !tree || !title) return;

    gui_panel_group_begin(p, &tree->group, title, offset);
    gui_panel_layout(&tree->group, height, 1);

    config = tree->group.config;
    padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);

    tree->at_x = 0;
    tree->skip = -1;
    tree->depth = 0;
    tree->x_off = tree->group.config->font.height + 2 * padding.x;
}

static enum gui_tree_node_operation
gui_panel_tree_node(struct gui_tree *tree, enum gui_tree_node_symbol symbol,
    const char *title, struct gui_image *img, gui_state *state)
{
    struct gui_text text;
    struct gui_rect bounds = {0,0,0,0};
    struct gui_rect sym, label, icon;
    struct gui_panel_layout *layout;
    enum gui_tree_node_operation op = GUI_NODE_NOP;

    const struct gui_input *i;
    const struct gui_config *config;
    struct gui_vec2 item_padding;
    struct gui_color col;

    GUI_ASSERT(tree);
    GUI_ASSERT(state);
    GUI_ASSERT(title);
    if (!tree || !state || !title)
        return GUI_NODE_NOP;

    layout = &tree->group;
    if (tree->skip >= 0 || !gui_panel_widget(&bounds, layout)) {
        if (!tree->depth) tree->at_x = bounds.x;
        return op;
    }

    if (tree->depth){
        bounds.w = (bounds.x + bounds.w) - tree->at_x;
        bounds.x = tree->at_x;
    } else tree->at_x = bounds.x;

    /* fetch some configuration constants */
    i = layout->input;
    config = layout->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    col = gui_config_color(config, GUI_COLOR_TEXT);

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
        if (gui_input_mouse_clicked(i, sym)) {
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

    if (!(layout->flags & GUI_PANEL_ROM)) {
        /* node selection */
        if (gui_input_mouse_clicked(i, label)) {
            if (*state & GUI_NODE_SELECTED)
                *state &= ~(gui_flags)GUI_NODE_SELECTED;
            else *state |= GUI_NODE_SELECTED;
        }

        /* tree node operations */
        if (gui_input_is_key_pressed(i, GUI_KEY_DEL) && (*state & GUI_NODE_SELECTED)) {
            *state &= ~(gui_flags)GUI_NODE_SELECTED;
            op = GUI_NODE_DELETE;
        } else if (gui_input_is_key_pressed(i, GUI_KEY_COPY) && (*state & GUI_NODE_SELECTED)) {
            *state &= ~(gui_flags)GUI_NODE_SELECTED;
            op = GUI_NODE_CLONE;
        } else if (gui_input_is_key_pressed(i, GUI_KEY_CUT) && (*state & GUI_NODE_SELECTED)) {
            *state &= ~(gui_flags)GUI_NODE_SELECTED;
            op = GUI_NODE_CUT;
        } else if (gui_input_is_key_pressed(i, GUI_KEY_PASTE) && (*state & GUI_NODE_SELECTED)) {
            *state &= ~(gui_flags)GUI_NODE_SELECTED;
            op = GUI_NODE_PASTE;
        }
    }

    /* output label */
    text.padding.x = item_padding.x;
    text.padding.y = item_padding.y;
    text.foreground = config->colors[GUI_COLOR_TEXT];
    text.background = (*state & GUI_NODE_SELECTED) ?
        config->colors[GUI_COLOR_BUTTON_HOVER]:
        config->colors[GUI_COLOR_PANEL];
    gui_text(layout->buffer, label, title, gui_strsiz(title), &text, GUI_TEXT_LEFT, &config->font);
    return op;
}

enum gui_tree_node_operation
gui_panel_tree_begin_node(struct gui_tree *tree, const char *title,
    gui_state *state)
{
    enum gui_tree_node_operation op;
    GUI_ASSERT(tree);
    GUI_ASSERT(state);
    GUI_ASSERT(title);
    if (!tree || !state || !title)
        return GUI_NODE_NOP;

    op = gui_panel_tree_node(tree, GUI_TREE_NODE_TRIANGLE, title, 0, state);
    tree->at_x += tree->x_off;
    if (tree->skip < 0 && !(*state & GUI_NODE_ACTIVE))
        tree->skip = tree->depth;
    tree->depth++;
    return op;
}

enum gui_tree_node_operation
gui_panel_tree_begin_node_icon(struct gui_tree *tree, const char *title,
    struct gui_image img, gui_state *state)
{
    enum gui_tree_node_operation op;
    GUI_ASSERT(tree);
    GUI_ASSERT(state);
    GUI_ASSERT(title);
    if (!tree || !state || !title)
        return GUI_NODE_NOP;

    op = gui_panel_tree_node(tree, GUI_TREE_NODE_TRIANGLE, title, &img, state);
    tree->at_x += tree->x_off;
    if (tree->skip < 0 && !(*state & GUI_NODE_ACTIVE))
        tree->skip = tree->depth;
    tree->depth++;
    return op;
}

enum gui_tree_node_operation
gui_panel_tree_leaf(struct gui_tree *tree, const char *title, gui_state *state)
{return gui_panel_tree_node(tree, GUI_TREE_NODE_BULLET, title, 0, state);}

enum gui_tree_node_operation
gui_panel_tree_leaf_icon(struct gui_tree *tree, const char *title, struct gui_image img,
    gui_state *state)
{return gui_panel_tree_node(tree, GUI_TREE_NODE_BULLET, title, &img, state);}

void
gui_panel_tree_end_node(struct gui_tree *tree)
{
    GUI_ASSERT(tree->depth);
    tree->depth--;
    tree->at_x -= tree->x_off;
    if (tree->skip == tree->depth)
        tree->skip = -1;
}

struct gui_vec2
gui_panel_tree_end(struct gui_panel_layout *p, struct gui_tree* tree)
{return gui_panel_group_end(p, &tree->group);}

/*
 * -------------------------------------------------------------
 *
 *                          GROUPS
 *
 * --------------------------------------------------------------
 */
void
gui_panel_group_begin(struct gui_panel_layout *p, struct gui_panel_layout *g,
    const char *title, struct gui_vec2 offset)
{
    gui_flags flags;
    struct gui_rect bounds;
    struct gui_panel panel;
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
    flags = GUI_PANEL_BORDER|GUI_PANEL_TAB;
    if (p->flags & GUI_PANEL_ROM)
        flags |= GUI_PANEL_ROM;

    gui_panel_init(&panel, bounds.x, bounds.y,bounds.w,bounds.h,flags, 0, p->config, p->input);
    panel.buffer = *p->buffer;
    gui_panel_begin(g, &panel);
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
        if (title) gui_panel_header(g, title, 0, 0, GUI_HEADER_LEFT);
        gui_unify(&clip, &p->clip, g->clip.x, g->clip.y, g->clip.x + g->clip.w,
            g->clip.y + g->clip.h);

        gui_command_buffer_push_scissor(out, clip);
        g->clip = clip;
    }
    return;

failed:
    /* invalid panels still need correct data */
    g->valid = gui_false;
    g->config = p->config;
    g->buffer = p->buffer;
    g->input = p->input;
    g->queue = p->queue;
}

struct gui_vec2
gui_panel_group_end(struct gui_panel_layout *p, struct gui_panel_layout *g)
{
    struct gui_panel pan;
    struct gui_command_buffer *out;
    struct gui_rect clip;

    GUI_ASSERT(p);
    GUI_ASSERT(g);
    if (!p || !g) return gui_vec2(0,0);
    if (!p->valid) return gui_vec2(0,0);
    gui_zero(&pan, sizeof(pan));

    out = p->buffer;
    pan.x = g->x;
    pan.y = g->y;
    pan.w = g->width;
    pan.h = g->height;
    pan.flags = g->flags|GUI_PANEL_TAB;

    /* setup clipping rect to finalize group panel drawing back to parent */
    gui_unify(&clip, &p->clip, g->clip.x, g->clip.y, g->x + g->w, g->y + g->h);
    gui_command_buffer_push_scissor(out, clip);
    gui_panel_end(g, &pan);
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
gui_panel_shelf_begin(struct gui_panel_layout *parent, struct gui_panel_layout *shelf,
    const char *tabs[], gui_size size, gui_size active, struct gui_vec2 offset)
{
    const struct gui_config *config;
    struct gui_command_buffer *out;
    const struct gui_font *font;
    struct gui_vec2 item_padding;
    struct gui_vec2 panel_padding;

    struct gui_rect bounds;
    struct gui_rect *c;
    struct gui_panel panel;

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

    config = parent->config;
    out = parent->buffer;
    font = &config->font;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    panel_padding = gui_config_property(config, GUI_PROPERTY_PADDING);

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
        struct gui_button button;
        gui_float item_width;
        gui_size i;

        input = (parent->flags & GUI_PANEL_ROM) ? 0 : parent->input;
        item_width = (header_w - (gui_float)size) / (gui_float)size;
        button.border = 1;
        button.alignment = GUI_TEXT_CENTERED;
        button.rounding = 0;
        button.padding.x = item_padding.x;
        button.padding.y = item_padding.y;
        button.foreground = config->colors[GUI_COLOR_BORDER];

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
            if (gui_button_text(out, b, tabs[i], GUI_BUTTON_DEFAULT, &button,
                        input, &config->font)) active = i;
        }
    }
    bounds.y += header_h;
    bounds.h -= header_h;
    {
        /* setup fake panel to create a panel layout */
        gui_flags flags;
        flags = GUI_PANEL_BORDER|GUI_PANEL_TAB;
        if (parent->flags & GUI_PANEL_ROM)
            flags |= GUI_PANEL_ROM;
        gui_panel_init(&panel, bounds.x, bounds.y, bounds.w, bounds.h, flags,
            0, config, parent->input);

        panel.buffer = *parent->buffer;
        gui_panel_begin(shelf, &panel);
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
    shelf->config = parent->config;
    shelf->buffer = parent->buffer;
    shelf->input = parent->input;
    shelf->queue = parent->queue;
    return active;
}

struct gui_vec2
gui_panel_shelf_end(struct gui_panel_layout *p, struct gui_panel_layout *s)
{
    struct gui_command_buffer *out;
    struct gui_rect clip;
    struct gui_panel pan;

    GUI_ASSERT(p);
    GUI_ASSERT(s);
    if (!p || !s) return gui_vec2(0,0);
    if (!p->valid) return gui_vec2(0,0);
    gui_zero(&pan, sizeof(pan));

    out = p->buffer;
    pan.x = s->x; pan.y = s->y;
    pan.w = s->w; pan.h = s->h;
    pan.flags = s->flags|GUI_PANEL_TAB;

    gui_unify(&clip, &p->clip, s->clip.x, s->clip.y, s->x + s->w, s->y + s->h);
    gui_command_buffer_push_scissor(out, clip);
    gui_panel_end(s, &pan);
    gui_command_buffer_push_scissor(out, p->clip);
    return pan.offset;
}

/*
 * ==============================================================
 *
 *                          Layout
 *
 * ===============================================================
 */
void gui_layout_begin(struct gui_layout *layout, struct gui_rect bounds,
    gui_state state)
{
    GUI_ASSERT(layout);
    if (!layout) return;
    gui_zero(layout, sizeof(*layout));
    layout->active = state;
    layout->bounds = bounds;
}

void
gui_layout_slot(struct gui_layout *layout, enum gui_layout_slot_index slot,
    gui_float ratio, enum gui_layout_format format, gui_size count)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(count);
    GUI_ASSERT(slot >= GUI_SLOT_TOP && slot < GUI_SLOT_MAX);
    if (!layout || !count) return;

    layout->slots[slot].capacity = count;
    layout->slots[slot].format = format;
    layout->slots[slot].value = GUI_SATURATE(ratio);
    layout->slots[slot].state = GUI_UNLOCKED;
}

void
gui_layout_slot_locked(struct gui_layout *layout, enum gui_layout_slot_index slot,
    gui_float ratio, enum gui_layout_format format, gui_size count)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(count);
    GUI_ASSERT(slot >= GUI_SLOT_TOP && slot < GUI_SLOT_MAX);
    if (!layout || !count) return;
    gui_layout_slot(layout, slot, ratio, format, count);
    layout->slots[slot].state = GUI_LOCKED;
}

void
gui_layout_end(struct gui_layout *layout)
{
    struct gui_layout_slot *top, *bottom;
    struct gui_layout_slot *left, *right;
    gui_float centerh, centerv;

    GUI_ASSERT(layout);
    if (!layout) return;

    top = &layout->slots[GUI_SLOT_TOP];
    bottom = &layout->slots[GUI_SLOT_BOTTOM];
    left = &layout->slots[GUI_SLOT_LEFT];
    right = &layout->slots[GUI_SLOT_RIGHT];

    /* calucalate the slot ratio size */
    centerh = MAX(0.0f, 1.0f - (left->value + right->value));
    centerv = MAX(0.0f, 1.0f - (top->value + bottom->value));
    layout->slots[GUI_SLOT_CENTER].ratio = gui_vec2(centerh, centerv);
    layout->slots[GUI_SLOT_TOP].ratio = gui_vec2(1.0f, top->value);
    layout->slots[GUI_SLOT_LEFT].ratio = gui_vec2(left->value, centerv);
    layout->slots[GUI_SLOT_BOTTOM].ratio = gui_vec2(1.0f, bottom->value);
    layout->slots[GUI_SLOT_RIGHT].ratio = gui_vec2(right->value, centerv);

    /* calucalate the slot screen position ratio */
    layout->slots[GUI_SLOT_TOP].offset = gui_vec2(0.0f, 0.0f);
    layout->slots[GUI_SLOT_LEFT].offset = gui_vec2(0.0f, top->value);
    layout->slots[GUI_SLOT_BOTTOM].offset = gui_vec2(0.0f, top->value + centerv);
    layout->slots[GUI_SLOT_RIGHT].offset = gui_vec2(left->value + centerh, top->value);
    layout->slots[GUI_SLOT_CENTER].offset = gui_vec2(left->value, top->value);
}

void
gui_layout_load(struct gui_layout *child, struct gui_layout *parent,
    enum gui_layout_slot_index slot, gui_size index)
{
    GUI_ASSERT(child);
    GUI_ASSERT(parent);
    if (!child || !parent) return;
    gui_layout_slot_panel_bounds(&child->bounds, parent, slot, index);
    child->active = parent->active;
}

void
gui_layout_set_size(struct gui_layout *layout, gui_size width, gui_size height)
{
    GUI_ASSERT(layout);
    if (!layout) return;
    layout->bounds.w = (gui_float)width;
    layout->bounds.h = (gui_float)height;
}

void
gui_layout_set_pos(struct gui_layout *layout, gui_size x, gui_size y)
{
    GUI_ASSERT(layout);
    if (!layout) return;
    layout->bounds.x = (gui_float)x;
    layout->bounds.y = (gui_float)y;
}

void
gui_layout_set_state(struct gui_layout *layout, gui_state state)
{
    GUI_ASSERT(layout);
    if (!layout) return;
    layout->active = state;
}

void
gui_layout_slot_bounds(struct gui_rect *bounds, const struct gui_layout* layout,
    enum gui_layout_slot_index slot)
{
    const struct gui_layout_slot *s;
    GUI_ASSERT(bounds);
    GUI_ASSERT(layout);
    if (!bounds || !layout || slot >= GUI_SLOT_MAX) return;

    s = &layout->slots[slot];
    bounds->x = layout->bounds.x + s->offset.x * (gui_float)layout->bounds.w;
    bounds->y = layout->bounds.y + s->offset.y * (gui_float)layout->bounds.h;
    bounds->w = s->ratio.x * (gui_float)layout->bounds.w;
    bounds->h = s->ratio.y * (gui_float)layout->bounds.h;
}

void
gui_layout_slot_panel_bounds(struct gui_rect *bounds, const struct gui_layout *layout,
    enum gui_layout_slot_index slot, gui_size index)
{
    /* calculate the bounds of the panel */
    struct gui_rect slot_bounds = {0,0,0,0};
    const struct gui_layout_slot *s;

    GUI_ASSERT(bounds);
    GUI_ASSERT(layout);
    if (!bounds || !layout) return;

    s = &layout->slots[slot];
    GUI_ASSERT(index < s->capacity);
    if (index >= s->capacity) {
        gui_zero(bounds, sizeof(*bounds));
        return;
    }

    gui_layout_slot_bounds(&slot_bounds, layout, slot);
    if (s->format == GUI_LAYOUT_HORIZONTAL) {
        bounds->h = slot_bounds.h;
        bounds->y = slot_bounds.y;
        bounds->x = slot_bounds.x + (gui_float)index * bounds->w;
        bounds->w = slot_bounds.w / (gui_float)s->capacity;
    } else {
        bounds->x = slot_bounds.x;
        bounds->w = slot_bounds.w;
        bounds->h = slot_bounds.h / (gui_float)s->capacity;
        bounds->y = slot_bounds.y + (gui_float)index * bounds->h;
    }
}

