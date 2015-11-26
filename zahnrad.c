/*
    Copyright (c) 2015 Micha Mettke

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1.  The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software
        in a product, an acknowledgment in the product documentation would be
        appreciated but is not required.
    2.  Altered source versions must be plainly marked as such, and must not be
        misrepresented as being the original software.
    3.  This notice may not be removed or altered from any source distribution.
*/
#include "zahnrad.h"

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#endif
#ifndef CLAMP
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#endif

/* ==============================================================
 *                          MATH
 * =============================================================== */
#define ZR_PI 3.141592654f
#define ZR_PI2 (ZR_PI * 2.0f)
#define ZR_4_DIV_PI (1.27323954f)
#define ZR_4_DIV_PI_SQRT (0.405284735f)
#define ZR_UTF_INVALID 0xFFFD
#define ZR_MAX_NUMBER_BUFFER 64
#define ZR_MAX_FLOAT_PRECISION 2

#define ZR_UNUSED(x) ((void)(x))
#define ZR_LERP(a, b, t) ((a) + ((b) - (a)) * (t))
#define ZR_SATURATE(x) (MAX(0, MIN(1.0f, x)))
#define ZR_LEN(a) (sizeof(a)/sizeof(a)[0])
#define ZR_ABS(a) (((a) < 0) ? -(a) : (a))
#define ZR_BETWEEN(x, a, b) ((a) <= (x) && (x) <= (b))
#define ZR_INBOX(px, py, x, y, w, h) (ZR_BETWEEN(px,x,x+w) && ZR_BETWEEN(py,y,y+h))
#define ZR_INTERSECT(x0, y0, w0, h0, x1, y1, w1, h1) \
    (!(((x1 > (x0 + w0)) || ((x1 + w1) < x0) || (y1 > (y0 + h0)) || (y1 + h1) < y0)))
#define ZR_CONTAINS(x, y, w, h, bx, by, bw, bh)\
    (ZR_INBOX(x,y, bx, by, bw, bh) && ZR_INBOX(x+w,y+h, bx, by, bw, bh))

#define zr_vec2_mov(to,from) (to).x = (from).x, (to).y = (from).y
#define zr_vec2_sub(a, b) zr_vec2((a).x - (b).x, (a).y - (b).y)
#define zr_vec2_add(a, b) zr_vec2((a).x + (b).x, (a).y + (b).y)
#define zr_vec2_len_sqr(a) ((a).x*(a).x+(a).y*(a).y)
#define zr_vec2_inv_len(a) zr_inv_sqrt(zr_vec2_len_sqr(a))
#define zr_vec2_muls(a, t) zr_vec2((a).x * (t), (a).y * (t))
#define zr_vec2_lerp(a, b, t) zr_vec2(ZR_LERP((a).x, (b).x, t), ZR_LERP((a).y, (b).y, t))

enum zr_tree_node_symbol {ZR_TREE_NODE_BULLET, ZR_TREE_NODE_TRIANGLE};
static const struct zr_rect zr_null_rect = {-8192.0f, -8192.0f, 16384, 16384};
static const double double_PRECISION = 0.00000000000001;

/* ==============================================================
 *                          ALIGNMENT
 * =============================================================== */
/* Pointer to Integer type conversion for pointer alignment */
#if defined(__PTRDIFF_TYPE__) /* This case should work for GCC*/
# define ZR_UINT_TO_PTR(x) ((void*)(__PTRDIFF_TYPE__)(x))
# define ZR_PTR_TO_UINT(x) ((zr_size)(__PTRDIFF_TYPE__)(x))
#elif !defined(__GNUC__) /* works for compilers other than LLVM */
# define ZR_UINT_TO_PTR(x) ((void*)&((char*)0)[x])
# define ZR_PTR_TO_UINT(x) ((zr_size)(((char*)x)-(char*)0))
#elif defined(ZR_USE_FIXED_TYPES) /* used if we have <stdint.h> */
# define ZR_UINT_TO_PTR(x) ((void*)(uintptr_t)(x))
# define ZR_PTR_TO_UINT(x) ((uintptr_t)(x))
#else /* generates warning but works */
# define ZR_UINT_TO_PTR(x) ((void*)(x))
# define ZR_PTR_TO_UINT(x) ((zr_size)(x))
#endif

#define ZR_MIN(a,b) (((a)<(b))?(a):(b))
#define ZR_MAX(a,b) (((a)>(b))?(a):(b))
#define ZR_ALIGN(x, mask) ((x) + (mask-1)) & ~(mask-1)
#define ZR_ALIGN_PTR(x, mask)\
    (ZR_UINT_TO_PTR((ZR_PTR_TO_UINT((zr_byte*)(x) + (mask-1)) & ~(mask-1))))
#define ZR_ALIGN_PTR_BACK(x, mask)\
    (ZR_UINT_TO_PTR((ZR_PTR_TO_UINT((zr_byte*)(x)) & ~(mask-1))))

#define ZR_OFFSETOF(st, m) ((zr_size)(&((st *)0)->m))
#define ZR_CONTAINER_OF_CONST(ptr, type, member)\
    ((const type*)((const void*)((const zr_byte*)ptr - ZR_OFFSETOF(type, member))))
#define ZR_CONTAINER_OF(ptr, type, member)\
    ((type*)((void*)((zr_byte*)ptr - ZR_OFFSETOF(type, member))))

#ifdef __cplusplus
template<typename T> struct zr_alignof;
template<typename T, int size_diff> struct zr_helper{enum {value = size_diff};};
template<typename T> struct zr_helper<T,0>{enum {value = zr_alignof<T>::value};};
template<typename T> struct zr_alignof{struct Big {T x; char c;}; enum {
    diff = sizeof(Big) - sizeof(T), value = zr_helper<Big, diff>::value};};
#define ZR_ALIGNOF(t) (zr_alignof<t>::value);
#else
#define ZR_ALIGNOF(t) ((char*)(&((struct {char c; t _h;}*)0)->_h) - (char*)0)
#endif
/*
 * ==============================================================
 *
 *                          MATH
 *
 * ===============================================================
 */
static uint32_t
zr_round_up_pow2(uint32_t v)
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

static float
zr_inv_sqrt(float number)
{
    float x2;
    const float threehalfs = 1.5f;
    union {uint32_t i; float f;} conv;
    conv.f = number;
    x2 = number * 0.5f;
    conv.i = 0x5f375A84 - (conv.i >> 1);
    conv.f = conv.f * (threehalfs - (x2 * conv.f * conv.f));
    return conv.f;
}

struct zr_rect
zr_get_null_rect(void)
{
    return zr_null_rect;
}

struct zr_rect
zr_rect(float x, float y, float w, float h)
{
    struct zr_rect r;
    r.x = x, r.y = y;
    r.w = w, r.h = h;
    return r;
}

static struct zr_rect
zr_shrink_rect(struct zr_rect r, float amount)
{
    struct zr_rect res;
    r.w = MAX(r.w, 2 * amount);
    r.h = MAX(r.h, 2 * amount);
    res.x = r.x + amount;
    res.y = r.y + amount;
    res.w = r.w - 2 * amount;
    res.h = r.h - 2 * amount;
    return res;
}

static struct zr_rect
zr_pad_rect(struct zr_rect r, struct zr_vec2 pad)
{
    r.w = MAX(r.w, 2 * pad.x);
    r.h = MAX(r.h, 2 * pad.y);
    r.x += pad.x; r.y += pad.y;
    r.w -= 2 * pad.x;
    r.h -= 2 * pad.y;
    return r;
}

struct zr_vec2
zr_vec2(float x, float y)
{
    struct zr_vec2 ret;
    ret.x = x; ret.y = y;
    return ret;
}
/*
 * ==============================================================
 *
 *                          STANDART
 *
 * ===============================================================
 */
static void*
zr_memcopy(void *dst0, const void *src0, zr_size length)
{
    zr_ptr t;
    typedef int word;
    char *dst = dst0;
    const char *src = src0;
    if (length == 0 || dst == src)
        goto done;

    #define wsize sizeof(word)
    #define wmask (wsize-1)
    #define TLOOP(s) if (t) TLOOP1(s)
    #define TLOOP1(s) do { s; } while (--t)

    if (dst < src) {
        t = (zr_ptr)src; /* only need low bits */
        if ((t | (zr_ptr)dst) & wmask) {
            if ((t ^ (zr_ptr)dst) & wmask || length < wsize)
                t = length;
            else
                t = wsize - (t & wmask);
            length -= t;
            TLOOP1(*dst++ = *src++);
        }
        t = length / wsize;
        TLOOP(*(word*)(void*)dst = *(const word*)(const void*)src; src += wsize; dst += wsize);
        t = length & wmask;
        TLOOP(*dst++ = *src++);
    } else {
        src += length;
        dst += length;
        t = (zr_ptr)src;
        if ((t | (zr_ptr)dst) & wmask) {
            if ((t ^ (zr_ptr)dst) & wmask || length <= wsize)
                t = length;
            else
                t &= wmask;
            length -= t;
            TLOOP1(*--dst = *--src);
        }
        t = length / wsize;
        TLOOP(src -= wsize; dst -= wsize; *(word*)(void*)dst = *(const word*)(const void*)src);
        t = length & wmask;
        TLOOP(*--dst = *--src);
    }
    #undef wsize
    #undef wmask
    #undef TLOOP
    #undef TLOOP1
done:
    return (dst0);
}

static void
zr_zero(void *dst, zr_size size)
{
    zr_size i;
    char *d = (char*)dst;
    ZR_ASSERT(dst);
    for (i = 0; i < size; ++i) d[i] = 0;
}

static zr_size
zr_strsiz(const char *str)
{
    zr_size siz = 0;
    ZR_ASSERT(str);
    while (str && *str++ != '\0') siz++;
    return siz;
}

static int
zr_strtoi(int *number, const char *buffer, zr_size len)
{
    zr_size i;
    ZR_ASSERT(number);
    ZR_ASSERT(buffer);
    if (!number || !buffer) return 0;
    *number = 0;
    for (i = 0; i < len; ++i)
        *number = *number * 10 + (buffer[i] - '0');
    return 1;
}

static int
zr_strtof(float *number, const char *buffer)
{
    int i;
    float m;
    int div;
    int pow;
    const char *p = buffer;
    float floatvalue = 0;

    ZR_ASSERT(number);
    ZR_ASSERT(buffer);
    if (!number || !buffer) return 0;
    *number = 0;

    while( *p && *p != '.' && *p != 'e' ) {
        floatvalue = floatvalue * 10.0f + (float) (*p - '0');
        p++;
    }

    if ( *p == '.' ) {
        p++;
        for(m = 0.1f; *p && *p != 'e'; p++ ) {
            floatvalue = floatvalue + (float) (*p - '0') * m;
            m *= 0.1f;
        }
    }
    if ( *p == 'e' ) {
        p++;
        if ( *p == '-' ) {
            div = zr_true;
            p++;
        } else if ( *p == '+' ) {
            div = zr_false;
            p++;
        } else div = zr_false;

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

static zr_size
zr_itos(char *buffer, int num)
{
    static const char digit[] = "0123456789";
    int shifter;
    zr_size len = 0;
    char *p = buffer;

    ZR_ASSERT(buffer);
    if (!buffer)
        return 0;

    if (num < 0) {
        num = ZR_ABS(num);
        *p++ = '-';
    }
    shifter = num;

    do {
        ++p;
        shifter = shifter/10;
    } while (shifter);
    *p = '\0';

    len = (zr_size)(p - buffer);
    do {
        *--p = digit[num % 10];
        num = num / 10;
    } while (num);
    return len;
}

static double
zr_pow(double x, int n)
{
    /*  check the sign of n */
    double r = 1;
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

static int32_t
zr_isinf(double x)
{
    union {uint64_t u; double f;} ieee754;
    ieee754.f = x;
    return ( (unsigned)(ieee754.u >> 32) & 0x7fffffff ) == 0x7ff00000 &&
           ( (unsigned)ieee754.u == 0 );
}

static int32_t
zr_isnan(double x)
{
    union {uint64_t u; double f;} ieee754;
    ieee754.f = x;
    return ( (unsigned)(ieee754.u >> 32) & 0x7fffffff ) +
           ( (unsigned)ieee754.u != 0 ) > 0x7ff00000;
}

static double
zr_floor(double x)
{
    return (double)((int)x - ((x < 0.0) ? 1 : 0));
}

static int
zr_log10(double n)
{
    int neg;
    int ret;
    int exp = 0;

    neg = (n < 0) ? 1 : 0;
    ret = (neg) ? (int)-n : (int)n;
    while ((ret / 10) > 0) {
        ret /= 10;
        exp++;
    }
    if (neg) exp = -exp;
    return exp;
}

static zr_size
zr_dtos(char *s, double n)
{
    int useExp;
    int digit, m, m1;
    char *c = s;
    int neg;

    if (zr_isnan(n)) {
        s[0] = 'n'; s[1] = 'a'; s[2] = 'n'; s[3] = '\0';
        return 3;
    } else if (zr_isinf(n)) {
        s[0] = 'i'; s[1] = 'n'; s[2] = 'f'; s[3] = '\0';
        return 3;
    } else if (n == 0.0) {
        s[0] = '0'; s[1] = '\0';
        return 1;
    }

    neg = (n < 0);
    if (neg) n = -n;

    /* calculate magnitude */
    m = zr_log10(n);
    useExp = (m >= 14 || (neg && m >= 9) || m <= -9);
    if (neg) *(c++) = '-';

    /* set up for scientific notation */
    if (useExp) {
        if (m < 0)
           m -= 1;
        n = n / zr_pow(10.0, m);
        m1 = m;
        m = 0;
    }
    if (m < 1.0) {
        m = 0;
    }

    /* convert the number */
    while (n > double_PRECISION || m >= 0) {
        double tmp;
        double weight = zr_pow(10.0, m);
        if (weight > 0 && !zr_isinf(weight)) {
            double t = (double)n / weight;
            tmp = zr_floor(t);
            digit = (int)tmp;
            n -= (digit * weight);
            *(c++) = (char)('0' + (char)digit);
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
            *(c++) = (char)('0' + (char)(m1 % 10));
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
    return (zr_size)(c - s);
}

/*
 * ==============================================================
 *
 *                          COLOR
 *
 * ===============================================================
 */
struct zr_color
zr_rgba(zr_byte r, zr_byte g, zr_byte b, zr_byte a)
{
    struct zr_color ret;
    ret.r = r; ret.g = g;
    ret.b = b; ret.a = a;
    return ret;
}

struct zr_color
zr_rgb(zr_byte r, zr_byte g, zr_byte b)
{
    struct zr_color ret;
    ret.r = r; ret.g = g;
    ret.b = b; ret.a = 255;
    return ret;
}

struct zr_color
zr_rgba32(uint32_t in)
{
    struct zr_color ret;
    ret.r = (in & 0xFF);
    ret.g = ((in >> 8) & 0xFF);
    ret.b = ((in >> 16) & 0xFF);
    ret.a = (zr_byte)((in >> 24) & 0xFF);
    return ret;
}

struct zr_color
zr_rgba_f(float r, float g, float b, float a)
{
    struct zr_color ret;
    ret.r = (zr_byte)(ZR_SATURATE(r) * 255.0f);
    ret.g = (zr_byte)(ZR_SATURATE(g) * 255.0f);
    ret.b = (zr_byte)(ZR_SATURATE(b) * 255.0f);
    ret.a = (zr_byte)(ZR_SATURATE(a) * 255.0f);
    return ret;
}

struct zr_color
zr_rgb_f(float r, float g, float b)
{
    struct zr_color ret;
    ret.r = (zr_byte)(ZR_SATURATE(r) * 255.0f);
    ret.g = (zr_byte)(ZR_SATURATE(g) * 255.0f);
    ret.b = (zr_byte)(ZR_SATURATE(b) * 255.0f);
    ret.a = 255;
    return ret;
}

struct zr_color
zr_hsv(zr_byte h, zr_byte s, zr_byte v)
{
    return zr_hsva(h, s, v, 255);
}

struct zr_color
zr_hsv_f(float h, float s, float v)
{
    return zr_hsva_f(h, s, v, 1.0f);
}

struct zr_color
zr_hsva(zr_byte h, zr_byte s, zr_byte v, zr_byte a)
{
    float hf = (float)h / 255.0f;
    float sf = (float)s / 255.0f;
    float vf = (float)v / 255.0f;
    float af = (float)a / 255.0f;
    return zr_hsva_f(hf, sf, vf, af);
}

struct zr_color
zr_hsva_f(float h, float s, float v, float a)
{
    struct zr_colorf {float r,g,b,a;} out;
    float hh, p, q, t, ff;
    uint32_t i;

    if (s <= 0.0f) {
        out.r = v; out.g = v; out.b = v;
        return zr_rgb_f(out.r, out.g, out.b);
    }

    hh = h;
    if (hh >= 360.0f) hh = 0;
    hh /= 60.0f;
    i = (uint32_t)hh;
    ff = hh - (float)i;
    p = v * (1.0f - s);
    q = v * (1.0f - (s * ff));
    t = v * (1.0f - (s * (1.0f - ff)));

    switch (i) {
    case 0:
        out.r = v;
        out.g = t;
        out.b = p;
        break;
    case 1:
        out.r = q;
        out.g = v;
        out.b = p;
        break;
    case 2:
        out.r = p;
        out.g = v;
        out.b = t;
        break;
    case 3:
        out.r = p;
        out.g = q;
        out.b = v;
        break;
    case 4:
        out.r = t;
        out.g = p;
        out.b = v;
        break;
    case 5:
    default:
        out.r = v;
        out.g = p;
        out.b = q;
        break;
    }
    return zr_rgba_f(out.r, out.g, out.b, a);
}

uint32_t
zr_color32(struct zr_color in)
{
    uint32_t out = (uint32_t)in.r;
    out |= ((uint32_t)in.g << 8);
    out |= ((uint32_t)in.b << 16);
    out |= ((uint32_t)in.a << 24);
    return out;
}

void
zr_colorf(float *r, float *g, float *b, float *a, struct zr_color in)
{
    static const float s = 1.0f/255.0f;
    *r = (float)in.r * s;
    *g = (float)in.g * s;
    *b = (float)in.b * s;
    *a = (float)in.a * s;
}

void
zr_color_hsv_f(float *out_h, float *out_s, float *out_v, struct zr_color in)
{
    float a;
    zr_color_hsva_f(out_h, out_s, out_v, &a, in);
}

void
zr_color_hsva_f(float *out_h, float *out_s,
    float *out_v, float *out_a, struct zr_color in)
{
    float chroma;
    float K = 0.0f;
    float r,g,b,a;

    zr_colorf(&r,&g,&b,&a, in);
    if (g < b) {
        const float t = g; g = b; b = t;
        K = -1.f;
    }
    if (a < g) {
        const float t = r; r = g; g = t;
        K = -2.f/6.0f - K;
    }
    chroma = r - ((g < b) ? g: b);
    *out_h = ZR_ABS(K + (g - b)/(6.0f * chroma + 1e-20f));
    *out_s = chroma / (r + 1e-20f);
    *out_v = r;
    *out_a = (float)in.a / 255.0f;
}

void
zr_color_hsva(int *out_h, int *out_s, int *out_v,
                int *out_a, struct zr_color in)
{
    float h,s,v,a;
    zr_color_hsva_f(&h, &s, &v, &a, in);
    *out_h = (zr_byte)(h * 255.0f);
    *out_s = (zr_byte)(s * 255.0f);
    *out_v = (zr_byte)(v * 255.0f);
    *out_a = (zr_byte)(a * 255.0f);
}

void
zr_color_hsv(int *out_h, int *out_s, int *out_v, struct zr_color in)
{
    int a;
    zr_color_hsva(out_h, out_s, out_v, &a, in);
}
/*
 * ==============================================================
 *
 *                          IMAGE
 *
 * ===============================================================
 */
zr_handle
zr_handle_ptr(void *ptr)
{
    zr_handle handle;
    handle.ptr = ptr;
    return handle;
}

zr_handle
zr_handle_id(int id)
{
    zr_handle handle;
    handle.id = id;
    return handle;
}

struct zr_image
zr_subimage_ptr(void *ptr, unsigned short w, unsigned short h, struct zr_rect r)
{
    struct zr_image s;
    s.handle.ptr = ptr;
    s.w = w; s.h = h;
    s.region[0] = (unsigned short)r.x;
    s.region[1] = (unsigned short)r.y;
    s.region[2] = (unsigned short)r.w;
    s.region[3] = (unsigned short)r.h;
    return s;
}

struct zr_image
zr_subimage_id(int id, unsigned short w, unsigned short h, struct zr_rect r)
{
    struct zr_image s;
    s.handle.id = id;
    s.w = w; s.h = h;
    s.region[0] = (unsigned short)r.x;
    s.region[1] = (unsigned short)r.y;
    s.region[2] = (unsigned short)r.w;
    s.region[3] = (unsigned short)r.h;
    return s;
}

struct zr_image
zr_image_ptr(void *ptr)
{
    struct zr_image s;
    ZR_ASSERT(ptr);
    s.handle.ptr = ptr;
    s.w = 0; s.h = 0;
    s.region[0] = 0;
    s.region[1] = 0;
    s.region[2] = 0;
    s.region[3] = 0;
    return s;
}

struct zr_image
zr_image_id(int id)
{
    struct zr_image s;
    s.handle.id = id;
    s.w = 0; s.h = 0;
    s.region[0] = 0;
    s.region[1] = 0;
    s.region[2] = 0;
    s.region[3] = 0;
    return s;
}

int
zr_image_is_subimage(const struct zr_image* img)
{
    ZR_ASSERT(img);
    return (img->w == 0 && img->h == 0);
}

static void
zr_unify(struct zr_rect *clip, const struct zr_rect *a, float x0, float y0,
    float x1, float y1)
{
    ZR_ASSERT(a);
    ZR_ASSERT(clip);
    clip->x = MAX(a->x, x0);
    clip->y = MAX(a->y, y0);
    clip->w = MIN(a->x + a->w, x1) - clip->x;
    clip->h = MIN(a->y + a->h, y1) - clip->y;
    clip->w = MAX(0, clip->w);
    clip->h = MAX(0, clip->h);
}

static void
zr_triangle_from_direction(struct zr_vec2 *result, struct zr_rect r,
    float pad_x, float pad_y, enum zr_heading direction)
{
    float w_half, h_half;
    ZR_ASSERT(result);

    r.w = MAX(2 * pad_x, r.w);
    r.h = MAX(2 * pad_y, r.h);
    r.w = r.w - 2 * pad_x;
    r.h = r.h - 2 * pad_y;

    r.x = r.x + pad_x;
    r.y = r.y + pad_y;

    w_half = r.w / 2.0f;
    h_half = r.h / 2.0f;

    if (direction == ZR_UP) {
        result[0] = zr_vec2(r.x + w_half, r.y);
        result[1] = zr_vec2(r.x + r.w, r.y + r.h);
        result[2] = zr_vec2(r.x, r.y + r.h);
    } else if (direction == ZR_RIGHT) {
        result[0] = zr_vec2(r.x, r.y);
        result[1] = zr_vec2(r.x + r.w, r.y + h_half);
        result[2] = zr_vec2(r.x, r.y + r.h);
    } else if (direction == ZR_DOWN) {
        result[0] = zr_vec2(r.x, r.y);
        result[1] = zr_vec2(r.x + r.w, r.y);
        result[2] = zr_vec2(r.x + w_half, r.y + r.h);
    } else {
        result[0] = zr_vec2(r.x, r.y + h_half);
        result[1] = zr_vec2(r.x + r.w, r.y);
        result[2] = zr_vec2(r.x + r.w, r.y + r.h);
    }
}

static zr_size
zr_string_float_limit(char *string, int prec)
{
    int dot = 0;
    char *c = string;
    while (*c) {
        if (*c == '.') {
            dot = 1;
            c++;
            continue;
        }
        if (dot == (prec+1)) {
            *c = 0;
            break;
        }
        if (dot > 0) dot++;
        c++;
    }
    return (zr_size)(c - string);
}
/* ==============================================================
 *
 *                          UTF-8
 *
 * ===============================================================*/
static const zr_byte zr_utfbyte[ZR_UTF_SIZE+1] = {0x80, 0, 0xC0, 0xE0, 0xF0};
static const zr_byte zr_utfmask[ZR_UTF_SIZE+1] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
static const long zr_utfmin[ZR_UTF_SIZE+1] = {0, 0, 0x80, 0x800, 0x100000};
static const long zr_utfmax[ZR_UTF_SIZE+1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};

static zr_size
zr_utf_validate(zr_rune *u, zr_size i)
{
    ZR_ASSERT(u);
    if (!u) return 0;
    if (!ZR_BETWEEN(*u, zr_utfmin[i], zr_utfmax[i]) ||
         ZR_BETWEEN(*u, 0xD800, 0xDFFF))
            *u = ZR_UTF_INVALID;
    for (i = 1; *u > zr_utfmax[i]; ++i);
    return i;
}

static zr_rune
zr_utf_decode_byte(char c, zr_size *i)
{
    ZR_ASSERT(i);
    if (!i) return 0;
    for(*i = 0; *i < ZR_LEN(zr_utfmask); ++(*i)) {
        if (((zr_byte)c & zr_utfmask[*i]) == zr_utfbyte[*i])
            return (zr_byte)(c & ~zr_utfmask[*i]);
    }
    return 0;
}

zr_size
zr_utf_decode(const char *c, zr_rune *u, zr_size clen)
{
    zr_size i, j, len, type=0;
    zr_rune udecoded;

    ZR_ASSERT(c);
    ZR_ASSERT(u);

    if (!c || !u) return 0;
    if (!clen) return 0;
    *u = ZR_UTF_INVALID;

    udecoded = zr_utf_decode_byte(c[0], &len);
    if (!ZR_BETWEEN(len, 1, ZR_UTF_SIZE))
        return 1;

    for (i = 1, j = 1; i < clen && j < len; ++i, ++j) {
        udecoded = (udecoded << 6) | zr_utf_decode_byte(c[i], &type);
        if (type != 0)
            return j;
    }
    if (j < len)
        return 0;
    *u = udecoded;
    zr_utf_validate(u, len);
    return len;
}

static char
zr_utf_encode_byte(zr_rune u, zr_size i)
{
    return (char)((zr_utfbyte[i]) | ((zr_byte)u & ~zr_utfmask[i]));
}

zr_size
zr_utf_encode(zr_rune u, char *c, zr_size clen)
{
    zr_size len, i;
    len = zr_utf_validate(&u, 0);
    if (clen < len || !len || len > ZR_UTF_SIZE)
        return 0;

    for (i = len - 1; i != 0; --i) {
        c[i] = zr_utf_encode_byte(u, 0);
        u >>= 6;
    }
    c[0] = zr_utf_encode_byte(u, len);
    return len;
}

zr_size
zr_utf_len(const char *str, zr_size len)
{
    const char *text;
    zr_size glyphes = 0;
    zr_size text_len;
    zr_size glyph_len;
    zr_size src_len = 0;
    zr_rune unicode;

    ZR_ASSERT(str);
    if (!str || !len) return 0;

    text = str;
    text_len = len;
    glyph_len = zr_utf_decode(text, &unicode, text_len);
    while (glyph_len && src_len < len) {
        glyphes++;
        src_len = src_len + glyph_len;
        glyph_len = zr_utf_decode(text + src_len, &unicode, text_len - src_len);
    }
    return glyphes;
}

/*
 * ==============================================================
 *
 *                          USER FONT
 *
 * ===============================================================
 */
static zr_size
zr_user_font_glyph_index_at_pos(const struct zr_user_font *font, const char *text,
    zr_size text_len, float xoff)
{
    zr_rune unicode;
    zr_size glyph_offset = 0;
    zr_size glyph_len = zr_utf_decode(text, &unicode, text_len);
    zr_size text_width = font->width(font->userdata, font->height, text, glyph_len);
    zr_size src_len = glyph_len;

    while (text_len && glyph_len) {
        if (text_width >= xoff)
            return glyph_offset;
        glyph_offset++;
        text_len -= glyph_len;
        glyph_len = zr_utf_decode(text + src_len, &unicode, text_len);
        src_len += glyph_len;
        text_width = font->width(font->userdata, font->height, text, src_len);
    }
    return glyph_offset;
}

static zr_size
zr_use_font_glyph_clamp(const struct zr_user_font *font, const char *text,
    zr_size text_len, float space, zr_size *glyphes, float *text_width)
{
    zr_size len = 0;
    zr_size glyph_len;
    float width = 0;
    float last_width = 0;
    zr_rune unicode;
    zr_size g = 0;

    glyph_len = zr_utf_decode(text, &unicode, text_len);
    while (glyph_len && (width < space) && (len < text_len)) {
        zr_size s;
        len += glyph_len;
        s = font->width(font->userdata, font->height, text, len);
        last_width = width;
        width = (float)s;
        glyph_len = zr_utf_decode(&text[len], &unicode, text_len - len);
        g++;
    }

    *glyphes = g;
    *text_width = last_width;
    return len;
}

static zr_size
zr_user_font_glyphes_fitting_in_space(const struct zr_user_font *font, const char *text,
    zr_size text_len, float space, zr_size *glyphes, float *text_width)
{
    zr_size glyph_len;
    float width = 0;
    float last_width = 0;
    zr_rune unicode;
    zr_size offset = 0;
    zr_size g = 0;
    zr_size s;

    glyph_len = zr_utf_decode(text, &unicode, text_len);
    s = font->width(font->userdata, font->height, text, glyph_len);
    width = last_width = (float)s;
    while ((width < space) && text_len) {
        g++;
        offset += glyph_len;
        text_len -= glyph_len;
        last_width = width;
        glyph_len = zr_utf_decode(&text[offset], &unicode, text_len);
        s = font->width(font->userdata, font->height, &text[offset], glyph_len);
        width += (float)s;
    }

    *glyphes = g;
    *text_width = last_width;
    return offset;
}
/* ==============================================================
 *
 *                          Input
 *
 * ===============================================================*/
void
zr_input_begin(struct zr_input *in)
{
    zr_size i;
    ZR_ASSERT(in);
    if (!in) return;

    for (i = 0; i < ZR_BUTTON_MAX; ++i)
        in->mouse.buttons[i].clicked = 0;
    in->keyboard.text_len = 0;
    in->mouse.scroll_delta = 0;
    zr_vec2_mov(in->mouse.prev, in->mouse.pos);
    for (i = 0; i < ZR_KEY_MAX; i++)
        in->keyboard.keys[i].clicked = 0;
}

void
zr_input_motion(struct zr_input *in, int x, int y)
{
    ZR_ASSERT(in);
    if (!in) return;
    in->mouse.pos.x = (float)x;
    in->mouse.pos.y = (float)y;
}

void
zr_input_key(struct zr_input *in, enum zr_keys key, int down)
{
    ZR_ASSERT(in);
    if (!in) return;
    if (in->keyboard.keys[key].down == down) return;
    in->keyboard.keys[key].down = down;
    in->keyboard.keys[key].clicked++;
}

void
zr_input_button(struct zr_input *in, enum zr_buttons id, int x, int y, int down)
{
    struct zr_mouse_button *btn;
    ZR_ASSERT(in);
    if (!in) return;
    if (in->mouse.buttons[id].down == down) return;
    btn = &in->mouse.buttons[id];
    btn->clicked_pos.x = (float)x;
    btn->clicked_pos.y = (float)y;
    btn->down = down;
    btn->clicked++;
}

void
zr_input_scroll(struct zr_input *in, float y)
{
    ZR_ASSERT(in);
    if (!in) return;
    in->mouse.scroll_delta += y;
}

void
zr_input_glyph(struct zr_input *in, const zr_glyph glyph)
{
    zr_size len = 0;
    zr_rune unicode;
    ZR_ASSERT(in);
    if (!in) return;

    len = zr_utf_decode(glyph, &unicode, ZR_UTF_SIZE);
    if (len && ((in->keyboard.text_len + len) < ZR_INPUT_MAX)) {
        zr_utf_encode(unicode, &in->keyboard.text[in->keyboard.text_len],
            ZR_INPUT_MAX - in->keyboard.text_len);
        in->keyboard.text_len += len;
    }
}

void
zr_input_char(struct zr_input *in, char c)
{
    zr_glyph glyph;
    ZR_ASSERT(in);
    glyph[0] = c;
    zr_input_glyph(in, glyph);
}

void
zr_input_unicode(struct zr_input *in, zr_rune unicode)
{
    zr_glyph rune;
    ZR_ASSERT(in);
    zr_utf_encode(unicode, rune, ZR_UTF_SIZE);
    zr_input_glyph(in, rune);
}

void
zr_input_end(struct zr_input *in)
{
    ZR_ASSERT(in);
    if (!in) return;
    in->mouse.delta = zr_vec2_sub(in->mouse.pos, in->mouse.prev);
}

int
zr_input_has_mouse_click_in_rect(const struct zr_input *i, enum zr_buttons id,
    struct zr_rect b)
{
    const struct zr_mouse_button *btn;
    if (!i) return zr_false;
    btn = &i->mouse.buttons[id];
    if (!ZR_INBOX(btn->clicked_pos.x,btn->clicked_pos.y,b.x,b.y,b.w,b.h))
        return zr_false;
    return zr_true;
}

int
zr_input_has_mouse_click_down_in_rect(const struct zr_input *i, enum zr_buttons id,
    struct zr_rect b, int down)
{
    const struct zr_mouse_button *btn;
    if (!i) return zr_false;
    btn = &i->mouse.buttons[id];
    return zr_input_has_mouse_click_in_rect(i, id, b) && (btn->down == down);
}

int
zr_input_is_mouse_click_in_rect(const struct zr_input *i, enum zr_buttons id,
    struct zr_rect b)
{
    const struct zr_mouse_button *btn;
    if (!i) return zr_false;
    btn = &i->mouse.buttons[id];
    return (zr_input_has_mouse_click_down_in_rect(i, id, b, zr_true) &&
            btn->clicked) ? zr_true : zr_false;
}

int
zr_input_is_mouse_hovering_rect(const struct zr_input *i, struct zr_rect rect)
{
    if (!i) return zr_false;
    return ZR_INBOX(i->mouse.pos.x, i->mouse.pos.y, rect.x, rect.y, rect.w, rect.h);
}

int
zr_input_is_mouse_prev_hovering_rect(const struct zr_input *i, struct zr_rect rect)
{
    if (!i) return zr_false;
    return ZR_INBOX(i->mouse.prev.x, i->mouse.prev.y, rect.x, rect.y, rect.w, rect.h);
}

int
zr_input_mouse_clicked(const struct zr_input *i, enum zr_buttons id, struct zr_rect rect)
{
    if (!i) return zr_false;
    if (!zr_input_is_mouse_hovering_rect(i, rect)) return zr_false;
    return zr_input_is_mouse_click_in_rect(i, id, rect);
}

int
zr_input_is_mouse_down(const struct zr_input *i, enum zr_buttons id)
{
    if (!i) return zr_false;
    return i->mouse.buttons[id].down;
}

int
zr_input_is_mouse_pressed(const struct zr_input *i, enum zr_buttons id)
{
    const struct zr_mouse_button *b;
    if (!i) return zr_false;
    b = &i->mouse.buttons[id];
    if (b->down && b->clicked)
        return zr_true;
    return zr_false;
}

int
zr_input_is_mouse_released(const struct zr_input *i, enum zr_buttons id)
{
    if (!i) return zr_false;
    return (!i->mouse.buttons[id].down && i->mouse.buttons[id].clicked) ? zr_true : zr_false;
}

int
zr_input_is_key_pressed(const struct zr_input *i, enum zr_keys key)
{
    const struct zr_key *k;
    if (!i) return zr_false;
    k = &i->keyboard.keys[key];
    if (k->down && k->clicked)
        return zr_true;
    return zr_false;
}

int
zr_input_is_key_released(const struct zr_input *i, enum zr_keys key)
{
    const struct zr_key *k;
    if (!i) return zr_false;
    k = &i->keyboard.keys[key];
    if (!k->down && k->clicked)
        return zr_true;
    return zr_false;
}

int
zr_input_is_key_down(const struct zr_input *i, enum zr_keys key)
{
    const struct zr_key *k;
    if (!i) return zr_false;
    k = &i->keyboard.keys[key];
    if (k->down) return zr_true;
    return zr_false;
}

/* ==============================================================
 *
 *                          Buffer
 *
 * ===============================================================*/
void
zr_buffer_init(struct zr_buffer *b, const struct zr_allocator *a,
    zr_size initial_size, float grow_factor)
{
    ZR_ASSERT(b);
    ZR_ASSERT(a);
    ZR_ASSERT(initial_size);
    if (!b || !a || !initial_size) return;

    zr_zero(b, sizeof(*b));
    b->type = ZR_BUFFER_DYNAMIC;
    b->memory.ptr = a->alloc(a->userdata, initial_size);
    b->memory.size = initial_size;
    b->size = initial_size;
    b->grow_factor = grow_factor;
    b->pool = *a;
}

void
zr_buffer_init_fixed(struct zr_buffer *b, void *m, zr_size size)
{
    ZR_ASSERT(b);
    ZR_ASSERT(m);
    ZR_ASSERT(size);
    if (!b || !m || !size) return;

    zr_zero(b, sizeof(*b));
    b->type = ZR_BUFFER_FIXED;
    b->memory.ptr = m;
    b->memory.size = size;
    b->size = size;
}

static void*
zr_buffer_align(void *unaligned, zr_size align, zr_size *alignment,
    enum zr_buffer_allocation_type type)
{
    void *memory = 0;
    if (type == ZR_BUFFER_FRONT) {
        if (align) {
            memory = ZR_ALIGN_PTR(unaligned, align);
            *alignment = (zr_size)((zr_byte*)memory - (zr_byte*)unaligned);
        } else {
            memory = unaligned;
            *alignment = 0;
        }
    } else {
        if (align) {
            memory = ZR_ALIGN_PTR_BACK(unaligned, align);
            *alignment = (zr_size)((zr_byte*)unaligned - (zr_byte*)memory);
        } else {
            memory = unaligned;
            *alignment = 0;
        }
    }
    return memory;
}

static void*
zr_buffer_realloc(struct zr_buffer *b, zr_size capacity, zr_size *size)
{
    void *temp;
    zr_size buffer_size;

    ZR_ASSERT(b);
    ZR_ASSERT(size);
    if (!b || !size || !b->pool.alloc || !b->pool.free)
        return 0;

    buffer_size = b->memory.size;
    temp = b->pool.alloc(b->pool.userdata, capacity);
    ZR_ASSERT(temp);
    if (!temp) return 0;
    zr_memcopy(temp, b->memory.ptr, buffer_size);
    b->pool.free(b->pool.userdata, b->memory.ptr);
    *size = capacity;

    if (b->size == buffer_size) {
        /* no back buffer so just set correct size */
        b->size = capacity;
        return temp;
    } else {
        /* copy back buffer to the end of the new buffer */
        void *dst, *src;
        zr_size back_size;
        back_size = buffer_size - b->size;
        dst = zr_ptr_add(void, temp, capacity - back_size);
        src = zr_ptr_add(void, temp, b->size);
        zr_memcopy(dst, src, back_size);
        b->size = capacity - back_size;
    }
    return temp;
}

static void*
zr_buffer_alloc(struct zr_buffer *b, enum zr_buffer_allocation_type type,
    zr_size size, zr_size align)
{
    int full;
    zr_size cap;
    zr_size alignment;
    void *unaligned;
    void *memory;

    ZR_ASSERT(b);
    ZR_ASSERT(size);
    if (!b || !size) return 0;
    b->needed += size;

    /* calculate total size with needed alignment + size */
    if (type == ZR_BUFFER_FRONT)
        unaligned = zr_ptr_add(void, b->memory.ptr, b->allocated);
    else unaligned = zr_ptr_add(void, b->memory.ptr, b->size - size);
    memory = zr_buffer_align(unaligned, align, &alignment, type);

    /* check if buffer has enough memory*/
    if (type == ZR_BUFFER_FRONT)
        full = ((b->allocated + size + alignment) > b->size);
    else full = ((b->size - (size + alignment)) <= b->allocated);

    if (full) {
        /* buffer is full so allocate bigger buffer if dynamic */
        ZR_ASSERT(b->type == ZR_BUFFER_DYNAMIC);
        ZR_ASSERT(b->pool.alloc && b->pool.free);
        if (b->type != ZR_BUFFER_DYNAMIC || !b->pool.alloc || !b->pool.free) return 0;

        cap = (zr_size)((float)b->memory.size * b->grow_factor);
        cap = MAX(cap, zr_round_up_pow2((uint32_t)(b->allocated + size)));
        b->memory.ptr = zr_buffer_realloc(b, cap, &b->memory.size);
        if (!b->memory.ptr) return 0;

        /* align newly allocated pointer */
        if (type == ZR_BUFFER_FRONT)
            unaligned = zr_ptr_add(void, b->memory.ptr, b->allocated);
        else unaligned = zr_ptr_add(void, b->memory.ptr, b->size);
        memory = zr_buffer_align(unaligned, align, &alignment, type);
    }

    if (type == ZR_BUFFER_FRONT)
        b->allocated += size + alignment;
    else b->size -= (size + alignment);
    b->needed += alignment;
    b->calls++;
    return memory;
}

int
zr_buffer_push(struct zr_buffer *buffer, enum zr_buffer_allocation_type type,
    void *data, zr_size size, zr_size align)
{
    zr_size i = 0;
    zr_byte *dst, *src;
    ZR_ASSERT(buffer);
    ZR_ASSERT(data);
    ZR_ASSERT(size);

    src = (zr_byte*)data;
    dst = (zr_byte*)zr_buffer_alloc(buffer, type, size, align);
    if (!dst) return zr_false;
    for (i = 0; i < size; ++i)
        dst[i] = src[i];
    return zr_true;
}

void
zr_buffer_mark(struct zr_buffer *buffer, enum zr_buffer_allocation_type type)
{
    ZR_ASSERT(buffer);
    if (!buffer) return;
    buffer->marker[type].active = zr_true;
    if (type == ZR_BUFFER_BACK)
        buffer->marker[type].offset = buffer->size;
    else buffer->marker[type].offset = buffer->allocated;
}

void
zr_buffer_reset(struct zr_buffer *buffer, enum zr_buffer_allocation_type type)
{
    ZR_ASSERT(buffer);
    if (!buffer) return;
    if (type == ZR_BUFFER_BACK) {
        /* reset back buffer either back to marker or empty */
        buffer->needed -= (buffer->memory.size - buffer->marker[type].offset);
        if (buffer->marker[type].active)
            buffer->size = buffer->marker[type].offset;
        else buffer->size = buffer->memory.size;
        buffer->marker[type].active = zr_false;
    } else {
        /* reset front buffer either back to back marker or empty */
        buffer->needed -= (buffer->allocated - buffer->marker[type].offset);
        if (buffer->marker[type].active)
            buffer->allocated = buffer->marker[type].offset;
        else buffer->allocated = 0;
        buffer->marker[type].active = zr_false;
    }
}

void
zr_buffer_clear(struct zr_buffer *b)
{
    ZR_ASSERT(b);
    if (!b) return;
    b->allocated = 0;
    b->size = b->memory.size;
    b->calls = 0;
    b->needed = 0;
}

void
zr_buffer_free(struct zr_buffer *b)
{
    ZR_ASSERT(b);
    if (!b || !b->memory.ptr) return;
    if (b->type == ZR_BUFFER_FIXED) return;
    if (!b->pool.free) return;
    ZR_ASSERT(b->pool.free);
    b->pool.free(b->pool.userdata, b->memory.ptr);
}

void
zr_buffer_info(struct zr_memory_status *s, struct zr_buffer *b)
{
    ZR_ASSERT(b);
    ZR_ASSERT(s);
    if (!s || !b) return;
    s->allocated = b->allocated;
    s->size =  b->memory.size;
    s->needed = b->needed;
    s->memory = b->memory.ptr;
    s->calls = b->calls;
}

void*
zr_buffer_memory(struct zr_buffer *buffer)
{
    ZR_ASSERT(buffer);
    if (!buffer) return 0;
    return buffer->memory.ptr;
}

zr_size
zr_buffer_total(struct zr_buffer *buffer)
{
    ZR_ASSERT(buffer);
    if (!buffer) return 0;
    return buffer->memory.size;
}
/* ==============================================================
 *
 *                      Command buffer
 *
 * ===============================================================*/
void
zr_command_buffer_init(struct zr_command_buffer *cmdbuf,
    struct zr_buffer *buffer, enum zr_command_clipping clip)
{
    ZR_ASSERT(cmdbuf);
    ZR_ASSERT(buffer);
    if (!cmdbuf || !buffer) return;
    cmdbuf->queue = 0;
    cmdbuf->base = buffer;
    cmdbuf->use_clipping = clip;
    cmdbuf->begin = buffer->allocated;
    cmdbuf->end = buffer->allocated;
    cmdbuf->last = buffer->allocated;
}

void
zr_command_buffer_reset(struct zr_command_buffer *buffer)
{
    ZR_ASSERT(buffer);
    if (!buffer) return;
    zr_zero(&buffer->stats, sizeof(buffer->stats));
    buffer->begin = 0;
    buffer->end = 0;
    buffer->last = 0;
    buffer->clip = zr_null_rect;
}

void
zr_command_buffer_clear(struct zr_command_buffer *buffer)
{
    ZR_ASSERT(buffer);
    if (!buffer) return;
    zr_command_buffer_reset(buffer);
    zr_buffer_clear(buffer->base);
}

void*
zr_command_buffer_push(struct zr_command_buffer* b,
    enum zr_command_type t, zr_size size)
{
    static const zr_size align = ZR_ALIGNOF(struct zr_command);
    struct zr_command *cmd;
    zr_size alignment;
    void *unaligned;
    void *memory;

    ZR_ASSERT(b);
    ZR_ASSERT(b->base);
    if (!b) return 0;

    cmd = (struct zr_command*)zr_buffer_alloc(b->base, ZR_BUFFER_FRONT, size, align);
    if (!cmd) return 0;

    /* make sure the offset to the next command is aligned */
    b->last = (zr_size)((zr_byte*)cmd - (zr_byte*)b->base->memory.ptr);
    unaligned = (zr_byte*)cmd + size;
    memory = ZR_ALIGN_PTR(unaligned, align);
    alignment = (zr_size)((zr_byte*)memory - (zr_byte*)unaligned);

    cmd->type = t;
    cmd->next = b->base->allocated + alignment;
    b->end = cmd->next;
    return cmd;
}

void
zr_command_buffer_push_scissor(struct zr_command_buffer *b, struct zr_rect r)
{
    struct zr_command_scissor *cmd;
    ZR_ASSERT(b);
    if (!b) return;

    b->clip.x = r.x;
    b->clip.y = r.y;
    b->clip.w = r.w;
    b->clip.h = r.h;
    cmd = (struct zr_command_scissor*)
        zr_command_buffer_push(b, ZR_COMMAND_SCISSOR, sizeof(*cmd));
    if (!cmd) return;
    cmd->x = (short)r.x;
    cmd->y = (short)r.y;
    cmd->w = (unsigned short)MAX(0, r.w);
    cmd->h = (unsigned short)MAX(0, r.h);
    b->stats.scissors++;
}

void
zr_command_buffer_push_line(struct zr_command_buffer *b, float x0, float y0,
    float x1, float y1, struct zr_color c)
{
    struct zr_command_line *cmd;
    ZR_ASSERT(b);
    if (!b) return;
    cmd = (struct zr_command_line*)
        zr_command_buffer_push(b, ZR_COMMAND_LINE, sizeof(*cmd));
    if (!cmd) return;
    cmd->begin.x = (short)x0;
    cmd->begin.y = (short)y0;
    cmd->end.x = (short)x1;
    cmd->end.y = (short)y1;
    cmd->color = c;
    b->stats.lines++;
}

void
zr_command_buffer_push_curve(struct zr_command_buffer *b, float ax, float ay,
    float ctrl0x, float ctrl0y, float ctrl1x, float ctrl1y,
    float bx, float by, struct zr_color col)
{
    struct zr_command_curve *cmd;
    ZR_ASSERT(b);
    if (!b) return;

    cmd = (struct zr_command_curve*)
        zr_command_buffer_push(b, ZR_COMMAND_CURVE, sizeof(*cmd));
    if (!cmd) return;
    cmd->begin.x = (short)ax;
    cmd->begin.y = (short)ay;
    cmd->ctrl[0].x = (short)ctrl0x;
    cmd->ctrl[0].y = (short)ctrl0y;
    cmd->ctrl[1].x = (short)ctrl1x;
    cmd->ctrl[1].y = (short)ctrl1y;
    cmd->end.x = (short)bx;
    cmd->end.y = (short)by;
    cmd->color = col;
    b->stats.curves++;
}

void
zr_command_buffer_push_rect(struct zr_command_buffer *b, struct zr_rect rect,
    float rounding, struct zr_color c)
{
    struct zr_command_rect *cmd;
    ZR_ASSERT(b);
    if (!b) return;
    if (b->use_clipping) {
        const struct zr_rect *clip = &b->clip;
        if (!ZR_INTERSECT(rect.x, rect.y, rect.w, rect.h,
            clip->x, clip->y, clip->w, clip->h)) return;
    }

    cmd = (struct zr_command_rect*)
        zr_command_buffer_push(b, ZR_COMMAND_RECT, sizeof(*cmd));
    if (!cmd) return;
    cmd->rounding = (uint32_t)rounding;
    cmd->x = (short)rect.x;
    cmd->y = (short)rect.y;
    cmd->w = (unsigned short)MAX(0, rect.w);
    cmd->h = (unsigned short)MAX(0, rect.h);
    cmd->color = c;
    b->stats.rectangles++;
}

void
zr_command_buffer_push_circle(struct zr_command_buffer *b, struct zr_rect r,
    struct zr_color c)
{
    struct zr_command_circle *cmd;
    ZR_ASSERT(b);
    if (!b) return;
    if (b->use_clipping) {
        const struct zr_rect *clip = &b->clip;
        if (!ZR_INTERSECT(r.x, r.y, r.w, r.h, clip->x, clip->y, clip->w, clip->h))
            return;
    }

    cmd = (struct zr_command_circle*)
        zr_command_buffer_push(b, ZR_COMMAND_CIRCLE, sizeof(*cmd));
    if (!cmd) return;
    cmd->x = (short)r.x;
    cmd->y = (short)r.y;
    cmd->w = (unsigned short)MAX(r.w, 0);
    cmd->h = (unsigned short)MAX(r.h, 0);
    cmd->color = c;
    b->stats.circles++;
}

void
zr_command_buffer_push_arc(struct zr_command_buffer *b, float cx,
    float cy, float radius, float a_min, float a_max, struct zr_color c)
{
    struct zr_command_arc *cmd;
    cmd = (struct zr_command_arc*)
        zr_command_buffer_push(b, ZR_COMMAND_ARC, sizeof(*cmd));
    if (!cmd) return;
    cmd->cx = (short)cx;
    cmd->cy = (short)cy;
    cmd->r = (unsigned short)radius;
    cmd->a[0] = a_min;
    cmd->a[1] = a_max;
    cmd->color = c;
}

void
zr_command_buffer_push_triangle(struct zr_command_buffer *b,float x0,float y0,
    float x1, float y1, float x2, float y2, struct zr_color c)
{
    struct zr_command_triangle *cmd;
    ZR_ASSERT(b);
    if (!b) return;
    if (b->use_clipping) {
        const struct zr_rect *clip = &b->clip;
        if (!ZR_INBOX(x0, y0, clip->x, clip->y, clip->w, clip->h) ||
            !ZR_INBOX(x1, y1, clip->x, clip->y, clip->w, clip->h) ||
            !ZR_INBOX(x2, y2, clip->x, clip->y, clip->w, clip->h))
            return;
    }

    cmd = (struct zr_command_triangle*)
        zr_command_buffer_push(b, ZR_COMMAND_TRIANGLE, sizeof(*cmd));
    if (!cmd) return;
    cmd->a.x = (short)x0;
    cmd->a.y = (short)y0;
    cmd->b.x = (short)x1;
    cmd->b.y = (short)y1;
    cmd->c.x = (short)x2;
    cmd->c.y = (short)y2;
    cmd->color = c;
    b->stats.triangles++;
}

void
zr_command_buffer_push_image(struct zr_command_buffer *b, struct zr_rect r,
    struct zr_image *img)
{
    struct zr_command_image *cmd;
    ZR_ASSERT(b);
    if (!b) return;
    if (b->use_clipping) {
        const struct zr_rect *c = &b->clip;
        if (!ZR_INTERSECT(r.x, r.y, r.w, r.h, c->x, c->y, c->w, c->h))
            return;
    }

    cmd = (struct zr_command_image*)
        zr_command_buffer_push(b, ZR_COMMAND_IMAGE, sizeof(*cmd));
    if (!cmd) return;
    cmd->x = (short)r.x;
    cmd->y = (short)r.y;
    cmd->w = (unsigned short)MAX(0, r.w);
    cmd->h = (unsigned short)MAX(0, r.h);
    cmd->img = *img;
    b->stats.images++;
}

void
zr_command_buffer_push_text(struct zr_command_buffer *b, struct zr_rect r,
    const char *string, zr_size length, const struct zr_user_font *font,
    struct zr_color bg, struct zr_color fg)
{
    zr_size text_width = 0;
    struct zr_command_text *cmd;
    ZR_ASSERT(b);
    ZR_ASSERT(font);
    if (!b || !string || !length) return;
    if (b->use_clipping) {
        const struct zr_rect *c = &b->clip;
        if (!ZR_INTERSECT(r.x, r.y, r.w, r.h, c->x, c->y, c->w, c->h))
            return;
    }

    /* make sure text fits inside bounds */
    text_width = font->width(font->userdata, font->height, string, length);
    if (text_width > r.w){
        float txt_width = (float)text_width;
        zr_size glyphes = 0;
        length = zr_use_font_glyph_clamp(font, string, length, r.w, &glyphes, &txt_width);
    }

    if (!length) return;
    cmd = (struct zr_command_text*)
        zr_command_buffer_push(b, ZR_COMMAND_TEXT, sizeof(*cmd) + length + 1);
    if (!cmd) return;
    cmd->x = (short)r.x;
    cmd->y = (short)r.y;
    cmd->w = (unsigned short)r.w;
    cmd->h = (unsigned short)r.h;
    cmd->background = bg;
    cmd->foreground = fg;
    cmd->font = font;
    cmd->length = length;
    cmd->height = font->height;
    zr_memcopy(cmd->string, string, length);
    cmd->string[length] = '\0';
    b->stats.text++;
    b->stats.glyphes += (unsigned int)length;
}

const struct zr_command*
zr_command_buffer_begin(struct zr_command_buffer *buffer)
{
    zr_byte *memory;
    ZR_ASSERT(buffer);
    if (!buffer || !buffer->base) return 0;
    if (buffer->last == buffer->begin) return 0;
    memory = (zr_byte*)buffer->base->memory.ptr;
    return zr_ptr_add_const(struct zr_command, memory, buffer->begin);
}

const struct zr_command*
zr_command_buffer_next(struct zr_command_buffer *buffer,
    struct zr_command *cmd)
{
    zr_byte *memory;
    ZR_ASSERT(buffer);
    if (!buffer) return 0;
    if (!cmd) return 0;
    if (cmd->next > buffer->last) return 0;
    if (cmd->next > buffer->base->allocated) return 0;
    memory = (zr_byte*)buffer->base->memory.ptr;
    return zr_ptr_add_const(struct zr_command, memory, cmd->next);
}
/* ==============================================================
 *
 *                      Command Queue
 *
 * ===============================================================*/
void
zr_command_queue_init(struct zr_command_queue *queue,
    const struct zr_allocator *alloc, zr_size initial_size, float grow_factor)
{
    ZR_ASSERT(alloc);
    ZR_ASSERT(queue);
    if (!alloc || !queue) return;
    zr_zero(queue, sizeof(*queue));
    zr_buffer_init(&queue->buffer, alloc, initial_size, grow_factor);
}

void
zr_command_queue_init_fixed(struct zr_command_queue *queue,
    void *memory, zr_size size)
{
    ZR_ASSERT(size);
    ZR_ASSERT(memory);
    ZR_ASSERT(queue);
    if (!memory || !size || !queue) return;
    zr_zero(queue, sizeof(*queue));
    zr_buffer_init_fixed(&queue->buffer, memory, size);
}

void
zr_command_queue_insert_back(struct zr_command_queue *queue,
    struct zr_command_buffer *buffer)
{
    struct zr_command_buffer_list *list;
    ZR_ASSERT(queue);
    ZR_ASSERT(buffer);
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
zr_command_queue_insert_front(struct zr_command_queue *queue,
    struct zr_command_buffer *buffer)
{
    struct zr_command_buffer_list *list;
    ZR_ASSERT(queue);
    ZR_ASSERT(buffer);
    if (!queue || !buffer) return;

    list = &queue->list;
    if (!list->begin) {
        zr_command_queue_insert_back(queue, buffer);
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
zr_command_queue_remove(struct zr_command_queue *queue,
    struct zr_command_buffer *buffer)
{
    struct zr_command_buffer_list *list;
    ZR_ASSERT(queue);
    ZR_ASSERT(buffer);
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

int
zr_command_queue_start_child(struct zr_command_queue *queue,
    struct zr_command_buffer *buffer)
{
    static const zr_size buf_size = sizeof(struct zr_command_sub_buffer);
    static const zr_size buf_align = ZR_ALIGNOF(struct zr_command_sub_buffer);
    struct zr_command_sub_buffer_stack *stack;
    struct zr_command_sub_buffer *buf;
    struct zr_command_sub_buffer *end;
    zr_size offset;
    zr_size size;
    void *memory;

    ZR_ASSERT(queue);
    ZR_ASSERT(buffer);
    if (!queue || !buffer) return zr_false;

    /* allocate header space from the buffer */
    memory = queue->buffer.memory.ptr;
    buf = (struct zr_command_sub_buffer*)
        zr_buffer_alloc(&queue->buffer, ZR_BUFFER_BACK, buf_size, buf_align);
    if (!buf) return zr_false;
    zr_zero(buf, sizeof(*buf));
    offset = (zr_size)(((zr_byte*)memory + queue->buffer.memory.size) - (zr_byte*)buf);

    /* setup sub buffer */
    buf->begin = buffer->end;
    buf->end = buffer->end;
    buf->parent_last = buffer->last;
    buf->last = buf->begin;
    buf->next = 0;

    /* add first sub-buffer into the stack */
    stack = &queue->stack;
    stack->count++;
    if (!stack->begin) {
        stack->begin = offset;
        stack->end = offset;
        return zr_true;
    }

    /* add sub-buffer at the end of the stack */
    size = queue->buffer.memory.size;
    end = zr_ptr_add(struct zr_command_sub_buffer, memory, (size - stack->end));
    end->next = offset;
    stack->end = offset;
    return zr_true;
}

void
zr_command_queue_finish_child(struct zr_command_queue *queue,
    struct zr_command_buffer *buffer)
{
    struct zr_command_sub_buffer *buf;
    struct zr_command_sub_buffer_stack *stack;
    zr_size offset;

    ZR_ASSERT(queue);
    ZR_ASSERT(buffer);
    if (!queue || !buffer) return;
    if (!queue->stack.count) return;

    stack = &queue->stack;
    offset = queue->buffer.memory.size - stack->end;
    buf = zr_ptr_add(struct zr_command_sub_buffer, queue->buffer.memory.ptr, offset);
    buf->last = buffer->last;
    buf->end = buffer->end;
}

void
zr_command_queue_start(struct zr_command_queue *queue,
    struct zr_command_buffer *buffer)
{
    ZR_ASSERT(queue);
    ZR_ASSERT(buffer);
    if (!queue || !buffer) return;
    zr_zero(&buffer->stats, sizeof(buffer->stats));
    buffer->begin = queue->buffer.allocated;
    buffer->end = buffer->begin;
    buffer->last = buffer->begin;
    buffer->clip = zr_null_rect;
}

void
zr_command_queue_finish(struct zr_command_queue *queue,
    struct zr_command_buffer *buffer)
{
    void *memory;
    zr_size size;
    zr_size i = 0;
    struct zr_command_sub_buffer *iter;
    struct zr_command_sub_buffer_stack *stack;

    ZR_ASSERT(queue);
    ZR_ASSERT(buffer);

    if (!queue || !buffer) return;
    stack = &queue->stack;
    buffer->end = queue->buffer.allocated;
    if (!stack->count) return;

    memory = queue->buffer.memory.ptr;
    size = queue->buffer.memory.size;
    iter = zr_ptr_add(struct zr_command_sub_buffer, memory, (size - stack->begin));

    /* fix buffer command list for subbuffers  */
    for (i = 0; i < queue->stack.count; ++i) {
        struct zr_command *parent_last, *sublast, *last;

        parent_last = zr_ptr_add(struct zr_command, memory, iter->parent_last);
        sublast = zr_ptr_add(struct zr_command, memory, iter->last);
        last = zr_ptr_add(struct zr_command, memory, buffer->last);

        /* redirect the subbuffer to the end of the current command buffer */
        parent_last->next = iter->end;
        if (i == (queue->stack.count-1))
            sublast->next = last->next;
        last->next = iter->begin;
        buffer->last = iter->last;
        buffer->end = iter->end;
        iter = zr_ptr_add(struct zr_command_sub_buffer, memory, size - iter->next);
    }
    queue->stack.count = 0;
    queue->stack.begin = 0;
    queue->stack.end = 0;
}

void
zr_command_queue_free(struct zr_command_queue *queue)
{
    ZR_ASSERT(queue);
    if (!queue) return;
    zr_buffer_clear(&queue->buffer);
    zr_buffer_free(&queue->buffer);
}

void
zr_command_queue_clear(struct zr_command_queue *queue)
{
    ZR_ASSERT(queue);
    if (!queue) return;
    zr_buffer_clear(&queue->buffer);
    queue->build = zr_false;
    zr_zero(&queue->stack, sizeof(queue->stack));
}

void
zr_command_queue_build(struct zr_command_queue *queue)
{
    struct zr_command_buffer *iter;
    struct zr_command_buffer *next;
    struct zr_command *cmd;
    zr_byte *buffer;

    iter = queue->list.begin;
    buffer = (zr_byte*)queue->buffer.memory.ptr;
    while (iter != 0) {
        next = iter->next;
        if (iter->last != iter->begin) {
            cmd = zr_ptr_add(struct zr_command, buffer, iter->last);
            while (next && next->last == next->begin)
                next = next->next; /* skip empty command buffers */
            if (next) {
                cmd->next = next->begin;
            } else cmd->next = queue->buffer.allocated;
        }
        iter = next;
    }
    queue->build = zr_true;
}

const struct zr_command*
zr_command_queue_begin(struct zr_command_queue *queue)
{
    struct zr_command_buffer *iter;
    zr_byte *buffer;
    ZR_ASSERT(queue);
    if (!queue) return 0;
    if (!queue->list.count) return 0;

    /* build one command list out of all command buffers */
    buffer = (zr_byte*)queue->buffer.memory.ptr;
    if (!queue->build)
        zr_command_queue_build(queue);

    iter = queue->list.begin;
    while (iter && iter->begin == iter->end)
        iter = iter->next;
    if (!iter) return 0;
    return zr_ptr_add_const(struct zr_command, buffer, iter->begin);
}

const struct zr_command*
zr_command_queue_next(struct zr_command_queue *queue, const struct zr_command *cmd)
{
    zr_byte *buffer;
    const struct zr_command *next;
    ZR_ASSERT(queue);
    if (!queue || !cmd || !queue->list.count) return 0;
    if (cmd->next >= queue->buffer.allocated)
        return 0;
    buffer = (zr_byte*)queue->buffer.memory.ptr;
    next = zr_ptr_add_const(struct zr_command, buffer, cmd->next);
    return next;
}
/* ==============================================================
 *
 *                      Draw List
 *
 * ===============================================================*/
#if ZR_COMPILE_WITH_VERTEX_BUFFER
void
zr_draw_list_init(struct zr_draw_list *list, struct zr_buffer *cmds,
    struct zr_buffer *vertexes, struct zr_buffer *elements,
    zr_sin_f sine, zr_cos_f cosine, struct zr_draw_null_texture null,
    enum zr_anti_aliasing AA)
{
    zr_zero(list, sizeof(*list));
    list->sin = sine;
    list->cos = cosine;
    list->null = null;
    list->clip_rect = zr_null_rect;
    list->vertexes = vertexes;
    list->elements = elements;
    list->buffer = cmds;
    list->AA = AA;
}

void
zr_draw_list_load(struct zr_draw_list *list, struct zr_command_queue *queue,
    float line_thickness, unsigned int curve_segments)
{
    const struct zr_command *cmd;
    ZR_ASSERT(list);
    ZR_ASSERT(list->vertexes);
    ZR_ASSERT(list->elements);
    ZR_ASSERT(queue);
    line_thickness = MAX(line_thickness, 1.0f);
    if (!list || !queue || !list->vertexes || !list->elements) return;

    zr_foreach_command(cmd, queue) {
        switch (cmd->type) {
        case ZR_COMMAND_NOP: break;
        case ZR_COMMAND_SCISSOR: {
            const struct zr_command_scissor *s = zr_command(scissor, cmd);
            zr_draw_list_add_clip(list, zr_rect(s->x, s->y, s->w, s->h));
        } break;
        case ZR_COMMAND_LINE: {
            const struct zr_command_line *l = zr_command(line, cmd);
            zr_draw_list_add_line(list, zr_vec2(l->begin.x, l->begin.y),
                zr_vec2(l->end.x, l->end.y), l->color, line_thickness);
        } break;
        case ZR_COMMAND_CURVE: {
            const struct zr_command_curve *q = zr_command(curve, cmd);
            zr_draw_list_add_curve(list, zr_vec2(q->begin.x, q->begin.y),
                zr_vec2(q->ctrl[0].x, q->ctrl[0].y), zr_vec2(q->ctrl[1].x, q->ctrl[1].y),
                zr_vec2(q->end.x, q->end.y), q->color, curve_segments, line_thickness);
        } break;
        case ZR_COMMAND_RECT: {
            const struct zr_command_rect *r = zr_command(rect, cmd);
            zr_draw_list_add_rect_filled(list, zr_rect(r->x, r->y, r->w, r->h),
                r->color, (float)r->rounding);
        } break;
        case ZR_COMMAND_CIRCLE: {
            const struct zr_command_circle *c = zr_command(circle, cmd);
            zr_draw_list_add_circle_filled(list, zr_vec2((float)c->x + (float)c->w/2,
                (float)c->y + (float)c->h/2), (float)c->w/2, c->color, curve_segments);
        } break;
        case ZR_COMMAND_ARC: {
            const struct zr_command_arc *c = zr_command(arc, cmd);
            zr_draw_list_path_arc_to(list, zr_vec2(c->cx, c->cy), c->r,
                c->a[0], c->a[1], curve_segments);
            zr_draw_list_path_fill(list, c->color);
        } break;
        case ZR_COMMAND_TRIANGLE: {
            const struct zr_command_triangle *t = zr_command(triangle, cmd);
            zr_draw_list_add_triangle_filled(list, zr_vec2(t->a.x, t->a.y),
                zr_vec2(t->b.x, t->b.y), zr_vec2(t->c.x, t->c.y), t->color);
        } break;
        case ZR_COMMAND_TEXT: {
            const struct zr_command_text *t = zr_command(text, cmd);
            zr_draw_list_add_text(list, t->font, zr_rect(t->x, t->y, t->w, t->h),
                t->string, t->length, t->height, t->background, t->foreground);
        } break;
        case ZR_COMMAND_IMAGE: {
            const struct zr_command_image *i = zr_command(image, cmd);
            zr_draw_list_add_image(list, i->img, zr_rect(i->x, i->y, i->w, i->h),
                zr_rgb(255, 255, 255));
        } break;
        default: break;
        }
    }
}

void
zr_draw_list_clear(struct zr_draw_list *list)
{
    ZR_ASSERT(list);
    if (!list) return;
    if (list->buffer)
        zr_buffer_clear(list->buffer);
    if (list->elements)
        zr_buffer_clear(list->vertexes);
    if (list->vertexes)
        zr_buffer_clear(list->elements);

    list->element_count = 0;
    list->vertex_count = 0;
    list->cmd_offset = 0;
    list->cmd_count = 0;
    list->path_count = 0;
    list->vertexes = 0;
    list->elements = 0;
    list->clip_rect = zr_null_rect;
}

int
zr_draw_list_is_empty(struct zr_draw_list *list)
{
    ZR_ASSERT(list);
    if (!list) return zr_true;
    return (list->cmd_count == 0);
}

const struct zr_draw_command*
zr_draw_list_begin(const struct zr_draw_list *list)
{
    zr_byte *memory;
    zr_size offset;
    const struct zr_draw_command *cmd;
    ZR_ASSERT(list);
    ZR_ASSERT(list->buffer);
    if (!list || !list->buffer || !list->cmd_count) return 0;

    memory = (zr_byte*)list->buffer->memory.ptr;
    offset = list->buffer->memory.size - list->cmd_offset;
    cmd = zr_ptr_add(const struct zr_draw_command, memory, offset);
    return cmd;
}

const struct zr_draw_command*
zr_draw_list_next(const struct zr_draw_list *list, const struct zr_draw_command *cmd)
{
    zr_byte *memory;
    zr_size offset;
    const struct zr_draw_command *end;
    ZR_ASSERT(list);
    if (!list || !list->buffer || !list->cmd_count) return 0;

    memory = (zr_byte*)list->buffer->memory.ptr;
    offset = list->buffer->memory.size - list->cmd_offset;
    end = zr_ptr_add(const struct zr_draw_command, memory, offset);
    end -= (list->cmd_count-1);
    if (cmd <= end) return 0;
    return (cmd - 1);
}

static struct zr_vec2*
zr_draw_list_alloc_path(struct zr_draw_list *list, zr_size count)
{
    void *memory;
    struct zr_vec2 *points;
    static const zr_size point_align = ZR_ALIGNOF(struct zr_vec2);
    static const zr_size point_size = sizeof(struct zr_vec2);
    points = (struct zr_vec2*)
        zr_buffer_alloc(list->buffer, ZR_BUFFER_FRONT, point_size * count, point_align);

    if (!points) return 0;
    if (!list->path_offset) {
        memory = zr_buffer_memory(list->buffer);
        list->path_offset = (unsigned int)((zr_byte*)points - (zr_byte*)memory);
    }
    list->path_count += (unsigned int)count;
    return points;
}

static struct zr_vec2
zr_draw_list_path_last(struct zr_draw_list *list)
{
    void *memory;
    struct zr_vec2 *point;
    ZR_ASSERT(list->path_count);
    memory = zr_buffer_memory(list->buffer);
    point = zr_ptr_add(struct zr_vec2, memory, list->path_offset);
    point += (list->path_count-1);
    return *point;
}

static struct zr_draw_command*
zr_draw_list_push_command(struct zr_draw_list *list, struct zr_rect clip,
    zr_handle texture)
{
    static const zr_size cmd_align = ZR_ALIGNOF(struct zr_draw_command);
    static const zr_size cmd_size = sizeof(struct zr_draw_command);
    struct zr_draw_command *cmd;

    ZR_ASSERT(list);
    cmd = (struct zr_draw_command*)
        zr_buffer_alloc(list->buffer, ZR_BUFFER_BACK, cmd_size, cmd_align);
    if (!cmd) return 0;
    if (!list->cmd_count) {
        zr_byte *memory = (zr_byte*)zr_buffer_memory(list->buffer);
        zr_size total = zr_buffer_total(list->buffer);
        memory = zr_ptr_add(zr_byte, memory, total);
        list->cmd_offset = (zr_size)(memory - (zr_byte*)cmd);
    }

    cmd->elem_count = 0;
    cmd->clip_rect = clip;
    cmd->texture = texture;
    list->cmd_count++;
    list->clip_rect = clip;
    return cmd;
}

static struct zr_draw_command*
zr_draw_list_command_last(struct zr_draw_list *list)
{
    void *memory;
    zr_size size;
    struct zr_draw_command *cmd;
    ZR_ASSERT(list->cmd_count);

    memory = zr_buffer_memory(list->buffer);
    size = zr_buffer_total(list->buffer);
    cmd = zr_ptr_add(struct zr_draw_command, memory, size - list->cmd_offset);
    return (cmd - (list->cmd_count-1));
}

void
zr_draw_list_add_clip(struct zr_draw_list *list, struct zr_rect rect)
{
    ZR_ASSERT(list);
    if (!list) return;
    if (!list->cmd_count) {
        zr_draw_list_push_command(list, rect, list->null.texture);
    } else {
        struct zr_draw_command *prev = zr_draw_list_command_last(list);
        zr_draw_list_push_command(list, rect, prev->texture);
    }
}

static void
zr_draw_list_push_image(struct zr_draw_list *list, zr_handle texture)
{
    ZR_ASSERT(list);
    if (!list) return;
    if (!list->cmd_count) {
        zr_draw_list_push_command(list, zr_null_rect, list->null.texture);
    } else {
        struct zr_draw_command *prev = zr_draw_list_command_last(list);
        if (prev->texture.id != texture.id)
            zr_draw_list_push_command(list, prev->clip_rect, texture);
    }
}

static struct zr_draw_vertex*
zr_draw_list_alloc_vertexes(struct zr_draw_list *list, zr_size count)
{
    struct zr_draw_vertex *vtx;
    static const zr_size vtx_align = ZR_ALIGNOF(struct zr_draw_vertex);
    static const zr_size vtx_size = sizeof(struct zr_draw_vertex);
    ZR_ASSERT(list);
    if (!list) return 0;

    vtx = (struct zr_draw_vertex*)
        zr_buffer_alloc(list->vertexes, ZR_BUFFER_FRONT, vtx_size * count, vtx_align);
    if (!vtx) return 0;
    list->vertex_count += (unsigned int)count;
    return vtx;
}

static zr_draw_index*
zr_draw_list_alloc_elements(struct zr_draw_list *list, zr_size count)
{
    zr_draw_index *ids;
    struct zr_draw_command *cmd;
    static const zr_size elem_align = ZR_ALIGNOF(zr_draw_index);
    static const zr_size elem_size = sizeof(zr_draw_index);
    ZR_ASSERT(list);
    if (!list) return 0;

    ids = (zr_draw_index*)
        zr_buffer_alloc(list->elements, ZR_BUFFER_FRONT, elem_size * count, elem_align);
    if (!ids) return 0;
    cmd = zr_draw_list_command_last(list);
    list->element_count += (unsigned int)count;
    cmd->elem_count += (unsigned int)count;
    return ids;
}

static struct zr_draw_vertex
zr_draw_vertex(struct zr_vec2 pos, struct zr_vec2 uv, zr_draw_vertex_color col)
{
    struct zr_draw_vertex out;
    out.position = pos;
    out.uv = uv;
    out.col = col;
    return out;
}

static void
zr_draw_list_add_poly_line(struct zr_draw_list *list, struct zr_vec2 *points,
    const unsigned int points_count, struct zr_color color, int closed,
    float thickness, enum zr_anti_aliasing aliasing)
{
    zr_size count;
    int thick_line;
    zr_draw_vertex_color col = zr_color32(color);
    ZR_ASSERT(list);
    if (!list) return;
    if (!list || points_count < 2) return;

    count = points_count;
    if (!closed) count = points_count-1;
    thick_line = thickness > 1.0f;

    if (aliasing == ZR_ANTI_ALIASING_ON) {
        /* ANTI-ALIASED STROKE */
        const float AA_SIZE = 1.0f;
        static const zr_size pnt_align = ZR_ALIGNOF(struct zr_vec2);
        static const zr_size pnt_size = sizeof(struct zr_vec2);
        const zr_draw_vertex_color col_trans = col & 0x00ffffff;

        /* allocate vertexes and elements  */
        zr_size i1 = 0;
        zr_size index = list->vertex_count;
        const zr_size idx_count = (thick_line) ?  (count * 18) : (count * 12);
        const zr_size vtx_count = (thick_line) ? (points_count * 4): (points_count *3);
        struct zr_draw_vertex *vtx = zr_draw_list_alloc_vertexes(list, vtx_count);
        zr_draw_index *ids = zr_draw_list_alloc_elements(list, idx_count);

        zr_size size;
        struct zr_vec2 *normals, *temp;
        if (!vtx || !ids) return;

        /* temporary allocate normals + points */
        zr_buffer_mark(list->vertexes, ZR_BUFFER_FRONT);
        size = pnt_size * ((thick_line) ? 5 : 3) * points_count;
        normals = (struct zr_vec2*)
            zr_buffer_alloc(list->vertexes, ZR_BUFFER_FRONT, size, pnt_align);
        if (!normals) return;
        temp = normals + points_count;

        /* calculate normals */
        for (i1 = 0; i1 < count; ++i1) {
            const zr_size i2 = ((i1 + 1) == points_count) ? 0 : (i1 + 1);
            struct zr_vec2 diff = zr_vec2_sub(points[i2], points[i1]);
            float len;

            /* vec2 inverted lenth  */
            len = zr_vec2_len_sqr(diff);
            if (len != 0.0f)
                len = zr_inv_sqrt(len);
            else len = 1.0f;

            diff = zr_vec2_muls(diff, len);
            normals[i1].x = diff.y;
            normals[i1].y = -diff.x;
        }

        if (!closed)
            normals[points_count-1] = normals[points_count-2];

        if (!thick_line) {
            zr_size idx1, i;
            if (!closed) {
                struct zr_vec2 d;
                temp[0] = zr_vec2_add(points[0], zr_vec2_muls(normals[0], AA_SIZE));
                temp[1] = zr_vec2_sub(points[0], zr_vec2_muls(normals[0], AA_SIZE));
                d = zr_vec2_muls(normals[points_count-1], AA_SIZE);
                temp[(points_count-1) * 2 + 0] = zr_vec2_add(points[points_count-1], d);
                temp[(points_count-1) * 2 + 1] = zr_vec2_sub(points[points_count-1], d);
            }

            /* fill elements */
            idx1 = index;
            for (i1 = 0; i1 < count; i1++) {
                struct zr_vec2 dm;
                float dmr2;
                zr_size i2 = ((i1 + 1) == points_count) ? 0 : (i1 + 1);
                zr_size idx2 = ((i1+1) == points_count) ? index: (idx1 + 3);

                /* average normals */
                dm = zr_vec2_muls(zr_vec2_add(normals[i1], normals[i2]), 0.5f);
                dmr2 = dm.x * dm.x + dm.y* dm.y;
                if (dmr2 > 0.000001f) {
                    float scale = 1.0f/dmr2;
                    scale = MIN(100.0f, scale);
                    dm = zr_vec2_muls(dm, scale);
                }

                dm = zr_vec2_muls(dm, AA_SIZE);
                temp[i2*2+0] = zr_vec2_add(points[i2], dm);
                temp[i2*2+1] = zr_vec2_sub(points[i2], dm);

                ids[0] = (zr_draw_index)(idx2 + 0); ids[1] = (zr_draw_index)(idx1+0);
                ids[2] = (zr_draw_index)(idx1 + 2); ids[3] = (zr_draw_index)(idx1+2);
                ids[4] = (zr_draw_index)(idx2 + 2); ids[5] = (zr_draw_index)(idx2+0);
                ids[6] = (zr_draw_index)(idx2 + 1); ids[7] = (zr_draw_index)(idx1+1);
                ids[8] = (zr_draw_index)(idx1 + 0); ids[9] = (zr_draw_index)(idx1+0);
                ids[10]= (zr_draw_index)(idx2 + 0); ids[11]= (zr_draw_index)(idx2+1);
                ids += 12;
                idx1 = idx2;
            }

            /* fill vertexes */
            for (i = 0; i < points_count; ++i) {
                const struct zr_vec2 uv = list->null.uv;
                vtx[0] = zr_draw_vertex(points[i], uv, col);
                vtx[1] = zr_draw_vertex(temp[i*2+0], uv, col_trans);
                vtx[2] = zr_draw_vertex(temp[i*2+1], uv, col_trans);
                vtx += 3;
            }
        } else {
            zr_size idx1, i;
            const float half_inner_thickness = (thickness - AA_SIZE) * 0.5f;
            if (!closed) {
                struct zr_vec2 d1 = zr_vec2_muls(normals[0], half_inner_thickness + AA_SIZE);
                struct zr_vec2 d2 = zr_vec2_muls(normals[0], half_inner_thickness);

                temp[0] = zr_vec2_add(points[0], d1);
                temp[1] = zr_vec2_add(points[0], d2);
                temp[2] = zr_vec2_sub(points[0], d2);
                temp[3] = zr_vec2_sub(points[0], d1);

                d1 = zr_vec2_muls(normals[points_count-1], half_inner_thickness + AA_SIZE);
                d2 = zr_vec2_muls(normals[points_count-1], half_inner_thickness);

                temp[(points_count-1)*4+0] = zr_vec2_add(points[points_count-1], d1);
                temp[(points_count-1)*4+1] = zr_vec2_add(points[points_count-1], d2);
                temp[(points_count-1)*4+2] = zr_vec2_sub(points[points_count-1], d2);
                temp[(points_count-1)*4+3] = zr_vec2_sub(points[points_count-1], d1);
            }

            /* add all elements */
            idx1 = index;
            for (i1 = 0; i1 < count; ++i1) {
                struct zr_vec2 dm_out, dm_in;
                const zr_size i2 = ((i1+1) == points_count) ? 0: (i1 + 1);
                zr_size idx2 = ((i1+1) == points_count) ? index: (idx1 + 4);

                /* average normals */
                struct zr_vec2 dm = zr_vec2_muls(zr_vec2_add(normals[i1], normals[i2]), 0.5f);
                float dmr2 = dm.x * dm.x + dm.y* dm.y;
                if (dmr2 > 0.000001f) {
                    float scale = 1.0f/dmr2;
                    scale = MIN(100.0f, scale);
                    dm = zr_vec2_muls(dm, scale);
                }

                dm_out = zr_vec2_muls(dm, ((half_inner_thickness) + AA_SIZE));
                dm_in = zr_vec2_muls(dm, half_inner_thickness);
                temp[i2*4+0] = zr_vec2_add(points[i2], dm_out);
                temp[i2*4+1] = zr_vec2_add(points[i2], dm_in);
                temp[i2*4+2] = zr_vec2_sub(points[i2], dm_in);
                temp[i2*4+3] = zr_vec2_sub(points[i2], dm_out);

                /* add indexes */
                ids[0] = (zr_draw_index)(idx2 + 1); ids[1] = (zr_draw_index)(idx1+1);
                ids[2] = (zr_draw_index)(idx1 + 2); ids[3] = (zr_draw_index)(idx1+2);
                ids[4] = (zr_draw_index)(idx2 + 2); ids[5] = (zr_draw_index)(idx2+1);
                ids[6] = (zr_draw_index)(idx2 + 1); ids[7] = (zr_draw_index)(idx1+1);
                ids[8] = (zr_draw_index)(idx1 + 0); ids[9] = (zr_draw_index)(idx1+0);
                ids[10]= (zr_draw_index)(idx2 + 0); ids[11] = (zr_draw_index)(idx2+1);
                ids[12]= (zr_draw_index)(idx2 + 2); ids[13] = (zr_draw_index)(idx1+2);
                ids[14]= (zr_draw_index)(idx1 + 3); ids[15] = (zr_draw_index)(idx1+3);
                ids[16]= (zr_draw_index)(idx2 + 3); ids[17] = (zr_draw_index)(idx2+2);
                ids += 18;
                idx1 = idx2;
            }

            /* add vertexes */
            for (i = 0; i < points_count; ++i) {
                const struct zr_vec2 uv = list->null.uv;
                vtx[0] = zr_draw_vertex(temp[i*4+0], uv, col_trans);
                vtx[1] = zr_draw_vertex(temp[i*4+1], uv, col);
                vtx[2] = zr_draw_vertex(temp[i*4+2], uv, col);
                vtx[3] = zr_draw_vertex(temp[i*4+3], uv, col_trans);
                vtx += 4;
            }
        }

        /* free temporary normals + points */
        zr_buffer_reset(list->vertexes, ZR_BUFFER_FRONT);
    } else {
        /* NON ANTI-ALIASED STROKE */
        zr_size i1 = 0;
        zr_size idx = list->vertex_count;
        const zr_size idx_count = count * 6;
        const zr_size vtx_count = count * 4;
        struct zr_draw_vertex *vtx = zr_draw_list_alloc_vertexes(list, vtx_count);
        zr_draw_index *ids = zr_draw_list_alloc_elements(list, idx_count);
        if (!vtx || !ids) return;

        for (i1 = 0; i1 < count; ++i1) {
            float dx, dy;
            const struct zr_vec2 uv = list->null.uv;
            const zr_size i2 = ((i1+1) == points_count) ? 0 : i1 + 1;
            const struct zr_vec2 p1 = points[i1];
            const struct zr_vec2 p2 = points[i2];
            struct zr_vec2 diff = zr_vec2_sub(p2, p1);
            float len;

            /* vec2 inverted lenth  */
            len = zr_vec2_len_sqr(diff);
            if (len != 0.0f)
                len = zr_inv_sqrt(len);
            else len = 1.0f;
            diff = zr_vec2_muls(diff, len);

            /* add vertexes */
            dx = diff.x * (thickness * 0.5f);
            dy = diff.y * (thickness * 0.5f);

            vtx[0] = zr_draw_vertex(zr_vec2(p1.x + dy, p1.y - dx), uv, col);
            vtx[1] = zr_draw_vertex(zr_vec2(p2.x + dy, p2.y - dx), uv, col);
            vtx[2] = zr_draw_vertex(zr_vec2(p2.x - dy, p2.y + dx), uv, col);
            vtx[3] = zr_draw_vertex(zr_vec2(p1.x - dy, p1.y + dx), uv, col);
            vtx += 4;

            ids[0] = (zr_draw_index)(idx+0); ids[1] = (zr_draw_index)(idx+1);
            ids[2] = (zr_draw_index)(idx+2); ids[3] = (zr_draw_index)(idx+0);
            ids[4] = (zr_draw_index)(idx+2); ids[5] = (zr_draw_index)(idx+3);
            ids += 6;
            idx += 4;
        }
    }
}

static void
zr_draw_list_add_poly_convex(struct zr_draw_list *list, struct zr_vec2 *points,
    const unsigned int points_count, struct zr_color color, enum zr_anti_aliasing aliasing)
{
    static const zr_size pnt_align = ZR_ALIGNOF(struct zr_vec2);
    static const zr_size pnt_size = sizeof(struct zr_vec2);
    zr_draw_vertex_color col = zr_color32(color);
    ZR_ASSERT(list);
    if (!list || points_count < 3) return;

    if (aliasing == ZR_ANTI_ALIASING_ON) {
        zr_size i = 0;
        zr_size i0 = 0, i1 = 0;

        const float AA_SIZE = 1.0f;
        const zr_draw_vertex_color col_trans = col & 0x00ffffff;
        zr_size index = list->vertex_count;
        const zr_size idx_count = (points_count-2)*3 + points_count*6;
        const zr_size vtx_count = (points_count*2);
        struct zr_draw_vertex *vtx = zr_draw_list_alloc_vertexes(list, vtx_count);
        zr_draw_index *ids = zr_draw_list_alloc_elements(list, idx_count);

        unsigned int vtx_inner_idx = (unsigned int)(index + 0);
        unsigned int vtx_outer_idx = (unsigned int)(index + 1);
        struct zr_vec2 *normals = 0;
        zr_size size = 0;
        if (!vtx || !ids) return;

        /* temporary allocate normals */
        zr_buffer_mark(list->vertexes, ZR_BUFFER_FRONT);
        size = pnt_size * points_count;
        normals = (struct zr_vec2*)
            zr_buffer_alloc(list->vertexes, ZR_BUFFER_FRONT, size, pnt_align);
        if (!normals) return;

        /* add elements */
        for (i = 2; i < points_count; i++) {
            ids[0] = (zr_draw_index)(vtx_inner_idx);
            ids[1] = (zr_draw_index)(vtx_inner_idx + ((i-1) << 1));
            ids[2] = (zr_draw_index)(vtx_inner_idx + (i << 1));
            ids += 3;
        }

        /* compute normals */
        for (i0 = points_count-1, i1 = 0; i1 < points_count; i0 = i1++) {
            struct zr_vec2 p0 = points[i0];
            struct zr_vec2 p1 = points[i1];
            struct zr_vec2 diff = zr_vec2_sub(p1, p0);

            /* vec2 inverted lenth  */
            float len = zr_vec2_len_sqr(diff);
            if (len != 0.0f)
                len = zr_inv_sqrt(len);
            else len = 1.0f;
            diff = zr_vec2_muls(diff, len);

            normals[i0].x = diff.y;
            normals[i0].y = -diff.x;
        }

        /* add vertexes + indexes */
        for (i0 = points_count-1, i1 = 0; i1 < points_count; i0 = i1++) {
            const struct zr_vec2 uv = list->null.uv;
            struct zr_vec2 n0 = normals[i0];
            struct zr_vec2 n1 = normals[i1];
            struct zr_vec2 dm = zr_vec2_muls(zr_vec2_add(n0, n1), 0.5f);

            float dmr2 = dm.x*dm.x + dm.y*dm.y;
            if (dmr2 > 0.000001f) {
                float scale = 1.0f / dmr2;
                scale = MIN(scale, 100.0f);
                dm = zr_vec2_muls(dm, scale);
            }
            dm = zr_vec2_muls(dm, AA_SIZE * 0.5f);

            /* add vertexes */
            vtx[0] = zr_draw_vertex(zr_vec2_sub(points[i1], dm), uv, col);
            vtx[1] = zr_draw_vertex(zr_vec2_add(points[i1], dm), uv, col_trans);
            vtx += 2;

            /* add indexes */
            ids[0] = (zr_draw_index)(vtx_inner_idx+(i1<<1));
            ids[1] = (zr_draw_index)(vtx_inner_idx+(i0<<1));
            ids[2] = (zr_draw_index)(vtx_outer_idx+(i0<<1));
            ids[3] = (zr_draw_index)(vtx_outer_idx+(i0<<1));
            ids[4] = (zr_draw_index)(vtx_outer_idx+(i1<<1));
            ids[5] = (zr_draw_index)(vtx_inner_idx+(i1<<1));
            ids += 6;
        }
        /* free temporary normals + points */
        zr_buffer_reset(list->vertexes, ZR_BUFFER_FRONT);
    } else {
        zr_size i = 0;
        zr_size index = list->vertex_count;
        const zr_size idx_count = (points_count-2)*3;
        const zr_size vtx_count = points_count;
        struct zr_draw_vertex *vtx = zr_draw_list_alloc_vertexes(list, vtx_count);
        zr_draw_index *ids = zr_draw_list_alloc_elements(list, idx_count);
        if (!vtx || !ids) return;
        for (i = 0; i < vtx_count; ++i) {
            vtx[0] = zr_draw_vertex(points[i], list->null.uv, col);
            vtx++;
        }
        for (i = 2; i < points_count; ++i) {
            ids[0] = (zr_draw_index)index;
            ids[1] = (zr_draw_index)(index+ i - 1);
            ids[2] = (zr_draw_index)(index+i);
            ids += 3;
        }
    }
}

void
zr_draw_list_add_line(struct zr_draw_list *list, struct zr_vec2 a,
    struct zr_vec2 b, struct zr_color col, float thickness)
{
    ZR_ASSERT(list);
    if (!list || !col.a) return;
    zr_draw_list_path_line_to(list, zr_vec2_add(a, zr_vec2(0.5f, 0.5f)));
    zr_draw_list_path_line_to(list, zr_vec2_add(b, zr_vec2(0.5f, 0.5f)));
    zr_draw_list_path_stroke(list,  col, ZR_STROKE_OPEN, thickness);
}

void
zr_draw_list_add_rect(struct zr_draw_list *list, struct zr_rect rect,
    struct zr_color col, float rounding, float line_thickness)
{
    ZR_ASSERT(list);
    if (!list || !col.a) return;
    zr_draw_list_path_rect_to(list, zr_vec2(rect.x + 0.5f, rect.y + 0.5f),
        zr_vec2(rect.x + rect.w + 0.5f, rect.y + rect.h + 0.5f), rounding);
    zr_draw_list_path_stroke(list,  col, ZR_STROKE_CLOSED, line_thickness);
}

void
zr_draw_list_add_rect_filled(struct zr_draw_list *list, struct zr_rect rect,
    struct zr_color col, float rounding)
{
    ZR_ASSERT(list);
    if (!list || !col.a) return;
    zr_draw_list_path_rect_to(list, zr_vec2(rect.x + 0.5f, rect.y + 0.5f),
        zr_vec2(rect.x + rect.w + 0.5f, rect.y + rect.h + 0.5f), rounding);
    zr_draw_list_path_fill(list,  col);
}

void
zr_draw_list_add_triangle(struct zr_draw_list *list, struct zr_vec2 a, struct zr_vec2 b,
    struct zr_vec2 c, struct zr_color col, float line_thickness)
{
    ZR_ASSERT(list);
    if (!list) return;
    if (!col.a) return;
    zr_draw_list_path_line_to(list, a);
    zr_draw_list_path_line_to(list, b);
    zr_draw_list_path_line_to(list, c);
    zr_draw_list_path_stroke(list, col, ZR_STROKE_CLOSED, line_thickness);
}

void
zr_draw_list_add_triangle_filled(struct zr_draw_list *list, struct zr_vec2 a,
    struct zr_vec2 b, struct zr_vec2 c, struct zr_color col)
{
    ZR_ASSERT(list);
    if (!list || !col.a) return;
    zr_draw_list_path_line_to(list, a);
    zr_draw_list_path_line_to(list, b);
    zr_draw_list_path_line_to(list, c);
    zr_draw_list_path_fill(list, col);
}

void
zr_draw_list_add_circle(struct zr_draw_list *list, struct zr_vec2 center,
    float radius, struct zr_color col, unsigned int segs, float line_thickness)
{
    float a_max;
    ZR_ASSERT(list);
    if (!list || !col.a) return;
    a_max = ZR_PI * 2.0f * ((float)segs - 1.0f) / (float)segs;
    zr_draw_list_path_arc_to(list, center, radius, 0.0f, a_max, segs);
    zr_draw_list_path_stroke(list, col, ZR_STROKE_CLOSED, line_thickness);
}

void
zr_draw_list_add_circle_filled(struct zr_draw_list *list, struct zr_vec2 center,
    float radius, struct zr_color col, unsigned int segs)
{
    float a_max;
    ZR_ASSERT(list);
    if (!list || !col.a) return;
    a_max = ZR_PI * 2.0f * ((float)segs - 1.0f) / (float)segs;
    zr_draw_list_path_arc_to(list, center, radius, 0.0f, a_max, segs);
    zr_draw_list_path_fill(list, col);
}

void
zr_draw_list_add_curve(struct zr_draw_list *list, struct zr_vec2 p0,
    struct zr_vec2 cp0, struct zr_vec2 cp1, struct zr_vec2 p1,
    struct zr_color col, unsigned int segments, float thickness)
{
    ZR_ASSERT(list);
    if (!list || !col.a) return;
    zr_draw_list_path_line_to(list, p0);
    zr_draw_list_path_curve_to(list, cp0, cp1, p1, segments);
    zr_draw_list_path_stroke(list, col, ZR_STROKE_OPEN, thickness);
}

static void
zr_draw_list_push_rect_uv(struct zr_draw_list *list, struct zr_vec2 a,
    struct zr_vec2 c, struct zr_vec2 uva, struct zr_vec2 uvc, struct zr_color color)
{
    zr_draw_vertex_color col = zr_color32(color);
    struct zr_draw_vertex *vtx;
    struct zr_vec2 uvb, uvd;
    struct zr_vec2 b,d;
    zr_draw_index *idx;
    zr_draw_index index;
    ZR_ASSERT(list);
    if (!list) return;

    uvb = zr_vec2(uvc.x, uva.y);
    uvd = zr_vec2(uva.x, uvc.y);
    b = zr_vec2(c.x, a.y);
    d = zr_vec2(a.x, c.y);

    index = (zr_draw_index)list->vertex_count;
    vtx = zr_draw_list_alloc_vertexes(list, 4);
    idx = zr_draw_list_alloc_elements(list, 6);
    if (!vtx || !idx) return;

    idx[0] = (zr_draw_index)(index+0); idx[1] = (zr_draw_index)(index+1);
    idx[2] = (zr_draw_index)(index+2); idx[3] = (zr_draw_index)(index+0);
    idx[4] = (zr_draw_index)(index+2); idx[5] = (zr_draw_index)(index+3);

    vtx[0] = zr_draw_vertex(a, uva, col);
    vtx[1] = zr_draw_vertex(b, uvb, col);
    vtx[2] = zr_draw_vertex(c, uvc, col);
    vtx[3] = zr_draw_vertex(d, uvd, col);
}

void
zr_draw_list_add_image(struct zr_draw_list *list, struct zr_image texture,
    struct zr_rect rect, struct zr_color color)
{
    ZR_ASSERT(list);
    if (!list) return;
    /* push new command with given texture */
    zr_draw_list_push_image(list, texture.handle);
    if (zr_image_is_subimage(&texture)) {
        /* add region inside of the texture  */
        struct zr_vec2 uv[2];
        uv[0].x = (float)texture.region[0]/(float)texture.w;
        uv[0].y = (float)texture.region[1]/(float)texture.h;
        uv[1].x = (float)(texture.region[0] + texture.region[2])/(float)texture.w;
        uv[1].y = (float)(texture.region[1] + texture.region[3])/(float)texture.h;
        zr_draw_list_push_rect_uv(list, zr_vec2(rect.x, rect.y),
            zr_vec2(rect.x + rect.w, rect.y + rect.h),  uv[0], uv[1], color);
    } else zr_draw_list_push_rect_uv(list, zr_vec2(rect.x, rect.y),
            zr_vec2(rect.x + rect.w, rect.y + rect.h),
            zr_vec2(0.0f, 0.0f), zr_vec2(1.0f, 1.0f),color);
}

void
zr_draw_list_add_text(struct zr_draw_list *list, const struct zr_user_font *font,
    struct zr_rect rect, const char *text, zr_size len, float font_height,
    struct zr_color bg, struct zr_color fg)
{
    float x, scale;
    zr_size text_len;
    zr_rune unicode, next;
    zr_size glyph_len, next_glyph_len;
    struct zr_user_font_glyph g;
    scale = font_height / font->height;

    ZR_ASSERT(list);
    if (!list || !len || !text) return;
    if (rect.x > (list->clip_rect.x + list->clip_rect.w) ||
        rect.y > (list->clip_rect.y + list->clip_rect.h) ||
        rect.x < list->clip_rect.x || rect.y < list->clip_rect.y)
        return;

    /* draw text background */
    zr_draw_list_add_rect_filled(list, rect, bg, 0.0f);
    zr_draw_list_push_image(list, font->texture);

    /* draw every glyph image */
    x = rect.x;
    glyph_len = text_len = zr_utf_decode(text, &unicode, len);
    if (!glyph_len) return;
    while (text_len <= len && glyph_len) {
        float gx, gy, gh, gw;
        float char_width = 0;
        if (unicode == ZR_UTF_INVALID) break;

        /* query currently drawn glyph information */
        next_glyph_len = zr_utf_decode(text + text_len, &next, len - text_len);
        font->query(font->userdata, font->height, &g, unicode, (next == ZR_UTF_INVALID) ? '\0' : next);

        /* calculate and draw glyph drawing rectangle and image */
        gx = x + g.offset.x * scale;
        gy = rect.y + (rect.h/2) - (font->height/2) + g.offset.y * scale;
        gw = g.width * scale; gh = g.height * scale;
        char_width = g.xadvance * scale;
        zr_draw_list_push_rect_uv(list, zr_vec2(gx,gy), zr_vec2(gx + gw, gy+ gh),
            g.uv[0], g.uv[1], fg);

        /* offset next glyph */
        text_len += glyph_len;
        x += char_width;
        glyph_len = next_glyph_len;
        unicode = next;
    }
}

void
zr_draw_list_path_clear(struct zr_draw_list *list)
{
    ZR_ASSERT(list);
    if (!list) return;
    zr_buffer_reset(list->buffer, ZR_BUFFER_FRONT);
    list->path_count = 0;
    list->path_offset = 0;
}

void
zr_draw_list_path_line_to(struct zr_draw_list *list, struct zr_vec2 pos)
{
    struct zr_vec2 *points;
    struct zr_draw_command *cmd;
    ZR_ASSERT(list);
    if (!list) return;
    if (!list->cmd_count)
        zr_draw_list_add_clip(list, zr_null_rect);

    cmd = zr_draw_list_command_last(list);
    if (cmd->texture.ptr != list->null.texture.ptr)
        zr_draw_list_push_image(list, list->null.texture);

    points = zr_draw_list_alloc_path(list, 1);
    if (!points) return;
    points[0] = pos;
}

static void
zr_draw_list_path_arc_to_fast(struct zr_draw_list *list, struct zr_vec2 center,
    float radius, int a_min, int a_max)
{
    static struct zr_vec2 circle_vtx[12];
    static int circle_vtx_builds = zr_false;
    static const int circle_vtx_count = ZR_LEN(circle_vtx);

    ZR_ASSERT(list);
    if (!list) return;
    if (!circle_vtx_builds) {
        int i = 0;
        for (i = 0; i < circle_vtx_count; ++i) {
            const float a = ((float)i / (float)circle_vtx_count) * 2 * ZR_PI;
            circle_vtx[i].x = list->cos(a);
            circle_vtx[i].y = list->sin(a);
        }
        circle_vtx_builds = zr_true;
    }

    if (a_min <= a_max) {
        int a = 0;
        for (a = a_min; a <= a_max; a++) {
            const struct zr_vec2 c = circle_vtx[a % circle_vtx_count];
            const float x = center.x + c.x * radius;
            const float y = center.y + c.y * radius;
            zr_draw_list_path_line_to(list, zr_vec2(x, y));
        }
    }
}

void
zr_draw_list_path_arc_to(struct zr_draw_list *list, struct zr_vec2 center,
    float radius, float a_min, float a_max, unsigned int segments)
{
    unsigned int i = 0;
    ZR_ASSERT(list);
    if (!list) return;
    if (radius == 0.0f) return;
    for (i = 0; i <= segments; ++i) {
        const float a = a_min + ((float)i / (float)segments) * (a_max - a_min);
        const float x = center.x + list->cos(a) * radius;
        const float y = center.y + list->sin(a) * radius;
        zr_draw_list_path_line_to(list, zr_vec2(x, y));
    }
}

void
zr_draw_list_path_rect_to(struct zr_draw_list *list, struct zr_vec2 a,
    struct zr_vec2 b, float rounding)
{
    float r;
    ZR_ASSERT(list);
    if (!list) return;
    r = rounding;
    r = MIN(r, ((b.x-a.x) < 0) ? -(b.x-a.x): (b.x-a.x));
    r = MIN(r, ((b.y-a.y) < 0) ? -(b.y-a.y): (b.y-a.y));

    if (r == 0.0f) {
        zr_draw_list_path_line_to(list, a);
        zr_draw_list_path_line_to(list, zr_vec2(b.x,a.y));
        zr_draw_list_path_line_to(list, b);
        zr_draw_list_path_line_to(list, zr_vec2(a.x,b.y));
    } else {
        zr_draw_list_path_arc_to_fast(list, zr_vec2(a.x + r, a.y + r), r, 6, 9);
        zr_draw_list_path_arc_to_fast(list, zr_vec2(b.x - r, a.y + r), r, 9, 12);
        zr_draw_list_path_arc_to_fast(list, zr_vec2(b.x - r, b.y - r), r, 0, 3);
        zr_draw_list_path_arc_to_fast(list, zr_vec2(a.x + r, b.y - r), r, 3, 6);
    }
}

void
zr_draw_list_path_curve_to(struct zr_draw_list *list, struct zr_vec2 p2,
    struct zr_vec2 p3, struct zr_vec2 p4, unsigned int num_segments)
{
    unsigned int i_step;
    float t_step;
    struct zr_vec2 p1;

    ZR_ASSERT(list);
    ZR_ASSERT(list->path_count);
    if (!list || !list->path_count) return;
    num_segments = MAX(num_segments, 1);

    p1 = zr_draw_list_path_last(list);
    t_step = 1.0f/(float)num_segments;
    for (i_step = 1; i_step <= num_segments; ++i_step) {
        float t = t_step * (float)i_step;
        float u = 1.0f - t;
        float w1 = u*u*u;
        float w2 = 3*u*u*t;
        float w3 = 3*u*t*t;
        float w4 = t * t *t;
        float x = w1 * p1.x + w2 * p2.x + w3 * p3.x + w4 * p4.x;
        float y = w1 * p1.y + w2 * p2.y + w3 * p3.y + w4 * p4.y;
        zr_draw_list_path_line_to(list, zr_vec2(x,y));
    }
}

void
zr_draw_list_path_fill(struct zr_draw_list *list, struct zr_color color)
{
    struct zr_vec2 *points;
    ZR_ASSERT(list);
    if (!list) return;
    points = (struct zr_vec2*)zr_buffer_memory(list->buffer);
    zr_draw_list_add_poly_convex(list, points, list->path_count, color, list->AA);
    zr_draw_list_path_clear(list);
}

void
zr_draw_list_path_stroke(struct zr_draw_list *list, struct zr_color color,
    int closed, float thickness)
{
    struct zr_vec2 *points;
    ZR_ASSERT(list);
    if (!list) return;
    points = (struct zr_vec2*)zr_buffer_memory(list->buffer);
    zr_draw_list_add_poly_line(list, points, list->path_count, color,
        closed, thickness, list->AA);
    zr_draw_list_path_clear(list);
}
#endif
/*
 * ==============================================================
 *
 *                          Font
 *
 * ===============================================================
 */
#ifdef ZR_COMPILE_WITH_FONT

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

#ifndef ZR_DISABLE_STB_RECT_PACK_IMPLEMENTATION
#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#endif
#include "stb_rect_pack.h"

#ifndef ZR_DISABLE_STB_TRUETYPE_IMPLEMENTATION
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

struct zr_font_bake_data {
    stbtt_fontinfo info;
    stbrp_rect *rects;
    stbtt_pack_range *ranges;
    zr_rune range_count;
};

struct zr_font_baker {
    stbtt_pack_context spc;
    struct zr_font_bake_data *build;
    stbtt_packedchar *packed_chars;
    stbrp_rect *rects;
    stbtt_pack_range *ranges;
};

static const zr_size zr_rect_align = ZR_ALIGNOF(stbrp_rect);
static const zr_size zr_range_align = ZR_ALIGNOF(stbtt_pack_range);
static const zr_size char_align = ZR_ALIGNOF(stbtt_packedchar);
static const zr_size zr_build_align = ZR_ALIGNOF(struct zr_font_bake_data);
static const zr_size zr_baker_align = ZR_ALIGNOF(struct zr_font_baker);

static int
zr_range_count(const zr_rune *range)
{
    const zr_rune *iter = range;
    ZR_ASSERT(range);
    if (!range) return 0;
    while (*(iter++) != 0);
    return (iter == range) ? 0 : (int)((iter - range)/2);
}

static int
zr_range_glyph_count(const zr_rune *range, int count)
{
    int i = 0;
    int total_glyphes = 0;
    for (i = 0; i < count; ++i) {
        int diff;
        zr_rune f = range[(i*2)+0];
        zr_rune t = range[(i*2)+1];
        ZR_ASSERT(t >= f);
        diff = (int)((t - f) + 1);
        total_glyphes += diff;
    }
    return total_glyphes;
}

const zr_rune*
zr_font_default_glyph_ranges(void)
{
    static const zr_rune ranges[] = {0x0020, 0x00FF, 0};
    return ranges;
}

const zr_rune*
zr_font_chinese_glyph_ranges(void)
{
    static const zr_rune ranges[] = {
        0x0020, 0x00FF,
        0x3000, 0x30FF,
        0x31F0, 0x31FF,
        0xFF00, 0xFFEF,
        0x4e00, 0x9FAF,
        0
    };
    return ranges;
}

const zr_rune*
zr_font_cyrillic_glyph_ranges(void)
{
    static const zr_rune ranges[] = {
        0x0020, 0x00FF,
        0x0400, 0x052F,
        0x2DE0, 0x2DFF,
        0xA640, 0xA69F,
        0
    };
    return ranges;
}

const zr_rune*
zr_font_korean_glyph_ranges(void)
{
    static const zr_rune ranges[] = {
        0x0020, 0x00FF,
        0x3131, 0x3163,
        0xAC00, 0xD79D,
        0
    };
    return ranges;
}

const zr_rune*
zr_font_japanese_glyph_range(void)
{
    static const short offsets_from_0x4E00[] = {
        -1,0,1,3,0,0,0,0,1,0,5,1,1,0,7,4,6,10,0,1,9,9,7,1,3,19,1,10,7,1,0,1,0,5,1,0,6,4,2,6,0,0,12,6,8,0,3,5,0,1,0,9,0,0,8,1,1,3,4,5,13,0,0,8,2,17,
        4,3,1,1,9,6,0,0,0,2,1,3,2,22,1,9,11,1,13,1,3,12,0,5,9,2,0,6,12,5,3,12,4,1,2,16,1,1,4,6,5,3,0,6,13,15,5,12,8,14,0,0,6,15,3,6,0,18,8,1,6,14,1,
        5,4,12,24,3,13,12,10,24,0,0,0,1,0,1,1,2,9,10,2,2,0,0,3,3,1,0,3,8,0,3,2,4,4,1,6,11,10,14,6,15,3,4,15,1,0,0,5,2,2,0,0,1,6,5,5,6,0,3,6,5,0,0,1,0,
        11,2,2,8,4,7,0,10,0,1,2,17,19,3,0,2,5,0,6,2,4,4,6,1,1,11,2,0,3,1,2,1,2,10,7,6,3,16,0,8,24,0,0,3,1,1,3,0,1,6,0,0,0,2,0,1,5,15,0,1,0,0,2,11,19,
        1,4,19,7,6,5,1,0,0,0,0,5,1,0,1,9,0,0,5,0,2,0,1,0,3,0,11,3,0,2,0,0,0,0,0,9,3,6,4,12,0,14,0,0,29,10,8,0,14,37,13,0,31,16,19,0,8,30,1,20,8,3,48,
        21,1,0,12,0,10,44,34,42,54,11,18,82,0,2,1,2,12,1,0,6,2,17,2,12,7,0,7,17,4,2,6,24,23,8,23,39,2,16,23,1,0,5,1,2,15,14,5,6,2,11,0,8,6,2,2,2,14,
        20,4,15,3,4,11,10,10,2,5,2,1,30,2,1,0,0,22,5,5,0,3,1,5,4,1,0,0,2,2,21,1,5,1,2,16,2,1,3,4,0,8,4,0,0,5,14,11,2,16,1,13,1,7,0,22,15,3,1,22,7,14,
        22,19,11,24,18,46,10,20,64,45,3,2,0,4,5,0,1,4,25,1,0,0,2,10,0,0,0,1,0,1,2,0,0,9,1,2,0,0,0,2,5,2,1,1,5,5,8,1,1,1,5,1,4,9,1,3,0,1,0,1,1,2,0,0,
        2,0,1,8,22,8,1,0,0,0,0,4,2,1,0,9,8,5,0,9,1,30,24,2,6,4,39,0,14,5,16,6,26,179,0,2,1,1,0,0,0,5,2,9,6,0,2,5,16,7,5,1,1,0,2,4,4,7,15,13,14,0,0,
        3,0,1,0,0,0,2,1,6,4,5,1,4,9,0,3,1,8,0,0,10,5,0,43,0,2,6,8,4,0,2,0,0,9,6,0,9,3,1,6,20,14,6,1,4,0,7,2,3,0,2,0,5,0,3,1,0,3,9,7,0,3,4,0,4,9,1,6,0,
        9,0,0,2,3,10,9,28,3,6,2,4,1,2,32,4,1,18,2,0,3,1,5,30,10,0,2,2,2,0,7,9,8,11,10,11,7,2,13,7,5,10,0,3,40,2,0,1,6,12,0,4,5,1,5,11,11,21,4,8,3,7,
        8,8,33,5,23,0,0,19,8,8,2,3,0,6,1,1,1,5,1,27,4,2,5,0,3,5,6,3,1,0,3,1,12,5,3,3,2,0,7,7,2,1,0,4,0,1,1,2,0,10,10,6,2,5,9,7,5,15,15,21,6,11,5,20,
        4,3,5,5,2,5,0,2,1,0,1,7,28,0,9,0,5,12,5,5,18,30,0,12,3,3,21,16,25,32,9,3,14,11,24,5,66,9,1,2,0,5,9,1,5,1,8,0,8,3,3,0,1,15,1,4,8,1,2,7,0,7,2,
        8,3,7,5,3,7,10,2,1,0,0,2,25,0,6,4,0,10,0,4,2,4,1,12,5,38,4,0,4,1,10,5,9,4,0,14,4,2,5,18,20,21,1,3,0,5,0,7,0,3,7,1,3,1,1,8,1,0,0,0,3,2,5,2,11,
        6,0,13,1,3,9,1,12,0,16,6,2,1,0,2,1,12,6,13,11,2,0,28,1,7,8,14,13,8,13,0,2,0,5,4,8,10,2,37,42,19,6,6,7,4,14,11,18,14,80,7,6,0,4,72,12,36,27,
        7,7,0,14,17,19,164,27,0,5,10,7,3,13,6,14,0,2,2,5,3,0,6,13,0,0,10,29,0,4,0,3,13,0,3,1,6,51,1,5,28,2,0,8,0,20,2,4,0,25,2,10,13,10,0,16,4,0,1,0,
        2,1,7,0,1,8,11,0,0,1,2,7,2,23,11,6,6,4,16,2,2,2,0,22,9,3,3,5,2,0,15,16,21,2,9,20,15,15,5,3,9,1,0,0,1,7,7,5,4,2,2,2,38,24,14,0,0,15,5,6,24,14,
        5,5,11,0,21,12,0,3,8,4,11,1,8,0,11,27,7,2,4,9,21,59,0,1,39,3,60,62,3,0,12,11,0,3,30,11,0,13,88,4,15,5,28,13,1,4,48,17,17,4,28,32,46,0,16,0,
        18,11,1,8,6,38,11,2,6,11,38,2,0,45,3,11,2,7,8,4,30,14,17,2,1,1,65,18,12,16,4,2,45,123,12,56,33,1,4,3,4,7,0,0,0,3,2,0,16,4,2,4,2,0,7,4,5,2,26,
        2,25,6,11,6,1,16,2,6,17,77,15,3,35,0,1,0,5,1,0,38,16,6,3,12,3,3,3,0,9,3,1,3,5,2,9,0,18,0,25,1,3,32,1,72,46,6,2,7,1,3,14,17,0,28,1,40,13,0,20,
        15,40,6,38,24,12,43,1,1,9,0,12,6,0,6,2,4,19,3,7,1,48,0,9,5,0,5,6,9,6,10,15,2,11,19,3,9,2,0,1,10,1,27,8,1,3,6,1,14,0,26,0,27,16,3,4,9,6,2,23,
        9,10,5,25,2,1,6,1,1,48,15,9,15,14,3,4,26,60,29,13,37,21,1,6,4,0,2,11,22,23,16,16,2,2,1,3,0,5,1,6,4,0,0,4,0,0,8,3,0,2,5,0,7,1,7,3,13,2,4,10,
        3,0,2,31,0,18,3,0,12,10,4,1,0,7,5,7,0,5,4,12,2,22,10,4,2,15,2,8,9,0,23,2,197,51,3,1,1,4,13,4,3,21,4,19,3,10,5,40,0,4,1,1,10,4,1,27,34,7,21,
        2,17,2,9,6,4,2,3,0,4,2,7,8,2,5,1,15,21,3,4,4,2,2,17,22,1,5,22,4,26,7,0,32,1,11,42,15,4,1,2,5,0,19,3,1,8,6,0,10,1,9,2,13,30,8,2,24,17,19,1,4,
        4,25,13,0,10,16,11,39,18,8,5,30,82,1,6,8,18,77,11,13,20,75,11,112,78,33,3,0,0,60,17,84,9,1,1,12,30,10,49,5,32,158,178,5,5,6,3,3,1,3,1,4,7,6,
        19,31,21,0,2,9,5,6,27,4,9,8,1,76,18,12,1,4,0,3,3,6,3,12,2,8,30,16,2,25,1,5,5,4,3,0,6,10,2,3,1,0,5,1,19,3,0,8,1,5,2,6,0,0,0,19,1,2,0,5,1,2,5,
        1,3,7,0,4,12,7,3,10,22,0,9,5,1,0,2,20,1,1,3,23,30,3,9,9,1,4,191,14,3,15,6,8,50,0,1,0,0,4,0,0,1,0,2,4,2,0,2,3,0,2,0,2,2,8,7,0,1,1,1,3,3,17,11,
        91,1,9,3,2,13,4,24,15,41,3,13,3,1,20,4,125,29,30,1,0,4,12,2,21,4,5,5,19,11,0,13,11,86,2,18,0,7,1,8,8,2,2,22,1,2,6,5,2,0,1,2,8,0,2,0,5,2,1,0,
        2,10,2,0,5,9,2,1,2,0,1,0,4,0,0,10,2,5,3,0,6,1,0,1,4,4,33,3,13,17,3,18,6,4,7,1,5,78,0,4,1,13,7,1,8,1,0,35,27,15,3,0,0,0,1,11,5,41,38,15,22,6,
        14,14,2,1,11,6,20,63,5,8,27,7,11,2,2,40,58,23,50,54,56,293,8,8,1,5,1,14,0,1,12,37,89,8,8,8,2,10,6,0,0,0,4,5,2,1,0,1,1,2,7,0,3,3,0,4,6,0,3,2,
        19,3,8,0,0,0,4,4,16,0,4,1,5,1,3,0,3,4,6,2,17,10,10,31,6,4,3,6,10,126,7,3,2,2,0,9,0,0,5,20,13,0,15,0,6,0,2,5,8,64,50,3,2,12,2,9,0,0,11,8,20,
        109,2,18,23,0,0,9,61,3,0,28,41,77,27,19,17,81,5,2,14,5,83,57,252,14,154,263,14,20,8,13,6,57,39,38,
    };
    static int ranges_unpacked;
    static zr_rune ranges[8 + ZR_LEN(offsets_from_0x4E00)*2 + 1] = {
        0x0020, 0x00FF, /* Basic Latin + Latin Supplement */
        0x3000, 0x30FF, /* Punctuations, Hiragana, Katakana */
        0x31F0, 0x31FF, /* Katakana Phonetic Extensions */
        0xFF00, 0xFFEF, /* Half-width characters */
    };
    if (!ranges_unpacked)
    {
        zr_size n = 0;
        zr_rune codepoint = 0x4e00;
        zr_rune* dst = &ranges[8];
        for (n = 0; n < ZR_LEN(offsets_from_0x4E00); n++, dst += 2)
            dst[0] = dst[1] = (zr_rune)(codepoint += (zr_rune)(offsets_from_0x4E00[n] + 1));
        dst[0] = 0;
        ranges_unpacked = 1;
    }
    return &ranges[0];
}

void zr_font_bake_memory(zr_size *temp, int *glyph_count,
    struct zr_font_config *config, int count)
{
    int i, range_count = 0;

    ZR_ASSERT(config);
    ZR_ASSERT(glyph_count);
    if (!config) {
        *temp = 0;
        *glyph_count = 0;
        return;
    }

    *glyph_count = 0;
    if (!config->range)
        config->range = zr_font_default_glyph_ranges();
    for (i = 0; i < count; ++i) {
        range_count += zr_range_count(config[i].range);
        *glyph_count += zr_range_glyph_count(config[i].range, range_count);
    }

    *temp = (zr_size)*glyph_count * sizeof(stbrp_rect);
    *temp += (zr_size)range_count * sizeof(stbtt_pack_range);
    *temp += (zr_size)*glyph_count * sizeof(stbtt_packedchar);
    *temp += (zr_size)count * sizeof(struct zr_font_bake_data);
    *temp += sizeof(struct zr_font_baker);
    *temp += zr_rect_align + zr_range_align + char_align;
    *temp += zr_build_align + zr_baker_align;
}

static struct zr_font_baker*
zr_font_baker(void *memory, int glyph_count, int count)
{
    struct zr_font_baker *baker;
    if (!memory) return 0;
    /* setup baker inside a memory block  */
    baker = (struct zr_font_baker*)ZR_ALIGN_PTR(memory, zr_baker_align);
    baker->build = (struct zr_font_bake_data*)ZR_ALIGN_PTR((baker + 1), zr_build_align);
    baker->packed_chars = (stbtt_packedchar*)ZR_ALIGN_PTR((baker->build + count), char_align);
    baker->rects = (stbrp_rect*)ZR_ALIGN_PTR((baker->packed_chars + glyph_count), zr_rect_align);
    baker->ranges = (stbtt_pack_range*)ZR_ALIGN_PTR((baker->rects + glyph_count), zr_range_align);
    return baker;
}

int
zr_font_bake_pack(zr_size *image_memory, int *width, int *height,
    struct zr_recti *custom, void *temp, zr_size temp_size,
    const struct zr_font_config *config, int count)
{
    static const zr_size max_height = 1024 * 32;
    struct zr_font_baker* baker;
    int total_glyph_count = 0;
    int total_range_count = 0;
    int i = 0;

    ZR_ASSERT(image_memory);
    ZR_ASSERT(width);
    ZR_ASSERT(height);
    ZR_ASSERT(config);
    ZR_ASSERT(temp);
    ZR_ASSERT(temp_size);
    ZR_ASSERT(count);
    if (!image_memory || !width || !height || !config || !temp ||
        !temp_size || !count) return zr_false;

    for (i = 0; i < count; ++i) {
        total_range_count += zr_range_count(config[i].range);
        total_glyph_count += zr_range_glyph_count(config[i].range, total_range_count);
    }

    /* setup font baker from temporary memory */
    zr_zero(temp, temp_size);
    baker = zr_font_baker(temp, total_glyph_count, count);
    if (!baker) return zr_false;
    for (i = 0; i < count; ++i) {
        const struct zr_font_config *cfg = &config[i];
        if (!stbtt_InitFont(&baker->build[i].info, (const unsigned char*)cfg->ttf_blob, 0))
            return zr_false;
    }

    *height = 0;
    *width = (total_glyph_count > 1000) ? 1024 : 512;
    stbtt_PackBegin(&baker->spc, 0, (int)*width, (int)max_height, 0, 1, 0);
    {
        int input_i = 0;
        int range_n = 0, rect_n = 0, char_n = 0;

        /* pack custom user data first so it will be in the upper left corner  */
        if (custom) {
            stbrp_rect custom_space;
            zr_zero(&custom_space, sizeof(custom_space));
            custom_space.w = (stbrp_coord)((custom->w * 2) + 1);
            custom_space.h = (stbrp_coord)(custom->h + 1);

            stbtt_PackSetOversampling(&baker->spc, 1, 1);
            stbrp_pack_rects((stbrp_context*)baker->spc.pack_info, &custom_space, 1);
            *height = MAX(*height, (int)(custom_space.y + custom_space.h));

            custom->x = (short)custom_space.x;
            custom->y = (short)custom_space.y;
            custom->w = (short)custom_space.w;
            custom->h = (short)custom_space.h;
        }

        /* first font pass: pack all glyphes */
        for (input_i = 0; input_i < count; input_i++) {
            int n = 0;
            const zr_rune *in_range;
            const struct zr_font_config *cfg = &config[input_i];
            struct zr_font_bake_data *tmp = &baker->build[input_i];
            int glyph_count, range_count;

            /* count glyphes + ranges in current font */
            glyph_count = 0; range_count = 0;
            for (in_range = cfg->range; in_range[0] && in_range[1]; in_range += 2) {
                glyph_count += (int)(in_range[1] - in_range[0]) + 1;
                range_count++;
            }

            /* setup ranges  */
            tmp->ranges = baker->ranges + range_n;
            tmp->range_count = (zr_rune)range_count;
            range_n += range_count;
            for (i = 0; i < range_count; ++i) {
                in_range = &cfg->range[i * 2];
                tmp->ranges[i].font_size = cfg->size;
                tmp->ranges[i].first_unicode_codepoint_in_range = (int)in_range[0];
                tmp->ranges[i].num_chars = (int)(in_range[1]- in_range[0]) + 1;
                tmp->ranges[i].chardata_for_range = baker->packed_chars + char_n;
                char_n += tmp->ranges[i].num_chars;
            }

            /* pack */
            tmp->rects = baker->rects + rect_n;
            rect_n += glyph_count;
            stbtt_PackSetOversampling(&baker->spc, cfg->oversample_h, cfg->oversample_v);
            n = stbtt_PackFontRangesGatherRects(&baker->spc, &tmp->info,
                tmp->ranges, (int)tmp->range_count, tmp->rects);
            stbrp_pack_rects((stbrp_context*)baker->spc.pack_info, tmp->rects, (int)n);

            /* texture height */
            for (i = 0; i < n; ++i) {
                if (tmp->rects[i].was_packed)
                    *height = MAX(*height, tmp->rects[i].y + tmp->rects[i].h);
            }
        }
        ZR_ASSERT(rect_n == total_glyph_count);
        ZR_ASSERT(char_n == total_glyph_count);
        ZR_ASSERT(range_n == total_range_count);
    }
    *height = (int32_t)zr_round_up_pow2((uint32_t)*height);
    *image_memory = (zr_size)((*width) * (*height));
    return zr_true;
}

void
zr_font_bake(void *image_memory, int width, int height,
    void *temp, zr_size temp_size, struct zr_font_glyph *glyphes,
    int glyphes_count, const struct zr_font_config *config, int font_count)
{
    int input_i = 0;
    struct zr_font_baker* baker;
    zr_rune glyph_n = 0;

    ZR_ASSERT(image_memory);
    ZR_ASSERT(width);
    ZR_ASSERT(height);
    ZR_ASSERT(config);
    ZR_ASSERT(temp);
    ZR_ASSERT(temp_size);
    ZR_ASSERT(font_count);
    ZR_ASSERT(glyphes_count);
    if (!image_memory || !width || !height || !config || !temp ||
        !temp_size || !font_count || !glyphes || !glyphes_count)
        return;

    /* second font pass: render glyphes */
    baker = (struct zr_font_baker*)ZR_ALIGN_PTR(temp, zr_baker_align);
    zr_zero(image_memory, (zr_size)(width * height));
    baker->spc.pixels = (unsigned char*)image_memory;
    baker->spc.height = (int)height;
    for (input_i = 0; input_i < font_count; ++input_i) {
        const struct zr_font_config *cfg = &config[input_i];
        struct zr_font_bake_data *tmp = &baker->build[input_i];
        stbtt_PackSetOversampling(&baker->spc, cfg->oversample_h, cfg->oversample_v);
        stbtt_PackFontRangesRenderIntoRects(&baker->spc, &tmp->info, tmp->ranges,
            (int)tmp->range_count, tmp->rects);
    }
    stbtt_PackEnd(&baker->spc);

    /* third pass: setup font and glyphes */
    for (input_i = 0; input_i < font_count; ++input_i)  {
        zr_size i = 0;
        int char_idx = 0;
        zr_rune glyph_count = 0;
        const struct zr_font_config *cfg = &config[input_i];
        struct zr_font_bake_data *tmp = &baker->build[input_i];
        struct zr_baked_font *dst_font = cfg->font;

        float font_scale = stbtt_ScaleForPixelHeight(&tmp->info, cfg->size);
        int unscaled_ascent, unscaled_descent, unscaled_line_gap;
        stbtt_GetFontVMetrics(&tmp->info, &unscaled_ascent, &unscaled_descent,
                                &unscaled_line_gap);

        /* fill baked font */
        dst_font->ranges = cfg->range;
        dst_font->height = cfg->size;
        dst_font->ascent = ((float)unscaled_ascent * font_scale);
        dst_font->descent = ((float)unscaled_descent * font_scale);
        dst_font->glyph_offset = glyph_n;

        /* fill own baked font glyph array */
        for (i = 0; i < tmp->range_count; ++i) {
            stbtt_pack_range *range = &tmp->ranges[i];
            for (char_idx = 0; char_idx < range->num_chars; char_idx++) {
                zr_rune codepoint = 0;
                float dummy_x = 0, dummy_y = 0;
                stbtt_aligned_quad q;
                struct zr_font_glyph *glyph;

                /* query glyph bounds from stb_truetype */
                const stbtt_packedchar *pc = &range->chardata_for_range[char_idx];
                glyph_count++;
                if (!pc->x0 && !pc->x1 && !pc->y0 && !pc->y1) continue;
                codepoint = (zr_rune)(range->first_unicode_codepoint_in_range + char_idx);
                stbtt_GetPackedQuad(range->chardata_for_range, (int)width,
                    (int)height, char_idx, &dummy_x, &dummy_y, &q, 0);

                /* fill own glyph type with data */
                glyph = &glyphes[dst_font->glyph_offset + (unsigned int)char_idx];
                glyph->codepoint = codepoint;
                glyph->x0 = q.x0; glyph->y0 = q.y0; glyph->x1 = q.x1; glyph->y1 = q.y1;
                glyph->y0 += (dst_font->ascent + 0.5f);
                glyph->y1 += (dst_font->ascent + 0.5f);
                if (cfg->coord_type == ZR_COORD_PIXEL) {
                    glyph->u0 = q.s0 * (float)width;
                    glyph->v0 = q.t0 * (float)height;
                    glyph->u1 = q.s1 * (float)width;
                    glyph->v1 = q.t1 * (float)height;
                } else {
                    glyph->u0 = q.s0;
                    glyph->v0 = q.t0;
                    glyph->u1 = q.s1;
                    glyph->v1 = q.t1;
                }
                glyph->xadvance = (pc->xadvance + cfg->spacing.x);
                if (cfg->pixel_snap)
                    glyph->xadvance = (float)(int)(glyph->xadvance + 0.5f);
            }
        }
        dst_font->glyph_count = glyph_count;
        glyph_n += dst_font->glyph_count;
    }
}

void
zr_font_bake_custom_data(void *img_memory, int img_width, int img_height,
    struct zr_recti img_dst, const char *texture_data_mask, int tex_width,
    int tex_height, char white, char black)
{
    zr_byte *pixels;
    int y = 0, x = 0, n = 0;
    ZR_ASSERT(img_memory);
    ZR_ASSERT(img_width);
    ZR_ASSERT(img_height);
    ZR_ASSERT(texture_data_mask);
    ZR_UNUSED(tex_height);
    if (!img_memory || !img_width || !img_height || !texture_data_mask)
        return;

    pixels = (zr_byte*)img_memory;
    for (y = 0, n = 0; y < tex_height; ++y) {
        for (x = 0; x < tex_width; ++x, ++n) {
            const int off0 = ((img_dst.x + x) + (img_dst.y + y) * img_width);
            const int off1 = off0 + 1 + tex_width;
            pixels[off0] = (texture_data_mask[n] == white) ? 0xFF : 0x00;
            pixels[off1] = (texture_data_mask[n] == black) ? 0xFF : 0x00;
        }
    }
}

void
zr_font_bake_convert(void *out_memory, int img_width, int img_height,
    const void *in_memory)
{
    int n = 0;
    const zr_byte *src;
    uint32_t *dst;
    ZR_ASSERT(out_memory);
    ZR_ASSERT(in_memory);
    ZR_ASSERT(img_width);
    ZR_ASSERT(img_height);
    if (!out_memory || !in_memory || !img_height || !img_width) return;

    dst = (uint32_t*)out_memory;
    src = (const zr_byte*)in_memory;
    for (n = (int)(img_width * img_height); n > 0; n--)
        *dst++ = ((uint32_t)(*src++) << 24) | 0x00FFFFFF;
}
/* -------------------------------------------------------------
 *
 *                          FONT
 *
 * --------------------------------------------------------------*/
void
zr_font_init(struct zr_font *font, float pixel_height,
    zr_rune fallback_codepoint, struct zr_font_glyph *glyphes,
    const struct zr_baked_font *baked_font, zr_handle atlas)
{
    ZR_ASSERT(font);
    ZR_ASSERT(glyphes);
    ZR_ASSERT(baked_font);
    if (!font || !glyphes || !baked_font)
        return;

    zr_zero(font, sizeof(*font));
    font->ascent = baked_font->ascent;
    font->descent = baked_font->descent;
    font->size = baked_font->height;
    font->scale = (float)pixel_height / (float)font->size;
    font->glyphes = &glyphes[baked_font->glyph_offset];
    font->glyph_count = baked_font->glyph_count;
    font->ranges = baked_font->ranges;
    font->atlas = atlas;
    font->fallback_codepoint = fallback_codepoint;
    font->fallback = zr_font_find_glyph(font, fallback_codepoint);
}

const struct zr_font_glyph*
zr_font_find_glyph(struct zr_font *font, zr_rune unicode)
{
    int i = 0;
    int count;
    int total_glyphes = 0;
    const struct zr_font_glyph *glyph = 0;
    ZR_ASSERT(font);

    glyph = font->fallback;
    count = zr_range_count(font->ranges);
    for (i = 0; i < count; ++i) {
        int diff;
        zr_rune f = font->ranges[(i*2)+0];
        zr_rune t = font->ranges[(i*2)+1];
        diff = (int)((t - f) + 1);
        if (unicode >= f && unicode <= t)
            return &font->glyphes[((zr_rune)total_glyphes + (unicode - f))];
        total_glyphes += diff;
    }
    return glyph;
}

static zr_size
zr_font_text_width(zr_handle handle, float height, const char *text, zr_size len)
{
    zr_rune unicode;
    const struct zr_font_glyph *glyph;
    struct zr_font *font;
    zr_size text_len  = 0;
    zr_size text_width = 0;
    zr_size glyph_len = 0;
    float scale = 0;
    font = (struct zr_font*)handle.ptr;
    ZR_ASSERT(font);
    if (!font || !text || !len)
        return 0;

    scale = height/font->size;
    glyph_len = zr_utf_decode(text, &unicode, len);
    while (text_len < len) {
        if (unicode == ZR_UTF_INVALID) return 0;
        glyph = zr_font_find_glyph(font, unicode);
        text_len += glyph_len;
        text_width += (zr_size)((glyph->xadvance * scale));
        glyph_len = zr_utf_decode(text + text_len, &unicode, len - text_len);
    }
    return text_width;
}

#if ZR_COMPILE_WITH_VERTEX_BUFFER
static void
zr_font_query_font_glyph(zr_handle handle, float height, struct zr_user_font_glyph *glyph,
        zr_rune codepoint, zr_rune next_codepoint)
{
    float scale;
    const struct zr_font_glyph *g;
    struct zr_font *font;
    ZR_ASSERT(glyph);
    ZR_UNUSED(next_codepoint);
    font = (struct zr_font*)handle.ptr;
    ZR_ASSERT(font);
    if (!font || !glyph)
        return;

    scale = height/font->size;
    g = zr_font_find_glyph(font, codepoint);
    glyph->width = (g->x1 - g->x0) * scale;
    glyph->height = (g->y1 - g->y0) * scale;
    glyph->offset = zr_vec2(g->x0 * scale, g->y0 * scale);
    glyph->xadvance = (g->xadvance * scale);
    glyph->uv[0] = zr_vec2(g->u0, g->v0);
    glyph->uv[1] = zr_vec2(g->u1, g->v1);
}
#endif

struct zr_user_font
zr_font_ref(struct zr_font *font)
{
    struct zr_user_font user_font;
    zr_zero(&user_font, sizeof(user_font));
    user_font.height = font->size * font->scale;
    user_font.width = zr_font_text_width;
    user_font.userdata.ptr = font;
#if ZR_COMPILE_WITH_VERTEX_BUFFER
    user_font.query = zr_font_query_font_glyph;
    user_font.texture = font->atlas;
#endif
    return user_font;
}
#endif
/*
 * ==============================================================
 *
 *                          Edit Box
 *
 * ===============================================================
 */
static void
zr_edit_buffer_append(zr_edit_buffer *buffer, const char *str, zr_size len)
{
    char *mem;
    ZR_ASSERT(buffer);
    ZR_ASSERT(str);
    if (!buffer || !str || !len) return;
    mem = (char*)zr_buffer_alloc(buffer, ZR_BUFFER_FRONT, len * sizeof(char), 0);
    if (!mem) return;
    zr_memcopy(mem, str, len * sizeof(char));
}

static int
zr_edit_buffer_insert(zr_edit_buffer *buffer, zr_size pos,
    const char *str, zr_size len)
{
    void *mem;
    zr_size i;
    char *src, *dst;

    zr_size copylen;
    ZR_ASSERT(buffer);
    if (!buffer || !str || !len || pos > buffer->allocated) return 0;

    copylen = buffer->allocated - pos;
    if (!copylen) {
        zr_edit_buffer_append(buffer, str, len);
        return 1;
    }
    mem = zr_buffer_alloc(buffer, ZR_BUFFER_FRONT, len * sizeof(char), 0);
    if (!mem) return 0;

    /* memmove */
    ZR_ASSERT(((int)pos + (int)len + ((int)copylen - 1)) >= 0);
    ZR_ASSERT(((int)pos + ((int)copylen - 1)) >= 0);
    dst = zr_ptr_add(char, buffer->memory.ptr, pos + len + (copylen - 1));
    src = zr_ptr_add(char, buffer->memory.ptr, pos + (copylen-1));
    for (i = 0; i < copylen; ++i) *dst-- = *src--;
    mem = zr_ptr_add(void, buffer->memory.ptr, pos);
    zr_memcopy(mem, str, len * sizeof(char));
    return 1;
}

static void
zr_edit_buffer_remove(zr_edit_buffer *buffer, zr_size len)
{
    ZR_ASSERT(buffer);
    if (!buffer || len > buffer->allocated) return;
    ZR_ASSERT(((int)buffer->allocated - (int)len) >= 0);
    buffer->allocated -= len;
}

static void
zr_edit_buffer_del(zr_edit_buffer *buffer, zr_size pos, zr_size len)
{
    char *src, *dst;
    ZR_ASSERT(buffer);
    if (!buffer || !len || pos > buffer->allocated ||
        pos + len > buffer->allocated) return;

    if (pos + len < buffer->allocated) {
        /* memmove */
        dst = zr_ptr_add(char, buffer->memory.ptr, pos);
        src = zr_ptr_add(char, buffer->memory.ptr, pos + len);
        zr_memcopy(dst, src, buffer->allocated - (pos + len));
        ZR_ASSERT(((int)buffer->allocated - (int)len) >= 0);
        buffer->allocated -= len;
    } else zr_edit_buffer_remove(buffer, len);
}

static char*
zr_edit_buffer_at_char(zr_edit_buffer *buffer, zr_size pos)
{
    ZR_ASSERT(buffer);
    if (!buffer || pos > buffer->allocated) return 0;
    return zr_ptr_add(char, buffer->memory.ptr, pos);
}

static char*
zr_edit_buffer_at(zr_edit_buffer *buffer, int pos, zr_rune *unicode,
    zr_size *len)
{
    int i = 0;
    zr_size src_len = 0;
    zr_size glyph_len = 0;
    char *text;
    zr_size text_len;

    ZR_ASSERT(buffer);
    ZR_ASSERT(unicode);
    ZR_ASSERT(len);
    if (!buffer || !unicode || !len) return 0;
    if (pos < 0) {
        *unicode = 0;
        *len = 0;
        return 0;
    }

    text = (char*)buffer->memory.ptr;
    text_len = buffer->allocated;
    glyph_len = zr_utf_decode(text, unicode, text_len);
    while (glyph_len) {
        if (i == pos) {
            *len = glyph_len;
            break;
        }

        i++;
        src_len = src_len + glyph_len;
        glyph_len = zr_utf_decode(text + src_len, unicode, text_len - src_len);
    }
    if (i != pos) return 0;
    return text + src_len;
}

void
zr_edit_box_init(struct zr_edit_box *eb, struct zr_allocator *a,
    zr_size initial_size, float grow_fac, const struct zr_clipboard *clip,
    zr_filter f)
{
    ZR_ASSERT(eb);
    ZR_ASSERT(a);
    ZR_ASSERT(initial_size);
    if (!eb || !a) return;

    zr_zero(eb, sizeof(*eb));
    zr_buffer_init(&eb->buffer, a, initial_size, grow_fac);
    if (clip) eb->clip = *clip;
    if (f) eb->filter = f;
    else eb->filter = zr_filter_default;
    eb->cursor = 0;
    eb->glyphes = 0;
}

void
zr_edit_box_init_fixed(struct zr_edit_box *eb, void *memory, zr_size size,
                        const struct zr_clipboard *clip, zr_filter f)
{
    ZR_ASSERT(eb);
    if (!eb) return;
    zr_zero(eb, sizeof(*eb));
    zr_buffer_init_fixed(&eb->buffer, memory, size);
    if (clip) eb->clip = *clip;
    if (f) eb->filter = f;
    else eb->filter = zr_filter_default;
    eb->cursor = 0;
    eb->glyphes = 0;
}

void
zr_edit_box_clear(struct zr_edit_box *box)
{
    ZR_ASSERT(box);
    if (!box) return;
    zr_buffer_clear(&box->buffer);
    box->cursor = box->glyphes = 0;
}

void
zr_edit_box_free(struct zr_edit_box *box)
{
    ZR_ASSERT(box);
    if (!box) return;
    zr_buffer_free(&box->buffer);
    box->cursor = box->glyphes = 0;
}

void
zr_edit_box_info(struct zr_memory_status *status, struct zr_edit_box *box)
{
    ZR_ASSERT(box);
    ZR_ASSERT(status);
    if (!box || !status) return;
    zr_buffer_info(status, &box->buffer);
}

void
zr_edit_box_add(struct zr_edit_box *eb, const char *str, zr_size len)
{
    int res = 0;
    ZR_ASSERT(eb);
    if (!eb || !str || !len) return;
    if (eb->cursor == eb->glyphes) {
        res = zr_edit_buffer_insert(&eb->buffer, eb->buffer.allocated, str, len);
    } else {
        zr_size l = 0;
        zr_rune unicode;
        int cursor = (eb->cursor) ? (int)(eb->cursor) : 0;
        char *sym = zr_edit_buffer_at(&eb->buffer, cursor, &unicode, &l);
        zr_size offset = (zr_size)(sym - (char*)eb->buffer.memory.ptr);
        res = zr_edit_buffer_insert(&eb->buffer, offset, str, len);
    }
    if (res) {
        zr_size l = zr_utf_len(str, len);
        eb->cursor += l;
        eb->glyphes += l;
    }
}

static zr_size
zr_edit_box_buffer_input(struct zr_edit_box *box, const struct zr_input *i)
{
    zr_rune unicode;
    zr_size src_len = 0;
    zr_size text_len = 0, glyph_len = 0;
    zr_size glyphes = 0;

    ZR_ASSERT(box);
    ZR_ASSERT(i);
    if (!box || !i) return 0;

    /* add user provided text to buffer until either no input or buffer space left*/
    glyph_len = zr_utf_decode(i->keyboard.text, &unicode, i->keyboard.text_len);
    while (glyph_len && ((text_len+glyph_len) <= i->keyboard.text_len)) {
        /* filter to make sure the value is correct */
        if (box->filter(unicode)) {
            zr_edit_box_add(box, &i->keyboard.text[text_len], glyph_len);
            text_len += glyph_len;
            glyphes++;
        }
        src_len = src_len + glyph_len;
        glyph_len = zr_utf_decode(i->keyboard.text + src_len, &unicode,
            i->keyboard.text_len - src_len);
    }
    return glyphes;
}

void
zr_edit_box_remove(struct zr_edit_box *box)
{
    zr_size len;
    char *buf;
    zr_size min, maxi, diff;
    ZR_ASSERT(box);
    if (!box) return;
    if (!box->glyphes) return;

    buf = (char*)box->buffer.memory.ptr;
    min = MIN(box->sel.end, box->sel.begin);
    maxi = MAX(box->sel.end, box->sel.begin);
    diff = maxi - min;

    if (diff && box->cursor != box->glyphes) {
        zr_size off;
        zr_rune unicode;
        char *begin, *end;

        /* calculate text selection byte position and size */
        begin = zr_edit_buffer_at(&box->buffer, (int)min, &unicode, &len);
        end = zr_edit_buffer_at(&box->buffer, (int)maxi, &unicode, &len);
        len = (zr_size)(end - begin);
        off = (zr_size)(begin - buf);

        /* delete text selection */
        zr_edit_buffer_del(&box->buffer, off, len);
        box->glyphes = zr_utf_len(buf, box->buffer.allocated);
    } else {
        zr_rune unicode;
        int cursor;
        char *glyph;
        zr_size offset;

        /* remove last glyph */
        cursor = (int)box->cursor - 1;
        glyph = zr_edit_buffer_at(&box->buffer, cursor, &unicode, &len);
        if (!glyph || !len) return;

        offset = (zr_size)(glyph - (char*)box->buffer.memory.ptr);
        zr_edit_buffer_del(&box->buffer, offset, len);
        box->glyphes--;
    }
    if (box->cursor >= box->glyphes) box->cursor = box->glyphes;
    else if (box->cursor > 0) box->cursor--;
}

char*
zr_edit_box_get(struct zr_edit_box *eb)
{
    ZR_ASSERT(eb);
    if (!eb) return 0;
    return (char*)eb->buffer.memory.ptr;
}

const char*
zr_edit_box_get_const(struct zr_edit_box *eb)
{
    ZR_ASSERT(eb);
    if (!eb) return 0;
    return (char*)eb->buffer.memory.ptr;
}

char
zr_edit_box_at_char(struct zr_edit_box *eb, zr_size pos)
{
    char *c;
    ZR_ASSERT(eb);
    if (!eb || pos >= eb->buffer.allocated) return 0;
    c = zr_edit_buffer_at_char(&eb->buffer, pos);
    return c ? *c : 0;
}

void
zr_edit_box_at(struct zr_edit_box *eb, zr_size pos, zr_glyph g, zr_size *len)
{
    char *sym;
    zr_rune unicode;

    ZR_ASSERT(eb);
    ZR_ASSERT(g);
    ZR_ASSERT(len);

    if (!eb || !len || !g) return;
    if (pos >= eb->glyphes) {
        *len = 0;
        return;
    }
    sym = zr_edit_buffer_at(&eb->buffer, (int)pos, &unicode, len);
    if (!sym) return;
    zr_memcopy(g, sym, *len);
}

void
zr_edit_box_at_cursor(struct zr_edit_box *eb, zr_glyph g, zr_size *len)
{
    const char *text;
    zr_size text_len;
    zr_size glyph_len = 0;

    ZR_ASSERT(eb);
    ZR_ASSERT(g);
    ZR_ASSERT(len);

    if (!eb || !g || !len) return;
    if (!eb->cursor) {
        *len = 0;
        return;
    }

    text = (char*)eb->buffer.memory.ptr;
    text_len = eb->buffer.allocated;
    glyph_len = zr_utf_len(text, text_len);
    zr_edit_box_at(eb, glyph_len, g, len);
}

void
zr_edit_box_set_cursor(struct zr_edit_box *eb, zr_size pos)
{
    ZR_ASSERT(eb);
    ZR_ASSERT(eb->glyphes >= pos);
    if (!eb || pos > eb->glyphes) return;
    eb->cursor = pos;
}

zr_size
zr_edit_box_get_cursor(struct zr_edit_box *eb)
{
    ZR_ASSERT(eb);
    if (!eb) return 0;
    return eb->cursor;
}

zr_size
zr_edit_box_len_char(struct zr_edit_box *eb)
{
    ZR_ASSERT(eb);
    return eb->buffer.allocated;
}

zr_size
zr_edit_box_len(struct zr_edit_box *eb)
{
    ZR_ASSERT(eb);
    return eb->glyphes;
}

/* ===============================================================
 *
 *                          TEXT
 *
 * ===============================================================*/
struct zr_text {
    struct zr_vec2 padding;
    struct zr_color background;
    struct zr_color text;
};

static void
zr_widget_text(struct zr_command_buffer *o, struct zr_rect b,
    const char *string, zr_size len, const struct zr_text *t,
    enum zr_text_align a, const struct zr_user_font *f)
{
    struct zr_rect label;
    zr_size text_width;

    ZR_ASSERT(o);
    ZR_ASSERT(t);
    if (!o || !t) return;

    b.h = MAX(b.h, 2 * t->padding.y);
    label.x = 0; label.w = 0;
    label.y = b.y + t->padding.y;
    label.h = b.h - 2 * t->padding.y;

    text_width = f->width(f->userdata, f->height, (const char*)string, len);
    text_width += (zr_size)(2 * t->padding.x);

    if (a == ZR_TEXT_LEFT) {
        label.x = b.x + t->padding.x;
        label.w = MAX(0, b.w - 2 * t->padding.x);
    } else if (a == ZR_TEXT_CENTERED) {
        label.w = MAX(1, 2 * t->padding.x + (float)text_width);
        label.x = (b.x + t->padding.x + ((b.w - 2 * t->padding.x)/2));
        if (label.x >= label.w/2) label.x -= (label.w/2);
        label.x = MAX(b.x + t->padding.x, label.x);
        label.w = MIN(b.x + b.w, label.x + label.w);
        if (label.w >= label.x) label.w -= label.x;
    } else if (a == ZR_TEXT_RIGHT) {
        label.x = MAX(b.x + t->padding.x, (b.x + b.w) - (2 * t->padding.x + (float)text_width));
        label.w = (float)text_width + 2 * t->padding.x;
    } else return;
    zr_command_buffer_push_text(o, label, (const char*)string,
        len, f, t->background, t->text);
}

static void
zr_widget_text_wrap(struct zr_command_buffer *o, struct zr_rect b,
    const char *string, zr_size len, const struct zr_text *t,
    const struct zr_user_font *f)
{
    float width;
    zr_size glyphes = 0;
    zr_size fitting = 0;
    zr_size done = 0;
    struct zr_rect line;
    struct zr_text text;

    ZR_ASSERT(o);
    ZR_ASSERT(t);
    if (!o || !t) return;

    text.padding = zr_vec2(0,0);
    text.background = t->background;
    text.text = t->text;

    b.w = MAX(b.w, 2 * t->padding.x);
    b.h = MAX(b.h, 2 * t->padding.y);
    b.h = b.h - 2 * t->padding.y;

    line.x = b.x + t->padding.x;
    line.y = b.y + t->padding.y;
    line.w = b.w - 2 * t->padding.x;
    line.h = 2 * t->padding.y + f->height;

    fitting = zr_use_font_glyph_clamp(f, string, len, line.w, &glyphes, &width);
    while (done < len) {
        if (!fitting || line.y + line.h >= (b.y + b.h)) break;
        zr_widget_text(o, line, &string[done], fitting, &text, ZR_TEXT_LEFT, f);
        done += fitting;
        line.y += f->height + 2 * t->padding.y;
        fitting = zr_use_font_glyph_clamp(f, &string[done], len-done, line.w, &glyphes, &width);
    }
}

/* ===============================================================
 *
 *                          BUTTON
 *
 * ===============================================================*/
struct zr_button {
    float border_width;
    float rounding;
    struct zr_vec2 padding;
    struct zr_color border;
    struct zr_color normal;
    struct zr_color hover;
    struct zr_color active;
};

struct zr_button_text {
    struct zr_button base;
    enum zr_text_align alignment;
    struct zr_color normal;
    struct zr_color hover;
    struct zr_color active;
};

struct zr_button_symbol {
    struct zr_button base;
    struct zr_color normal;
    struct zr_color hover;
    struct zr_color active;
};

struct zr_button_icon {
    struct zr_button base;
    struct zr_vec2 padding;
};

static void
zr_draw_symbol(struct zr_command_buffer *out, enum zr_symbol symbol,
    struct zr_rect content, struct zr_color background, struct zr_color foreground,
    float border_width, const struct zr_user_font *font)
{
    switch (symbol) {
    case ZR_SYMBOL_X:
    case ZR_SYMBOL_UNDERSCORE:
    case ZR_SYMBOL_PLUS:
    case ZR_SYMBOL_MINUS: {
        /* single character text symbol */
        const char *X = (symbol == ZR_SYMBOL_X) ? "x":
            (symbol == ZR_SYMBOL_UNDERSCORE) ? "_":
            (symbol == ZR_SYMBOL_PLUS) ? "+": "-";
        struct zr_text text;
        text.padding = zr_vec2(0,0);
        text.background = background;
        text.text = foreground;
        zr_widget_text(out, content, X, 1, &text, ZR_TEXT_CENTERED, font);
    } break;
    case ZR_SYMBOL_CIRCLE:
    case ZR_SYMBOL_CIRCLE_FILLED:
    case ZR_SYMBOL_RECT:
    case ZR_SYMBOL_RECT_FILLED: {
        /* simple empty/filled shapes */
        if (symbol == ZR_SYMBOL_RECT || symbol == ZR_SYMBOL_RECT_FILLED) {
            zr_command_buffer_push_rect(out, content,  0, foreground);
            if (symbol == ZR_SYMBOL_RECT_FILLED)
                zr_command_buffer_push_rect(out, zr_shrink_rect(content,
                    border_width), 0, background);
        } else {
            zr_command_buffer_push_circle(out, content, foreground);
            if (symbol == ZR_SYMBOL_CIRCLE_FILLED)
                zr_command_buffer_push_circle(out, zr_shrink_rect(content, 1),
                    background);
        }
    } break;
    case ZR_SYMBOL_TRIANGLE_UP:
    case ZR_SYMBOL_TRIANGLE_DOWN:
    case ZR_SYMBOL_TRIANGLE_LEFT:
    case ZR_SYMBOL_TRIANGLE_RIGHT: {
        enum zr_heading heading;
        struct zr_vec2 points[3];
        heading = (symbol == ZR_SYMBOL_TRIANGLE_RIGHT) ? ZR_RIGHT :
            (symbol == ZR_SYMBOL_TRIANGLE_LEFT) ? ZR_LEFT:
            (symbol == ZR_SYMBOL_TRIANGLE_UP) ? ZR_UP: ZR_DOWN;
        zr_triangle_from_direction(points, content, 0, 0, heading);
        zr_command_buffer_push_triangle(out,  points[0].x, points[0].y,
            points[1].x, points[1].y, points[2].x, points[2].y, foreground);
    } break;
    case ZR_SYMBOL_MAX:
    default: break;
    }
}

static int
zr_widget_button(struct zr_command_buffer *o, struct zr_rect r,
    const struct zr_button *b, const struct zr_input *i,
    enum zr_button_behavior behavior, struct zr_rect *content)
{
    int ret = zr_false;
    struct zr_color background;
    struct zr_vec2 pad;
    ZR_ASSERT(b);
    if (!o || !b)
        return zr_false;

    /* calculate button content space */
    pad.x = b->padding.x + b->border_width;
    pad.y = b->padding.y + b->border_width;
    *content = zr_pad_rect(r, pad);

    /* general button user input behavior */
    background = b->normal;
    if (zr_input_is_mouse_hovering_rect(i, r)) {
        background = b->hover;
        if (zr_input_is_mouse_down(i, ZR_BUTTON_LEFT))
            background = b->active;
        if (zr_input_has_mouse_click_in_rect(i, ZR_BUTTON_LEFT, r)) {
            ret = (behavior != ZR_BUTTON_DEFAULT) ?
                zr_input_is_mouse_down(i, ZR_BUTTON_LEFT):
                zr_input_is_mouse_pressed(i, ZR_BUTTON_LEFT);
        }
    }
    zr_command_buffer_push_rect(o, r, b->rounding, b->border);
    zr_command_buffer_push_rect(o, zr_shrink_rect(r, b->border_width),
                                    b->rounding, background);
    return ret;
}

static int
zr_widget_button_text(struct zr_command_buffer *o, struct zr_rect r,
     const char *string, enum zr_button_behavior behavior,
    const struct zr_button_text *b, const struct zr_input *i,
    const struct zr_user_font *f)
{
    struct zr_text t;
    struct zr_rect content;
    int ret = zr_false;

    ZR_ASSERT(b);
    ZR_ASSERT(o);
    ZR_ASSERT(string);
    ZR_ASSERT(f);
    if (!o || !b || !f)
        return zr_false;

    t.text = b->normal;
    t.background = b->base.normal;
    t.padding = zr_vec2(0,0);
    ret = zr_widget_button(o, r, &b->base, i, behavior, &content);
    if (zr_input_is_mouse_hovering_rect(i, r)) {
        int is_down = zr_input_is_mouse_down(i, ZR_BUTTON_LEFT);
        t.background =  (is_down) ? b->base.active: b->base.hover;
        t.text = (is_down) ? b->active : b->hover;
    }
    zr_widget_text(o, content, string, zr_strsiz(string), &t, b->alignment, f);
    return ret;
}

static int
zr_widget_button_symbol(struct zr_command_buffer *out, struct zr_rect r,
    enum zr_symbol symbol, enum zr_button_behavior bh,
    const struct zr_button_symbol *b, const struct zr_input *in,
    const struct zr_user_font *font)
{
    int ret;
    struct zr_color background;
    struct zr_color color;
    struct zr_rect content;

    ZR_ASSERT(b);
    ZR_ASSERT(out);
    if (!out || !b)
        return zr_false;

    ret = zr_widget_button(out, r, &b->base, in, bh, &content);
    if (zr_input_is_mouse_hovering_rect(in, r)) {
        int is_down = zr_input_is_mouse_down(in, ZR_BUTTON_LEFT);
        background = (is_down) ? b->base.active : b->base.hover;
        color = (is_down) ? b->active : b->hover;
    } else {
        background = b->base.normal;
        color = b->normal;
    }
    zr_draw_symbol(out, symbol, content, background, color, b->base.border_width, font);
    return ret;
}

static int
zr_widget_button_image(struct zr_command_buffer *out, struct zr_rect r,
    struct zr_image img, enum zr_button_behavior b,
    const struct zr_button_icon *button, const struct zr_input *in)
{
    int pressed;
    struct zr_rect bounds;
    ZR_ASSERT(button);
    ZR_ASSERT(out);
    if (!out || !button)
        return zr_false;

    pressed = zr_widget_button(out, r, &button->base, in, b, &bounds);
    zr_command_buffer_push_image(out, bounds, &img);
    return pressed;
}

static int
zr_widget_button_text_symbol(struct zr_command_buffer *out, struct zr_rect r,
    enum zr_symbol symbol, const char *text, enum zr_text_align align,
    enum zr_button_behavior behavior, const struct zr_button_text *button,
    const struct zr_user_font *f, const struct zr_input *i)
{
    int ret;
    struct zr_rect tri = {0,0,0,0};
    struct zr_color background, color;
    ZR_ASSERT(button);
    ZR_ASSERT(out);
    if (!out || !button)
        return zr_false;

    ret = zr_widget_button_text(out, r, text, behavior, button, i, f);
    if (zr_input_is_mouse_hovering_rect(i, r)) {
        int is_down = zr_input_is_mouse_down(i, ZR_BUTTON_LEFT);
        background = (is_down) ? button->base.active : button->base.hover;
        color = (is_down) ? button->active : button->hover;
    } else {
        background = button->base.normal;
        color = button->normal;
    }

    /* calculate symbol bounds */
    tri.y = r.y + (r.h/2) - f->height/2;
    tri.w = f->height; tri.h = f->height;
    if (align == ZR_TEXT_LEFT) {
        tri.x = (r.x + r.w) - (2 * button->base.padding.x + tri.w);
        tri.x = MAX(tri.x, 0);
    } else tri.x = r.x + 2 * button->base.padding.x;
    zr_draw_symbol(out, symbol, tri, background, color, button->base.border_width, f);
    return ret;
}

static int
zr_widget_button_text_image(struct zr_command_buffer *out, struct zr_rect r,
    struct zr_image img, const char* text, enum zr_button_behavior behavior,
    const struct zr_button_text *button, const struct zr_user_font *f,
    const struct zr_input *i)
{
    int pressed;
    struct zr_rect icon;
    ZR_ASSERT(button);
    ZR_ASSERT(out);
    if (!out || !button)
        return zr_false;

    pressed = zr_widget_button_text(out, r, text, behavior, button, i, f);
    icon.y = r.y + button->base.padding.y;
    icon.w = icon.h = r.h - 2 * button->base.padding.y;
    if (button->alignment == ZR_TEXT_LEFT) {
        icon.x = (r.x + r.w) - (2 * button->base.padding.x + icon.w);
        icon.x = MAX(icon.x, 0);
    } else icon.x = r.x + 2 * button->base.padding.x;
    zr_command_buffer_push_image(out, icon, &img);
    return pressed;
}

/* ===============================================================
 *
 *                          TOGGLE
 *
 * ===============================================================*/
enum zr_toggle_type {
    ZR_TOGGLE_CHECK,
    ZR_TOGGLE_OPTION
};

struct zr_toggle {
    float rounding;
    struct zr_vec2 padding;
    struct zr_color font;
    struct zr_color background;
    struct zr_color normal;
    struct zr_color hover;
    struct zr_color cursor;
};

static void
zr_widget_toggle(struct zr_command_buffer *out, struct zr_rect r,
    int *active, const char *string, enum zr_toggle_type type,
    const struct zr_toggle *toggle, const struct zr_input *in,
    const struct zr_user_font *font)
{
    int toggle_active;
    struct zr_rect select;
    struct zr_rect cursor;
    float cursor_pad;
    struct zr_color col;

    ZR_ASSERT(toggle);
    ZR_ASSERT(out);
    ZR_ASSERT(font);
    if (!out || !toggle || !font || !active)
        return;

    /* make sure correct values */
    r.w = MAX(r.w, font->height + 2 * toggle->padding.x);
    r.h = MAX(r.h, font->height + 2 * toggle->padding.y);
    toggle_active = *active;

    /* calculate the size of the complete toggle */
    select.w = MIN(r.h, font->height + toggle->padding.y);
    select.h = select.w;
    select.x = r.x + toggle->padding.x;
    select.y = (r.y + toggle->padding.y + (select.w / 2)) - (font->height / 2);

    /* calculate the bounds of the cursor inside the toggle */
    cursor_pad = (type == ZR_TOGGLE_OPTION) ?
        (float)(int)(select.w / 4):
        (float)(int)(select.h / 6);

    select.h = MAX(select.w, cursor_pad * 2);
    cursor.h = select.h - cursor_pad * 2;
    cursor.w = cursor.h;
    cursor.x = select.x + cursor_pad;
    cursor.y = select.y + cursor_pad;

    /* update toggle state with user input */
    toggle_active = zr_input_mouse_clicked(in, ZR_BUTTON_LEFT, cursor) ?
        !toggle_active : toggle_active;
    if (in && zr_input_is_mouse_hovering_rect(in, cursor))
        col = toggle->hover;
    else col = toggle->normal;

    /* draw radiobutton/checkbox background */
    if (type == ZR_TOGGLE_CHECK)
        zr_command_buffer_push_rect(out, select , toggle->rounding, col);
    else zr_command_buffer_push_circle(out, select, col);

    /* draw radiobutton/checkbox cursor if active */
    if (toggle_active) {
        if (type == ZR_TOGGLE_CHECK)
            zr_command_buffer_push_rect(out, cursor, toggle->rounding, toggle->cursor);
        else zr_command_buffer_push_circle(out, cursor, toggle->cursor);
    }

    /* draw toggle text */
    if (font && string) {
        struct zr_text text;
        struct zr_rect inner;

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
        zr_widget_text(out, inner, string, zr_strsiz(string), &text, ZR_TEXT_LEFT, font);
    }
    *active = toggle_active;
}

/* ===============================================================
 *
 *                          SLIDER
 *
 * ===============================================================*/
struct zr_progress {
    float rounding;
    struct zr_vec2 padding;
    struct zr_color border;
    struct zr_color background;
    struct zr_color normal;
    struct zr_color hover;
    struct zr_color active;
};

struct zr_slider {
    struct zr_vec2 padding;
    struct zr_color border;
    struct zr_color bg;
    struct zr_color normal;
    struct zr_color hover;
    struct zr_color active;
    float rounding;
};

struct zr_drag {
    struct zr_vec2 padding;
    struct zr_color border;
    struct zr_color normal;
    struct zr_color hover;
    struct zr_color active;
    struct zr_color text;
    struct zr_color text_active;
    float border_width;
};

static float
zr_widget_slider(struct zr_command_buffer *out, struct zr_rect slider,
    float min, float val, float max, float step,
    const struct zr_slider *s, const struct zr_input *in)
{
    float slider_range;
    float slider_min, slider_max;
    float slider_value, slider_steps;
    float cursor_offset;
    struct zr_rect cursor;
    struct zr_rect bar;
    struct zr_color col;
    int inslider;
    int incursor;

    ZR_ASSERT(s);
    ZR_ASSERT(out);
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
    inslider = in && zr_input_is_mouse_hovering_rect(in, slider);
    incursor = in && zr_input_has_mouse_click_down_in_rect(in, ZR_BUTTON_LEFT, slider, zr_true);
    col = (inslider) ? s->hover: s->normal;

    if (in && inslider && incursor)
    {
        const float d = in->mouse.pos.x - (cursor.x + cursor.w / 2.0f);
        const float pxstep = (slider.w - (2 * s->padding.x)) / slider_steps;
        /* only update value if the next slider step is reached */
        col = s->active;
        if (ZR_ABS(d) >= pxstep) {
            const float steps = (float)((int)(ZR_ABS(d) / pxstep));
            slider_value += (d > 0) ? (step * steps) : -(step * steps);
            slider_value = CLAMP(slider_min, slider_value, slider_max);
            cursor.x = slider.x + (cursor.w * ((slider_value - slider_min) / step));
        }
    }

    {
        struct zr_rect fill;
        cursor.w = cursor.h;
        cursor.x = (slider_value <= slider_min) ? cursor.x:
            (slider_value >= slider_max) ? ((bar.x + bar.w) - cursor.w) :
            cursor.x - (cursor.w/2);

        fill.x = bar.x;
        fill.y = bar.y;
        fill.w = (cursor.x + (cursor.w/2.0f)) - bar.x;
        fill.h = bar.h;

        /* draw slider with background and circle cursor*/
        zr_command_buffer_push_rect(out, bar, 0, s->bg);
        zr_command_buffer_push_rect(out, fill, 0, col);
        zr_command_buffer_push_circle(out, cursor, col);
    }
    return slider_value;
}

static float
zr_widget_drag(struct zr_command_buffer *out, struct zr_rect drag,
    float min, float val, float max,
    float inc_per_pixel, const struct zr_drag *d,
    const struct zr_input *in, const struct zr_user_font *f)
{
    struct zr_text t;
    struct zr_color background;

    float drag_min, drag_max;
    float drag_value;
    char string[ZR_MAX_NUMBER_BUFFER];
    zr_size len;

    int left_mouse_down;
    int left_mouse_click_in_cursor;

    ZR_ASSERT(d);
    ZR_ASSERT(out);
    ZR_ASSERT(f);
    if (!out || !d)
        return 0;

    /* make sure the provided values are correct */
    drag.x = drag.x + d->padding.x;
    drag.y = drag.y + d->padding.y;
    drag.h = MAX(drag.h, 2 * d->padding.y);
    drag.w = MAX(drag.w, 1 + drag.h + 2 * d->padding.x);
    drag.h -= 2 * d->padding.y;
    drag.w -= 2 * d->padding.y;
    drag_max = MAX(min, max);
    drag_min = MIN(min, max);
    drag_value = CLAMP(drag_min, val, drag_max);

    /* update value user input */
    left_mouse_down = in && in->mouse.buttons[ZR_BUTTON_LEFT].down;
    left_mouse_click_in_cursor = in && zr_input_has_mouse_click_down_in_rect(in,
        ZR_BUTTON_LEFT, drag, zr_true);

    t.text = d->text;
    background = d->normal;
    if (zr_input_is_mouse_hovering_rect(in, drag))
        background = d->hover;

    if (left_mouse_down && left_mouse_click_in_cursor) {
        float delta, pixels;
        background = d->active;
        t.text = d->text_active;

        pixels = in->mouse.delta.x;
        delta = pixels * inc_per_pixel;
        drag_value += delta;
        drag_value = CLAMP(drag_min, drag_value, drag_max);
    }

    /* draw border + background */
    zr_command_buffer_push_rect(out, drag, 0, d->border);
    drag = zr_shrink_rect(drag, d->border_width);
    zr_command_buffer_push_rect(out, drag, 0, background);

    /* draw value as text */
    t.background = background;
    t.padding = zr_vec2(0,0);
    zr_dtos(string, drag_value);
    len = zr_string_float_limit(string, ZR_MAX_FLOAT_PRECISION);
    zr_widget_text(out, drag, string, len, &t, ZR_TEXT_CENTERED, f);
    return drag_value;
}

static zr_size
zr_widget_progress(struct zr_command_buffer *out, struct zr_rect r,
    zr_size value, zr_size max, int modifyable,
    const struct zr_progress *prog, const struct zr_input *in)
{
    float prog_scale;
    zr_size prog_value;
    struct zr_color col;

    ZR_ASSERT(prog);
    ZR_ASSERT(out);
    if (!out || !prog) return 0;

    /* make sure given values are correct */
    r.w = MAX(r.w, 2 * prog->padding.x + 5);
    r.h = MAX(r.h, 2 * prog->padding.y + 5);
    r = zr_pad_rect(r, zr_vec2(prog->padding.x, prog->padding.y));
    prog_value = MIN(value, max);

    /* update progress by user input if modifyable */
    if (in && modifyable && zr_input_is_mouse_hovering_rect(in, r)) {
        if (zr_input_is_mouse_down(in, ZR_BUTTON_LEFT)) {
            float ratio = (float)(in->mouse.pos.x - r.x) / (float)r.w;
            prog_value = (zr_size)((float)max * ratio);
            col = prog->active;
        } else col = prog->hover;
    } else col = prog->normal;

    /* make sure calculated values are correct */
    if (!max) return prog_value;
    prog_value = MIN(prog_value, max);
    prog_scale = (float)prog_value / (float)max;

    /* draw progressbar with background and cursor */
    zr_command_buffer_push_rect(out, r, prog->rounding, prog->background);
    r.w = (r.w - 2) * prog_scale;
    zr_command_buffer_push_rect(out, r, prog->rounding, col);
    return prog_value;
}

/* ===============================================================
 *
 *                          EDIT
 *
 * ===============================================================*/
struct zr_edit {
    float border_size;
    float rounding;
    struct zr_vec2 padding;
    int show_cursor;
    struct zr_color background;
    struct zr_color border;
    struct zr_color cursor;
    struct zr_color text;
};

static void
zr_widget_editbox(struct zr_command_buffer *out, struct zr_rect r,
    struct zr_edit_box *box, const struct zr_edit *field,
    const struct zr_input *in, const struct zr_user_font *font)
{
    char *buffer;
    zr_size len;
    ZR_ASSERT(out);
    ZR_ASSERT(font);
    ZR_ASSERT(field);
    if (!out || !box || !field)
        return;

    r.w = MAX(r.w, 2 * field->padding.x + 2 * field->border_size);
    r.h = MAX(r.h, font->height + (2 * field->padding.y + 2 * field->border_size));
    len = zr_edit_box_len_char(box);
    buffer = zr_edit_box_get(box);

    /* draw editbox background and border */
    zr_command_buffer_push_rect(out, r, field->rounding, field->border);
    zr_command_buffer_push_rect(out, zr_shrink_rect(r, field->border_size),
        field->rounding, field->background);

    /* check if the editbox is activated/deactivated */
    if (in && in->mouse.buttons[ZR_BUTTON_LEFT].clicked &&
            in->mouse.buttons[ZR_BUTTON_LEFT].down)
        box->active = ZR_INBOX(in->mouse.pos.x,in->mouse.pos.y,r.x,r.y,r.w,r.h);

    /* input handling */
    if (box->active && in) {
        zr_size min = MIN(box->sel.end, box->sel.begin);
        zr_size maxi = MAX(box->sel.end, box->sel.begin);
        zr_size diff = maxi - min;

        /* text manipulation */
        if (zr_input_is_key_pressed(in,ZR_KEY_DEL) ||
            zr_input_is_key_pressed(in,ZR_KEY_BACKSPACE))
            zr_edit_box_remove(box);
        if (in->keyboard.text_len) {
            if (diff && box->cursor != box->glyphes) {
                /* replace text selection */
                zr_edit_box_remove(box);
                box->cursor = min;
                zr_edit_box_buffer_input(box, in);
                box->sel.begin = box->cursor;
                box->sel.end = box->cursor;
            } else{
                /* append text into the buffer */
                zr_edit_box_buffer_input(box, in);
                box->sel.begin = box->cursor;
                box->sel.end = box->cursor;
            }
       }

        /* cursor key movement */
        if (zr_input_is_key_pressed(in, ZR_KEY_LEFT)) {
            box->cursor = (zr_size)MAX(0, (int)(box->cursor - 1));
            box->sel.begin = box->cursor;
            box->sel.end = box->cursor;
        }
        if (zr_input_is_key_pressed(in, ZR_KEY_RIGHT) && box->cursor < box->glyphes) {
            box->cursor = MIN((!box->glyphes) ? 0 : box->glyphes, box->cursor + 1);
            box->sel.begin = box->cursor;
            box->sel.end = box->cursor;
        }

        /* copy & cut & paste functionatlity */
        if (zr_input_is_key_pressed(in, ZR_KEY_PASTE) && box->clip.paste)
            box->clip.paste(box->clip.userdata, box);
        if ((zr_input_is_key_pressed(in, ZR_KEY_COPY) && box->clip.copy) ||
            (zr_input_is_key_pressed(in, ZR_KEY_CUT) && box->clip.copy)) {
            if (diff && box->cursor != box->glyphes) {
                /* copy or cut text selection */
                zr_size l;
                zr_rune unicode;
                char *begin, *end;
                begin = zr_edit_buffer_at(&box->buffer, (int)min, &unicode, &l);
                end = zr_edit_buffer_at(&box->buffer, (int)maxi, &unicode, &l);
                box->clip.copy(box->clip.userdata, begin, (zr_size)(end - begin));
                if (zr_input_is_key_pressed(in, ZR_KEY_CUT))
                    zr_edit_box_remove(box);
            } else {
                /* copy or cut complete buffer */
                box->clip.copy(box->clip.userdata, buffer, len);
                if (zr_input_is_key_pressed(in, ZR_KEY_CUT))
                    zr_edit_box_clear(box);
            }

        }
    }
    {
        /* text management */
        struct zr_rect label;
        zr_size  cursor_w = font->width(font->userdata,font->height,(const char*)"X", 1);
        zr_size text_len = len;
        zr_size glyph_off = 0;
        zr_size glyph_cnt = 0;
        zr_size offset = 0;
        float text_width = 0;

        /* calculate text frame */
        label.w = MAX(r.w,  - 2 * field->padding.x - 2 * field->border_size);
        label.w -= 2 * field->padding.x - 2 * field->border_size;
        {
            zr_size frames = 0;
            zr_size glyphes = 0;
            zr_size frame_len = 0;
            float space = MAX(label.w, (float)cursor_w);
            space -= (float)cursor_w;
            while (text_len) {
                frames++;
                offset += frame_len;
                frame_len = zr_user_font_glyphes_fitting_in_space(font,
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
        if (in && field->show_cursor && in->mouse.buttons[ZR_BUTTON_LEFT].down && box->active) {
            const char *visible = &buffer[offset];
            float xoff = in->mouse.pos.x - (r.x + field->padding.x + field->border_size);
            if (ZR_INBOX(in->mouse.pos.x, in->mouse.pos.y, r.x, r.y, r.w, r.h))
            {
                /* text selection in the current text frame */
                zr_size glyph_index;
                zr_size glyph_pos=zr_user_font_glyph_index_at_pos(font,visible,text_len,xoff);
                if (glyph_cnt + glyph_off >= box->glyphes)
                    glyph_index = glyph_off + MIN(glyph_pos, glyph_cnt);
                else glyph_index = glyph_off + MIN(glyph_pos, glyph_cnt-1);

                if (text_len)
                    zr_edit_box_set_cursor(box, glyph_index);
                if (!box->sel.active) {
                    box->sel.active = zr_true;
                    box->sel.begin = glyph_index;
                    box->sel.end = box->sel.begin;
                } else {
                    if (box->sel.begin > glyph_index) {
                        box->sel.end = glyph_index;
                        box->sel.active = zr_true;
                    }
                }
            } else if (!ZR_INBOX(in->mouse.pos.x,in->mouse.pos.y,r.x,r.y,r.w,r.h) &&
                ZR_INBOX(in->mouse.buttons[ZR_BUTTON_LEFT].clicked_pos.x,
                    in->mouse.buttons[ZR_BUTTON_LEFT].clicked_pos.y,r.x,r.y,r.w,r.h)
                && box->cursor != box->glyphes && box->cursor > 0)
            {
                /* text selection out of the current text frame */
                zr_size glyph = ((in->mouse.pos.x > r.x) &&
                    box->cursor+1 < box->glyphes) ?
                    box->cursor+1: box->cursor-1;
                zr_edit_box_set_cursor(box, glyph);
                if (box->sel.active) {
                    box->sel.end = glyph;
                    box->sel.active = zr_true;
                }
            } else box->sel.active = zr_false;
        } else box->sel.active = zr_false;

        /* calculate the text bounds */
        label.x = r.x + field->padding.x + field->border_size;
        label.y = r.y + field->padding.y + field->border_size;
        label.h = r.h - (2 * field->padding.y + 2 * field->border_size);
        zr_command_buffer_push_text(out , label, &buffer[offset], text_len,
            font, field->background, field->text);

        /* draw selected text */
        if (box->active && field->show_cursor) {
            if (box->cursor == box->glyphes) {
                /* draw the cursor at the end of the string */
                zr_command_buffer_push_rect(out, zr_rect(label.x+(float)text_width,
                        label.y, (float)cursor_w, label.h), 0, field->cursor);
            } else {
                /* draw text selection */
                zr_size l = 0, s;
                zr_rune unicode;
                char *begin, *end;
                zr_size off_begin, off_end;
                zr_size min = MIN(box->sel.end, box->sel.begin);
                zr_size maxi = MAX(box->sel.end, box->sel.begin);
                struct zr_rect clip = out->clip;

                /* calculate selection text range */
                begin = zr_edit_buffer_at(&box->buffer, (int)min, &unicode, &l);
                end = zr_edit_buffer_at(&box->buffer, (int)maxi, &unicode, &l);
                off_begin = (zr_size)(begin - (char*)box->buffer.memory.ptr);
                off_end = (zr_size)(end - (char*)box->buffer.memory.ptr);

                /* calculate selected text width */
                zr_command_buffer_push_scissor(out, label);
                s = font->width(font->userdata, font->height, buffer + offset, off_begin - offset);
                label.x += (float)s;
                s = font->width(font->userdata, font->height, begin, MAX(l, off_end - off_begin));
                label.w = (float)s;

                /* draw selected text */
                zr_command_buffer_push_text(out , label, begin,
                    MAX(l, off_end - off_begin), font, field->cursor, field->background);
                zr_command_buffer_push_scissor(out, clip);
            }
        }
    }
}

static zr_size
zr_widget_edit_filtered(struct zr_command_buffer *out, struct zr_rect r,
    char *buffer, zr_size len, zr_size max, int *active,
    zr_size *cursor,  const struct zr_edit *field, zr_filter filter,
    const struct zr_input *in, const struct zr_user_font *font)
{
    struct zr_edit_box box;
    zr_edit_box_init_fixed(&box, buffer, max, 0, filter);

    box.buffer.allocated = len;
    box.active = *active;
    box.glyphes = zr_utf_len(buffer, len);
    if (!cursor) {
        box.cursor = box.glyphes;
    } else{
        box.cursor = MIN(*cursor, box.glyphes);
        box.sel.begin = box.cursor;
        box.sel.end = box.cursor;
    }

    zr_widget_editbox(out, r, &box, field, in, font);
    *active = box.active;
    if (cursor)
        *cursor = box.cursor;
    return zr_edit_box_len_char(&box);
}

int zr_filter_default(zr_rune unicode)
{(void)unicode;return zr_true;}

int
zr_filter_ascii(zr_rune unicode)
{
    if (unicode > 128) return zr_false;
    else return zr_true;
}

int
zr_filter_float(zr_rune unicode)
{
    if ((unicode < '0' || unicode > '9') && unicode != '.' && unicode != '-')
        return zr_false;
    else return zr_true;
}

int
zr_filter_decimal(zr_rune unicode)
{
    if (unicode < '0' || unicode > '9')
        return zr_false;
    else return zr_true;
}

int
zr_filter_hex(zr_rune unicode)
{
    if ((unicode < '0' || unicode > '9') &&
        (unicode < 'a' || unicode > 'f') &&
        (unicode < 'A' || unicode > 'F'))
        return zr_false;
    else return zr_true;
}

int
zr_filter_oct(zr_rune unicode)
{
    if (unicode < '0' || unicode > '7')
        return zr_false;
    else return zr_true;
}

int
zr_filter_binary(zr_rune unicode)
{
    if (unicode < '0' || unicode > '1')
        return zr_false;
    else return zr_true;
}

static zr_size
zr_widget_edit(struct zr_command_buffer *out, struct zr_rect r,
    char *buffer, zr_size len, zr_size max, int *active,
    zr_size *cursor, const struct zr_edit *field, enum zr_input_filter f,
    const struct zr_input *in, const struct zr_user_font *font)
{
    static const zr_filter filter[] = {
        zr_filter_default,
        zr_filter_ascii,
        zr_filter_float,
        zr_filter_decimal,
        zr_filter_hex,
        zr_filter_oct,
        zr_filter_binary,
    };
    return zr_widget_edit_filtered(out, r, buffer, len, max, active,
            cursor, field, filter[f], in, font);
}

/* ===============================================================
 *
 *                      SCROLLBAR
 *
 * ===============================================================*/
struct zr_scrollbar {
    float rounding;
    struct zr_color border;
    struct zr_color background;
    struct zr_color normal;
    struct zr_color hover;
    struct zr_color active;
    int has_scrolling;
};

static float
zr_widget_scrollbarv(struct zr_command_buffer *out, struct zr_rect scroll,
    float offset, float target, float step, const struct zr_scrollbar *s,
    struct zr_input *i)
{
    struct zr_rect cursor;
    float scroll_step;
    float scroll_offset;
    float scroll_off, scroll_ratio;
    struct zr_color col;

    ZR_ASSERT(out);
    ZR_ASSERT(s);
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
        int left_mouse_down, left_mouse_click_in_cursor;
        left_mouse_down = i->mouse.buttons[ZR_BUTTON_LEFT].down;
        left_mouse_click_in_cursor = zr_input_has_mouse_click_down_in_rect(i,
            ZR_BUTTON_LEFT, cursor, zr_true);
        if (zr_input_is_mouse_hovering_rect(i, cursor))
            col = s->hover;

        if (left_mouse_down && left_mouse_click_in_cursor) {
            /* update cursor by mouse dragging */
            const float pixel = i->mouse.delta.y;
            const float delta =  (pixel / scroll.h) * target;
            scroll_offset = CLAMP(0, scroll_offset + delta, target - scroll.h);
            col = s->active;
            /* This is probably one of my most distgusting hacks I have ever done.
             * This basically changes the mouse clicked position with the moving
             * cursor. This allows for better scroll behavior but resulted into me
             * having to remove const correctness for input. But in the end I believe
             * it is worth it. */
            i->mouse.buttons[ZR_BUTTON_LEFT].clicked_pos.y += i->mouse.delta.y;
        } else if (s->has_scrolling && ((i->mouse.scroll_delta<0) || (i->mouse.scroll_delta>0))) {
            /* update cursor by mouse scrolling */
            scroll_offset = scroll_offset + scroll_step * (-i->mouse.scroll_delta);
            scroll_offset = CLAMP(0, scroll_offset, target - scroll.h);
        }
        scroll_off = scroll_offset / target;
        cursor.y = scroll.y + (scroll_off * scroll.h);
    }

    /* draw scrollbar cursor */
    zr_command_buffer_push_rect(out, zr_shrink_rect(scroll,1), s->rounding, s->border);
    zr_command_buffer_push_rect(out, scroll, s->rounding, s->background);
    zr_command_buffer_push_rect(out, cursor, s->rounding, col);
    return scroll_offset;
}

static float
zr_widget_scrollbarh(struct zr_command_buffer *out, struct zr_rect scroll,
    float offset, float target, float step, const struct zr_scrollbar *s,
    struct zr_input *i)
{
    struct zr_rect cursor;
    float scroll_step;
    float scroll_offset;
    float scroll_off, scroll_ratio;
    struct zr_color col;

    ZR_ASSERT(out);
    ZR_ASSERT(s);
    if (!out || !s) return 0;

    /* scrollbar background */
    scroll.h = MAX(scroll.h, 1);
    scroll.w = MAX(scroll.w, 2 * scroll.h);
    if (target <= scroll.w) return 0;

    /* calculate scrollbar constants */
    scroll_step = MIN(step, scroll.w);
    scroll_offset = MIN(offset, target - scroll.w);
    scroll_ratio = scroll.w / target;
    scroll_off = scroll_offset / target;

    /* calculate scrollbar cursor bounds */
    cursor.w = scroll_ratio * scroll.w - 2;
    cursor.x = scroll.x + (scroll_off * scroll.w) + 1;
    cursor.h = scroll.h - 2;
    cursor.y = scroll.y + 1;

    col = s->normal;
    if (i) {
        int left_mouse_down, left_mouse_click_in_cursor;
        left_mouse_down = i->mouse.buttons[ZR_BUTTON_LEFT].down;
        left_mouse_click_in_cursor = zr_input_has_mouse_click_down_in_rect(i,
            ZR_BUTTON_LEFT, cursor, zr_true);
        if (zr_input_is_mouse_hovering_rect(i, cursor))
            col = s->hover;

        if (left_mouse_down && left_mouse_click_in_cursor) {
            /* update cursor by mouse dragging */
            const float pixel = i->mouse.delta.x;
            const float delta =  (pixel / scroll.w) * target;
            scroll_offset = CLAMP(0, scroll_offset + delta, target - scroll.w);
            col = s->active;
            i->mouse.buttons[ZR_BUTTON_LEFT].clicked_pos.x += i->mouse.delta.x;
        } else if (s->has_scrolling && ((i->mouse.scroll_delta<0) || (i->mouse.scroll_delta>0))) {
            /* update cursor by mouse scrolling */
            scroll_offset = scroll_offset + scroll_step * (-i->mouse.scroll_delta);
            scroll_offset = CLAMP(0, scroll_offset, target - scroll.w);
        }
        scroll_off = scroll_offset / target;
        cursor.x = scroll.x + (scroll_off * scroll.w);
    }

    /* draw scrollbar cursor */
    zr_command_buffer_push_rect(out, zr_shrink_rect(scroll,1), s->rounding, s->border);
    zr_command_buffer_push_rect(out, scroll, s->rounding, s->background);
    zr_command_buffer_push_rect(out, cursor, s->rounding, col);
    return scroll_offset;
}

/* ===============================================================
 *
 *                          SPINNER
 *
 * ===============================================================*/
struct zr_spinner {
    struct zr_button_symbol button;
    struct zr_color color;
    struct zr_color border;
    struct zr_color text;
    struct zr_vec2 padding;
    int show_cursor;
};

static int
zr_widget_spinner_base(struct zr_command_buffer *out, struct zr_rect r,
    const struct zr_spinner *s, char *buffer, zr_size *len,
    enum zr_input_filter filter, int *active,
    const struct zr_input *in, const struct zr_user_font *font)
{
    int ret = 0;
    struct zr_rect bounds;
    struct zr_edit field;
    int button_up_clicked, button_down_clicked;
    int is_active = (active) ? *active : zr_false;

    r.h = MAX(r.h, font->height + 2 * s->padding.x);
    r.w = MAX(r.w, r.h - (s->padding.x + (float)s->button.base.border_width * 2));

    /* up/down button setup and execution */
    bounds.y = r.y;
    bounds.h = r.h / 2;
    bounds.w = r.h - s->padding.x;
    bounds.x = r.x + r.w - bounds.w;
    button_up_clicked = zr_widget_button_symbol(out, bounds, ZR_SYMBOL_TRIANGLE_UP,
        ZR_BUTTON_DEFAULT, &s->button, in, font);
    if (button_up_clicked) ret = 1;

    bounds.y = r.y + bounds.h;
    button_down_clicked = zr_widget_button_symbol(out, bounds, ZR_SYMBOL_TRIANGLE_DOWN,
        ZR_BUTTON_DEFAULT, &s->button, in, font);
    if (button_down_clicked)
        ret = -1;

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
    field.cursor = s->text;
    *len = zr_widget_edit(out, bounds, buffer, *len, ZR_MAX_NUMBER_BUFFER,
        &is_active, 0, &field, filter, in, font);
    if (active)
        *active = is_active;
    return ret;
}

static int
zr_widget_spinner_int(struct zr_command_buffer *out, struct zr_rect r,
    const struct zr_spinner *s, int min, int value,
    int max, int step, int *active, const struct zr_input *in,
    const struct zr_user_font *font)
{
    int res;
    char string[ZR_MAX_NUMBER_BUFFER];
    zr_size len, old_len;
    int is_active;

    ZR_ASSERT(out);
    ZR_ASSERT(s);
    ZR_ASSERT(font);
    if (!out || !s || !font) return value;

    /* make sure given values are correct */
    value = CLAMP(min, value, max);
    len = zr_itos(string, value);
    is_active = (active) ? *active : zr_false;
    old_len = len;

    res = zr_widget_spinner_base(out, r, s, string, &len, ZR_INPUT_DEC, &is_active, in, font);
    if (res) {
        value += (res > 0) ? step : -step;
        value = CLAMP(min, value, max);
    }

    if (old_len != len)
        zr_strtoi(&value, string, len);
    if (active) *active = is_active;
    return value;
}

static float
zr_widget_spinner_float(struct zr_command_buffer *out, struct zr_rect r,
    const struct zr_spinner *s, float min, float value, float max,
    float step, int *active, const struct zr_input *in,
    const struct zr_user_font *font)
{
    int res;
    char string[ZR_MAX_NUMBER_BUFFER];
    zr_size len, old_len;
    int is_active;

    ZR_ASSERT(out);
    ZR_ASSERT(s);
    ZR_ASSERT(font);
    if (!out || !s || !font) return value;

    /* make sure given values are correct */
    value = CLAMP(min, value, max);
    len = zr_dtos(string, value);
    len = zr_string_float_limit(string, ZR_MAX_FLOAT_PRECISION);
    is_active = (active) ? *active : zr_false;
    old_len = len;

    res = zr_widget_spinner_base(out, r, s, string, &len, ZR_INPUT_FLOAT, &is_active, in, font);
    if (res) {
        int val;
        float f = (float)zr_pow(10.0, ZR_MAX_FLOAT_PRECISION);
        value += (res > 0) ? step : -step;
        value = CLAMP(min, value, max);
        val = (int)(value * f);
        val = ((val + (int)((step*f)*0.5f)) / (int)(step*f)) * (int)(step*f);
        value = (float)val / f;
    }

    if (old_len != len) {
        zr_string_float_limit(string, ZR_MAX_FLOAT_PRECISION);
        zr_strtof(&value, string);
    }
    if (active) *active = is_active;
    return value;
}

/* ==============================================================
 *
 *                          STYLE
 *
 * ===============================================================*/
#define ZR_STYLE_PROPERTY_MAP(PROPERTY)\
    PROPERTY(SCROLLBAR_SIZE,    14.0f, 14.0f)\
    PROPERTY(PADDING,           15.0f, 10.0f)\
    PROPERTY(SIZE,              64.0f, 64.0f)\
    PROPERTY(ITEM_SPACING,      4.0f, 4.0f)\
    PROPERTY(ITEM_PADDING,      4.0f, 4.0f)\
    PROPERTY(SCALER_SIZE,       16.0f, 16.0f)

#define ZR_STYLE_ROUNDING_MAP(ROUNDING)\
    ROUNDING(BUTTON,    4.0f)\
    ROUNDING(SLIDER,    8.0f)\
    ROUNDING(PROGRESS,  4.0f)\
    ROUNDING(CHECK,     0.0f)\
    ROUNDING(INPUT,     0.0f)\
    ROUNDING(GRAPH,     4.0f)\
    ROUNDING(SCROLLBAR, 5.0f)

#define ZR_STYLE_COLOR_MAP(COLOR)\
    COLOR(TEXT,                     175, 175, 175, 255)\
    COLOR(TEXT_HOVERING,            120, 120, 120, 255)\
    COLOR(TEXT_ACTIVE,              100, 100, 100, 255)\
    COLOR(WINDOW,                   45, 45, 45, 255)\
    COLOR(HEADER,                   40, 40, 40, 255)\
    COLOR(BORDER,                   65, 65, 65, 255)\
    COLOR(BUTTON,                   50, 50, 50, 255)\
    COLOR(BUTTON_HOVER,             35, 35, 35, 255)\
    COLOR(BUTTON_ACTIVE,            40, 40, 40, 255)\
    COLOR(TOGGLE,                   100, 100, 100, 255)\
    COLOR(TOGGLE_HOVER,             120, 120, 120, 255)\
    COLOR(TOGGLE_CURSOR,            45, 45, 45, 255)\
    COLOR(SELECTABLE,               100, 100, 100, 255)\
    COLOR(SELECTABLE_HOVER,         80, 80, 80, 255)\
    COLOR(SELECTABLE_TEXT,          45, 45, 45, 255)\
    COLOR(SLIDER,                   38, 38, 38, 255)\
    COLOR(SLIDER_CURSOR,            100, 100, 100, 255)\
    COLOR(SLIDER_CURSOR_HOVER,      120, 120, 120, 255)\
    COLOR(SLIDER_CURSOR_ACTIVE,     150, 150, 150, 255)\
    COLOR(PROGRESS,                 38, 38, 38, 255)\
    COLOR(PROGRESS_CURSOR,          100, 100, 100, 255)\
    COLOR(PROGRESS_CURSOR_HOVER,    120, 120, 120, 255)\
    COLOR(PROGRESS_CURSOR_ACTIVE,   150, 150, 150, 255)\
    COLOR(DRAG,                     38, 38, 38, 255)\
    COLOR(DRAG_HOVER,               50, 50, 50, 255)\
    COLOR(DRAG_ACTIVE,              60, 60, 60, 255)\
    COLOR(INPUT,                    45, 45, 45, 255)\
    COLOR(INPUT_CURSOR,             100, 100, 100, 255)\
    COLOR(INPUT_TEXT,               135, 135, 135, 255)\
    COLOR(SPINNER,                  45, 45, 45, 255)\
    COLOR(SPINNER_TRIANGLE,         100, 100, 100, 255)\
    COLOR(HISTO,                    120, 120, 120, 255)\
    COLOR(HISTO_BARS,               45, 45, 45, 255)\
    COLOR(HISTO_NEGATIVE,           255, 255, 255, 255)\
    COLOR(HISTO_HIGHLIGHT,          255, 0, 0, 255)\
    COLOR(PLOT,                     120, 120, 120, 255)\
    COLOR(PLOT_LINES,               45, 45, 45, 255)\
    COLOR(PLOT_HIGHLIGHT,           255, 0, 0, 255)\
    COLOR(SCROLLBAR,                40, 40, 40, 255)\
    COLOR(SCROLLBAR_CURSOR,         100, 100, 100, 255)\
    COLOR(SCROLLBAR_CURSOR_HOVER,   120, 120, 120, 255)\
    COLOR(SCROLLBAR_CURSOR_ACTIVE,  150, 150, 150, 255)\
    COLOR(TABLE_LINES,              100, 100, 100, 255)\
    COLOR(TAB_HEADER,               40, 40, 40, 255)\
    COLOR(SCALER,                   100, 100, 100, 255)

static const char *zr_style_color_names[] = {
    #define COLOR(a,b,c,d,e) #a,
        ZR_STYLE_COLOR_MAP(COLOR)
    #undef COLOR
};
static const char *zr_style_rounding_names[] = {
    #define ROUNDING(a,b) #a,
        ZR_STYLE_ROUNDING_MAP(ROUNDING)
    #undef ROUNDING
};
static const char *zr_style_property_names[] = {
    #define PROPERTY(a,b,c) #a,
        ZR_STYLE_PROPERTY_MAP(PROPERTY)
    #undef PROPERTY
};

const char*
zr_style_color_name(enum zr_style_colors color)
{
    return zr_style_color_names[color];
}

const char*
zr_style_rounding_name(enum zr_style_rounding rounding)
{
    return zr_style_rounding_names[rounding];
}

const char*
zr_style_property_name(enum zr_style_properties property)
{
    return zr_style_property_names[property];
}

static void
zr_style_default_properties(struct zr_style *style)
{
    #define PROPERTY(a,b,c) style->properties[ZR_PROPERTY_##a] = zr_vec2(b, c);
        ZR_STYLE_PROPERTY_MAP(PROPERTY)
    #undef PROPERTY
}

static void
zr_style_default_rounding(struct zr_style *style)
{
    #define ROUNDING(a,b) style->rounding[ZR_ROUNDING_##a] = b;
        ZR_STYLE_ROUNDING_MAP(ROUNDING)
    #undef ROUNDING
}

static void
zr_style_default_color(struct zr_style *style)
{
    #define COLOR(a,b,c,d,e) style->colors[ZR_COLOR_##a] = zr_rgba(b,c,d,e);
        ZR_STYLE_COLOR_MAP(COLOR)
    #undef COLOR
}

void
zr_style_default(struct zr_style *style, zr_flags flags,
    const struct zr_user_font *font)
{
    ZR_ASSERT(style);
    if (!style) return;
    zr_zero(style, sizeof(*style));
    style->font = *font;

    if (flags & ZR_DEFAULT_COLOR)
        zr_style_default_color(style);
    if (flags & ZR_DEFAULT_PROPERTIES)
        zr_style_default_properties(style);
    if (flags & ZR_DEFAULT_ROUNDING)
        zr_style_default_rounding(style);

    style->header.align = ZR_HEADER_RIGHT;
    style->header.close_symbol = 'x';
    style->header.minimize_symbol = '-';
    style->header.maximize_symbol = '+';
}

void
zr_style_set_font(struct zr_style *style, const struct zr_user_font *font)
{
    ZR_ASSERT(style);
    if (!style) return;
    style->font = *font;
}

struct zr_vec2
zr_style_property(const struct zr_style *style, enum zr_style_properties index)
{
    static struct zr_vec2 zero;
    ZR_ASSERT(style);
    if (!style) return zero;
    return style->properties[index];
}

struct zr_color
zr_style_color(const struct zr_style *style, enum zr_style_colors index)
{
    static struct zr_color zero;
    ZR_ASSERT(style);
    if (!style) return zero;
    return style->colors[index];
}

void
zr_style_push_color(struct zr_style *style, enum zr_style_colors index,
    struct zr_color col)
{
    struct zr_saved_color *c;
    ZR_ASSERT(style);
    if (!style) return;
    if (style->stack.color >= ZR_MAX_COLOR_STACK) return;

    c = &style->stack.colors[style->stack.color++];
    c->value = style->colors[index];
    c->type = index;
    style->colors[index] = col;
}

void
zr_style_push_property(struct zr_style *style, enum zr_style_properties index,
    struct zr_vec2 v)
{
    struct zr_saved_property *p;
    ZR_ASSERT(style);
    if (!style) return;
    if (style->stack.property >= ZR_MAX_ATTRIB_STACK) return;

    p = &style->stack.properties[style->stack.property++];
    p->value = style->properties[index];
    p->type = index;
    style->properties[index] = v;
}

void
zr_style_push_font(struct zr_style *style, struct zr_user_font font)
{
    struct zr_saved_font *f;
    ZR_ASSERT(style);
    if (!style) return;
    if (style->stack.font >= ZR_MAX_FONT_STACK) return;

    f = &style->stack.fonts[style->stack.font++];
    f->font_height_begin = style->stack.font_height;
    f->font_height_end = style->stack.font_height;
    f->value = style->font;
    style->font = font;
}

void
zr_style_push_font_height(struct zr_style *style, float font_height)
{
    ZR_ASSERT(style);
    if (!style) return;
    if (style->stack.font >= ZR_MAX_FONT_HEIGHT_STACK) return;

    style->stack.font_heights[style->stack.font_height++] = style->font.height;
    if (style->stack.font)
        style->stack.fonts[style->stack.font-1].font_height_end++;
    style->font.height = font_height;
}

void
zr_style_pop_color(struct zr_style *style)
{
    struct zr_saved_color *c;
    ZR_ASSERT(style);
    if (!style) return;
    if (!style->stack.color) return;
    c = &style->stack.colors[--style->stack.color];
    style->colors[c->type] = c->value;
}

void
zr_style_pop_property(struct zr_style *style)
{
    struct zr_saved_property *p;
    ZR_ASSERT(style);
    if (!style) return;
    if (!style->stack.property) return;
    p = &style->stack.properties[--style->stack.property];
    style->properties[p->type] = p->value;
}

void
zr_style_pop_font(struct zr_style *style)
{
    struct zr_saved_font *f;
    ZR_ASSERT(style);
    if (!style) return;
    if (!style->stack.font) return;

    f = &style->stack.fonts[--style->stack.font];
    style->stack.font_height = f->font_height_begin;
    style->font = f->value;
    if (style->stack.font_height)
        style->font.height = style->stack.font_heights[style->stack.font_height-1];
}

void
zr_style_pop_font_height(struct zr_style *style)
{
    float font_height;
    ZR_ASSERT(style);
    if (!style) return;
    if (!style->stack.font_height) return;
    font_height = style->stack.font_heights[--style->stack.font_height];
    style->font.height = font_height;
    if (style->stack.font) {
        ZR_ASSERT(style->stack.fonts[style->stack.font-1].font_height_end);
        style->stack.fonts[style->stack.font-1].font_height_end--;
    }
}

void
zr_style_reset_colors(struct zr_style *style)
{
    ZR_ASSERT(style);
    if (!style) return;
    while (style->stack.color)
        zr_style_pop_color(style);
}

void
zr_style_reset_properties(struct zr_style *style)
{
    ZR_ASSERT(style);
    if (!style) return;
    while (style->stack.property)
        zr_style_pop_property(style);
}

void
zr_style_reset_font(struct zr_style *style)
{
    ZR_ASSERT(style);
    if (!style) return;
    while (style->stack.font)
        zr_style_pop_font(style);
}

void
zr_style_reset_font_height(struct zr_style *style)
{
    ZR_ASSERT(style);
    if (!style) return;
    while (style->stack.font_height)
        zr_style_pop_font_height(style);
}

void
zr_style_reset(struct zr_style *style)
{
    ZR_ASSERT(style);
    if (!style) return;
    zr_style_reset_colors(style);
    zr_style_reset_properties(style);
    zr_style_reset_font(style);
    zr_style_reset_font_height(style);
}

/* ==============================================================
 *
 *                          WINDOW
 *
 * ===============================================================*/
void
zr_window_init(struct zr_window *window, struct zr_rect bounds,
    zr_flags flags, struct zr_command_queue *queue,
    struct zr_style *style, struct zr_input *input)
{
    ZR_ASSERT(window);
    ZR_ASSERT(style);
    ZR_ASSERT(input);
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
        zr_command_buffer_init(&window->buffer, &queue->buffer, ZR_CLIP);
        zr_command_queue_insert_back(queue, &window->buffer);
    }
}

void
zr_window_link(struct zr_window *window, struct zr_command_queue *queue)
{
    ZR_ASSERT(window);
    ZR_ASSERT(window->queue);
    if (!window || !window->queue) return;
    if (queue) {
        zr_command_buffer_init(&window->buffer, &queue->buffer, ZR_CLIP);
        zr_command_queue_insert_back(queue, &window->buffer);
    }
}

void
zr_window_unlink(struct zr_window *window)
{
    ZR_ASSERT(window);
    ZR_ASSERT(window->queue);
    if (!window || !window->queue) return;
    zr_command_queue_remove(window->queue, &window->buffer);
    window->queue = 0;
}

void
zr_window_add_flag(struct zr_window *panel, zr_flags f)
{panel->flags |= f;}

void
zr_window_remove_flag(struct zr_window *panel, zr_flags f)
{panel->flags &= (zr_flags)~f;}

int
zr_window_has_flag(struct zr_window *panel, zr_flags f)
{return (panel->flags & f) ? zr_true: zr_false;}

int
zr_window_is_minimized(struct zr_window *panel)
{return panel->flags & ZR_WINDOW_MINIMIZED;}

/*-------------------------------------------------------------
 *
 *                          HEADER
 *
 * --------------------------------------------------------------*/
struct zr_window_header {
    float x, y, w, h;
    float front, back;
};

static int
zr_header_button(struct zr_context *layout, struct zr_window_header *header,
   zr_rune symbol, enum zr_style_header_align align)
{
    /* calculate the position of the close icon position and draw it */
    zr_glyph glyph;
    struct zr_rect sym = {0,0,0,0};
    float sym_bw = 0;
    int ret = zr_false;

    const struct zr_style *c;
    struct zr_command_buffer *out;
    struct zr_vec2 item_padding;

    ZR_ASSERT(layout);
    ZR_ASSERT(layout->buffer);
    ZR_ASSERT(layout->style);
    if (!layout || layout->flags & ZR_WINDOW_HIDDEN)
        return zr_false;

    /* cache configuration data */
    c = layout->style;
    out = layout->buffer;
    item_padding = zr_style_property(c, ZR_PROPERTY_ITEM_PADDING);

    sym.x = header->front;
    sym.y = header->y;
    {
        /* single unicode rune text icon */
        const char *X = glyph;
        const zr_size len = zr_utf_encode(symbol, glyph, sizeof(glyph));
        const zr_size t = c->font.width(c->font.userdata, c->font.height, X, len);
        const float text_width = (float)t;

        /* calculate bounds of the icon */
        sym_bw = text_width;
        sym.w = (float)text_width + 2 * item_padding.x;
        sym.h = c->font.height + 2 * item_padding.y;
        if (align == ZR_HEADER_RIGHT)
            sym.x = header->back - sym.w;
        zr_command_buffer_push_text(out, sym, X, len, &c->font,
            c->colors[ZR_COLOR_HEADER],c->colors[ZR_COLOR_TEXT]);
    }

    /* check if the icon has been pressed */
    if (!(layout->flags & ZR_WINDOW_ROM)) {
        float mouse_x = layout->input->mouse.pos.x;
        float mouse_y = layout->input->mouse.pos.y;
        float clicked_x = layout->input->mouse.buttons[ZR_BUTTON_LEFT].clicked_pos.x;
        float clicked_y = layout->input->mouse.buttons[ZR_BUTTON_LEFT].clicked_pos.y;
        if (ZR_INBOX(mouse_x, mouse_y, sym.x, sym.y, sym_bw, sym.h)) {
            if (ZR_INBOX(clicked_x, clicked_y, sym.x, sym.y, sym_bw, sym.h))
                ret = (layout->input->mouse.buttons[ZR_BUTTON_LEFT].down &&
                        layout->input->mouse.buttons[ZR_BUTTON_LEFT].clicked);
        }
    }

    /* update the header space */
    if (align == ZR_HEADER_RIGHT)
        header->back -= (sym.w + item_padding.x);
    else header->front += sym.w + item_padding.x;
    return ret;
}

static int
zr_header_toggle(struct zr_context *layout,struct zr_window_header *header,
    zr_rune active, zr_rune inactive, enum zr_style_header_align align, int state)
{
    int ret = zr_header_button(layout, header,(state) ? active : inactive, align);
    if (ret) return !state;
    return state;
}

static int
zr_header_flag(struct zr_context *layout, struct zr_window_header *header,
    zr_rune inactive, zr_rune active, enum zr_style_header_align align,
    enum zr_window_flags flag)
{
    zr_flags flags = layout->flags;
    int state = (flags & flag) ? zr_true : zr_false;
    int ret = zr_header_toggle(layout, header, inactive, active, align, state);
    if (ret != ((flags & flag) ? zr_true : zr_false)) {
        /* the state of the toggle icon has been changed  */
        if (!ret) layout->flags &= ~flag;
        else layout->flags |= flag;
        /* update the state of the panel since the flag have changed */
        layout->valid = !(layout->flags & ZR_WINDOW_HIDDEN) &&
                        !(layout->flags & ZR_WINDOW_MINIMIZED);
        return zr_true;
    }
    return zr_false;
}

/*-------------------------------------------------------------
 *
 *                          CONTEXT
 *
 * --------------------------------------------------------------*/
zr_flags
zr_begin(struct zr_context *context, struct zr_window *window, const char *title)
{
    int header_active = 0;
    zr_flags ret = 0;
    const struct zr_style *c;
    float scrollbar_size;
    struct zr_vec2 item_padding;
    struct zr_vec2 item_spacing;
    struct zr_vec2 window_padding;
    struct zr_vec2 scaler_size;
    struct zr_command_buffer *out;
    struct zr_input *in;

    ZR_ASSERT(context);
    ZR_ASSERT(window);
    ZR_ASSERT(window->style);

    /* cache configuration data */
    c = window->style;
    in = window->input;
    scrollbar_size = zr_style_property(c, ZR_PROPERTY_SCROLLBAR_SIZE).x;
    window_padding = zr_style_property(c, ZR_PROPERTY_PADDING);
    item_padding = zr_style_property(c, ZR_PROPERTY_ITEM_PADDING);
    item_spacing = zr_style_property(c, ZR_PROPERTY_ITEM_SPACING);
    scaler_size = zr_style_property(c, ZR_PROPERTY_SCALER_SIZE);

    /* check arguments */
    if (!window || !context) return ret;
    zr_zero(context, sizeof(*context));
    if (window->flags & ZR_WINDOW_HIDDEN) {
        context->flags = window->flags;
        context->valid = zr_false;
        context->style = window->style;
        context->buffer = &window->buffer;
        return ret;
    }

    /* overlapping panels */
    if (window->queue && !(window->flags & ZR_WINDOW_TAB))
    {
        context->queue = window->queue;
        zr_command_queue_start(window->queue, &window->buffer);
        {
            int inpanel;
            struct zr_command_buffer_list *s = &window->queue->list;
            inpanel = zr_input_mouse_clicked(in, ZR_BUTTON_LEFT, window->bounds);
            if (inpanel && (&window->buffer != s->end)) {
                const struct zr_command_buffer *iter = window->buffer.next;
                while (iter) {
                    /* try to find a panel with higher priorty in the same position */
                    const struct zr_window *cur;
                    cur = ZR_CONTAINER_OF_CONST(iter, struct zr_window, buffer);
                    if (ZR_INBOX(in->mouse.prev.x, in->mouse.prev.y, cur->bounds.x,
                        cur->bounds.y, cur->bounds.w, cur->bounds.h) &&
                      !(cur->flags & ZR_WINDOW_MINIMIZED) && !(cur->flags & ZR_WINDOW_HIDDEN))
                        break;
                    iter = iter->next;
                }
                /* current panel is active panel in that position so transfer to top
                 * at the highest priority in stack */
                if (!iter) {
                    zr_command_queue_remove(window->queue, &window->buffer);
                    zr_command_queue_insert_back(window->queue, &window->buffer);
                    window->flags &= ~(zr_flags)ZR_WINDOW_ROM;
                }
            }
            if (s->end != &window->buffer)
                window->flags |= ZR_WINDOW_ROM;
        }
    }

    /* move panel position if requested */
    context->header_h = c->font.height + 4 * item_padding.y;
    context->header_h += window_padding.y;
    if ((window->flags & ZR_WINDOW_MOVEABLE) && !(window->flags & ZR_WINDOW_ROM)) {
        int incursor;
        struct zr_rect move;
        move.x = window->bounds.x;
        move.y = window->bounds.y;
        move.w = window->bounds.w;
        move.h = context->header_h;
        incursor = zr_input_is_mouse_prev_hovering_rect(in, move);
        if (zr_input_is_mouse_down(in, ZR_BUTTON_LEFT) && incursor) {
            window->bounds.x = window->bounds.x + in->mouse.delta.x;
            window->bounds.y = window->bounds.y + in->mouse.delta.y;
            ret = ZR_WINDOW_MOVEABLE;
        }
    }

    /* setup panel context */
    out = &window->buffer;
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
    context->max_x = 0;

    /* window header */
    if (window->flags & ZR_WINDOW_MINIMIZED) {
        context->header_h = 0;
        context->row.height = 0;
    } else {
        context->header_h = 2 * item_spacing.y;
        context->row.height = context->header_h + 1;
    }

    /* window activation by click inside */
    if (!(window->flags & ZR_WINDOW_TAB) && !(window->flags & ZR_WINDOW_ROM)) {
        float clicked_x = in->mouse.buttons[ZR_BUTTON_LEFT].clicked_pos.x;
        float clicked_y = in->mouse.buttons[ZR_BUTTON_LEFT].clicked_pos.y;
        if (zr_input_is_mouse_down(in, ZR_BUTTON_LEFT)) {
            if (ZR_INBOX(clicked_x, clicked_y, window->bounds.x, window->bounds.y,
                window->bounds.w, window->bounds.h))
                window->flags |= ZR_WINDOW_ACTIVE;
            else window->flags &= (zr_flags)~ZR_WINDOW_ACTIVE;
        }
    }

    context->flags = window->flags;
    context->valid = !(window->flags & ZR_WINDOW_HIDDEN) &&
        !(window->flags & ZR_WINDOW_MINIMIZED);

    /* calculate window footer height */
    if (!(window->flags & ZR_WINDOW_COMBO_MENU) &&
        (!(window->flags & ZR_WINDOW_NO_SCROLLBAR) || (window->flags & ZR_WINDOW_SCALEABLE)))
        context->footer_h = scaler_size.y + item_padding.y;
    else context->footer_h = 0;

    /* calculate the window size */
    if (!(window->flags & ZR_WINDOW_NO_SCROLLBAR))
        context->width = window->bounds.w - scrollbar_size;
    context->height = window->bounds.h - (context->header_h + 2 * item_spacing.y);
    context->height -= context->footer_h;

    /* draw window background if not a dynamic window */
    if (!(context->flags & ZR_WINDOW_DYNAMIC) && context->valid) {
        zr_command_buffer_push_rect(out, context->bounds, 0, c->colors[ZR_COLOR_WINDOW]);
    } else{
        zr_command_buffer_push_rect(out, zr_rect(context->bounds.x, context->bounds.y,
            context->bounds.w, context->row.height + window_padding.y), 0, c->colors[ZR_COLOR_WINDOW]);
    }

    /* window header */
    header_active = (window->flags & (ZR_WINDOW_CLOSEABLE|ZR_WINDOW_MINIMIZABLE));
    header_active = header_active || (title != 0);
    header_active = header_active && !(window->flags & ZR_WINDOW_HIDDEN);
    if (header_active) {
        zr_flags old;
        struct zr_rect old_clip = out->clip;
        struct zr_window_header header;

        /* This is a little bit of a performace hack. To make sure the header does
         * not get overdrawn with text you do not have to push a scissor rect. This
         * is possible because the command buffer automatically clips text by using
         * its clipping rectangle. But since the clipping rect gets reused to calculate
         * the window clipping rect the old clipping rect has to be stored and reset afterwards. */
        out->clip.x = header.x = context->bounds.x + window_padding.x;
        out->clip.y = header.y = context->bounds.y + item_padding.y;
        out->clip.w = header.w = MAX(context->bounds.w, 2 * window_padding.x);
        out->clip.h = header.w -= 2 * window_padding.x;

        /* update the header height and first row height */
        context->header_h = c->font.height + 2 * item_padding.y;
        context->header_h += window_padding.y;
        context->row.height += context->header_h;

        header.h = context->header_h;
        header.back = header.x + header.w;
        header.front = header.x;

        context->height = context->bounds.h - (header.h + 2 * item_spacing.y);
        context->height -= context->footer_h;

        /* draw header background */
        if (!(context->flags & ZR_WINDOW_BORDER)) {
            zr_command_buffer_push_rect(out, zr_rect(context->bounds.x, context->bounds.y,
                context->bounds.w, context->header_h), 0, c->colors[ZR_COLOR_HEADER]);
        } else {
            zr_command_buffer_push_rect(out, zr_rect(context->bounds.x, context->bounds.y+1,
                context->bounds.w, context->header_h), 0, c->colors[ZR_COLOR_HEADER]);
        }

        /* window header icons */
        old = window->flags;
        if (window->flags & ZR_WINDOW_CLOSEABLE)
            zr_header_flag(context, &header, c->header.close_symbol, c->header.close_symbol,
                c->header.align, ZR_WINDOW_HIDDEN);
        if (window->flags & ZR_WINDOW_MINIMIZABLE)
            zr_header_flag(context, &header, c->header.maximize_symbol, c->header.minimize_symbol,
                c->header.align, ZR_WINDOW_MINIMIZED);

        /* window state change notifcations */
        if ((old & ZR_WINDOW_HIDDEN) ^ (context->flags & ZR_WINDOW_HIDDEN))
            ret |= ZR_WINDOW_CLOSEABLE;
        if ((old & ZR_WINDOW_MINIMIZED) ^ (context->flags & ZR_WINDOW_MINIMIZED))
            ret |= ZR_WINDOW_MINIMIZABLE;

        /* window header title */
        if (title) {
            zr_size text_len = zr_strsiz(title);
            struct zr_rect label = {0,0,0,0};

            /* calculate and allocate space from the header */
            zr_size t = c->font.width(c->font.userdata, c->font.height, title, text_len);
            if (c->header.align == ZR_HEADER_LEFT) {
                header.back = header.back - (3 * item_padding.x + (float)t);
                label.x = header.back;
            } else {
                label.x = header.front;
                header.front += 3 * item_padding.x + (float)t;
            }

            /* calculate label bounds and draw text */
            label.y = header.y;
            label.h = c->font.height + 2 * item_padding.y;
            label.w = MAX((float)t + 2 * item_padding.x, 4 * item_padding.x);
            zr_command_buffer_push_text(out, label, (const char*)title, text_len,
                &c->font, c->colors[ZR_COLOR_HEADER], c->colors[ZR_COLOR_TEXT]);
        }
        out->clip = old_clip;
    }

    /* draw top window border line */
    if (context->flags & ZR_WINDOW_BORDER) {
        zr_command_buffer_push_line(out, context->bounds.x, context->bounds.y,
            context->bounds.x + context->bounds.w, context->bounds.y,
            c->colors[ZR_COLOR_BORDER]);
    }

    {
        /* calculate and set the window clipping rectangle*/
        struct zr_rect clip;
        if (!(window->flags & ZR_WINDOW_DYNAMIC)) {
            context->clip.x = window->bounds.x + window_padding.x;
            context->clip.w = context->width - 2 * window_padding.x;
        } else {
            context->clip.x = window->bounds.x;
            context->clip.w = context->width;
        }

        context->clip.h = window->bounds.h - (context->footer_h + context->header_h);
        context->clip.h -= (window_padding.y + item_padding.y);
        context->clip.y = window->bounds.y;
        if (!(window->flags & ZR_WINDOW_COMBO_MENU))
            context->clip.y += context->header_h;
        if (window->flags & ZR_WINDOW_BORDER) {
            context->clip.y += 1;
            context->clip.h -= 1;
        }

        zr_unify(&clip, &context->buffer->clip, context->clip.x, context->clip.y,
            context->clip.x + context->clip.w, context->clip.y + context->clip.h);
        zr_command_buffer_push_scissor(out, clip);

        context->buffer->clip.x = context->bounds.x;
        context->buffer->clip.w = context->width;
        if (!(window->flags & ZR_WINDOW_NO_SCROLLBAR))
            context->buffer->clip.w += scrollbar_size;
    }
    return ret;
}

zr_flags
zr_end(struct zr_context *layout, struct zr_window *window)
{
    zr_flags ret = 0;
    struct zr_input *in;
    const struct zr_style *config;
    struct zr_command_buffer *out;
    float scrollbar_size;
    struct zr_vec2 item_padding;
    struct zr_vec2 item_spacing;
    struct zr_vec2 window_padding;
    struct zr_vec2 scaler_size;
    struct zr_rect footer = {0,0,0,0};

    ZR_ASSERT(layout);
    ZR_ASSERT(window);
    if (!window || !layout) return ret;

    config = layout->style;
    out = layout->buffer;
    in = (layout->flags & ZR_WINDOW_ROM) ? 0 :layout->input;
    if (!(layout->flags & ZR_WINDOW_TAB))
        zr_command_buffer_push_scissor(out, zr_null_rect);

    /* cache configuration data */
    item_padding = zr_style_property(config, ZR_PROPERTY_ITEM_PADDING);
    item_spacing = zr_style_property(config, ZR_PROPERTY_ITEM_SPACING);
    window_padding = zr_style_property(config, ZR_PROPERTY_PADDING);
    scrollbar_size = zr_style_property(config, ZR_PROPERTY_SCROLLBAR_SIZE).x;
    scaler_size = zr_style_property(config, ZR_PROPERTY_SCALER_SIZE);

    /* update the current Y-position to point over the last added widget */
    layout->at_y += layout->row.height;

    /* draw footer and fill empty spaces inside a dynamically growing panel */
    if (layout->valid && (layout->flags & ZR_WINDOW_DYNAMIC)) {
        layout->height = MIN(layout->at_y - layout->bounds.y, layout->bounds.h);
        if ((layout->offset.x == 0) || (layout->flags & ZR_WINDOW_NO_SCROLLBAR)) {
            footer.x = window->bounds.x;
            footer.y = window->bounds.y + layout->height + item_spacing.y;
            footer.w = window->bounds.w + scrollbar_size;
            layout->footer_h = 0;
        } else {
            footer.x = window->bounds.x;
            footer.w = window->bounds.w + scrollbar_size;
            footer.h = layout->footer_h;
            if (layout->flags & ZR_WINDOW_COMBO_MENU)
                footer.y = window->bounds.y + layout->height;
            else footer.y = window->bounds.y + layout->height + layout->footer_h;
            zr_command_buffer_push_rect(out, footer, 0, config->colors[ZR_COLOR_WINDOW]);

            if (!(layout->flags & ZR_WINDOW_COMBO_MENU)) {
                struct zr_rect bounds;
                bounds.x = layout->bounds.x;
                bounds.y = window->bounds.y + layout->height;
                bounds.w = layout->width;
                bounds.h = layout->row.height;
                zr_command_buffer_push_rect(out, bounds, 0, config->colors[ZR_COLOR_WINDOW]);
            }
        }
    }

    /* scrollbars */
    if (layout->valid && !(layout->flags & ZR_WINDOW_NO_SCROLLBAR)) {
        struct zr_rect bounds;
        float scroll_target, scroll_offset, scroll_step;

        struct zr_scrollbar scroll;
        scroll.rounding = config->rounding[ZR_ROUNDING_SCROLLBAR];
        scroll.background = config->colors[ZR_COLOR_SCROLLBAR];
        scroll.normal = config->colors[ZR_COLOR_SCROLLBAR_CURSOR];
        scroll.hover = config->colors[ZR_COLOR_SCROLLBAR_CURSOR_HOVER];
        scroll.active = config->colors[ZR_COLOR_SCROLLBAR_CURSOR_ACTIVE];
        scroll.border = config->colors[ZR_COLOR_BORDER];
        {
            /* vertical scollbar */
            bounds.x = layout->bounds.x + layout->width;
            bounds.y = (layout->flags & ZR_WINDOW_BORDER) ?
                        layout->bounds.y + 1 : layout->bounds.y;
            bounds.y += layout->header_h + layout->menu.h;
            bounds.w = scrollbar_size;
            bounds.h = layout->height;
            if (layout->flags & ZR_WINDOW_BORDER) bounds.h -= 1;

            scroll_offset = layout->offset.y;
            scroll_step = layout->clip.h * 0.10f;
            scroll_target = (layout->at_y - layout->bounds.y)-(layout->header_h+2*item_spacing.y);
            scroll.has_scrolling = (layout->flags & ZR_WINDOW_ACTIVE);
            window->offset.y = zr_widget_scrollbarv(out, bounds, scroll_offset,
                                scroll_target, scroll_step, &scroll, in);
        }
        {
            /* horizontal scrollbar */
            bounds.x = layout->bounds.x + window_padding.x;
            if (layout->flags & ZR_WINDOW_TAB) {
                bounds.h = scrollbar_size;
                bounds.y = (layout->flags & ZR_WINDOW_BORDER) ?
                            layout->bounds.y + 1 : layout->bounds.y;
                bounds.y += layout->header_h + layout->menu.h + layout->height;
                bounds.w = layout->clip.w;
            } else if (layout->flags & ZR_WINDOW_DYNAMIC) {
                bounds.h = MIN(scrollbar_size, layout->footer_h);
                bounds.w = layout->bounds.w;
                bounds.y = footer.y;
            } else {
                bounds.h = MIN(scrollbar_size, layout->footer_h);
                bounds.y = layout->bounds.y + window->bounds.h - MAX(layout->footer_h, scrollbar_size);
                bounds.w = layout->width - 2 * window_padding.x;
            }
            scroll_offset = layout->offset.x;
            scroll_target = layout->max_x - bounds.x;
            scroll_step = layout->max_x * 0.05f;
            scroll.has_scrolling = zr_false;
            window->offset.x = zr_widget_scrollbarh(out, bounds, scroll_offset,
                                    scroll_target, scroll_step, &scroll, in);
        }
    };

    /* draw the panel scaler into the right corner of the panel footer and
     * update panel size if user drags the scaler */
    if ((layout->flags & ZR_WINDOW_SCALEABLE) && layout->valid && in) {
        float scaler_y;
        struct zr_color col = config->colors[ZR_COLOR_SCALER];
        float scaler_w = MAX(0, scaler_size.x - item_padding.x);
        float scaler_h = MAX(0, scaler_size.y - item_padding.y);
        float scaler_x = (layout->bounds.x + layout->bounds.w) - (item_padding.x + scaler_w);

        if (layout->flags & ZR_WINDOW_DYNAMIC)
            scaler_y = footer.y + layout->footer_h - scaler_size.y;
        else scaler_y = layout->bounds.y + layout->bounds.h - scaler_size.y;
        zr_command_buffer_push_triangle(out, scaler_x + scaler_w, scaler_y,
            scaler_x + scaler_w, scaler_y + scaler_h, scaler_x, scaler_y + scaler_h, col);

        if (!(window->flags & ZR_WINDOW_ROM)) {
            int incursor;
            float prev_x = in->mouse.prev.x;
            float prev_y = in->mouse.prev.y;
            struct zr_vec2 window_size = zr_style_property(config, ZR_PROPERTY_SIZE);
            incursor = ZR_INBOX(prev_x,prev_y,scaler_x,scaler_y,scaler_w,scaler_h);

            if (zr_input_is_mouse_down(in, ZR_BUTTON_LEFT) && incursor) {
                window->bounds.w = MAX(window_size.x, window->bounds.w + in->mouse.delta.x);
                /* draging in y-direction is only possible if static window */
                ret = ZR_WINDOW_SCALEABLE;
                if (!(layout->flags & ZR_WINDOW_DYNAMIC))
                    window->bounds.h = MAX(window_size.y, window->bounds.h + in->mouse.delta.y);
            }
        }
    }

    if (layout->flags & ZR_WINDOW_BORDER) {
        /* draw the border around the complete panel */
        const float width = (layout->flags & ZR_WINDOW_NO_SCROLLBAR) ?
            layout->width: layout->width + scrollbar_size;
        const float padding_y = (!layout->valid) ?
            window->bounds.y + layout->header_h:
            (layout->flags & ZR_WINDOW_DYNAMIC)?
            layout->footer_h + footer.y:
            layout->bounds.y + layout->bounds.h;

        if (window->flags & ZR_WINDOW_BORDER_HEADER)
            zr_command_buffer_push_line(out, window->bounds.x, window->bounds.y + layout->header_h,
                window->bounds.x + window->bounds.w, window->bounds.y + layout->header_h,
                config->colors[ZR_COLOR_BORDER]);
        zr_command_buffer_push_line(out, window->bounds.x, padding_y, window->bounds.x + width,
                padding_y, config->colors[ZR_COLOR_BORDER]);
        zr_command_buffer_push_line(out, window->bounds.x, window->bounds.y, window->bounds.x,
                padding_y, config->colors[ZR_COLOR_BORDER]);
        zr_command_buffer_push_line(out, window->bounds.x + width, window->bounds.y,
                window->bounds.x + width, padding_y, config->colors[ZR_COLOR_BORDER]);
    }

    if (!(window->flags & ZR_WINDOW_TAB)) {
        /* window is hidden so clear command buffer  */
        if (layout->flags & ZR_WINDOW_HIDDEN)
            zr_command_buffer_reset(&window->buffer);
        /* window is visible and not tab */
        else zr_command_queue_finish(window->queue, &window->buffer);
    }

    /* remove ROM flag was set so remove ROM FLAG*/
    if (layout->flags & ZR_WINDOW_REMOVE_ROM) {
        layout->flags &= ~(zr_flags)ZR_WINDOW_ROM;
        layout->flags &= ~(zr_flags)ZR_WINDOW_REMOVE_ROM;
    }
    window->flags = layout->flags;
    return ret;
}

struct zr_command_buffer*
zr_canvas(struct zr_context *layout)
{return layout->buffer;}

const struct zr_input*
zr_input(struct zr_context *layout)
{return layout->input;}

struct zr_command_queue*
zr_queue(struct zr_context *layout)
{return layout->queue;}

struct zr_rect
zr_space(struct zr_context *layout)
{ZR_ASSERT(layout); return layout->clip;}

void
zr_menubar_begin(struct zr_context *layout)
{
    ZR_ASSERT(layout);
    if (!layout || layout->flags & ZR_WINDOW_HIDDEN || layout->flags & ZR_WINDOW_MINIMIZED)
        return;
    layout->menu.x = layout->at_x;
    layout->menu.y = layout->bounds.y + layout->header_h;
    layout->menu.w = layout->width;
    layout->menu.offset = layout->offset;
    layout->offset.y = 0;
}

void
zr_menubar_end(struct zr_context *layout)
{
    struct zr_command_buffer *out;
    ZR_ASSERT(layout);
    if (!layout || layout->flags & ZR_WINDOW_HIDDEN || layout->flags & ZR_WINDOW_MINIMIZED)
        return;

    out = layout->buffer;
    layout->menu.h = layout->at_y - layout->menu.y;
    layout->clip.y = layout->bounds.y + layout->header_h + layout->menu.h + layout->row.height;
    layout->height -= layout->menu.h;
    layout->offset = layout->menu.offset;
    layout->clip.h -= layout->menu.h + layout->row.height;
    layout->at_y = layout->menu.y + layout->menu.h;
    zr_command_buffer_push_scissor(out, layout->clip);
}
/*
 * -------------------------------------------------------------
 *
 *                          LAYOUT
 *
 * --------------------------------------------------------------
 */
static void
zr_panel_layout(struct zr_context *layout, float height, zr_size cols)
{
    const struct zr_style *config;
    const struct zr_color *color;
    struct zr_command_buffer *out;
    struct zr_vec2 item_spacing;
    struct zr_vec2 panel_padding;

    ZR_ASSERT(layout);
    ZR_ASSERT(layout->style);
    ZR_ASSERT(layout->buffer);

    if (!layout) return;
    if (!layout->valid) return;

    /* prefetch some configuration data */
    config = layout->style;
    out = layout->buffer;
    color = &config->colors[ZR_COLOR_WINDOW];
    item_spacing = zr_style_property(config, ZR_PROPERTY_ITEM_SPACING);
    panel_padding = zr_style_property(config, ZR_PROPERTY_PADDING);

    /* draw the current row and set the current row layout */
    layout->row.index = 0;
    layout->at_y += layout->row.height;
    layout->row.columns = cols;
    layout->row.height = height + item_spacing.y;
    layout->row.item_offset = 0;
    if (layout->flags & ZR_WINDOW_DYNAMIC)
        zr_command_buffer_push_rect(out,  zr_rect(layout->bounds.x, layout->at_y,
            layout->bounds.w, height + panel_padding.y), 0, *color);
}

static void
zr_row_layout(struct zr_context *layout,
    enum zr_layout_format fmt, float height, zr_size cols,
    zr_size width)
{
    ZR_ASSERT(layout);
    ZR_ASSERT(layout->style);
    ZR_ASSERT(layout->buffer);

    if (!layout) return;
    if (!layout->valid) return;

    /* draw the current row and set the current row layout */
    zr_panel_layout(layout, height, cols);
    if (fmt == ZR_DYNAMIC)
        layout->row.type = ZR_LAYOUT_DYNAMIC_FIXED;
    else layout->row.type = ZR_LAYOUT_STATIC_FIXED;

    layout->row.item_width = (float)width;
    layout->row.ratio = 0;
    layout->row.item_offset = 0;
    layout->row.filled = 0;
}

void
zr_layout_row_dynamic(struct zr_context *layout, float height, zr_size cols)
{zr_row_layout(layout, ZR_DYNAMIC, height, cols, 0);}

void
zr_layout_row_static(struct zr_context *layout, float height,
    zr_size item_width, zr_size cols)
{zr_row_layout(layout, ZR_STATIC, height, cols, item_width);}

void
zr_layout_row_begin(struct zr_context *layout,
    enum zr_layout_format fmt, float row_height, zr_size cols)
{
    ZR_ASSERT(layout);
    ZR_ASSERT(layout->style);
    ZR_ASSERT(layout->buffer);

    if (!layout) return;
    if (!layout->valid) return;

    zr_panel_layout(layout, row_height, cols);
    if (fmt == ZR_DYNAMIC)
        layout->row.type = ZR_LAYOUT_DYNAMIC_ROW;
    else layout->row.type = ZR_LAYOUT_STATIC_ROW;

    layout->row.ratio = 0;
    layout->row.item_width = 0;
    layout->row.item_offset = 0;
    layout->row.filled = 0;
    layout->row.columns = cols;
}

void
zr_layout_row_push(struct zr_context *layout, float ratio_or_width)
{
    ZR_ASSERT(layout);
    ZR_ASSERT(layout->style);
    if (!layout || !layout->valid) return;

    if (layout->row.type == ZR_LAYOUT_DYNAMIC_ROW) {
        float ratio = ratio_or_width;
        if ((ratio + layout->row.filled) > 1.0f) return;
        if (ratio > 0.0f)
            layout->row.item_width = ZR_SATURATE(ratio);
        else layout->row.item_width = 1.0f - layout->row.filled;
    } else layout->row.item_width = ratio_or_width;
}

void
zr_layout_row_end(struct zr_context *layout)
{
    ZR_ASSERT(layout);
    ZR_ASSERT(layout->style);
    if (!layout) return;
    if (!layout->valid) return;
    layout->row.item_width = 0;
    layout->row.item_offset = 0;
}

void
zr_layout_row(struct zr_context *layout, enum zr_layout_format fmt,
    float height, zr_size cols, const float *ratio)
{
    zr_size i;
    zr_size n_undef = 0;
    float r = 0;

    ZR_ASSERT(layout);
    ZR_ASSERT(layout->style);
    ZR_ASSERT(layout->buffer);

    if (!layout) return;
    if (!layout->valid) return;

    zr_panel_layout(layout, height, cols);
    if (fmt == ZR_DYNAMIC) {
        /* calculate width of undefined widget ratios */
        layout->row.ratio = ratio;
        for (i = 0; i < cols; ++i) {
            if (ratio[i] < 0.0f)
                n_undef++;
            else r += ratio[i];
        }
        r = ZR_SATURATE(1.0f - r);
        layout->row.type = ZR_LAYOUT_DYNAMIC;
        layout->row.item_width = (r > 0 && n_undef > 0) ? (r / (float)n_undef):0;
    } else {
        layout->row.ratio = ratio;
        layout->row.type = ZR_LAYOUT_STATIC;
        layout->row.item_width = 0;
        layout->row.item_offset = 0;
    }
    layout->row.item_offset = 0;
    layout->row.filled = 0;
}

void
zr_layout_row_space_begin(struct zr_context *layout,
    enum zr_layout_format fmt, float height, zr_size widget_count)
{
    ZR_ASSERT(layout);
    ZR_ASSERT(layout->style);
    ZR_ASSERT(layout->buffer);

    if (!layout) return;
    if (!layout->valid) return;

    zr_panel_layout(layout, height, widget_count);
    if (fmt == ZR_STATIC) {
        /* calculate bounds of the free to use panel space */
        struct zr_rect clip, space;
        space.x = layout->at_x;
        space.y = layout->at_y;
        space.w = layout->width;
        space.h = layout->row.height;

        /* setup clipping rect for the free space to prevent overdraw  */
        zr_unify(&clip, &layout->clip, space.x, space.y, space.x + space.w, space.y + space.h);
        zr_command_buffer_push_scissor(layout->buffer, clip);

        layout->row.type = ZR_LAYOUT_STATIC_FREE;
        layout->row.clip = layout->clip;
        layout->clip = clip;
    } else layout->row.type = ZR_LAYOUT_DYNAMIC_FREE;

    layout->row.ratio = 0;
    layout->row.item_width = 0;
    layout->row.item_offset = 0;
    layout->row.filled = 0;
}

void
zr_layout_row_space_push(struct zr_context *layout, struct zr_rect rect)
{
    ZR_ASSERT(layout);
    ZR_ASSERT(layout->style);
    ZR_ASSERT(layout->buffer);
    if (!layout) return;
    if (!layout->valid) return;
    layout->row.item = rect;
}

struct zr_rect
zr_layout_row_space_bounds(struct zr_context *ctx)
{
    struct zr_rect ret;
    ret.x = ctx->clip.x;
    ret.y = ctx->clip.y;
    ret.w = ctx->clip.w;
    ret.h = ctx->row.height;
    return ret;
}

struct zr_vec2
zr_layout_row_space_to_screen(struct zr_context *layout, struct zr_vec2 ret)
{
    ZR_ASSERT(layout);
    ret.x += layout->clip.x - layout->offset.x;
    ret.y += layout->clip.y - layout->offset.y;
    return ret;
}

struct zr_vec2
zr_layout_row_space_to_local(struct zr_context *layout, struct zr_vec2 ret)
{
    ZR_ASSERT(layout);
    ret.x += -layout->clip.x + layout->offset.x;
    ret.y += -layout->clip.y + layout->offset.y;
    return ret;
}

struct zr_rect
zr_layout_row_space_rect_to_screen(struct zr_context *layout, struct zr_rect ret)
{
    ZR_ASSERT(layout);
    ret.x += layout->clip.x - layout->offset.x;
    ret.y += layout->clip.y - layout->offset.y;
    return ret;
}

struct zr_rect
zr_layout_row_space_rect_to_local(struct zr_context *layout, struct zr_rect ret)
{
    ZR_ASSERT(layout);
    ret.x += -layout->clip.x + layout->offset.x;
    ret.y += -layout->clip.y + layout->offset.y;
    return ret;
}

void
zr_layout_row_space_end(struct zr_context *layout)
{
    ZR_ASSERT(layout);
    ZR_ASSERT(layout->style);
    if (!layout) return;
    if (!layout->valid) return;

    layout->row.item_width = 0;
    layout->row.item_height = 0;
    layout->row.item_offset = 0;
    zr_zero(&layout->row.item, sizeof(layout->row.item));
    if (layout->row.type == ZR_LAYOUT_STATIC_FREE)
        zr_command_buffer_push_scissor(layout->buffer, layout->clip);
}

static void
zr_panel_alloc_row(struct zr_context *layout)
{
    const struct zr_style *c = layout->style;
    struct zr_vec2 spacing = zr_style_property(c, ZR_PROPERTY_ITEM_SPACING);
    const float row_height = layout->row.height - spacing.y;
    zr_panel_layout(layout, row_height, layout->row.columns);
}

static void
zr_layout_widget_space(struct zr_rect *bounds, struct zr_context *layout,
    int modify)
{
    const struct zr_style *config;
    float panel_padding, panel_spacing, panel_space;
    float item_offset = 0, item_width = 0, item_spacing = 0;
    struct zr_vec2 spacing, padding;

    ZR_ASSERT(layout);
    ZR_ASSERT(layout->style);
    ZR_ASSERT(bounds);
    if (!layout || !layout->style || !bounds)
        return;

    /* cache some configuration data */
    config = layout->style;
    spacing = zr_style_property(config, ZR_PROPERTY_ITEM_SPACING);
    padding = zr_style_property(config, ZR_PROPERTY_PADDING);

    /* calculate the useable panel space */
    panel_padding = 2 * padding.x;
    panel_spacing = (float)(layout->row.columns - 1) * spacing.x;
    panel_space  = layout->width - panel_padding - panel_spacing;

    /* calculate the width of one item inside the current layout space */
    switch (layout->row.type) {
    case ZR_LAYOUT_DYNAMIC_FIXED: {
        /* scaling fixed size widgets item width */
        item_width = panel_space / (float)layout->row.columns;
        item_offset = (float)layout->row.index * item_width;
        item_spacing = (float)layout->row.index * spacing.x;
    } break;
    case ZR_LAYOUT_DYNAMIC_ROW: {
        /* scaling single ratio widget width */
        item_width = layout->row.item_width * panel_space;
        item_offset = layout->row.item_offset;
        item_spacing = (float)layout->row.index * spacing.x;

        if (modify) {
            layout->row.item_offset += item_width + spacing.x;
            layout->row.filled += layout->row.item_width;
            layout->row.index = 0;
        }
    } break;
    case ZR_LAYOUT_DYNAMIC_FREE: {
        /* panel width depended free widget placing */
        bounds->x = layout->at_x + (layout->width * layout->row.item.x);
        bounds->x -= layout->offset.x;
        bounds->y = layout->at_y + (layout->row.height * layout->row.item.y);
        bounds->w = layout->width  * layout->row.item.w;
        bounds->h = layout->row.height * layout->row.item.h;
        return;
    } break;
    case ZR_LAYOUT_DYNAMIC: {
        /* scaling arrays of panel width ratios for every widget */
        float ratio;
        ZR_ASSERT(layout->row.ratio);
        ratio = (layout->row.ratio[layout->row.index] < 0) ?
            layout->row.item_width : layout->row.ratio[layout->row.index];

        item_spacing = (float)layout->row.index * spacing.x;
        if (layout->row.index < layout->row.columns-1)
            item_width = (ratio * panel_space) - spacing.x;
        else item_width = (ratio * panel_space);

        item_offset = layout->row.item_offset;
        if (modify) {
            layout->row.item_offset += item_width + spacing.x;
            layout->row.filled += ratio;
        }
    } break;
    case ZR_LAYOUT_STATIC_FIXED: {
        /* non-scaling fixed widgets item width */
        item_width = layout->row.item_width;
        item_offset = (float)layout->row.index * item_width;
        item_spacing = (float)layout->row.index * spacing.x;
    } break;
    case ZR_LAYOUT_STATIC_ROW: {
        /* scaling single ratio widget width */
        item_width = layout->row.item_width;
        item_offset = layout->row.item_offset;
        item_spacing = (float)layout->row.index * spacing.x;
        if (modify) {
            layout->row.item_offset += item_width + spacing.x;
            layout->row.index = 0;
        }
    } break;
    case ZR_LAYOUT_STATIC_FREE: {
        /* free widget placing */
        bounds->x = layout->clip.x + layout->row.item.x;
        if (((bounds->x + bounds->w) > layout->max_x) && modify)
            layout->max_x = (bounds->x + bounds->w);
        bounds->x -= layout->offset.x;
        bounds->y = layout->clip.y + layout->row.item.y;
        bounds->y -= layout->offset.y;
        bounds->w = layout->row.item.w;
        bounds->h = layout->row.item.h;
        return;
    } break;
    case ZR_LAYOUT_STATIC: {
        /* non-scaling array of panel pixel width for every widget */
        item_spacing = (float)layout->row.index * spacing.x;
        item_width = layout->row.ratio[layout->row.index];
        item_offset = layout->row.item_offset;
        if (modify) layout->row.item_offset += item_width + spacing.x;
        } break;
    default: break;
    };

    /* set the bounds of the newly allocated widget */
    bounds->w = item_width;
    bounds->h = layout->row.height - spacing.y;
    bounds->y = layout->at_y - layout->offset.y;
    bounds->x = layout->at_x + item_offset + item_spacing + padding.x;
    if (((bounds->x + bounds->w) > layout->max_x) && modify)
        layout->max_x = bounds->x + bounds->w;
    bounds->x -= layout->offset.x;
}

static void
zr_panel_alloc_space(struct zr_rect *bounds, struct zr_context *layout)
{
    /* check if the end of the row has been hit and begin new row if so */
    if (layout->row.index >= layout->row.columns)
        zr_panel_alloc_row(layout);

    /* calculate widget position and size */
    zr_layout_widget_space(bounds, layout, zr_true);
    layout->row.index++;
}

void
zr_layout_peek(struct zr_rect *bounds, struct zr_context *layout)
{
    float y = layout->at_y;
    zr_size index = layout->row.index;
    if (layout->row.index >= layout->row.columns) {
        layout->at_y += layout->row.height;
        layout->row.index = 0;
    }
    zr_layout_widget_space(bounds, layout, zr_false);
    layout->at_y = y;
    layout->row.index = index;
}

int
zr_layout_push(struct zr_context *layout,
    enum zr_layout_node_type type,
    const char *title, int *state)
{
    const struct zr_style *config;
    struct zr_command_buffer *out;
    struct zr_vec2 item_spacing;
    struct zr_vec2 item_padding;
    struct zr_vec2 panel_padding;
    struct zr_rect header = {0,0,0,0};
    struct zr_rect sym = {0,0,0,0};

    ZR_ASSERT(layout);
    ZR_ASSERT(layout->style);
    ZR_ASSERT(layout->buffer);

    if (!layout) return zr_false;
    if (!layout->valid) return zr_false;

    /* cache some data */
    out = layout->buffer;
    config = layout->style;

    item_spacing = zr_style_property(config, ZR_PROPERTY_ITEM_SPACING);
    item_padding = zr_style_property(config, ZR_PROPERTY_ITEM_PADDING);
    panel_padding = zr_style_property(config, ZR_PROPERTY_PADDING);

    /* calculate header bounds and draw background */
    zr_layout_row_dynamic(layout, config->font.height + 2 * item_padding.y, 1);
    zr_panel_alloc_space(&header, layout);
    if (type == ZR_LAYOUT_TAB)
        zr_command_buffer_push_rect(out, header, 0, config->colors[ZR_COLOR_TAB_HEADER]);

    {
        /* and draw closing/open icon */
        enum zr_heading heading;
        struct zr_vec2 points[3];
        heading = (*state == ZR_MAXIMIZED) ? ZR_DOWN : ZR_RIGHT;

        /* calculate the triangle bounds */
        sym.w = sym.h = config->font.height;
        sym.y = header.y + item_padding.y;
        sym.x = header.x + panel_padding.x;

        /* calculate the triangle points and draw triangle */
        zr_triangle_from_direction(points, sym, 0, 0, heading);
        zr_command_buffer_push_triangle(layout->buffer,  points[0].x, points[0].y,
            points[1].x, points[1].y, points[2].x, points[2].y, config->colors[ZR_COLOR_TEXT]);

        /* calculate the space the icon occupied */
        sym.w = config->font.height + 2 * item_padding.x;
    }
    {
        /* draw node label */
        struct zr_color color;
        struct zr_rect label;
        zr_size text_len;

        header.w = MAX(header.w, sym.w + item_spacing.y + panel_padding.x);
        label.x = sym.x + sym.w + item_spacing.x;
        label.y = sym.y;
        label.w = header.w - (sym.w + item_spacing.y + panel_padding.x);
        label.h = config->font.height;
        text_len = zr_strsiz(title);
        color = (type == ZR_LAYOUT_TAB) ? config->colors[ZR_COLOR_TAB_HEADER]:
            config->colors[ZR_COLOR_WINDOW];
        zr_command_buffer_push_text(out, label, (const char*)title, text_len,
            &config->font, color, config->colors[ZR_COLOR_TEXT]);
    }

    /* update node state */
    if (!(layout->flags & ZR_WINDOW_ROM)) {
        float mouse_x = layout->input->mouse.pos.x;
        float mouse_y = layout->input->mouse.pos.y;
        float clicked_x = layout->input->mouse.buttons[ZR_BUTTON_LEFT].clicked_pos.x;
        float clicked_y = layout->input->mouse.buttons[ZR_BUTTON_LEFT].clicked_pos.y;
        if (ZR_INBOX(mouse_x, mouse_y, sym.x, sym.y, sym.w, sym.h)) {
            if (ZR_INBOX(clicked_x, clicked_y, sym.x, sym.y, sym.w, sym.h)) {
                if (layout->input->mouse.buttons[ZR_BUTTON_LEFT].down &&
                    layout->input->mouse.buttons[ZR_BUTTON_LEFT].clicked)
                    *state = (*state == ZR_MAXIMIZED) ? ZR_MINIMIZED : ZR_MAXIMIZED;
            }
        }
    }

    if (type == ZR_LAYOUT_TAB) {
        /* special node with border around the header */
        zr_command_buffer_push_line(out, header.x, header.y,
            header.x + header.w-1, header.y, config->colors[ZR_COLOR_BORDER]);
        zr_command_buffer_push_line(out, header.x, header.y,
            header.x, header.y + header.h, config->colors[ZR_COLOR_BORDER]);
        zr_command_buffer_push_line(out, header.x + header.w-1, header.y,
            header.x + header.w-1, header.y + header.h, config->colors[ZR_COLOR_BORDER]);
        zr_command_buffer_push_line(out, header.x, header.y + header.h,
            header.x + header.w-1, header.y + header.h, config->colors[ZR_COLOR_BORDER]);
    }

    if (*state == ZR_MAXIMIZED) {
        layout->at_x = header.x + layout->offset.x;
        layout->width = MAX(layout->width, 2 * panel_padding.x);
        layout->width -= 2 * panel_padding.x;
        return zr_true;
    } else return zr_false;
}

void
zr_layout_pop(struct zr_context *layout)
{
    const struct zr_style *config;
    struct zr_vec2 panel_padding;

    ZR_ASSERT(layout);
    ZR_ASSERT(layout->style);
    ZR_ASSERT(layout->buffer);

    if (!layout) return;
    if (!layout->valid) return;

    config = layout->style;
    panel_padding = zr_style_property(config, ZR_PROPERTY_PADDING);
    layout->at_x -= panel_padding.x;
    layout->width += 2 * panel_padding.x;
}
/*----------------------------------------------------------------
 *
 *                      WINDOW WIDGETS
 *
 * --------------------------------------------------------------*/
void
zr_spacing(struct zr_context *l, zr_size cols)
{
    zr_size i, index, rows;
    struct zr_rect nil;

    ZR_ASSERT(l);
    ZR_ASSERT(l->style);
    ZR_ASSERT(l->buffer);
    if (!l) return;
    if (!l->valid) return;

    /* spacing over row boundries */
    index = (l->row.index + cols) % l->row.columns;
    rows = (l->row.index + cols) / l->row.columns;
    if (rows) {
        for (i = 0; i < rows; ++i)
            zr_panel_alloc_row(l);
        cols = index;
    }

    /* non table layout need to allocate space */
    if (l->row.type != ZR_LAYOUT_DYNAMIC_FIXED &&
        l->row.type != ZR_LAYOUT_STATIC_FIXED) {
        for (i = 0; i < cols; ++i)
            zr_panel_alloc_space(&nil, l);
    }
    l->row.index = index;
}

void
zr_seperator(struct zr_context *layout)
{
    struct zr_command_buffer *out;
    const struct zr_style *config;
    struct zr_vec2 item_padding;
    struct zr_vec2 item_spacing;
    struct zr_rect bounds;
    ZR_ASSERT(layout);
    if (!layout) return;

    out = layout->buffer;
    config = layout->style;
    item_padding = zr_style_property(config, ZR_PROPERTY_ITEM_PADDING);
    item_spacing = zr_style_property(config, ZR_PROPERTY_ITEM_SPACING);

    bounds.h = 1;
    bounds.w = MAX(layout->width, 2 * item_spacing.x + 2 * item_padding.x);
    bounds.y = (layout->at_y + layout->row.height + item_padding.y) - layout->offset.y;
    bounds.x = layout->at_x + item_spacing.x + item_padding.x - layout->offset.x;
    bounds.w = bounds.w - (2 * item_spacing.x + 2 * item_padding.x);
    zr_command_buffer_push_line(out, bounds.x, bounds.y, bounds.x + bounds.w,
        bounds.y + bounds.h, config->colors[ZR_COLOR_BORDER]);
}

enum zr_widget_state
zr_widget(struct zr_rect *bounds, struct zr_context *layout)
{
    struct zr_rect *c = 0;
    ZR_ASSERT(layout);
    ZR_ASSERT(layout->style);
    ZR_ASSERT(layout->buffer);

    if (!layout) return ZR_WIDGET_INVALID;
    if (!layout->valid || !layout->style || !layout->buffer) return ZR_WIDGET_INVALID;

    /* allocated space for the panel and check if the widget needs to be updated */
    zr_panel_alloc_space(bounds, layout);
    c = &layout->clip;
    if (!ZR_INTERSECT(c->x, c->y, c->w, c->h, bounds->x, bounds->y, bounds->w, bounds->h))
        return ZR_WIDGET_INVALID;
    if (!ZR_CONTAINS(bounds->x, bounds->y, bounds->w, bounds->h, c->x, c->y, c->w, c->h))
        return ZR_WIDGET_ROM;
    return ZR_WIDGET_VALID;
}

enum zr_widget_state
zr_widget_fitting(struct zr_rect *bounds, struct zr_context *layout)
{
    /* update the bounds to stand without padding  */
    enum zr_widget_state state;
    ZR_ASSERT(layout);
    ZR_ASSERT(layout->style);
    ZR_ASSERT(layout->buffer);
    if (!layout) return ZR_WIDGET_INVALID;
    if (!layout->valid || !layout->style || !layout->buffer)
        return ZR_WIDGET_INVALID;

    state = zr_widget(bounds, layout);
    if (layout->row.index == 1) {
        bounds->w += layout->style->properties[ZR_PROPERTY_PADDING].x;
        bounds->x -= layout->style->properties[ZR_PROPERTY_PADDING].x;
    } else bounds->x -= layout->style->properties[ZR_PROPERTY_ITEM_PADDING].x;
    if (layout->row.index == layout->row.columns)
        bounds->w += layout->style->properties[ZR_PROPERTY_PADDING].x;
    else bounds->w += layout->style->properties[ZR_PROPERTY_ITEM_PADDING].x;
    return state;
}

void
zr_text_colored(struct zr_context *layout, const char *str, zr_size len,
    enum zr_text_align alignment, struct zr_color color)
{
    struct zr_rect bounds;
    struct zr_text text;
    const struct zr_style *config;
    struct zr_vec2 item_padding;

    ZR_ASSERT(layout);
    ZR_ASSERT(layout->style);
    ZR_ASSERT(layout->buffer);

    if (!layout) return;
    if (!layout->valid || !layout->style || !layout->buffer) return;
    zr_panel_alloc_space(&bounds, layout);
    config = layout->style;
    item_padding = zr_style_property(config, ZR_PROPERTY_ITEM_PADDING);

    text.padding.x = item_padding.x;
    text.padding.y = item_padding.y;
    text.background = config->colors[ZR_COLOR_WINDOW];
    text.text = color;
    zr_widget_text(layout->buffer, bounds, str, len, &text, alignment, &config->font);
}

void
zr_text_wrap_colored(struct zr_context *layout, const char *str,
    zr_size len, struct zr_color color)
{
    struct zr_rect bounds;
    struct zr_text text;
    const struct zr_style *config;
    struct zr_vec2 item_padding;

    ZR_ASSERT(layout);
    ZR_ASSERT(layout->style);
    ZR_ASSERT(layout->buffer);

    if (!layout) return;
    if (!layout->valid || !layout->style || !layout->buffer) return;
    zr_panel_alloc_space(&bounds, layout);
    config = layout->style;
    item_padding = zr_style_property(config, ZR_PROPERTY_ITEM_PADDING);

    text.padding.x = item_padding.x;
    text.padding.y = item_padding.y;
    text.background = config->colors[ZR_COLOR_WINDOW];
    text.text = color;
    zr_widget_text_wrap(layout->buffer, bounds, str, len, &text, &config->font);
}

void
zr_text_wrap(struct zr_context *l, const char *str, zr_size len)
{zr_text_wrap_colored(l, str, len, l->style->colors[ZR_COLOR_TEXT]);}

void
zr_text(struct zr_context *l, const char *str, zr_size len,
    enum zr_text_align alignment)
{zr_text_colored(l, str, len, alignment, l->style->colors[ZR_COLOR_TEXT]);}

void
zr_label_colored(struct zr_context *layout, const char *text,
    enum zr_text_align align, struct zr_color color)
{zr_text_colored(layout, text, zr_strsiz(text), align, color);}

void
zr_label(struct zr_context *layout, const char *text,
    enum zr_text_align align)
{zr_text(layout, text, zr_strsiz(text), align);}

void
zr_label_wrap(struct zr_context *l, const char *str)
{zr_text_wrap(l, str, zr_strsiz(str));}

void
zr_label_colored_wrap(struct zr_context *l, const char *str, struct zr_color color)
{zr_text_wrap_colored(l, str, zr_strsiz(str), color);}

void
zr_image(struct zr_context *layout, struct zr_image img)
{
    const struct zr_style *config;
    struct zr_vec2 item_padding;
    struct zr_rect bounds;
    ZR_ASSERT(layout);
    if (!zr_widget(&bounds, layout))
        return;

    config = layout->style;
    item_padding = zr_style_property(config, ZR_PROPERTY_ITEM_PADDING);
    bounds.x += item_padding.x;
    bounds.y += item_padding.y;
    bounds.w -= 2 * item_padding.x;
    bounds.h -= 2 * item_padding.y;
    zr_command_buffer_push_image(layout->buffer, bounds, &img);
}

static void
zr_fill_button(const struct zr_style *config, struct zr_button *button)
{
    struct zr_vec2 item_padding;
    zr_zero(button, sizeof(*button));
    item_padding = zr_style_property(config, ZR_PROPERTY_ITEM_PADDING);
    button->rounding = config->rounding[ZR_ROUNDING_BUTTON];
    button->normal = config->colors[ZR_COLOR_BUTTON];
    button->hover = config->colors[ZR_COLOR_BUTTON_HOVER];
    button->active = config->colors[ZR_COLOR_BUTTON_ACTIVE];
    button->border = config->colors[ZR_COLOR_BORDER];
    button->border_width = 1;
    button->padding.x = item_padding.x;
    button->padding.y = item_padding.y;
}

enum zr_button_alloc {ZR_BUTTON_NORMAL, ZR_BUTTON_FITTING};
static enum zr_widget_state
zr_button(struct zr_button *button, struct zr_rect *bounds,
    struct zr_context *layout, enum zr_button_alloc type)
{
    const struct zr_style *config;
    enum zr_widget_state state;
    if (type == ZR_BUTTON_NORMAL)
        state = zr_widget(bounds, layout);
    else state = zr_widget_fitting(bounds, layout);
    if (!state) return state;
    zr_zero(button, sizeof(*button));
    config = layout->style;
    zr_fill_button(config, button);
    return state;
}

int
zr_button_text(struct zr_context *layout, const char *str,
    enum zr_button_behavior behavior)
{
    struct zr_rect bounds;
    struct zr_button_text button;
    const struct zr_style *config;

    const struct zr_input *i;
    enum zr_widget_state state;
    state = zr_button(&button.base, &bounds, layout, ZR_BUTTON_NORMAL);
    if (!state) return zr_false;
    i = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : layout->input;

    config = layout->style;
    button.alignment = ZR_TEXT_CENTERED;
    button.normal = config->colors[ZR_COLOR_TEXT];
    button.hover = config->colors[ZR_COLOR_TEXT_HOVERING];
    button.active = config->colors[ZR_COLOR_TEXT_ACTIVE];
    return zr_widget_button_text(layout->buffer, bounds, str, behavior,
            &button, i, &config->font);
}

int
zr_button_color(struct zr_context *layout,
   struct zr_color color, enum zr_button_behavior behavior)
{
    struct zr_rect bounds;
    struct zr_button button;
    const struct zr_input *i;

    enum zr_widget_state state;
    state = zr_button(&button, &bounds, layout, ZR_BUTTON_NORMAL);
    if (!state) return zr_false;
    i = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : layout->input;

    button.normal = color;
    button.hover = color;
    button.active = color;
    return zr_widget_button(layout->buffer, bounds, &button, i, behavior, &bounds);
}

int
zr_button_symbol(struct zr_context *layout, enum zr_symbol symbol,
    enum zr_button_behavior behavior)
{
    struct zr_rect bounds;
    struct zr_button_symbol button;
    const struct zr_style *config;

    const struct zr_input *i;
    enum zr_widget_state state;
    state = zr_button(&button.base, &bounds, layout, ZR_BUTTON_NORMAL);
    if (!state) return zr_false;
    i = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : layout->input;

    config = layout->style;
    button.normal = config->colors[ZR_COLOR_TEXT];
    button.hover = config->colors[ZR_COLOR_TEXT_HOVERING];
    button.active = config->colors[ZR_COLOR_TEXT_ACTIVE];
    return zr_widget_button_symbol(layout->buffer, bounds, symbol,
            behavior, &button, i, &config->font);
}

int
zr_button_image(struct zr_context *layout, struct zr_image image,
    enum zr_button_behavior behavior)
{
    struct zr_rect bounds;
    struct zr_button_icon button;

    const struct zr_input *i;
    enum zr_widget_state state;
    state = zr_button(&button.base, &bounds, layout, ZR_BUTTON_NORMAL);
    if (!state) return zr_false;
    i = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : layout->input;
    button.padding = zr_vec2(0,0);
    return zr_widget_button_image(layout->buffer, bounds, image, behavior, &button, i);
}

int
zr_button_text_symbol(struct zr_context *layout, enum zr_symbol symbol,
    const char *text, enum zr_text_align align, enum zr_button_behavior behavior)
{
    struct zr_rect bounds;
    struct zr_button_text button;
    const struct zr_style *config;

    const struct zr_input *i;
    enum zr_widget_state state;
    state = zr_button(&button.base, &bounds, layout, ZR_BUTTON_NORMAL);
    if (!state) return zr_false;
    i = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : layout->input;

    config = layout->style;
    button.alignment = ZR_TEXT_CENTERED;
    button.normal = config->colors[ZR_COLOR_TEXT];
    button.hover = config->colors[ZR_COLOR_TEXT_HOVERING];
    button.active = config->colors[ZR_COLOR_TEXT_ACTIVE];
    return zr_widget_button_text_symbol(layout->buffer, bounds, symbol, text, align,
            behavior, &button, &config->font, i);
}

int
zr_button_text_image(struct zr_context *layout, struct zr_image img,
    const char *text, enum zr_text_align align, enum zr_button_behavior behavior)
{
    struct zr_rect bounds;
    struct zr_button_text button;
    const struct zr_style *config;

    const struct zr_input *i;
    enum zr_widget_state state;
    state = zr_button(&button.base, &bounds, layout, ZR_BUTTON_NORMAL);
    if (!state) return zr_false;
    i = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : layout->input;

    config = layout->style;
    button.alignment = align;
    button.normal = config->colors[ZR_COLOR_TEXT];
    button.hover = config->colors[ZR_COLOR_TEXT_HOVERING];
    button.active = config->colors[ZR_COLOR_TEXT_ACTIVE];
    return zr_widget_button_text_image(layout->buffer, bounds, img, text,
            behavior, &button, &config->font, i);
}

int
zr_select(struct zr_context *layout, const char *str,
    enum zr_text_align align, int value)
{
    struct zr_rect bounds;
    struct zr_text text;
    const struct zr_style *config;
    struct zr_vec2 item_padding;
    struct zr_color background;

    ZR_ASSERT(layout);
    ZR_ASSERT(layout->style);
    ZR_ASSERT(layout->buffer);

    if (!layout) return value;
    if (!layout->valid || !layout->style || !layout->buffer) return value;
    zr_panel_alloc_space(&bounds, layout);
    config = layout->style;
    item_padding = zr_style_property(config, ZR_PROPERTY_ITEM_PADDING);

    background = (!value) ? config->colors[ZR_COLOR_WINDOW]:
        config->colors[ZR_COLOR_SELECTABLE];
    if (zr_input_is_mouse_click_in_rect(layout->input, ZR_BUTTON_LEFT, bounds)) {
        background = config->colors[ZR_COLOR_SELECTABLE_HOVER];
        if (zr_input_has_mouse_click_in_rect(layout->input, ZR_BUTTON_LEFT, bounds)) {
            if (zr_input_is_mouse_down(layout->input, ZR_BUTTON_LEFT))
                value = !value;
        }
    }

    text.padding.x = item_padding.x;
    text.padding.y = item_padding.y;
    text.background = background;
    text.text = (!value) ? config->colors[ZR_COLOR_TEXT] :
        config->colors[ZR_COLOR_SELECTABLE_TEXT];

    zr_command_buffer_push_rect(layout->buffer, bounds, 0, background);
    zr_widget_text(layout->buffer, bounds, str, zr_strsiz(str),
        &text, align, &config->font);
    return value;
}

int
zr_selectable(struct zr_context *layout, const char *str,
    enum zr_text_align align, int *value)
{
    int old = *value;
    int ret = zr_select(layout, str, align, old);
    *value = ret;
    return ret != old;
}

static enum zr_widget_state
zr_toggle_base(struct zr_toggle *toggle, struct zr_rect *bounds,
    struct zr_context *layout)
{
    const struct zr_style *config;
    struct zr_vec2 item_padding;
    enum zr_widget_state state;
    state = zr_widget(bounds, layout);
    if (!state) return state;

    config = layout->style;
    item_padding = zr_style_property(config, ZR_PROPERTY_ITEM_PADDING);
    toggle->rounding = 0;
    toggle->padding.x = item_padding.x;
    toggle->padding.y = item_padding.y;
    toggle->font = config->colors[ZR_COLOR_TEXT];
    return state;
}

int
zr_check(struct zr_context *layout, const char *text, int active)
{
    zr_checkbox(layout, text, &active);
    return active;
}

void
zr_checkbox(struct zr_context *layout, const char *text, int *is_active)
{
    struct zr_rect bounds;
    struct zr_toggle toggle;
    const struct zr_style *config;

    const struct zr_input *i;
    enum zr_widget_state state;
    state = zr_toggle_base(&toggle, &bounds, layout);
    if (!state) return;
    i = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : layout->input;

    config = layout->style;
    toggle.rounding = config->rounding[ZR_ROUNDING_CHECK];
    toggle.cursor = config->colors[ZR_COLOR_TOGGLE_CURSOR];
    toggle.normal = config->colors[ZR_COLOR_TOGGLE];
    toggle.hover = config->colors[ZR_COLOR_TOGGLE_HOVER];
    zr_widget_toggle(layout->buffer, bounds, is_active, text, ZR_TOGGLE_CHECK,
                        &toggle, i, &config->font);
}

void
zr_radio(struct zr_context *layout, const char *text, int *active)
{
    *active = zr_option(layout, text, *active);
}

int
zr_option(struct zr_context *layout, const char *text, int is_active)
{
    struct zr_rect bounds;
    struct zr_toggle toggle;
    const struct zr_style *config;

    const struct zr_input *i;
    enum zr_widget_state state;
    state = zr_toggle_base(&toggle, &bounds, layout);
    if (!state) return is_active;
    i = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : layout->input;

    config = layout->style;
    toggle.cursor = config->colors[ZR_COLOR_TOGGLE_CURSOR];
    toggle.normal = config->colors[ZR_COLOR_TOGGLE];
    toggle.hover = config->colors[ZR_COLOR_TOGGLE_HOVER];
    zr_widget_toggle(layout->buffer, bounds, &is_active, text, ZR_TOGGLE_OPTION,
                        &toggle, i, &config->font);
    return is_active;
}

void
zr_slider_float(struct zr_context *layout, float min_value, float *value,
    float max_value, float value_step)
{
    struct zr_rect bounds;
    struct zr_slider slider;
    const struct zr_style *config;
    struct zr_vec2 item_padding;

    const struct zr_input *i;
    enum zr_widget_state state;
    state = zr_widget(&bounds, layout);
    if (!state) return;
    i = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : layout->input;

    config = layout->style;
    item_padding = zr_style_property(config, ZR_PROPERTY_ITEM_PADDING);
    slider.padding.x = item_padding.x;
    slider.padding.y = item_padding.y;
    slider.bg = config->colors[ZR_COLOR_SLIDER];
    slider.normal = config->colors[ZR_COLOR_SLIDER_CURSOR];
    slider.hover = config->colors[ZR_COLOR_SLIDER_CURSOR_HOVER];
    slider.active = config->colors[ZR_COLOR_SLIDER_CURSOR_ACTIVE];
    slider.border = config->colors[ZR_COLOR_BORDER];
    slider.rounding = config->rounding[ZR_ROUNDING_SLIDER];
    *value = zr_widget_slider(layout->buffer, bounds, min_value, *value, max_value,
                        value_step, &slider, i);
}

void
zr_slider_int(struct zr_context *layout, int min_value, int *value,
    int max_value, int value_step)
{
    float val = (float)*value;
    zr_slider_float(layout, (float)min_value, &val,
        (float)max_value, (float)value_step);
    *value = (int)val;
}

void
zr_drag_float(struct zr_context *layout, float min, float *val,
    float max, float inc_per_pixel)
{
    struct zr_rect bounds;
    struct zr_drag drag;
    const struct zr_style *config;
    struct zr_vec2 item_padding;

    const struct zr_input *i;
    enum zr_widget_state state;
    zr_zero(&drag, sizeof(drag));
    state = zr_widget(&bounds, layout);
    if (!state) return;
    i = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : layout->input;

    config = layout->style;
    item_padding = zr_style_property(config, ZR_PROPERTY_ITEM_PADDING);

    drag.padding.x = item_padding.x;
    drag.padding.y = item_padding.y;
    drag.border = config->colors[ZR_COLOR_BORDER];
    drag.normal = config->colors[ZR_COLOR_DRAG];
    drag.hover = config->colors[ZR_COLOR_DRAG_HOVER];
    drag.active = config->colors[ZR_COLOR_DRAG_ACTIVE];
    drag.text = config->colors[ZR_COLOR_TEXT];
    drag.text_active = config->colors[ZR_COLOR_TEXT_ACTIVE];
    *val = zr_widget_drag(layout->buffer, bounds, min, *val, max,
                        inc_per_pixel, &drag, i, &config->font);
}

void zr_drag_int(struct zr_context *layout, int min, int *val,
    int max, int inc_per_pixel)
{
    float value = (float)*val;
    zr_drag_float(layout, (float)min, &value,
        (float)max, (float)inc_per_pixel);
    *val = (int)value;
}

void
zr_progress(struct zr_context *layout, zr_size *cur_value, zr_size max_value,
    int is_modifyable)
{
    struct zr_rect bounds;
    struct zr_progress prog;
    const struct zr_style *config;
    struct zr_vec2 item_padding;

    const struct zr_input *i;
    enum zr_widget_state state;
    state = zr_widget(&bounds, layout);
    if (!state) return;
    i = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : layout->input;

    config = layout->style;
    item_padding = zr_style_property(config, ZR_PROPERTY_ITEM_PADDING);
    prog.rounding = config->rounding[ZR_ROUNDING_PROGRESS];
    prog.padding.x = item_padding.x;
    prog.padding.y = item_padding.y;
    prog.border = config->colors[ZR_COLOR_BORDER];
    prog.background = config->colors[ZR_COLOR_PROGRESS];
    prog.normal = config->colors[ZR_COLOR_PROGRESS_CURSOR];
    prog.hover = config->colors[ZR_COLOR_PROGRESS_CURSOR_HOVER];
    prog.active = config->colors[ZR_COLOR_PROGRESS_CURSOR_ACTIVE];
    prog.rounding = config->rounding[ZR_ROUNDING_PROGRESS];
    *cur_value = zr_widget_progress(layout->buffer, bounds, *cur_value, max_value,
                        is_modifyable, &prog, i);
}

static enum zr_widget_state
zr_edit_base(struct zr_rect *bounds, struct zr_edit *field,
    struct zr_context *layout)
{
    const struct zr_style *config;
    struct zr_vec2 item_padding;
    enum zr_widget_state state = zr_widget(bounds, layout);
    if (!state) return state;

    config = layout->style;
    item_padding = zr_style_property(config, ZR_PROPERTY_ITEM_PADDING);
    field->border_size = 1;
    field->rounding = config->rounding[ZR_ROUNDING_INPUT];
    field->padding.x = item_padding.x;
    field->padding.y = item_padding.y;
    field->show_cursor = zr_true;
    field->background = config->colors[ZR_COLOR_INPUT];
    field->border = config->colors[ZR_COLOR_BORDER];
    field->cursor = config->colors[ZR_COLOR_INPUT_CURSOR];
    field->text = config->colors[ZR_COLOR_INPUT_TEXT];
    return state;
}

void
zr_editbox(struct zr_context *layout, struct zr_edit_box *box)
{
    struct zr_rect bounds;
    struct zr_edit field;
    const struct zr_style *config = layout->style;
    const struct zr_input *i;
    enum zr_widget_state state;

    state = zr_edit_base(&bounds, &field, layout);
    if (!state) return;
    i = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : layout->input;
    zr_widget_editbox(layout->buffer, bounds, box, &field, i, &config->font);
}

void
zr_edit(struct zr_context *layout, char *buffer, zr_size *len,
    zr_size max, int *active, zr_size *cursor, enum zr_input_filter filter)
{
    struct zr_rect bounds;
    struct zr_edit field;
    const struct zr_style *config = layout->style;
    const struct zr_input *i;
    enum zr_widget_state state;

    state = zr_edit_base(&bounds, &field, layout);
    if (!state) return;
    i = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : layout->input;
    *len = zr_widget_edit(layout->buffer, bounds,  buffer, *len, max, active, cursor,
                    &field, filter, i, &config->font);
}

void
zr_edit_filtered(struct zr_context *layout, char *buffer, zr_size *len,
    zr_size max, int *active, zr_size *cursor, zr_filter filter)
{
    struct zr_rect bounds;
    struct zr_edit field;
    const struct zr_style *config = layout->style;
    const struct zr_input *i;
    enum zr_widget_state state;

    state = zr_edit_base(&bounds, &field, layout);
    if (!state) return;
    i = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : layout->input;
    *len = zr_widget_edit_filtered(layout->buffer, bounds, buffer, *len, max, active,
                            cursor, &field, filter, i, &config->font);
}

static enum zr_widget_state
zr_spinner_base(struct zr_context *layout, struct zr_rect *bounds,
        struct zr_spinner *spinner)
{
    struct zr_vec2 item_padding;
    enum zr_widget_state state;
    const struct zr_style *config;

    ZR_ASSERT(layout);
    ZR_ASSERT(bounds);
    ZR_ASSERT(spinner);
    if (!layout || !bounds || !spinner)
        return ZR_WIDGET_INVALID;

    state = zr_widget(bounds, layout);
    if (!state) return state;
    config = layout->style;
    item_padding = zr_style_property(config, ZR_PROPERTY_ITEM_PADDING);

    zr_fill_button(config, &spinner->button.base);
    spinner->button.base.rounding = 0;
    spinner->button.normal = config->colors[ZR_COLOR_TEXT];
    spinner->button.hover = config->colors[ZR_COLOR_TEXT_HOVERING];
    spinner->button.active = config->colors[ZR_COLOR_TEXT_ACTIVE];
    spinner->button.base.padding = item_padding;
    spinner->padding.x = item_padding.x;
    spinner->padding.y = item_padding.y;
    spinner->color = config->colors[ZR_COLOR_SPINNER];
    spinner->border = config->colors[ZR_COLOR_BORDER];
    spinner->text = config->colors[ZR_COLOR_TEXT];
    spinner->show_cursor = zr_true;
    return state;
}

void
zr_spinner_int(struct zr_context *layout, int min, int *value,
    int max, int step, int *active)
{
    struct zr_rect bounds;
    struct zr_spinner spinner;
    struct zr_command_buffer *out;
    const struct zr_input *i;
    enum zr_widget_state state;

    state = zr_spinner_base(layout, &bounds, &spinner);
    if (!state) return;
    out = layout->buffer;
    i = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : layout->input;
    *value = zr_widget_spinner_int(out, bounds, &spinner, min, *value, max, step, active,
                        i, &layout->style->font);
}

void
zr_spinner_float(struct zr_context *layout, float min, float *value,
    float max, float step, int *active)
{
    struct zr_rect bounds;
    struct zr_spinner spinner;
    struct zr_command_buffer *out;
    const struct zr_input *i;
    enum zr_widget_state state;

    state = zr_spinner_base(layout, &bounds, &spinner);
    if (!state) return;
    out = layout->buffer;
    i = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : layout->input;
    *value = zr_widget_spinner_float(out, bounds, &spinner, min, *value, max, step,
                active, i, &layout->style->font);
}
/* -------------------------------------------------------------
 *
 *                          GRAPH
 *
 * --------------------------------------------------------------*/
void
zr_graph_begin(struct zr_context *layout, struct zr_graph *graph,
    enum zr_graph_type type, zr_size count, float min_value, float max_value)
{
    struct zr_rect bounds = {0, 0, 0, 0};
    const struct zr_style *config;
    struct zr_command_buffer *out;
    struct zr_color color;
    struct zr_vec2 item_padding;
    if (!zr_widget(&bounds, layout)) {
        zr_zero(graph, sizeof(*graph));
        return;
    }

    /* draw graph background */
    config = layout->style;
    out = layout->buffer;
    item_padding = zr_style_property(config, ZR_PROPERTY_ITEM_PADDING);
    color = (type == ZR_GRAPH_LINES) ?
        config->colors[ZR_COLOR_PLOT]: config->colors[ZR_COLOR_HISTO];
    zr_command_buffer_push_rect(out, bounds, config->rounding[ZR_ROUNDING_GRAPH], color);

    /* setup basic generic graph  */
    graph->valid = zr_true;
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

static zr_flags
zr_graph_push_line(struct zr_context *layout,
    struct zr_graph *g, float value)
{
    struct zr_command_buffer *out = layout->buffer;
    const struct zr_style *config = layout->style;
    const struct zr_input *i = layout->input;
    struct zr_color color;
    zr_flags ret = 0;
    float step, range, ratio;
    struct zr_vec2 cur;
    struct zr_rect bounds;

    ZR_ASSERT(g);
    ZR_ASSERT(layout);
    ZR_ASSERT(out);
    if (!g || !layout || !g->valid || g->index >= g->count)
        return zr_false;

    step = g->w / (float)g->count;
    range = g->max - g->min;
    ratio = (value - g->min) / range;

    if (g->index == 0) {
        /* special case for the first data point since it does not have a connection */
        g->last.x = g->x;
        g->last.y = (g->y + g->h) - ratio * (float)g->h;
        bounds.x = g->last.x - 3;
        bounds.y = g->last.y - 3;
        bounds.w = 6;
        bounds.h = 6;

        color = config->colors[ZR_COLOR_PLOT_LINES];
        if (!(layout->flags & ZR_WINDOW_ROM) &&
            ZR_INBOX(i->mouse.pos.x,i->mouse.pos.y, g->last.x-3, g->last.y-3, 6, 6)){
            ret = zr_input_is_mouse_hovering_rect(i, bounds) ? ZR_GRAPH_HOVERING : 0;
            ret |= (i->mouse.buttons[ZR_BUTTON_LEFT].down &&
                i->mouse.buttons[ZR_BUTTON_LEFT].clicked) ? ZR_GRAPH_CLICKED: 0;
            color = config->colors[ZR_COLOR_PLOT_HIGHLIGHT];
        }
        zr_command_buffer_push_rect(out, bounds, 0, color);
        g->index++;
        return ret;
    }

    /* draw a line between the last data point and the new one */
    cur.x = g->x + (float)(step * (float)g->index);
    cur.y = (g->y + g->h) - (ratio * (float)g->h);
    zr_command_buffer_push_line(out, g->last.x, g->last.y, cur.x, cur.y,
        config->colors[ZR_COLOR_PLOT_LINES]);

    bounds.x = cur.x - 3;
    bounds.y = cur.y - 3;
    bounds.w = 6;
    bounds.h = 6;

    /* user selection of the current data point */
    color = config->colors[ZR_COLOR_PLOT_LINES];
    if (!(layout->flags & ZR_WINDOW_ROM)) {
        if (zr_input_is_mouse_hovering_rect(i, bounds)) {
            ret = ZR_GRAPH_HOVERING;
            ret |= (i->mouse.buttons[ZR_BUTTON_LEFT].down &&
                i->mouse.buttons[ZR_BUTTON_LEFT].clicked) ? ZR_GRAPH_CLICKED: 0;
            color = config->colors[ZR_COLOR_PLOT_HIGHLIGHT];
        }
    }
    zr_command_buffer_push_rect(out, zr_rect(cur.x - 3, cur.y - 3, 6, 6), 0, color);

    /* save current data point position */
    g->last.x = cur.x;
    g->last.y = cur.y;
    g->index++;
    return ret;
}

static zr_flags
zr_graph_push_column(struct zr_context *layout,
    struct zr_graph *graph, float value)
{
    struct zr_command_buffer *out = layout->buffer;
    const struct zr_style *config = layout->style;
    const struct zr_input *in = layout->input;
    struct zr_vec2 item_padding;
    struct zr_color color;

    float ratio;
    zr_flags ret = 0;
    struct zr_rect item = {0,0,0,0};
    item_padding = zr_style_property(config, ZR_PROPERTY_ITEM_PADDING);

    if (!graph->valid || graph->index >= graph->count)
        return zr_false;
    if (graph->count) {
        float padding = (float)(graph->count-1) * item_padding.x;
        item.w = (graph->w - padding) / (float)(graph->count);
    }

    ratio = ZR_ABS(value) / graph->max;
    color = (value < 0) ? config->colors[ZR_COLOR_HISTO_NEGATIVE]:
        config->colors[ZR_COLOR_HISTO_BARS];

    /* calculate bounds of the current bar graph entry */
    item.h = graph->h * ratio;
    item.y = (graph->y + graph->h) - item.h;
    item.x = graph->x + ((float)graph->index * item.w);
    item.x = item.x + ((float)graph->index * item_padding.x);

    /* user graph bar selection */
    if (!(layout->flags & ZR_WINDOW_ROM) &&
        ZR_INBOX(in->mouse.pos.x,in->mouse.pos.y,item.x,item.y,item.w,item.h)) {
        ret = ZR_GRAPH_HOVERING;
        ret |= (in->mouse.buttons[ZR_BUTTON_LEFT].down &&
                in->mouse.buttons[ZR_BUTTON_LEFT].clicked) ? ZR_GRAPH_CLICKED: 0;
        color = config->colors[ZR_COLOR_HISTO_HIGHLIGHT];
    }
    zr_command_buffer_push_rect(out, item, 0, color);
    graph->index++;
    return ret;
}

zr_flags
zr_graph_push(struct zr_context *layout, struct zr_graph *graph,
    float value)
{
    ZR_ASSERT(layout);
    ZR_ASSERT(graph);
    if (!layout || !graph || !layout->valid) return zr_false;
    switch (graph->type) {
    case ZR_GRAPH_LINES:
        return zr_graph_push_line(layout, graph, value);
    case ZR_GRAPH_COLUMN:
        return zr_graph_push_column(layout, graph, value);
    case ZR_GRAPH_MAX:
    default: return zr_false;
    }
}

void
zr_graph_end(struct zr_context *layout, struct zr_graph *graph)
{
    ZR_ASSERT(layout);
    ZR_ASSERT(graph);
    if (!layout || !graph) return;
    graph->type = ZR_GRAPH_MAX;
    graph->index = 0;
    graph->count = 0;
    graph->min = 0;
    graph->max = 0;
    graph->x = 0;
    graph->y = 0;
    graph->w = 0;
    graph->h = 0;
}

/* -------------------------------------------------------------
 *
 *                          GROUPS
 *
 * --------------------------------------------------------------*/
void
zr_group_begin(struct zr_context *p, struct zr_context *g,
    const char *title, zr_flags flags, struct zr_vec2 offset)
{
    struct zr_rect bounds;
    struct zr_window panel;
    const struct zr_rect *c;
    struct zr_rect temp;

    ZR_ASSERT(p);
    ZR_ASSERT(g);
    if (!p || !g) return;
    if (!p->valid)
        goto failed;

    /* allocate space for the group panel inside the panel */
    c = &p->clip;
    zr_panel_alloc_space(&bounds, p);
    zr_zero(g, sizeof(*g));
    if (!ZR_INTERSECT(c->x, c->y, c->w, c->h, bounds.x, bounds.y, bounds.w, bounds.h) &&
        !(flags & ZR_WINDOW_MOVEABLE))
        goto failed;

    /* initialize a fake window to create the layout from */
    flags |= ZR_WINDOW_TAB;
    if (p->flags & ZR_WINDOW_ROM)
        flags |= ZR_WINDOW_ROM;
    temp = p->buffer->clip;
    zr_window_init(&panel, bounds, flags, 0, p->style, p->input);

    panel.buffer = *p->buffer;
    zr_begin(g, &panel, title);
    *p->buffer = panel.buffer;
    g->buffer = p->buffer;
    g->offset = offset;
    g->queue = p->queue;
    return;

failed:
    /* invalid panels still need correct data */
    g->valid = zr_false;
    g->style = p->style;
    g->buffer = p->buffer;
    g->input = p->input;
    g->queue = p->queue;
}

void
zr_group_end(struct zr_context *p, struct zr_context *g, struct zr_vec2 *scrollbar)
{
    struct zr_window pan;
    struct zr_command_buffer *out;
    struct zr_rect clip;

    ZR_ASSERT(p);
    ZR_ASSERT(g);
    if (!p || !g) return;
    if (!p->valid) return;
    zr_zero(&pan, sizeof(pan));

    out = p->buffer;
    pan.bounds.x = g->bounds.x;
    pan.bounds.y = g->bounds.y;
    pan.bounds.w = g->width;
    pan.bounds.h = g->height;
    pan.flags = g->flags|ZR_WINDOW_TAB;

    /* setup clipping rect to finalize group panel drawing back to parent */
    zr_unify(&clip, &p->clip, g->bounds.x, g->clip.y - g->header_h, g->bounds.x + g->bounds.w+1,
        g->bounds.y + g->bounds.h + 1);

    zr_command_buffer_push_scissor(out, clip);
    zr_end(g, &pan);
    zr_command_buffer_push_scissor(out, p->clip);
    if (scrollbar)
        *scrollbar = pan.offset;
}
/*
 * -------------------------------------------------------------
 *
 *                          POPUP
 *
 * --------------------------------------------------------------
 */
zr_flags
zr_popup_begin(struct zr_context *parent, struct zr_context *popup,
    enum zr_popup_type type, const char *title, zr_flags flags,
    struct zr_rect rect, struct zr_vec2 scrollbar)
{
    zr_flags ret;
    struct zr_window panel;
    ZR_ASSERT(parent);
    ZR_ASSERT(popup);
    if (!parent || !popup || !parent->valid) {
        popup->valid = zr_false;
        popup->style = parent->style;
        popup->buffer = parent->buffer;
        popup->input = parent->input;
        return 0;
    }

    /* calculate bounds of the panel */
    zr_zero(popup, sizeof(*popup));
    rect.x += parent->clip.x;
    rect.y += parent->clip.y;
    parent->flags |= ZR_WINDOW_ROM;

    /* initialize a fake window */
    flags |= ZR_WINDOW_BORDER|ZR_WINDOW_TAB;
    if (type == ZR_POPUP_DYNAMIC)
        flags |= ZR_WINDOW_DYNAMIC;
    zr_window_init(&panel, zr_rect(rect.x, rect.y, rect.w, rect.h),flags, 0,
        parent->style, parent->input);

    /* begin sub-buffer and create panel layout  */
    zr_command_buffer_push_scissor(parent->buffer, zr_null_rect);
    zr_command_queue_start_child(parent->queue, parent->buffer);
    panel.buffer = *parent->buffer;
    ret = zr_begin(popup, &panel, title);
    *parent->buffer = panel.buffer;
    parent->flags |= ZR_WINDOW_ROM;

    popup->buffer = parent->buffer;
    popup->offset = scrollbar;
    popup->queue = parent->queue;
    return ret;
}

void
zr_popup_close(struct zr_context *popup)
{
    ZR_ASSERT(popup);
    if (!popup || !popup->valid) return;
    popup->flags |= ZR_WINDOW_HIDDEN;
    popup->valid = zr_false;
}

void
zr_popup_end(struct zr_context *parent, struct zr_context *popup,
    struct zr_vec2 *scrollbar)
{
    struct zr_window pan;
    struct zr_command_buffer *out;

    ZR_ASSERT(parent);
    ZR_ASSERT(popup);
    if (!parent || !popup) return;
    if (!parent->valid) return;

    zr_zero(&pan, sizeof(pan));
    if (popup->flags & ZR_WINDOW_HIDDEN) {
        parent->flags |= ZR_WINDOW_REMOVE_ROM;
        popup->valid = zr_true;
    }

    out = parent->buffer;
    pan.bounds.x = popup->bounds.x;
    pan.bounds.y = popup->bounds.y;
    pan.bounds.w = popup->width;
    pan.bounds.h = popup->height;
    pan.flags = ZR_WINDOW_BORDER|ZR_WINDOW_TAB;

    /* end popup and reset clipping rect back to parent panel */
    zr_command_buffer_push_scissor(out, zr_null_rect);
    zr_end(popup, &pan);
    zr_command_queue_finish_child(parent->queue, parent->buffer);
    zr_command_buffer_push_scissor(out, parent->clip);
    if (scrollbar)
        *scrollbar = pan.offset;
}
/*
 * -------------------------------------------------------------
 *
 *                          CONTEXTUAL
 *
 * --------------------------------------------------------------
 */
static int
zr_popup_nonblocking_begin(struct zr_context *parent,
    struct zr_context *popup, zr_flags flags, int *active, int is_active,
    struct zr_rect body, struct zr_vec2 scrollbar)
{
    /* deactivate popup if user clicked outside the popup*/
    const struct zr_input *in = parent->input;
    if (in && *active) {
        int inbody = zr_input_is_mouse_click_in_rect(in, ZR_BUTTON_LEFT, body);
        int inpanel = zr_input_is_mouse_click_in_rect(in, ZR_BUTTON_LEFT, parent->bounds);
        if (!inbody && inpanel)
            is_active = zr_false;
    }

    /* recalculate body bounds into local panel position */
    body.x -= parent->clip.x;
    body.y -= parent->clip.y;

    /* if active create popup otherwise deactive the panel layout  */
    if (!is_active && *active) {
        zr_popup_begin(parent, popup, ZR_POPUP_DYNAMIC, 0, flags, body, scrollbar);
        zr_popup_close(popup);
        popup->flags &= ~(zr_flags)ZR_WINDOW_MINIMIZED;
        parent->flags &= ~(zr_flags)ZR_WINDOW_ROM;
    } else if (!is_active && !*active) {
        *active = is_active;
        popup->flags |= ZR_WINDOW_MINIMIZED;
        return zr_false;
    } else {
        zr_popup_begin(parent, popup, ZR_POPUP_DYNAMIC, 0, flags, body, zr_vec2(0,0));
        popup->flags &= ~(zr_flags)ZR_WINDOW_MINIMIZED;
    }
    *active = is_active;
    return zr_true;
}

static void
zr_popup_nonblocking_end(struct zr_context *parent,
    struct zr_context *popup, struct zr_vec2 *scrollbar)
{
    ZR_ASSERT(parent);
    ZR_ASSERT(popup);
    if (!parent || !popup) return;
    if (!parent->valid) return;
    if (!(popup->flags & ZR_WINDOW_MINIMIZED))
        zr_popup_end(parent, popup, scrollbar);
}

void
zr_contextual_begin(struct zr_context *parent, struct zr_context *popup,
    zr_flags flags, int *active, struct zr_rect body)
{
    ZR_ASSERT(parent);
    ZR_ASSERT(popup);
    ZR_ASSERT(active);
    if (!parent || !popup || !active) return;
    zr_popup_nonblocking_begin(parent, popup, flags, active, *active, body, zr_vec2(0,0));
}

static int
zr_contextual_button(struct zr_context *layout, const char *text,
    enum zr_text_align align, enum zr_button_behavior behavior)
{
    struct zr_rect bounds;
    struct zr_button_text button;
    const struct zr_style *config;

    const struct zr_input *i;
    enum zr_widget_state state;
    state = zr_button(&button.base, &bounds, layout, ZR_BUTTON_FITTING);
    if (!state) return zr_false;
    i = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : layout->input;

    config = layout->style;
    button.base.border_width = 0;
    button.base.normal = config->colors[ZR_COLOR_WINDOW];
    button.base.border = config->colors[ZR_COLOR_WINDOW];
    button.normal = config->colors[ZR_COLOR_TEXT];
    button.hover = config->colors[ZR_COLOR_TEXT_HOVERING];
    button.active = config->colors[ZR_COLOR_TEXT_ACTIVE];
    button.alignment = align;
    return zr_widget_button_text(layout->buffer, bounds, text,  behavior,
            &button, i, &config->font);
}

static int
zr_contextual_button_symbol(struct zr_context *layout, enum zr_symbol symbol,
    const char *text, enum zr_text_align align, enum zr_button_behavior behavior)
{
    struct zr_rect bounds;
    struct zr_button_text button;
    const struct zr_style *config;

    const struct zr_input *i;
    enum zr_widget_state state;
    state = zr_button(&button.base, &bounds, layout, ZR_BUTTON_FITTING);
    if (!state) return zr_false;
    i = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : layout->input;

    config = layout->style;
    button.base.border_width = 0;
    button.base.normal = config->colors[ZR_COLOR_WINDOW];
    button.base.border = config->colors[ZR_COLOR_WINDOW];
    button.normal = config->colors[ZR_COLOR_TEXT];
    button.hover = config->colors[ZR_COLOR_TEXT_HOVERING];
    button.active = config->colors[ZR_COLOR_TEXT_ACTIVE];
    return zr_widget_button_text_symbol(layout->buffer, bounds, symbol, text, align,
            behavior, &button, &config->font, i);
}

static int
zr_contextual_button_icon(struct zr_context *layout, struct zr_image img,
    const char *text, enum zr_text_align align, enum zr_button_behavior behavior)
{
    struct zr_rect bounds;
    struct zr_button_text button;
    const struct zr_style *config;

    const struct zr_input *i;
    enum zr_widget_state state;
    state = zr_button(&button.base, &bounds, layout, ZR_BUTTON_FITTING);
    if (!state) return zr_false;
    i = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : layout->input;

    config = layout->style;
    button.base.border_width = 0;
    button.base.normal = config->colors[ZR_COLOR_WINDOW];
    button.base.border = config->colors[ZR_COLOR_WINDOW];
    button.normal = config->colors[ZR_COLOR_TEXT];
    button.hover = config->colors[ZR_COLOR_TEXT_HOVERING];
    button.active = config->colors[ZR_COLOR_TEXT_ACTIVE];
    button.alignment = align;
    return zr_widget_button_text_image(layout->buffer, bounds, img, text,
                                behavior, &button, &config->font, i);
}

int
zr_contextual_item(struct zr_context *menu, const char *title, enum zr_text_align align)
{
    ZR_ASSERT(menu);
    if (zr_contextual_button(menu, title, align, ZR_BUTTON_DEFAULT)) {
        zr_contextual_close(menu);
        return zr_true;
    }
    return zr_false;
}

int
zr_contextual_item_icon(struct zr_context *menu, struct zr_image img,
    const char *title, enum zr_text_align align)
{
    ZR_ASSERT(menu);
    if (zr_contextual_button_icon(menu, img, title, align, ZR_BUTTON_DEFAULT)){
        zr_contextual_close(menu);
        return zr_true;
    }
    return zr_false;
}

int
zr_contextual_item_symbol(struct zr_context *menu, enum zr_symbol symbol,
    const char *title, enum zr_text_align align)
{
    ZR_ASSERT(menu);
    if (zr_contextual_button_symbol(menu, symbol, title, align, ZR_BUTTON_DEFAULT)){
        zr_contextual_close(menu);
        return zr_true;
    }
    return zr_false;
}

void
zr_contextual_close(struct zr_context *popup)
{
    ZR_ASSERT(popup);
    if (!popup) return;
    zr_popup_close(popup);
    popup->flags |= ZR_WINDOW_HIDDEN;
}

void
zr_contextual_end(struct zr_context *parent, struct zr_context *menu, int *state)
{
    ZR_ASSERT(parent);
    ZR_ASSERT(menu);
    if (!parent || !menu)
        goto failed;
    if (!parent->valid)
        goto failed;
    if ((!parent->valid) || (!menu->valid && !(menu->flags & ZR_WINDOW_HIDDEN)))
        goto failed;
    zr_popup_nonblocking_end(parent, menu, 0);
    if (menu->flags & ZR_WINDOW_HIDDEN)
        goto failed;
    if (state) *state = zr_true;
    return;
failed:
    if (state) *state = zr_false;
}
/* -------------------------------------------------------------
 *
 *                          COMBO
 *
 * --------------------------------------------------------------*/
void
zr_combo_begin(struct zr_context *parent, struct zr_context *combo,
    const char *selected, int *active)
{
    struct zr_rect bounds = {0,0,0,0};
    const struct zr_input *in;
    const struct zr_style *config;
    struct zr_command_buffer *out;
    struct zr_vec2 item_padding;
    struct zr_rect header;
    int is_active;

    ZR_ASSERT(parent);
    ZR_ASSERT(combo);
    ZR_ASSERT(selected);
    ZR_ASSERT(active);
    if (!parent || !combo || !selected || !active) return;
    if (!parent->valid || !zr_widget(&header, parent))
        goto failed;

    zr_zero(combo, sizeof(*combo));
    in = (parent->flags & ZR_WINDOW_ROM) ? 0 : parent->input;
    is_active = *active;
    out = parent->buffer;
    config = parent->style;
    item_padding = zr_style_property(config, ZR_PROPERTY_ITEM_PADDING);

    /* draw combo box header background and border */
    zr_command_buffer_push_rect(out, header, 0, config->colors[ZR_COLOR_BORDER]);
    zr_command_buffer_push_rect(out, zr_shrink_rect(header, 1), 0,
        config->colors[ZR_COLOR_SPINNER]);

    {
        /* print currently selected item */
        zr_size text_len;
        struct zr_rect label;
        label.x = header.x + item_padding.x;
        label.y = header.y + item_padding.y;
        label.w = header.w - (header.h + 2 * item_padding.x);
        label.h = header.h - 2 * item_padding.y;
        text_len = zr_strsiz(selected);
        zr_command_buffer_push_text(out, label, selected, text_len, &config->font,
                config->colors[ZR_COLOR_WINDOW], config->colors[ZR_COLOR_TEXT]);
    }
    {
        /* button setup and execution */
        struct zr_button_symbol button;
        bounds.y = header.y + 2;
        bounds.h = MAX(2, header.h);
        bounds.h = bounds.h - 4;
        bounds.w = bounds.h - 4;
        bounds.x = (header.x + header.w) - (bounds.w+4);

        button.base.rounding = 0;
        button.base.border_width = 0;
        button.base.padding.x = bounds.w/4.0f;
        button.base.padding.y = bounds.h/4.0f;
        button.base.border = config->colors[ZR_COLOR_SPINNER];
        button.base.normal = config->colors[ZR_COLOR_SPINNER];
        button.base.hover = config->colors[ZR_COLOR_SPINNER];
        button.base.active = config->colors[ZR_COLOR_SPINNER];
        button.normal = config->colors[ZR_COLOR_TEXT];
        button.hover = config->colors[ZR_COLOR_TEXT_HOVERING];
        button.active = config->colors[ZR_COLOR_TEXT_ACTIVE];
        if (zr_widget_button_symbol(out, bounds, ZR_SYMBOL_TRIANGLE_DOWN,
            ZR_BUTTON_DEFAULT, &button, in, &config->font))
            is_active = !is_active;
    }
    {
        /* calculate the maximum height of the combo box*/
        struct zr_rect body;
        body.x = header.x;
        body.w = header.w;
        body.y = header.y + header.h;
        body.h = zr_null_rect.h;
        if (!zr_popup_nonblocking_begin(parent, combo, ZR_WINDOW_COMBO_MENU|ZR_WINDOW_NO_SCROLLBAR,
                active, is_active, body, zr_vec2(0,0))) goto failed;
    }
    return;

failed:
    zr_zero(combo, sizeof(*combo));
    combo->valid = zr_false;
    combo->style = parent->style;
    combo->buffer = parent->buffer;
    combo->input = parent->input;
    combo->queue = parent->queue;
}

int zr_combo_item(struct zr_context *combo, const char *title, enum zr_text_align align)
{return zr_contextual_item(combo, title, align);}

int zr_combo_item_icon(struct zr_context *menu, struct zr_image img,
    const char *title, enum zr_text_align align)
{return zr_contextual_item_icon(menu, img, title, align);}

int zr_combo_item_symbol(struct zr_context *menu, enum zr_symbol symbol,
    const char *title, enum zr_text_align align)
{return zr_contextual_item_symbol(menu, symbol, title, align);}

void zr_combo_end(struct zr_context *parent, struct zr_context *menu, int *state)
{zr_contextual_end(parent, menu, state);}

void zr_combo_close(struct zr_context *combo)
{zr_contextual_close(combo);}
/*
 * -------------------------------------------------------------
 *
 *                          MENU
 *
 * --------------------------------------------------------------
 */
void
zr_menu_begin(struct zr_context *parent, struct zr_context *menu,
    const char *title, float width, int *active)
{
    const struct zr_input *in;
    const struct zr_style *config;
    struct zr_rect header;
    int is_active;

    ZR_ASSERT(parent);
    ZR_ASSERT(menu);
    ZR_ASSERT(title);
    ZR_ASSERT(active);
    if (!parent || !menu || !title || !active) return;
    if (!parent->valid) goto failed;

    is_active = *active;
    in = parent->input;
    config = parent->style;
    zr_zero(menu, sizeof(*menu));
    {
        /* exeucte menu button for open/closing the popup */
        struct zr_button_text button;
        zr_zero(&button, sizeof(header));
        zr_button(&button.base, &header, parent, ZR_BUTTON_NORMAL);
        button.alignment = ZR_TEXT_CENTERED;
        button.base.rounding = 0;
        button.base.border = (*active) ? config->colors[ZR_COLOR_BORDER]:
            config->colors[ZR_COLOR_WINDOW];
        button.base.normal = (is_active) ? config->colors[ZR_COLOR_BUTTON_HOVER]:
            config->colors[ZR_COLOR_WINDOW];
        button.base.active = config->colors[ZR_COLOR_WINDOW];
        button.normal = config->colors[ZR_COLOR_TEXT];
        button.active = config->colors[ZR_COLOR_TEXT];
        button.hover = config->colors[ZR_COLOR_TEXT];
        if (zr_widget_button_text(parent->buffer, header, title, ZR_BUTTON_DEFAULT,
                &button, in, &config->font))
            is_active = !is_active;
    }
    {
        /* calculate the maximum height of the menu */
        struct zr_rect body;
        body.x = header.x;
        body.w = width;
        body.y = header.y + header.h;
        body.h = (parent->bounds.y + parent->bounds.h) - body.y;
        if (!zr_popup_nonblocking_begin(parent, menu, ZR_WINDOW_COMBO_MENU|ZR_WINDOW_NO_SCROLLBAR, active,
            is_active, body, zr_vec2(0,0))) goto failed;
    }
    return;

failed:
    zr_zero(menu, sizeof(*menu));
    menu->valid = zr_false;
    menu->style = parent->style;
    menu->buffer = parent->buffer;
    menu->input = parent->input;
    menu->queue = parent->queue;
}

int zr_menu_item(struct zr_context *menu, enum zr_text_align align, const char *title)
{return zr_contextual_item(menu, title, align);}

int zr_menu_item_icon(struct zr_context *menu, struct zr_image img,
    const char *title, enum zr_text_align align)
{return zr_contextual_item_icon(menu, img, title, align);}

int zr_menu_item_symbol(struct zr_context *menu, enum zr_symbol symbol,
    const char *title, enum zr_text_align align)
{return zr_contextual_item_symbol(menu, symbol, title, align);}

void zr_menu_close(struct zr_context *menu, int *state)
{zr_popup_close(menu); *state = zr_false;}

void
zr_menu_end(struct zr_context *parent, struct zr_context *menu)
{
    ZR_ASSERT(parent);
    ZR_ASSERT(menu);
    if (!parent || !menu) return;
    if (!parent->valid) return;
    zr_popup_nonblocking_end(parent, menu, 0);
}
/*
 * -------------------------------------------------------------
 *
 *                          TOOLTIP
 *
 * --------------------------------------------------------------
 */
void
zr_tooltip_begin(struct zr_context *parent, struct zr_context *tip, float width)
{
    const struct zr_input *in;
    struct zr_rect bounds;
    ZR_ASSERT(tip);
    ZR_ASSERT(parent);
    if (!tip || !parent || !parent->valid || parent->flags & ZR_WINDOW_ROM) {
        zr_zero(tip, sizeof(*tip));
        tip->valid = zr_false;
        tip->style = parent->style;
        tip->buffer = parent->buffer;
        tip->input = parent->input;
        tip->queue = parent->queue;
        return;
    }

    in = parent->input;
    bounds.w = width;
    bounds.h = zr_null_rect.h;
    bounds.x = (in->mouse.pos.x + 1) - parent->clip.x;
    bounds.y = (in->mouse.pos.y + 1) - parent->clip.y;
    zr_popup_begin(parent, tip, ZR_POPUP_DYNAMIC, 0, ZR_WINDOW_NO_SCROLLBAR, bounds, zr_vec2(0,0));
}

void
zr_tooltip_end(struct zr_context *parent, struct zr_context *tip)
{
    if (!parent || !tip || !parent->valid) return;
    zr_popup_close(tip);
    zr_popup_end(parent, tip, 0);
}

void
zr_tooltip(struct zr_context *layout, const char *text)
{
    zr_size text_len;
    zr_size text_width;
    zr_size text_height;

    struct zr_context tip;
    const struct zr_style *config;
    struct zr_vec2 item_padding;
    struct zr_vec2 padding;

    ZR_ASSERT(layout);
    ZR_ASSERT(text);
    if (!layout || !text || !layout->valid || layout->flags & ZR_WINDOW_ROM)
        return;

    /* fetch configuration data */
    config = layout->style;
    padding = config->properties[ZR_PROPERTY_PADDING];
    item_padding = config->properties[ZR_PROPERTY_ITEM_PADDING];

    /* calculate size of the text and tooltip */
    text_len = zr_strsiz(text);
    text_width = config->font.width(config->font.userdata, config->font.height, text, text_len);
    text_width += (zr_size)(2 * padding.x + 2 * item_padding.x);
    text_height = (zr_size)(config->font.height + 2 * item_padding.y);

    /* execute tooltip and fill with text */
    zr_tooltip_begin(layout, &tip, (float)text_width);
    zr_layout_row_dynamic(&tip, (float)text_height, 1);
    zr_text(&tip, text, text_len, ZR_TEXT_LEFT);
    zr_tooltip_end(layout, &tip);
}

