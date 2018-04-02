#include "nuklear.h"
#include "nuklear_internal.h"

/* ===============================================================
 *
 *                              STRING
 *
 * ===============================================================*/
#ifdef NK_INCLUDE_DEFAULT_ALLOCATOR
NK_API void
nk_str_init_default(struct nk_str *str)
{
    struct nk_allocator alloc;
    alloc.userdata.ptr = 0;
    alloc.alloc = nk_malloc;
    alloc.free = nk_mfree;
    nk_buffer_init(&str->buffer, &alloc, 32);
    str->len = 0;
}
#endif

NK_API void
nk_str_init(struct nk_str *str, const struct nk_allocator *alloc, nk_size size)
{
    nk_buffer_init(&str->buffer, alloc, size);
    str->len = 0;
}
NK_API void
nk_str_init_fixed(struct nk_str *str, void *memory, nk_size size)
{
    nk_buffer_init_fixed(&str->buffer, memory, size);
    str->len = 0;
}
NK_API int
nk_str_append_text_char(struct nk_str *s, const char *str, int len)
{
    char *mem;
    NK_ASSERT(s);
    NK_ASSERT(str);
    if (!s || !str || !len) return 0;
    mem = (char*)nk_buffer_alloc(&s->buffer, NK_BUFFER_FRONT, (nk_size)len * sizeof(char), 0);
    if (!mem) return 0;
    NK_MEMCPY(mem, str, (nk_size)len * sizeof(char));
    s->len += nk_utf_len(str, len);
    return len;
}
NK_API int
nk_str_append_str_char(struct nk_str *s, const char *str)
{
    return nk_str_append_text_char(s, str, nk_strlen(str));
}
NK_API int
nk_str_append_text_utf8(struct nk_str *str, const char *text, int len)
{
    int i = 0;
    int byte_len = 0;
    nk_rune unicode;
    if (!str || !text || !len) return 0;
    for (i = 0; i < len; ++i)
        byte_len += nk_utf_decode(text+byte_len, &unicode, 4);
    nk_str_append_text_char(str, text, byte_len);
    return len;
}
NK_API int
nk_str_append_str_utf8(struct nk_str *str, const char *text)
{
    int runes = 0;
    int byte_len = 0;
    int num_runes = 0;
    int glyph_len = 0;
    nk_rune unicode;
    if (!str || !text) return 0;

    glyph_len = byte_len = nk_utf_decode(text+byte_len, &unicode, 4);
    while (unicode != '\0' && glyph_len) {
        glyph_len = nk_utf_decode(text+byte_len, &unicode, 4);
        byte_len += glyph_len;
        num_runes++;
    }
    nk_str_append_text_char(str, text, byte_len);
    return runes;
}
NK_API int
nk_str_append_text_runes(struct nk_str *str, const nk_rune *text, int len)
{
    int i = 0;
    int byte_len = 0;
    nk_glyph glyph;

    NK_ASSERT(str);
    if (!str || !text || !len) return 0;
    for (i = 0; i < len; ++i) {
        byte_len = nk_utf_encode(text[i], glyph, NK_UTF_SIZE);
        if (!byte_len) break;
        nk_str_append_text_char(str, glyph, byte_len);
    }
    return len;
}
NK_API int
nk_str_append_str_runes(struct nk_str *str, const nk_rune *runes)
{
    int i = 0;
    nk_glyph glyph;
    int byte_len;
    NK_ASSERT(str);
    if (!str || !runes) return 0;
    while (runes[i] != '\0') {
        byte_len = nk_utf_encode(runes[i], glyph, NK_UTF_SIZE);
        nk_str_append_text_char(str, glyph, byte_len);
        i++;
    }
    return i;
}
NK_API int
nk_str_insert_at_char(struct nk_str *s, int pos, const char *str, int len)
{
    int i;
    void *mem;
    char *src;
    char *dst;

    int copylen;
    NK_ASSERT(s);
    NK_ASSERT(str);
    NK_ASSERT(len >= 0);
    if (!s || !str || !len || (nk_size)pos > s->buffer.allocated) return 0;
    if ((s->buffer.allocated + (nk_size)len >= s->buffer.memory.size) &&
        (s->buffer.type == NK_BUFFER_FIXED)) return 0;

    copylen = (int)s->buffer.allocated - pos;
    if (!copylen) {
        nk_str_append_text_char(s, str, len);
        return 1;
    }
    mem = nk_buffer_alloc(&s->buffer, NK_BUFFER_FRONT, (nk_size)len * sizeof(char), 0);
    if (!mem) return 0;

    /* memmove */
    NK_ASSERT(((int)pos + (int)len + ((int)copylen - 1)) >= 0);
    NK_ASSERT(((int)pos + ((int)copylen - 1)) >= 0);
    dst = nk_ptr_add(char, s->buffer.memory.ptr, pos + len + (copylen - 1));
    src = nk_ptr_add(char, s->buffer.memory.ptr, pos + (copylen-1));
    for (i = 0; i < copylen; ++i) *dst-- = *src--;
    mem = nk_ptr_add(void, s->buffer.memory.ptr, pos);
    NK_MEMCPY(mem, str, (nk_size)len * sizeof(char));
    s->len = nk_utf_len((char *)s->buffer.memory.ptr, (int)s->buffer.allocated);
    return 1;
}
NK_API int
nk_str_insert_at_rune(struct nk_str *str, int pos, const char *cstr, int len)
{
    int glyph_len;
    nk_rune unicode;
    const char *begin;
    const char *buffer;

    NK_ASSERT(str);
    NK_ASSERT(cstr);
    NK_ASSERT(len);
    if (!str || !cstr || !len) return 0;
    begin = nk_str_at_rune(str, pos, &unicode, &glyph_len);
    if (!str->len)
        return nk_str_append_text_char(str, cstr, len);
    buffer = nk_str_get_const(str);
    if (!begin) return 0;
    return nk_str_insert_at_char(str, (int)(begin - buffer), cstr, len);
}
NK_API int
nk_str_insert_text_char(struct nk_str *str, int pos, const char *text, int len)
{
    return nk_str_insert_text_utf8(str, pos, text, len);
}
NK_API int
nk_str_insert_str_char(struct nk_str *str, int pos, const char *text)
{
    return nk_str_insert_text_utf8(str, pos, text, nk_strlen(text));
}
NK_API int
nk_str_insert_text_utf8(struct nk_str *str, int pos, const char *text, int len)
{
    int i = 0;
    int byte_len = 0;
    nk_rune unicode;

    NK_ASSERT(str);
    NK_ASSERT(text);
    if (!str || !text || !len) return 0;
    for (i = 0; i < len; ++i)
        byte_len += nk_utf_decode(text+byte_len, &unicode, 4);
    nk_str_insert_at_rune(str, pos, text, byte_len);
    return len;
}
NK_API int
nk_str_insert_str_utf8(struct nk_str *str, int pos, const char *text)
{
    int runes = 0;
    int byte_len = 0;
    int num_runes = 0;
    int glyph_len = 0;
    nk_rune unicode;
    if (!str || !text) return 0;

    glyph_len = byte_len = nk_utf_decode(text+byte_len, &unicode, 4);
    while (unicode != '\0' && glyph_len) {
        glyph_len = nk_utf_decode(text+byte_len, &unicode, 4);
        byte_len += glyph_len;
        num_runes++;
    }
    nk_str_insert_at_rune(str, pos, text, byte_len);
    return runes;
}
NK_API int
nk_str_insert_text_runes(struct nk_str *str, int pos, const nk_rune *runes, int len)
{
    int i = 0;
    int byte_len = 0;
    nk_glyph glyph;

    NK_ASSERT(str);
    if (!str || !runes || !len) return 0;
    for (i = 0; i < len; ++i) {
        byte_len = nk_utf_encode(runes[i], glyph, NK_UTF_SIZE);
        if (!byte_len) break;
        nk_str_insert_at_rune(str, pos+i, glyph, byte_len);
    }
    return len;
}
NK_API int
nk_str_insert_str_runes(struct nk_str *str, int pos, const nk_rune *runes)
{
    int i = 0;
    nk_glyph glyph;
    int byte_len;
    NK_ASSERT(str);
    if (!str || !runes) return 0;
    while (runes[i] != '\0') {
        byte_len = nk_utf_encode(runes[i], glyph, NK_UTF_SIZE);
        nk_str_insert_at_rune(str, pos+i, glyph, byte_len);
        i++;
    }
    return i;
}
NK_API void
nk_str_remove_chars(struct nk_str *s, int len)
{
    NK_ASSERT(s);
    NK_ASSERT(len >= 0);
    if (!s || len < 0 || (nk_size)len > s->buffer.allocated) return;
    NK_ASSERT(((int)s->buffer.allocated - (int)len) >= 0);
    s->buffer.allocated -= (nk_size)len;
    s->len = nk_utf_len((char *)s->buffer.memory.ptr, (int)s->buffer.allocated);
}
NK_API void
nk_str_remove_runes(struct nk_str *str, int len)
{
    int index;
    const char *begin;
    const char *end;
    nk_rune unicode;

    NK_ASSERT(str);
    NK_ASSERT(len >= 0);
    if (!str || len < 0) return;
    if (len >= str->len) {
        str->len = 0;
        return;
    }

    index = str->len - len;
    begin = nk_str_at_rune(str, index, &unicode, &len);
    end = (const char*)str->buffer.memory.ptr + str->buffer.allocated;
    nk_str_remove_chars(str, (int)(end-begin)+1);
}
NK_API void
nk_str_delete_chars(struct nk_str *s, int pos, int len)
{
    NK_ASSERT(s);
    if (!s || !len || (nk_size)pos > s->buffer.allocated ||
        (nk_size)(pos + len) > s->buffer.allocated) return;

    if ((nk_size)(pos + len) < s->buffer.allocated) {
        /* memmove */
        char *dst = nk_ptr_add(char, s->buffer.memory.ptr, pos);
        char *src = nk_ptr_add(char, s->buffer.memory.ptr, pos + len);
        NK_MEMCPY(dst, src, s->buffer.allocated - (nk_size)(pos + len));
        NK_ASSERT(((int)s->buffer.allocated - (int)len) >= 0);
        s->buffer.allocated -= (nk_size)len;
    } else nk_str_remove_chars(s, len);
    s->len = nk_utf_len((char *)s->buffer.memory.ptr, (int)s->buffer.allocated);
}
NK_API void
nk_str_delete_runes(struct nk_str *s, int pos, int len)
{
    char *temp;
    nk_rune unicode;
    char *begin;
    char *end;
    int unused;

    NK_ASSERT(s);
    NK_ASSERT(s->len >= pos + len);
    if (s->len < pos + len)
        len = NK_CLAMP(0, (s->len - pos), s->len);
    if (!len) return;

    temp = (char *)s->buffer.memory.ptr;
    begin = nk_str_at_rune(s, pos, &unicode, &unused);
    if (!begin) return;
    s->buffer.memory.ptr = begin;
    end = nk_str_at_rune(s, len, &unicode, &unused);
    s->buffer.memory.ptr = temp;
    if (!end) return;
    nk_str_delete_chars(s, (int)(begin - temp), (int)(end - begin));
}
NK_API char*
nk_str_at_char(struct nk_str *s, int pos)
{
    NK_ASSERT(s);
    if (!s || pos > (int)s->buffer.allocated) return 0;
    return nk_ptr_add(char, s->buffer.memory.ptr, pos);
}
NK_API char*
nk_str_at_rune(struct nk_str *str, int pos, nk_rune *unicode, int *len)
{
    int i = 0;
    int src_len = 0;
    int glyph_len = 0;
    char *text;
    int text_len;

    NK_ASSERT(str);
    NK_ASSERT(unicode);
    NK_ASSERT(len);

    if (!str || !unicode || !len) return 0;
    if (pos < 0) {
        *unicode = 0;
        *len = 0;
        return 0;
    }

    text = (char*)str->buffer.memory.ptr;
    text_len = (int)str->buffer.allocated;
    glyph_len = nk_utf_decode(text, unicode, text_len);
    while (glyph_len) {
        if (i == pos) {
            *len = glyph_len;
            break;
        }

        i++;
        src_len = src_len + glyph_len;
        glyph_len = nk_utf_decode(text + src_len, unicode, text_len - src_len);
    }
    if (i != pos) return 0;
    return text + src_len;
}
NK_API const char*
nk_str_at_char_const(const struct nk_str *s, int pos)
{
    NK_ASSERT(s);
    if (!s || pos > (int)s->buffer.allocated) return 0;
    return nk_ptr_add(char, s->buffer.memory.ptr, pos);
}
NK_API const char*
nk_str_at_const(const struct nk_str *str, int pos, nk_rune *unicode, int *len)
{
    int i = 0;
    int src_len = 0;
    int glyph_len = 0;
    char *text;
    int text_len;

    NK_ASSERT(str);
    NK_ASSERT(unicode);
    NK_ASSERT(len);

    if (!str || !unicode || !len) return 0;
    if (pos < 0) {
        *unicode = 0;
        *len = 0;
        return 0;
    }

    text = (char*)str->buffer.memory.ptr;
    text_len = (int)str->buffer.allocated;
    glyph_len = nk_utf_decode(text, unicode, text_len);
    while (glyph_len) {
        if (i == pos) {
            *len = glyph_len;
            break;
        }

        i++;
        src_len = src_len + glyph_len;
        glyph_len = nk_utf_decode(text + src_len, unicode, text_len - src_len);
    }
    if (i != pos) return 0;
    return text + src_len;
}
NK_API nk_rune
nk_str_rune_at(const struct nk_str *str, int pos)
{
    int len;
    nk_rune unicode = 0;
    nk_str_at_const(str, pos, &unicode, &len);
    return unicode;
}
NK_API char*
nk_str_get(struct nk_str *s)
{
    NK_ASSERT(s);
    if (!s || !s->len || !s->buffer.allocated) return 0;
    return (char*)s->buffer.memory.ptr;
}
NK_API const char*
nk_str_get_const(const struct nk_str *s)
{
    NK_ASSERT(s);
    if (!s || !s->len || !s->buffer.allocated) return 0;
    return (const char*)s->buffer.memory.ptr;
}
NK_API int
nk_str_len(struct nk_str *s)
{
    NK_ASSERT(s);
    if (!s || !s->len || !s->buffer.allocated) return 0;
    return s->len;
}
NK_API int
nk_str_len_char(struct nk_str *s)
{
    NK_ASSERT(s);
    if (!s || !s->len || !s->buffer.allocated) return 0;
    return (int)s->buffer.allocated;
}
NK_API void
nk_str_clear(struct nk_str *str)
{
    NK_ASSERT(str);
    nk_buffer_clear(&str->buffer);
    str->len = 0;
}
NK_API void
nk_str_free(struct nk_str *str)
{
    NK_ASSERT(str);
    nk_buffer_free(&str->buffer);
    str->len = 0;
}

