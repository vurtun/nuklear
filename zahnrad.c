/*
    Copyright (c) 2016 Micha Mettke

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

/* ==============================================================
 *
 *                          INTERNAL
 *
 * =============================================================== */
#define ZR_POOL_DEFAULT_CAPACITY 16
#define ZR_VALUE_PAGE_CAPACITY 32
#define ZR_DEFAULT_COMMAND_BUFFER_SIZE (4*1024)

/* ==============================================================
 *                          MATH
 * =============================================================== */
#define ZR_MIN(a,b) ((a) < (b) ? (a) : (b))
#define ZR_MAX(a,b) ((a) < (b) ? (b) : (a))
#define ZR_CLAMP(i,v,x) (ZR_MAX(ZR_MIN(v,x), i))

#define ZR_PI 3.141592654f
#define ZR_UTF_INVALID 0xFFFD
#define ZR_MAX_FLOAT_PRECISION 2

#define ZR_UNUSED(x) ((void)(x))
#define ZR_SATURATE(x) (ZR_MAX(0, ZR_MIN(1.0f, x)))
#define ZR_LEN(a) (sizeof(a)/sizeof(a)[0])
#define ZR_ABS(a) (((a) < 0) ? -(a) : (a))
#define ZR_BETWEEN(x, a, b) ((a) <= (x) && (x) <= (b))
#define ZR_INBOX(px, py, x, y, w, h)\
    (ZR_BETWEEN(px,x,x+w) && ZR_BETWEEN(py,y,y+h))
#define ZR_INTERSECT(x0, y0, w0, h0, x1, y1, w1, h1) \
    (!(((x1 > (x0 + w0)) || ((x1 + w1) < x0) || (y1 > (y0 + h0)) || (y1 + h1) < y0)))
#define ZR_CONTAINS(x, y, w, h, bx, by, bw, bh)\
    (ZR_INBOX(x,y, bx, by, bw, bh) && ZR_INBOX(x+w,y+h, bx, by, bw, bh))

#define zr_vec2_mov(to,from) (to).x = (from).x, (to).y = (from).y
#define zr_vec2_sub(a, b) zr_vec2((a).x - (b).x, (a).y - (b).y)
#define zr_vec2_add(a, b) zr_vec2((a).x + (b).x, (a).y + (b).y)
#define zr_vec2_len_sqr(a) ((a).x*(a).x+(a).y*(a).y)
#define zr_vec2_muls(a, t) zr_vec2((a).x * (t), (a).y * (t))

#define zr_ptr_add(t, p, i) ((t*)((void*)((zr_byte*)(p) + (i))))
#define zr_ptr_add_const(t, p, i) ((const t*)((const void*)((const zr_byte*)(p) + (i))))
#define zr_zero_struct(s) zr_zero(&s, sizeof(s))

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

#define ZR_ALIGN_PTR(x, mask)\
    (ZR_UINT_TO_PTR((ZR_PTR_TO_UINT((zr_byte*)(x) + (mask-1)) & ~(mask-1))))
#define ZR_ALIGN_PTR_BACK(x, mask)\
    (ZR_UINT_TO_PTR((ZR_PTR_TO_UINT((zr_byte*)(x)) & ~(mask-1))))

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

/* make sure correct type size */
typedef int zr__check_size[(sizeof(zr_size) >= sizeof(void*)) ? 1 : -1];
typedef int zr__check_ptr[(sizeof(zr_ptr) == sizeof(void*)) ? 1 : -1];
typedef int zr__check_flags[(sizeof(zr_flags) >= 4) ? 1 : -1];
typedef int zr__check_rune[(sizeof(zr_rune) >= 4) ? 1 : -1];
typedef int zr__check_uint[(sizeof(zr_uint) == 4) ? 1 : -1];
typedef int zr__check_byte[(sizeof(zr_byte) == 1) ? 1 : -1];

static const struct zr_rect zr_null_rect = {-8192.0f, -8192.0f, 16384, 16384};
static const float FLOAT_PRECISION = 0.00000000000001f;
/*
 * ==============================================================
 *
 *                          MATH
 *
 * ===============================================================
 */
/*  Since zahnrad is supposed to work on all systems providing floating point
    math without any dependencies I also had to implement my own math functions
    for sqrt, sin and cos. Since the actual highly accurate implementations for
    the standard library functions are quite complex and I do not need high
    precision for my use cases I use approximations.

    Sqrt
    ----
    For square root zahnrad uses the famous fast inverse square root:
    https://en.wikipedia.org/wiki/Fast_inverse_square_root with
    slightly tweaked magic constant. While on todays hardware it is
    probably not faster it is still fast and accurate enough for
    zahnrads use cases.

    Sine/Cosine
    -----------
    All constants inside both function are generated Remez's minimax
    approximations for value range 0...2*PI. The reason why I decided to
    approximate exactly that range is that zahnrad only needs sine and
    cosine to generate circles which only requires that exact range.
    In addition I used Remez instead of Taylor for additional precision:
    www.lolengine.net/blog/2011/12/21/better-function-approximatations.

    The tool I used to generate constants for both sine and cosine
    (it can actually approximate a lot more functions) can be
    found here: www.lolengine.net/wiki/oss/lolremez
*/
static float
zr_inv_sqrt(float number)
{
    float x2;
    const float threehalfs = 1.5f;
    union {zr_uint i; float f;} conv = {0};
    conv.f = number;
    x2 = number * 0.5f;
    conv.i = 0x5f375A84 - (conv.i >> 1);
    conv.f = conv.f * (threehalfs - (x2 * conv.f * conv.f));
    return conv.f;
}

static float
zr_sin(float x)
{
    static const float a0 = +1.91059300966915117e-31f;
    static const float a1 = +1.00086760103908896f;
    static const float a2 = -1.21276126894734565e-2f;
    static const float a3 = -1.38078780785773762e-1f;
    static const float a4 = -2.67353392911981221e-2f;
    static const float a5 = +2.08026600266304389e-2f;
    static const float a6 = -3.03996055049204407e-3f;
    static const float a7 = +1.38235642404333740e-4f;
    return a0 + x*(a1 + x*(a2 + x*(a3 + x*(a4 + x*(a5 + x*(a6 + x*a7))))));
}

static float
zr_cos(float x)
{
    static const float a0 = +1.00238601909309722f;
    static const float a1 = -3.81919947353040024e-2f;
    static const float a2 = -3.94382342128062756e-1f;
    static const float a3 = -1.18134036025221444e-1f;
    static const float a4 = +1.07123798512170878e-1f;
    static const float a5 = -1.86637164165180873e-2f;
    static const float a6 = +9.90140908664079833e-4f;
    static const float a7 = -5.23022132118824778e-14f;
    return a0 + x*(a1 + x*(a2 + x*(a3 + x*(a4 + x*(a5 + x*(a6 + x*a7))))));
}

static zr_uint
zr_round_up_pow2(zr_uint v)
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


struct zr_rect
zr_recti(int x, int y, int w, int h)
{
    struct zr_rect r;
    r.x = (float)x;
    r.y = (float)y;
    r.w = (float)w;
    r.h = (float)h;
    return r;
}

struct zr_rect
zr_recta(struct zr_vec2 pos, struct zr_vec2 size)
{
    return zr_rect(pos.x, pos.y, size.x, size.y);
}

struct zr_rect
zr_rectv(const float *r)
{
    return zr_rect(r[0], r[1], r[2], r[3]);
}

struct zr_rect
zr_rectiv(const int *r)
{
    return zr_recti(r[0], r[1], r[2], r[3]);
}

static struct zr_rect
zr_shrink_rect(struct zr_rect r, float amount)
{
    struct zr_rect res;
    r.w = ZR_MAX(r.w, 2 * amount);
    r.h = ZR_MAX(r.h, 2 * amount);
    res.x = r.x + amount;
    res.y = r.y + amount;
    res.w = r.w - 2 * amount;
    res.h = r.h - 2 * amount;
    return res;
}

static struct zr_rect
zr_pad_rect(struct zr_rect r, struct zr_vec2 pad)
{
    r.w = ZR_MAX(r.w, 2 * pad.x);
    r.h = ZR_MAX(r.h, 2 * pad.y);
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

struct zr_vec2
zr_vec2i(int x, int y)
{
    struct zr_vec2 ret;
    ret.x = (float)x;
    ret.y = (float)y;
    return ret;
}

struct zr_vec2
zr_vec2v(const float *v)
{
    return zr_vec2(v[0], v[1]);
}

struct zr_vec2
zr_vec2iv(const int *v)
{
    return zr_vec2i(v[0], v[1]);
}
/*
 * ==============================================================
 *
 *                          STANDARD
 *
 * ===============================================================
 */
static int zr_str_match_here(const char *regexp, const char *text);
static int zr_str_match_star(int c, const char *regexp, const char *text);

static void*
zr_memcopy(void *dst0, const void *src0, zr_size length)
{
    zr_ptr t;
    typedef int word;
    char *dst = (char*)dst0;
    const char *src = (const char*)src0;
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
        TLOOP(*(word*)(void*)dst = *(const word*)(const void*)src;
            src += wsize; dst += wsize);
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
        TLOOP(src -= wsize; dst -= wsize;
            *(word*)(void*)dst = *(const word*)(const void*)src);
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
zr_memset(void *ptr, int c0, zr_size size)
{
    #define word unsigned
    #define wsize sizeof(word)
    #define wmask (wsize - 1)
    zr_byte *dst = (zr_byte*)ptr;
    unsigned c = 0;
    zr_size t = 0;

    if ((c = (zr_byte)c0) != 0) {
        c = (c << 8) | c; /* at least 16-bits  */
        if (sizeof(unsigned int) > 2)
            c = (c << 16) | c; /* at least 32-bits*/
    }

    /* to small of a word count */
    dst = (zr_byte*)ptr;
    if (size < 3 * wsize) {
        while (size--) *dst++ = (zr_byte)c0;
        return;
    }

    /* align destination */
    if ((t = ZR_PTR_TO_UINT(dst) & wmask) != 0) {
        t = wsize -t;
        size -= t;
        do {
            *dst++ = (zr_byte)c0;
        } while (--t != 0);
    }

    /* fill word */
    t = size / wsize;
    do {
        *(word*)((void*)dst) = c;
        dst += wsize;
    } while (--t != 0);

    /* fill trailing bytes */
    t = (size & wmask);
    if (t != 0) {
        do {
            *dst++ = (zr_byte)c0;
        } while (--t != 0);
    }

    #undef word
    #undef wsize
    #undef wmask
}

static void
zr_zero(void *ptr, zr_size size)
{
    ZR_ASSERT(ptr);
    zr_memset(ptr, 0, size);
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
zr_strtof(float *number, const char *buffer)
{
    float m;
    float neg = 1.0f;
    const char *p = buffer;
    float floatvalue = 0;

    ZR_ASSERT(number);
    ZR_ASSERT(buffer);
    if (!number || !buffer) return 0;
    *number = 0;

    /* skip whitespace */
    while (*p && *p == ' ') p++;
    if (*p == '-') {
        neg = -1.0f;
        p++;
    }

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
        int i, pow, div;
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
    *number = floatvalue * neg;
    return 1;
}

static int
zr_str_match_here(const char *regexp, const char *text)
{
    if (regexp[0] == '\0')
        return 1;
    if (regexp[1] == '*')
        return zr_str_match_star(regexp[0], regexp+2, text);
    if (regexp[0] == '$' && regexp[1] == '\0')
        return *text == '\0';
    if (*text!='\0' && (regexp[0]=='.' || regexp[0]==*text))
        return zr_str_match_here(regexp+1, text+1);
    return 0;
}

static int
zr_str_match_star(int c, const char *regexp, const char *text)
{
    do {/* a '* matches zero or more instances */
        if (zr_str_match_here(regexp, text))
            return 1;
    } while (*text != '\0' && (*text++ == c || c == '.'));
    return 0;
}

static int
zr_strfilter(const char *text, const char *regexp)
{
    /*
    c    matches any literal character c
    .    matches any single character
    ^    matches the beginning of the input string
    $    matches the end of the input string
    *    matches zero or more occurrences of the previous character*/
    if (regexp[0] == '^')
        return zr_str_match_here(regexp+1, text);
    do {    /* must look even if string is empty */
        if (zr_str_match_here(regexp, text))
            return 1;
    } while (*text++ != '\0');
    return 0;
}

static float
zr_pow(float x, int n)
{
    /*  check the sign of n */
    float r = 1;
    int plus = n >= 0;
    n = (plus) ? n : -n;
    while (n > 0) {
        if ((n & 1) == 1)
            r *= x;
        n /= 2;
        x *= x;
    }
    return plus ? r : 1.0f / r;
}

static float
zr_floor(float x)
{
    return (float)((int)x - ((x < 0.0) ? 1 : 0));
}

static int
zr_log10(float n)
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
zr_ftos(char *s, float n)
{
    int useExp = 0;
    int digit = 0, m = 0, m1 = 0;
    char *c = s;
    int neg = 0;

    if (n == 0.0) {
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
    while (n > FLOAT_PRECISION || m >= 0) {
        float weight = zr_pow(10.0, m);
        if (weight > 0) {
            float t = (float)n / weight;
            float tmp = zr_floor(t);
            digit = (int)tmp;
            n -= ((float)digit * weight);
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

zr_hash
zr_murmur_hash(const void * key, int len, zr_hash seed)
{
    /* 32-Bit MurmurHash3: https://code.google.com/p/smhasher/wiki/MurmurHash3*/
    #define ZR_ROTL(x,r) ((x) << (r) | ((x) >> (32 - r)))
    union {const zr_uint *i; const zr_byte *b;} conv = {0};
    const zr_byte *data = (const zr_byte*)key;
    const int nblocks = len/4;
    zr_uint h1 = seed;
    const zr_uint c1 = 0xcc9e2d51;
    const zr_uint c2 = 0x1b873593;
    const zr_byte *tail;
    const zr_uint *blocks;
    zr_uint k1;
    int i;

    /* body */
    if (!key) return 0;
    conv.b = (data + nblocks*4);
    blocks = (const zr_uint*)conv.i;
    for (i = -nblocks; i; ++i) {
        k1 = blocks[i];
        k1 *= c1;
        k1 = ZR_ROTL(k1,15);
        k1 *= c2;

        h1 ^= k1;
        h1 = ZR_ROTL(h1,13);
        h1 = h1*5+0xe6546b64;
    }

    /* tail */
    tail = (const zr_byte*)(data + nblocks*4);
    k1 = 0;
    switch (len & 3) {
    case 3: k1 ^= (zr_uint)(tail[2] << 16);
    case 2: k1 ^= (zr_uint)(tail[1] << 8u);
    case 1: k1 ^= tail[0];
            k1 *= c1;
            k1 = ZR_ROTL(k1,15);
            k1 *= c2;
            h1 ^= k1;
    default: break;
    }

    /* finalization */
    h1 ^= (zr_uint)len;
    /* fmix32 */
    h1 ^= h1 >> 16;
    h1 *= 0x85ebca6b;
    h1 ^= h1 >> 13;
    h1 *= 0xc2b2ae35;
    h1 ^= h1 >> 16;

    #undef ZR_ROTL
    return h1;
}

/*
 * ==============================================================
 *
 *                          COLOR
 *
 * ===============================================================
 */
static int
zr_parse_hex(const char *p, int length)
{
    int i = 0;
    int len = 0;
    while (len < length) {
        i <<= 4;
        if (p[len] >= 'a' && p[len] <= 'f')
            i += ((p[len] - 'a') + 10);
        else if (p[len] >= 'A' && p[len] <= 'F') {
            i += ((p[len] - 'A') + 10);
        } else i += (p[len] - '0');
        len++;
    }
    return i;
}

struct zr_color
zr_rgba(int r, int g, int b, int a)
{
    struct zr_color ret;
    ret.r = (zr_byte)ZR_CLAMP(0, r, 255);
    ret.g = (zr_byte)ZR_CLAMP(0, g, 255);
    ret.b = (zr_byte)ZR_CLAMP(0, b, 255);
    ret.a = (zr_byte)ZR_CLAMP(0, a, 255);
    return ret;
}

struct zr_color
zr_rgb_hex(const char *rgb)
{
    struct zr_color col;
    const char *c = rgb;
    if (*c == '#') c++;
    col.r = (zr_byte)zr_parse_hex(c, 2);
    col.g = (zr_byte)zr_parse_hex(c+2, 2);
    col.b = (zr_byte)zr_parse_hex(c+4, 2);
    col.a = 255;
    return col;
}

struct zr_color
zr_rgba_hex(const char *rgb)
{
    struct zr_color col;
    const char *c = rgb;
    if (*c == '#') c++;
    col.r = (zr_byte)zr_parse_hex(c, 2);
    col.g = (zr_byte)zr_parse_hex(c+2, 2);
    col.b = (zr_byte)zr_parse_hex(c+4, 2);
    col.a = (zr_byte)zr_parse_hex(c+6, 2);
    return col;
}

void
zr_color_hex_rgba(char *output, struct zr_color col)
{
    #define ZR_TO_HEX(i) ((i) <= 9 ? '0' + (i): 'A' - 10 + (i))
    output[0] = (char)ZR_TO_HEX((col.r & 0x0F));
    output[1] = (char)ZR_TO_HEX((col.r & 0xF0) >> 4);
    output[2] = (char)ZR_TO_HEX((col.g & 0x0F));
    output[3] = (char)ZR_TO_HEX((col.g & 0xF0) >> 4);
    output[4] = (char)ZR_TO_HEX((col.b & 0x0F));
    output[5] = (char)ZR_TO_HEX((col.b & 0xF0) >> 4);
    output[6] = (char)ZR_TO_HEX((col.a & 0x0F));
    output[7] = (char)ZR_TO_HEX((col.a & 0xF0) >> 4);
    output[8] = '\0';
    #undef ZR_TO_HEX
}

void
zr_color_hex_rgb(char *output, struct zr_color col)
{
    #define ZR_TO_HEX(i) ((i) <= 9 ? '0' + (i): 'A' - 10 + (i))
    output[0] = (char)ZR_TO_HEX((col.r & 0x0F));
    output[1] = (char)ZR_TO_HEX((col.r & 0xF0) >> 4);
    output[2] = (char)ZR_TO_HEX((col.g & 0x0F));
    output[3] = (char)ZR_TO_HEX((col.g & 0xF0) >> 4);
    output[4] = (char)ZR_TO_HEX((col.b & 0x0F));
    output[5] = (char)ZR_TO_HEX((col.b & 0xF0) >> 4);
    output[6] = '\0';
    #undef ZR_TO_HEX
}

struct zr_color
zr_rgba_iv(const int *c)
{
    return zr_rgba(c[0], c[1], c[2], c[3]);
}

struct zr_color
zr_rgba_bv(const zr_byte *c)
{
    return zr_rgba(c[0], c[1], c[2], c[3]);
}

struct zr_color
zr_rgb(int r, int g, int b)
{
    struct zr_color ret;
    ret.r =(zr_byte)ZR_CLAMP(0, r, 255);
    ret.g =(zr_byte)ZR_CLAMP(0, g, 255);
    ret.b =(zr_byte)ZR_CLAMP(0, b, 255);
    ret.a =(zr_byte)255;
    return ret;
}

struct zr_color
zr_rgb_iv(const int *c)
{
    return zr_rgb(c[0], c[1], c[2]);
}

struct zr_color
zr_rgb_bv(const zr_byte* c)
{
    return zr_rgb(c[0], c[1], c[2]);
}

struct zr_color
zr_rgba_u32(zr_uint in)
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
zr_rgba_fv(const float *c)
{
    return zr_rgba_f(c[0], c[1], c[2], c[3]);
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
zr_rgb_fv(const float *c)
{
    return zr_rgb_f(c[0], c[1], c[2]);
}

struct zr_color
zr_hsv(int h, int s, int v)
{
    return zr_hsva(h, s, v, 255);
}

struct zr_color
zr_hsv_iv(const int *c)
{
    return zr_hsv(c[0], c[1], c[2]);
}

struct zr_color
zr_hsv_bv(const zr_byte *c)
{
    return zr_hsv(c[0], c[1], c[2]);
}

struct zr_color
zr_hsv_f(float h, float s, float v)
{
    return zr_hsva_f(h, s, v, 1.0f);
}

struct zr_color
zr_hsv_fv(const float *c)
{
    return zr_hsv_f(c[0], c[1], c[2]);
}

struct zr_color
zr_hsva(int h, int s, int v, int a)
{
    float hf = ((float)ZR_CLAMP(0, h, 255)) / 255.0f;
    float sf = ((float)ZR_CLAMP(0, s, 255)) / 255.0f;
    float vf = ((float)ZR_CLAMP(0, v, 255)) / 255.0f;
    float af = ((float)ZR_CLAMP(0, a, 255)) / 255.0f;
    return zr_hsva_f(hf, sf, vf, af);
}

struct zr_color
zr_hsva_iv(const int *c)
{
    return zr_hsva(c[0], c[1], c[2], c[3]);
}

struct zr_color
zr_hsva_bv(const zr_byte *c)
{
    return zr_hsva(c[0], c[1], c[2], c[3]);
}

struct zr_color
zr_hsva_f(float h, float s, float v, float a)
{
    struct zr_colorf {float r,g,b;} out = {0,0,0};
    float p, q, t, f;
    int i;

    if (s <= 0.0f) {
        out.r = v; out.g = v; out.b = v;
        return zr_rgb_f(out.r, out.g, out.b);
    }

    h = h / (60.0f/360.0f);
    i = (int)h;
    f = h - (float)i;
    p = v * (1.0f - s);
    q = v * (1.0f - (s * f));
    t = v * (1.0f - s * (1.0f - f));

    switch (i) {
    case 0: out.r = v; out.g = t; out.b = p; break;
    case 1: out.r = q; out.g = v; out.b = p; break;
    case 2: out.r = p; out.g = v; out.b = t; break;
    case 3: out.r = p; out.g = q; out.b = v; break;
    case 4: out.r = t; out.g = p; out.b = v; break;
    case 5: default: out.r = v; out.g = p; out.b = q; break;
    }
    return zr_rgba_f(out.r, out.g, out.b, a);
}

struct zr_color
zr_hsva_fv(const float *c)
{
    return zr_hsva_f(c[0], c[1], c[2], c[3]);
}

zr_uint
zr_color_u32(struct zr_color in)
{
    zr_uint out = (zr_uint)in.r;
    out |= ((zr_uint)in.g << 8);
    out |= ((zr_uint)in.b << 16);
    out |= ((zr_uint)in.a << 24);
    return out;
}

void
zr_color_f(float *r, float *g, float *b, float *a, struct zr_color in)
{
    static const float s = 1.0f/255.0f;
    *r = (float)in.r * s;
    *g = (float)in.g * s;
    *b = (float)in.b * s;
    *a = (float)in.a * s;
}

void
zr_color_fv(float *c, struct zr_color in)
{
    zr_color_f(&c[0], &c[1], &c[2], &c[3], in);
}

void
zr_color_hsv_f(float *out_h, float *out_s, float *out_v, struct zr_color in)
{
    float a;
    zr_color_hsva_f(out_h, out_s, out_v, &a, in);
}

void
zr_color_hsv_fv(float *out, struct zr_color in)
{
    float a;
    zr_color_hsva_f(&out[0], &out[1], &out[2], &a, in);
}

void
zr_color_hsva_f(float *out_h, float *out_s,
    float *out_v, float *out_a, struct zr_color in)
{
    float chroma;
    float K = 0.0f;
    float r,g,b,a;

    zr_color_f(&r,&g,&b,&a, in);
    if (g < b) {
        const float t = g; g = b; b = t;
        K = -1.f;
    }
    if (r < g) {
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
zr_color_hsva_fv(float *out, struct zr_color in)
{
    zr_color_hsva_f(&out[0], &out[1], &out[2], &out[3], in);
}

void
zr_color_hsva_i(int *out_h, int *out_s, int *out_v,
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
zr_color_hsva_iv(int *out, struct zr_color in)
{
    zr_color_hsva_i(&out[0], &out[1], &out[2], &out[3], in);
}

void
zr_color_hsva_bv(zr_byte *out, struct zr_color in)
{
    int tmp[4];
    zr_color_hsva_i(&tmp[0], &tmp[1], &tmp[2], &tmp[3], in);
    out[0] = (zr_byte)tmp[0];
    out[1] = (zr_byte)tmp[1];
    out[2] = (zr_byte)tmp[2];
    out[3] = (zr_byte)tmp[3];
}

void
zr_color_hsv_i(int *out_h, int *out_s, int *out_v, struct zr_color in)
{
    int a;
    zr_color_hsva_i(out_h, out_s, out_v, &a, in);
}

void
zr_color_hsv_iv(int *out, struct zr_color in)
{
    zr_color_hsv_i(&out[0], &out[1], &out[2], in);
}

void
zr_color_hsv_bv(zr_byte *out, struct zr_color in)
{
    int tmp[4];
    zr_color_hsv_i(&tmp[0], &tmp[1], &tmp[2], in);
    out[0] = (zr_byte)tmp[0];
    out[1] = (zr_byte)tmp[1];
    out[2] = (zr_byte)tmp[2];
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
    zr_handle handle = {0};
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
    zr_zero(&s, sizeof(s));
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
    zr_zero(&s, sizeof(s));
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
    zr_zero(&s, sizeof(s));
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
    zr_zero(&s, sizeof(s));
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
    return !(img->w == 0 && img->h == 0);
}

static void
zr_unify(struct zr_rect *clip, const struct zr_rect *a, float x0, float y0,
    float x1, float y1)
{
    ZR_ASSERT(a);
    ZR_ASSERT(clip);
    clip->x = ZR_MAX(a->x, x0);
    clip->y = ZR_MAX(a->y, y0);
    clip->w = ZR_MIN(a->x + a->w, x1) - clip->x;
    clip->h = ZR_MIN(a->y + a->h, y1) - clip->y;
    clip->w = ZR_MAX(0, clip->w);
    clip->h = ZR_MAX(0, clip->h);
}

void
zr_triangle_from_direction(struct zr_vec2 *result, struct zr_rect r,
    float pad_x, float pad_y, enum zr_heading direction)
{
    float w_half, h_half;
    ZR_ASSERT(result);

    r.w = ZR_MAX(2 * pad_x, r.w);
    r.h = ZR_MAX(2 * pad_y, r.h);
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
static const zr_uint zr_utfmin[ZR_UTF_SIZE+1] = {0, 0, 0x80, 0x800, 0x10000};
static const zr_uint zr_utfmax[ZR_UTF_SIZE+1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};

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
    zr_size glyphs = 0;
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
        glyphs++;
        src_len = src_len + glyph_len;
        glyph_len = zr_utf_decode(text + src_len, &unicode, text_len - src_len);
    }
    return glyphs;
}

static const char*
zr_utf_at(const char *buffer, zr_size length, int index,
    zr_rune *unicode, zr_size *len)
{
    int i = 0;
    zr_size src_len = 0;
    zr_size glyph_len = 0;
    const char *text;
    zr_size text_len;

    ZR_ASSERT(buffer);
    ZR_ASSERT(unicode);
    ZR_ASSERT(len);

    if (!buffer || !unicode || !len) return 0;
    if (index < 0) {
        *unicode = ZR_UTF_INVALID;
        *len = 0;
        return 0;
    }

    text = buffer;
    text_len = length;
    glyph_len = zr_utf_decode(text, unicode, text_len);
    while (glyph_len) {
        if (i == index) {
            *len = glyph_len;
            break;
        }

        i++;
        src_len = src_len + glyph_len;
        glyph_len = zr_utf_decode(text + src_len, unicode, text_len - src_len);
    }
    if (i != index) return 0;
    return buffer + src_len;
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
    zr_size text_len, float space, zr_size *glyphs, float *text_width)
{
    zr_size glyph_len = 0;
    float last_width = 0;
    zr_rune unicode = 0;
    float width = 0;
    zr_size len = 0;
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

    *glyphs = g;
    *text_width = last_width;
    return len;
}

static zr_size
zr_user_font_glyphs_fitting_in_space(const struct zr_user_font *font,
    const char *text, zr_size len, float space, zr_size *row_len,
    zr_size *glyphs, float *text_width, int has_newline)
{
    zr_size glyph_len;
    zr_size width = 0;
    zr_rune unicode = 0;
    zr_size text_len = 0;
    zr_size row_advance = 0;

    ZR_ASSERT(glyphs);
    ZR_ASSERT(text_width);
    ZR_ASSERT(row_len);

    *glyphs = 0;
    *row_len = 0;
    *text_width = 0;

    glyph_len = text_len = zr_utf_decode(text, &unicode, len);
    if (!glyph_len) return 0;
    width = font->width(font->userdata, font->height, text, text_len);
    while ((width <= space) && (text_len <= len) && glyph_len) {
        *text_width = (float)width;
        *glyphs+=1;
        *row_len = text_len;
        row_advance += glyph_len;

        if (has_newline && (unicode == '\n' || unicode == '\r')) {
            zr_rune next = 0;
            zr_utf_decode(text+text_len, &next, len - text_len);
            if (unicode == '\r') {
                *row_len-=1;;
            } else if ((unicode == '\n') && (next == '\r')) {
                *row_len-= 2;
            } else {
                *row_len-=1;
            }
            width = font->width(font->userdata, font->height, text, *row_len);
            *text_width = (float)width;
            break;
        }
        glyph_len = zr_utf_decode(text + text_len, &unicode, len - text_len);
        text_len += glyph_len;
        width = font->width(font->userdata, font->height, text, text_len);
    }
    return row_advance;
}

/* ==============================================================
 *
 *                          BUFFER
 *
 * ===============================================================*/
void
zr_buffer_init(struct zr_buffer *b, const struct zr_allocator *a,
    zr_size initial_size)
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
    b->grow_factor = 2.0f;
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
    switch (type) {
    default:
    case ZR_BUFFER_MAX:
    case ZR_BUFFER_FRONT:
        if (align) {
            memory = ZR_ALIGN_PTR(unaligned, align);
            *alignment = (zr_size)((zr_byte*)memory - (zr_byte*)unaligned);
        } else {
            memory = unaligned;
            *alignment = 0;
        }
        break;
    case ZR_BUFFER_BACK:
        if (align) {
            memory = ZR_ALIGN_PTR_BACK(unaligned, align);
            *alignment = (zr_size)((zr_byte*)unaligned - (zr_byte*)memory);
        } else {
            memory = unaligned;
            *alignment = 0;
        }
        break;
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
        zr_size capacity;
        ZR_ASSERT(b->type == ZR_BUFFER_DYNAMIC);
        ZR_ASSERT(b->pool.alloc && b->pool.free);
        if (b->type != ZR_BUFFER_DYNAMIC || !b->pool.alloc || !b->pool.free)
            return 0;

        /* buffer is full so allocate bigger buffer if dynamic */
        capacity = (zr_size)((float)b->memory.size * b->grow_factor);
        capacity = ZR_MAX(capacity, zr_round_up_pow2((zr_uint)(b->allocated + size)));
        b->memory.ptr = zr_buffer_realloc(b, capacity, &b->memory.size);
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

static void
zr_buffer_push(struct zr_buffer *b, enum zr_buffer_allocation_type type,
    void *memory, zr_size size, zr_size align)
{
    void *mem = zr_buffer_alloc(b, type, size, align);
    if (!mem) return;
    zr_memcopy(mem, memory, size);
}

static void
zr_buffer_mark(struct zr_buffer *buffer, enum zr_buffer_allocation_type type)
{
    ZR_ASSERT(buffer);
    if (!buffer) return;
    buffer->marker[type].active = zr_true;
    if (type == ZR_BUFFER_BACK)
        buffer->marker[type].offset = buffer->size;
    else buffer->marker[type].offset = buffer->allocated;
}

static void
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

static void
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

const void*
zr_buffer_memory_const(const struct zr_buffer *buffer)
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

/*
 * ==============================================================
 *
 *                      Command buffer
 *
 * ===============================================================
*/
static void
zr_command_buffer_init(struct zr_command_buffer *cmdbuf,
    struct zr_buffer *buffer, enum zr_command_clipping clip)
{
    ZR_ASSERT(cmdbuf);
    ZR_ASSERT(buffer);
    if (!cmdbuf || !buffer) return;
    cmdbuf->base = buffer;
    cmdbuf->use_clipping = clip;
    cmdbuf->begin = buffer->allocated;
    cmdbuf->end = buffer->allocated;
    cmdbuf->last = buffer->allocated;
}

static void
zr_command_buffer_reset(struct zr_command_buffer *buffer)
{
    ZR_ASSERT(buffer);
    if (!buffer) return;
    buffer->begin = 0;
    buffer->end = 0;
    buffer->last = 0;
    buffer->clip = zr_null_rect;
#if ZR_COMPILE_WITH_COMMAND_USERDATA
    buffer->userdata.ptr = 0;
#endif
}

static void*
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

    cmd = (struct zr_command*)zr_buffer_alloc(b->base,ZR_BUFFER_FRONT,size,align);
    if (!cmd) return 0;

    /* make sure the offset to the next command is aligned */
    b->last = (zr_size)((zr_byte*)cmd - (zr_byte*)b->base->memory.ptr);
    unaligned = (zr_byte*)cmd + size;
    memory = ZR_ALIGN_PTR(unaligned, align);
    alignment = (zr_size)((zr_byte*)memory - (zr_byte*)unaligned);

    cmd->type = t;
    cmd->next = b->base->allocated + alignment;
#if ZR_COMPILE_WITH_COMMAND_USERDATA
    cmd->userdata = b->userdata;
#endif
    b->end = cmd->next;
    return cmd;
}

void
zr_push_scissor(struct zr_command_buffer *b, struct zr_rect r)
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
    cmd->w = (unsigned short)ZR_MAX(0, r.w);
    cmd->h = (unsigned short)ZR_MAX(0, r.h);
}

void
zr_stroke_line(struct zr_command_buffer *b, float x0, float y0,
    float x1, float y1, float line_thickness, struct zr_color c)
{
    struct zr_command_line *cmd;
    ZR_ASSERT(b);
    if (!b) return;
    cmd = (struct zr_command_line*)
        zr_command_buffer_push(b, ZR_COMMAND_LINE, sizeof(*cmd));
    if (!cmd) return;
    cmd->line_thickness = (unsigned short)line_thickness;
    cmd->begin.x = (short)x0;
    cmd->begin.y = (short)y0;
    cmd->end.x = (short)x1;
    cmd->end.y = (short)y1;
    cmd->color = c;
}

void
zr_stroke_curve(struct zr_command_buffer *b, float ax, float ay,
    float ctrl0x, float ctrl0y, float ctrl1x, float ctrl1y,
    float bx, float by, float line_thickness, struct zr_color col)
{
    struct zr_command_curve *cmd;
    ZR_ASSERT(b);
    if (!b || col.a == 0) return;

    cmd = (struct zr_command_curve*)
        zr_command_buffer_push(b, ZR_COMMAND_CURVE, sizeof(*cmd));
    if (!cmd) return;
    cmd->line_thickness = (unsigned short)line_thickness;
    cmd->begin.x = (short)ax;
    cmd->begin.y = (short)ay;
    cmd->ctrl[0].x = (short)ctrl0x;
    cmd->ctrl[0].y = (short)ctrl0y;
    cmd->ctrl[1].x = (short)ctrl1x;
    cmd->ctrl[1].y = (short)ctrl1y;
    cmd->end.x = (short)bx;
    cmd->end.y = (short)by;
    cmd->color = col;
}

void
zr_stroke_rect(struct zr_command_buffer *b, struct zr_rect rect,
    float rounding, float line_thickness, struct zr_color c)
{
    struct zr_command_rect *cmd;
    ZR_ASSERT(b);
    if (!b || c.a == 0) return;
    if (b->use_clipping) {
        const struct zr_rect *clip = &b->clip;
        if (!ZR_INTERSECT(rect.x, rect.y, rect.w, rect.h,
            clip->x, clip->y, clip->w, clip->h)) return;
    }

    cmd = (struct zr_command_rect*)
        zr_command_buffer_push(b, ZR_COMMAND_RECT, sizeof(*cmd));
    if (!cmd) return;
    cmd->rounding = (unsigned short)rounding;
    cmd->line_thickness = (unsigned short)line_thickness;
    cmd->x = (short)rect.x;
    cmd->y = (short)rect.y;
    cmd->w = (unsigned short)ZR_MAX(0, rect.w);
    cmd->h = (unsigned short)ZR_MAX(0, rect.h);
    cmd->color = c;
}

void
zr_fill_rect(struct zr_command_buffer *b, struct zr_rect rect,
    float rounding, struct zr_color c)
{
    struct zr_command_rect_filled *cmd;
    ZR_ASSERT(b);
    if (!b || c.a == 0) return;
    if (b->use_clipping) {
        const struct zr_rect *clip = &b->clip;
        if (!ZR_INTERSECT(rect.x, rect.y, rect.w, rect.h,
            clip->x, clip->y, clip->w, clip->h)) return;
    }

    cmd = (struct zr_command_rect_filled*)
        zr_command_buffer_push(b, ZR_COMMAND_RECT_FILLED, sizeof(*cmd));
    if (!cmd) return;
    cmd->rounding = (unsigned short)rounding;
    cmd->x = (short)rect.x;
    cmd->y = (short)rect.y;
    cmd->w = (unsigned short)ZR_MAX(0, rect.w);
    cmd->h = (unsigned short)ZR_MAX(0, rect.h);
    cmd->color = c;
}

void
zr_fill_rect_multi_color(struct zr_command_buffer *b, struct zr_rect rect,
    struct zr_color left, struct zr_color top, struct zr_color right,
    struct zr_color bottom)
{
    struct zr_command_rect_multi_color *cmd;
    ZR_ASSERT(b);
    if (!b) return;
    if (b->use_clipping) {
        const struct zr_rect *clip = &b->clip;
        if (!ZR_INTERSECT(rect.x, rect.y, rect.w, rect.h,
            clip->x, clip->y, clip->w, clip->h)) return;
    }

    cmd = (struct zr_command_rect_multi_color*)
        zr_command_buffer_push(b, ZR_COMMAND_RECT_MULTI_COLOR, sizeof(*cmd));
    cmd->x = (short)rect.x;
    cmd->y = (short)rect.y;
    cmd->w = (unsigned short)ZR_MAX(0, rect.w);
    cmd->h = (unsigned short)ZR_MAX(0, rect.h);
    cmd->left = left;
    cmd->top = top;
    cmd->right = right;
    cmd->bottom = bottom;
}

void
zr_stroke_circle(struct zr_command_buffer *b, struct zr_rect r,
    float line_thickness, struct zr_color c)
{
    struct zr_command_circle *cmd;
    if (!b || c.a == 0) return;
    if (b->use_clipping) {
        const struct zr_rect *clip = &b->clip;
        if (!ZR_INTERSECT(r.x, r.y, r.w, r.h, clip->x, clip->y, clip->w, clip->h))
            return;
    }

    cmd = (struct zr_command_circle*)
        zr_command_buffer_push(b, ZR_COMMAND_CIRCLE, sizeof(*cmd));
    if (!cmd) return;
    cmd->line_thickness = (unsigned short)line_thickness;
    cmd->x = (short)r.x;
    cmd->y = (short)r.y;
    cmd->w = (unsigned short)ZR_MAX(r.w, 0);
    cmd->h = (unsigned short)ZR_MAX(r.h, 0);
    cmd->color = c;
}

void
zr_fill_circle(struct zr_command_buffer *b, struct zr_rect r, struct zr_color c)
{
    struct zr_command_circle_filled *cmd;
    ZR_ASSERT(b);
    if (!b || c.a == 0) return;
    if (b->use_clipping) {
        const struct zr_rect *clip = &b->clip;
        if (!ZR_INTERSECT(r.x, r.y, r.w, r.h, clip->x, clip->y, clip->w, clip->h))
            return;
    }

    cmd = (struct zr_command_circle_filled*)
        zr_command_buffer_push(b, ZR_COMMAND_CIRCLE_FILLED, sizeof(*cmd));
    if (!cmd) return;
    cmd->x = (short)r.x;
    cmd->y = (short)r.y;
    cmd->w = (unsigned short)ZR_MAX(r.w, 0);
    cmd->h = (unsigned short)ZR_MAX(r.h, 0);
    cmd->color = c;
}

void
zr_stroke_arc(struct zr_command_buffer *b, float cx, float cy, float radius,
    float a_min, float a_max, float line_thickness, struct zr_color c)
{
    struct zr_command_arc *cmd;
    if (!b || c.a == 0) return;
    cmd = (struct zr_command_arc*)
        zr_command_buffer_push(b, ZR_COMMAND_ARC, sizeof(*cmd));
    if (!cmd) return;
    cmd->line_thickness = (unsigned short)line_thickness;
    cmd->cx = (short)cx;
    cmd->cy = (short)cy;
    cmd->r = (unsigned short)radius;
    cmd->a[0] = a_min;
    cmd->a[1] = a_max;
    cmd->color = c;
}

void
zr_fill_arc(struct zr_command_buffer *b, float cx, float cy, float radius,
    float a_min, float a_max, struct zr_color c)
{
    struct zr_command_arc_filled *cmd;
    ZR_ASSERT(b);
    if (!b || c.a == 0) return;
    cmd = (struct zr_command_arc_filled*)
        zr_command_buffer_push(b, ZR_COMMAND_ARC_FILLED, sizeof(*cmd));
    if (!cmd) return;
    cmd->cx = (short)cx;
    cmd->cy = (short)cy;
    cmd->r = (unsigned short)radius;
    cmd->a[0] = a_min;
    cmd->a[1] = a_max;
    cmd->color = c;
}

void
zr_stroke_triangle(struct zr_command_buffer *b, float x0, float y0, float x1,
    float y1, float x2, float y2, float line_thickness, struct zr_color c)
{
    struct zr_command_triangle *cmd;
    ZR_ASSERT(b);
    if (!b || c.a == 0) return;
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
    cmd->line_thickness = (unsigned short)line_thickness;
    cmd->a.x = (short)x0;
    cmd->a.y = (short)y0;
    cmd->b.x = (short)x1;
    cmd->b.y = (short)y1;
    cmd->c.x = (short)x2;
    cmd->c.y = (short)y2;
    cmd->color = c;
}

void
zr_fill_triangle(struct zr_command_buffer *b, float x0, float y0, float x1,
    float y1, float x2, float y2, struct zr_color c)
{
    struct zr_command_triangle_filled *cmd;
    ZR_ASSERT(b);
    if (!b || c.a == 0) return;
    if (!b) return;
    if (b->use_clipping) {
        const struct zr_rect *clip = &b->clip;
        if (!ZR_INBOX(x0, y0, clip->x, clip->y, clip->w, clip->h) ||
            !ZR_INBOX(x1, y1, clip->x, clip->y, clip->w, clip->h) ||
            !ZR_INBOX(x2, y2, clip->x, clip->y, clip->w, clip->h))
            return;
    }

    cmd = (struct zr_command_triangle_filled*)
        zr_command_buffer_push(b, ZR_COMMAND_TRIANGLE_FILLED, sizeof(*cmd));
    if (!cmd) return;
    cmd->a.x = (short)x0;
    cmd->a.y = (short)y0;
    cmd->b.x = (short)x1;
    cmd->b.y = (short)y1;
    cmd->c.x = (short)x2;
    cmd->c.y = (short)y2;
    cmd->color = c;
}

void
zr_stroke_polygon(struct zr_command_buffer *b,  float *points, int point_count,
    float line_thickness, struct zr_color col)
{
    int i;
    zr_size size = 0;
    struct zr_command_polygon *cmd;

    ZR_ASSERT(b);
    if (!b || col.a == 0) return;
    size = sizeof(*cmd) + sizeof(short) * 2 * (zr_size)point_count;
    cmd = (struct zr_command_polygon*) zr_command_buffer_push(b, ZR_COMMAND_POLYGON, size);
    if (!cmd) return;
    cmd->line_thickness = (unsigned short)line_thickness;
    cmd->color = col;
    cmd->point_count = (unsigned short)point_count;
    for (i = 0; i < point_count; ++i) {
        cmd->points[i].x = (short)points[i*2];
        cmd->points[i].y = (short)points[i*2+1];
    }
}

void
zr_fill_polygon(struct zr_command_buffer *b, float *points, int point_count,
    struct zr_color col)
{
    int i;
    zr_size size = 0;
    struct zr_command_polygon_filled *cmd;

    ZR_ASSERT(b);
    if (!b || col.a == 0) return;
    size = sizeof(*cmd) + sizeof(short) * 2 * (zr_size)point_count;
    cmd = (struct zr_command_polygon_filled*)
        zr_command_buffer_push(b, ZR_COMMAND_POLYGON_FILLED, size);
    if (!cmd) return;
    cmd->color = col;
    cmd->point_count = (unsigned short)point_count;
    for (i = 0; i < point_count; ++i) {
        cmd->points[i].x = (short)points[i*2];
        cmd->points[i].y = (short)points[i*2+1];
    }
}

void
zr_stroke_polyline(struct zr_command_buffer *b,
    float *points, int point_count, struct zr_color col)
{
    int i;
    zr_size size = 0;
    struct zr_command_polyline *cmd;

    ZR_ASSERT(b);
    if (!b || col.a == 0) return;
    size = sizeof(*cmd) + sizeof(short) * 2 * (zr_size)point_count;
    cmd = (struct zr_command_polyline*) zr_command_buffer_push(b, ZR_COMMAND_POLYLINE, size);
    if (!cmd) return;
    cmd->color = col;
    cmd->point_count = (unsigned short)point_count;
    for (i = 0; i < point_count; ++i) {
        cmd->points[i].x = (short)points[i*2];
        cmd->points[i].y = (short)points[i*2+1];
    }
}

void
zr_draw_image(struct zr_command_buffer *b, struct zr_rect r,
    const struct zr_image *img)
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
    cmd->w = (unsigned short)ZR_MAX(0, r.w);
    cmd->h = (unsigned short)ZR_MAX(0, r.h);
    cmd->img = *img;
}

void
zr_draw_text(struct zr_command_buffer *b, struct zr_rect r,
    const char *string, zr_size length, const struct zr_user_font *font,
    struct zr_color bg, struct zr_color fg)
{
    zr_size text_width = 0;
    struct zr_command_text *cmd;

    ZR_ASSERT(b);
    ZR_ASSERT(font);
    if (!b || !string || !length || (bg.a == 0 && fg.a == 0)) return;
    if (b->use_clipping) {
        const struct zr_rect *c = &b->clip;
        if (!ZR_INTERSECT(r.x, r.y, r.w, r.h, c->x, c->y, c->w, c->h))
            return;
    }

    /* make sure text fits inside bounds */
    text_width = font->width(font->userdata, font->height, string, length);
    if (text_width > r.w){
        float txt_width = (float)text_width;
        zr_size glyphs = 0;
        length = zr_use_font_glyph_clamp(font, string, length,
                    r.w, &glyphs, &txt_width);
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
}

/* ==============================================================
 *
 *                          CANVAS
 *
 * ===============================================================*/
#if ZR_COMPILE_WITH_VERTEX_BUFFER
enum zr_draw_list_stroke {
    ZR_STROKE_OPEN = zr_false,
    /* build up path has no connection back to the beginning */
    ZR_STROKE_CLOSED = zr_true
    /* build up path has a connection back to the beginning */
};

static void
zr_canvas_init(struct zr_canvas *list)
{
    zr_size i = 0;
    zr_zero(list, sizeof(*list));
    for (i = 0; i < ZR_LEN(list->circle_vtx); ++i) {
        const float a = ((float)i / (float)ZR_LEN(list->circle_vtx)) * 2 * ZR_PI;
        list->circle_vtx[i].x = (float)zr_cos(a);
        list->circle_vtx[i].y = (float)zr_sin(a);
    }
}

static void
zr_canvas_clear(struct zr_canvas *list)
{
    ZR_ASSERT(list);
    if (!list) return;
    if (list->buffer)
        zr_buffer_clear(list->buffer);
    if (list->elements)
        zr_buffer_clear(list->vertices);
    if (list->vertices)
        zr_buffer_clear(list->elements);

    list->element_count = 0;
    list->vertex_count = 0;
    list->cmd_offset = 0;
    list->cmd_count = 0;
    list->path_count = 0;
    list->vertices = 0;
    list->elements = 0;
    list->clip_rect = zr_null_rect;
}

static struct zr_vec2*
zr_canvas_alloc_path(struct zr_canvas *list, zr_size count)
{
    struct zr_vec2 *points;
    static const zr_size point_align = ZR_ALIGNOF(struct zr_vec2);
    static const zr_size point_size = sizeof(struct zr_vec2);
    points = (struct zr_vec2*)
        zr_buffer_alloc(list->buffer, ZR_BUFFER_FRONT,
                        point_size * count, point_align);

    if (!points) return 0;
    if (!list->path_offset) {
        void *memory = zr_buffer_memory(list->buffer);
        list->path_offset = (unsigned int)((zr_byte*)points - (zr_byte*)memory);
    }
    list->path_count += (unsigned int)count;
    return points;
}

static struct zr_vec2
zr_canvas_path_last(struct zr_canvas *list)
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
zr_canvas_push_command(struct zr_canvas *list, struct zr_rect clip,
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
zr_canvas_command_last(struct zr_canvas *list)
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

static void
zr_canvas_add_clip(struct zr_canvas *list, struct zr_rect rect)
{
    ZR_ASSERT(list);
    if (!list) return;
    if (!list->cmd_count) {
        zr_canvas_push_command(list, rect, list->null.texture);
    } else {
        struct zr_draw_command *prev = zr_canvas_command_last(list);
        zr_canvas_push_command(list, rect, prev->texture);
    }
}

static void
zr_canvas_push_image(struct zr_canvas *list, zr_handle texture)
{
    ZR_ASSERT(list);
    if (!list) return;
    if (!list->cmd_count) {
        zr_canvas_push_command(list, zr_null_rect, list->null.texture);
    } else {
        struct zr_draw_command *prev = zr_canvas_command_last(list);
        if (prev->texture.id != texture.id)
            zr_canvas_push_command(list, prev->clip_rect, texture);
    }
}

#if ZR_COMPILE_WITH_COMMAND_USERDATA
static void
zr_canvas_push_userdata(struct zr_canvas *list, zr_handle userdata)
{
    ZR_ASSERT(list);
    if (!list) return;
    if (!list->cmd_count) {
        struct zr_draw_command *prev;
        zr_canvas_push_command(list, zr_null_rect, list->null.texture);
        prev = zr_canvas_command_last(list);
        prev->userdata = userdata;
    } else {
        struct zr_draw_command *prev = zr_canvas_command_last(list);
        if (prev->userdata.ptr != userdata.ptr) {
            zr_canvas_push_command(list, prev->clip_rect, prev->texture);
            prev = zr_canvas_command_last(list);
            prev->userdata = userdata;
        }
    }
}
#endif

static struct zr_draw_vertex*
zr_canvas_alloc_vertices(struct zr_canvas *list, zr_size count)
{
    struct zr_draw_vertex *vtx;
    static const zr_size vtx_align = ZR_ALIGNOF(struct zr_draw_vertex);
    static const zr_size vtx_size = sizeof(struct zr_draw_vertex);
    ZR_ASSERT(list);
    if (!list) return 0;

    vtx = (struct zr_draw_vertex*)
        zr_buffer_alloc(list->vertices, ZR_BUFFER_FRONT, vtx_size*count, vtx_align);
    if (!vtx) return 0;
    list->vertex_count += (unsigned int)count;
    return vtx;
}

static zr_draw_index*
zr_canvas_alloc_elements(struct zr_canvas *list, zr_size count)
{
    zr_draw_index *ids;
    struct zr_draw_command *cmd;
    static const zr_size elem_align = ZR_ALIGNOF(zr_draw_index);
    static const zr_size elem_size = sizeof(zr_draw_index);
    ZR_ASSERT(list);
    if (!list) return 0;

    ids = (zr_draw_index*)
        zr_buffer_alloc(list->elements, ZR_BUFFER_FRONT, elem_size*count, elem_align);
    if (!ids) return 0;
    cmd = zr_canvas_command_last(list);
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
zr_canvas_add_poly_line(struct zr_canvas *list, struct zr_vec2 *points,
    const unsigned int points_count, struct zr_color color, int closed,
    float thickness, enum zr_anti_aliasing aliasing)
{
    zr_size count;
    int thick_line;
    zr_draw_vertex_color col;
    ZR_ASSERT(list);
    if (!list || points_count < 2) return;

    color.a = (zr_byte)((float)color.a * list->global_alpha);
    col = zr_color_u32(color);
    count = points_count;
    if (!closed) count = points_count-1;
    thick_line = thickness > 1.0f;

#if ZR_COMPILE_WITH_COMMAND_USERDATA
    zr_canvas_push_userdata(list, list->userdata);
#endif

    if (aliasing == ZR_ANTI_ALIASING_ON) {
        /* ANTI-ALIASED STROKE */
        const float AA_SIZE = 1.0f;
        static const zr_size pnt_align = ZR_ALIGNOF(struct zr_vec2);
        static const zr_size pnt_size = sizeof(struct zr_vec2);
        const zr_draw_vertex_color col_trans = col & 0x00ffffff;

        /* allocate vertices and elements  */
        zr_size i1 = 0;
        zr_size index = list->vertex_count;
        const zr_size idx_count = (thick_line) ?  (count * 18) : (count * 12);
        const zr_size vtx_count = (thick_line) ? (points_count * 4): (points_count *3);
        struct zr_draw_vertex *vtx = zr_canvas_alloc_vertices(list, vtx_count);
        zr_draw_index *ids = zr_canvas_alloc_elements(list, idx_count);

        zr_size size;
        struct zr_vec2 *normals, *temp;
        if (!vtx || !ids) return;

        /* temporary allocate normals + points */
        zr_buffer_mark(list->vertices, ZR_BUFFER_FRONT);
        size = pnt_size * ((thick_line) ? 5 : 3) * points_count;
        normals = (struct zr_vec2*)
            zr_buffer_alloc(list->vertices, ZR_BUFFER_FRONT, size, pnt_align);
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
                    scale = ZR_MIN(100.0f, scale);
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

            /* fill vertices */
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
                    scale = ZR_MIN(100.0f, scale);
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

            /* add vertices */
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
        zr_buffer_reset(list->vertices, ZR_BUFFER_FRONT);
    } else {
        /* NON ANTI-ALIASED STROKE */
        zr_size i1 = 0;
        zr_size idx = list->vertex_count;
        const zr_size idx_count = count * 6;
        const zr_size vtx_count = count * 4;
        struct zr_draw_vertex *vtx = zr_canvas_alloc_vertices(list, vtx_count);
        zr_draw_index *ids = zr_canvas_alloc_elements(list, idx_count);
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

            /* add vertices */
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
zr_canvas_add_poly_convex(struct zr_canvas *list, struct zr_vec2 *points,
    const unsigned int points_count, struct zr_color color,
    enum zr_anti_aliasing aliasing)
{
    static const zr_size pnt_align = ZR_ALIGNOF(struct zr_vec2);
    static const zr_size pnt_size = sizeof(struct zr_vec2);
    zr_draw_vertex_color col;
    ZR_ASSERT(list);
    if (!list || points_count < 3) return;

#if ZR_COMPILE_WITH_COMMAND_USERDATA
    zr_canvas_push_userdata(list, list->userdata);
#endif

    color.a = (zr_byte)((float)color.a * list->global_alpha);
    col = zr_color_u32(color);
    if (aliasing == ZR_ANTI_ALIASING_ON) {
        zr_size i = 0;
        zr_size i0 = 0;
        zr_size i1 = 0;

        const float AA_SIZE = 1.0f;
        const zr_draw_vertex_color col_trans = col & 0x00ffffff;
        zr_size index = list->vertex_count;
        const zr_size idx_count = (points_count-2)*3 + points_count*6;
        const zr_size vtx_count = (points_count*2);
        struct zr_draw_vertex *vtx = zr_canvas_alloc_vertices(list, vtx_count);
        zr_draw_index *ids = zr_canvas_alloc_elements(list, idx_count);

        unsigned int vtx_inner_idx = (unsigned int)(index + 0);
        unsigned int vtx_outer_idx = (unsigned int)(index + 1);
        struct zr_vec2 *normals = 0;
        zr_size size = 0;
        if (!vtx || !ids) return;

        /* temporary allocate normals */
        zr_buffer_mark(list->vertices, ZR_BUFFER_FRONT);
        size = pnt_size * points_count;
        normals = (struct zr_vec2*)
            zr_buffer_alloc(list->vertices, ZR_BUFFER_FRONT, size, pnt_align);
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

        /* add vertices + indexes */
        for (i0 = points_count-1, i1 = 0; i1 < points_count; i0 = i1++) {
            const struct zr_vec2 uv = list->null.uv;
            struct zr_vec2 n0 = normals[i0];
            struct zr_vec2 n1 = normals[i1];
            struct zr_vec2 dm = zr_vec2_muls(zr_vec2_add(n0, n1), 0.5f);

            float dmr2 = dm.x*dm.x + dm.y*dm.y;
            if (dmr2 > 0.000001f) {
                float scale = 1.0f / dmr2;
                scale = ZR_MIN(scale, 100.0f);
                dm = zr_vec2_muls(dm, scale);
            }
            dm = zr_vec2_muls(dm, AA_SIZE * 0.5f);

            /* add vertices */
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
        zr_buffer_reset(list->vertices, ZR_BUFFER_FRONT);
    } else {
        zr_size i = 0;
        zr_size index = list->vertex_count;
        const zr_size idx_count = (points_count-2)*3;
        const zr_size vtx_count = points_count;
        struct zr_draw_vertex *vtx = zr_canvas_alloc_vertices(list, vtx_count);
        zr_draw_index *ids = zr_canvas_alloc_elements(list, idx_count);
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

static void
zr_canvas_path_clear(struct zr_canvas *list)
{
    ZR_ASSERT(list);
    if (!list) return;
    zr_buffer_reset(list->buffer, ZR_BUFFER_FRONT);
    list->path_count = 0;
    list->path_offset = 0;
}

static void
zr_canvas_path_line_to(struct zr_canvas *list, struct zr_vec2 pos)
{
    struct zr_vec2 *points = 0;
    struct zr_draw_command *cmd = 0;
    ZR_ASSERT(list);
    if (!list) return;
    if (!list->cmd_count)
        zr_canvas_add_clip(list, zr_null_rect);

    cmd = zr_canvas_command_last(list);
    if (cmd && cmd->texture.ptr != list->null.texture.ptr)
        zr_canvas_push_image(list, list->null.texture);

    points = zr_canvas_alloc_path(list, 1);
    if (!points) return;
    points[0] = pos;
}

static void
zr_canvas_path_arc_to_fast(struct zr_canvas *list, struct zr_vec2 center,
    float radius, int a_min, int a_max)
{
    ZR_ASSERT(list);
    if (!list) return;
    if (a_min <= a_max) {
        int a = 0;
        for (a = a_min; a <= a_max; a++) {
            const struct zr_vec2 c = list->circle_vtx[(zr_size)a % ZR_LEN(list->circle_vtx)];
            const float x = center.x + c.x * radius;
            const float y = center.y + c.y * radius;
            zr_canvas_path_line_to(list, zr_vec2(x, y));
        }
    }
}

static void
zr_canvas_path_arc_to(struct zr_canvas *list, struct zr_vec2 center,
    float radius, float a_min, float a_max, unsigned int segments)
{
    unsigned int i = 0;
    ZR_ASSERT(list);
    if (!list) return;
    if (radius == 0.0f) return;
    for (i = 0; i <= segments; ++i) {
        const float a = a_min + ((float)i / ((float)segments) * (a_max - a_min));
        const float x = center.x + (float)zr_cos(a) * radius;
        const float y = center.y + (float)zr_sin(a) * radius;
        zr_canvas_path_line_to(list, zr_vec2(x, y));
    }
}

static void
zr_canvas_path_rect_to(struct zr_canvas *list, struct zr_vec2 a,
    struct zr_vec2 b, float rounding)
{
    float r;
    ZR_ASSERT(list);
    if (!list) return;
    r = rounding;
    r = ZR_MIN(r, ((b.x-a.x) < 0) ? -(b.x-a.x): (b.x-a.x));
    r = ZR_MIN(r, ((b.y-a.y) < 0) ? -(b.y-a.y): (b.y-a.y));

    if (r == 0.0f) {
        zr_canvas_path_line_to(list, a);
        zr_canvas_path_line_to(list, zr_vec2(b.x,a.y));
        zr_canvas_path_line_to(list, b);
        zr_canvas_path_line_to(list, zr_vec2(a.x,b.y));
    } else {
        zr_canvas_path_arc_to_fast(list, zr_vec2(a.x + r, a.y + r), r, 6, 9);
        zr_canvas_path_arc_to_fast(list, zr_vec2(b.x - r, a.y + r), r, 9, 12);
        zr_canvas_path_arc_to_fast(list, zr_vec2(b.x - r, b.y - r), r, 0, 3);
        zr_canvas_path_arc_to_fast(list, zr_vec2(a.x + r, b.y - r), r, 3, 6);
    }
}

static void
zr_canvas_path_curve_to(struct zr_canvas *list, struct zr_vec2 p2,
    struct zr_vec2 p3, struct zr_vec2 p4, unsigned int num_segments)
{
    unsigned int i_step;
    float t_step;
    struct zr_vec2 p1;

    ZR_ASSERT(list);
    ZR_ASSERT(list->path_count);
    if (!list || !list->path_count) return;
    num_segments = ZR_MAX(num_segments, 1);

    p1 = zr_canvas_path_last(list);
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
        zr_canvas_path_line_to(list, zr_vec2(x,y));
    }
}

static void
zr_canvas_path_fill(struct zr_canvas *list, struct zr_color color)
{
    struct zr_vec2 *points;
    ZR_ASSERT(list);
    if (!list) return;
    points = (struct zr_vec2*)zr_buffer_memory(list->buffer);
    zr_canvas_add_poly_convex(list, points, list->path_count, color, list->shape_AA);
    zr_canvas_path_clear(list);
}

static void
zr_canvas_path_stroke(struct zr_canvas *list, struct zr_color color,
    int closed, float thickness)
{
    struct zr_vec2 *points;
    ZR_ASSERT(list);
    if (!list) return;
    points = (struct zr_vec2*)zr_buffer_memory(list->buffer);
    zr_canvas_add_poly_line(list, points, list->path_count, color,
        closed, thickness, list->line_AA);
    zr_canvas_path_clear(list);
}

static void
zr_canvas_stroke_line(struct zr_canvas *list, struct zr_vec2 a,
    struct zr_vec2 b, struct zr_color col, float thickness)
{
    ZR_ASSERT(list);
    if (!list || !col.a) return;
    zr_canvas_path_line_to(list, zr_vec2_add(a, zr_vec2(0.5f, 0.5f)));
    zr_canvas_path_line_to(list, zr_vec2_add(b, zr_vec2(0.5f, 0.5f)));
    zr_canvas_path_stroke(list,  col, ZR_STROKE_OPEN, thickness);
}

static void
zr_canvas_fill_rect(struct zr_canvas *list, struct zr_rect rect,
    struct zr_color col, float rounding)
{
    ZR_ASSERT(list);
    if (!list || !col.a) return;
    zr_canvas_path_rect_to(list, zr_vec2(rect.x + 0.5f, rect.y + 0.5f),
        zr_vec2(rect.x + rect.w + 0.5f, rect.y + rect.h + 0.5f), rounding);
    zr_canvas_path_fill(list,  col);
}

static void
zr_canvas_stroke_rect(struct zr_canvas *list, struct zr_rect rect,
    struct zr_color col, float rounding, float thickness)
{
    ZR_ASSERT(list);
    if (!list || !col.a) return;
    zr_canvas_path_rect_to(list, zr_vec2(rect.x + 0.5f, rect.y + 0.5f),
        zr_vec2(rect.x + rect.w + 0.5f, rect.y + rect.h + 0.5f), rounding);
    zr_canvas_path_stroke(list,  col, ZR_STROKE_CLOSED, thickness);
}

static void
zr_canvas_add_rect_multi_color(struct zr_canvas *list, struct zr_rect rect,
    struct zr_color left, struct zr_color top, struct zr_color right,
    struct zr_color bottom)
{
    zr_draw_vertex_color col_left = zr_color_u32(left);
    zr_draw_vertex_color col_top = zr_color_u32(top);
    zr_draw_vertex_color col_right = zr_color_u32(right);
    zr_draw_vertex_color col_bottom = zr_color_u32(bottom);

    struct zr_draw_vertex *vtx;
    zr_draw_index *idx;
    zr_draw_index index;
    ZR_ASSERT(list);
    if (!list) return;

    zr_canvas_push_image(list, list->null.texture);
    index = (zr_draw_index)list->vertex_count;
    vtx = zr_canvas_alloc_vertices(list, 4);
    idx = zr_canvas_alloc_elements(list, 6);
    if (!vtx || !idx) return;

    idx[0] = (zr_draw_index)(index+0); idx[1] = (zr_draw_index)(index+1);
    idx[2] = (zr_draw_index)(index+2); idx[3] = (zr_draw_index)(index+0);
    idx[4] = (zr_draw_index)(index+2); idx[5] = (zr_draw_index)(index+3);

    vtx[0] = zr_draw_vertex(zr_vec2(rect.x, rect.y), list->null.uv, col_left);
    vtx[1] = zr_draw_vertex(zr_vec2(rect.x + rect.w, rect.y), list->null.uv, col_top);
    vtx[2] = zr_draw_vertex(zr_vec2(rect.x + rect.w, rect.y + rect.h), list->null.uv, col_right);
    vtx[3] = zr_draw_vertex(zr_vec2(rect.x, rect.y + rect.h), list->null.uv, col_bottom);
}

static void
zr_canvas_fill_triangle(struct zr_canvas *list, struct zr_vec2 a,
    struct zr_vec2 b, struct zr_vec2 c, struct zr_color col)
{
    ZR_ASSERT(list);
    if (!list || !col.a) return;
    zr_canvas_path_line_to(list, a);
    zr_canvas_path_line_to(list, b);
    zr_canvas_path_line_to(list, c);
    zr_canvas_path_fill(list, col);
}

static void
zr_canvas_stroke_triangle(struct zr_canvas *list, struct zr_vec2 a,
    struct zr_vec2 b, struct zr_vec2 c, struct zr_color col, float thickness)
{
    ZR_ASSERT(list);
    if (!list || !col.a) return;
    zr_canvas_path_line_to(list, a);
    zr_canvas_path_line_to(list, b);
    zr_canvas_path_line_to(list, c);
    zr_canvas_path_stroke(list, col, ZR_STROKE_CLOSED, thickness);
}

static void
zr_canvas_fill_circle(struct zr_canvas *list, struct zr_vec2 center,
    float radius, struct zr_color col, unsigned int segs)
{
    float a_max;
    ZR_ASSERT(list);
    if (!list || !col.a) return;
    a_max = ZR_PI * 2.0f * ((float)segs - 1.0f) / (float)segs;
    zr_canvas_path_arc_to(list, center, radius, 0.0f, a_max, segs);
    zr_canvas_path_fill(list, col);
}

static void
zr_canvas_stroke_circle(struct zr_canvas *list, struct zr_vec2 center,
    float radius, struct zr_color col, unsigned int segs, float thickness)
{
    float a_max;
    ZR_ASSERT(list);
    if (!list || !col.a) return;
    a_max = ZR_PI * 2.0f * ((float)segs - 1.0f) / (float)segs;
    zr_canvas_path_arc_to(list, center, radius, 0.0f, a_max, segs);
    zr_canvas_path_stroke(list, col, ZR_STROKE_CLOSED, thickness);
}

static void
zr_canvas_add_curve(struct zr_canvas *list, struct zr_vec2 p0,
    struct zr_vec2 cp0, struct zr_vec2 cp1, struct zr_vec2 p1,
    struct zr_color col, unsigned int segments, float thickness)
{
    ZR_ASSERT(list);
    if (!list || !col.a) return;
    zr_canvas_path_line_to(list, p0);
    zr_canvas_path_curve_to(list, cp0, cp1, p1, segments);
    zr_canvas_path_stroke(list, col, ZR_STROKE_OPEN, thickness);
}

static void
zr_canvas_push_rect_uv(struct zr_canvas *list, struct zr_vec2 a,
    struct zr_vec2 c, struct zr_vec2 uva, struct zr_vec2 uvc,
    struct zr_color color)
{
    zr_draw_vertex_color col = zr_color_u32(color);
    struct zr_draw_vertex *vtx;
    struct zr_vec2 uvb;
    struct zr_vec2 uvd;
    struct zr_vec2 b;
    struct zr_vec2 d;
    zr_draw_index *idx;
    zr_draw_index index;
    ZR_ASSERT(list);
    if (!list) return;

    uvb = zr_vec2(uvc.x, uva.y);
    uvd = zr_vec2(uva.x, uvc.y);
    b = zr_vec2(c.x, a.y);
    d = zr_vec2(a.x, c.y);

    index = (zr_draw_index)list->vertex_count;
    vtx = zr_canvas_alloc_vertices(list, 4);
    idx = zr_canvas_alloc_elements(list, 6);
    if (!vtx || !idx) return;

    idx[0] = (zr_draw_index)(index+0); idx[1] = (zr_draw_index)(index+1);
    idx[2] = (zr_draw_index)(index+2); idx[3] = (zr_draw_index)(index+0);
    idx[4] = (zr_draw_index)(index+2); idx[5] = (zr_draw_index)(index+3);

    vtx[0] = zr_draw_vertex(a, uva, col);
    vtx[1] = zr_draw_vertex(b, uvb, col);
    vtx[2] = zr_draw_vertex(c, uvc, col);
    vtx[3] = zr_draw_vertex(d, uvd, col);
}

static void
zr_canvas_add_image(struct zr_canvas *list, struct zr_image texture,
    struct zr_rect rect, struct zr_color color)
{
    ZR_ASSERT(list);
    if (!list) return;
    /* push new command with given texture */
    zr_canvas_push_image(list, texture.handle);
    if (zr_image_is_subimage(&texture)) {
        /* add region inside of the texture  */
        struct zr_vec2 uv[2];
        uv[0].x = (float)texture.region[0]/(float)texture.w;
        uv[0].y = (float)texture.region[1]/(float)texture.h;
        uv[1].x = (float)(texture.region[0] + texture.region[2])/(float)texture.w;
        uv[1].y = (float)(texture.region[1] + texture.region[3])/(float)texture.h;
        zr_canvas_push_rect_uv(list, zr_vec2(rect.x, rect.y),
            zr_vec2(rect.x + rect.w, rect.y + rect.h),  uv[0], uv[1], color);
    } else zr_canvas_push_rect_uv(list, zr_vec2(rect.x, rect.y),
            zr_vec2(rect.x + rect.w, rect.y + rect.h),
            zr_vec2(0.0f, 0.0f), zr_vec2(1.0f, 1.0f),color);
}

static void
zr_canvas_add_text(struct zr_canvas *list, const struct zr_user_font *font,
    struct zr_rect rect, const char *text, zr_size len, float font_height,
    struct zr_color fg)
{
    float x;
    zr_size text_len;
    zr_rune unicode;
    zr_rune next;
    zr_size glyph_len;
    zr_size next_glyph_len;
    struct zr_user_font_glyph g;

    ZR_ASSERT(list);
    if (!list || !len || !text) return;
    if (rect.x > (list->clip_rect.x + list->clip_rect.w) ||
        rect.y > (list->clip_rect.y + list->clip_rect.h) ||
        rect.x < list->clip_rect.x || rect.y < list->clip_rect.y)
        return;

    zr_canvas_push_image(list, font->texture);
    x = rect.x;
    glyph_len = text_len = zr_utf_decode(text, &unicode, len);
    if (!glyph_len) return;

    /* draw every glyph image */
    while (text_len <= len && glyph_len) {
        float gx, gy, gh, gw;
        float char_width = 0;
        if (unicode == ZR_UTF_INVALID) break;

        /* query currently drawn glyph information */
        next_glyph_len = zr_utf_decode(text + text_len, &next, len - text_len);
        font->query(font->userdata, font_height, &g, unicode,
                    (next == ZR_UTF_INVALID) ? '\0' : next);

        /* calculate and draw glyph drawing rectangle and image */
        gx = x + g.offset.x;
        /*gy = rect.y + (rect.h/2) - (font->height/2) + g.offset.y;*/
        gy = rect.y + g.offset.y;
        gw = g.width; gh = g.height;
        char_width = g.xadvance;
        fg.a = (zr_byte)((float)fg.a * list->global_alpha);
        zr_canvas_push_rect_uv(list, zr_vec2(gx,gy), zr_vec2(gx + gw, gy+ gh),
            g.uv[0], g.uv[1], fg);

        /* offset next glyph */
        text_len += glyph_len;
        x += char_width;
        glyph_len = next_glyph_len;
        unicode = next;
    }
}

void
zr_convert(struct zr_context *ctx, struct zr_buffer *cmds,
    struct zr_buffer *vertices, struct zr_buffer *elements,
    const struct zr_convert_config *config)
{
    const struct zr_command *cmd;
    ZR_ASSERT(ctx);
    ZR_ASSERT(cmds);
    ZR_ASSERT(vertices);
    ZR_ASSERT(elements);
    if (!ctx || !cmds || !vertices || !elements)
        return;

    ctx->canvas.null = config->null;
    ctx->canvas.clip_rect = zr_null_rect;
    ctx->canvas.vertices = vertices;
    ctx->canvas.elements = elements;
    ctx->canvas.buffer = cmds;
    ctx->canvas.line_AA = config->line_AA;
    ctx->canvas.shape_AA = config->shape_AA;
    ctx->canvas.global_alpha = config->global_alpha;

    zr_foreach(cmd, ctx)
    {
#if ZR_COMPILE_WITH_COMMAND_USERDATA
        list->userdata = cmd->userdata;
#endif
        switch (cmd->type) {
        case ZR_COMMAND_NOP: break;
        case ZR_COMMAND_SCISSOR: {
            const struct zr_command_scissor *s = zr_command(scissor, cmd);
            zr_canvas_add_clip(&ctx->canvas, zr_rect(s->x, s->y, s->w, s->h));
        } break;
        case ZR_COMMAND_LINE: {
            const struct zr_command_line *l = zr_command(line, cmd);
            zr_canvas_stroke_line(&ctx->canvas, zr_vec2(l->begin.x, l->begin.y),
                zr_vec2(l->end.x, l->end.y), l->color, l->line_thickness);
        } break;
        case ZR_COMMAND_CURVE: {
            const struct zr_command_curve *q = zr_command(curve, cmd);
            zr_canvas_add_curve(&ctx->canvas, zr_vec2(q->begin.x, q->begin.y),
                zr_vec2(q->ctrl[0].x, q->ctrl[0].y), zr_vec2(q->ctrl[1].x,
                q->ctrl[1].y), zr_vec2(q->end.x, q->end.y), q->color,
                config->curve_segment_count, q->line_thickness);
        } break;
        case ZR_COMMAND_RECT: {
            const struct zr_command_rect *r = zr_command(rect, cmd);
            zr_canvas_stroke_rect(&ctx->canvas, zr_rect(r->x, r->y, r->w, r->h),
                r->color, (float)r->rounding, r->line_thickness);
        } break;
        case ZR_COMMAND_RECT_FILLED: {
            const struct zr_command_rect_filled *r = zr_command(rect_filled, cmd);
            zr_canvas_fill_rect(&ctx->canvas, zr_rect(r->x, r->y, r->w, r->h),
                r->color, (float)r->rounding);
        } break;
        case ZR_COMMAND_RECT_MULTI_COLOR: {
            const struct zr_command_rect_multi_color *r = zr_command(rect_multi_color, cmd);
            zr_canvas_add_rect_multi_color(&ctx->canvas, zr_rect(r->x, r->y, r->w, r->h),
                r->left, r->top, r->right, r->bottom);
        } break;
        case ZR_COMMAND_CIRCLE: {
            const struct zr_command_circle *c = zr_command(circle, cmd);
            zr_canvas_stroke_circle(&ctx->canvas, zr_vec2((float)c->x + (float)c->w/2,
                (float)c->y + (float)c->h/2), (float)c->w/2, c->color,
                config->circle_segment_count, c->line_thickness);
        } break;
        case ZR_COMMAND_CIRCLE_FILLED: {
            const struct zr_command_circle_filled *c = zr_command(circle_filled, cmd);
            zr_canvas_fill_circle(&ctx->canvas, zr_vec2((float)c->x + (float)c->w/2,
                (float)c->y + (float)c->h/2), (float)c->w/2, c->color,
                config->circle_segment_count);
        } break;
        case ZR_COMMAND_ARC: {
            const struct zr_command_arc *c = zr_command(arc, cmd);
            zr_canvas_path_line_to(&ctx->canvas, zr_vec2(c->cx, c->cy));
            zr_canvas_path_arc_to(&ctx->canvas, zr_vec2(c->cx, c->cy), c->r,
                c->a[0], c->a[1], config->arc_segment_count);
            zr_canvas_path_stroke(&ctx->canvas, c->color, ZR_STROKE_CLOSED, c->line_thickness);
        } break;
        case ZR_COMMAND_ARC_FILLED: {
            const struct zr_command_arc_filled *c = zr_command(arc_filled, cmd);
            zr_canvas_path_line_to(&ctx->canvas, zr_vec2(c->cx, c->cy));
            zr_canvas_path_arc_to(&ctx->canvas, zr_vec2(c->cx, c->cy), c->r,
                c->a[0], c->a[1], config->arc_segment_count);
            zr_canvas_path_fill(&ctx->canvas, c->color);
        } break;
        case ZR_COMMAND_TRIANGLE: {
            const struct zr_command_triangle *t = zr_command(triangle, cmd);
            zr_canvas_stroke_triangle(&ctx->canvas, zr_vec2(t->a.x, t->a.y),
                zr_vec2(t->b.x, t->b.y), zr_vec2(t->c.x, t->c.y), t->color,
                t->line_thickness);
        } break;
        case ZR_COMMAND_TRIANGLE_FILLED: {
            const struct zr_command_triangle_filled *t = zr_command(triangle_filled, cmd);
            zr_canvas_fill_triangle(&ctx->canvas, zr_vec2(t->a.x, t->a.y),
                zr_vec2(t->b.x, t->b.y), zr_vec2(t->c.x, t->c.y), t->color);
        } break;
        case ZR_COMMAND_POLYGON: {
            int i;
            const struct zr_command_polygon*p = zr_command(polygon, cmd);
            for (i = 0; i < p->point_count; ++i) {
                struct zr_vec2 pnt = zr_vec2((float)p->points[i].x, (float)p->points[i].y);
                zr_canvas_path_line_to(&ctx->canvas, pnt);
            }
            zr_canvas_path_stroke(&ctx->canvas, p->color, ZR_STROKE_CLOSED, p->line_thickness);
        } break;
        case ZR_COMMAND_POLYGON_FILLED: {
            int i;
            const struct zr_command_polygon_filled *p = zr_command(polygon_filled, cmd);
            for (i = 0; i < p->point_count; ++i) {
                struct zr_vec2 pnt = zr_vec2((float)p->points[i].x, (float)p->points[i].y);
                zr_canvas_path_line_to(&ctx->canvas, pnt);
            }
            zr_canvas_path_fill(&ctx->canvas, p->color);
        } break;
        case ZR_COMMAND_POLYLINE: {
            int i;
            const struct zr_command_polyline *p = zr_command(polyline, cmd);
            for (i = 0; i < p->point_count; ++i) {
                struct zr_vec2 pnt = zr_vec2((float)p->points[i].x, (float)p->points[i].y);
                zr_canvas_path_line_to(&ctx->canvas, pnt);
            }
            zr_canvas_path_stroke(&ctx->canvas, p->color, ZR_STROKE_OPEN, p->line_thickness);
        } break;
        case ZR_COMMAND_TEXT: {
            const struct zr_command_text *t = zr_command(text, cmd);
            zr_canvas_add_text(&ctx->canvas, t->font, zr_rect(t->x, t->y, t->w, t->h),
                t->string, t->length, t->height, t->foreground);
        } break;
        case ZR_COMMAND_IMAGE: {
            const struct zr_command_image *i = zr_command(image, cmd);
            zr_canvas_add_image(&ctx->canvas, i->img, zr_rect(i->x, i->y, i->w, i->h),
                zr_rgb(255, 255, 255));
        } break;
        default: break;
        }
    }
}

const struct zr_draw_command*
zr__draw_begin(const struct zr_context *ctx,
    const struct zr_buffer *buffer)
{
    zr_byte *memory;
    zr_size offset;
    const struct zr_draw_command *cmd;

    ZR_ASSERT(buffer);
    if (!buffer || !buffer->size || !ctx->canvas.cmd_count)
        return 0;

    memory = (zr_byte*)buffer->memory.ptr;
    offset = buffer->memory.size - ctx->canvas.cmd_offset;
    cmd = zr_ptr_add(const struct zr_draw_command, memory, offset);
    return cmd;
}

const struct zr_draw_command*
zr__draw_next(const struct zr_draw_command *cmd,
    const struct zr_buffer *buffer, const struct zr_context *ctx)
{
    zr_byte *memory;
    zr_size size;
    zr_size offset;
    const struct zr_draw_command *end;

    ZR_ASSERT(buffer);
    ZR_ASSERT(ctx);
    if (!cmd || !buffer || !ctx)
        return 0;

    memory = (zr_byte*)buffer->memory.ptr;
    size = buffer->memory.size;
    offset = size - ctx->canvas.cmd_offset;
    end = zr_ptr_add(const struct zr_draw_command, memory, offset);
    end -= (ctx->canvas.cmd_count-1);

    if (cmd <= end) return 0;
    return (cmd-1);
}

#endif
/*
 * ==============================================================
 *
 *                          FONT
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

#if !ZR_DISABLE_STB_RECT_PACK_IMPLEMENTATION
#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION
#endif
#include "stb_rect_pack.h"

#if !ZR_DISABLE_STB_TRUETYPE_IMPLEMENTATION
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
static const zr_size zr_char_align = ZR_ALIGNOF(stbtt_packedchar);
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
    int total_glyphs = 0;
    for (i = 0; i < count; ++i) {
        int diff;
        zr_rune f = range[(i*2)+0];
        zr_rune t = range[(i*2)+1];
        ZR_ASSERT(t >= f);
        diff = (int)((t - f) + 1);
        total_glyphs += diff;
    }
    return total_glyphs;
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

void
zr_font_bake_memory(zr_size *temp, int *glyph_count,
    struct zr_font_config *config, int count)
{
    int i;
    int range_count = 0;
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
    *temp += zr_rect_align + zr_range_align + zr_char_align;
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
    baker->packed_chars = (stbtt_packedchar*)ZR_ALIGN_PTR((baker->build + count), zr_char_align);
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
        int range_n = 0;
        int rect_n = 0;
        int char_n = 0;

        /* pack custom user data first so it will be in the upper left corner*/
        if (custom) {
            stbrp_rect custom_space;
            zr_zero(&custom_space, sizeof(custom_space));
            custom_space.w = (stbrp_coord)((custom->w * 2) + 1);
            custom_space.h = (stbrp_coord)(custom->h + 1);

            stbtt_PackSetOversampling(&baker->spc, 1, 1);
            stbrp_pack_rects((stbrp_context*)baker->spc.pack_info, &custom_space, 1);
            *height = ZR_MAX(*height, (int)(custom_space.y + custom_space.h));

            custom->x = (short)custom_space.x;
            custom->y = (short)custom_space.y;
            custom->w = (short)custom_space.w;
            custom->h = (short)custom_space.h;
        }

        /* first font pass: pack all glyphs */
        for (input_i = 0; input_i < count; input_i++) {
            int n = 0;
            const zr_rune *in_range;
            const struct zr_font_config *cfg = &config[input_i];
            struct zr_font_bake_data *tmp = &baker->build[input_i];
            int glyph_count;
            int range_count;

            /* count glyphs + ranges in current font */
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
                    *height = ZR_MAX(*height, tmp->rects[i].y + tmp->rects[i].h);
            }
        }
        ZR_ASSERT(rect_n == total_glyph_count);
        ZR_ASSERT(char_n == total_glyph_count);
        ZR_ASSERT(range_n == total_range_count);
    }
    *height = (int)zr_round_up_pow2((zr_uint)*height);
    *image_memory = (zr_size)(*width) * (zr_size)(*height);
    return zr_true;
}

void
zr_font_bake(void *image_memory, int width, int height,
    void *temp, zr_size temp_size, struct zr_font_glyph *glyphs,
    int glyphs_count, const struct zr_font_config *config, int font_count)
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
    ZR_ASSERT(glyphs_count);
    if (!image_memory || !width || !height || !config || !temp ||
        !temp_size || !font_count || !glyphs || !glyphs_count)
        return;

    /* second font pass: render glyphs */
    baker = (struct zr_font_baker*)ZR_ALIGN_PTR(temp, zr_baker_align);
    zr_zero(image_memory, (zr_size)((zr_size)width * (zr_size)height));
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

    /* third pass: setup font and glyphs */
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
                glyph = &glyphs[dst_font->glyph_offset + (unsigned int)char_idx];
                glyph->codepoint = codepoint;
                glyph->x0 = q.x0; glyph->y0 = q.y0;
                glyph->x1 = q.x1; glyph->y1 = q.y1;
                glyph->y0 += (dst_font->ascent + 0.5f);
                glyph->y1 += (dst_font->ascent + 0.5f);
                glyph->w = glyph->x1 - glyph->x0 + 0.5f;
                glyph->h = glyph->y1 - glyph->y0;

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
    int y = 0;
    int x = 0;
    int n = 0;

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
    zr_rune *dst;

    ZR_ASSERT(out_memory);
    ZR_ASSERT(in_memory);
    ZR_ASSERT(img_width);
    ZR_ASSERT(img_height);
    if (!out_memory || !in_memory || !img_height || !img_width) return;

    dst = (zr_rune*)out_memory;
    src = (const zr_byte*)in_memory;
    for (n = (int)(img_width * img_height); n > 0; n--)
        *dst++ = ((zr_rune)(*src++) << 24) | 0x00FFFFFF;
}
/* -------------------------------------------------------------
 *
 *                          FONT
 *
 * --------------------------------------------------------------*/
void
zr_font_init(struct zr_font *font, float pixel_height,
    zr_rune fallback_codepoint, struct zr_font_glyph *glyphs,
    const struct zr_baked_font *baked_font, zr_handle atlas)
{
    ZR_ASSERT(font);
    ZR_ASSERT(glyphs);
    ZR_ASSERT(baked_font);
    if (!font || !glyphs || !baked_font)
        return;

    zr_zero(font, sizeof(*font));
    font->ascent = baked_font->ascent;
    font->descent = baked_font->descent;
    font->size = baked_font->height;
    font->scale = (float)pixel_height / (float)font->size;
    font->glyphs = &glyphs[baked_font->glyph_offset];
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
    int total_glyphs = 0;
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
            return &font->glyphs[((zr_rune)total_glyphs + (unicode - f))];
        total_glyphs += diff;
    }
    return glyph;
}

static zr_size
zr_font_text_width(zr_handle handle, float height, const char *text, zr_size len)
{
    zr_rune unicode;
    zr_size text_len  = 0;
    float text_width = 0;
    zr_size glyph_len = 0;
    float scale = 0;

    struct zr_font *font = (struct zr_font*)handle.ptr;
    ZR_ASSERT(font);
    if (!font || !text || !len)
        return 0;

    scale = height/font->size;
    glyph_len = text_len = zr_utf_decode(text, &unicode, len);
    if (!glyph_len) return 0;
    while (text_len <= len && glyph_len) {
        const struct zr_font_glyph *g;
        if (unicode == ZR_UTF_INVALID) break;

        /* query currently drawn glyph information */
        g = zr_font_find_glyph(font, unicode);
        text_width += g->xadvance * scale;

        /* offset next glyph */
        glyph_len = zr_utf_decode(text + text_len, &unicode, len - text_len);
        text_len += glyph_len;
    }
    return (zr_size)text_width;
}

#if ZR_COMPILE_WITH_VERTEX_BUFFER
static void
zr_font_query_font_glyph(zr_handle handle, float height,
    struct zr_user_font_glyph *glyph, zr_rune codepoint, zr_rune next_codepoint)
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
zr_edit_buffer_append(struct zr_buffer *buffer, const char *str, zr_size len)
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
zr_edit_buffer_insert(struct zr_buffer *buffer, zr_size pos,
    const char *str, zr_size len)
{
    zr_size i;
    void *mem;
    char *src;
    char *dst;

    zr_size copylen;
    ZR_ASSERT(buffer);
    if (!buffer || !str || !len || pos > buffer->allocated) return 0;
    if ((buffer->allocated + len >= buffer->memory.size) &&
        (buffer->type == ZR_BUFFER_FIXED)) return 0;

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
zr_edit_buffer_remove(struct zr_buffer *buffer, zr_size len)
{
    ZR_ASSERT(buffer);
    if (!buffer || len > buffer->allocated) return;
    ZR_ASSERT(((int)buffer->allocated - (int)len) >= 0);
    buffer->allocated -= len;
}

static void
zr_edit_buffer_del(struct zr_buffer *buffer, zr_size pos, zr_size len)
{
    ZR_ASSERT(buffer);
    if (!buffer || !len || pos > buffer->allocated ||
        pos + len > buffer->allocated) return;

    if (pos + len < buffer->allocated) {
        /* memmove */
        char *dst = zr_ptr_add(char, buffer->memory.ptr, pos);
        char *src = zr_ptr_add(char, buffer->memory.ptr, pos + len);
        zr_memcopy(dst, src, buffer->allocated - (pos + len));
        ZR_ASSERT(((int)buffer->allocated - (int)len) >= 0);
        buffer->allocated -= len;
    } else zr_edit_buffer_remove(buffer, len);
}

static char*
zr_edit_buffer_at_char(struct zr_buffer *buffer, zr_size pos)
{
    ZR_ASSERT(buffer);
    if (!buffer || pos > buffer->allocated) return 0;
    return zr_ptr_add(char, buffer->memory.ptr, pos);
}

static char*
zr_edit_buffer_at(struct zr_buffer *buffer, int pos, zr_rune *unicode,
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

static void
zr_edit_box_init_buffer(struct zr_edit_box *eb, struct zr_buffer *buffer,
    const struct zr_clipboard *clip, zr_filter f)
{
    ZR_ASSERT(eb);
    if (!eb) return;
    zr_zero(eb, sizeof(*eb));
    eb->buffer = *buffer;
    if (clip) eb->clip = *clip;
    if (f) eb->filter = f;
    else eb->filter = zr_filter_default;
    eb->cursor = 0;
    eb->glyphs = 0;
}

static void
zr_edit_box_init(struct zr_edit_box *eb, void *memory, zr_size size,
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
    eb->glyphs = 0;
}

void
zr_edit_box_clear(struct zr_edit_box *box)
{
    ZR_ASSERT(box);
    if (!box) return;
    zr_buffer_clear(&box->buffer);
    box->cursor = box->glyphs = 0;
}

void
zr_edit_box_add(struct zr_edit_box *eb, const char *str, zr_size len)
{
    int res = 0;
    ZR_ASSERT(eb);
    if (!eb || !str || !len) return;

    if (eb->cursor != eb->glyphs) {
        zr_size l = 0;
        zr_rune unicode;
        int cursor = (eb->cursor) ? (int)(eb->cursor) : 0;
        char *sym = zr_edit_buffer_at(&eb->buffer, cursor, &unicode, &l);
        zr_size offset = (zr_size)(sym - (char*)eb->buffer.memory.ptr);
        res = zr_edit_buffer_insert(&eb->buffer, offset, str, len);
    } else res = zr_edit_buffer_insert(&eb->buffer, eb->buffer.allocated, str, len);

    if (res) {
        zr_size l = zr_utf_len(str, len);
        eb->glyphs += l;
        eb->cursor += l;
        eb->text_inserted = 1;
    }
}

static zr_size
zr_edit_box_buffer_input(struct zr_edit_box *box, const struct zr_input *i)
{
    zr_rune unicode;
    zr_size src_len = 0;
    zr_size text_len = 0;
    zr_size glyph_len = 0;
    zr_size glyphs = 0;

    ZR_ASSERT(box);
    ZR_ASSERT(i);
    if (!box || !i) return 0;

    /* add user provided text to buffer until either no input or buffer space left*/
    glyph_len = zr_utf_decode(i->keyboard.text, &unicode, i->keyboard.text_len);
    while (glyph_len && ((text_len+glyph_len) <= i->keyboard.text_len)) {
        /* filter to make sure the value is correct */
        if (box->filter(box, unicode)) {
            zr_edit_box_add(box, &i->keyboard.text[text_len], glyph_len);
            text_len += glyph_len;
            glyphs++;
        }
        src_len = src_len + glyph_len;
        glyph_len = zr_utf_decode(i->keyboard.text + src_len, &unicode,
            i->keyboard.text_len - src_len);
    }
    return glyphs;
}

void
zr_edit_box_remove(struct zr_edit_box *box, enum zr_edit_remove_operation op)
{
    char *buf;
    zr_size len;
    zr_size min;
    zr_size maxi;
    zr_size diff;

    ZR_ASSERT(box);
    if (!box) return;
    if (!box->glyphs) return;

    buf = (char*)box->buffer.memory.ptr;
    min = ZR_MIN(box->sel.end, box->sel.begin);
    maxi = ZR_MAX(box->sel.end, box->sel.begin);
    diff = ZR_MAX(1, maxi - min);

    if (diff && box->cursor != box->glyphs) {
        zr_size off;
        zr_rune unicode;
        char *begin;
        char *end;

        /* calculate text selection byte position and size */
        begin = zr_edit_buffer_at(&box->buffer, (int)min, &unicode, &len);
        end = zr_edit_buffer_at(&box->buffer, (int)maxi, &unicode, &len);
        len = ZR_MAX((zr_size)(end - begin), 1);
        off = (zr_size)(begin - buf);

        /* delete text selection */
        if (len > 1) {
            zr_edit_buffer_del(&box->buffer, off, len);
        } else {
            if (op == ZR_DELETE) {
                zr_edit_buffer_del(&box->buffer, off, len);
            } else {
                zr_edit_buffer_del(&box->buffer, off-1, len);
                box->cursor = (box->cursor) ? box->cursor-1: 0;
                box->sel.begin = box->cursor;
                box->sel.end = box->cursor;
            }
        }
        box->glyphs = zr_utf_len(buf, box->buffer.allocated);
    } else if (op == ZR_REMOVE) {
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
        box->glyphs--;
    }
    if (op == ZR_DELETE) {
        if (box->cursor >= box->glyphs)
            box->cursor = box->glyphs;
        else if (box->cursor > 0)
            box->cursor--;
    }
}

int
zr_edit_box_has_selection(const struct zr_edit_box *eb)
{
    ZR_ASSERT(eb);
    if (!eb) return 0;
    return (int)(eb->sel.end - eb->sel.begin);
}

const char*
zr_edit_box_get_selection(zr_size *len, struct zr_edit_box *eb)
{
    zr_size l;
    const char *begin;
    zr_rune unicode;
    ZR_ASSERT(eb);
    ZR_ASSERT(len);
    if (!eb || !len || !(eb->sel.end - eb->sel.begin))
        return 0;

    begin =  zr_edit_buffer_at(&eb->buffer, (int)eb->sel.begin, &unicode, &l);
    *len = (eb->sel.end - eb->sel.begin) + 1;
    return begin;
}

char*
zr_edit_box_get(struct zr_edit_box *eb)
{
    ZR_ASSERT(eb);
    if (!eb) return 0;
    return (char*)eb->buffer.memory.ptr;
}

const char*
zr_edit_box_get_const(const struct zr_edit_box *eb)
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
    if (pos >= eb->glyphs) {
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
    ZR_ASSERT(eb->glyphs >= pos);
    if (!eb || pos > eb->glyphs) return;
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
    return eb->glyphs;
}

/* ==============================================================
 *
 *                          INPUT
 *
 * ===============================================================*/
void
zr_input_begin(struct zr_context *ctx)
{
    zr_size i;
    struct zr_input *in;
    ZR_ASSERT(ctx);
    if (!ctx) return;

    in = &ctx->input;
    for (i = 0; i < ZR_BUTTON_MAX; ++i)
        in->mouse.buttons[i].clicked = 0;
    in->keyboard.text_len = 0;
    in->mouse.scroll_delta = 0;
    zr_vec2_mov(in->mouse.prev, in->mouse.pos);
    for (i = 0; i < ZR_KEY_MAX; i++)
        in->keyboard.keys[i].clicked = 0;
}

void
zr_input_motion(struct zr_context *ctx, int x, int y)
{
    struct zr_input *in;
    ZR_ASSERT(ctx);
    if (!ctx) return;
    in = &ctx->input;
    in->mouse.pos.x = (float)x;
    in->mouse.pos.y = (float)y;
}

void
zr_input_key(struct zr_context *ctx, enum zr_keys key, int down)
{
    struct zr_input *in;
    ZR_ASSERT(ctx);
    if (!ctx) return;
    in = &ctx->input;
    if (in->keyboard.keys[key].down == down) return;
    in->keyboard.keys[key].down = down;
    in->keyboard.keys[key].clicked++;
}

void
zr_input_button(struct zr_context *ctx, enum zr_buttons id, int x, int y, int down)
{
    struct zr_mouse_button *btn;
    struct zr_input *in;
    ZR_ASSERT(ctx);
    if (!ctx) return;
    in = &ctx->input;
    if (in->mouse.buttons[id].down == down) return;

    btn = &in->mouse.buttons[id];
    btn->clicked_pos.x = (float)x;
    btn->clicked_pos.y = (float)y;
    btn->down = down;
    btn->clicked++;
}

void
zr_input_scroll(struct zr_context *ctx, float y)
{
    ZR_ASSERT(ctx);
    if (!ctx) return;
    ctx->input.mouse.scroll_delta += y;
}

void
zr_input_glyph(struct zr_context *ctx, const zr_glyph glyph)
{
    zr_size len = 0;
    zr_rune unicode;

    struct zr_input *in;
    ZR_ASSERT(ctx);
    if (!ctx) return;
    in = &ctx->input;

    len = zr_utf_decode(glyph, &unicode, ZR_UTF_SIZE);
    if (len && ((in->keyboard.text_len + len) < ZR_INPUT_MAX)) {
        zr_utf_encode(unicode, &in->keyboard.text[in->keyboard.text_len],
            ZR_INPUT_MAX - in->keyboard.text_len);
        in->keyboard.text_len += len;
    }
}

void
zr_input_char(struct zr_context *ctx, char c)
{
    zr_glyph glyph;
    ZR_ASSERT(ctx);
    if (!ctx) return;
    glyph[0] = c;
    zr_input_glyph(ctx, glyph);
}

void
zr_input_unicode(struct zr_context *ctx, zr_rune unicode)
{
    zr_glyph rune;
    ZR_ASSERT(ctx);
    if (!ctx) return;
    zr_utf_encode(unicode, rune, ZR_UTF_SIZE);
    zr_input_glyph(ctx, rune);
}

void
zr_input_end(struct zr_context *ctx)
{
    struct zr_input *in;
    ZR_ASSERT(ctx);
    if (!ctx) return;
    in = &ctx->input;
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
    return (zr_input_has_mouse_click_down_in_rect(i, id, b, zr_false) &&
            btn->clicked) ? zr_true : zr_false;
}

int
zr_input_is_mouse_click_down_in_rect(const struct zr_input *i, enum zr_buttons id,
    struct zr_rect b, int down)
{
    const struct zr_mouse_button *btn;
    if (!i) return zr_false;
    btn = &i->mouse.buttons[id];
    return (zr_input_has_mouse_click_down_in_rect(i, id, b, down) &&
            btn->clicked) ? zr_true : zr_false;
}

int
zr_input_any_mouse_click_in_rect(const struct zr_input *in, struct zr_rect b)
{
    int i, down = 0;
    for (i = 0; i < ZR_BUTTON_MAX; ++i)
        down = down || zr_input_is_mouse_click_in_rect(in, (enum zr_buttons)i, b);
    return down;
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
    return (!i->mouse.buttons[id].down && i->mouse.buttons[id].clicked);
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
    zr_flags a, const struct zr_user_font *f)
{
    struct zr_rect label;
    zr_size text_width;

    ZR_ASSERT(o);
    ZR_ASSERT(t);
    if (!o || !t) return;

    b.h = ZR_MAX(b.h, 2 * t->padding.y);
    label.x = 0; label.w = 0;
    label.y = b.y + t->padding.y;
    label.h = b.h - 2 * t->padding.y;

    text_width = f->width(f->userdata, f->height, (const char*)string, len);
    text_width += (zr_size)(2 * t->padding.x);

    /* align in x-axis */
    if (a & ZR_TEXT_ALIGN_LEFT) {
        label.x = b.x + t->padding.x;
        label.w = ZR_MAX(0, b.w - 2 * t->padding.x);
    } else if (a & ZR_TEXT_ALIGN_CENTERED) {
        label.w = ZR_MAX(1, 2 * t->padding.x + (float)text_width);
        label.x = (b.x + t->padding.x + ((b.w - 2 * t->padding.x) - label.w) / 2);
        label.x = ZR_MAX(b.x + t->padding.x, label.x);
        label.w = ZR_MIN(b.x + b.w, label.x + label.w);
        if (label.w >= label.x) label.w -= label.x;
    } else if (a & ZR_TEXT_ALIGN_RIGHT) {
        label.x = ZR_MAX(b.x + t->padding.x, (b.x + b.w) - (2 * t->padding.x + (float)text_width));
        label.w = (float)text_width + 2 * t->padding.x;
    } else return;

    /* align in y-axis */
    if (a & ZR_TEXT_ALIGN_MIDDLE) {
        label.y = b.y + b.h/2.0f - (float)f->height/2.0f;
        label.h = b.h - (b.h/2.0f + f->height/2.0f);
    } else if (a & ZR_TEXT_ALIGN_BOTTOM) {
        label.y = b.y + b.h - f->height;
        label.h = f->height;
    }
    zr_draw_text(o, label, (const char*)string,
        len, f, t->background, t->text);
}

static void
zr_widget_text_wrap(struct zr_command_buffer *o, struct zr_rect b,
    const char *string, zr_size len, const struct zr_text *t,
    const struct zr_user_font *f)
{
    float width;
    zr_size glyphs = 0;
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

    b.w = ZR_MAX(b.w, 2 * t->padding.x);
    b.h = ZR_MAX(b.h, 2 * t->padding.y);
    b.h = b.h - 2 * t->padding.y;

    line.x = b.x + t->padding.x;
    line.y = b.y + t->padding.y;
    line.w = b.w - 2 * t->padding.x;
    line.h = 2 * t->padding.y + f->height;

    fitting = zr_use_font_glyph_clamp(f, string, len, line.w, &glyphs, &width);
    while (done < len) {
        if (!fitting || line.y + line.h >= (b.y + b.h)) break;
        zr_widget_text(o, line, &string[done], fitting, &text, ZR_TEXT_LEFT, f);
        done += fitting;
        line.y += f->height + 2 * t->padding.y;
        fitting = zr_use_font_glyph_clamp(f, &string[done], len - done,
                                            line.w, &glyphs, &width);
    }
}

/* ===============================================================
 *
 *                          BUTTON
 *
 * ===============================================================*/
static void
zr_draw_symbol(struct zr_command_buffer *out, enum zr_symbol_type type,
    struct zr_rect content, struct zr_color background, struct zr_color foreground,
    float border_width, const struct zr_user_font *font)
{
    switch (type) {
    case ZR_SYMBOL_X:
    case ZR_SYMBOL_UNDERSCORE:
    case ZR_SYMBOL_PLUS:
    case ZR_SYMBOL_MINUS: {
        /* single character text symbol */
        const char *X = (type == ZR_SYMBOL_X) ? "x":
            (type == ZR_SYMBOL_UNDERSCORE) ? "_":
            (type == ZR_SYMBOL_PLUS) ? "+": "-";
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
        if (type == ZR_SYMBOL_RECT || type == ZR_SYMBOL_RECT_FILLED) {
            zr_fill_rect(out, content,  0, foreground);
            if (type == ZR_SYMBOL_RECT_FILLED)
                zr_fill_rect(out, zr_shrink_rect(content, border_width), 0, background);
        } else {
            zr_fill_circle(out, content, foreground);
            if (type == ZR_SYMBOL_CIRCLE_FILLED)
                zr_fill_circle(out, zr_shrink_rect(content, 1), background);
        }
    } break;
    case ZR_SYMBOL_TRIANGLE_UP:
    case ZR_SYMBOL_TRIANGLE_DOWN:
    case ZR_SYMBOL_TRIANGLE_LEFT:
    case ZR_SYMBOL_TRIANGLE_RIGHT: {
        enum zr_heading heading;
        struct zr_vec2 points[3];
        heading = (type == ZR_SYMBOL_TRIANGLE_RIGHT) ? ZR_RIGHT :
            (type == ZR_SYMBOL_TRIANGLE_LEFT) ? ZR_LEFT:
            (type == ZR_SYMBOL_TRIANGLE_UP) ? ZR_UP: ZR_DOWN;
        zr_triangle_from_direction(points, content, 0, 0, heading);
        zr_fill_triangle(out, points[0].x, points[0].y, points[1].x, points[1].y,
            points[2].x, points[2].y, foreground);
    } break;
    default:
    case ZR_SYMBOL_NONE:
    case ZR_SYMBOL_MAX: break;
    }
}

static int
zr_button_behavior(zr_flags *state, struct zr_rect r,
    const struct zr_input *i, enum zr_button_behavior behavior)
{
    int ret = 0;
    *state = ZR_WIDGET_STATE_INACTIVE;
    if (!i) return 0;
    if (zr_input_is_mouse_hovering_rect(i, r)) {
        *state = ZR_WIDGET_STATE_HOVERED;
        if (zr_input_is_mouse_down(i, ZR_BUTTON_LEFT))
            *state = ZR_WIDGET_STATE_ACTIVE;
        if (zr_input_has_mouse_click_in_rect(i, ZR_BUTTON_LEFT, r)) {
            ret = (behavior != ZR_BUTTON_DEFAULT) ?
                zr_input_is_mouse_down(i, ZR_BUTTON_LEFT):
                zr_input_is_mouse_released(i, ZR_BUTTON_LEFT);
        }
    }
    if (*state == ZR_WIDGET_STATE_HOVERED && !zr_input_is_mouse_prev_hovering_rect(i, r))
        *state |= ZR_WIDGET_STATE_ENTERED;
    else if (zr_input_is_mouse_prev_hovering_rect(i, r))
        *state |= ZR_WIDGET_STATE_LEFT;
    return ret;
}

static const struct zr_style_item*
zr_draw_button(struct zr_command_buffer *out,
    const struct zr_rect *bounds, zr_flags state,
    const struct zr_style_button *style)
{
    const struct zr_style_item *background;
    if (state & ZR_WIDGET_STATE_HOVERED)
        background = &style->hover;
    else if (state & ZR_WIDGET_STATE_ACTIVE)
        background = &style->active;
    else background = &style->normal;

    if (background->type == ZR_STYLE_ITEM_IMAGE) {
        zr_draw_image(out, *bounds, &background->data.image);
    } else {
        zr_fill_rect(out, *bounds, style->rounding, style->border_color);
        zr_fill_rect(out, zr_shrink_rect(*bounds, style->border), style->rounding,
                    background->data.color);
    }
    return background;
}

static void
zr_draw_button_text(struct zr_command_buffer *out,
    const struct zr_rect *bounds, const struct zr_rect *content, zr_flags state,
    const struct zr_style_button *style, const char *txt, zr_size len,
    zr_flags text_alignment, const struct zr_user_font *font)
{
    struct zr_text text;
    const struct zr_style_item *background;
    background = zr_draw_button(out, bounds, state, style);

    if (background->type == ZR_STYLE_ITEM_COLOR)
        text.background = background->data.color;
    else text.background = style->text_background;
    if (state & ZR_WIDGET_STATE_HOVERED)
        text.text = style->text_hover;
    else if (state & ZR_WIDGET_STATE_ACTIVE)
        text.text = style->text_active;
    else text.text = style->text_normal;
    text.padding = zr_vec2(0,0);
    zr_widget_text(out, *content, txt, len, &text, text_alignment, font);
}

static void
zr_draw_button_symbol(struct zr_command_buffer *out,
    const struct zr_rect *bounds, const struct zr_rect *content,
    zr_flags state, const struct zr_style_button *style,
    enum zr_symbol_type type, const struct zr_user_font *font)
{
    struct zr_color sym, bg;
    const struct zr_style_item *background;
    background = zr_draw_button(out, bounds, state, style);
    if (background->type == ZR_STYLE_ITEM_COLOR)
        bg = background->data.color;
    else bg = style->text_background;

    if (state & ZR_WIDGET_STATE_HOVERED)
        sym = style->text_hover;
    else if (state & ZR_WIDGET_STATE_ACTIVE)
        sym = style->text_active;
    else sym = style->text_normal;
    zr_draw_symbol(out, type, *content, bg, sym, 1, font);
}

static void
zr_draw_button_image(struct zr_command_buffer *out,
    const struct zr_rect *bounds, const struct zr_rect *content,
    zr_flags state, const struct zr_style_button *style, const struct zr_image *img)
{
    zr_draw_button(out, bounds, state, style);
    zr_draw_image(out, *content, img);
}

static void
zr_draw_button_text_symbol(struct zr_command_buffer *out,
    const struct zr_rect *bounds, const struct zr_rect *label,
    const struct zr_rect *symbol, zr_flags state, const struct zr_style_button *style,
    const char *str, zr_size len, enum zr_symbol_type type,
    const struct zr_user_font *font)
{
    struct zr_color sym, bg;
    struct zr_text text;
    const struct zr_style_item *background;
    background = zr_draw_button(out, bounds, state, style);
    if (background->type == ZR_STYLE_ITEM_COLOR) {
        text.background = background->data.color;
        bg = background->data.color;
    } else {
        text.background = style->text_background;
        bg = style->text_background;
    }

    if (state & ZR_WIDGET_STATE_HOVERED) {
        sym = style->text_hover;
        text.text = style->text_hover;
    } else if (state & ZR_WIDGET_STATE_ACTIVE) {
        sym = style->text_active;
        text.text = style->text_active;
    } else {
        sym = style->text_normal;
        text.text = style->text_normal;
    }
    text.padding = zr_vec2(0,0);
    zr_draw_symbol(out, type, *symbol, style->text_background, sym, 0, font);
    zr_widget_text(out, *label, str, len, &text, ZR_TEXT_CENTERED, font);
}

static void
zr_draw_button_text_image(struct zr_command_buffer *out,
    const struct zr_rect *bounds, const struct zr_rect *label,
    const struct zr_rect *image, zr_flags state, const struct zr_style_button *style,
    const char *str, zr_size len, const struct zr_user_font *font,
    const struct zr_image *img)
{
    struct zr_text text;
    const struct zr_style_item *background;
    background = zr_draw_button(out, bounds, state, style);

    if (background->type == ZR_STYLE_ITEM_COLOR)
        text.background = background->data.color;
    else text.background = style->text_background;
    if (state & ZR_WIDGET_STATE_HOVERED)
        text.text = style->text_hover;
    else if (state & ZR_WIDGET_STATE_ACTIVE)
        text.text = style->text_active;
    else text.text = style->text_normal;

    text.padding = zr_vec2(0,0);
    zr_widget_text(out, *label, str, len, &text, ZR_TEXT_CENTERED, font);
    zr_draw_image(out, *image, img);
}

static int
zr_do_button(zr_flags *state, struct zr_command_buffer *out, struct zr_rect r,
    const struct zr_style_button *style, const struct zr_input *in,
    enum zr_button_behavior behavior, struct zr_rect *content)
{
    struct zr_vec2 pad;
    struct zr_rect bounds;

    ZR_ASSERT(style);
    ZR_ASSERT(state);
    ZR_ASSERT(out);
    if (!out || !style)
        return zr_false;

    /* calculate button content space */
    pad.x = style->padding.x + style->border;
    pad.y = style->padding.y + style->border;

    content->x = r.x + style->padding.x;
    content->y = r.y + style->padding.y;
    content->w = r.w - 2 * style->padding.x;
    content->h = r.h - 2 * style->padding.y;

    /* execute button behavior */
    bounds.x = r.x - style->touch_padding.x;
    bounds.y = r.y - style->touch_padding.y;
    bounds.w = r.w + 2 * style->touch_padding.x;
    bounds.h = r.h + 2 * style->touch_padding.y;
    return zr_button_behavior(state, bounds, in, behavior);
}

static int
zr_do_button_text(zr_flags *state,
    struct zr_command_buffer *out, struct zr_rect bounds,
    const char *string, zr_size len, zr_flags align, enum zr_button_behavior behavior,
    const struct zr_style_button *style, const struct zr_input *in,
    const struct zr_user_font *font)
{
    struct zr_rect content;
    int ret = zr_false;

    ZR_ASSERT(state);
    ZR_ASSERT(style);
    ZR_ASSERT(out);
    ZR_ASSERT(string);
    ZR_ASSERT(font);
    if (!out || !style || !font || !string)
        return zr_false;

    ret = zr_do_button(state, out, bounds, style, in, behavior, &content);
    if (style->draw_begin)
        style->draw_begin(out, style->userdata);
    if (style->draw.button_text)
        style->draw.button_text(out, &bounds, &content, *state, style,
                                string, len, align, font);
    else zr_draw_button_text(out, &bounds, &content, *state, style,
                            string, len, align, font);
    if (style->draw_end)
        style->draw_begin(out, style->userdata);
    return ret;
}

static int
zr_do_button_symbol(zr_flags *state,
    struct zr_command_buffer *out, struct zr_rect bounds,
    enum zr_symbol_type symbol, enum zr_button_behavior behavior,
    const struct zr_style_button *style, const struct zr_input *in,
    const struct zr_user_font *font)
{
    int ret;
    struct zr_rect content;

    ZR_ASSERT(state);
    ZR_ASSERT(style);
    ZR_ASSERT(font);
    ZR_ASSERT(out);
    if (!out || !style || !font || !state)
        return zr_false;

    ret = zr_do_button(state, out, bounds, style, in, behavior, &content);
    if (style->draw_begin)
        style->draw_begin(out, style->userdata);
    if (style->draw.button_symbol)
        style->draw.button_symbol(out, &bounds, &content, *state, style, symbol, font);
    else zr_draw_button_symbol(out, &bounds, &content, *state, style, symbol, font);
    if (style->draw_end)
        style->draw_begin(out, style->userdata);
    return ret;
}

static int
zr_do_button_image(zr_flags *state,
    struct zr_command_buffer *out, struct zr_rect bounds,
    struct zr_image img, enum zr_button_behavior b,
    const struct zr_style_button *style, const struct zr_input *in)
{
    int ret;
    struct zr_rect content;

    ZR_ASSERT(state);
    ZR_ASSERT(style);
    ZR_ASSERT(out);
    if (!out || !style || !state)
        return zr_false;

    ret = zr_do_button(state, out, bounds, style, in, b, &content);
    content.x += style->image_padding.x;
    content.y += style->image_padding.y;
    content.w -= 2 * style->image_padding.x;
    content.h -= 2 * style->image_padding.y;

    if (style->draw_begin)
        style->draw_begin(out, style->userdata);
    if (style->draw.button_image)
        style->draw.button_image(out, &bounds, &content, *state, style, &img);
    else zr_draw_button_image(out, &bounds, &content, *state, style, &img);
    if (style->draw_end)
        style->draw_begin(out, style->userdata);
    return ret;
}

static int
zr_do_button_text_symbol(zr_flags *state,
    struct zr_command_buffer *out, struct zr_rect bounds,
    enum zr_symbol_type symbol, const char *str, zr_size len, zr_flags align,
    enum zr_button_behavior behavior, const struct zr_style_button *style,
    const struct zr_user_font *font, const struct zr_input *in)
{
    int ret;
    struct zr_rect tri = {0,0,0,0};
    struct zr_rect content;

    ZR_ASSERT(style);
    ZR_ASSERT(out);
    ZR_ASSERT(font);
    if (!out || !style || !font)
        return zr_false;

    ret = zr_do_button(state, out, bounds, style, in, behavior, &content);
    tri.y = content.y + (content.h/2) - font->height/2;
    tri.w = font->height; tri.h = font->height;
    if (align & ZR_TEXT_LEFT) {
        tri.x = (content.x + content.w) - (2 * style->padding.x + tri.w);
        tri.x = ZR_MAX(tri.x, 0);
    } else tri.x = content.x + 2 * style->padding.x;

    if (style->draw_begin)
        style->draw_begin(out, style->userdata);
    if (style->draw.button_text_symbol)
        zr_draw_button_text_symbol(out, &bounds, &content, &tri,
                                    *state, style, str, len, symbol, font);
    else zr_draw_button_text_symbol(out, &bounds, &content, &tri,
                                    *state, style, str, len, symbol, font);
    if (style->draw_end)
        style->draw_begin(out, style->userdata);
    return ret;
}

static int
zr_do_button_text_image(zr_flags *state,
    struct zr_command_buffer *out, struct zr_rect bounds,
    struct zr_image img, const char* str, zr_size len, zr_flags align,
    enum zr_button_behavior behavior, const struct zr_style_button *style,
    const struct zr_user_font *font, const struct zr_input *in)
{
    int ret;
    struct zr_rect icon;
    struct zr_rect content;

    ZR_ASSERT(style);
    ZR_ASSERT(state);
    ZR_ASSERT(font);
    ZR_ASSERT(out);
    if (!out || !font || !style || !str)
        return zr_false;

    ret = zr_do_button(state, out, bounds, style, in, behavior, &content);
    icon.y = bounds.y + style->padding.y;
    icon.w = icon.h = bounds.h - 2 * style->padding.y;
    if (align & ZR_TEXT_ALIGN_LEFT) {
        icon.x = (bounds.x + bounds.w) - (2 * style->padding.x + icon.w);
        icon.x = ZR_MAX(icon.x, 0);
    } else icon.x = bounds.x + 2 * style->padding.x;

    icon.x += style->image_padding.x;
    icon.y += style->image_padding.y;
    icon.w -= 2 * style->image_padding.x;
    icon.h -= 2 * style->image_padding.y;

    if (style->draw_begin)
        style->draw_begin(out, style->userdata);
    if (style->draw.button_text_image)
        zr_draw_button_text_image(out, &bounds, &content, &icon,
                                    *state, style, str, len, font, &img);
    else zr_draw_button_text_image(out, &bounds, &content, &icon,
                                    *state, style, str, len, font, &img);
    if (style->draw_end)
        style->draw_begin(out, style->userdata);
    return ret;
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

static int
zr_toggle_behavior(const struct zr_input *in, struct zr_rect select,
    zr_flags *state, int active)
{
    *state = ZR_WIDGET_STATE_INACTIVE;
    if (in && zr_input_is_mouse_hovering_rect(in, select))
        *state = ZR_WIDGET_STATE_HOVERED;
    if (zr_input_mouse_clicked(in, ZR_BUTTON_LEFT, select)) {
        *state = ZR_WIDGET_STATE_ACTIVE;
        active = !active;
    }
    if (*state == ZR_WIDGET_STATE_HOVERED && !zr_input_is_mouse_prev_hovering_rect(in, select))
        *state |= ZR_WIDGET_STATE_ENTERED;
    else if (zr_input_is_mouse_prev_hovering_rect(in, select))
        *state |= ZR_WIDGET_STATE_LEFT;
    return active;
}

static void
zr_draw_checkbox(struct zr_command_buffer *out,
    zr_flags state, const struct zr_style_toggle *style, int active,
    const struct zr_rect *label, const struct zr_rect *selector,
    const struct zr_rect *cursors, const char *string, zr_size len,
    const struct zr_user_font *font)
{
    const struct zr_style_item *background;
    const struct zr_style_item *cursor;
    struct zr_text text;

    if (state & ZR_WIDGET_STATE_HOVERED) {
        background = &style->hover;
        cursor = &style->cursor_hover;
        text.text = style->text_hover;
    } else if (state & ZR_WIDGET_STATE_ACTIVE) {
        background = &style->hover;
        cursor = &style->cursor_hover;
        text.text = style->text_active;
    } else {
        background = &style->normal;
        cursor = &style->cursor_normal;
        text.text = style->text_normal;
    }

    if (background->type == ZR_STYLE_ITEM_IMAGE)
        zr_draw_image(out, *selector, &background->data.image);
    else zr_fill_rect(out, *selector, 0, background->data.color);
    if (active) {
        if (cursor->type == ZR_STYLE_ITEM_IMAGE)
            zr_draw_image(out, *cursors, &cursor->data.image);
        else zr_fill_rect(out, *cursors, 0, cursor->data.color);
    }

    text.padding.x = 0;
    text.padding.y = 0;
    text.background = style->text_background;
    zr_widget_text(out, *label, string, len, &text, ZR_TEXT_LEFT, font);
}

static void
zr_draw_option(struct zr_command_buffer *out,
    zr_flags state, const struct zr_style_toggle *style, int active,
    const struct zr_rect *label, const struct zr_rect *selector,
    const struct zr_rect *cursors, const char *string, zr_size len,
    const struct zr_user_font *font)
{
    const struct zr_style_item *background;
    const struct zr_style_item *cursor;
    struct zr_text text;

    if (state & ZR_WIDGET_STATE_HOVERED) {
        background = &style->hover;
        cursor = &style->cursor_hover;
        text.text = style->text_hover;
    } else if (state & ZR_WIDGET_STATE_ACTIVE) {
        background = &style->hover;
        cursor = &style->cursor_hover;
        text.text = style->text_active;
    } else {
        background = &style->normal;
        cursor = &style->cursor_normal;
        text.text = style->text_normal;
    }

    if (background->type == ZR_STYLE_ITEM_IMAGE)
        zr_draw_image(out, *selector, &background->data.image);
    else zr_fill_circle(out, *selector, background->data.color);
    if (active) {
        if (cursor->type == ZR_STYLE_ITEM_IMAGE)
            zr_draw_image(out, *cursors, &cursor->data.image);
        else zr_fill_circle(out, *cursors, cursor->data.color);
    }

    text.padding.x = 0;
    text.padding.y = 0;
    text.background = style->text_background;
    zr_widget_text(out, *label, string, len, &text, ZR_TEXT_LEFT, font);
}

static int
zr_do_toggle(zr_flags *state,
    struct zr_command_buffer *out, struct zr_rect r,
    int *active, const char *str, zr_size len, enum zr_toggle_type type,
    const struct zr_style_toggle *style, const struct zr_input *in,
    const struct zr_user_font *font)
{
    int was_active;
    struct zr_rect bounds;
    struct zr_rect select;
    struct zr_rect cursor;
    struct zr_rect label;
    float cursor_pad;

    ZR_ASSERT(style);
    ZR_ASSERT(out);
    ZR_ASSERT(font);
    if (!out || !style || !font || !active)
        return 0;

    r.w = ZR_MAX(r.w, font->height + 2 * style->padding.x);
    r.h = ZR_MAX(r.h, font->height + 2 * style->padding.y);

    /* add additional touch padding for touch screen devices */
    bounds.x = r.x - style->touch_padding.x;
    bounds.y = r.y - style->touch_padding.y;
    bounds.w = r.w + 2 * style->touch_padding.x;
    bounds.h = r.h + 2 * style->touch_padding.y;

    /* calculate the selector space */
    select.w = ZR_MIN(r.h, font->height + style->padding.y);
    select.h = select.w;
    select.x = r.x + style->padding.x;
    select.y = (r.y + style->padding.y + (select.w / 2)) - (font->height / 2);
    cursor_pad = (type == ZR_TOGGLE_OPTION) ?
        (float)(int)(select.w / 4):
        (float)(int)(select.h / 6);

    /* calculate the bounds of the cursor inside the selector */
    select.h = ZR_MAX(select.w, cursor_pad * 2);
    cursor.h = select.h - cursor_pad * 2;
    cursor.w = cursor.h;
    cursor.x = select.x + cursor_pad;
    cursor.y = select.y + cursor_pad;

    /* label behind the selector */
    label.x = r.x + select.w + style->padding.x * 2;
    label.y = select.y;
    label.w = ZR_MAX(r.x + r.w, label.x + style->padding.x);
    label.w -= (label.x + style->padding.x);
    label.h = select.w;

    was_active = *active;
    *active = zr_toggle_behavior(in, bounds, state, *active);
    if (style->draw_begin)
        style->draw_begin(out, style->userdata);
    if (type == ZR_TOGGLE_CHECK) {
        if (style->draw.checkbox)
            style->draw.checkbox(out, *state,
                style, *active, &label, &select, &cursor, str, len, font);
        else zr_draw_checkbox(out, *state, style, *active, &label,
                &select, &cursor, str, len, font);
    } else {
        if (style->draw.radio)
            style->draw.radio(out, *state, style,
                *active, &label, &select, &cursor, str, len, font);
        else zr_draw_option(out, *state, style, *active, &label,
            &select, &cursor, str, len, font);
    }
    if (style->draw_end)
        style->draw_begin(out, style->userdata);
    return (was_active != *active);
}

/* ===============================================================
 *
 *                          SELECTABLE
 *
 * ===============================================================*/
static void
zr_draw_selectable(struct zr_command_buffer *out,
    zr_flags state, const struct zr_style_selectable *style, int active,
    const struct zr_rect *bounds, const char *string, zr_size len,
    zr_flags align, const struct zr_user_font *font)
{
    const struct zr_style_item *background;
    struct zr_text text;
    text.padding = style->padding;

    if (!active) {
        if (state & ZR_WIDGET_STATE_ACTIVE) {
            background = &style->pressed;
            text.text = style->text_pressed;
        } else if (state & ZR_WIDGET_STATE_HOVERED) {
            background = &style->hover;
            text.text = style->text_hover;
        } else {
            background = &style->normal;
            text.text = style->text_normal;
        }
    } else {
        if (state & ZR_WIDGET_STATE_ACTIVE) {
            background = &style->pressed_active;
            text.text = style->text_pressed_active;
        } else if (state & ZR_WIDGET_STATE_HOVERED) {
            background = &style->hover_active;
            text.text = style->text_hover_active;
        } else {
            background = &style->normal_active;
            text.text = style->text_normal_active;
        }
    }

    if (background->type == ZR_STYLE_ITEM_IMAGE) {
        zr_draw_image(out, *bounds, &background->data.image);
        text.background = zr_rgba(0,0,0,0);
    } else {
        zr_fill_rect(out, *bounds, style->rounding, background->data.color);
        text.background = background->data.color;
    }
    zr_widget_text(out, *bounds, string, len, &text, align, font);
}

static int
zr_do_selectable(zr_flags *state, struct zr_command_buffer *out,
    struct zr_rect bounds, const char *str, zr_size len, zr_flags align, int *value,
    const struct zr_style_selectable *style, const struct zr_input *in,
    const struct zr_user_font *font)
{
    int old_value;
    struct zr_rect touch;

    ZR_ASSERT(state);
    ZR_ASSERT(out);
    ZR_ASSERT(str);
    ZR_ASSERT(len);
    ZR_ASSERT(value);
    ZR_ASSERT(style);
    ZR_ASSERT(font);

    if (!state || !out || !str || !len || !value || !style || !font) return 0;
    old_value = *value;

    touch.x = bounds.x - style->touch_padding.x;
    touch.y = bounds.y - style->touch_padding.y;
    touch.w = bounds.w + style->touch_padding.x * 2;
    touch.h = bounds.h + style->touch_padding.y * 2;
    if (zr_button_behavior(state, touch, in, ZR_BUTTON_DEFAULT))
        *value = !(*value);

    if (style->draw_begin)
        style->draw_begin(out, style->userdata);
    if (style->draw)
        style->draw(out, *state, style, *value, &bounds,
            str, len, align, font);
    else zr_draw_selectable(out, *state, style, *value, &bounds,
            str, len, align, font);
    if (style->draw_end)
        style->draw_begin(out, style->userdata);
    return old_value != *value;
}

/* ===============================================================
 *
 *                          SLIDER
 *
 * ===============================================================*/
static float
zr_slider_behavior(zr_flags *state, struct zr_rect *cursor,
    const struct zr_input *in, const struct zr_style_slider *style,
    struct zr_rect bounds, float slider_min, float slider_max, float slider_value,
    float slider_step, float slider_steps)
{
    int inslider = in && zr_input_is_mouse_hovering_rect(in, bounds);
    int incursor = in && zr_input_has_mouse_click_down_in_rect(in, ZR_BUTTON_LEFT, bounds, zr_true);

    *state = (inslider) ? ZR_WIDGET_STATE_HOVERED: ZR_WIDGET_STATE_INACTIVE;
    if (in && inslider && incursor)
    {
        const float d = in->mouse.pos.x - (cursor->x + cursor->w / 2.0f);
        const float pxstep = (bounds.w - (2 * style->padding.x)) / slider_steps;

        /* only update value if the next slider step is reached */
        *state = ZR_WIDGET_STATE_ACTIVE;
        if (ZR_ABS(d) >= pxstep) {
            float ratio = 0;
            const float steps = (float)((int)(ZR_ABS(d) / pxstep));
            slider_value += (d > 0) ? (slider_step*steps) : -(slider_step*steps);
            slider_value = ZR_CLAMP(slider_min, slider_value, slider_max);
            ratio = (slider_value - slider_min)/slider_step;
            cursor->x = bounds.x + (cursor->w * ratio);
        }
    }
    if (*state == ZR_WIDGET_STATE_HOVERED && !zr_input_is_mouse_prev_hovering_rect(in, bounds))
        *state |= ZR_WIDGET_STATE_ENTERED;
    else if (zr_input_is_mouse_prev_hovering_rect(in, bounds))
        *state |= ZR_WIDGET_STATE_LEFT;
    return slider_value;
}

static void
zr_draw_slider(struct zr_command_buffer *out, zr_flags state,
    const struct zr_style_slider *style, const struct zr_rect *bounds,
    const struct zr_rect *virtual_cursor, float min, float value, float max)
{
    struct zr_rect fill;
    struct zr_rect bar;
    struct zr_rect scursor;
    const struct zr_style_item *background;

    struct zr_color bar_color;
    const struct zr_style_item *cursor;
    if (state & ZR_WIDGET_STATE_ACTIVE) {
        background = &style->active;
        bar_color = style->bar_active;
        cursor = &style->cursor_active;
    } else if (state & ZR_WIDGET_STATE_HOVERED) {
        background = &style->hover;
        bar_color = style->bar_hover;
        cursor = &style->cursor_hover;
    } else {
        background = &style->normal;
        bar_color = style->bar_normal;
        cursor = &style->cursor_normal;
    }

    bar.x = bounds->x;
    bar.y = (bounds->y + virtual_cursor->h/2) - virtual_cursor->h/8;
    bar.w = bounds->w;
    bar.h = bounds->h/6;

    scursor.h = style->cursor_size.y;
    scursor.w = style->cursor_size.x;
    scursor.y = (bar.y + bar.h/2.0f) - scursor.h/2.0f;
    scursor.x = (value <= min) ? virtual_cursor->x: (value >= max) ?
        ((bar.x + bar.w) - virtual_cursor->w):
        virtual_cursor->x - (virtual_cursor->w/2);

    fill.w = (scursor.x + (scursor.w/2.0f)) - bar.x;
    fill.x = bar.x;
    fill.y = bar.y;
    fill.h = bar.h;

    /* draw background */
    if (background->type == ZR_STYLE_ITEM_IMAGE) {
        zr_draw_image(out, *bounds, &background->data.image);
    } else {
        zr_fill_rect(out, *bounds, style->rounding, style->border_color);
        zr_fill_rect(out, zr_shrink_rect(*bounds, style->border), style->rounding,
            background->data.color);
    }

    /* draw slider bar */
    zr_fill_rect(out, bar, style->rounding, bar_color);
    zr_fill_rect(out, fill, style->rounding, style->bar_filled);

    /* draw cursor */
    if (cursor->type == ZR_STYLE_ITEM_IMAGE)
        zr_draw_image(out, scursor, &cursor->data.image);
    else zr_fill_circle(out, scursor, cursor->data.color);
}

static float
zr_do_slider(zr_flags *state,
    struct zr_command_buffer *out, struct zr_rect bounds,
    float min, float val, float max, float step,
    const struct zr_style_slider *style, const struct zr_input *in,
    const struct zr_user_font *font)
{
    float slider_range;
    float slider_min;
    float slider_max;
    float slider_value;
    float slider_steps;
    float cursor_offset;
    struct zr_rect cursor;

    ZR_ASSERT(style);
    ZR_ASSERT(out);
    if (!out || !style)
        return 0;

    /* remove padding from slider bounds */
    bounds.x = bounds.x + style->padding.x;
    bounds.y = bounds.y + style->padding.y;
    bounds.h = ZR_MAX(bounds.h, 2 * style->padding.y);
    bounds.w = ZR_MAX(bounds.w, 1 + bounds.h + 2 * style->padding.x);
    bounds.h -= 2 * style->padding.y;
    bounds.w -= 2 * style->padding.y;

    /* optional buttons */
    if (style->show_buttons) {
        zr_flags ws;
        struct zr_rect button;
        button.y = bounds.y;
        button.w = bounds.h;
        button.h = bounds.h;

        /* decrement button */
        button.x = bounds.x;
        if (zr_do_button_symbol(&ws, out, button, style->dec_symbol, ZR_BUTTON_DEFAULT,
            &style->dec_button, in, font))
            val -= step;

        /* increment button */
        button.x = (bounds.x + bounds.w) - button.w;
        if (zr_do_button_symbol(&ws, out, button, style->inc_symbol, ZR_BUTTON_DEFAULT,
            &style->inc_button, in, font))
            val += step;

        bounds.x = bounds.x + button.w + style->spacing.x;
        bounds.w = bounds.w - (2 * button.w + 2 * style->spacing.x);
    }

    /* make sure the provided values are correct */
    slider_max = ZR_MAX(min, max);
    slider_min = ZR_MIN(min, max);
    slider_value = ZR_CLAMP(slider_min, val, slider_max);
    slider_range = slider_max - slider_min;
    slider_steps = slider_range / step;

    /* calculate slider virtual cursor bounds */
    cursor_offset = (slider_value - slider_min) / step;
    cursor.h = bounds.h;
    cursor.w = bounds.w / (slider_steps + 1);
    cursor.x = bounds.x + (cursor.w * cursor_offset);
    cursor.y = bounds.y;
    slider_value = zr_slider_behavior(state, &cursor, in, style, bounds,
                        slider_min, slider_max, slider_value, step, slider_steps);

    if (style->draw_begin)
        style->draw_begin(out, style->userdata);
    if (style->draw)
        style->draw(out, *state, style, &bounds, &cursor,
            slider_min, slider_value, slider_max);
    else zr_draw_slider(out, *state, style, &bounds, &cursor,
        slider_min, slider_value, slider_max);
    if (style->draw_end)
        style->draw_begin(out, style->userdata);
    return slider_value;
}

/* ===============================================================
 *
 *                          PROGRESSBAR
 *
 * ===============================================================*/
static zr_size
zr_progress_behavior(zr_flags *state, const struct zr_input *in,
    struct zr_rect r, zr_size max, zr_size value, int modifiable)
{
    *state = ZR_WIDGET_STATE_INACTIVE;
    if (in && modifiable && zr_input_is_mouse_hovering_rect(in, r)) {
        if (zr_input_is_mouse_down(in, ZR_BUTTON_LEFT)) {
            float ratio = ZR_MAX(0, (float)(in->mouse.pos.x - r.x)) / (float)r.w;
            value = (zr_size)ZR_MAX(0,((float)max * ratio));
            *state = ZR_WIDGET_STATE_ACTIVE;
        } else *state = ZR_WIDGET_STATE_HOVERED;
    }
    if (*state == ZR_WIDGET_STATE_HOVERED && !zr_input_is_mouse_prev_hovering_rect(in, r))
        *state |= ZR_WIDGET_STATE_ENTERED;
    else if (zr_input_is_mouse_prev_hovering_rect(in, r))
        *state |= ZR_WIDGET_STATE_LEFT;

    if (!max) return value;
    value = ZR_MIN(value, max);
    return value;
}

static void
zr_draw_progress(struct zr_command_buffer *out, zr_flags state,
    const struct zr_style_progress *style, const struct zr_rect *bounds,
    const struct zr_rect *scursor, zr_size value, zr_size max)
{
    const struct zr_style_item *background;
    const struct zr_style_item *cursor;

    ZR_UNUSED(max);
    ZR_UNUSED(value);
    if (state & ZR_WIDGET_STATE_ACTIVE) {
        background = &style->active;
        cursor = &style->cursor_active;
    } else if (state & ZR_WIDGET_STATE_HOVERED){
        background = &style->hover;
        cursor = &style->cursor_hover;
    } else {
        background = &style->normal;
        cursor = &style->cursor_normal;
    }

    if (background->type == ZR_STYLE_ITEM_IMAGE)
        zr_draw_image(out, *bounds, &background->data.image);
    else zr_fill_rect(out, *bounds, style->rounding, background->data.color);

    if (cursor->type == ZR_STYLE_ITEM_IMAGE)
        zr_draw_image(out, *scursor, &cursor->data.image);
    else zr_fill_rect(out, *scursor, style->rounding, cursor->data.color);
}

static zr_size
zr_do_progress(zr_flags *state,
    struct zr_command_buffer *out, struct zr_rect bounds,
    zr_size value, zr_size max, int modifiable,
    const struct zr_style_progress *style, const struct zr_input *in)
{
    zr_size prog_value;
    float prog_scale;
    struct zr_rect cursor;

    ZR_ASSERT(style);
    ZR_ASSERT(out);
    if (!out || !style) return 0;

    cursor.w = ZR_MAX(bounds.w, 2 * style->padding.x);
    cursor.h = ZR_MAX(bounds.h, 2 * style->padding.y);
    cursor = zr_pad_rect(bounds, zr_vec2(style->padding.x, style->padding.y));

    prog_scale = (float)value / (float)max;
    cursor.w = (bounds.w - 2) * prog_scale;

    prog_value = ZR_MIN(value, max);
    prog_value = zr_progress_behavior(state, in, bounds, max, prog_value, modifiable);

    if (style->draw_begin)
        style->draw_begin(out, style->userdata);
    if (style->draw)
        style->draw(out, *state, style, &bounds, &cursor, value, max);
    else zr_draw_progress(out, *state, style, &bounds, &cursor, value, max);
    if (style->draw_end)
        style->draw_begin(out, style->userdata);
    return prog_value;
}

/* ===============================================================
 *
 *                          SCROLLBAR
 *
 * ===============================================================*/
static float
zr_scrollbar_behavior(zr_flags *state, struct zr_input *in,
    int has_scrolling, struct zr_rect scroll,
    struct zr_rect cursor, float scroll_offset,
    float target, float scroll_step, enum zr_orientation o)
{
    int left_mouse_down;
    int left_mouse_click_in_cursor;
    if (!in) return scroll_offset;

    *state = ZR_WIDGET_STATE_INACTIVE;
    left_mouse_down = in->mouse.buttons[ZR_BUTTON_LEFT].down;
    left_mouse_click_in_cursor = zr_input_has_mouse_click_down_in_rect(in,
        ZR_BUTTON_LEFT, cursor, zr_true);
    if (zr_input_is_mouse_hovering_rect(in, scroll))
        *state = ZR_WIDGET_STATE_HOVERED;

    if (left_mouse_down && left_mouse_click_in_cursor) {
        /* update cursor by mouse dragging */
        float pixel, delta;
        *state = ZR_WIDGET_STATE_ACTIVE;
        if (o == ZR_VERTICAL) {
            pixel = in->mouse.delta.y;
            delta = (pixel / scroll.h) * target;
            scroll_offset = ZR_CLAMP(0, scroll_offset + delta, target - scroll.h);
            /* This is probably one of my most disgusting hacks I have ever done.
             * This basically changes the mouse clicked position with the moving
             * cursor. This allows for better scroll behavior but resulted into me
             * having to remove const correctness for input. But in the end I believe
             * it is worth it. */
            in->mouse.buttons[ZR_BUTTON_LEFT].clicked_pos.y += in->mouse.delta.y;
        } else {
            pixel = in->mouse.delta.x;
            delta = (pixel / scroll.w) * target;
            scroll_offset = ZR_CLAMP(0, scroll_offset + delta, target - scroll.w);
            in->mouse.buttons[ZR_BUTTON_LEFT].clicked_pos.x += in->mouse.delta.x;
        }
    } else if (has_scrolling && ((in->mouse.scroll_delta<0) ||
            (in->mouse.scroll_delta>0))) {
        /* update cursor by mouse scrolling */
        scroll_offset = scroll_offset + scroll_step * (-in->mouse.scroll_delta);
        if (o == ZR_VERTICAL)
            scroll_offset = ZR_CLAMP(0, scroll_offset, target - scroll.h);
        else scroll_offset = ZR_CLAMP(0, scroll_offset, target - scroll.w);
    }
    if (*state == ZR_WIDGET_STATE_HOVERED && !zr_input_is_mouse_prev_hovering_rect(in, scroll))
        *state |= ZR_WIDGET_STATE_ENTERED;
    else if (zr_input_is_mouse_prev_hovering_rect(in, scroll))
        *state |= ZR_WIDGET_STATE_LEFT;
    return scroll_offset;
}

static void
zr_draw_scrollbar(struct zr_command_buffer *out, zr_flags state,
    const struct zr_style_scrollbar *style, const struct zr_rect *bounds,
    const struct zr_rect *scroll)
{
    const struct zr_style_item *background;
    const struct zr_style_item *cursor;

    if (state & ZR_WIDGET_STATE_ACTIVE) {
        background = &style->active;
        cursor = &style->cursor_active;
    } else if (state & ZR_WIDGET_STATE_HOVERED) {
        background = &style->hover;
        cursor = &style->cursor_hover;
    } else {
        background = &style->normal;
        cursor = &style->cursor_normal;
    }

    /* draw background */
    if (background->type == ZR_STYLE_ITEM_COLOR) {
        zr_fill_rect(out, *bounds, style->rounding, style->border_color);
        zr_fill_rect(out, zr_shrink_rect(*bounds,style->border), style->rounding, background->data.color);
    } else {
        zr_draw_image(out, *bounds, &background->data.image);
    }

    /* draw cursor */
    if (cursor->type == ZR_STYLE_ITEM_IMAGE)
        zr_draw_image(out, *scroll, &cursor->data.image);
    else zr_fill_rect(out, *scroll, style->rounding, cursor->data.color);
}

static float
zr_do_scrollbarv(zr_flags *state,
    struct zr_command_buffer *out, struct zr_rect scroll, int has_scrolling,
    float offset, float target, float step, float button_pixel_inc,
    const struct zr_style_scrollbar *style, struct zr_input *in,
    const struct zr_user_font *font)
{
    struct zr_rect cursor;
    float scroll_step;
    float scroll_offset;
    float scroll_off;
    float scroll_ratio;

    ZR_ASSERT(out);
    ZR_ASSERT(style);
    ZR_ASSERT(state);
    if (!out || !style) return 0;

    scroll.w = ZR_MAX(scroll.w, 1);
    scroll.h = ZR_MAX(scroll.h, 2 * scroll.w);
    if (target <= scroll.h) return 0;

    /* optional scrollbar buttons */
    if (style->show_buttons) {
        zr_flags ws;
        float scroll_h;
        struct zr_rect button;
        button.x = scroll.x;
        button.w = scroll.w;
        button.h = scroll.w;

        scroll_h = scroll.h - 2 * button.h;
        scroll_step = ZR_MIN(step, button_pixel_inc);

        /* decrement button */
        button.y = scroll.y;
        if (zr_do_button_symbol(&ws, out, button, style->dec_symbol,
            ZR_BUTTON_REPEATER, &style->dec_button, in, font))
            offset = offset - scroll_step;

        /* increment button */
        button.y = scroll.y + scroll.h - button.h;
        if (zr_do_button_symbol(&ws, out, button, style->inc_symbol,
            ZR_BUTTON_REPEATER, &style->inc_button, in, font))
            offset = offset + scroll_step;

        scroll.y = scroll.y + button.h;
        scroll.h = scroll_h;
    }

    /* calculate scrollbar constants */
    scroll_step = ZR_MIN(step, scroll.h);
    scroll_offset = ZR_CLAMP(0, offset, target - scroll.h);
    scroll_ratio = scroll.h / target;
    scroll_off = scroll_offset / target;

    /* calculate scrollbar cursor bounds */
    cursor.h = (scroll_ratio * scroll.h - 2);
    cursor.y = scroll.y + (scroll_off * scroll.h) + 1;
    cursor.w = scroll.w - 2;
    cursor.x = scroll.x + 1;

    /* update scrollbar */
    scroll_offset = zr_scrollbar_behavior(state, in, has_scrolling, scroll, cursor,
        scroll_offset, target, scroll_step, ZR_VERTICAL);
    scroll_off = scroll_offset / target;
    cursor.y = scroll.y + (scroll_off * scroll.h);

    /* draw scrollbar */
    if (style->draw_begin)
        style->draw_begin(out, style->userdata);
    if (style->draw)
        style->draw(out, *state, style, &scroll, &cursor);
    else zr_draw_scrollbar(out, *state, style, &scroll, &cursor);
    if (style->draw_end)
        style->draw_begin(out, style->userdata);
    return scroll_offset;
}

static float
zr_do_scrollbarh(zr_flags *state,
    struct zr_command_buffer *out, struct zr_rect scroll, int has_scrolling,
    float offset, float target, float step, float button_pixel_inc,
    const struct zr_style_scrollbar *style, struct zr_input *in,
    const struct zr_user_font *font)
{
    struct zr_rect cursor;
    float scroll_step;
    float scroll_offset;
    float scroll_off;
    float scroll_ratio;

    ZR_ASSERT(out);
    ZR_ASSERT(style);
    if (!out || !style) return 0;

    /* scrollbar background */
    scroll.h = ZR_MAX(scroll.h, 1);
    scroll.w = ZR_MAX(scroll.w, 2 * scroll.h);
    if (target <= scroll.w) return 0;

    /* optional scrollbar buttons */
    if (style->show_buttons) {
        zr_flags ws;
        float scroll_w;
        struct zr_rect button;
        button.y = scroll.y;
        button.w = scroll.h;
        button.h = scroll.h;

        scroll_w = scroll.w - 2 * button.w;
        scroll_step = ZR_MIN(step, button_pixel_inc);

        /* decrement button */
        button.x = scroll.x;
        if (zr_do_button_symbol(&ws, out, button, style->dec_symbol,
            ZR_BUTTON_REPEATER, &style->dec_button, in, font))
            offset = offset - scroll_step;

        /* increment button */
        button.x = scroll.x + scroll.w - button.w;
        if (zr_do_button_symbol(&ws, out, button, style->inc_symbol,
            ZR_BUTTON_REPEATER, &style->inc_button, in, font))
            offset = offset + scroll_step;

        scroll.x = scroll.x + button.w;
        scroll.w = scroll_w;
    }

    /* calculate scrollbar constants */
    scroll_step = ZR_MIN(step, scroll.w);
    scroll_offset = ZR_CLAMP(0, offset, target - scroll.w);
    scroll_ratio = scroll.w / target;
    scroll_off = scroll_offset / target;

    /* calculate cursor bounds */
    cursor.w = scroll_ratio * scroll.w - 2;
    cursor.x = scroll.x + (scroll_off * scroll.w) + 1;
    cursor.h = scroll.h - 2;
    cursor.y = scroll.y + 1;

    /* update scrollbar */
    scroll_offset = zr_scrollbar_behavior(state, in, has_scrolling, scroll, cursor,
        scroll_offset, target, scroll_step, ZR_HORIZONTAL);
    scroll_off = scroll_offset / target;
    cursor.x = scroll.x + (scroll_off * scroll.w);

    /* draw scrollbar */
    if (style->draw_begin)
        style->draw_begin(out, style->userdata);
    if (style->draw)
        style->draw(out, *state, style, &scroll, &cursor);
    else zr_draw_scrollbar(out, *state, style, &scroll, &cursor);
    if (style->draw_end)
        style->draw_begin(out, style->userdata);
    return scroll_offset;
}

/* ===============================================================
 *
 *                          FILTER
 *
 * ===============================================================*/
int zr_filter_default(const struct zr_edit_box *box, zr_rune unicode)
{(void)unicode;ZR_UNUSED(box);return zr_true;}

int
zr_filter_ascii(const struct zr_edit_box *box, zr_rune unicode)
{
    ZR_UNUSED(box);
    if (unicode > 128) return zr_false;
    else return zr_true;
}

int
zr_filter_float(const struct zr_edit_box *box, zr_rune unicode)
{
    ZR_UNUSED(box);
    if ((unicode < '0' || unicode > '9') && unicode != '.' && unicode != '-')
        return zr_false;
    else return zr_true;
}

int
zr_filter_decimal(const struct zr_edit_box *box, zr_rune unicode)
{
    ZR_UNUSED(box);
    if ((unicode < '0' || unicode > '9') && unicode != '-')
        return zr_false;
    else return zr_true;
}

int
zr_filter_hex(const struct zr_edit_box *box, zr_rune unicode)
{
    ZR_UNUSED(box);
    if ((unicode < '0' || unicode > '9') &&
        (unicode < 'a' || unicode > 'f') &&
        (unicode < 'A' || unicode > 'F'))
        return zr_false;
    else return zr_true;
}

int
zr_filter_oct(const struct zr_edit_box *box, zr_rune unicode)
{
    ZR_UNUSED(box);
    if (unicode < '0' || unicode > '7')
        return zr_false;
    else return zr_true;
}

int
zr_filter_binary(const struct zr_edit_box *box, zr_rune unicode)
{
    ZR_UNUSED(box);
    if (unicode != '0' && unicode != '1')
        return zr_false;
    else return zr_true;
}

/* ===============================================================
 *
 *                              EDIT
 *
 * ===============================================================*/
static void
zr_edit_input_behavior(struct zr_edit_box *box, const struct zr_input *in, int has_special)
{
    char *buffer = zr_edit_box_get(box);
    zr_size len = zr_edit_box_len_char(box);
    zr_size min = ZR_MIN(box->sel.end, box->sel.begin);
    zr_size maxi = ZR_MAX(box->sel.end, box->sel.begin);
    zr_size diff = maxi - min;
    int enter, tab;

    /* text manipulation */
    if (zr_input_is_key_pressed(in,ZR_KEY_DEL))
        zr_edit_box_remove(box, ZR_DELETE);
    else if (zr_input_is_key_pressed(in,ZR_KEY_BACKSPACE))
        zr_edit_box_remove(box, ZR_REMOVE);

    enter = has_special && zr_input_is_key_pressed(in, ZR_KEY_ENTER);
    tab = has_special && zr_input_is_key_pressed(in, ZR_KEY_TAB);
    if (in->keyboard.text_len || enter || tab) {
        if (diff && box->cursor != box->glyphs) {
            /* replace text selection */
            zr_edit_box_remove(box, ZR_DELETE);
            box->cursor = min;
        }
        if (enter) zr_edit_box_add(box, "\n", 1);
        else if (tab) zr_edit_box_add(box, "    ", 4);
        else zr_edit_box_buffer_input(box, in);
        box->sel.begin = box->cursor;
        box->sel.end = box->cursor;
    }

    /* cursor key movement */
    if (zr_input_is_key_pressed(in, ZR_KEY_LEFT)) {
        box->cursor = (zr_size)(ZR_MAX(0, (int)box->cursor - 1));
        box->sel.begin = box->cursor;
        box->sel.end = box->cursor;
    }
    if (zr_input_is_key_pressed(in, ZR_KEY_RIGHT) && box->cursor < box->glyphs) {
        box->cursor = ZR_MIN((!box->glyphs) ? 0 : box->glyphs, box->cursor + 1);
        box->sel.begin = box->cursor;
        box->sel.end = box->cursor;
    }

    /* copy & cut & paste functionlity */
    if (zr_input_is_key_pressed(in, ZR_KEY_PASTE) && box->clip.paste)
        box->clip.paste(box->clip.userdata, box);

    if ((zr_input_is_key_pressed(in, ZR_KEY_COPY) && box->clip.copy) ||
        (zr_input_is_key_pressed(in, ZR_KEY_CUT) && box->clip.copy)) {
        if (diff && box->cursor != box->glyphs) {
            /* copy or cut text selection */
            zr_size l;
            zr_rune unicode;
            char *begin, *end;
            begin = zr_edit_buffer_at(&box->buffer, (int)min, &unicode, &l);
            end = zr_edit_buffer_at(&box->buffer, (int)maxi, &unicode, &l);
            box->clip.copy(box->clip.userdata, begin, (zr_size)(end - begin));
            if (zr_input_is_key_pressed(in, ZR_KEY_CUT))
                zr_edit_box_remove(box, ZR_DELETE);
        } else {
            /* copy or cut complete buffer */
            box->clip.copy(box->clip.userdata, buffer, len);
            if (zr_input_is_key_pressed(in, ZR_KEY_CUT))
                zr_edit_box_clear(box);
        }
    }
}

static void
zr_edit_click_behavior(zr_flags *state, const struct zr_style_edit *style,
    struct zr_rect *bounds, int show_cursor, struct zr_edit_box *box,
    const struct zr_input *in, const struct zr_user_font *font, const char *text,
    /* these three here describe the current text frame */
    zr_size text_len, zr_size glyph_off, zr_size glyph_cnt)
{
    /* check if the editbox is activated/deactivated */
    if (!in) return;
    *state = ZR_WIDGET_STATE_INACTIVE;
    if (in && in->mouse.buttons[ZR_BUTTON_LEFT].clicked &&
            in->mouse.buttons[ZR_BUTTON_LEFT].down) {
        box->active = ZR_INBOX(in->mouse.pos.x,in->mouse.pos.y,bounds->x,bounds->y,bounds->w,bounds->h);
        if (box->active) *state = ZR_WIDGET_STATE_ACTIVE;
    }
    if (ZR_INBOX(in->mouse.pos.x, in->mouse.pos.y, bounds->x, bounds->y, bounds->w, bounds->h))
        *state |= ZR_WIDGET_STATE_HOVERED;

    /* set cursor by mouse click and handle text selection */
    if (in && show_cursor && in->mouse.buttons[ZR_BUTTON_LEFT].down && box->active) {
        const char *visible = text;
        float xoff = in->mouse.pos.x - (bounds->x + style->padding.x + style->border);
        if (*state & ZR_WIDGET_STATE_HOVERED)
        {
            /* text selection in current text frame */
            zr_size glyph_index;
            zr_size glyph_pos=zr_user_font_glyph_index_at_pos(font,visible,text_len,xoff);
            if (glyph_cnt + glyph_off >= box->glyphs)
                glyph_index = glyph_off + ZR_MIN(glyph_pos, glyph_cnt);
            else glyph_index = glyph_off + ZR_MIN(glyph_pos, glyph_cnt-1);

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
        } else if (!ZR_INBOX(in->mouse.pos.x,in->mouse.pos.y,bounds->x,bounds->y,bounds->w,bounds->h) &&
            ZR_INBOX(in->mouse.buttons[ZR_BUTTON_LEFT].clicked_pos.x,
                in->mouse.buttons[ZR_BUTTON_LEFT].clicked_pos.y,bounds->x,bounds->y,bounds->w,bounds->h)
            && box->cursor != box->glyphs && box->cursor > 0)
        {
            /* text selection out of the current text frame */
            zr_size glyph = ((in->mouse.pos.x > bounds->x) &&
                box->cursor+1 < box->glyphs) ?
                box->cursor+1: box->cursor-1;
            zr_edit_box_set_cursor(box, glyph);
            if (box->sel.active) {
                box->sel.end = glyph;
                box->sel.active = zr_true;
            }
        } else box->sel.active = zr_false;
    } else box->sel.active = zr_false;
}

static void
zr_draw_edit(struct zr_command_buffer *out, zr_flags state,
    const struct zr_style_edit *style, const struct zr_rect *bounds,
    const struct zr_rect *label, const struct zr_rect *selection,
    int show_cursor, const char *unselected_text, zr_size unselected_len,
    const char *selected_text, zr_size selected_len,
    const struct zr_edit_box *box, const struct zr_user_font *font)
{
    const struct zr_style_item *background;
    const struct zr_style_item *cursor;
    struct zr_color selected;
    struct zr_color sel_text;
    struct zr_color text;

    /* select correct colors */
    struct zr_text txt;
    if (state & ZR_WIDGET_STATE_ACTIVE) {
        background = &style->active;
        cursor = &style->cursor_active;
        text = style->text_active;
        selected = style->selected_normal;
        sel_text = style->selected_text_normal;
    } else if (state & ZR_WIDGET_STATE_HOVERED) {
        background = &style->hover;
        cursor = &style->cursor_hover;
        text = style->text_hover;
        selected = style->selected_hover;
        sel_text = style->selected_hover;
    } else {
        background = &style->normal;
        cursor = &style->cursor_normal;
        text = style->text_normal;
        selected = style->selected_normal;
        sel_text = style->selected_normal;
    }

    /* draw background color/image */
    if (background->type == ZR_STYLE_ITEM_IMAGE) {
        zr_draw_image(out, *bounds, &background->data.image);
        txt.background = zr_rgba(0,0,0,0);
    } else {
        txt.background = background->data.color;
        zr_fill_rect(out, *bounds, style->rounding, style->border_color);
        zr_fill_rect(out, zr_shrink_rect(*bounds, style->border),
            style->rounding, style->normal.data.color);
    }

    /* draw unselected text */
    txt.padding = zr_vec2(0,0);
    txt.text = text;
    zr_widget_text(out, *label, unselected_text, unselected_len,
        &txt, ZR_TEXT_LEFT, font);

    if (box->active && show_cursor) {
        if (box->cursor == box->glyphs) {
            /* draw cursor at the end of the string */
            float text_width;
            zr_size cursor_w = (zr_size)style->cursor_size;
            zr_size s = font->width(font->userdata, font->height,
                                    unselected_text, unselected_len);
            text_width = (float)s;
            if (cursor->type == ZR_STYLE_ITEM_IMAGE)
                zr_draw_image(out, zr_rect(label->x+(float)text_width,
                        label->y, (float)cursor_w, label->h), &cursor->data.image);
            else zr_fill_rect(out, zr_rect(label->x+(float)text_width,
                        label->y, (float)cursor_w, label->h), 0, cursor->data.color);
        } else {
            /* draw text selection */
            struct zr_rect clip = out->clip;
            zr_push_scissor(out, clip);
            zr_fill_rect(out, *selection, 0, selected);

            txt.padding = zr_vec2(0,0);
            txt.background = selected;
            txt.text = sel_text;
            zr_widget_text(out, *selection, selected_text, selected_len,
                &txt, ZR_TEXT_LEFT, font);
            zr_push_scissor(out, clip);
        }
    }
}

static void
zr_do_edit_buffer(zr_flags *state, struct zr_command_buffer *out,
    struct zr_rect bounds, struct zr_edit_box *box, const struct zr_style_edit *style,
    int show_cursor, const struct zr_input *in, const struct zr_user_font *font)
{
    char *buffer;
    zr_size len;

    /* text frame */
    zr_size text_len = 0;
    zr_size glyph_off = 0;
    zr_size glyph_cnt = 0;
    zr_size offset = 0;
    float text_width = 0;

    /* selection text */
    char *selection_begin = 0, *selection_end = 0;
    zr_size off_begin = 0, off_end = 0, off_max = 0;

    struct zr_rect label;
    struct zr_rect selection = zr_rect(0,0,0,0);

    ZR_ASSERT(out);
    ZR_ASSERT(font);
    ZR_ASSERT(style);
    ZR_ASSERT(state);
    if (!out || !box || !style)
        return;

    buffer = zr_edit_box_get(box);
    text_len = len = zr_edit_box_len_char(box);
    bounds.w = ZR_MAX(bounds.w, 2 * style->padding.x + 2 * style->border);
    bounds.h = ZR_MAX(bounds.h, font->height + (2 * style->padding.y + 2 * style->border));

    /* calculate visible text frame */
    label.w = ZR_MAX(bounds.w,  - 2 * style->padding.x - 2 * style->border);
    label.w -= 2 * style->padding.x - 2 * style->border;
    {
        zr_size frames = 0;
        zr_size glyphs = 0;
        zr_size frame_len = 0;
        zr_size row_len = 0;
        zr_size cursor_w = (zr_size)style->cursor_size;
        float space = ZR_MAX(label.w, (float)cursor_w);
        space -= (float)cursor_w;

        while (text_len) {
            frames++;
            offset += frame_len;
            frame_len = zr_user_font_glyphs_fitting_in_space(font,
                &buffer[offset], text_len, space, &row_len, &glyphs, &text_width, 0);
            glyph_off += glyphs;
            if (glyph_off > box->cursor || !frame_len) break;
            text_len -= frame_len;
        }

        text_len = frame_len;
        glyph_cnt = glyphs;
        glyph_off = (frames <= 1) ? 0 : (glyph_off - glyphs);
        offset = (frames <= 1) ? 0 : offset;
    }

    /* update edit state */
    if (box->active && in)
        zr_edit_input_behavior(box, in, 0);
    zr_edit_click_behavior(state, style, &bounds, show_cursor, box, in, font,
        &buffer[offset], text_len, glyph_off, glyph_cnt);

    /* calculate unselected text bounds */
    label.x = bounds.x + style->padding.x + style->border;
    label.y = bounds.y + style->padding.y + style->border;
    label.h = bounds.h - (2 * style->padding.y + 2 * style->border);

    /* calculate selected text */
    if (box->active && show_cursor && box->cursor < box->glyphs) {
        zr_size s;
        zr_rune unicode;

        /* calculate selection text range */
        zr_size min = ZR_MIN(box->sel.end, box->sel.begin);
        zr_size maxi = ZR_MAX(box->sel.end, box->sel.begin);
        selection_begin = zr_edit_buffer_at(&box->buffer, (int)min, &unicode, &off_max);
        selection_end = zr_edit_buffer_at(&box->buffer, (int)maxi, &unicode, &off_max);
        off_begin = (zr_size)(selection_begin - (char*)box->buffer.memory.ptr);
        off_end = (zr_size)(selection_end - (char*)box->buffer.memory.ptr);

        /* calculate selected text bounds */
        selection = label;
        s = font->width(font->userdata, font->height, buffer + offset, off_begin - offset);
        selection.x += (float)s;
        s = font->width(font->userdata, font->height, selection_begin, ZR_MAX(off_max, off_end - off_begin));
        selection.w = (float)s;
    }

    /* draw edit */
    if (style->draw_begin)
        style->draw_begin(out, style->userdata);
    if (style->draw)
        style->draw(out, *state, style, &bounds, &label, &selection,
            show_cursor, &buffer[offset], text_len, selection_begin,
            ZR_MAX(off_max, off_end - off_begin), box, font);
    else zr_draw_edit(out, *state, style, &bounds, &label, &selection,
        show_cursor, &buffer[offset], text_len, selection_begin,
        ZR_MAX(off_max, off_end - off_begin), box, font);
    if (style->draw_end)
        style->draw_begin(out, style->userdata);
}

static zr_size
zr_do_edit_string(zr_flags *state, struct zr_command_buffer *out, struct zr_rect r,
    char *buffer, zr_size len, zr_size max, int *active,
    zr_size *cursor, int show_cursor, const struct zr_style_edit *style,
    zr_filter filter, const struct zr_input *in, const struct zr_user_font *font)
{
    struct zr_edit_box box;
    zr_edit_box_init(&box, buffer, max, 0, filter);
    box.buffer.allocated = len;
    box.active = *active;
    box.glyphs = zr_utf_len(buffer, len);
    if (!cursor) {
        box.cursor = box.glyphs;
    } else{
        box.cursor = ZR_MIN(*cursor, box.glyphs);
        box.sel.begin = box.cursor;
        box.sel.end = box.cursor;
    }
    zr_do_edit_buffer(state, out, r, &box, style, show_cursor, in, font);

    *active = box.active;
    if (cursor) *cursor = box.cursor;
    return zr_edit_box_len_char(&box);
}

/* ===============================================================
 *
 *                          PROPERTY
 *
 * ===============================================================*/
enum zr_property_status {
    ZR_PROPERTY_DEFAULT,
    ZR_PROPERTY_EDIT,
    ZR_PROPERTY_DRAG
};

enum zr_property_filter {
    ZR_FILTER_INT,
    ZR_FILTER_FLOAT
};

static float
zr_drag_behavior(zr_flags *state, const struct zr_input *in,
    struct zr_rect drag, float min, float val, float max, float inc_per_pixel)
{
    int left_mouse_down = in && in->mouse.buttons[ZR_BUTTON_LEFT].down;
    int left_mouse_click_in_cursor = in &&
        zr_input_has_mouse_click_down_in_rect(in, ZR_BUTTON_LEFT, drag, zr_true);

    *state = ZR_WIDGET_STATE_INACTIVE;
    if (zr_input_is_mouse_hovering_rect(in, drag))
        *state = ZR_WIDGET_STATE_HOVERED;

    if (left_mouse_down && left_mouse_click_in_cursor) {
        float delta, pixels;
        pixels = in->mouse.delta.x;
        delta = pixels * inc_per_pixel;
        val += delta;
        val = ZR_CLAMP(min, val, max);
        *state = ZR_WIDGET_STATE_ACTIVE;
    }
    if (*state == ZR_WIDGET_STATE_HOVERED && !zr_input_is_mouse_prev_hovering_rect(in, drag))
        *state |= ZR_WIDGET_STATE_ENTERED;
    else if (zr_input_is_mouse_prev_hovering_rect(in, drag))
        *state |= ZR_WIDGET_STATE_LEFT;
    return val;
}

static float
zr_property_behavior(zr_flags *ws, const struct zr_input *in,
    struct zr_rect property,  struct zr_rect label, struct zr_rect edit,
    struct zr_rect empty, int *state, float min, float value, float max,
    float step, float inc_per_pixel)
{
    ZR_UNUSED(step);
    if (in && *state == ZR_PROPERTY_DEFAULT) {
        if (zr_button_behavior(ws, edit, in, ZR_BUTTON_DEFAULT))
            *state = ZR_PROPERTY_EDIT;
        else if (zr_input_is_mouse_click_down_in_rect(in, ZR_BUTTON_LEFT, label, zr_true))
            *state = ZR_PROPERTY_DRAG;
        else if (zr_input_is_mouse_click_down_in_rect(in, ZR_BUTTON_LEFT, empty, zr_true))
            *state = ZR_PROPERTY_DRAG;
    }
    if (*state == ZR_PROPERTY_DRAG) {
        value = zr_drag_behavior(ws, in, property, min, value, max, inc_per_pixel);
        if (!(*ws & ZR_WIDGET_STATE_ACTIVE)) *state = ZR_PROPERTY_DEFAULT;
    }
    return value;
}

static void
zr_draw_property(struct zr_command_buffer *out, const struct zr_style_property *style,
    const struct zr_rect *bounds, const struct zr_rect *label, zr_flags state,
    const char *name, zr_size len, const struct zr_user_font *font)
{
    struct zr_text text;
    const struct zr_style_item *background;

    /* select correct background and text color */
    if (state & ZR_WIDGET_STATE_ACTIVE) {
        background = &style->active;
        text.text = style->label_active;
    } else if (state & ZR_WIDGET_STATE_HOVERED) {
        background = &style->hover;
        text.text = style->label_hover;
    } else {
        background = &style->normal;
        text.text = style->label_normal;
    }

    /* draw background */
    if (background->type == ZR_STYLE_ITEM_IMAGE) {
        zr_draw_image(out, *bounds, &background->data.image);
        text.background = zr_rgba(0,0,0,0);
    } else {
        text.background = background->data.color;
        zr_fill_rect(out, *bounds, style->rounding, style->border_color);
        zr_fill_rect(out, zr_shrink_rect(*bounds,style->border), style->rounding, background->data.color);
    }

    /* draw label */
    text.padding = zr_vec2(0,0);
    zr_widget_text(out, *label, name, len, &text, ZR_TEXT_CENTERED, font);
}

static float
zr_do_property(zr_flags *ws,
    struct zr_command_buffer *out, struct zr_rect property,
    const char *name, float min, float val, float max,
    float step, float inc_per_pixel, char *buffer, zr_size *len,
    int *state, zr_size *cursor, const struct zr_style_property *style,
    enum zr_property_filter filter, const struct zr_input *in,
    const struct zr_user_font *font)
{
    const zr_filter filters[] = {
        zr_filter_decimal,
        zr_filter_float
    };
    int active, old;
    zr_size num_len, name_len;
    char string[ZR_MAX_NUMBER_BUFFER];
    zr_size size;

    float property_min;
    float property_max;
    float property_value;

    char *dst = 0;
    zr_size *length;

    struct zr_rect left;
    struct zr_rect right;
    struct zr_rect label;
    struct zr_rect edit;
    struct zr_rect empty;

    /* make sure the provided values are correct */
    property_max = ZR_MAX(min, max);
    property_min = ZR_MIN(min, max);
    property_value = ZR_CLAMP(property_min, val, property_max);

    /* left decrement button */
    left.h = font->height/2;
    left.w = left.h;
    left.x = property.x + style->border + style->padding.x;
    left.y = property.y + style->border + property.h/2.0f - left.h/2;

    /* text label */
    name_len = zr_strsiz(name);
    size = font->width(font->userdata, font->height, name, name_len);
    label.x = left.x + left.w + style->padding.x;
    label.w = (float)size + 2 * style->padding.x;
    label.y = property.y + style->border;
    label.h = property.h - 2 * style->border;

    /* right increment button */
    right.y = left.y;
    right.w = left.w;
    right.h = left.h;
    right.x = property.x + property.w - (right.w + style->padding.x);

    /* edit */
    if (*state == ZR_PROPERTY_EDIT) {
        size = font->width(font->userdata, font->height, buffer, *len);
        length = len;
        dst = buffer;
    } else {
        zr_ftos(string, property_value);
        num_len = zr_string_float_limit(string, ZR_MAX_FLOAT_PRECISION);
        size = font->width(font->userdata, font->height, string, num_len);
        dst = string;
        length = &num_len;
    }
    edit.w =  (float)size + 2 * style->padding.x;
    edit.x = right.x - (edit.w + style->padding.x);
    edit.y = property.y + style->border + 1;
    edit.h = property.h - (2 * style->border + 2);

    /* empty left space activator */
    empty.w = edit.x - (label.x + label.w);
    empty.x = label.x + label.w;
    empty.y = property.y;
    empty.h = property.h;

    /* update property */
    old = (*state == ZR_PROPERTY_EDIT);
    property_value = zr_property_behavior(ws, in, property, label, edit, empty,
                        state, property_min, property_value, property_max,
                        step, inc_per_pixel);

    if (style->draw_begin)
        style->draw_begin(out, style->userdata);
    if (style->draw)
        style->draw(out, style, &property, &label, *ws, name, name_len, font);
    else zr_draw_property(out, style, &property, &label, *ws, name, name_len, font);
    if (style->draw_end)
        style->draw_begin(out, style->userdata);

    /* execute right and left button  */
    if (zr_do_button_symbol(ws, out, left, style->sym_left, ZR_BUTTON_DEFAULT,
        &style->dec_button, in, font))
        property_value = ZR_CLAMP(min, property_value - step, max);
    if (zr_do_button_symbol(ws, out, right, style->sym_right, ZR_BUTTON_DEFAULT,
        &style->inc_button, in, font))
        property_value = ZR_CLAMP(min, property_value + step, max);

    active = (*state == ZR_PROPERTY_EDIT);
    if (old != ZR_PROPERTY_EDIT && active) {
        /* property has been activated so setup buffer */
        zr_memcopy(buffer, dst, *length);
        *cursor = zr_utf_len(buffer, *length);
        *len = *length;
        length = len;
        dst = buffer;
    }

    *length = zr_do_edit_string(ws, out, edit, dst, *length, ZR_MAX_NUMBER_BUFFER,
        &active, cursor, 1, &style->edit, filters[filter],
        (*state == ZR_PROPERTY_EDIT) ? in: 0, font);
    if (active && zr_input_is_key_pressed(in, ZR_KEY_ENTER))
        active = !active;

    if (old && !active) {
        /* property is now not active so convert edit text to value*/
        *state = ZR_PROPERTY_DEFAULT;
        buffer[*len] = '\0';
        zr_string_float_limit(buffer, ZR_MAX_FLOAT_PRECISION);
        zr_strtof(&property_value, buffer);
        property_value = ZR_CLAMP(min, property_value, max);
    }
    return property_value;
}

/* ===============================================================
 *
 *                          COLOR PICKER
 *
 * ===============================================================*/
static int
zr_color_picker_behavior(zr_flags *state,
    const struct zr_rect *bounds, const struct zr_rect *matrix,
    const struct zr_rect *hue_bar, const struct zr_rect *alpha_bar,
    struct zr_color *color, const struct zr_input *in)
{
    float hsva[4];
    int value_changed = 0;
    int hsv_changed = 0;

    ZR_ASSERT(state);
    ZR_ASSERT(matrix);
    ZR_ASSERT(hue_bar);
    ZR_ASSERT(color);

    /* color matrix */
    zr_color_hsva_fv(hsva, *color);
    if (zr_button_behavior(state, *matrix, in, ZR_BUTTON_REPEATER)) {
        hsva[1] = ZR_SATURATE((in->mouse.pos.x - matrix->x) / (matrix->w-1));
        hsva[2] = 1.0f - ZR_SATURATE((in->mouse.pos.y - matrix->y) / (matrix->h-1));
        value_changed = hsv_changed = 1;
    }

    /* hue bar */
    if (zr_button_behavior(state, *hue_bar, in, ZR_BUTTON_REPEATER)) {
        hsva[0] = ZR_SATURATE((in->mouse.pos.y - hue_bar->y) / (hue_bar->h-1));
        value_changed = hsv_changed = 1;
    }
    /* alpha bar */
    if (alpha_bar) {
        if (zr_button_behavior(state, *alpha_bar, in, ZR_BUTTON_REPEATER)) {
            hsva[3] = 1.0f - ZR_SATURATE((in->mouse.pos.y - alpha_bar->y) / (alpha_bar->h-1));
            value_changed = 1;
        }
    }

    *state = ZR_WIDGET_STATE_INACTIVE;
    if (hsv_changed) {
        *color = zr_hsva_fv(hsva);
        *state = ZR_WIDGET_STATE_ACTIVE;
    }
    if (value_changed) {
        color->a = (zr_byte)(hsva[3] * 255.0f);
        *state = ZR_WIDGET_STATE_ACTIVE;
    }
    if (zr_input_is_mouse_hovering_rect(in, *bounds))
        *state = ZR_WIDGET_STATE_HOVERED;
    if (*state == ZR_WIDGET_STATE_HOVERED && !zr_input_is_mouse_prev_hovering_rect(in, *bounds))
        *state |= ZR_WIDGET_STATE_ENTERED;
    else if (zr_input_is_mouse_prev_hovering_rect(in, *bounds))
        *state |= ZR_WIDGET_STATE_LEFT;
    return value_changed;
}

static void
zr_draw_color_picker(struct zr_command_buffer *o, const struct zr_rect *matrix,
    const struct zr_rect *hue_bar, const struct zr_rect *alpha_bar,
    struct zr_color color)
{
    static const struct zr_color black = {0,0,0,255};
    static const struct zr_color white = {255, 255, 255, 255};
    static const struct zr_color black_trans = {0,0,0,0};

    const float crosshair_size = 7.0f;
    struct zr_color temp;
    float hsva[4];
    float line_y;
    int i;

    ZR_ASSERT(o);
    ZR_ASSERT(matrix);
    ZR_ASSERT(hue_bar);
    ZR_ASSERT(alpha_bar);

    /* draw hue bar */
    zr_color_hsv_fv(hsva, color);
    for (i = 0; i < 6; ++i) {
        static const struct zr_color hue_colors[] = {
            {255, 0, 0, 255}, {255,255,0,255}, {0,255,0,255}, {0, 255,255,255},
            {0,0,255,255}, {255, 0, 255, 255}, {255, 0, 0, 255}};
        zr_fill_rect_multi_color(o,
            zr_rect(hue_bar->x, hue_bar->y + (float)i * (hue_bar->h/6.0f) + 0.5f,
                hue_bar->w, (hue_bar->h/6.0f) + 0.5f), hue_colors[i], hue_colors[i],
                hue_colors[i+1], hue_colors[i+1]);
    }
    line_y = (float)(int)(hue_bar->y + hsva[0] * matrix->h + 0.5f);
    zr_stroke_line(o, hue_bar->x-1, line_y, hue_bar->x + hue_bar->w + 2,
        line_y, 1, zr_rgb(255,255,255));

    /* draw alpha bar */
    if (alpha_bar) {
        float alpha = ZR_SATURATE((float)color.a/255.0f);
        line_y = (float)(int)(alpha_bar->y +  (1.0f - alpha) * matrix->h + 0.5f);

        zr_fill_rect_multi_color(o, *alpha_bar, white, white, black, black);
        zr_stroke_line(o, alpha_bar->x-1, line_y, alpha_bar->x + alpha_bar->w + 2,
            line_y, 1, zr_rgb(255,255,255));
    }

    /* draw color matrix */
    temp = zr_hsv_f(hsva[0], 1.0f, 1.0f);
    zr_fill_rect_multi_color(o, *matrix, white, temp, temp, white);
    zr_fill_rect_multi_color(o, *matrix, black_trans, black_trans, black, black);

    /* draw cross-hair */
    {struct zr_vec2 p; float S = hsva[1]; float V = hsva[2];
    p.x = (float)(int)(matrix->x + S * matrix->w + 0.5f);
    p.y = (float)(int)(matrix->y + (1.0f - V) * matrix->h + 0.5f);
    zr_stroke_line(o, p.x - crosshair_size, p.y, p.x-2, p.y, 1.0f, white);
    zr_stroke_line(o, p.x + crosshair_size, p.y, p.x+2, p.y, 1.0f, white);
    zr_stroke_line(o, p.x, p.y + crosshair_size, p.x, p.y+2, 1.0f, zr_rgb(255,255,255));
    zr_stroke_line(o, p.x, p.y - crosshair_size, p.x, p.y-2, 1.0f, zr_rgb(255,255,255));}
}

static int
zr_do_color_picker(zr_flags *state,
    struct zr_command_buffer *out, struct zr_color *color,
    enum zr_color_picker_format fmt, struct zr_rect bounds,
    struct zr_vec2 padding, const struct zr_input *in,
    const struct zr_user_font *font)
{
    int ret = 0;
    struct zr_rect matrix;
    struct zr_rect hue_bar;
    struct zr_rect alpha_bar;
    float bar_w;

    ZR_ASSERT(out);
    ZR_ASSERT(color);
    ZR_ASSERT(state);
    ZR_ASSERT(font);
    if (!out || !color || !state || !font)
        return ret;

    bar_w = font->height;
    bounds.x += padding.x;
    bounds.y += padding.x;
    bounds.w -= 2 * padding.x;
    bounds.h -= 2 * padding.y;

    matrix.x = bounds.x;
    matrix.y = bounds.y;
    matrix.h = bounds.h;
    matrix.w = bounds.w - (3 * padding.x + 2 * bar_w);

    hue_bar.w = bar_w;
    hue_bar.y = bounds.y;
    hue_bar.h = matrix.h;
    hue_bar.x = matrix.x + matrix.w + padding.x;

    alpha_bar.x = hue_bar.x + hue_bar.w + padding.x;
    alpha_bar.y = bounds.y;
    alpha_bar.w = bar_w;
    alpha_bar.h = matrix.h;

    ret = zr_color_picker_behavior(state, &bounds, &matrix, &hue_bar,
        (fmt == ZR_RGBA) ? &alpha_bar:0, color, in);
    zr_draw_color_picker(out, &matrix, &hue_bar, (fmt == ZR_RGBA) ? &alpha_bar:0, *color);
    return ret;
}

/* ==============================================================
 *
 *                          STYLE
 *
 * ===============================================================*/
struct zr_style_item zr_style_item_image(struct zr_image img)
{struct zr_style_item i; i.type = ZR_STYLE_ITEM_IMAGE; i.data.image = img; return i;}

struct zr_style_item zr_style_item_color(struct zr_color col)
{struct zr_style_item i; i.type = ZR_STYLE_ITEM_COLOR; i.data.color = col; return i;}

struct zr_style_item zr_style_item_hide(void)
{struct zr_style_item i; i.type = ZR_STYLE_ITEM_COLOR; i.data.color = zr_rgba(0,0,0,0); return i;}

void
zr_style_default(struct zr_context *ctx)
{
    struct zr_style *style;
    struct zr_style_text *text;
    struct zr_style_button *button;
    struct zr_style_toggle *toggle;
    struct zr_style_selectable *select;
    struct zr_style_slider *slider;
    struct zr_style_progress *prog;
    struct zr_style_scrollbar *scroll;
    struct zr_style_edit *edit;
    struct zr_style_property *property;
    struct zr_style_combo *combo;
    struct zr_style_chart *chart;
    struct zr_style_tab *tab;
    struct zr_style_window *win;

    ZR_ASSERT(ctx);
    if (!ctx) return;
    style = &ctx->style;

    /* default text */
    text = &style->text;
    text->color = zr_rgb(175,175,175);
    text->padding = zr_vec2(4,4);

    /* default button */
    button = &style->button;
    zr_zero_struct(*button);
    button->normal          = zr_style_item_color(zr_rgb(50,50,50));
    button->hover           = zr_style_item_color(zr_rgb(40,40,40));
    button->active          = zr_style_item_color(zr_rgb(35,35,35));
    button->border_color    = zr_rgb(65,65,65);
    button->text_background = zr_rgb(50,50,50);
    button->text_normal     = zr_rgb(175, 175, 175);
    button->text_hover      = zr_rgb(165,165,165);
    button->text_active     = zr_rgb(155,155,155);
    button->padding         = zr_vec2(4.0f,4.0f);
    button->image_padding   = zr_vec2(0.0f,0.0f);
    button->touch_padding   = zr_vec2(0.0f, 0.0f);
    button->userdata        = zr_handle_ptr(0);
    button->text_alignment  = ZR_TEXT_CENTERED;
    button->border          = 1.0f;
    button->rounding        = 4.0f;
    button->fixed_width     = 0;
    button->fixed_height    = 0;
    button->has_fixed_size  = 0;
    button->draw_begin      = 0;
    button->draw_end        = 0;

    /* contextual button */
    button = &style->contextual_button;
    zr_zero_struct(*button);
    button->normal          = zr_style_item_color(zr_rgb(45,45,45));
    button->hover           = zr_style_item_color(zr_rgb(40,40,40));
    button->active          = zr_style_item_color(zr_rgb(35,35,35));
    button->border_color    = zr_rgb(45,45,45);
    button->text_background = zr_rgb(45,45,45);
    button->text_normal     = zr_rgb(175,175,175);
    button->text_hover      = zr_rgb(165,165,165);
    button->text_active     = zr_rgb(155,155,155);
    button->padding         = zr_vec2(4.0f,4.0f);
    button->touch_padding   = zr_vec2(0.0f,0.0f);
    button->userdata        = zr_handle_ptr(0);
    button->text_alignment  = ZR_TEXT_CENTERED;
    button->border          = 0.0f;
    button->rounding        = 0.0f;
    button->fixed_width     = 0;
    button->fixed_height    = 0;
    button->has_fixed_size  = 0;
    button->draw_begin      = 0;
    button->draw_end        = 0;

    /* menu button */
    button = &style->menu_button;
    zr_zero_struct(*button);
    button->normal          = zr_style_item_color(zr_rgb(45,45,45));
    button->hover           = zr_style_item_color(zr_rgb(40,40,40));
    button->active          = zr_style_item_color(zr_rgb(35,35,35));
    button->border_color    = zr_rgb(65,65,65);
    button->text_background = zr_rgb(40,40,40);
    button->text_normal     = zr_rgb(175,175,175);
    button->text_hover      = zr_rgb(175,175,175);
    button->text_active     = zr_rgb(175,175,175);
    button->padding         = zr_vec2(4.0f,4.0f);
    button->touch_padding   = zr_vec2(0.0f,0.0f);
    button->userdata        = zr_handle_ptr(0);
    button->text_alignment  = ZR_TEXT_CENTERED;
    button->border          = 0.0f;
    button->rounding        = 1.0f;
    button->fixed_width     = 0;
    button->fixed_height    = 0;
    button->has_fixed_size  = 0;
    button->draw_begin      = 0;
    button->draw_end        = 0;

    /* checkbox toggle */
    toggle = &style->checkbox;
    zr_zero_struct(*toggle);
    toggle->normal          = zr_style_item_color(zr_rgb(100,100,100));
    toggle->hover           = zr_style_item_color(zr_rgb(120,120,120));
    toggle->active          = zr_style_item_color(zr_rgb(100,100,100));
    toggle->cursor_normal   = zr_style_item_color(zr_rgb(45,45,45));
    toggle->cursor_hover    = zr_style_item_color(zr_rgb(45,45,45));
    toggle->userdata        = zr_handle_ptr(0);
    toggle->text_background = zr_rgb(45,45,45);
    toggle->text_normal     = zr_rgb(175,175,175);
    toggle->text_hover      = zr_rgb(175,175,175);
    toggle->text_active     = zr_rgb(175,175,175);
    toggle->padding         = zr_vec2(4.0f, 4.0f);
    toggle->touch_padding   = zr_vec2(0,0);
    toggle->fixed_width     = 0;
    toggle->fixed_height    = 0;
    toggle->has_fixed_size  = 0;

    /* option toggle */
    toggle = &style->option;
    zr_zero_struct(*toggle);
    toggle->normal          = zr_style_item_color(zr_rgb(100,100,100));
    toggle->hover           = zr_style_item_color(zr_rgb(120,120,120));
    toggle->active          = zr_style_item_color(zr_rgb(100,100,100));
    toggle->cursor_normal   = zr_style_item_color(zr_rgb(45,45,45));
    toggle->cursor_hover    = zr_style_item_color(zr_rgb(45,45,45));
    toggle->userdata        = zr_handle_ptr(0);
    toggle->text_background = zr_rgb(45,45,45);
    toggle->text_normal     = zr_rgb(175,175,175);
    toggle->text_hover      = zr_rgb(175,175,175);
    toggle->text_active     = zr_rgb(175,175,175);
    toggle->padding         = zr_vec2(4.0f, 4.0f);
    toggle->touch_padding   = zr_vec2(0,0);
    toggle->fixed_width     = 0;
    toggle->fixed_height    = 0;
    toggle->has_fixed_size  = 0;

    /* selectable */
    select = &style->selectable;
    zr_zero_struct(*select);
    select->normal          = zr_style_item_color(zr_rgb(45,45,45));
    select->hover           = zr_style_item_color(zr_rgb(45,45,45));
    select->pressed         = zr_style_item_color(zr_rgb(45,45,45));
    select->normal_active   = zr_style_item_color(zr_rgb(100,100,100));
    select->hover_active    = zr_style_item_color(zr_rgb(100,100,100));
    select->pressed_active  = zr_style_item_color(zr_rgb(100,100,100));
    select->text_normal     = zr_rgb(175,175,175);
    select->text_hover      = zr_rgb(175,175,175);
    select->text_pressed    = zr_rgb(175,175,175);
    select->text_normal_active  = zr_rgb(45,45,45);
    select->text_hover_active   = zr_rgb(45,45,45);
    select->text_pressed_active = zr_rgb(45,45,45);
    select->fixed_width     = 0;
    select->fixed_height    = 0;
    select->rounding        = 0.0f;
    select->has_fixed_size  = 0;
    select->padding         = zr_vec2(4.0f,4.0f);
    select->touch_padding   = zr_vec2(0,0);
    select->userdata        = zr_handle_ptr(0);
    select->draw_begin      = 0;
    select->draw            = 0;
    select->draw_end        = 0;

    /* slider */
    slider = &style->slider;
    zr_zero_struct(*slider);
    slider->normal          = zr_style_item_hide();
    slider->hover           = zr_style_item_hide();
    slider->active          = zr_style_item_hide();
    slider->bar_normal      = zr_rgb(38,38,38);
    slider->bar_hover       = zr_rgb(38,38,38);
    slider->bar_active      = zr_rgb(38,38,38);
    slider->bar_filled      = zr_rgb(100,100,100);
    slider->cursor_normal   = zr_style_item_color(zr_rgb(100,100,100));
    slider->cursor_hover    = zr_style_item_color(zr_rgb(120,120,120));
    slider->cursor_active   = zr_style_item_color(zr_rgb(150,150,150));;
    slider->inc_symbol      = ZR_SYMBOL_TRIANGLE_RIGHT;
    slider->dec_symbol      = ZR_SYMBOL_TRIANGLE_LEFT;
    slider->cursor_size     = zr_vec2(16,16);
    slider->padding         = zr_vec2(4,4);
    slider->spacing         = zr_vec2(4,4);
    slider->userdata        = zr_handle_ptr(0);
    slider->show_buttons    = zr_false;
    slider->bar_height      = 8;
    slider->rounding        = 0;
    slider->fixed_width     = 0;
    slider->fixed_height    = 0;
    slider->has_fixed_size  = 0;
    slider->draw_begin      = 0;
    slider->draw            = 0;
    slider->draw_end        = 0;

    /* slider buttons */
    button = &style->slider.inc_button;
    button->normal          = zr_style_item_color(zr_rgb(40,40,40));
    button->hover           = zr_style_item_color(zr_rgb(42,42,42));
    button->active          = zr_style_item_color(zr_rgb(44,44,44));
    button->border_color    = zr_rgb(65,65,65);
    button->text_background = zr_rgb(40,40,40);
    button->text_normal     = zr_rgb(175,175,175);
    button->text_hover      = zr_rgb(175,175,175);
    button->text_active     = zr_rgb(175,175,175);
    button->padding         = zr_vec2(8.0f,8.0f);
    button->touch_padding   = zr_vec2(0.0f,0.0f);
    button->userdata        = zr_handle_ptr(0);
    button->text_alignment  = ZR_TEXT_CENTERED;
    button->border          = 1.0f;
    button->rounding        = 0.0f;
    button->fixed_width     = 0;
    button->fixed_height    = 0;
    button->has_fixed_size  = 0;
    button->draw_begin      = 0;
    button->draw_end        = 0;
    style->slider.dec_button = style->slider.inc_button;

    /* progressbar */
    prog = &style->progress;
    zr_zero_struct(*prog);
    prog->normal            = zr_style_item_color(zr_rgb(38,38,38));
    prog->hover             = zr_style_item_color(zr_rgb(40,40,40));
    prog->active            = zr_style_item_color(zr_rgb(42,42,42));
    prog->cursor_normal     = zr_style_item_color(zr_rgb(100,100,100));
    prog->cursor_hover      = zr_style_item_color(zr_rgb(120,120,120));
    prog->cursor_active     = zr_style_item_color(zr_rgb(150,150,150));
    prog->userdata          = zr_handle_ptr(0);
    prog->padding           = zr_vec2(4,4);
    prog->rounding          = 0;
    prog->fixed_width       = 0;
    prog->fixed_height      = 0;
    prog->has_fixed_size    = 0;
    prog->draw_begin        = 0;
    prog->draw              = 0;
    prog->draw_end          = 0;

    /* scrollbars */
    scroll = &style->scrollh;
    zr_zero_struct(*scroll);
    scroll->normal          = zr_style_item_color(zr_rgb(40,40,40));
    scroll->hover           = zr_style_item_color(zr_rgb(40,40,40));
    scroll->active          = zr_style_item_color(zr_rgb(40,40,40));
    scroll->cursor_normal   = zr_style_item_color(zr_rgb(100,100,100));
    scroll->cursor_hover    = zr_style_item_color(zr_rgb(120,120,120));
    scroll->cursor_active   = zr_style_item_color(zr_rgb(150,150,150));
    scroll->dec_symbol      = ZR_SYMBOL_CIRCLE_FILLED;
    scroll->inc_symbol      = ZR_SYMBOL_CIRCLE_FILLED;
    scroll->userdata        = zr_handle_ptr(0);
    scroll->border_color    = zr_rgb(65,65,65);
    scroll->padding         = zr_vec2(4,4);
    scroll->show_buttons    = zr_false;
    scroll->border          = 0;
    scroll->rounding        = 0;
    scroll->draw_begin      = 0;
    scroll->draw            = 0;
    scroll->draw_end        = 0;
    style->scrollv = style->scrollh;

    /* scrollbars buttons */
    button = &style->scrollh.inc_button;
    button->normal          = zr_style_item_color(zr_rgb(40,40,40));
    button->hover           = zr_style_item_color(zr_rgb(42,42,42));
    button->active          = zr_style_item_color(zr_rgb(44,44,44));
    button->border_color    = zr_rgb(65,65,65);
    button->text_background = zr_rgb(40,40,40);
    button->text_normal     = zr_rgb(175,175,175);
    button->text_hover      = zr_rgb(175,175,175);
    button->text_active     = zr_rgb(175,175,175);
    button->padding         = zr_vec2(4.0f,4.0f);
    button->touch_padding   = zr_vec2(0.0f,0.0f);
    button->userdata        = zr_handle_ptr(0);
    button->text_alignment  = ZR_TEXT_CENTERED;
    button->border          = 1.0f;
    button->rounding        = 0.0f;
    button->fixed_width     = 0;
    button->fixed_height    = 0;
    button->has_fixed_size  = 0;
    button->draw_begin      = 0;
    button->draw_end        = 0;
    style->scrollh.dec_button = style->scrollh.inc_button;
    style->scrollv.inc_button = style->scrollh.inc_button;
    style->scrollv.dec_button = style->scrollh.inc_button;

    /* edit */
    edit = &style->edit;
    zr_zero_struct(*edit);
    edit->normal            = zr_style_item_color(zr_rgb(45,45,45));
    edit->hover             = zr_style_item_color(zr_rgb(47,47,47));
    edit->active            = zr_style_item_color(zr_rgb(49,49,49));
    edit->cursor_normal     = zr_style_item_color(zr_rgb(100,100,100));
    edit->cursor_hover      = zr_style_item_color(zr_rgb(102,102,102));
    edit->cursor_active     = zr_style_item_color(zr_rgb(104,104,104));
    edit->border_color      = zr_rgb(65,65,65);;
    edit->text_normal       = zr_rgb(135,135,135);
    edit->text_hover        = zr_rgb(135,135,135);
    edit->text_active       = zr_rgb(135,135,135);
    edit->selected_normal   = zr_rgb(135,135,135);
    edit->selected_hover    = zr_rgb(135,135,135);
    edit->selected_text_normal  = zr_rgb(45,45,45);
    edit->selected_text_hover   = zr_rgb(45,45,45);
    edit->userdata          = zr_handle_ptr(0);
    edit->padding           = zr_vec2(4,4);
    edit->cursor_size       = 8;
    edit->border            = 1;
    edit->rounding          = 0;
    edit->fixed_width       = 0;
    edit->fixed_height      = 0;
    edit->has_fixed_size    = 0;
    edit->draw_begin        = 0;
    edit->draw              = 0;
    edit->draw_end          = 0;

    /* property */
    property = &style->property;
    zr_zero_struct(*property);
    property->normal        = zr_style_item_color(zr_rgb(38,38,38));
    property->hover         = zr_style_item_color(zr_rgb(40,40,40));
    property->active        = zr_style_item_color(zr_rgb(42,42,42));
    property->border_color  = zr_rgb(65,65,65);
    property->label_normal  = zr_rgb(175,175,175);
    property->label_hover   = zr_rgb(175,175,175);
    property->label_active  = zr_rgb(175,175,175);
    property->sym_left      = ZR_SYMBOL_TRIANGLE_LEFT;
    property->sym_right     = ZR_SYMBOL_TRIANGLE_RIGHT;
    property->userdata      = zr_handle_ptr(0);
    property->padding       = zr_vec2(4,4);
    property->border        = 1;
    property->rounding      = 10;
    property->has_fixed_size = 0;
    property->fixed_width   = 0;
    property->fixed_height  = 0;
    property->draw_begin    = 0;
    property->draw          = 0;
    property->draw_end      = 0;

    /* property buttons */
    button = &style->property.dec_button;
    zr_zero_struct(*button);
    button->normal          = zr_style_item_color(zr_rgb(38,38,38));
    button->hover           = zr_style_item_color(zr_rgb(40,40,40));
    button->active          = zr_style_item_color(zr_rgb(42,42,42));
    button->border_color    = zr_rgba(0,0,0,0);
    button->text_background = zr_rgb(38,38,38);
    button->text_normal     = zr_rgb(175,175,175);
    button->text_hover      = zr_rgb(175,175,175);
    button->text_active     = zr_rgb(175,175,175);
    button->padding         = zr_vec2(0.0f,0.0f);
    button->touch_padding   = zr_vec2(0.0f,0.0f);
    button->userdata        = zr_handle_ptr(0);
    button->text_alignment  = ZR_TEXT_CENTERED;
    button->border          = 0.0f;
    button->rounding        = 0.0f;
    button->fixed_width     = 0;
    button->fixed_height    = 0;
    button->has_fixed_size  = 0;
    button->draw_begin      = 0;
    button->draw_end        = 0;
    style->property.inc_button = style->property.dec_button;

    /* property edit */
    edit = &style->property.edit;
    zr_zero_struct(*edit);
    edit->normal            = zr_style_item_color(zr_rgb(38,38,38));
    edit->hover             = zr_style_item_color(zr_rgb(40,40,40));
    edit->active            = zr_style_item_color(zr_rgb(42,42,42));
    edit->cursor_normal     = zr_style_item_color(zr_rgb(175,175,175));
    edit->cursor_hover      = zr_style_item_color(zr_rgb(175,175,175));
    edit->cursor_active     = zr_style_item_color(zr_rgb(175,175,175));
    edit->border_color      = zr_rgba(0,0,0,0);
    edit->text_normal       = zr_rgb(175,175,175);
    edit->text_hover        = zr_rgb(175,175,175);
    edit->text_active       = zr_rgb(175,175,175);
    edit->selected_normal   = zr_rgb(175,175,175);
    edit->selected_hover    = zr_rgb(175,175,175);
    edit->selected_text_normal  = zr_rgb(38,38,38);
    edit->selected_text_hover   = zr_rgb(50,50,50);
    edit->userdata          = zr_handle_ptr(0);
    edit->padding           = zr_vec2(0,0);
    edit->cursor_size       = 8;
    edit->border            = 0;
    edit->rounding          = 0;
    edit->fixed_width       = 0;
    edit->fixed_height      = 0;
    edit->has_fixed_size    = 0;
    edit->draw_begin        = 0;
    edit->draw              = 0;
    edit->draw_end          = 0;

    /* chart */
    chart = &style->line_chart;
    zr_zero_struct(*chart);
    chart->background = zr_style_item_color(zr_rgb(120,120,120));
    chart->border_color = zr_rgb(65,65,65);
    chart->selected_color = zr_rgb(256,0,0);
    chart->color = zr_rgb(45,45,45);
    chart->border = 0;
    chart->rounding = 0;
    chart->has_fixed_size = 0;
    chart->fixed_width = 0;
    chart->fixed_height = 0;
    chart->padding = zr_vec2(4,4);
    style->column_chart = *chart;

    /* combo */
    combo = &style->combo;
    combo->normal = zr_style_item_color(zr_rgb(45,45,45));
    combo->hover = zr_style_item_color(zr_rgb(45,45,45));
    combo->active = zr_style_item_color(zr_rgb(45,45,45));
    combo->border_color = zr_rgb(65,65,65);
    combo->label_normal = zr_rgb(175,175,175);
    combo->label_hover = zr_rgb(175,175,175);
    combo->label_active = zr_rgb(175,175,175);
    combo->sym_normal = ZR_SYMBOL_TRIANGLE_DOWN;
    combo->sym_hover = ZR_SYMBOL_TRIANGLE_DOWN;
    combo->sym_active =ZR_SYMBOL_TRIANGLE_DOWN;
    combo->content_padding = zr_vec2(4,4);
    combo->button_padding = zr_vec2(0,4);
    combo->spacing = zr_vec2(4,0);
    combo->border = 1;
    combo->rounding = 0;
    combo->has_fixed_size = 0;
    combo->fixed_width = 0;
    combo->fixed_height = 0;

    /* combo button */
    button = &style->combo.button;
    zr_zero_struct(*button);
    button->normal          = zr_style_item_color(zr_rgb(45,45,45));
    button->hover           = zr_style_item_color(zr_rgb(45,45,45));
    button->active          = zr_style_item_color(zr_rgb(45,45,45));
    button->border_color    = zr_rgba(0,0,0,0);
    button->text_background = zr_rgb(45,45,38);
    button->text_normal     = zr_rgb(175,175,175);
    button->text_hover      = zr_rgb(175,175,175);
    button->text_active     = zr_rgb(175,175,175);
    button->padding         = zr_vec2(2.0f,2.0f);
    button->touch_padding   = zr_vec2(0.0f,0.0f);
    button->userdata        = zr_handle_ptr(0);
    button->text_alignment  = ZR_TEXT_CENTERED;
    button->border          = 0.0f;
    button->rounding        = 0.0f;
    button->fixed_width     = 0;
    button->fixed_height    = 0;
    button->has_fixed_size  = 0;
    button->draw_begin      = 0;
    button->draw_end        = 0;

    /* tab */
    tab = &style->tab;
    tab->background = zr_style_item_color(zr_rgb(40,40,40));
    tab->border_color = zr_rgb(65,65,65);
    tab->text = zr_rgb(175,175,175);
    tab->sym_minimize = ZR_SYMBOL_TRIANGLE_DOWN;
    tab->sym_maximize = ZR_SYMBOL_TRIANGLE_RIGHT;
    tab->border = 1;
    tab->rounding = 0;
    tab->padding = zr_vec2(4,4);
    tab->spacing = zr_vec2(4,4);

    /* tab button */
    button = &style->tab.tab_button;
    zr_zero_struct(*button);
    button->normal          = zr_style_item_color(zr_rgb(40,40,40));
    button->hover           = zr_style_item_color(zr_rgb(40,40,40));
    button->active          = zr_style_item_color(zr_rgb(40,40,40));
    button->border_color    = zr_rgba(0,0,0,0);
    button->text_background = zr_rgb(40,40,40);
    button->text_normal     = zr_rgb(175,175,175);
    button->text_hover      = zr_rgb(175,175,175);
    button->text_active     = zr_rgb(175,175,175);
    button->padding         = zr_vec2(2.0f,2.0f);
    button->touch_padding   = zr_vec2(0.0f,0.0f);
    button->userdata        = zr_handle_ptr(0);
    button->text_alignment  = ZR_TEXT_CENTERED;
    button->border          = 0.0f;
    button->rounding        = 0.0f;
    button->fixed_width     = 0;
    button->fixed_height    = 0;
    button->has_fixed_size  = 0;
    button->draw_begin      = 0;
    button->draw_end        = 0;

    /* node button */
    button = &style->tab.node_button;
    zr_zero_struct(*button);
    button->normal          = zr_style_item_color(zr_rgb(45,45,45));
    button->hover           = zr_style_item_color(zr_rgb(45,45,45));
    button->active          = zr_style_item_color(zr_rgb(45,45,45));
    button->border_color    = zr_rgba(0,0,0,0);
    button->text_background = zr_rgb(40,40,40);
    button->text_normal     = zr_rgb(175,175,175);
    button->text_hover      = zr_rgb(175,175,175);
    button->text_active     = zr_rgb(175,175,175);
    button->padding         = zr_vec2(2.0f,2.0f);
    button->touch_padding   = zr_vec2(0.0f,0.0f);
    button->userdata        = zr_handle_ptr(0);
    button->text_alignment  = ZR_TEXT_CENTERED;
    button->border          = 0.0f;
    button->rounding        = 0.0f;
    button->fixed_width     = 0;
    button->fixed_height    = 0;
    button->has_fixed_size  = 0;
    button->draw_begin      = 0;
    button->draw_end        = 0;

    /* window header */
    win = &style->window;
    win->header.align = ZR_HEADER_RIGHT;
    win->header.close_symbol = ZR_SYMBOL_X;
    win->header.minimize_symbol = ZR_SYMBOL_MINUS;
    win->header.maximize_symbol = ZR_SYMBOL_PLUS;
    win->header.normal = zr_style_item_color(zr_rgb(40,40,40));
    win->header.hover = zr_style_item_color(zr_rgb(40,40,40));
    win->header.active = zr_style_item_color(zr_rgb(40,40,40));
    win->header.label_normal = zr_rgb(175,175,175);
    win->header.label_hover = zr_rgb(175,175,175);
    win->header.label_active = zr_rgb(175,175,175);
    win->header.label_padding = zr_vec2(4,4);
    win->header.padding = zr_vec2(4,4);
    win->header.spacing = zr_vec2(0,0);

    /* window header close button */
    button = &style->window.header.close_button;
    zr_zero_struct(*button);
    button->normal          = zr_style_item_color(zr_rgb(40,40,40));
    button->hover           = zr_style_item_color(zr_rgb(40,40,40));
    button->active          = zr_style_item_color(zr_rgb(40,40,40));
    button->border_color    = zr_rgba(0,0,0,0);
    button->text_background = zr_rgb(40,40,40);
    button->text_normal     = zr_rgb(175,175,175);
    button->text_hover      = zr_rgb(175,175,175);
    button->text_active     = zr_rgb(175,175,175);
    button->padding         = zr_vec2(0.0f,0.0f);
    button->touch_padding   = zr_vec2(0.0f,0.0f);
    button->userdata        = zr_handle_ptr(0);
    button->text_alignment  = ZR_TEXT_CENTERED;
    button->border          = 0.0f;
    button->rounding        = 0.0f;
    button->fixed_width     = 0;
    button->fixed_height    = 0;
    button->has_fixed_size  = 0;
    button->draw_begin      = 0;
    button->draw_end        = 0;

    /* window header minimize button */
    button = &style->window.header.minimize_button;
    zr_zero_struct(*button);
    button->normal          = zr_style_item_color(zr_rgb(40,40,40));
    button->hover           = zr_style_item_color(zr_rgb(40,40,40));
    button->active          = zr_style_item_color(zr_rgb(40,40,40));
    button->border_color    = zr_rgba(0,0,0,0);
    button->text_background = zr_rgb(40,40,40);
    button->text_normal     = zr_rgb(175,175,175);
    button->text_hover      = zr_rgb(175,175,175);
    button->text_active     = zr_rgb(175,175,175);
    button->padding         = zr_vec2(0.0f,0.0f);
    button->touch_padding   = zr_vec2(0.0f,0.0f);
    button->userdata        = zr_handle_ptr(0);
    button->text_alignment  = ZR_TEXT_CENTERED;
    button->border          = 0.0f;
    button->rounding        = 0.0f;
    button->fixed_width     = 0;
    button->fixed_height    = 0;
    button->has_fixed_size  = 0;
    button->draw_begin      = 0;
    button->draw_end        = 0;

    /* window */
    win->background = zr_rgb(45,45,45);
    win->fixed_background = zr_style_item_color(zr_rgb(45,45,45));
    win->border_color = zr_rgb(65,65,65);
    win->scaler = zr_style_item_color(zr_rgb(175,175,175));
    win->footer_padding = zr_vec2(4,4);
    win->border = 1.0f;
    win->rounding = 0.0f;
    win->has_fixed_size = 0;
    win->fixed_width = 0;
    win->fixed_height = 0;
    win->scaler_size = zr_vec2(16,16);
    win->padding = zr_vec2(8,8);
    win->spacing = zr_vec2(4,4);
    win->scrollbar_size = zr_vec2(10,10);
    win->min_size = zr_vec2(64,64);
}

void
zr_style_set_font(struct zr_context *ctx, const struct zr_user_font *font)
{
    struct zr_style *style;
    ZR_ASSERT(ctx);
    if (!ctx) return;
    style = &ctx->style;
    style->font = *font;
}

/* ===============================================================
 *
 *                          POOL
 *
 * ===============================================================*/
struct zr_table {
    unsigned int seq;
    zr_hash keys[ZR_VALUE_PAGE_CAPACITY];
    zr_uint values[ZR_VALUE_PAGE_CAPACITY];
    struct zr_table *next, *prev;
};

union zr_page_data {
    struct zr_table tbl;
    struct zr_window win;
};

struct zr_window_page {
    unsigned size;
    struct zr_window_page *next;
    union zr_page_data win[1];
};

struct zr_pool {
    struct zr_allocator alloc;
    enum zr_allocation_type type;
    unsigned int page_count;
    struct zr_window_page *pages;
    unsigned capacity;
    zr_size size;
    zr_size cap;
};

static void
zr_pool_init(struct zr_pool *pool, struct zr_allocator *alloc,
            unsigned int capacity)
{
    zr_zero(pool, sizeof(*pool));
    pool->alloc = *alloc;
    pool->capacity = capacity;
    pool->pages = 0;
    pool->type = ZR_BUFFER_DYNAMIC;
}

static void
zr_pool_free(struct zr_pool *pool)
{
    struct zr_window_page *next;
    struct zr_window_page *iter = pool->pages;
    if (!pool) return;
    if (pool->type == ZR_BUFFER_FIXED) return;
    while (iter) {
        next = iter->next;
        pool->alloc.free(pool->alloc.userdata, iter);
        iter = next;
    }
    pool->alloc.free(pool->alloc.userdata, pool);
}

static void
zr_pool_init_fixed(struct zr_pool *pool, void *memory, zr_size size)
{
    zr_zero(pool, sizeof(*pool));
    /* make sure pages have correct granularity to at least fit one page into memory */
    if (size < sizeof(struct zr_window_page) + ZR_POOL_DEFAULT_CAPACITY * sizeof(struct zr_window))
        pool->capacity = (unsigned)(size - sizeof(struct zr_window_page)) / sizeof(struct zr_window);
    else pool->capacity = ZR_POOL_DEFAULT_CAPACITY;
    pool->pages = (struct zr_window_page*)memory;
    pool->type = ZR_BUFFER_FIXED;
    pool->size = size;
}

static void*
zr_pool_alloc(struct zr_pool *pool)
{
    if (!pool->pages || pool->pages->size >= pool->capacity) {
        /* allocate new page */
        struct zr_window_page *page;
        if (pool->type == ZR_BUFFER_FIXED) {
            if (!pool->pages) {
                ZR_ASSERT(pool->pages);
                return 0;
            }
            ZR_ASSERT(pool->pages->size < pool->capacity);
            return 0;
        } else {
            zr_size size = sizeof(struct zr_window_page);
            size += ZR_POOL_DEFAULT_CAPACITY * sizeof(union zr_page_data);
            page = (struct zr_window_page*)pool->alloc.alloc(pool->alloc.userdata, size);
            page->size = 0;
            page->next = pool->pages;
            pool->pages = page;
        }
    }
    return &pool->pages->win[pool->pages->size++];
}

/* ===============================================================
 *
 *                          CONTEXT
 *
 * ===============================================================*/
static void zr_remove_window(struct zr_context*, struct zr_window*);
static void* zr_create_window(struct zr_context *ctx);
static void zr_free_window(struct zr_context *ctx, struct zr_window *win);
static void zr_free_table(struct zr_context *ctx, struct zr_table *tbl);
static void zr_remove_table(struct zr_window *win, struct zr_table *tbl);

static void
zr_setup(struct zr_context *ctx, const struct zr_user_font *font)
{
    ZR_ASSERT(ctx);
    ZR_ASSERT(font);
    if (!ctx || !font) return;
    zr_zero_struct(*ctx);
    zr_style_default(ctx);
    ctx->style.font = *font;
#if ZR_COMPILE_WITH_VERTEX_BUFFER
    zr_canvas_init(&ctx->canvas);
#endif
}

int
zr_init_fixed(struct zr_context *ctx, void *memory, zr_size size,
    const struct zr_user_font *font)
{
    ZR_ASSERT(memory);
    if (!memory) return 0;
    zr_setup(ctx, font);
    zr_buffer_init_fixed(&ctx->memory, memory, size);
    ctx->pool = 0;
    return 1;
}

int
zr_init_custom(struct zr_context *ctx, struct zr_buffer *cmds,
    struct zr_buffer *pool, const struct zr_user_font *font)
{
    ZR_ASSERT(cmds);
    ZR_ASSERT(pool);
    if (!cmds || !pool) return 0;
    zr_setup(ctx, font);
    ctx->memory = *cmds;
    if (pool->type == ZR_BUFFER_FIXED) {
        /* take memory from buffer and alloc fixed pool */
        void *memory = pool->memory.ptr;
        zr_size size = pool->memory.size;
        ctx->pool = memory;
        ZR_ASSERT(size > sizeof(struct zr_pool));
        size -= sizeof(struct zr_pool);
        zr_pool_init_fixed((struct zr_pool*)ctx->pool,
            (void*)((zr_byte*)ctx->pool+sizeof(struct zr_pool)), size);
    } else {
        /* create dynamic pool from buffer allocator */
        struct zr_allocator *alloc = &pool->pool;
        ctx->pool = alloc->alloc(alloc->userdata, sizeof(struct zr_pool));
        zr_pool_init((struct zr_pool*)ctx->pool, alloc, ZR_POOL_DEFAULT_CAPACITY);
    }
    return 1;
}

int
zr_init(struct zr_context *ctx, struct zr_allocator *alloc,
    const struct zr_user_font *font)
{
    ZR_ASSERT(alloc);
    if (!alloc) return 0;
    zr_setup(ctx, font);
    zr_buffer_init(&ctx->memory, alloc, ZR_DEFAULT_COMMAND_BUFFER_SIZE);
    ctx->pool = alloc->alloc(alloc->userdata, sizeof(struct zr_pool));
    zr_pool_init((struct zr_pool*)ctx->pool, alloc, ZR_POOL_DEFAULT_CAPACITY);
    return 1;
}

#if ZR_COMPILE_WITH_COMMAND_USERDATA
void
zr_set_user_data(struct zr_context *ctx, zr_handle handle)
{
    if (!ctx) return;
    ctx->userdata = handle;
    if (ctx->current)
        ctx->current->buffer.userdata = handle;
}
#endif

void
zr_free(struct zr_context *ctx)
{
    ZR_ASSERT(ctx);
    if (!ctx) return;
    zr_buffer_free(&ctx->memory);
    if (ctx->pool) zr_pool_free((struct zr_pool*)ctx->pool);

    zr_zero(&ctx->input, sizeof(ctx->input));
    zr_zero(&ctx->style, sizeof(ctx->style));
    zr_zero(&ctx->memory, sizeof(ctx->memory));

    ctx->seq = 0;
    ctx->pool = 0;
    ctx->build = 0;
    ctx->begin = 0;
    ctx->end = 0;
    ctx->active = 0;
    ctx->current = 0;
    ctx->freelist = 0;
    ctx->count = 0;
}

void
zr_clear(struct zr_context *ctx)
{
    struct zr_window *iter;
    struct zr_window *next;
    ZR_ASSERT(ctx);

    if (!ctx) return;
    if (ctx->pool)
        zr_buffer_clear(&ctx->memory);
    else zr_buffer_reset(&ctx->memory, ZR_BUFFER_FRONT);

    ctx->build = 0;
    ctx->memory.calls = 0;
#if ZR_COMPILE_WITH_VERTEX_BUFFER
    zr_canvas_clear(&ctx->canvas);
#endif

    /* garbage collector */
    iter = ctx->begin;
    while (iter) {
        /* make sure minimized windows do not get removed */
        if (iter->flags & ZR_WINDOW_MINIMIZED) {
            iter = iter->next;
            continue;
        }

        /* free unused popup windows */
        if (iter->popup.win && iter->popup.win->seq != ctx->seq) {
            zr_free_window(ctx, iter->popup.win);
            iter->popup.win = 0;
        }

        {struct zr_table *n, *it = iter->tables;
        while (it) {
            /* remove unused window state tables */
            n = it->next;
            if (it->seq != ctx->seq) {
                zr_remove_table(iter, it);
                zr_zero(it, sizeof(union zr_page_data));
                zr_free_table(ctx, it);
                if (it == iter->tables)
                    iter->tables = n;
            }
            it = n;
        }}

        /* window itself is not used anymore so free */
        if (iter->seq != ctx->seq || iter->flags & ZR_WINDOW_HIDDEN) {
            next = iter->next;
            zr_remove_window(ctx, iter);
            zr_free_window(ctx, iter);
            iter = next;
        } else iter = iter->next;
    }
    ctx->seq++;
}

/* ----------------------------------------------------------------
 *
 *                          BUFFERING
 *
 * ---------------------------------------------------------------*/
static void
zr_start(struct zr_context *ctx, struct zr_window *win)
{
    ZR_ASSERT(ctx);
    ZR_ASSERT(win);
    if (!ctx || !win) return;
    win->buffer.begin = ctx->memory.allocated;
    win->buffer.end = win->buffer.begin;
    win->buffer.last = win->buffer.begin;
    win->buffer.clip = zr_null_rect;
}

static void
zr_start_popup(struct zr_context *ctx, struct zr_window *win)
{
    struct zr_popup_buffer *buf;
    struct zr_panel *iter;
    ZR_ASSERT(ctx);
    ZR_ASSERT(win);
    if (!ctx || !win) return;

    /* make sure to use the correct popup buffer*/
    iter = win->layout;
    while (iter->parent)
        iter = iter->parent;

    /* save buffer fill state for popup */
    buf = &iter->popup_buffer;
    buf->begin = win->buffer.end;
    buf->end = win->buffer.end;
    buf->parent = win->buffer.last;
    buf->last = buf->begin;
    buf->active = zr_true;
}

static void
zr_finish_popup(struct zr_context *ctx, struct zr_window *win)
{
    struct zr_popup_buffer *buf;
    struct zr_panel *iter;
    ZR_ASSERT(ctx);
    ZR_ASSERT(win);
    if (!ctx || !win) return;

    /* make sure to use the correct popup buffer*/
    iter = win->layout;
    while (iter->parent)
        iter = iter->parent;

    buf = &iter->popup_buffer;
    buf->last = win->buffer.last;
    buf->end = win->buffer.end;
}

static void
zr_finish(struct zr_context *ctx, struct zr_window *win)
{
    struct zr_popup_buffer *buf;
    struct zr_command *parent_last;
    struct zr_command *sublast;
    struct zr_command *last;
    void *memory;

    ZR_ASSERT(ctx);
    ZR_ASSERT(win);
    if (!ctx || !win) return;
    win->buffer.end = ctx->memory.allocated;
    if (!win->layout->popup_buffer.active) return;

    /* frome here this case is for popup windows */
    buf = &win->layout->popup_buffer;
    memory = ctx->memory.memory.ptr;

    /* redirect the sub-window buffer to the end of the window command buffer */
    parent_last = zr_ptr_add(struct zr_command, memory, buf->parent);
    sublast = zr_ptr_add(struct zr_command, memory, buf->last);
    last = zr_ptr_add(struct zr_command, memory, win->buffer.last);

    parent_last->next = buf->end;
    sublast->next = last->next;
    last->next = buf->begin;
    win->buffer.last = buf->last;
    win->buffer.end = buf->end;
    buf->active = zr_false;
}

static void
zr_build(struct zr_context *ctx)
{
    struct zr_window *iter;
    struct zr_window *next;
    struct zr_command *cmd;
    zr_byte *buffer;

    iter = ctx->begin;
    buffer = (zr_byte*)ctx->memory.memory.ptr;
    while (iter != 0) {
        next = iter->next;
        if (iter->buffer.last == iter->buffer.begin || (iter->flags & ZR_WINDOW_HIDDEN)) {
            iter = next;
            continue;
        }
        cmd = zr_ptr_add(struct zr_command, buffer, iter->buffer.last);
        while (next && ((next->buffer.last == next->buffer.begin) ||
            (next->flags & ZR_WINDOW_HIDDEN)))
            next = next->next; /* skip empty command buffers */

        if (next) {
            cmd->next = next->buffer.begin;
        } else cmd->next = ctx->memory.allocated;
        iter = next;
    }
}

const struct zr_command*
zr__begin(struct zr_context *ctx)
{
    struct zr_window *iter;
    zr_byte *buffer;
    ZR_ASSERT(ctx);
    if (!ctx) return 0;
    if (!ctx->count) return 0;

    /* build one command list out of all windows */
    buffer = (zr_byte*)ctx->memory.memory.ptr;
    if (!ctx->build) {
        zr_build(ctx);
        ctx->build = zr_true;
    }

    iter = ctx->begin;
    while (iter && ((iter->buffer.begin == iter->buffer.end) || (iter->flags & ZR_WINDOW_HIDDEN)))
        iter = iter->next;
    if (!iter) return 0;
    return zr_ptr_add_const(struct zr_command, buffer, iter->buffer.begin);
}

const struct zr_command*
zr__next(struct zr_context *ctx, const struct zr_command *cmd)
{
    zr_byte *buffer;
    const struct zr_command *next;
    ZR_ASSERT(ctx);
    if (!ctx || !cmd || !ctx->count) return 0;
    if (cmd->next >= ctx->memory.allocated) return 0;
    buffer = (zr_byte*)ctx->memory.memory.ptr;
    next = zr_ptr_add_const(struct zr_command, buffer, cmd->next);
    return next;
}

/* ----------------------------------------------------------------
 *
 *                          TABLES
 *
 * ---------------------------------------------------------------*/
static struct zr_table*
zr_create_table(struct zr_context *ctx)
{
    void *tbl = (void*)zr_create_window(ctx);
    zr_zero(tbl, sizeof(struct zr_table));
    return (struct zr_table*)tbl;
}

static void
zr_free_table(struct zr_context *ctx, struct zr_table *tbl)
{zr_free_window(ctx, (struct zr_window*)tbl);}

static void
zr_push_table(struct zr_window *win, struct zr_table *tbl)
{
    if (!win->tables) {
        win->tables = tbl;
        tbl->next = 0;
        tbl->prev = 0;
        win->table_count = 1;
        win->table_size = 1;
        return;
    }
    win->tables->prev = tbl;
    tbl->next = win->tables;
    tbl->prev = 0;
    win->tables = tbl;
    win->table_count++;
    win->table_size = 0;
}

static void
zr_remove_table(struct zr_window *win, struct zr_table *tbl)
{
    if (win->tables == tbl)
        win->tables = tbl->next;
    if (tbl->next)
        tbl->next->prev = tbl->prev;
    if (tbl->prev)
        tbl->prev->next = tbl->next;
    tbl->next = 0;
    tbl->prev = 0;
}

static zr_uint*
zr_add_value(struct zr_context *ctx, struct zr_window *win,
            zr_hash name, zr_uint value)
{
    if (!win->tables || win->table_size == ZR_VALUE_PAGE_CAPACITY) {
        struct zr_table *tbl = zr_create_table(ctx);
        zr_push_table(win, tbl);
    }
    win->tables->seq = win->seq;
    win->tables->keys[win->table_size] = name;
    win->tables->values[win->table_size] = value;
    return &win->tables->values[win->table_size++];
}

static zr_uint*
zr_find_value(struct zr_window *win, zr_hash name)
{
    unsigned short size = win->table_size;
    struct zr_table *iter = win->tables;
    while (iter) {
        unsigned short i = 0;
        for (i = 0; i < size; ++i) {
            if (iter->keys[i] == name) {
                iter->seq = win->seq;
                return &iter->values[i];
            }
        }
        size = ZR_VALUE_PAGE_CAPACITY;
        iter = iter->next;
    }
    return 0;
}

/* ----------------------------------------------------------------
 *
 *                          WINDOW
 *
 * ---------------------------------------------------------------*/
static int zr_panel_begin(struct zr_context *ctx, const char *title);
static void zr_panel_end(struct zr_context *ctx);

static void*
zr_create_window(struct zr_context *ctx)
{
    struct zr_window *win = 0;
    if (ctx->freelist) {
        /* unlink window from free list */
        win = ctx->freelist;
        ctx->freelist = win->next;
    } else if (ctx->pool) {
        /* allocate window from memory pool */
        win = (struct zr_window*) zr_pool_alloc((struct zr_pool*)ctx->pool);
        ZR_ASSERT(win);
        if (!win) return 0;
    } else {
        /* allocate new window from the back of the fixed size memory buffer */
        static const zr_size size = sizeof(union zr_page_data);
        static const zr_size align = ZR_ALIGNOF(union zr_page_data);
        win = (struct zr_window*)zr_buffer_alloc(&ctx->memory, ZR_BUFFER_BACK, size, align);
        ZR_ASSERT(win);
        if (!win) return 0;
    }
    zr_zero(win, sizeof(*win));
    win->next = 0;
    win->prev = 0;
    win->seq = ctx->seq;
    return win;
}

static void
zr_free_window(struct zr_context *ctx, struct zr_window *win)
{
    /* unlink windows from list */
    struct zr_table *n, *it = win->tables;
    if (win->popup.win) {
        zr_free_window(ctx, win->popup.win);
        win->popup.win = 0;
    }

    win->next = 0;
    win->prev = 0;

    while (it) {
        /*free window state tables */
        n = it->next;
        if (it->seq != ctx->seq) {
            zr_remove_table(win, it);
            zr_free_table(ctx, it);
            if (it == win->tables)
                win->tables = n;
        }
        it = n;
    }

    /* link windows into freelist */
    if (!ctx->freelist) {
        ctx->freelist = win;
    } else {
        win->next = ctx->freelist;
        ctx->freelist = win;
    }
}

static struct zr_window*
zr_find_window(struct zr_context *ctx, zr_hash hash)
{
    struct zr_window *iter;
    iter = ctx->begin;
    while (iter) {
        ZR_ASSERT(iter != iter->next);
        if (iter->name == hash)
            return iter;
        iter = iter->next;
    }
    return 0;
}

static void
zr_insert_window(struct zr_context *ctx, struct zr_window *win)
{
    const struct zr_window *iter;
    struct zr_window *end;
    ZR_ASSERT(ctx);
    ZR_ASSERT(win);
    if (!win || !ctx) return;

    iter = ctx->begin;
    while (iter) {
        ZR_ASSERT(iter != iter->next);
        ZR_ASSERT(iter != win);
        if (iter == win) return;
        iter = iter->next;
    }

    if (!ctx->begin) {
        win->next = 0;
        win->prev = 0;
        ctx->begin = win;
        ctx->end = win;
        ctx->count = 1;
        return;
    }

    end = ctx->end;
    end->flags |= ZR_WINDOW_ROM;
    end->next = win;
    win->prev = ctx->end;
    win->next = 0;
    ctx->end = win;
    ctx->count++;

    ctx->active = ctx->end;
    ctx->end->flags &= ~(zr_flags)ZR_WINDOW_ROM;
}

static void
zr_remove_window(struct zr_context *ctx, struct zr_window *win)
{
    if (win == ctx->begin || win == ctx->end) {
        if (win == ctx->begin) {
            ctx->begin = win->next;
            if (win->next)
                win->next->prev = 0;
        }
        if (win == ctx->end) {
            ctx->end = win->prev;
            if (win->prev)
                win->prev->next = 0;
        }
    } else {
        if (win->next)
            win->next->prev = win->prev;
        if (win->prev)
            win->prev->next = win->next;
    }
    if (win == ctx->active || !ctx->active) {
        ctx->active = ctx->end;
        if (ctx->end)
            ctx->end->flags &= ~(zr_flags)ZR_WINDOW_ROM;
    }
    win->next = 0;
    win->prev = 0;
    ctx->count--;
}

int
zr_begin(struct zr_context *ctx, struct zr_panel *layout, const char *title,
    struct zr_rect bounds, zr_flags flags)
{
    struct zr_window *win;
    struct zr_style *style;
    zr_hash title_hash;
    int title_len;
    int ret = 0;

    ZR_ASSERT(ctx);
    ZR_ASSERT(!ctx->current && "if this triggers you missed a `zr_end` call");
    if (!ctx || ctx->current || !title)
        return 0;

    /* find or create window */
    style = &ctx->style;
    title_len = (int)zr_strsiz(title);
    title_hash = zr_murmur_hash(title, (int)title_len, ZR_WINDOW_TITLE);
    win = zr_find_window(ctx, title_hash);
    if (!win) {
        win = (struct zr_window*)zr_create_window(ctx);
        zr_insert_window(ctx, win);
        zr_command_buffer_init(&win->buffer, &ctx->memory, ZR_CLIPPING_ON);
        ZR_ASSERT(win);
        if (!win) return 0;

        win->flags = flags;
        win->bounds = bounds;
        win->name = title_hash;
        win->popup.win = 0;

    } else {
        /* update public window flags */
        win->flags &= ~(zr_flags)(ZR_WINDOW_PRIVATE-1);
        win->flags |= flags;
        win->seq++;
    }
    if (win->flags & ZR_WINDOW_HIDDEN) {
        ctx->current = win;
        return 0;
    }

    /* overlapping window */
    if (!(win->flags & ZR_WINDOW_SUB) && !(win->flags & ZR_WINDOW_HIDDEN))
    {
        int inpanel, ishovered;
        const struct zr_window *iter = win;

        /* This is so terrible but neccessary for minimized windows. The difference
         * lies in the size of the window. But it is not possible to get the size
         * without cheating because you do not have the information at this point.
         * Even worse this is wrong since windows could have different window heights.
         * I leave it in for now since I otherwise loose my mind. */
        float h = ctx->style.font.height + 2 * style->window.header.padding.y;

        /* activate window if hovered and no other window is overlapping this window */
        zr_start(ctx, win);
        inpanel = zr_input_mouse_clicked(&ctx->input, ZR_BUTTON_LEFT, win->bounds);
        ishovered = zr_input_is_mouse_hovering_rect(&ctx->input, win->bounds);
        if ((win != ctx->active) && ishovered) {
            iter = win->next;
            while (iter) {
                if (!(iter->flags & ZR_WINDOW_MINIMIZED)) {
                    if (ZR_INTERSECT(win->bounds.x, win->bounds.y, win->bounds.w, win->bounds.h,
                        iter->bounds.x, iter->bounds.y, iter->bounds.w, iter->bounds.h) &&
                        !(iter->flags & ZR_WINDOW_HIDDEN))
                        break;
                } else {
                    if (ZR_INTERSECT(win->bounds.x, win->bounds.y, win->bounds.w, win->bounds.h,
                        iter->bounds.x, iter->bounds.y, iter->bounds.w, h) &&
                        !(iter->flags & ZR_WINDOW_HIDDEN))
                        break;
                }
                if (iter->popup.win && iter->popup.active && !(iter->flags & ZR_WINDOW_HIDDEN) &&
                    ZR_INTERSECT(win->bounds.x, win->bounds.y, win->bounds.w, win->bounds.h,
                    iter->popup.win->bounds.x, iter->popup.win->bounds.y,
                    iter->popup.win->bounds.w, iter->popup.win->bounds.h))
                    break;
                iter = iter->next;
            }
        }

        /* activate window if clicked */
        if (iter && inpanel && (win != ctx->end)) {
            iter = win->next;
            while (iter) {
                /* try to find a panel with higher priorty in the same position */
                if (!(iter->flags & ZR_WINDOW_MINIMIZED)) {
                    if (ZR_INBOX(ctx->input.mouse.prev.x, ctx->input.mouse.prev.y, iter->bounds.x,
                        iter->bounds.y, iter->bounds.w, iter->bounds.h) &&
                        !(iter->flags & ZR_WINDOW_HIDDEN))
                        break;
                } else {
                    if (ZR_INBOX(ctx->input.mouse.prev.x, ctx->input.mouse.prev.y, iter->bounds.x,
                        iter->bounds.y, iter->bounds.w, h) &&
                        !(iter->flags & ZR_WINDOW_HIDDEN))
                        break;
                }
                if (iter->popup.win && iter->popup.active && !(iter->flags & ZR_WINDOW_HIDDEN) &&
                    ZR_INTERSECT(win->bounds.x, win->bounds.y, win->bounds.w, win->bounds.h,
                    iter->popup.win->bounds.x, iter->popup.win->bounds.y,
                    iter->popup.win->bounds.w, iter->popup.win->bounds.h))
                    break;
                iter = iter->next;
            }
        }

        if (!iter && ctx->end != win) {
            /* current window is active in that position so transfer to top
             * at the highest priority in stack */
            zr_remove_window(ctx, win);
            zr_insert_window(ctx, win);

            win->flags &= ~(zr_flags)ZR_WINDOW_ROM;
            ctx->active = win;
        }
        if (ctx->end != win)
            win->flags |= ZR_WINDOW_ROM;
    }

    win->layout = layout;
    ctx->current = win;
    ret = zr_panel_begin(ctx, title);
    layout->offset = &win->scrollbar;
    return ret;
}

void
zr_end(struct zr_context *ctx)
{
    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current) return;
    if (ctx->current->flags & ZR_WINDOW_HIDDEN) {
        ctx->current = 0;
        return;
    }
    zr_panel_end(ctx);
    ctx->current = 0;
    ctx->last_widget_state = 0;
}

struct zr_rect
zr_window_get_bounds(const struct zr_context *ctx)
{
    ZR_ASSERT(ctx); ZR_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return zr_rect(0,0,0,0);
    return ctx->current->bounds;
}
struct zr_vec2
zr_window_get_position(const struct zr_context *ctx)
{
    ZR_ASSERT(ctx); ZR_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return zr_vec2(0,0);
    return zr_vec2(ctx->current->bounds.x, ctx->current->bounds.y);
}

struct zr_vec2
zr_window_get_size(const struct zr_context *ctx)
{
    ZR_ASSERT(ctx); ZR_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return zr_vec2(0,0);
    return zr_vec2(ctx->current->bounds.w, ctx->current->bounds.h);
}

float
zr_window_get_width(const struct zr_context *ctx)
{
    ZR_ASSERT(ctx); ZR_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return 0;
    return ctx->current->bounds.w;
}

float
zr_window_get_height(const struct zr_context *ctx)
{
    ZR_ASSERT(ctx); ZR_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return 0;
    return ctx->current->bounds.h;
}

struct zr_rect
zr_window_get_content_region(struct zr_context *ctx)
{
    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return zr_rect(0,0,0,0);
    return ctx->current->layout->clip;
}

struct zr_vec2
zr_window_get_content_region_min(struct zr_context *ctx)
{
    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current) return zr_vec2(0,0);
    return zr_vec2(ctx->current->layout->clip.x, ctx->current->layout->clip.y);
}

struct zr_vec2
zr_window_get_content_region_max(struct zr_context *ctx)
{
    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current) return zr_vec2(0,0);
    return zr_vec2(ctx->current->layout->clip.x + ctx->current->layout->clip.w,
        ctx->current->layout->clip.y + ctx->current->layout->clip.h);
}

struct zr_vec2
zr_window_get_content_region_size(struct zr_context *ctx)
{
    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current) return zr_vec2(0,0);
    return zr_vec2(ctx->current->layout->clip.w, ctx->current->layout->clip.h);
}

struct zr_command_buffer*
zr_window_get_canvas(struct zr_context *ctx)
{
    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current) return 0;
    return &ctx->current->buffer;
}

struct zr_panel*
zr_window_get_panel(struct zr_context *ctx)
{
    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return 0;
    return ctx->current->layout;
}

int
zr_window_has_focus(const struct zr_context *ctx)
{
    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current) return 0;
    return ctx->current == ctx->active;
}

int
zr_window_is_hovered(struct zr_context *ctx)
{
    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return 0;
    return zr_input_is_mouse_hovering_rect(&ctx->input, ctx->current->bounds);
}

int
zr_window_is_any_hovered(struct zr_context *ctx)
{
    struct zr_window *iter;
    ZR_ASSERT(ctx);
    if (!ctx) return 0;
    iter = ctx->begin;
    while (iter) {
        if (zr_input_is_mouse_hovering_rect(&ctx->input, iter->bounds))
            return 1;
        iter = iter->next;
    }
    return 0;
}

int
zr_window_is_collapsed(struct zr_context *ctx, const char *name)
{
    int title_len;
    zr_hash title_hash;
    struct zr_window *win;
    ZR_ASSERT(ctx);
    if (!ctx) return 0;

    title_len = (int)zr_strsiz(name);
    title_hash = zr_murmur_hash(name, (int)title_len, ZR_WINDOW_TITLE);
    win = zr_find_window(ctx, title_hash);
    if (!win) return 0;
    return win->flags & ZR_WINDOW_MINIMIZED;
}

int
zr_window_is_closed(struct zr_context *ctx, const char *name)
{
    int title_len;
    zr_hash title_hash;
    struct zr_window *win;
    ZR_ASSERT(ctx);
    if (!ctx) return 1;

    title_len = (int)zr_strsiz(name);
    title_hash = zr_murmur_hash(name, (int)title_len, ZR_WINDOW_TITLE);
    win = zr_find_window(ctx, title_hash);
    if (!win) return 1;
    return (win->flags & ZR_WINDOW_HIDDEN);
}

int
zr_window_is_active(struct zr_context *ctx, const char *name)
{
    int title_len;
    zr_hash title_hash;
    struct zr_window *win;
    ZR_ASSERT(ctx);
    if (!ctx) return 0;

    title_len = (int)zr_strsiz(name);
    title_hash = zr_murmur_hash(name, (int)title_len, ZR_WINDOW_TITLE);
    win = zr_find_window(ctx, title_hash);
    if (!win) return 0;
    return win == ctx->active;
}

struct zr_window*
zr_window_find(struct zr_context *ctx, const char *name)
{
    int title_len;
    zr_hash title_hash;
    title_len = (int)zr_strsiz(name);
    title_hash = zr_murmur_hash(name, (int)title_len, ZR_WINDOW_TITLE);
    return zr_find_window(ctx, title_hash);
}

void
zr_window_close(struct zr_context *ctx, const char *name)
{
    struct zr_window *win;
    ZR_ASSERT(ctx);
    if (!ctx) return;
    win = zr_window_find(ctx, name);
    if (!win) return;
    ZR_ASSERT(ctx->current != win && "You cannot close a current window");
    if (ctx->current == win) return;
    win->flags |= ZR_WINDOW_HIDDEN;
}

void
zr_window_set_bounds(struct zr_context *ctx, struct zr_rect bounds)
{
    ZR_ASSERT(ctx); ZR_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return;
    ctx->current->bounds = bounds;
}

void
zr_window_set_position(struct zr_context *ctx, struct zr_vec2 pos)
{
    ZR_ASSERT(ctx); ZR_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return;
    ctx->current->bounds.x = pos.x;
    ctx->current->bounds.y = pos.y;
}

void
zr_window_set_size(struct zr_context *ctx, struct zr_vec2 size)
{
    ZR_ASSERT(ctx); ZR_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return;
    ctx->current->bounds.w = size.x;
    ctx->current->bounds.h = size.y;
}

void
zr_window_collapse(struct zr_context *ctx, const char *name,
                    enum zr_collapse_states c)
{
    int title_len;
    zr_hash title_hash;
    struct zr_window *win;
    ZR_ASSERT(ctx);
    if (!ctx) return;

    title_len = (int)zr_strsiz(name);
    title_hash = zr_murmur_hash(name, (int)title_len, ZR_WINDOW_TITLE);
    win = zr_find_window(ctx, title_hash);
    if (!win) return;
    if (c == ZR_MINIMIZED)
        win->flags |= ZR_WINDOW_MINIMIZED;
    else win->flags &= ~(zr_flags)ZR_WINDOW_MINIMIZED;
}

void
zr_window_collapse_if(struct zr_context *ctx, const char *name,
    enum zr_collapse_states c, int cond)
{
    ZR_ASSERT(ctx);
    if (!ctx || !cond) return;
    zr_window_collapse(ctx, name, c);
}

void
zr_window_show(struct zr_context *ctx, const char *name, enum zr_show_states s)
{
    int title_len;
    zr_hash title_hash;
    struct zr_window *win;
    ZR_ASSERT(ctx);
    if (!ctx) return;

    title_len = (int)zr_strsiz(name);
    title_hash = zr_murmur_hash(name, (int)title_len, ZR_WINDOW_TITLE);
    win = zr_find_window(ctx, title_hash);
    if (!win) return;
    if (s == ZR_HIDDEN)
        win->flags |= ZR_WINDOW_HIDDEN;
    else win->flags &= ~(zr_flags)ZR_WINDOW_HIDDEN;
}

void
zr_window_show_if(struct zr_context *ctx, const char *name,
    enum zr_show_states s, int cond)
{
    ZR_ASSERT(ctx);
    if (!ctx || !cond) return;
    zr_window_show(ctx, name, s);
}

void
zr_window_set_focus(struct zr_context *ctx, const char *name)
{
    int title_len;
    zr_hash title_hash;
    struct zr_window *win;
    ZR_ASSERT(ctx);
    if (!ctx) return;

    title_len = (int)zr_strsiz(name);
    title_hash = zr_murmur_hash(name, (int)title_len, ZR_WINDOW_TITLE);
    win = zr_find_window(ctx, title_hash);
    if (win && ctx->end != win) {
        zr_remove_window(ctx, win);
        zr_insert_window(ctx, win);
    }
    ctx->active = win;
}

/*----------------------------------------------------------------
 *
 *                          PANEL
 *
 * --------------------------------------------------------------*/
static int
zr_panel_begin(struct zr_context *ctx, const char *title)
{
    struct zr_input *in;
    struct zr_window *win;
    struct zr_panel *layout;
    struct zr_command_buffer *out;
    const struct zr_style *style;
    const struct zr_user_font *font;

    int header_active = 0;
    struct zr_vec2 scrollbar_size;
    struct zr_vec2 item_spacing;
    struct zr_vec2 window_padding;
    struct zr_vec2 scaler_size;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    style = &ctx->style;
    font = &style->font;
    in = &ctx->input;
    win = ctx->current;
    layout = win->layout;

    /* cache style data */
    scrollbar_size = style->window.scrollbar_size;
    window_padding = style->window.padding;
    item_spacing = style->window.spacing;
    scaler_size = style->window.scaler_size;

    /* check arguments */
    zr_zero(layout, sizeof(*layout));
    if (win->flags & ZR_WINDOW_HIDDEN)
        return 0;

    /* move panel position if requested */
    layout->header_h = font->height + 2 * style->window.header.padding.y;
    layout->header_h += 2 * style->window.header.label_padding.y;
    if ((win->flags & ZR_WINDOW_MOVABLE) && !(win->flags & ZR_WINDOW_ROM)) {
        int incursor;
        struct zr_rect move;
        move.x = win->bounds.x;
        move.y = win->bounds.y;
        move.w = win->bounds.w;
        move.h = layout->header_h;
        incursor = zr_input_is_mouse_prev_hovering_rect(in, move);
        if (zr_input_is_mouse_down(in, ZR_BUTTON_LEFT) && incursor) {
            win->bounds.x = win->bounds.x + in->mouse.delta.x;
            win->bounds.y = win->bounds.y + in->mouse.delta.y;
        }
    }

#if ZR_COMPILE_WITH_COMMAND_USERDATA
    win->buffer.userdata = ctx->userdata;
#endif

    /* setup window layout */
    out = &win->buffer;
    layout->bounds = win->bounds;
    layout->at_x = win->bounds.x;
    layout->at_y = win->bounds.y;
    layout->width = win->bounds.w;
    layout->height = win->bounds.h;
    layout->max_x = 0;
    layout->row.index = 0;
    layout->row.columns = 0;
    layout->row.height = 0;
    layout->row.ratio = 0;
    layout->row.item_width = 0;
    layout->row.tree_depth = 0;
    layout->flags = win->flags;

    /* calculate window header */
    if (win->flags & ZR_WINDOW_MINIMIZED) {
        layout->header_h = 0;
        layout->row.height = 0;
    } else {
        layout->header_h = 0;
        layout->row.height = item_spacing.y;
    }

    /* calculate window footer height */
    if (!(win->flags & ZR_WINDOW_NONBLOCK) &&
        (!(win->flags & ZR_WINDOW_NO_SCROLLBAR) || (win->flags & ZR_WINDOW_SCALABLE)))
        layout->footer_h = scaler_size.y + style->window.footer_padding.y;
    else layout->footer_h = 0;

    /* calculate the window size */
    if (!(win->flags & ZR_WINDOW_NO_SCROLLBAR))
        layout->width = win->bounds.w - scrollbar_size.x;
    layout->height = win->bounds.h - (layout->header_h + 2 * item_spacing.y);
    layout->height -= layout->footer_h;

    /* window header state */
    header_active = (win->flags & (ZR_WINDOW_CLOSABLE|ZR_WINDOW_MINIMIZABLE));
    header_active = header_active || (win->flags & ZR_WINDOW_TITLE);
    header_active = header_active && !(win->flags & ZR_WINDOW_HIDDEN) && title;

    /* window header */
    if (header_active)
    {
        struct zr_rect header;
        struct zr_rect button;
        struct zr_text text;
        const struct zr_style_item *background;

        /* calculate header bounds */
        header.x = layout->bounds.x;
        header.y = layout->bounds.y;
        header.w = layout->bounds.w;

        /* set correct header/first row height */
        layout->header_h = font->height + 2.0f * style->window.header.padding.y;
        layout->header_h += 2.0f * style->window.header.label_padding.y;
        layout->row.height += layout->header_h;
        header.h = layout->header_h + 0.5f;

        /* update window height */
        layout->height = layout->bounds.h - (header.h + 2 * item_spacing.y);
        layout->height -= layout->footer_h;

        /* draw header background */
        if (ctx->active == win) {
            background = &style->window.header.normal;
            text.text = style->window.header.label_active;
        } else if (zr_input_is_mouse_hovering_rect(&ctx->input, header)) {
            background = &style->window.header.hover;
            text.text = style->window.header.label_hover;
        } else {
            background = &style->window.header.normal;
            text.text = style->window.header.label_normal;
        }
        if (background->type == ZR_STYLE_ITEM_IMAGE) {
            text.background = zr_rgba(0,0,0,0);
            zr_draw_image(&win->buffer, header, &background->data.image);
        } else {
            text.background = background->data.color;
            zr_fill_rect(out, zr_rect(layout->bounds.x, layout->bounds.y,
                layout->bounds.w, layout->header_h), 0, background->data.color);
        }

        /* window close button */
        button.y = header.y + style->window.header.padding.y;
        button.h = layout->header_h - 2 * style->window.header.padding.y;
        button.w = button.h;
        if (win->flags & ZR_WINDOW_CLOSABLE) {
            zr_flags ws;
            if (style->window.header.align == ZR_HEADER_RIGHT) {
                button.x = (header.w + header.x) - (button.w + style->window.header.padding.x);
                header.w -= button.w + style->window.header.spacing.x + style->window.header.padding.x;
            } else {
                button.x = header.x;
                header.x += button.w + style->window.header.spacing.x + style->window.header.padding.x;
            }
            if (zr_do_button_symbol(&ws, &win->buffer, button,
                style->window.header.close_symbol, ZR_BUTTON_DEFAULT,
                &style->window.header.close_button, in, &style->font))
                layout->flags |= ZR_WINDOW_HIDDEN;
        }

        /* window minimize button */
        if (win->flags & ZR_WINDOW_MINIMIZABLE) {
            zr_flags ws;
            if (style->window.header.align == ZR_HEADER_RIGHT) {
                button.x = (header.w + header.x) - button.w;
                if (!(win->flags & ZR_WINDOW_CLOSABLE)) {
                    button.x -= style->window.header.padding.x;
                    header.w -= style->window.header.padding.x;
                }
                header.w -= button.w + style->window.header.spacing.x;
            } else {
                button.x = header.x;
                header.x += button.w + style->window.header.spacing.x + style->window.header.padding.x;
            }
            if (zr_do_button_symbol(&ws, &win->buffer, button,
                (layout->flags & ZR_WINDOW_MINIMIZED)?
                style->window.header.maximize_symbol:
                style->window.header.minimize_symbol,
                ZR_BUTTON_DEFAULT, &style->window.header.minimize_button, in, &style->font))
                layout->flags = (layout->flags & ZR_WINDOW_MINIMIZED) ?
                    layout->flags & (zr_flags)~ZR_WINDOW_MINIMIZED:
                    layout->flags | ZR_WINDOW_MINIMIZED;
        }
        {
            /* window header title */
            zr_size text_len = zr_strsiz(title);
            struct zr_rect label = {0,0,0,0};
            zr_size t = font->width(font->userdata, font->height, title, text_len);

            label.x = header.x + style->window.header.padding.x;
            label.x += style->window.header.label_padding.x;
            label.y = header.y + style->window.header.label_padding.y;
            label.h = font->height + 2 * style->window.header.label_padding.y;
            label.w = (float)t + 2 * style->window.header.spacing.x;
            text.padding = zr_vec2(0,0);
            zr_widget_text(out, label,(const char*)title, text_len, &text,
                ZR_TEXT_LEFT, font);
        }
    }

    /* fix header height for transistion between minimized and maximized window state */
    if (win->flags & ZR_WINDOW_MINIMIZED && !(layout->flags & ZR_WINDOW_MINIMIZED))
        layout->row.height += 2 * item_spacing.y + style->window.border;

    if (layout->flags & ZR_WINDOW_MINIMIZED) {
        /* draw window background if minimized */
        layout->row.height = 0;
        zr_fill_rect(out, zr_rect(layout->bounds.x, layout->bounds.y,
            layout->bounds.w, layout->row.height), 0, style->window.background);
    } else if (!(layout->flags & ZR_WINDOW_DYNAMIC)) {
        /* draw fixed window body */
        struct zr_rect body = layout->bounds;
        if (header_active) {
            body.y += layout->header_h - 0.5f;
            body.h -= layout->header_h;
        }
        if (style->window.fixed_background.type == ZR_STYLE_ITEM_IMAGE)
            zr_draw_image(out, body, &style->window.fixed_background.data.image);
        else zr_fill_rect(out, body, 0, style->window.fixed_background.data.color);
    } else {
        /* draw dynamic window body */
        zr_fill_rect(out, zr_rect(layout->bounds.x, layout->bounds.y,
            layout->bounds.w, layout->row.height + window_padding.y), 0,
            style->window.background);
    }

    /* draw top window border line */
    if (layout->flags & ZR_WINDOW_BORDER) {
        zr_stroke_line(out, layout->bounds.x, layout->bounds.y,
            layout->bounds.x + layout->bounds.w, layout->bounds.y, style->window.border,
            style->window.border_color);
    }
    {
        /* calculate and set the window clipping rectangle*/
        struct zr_rect clip;
        if (!(win->flags & ZR_WINDOW_DYNAMIC)) {
            layout->clip.x = win->bounds.x + window_padding.x;
            layout->clip.w = layout->width - 2 * window_padding.x;
        } else {
            layout->clip.x = win->bounds.x;
            layout->clip.w = layout->width;
        }

        layout->clip.h = win->bounds.h - (layout->footer_h + layout->header_h);
        layout->clip.h -= (2.0f * window_padding.y);
        layout->clip.y = win->bounds.y;
        if (win->flags & ZR_WINDOW_BORDER) {
            layout->clip.y += style->window.border;
            layout->clip.h -= 2.0f * style->window.border;
        }

        /* combo box and menu do not have header space */
        if (!(win->flags & ZR_WINDOW_COMBO) && !(win->flags & ZR_WINDOW_MENU))
            layout->clip.y += layout->header_h;

        zr_unify(&clip, &win->buffer.clip, layout->clip.x, layout->clip.y,
            layout->clip.x + layout->clip.w, layout->clip.y + layout->clip.h);
        zr_push_scissor(out, clip);

        win->buffer.clip.x = layout->bounds.x;
        win->buffer.clip.w = layout->width;
        if (!(win->flags & ZR_WINDOW_NO_SCROLLBAR))
            win->buffer.clip.w += scrollbar_size.x;
    }
    return !(layout->flags & ZR_WINDOW_HIDDEN) && !(layout->flags & ZR_WINDOW_MINIMIZED);
}

static void
zr_panel_end(struct zr_context *ctx)
{
    struct zr_input *in;
    struct zr_window *window;
    struct zr_panel *layout;
    const struct zr_style *style;
    struct zr_command_buffer *out;

    struct zr_vec2 scrollbar_size;
    struct zr_vec2 scaler_size;
    struct zr_vec2 item_spacing;
    struct zr_vec2 window_padding;
    struct zr_rect footer = {0,0,0,0};

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    window = ctx->current;
    layout = window->layout;
    style = &ctx->style;
    out = &window->buffer;
    in = (layout->flags & ZR_WINDOW_ROM) ? 0 :&ctx->input;
    if (!(layout->flags & ZR_WINDOW_SUB))
        zr_push_scissor(out, zr_null_rect);

    /* cache configuration data */
    item_spacing = style->window.spacing;
    window_padding = style->window.padding;
    scrollbar_size = style->window.scrollbar_size;
    scaler_size = style->window.scaler_size;

    /* update the current cursor Y-position to point over the last added widget */
    layout->at_y += layout->row.height;

    /* draw footer and fill empty spaces inside a dynamically growing panel */
    if (layout->flags & ZR_WINDOW_DYNAMIC && !(layout->flags & ZR_WINDOW_MINIMIZED)) {
        layout->height = layout->at_y - layout->bounds.y;
        layout->height = ZR_MIN(layout->height, layout->bounds.h);

        if ((layout->offset->x == 0) || (layout->flags & ZR_WINDOW_NO_SCROLLBAR)) {
            /* special case for dynamic windows without horizontal scrollbar
             * or hidden scrollbars */
            footer.x = window->bounds.x;
            footer.y = window->bounds.y + layout->height + item_spacing.y;
            footer.w = window->bounds.w + scrollbar_size.x;
            layout->footer_h = 0;
            footer.h = 0;

            if ((layout->offset->x == 0) && !(layout->flags & ZR_WINDOW_NO_SCROLLBAR)) {
                /* special case for windows like combobox, menu require draw call
                 * to fill the empty scrollbar background */
                struct zr_rect bounds;
                bounds.x = layout->bounds.x + layout->width;
                bounds.y = layout->clip.y;
                bounds.w = scrollbar_size.x;
                bounds.h = layout->height;
                zr_fill_rect(out, bounds, 0, style->window.background);
            }
        } else {
            /* dynamic window with visible scrollbars and therefore bigger footer */
            footer.x = window->bounds.x;
            footer.w = window->bounds.w + scrollbar_size.x;
            footer.h = layout->footer_h;
            if ((layout->flags & ZR_WINDOW_COMBO) || (layout->flags & ZR_WINDOW_MENU) ||
                (layout->flags & ZR_WINDOW_CONTEXTUAL))
                footer.y = window->bounds.y + layout->height;
            else footer.y = window->bounds.y + layout->height + layout->footer_h;
            zr_fill_rect(out, footer, 0, style->window.background);

            if (!(layout->flags & ZR_WINDOW_COMBO) && !(layout->flags & ZR_WINDOW_MENU)) {
                /* fill empty scrollbar space */
                struct zr_rect bounds;
                bounds.x = layout->bounds.x;
                bounds.y = window->bounds.y + layout->height;
                bounds.w = layout->bounds.w;
                bounds.h = layout->row.height;
                zr_fill_rect(out, bounds, 0, style->window.background);
            }
        }
    }

    /* scrollbars */
    if (!(layout->flags & ZR_WINDOW_NO_SCROLLBAR) && !(layout->flags & ZR_WINDOW_MINIMIZED))
    {
        struct zr_rect bounds;
        int scroll_has_scrolling;
        float scroll_target;
        float scroll_offset;
        float scroll_step;
        float scroll_inc;
        {
            /* vertical scollbar */
            zr_flags state;
            bounds.x = layout->bounds.x + layout->width;
            bounds.y = layout->clip.y;
            bounds.w = scrollbar_size.y;
            bounds.h = layout->clip.h;
            if (layout->flags & ZR_WINDOW_BORDER) bounds.h -= 1;

            scroll_offset = layout->offset->y;
            scroll_step = layout->clip.h * 0.10f;
            scroll_inc = layout->clip.h * 0.01f;
            scroll_target = (float)(int)(layout->at_y - layout->clip.y);
            scroll_has_scrolling = (window == ctx->active);
            scroll_offset = zr_do_scrollbarv(&state, out, bounds, scroll_has_scrolling,
                    scroll_offset, scroll_target, scroll_step, scroll_inc,
                    &ctx->style.scrollv, in, &style->font);
            layout->offset->y = (unsigned short)scroll_offset;
        }
        {
            /* horizontal scrollbar */
            zr_flags state;
            bounds.x = layout->bounds.x + window_padding.x;
            if (layout->flags & ZR_WINDOW_SUB) {
                bounds.h = scrollbar_size.x;
                bounds.y = (layout->flags & ZR_WINDOW_BORDER) ?
                            layout->bounds.y + 1 : layout->bounds.y;
                bounds.y += layout->header_h + layout->menu.h + layout->height;
                bounds.w = layout->clip.w;
            } else if (layout->flags & ZR_WINDOW_DYNAMIC) {
                bounds.h = ZR_MIN(scrollbar_size.x, layout->footer_h);
                bounds.w = layout->bounds.w;
                bounds.y = footer.y;
            } else {
                bounds.h = ZR_MIN(scrollbar_size.x, layout->footer_h);
                bounds.y = layout->bounds.y + window->bounds.h;
                bounds.y -= ZR_MAX(layout->footer_h, scrollbar_size.x);
                bounds.w = layout->width - 2 * window_padding.x;
            }
            scroll_offset = layout->offset->x;
            scroll_target = (float)(int)(layout->max_x - bounds.x);
            scroll_step = layout->max_x * 0.05f;
            scroll_inc = layout->max_x * 0.005f;
            scroll_has_scrolling = zr_false;
            scroll_offset = zr_do_scrollbarh(&state, out, bounds, scroll_has_scrolling,
                    scroll_offset, scroll_target, scroll_step, scroll_inc,
                    &ctx->style.scrollh, in, &style->font);
            layout->offset->x = (unsigned short)scroll_offset;
        }
    }

    /* scaler */
    if ((layout->flags & ZR_WINDOW_SCALABLE) && in && !(layout->flags & ZR_WINDOW_MINIMIZED)) {
        /* caluclate scaler bounds */
        const struct zr_style_item *scaler;
        float scaler_w = ZR_MAX(0, scaler_size.x - window_padding.x);
        float scaler_h = ZR_MAX(0, scaler_size.y - window_padding.y);
        float scaler_x = (layout->bounds.x + layout->bounds.w) - (window_padding.x + scaler_w);
        float scaler_y;
        if (layout->flags & ZR_WINDOW_DYNAMIC)
            scaler_y = footer.y + layout->footer_h - scaler_size.y;
        else scaler_y = layout->bounds.y + layout->bounds.h - scaler_size.y;

        /* draw scaler */
        scaler = &style->window.scaler;
        if (scaler->type == ZR_STYLE_ITEM_IMAGE) {
            zr_draw_image(out, zr_rect(scaler_x, scaler_y, scaler_w, scaler_h),
                &scaler->data.image);
        } else {
            zr_fill_triangle(out, scaler_x + scaler_w, scaler_y, scaler_x + scaler_w,
                scaler_y + scaler_h, scaler_x, scaler_y + scaler_h,
                scaler->data.color);
        }

        /* do window scaling logic */
        if (!(window->flags & ZR_WINDOW_ROM)) {
            float prev_x = in->mouse.prev.x;
            float prev_y = in->mouse.prev.y;
            struct zr_vec2 window_size = style->window.min_size;
            int incursor = ZR_INBOX(prev_x,prev_y,scaler_x,scaler_y,scaler_w,scaler_h);

            if (zr_input_is_mouse_down(in, ZR_BUTTON_LEFT) && incursor) {
                window->bounds.w = ZR_MAX(window_size.x, window->bounds.w + in->mouse.delta.x);
                /* draging in y-direction is only possible if static window */
                if (!(layout->flags & ZR_WINDOW_DYNAMIC))
                    window->bounds.h = ZR_MAX(window_size.y, window->bounds.h + in->mouse.delta.y);
            }
        }
    }

    /* window border */
    if (layout->flags & ZR_WINDOW_BORDER) {
        const float width = (layout->flags & ZR_WINDOW_NO_SCROLLBAR) ?
            layout->width: layout->width + scrollbar_size.x;
        const float padding_y = (layout->flags & ZR_WINDOW_MINIMIZED) ?
            window->bounds.y + layout->header_h:
            (layout->flags & ZR_WINDOW_DYNAMIC)?
            layout->footer_h + footer.y:
            layout->bounds.y + layout->bounds.h;

        if (window->flags & ZR_WINDOW_BORDER_HEADER)
            zr_stroke_line(out, window->bounds.x, window->bounds.y + layout->header_h,
                window->bounds.x + window->bounds.w, window->bounds.y + layout->header_h,
                style->window.border, style->window.border_color);
        zr_stroke_line(out, window->bounds.x, padding_y, window->bounds.x + width,
                padding_y, style->window.border, style->window.border_color);
        zr_stroke_line(out, window->bounds.x, window->bounds.y, window->bounds.x,
                padding_y, style->window.border, style->window.border_color);
        zr_stroke_line(out, window->bounds.x + width, window->bounds.y,
                window->bounds.x + width, padding_y, style->window.border,
                style->window.border_color);
    }

    if (!(window->flags & ZR_WINDOW_SUB)) {
        /* window is hidden so clear command buffer  */
        if (layout->flags & ZR_WINDOW_HIDDEN)
            zr_command_buffer_reset(&window->buffer);
        /* window is visible and not tab */
        else zr_finish(ctx, window);
    }

    /* ZR_WINDOW_REMOVE_ROM flag was set so remove ZR_WINDOW_ROM */
    if (layout->flags & ZR_WINDOW_REMOVE_ROM) {
        layout->flags &= ~(zr_flags)ZR_WINDOW_ROM;
        layout->flags &= ~(zr_flags)ZR_WINDOW_REMOVE_ROM;
    }
    window->flags = layout->flags;

    /* property garbage collector */
    if (window->property.active && window->property.old != window->property.seq &&
        window->property.active == window->property.prev) {
        zr_zero(&window->property, sizeof(window->property));
    } else {
        window->property.old = window->property.seq;
        window->property.prev = window->property.active;
        window->property.seq = 0;
    }

    /* edit garbage collector */
    if (window->edit.active && window->edit.old != window->edit.seq &&
        window->edit.active == window->edit.prev) {
        zr_zero(&window->edit, sizeof(window->edit));
    } else {
        window->edit.old = window->edit.seq;
        window->edit.prev = window->edit.active;
        window->edit.seq = 0;
    }

    /* contextual gargabe collector */
    if (window->popup.active_con && window->popup.con_old != window->popup.con_count) {
        window->popup.con_count = 0;
        window->popup.con_old = 0;
        window->popup.active_con = 0;
    } else {
        window->popup.con_old = window->popup.con_count;
        window->popup.con_count = 0;
    }
    window->popup.combo_count = 0;
    /* helper to make sure you have a 'zr_layout_push'
     * for every 'zr_layout_pop' */
    ZR_ASSERT(!layout->row.tree_depth);
}

void
zr_menubar_begin(struct zr_context *ctx)
{
    struct zr_panel *layout;
    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    layout = ctx->current->layout;
    if (layout->flags & ZR_WINDOW_HIDDEN || layout->flags & ZR_WINDOW_MINIMIZED)
        return;

    layout->menu.x = layout->at_x;
    layout->menu.y = layout->bounds.y + layout->header_h;
    layout->menu.w = layout->width;
    layout->menu.offset = *layout->offset;
    layout->offset->y = 0;
}

void
zr_menubar_end(struct zr_context *ctx)
{
    struct zr_window *win;
    struct zr_panel *layout;
    struct zr_command_buffer *out;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    if (!ctx || layout->flags & ZR_WINDOW_HIDDEN || layout->flags & ZR_WINDOW_MINIMIZED)
        return;

    out = &win->buffer;
    layout->menu.h = layout->at_y - layout->menu.y;
    layout->clip.y = layout->bounds.y + layout->header_h + layout->menu.h + layout->row.height;
    layout->height -= layout->menu.h;
    *layout->offset = layout->menu.offset;
    layout->clip.h -= layout->menu.h + layout->row.height;
    layout->at_y = layout->menu.y + layout->menu.h;
    zr_push_scissor(out, layout->clip);
}
/* -------------------------------------------------------------
 *
 *                          LAYOUT
 *
 * --------------------------------------------------------------*/
#define ZR_LAYOUT_DYNAMIC_FIXED     0
#define ZR_LAYOUT_DYNAMIC_ROW       1
#define ZR_LAYOUT_DYNAMIC_FREE      2
#define ZR_LAYOUT_DYNAMIC           3
#define ZR_LAYOUT_STATIC_FIXED      4
#define ZR_LAYOUT_STATIC_ROW        5
#define ZR_LAYOUT_STATIC_FREE       6
#define ZR_LAYOUT_STATIC            7

static void
zr_panel_layout(const struct zr_context *ctx, struct zr_window *win,
    float height, int cols)
{
    struct zr_panel *layout;
    const struct zr_style *style;
    struct zr_command_buffer *out;

    struct zr_vec2 item_spacing;
    struct zr_vec2 panel_padding;
    struct zr_color color;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    /* prefetch some configuration data */
    layout = win->layout;
    style = &ctx->style;
    out = &win->buffer;
    color = style->window.background;
    item_spacing = style->window.spacing;
    panel_padding = style->window.padding;

    /* update the current row and set the current row layout */
    layout->row.index = 0;
    layout->at_y += layout->row.height;
    layout->row.columns = cols;
    layout->row.height = height + item_spacing.y;
    layout->row.item_offset = 0;
    if (layout->flags & ZR_WINDOW_DYNAMIC)
        zr_fill_rect(out,  zr_rect(layout->bounds.x, layout->at_y,
            layout->bounds.w, height + panel_padding.y), 0, color);
}

static void
zr_row_layout(struct zr_context *ctx, enum zr_layout_format fmt,
    float height, int cols, int width)
{
    /* update the current row and set the current row layout */
    struct zr_window *win;
    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    zr_panel_layout(ctx, win, height, cols);
    if (fmt == ZR_DYNAMIC)
        win->layout->row.type = ZR_LAYOUT_DYNAMIC_FIXED;
    else win->layout->row.type = ZR_LAYOUT_STATIC_FIXED;

    win->layout->row.item_width = (float)width;
    win->layout->row.ratio = 0;
    win->layout->row.item_offset = 0;
    win->layout->row.filled = 0;
}

void
zr_layout_row_dynamic(struct zr_context *ctx, float height, int cols)
{
    zr_row_layout(ctx, ZR_DYNAMIC, height, cols, 0);
}

void
zr_layout_row_static(struct zr_context *ctx, float height, int item_width, int cols)
{
    zr_row_layout(ctx, ZR_STATIC, height, cols, item_width);
}

void
zr_layout_row_begin(struct zr_context *ctx, enum zr_layout_format fmt,
    float row_height, int cols)
{
    struct zr_window *win;
    struct zr_panel *layout;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;

    zr_panel_layout(ctx, win, row_height, cols);
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
zr_layout_row_push(struct zr_context *ctx, float ratio_or_width)
{
    struct zr_window *win;
    struct zr_panel *layout;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;

    if (layout->row.type == ZR_LAYOUT_DYNAMIC_ROW) {
        float ratio = ratio_or_width;
        if ((ratio + layout->row.filled) > 1.0f) return;
        if (ratio > 0.0f)
            layout->row.item_width = ZR_SATURATE(ratio);
        else layout->row.item_width = 1.0f - layout->row.filled;
    } else layout->row.item_width = ratio_or_width;
}

void
zr_layout_row_end(struct zr_context *ctx)
{
    struct zr_window *win;
    struct zr_panel *layout;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    layout->row.item_width = 0;
    layout->row.item_offset = 0;
}

void
zr_layout_row(struct zr_context *ctx, enum zr_layout_format fmt,
    float height, int cols, const float *ratio)
{
    int i;
    int n_undef = 0;
    struct zr_window *win;
    struct zr_panel *layout;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    zr_panel_layout(ctx, win, height, cols);
    if (fmt == ZR_DYNAMIC) {
        /* calculate width of undefined widget ratios */
        float r = 0;
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
zr_layout_space_begin(struct zr_context *ctx, enum zr_layout_format fmt,
    float height, int widget_count)
{
    struct zr_window *win;
    struct zr_panel *layout;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    zr_panel_layout(ctx, win, height, widget_count);
    if (fmt == ZR_STATIC)
        layout->row.type = ZR_LAYOUT_STATIC_FREE;
    else layout->row.type = ZR_LAYOUT_DYNAMIC_FREE;

    layout->row.ratio = 0;
    layout->row.item_width = 0;
    layout->row.item_offset = 0;
    layout->row.filled = 0;
}

void
zr_layout_space_end(struct zr_context *ctx)
{
    struct zr_window *win;
    struct zr_panel *layout;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    layout->row.item_width = 0;
    layout->row.item_height = 0;
    layout->row.item_offset = 0;
    zr_zero(&layout->row.item, sizeof(layout->row.item));
}

void
zr_layout_space_push(struct zr_context *ctx, struct zr_rect rect)
{
    struct zr_window *win;
    struct zr_panel *layout;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    layout->row.item = rect;
}

struct zr_rect
zr_layout_space_bounds(struct zr_context *ctx)
{
    struct zr_rect ret;
    struct zr_window *win;
    struct zr_panel *layout;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    win = ctx->current;
    layout = win->layout;

    ret.x = layout->clip.x;
    ret.y = layout->clip.y;
    ret.w = layout->clip.w;
    ret.h = layout->row.height;
    return ret;
}

struct zr_vec2
zr_layout_space_to_screen(struct zr_context *ctx, struct zr_vec2 ret)
{
    struct zr_window *win;
    struct zr_panel *layout;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    win = ctx->current;
    layout = win->layout;

    ret.x += layout->at_x - layout->offset->x;
    ret.y += layout->at_y - layout->offset->y;
    return ret;
}

struct zr_vec2
zr_layout_space_to_local(struct zr_context *ctx, struct zr_vec2 ret)
{
    struct zr_window *win;
    struct zr_panel *layout;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    win = ctx->current;
    layout = win->layout;

    ret.x += -layout->at_x + layout->offset->x;
    ret.y += -layout->at_y + layout->offset->y;
    return ret;
}

struct zr_rect
zr_layout_space_rect_to_screen(struct zr_context *ctx, struct zr_rect ret)
{
    struct zr_window *win;
    struct zr_panel *layout;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    win = ctx->current;
    layout = win->layout;

    ret.x += layout->at_x - layout->offset->x;
    ret.y += layout->at_y - layout->offset->y;
    return ret;
}

struct zr_rect
zr_layout_space_rect_to_local(struct zr_context *ctx, struct zr_rect ret)
{
    struct zr_window *win;
    struct zr_panel *layout;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    win = ctx->current;
    layout = win->layout;

    ret.x += -layout->at_x + layout->offset->x;
    ret.y += -layout->at_y + layout->offset->y;
    return ret;
}

static void
zr_panel_alloc_row(const struct zr_context *ctx, struct zr_window *win)
{
    struct zr_panel *layout = win->layout;
    struct zr_vec2 spacing = ctx->style.window.spacing;
    const float row_height = layout->row.height - spacing.y;
    zr_panel_layout(ctx, win, row_height, layout->row.columns);
}

static void
zr_layout_widget_space(struct zr_rect *bounds, const struct zr_context *ctx,
    struct zr_window *win, int modify)
{
    struct zr_panel *layout;
    float item_offset = 0;
    float item_width = 0;
    float item_spacing = 0;

    float panel_padding;
    float panel_spacing;
    float panel_space;

    struct zr_vec2 spacing;
    struct zr_vec2 padding;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    ZR_ASSERT(bounds);

    /* cache some configuration data */
    spacing = ctx->style.window.spacing;
    padding = ctx->style.window.padding;

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
        bounds->x -= layout->offset->x;
        bounds->y = layout->at_y + (layout->row.height * layout->row.item.y);
        bounds->y -= layout->offset->y;
        bounds->w = layout->width  * layout->row.item.w;
        bounds->h = layout->row.height * layout->row.item.h;
        return;
    };
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
        bounds->x = layout->at_x + layout->row.item.x;
        bounds->w = layout->row.item.w;
        if (((bounds->x + bounds->w) > layout->max_x) && modify)
            layout->max_x = (bounds->x + bounds->w);
        bounds->x -= layout->offset->x;
        bounds->y = layout->at_y + layout->row.item.y;
        bounds->y -= layout->offset->y;
        bounds->h = layout->row.item.h;
        return;
    };
    case ZR_LAYOUT_STATIC: {
        /* non-scaling array of panel pixel width for every widget */
        item_spacing = (float)layout->row.index * spacing.x;
        item_width = layout->row.ratio[layout->row.index];
        item_offset = layout->row.item_offset;
        if (modify) layout->row.item_offset += item_width + spacing.x;
        } break;
    default: ZR_ASSERT(0); break;
    };

    /* set the bounds of the newly allocated widget */
    bounds->w = item_width;
    bounds->h = layout->row.height - spacing.y;
    bounds->y = layout->at_y - layout->offset->y;
    bounds->x = layout->at_x + item_offset + item_spacing + padding.x;
    if (((bounds->x + bounds->w) > layout->max_x) && modify)
        layout->max_x = bounds->x + bounds->w;
    bounds->x -= layout->offset->x;
}

static void
zr_panel_alloc_space(struct zr_rect *bounds, const struct zr_context *ctx)
{
    struct zr_window *win;
    struct zr_panel *layout;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    /* check if the end of the row has been hit and begin new row if so */
    win = ctx->current;
    layout = win->layout;
    if (layout->row.index >= layout->row.columns)
        zr_panel_alloc_row(ctx, win);

    /* calculate widget position and size */
    zr_layout_widget_space(bounds, ctx, win, zr_true);
    layout->row.index++;
}

static void
zr_layout_peek(struct zr_rect *bounds, struct zr_context *ctx)
{
    float y;
    int index;
    struct zr_window *win;
    struct zr_panel *layout;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    y = layout->at_y;
    index = layout->row.index;
    if (layout->row.index >= layout->row.columns) {
        layout->at_y += layout->row.height;
        layout->row.index = 0;
    }
    zr_layout_widget_space(bounds, ctx, win, zr_false);
    layout->at_y = y;
    layout->row.index = index;
}

int
zr__layout_push(struct zr_context *ctx, enum zr_layout_node_type type,
    const char *title, enum zr_collapse_states initial_state,
    const char *file, int line)
{
    struct zr_window *win;
    struct zr_panel *layout;
    const struct zr_style *style;
    struct zr_command_buffer *out;
    const struct zr_input *in;

    struct zr_vec2 item_spacing;
    struct zr_vec2 panel_padding;
    struct zr_rect header = {0,0,0,0};
    struct zr_rect sym = {0,0,0,0};
    struct zr_text text;

    zr_flags ws;
    int title_len;
    zr_hash title_hash;
    zr_uint *state = 0;
    enum zr_widget_layout_states widget_state;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    /* cache some data */
    win = ctx->current;
    layout = win->layout;
    out = &win->buffer;
    style = &ctx->style;

    item_spacing = style->window.spacing;
    panel_padding = style->window.padding;

    /* calculate header bounds and draw background */
    zr_layout_row_dynamic(ctx, style->font.height + 2 * style->tab.padding.y, 1);
    widget_state = zr_widget(&header, ctx);
    if (type == ZR_LAYOUT_TAB) {
        const struct zr_style_item *background = &style->tab.background;
        if (background->type == ZR_STYLE_ITEM_IMAGE) {
            zr_draw_image(out, header, &background->data.image);
            text.background = zr_rgba(0,0,0,0);
        } else {
            text.background = background->data.color;
            zr_fill_rect(out, header, 0, style->tab.border_color);
            zr_fill_rect(out, zr_shrink_rect(header, style->tab.border),
                style->tab.rounding, background->data.color);
        }
    } else text.background = style->window.background;

    /* find or create tab persistent state (open/closed) */
    title_len = (int)zr_strsiz(title);
    title_hash = zr_murmur_hash(title, (int)title_len, (zr_hash)line);
    if (file) title_hash += zr_murmur_hash(file, (int)zr_strsiz(file), (zr_hash)line);
    state = zr_find_value(win, title_hash);
    if (!state) {
        state = zr_add_value(ctx, win, title_hash, 0);
        *state = initial_state;
    }

    /* update node state */
    in = (!(layout->flags & ZR_WINDOW_ROM)) ? &ctx->input: 0;
    in = (in && widget_state == ZR_WIDGET_VALID) ? &ctx->input : 0;
    if (zr_button_behavior(&ws, header, in, ZR_BUTTON_DEFAULT))
        *state = (*state == ZR_MAXIMIZED) ? ZR_MINIMIZED : ZR_MAXIMIZED;

    {
        /* draw closing/open icon */
        enum zr_heading heading;
        heading = (*state == ZR_MAXIMIZED) ? ZR_DOWN : ZR_RIGHT;

        /* calculate the triangle bounds */
        sym.w = sym.h = style->font.height;
        sym.y = header.y + style->tab.padding.y;
        sym.x = header.x + panel_padding.x + style->tab.padding.x;

        /* calculate the triangle points and draw triangle */
        zr_do_button_symbol(&ws, &win->buffer, sym,
            (*state == ZR_MAXIMIZED)? style->tab.sym_minimize: style->tab.sym_maximize,
            ZR_BUTTON_DEFAULT, (type == ZR_LAYOUT_TAB)?
            &style->tab.tab_button: &style->tab.node_button,
            in, &style->font);

        /* calculate the space the icon occupied */
        sym.w = style->font.height + 2 * style->tab.spacing.x;
    }
    {
        /* draw node label */
        struct zr_rect label;
        header.w = ZR_MAX(header.w, sym.w + item_spacing.y + panel_padding.x);
        label.x = sym.x + sym.w + item_spacing.x;
        label.y = sym.y;
        label.w = header.w - (sym.w + item_spacing.y + panel_padding.x);
        label.h = style->font.height;

        text.text = style->tab.text;
        text.padding = zr_vec2(0,0);
        zr_widget_text(out, label, title, zr_strsiz(title), &text,
            ZR_TEXT_LEFT, &style->font);
    }

    /* increase x-axis cursor widget position pointer */
    if (*state == ZR_MAXIMIZED) {
        layout->at_x = header.x + layout->offset->x;
        layout->width = ZR_MAX(layout->width, 2 * panel_padding.x);
        layout->width -= 2 * panel_padding.x;
        layout->row.tree_depth++;
        return zr_true;
    } else return zr_false;
}

void zr_layout_pop(struct zr_context *ctx)
{
    struct zr_vec2 panel_padding;
    struct zr_window *win = 0;
    struct zr_panel *layout = 0;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    win = ctx->current;
    layout = win->layout;
    panel_padding = ctx->style.window.padding;
    layout->at_x -= panel_padding.x;
    layout->width += 2 * panel_padding.x;
    ZR_ASSERT(layout->row.tree_depth);
    layout->row.tree_depth--;
}
/*----------------------------------------------------------------
 *
 *                          WIDGETS
 *
 * --------------------------------------------------------------*/
struct zr_rect
zr_widget_bounds(struct zr_context *ctx)
{
    struct zr_rect bounds;
    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    if (!ctx || !ctx->current)
        return zr_rect(0,0,0,0);
    zr_layout_peek(&bounds, ctx);
    return bounds;
}

struct zr_vec2
zr_widget_position(struct zr_context *ctx)
{
    struct zr_rect bounds;
    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    if (!ctx || !ctx->current)
        return zr_vec2(0,0);

    zr_layout_peek(&bounds, ctx);
    return zr_vec2(bounds.x, bounds.y);
}

struct zr_vec2
zr_widget_size(struct zr_context *ctx)
{
    struct zr_rect bounds;
    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    if (!ctx || !ctx->current)
        return zr_vec2(0,0);

    zr_layout_peek(&bounds, ctx);
    return zr_vec2(bounds.w, bounds.h);
}

int
zr_widget_is_hovered(struct zr_context *ctx)
{
    int ret;
    struct zr_rect bounds;
    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    if (!ctx || !ctx->current)
        return 0;

    zr_layout_peek(&bounds, ctx);
    ret = (ctx->active == ctx->current);
    ret = ret && zr_input_is_mouse_hovering_rect(&ctx->input, bounds);
    return ret;
}

int
zr_widget_is_mouse_clicked(struct zr_context *ctx, enum zr_buttons btn)
{
    int ret;
    struct zr_rect bounds;
    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    if (!ctx || !ctx->current)
        return 0;

    zr_layout_peek(&bounds, ctx);
    ret = (ctx->active == ctx->current);
    ret = ret && zr_input_mouse_clicked(&ctx->input, btn, bounds);
    return ret;
}

int
zr_widget_has_mouse_click_down(struct zr_context *ctx, enum zr_buttons btn, int down)
{
    int ret;
    struct zr_rect bounds;
    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    if (!ctx || !ctx->current)
        return 0;

    zr_layout_peek(&bounds, ctx);
    ret = (ctx->active == ctx->current);
    ret = ret && zr_input_has_mouse_click_down_in_rect(&ctx->input, btn, bounds, down);
    return ret;
}

enum zr_widget_layout_states
zr_widget(struct zr_rect *bounds, const struct zr_context *ctx)
{
    struct zr_rect *c = 0;
    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return ZR_WIDGET_INVALID;

    /* allocate space  and check if the widget needs to be updated and drawn */
    zr_panel_alloc_space(bounds, ctx);
    c = &ctx->current->layout->clip;
    if (!ZR_INTERSECT(c->x, c->y, c->w, c->h, bounds->x, bounds->y, bounds->w, bounds->h))
        return ZR_WIDGET_INVALID;
    if (!ZR_CONTAINS(bounds->x, bounds->y, bounds->w, bounds->h, c->x, c->y, c->w, c->h))
        return ZR_WIDGET_ROM;
    return ZR_WIDGET_VALID;
}

enum zr_widget_layout_states
zr_widget_fitting(struct zr_rect *bounds, struct zr_context *ctx,
    struct zr_vec2 item_padding)
{
    /* update the bounds to stand without padding  */
    struct zr_window *win;
    struct zr_style *style;
    struct zr_panel *layout;
    enum zr_widget_layout_states state;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return ZR_WIDGET_INVALID;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;
    state = zr_widget(bounds, ctx);
    if (layout->row.index == 1) {
        bounds->w += style->window.padding.x;
        bounds->x -= style->window.padding.x;
    } else bounds->x -= item_padding.x;

    if (layout->row.index == layout->row.columns)
        bounds->w += style->window.padding.x;
    else bounds->w += item_padding.x;
    return state;
}

/*----------------------------------------------------------------
 *                          MISC
 * --------------------------------------------------------------*/
void
zr_spacing(struct zr_context *ctx, int cols)
{
    struct zr_window *win;
    struct zr_panel *layout;
    struct zr_rect nil;
    int i, index, rows;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    /* spacing over row boundries */
    win = ctx->current;
    layout = win->layout;
    index = (layout->row.index + cols) % layout->row.columns;
    rows = (layout->row.index + cols) / layout->row.columns;
    if (rows) {
        for (i = 0; i < rows; ++i)
            zr_panel_alloc_row(ctx, win);
        cols = index;
    }

    /* non table layout need to allocate space */
    if (layout->row.type != ZR_LAYOUT_DYNAMIC_FIXED &&
        layout->row.type != ZR_LAYOUT_STATIC_FIXED) {
        for (i = 0; i < cols; ++i)
            zr_panel_alloc_space(&nil, ctx);
    }
    layout->row.index = index;
}

/*----------------------------------------------------------------
 *                          TEXT
 * --------------------------------------------------------------*/
void
zr_text_colored(struct zr_context *ctx, const char *str, zr_size len, zr_flags alignment,
    struct zr_color color)
{
    struct zr_window *win;
    const struct zr_style *style;

    struct zr_vec2 item_padding;
    struct zr_rect bounds;
    struct zr_text text;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout) return;

    win = ctx->current;
    style = &ctx->style;
    zr_panel_alloc_space(&bounds, ctx);
    item_padding = style->text.padding;

    text.padding.x = item_padding.x;
    text.padding.y = item_padding.y;
    text.background = style->window.background;
    text.text = color;
    zr_widget_text(&win->buffer, bounds, str, len, &text, alignment, &style->font);
}

void
zr_text_wrap_colored(struct zr_context *ctx, const char *str,
    zr_size len, struct zr_color color)
{
    struct zr_window *win;
    const struct zr_style *style;

    struct zr_vec2 item_padding;
    struct zr_rect bounds;
    struct zr_text text;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout) return;

    win = ctx->current;
    style = &ctx->style;
    zr_panel_alloc_space(&bounds, ctx);
    item_padding = style->text.padding;

    text.padding.x = item_padding.x;
    text.padding.y = item_padding.y;
    text.background = style->window.background;
    text.text = color;
    zr_widget_text_wrap(&win->buffer, bounds, str, len, &text, &style->font);
    ctx->last_widget_state = 0;
}

void
zr_text(struct zr_context *ctx, const char *str, zr_size len, zr_flags alignment)
{
    ZR_ASSERT(ctx);
    if (!ctx) return;
    zr_text_colored(ctx, str, len, alignment, ctx->style.text.color);
}

void
zr_text_wrap(struct zr_context *ctx, const char *str, zr_size len)
{
    ZR_ASSERT(ctx);
    if (!ctx) return;
    zr_text_wrap_colored(ctx, str, len, ctx->style.text.color);
}

void
zr_label(struct zr_context *ctx, const char *str, zr_flags alignment)
{zr_text(ctx, str, zr_strsiz(str), alignment);}

void
zr_label_colored(struct zr_context *ctx, const char *str, zr_flags align,
    struct zr_color color)
{zr_text_colored(ctx, str, zr_strsiz(str), align, color);}

void
zr_label_wrap(struct zr_context *ctx, const char *str)
{zr_text_wrap(ctx, str, zr_strsiz(str));}

void
zr_label_colored_wrap(struct zr_context *ctx, const char *str, struct zr_color color)
{zr_text_wrap_colored(ctx, str, zr_strsiz(str), color);}

void
zr_image(struct zr_context *ctx, struct zr_image img)
{
    struct zr_window *win;
    struct zr_rect bounds;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout) return;

    win = ctx->current;
    if (!zr_widget(&bounds, ctx)) return;
    zr_draw_image(&win->buffer, bounds, &img);
    ctx->last_widget_state = 0;
}

/*----------------------------------------------------------------
 *                          BUTTON
 * --------------------------------------------------------------*/
int
zr_button_text(struct zr_context *ctx, const char *title, zr_size len,
    enum zr_button_behavior behavior)
{
    struct zr_window *win;
    struct zr_panel *layout;
    const struct zr_input *in;
    const struct zr_style *style;

    struct zr_rect bounds;
    enum zr_widget_layout_states state;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout) return 0;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;
    state = zr_widget(&bounds, ctx);

    if (!state) return 0;
    in = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : &ctx->input;
    return zr_do_button_text(&ctx->last_widget_state, &win->buffer, bounds,
                    title, len, style->button.text_alignment, behavior,
                    &style->button, in, &style->font);
}

int zr_button_label(struct zr_context *ctx, const char *title,
    enum zr_button_behavior behavior)
{return zr_button_text(ctx, title, zr_strsiz(title), behavior);}

int
zr_button_color(struct zr_context *ctx, struct zr_color color,
    enum zr_button_behavior behavior)
{
    struct zr_window *win;
    struct zr_panel *layout;
    const struct zr_input *in;
    struct zr_style_button button;
    const struct zr_style *style;

    int ret = 0;
    struct zr_rect bounds;
    enum zr_widget_layout_states state;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;

    state = zr_widget(&bounds, ctx);
    if (!state) return 0;
    in = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : &ctx->input;

    button = ctx->style.button;
    button.normal = zr_style_item_color(color);
    button.hover = zr_style_item_color(color);
    button.active = zr_style_item_color(color);
    button.padding = zr_vec2(0,0);
    ret = zr_do_button(&ctx->last_widget_state, &win->buffer, bounds,
                &button, in, behavior, &bounds);
    zr_draw_button(&win->buffer, &bounds, ctx->last_widget_state, &button);
    return ret;
}

int
zr_button_symbol(struct zr_context *ctx, enum zr_symbol_type symbol,
    enum zr_button_behavior behavior)
{
    struct zr_window *win;
    struct zr_panel *layout;
    const struct zr_input *in;
    const struct zr_style *style;

    struct zr_rect bounds;
    enum zr_widget_layout_states state;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;

    state = zr_widget(&bounds, ctx);
    if (!state) return 0;
    in = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : &ctx->input;
    return zr_do_button_symbol(&ctx->last_widget_state, &win->buffer, bounds,
            symbol, behavior, &style->button, in, &style->font);
}

int
zr_button_image(struct zr_context *ctx, struct zr_image img,
    enum zr_button_behavior behavior)
{
    struct zr_window *win;
    struct zr_panel *layout;
    const struct zr_input *in;
    const struct zr_style *style;

    struct zr_rect bounds;
    enum zr_widget_layout_states state;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;

    state = zr_widget(&bounds, ctx);
    if (!state) return 0;
    in = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : &ctx->input;
    return zr_do_button_image(&ctx->last_widget_state, &win->buffer, bounds,
                img, behavior, &style->button, in);
}

int
zr_button_symbol_text(struct zr_context *ctx, enum zr_symbol_type symbol,
    const char* text, zr_size len, zr_flags align, enum zr_button_behavior behavior)
{
    struct zr_window *win;
    struct zr_panel *layout;
    const struct zr_input *in;
    const struct zr_style *style;

    struct zr_rect bounds;
    enum zr_widget_layout_states state;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;

    state = zr_widget(&bounds, ctx);
    if (!state) return 0;
    in = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : &ctx->input;
    return zr_do_button_text_symbol(&ctx->last_widget_state, &win->buffer, bounds,
                symbol, text, len, align, behavior, &style->button, &style->font, in);
}

int zr_button_symbol_label(struct zr_context *ctx, enum zr_symbol_type symbol,
    const char *label, zr_flags align, enum zr_button_behavior behavior)
{return zr_button_symbol_text(ctx, symbol, label, zr_strsiz(label), align, behavior);}

int
zr_button_image_text(struct zr_context *ctx, struct zr_image img,
    const char *text, zr_size len, zr_flags align, enum zr_button_behavior behavior)
{
    struct zr_window *win;
    struct zr_panel *layout;
    const struct zr_input *in;
    const struct zr_style *style;

    struct zr_rect bounds;
    enum zr_widget_layout_states state;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;

    state = zr_widget(&bounds, ctx);
    if (!state) return 0;
    in = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : &ctx->input;
    return zr_do_button_text_image(&ctx->last_widget_state, &win->buffer,
            bounds, img, text, len, align, behavior, &style->button, &style->font, in);
}

int zr_button_image_label(struct zr_context *ctx, struct zr_image img,
    const char *label, zr_flags align, enum zr_button_behavior behavior)
{return zr_button_image_text(ctx, img, label, zr_strsiz(label), align, behavior);}

/*----------------------------------------------------------------
 *                          SELECTABLE
 * --------------------------------------------------------------*/
int
zr_selectable_text(struct zr_context *ctx, const char *str, zr_size len,
    zr_flags align, int *value)
{
    struct zr_window *win;
    struct zr_panel *layout;
    const struct zr_input *in;
    const struct zr_style *style;

    enum zr_widget_layout_states state;
    struct zr_rect bounds;

    ZR_ASSERT(ctx);
    ZR_ASSERT(value);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout || !value)
        return 0;

    win = ctx->current;
    layout = win->layout;
    style = &ctx->style;
    state = zr_widget(&bounds, ctx);
    if (!state) return 0;
    in = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : &ctx->input;
    return zr_do_selectable(&ctx->last_widget_state, &win->buffer, bounds,
                str, len, align, value, &style->selectable, in, &style->font);
}

int zr_select_text(struct zr_context *ctx, const char *str, zr_size len,
    zr_flags align, int value)
{zr_selectable_text(ctx, str, len, align, &value);return value;}

int zr_selectable_label(struct zr_context *ctx, const char *str, zr_flags align, int *value)
{return zr_selectable_text(ctx, str, zr_strsiz(str), align, value);}

int zr_select_label(struct zr_context *ctx, const char *str, zr_flags align, int value)
{zr_selectable_text(ctx, str, zr_strsiz(str), align, &value);return value;}

/*----------------------------------------------------------------
 *                          CHECKBOX
 * --------------------------------------------------------------*/
int
zr_check_text(struct zr_context *ctx, const char *text, zr_size len, int active)
{
    struct zr_window *win;
    struct zr_panel *layout;
    const struct zr_input *in;
    const struct zr_style *style;

    struct zr_rect bounds;
    enum zr_widget_layout_states state;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return active;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;
    state = zr_widget(&bounds, ctx);
    if (!state) return active;
    in = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : &ctx->input;
    zr_do_toggle(&ctx->last_widget_state, &win->buffer, bounds, &active,
        text, len, ZR_TOGGLE_CHECK, &style->checkbox, in, &style->font);
    return active;
}

unsigned int
zr_check_flag_text(struct zr_context *ctx, const char *text, zr_size len,
    unsigned int flags, unsigned int value)
{
    int old_active, active;
    ZR_ASSERT(ctx);
    ZR_ASSERT(text);
    if (!ctx || !text) return flags;
    old_active = active = (int)(flags & value);
    if (zr_check_text(ctx, text, len, old_active))
        flags |= value;
    else flags &= ~value;
    return flags;
}

int
zr_checkbox_text(struct zr_context *ctx, const char *text, zr_size len, int *active)
{
    int old_val;
    ZR_ASSERT(ctx);
    ZR_ASSERT(text);
    ZR_ASSERT(active);
    if (!ctx || !text || !active) return 0;
    old_val = *active;
    *active = zr_check_text(ctx, text, len, *active);
    return old_val != *active;
}

int
zr_checkbox_flag_text(struct zr_context *ctx, const char *text, zr_size len,
    unsigned int *flags, unsigned int value)
{
    int active;
    ZR_ASSERT(ctx);
    ZR_ASSERT(text);
    ZR_ASSERT(flags);
    if (!ctx || !text || !flags) return 0;
    active = (int)(*flags & value);
    if (zr_checkbox_text(ctx, text, len, &active)) {
        if (active) *flags |= value;
        else *flags &= ~value;
        return 1;
    }
    return 0;
}

int zr_check_label(struct zr_context *ctx, const char *label, int active)
{return zr_check_text(ctx, label, zr_strsiz(label), active);}

unsigned int zr_check_flag_label(struct zr_context *ctx, const char *label,
    unsigned int flags, unsigned int value)
{return zr_check_flag_text(ctx, label, zr_strsiz(label), flags, value);}

int zr_checkbox_label(struct zr_context *ctx, const char *label, int *active)
{return zr_checkbox_text(ctx, label, zr_strsiz(label), active);}

int zr_checkbox_flag_label(struct zr_context *ctx, const char *label,
    unsigned int *flags, unsigned int value)
{return zr_checkbox_flag_text(ctx, label, zr_strsiz(label), flags, value);}

/*----------------------------------------------------------------
 *                          OPTION
 * --------------------------------------------------------------*/
int
zr_option_text(struct zr_context *ctx, const char *text, zr_size len, int is_active)
{
    struct zr_window *win;
    struct zr_panel *layout;
    const struct zr_input *in;
    const struct zr_style *style;

    struct zr_rect bounds;
    enum zr_widget_layout_states state;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return is_active;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;
    state = zr_widget(&bounds, ctx);
    if (!state) return state;
    in = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : &ctx->input;
    zr_do_toggle(&ctx->last_widget_state, &win->buffer, bounds, &is_active,
        text, len, ZR_TOGGLE_OPTION, &style->option, in, &style->font);
    return is_active;
}

int
zr_radio_text(struct zr_context *ctx, const char *text, zr_size len, int *active)
{
    int old_value;
    ZR_ASSERT(ctx);
    ZR_ASSERT(text);
    ZR_ASSERT(active);
    if (!ctx || !text || !active) return 0;
    old_value = *active;
    *active = zr_option_text(ctx, text, len, old_value);
    return old_value != *active;
}

int
zr_option_label(struct zr_context *ctx, const char *label, int active)
{return zr_option_text(ctx, label, zr_strsiz(label), active);}

int
zr_radio_label(struct zr_context *ctx, const char *label, int *active)
{return zr_radio_text(ctx, label, zr_strsiz(label), active);}

/*----------------------------------------------------------------
 *                          SLIDER
 * --------------------------------------------------------------*/
int
zr_slider_float(struct zr_context *ctx, float min_value, float *value, float max_value,
    float value_step)
{
    struct zr_window *win;
    struct zr_panel *layout;
    const struct zr_input *in;
    const struct zr_style *style;

    int ret = 0;
    float old_value;
    struct zr_rect bounds;
    enum zr_widget_layout_states state;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    ZR_ASSERT(value);
    if (!ctx || !ctx->current || !ctx->current->layout || !value)
        return ret;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;
    state = zr_widget(&bounds, ctx);
    if (!state) return ret;
    in = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : &ctx->input;

    old_value = *value;
    *value = zr_do_slider(&ctx->last_widget_state, &win->buffer, bounds, min_value,
                old_value, max_value, value_step, &style->slider, in, &style->font);
    return (old_value > *value || old_value < *value);
}

float
zr_slide_float(struct zr_context *ctx, float min, float val, float max, float step)
{
    zr_slider_float(ctx, min, &val, max, step); return val;
}

int
zr_slide_int(struct zr_context *ctx, int min, int val, int max, int step)
{
    float value = (float)val;
    zr_slider_float(ctx, (float)min, &value, (float)max, (float)step);
    return (int)value;
}

int
zr_slider_int(struct zr_context *ctx, int min, int *val, int max, int step)
{
    int ret;
    float value = (float)*val;
    ret = zr_slider_float(ctx, (float)min, &value, (float)max, (float)step);
    *val =  (int)value;
    return ret;
}

/*----------------------------------------------------------------
 *                          PROGRESSBAR
 * --------------------------------------------------------------*/
int
zr_progress(struct zr_context *ctx, zr_size *cur, zr_size max, int is_modifyable)
{
    struct zr_window *win;
    struct zr_panel *layout;
    const struct zr_style *style;
    const struct zr_input *in;

    struct zr_rect bounds;
    enum zr_widget_layout_states state;
    zr_size old_value;

    ZR_ASSERT(ctx);
    ZR_ASSERT(cur);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout || !cur)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;
    state = zr_widget(&bounds, ctx);
    if (!state) return 0;
    in = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : &ctx->input;
    old_value = *cur;
    *cur = zr_do_progress(&ctx->last_widget_state, &win->buffer, bounds,
            *cur, max, is_modifyable, &style->progress, in);
    return (*cur != old_value);
}

zr_size zr_prog(struct zr_context *ctx, zr_size cur, zr_size max, int modifyable)
{zr_progress(ctx, &cur, max, modifyable);return cur;}

/*----------------------------------------------------------------
 *                          EDIT
 * --------------------------------------------------------------*/
zr_flags
zr_edit_string(struct zr_context *ctx, zr_flags flags,
    char *memory, zr_size *len, zr_size max, zr_filter filter)
{
    zr_flags active;
    struct zr_buffer buffer;

    max = ZR_MAX(1, max);
    *len = ZR_MIN(*len, max-1);
    zr_buffer_init_fixed(&buffer, memory, max);
    buffer.allocated = *len;
    active = zr_edit_buffer(ctx, flags, &buffer, filter);
    *len = buffer.allocated;
    return active;
}

zr_flags
zr_edit_buffer(struct zr_context *ctx, zr_flags flags,
    struct zr_buffer *buffer, zr_filter filter)
{
    struct zr_window *win;
    struct zr_style *style;
    struct zr_input *in;

    enum zr_widget_layout_states state;
    struct zr_rect bounds;
    zr_flags ret_flags = 0;
    int modifiable = 0;
    zr_flags old_flags;
    int show_cursor = 0;
    zr_hash hash;

    int *active = 0;
    float *scroll = 0;
    zr_size *cursor = 0;
    struct zr_text_selection *sel = 0;

    /* dummy state for non active edit */
    int dummy_active = 0;
    float dummy_scroll = 0;
    zr_size dummy_cursor = 0;
    struct zr_text_selection dummy_sel = {0,0,0};

    /* make sure correct values */
    ZR_ASSERT(ctx);
    ZR_ASSERT(buffer);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    state = zr_widget(&bounds, ctx);
    if (!state) return state;
    in = (state == ZR_WIDGET_ROM || win->layout->flags & ZR_WINDOW_ROM) ? 0 : &ctx->input;
    if ((flags & ZR_EDIT_READ_ONLY)) {
        modifiable = 0;
        show_cursor = 0;
    } else {
        modifiable = 1;
    }

    /* check if edit is currently hot item */
    hash = win->edit.seq++;
    if (win->edit.active && hash == win->edit.name) {
        active = &win->edit.active;
        cursor = &win->edit.cursor;
        scroll = &win->edit.scrollbar;
        sel =  &win->edit.sel;
    } else {
        active = &dummy_active;
        cursor = &dummy_cursor;
        scroll = &dummy_scroll;
        sel =  &dummy_sel;
    }

    old_flags = (*active) ? ZR_EDIT_ACTIVE: ZR_EDIT_INACTIVE;
    if (!flags || flags == ZR_EDIT_CURSOR) {
        /* edit which only support appending and removing at end of line */
        int old = *active;
        in = (flags & ZR_EDIT_READ_ONLY || !(modifiable)) ? 0: in;
        if (!flags) {
            /* simple edit field with only appending and removing at the end of the buffer */
            buffer->allocated = zr_do_edit_string(&ctx->last_widget_state,
                &win->buffer, bounds, (char*)buffer->memory.ptr, buffer->allocated,
                buffer->memory.size, active, 0, show_cursor, &style->edit,
                filter, in, &ctx->style.font);
        } else {
            /* simple edit field cursor based movement, inserting and removing */
            zr_size glyphs = zr_utf_len((const char*)buffer->memory.ptr, buffer->allocated);
            *cursor = ZR_MIN(*cursor, glyphs);
            buffer->allocated = zr_do_edit_string(&ctx->last_widget_state,
                &win->buffer, bounds, (char*)buffer->memory.ptr, buffer->allocated,
                buffer->memory.size, active, cursor, show_cursor, &style->edit,
                filter, in, &ctx->style.font);
        }

        if (dummy_active) {
            /* set hot edit widget state */
            win->edit.active = 1;
            win->edit.name = hash;
            win->edit.scrollbar = 0;
            win->edit.sel.begin = 0;
            win->edit.sel.end = 0;
            win->edit.cursor = 0;
        } else if (old && !*active) {
            win->edit.active = 0;
        }
    } else {
        /* edit with cursor and text selection */
        struct zr_edit_box box;
        in = (flags & ZR_EDIT_READ_ONLY || !(modifiable)) ? 0: in;
        if (flags & ZR_EDIT_CLIPBOARD)
            zr_edit_box_init_buffer(&box, buffer, &ctx->clip, filter);
        else zr_edit_box_init_buffer(&box, buffer, 0, filter);

        box.glyphs = zr_utf_len((const char*)buffer->memory.ptr, buffer->allocated);
        box.active = *active;
        box.filter = filter;
        box.scrollbar = *scroll;
        *cursor = ZR_MIN(box.glyphs, *cursor);
        box.cursor = *cursor;

        if (!(flags & ZR_EDIT_CURSOR)) {
            box.sel.begin = box.cursor;
            box.sel.end = box.cursor;
        } else {
            if (!(flags & ZR_EDIT_SELECTABLE)) {
                box.sel.active = 0;
                box.sel.begin = box.cursor;
                box.sel.end = box.cursor;
            } else box.sel = *sel;
        }
        zr_do_edit_buffer(&ctx->last_widget_state, &win->buffer, bounds, &box,
            &style->edit, show_cursor, in, &style->font);

        if (box.active) {
            /* update hot edit widget state */
            *active = 1;
            win->edit.active = 1;
            win->edit.name = hash;
            win->edit.scrollbar = box.scrollbar;
            win->edit.sel = box.sel;
            win->edit.cursor = box.cursor;
            buffer->allocated = box.buffer.allocated;
        } else if (!box.active && *active) {
            win->edit.active = 0;
        }
    }

    /* enter deactivates edit and returns SIGCOMIT flag */
    if (*active && (flags & ZR_EDIT_SIGCOMIT) &&
        zr_input_is_key_pressed(in, ZR_KEY_ENTER)) {
        ret_flags |= ZR_EDIT_SIGCOMIT;
        *active = 0;
    }

    /* pack edit widget state and state changes into flags */
    ret_flags |= (*active) ? ZR_EDIT_ACTIVE: ZR_EDIT_INACTIVE;
    if (old_flags == ZR_EDIT_INACTIVE && ret_flags & ZR_EDIT_ACTIVE)
        ret_flags |= ZR_EDIT_ACTIVATED;
    else if (old_flags == ZR_EDIT_ACTIVE && ret_flags & ZR_EDIT_INACTIVE)
        ret_flags |= ZR_EDIT_DEACTIVATED;
    return ret_flags;
}

static float
zr_property(struct zr_context *ctx, const char *name, float min, float val,
    float max, float step, float inc_per_pixel, const enum zr_property_filter filter)
{
    struct zr_window *win;
    struct zr_panel *layout;
    const struct zr_input *in;
    const struct zr_style *style;

    struct zr_rect bounds;
    enum zr_widget_layout_states s;

    int *state = 0;
    zr_hash hash = 0;
    char *buffer = 0;
    zr_size *len = 0;
    zr_size *cursor = 0;
    int old_state;

    char dummy_buffer[ZR_MAX_NUMBER_BUFFER];
    int dummy_state = ZR_PROPERTY_DEFAULT;
    zr_size dummy_length = 0;
    zr_size dummy_cursor = 0;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return val;

    win = ctx->current;
    layout = win->layout;
    style = &ctx->style;
    s = zr_widget(&bounds, ctx);
    if (!s) return val;
    in = (s == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : &ctx->input;

    /* calculate hash from name */
    if (name[0] == '#') {
        hash = zr_murmur_hash(name, (int)zr_strsiz(name), win->property.seq++);
        name++; /* special number hash */
    } else hash = zr_murmur_hash(name, (int)zr_strsiz(name), 42);

    /* check if property is currently hot item */
    if (win->property.active && hash == win->property.name) {
        buffer = win->property.buffer;
        len = &win->property.length;
        cursor = &win->property.cursor;
        state = &win->property.state;
    } else {
        buffer = dummy_buffer;
        len = &dummy_length;
        cursor = &dummy_cursor;
        state = &dummy_state;
    }

    /* execute property widget */
    old_state = *state;
    val = zr_do_property(&ctx->last_widget_state, &win->buffer, bounds, name,
        min, val, max, step, inc_per_pixel, buffer, len, state, cursor,
        &style->property, filter, in, &style->font);

    if (in && *state != ZR_PROPERTY_DEFAULT && !win->property.active) {
        /* current property is now hot */
        win->property.active = 1;
        zr_memcopy(win->property.buffer, buffer, *len);
        win->property.length = *len;
        win->property.cursor = *cursor;
        win->property.state = *state;
        win->property.name = hash;
    }

    /* check if previously active property is now unactive */
    if (*state == ZR_PROPERTY_DEFAULT && old_state != ZR_PROPERTY_DEFAULT)
        win->property.active = 0;
    return val;
}

void
zr_property_float(struct zr_context *ctx, const char *name,
    float min, float *val, float max, float step, float inc_per_pixel)
{
    ZR_ASSERT(ctx);
    ZR_ASSERT(name);
    ZR_ASSERT(val);
    if (!ctx || !ctx->current || !name || !val) return;
    *val = zr_property(ctx, name, min, *val, max, step, inc_per_pixel, ZR_FILTER_FLOAT);
}

void
zr_property_int(struct zr_context *ctx, const char *name,
    int min, int *val, int max, int step, int inc_per_pixel)
{
    float value;
    ZR_ASSERT(ctx);
    ZR_ASSERT(name);
    ZR_ASSERT(val);
    if (!ctx || !ctx->current || !name || !val) return;
    value = zr_property(ctx, name, (float)min, (float)*val, (float)max, (float)step,
        (float)inc_per_pixel, ZR_FILTER_FLOAT);
    *val = (int)value;
}

float
zr_propertyf(struct zr_context *ctx, const char *name, float min,
    float val, float max, float step, float inc_per_pixel)
{
    ZR_ASSERT(ctx);
    ZR_ASSERT(name);
    if (!ctx || !ctx->current || !name) return val;
    return zr_property(ctx, name, min, val, max, step, inc_per_pixel, ZR_FILTER_FLOAT);
}

int
zr_propertyi(struct zr_context *ctx, const char *name, int min, int val,
    int max, int step, int inc_per_pixel)
{
    float value;
    ZR_ASSERT(ctx);
    ZR_ASSERT(name);
    if (!ctx || !ctx->current || !name) return val;
    value = zr_property(ctx, name, (float)min, (float)val, (float)max, (float)step,
        (float)inc_per_pixel, ZR_FILTER_FLOAT);
    return (int)value;
}

int
zr_color_pick(struct zr_context * ctx, struct zr_color *color,
    enum zr_color_picker_format fmt)
{
    struct zr_window *win;
    struct zr_panel *layout;
    const struct zr_style *config;
    const struct zr_input *in;

    zr_flags ws;
    enum zr_widget_layout_states state;
    struct zr_rect bounds;

    ZR_ASSERT(ctx);
    ZR_ASSERT(color);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout || !color)
        return 0;

    win = ctx->current;
    config = &ctx->style;
    layout = win->layout;
    state = zr_widget(&bounds, ctx);
    if (!state) return 0;
    in = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : &ctx->input;
    return zr_do_color_picker(&ws, &win->buffer, color, fmt, bounds,
                zr_vec2(0,0), in, &config->font);
}

struct zr_color
zr_color_picker(struct zr_context *ctx, struct zr_color color,
    enum zr_color_picker_format fmt)
{
    zr_color_pick(ctx, &color, fmt);
    return color;
}

/* -------------------------------------------------------------
 *
 *                          CHART
 *
 * --------------------------------------------------------------*/
int
zr_chart_begin(struct zr_context *ctx, const enum zr_chart_type type,
    int count, float min_value, float max_value)
{
    struct zr_window *win;
    struct zr_command_buffer *out;
    struct zr_chart *chart;
    const struct zr_style *config;

    const struct zr_style_item *background;
    struct zr_rect bounds = {0, 0, 0, 0};

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout) return 0;
    if (!zr_widget(&bounds, ctx)) {
        chart = &ctx->current->layout->chart;
        chart->style = 0;
        zr_zero(chart, sizeof(*chart));
        return 0;
    }

    win = ctx->current;
    out = &win->buffer;
    config = &ctx->style;
    chart = &win->layout->chart;

    /* setup basic generic chart  */
    zr_zero(chart, sizeof(*chart));
    chart->type = type;
    chart->style = (type == ZR_CHART_LINES) ? &config->line_chart: &config->column_chart;
    chart->index = 0;
    chart->count = count;
    chart->min = ZR_MIN(min_value, max_value);
    chart->max = ZR_MAX(min_value, max_value);
    chart->range = chart->max - chart->min;
    chart->x = bounds.x + chart->style->padding.x;
    chart->y = bounds.y + chart->style->padding.y;
    chart->w = bounds.w - 2 * chart->style->padding.x;
    chart->h = bounds.h - 2 * chart->style->padding.y;
    chart->w = ZR_MAX(chart->w, 2 * chart->style->padding.x);
    chart->h = ZR_MAX(chart->h, 2 * chart->style->padding.y);
    chart->last.x = 0; chart->last.y = 0;

    /* draw chart background */
    background = &chart->style->background;
    if (background->type == ZR_STYLE_ITEM_IMAGE) {
        zr_draw_image(&win->buffer, bounds, &background->data.image);
    } else {
        zr_fill_rect(&win->buffer, bounds, chart->style->rounding, chart->style->border_color);
        zr_fill_rect(&win->buffer, zr_shrink_rect(bounds, chart->style->border),
            chart->style->rounding, chart->style->border_color);
    }
    return 1;
}

static zr_flags
zr_chart_push_line(struct zr_context *ctx, struct zr_window *win,
    struct zr_chart *g, float value)
{
    struct zr_panel *layout = win->layout;
    const struct zr_input *i = &ctx->input;
    struct zr_command_buffer *out = &win->buffer;

    zr_flags ret = 0;
    struct zr_vec2 cur;
    struct zr_rect bounds;
    struct zr_color color;
    float step;
    float range;
    float ratio;

    step = g->w / (float)g->count;
    range = g->max - g->min;
    ratio = (value - g->min) / range;

    if (g->index == 0) {
        /* first data point does not have a connection */
        g->last.x = g->x;
        g->last.y = (g->y + g->h) - ratio * (float)g->h;

        bounds.x = g->last.x - 2;
        bounds.y = g->last.y - 2;
        bounds.w = 4;
        bounds.h = 4;

        color = g->style->color;
        if (!(layout->flags & ZR_WINDOW_ROM) &&
            ZR_INBOX(i->mouse.pos.x,i->mouse.pos.y, g->last.x-3, g->last.y-3, 6, 6)){
            ret = zr_input_is_mouse_hovering_rect(i, bounds) ? ZR_CHART_HOVERING : 0;
            ret |= (i->mouse.buttons[ZR_BUTTON_LEFT].down &&
                i->mouse.buttons[ZR_BUTTON_LEFT].clicked) ? ZR_CHART_CLICKED: 0;
            color = g->style->selected_color;
        }
        zr_fill_rect(out, bounds, 0, color);
        g->index++;
        return ret;
    }

    /* draw a line between the last data point and the new one */
    cur.x = g->x + (float)(step * (float)g->index);
    cur.y = (g->y + g->h) - (ratio * (float)g->h);
    zr_stroke_line(out, g->last.x, g->last.y, cur.x, cur.y, 1.0f, g->style->color);

    bounds.x = cur.x - 3;
    bounds.y = cur.y - 3;
    bounds.w = 6;
    bounds.h = 6;

    /* user selection of current data point */
    color = g->style->color;
    if (!(layout->flags & ZR_WINDOW_ROM)) {
        if (zr_input_is_mouse_hovering_rect(i, bounds)) {
            ret = ZR_CHART_HOVERING;
            ret |= (!i->mouse.buttons[ZR_BUTTON_LEFT].down &&
                i->mouse.buttons[ZR_BUTTON_LEFT].clicked) ? ZR_CHART_CLICKED: 0;
            color = g->style->selected_color;
        }
    }
    zr_fill_rect(out, zr_rect(cur.x - 2, cur.y - 2, 4, 4), 0, color);

    /* save current data point position */
    g->last.x = cur.x;
    g->last.y = cur.y;
    g->index++;
    return ret;
}

static zr_flags
zr_chart_push_column(const struct zr_context *ctx, struct zr_window *win,
    struct zr_chart *chart, float value)
{
    struct zr_command_buffer *out = &win->buffer;
    const struct zr_input *in = &ctx->input;
    struct zr_panel *layout = win->layout;

    float ratio;
    zr_flags ret = 0;
    struct zr_color color;
    struct zr_rect item = {0,0,0,0};

    if (chart->index >= chart->count)
        return zr_false;
    if (chart->count) {
        float padding = (float)(chart->count-1);
        item.w = (chart->w - padding) / (float)(chart->count);
    }

    /* calculate bounds of the current bar chart entry */
    color = chart->style->color;
    item.h = chart->h * ZR_ABS((value/chart->range));
    if (value >= 0) {
        ratio = (value + ZR_ABS(chart->min)) / ZR_ABS(chart->range);
        item.y = (chart->y + chart->h) - chart->h * ratio;
    } else {
        ratio = (value - chart->max) / chart->range;
        item.y = chart->y + (chart->h * ZR_ABS(ratio)) - item.h;
    }
    item.x = chart->x + ((float)chart->index * item.w);
    item.x = item.x + ((float)chart->index);

    /* user chart bar selection */
    if (!(layout->flags & ZR_WINDOW_ROM) &&
        ZR_INBOX(in->mouse.pos.x,in->mouse.pos.y,item.x,item.y,item.w,item.h)) {
        ret = ZR_CHART_HOVERING;
        ret |= (!in->mouse.buttons[ZR_BUTTON_LEFT].down &&
                in->mouse.buttons[ZR_BUTTON_LEFT].clicked) ? ZR_CHART_CLICKED: 0;
        color = chart->style->selected_color;
    }
    zr_fill_rect(out, item, 0, color);
    chart->index++;
    return ret;
}

zr_flags
zr_chart_push(struct zr_context *ctx, float value)
{
    zr_flags flags;
    struct zr_window *win;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    if (!ctx || !ctx->current || !ctx->current->layout->chart.style)
        return zr_false;

    win = ctx->current;
    switch (win->layout->chart.type) {
    case ZR_CHART_LINES:
        flags = zr_chart_push_line(ctx, win, &win->layout->chart, value); break;
    case ZR_CHART_COLUMN:
        flags = zr_chart_push_column(ctx, win, &win->layout->chart, value); break;
    default:
    case ZR_CHART_MAX:
        flags = 0;
    }
    return flags;
}

void
zr_chart_end(struct zr_context *ctx)
{
    struct zr_window *win;
    struct zr_chart *chart;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    if (!ctx || !ctx->current)
        return;

    win = ctx->current;
    chart = &win->layout->chart;
    chart->type = ZR_CHART_MAX;
    chart->index = 0;
    chart->count = 0;
    chart->min = 0;
    chart->max = 0;
    chart->x = 0;
    chart->y = 0;
    chart->w = 0;
    chart->h = 0;
    return;
}

/* -------------------------------------------------------------
 *
 *                          GROUP
 *
 * --------------------------------------------------------------*/
int
zr_group_begin(struct zr_context *ctx, struct zr_panel *layout, const char *title,
    zr_flags flags)
{
    struct zr_window *win;
    const struct zr_rect *c;
    union {struct zr_scroll *s; zr_uint *i;} value;
    struct zr_window panel;
    struct zr_rect bounds;
    zr_hash title_hash;
    int title_len;

    ZR_ASSERT(ctx);
    ZR_ASSERT(title);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout || !title)
        return 0;

    /* allocate space for the group panel inside the panel */
    win = ctx->current;
    c = &win->layout->clip;
    zr_panel_alloc_space(&bounds, ctx);
    zr_zero(layout, sizeof(*layout));

    /* find group persistent scrollbar value */
    title_len = (int)zr_strsiz(title);
    title_hash = zr_murmur_hash(title, (int)title_len, ZR_WINDOW_SUB);
    value.i = zr_find_value(win, title_hash);
    if (!value.i) {
        value.i = zr_add_value(ctx, win, title_hash, 0);
        *value.i = 0;
    }
    if (!ZR_INTERSECT(c->x, c->y, c->w, c->h, bounds.x, bounds.y, bounds.w, bounds.h) &&
        !(flags & ZR_WINDOW_MOVABLE)) {
        return 0;
    }

    flags |= ZR_WINDOW_SUB;
    if (win->flags & ZR_WINDOW_ROM)
        flags |= ZR_WINDOW_ROM;

    /* initialize a fake window to create the layout from */
    zr_zero(&panel, sizeof(panel));
    panel.bounds = bounds;
    panel.flags = flags;
    panel.scrollbar.x = (unsigned short)value.s->x;
    panel.scrollbar.y = (unsigned short)value.s->y;
    panel.buffer = win->buffer;
    panel.layout = layout;
    ctx->current = &panel;
    zr_panel_begin(ctx, (flags & ZR_WINDOW_TITLE) ? title: 0);

    win->buffer = panel.buffer;
    layout->offset = value.s;
    layout->parent = win->layout;
    win->layout = layout;
    ctx->current = win;
    return 1;
}

void
zr_group_end(struct zr_context *ctx)
{
    struct zr_window *win;
    struct zr_panel *parent;
    struct zr_panel *g;

    struct zr_rect clip;
    struct zr_window pan;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    if (!ctx || !ctx->current)
        return;

    /* make sure zr_group_begin was called correctly */
    ZR_ASSERT(ctx->current);
    win = ctx->current;
    ZR_ASSERT(win->layout);
    g = win->layout;
    ZR_ASSERT(g->parent);
    parent = g->parent;

    /* dummy window */
    zr_zero(&pan, sizeof(pan));
    pan.bounds = g->bounds;
    pan.scrollbar.x = (unsigned short)g->offset->x;
    pan.scrollbar.y = (unsigned short)g->offset->y;
    pan.flags = g->flags|ZR_WINDOW_SUB;
    pan.buffer = win->buffer;
    pan.layout = g;
    ctx->current = &pan;

    /* make sure group has correct clipping rectangle */
    zr_unify(&clip, &parent->clip,
        g->bounds.x, g->clip.y - g->header_h,
        g->bounds.x + g->bounds.w+1,
        g->bounds.y + g->bounds.h + 1);
    zr_push_scissor(&pan.buffer, clip);
    zr_end(ctx);

    win->buffer = pan.buffer;
    zr_push_scissor(&win->buffer, parent->clip);
    ctx->current = win;
    win->layout = parent;
    win->bounds = parent->bounds;
    return;
}

/* --------------------------------------------------------------
 *
 *                          POPUP
 *
 * --------------------------------------------------------------*/
int
zr_popup_begin(struct zr_context *ctx, struct zr_panel *layout,
    enum zr_popup_type type, const char *title, zr_flags flags, struct zr_rect rect)
{
    struct zr_window *popup;
    struct zr_window *win;

    int title_len;
    zr_hash title_hash;
    zr_size allocated;

    ZR_ASSERT(ctx);
    ZR_ASSERT(title);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    ZR_ASSERT(!(win->flags & ZR_WINDOW_POPUP));
    title_len = (int)zr_strsiz(title);
    title_hash = zr_murmur_hash(title, (int)title_len, ZR_WINDOW_POPUP);

    popup = win->popup.win;
    if (!popup) {
        popup = (struct zr_window*)zr_create_window(ctx);
        win->popup.win = popup;
        win->popup.active = 0;
    }

    /* make sure we have to correct popup */
    if (win->popup.name != title_hash) {
        if (!win->popup.active) {
            zr_zero(popup, sizeof(*popup));
            win->popup.name = title_hash;
            win->popup.active = 1;
        } else return 0;
    }

    /* popup position is local to window */
    ctx->current = popup;
    rect.x += win->layout->clip.x;
    rect.y += win->layout->clip.y;

    /* setup popup data */
    popup->parent = win;
    popup->bounds = rect;
    popup->seq = ctx->seq;
    popup->layout = layout;
    popup->flags = flags;
    popup->flags |= ZR_WINDOW_BORDER|ZR_WINDOW_SUB|ZR_WINDOW_POPUP;
    if (type == ZR_POPUP_DYNAMIC)
        popup->flags |= ZR_WINDOW_DYNAMIC;

    popup->buffer = win->buffer;
    zr_start_popup(ctx, win);
    allocated = ctx->memory.allocated;
    zr_push_scissor(&popup->buffer, zr_null_rect);

    if (zr_panel_begin(ctx, title)) {
        /* popup is running therefore invalidate parent window  */
        win->layout->flags |= ZR_WINDOW_ROM;
        win->layout->flags &= ~(zr_flags)ZR_WINDOW_REMOVE_ROM;
        win->popup.active = 1;
        layout->offset = &popup->scrollbar;
        return 1;
    } else {
        /* popup was closed/is invalid so cleanup */
        win->layout->flags |= ZR_WINDOW_REMOVE_ROM;
        win->layout->popup_buffer.active = 0;
        win->popup.active = 0;
        ctx->memory.allocated = allocated;
        ctx->current = win;
        return 0;
    }
}

static int
zr_nonblock_begin(struct zr_panel *layout, struct zr_context *ctx,
    zr_flags flags, struct zr_rect body, struct zr_rect header)
{
    struct zr_window *popup;
    struct zr_window *win;
    int is_active = zr_true;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    /* popups cannot have popups */
    win = ctx->current;
    ZR_ASSERT(!(win->flags & ZR_WINDOW_POPUP));
    popup = win->popup.win;
    if (!popup) {
        /* create window for nonblocking popup */
        popup = (struct zr_window*)zr_create_window(ctx);
        win->popup.win = popup;
        zr_command_buffer_init(&popup->buffer, &ctx->memory, ZR_CLIPPING_ON);
    } else {
        /* check if user clicked outside the popup and close if so */
        int in_panel, in_body, in_header;
        in_panel = zr_input_is_mouse_click_in_rect(&ctx->input, ZR_BUTTON_LEFT, win->layout->bounds);
        in_body = zr_input_is_mouse_click_in_rect(&ctx->input, ZR_BUTTON_LEFT, body);
        in_header = zr_input_is_mouse_click_in_rect(&ctx->input, ZR_BUTTON_LEFT, header);
        if (!in_body && in_panel && !in_header)
            is_active = zr_false;
    }

    if (!is_active) {
        win->layout->flags |= ZR_WINDOW_REMOVE_ROM;
        return is_active;
    }

    popup->bounds = body;
    popup->parent = win;
    popup->layout = layout;
    popup->flags = flags;
    popup->flags |= ZR_WINDOW_BORDER|ZR_WINDOW_POPUP;
    popup->flags |= ZR_WINDOW_DYNAMIC|ZR_WINDOW_SUB;
    popup->flags |= ZR_WINDOW_NONBLOCK;
    popup->seq = ctx->seq;
    win->popup.active = 1;

    zr_start_popup(ctx, win);
    popup->buffer = win->buffer;
    zr_push_scissor(&popup->buffer, zr_null_rect);
    ctx->current = popup;

    zr_panel_begin(ctx, 0);
    win->buffer = popup->buffer;
    win->layout->flags |= ZR_WINDOW_ROM;
    layout->offset = &popup->scrollbar;
    return is_active;
}

void
zr_popup_close(struct zr_context *ctx)
{
    struct zr_window *popup;
    ZR_ASSERT(ctx);
    if (!ctx || !ctx->current) return;

    popup = ctx->current;
    ZR_ASSERT(popup->parent);
    ZR_ASSERT(popup->flags & ZR_WINDOW_POPUP);
    popup->flags |= ZR_WINDOW_HIDDEN;
}

void
zr_popup_end(struct zr_context *ctx)
{
    struct zr_window *win;
    struct zr_window *popup;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    popup = ctx->current;
    ZR_ASSERT(popup->parent);
    win = popup->parent;
    if (popup->flags & ZR_WINDOW_HIDDEN) {
        win->layout->flags |= ZR_WINDOW_REMOVE_ROM;
        win->popup.active = 0;
    }
    zr_push_scissor(&popup->buffer, zr_null_rect);
    zr_end(ctx);

    win->buffer = popup->buffer;
    zr_finish_popup(ctx, win);
    ctx->current = win;
    zr_push_scissor(&win->buffer, win->layout->clip);
}
/* -------------------------------------------------------------
 *
 *                          TOOLTIP
 *
 * -------------------------------------------------------------- */
int
zr_tooltip_begin(struct zr_context *ctx, struct zr_panel *layout, float width)
{
    struct zr_window *win;
    const struct zr_input *in;
    struct zr_rect bounds;
    int ret;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    /* make sure that no nonblocking popup is currently active */
    win = ctx->current;
    in = &ctx->input;
    if (win->popup.win && (win->popup.win->flags & ZR_WINDOW_NONBLOCK))
        return 0;

    bounds.w = width;
    bounds.h = zr_null_rect.h;
    bounds.x = (in->mouse.pos.x + 1) - win->layout->clip.x;
    bounds.y = (in->mouse.pos.y + 1) - win->layout->clip.y;

    ret = zr_popup_begin(ctx, layout, ZR_POPUP_DYNAMIC,
        "__##Tooltip##__", ZR_WINDOW_NO_SCROLLBAR|ZR_WINDOW_TOOLTIP, bounds);
    if (ret) win->layout->flags &= ~(zr_flags)ZR_WINDOW_ROM;
    return ret;
}

void
zr_tooltip_end(struct zr_context *ctx)
{
    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    if (!ctx || !ctx->current)
        return;
    zr_popup_close(ctx);
    zr_popup_end(ctx);
}

void
zr_tooltip(struct zr_context *ctx, const char *text)
{
    const struct zr_style *style;
    struct zr_vec2 padding;
    struct zr_panel layout;

    zr_size text_len;
    zr_size text_width;
    zr_size text_height;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    ZR_ASSERT(text);
    if (!ctx || !ctx->current || !ctx->current->layout || !text)
        return;

    /* fetch configuration data */
    style = &ctx->style;
    padding = style->window.padding;

    /* calculate size of the text and tooltip */
    text_len = zr_strsiz(text);
    text_width = style->font.width(style->font.userdata,
                        style->font.height, text, text_len);
    text_width += (zr_size)(4 * padding.x);
    text_height = (zr_size)(style->font.height + 2 * padding.y);

    /* execute tooltip and fill with text */
    if (zr_tooltip_begin(ctx, &layout, (float)text_width)) {
        zr_layout_row_dynamic(ctx, (float)text_height, 1);
        zr_text(ctx, text, text_len, ZR_TEXT_LEFT);
        zr_tooltip_end(ctx);
    }
}

/* -------------------------------------------------------------
 *
 *                          CONTEXTUAL
 *
 * -------------------------------------------------------------- */
int
zr_contextual_begin(struct zr_context *ctx, struct zr_panel *layout,
    zr_flags flags, struct zr_vec2 size, struct zr_rect trigger_bounds)
{
    struct zr_window *win;
    struct zr_window *popup;
    struct zr_rect body;

    static const struct zr_rect null_rect = {0,0,0,0};
    int is_clicked = 0;
    int is_active = 0;
    int is_open = 0;
    int ret = 0;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    ++win->popup.con_count;

    /* check if currently active contextual is active */
    popup = win->popup.win;
    is_open = (popup && (popup->flags & ZR_WINDOW_CONTEXTUAL) && win->popup.type == ZR_WINDOW_CONTEXTUAL);
    is_clicked = zr_input_mouse_clicked(&ctx->input, ZR_BUTTON_RIGHT, trigger_bounds);
    if (win->popup.active_con && win->popup.con_count != win->popup.active_con)
        return 0;
    if ((is_clicked && is_open && !is_active) || (!is_open && !is_active && !is_clicked))
        return 0;

    /* calculate contextual position on click */
    win->popup.active_con = win->popup.con_count;
    if (is_clicked) {
        body.x = ctx->input.mouse.pos.x;
        body.y = ctx->input.mouse.pos.y;
    } else {
        body.x = popup->bounds.x;
        body.y = popup->bounds.y;
    }
    body.w = size.x;
    body.h = size.y;

    /* start nonblocking contextual popup */
    ret = zr_nonblock_begin(layout, ctx,
            flags|ZR_WINDOW_CONTEXTUAL|ZR_WINDOW_NO_SCROLLBAR, body, null_rect);
    if (ret) win->popup.type = ZR_WINDOW_CONTEXTUAL;
    else {
        win->popup.active_con = 0;
        win->popup.win->flags = 0;
    }
    return ret;
}

int
zr_contextual_item_text(struct zr_context *ctx, const char *text, zr_size len,
    zr_flags alignment)
{
    struct zr_window *win;
    struct zr_panel *layout;
    const struct zr_input *in;
    const struct zr_style *style;

    struct zr_rect bounds;
    enum zr_widget_layout_states state;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;
    state = zr_widget_fitting(&bounds, ctx, style->contextual_button.padding);
    if (!state) return zr_false;
    in = (state == ZR_WIDGET_ROM || win->layout->flags & ZR_WINDOW_ROM) ? 0 : &ctx->input;
    if (zr_do_button_text(&ctx->last_widget_state, &win->buffer, bounds,
        text, len, alignment, ZR_BUTTON_DEFAULT, &style->contextual_button, in, &style->font)) {
        zr_contextual_close(ctx);
        return zr_true;
    }
    return zr_false;
}

int zr_contextual_item_label(struct zr_context *ctx, const char *label, zr_flags align)
{return zr_contextual_item_text(ctx, label, zr_strsiz(label), align);}

int
zr_contextual_item_image_text(struct zr_context *ctx, struct zr_image img,
    const char *text, zr_size len, zr_flags align)
{
    struct zr_window *win;
    struct zr_panel *layout;
    const struct zr_input *in;
    const struct zr_style *style;

    struct zr_rect bounds;
    enum zr_widget_layout_states state;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;
    state = zr_widget_fitting(&bounds, ctx, style->contextual_button.padding);
    if (!state) return zr_false;
    in = (state == ZR_WIDGET_ROM || win->layout->flags & ZR_WINDOW_ROM) ? 0 : &ctx->input;
    if (zr_do_button_text_image(&ctx->last_widget_state, &win->buffer, bounds,
        img, text, len, align, ZR_BUTTON_DEFAULT, &style->contextual_button, &style->font, in)){
        zr_contextual_close(ctx);
        return zr_true;
    }
    return zr_false;
}

int zr_contextual_item_image_label(struct zr_context *ctx, struct zr_image img,
    const char *label, zr_flags align)
{return zr_contextual_item_image_text(ctx, img, label, zr_strsiz(label), align);}

int
zr_contextual_item_symbol_text(struct zr_context *ctx, enum zr_symbol_type symbol,
    const char *text, zr_size len, zr_flags align)
{
    struct zr_window *win;
    struct zr_panel *layout;
    const struct zr_input *in;
    const struct zr_style *style;

    struct zr_rect bounds;
    enum zr_widget_layout_states state;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    layout = win->layout;
    state = zr_widget_fitting(&bounds, ctx, style->contextual_button.padding);
    if (!state) return zr_false;
    in = (state == ZR_WIDGET_ROM || win->layout->flags & ZR_WINDOW_ROM) ? 0 : &ctx->input;
    if (zr_do_button_text_symbol(&ctx->last_widget_state, &win->buffer, bounds,
        symbol, text, len, align, ZR_BUTTON_DEFAULT, &style->contextual_button, &style->font, in)) {
        zr_contextual_close(ctx);
        return zr_true;
    }
    return zr_false;
}

int zr_contextual_item_symbol_label(struct zr_context *ctx, enum zr_symbol_type symbol,
    const char *text, zr_flags align)
{return zr_contextual_item_symbol_text(ctx, symbol, text, zr_strsiz(text), align);}

void
zr_contextual_close(struct zr_context *ctx)
{
    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    if (!ctx->current)
        return;
    zr_popup_close(ctx);
}

void
zr_contextual_end(struct zr_context *ctx)
{
    struct zr_window *popup;
    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    popup = ctx->current;
    ZR_ASSERT(popup->parent);
    if (popup->flags & ZR_WINDOW_HIDDEN)
        popup->seq = 0;
    zr_popup_end(ctx);
    return;
}
/* -------------------------------------------------------------
 *
 *                          COMBO
 *
 * --------------------------------------------------------------*/
static int
zr_combo_begin(struct zr_panel *layout, struct zr_context *ctx, struct zr_window *win,
    int height, int is_clicked, struct zr_rect header)
{
    struct zr_window *popup;
    int is_open = 0;
    int is_active = 0;
    struct zr_rect body;
    zr_hash hash;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    popup = win->popup.win;
    body.x = header.x;
    body.w = header.w;
    body.y = header.y + header.h-1;
    body.h = (float)height;

    hash = win->popup.combo_count++;
    is_open = (popup && (popup->flags & ZR_WINDOW_COMBO));
    is_active = (popup && (win->popup.name == hash) && win->popup.type == ZR_WINDOW_COMBO);
    if ((is_clicked && is_open && !is_active) || (is_open && !is_active) ||
        (!is_open && !is_active && !is_clicked)) return 0;
    if (!zr_nonblock_begin(layout, ctx, ZR_WINDOW_COMBO,
            body, zr_rect(0,0,0,0))) return 0;

    win->popup.type = ZR_WINDOW_COMBO;
    win->popup.name = hash;
    return 1;
}

int
zr_combo_begin_text(struct zr_context *ctx, struct zr_panel *layout,
    const char *selected, zr_size len, int height)
{
    const struct zr_input *in;
    struct zr_window *win;
    struct zr_style *style;

    enum zr_widget_layout_states s;
    int is_active = zr_false;
    struct zr_rect header;

    const struct zr_style_item *background;
    struct zr_text text;

    ZR_ASSERT(ctx);
    ZR_ASSERT(selected);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout || !selected)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    s = zr_widget(&header, ctx);
    if (s == ZR_WIDGET_INVALID)
        return 0;

    in = (win->layout->flags & ZR_WINDOW_ROM || s == ZR_WIDGET_ROM)? 0: &ctx->input;
    if (zr_button_behavior(&ctx->last_widget_state, header, in, ZR_BUTTON_DEFAULT))
        is_active = zr_true;

    /* draw combo box header background and border */
    if (ctx->last_widget_state & ZR_WIDGET_STATE_ACTIVE) {
        background = &style->combo.active;
        text.text = style->combo.label_active;
    } else if (ctx->last_widget_state & ZR_WIDGET_STATE_HOVERED) {
        background = &style->combo.hover;
        text.text = style->combo.label_hover;
    } else {
        background = &style->combo.normal;
        text.text = style->combo.label_normal;
    }
    if (background->type == ZR_STYLE_ITEM_IMAGE) {
        text.background = zr_rgba(0,0,0,0);
        zr_draw_image(&win->buffer, header, &background->data.image);
    } else {
        text.background = background->data.color;
        zr_fill_rect(&win->buffer, header, style->combo.rounding, style->combo.border_color);
        zr_fill_rect(&win->buffer, zr_shrink_rect(header, 1), style->combo.rounding,
            background->data.color);
    }
    {
        /* print currently selected text item */
        struct zr_rect label;
        struct zr_rect button;
        struct zr_rect content;

        enum zr_symbol_type sym;
        if (ctx->last_widget_state & ZR_WIDGET_STATE_HOVERED)
            sym = style->combo.sym_hover;
        else if (is_active)
            sym = style->combo.sym_active;
        else sym = style->combo.sym_normal;

        /* calculate button */
        button.w = header.h - 2 * style->combo.button_padding.y;
        button.x = (header.x + header.w - header.h) - style->combo.button_padding.x;
        button.y = header.y + style->combo.button_padding.y;
        button.h = button.w;

        content.x = button.x + style->combo.button.padding.x;
        content.y = button.y + style->combo.button.padding.y;
        content.w = button.w - 2 * style->combo.button.padding.x;
        content.h = button.h - 2 * style->combo.button.padding.y;

        /* draw selected label */
        text.padding = zr_vec2(0,0);
        label.x = header.x + style->combo.content_padding.x;
        label.y = header.y + style->combo.content_padding.y;
        label.w = button.x - (style->combo.content_padding.x + style->combo.spacing.x) - label.x;;
        label.h = header.h - 2 * style->combo.content_padding.y;
        zr_widget_text(&win->buffer, label, selected, len, &text,
            ZR_TEXT_LEFT, &ctx->style.font);

        /* draw open/close button */
        zr_draw_button_symbol(&win->buffer, &button, &content, ctx->last_widget_state,
            &ctx->style.combo.button, sym, &style->font);
    }
    return zr_combo_begin(layout, ctx, win, height, is_active, header);
}

int zr_combo_begin_label(struct zr_context *ctx, struct zr_panel *layout,
    const char *selected, int max_height)
{return zr_combo_begin_text(ctx, layout, selected, zr_strsiz(selected), max_height);}

int
zr_combo_begin_color(struct zr_context *ctx, struct zr_panel *layout,
    struct zr_color color, int height)
{
    struct zr_window *win;
    struct zr_style *style;
    const struct zr_input *in;

    struct zr_rect header;
    int is_active = zr_false;
    enum zr_widget_layout_states s;
    const struct zr_style_item *background;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    s = zr_widget(&header, ctx);
    if (s == ZR_WIDGET_INVALID)
        return 0;

    in = (win->layout->flags & ZR_WINDOW_ROM || s == ZR_WIDGET_ROM)? 0: &ctx->input;
    if (zr_button_behavior(&ctx->last_widget_state, header, in, ZR_BUTTON_DEFAULT))
        is_active = zr_true;

    /* draw combo box header background and border */
    if (ctx->last_widget_state & ZR_WIDGET_STATE_ACTIVE)
        background = &style->combo.active;
    else if (ctx->last_widget_state & ZR_WIDGET_STATE_HOVERED)
        background = &style->combo.hover;
    else background = &style->combo.normal;

    if (background->type == ZR_STYLE_ITEM_IMAGE) {
        zr_draw_image(&win->buffer, header, &background->data.image);
    } else {
        zr_fill_rect(&win->buffer, header, 0, style->combo.border_color);
        zr_fill_rect(&win->buffer, zr_shrink_rect(header, 1), 0,
            background->data.color);
    }
    {
        struct zr_rect content;
        struct zr_rect button;
        struct zr_rect bounds;

        enum zr_symbol_type sym;
        if (ctx->last_widget_state & ZR_WIDGET_STATE_HOVERED)
            sym = style->combo.sym_hover;
        else if (is_active)
            sym = style->combo.sym_active;
        else sym = style->combo.sym_normal;

        /* calculate button */
        button.w = header.h - 2 * style->combo.button_padding.y;
        button.x = (header.x + header.w - header.h) - style->combo.button_padding.x;
        button.y = header.y + style->combo.button_padding.y;
        button.h = button.w;

        content.x = button.x + style->combo.button.padding.x;
        content.y = button.y + style->combo.button.padding.y;
        content.w = button.w - 2 * style->combo.button.padding.x;
        content.h = button.h - 2 * style->combo.button.padding.y;

        /* draw color */
        bounds.h = header.h - 4 * style->combo.content_padding.y;
        bounds.y = header.y + 2 * style->combo.content_padding.y;
        bounds.x = header.x + 2 * style->combo.content_padding.x;
        bounds.w = (button.x - (style->combo.content_padding.x + style->combo.spacing.x)) - bounds.x;
        zr_fill_rect(&win->buffer, bounds, 0, color);

        /* draw open/close button */
        zr_draw_button_symbol(&win->buffer, &button, &content, ctx->last_widget_state,
            &ctx->style.combo.button, sym, &style->font);
    }
    return zr_combo_begin(layout, ctx, win, height, is_active, header);
}

int
zr_combo_begin_symbol(struct zr_context *ctx, struct zr_panel *layout,
    enum zr_symbol_type symbol, int height)
{
    struct zr_window *win;
    struct zr_style *style;
    const struct zr_input *in;

    struct zr_rect header;
    int is_active = zr_false;
    enum zr_widget_layout_states s;
    const struct zr_style_item *background;
    struct zr_color sym_background;
    struct zr_color symbol_color;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    s = zr_widget(&header, ctx);
    if (s == ZR_WIDGET_INVALID)
        return 0;

    in = (win->layout->flags & ZR_WINDOW_ROM || s == ZR_WIDGET_ROM)? 0: &ctx->input;
    if (zr_button_behavior(&ctx->last_widget_state, header, in, ZR_BUTTON_DEFAULT))
        is_active = zr_true;

    /* draw combo box header background and border */
    if (ctx->last_widget_state & ZR_WIDGET_STATE_ACTIVE) {
        background = &style->combo.active;
        symbol_color = style->combo.symbol_active;
    } else if (ctx->last_widget_state & ZR_WIDGET_STATE_HOVERED) {
        background = &style->combo.hover;
        symbol_color = style->combo.symbol_hover;
    } else {
        background = &style->combo.normal;
        symbol_color = style->combo.symbol_hover;
    }

    if (background->type == ZR_STYLE_ITEM_IMAGE) {
        sym_background = zr_rgba(0,0,0,0);
        zr_draw_image(&win->buffer, header, &background->data.image);
    } else {
        sym_background = background->data.color;
        zr_fill_rect(&win->buffer, header, 0, style->combo.border_color);
        zr_fill_rect(&win->buffer, zr_shrink_rect(header, 1), 0,
            background->data.color);
    }
    {
        struct zr_rect bounds = {0,0,0,0};
        struct zr_rect content;
        struct zr_rect button;

        enum zr_symbol_type sym;
        if (ctx->last_widget_state & ZR_WIDGET_STATE_HOVERED)
            sym = style->combo.sym_hover;
        else if (is_active)
            sym = style->combo.sym_active;
        else sym = style->combo.sym_normal;

        /* calculate button */
        button.w = header.h - 2 * style->combo.button_padding.y;
        button.x = (header.x + header.w - header.h) - style->combo.button_padding.y;
        button.y = header.y + style->combo.button_padding.y;
        button.h = button.w;

        content.x = button.x + style->combo.button.padding.x;
        content.y = button.y + style->combo.button.padding.y;
        content.w = button.w - 2 * style->combo.button.padding.x;
        content.h = button.h - 2 * style->combo.button.padding.y;

        /* draw symbol */
        bounds.h = header.h - 2 * style->combo.content_padding.y;
        bounds.y = header.y + style->combo.content_padding.y;
        bounds.x = header.x + style->combo.content_padding.x;
        bounds.w = (button.x - style->combo.content_padding.y) - bounds.x;
        zr_draw_symbol(&win->buffer, symbol, bounds, sym_background, symbol_color,
            1.0f, &style->font);

        /* draw open/close button */
        zr_draw_button_symbol(&win->buffer, &bounds, &content, ctx->last_widget_state,
            &ctx->style.combo.button, sym, &style->font);
    }
    return zr_combo_begin(layout, ctx, win, height, is_active, header);
}

int
zr_combo_begin_symbol_text(struct zr_context *ctx, struct zr_panel *layout,
    const char *selected, zr_size len, enum zr_symbol_type symbol, int height)
{
    struct zr_window *win;
    struct zr_style *style;
    struct zr_input *in;

    struct zr_rect header;
    int is_active = zr_false;
    enum zr_widget_layout_states s;
    const struct zr_style_item *background;
    struct zr_color symbol_color;
    struct zr_text text;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    s = zr_widget(&header, ctx);
    if (!s) return 0;

    in = (win->layout->flags & ZR_WINDOW_ROM || s == ZR_WIDGET_ROM)? 0: &ctx->input;
    if (zr_button_behavior(&ctx->last_widget_state, header, in, ZR_BUTTON_DEFAULT))
        is_active = zr_true;

    /* draw combo box header background and border */
    if (ctx->last_widget_state & ZR_WIDGET_STATE_ACTIVE) {
        background = &style->combo.active;
        symbol_color = style->combo.symbol_active;
        text.text = style->combo.label_active;
    } else if (ctx->last_widget_state & ZR_WIDGET_STATE_HOVERED) {
        background = &style->combo.hover;
        symbol_color = style->combo.symbol_hover;
        text.text = style->combo.label_hover;
    } else {
        background = &style->combo.normal;
        symbol_color = style->combo.symbol_normal;
        text.text = style->combo.label_normal;
    }
    if (background->type == ZR_STYLE_ITEM_IMAGE) {
        text.background = zr_rgba(0,0,0,0);
        zr_draw_image(&win->buffer, header, &background->data.image);
    } else {
        text.background = background->data.color;
        zr_fill_rect(&win->buffer, header, 0, style->combo.border_color);
        zr_fill_rect(&win->buffer, zr_shrink_rect(header, 1), 0,
            background->data.color);
    }
    {
        struct zr_rect content;
        struct zr_rect button;
        struct zr_rect label;
        struct zr_rect image;

        enum zr_symbol_type sym;
        if (ctx->last_widget_state & ZR_WIDGET_STATE_HOVERED)
            sym = style->combo.sym_hover;
        else if (is_active)
            sym = style->combo.sym_active;
        else sym = style->combo.sym_normal;

        /* calculate button */
        button.w = header.h - 2 * style->combo.button_padding.y;
        button.x = (header.x + header.w - header.h) - style->combo.button_padding.x;
        button.y = header.y + style->combo.button_padding.y;
        button.h = button.w;

        content.x = button.x + style->combo.button.padding.x;
        content.y = button.y + style->combo.button.padding.y;
        content.w = button.w - 2 * style->combo.button.padding.x;
        content.h = button.h - 2 * style->combo.button.padding.y;
        zr_draw_button_symbol(&win->buffer, &button, &content, ctx->last_widget_state,
            &ctx->style.combo.button, sym, &style->font);

        /* draw symbol */
        image.x = header.x + style->combo.content_padding.x;
        image.y = header.y + style->combo.content_padding.y;
        image.h = header.h - 2 * style->combo.content_padding.y;
        image.w = image.h;
        zr_draw_symbol(&win->buffer, symbol, image, text.background, symbol_color,
            1.0f, &style->font);

        /* draw label */
        text.padding = zr_vec2(0,0);
        label.x = image.x + image.w + style->combo.spacing.x + style->combo.content_padding.x;
        label.y = header.y + style->combo.content_padding.y;
        label.w = (button.x - style->combo.content_padding.x) - label.x;
        label.h = header.h - 2 * style->combo.content_padding.y;
        zr_widget_text(&win->buffer, label, selected, len, &text, ZR_TEXT_LEFT, &style->font);
    }
    return zr_combo_begin(layout, ctx, win, height, is_active, header);
}


int
zr_combo_begin_image(struct zr_context *ctx, struct zr_panel *layout,
    struct zr_image img, int height)
{
    struct zr_window *win;
    struct zr_style *style;
    const struct zr_input *in;

    struct zr_rect header;
    int is_active = zr_false;
    enum zr_widget_layout_states s;
    const struct zr_style_item *background;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    s = zr_widget(&header, ctx);
    if (s == ZR_WIDGET_INVALID)
        return 0;

    in = (win->layout->flags & ZR_WINDOW_ROM || s == ZR_WIDGET_ROM)? 0: &ctx->input;
    if (zr_button_behavior(&ctx->last_widget_state, header, in, ZR_BUTTON_DEFAULT))
        is_active = zr_true;

    /* draw combo box header background and border */
    if (ctx->last_widget_state & ZR_WIDGET_STATE_ACTIVE)
        background = &style->combo.active;
    else if (ctx->last_widget_state & ZR_WIDGET_STATE_HOVERED)
        background = &style->combo.hover;
    else background = &style->combo.normal;

    if (background->type == ZR_STYLE_ITEM_IMAGE) {
        zr_draw_image(&win->buffer, header, &background->data.image);
    } else {
        zr_fill_rect(&win->buffer, header, 0, style->combo.border_color);
        zr_fill_rect(&win->buffer, zr_shrink_rect(header, 1), 0,
            background->data.color);
    }
    {
        struct zr_rect bounds = {0,0,0,0};
        struct zr_rect content;
        struct zr_rect button;

        enum zr_symbol_type sym;
        if (ctx->last_widget_state & ZR_WIDGET_STATE_HOVERED)
            sym = style->combo.sym_hover;
        else if (is_active)
            sym = style->combo.sym_active;
        else sym = style->combo.sym_normal;

        /* calculate button */
        button.w = header.h - 2 * style->combo.button_padding.y;
        button.x = (header.x + header.w - header.h) - style->combo.button_padding.y;
        button.y = header.y + style->combo.button_padding.y;
        button.h = button.w;

        content.x = button.x + style->combo.button.padding.x;
        content.y = button.y + style->combo.button.padding.y;
        content.w = button.w - 2 * style->combo.button.padding.x;
        content.h = button.h - 2 * style->combo.button.padding.y;

        /* draw image */
        bounds.h = header.h - 2 * style->combo.content_padding.y;
        bounds.y = header.y + style->combo.content_padding.y;
        bounds.x = header.x + style->combo.content_padding.x;
        bounds.w = (button.x - style->combo.content_padding.y) - bounds.x;
        zr_draw_image(&win->buffer, bounds, &img);

        /* draw open/close button */
        zr_draw_button_symbol(&win->buffer, &bounds, &content, ctx->last_widget_state,
            &ctx->style.combo.button, sym, &style->font);
    }
    return zr_combo_begin(layout, ctx, win, height, is_active, header);
}

int
zr_combo_begin_image_text(struct zr_context *ctx, struct zr_panel *layout,
    const char *selected, zr_size len, struct zr_image img, int height)
{
    struct zr_window *win;
    struct zr_style *style;
    struct zr_input *in;

    struct zr_rect header;
    int is_active = zr_false;
    enum zr_widget_layout_states s;
    const struct zr_style_item *background;
    struct zr_text text;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    s = zr_widget(&header, ctx);
    if (!s) return 0;

    in = (win->layout->flags & ZR_WINDOW_ROM || s == ZR_WIDGET_ROM)? 0: &ctx->input;
    if (zr_button_behavior(&ctx->last_widget_state, header, in, ZR_BUTTON_DEFAULT))
        is_active = zr_true;

    /* draw combo box header background and border */
    if (ctx->last_widget_state & ZR_WIDGET_STATE_ACTIVE) {
        background = &style->combo.active;
        text.text = style->combo.label_active;
    } else if (ctx->last_widget_state & ZR_WIDGET_STATE_HOVERED) {
        background = &style->combo.hover;
        text.text = style->combo.label_hover;
    } else {
        background = &style->combo.normal;
        text.text = style->combo.label_normal;
    }
    if (background->type == ZR_STYLE_ITEM_IMAGE) {
        text.background = zr_rgba(0,0,0,0);
        zr_draw_image(&win->buffer, header, &background->data.image);
    } else {
        text.background = background->data.color;
        zr_fill_rect(&win->buffer, header, 0, style->combo.border_color);
        zr_fill_rect(&win->buffer, zr_shrink_rect(header, 1), 0,
            background->data.color);
    }

    {
        struct zr_rect content;
        struct zr_rect button;
        struct zr_rect label;
        struct zr_rect image;

        enum zr_symbol_type sym;
        if (ctx->last_widget_state & ZR_WIDGET_STATE_HOVERED)
            sym = style->combo.sym_hover;
        else if (is_active)
            sym = style->combo.sym_active;
        else sym = style->combo.sym_normal;

        /* calculate button */
        button.w = header.h - 2 * style->combo.button_padding.y;
        button.x = (header.x + header.w - header.h) - style->combo.button_padding.x;
        button.y = header.y + style->combo.button_padding.y;
        button.h = button.w;

        content.x = button.x + style->combo.button.padding.x;
        content.y = button.y + style->combo.button.padding.y;
        content.w = button.w - 2 * style->combo.button.padding.x;
        content.h = button.h - 2 * style->combo.button.padding.y;
        zr_draw_button_symbol(&win->buffer, &button, &content, ctx->last_widget_state,
            &ctx->style.combo.button, sym, &style->font);

        /* draw image */
        image.x = header.x + style->combo.content_padding.x;
        image.y = header.y + style->combo.content_padding.y;
        image.h = header.h - 2 * style->combo.content_padding.y;
        image.w = image.h;
        zr_draw_image(&win->buffer, image, &img);

        /* draw label */
        text.padding = zr_vec2(0,0);
        label.x = image.x + image.w + style->combo.spacing.x + style->combo.content_padding.x;
        label.y = header.y + style->combo.content_padding.y;
        label.w = (button.x - style->combo.content_padding.x) - label.x;
        label.h = header.h - 2 * style->combo.content_padding.y;
        zr_widget_text(&win->buffer, label, selected, len, &text, ZR_TEXT_LEFT, &style->font);
    }
    return zr_combo_begin(layout, ctx, win, height, is_active, header);
}

int zr_combo_begin_symbol_label(struct zr_context *ctx, struct zr_panel *layout,
    const char *selected, enum zr_symbol_type type, int height)
{return zr_combo_begin_symbol_text(ctx, layout, selected, zr_strsiz(selected), type, height);}

int zr_combo_begin_image_label(struct zr_context *ctx, struct zr_panel *layout,
    const char *selected, struct zr_image img, int height)
{return zr_combo_begin_image_text(ctx, layout, selected, zr_strsiz(selected), img, height);}

int zr_combo_item_text(struct zr_context *ctx, const char *text, zr_size len,zr_flags align)
{return zr_contextual_item_text(ctx, text, len, align);}

int zr_combo_item_label(struct zr_context *ctx, const char *label, zr_flags align)
{return zr_contextual_item_label(ctx, label, align);}

int zr_combo_item_image_text(struct zr_context *ctx, struct zr_image img, const char *text,
    zr_size len, zr_flags alignment)
{return zr_contextual_item_image_text(ctx, img, text, len, alignment);}

int zr_combo_item_image_label(struct zr_context *ctx, struct zr_image img,
    const char *text, zr_flags alignment)
{return zr_contextual_item_image_label(ctx, img, text, alignment);}

int zr_combo_item_symbol_text(struct zr_context *ctx, enum zr_symbol_type sym,
    const char *text, zr_size len, zr_flags alignment)
{return zr_contextual_item_symbol_text(ctx, sym, text, len, alignment);}

int zr_combo_item_symbol_label(struct zr_context *ctx, enum zr_symbol_type sym,
    const char *label, zr_flags alignment)
{return zr_contextual_item_symbol_label(ctx, sym, label, alignment);}

void zr_combo_end(struct zr_context *ctx)
{zr_contextual_end(ctx);}

void zr_combo_close(struct zr_context *ctx)
{zr_contextual_close(ctx);}

int
zr_combo(struct zr_context *ctx, const char **items, int count,
    int selected, int item_height)
{
    int i = 0;
    int max_height;
    struct zr_panel combo;
    float item_padding;
    float window_padding;

    ZR_ASSERT(ctx);
    ZR_ASSERT(items);
    if (!ctx || !items ||!count)
        return selected;

    item_padding = ctx->style.combo.button_padding.y;
    window_padding = ctx->style.window.padding.y;
    max_height = (count+1) * item_height + (int)item_padding * 3 + (int)window_padding * 2;
    if (zr_combo_begin_label(ctx, &combo, items[selected], max_height)) {
        zr_layout_row_dynamic(ctx, (float)item_height, 1);
        for (i = 0; i < count; ++i) {
            if (zr_combo_item_label(ctx, items[i], ZR_TEXT_LEFT))
                selected = i;
        }
        zr_combo_end(ctx);
    }
    return selected;
}

int
zr_combo_seperator(struct zr_context *ctx, const char *items_seperated_by_seperator,
    int seperator, int selected, int count, int item_height)
{
    int i;
    int max_height;
    struct zr_panel combo;
    float item_padding;
    float window_padding;
    const char *current_item;
    const char *iter;
    zr_size length = 0;

    ZR_ASSERT(ctx);
    ZR_ASSERT(items_seperated_by_seperator);
    if (!ctx || !items_seperated_by_seperator)
        return selected;

    /* calculate popup window */
    item_padding = ctx->style.combo.content_padding.y;
    window_padding = ctx->style.window.padding.y;
    max_height = (count+1) * item_height + (int)item_padding * 3 + (int)window_padding * 2;

    /* find selected item */
    current_item = items_seperated_by_seperator;
    for (i = 0; i < selected; ++i) {
        iter = current_item;
        while (*iter != seperator) iter++;
        length = (zr_size)(iter - current_item);
        current_item = iter + 1;
    }

    if (zr_combo_begin_text(ctx, &combo, current_item, length, max_height)) {
        current_item = items_seperated_by_seperator;
        zr_layout_row_dynamic(ctx, (float)item_height, 1);
        for (i = 0; i < count; ++i) {
            iter = current_item;
            while (*iter != seperator) iter++;
            length = (zr_size)(iter - current_item);
            if (zr_combo_item_text(ctx, current_item, length, ZR_TEXT_LEFT))
                selected = i;
            current_item = current_item + length + 1;
        }
        zr_combo_end(ctx);
    }
    return selected;
}

int
zr_combo_string(struct zr_context *ctx, const char *items_seperated_by_zeros,
    int selected, int count, int item_height)
{return zr_combo_seperator(ctx, items_seperated_by_zeros, '\0', selected, count, item_height);}

int
zr_combo_callback(struct zr_context *ctx, void(item_getter)(void*, int, const char**),
    void *userdata, int selected, int count, int item_height)
{
    int i;
    int max_height;
    struct zr_panel combo;
    float item_padding;
    float window_padding;
    const char *item;

    ZR_ASSERT(ctx);
    ZR_ASSERT(item_getter);
    if (!ctx || !item_getter)
        return selected;

    /* calculate popup window */
    item_padding = ctx->style.combo.content_padding.y;
    window_padding = ctx->style.window.padding.y;
    max_height = (count+1) * item_height + (int)item_padding * 3 + (int)window_padding * 2;

    item_getter(userdata, selected, &item);
    if (zr_combo_begin_label(ctx, &combo, item, max_height)) {
        zr_layout_row_dynamic(ctx, (float)item_height, 1);
        for (i = 0; i < count; ++i) {
            item_getter(userdata, i, &item);
            if (zr_combo_item_label(ctx, item, ZR_TEXT_LEFT))
                selected = i;
        }
        zr_combo_end(ctx);
    }
    return selected;
}

void zr_combobox(struct zr_context *ctx, const char **items, int count,
    int *selected, int item_height)
{*selected = zr_combo(ctx, items, count, *selected, item_height);}

void zr_combobox_string(struct zr_context *ctx, const char *items_seperated_by_zeros,
    int *selected, int count, int item_height)
{*selected = zr_combo_string(ctx, items_seperated_by_zeros, *selected, count, item_height);}

void zr_combobox_seperator(struct zr_context *ctx, const char *items_seperated_by_seperator,
    int seperator,int *selected, int count, int item_height)
{*selected = zr_combo_seperator(ctx, items_seperated_by_seperator, seperator,
    *selected, count, item_height);}

void zr_combobox_callback(struct zr_context *ctx,
    void(item_getter)(void* data, int id, const char **out_text),
    void *userdata, int *selected, int count, int item_height)
{*selected = zr_combo_callback(ctx, item_getter, userdata,  *selected, count, item_height);}

/*
 * -------------------------------------------------------------
 *
 *                          MENU
 *
 * --------------------------------------------------------------
 */
static int
zr_menu_begin(struct zr_panel *layout, struct zr_context *ctx, struct zr_window *win,
    const char *id, int is_clicked, struct zr_rect header, float width)
{
    int is_open = 0;
    int is_active = 0;
    struct zr_rect body;
    struct zr_window *popup;
    zr_hash hash = zr_murmur_hash(id, (int)zr_strsiz(id), ZR_WINDOW_MENU);

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    body.x = header.x;
    body.w = width;
    body.y = header.y + header.h;
    body.h = (win->layout->bounds.y + win->layout->bounds.h) - body.y;

    popup = win->popup.win;
    is_open = (popup && (popup->flags & ZR_WINDOW_MENU));
    is_active = (popup && (win->popup.name == hash) && win->popup.type == ZR_WINDOW_MENU);
    if ((is_clicked && is_open && !is_active) || (is_open && !is_active) ||
        (!is_open && !is_active && !is_clicked)) return 0;
    if (!zr_nonblock_begin(layout, ctx, ZR_WINDOW_MENU|ZR_WINDOW_NO_SCROLLBAR, body, header))
        return 0;
    win->popup.type = ZR_WINDOW_MENU;
    win->popup.name = hash;
    return 1;
}

int
zr_menu_begin_text(struct zr_context *ctx, struct zr_panel *layout,
    const char *title, zr_size len, zr_flags align, float width)
{
    struct zr_window *win;
    const struct zr_input *in;
    struct zr_rect header;
    int is_clicked = zr_false;
    zr_flags state;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    state = zr_widget(&header, ctx);
    if (!state) return 0;
    in = (state == ZR_WIDGET_ROM || win->flags & ZR_WINDOW_ROM) ? 0 : &ctx->input;
    if (zr_do_button_text(&ctx->last_widget_state, &win->buffer, header,
        title, len, align, ZR_BUTTON_DEFAULT, &ctx->style.menu_button, in, &ctx->style.font))
        is_clicked = zr_true;
    return zr_menu_begin(layout, ctx, win, title, is_clicked, header, width);
}

int zr_menu_begin_label(struct zr_context *ctx, struct zr_panel *layout,
    const char *text, zr_flags align, float width)
{return zr_menu_begin_text(ctx, layout, text, zr_strsiz(text), align, width);}

int
zr_menu_begin_image(struct zr_context *ctx, struct zr_panel *layout,
    const char *id, struct zr_image img, float width)
{
    struct zr_window *win;
    struct zr_rect header;
    const struct zr_input *in;
    int is_clicked = zr_false;
    zr_flags state;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    state = zr_widget(&header, ctx);
    if (!state) return 0;
    in = (state == ZR_WIDGET_ROM || win->layout->flags & ZR_WINDOW_ROM) ? 0 : &ctx->input;
    if (zr_do_button_image(&ctx->last_widget_state, &win->buffer, header,
        img, ZR_BUTTON_DEFAULT, &ctx->style.menu_button, in))
        is_clicked = zr_true;
    return zr_menu_begin(layout, ctx, win, id, is_clicked, header, width);
}

int
zr_menu_begin_symbol(struct zr_context *ctx, struct zr_panel *layout,
    const char *id, enum zr_symbol_type sym, float width)
{
    struct zr_window *win;
    const struct zr_input *in;
    struct zr_rect header;
    int is_clicked = zr_false;
    zr_flags state;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    state = zr_widget(&header, ctx);
    if (!state) return 0;
    in = (state == ZR_WIDGET_ROM || win->layout->flags & ZR_WINDOW_ROM) ? 0 : &ctx->input;
    if (zr_do_button_symbol(&ctx->last_widget_state,  &win->buffer, header,
        sym, ZR_BUTTON_DEFAULT, &ctx->style.menu_button, in, &ctx->style.font))
        is_clicked = zr_true;
    return zr_menu_begin(layout, ctx, win, id, is_clicked, header, width);
}

int
zr_menu_begin_image_text(struct zr_context *ctx, struct zr_panel *layout,
    const char *title, zr_size len, zr_flags align, struct zr_image img, float width)
{
    struct zr_window *win;
    struct zr_rect header;
    const struct zr_input *in;
    int is_clicked = zr_false;
    zr_flags state;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    state = zr_widget(&header, ctx);
    if (!state) return 0;
    in = (state == ZR_WIDGET_ROM || win->layout->flags & ZR_WINDOW_ROM) ? 0 : &ctx->input;
    if (zr_do_button_text_image(&ctx->last_widget_state, &win->buffer,
        header, img, title, len, align, ZR_BUTTON_DEFAULT, &ctx->style.menu_button,
        &ctx->style.font, in))
        is_clicked = zr_true;
    return zr_menu_begin(layout, ctx, win, title, is_clicked, header, width);
}

int zr_menu_begin_image_label(struct zr_context *ctx, struct zr_panel *layout,
    const char *title, zr_flags align, struct zr_image img, float width)
{return zr_menu_begin_image_text(ctx, layout, title, zr_strsiz(title), align, img, width);}

int
zr_menu_begin_symbol_text(struct zr_context *ctx, struct zr_panel *layout,
    const char *title, zr_size size, zr_flags align, enum zr_symbol_type sym, float width)
{
    struct zr_window *win;
    struct zr_rect header;
    const struct zr_input *in;
    int is_clicked = zr_false;
    zr_flags state;

    ZR_ASSERT(ctx);
    ZR_ASSERT(ctx->current);
    ZR_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    state = zr_widget(&header, ctx);
    if (!state) return 0;
    in = (state == ZR_WIDGET_ROM || win->layout->flags & ZR_WINDOW_ROM) ? 0 : &ctx->input;
    if (zr_do_button_text_symbol(&ctx->last_widget_state, &win->buffer,
        header, sym, title, size, align, ZR_BUTTON_DEFAULT, &ctx->style.menu_button,
        &ctx->style.font, in))
        is_clicked = zr_true;
    return zr_menu_begin(layout, ctx, win, title, is_clicked, header, width);
}

int zr_menu_begin_symbol_label(struct zr_context *ctx, struct zr_panel *layout,
    const char *title, zr_flags align, enum zr_symbol_type sym, float width)
{return zr_menu_begin_symbol_text(ctx, layout, title, zr_strsiz(title), align,sym, width);}

int zr_menu_item_text(struct zr_context *ctx, const char *title, zr_size len, zr_flags align)
{return zr_contextual_item_text(ctx, title, len, align);}

int zr_menu_item_label(struct zr_context *ctx, const char *label, zr_flags align)
{return zr_contextual_item_label(ctx, label, align);}

int zr_menu_item_image_label(struct zr_context *ctx, struct zr_image img,
    const char *label, zr_flags align)
{return zr_contextual_item_image_label(ctx, img, label, align);}

int zr_menu_item_image_text(struct zr_context *ctx, struct zr_image img,
    const char *text, zr_size len, zr_flags align)
{return zr_contextual_item_image_text(ctx, img, text, len, align);}

int zr_menu_item_symbol_text(struct zr_context *ctx, enum zr_symbol_type sym,
    const char *text, zr_size len, zr_flags align)
{return zr_contextual_item_symbol_text(ctx, sym, text, len, align);}

int zr_menu_item_symbol_label(struct zr_context *ctx, enum zr_symbol_type sym,
    const char *label, zr_flags align)
{return zr_contextual_item_symbol_label(ctx, sym, label, align);}

void zr_menu_close(struct zr_context *ctx)
{zr_contextual_close(ctx);}

void
zr_menu_end(struct zr_context *ctx)
{zr_contextual_end(ctx);}

