#include "nuklear.h"
#include "nuklear_internal.h"

#ifdef NK_INCLUDE_FONT_BAKING
/* -------------------------------------------------------------
 *
 *                          RECT PACK
 *
 * --------------------------------------------------------------*/
/* stb_rect_pack.h - v0.05 - public domain - rectangle packing */
/* Sean Barrett 2014 */
#define NK_RP__MAXVAL  0xffff
typedef unsigned short nk_rp_coord;

struct nk_rp_rect {
    /* reserved for your use: */
    int id;
    /* input: */
    nk_rp_coord w, h;
    /* output: */
    nk_rp_coord x, y;
    int was_packed;
    /* non-zero if valid packing */
}; /* 16 bytes, nominally */

struct nk_rp_node {
    nk_rp_coord  x,y;
    struct nk_rp_node  *next;
};

struct nk_rp_context {
    int width;
    int height;
    int align;
    int init_mode;
    int heuristic;
    int num_nodes;
    struct nk_rp_node *active_head;
    struct nk_rp_node *free_head;
    struct nk_rp_node extra[2];
    /* we allocate two extra nodes so optimal user-node-count is 'width' not 'width+2' */
};

struct nk_rp__findresult {
    int x,y;
    struct nk_rp_node **prev_link;
};

enum NK_RP_HEURISTIC {
    NK_RP_HEURISTIC_Skyline_default=0,
    NK_RP_HEURISTIC_Skyline_BL_sortHeight = NK_RP_HEURISTIC_Skyline_default,
    NK_RP_HEURISTIC_Skyline_BF_sortHeight
};
enum NK_RP_INIT_STATE{NK_RP__INIT_skyline = 1};

NK_INTERN void
nk_rp_setup_allow_out_of_mem(struct nk_rp_context *context, int allow_out_of_mem)
{
    if (allow_out_of_mem)
        /* if it's ok to run out of memory, then don't bother aligning them; */
        /* this gives better packing, but may fail due to OOM (even though */
        /* the rectangles easily fit). @TODO a smarter approach would be to only */
        /* quantize once we've hit OOM, then we could get rid of this parameter. */
        context->align = 1;
    else {
        /* if it's not ok to run out of memory, then quantize the widths */
        /* so that num_nodes is always enough nodes. */
        /* */
        /* I.e. num_nodes * align >= width */
        /*                  align >= width / num_nodes */
        /*                  align = ceil(width/num_nodes) */
        context->align = (context->width + context->num_nodes-1) / context->num_nodes;
    }
}
NK_INTERN void
nk_rp_init_target(struct nk_rp_context *context, int width, int height,
    struct nk_rp_node *nodes, int num_nodes)
{
    int i;
#ifndef STBRP_LARGE_RECTS
    NK_ASSERT(width <= 0xffff && height <= 0xffff);
#endif

    for (i=0; i < num_nodes-1; ++i)
        nodes[i].next = &nodes[i+1];
    nodes[i].next = 0;
    context->init_mode = NK_RP__INIT_skyline;
    context->heuristic = NK_RP_HEURISTIC_Skyline_default;
    context->free_head = &nodes[0];
    context->active_head = &context->extra[0];
    context->width = width;
    context->height = height;
    context->num_nodes = num_nodes;
    nk_rp_setup_allow_out_of_mem(context, 0);

    /* node 0 is the full width, node 1 is the sentinel (lets us not store width explicitly) */
    context->extra[0].x = 0;
    context->extra[0].y = 0;
    context->extra[0].next = &context->extra[1];
    context->extra[1].x = (nk_rp_coord) width;
    context->extra[1].y = 65535;
    context->extra[1].next = 0;
}
/* find minimum y position if it starts at x1 */
NK_INTERN int
nk_rp__skyline_find_min_y(struct nk_rp_context *c, struct nk_rp_node *first,
    int x0, int width, int *pwaste)
{
    struct nk_rp_node *node = first;
    int x1 = x0 + width;
    int min_y, visited_width, waste_area;
    NK_ASSERT(first->x <= x0);
    NK_UNUSED(c);

    NK_ASSERT(node->next->x > x0);
    /* we ended up handling this in the caller for efficiency */
    NK_ASSERT(node->x <= x0);

    min_y = 0;
    waste_area = 0;
    visited_width = 0;
    while (node->x < x1)
    {
        if (node->y > min_y) {
            /* raise min_y higher. */
            /* we've accounted for all waste up to min_y, */
            /* but we'll now add more waste for everything we've visited */
            waste_area += visited_width * (node->y - min_y);
            min_y = node->y;
            /* the first time through, visited_width might be reduced */
            if (node->x < x0)
            visited_width += node->next->x - x0;
            else
            visited_width += node->next->x - node->x;
        } else {
            /* add waste area */
            int under_width = node->next->x - node->x;
            if (under_width + visited_width > width)
            under_width = width - visited_width;
            waste_area += under_width * (min_y - node->y);
            visited_width += under_width;
        }
        node = node->next;
    }
    *pwaste = waste_area;
    return min_y;
}
NK_INTERN struct nk_rp__findresult
nk_rp__skyline_find_best_pos(struct nk_rp_context *c, int width, int height)
{
    int best_waste = (1<<30), best_x, best_y = (1 << 30);
    struct nk_rp__findresult fr;
    struct nk_rp_node **prev, *node, *tail, **best = 0;

    /* align to multiple of c->align */
    width = (width + c->align - 1);
    width -= width % c->align;
    NK_ASSERT(width % c->align == 0);

    node = c->active_head;
    prev = &c->active_head;
    while (node->x + width <= c->width) {
        int y,waste;
        y = nk_rp__skyline_find_min_y(c, node, node->x, width, &waste);
        /* actually just want to test BL */
        if (c->heuristic == NK_RP_HEURISTIC_Skyline_BL_sortHeight) {
            /* bottom left */
            if (y < best_y) {
            best_y = y;
            best = prev;
            }
        } else {
            /* best-fit */
            if (y + height <= c->height) {
                /* can only use it if it first vertically */
                if (y < best_y || (y == best_y && waste < best_waste)) {
                    best_y = y;
                    best_waste = waste;
                    best = prev;
                }
            }
        }
        prev = &node->next;
        node = node->next;
    }
    best_x = (best == 0) ? 0 : (*best)->x;

    /* if doing best-fit (BF), we also have to try aligning right edge to each node position */
    /* */
    /* e.g, if fitting */
    /* */
    /*     ____________________ */
    /*    |____________________| */
    /* */
    /*            into */
    /* */
    /*   |                         | */
    /*   |             ____________| */
    /*   |____________| */
    /* */
    /* then right-aligned reduces waste, but bottom-left BL is always chooses left-aligned */
    /* */
    /* This makes BF take about 2x the time */
    if (c->heuristic == NK_RP_HEURISTIC_Skyline_BF_sortHeight)
    {
        tail = c->active_head;
        node = c->active_head;
        prev = &c->active_head;
        /* find first node that's admissible */
        while (tail->x < width)
            tail = tail->next;
        while (tail)
        {
            int xpos = tail->x - width;
            int y,waste;
            NK_ASSERT(xpos >= 0);
            /* find the left position that matches this */
            while (node->next->x <= xpos) {
                prev = &node->next;
                node = node->next;
            }
            NK_ASSERT(node->next->x > xpos && node->x <= xpos);
            y = nk_rp__skyline_find_min_y(c, node, xpos, width, &waste);
            if (y + height < c->height) {
                if (y <= best_y) {
                    if (y < best_y || waste < best_waste || (waste==best_waste && xpos < best_x)) {
                        best_x = xpos;
                        NK_ASSERT(y <= best_y);
                        best_y = y;
                        best_waste = waste;
                        best = prev;
                    }
                }
            }
            tail = tail->next;
        }
    }
    fr.prev_link = best;
    fr.x = best_x;
    fr.y = best_y;
    return fr;
}
NK_INTERN struct nk_rp__findresult
nk_rp__skyline_pack_rectangle(struct nk_rp_context *context, int width, int height)
{
    /* find best position according to heuristic */
    struct nk_rp__findresult res = nk_rp__skyline_find_best_pos(context, width, height);
    struct nk_rp_node *node, *cur;

    /* bail if: */
    /*    1. it failed */
    /*    2. the best node doesn't fit (we don't always check this) */
    /*    3. we're out of memory */
    if (res.prev_link == 0 || res.y + height > context->height || context->free_head == 0) {
        res.prev_link = 0;
        return res;
    }

    /* on success, create new node */
    node = context->free_head;
    node->x = (nk_rp_coord) res.x;
    node->y = (nk_rp_coord) (res.y + height);

    context->free_head = node->next;

    /* insert the new node into the right starting point, and */
    /* let 'cur' point to the remaining nodes needing to be */
    /* stitched back in */
    cur = *res.prev_link;
    if (cur->x < res.x) {
        /* preserve the existing one, so start testing with the next one */
        struct nk_rp_node *next = cur->next;
        cur->next = node;
        cur = next;
    } else {
        *res.prev_link = node;
    }

    /* from here, traverse cur and free the nodes, until we get to one */
    /* that shouldn't be freed */
    while (cur->next && cur->next->x <= res.x + width) {
        struct nk_rp_node *next = cur->next;
        /* move the current node to the free list */
        cur->next = context->free_head;
        context->free_head = cur;
        cur = next;
    }
    /* stitch the list back in */
    node->next = cur;

    if (cur->x < res.x + width)
        cur->x = (nk_rp_coord) (res.x + width);
    return res;
}
NK_INTERN int
nk_rect_height_compare(const void *a, const void *b)
{
    const struct nk_rp_rect *p = (const struct nk_rp_rect *) a;
    const struct nk_rp_rect *q = (const struct nk_rp_rect *) b;
    if (p->h > q->h)
        return -1;
    if (p->h < q->h)
        return  1;
    return (p->w > q->w) ? -1 : (p->w < q->w);
}
NK_INTERN int
nk_rect_original_order(const void *a, const void *b)
{
    const struct nk_rp_rect *p = (const struct nk_rp_rect *) a;
    const struct nk_rp_rect *q = (const struct nk_rp_rect *) b;
    return (p->was_packed < q->was_packed) ? -1 : (p->was_packed > q->was_packed);
}
NK_INTERN void
nk_rp_qsort(struct nk_rp_rect *array, unsigned int len, int(*cmp)(const void*,const void*))
{
    /* iterative quick sort */
    #define NK_MAX_SORT_STACK 64
    unsigned right, left = 0, stack[NK_MAX_SORT_STACK], pos = 0;
    unsigned seed = len/2 * 69069+1;
    for (;;) {
        for (; left+1 < len; len++) {
            struct nk_rp_rect pivot, tmp;
            if (pos == NK_MAX_SORT_STACK) len = stack[pos = 0];
            pivot = array[left+seed%(len-left)];
            seed = seed * 69069 + 1;
            stack[pos++] = len;
            for (right = left-1;;) {
                while (cmp(&array[++right], &pivot) < 0);
                while (cmp(&pivot, &array[--len]) < 0);
                if (right >= len) break;
                tmp = array[right];
                array[right] = array[len];
                array[len] = tmp;
            }
        }
        if (pos == 0) break;
        left = len;
        len = stack[--pos];
    }
    #undef NK_MAX_SORT_STACK
}
NK_INTERN void
nk_rp_pack_rects(struct nk_rp_context *context, struct nk_rp_rect *rects, int num_rects)
{
    int i;
    /* we use the 'was_packed' field internally to allow sorting/unsorting */
    for (i=0; i < num_rects; ++i) {
        rects[i].was_packed = i;
    }

    /* sort according to heuristic */
    nk_rp_qsort(rects, (unsigned)num_rects, nk_rect_height_compare);

    for (i=0; i < num_rects; ++i) {
        struct nk_rp__findresult fr = nk_rp__skyline_pack_rectangle(context, rects[i].w, rects[i].h);
        if (fr.prev_link) {
            rects[i].x = (nk_rp_coord) fr.x;
            rects[i].y = (nk_rp_coord) fr.y;
        } else {
            rects[i].x = rects[i].y = NK_RP__MAXVAL;
        }
    }

    /* unsort */
    nk_rp_qsort(rects, (unsigned)num_rects, nk_rect_original_order);

    /* set was_packed flags */
    for (i=0; i < num_rects; ++i)
        rects[i].was_packed = !(rects[i].x == NK_RP__MAXVAL && rects[i].y == NK_RP__MAXVAL);
}

/*
 * ==============================================================
 *
 *                          TRUETYPE
 *
 * ===============================================================
 */
/* stb_truetype.h - v1.07 - public domain */
#define NK_TT_MAX_OVERSAMPLE   8
#define NK_TT__OVER_MASK  (NK_TT_MAX_OVERSAMPLE-1)

struct nk_tt_bakedchar {
    unsigned short x0,y0,x1,y1;
    /* coordinates of bbox in bitmap */
    float xoff,yoff,xadvance;
};

struct nk_tt_aligned_quad{
    float x0,y0,s0,t0; /* top-left */
    float x1,y1,s1,t1; /* bottom-right */
};

struct nk_tt_packedchar {
    unsigned short x0,y0,x1,y1;
    /* coordinates of bbox in bitmap */
    float xoff,yoff,xadvance;
    float xoff2,yoff2;
};

struct nk_tt_pack_range {
    float font_size;
    int first_unicode_codepoint_in_range;
    /* if non-zero, then the chars are continuous, and this is the first codepoint */
    int *array_of_unicode_codepoints;
    /* if non-zero, then this is an array of unicode codepoints */
    int num_chars;
    struct nk_tt_packedchar *chardata_for_range; /* output */
    unsigned char h_oversample, v_oversample;
    /* don't set these, they're used internally */
};

struct nk_tt_pack_context {
    void *pack_info;
    int   width;
    int   height;
    int   stride_in_bytes;
    int   padding;
    unsigned int   h_oversample, v_oversample;
    unsigned char *pixels;
    void  *nodes;
};

struct nk_tt_fontinfo {
    const unsigned char* data; /* pointer to .ttf file */
    int fontstart;/* offset of start of font */
    int numGlyphs;/* number of glyphs, needed for range checking */
    int loca,head,glyf,hhea,hmtx,kern; /* table locations as offset from start of .ttf */
    int index_map; /* a cmap mapping for our chosen character encoding */
    int indexToLocFormat; /* format needed to map from glyph index to glyph */
};

enum {
  NK_TT_vmove=1,
  NK_TT_vline,
  NK_TT_vcurve
};

struct nk_tt_vertex {
    short x,y,cx,cy;
    unsigned char type,padding;
};

struct nk_tt__bitmap{
   int w,h,stride;
   unsigned char *pixels;
};

struct nk_tt__hheap_chunk {
    struct nk_tt__hheap_chunk *next;
};
struct nk_tt__hheap {
    struct nk_allocator alloc;
    struct nk_tt__hheap_chunk *head;
    void   *first_free;
    int    num_remaining_in_head_chunk;
};

struct nk_tt__edge {
    float x0,y0, x1,y1;
    int invert;
};

struct nk_tt__active_edge {
    struct nk_tt__active_edge *next;
    float fx,fdx,fdy;
    float direction;
    float sy;
    float ey;
};
struct nk_tt__point {float x,y;};

#define NK_TT_MACSTYLE_DONTCARE     0
#define NK_TT_MACSTYLE_BOLD         1
#define NK_TT_MACSTYLE_ITALIC       2
#define NK_TT_MACSTYLE_UNDERSCORE   4
#define NK_TT_MACSTYLE_NONE         8
/* <= not same as 0, this makes us check the bitfield is 0 */

enum { /* platformID */
   NK_TT_PLATFORM_ID_UNICODE   =0,
   NK_TT_PLATFORM_ID_MAC       =1,
   NK_TT_PLATFORM_ID_ISO       =2,
   NK_TT_PLATFORM_ID_MICROSOFT =3
};

enum { /* encodingID for NK_TT_PLATFORM_ID_UNICODE */
   NK_TT_UNICODE_EID_UNICODE_1_0    =0,
   NK_TT_UNICODE_EID_UNICODE_1_1    =1,
   NK_TT_UNICODE_EID_ISO_10646      =2,
   NK_TT_UNICODE_EID_UNICODE_2_0_BMP=3,
   NK_TT_UNICODE_EID_UNICODE_2_0_FULL=4
};

enum { /* encodingID for NK_TT_PLATFORM_ID_MICROSOFT */
   NK_TT_MS_EID_SYMBOL        =0,
   NK_TT_MS_EID_UNICODE_BMP   =1,
   NK_TT_MS_EID_SHIFTJIS      =2,
   NK_TT_MS_EID_UNICODE_FULL  =10
};

enum { /* encodingID for NK_TT_PLATFORM_ID_MAC; same as Script Manager codes */
   NK_TT_MAC_EID_ROMAN        =0,   NK_TT_MAC_EID_ARABIC       =4,
   NK_TT_MAC_EID_JAPANESE     =1,   NK_TT_MAC_EID_HEBREW       =5,
   NK_TT_MAC_EID_CHINESE_TRAD =2,   NK_TT_MAC_EID_GREEK        =6,
   NK_TT_MAC_EID_KOREAN       =3,   NK_TT_MAC_EID_RUSSIAN      =7
};

enum { /* languageID for NK_TT_PLATFORM_ID_MICROSOFT; same as LCID... */
       /* problematic because there are e.g. 16 english LCIDs and 16 arabic LCIDs */
   NK_TT_MS_LANG_ENGLISH     =0x0409,   NK_TT_MS_LANG_ITALIAN     =0x0410,
   NK_TT_MS_LANG_CHINESE     =0x0804,   NK_TT_MS_LANG_JAPANESE    =0x0411,
   NK_TT_MS_LANG_DUTCH       =0x0413,   NK_TT_MS_LANG_KOREAN      =0x0412,
   NK_TT_MS_LANG_FRENCH      =0x040c,   NK_TT_MS_LANG_RUSSIAN     =0x0419,
   NK_TT_MS_LANG_GERMAN      =0x0407,   NK_TT_MS_LANG_SPANISH     =0x0409,
   NK_TT_MS_LANG_HEBREW      =0x040d,   NK_TT_MS_LANG_SWEDISH     =0x041D
};

enum { /* languageID for NK_TT_PLATFORM_ID_MAC */
   NK_TT_MAC_LANG_ENGLISH      =0 ,   NK_TT_MAC_LANG_JAPANESE     =11,
   NK_TT_MAC_LANG_ARABIC       =12,   NK_TT_MAC_LANG_KOREAN       =23,
   NK_TT_MAC_LANG_DUTCH        =4 ,   NK_TT_MAC_LANG_RUSSIAN      =32,
   NK_TT_MAC_LANG_FRENCH       =1 ,   NK_TT_MAC_LANG_SPANISH      =6 ,
   NK_TT_MAC_LANG_GERMAN       =2 ,   NK_TT_MAC_LANG_SWEDISH      =5 ,
   NK_TT_MAC_LANG_HEBREW       =10,   NK_TT_MAC_LANG_CHINESE_SIMPLIFIED =33,
   NK_TT_MAC_LANG_ITALIAN      =3 ,   NK_TT_MAC_LANG_CHINESE_TRAD =19
};

#define nk_ttBYTE(p)     (* (const nk_byte *) (p))
#define nk_ttCHAR(p)     (* (const char *) (p))

#if defined(NK_BIGENDIAN) && !defined(NK_ALLOW_UNALIGNED_TRUETYPE)
   #define nk_ttUSHORT(p)   (* (nk_ushort *) (p))
   #define nk_ttSHORT(p)    (* (nk_short *) (p))
   #define nk_ttULONG(p)    (* (nk_uint *) (p))
   #define nk_ttLONG(p)     (* (nk_int *) (p))
#else
    static nk_ushort nk_ttUSHORT(const nk_byte *p) { return (nk_ushort)(p[0]*256 + p[1]); }
    static nk_short nk_ttSHORT(const nk_byte *p)   { return (nk_short)(p[0]*256 + p[1]); }
    static nk_uint nk_ttULONG(const nk_byte *p)  { return (nk_uint)((p[0]<<24) + (p[1]<<16) + (p[2]<<8) + p[3]); }
#endif

#define nk_tt_tag4(p,c0,c1,c2,c3)\
    ((p)[0] == (c0) && (p)[1] == (c1) && (p)[2] == (c2) && (p)[3] == (c3))
#define nk_tt_tag(p,str) nk_tt_tag4(p,str[0],str[1],str[2],str[3])

NK_INTERN int nk_tt_GetGlyphShape(const struct nk_tt_fontinfo *info, struct nk_allocator *alloc,
                                int glyph_index, struct nk_tt_vertex **pvertices);

NK_INTERN nk_uint
nk_tt__find_table(const nk_byte *data, nk_uint fontstart, const char *tag)
{
    /* @OPTIMIZE: binary search */
    nk_int num_tables = nk_ttUSHORT(data+fontstart+4);
    nk_uint tabledir = fontstart + 12;
    nk_int i;
    for (i = 0; i < num_tables; ++i) {
        nk_uint loc = tabledir + (nk_uint)(16*i);
        if (nk_tt_tag(data+loc+0, tag))
            return nk_ttULONG(data+loc+8);
    }
    return 0;
}
NK_INTERN int
nk_tt_InitFont(struct nk_tt_fontinfo *info, const unsigned char *data2, int fontstart)
{
    nk_uint cmap, t;
    nk_int i,numTables;
    const nk_byte *data = (const nk_byte *) data2;

    info->data = data;
    info->fontstart = fontstart;

    cmap = nk_tt__find_table(data, (nk_uint)fontstart, "cmap");       /* required */
    info->loca = (int)nk_tt__find_table(data, (nk_uint)fontstart, "loca"); /* required */
    info->head = (int)nk_tt__find_table(data, (nk_uint)fontstart, "head"); /* required */
    info->glyf = (int)nk_tt__find_table(data, (nk_uint)fontstart, "glyf"); /* required */
    info->hhea = (int)nk_tt__find_table(data, (nk_uint)fontstart, "hhea"); /* required */
    info->hmtx = (int)nk_tt__find_table(data, (nk_uint)fontstart, "hmtx"); /* required */
    info->kern = (int)nk_tt__find_table(data, (nk_uint)fontstart, "kern"); /* not required */
    if (!cmap || !info->loca || !info->head || !info->glyf || !info->hhea || !info->hmtx)
        return 0;

    t = nk_tt__find_table(data, (nk_uint)fontstart, "maxp");
    if (t) info->numGlyphs = nk_ttUSHORT(data+t+4);
    else info->numGlyphs = 0xffff;

    /* find a cmap encoding table we understand *now* to avoid searching */
    /* later. (todo: could make this installable) */
    /* the same regardless of glyph. */
    numTables = nk_ttUSHORT(data + cmap + 2);
    info->index_map = 0;
    for (i=0; i < numTables; ++i)
    {
        nk_uint encoding_record = cmap + 4 + 8 * (nk_uint)i;
        /* find an encoding we understand: */
        switch(nk_ttUSHORT(data+encoding_record)) {
        case NK_TT_PLATFORM_ID_MICROSOFT:
            switch (nk_ttUSHORT(data+encoding_record+2)) {
            case NK_TT_MS_EID_UNICODE_BMP:
            case NK_TT_MS_EID_UNICODE_FULL:
                /* MS/Unicode */
                info->index_map = (int)(cmap + nk_ttULONG(data+encoding_record+4));
                break;
            default: break;
            } break;
        case NK_TT_PLATFORM_ID_UNICODE:
            /* Mac/iOS has these */
            /* all the encodingIDs are unicode, so we don't bother to check it */
            info->index_map = (int)(cmap + nk_ttULONG(data+encoding_record+4));
            break;
        default: break;
        }
    }
    if (info->index_map == 0)
        return 0;
    info->indexToLocFormat = nk_ttUSHORT(data+info->head + 50);
    return 1;
}
NK_INTERN int
nk_tt_FindGlyphIndex(const struct nk_tt_fontinfo *info, int unicode_codepoint)
{
    const nk_byte *data = info->data;
    nk_uint index_map = (nk_uint)info->index_map;

    nk_ushort format = nk_ttUSHORT(data + index_map + 0);
    if (format == 0) { /* apple byte encoding */
        nk_int bytes = nk_ttUSHORT(data + index_map + 2);
        if (unicode_codepoint < bytes-6)
            return nk_ttBYTE(data + index_map + 6 + unicode_codepoint);
        return 0;
    } else if (format == 6) {
        nk_uint first = nk_ttUSHORT(data + index_map + 6);
        nk_uint count = nk_ttUSHORT(data + index_map + 8);
        if ((nk_uint) unicode_codepoint >= first && (nk_uint) unicode_codepoint < first+count)
            return nk_ttUSHORT(data + index_map + 10 + (unicode_codepoint - (int)first)*2);
        return 0;
    } else if (format == 2) {
        NK_ASSERT(0); /* @TODO: high-byte mapping for japanese/chinese/korean */
        return 0;
    } else if (format == 4) { /* standard mapping for windows fonts: binary search collection of ranges */
        nk_ushort segcount = nk_ttUSHORT(data+index_map+6) >> 1;
        nk_ushort searchRange = nk_ttUSHORT(data+index_map+8) >> 1;
        nk_ushort entrySelector = nk_ttUSHORT(data+index_map+10);
        nk_ushort rangeShift = nk_ttUSHORT(data+index_map+12) >> 1;

        /* do a binary search of the segments */
        nk_uint endCount = index_map + 14;
        nk_uint search = endCount;

        if (unicode_codepoint > 0xffff)
            return 0;

        /* they lie from endCount .. endCount + segCount */
        /* but searchRange is the nearest power of two, so... */
        if (unicode_codepoint >= nk_ttUSHORT(data + search + rangeShift*2))
            search += (nk_uint)(rangeShift*2);

        /* now decrement to bias correctly to find smallest */
        search -= 2;
        while (entrySelector) {
            nk_ushort end;
            searchRange >>= 1;
            end = nk_ttUSHORT(data + search + searchRange*2);
            if (unicode_codepoint > end)
                search += (nk_uint)(searchRange*2);
            --entrySelector;
        }
        search += 2;

      {
         nk_ushort offset, start;
         nk_ushort item = (nk_ushort) ((search - endCount) >> 1);

         NK_ASSERT(unicode_codepoint <= nk_ttUSHORT(data + endCount + 2*item));
         start = nk_ttUSHORT(data + index_map + 14 + segcount*2 + 2 + 2*item);
         if (unicode_codepoint < start)
            return 0;

         offset = nk_ttUSHORT(data + index_map + 14 + segcount*6 + 2 + 2*item);
         if (offset == 0)
            return (nk_ushort) (unicode_codepoint + nk_ttSHORT(data + index_map + 14 + segcount*4 + 2 + 2*item));

         return nk_ttUSHORT(data + offset + (unicode_codepoint-start)*2 + index_map + 14 + segcount*6 + 2 + 2*item);
      }
   } else if (format == 12 || format == 13) {
        nk_uint ngroups = nk_ttULONG(data+index_map+12);
        nk_int low,high;
        low = 0; high = (nk_int)ngroups;
        /* Binary search the right group. */
        while (low < high) {
            nk_int mid = low + ((high-low) >> 1); /* rounds down, so low <= mid < high */
            nk_uint start_char = nk_ttULONG(data+index_map+16+mid*12);
            nk_uint end_char = nk_ttULONG(data+index_map+16+mid*12+4);
            if ((nk_uint) unicode_codepoint < start_char)
                high = mid;
            else if ((nk_uint) unicode_codepoint > end_char)
                low = mid+1;
            else {
                nk_uint start_glyph = nk_ttULONG(data+index_map+16+mid*12+8);
                if (format == 12)
                    return (int)start_glyph + (int)unicode_codepoint - (int)start_char;
                else /* format == 13 */
                    return (int)start_glyph;
            }
        }
        return 0; /* not found */
    }
    /* @TODO */
    NK_ASSERT(0);
    return 0;
}
NK_INTERN void
nk_tt_setvertex(struct nk_tt_vertex *v, nk_byte type, nk_int x, nk_int y, nk_int cx, nk_int cy)
{
    v->type = type;
    v->x = (nk_short) x;
    v->y = (nk_short) y;
    v->cx = (nk_short) cx;
    v->cy = (nk_short) cy;
}
NK_INTERN int
nk_tt__GetGlyfOffset(const struct nk_tt_fontinfo *info, int glyph_index)
{
    int g1,g2;
    if (glyph_index >= info->numGlyphs) return -1; /* glyph index out of range */
    if (info->indexToLocFormat >= 2)    return -1; /* unknown index->glyph map format */

    if (info->indexToLocFormat == 0) {
        g1 = info->glyf + nk_ttUSHORT(info->data + info->loca + glyph_index * 2) * 2;
        g2 = info->glyf + nk_ttUSHORT(info->data + info->loca + glyph_index * 2 + 2) * 2;
    } else {
        g1 = info->glyf + (int)nk_ttULONG (info->data + info->loca + glyph_index * 4);
        g2 = info->glyf + (int)nk_ttULONG (info->data + info->loca + glyph_index * 4 + 4);
    }
    return g1==g2 ? -1 : g1; /* if length is 0, return -1 */
}
NK_INTERN int
nk_tt_GetGlyphBox(const struct nk_tt_fontinfo *info, int glyph_index,
    int *x0, int *y0, int *x1, int *y1)
{
    int g = nk_tt__GetGlyfOffset(info, glyph_index);
    if (g < 0) return 0;

    if (x0) *x0 = nk_ttSHORT(info->data + g + 2);
    if (y0) *y0 = nk_ttSHORT(info->data + g + 4);
    if (x1) *x1 = nk_ttSHORT(info->data + g + 6);
    if (y1) *y1 = nk_ttSHORT(info->data + g + 8);
    return 1;
}
NK_INTERN int
nk_tt__close_shape(struct nk_tt_vertex *vertices, int num_vertices, int was_off,
    int start_off, nk_int sx, nk_int sy, nk_int scx, nk_int scy, nk_int cx, nk_int cy)
{
   if (start_off) {
      if (was_off)
         nk_tt_setvertex(&vertices[num_vertices++], NK_TT_vcurve, (cx+scx)>>1, (cy+scy)>>1, cx,cy);
      nk_tt_setvertex(&vertices[num_vertices++], NK_TT_vcurve, sx,sy,scx,scy);
   } else {
      if (was_off)
         nk_tt_setvertex(&vertices[num_vertices++], NK_TT_vcurve,sx,sy,cx,cy);
      else
         nk_tt_setvertex(&vertices[num_vertices++], NK_TT_vline,sx,sy,0,0);
   }
   return num_vertices;
}
NK_INTERN int
nk_tt_GetGlyphShape(const struct nk_tt_fontinfo *info, struct nk_allocator *alloc,
    int glyph_index, struct nk_tt_vertex **pvertices)
{
    nk_short numberOfContours;
    const nk_byte *endPtsOfContours;
    const nk_byte *data = info->data;
    struct nk_tt_vertex *vertices=0;
    int num_vertices=0;
    int g = nk_tt__GetGlyfOffset(info, glyph_index);
    *pvertices = 0;

    if (g < 0) return 0;
    numberOfContours = nk_ttSHORT(data + g);
    if (numberOfContours > 0) {
        nk_byte flags=0,flagcount;
        nk_int ins, i,j=0,m,n, next_move, was_off=0, off, start_off=0;
        nk_int x,y,cx,cy,sx,sy, scx,scy;
        const nk_byte *points;
        endPtsOfContours = (data + g + 10);
        ins = nk_ttUSHORT(data + g + 10 + numberOfContours * 2);
        points = data + g + 10 + numberOfContours * 2 + 2 + ins;

        n = 1+nk_ttUSHORT(endPtsOfContours + numberOfContours*2-2);
        m = n + 2*numberOfContours;  /* a loose bound on how many vertices we might need */
        vertices = (struct nk_tt_vertex *)alloc->alloc(alloc->userdata, 0, (nk_size)m * sizeof(vertices[0]));
        if (vertices == 0)
            return 0;

        next_move = 0;
        flagcount=0;

        /* in first pass, we load uninterpreted data into the allocated array */
        /* above, shifted to the end of the array so we won't overwrite it when */
        /* we create our final data starting from the front */
        off = m - n; /* starting offset for uninterpreted data, regardless of how m ends up being calculated */

        /* first load flags */
        for (i=0; i < n; ++i) {
            if (flagcount == 0) {
                flags = *points++;
                if (flags & 8)
                    flagcount = *points++;
            } else --flagcount;
            vertices[off+i].type = flags;
        }

        /* now load x coordinates */
        x=0;
        for (i=0; i < n; ++i) {
            flags = vertices[off+i].type;
            if (flags & 2) {
                nk_short dx = *points++;
                x += (flags & 16) ? dx : -dx; /* ??? */
            } else {
                if (!(flags & 16)) {
                    x = x + (nk_short) (points[0]*256 + points[1]);
                    points += 2;
                }
            }
            vertices[off+i].x = (nk_short) x;
        }

        /* now load y coordinates */
        y=0;
        for (i=0; i < n; ++i) {
            flags = vertices[off+i].type;
            if (flags & 4) {
                nk_short dy = *points++;
                y += (flags & 32) ? dy : -dy; /* ??? */
            } else {
                if (!(flags & 32)) {
                    y = y + (nk_short) (points[0]*256 + points[1]);
                    points += 2;
                }
            }
            vertices[off+i].y = (nk_short) y;
        }

        /* now convert them to our format */
        num_vertices=0;
        sx = sy = cx = cy = scx = scy = 0;
        for (i=0; i < n; ++i)
        {
            flags = vertices[off+i].type;
            x     = (nk_short) vertices[off+i].x;
            y     = (nk_short) vertices[off+i].y;

            if (next_move == i) {
                if (i != 0)
                    num_vertices = nk_tt__close_shape(vertices, num_vertices, was_off, start_off, sx,sy,scx,scy,cx,cy);

                /* now start the new one                */
                start_off = !(flags & 1);
                if (start_off) {
                    /* if we start off with an off-curve point, then when we need to find a point on the curve */
                    /* where we can start, and we need to save some state for when we wraparound. */
                    scx = x;
                    scy = y;
                    if (!(vertices[off+i+1].type & 1)) {
                        /* next point is also a curve point, so interpolate an on-point curve */
                        sx = (x + (nk_int) vertices[off+i+1].x) >> 1;
                        sy = (y + (nk_int) vertices[off+i+1].y) >> 1;
                    } else {
                        /* otherwise just use the next point as our start point */
                        sx = (nk_int) vertices[off+i+1].x;
                        sy = (nk_int) vertices[off+i+1].y;
                        ++i; /* we're using point i+1 as the starting point, so skip it */
                    }
                } else {
                    sx = x;
                    sy = y;
                }
                nk_tt_setvertex(&vertices[num_vertices++], NK_TT_vmove,sx,sy,0,0);
                was_off = 0;
                next_move = 1 + nk_ttUSHORT(endPtsOfContours+j*2);
                ++j;
            } else {
                if (!(flags & 1))
                { /* if it's a curve */
                    if (was_off) /* two off-curve control points in a row means interpolate an on-curve midpoint */
                        nk_tt_setvertex(&vertices[num_vertices++], NK_TT_vcurve, (cx+x)>>1, (cy+y)>>1, cx, cy);
                    cx = x;
                    cy = y;
                    was_off = 1;
                } else {
                    if (was_off)
                        nk_tt_setvertex(&vertices[num_vertices++], NK_TT_vcurve, x,y, cx, cy);
                    else nk_tt_setvertex(&vertices[num_vertices++], NK_TT_vline, x,y,0,0);
                    was_off = 0;
                }
            }
        }
        num_vertices = nk_tt__close_shape(vertices, num_vertices, was_off, start_off, sx,sy,scx,scy,cx,cy);
    } else if (numberOfContours == -1) {
        /* Compound shapes. */
        int more = 1;
        const nk_byte *comp = data + g + 10;
        num_vertices = 0;
        vertices = 0;

        while (more)
        {
            nk_ushort flags, gidx;
            int comp_num_verts = 0, i;
            struct nk_tt_vertex *comp_verts = 0, *tmp = 0;
            float mtx[6] = {1,0,0,1,0,0}, m, n;

            flags = (nk_ushort)nk_ttSHORT(comp); comp+=2;
            gidx = (nk_ushort)nk_ttSHORT(comp); comp+=2;

            if (flags & 2) { /* XY values */
                if (flags & 1) { /* shorts */
                    mtx[4] = nk_ttSHORT(comp); comp+=2;
                    mtx[5] = nk_ttSHORT(comp); comp+=2;
                } else {
                    mtx[4] = nk_ttCHAR(comp); comp+=1;
                    mtx[5] = nk_ttCHAR(comp); comp+=1;
                }
            } else {
                /* @TODO handle matching point */
                NK_ASSERT(0);
            }
            if (flags & (1<<3)) { /* WE_HAVE_A_SCALE */
                mtx[0] = mtx[3] = nk_ttSHORT(comp)/16384.0f; comp+=2;
                mtx[1] = mtx[2] = 0;
            } else if (flags & (1<<6)) { /* WE_HAVE_AN_X_AND_YSCALE */
                mtx[0] = nk_ttSHORT(comp)/16384.0f; comp+=2;
                mtx[1] = mtx[2] = 0;
                mtx[3] = nk_ttSHORT(comp)/16384.0f; comp+=2;
            } else if (flags & (1<<7)) { /* WE_HAVE_A_TWO_BY_TWO */
                mtx[0] = nk_ttSHORT(comp)/16384.0f; comp+=2;
                mtx[1] = nk_ttSHORT(comp)/16384.0f; comp+=2;
                mtx[2] = nk_ttSHORT(comp)/16384.0f; comp+=2;
                mtx[3] = nk_ttSHORT(comp)/16384.0f; comp+=2;
            }

             /* Find transformation scales. */
            m = (float) NK_SQRT(mtx[0]*mtx[0] + mtx[1]*mtx[1]);
            n = (float) NK_SQRT(mtx[2]*mtx[2] + mtx[3]*mtx[3]);

             /* Get indexed glyph. */
            comp_num_verts = nk_tt_GetGlyphShape(info, alloc, gidx, &comp_verts);
            if (comp_num_verts > 0)
            {
                /* Transform vertices. */
                for (i = 0; i < comp_num_verts; ++i) {
                    struct nk_tt_vertex* v = &comp_verts[i];
                    short x,y;
                    x=v->x; y=v->y;
                    v->x = (short)(m * (mtx[0]*x + mtx[2]*y + mtx[4]));
                    v->y = (short)(n * (mtx[1]*x + mtx[3]*y + mtx[5]));
                    x=v->cx; y=v->cy;
                    v->cx = (short)(m * (mtx[0]*x + mtx[2]*y + mtx[4]));
                    v->cy = (short)(n * (mtx[1]*x + mtx[3]*y + mtx[5]));
                }
                /* Append vertices. */
                tmp = (struct nk_tt_vertex*)alloc->alloc(alloc->userdata, 0,
                    (nk_size)(num_vertices+comp_num_verts)*sizeof(struct nk_tt_vertex));
                if (!tmp) {
                    if (vertices) alloc->free(alloc->userdata, vertices);
                    if (comp_verts) alloc->free(alloc->userdata, comp_verts);
                    return 0;
                }
                if (num_vertices > 0) NK_MEMCPY(tmp, vertices, (nk_size)num_vertices*sizeof(struct nk_tt_vertex));
                NK_MEMCPY(tmp+num_vertices, comp_verts, (nk_size)comp_num_verts*sizeof(struct nk_tt_vertex));
                if (vertices) alloc->free(alloc->userdata,vertices);
                vertices = tmp;
                alloc->free(alloc->userdata,comp_verts);
                num_vertices += comp_num_verts;
            }
            /* More components ? */
            more = flags & (1<<5);
        }
    } else if (numberOfContours < 0) {
        /* @TODO other compound variations? */
        NK_ASSERT(0);
    } else {
        /* numberOfCounters == 0, do nothing */
    }
    *pvertices = vertices;
    return num_vertices;
}
NK_INTERN void
nk_tt_GetGlyphHMetrics(const struct nk_tt_fontinfo *info, int glyph_index,
    int *advanceWidth, int *leftSideBearing)
{
    nk_ushort numOfLongHorMetrics = nk_ttUSHORT(info->data+info->hhea + 34);
    if (glyph_index < numOfLongHorMetrics) {
        if (advanceWidth)
            *advanceWidth    = nk_ttSHORT(info->data + info->hmtx + 4*glyph_index);
        if (leftSideBearing)
            *leftSideBearing = nk_ttSHORT(info->data + info->hmtx + 4*glyph_index + 2);
    } else {
        if (advanceWidth)
            *advanceWidth    = nk_ttSHORT(info->data + info->hmtx + 4*(numOfLongHorMetrics-1));
        if (leftSideBearing)
            *leftSideBearing = nk_ttSHORT(info->data + info->hmtx + 4*numOfLongHorMetrics + 2*(glyph_index - numOfLongHorMetrics));
    }
}
NK_INTERN void
nk_tt_GetFontVMetrics(const struct nk_tt_fontinfo *info,
    int *ascent, int *descent, int *lineGap)
{
   if (ascent ) *ascent  = nk_ttSHORT(info->data+info->hhea + 4);
   if (descent) *descent = nk_ttSHORT(info->data+info->hhea + 6);
   if (lineGap) *lineGap = nk_ttSHORT(info->data+info->hhea + 8);
}
NK_INTERN float
nk_tt_ScaleForPixelHeight(const struct nk_tt_fontinfo *info, float height)
{
   int fheight = nk_ttSHORT(info->data + info->hhea + 4) - nk_ttSHORT(info->data + info->hhea + 6);
   return (float) height / (float)fheight;
}
NK_INTERN float
nk_tt_ScaleForMappingEmToPixels(const struct nk_tt_fontinfo *info, float pixels)
{
   int unitsPerEm = nk_ttUSHORT(info->data + info->head + 18);
   return pixels / (float)unitsPerEm;
}

/*-------------------------------------------------------------
 *            antialiasing software rasterizer
 * --------------------------------------------------------------*/
NK_INTERN void
nk_tt_GetGlyphBitmapBoxSubpixel(const struct nk_tt_fontinfo *font,
    int glyph, float scale_x, float scale_y,float shift_x, float shift_y,
    int *ix0, int *iy0, int *ix1, int *iy1)
{
    int x0,y0,x1,y1;
    if (!nk_tt_GetGlyphBox(font, glyph, &x0,&y0,&x1,&y1)) {
        /* e.g. space character */
        if (ix0) *ix0 = 0;
        if (iy0) *iy0 = 0;
        if (ix1) *ix1 = 0;
        if (iy1) *iy1 = 0;
    } else {
        /* move to integral bboxes (treating pixels as little squares, what pixels get touched)? */
        if (ix0) *ix0 = nk_ifloorf((float)x0 * scale_x + shift_x);
        if (iy0) *iy0 = nk_ifloorf((float)-y1 * scale_y + shift_y);
        if (ix1) *ix1 = nk_iceilf ((float)x1 * scale_x + shift_x);
        if (iy1) *iy1 = nk_iceilf ((float)-y0 * scale_y + shift_y);
    }
}
NK_INTERN void
nk_tt_GetGlyphBitmapBox(const struct nk_tt_fontinfo *font, int glyph,
    float scale_x, float scale_y, int *ix0, int *iy0, int *ix1, int *iy1)
{
   nk_tt_GetGlyphBitmapBoxSubpixel(font, glyph, scale_x, scale_y,0.0f,0.0f, ix0, iy0, ix1, iy1);
}

/*-------------------------------------------------------------
 *                          Rasterizer
 * --------------------------------------------------------------*/
NK_INTERN void*
nk_tt__hheap_alloc(struct nk_tt__hheap *hh, nk_size size)
{
    if (hh->first_free) {
        void *p = hh->first_free;
        hh->first_free = * (void **) p;
        return p;
    } else {
        if (hh->num_remaining_in_head_chunk == 0) {
            int count = (size < 32 ? 2000 : size < 128 ? 800 : 100);
            struct nk_tt__hheap_chunk *c = (struct nk_tt__hheap_chunk *)
                hh->alloc.alloc(hh->alloc.userdata, 0,
                sizeof(struct nk_tt__hheap_chunk) + size * (nk_size)count);
            if (c == 0) return 0;
            c->next = hh->head;
            hh->head = c;
            hh->num_remaining_in_head_chunk = count;
        }
        --hh->num_remaining_in_head_chunk;
        return (char *) (hh->head) + size * (nk_size)hh->num_remaining_in_head_chunk;
    }
}
NK_INTERN void
nk_tt__hheap_free(struct nk_tt__hheap *hh, void *p)
{
    *(void **) p = hh->first_free;
    hh->first_free = p;
}
NK_INTERN void
nk_tt__hheap_cleanup(struct nk_tt__hheap *hh)
{
    struct nk_tt__hheap_chunk *c = hh->head;
    while (c) {
        struct nk_tt__hheap_chunk *n = c->next;
        hh->alloc.free(hh->alloc.userdata, c);
        c = n;
    }
}
NK_INTERN struct nk_tt__active_edge*
nk_tt__new_active(struct nk_tt__hheap *hh, struct nk_tt__edge *e,
    int off_x, float start_point)
{
    struct nk_tt__active_edge *z = (struct nk_tt__active_edge *)
        nk_tt__hheap_alloc(hh, sizeof(*z));
    float dxdy = (e->x1 - e->x0) / (e->y1 - e->y0);
    /*STBTT_assert(e->y0 <= start_point); */
    if (!z) return z;
    z->fdx = dxdy;
    z->fdy = (dxdy != 0) ? (1/dxdy): 0;
    z->fx = e->x0 + dxdy * (start_point - e->y0);
    z->fx -= (float)off_x;
    z->direction = e->invert ? 1.0f : -1.0f;
    z->sy = e->y0;
    z->ey = e->y1;
    z->next = 0;
    return z;
}
NK_INTERN void
nk_tt__handle_clipped_edge(float *scanline, int x, struct nk_tt__active_edge *e,
    float x0, float y0, float x1, float y1)
{
    if (y0 == y1) return;
    NK_ASSERT(y0 < y1);
    NK_ASSERT(e->sy <= e->ey);
    if (y0 > e->ey) return;
    if (y1 < e->sy) return;
    if (y0 < e->sy) {
        x0 += (x1-x0) * (e->sy - y0) / (y1-y0);
        y0 = e->sy;
    }
    if (y1 > e->ey) {
        x1 += (x1-x0) * (e->ey - y1) / (y1-y0);
        y1 = e->ey;
    }

    if (x0 == x) NK_ASSERT(x1 <= x+1);
    else if (x0 == x+1) NK_ASSERT(x1 >= x);
    else if (x0 <= x) NK_ASSERT(x1 <= x);
    else if (x0 >= x+1) NK_ASSERT(x1 >= x+1);
    else NK_ASSERT(x1 >= x && x1 <= x+1);

    if (x0 <= x && x1 <= x)
        scanline[x] += e->direction * (y1-y0);
    else if (x0 >= x+1 && x1 >= x+1);
    else {
        NK_ASSERT(x0 >= x && x0 <= x+1 && x1 >= x && x1 <= x+1);
        /* coverage = 1 - average x position */
        scanline[x] += (float)e->direction * (float)(y1-y0) * (1.0f-((x0-(float)x)+(x1-(float)x))/2.0f);
    }
}
NK_INTERN void
nk_tt__fill_active_edges_new(float *scanline, float *scanline_fill, int len,
    struct nk_tt__active_edge *e, float y_top)
{
    float y_bottom = y_top+1;
    while (e)
    {
        /* brute force every pixel */
        /* compute intersection points with top & bottom */
        NK_ASSERT(e->ey >= y_top);
        if (e->fdx == 0) {
            float x0 = e->fx;
            if (x0 < len) {
                if (x0 >= 0) {
                    nk_tt__handle_clipped_edge(scanline,(int) x0,e, x0,y_top, x0,y_bottom);
                    nk_tt__handle_clipped_edge(scanline_fill-1,(int) x0+1,e, x0,y_top, x0,y_bottom);
                } else {
                    nk_tt__handle_clipped_edge(scanline_fill-1,0,e, x0,y_top, x0,y_bottom);
                }
            }
        } else {
            float x0 = e->fx;
            float dx = e->fdx;
            float xb = x0 + dx;
            float x_top, x_bottom;
            float y0,y1;
            float dy = e->fdy;
            NK_ASSERT(e->sy <= y_bottom && e->ey >= y_top);

            /* compute endpoints of line segment clipped to this scanline (if the */
            /* line segment starts on this scanline. x0 is the intersection of the */
            /* line with y_top, but that may be off the line segment. */
            if (e->sy > y_top) {
                x_top = x0 + dx * (e->sy - y_top);
                y0 = e->sy;
            } else {
                x_top = x0;
                y0 = y_top;
            }

            if (e->ey < y_bottom) {
                x_bottom = x0 + dx * (e->ey - y_top);
                y1 = e->ey;
            } else {
                x_bottom = xb;
                y1 = y_bottom;
            }

            if (x_top >= 0 && x_bottom >= 0 && x_top < len && x_bottom < len)
            {
                /* from here on, we don't have to range check x values */
                if ((int) x_top == (int) x_bottom) {
                    float height;
                    /* simple case, only spans one pixel */
                    int x = (int) x_top;
                    height = y1 - y0;
                    NK_ASSERT(x >= 0 && x < len);
                    scanline[x] += e->direction * (1.0f-(((float)x_top - (float)x) + ((float)x_bottom-(float)x))/2.0f)  * (float)height;
                    scanline_fill[x] += e->direction * (float)height; /* everything right of this pixel is filled */
                } else {
                    int x,x1,x2;
                    float y_crossing, step, sign, area;
                    /* covers 2+ pixels */
                    if (x_top > x_bottom)
                    {
                        /* flip scanline vertically; signed area is the same */
                        float t;
                        y0 = y_bottom - (y0 - y_top);
                        y1 = y_bottom - (y1 - y_top);
                        t = y0; y0 = y1; y1 = t;
                        t = x_bottom; x_bottom = x_top; x_top = t;
                        dx = -dx;
                        dy = -dy;
                        t = x0; x0 = xb; xb = t;
                    }

                    x1 = (int) x_top;
                    x2 = (int) x_bottom;
                    /* compute intersection with y axis at x1+1 */
                    y_crossing = ((float)x1+1 - (float)x0) * (float)dy + (float)y_top;

                    sign = e->direction;
                    /* area of the rectangle covered from y0..y_crossing */
                    area = sign * (y_crossing-y0);
                    /* area of the triangle (x_top,y0), (x+1,y0), (x+1,y_crossing) */
                    scanline[x1] += area * (1.0f-((float)((float)x_top - (float)x1)+(float)(x1+1-x1))/2.0f);

                    step = sign * dy;
                    for (x = x1+1; x < x2; ++x) {
                        scanline[x] += area + step/2;
                        area += step;
                    }
                    y_crossing += (float)dy * (float)(x2 - (x1+1));

                    scanline[x2] += area + sign * (1.0f-((float)(x2-x2)+((float)x_bottom-(float)x2))/2.0f) * (y1-y_crossing);
                    scanline_fill[x2] += sign * (y1-y0);
                }
            }
            else
            {
                /* if edge goes outside of box we're drawing, we require */
                /* clipping logic. since this does not match the intended use */
                /* of this library, we use a different, very slow brute */
                /* force implementation */
                int x;
                for (x=0; x < len; ++x)
                {
                    /* cases: */
                    /* */
                    /* there can be up to two intersections with the pixel. any intersection */
                    /* with left or right edges can be handled by splitting into two (or three) */
                    /* regions. intersections with top & bottom do not necessitate case-wise logic. */
                    /* */
                    /* the old way of doing this found the intersections with the left & right edges, */
                    /* then used some simple logic to produce up to three segments in sorted order */
                    /* from top-to-bottom. however, this had a problem: if an x edge was epsilon */
                    /* across the x border, then the corresponding y position might not be distinct */
                    /* from the other y segment, and it might ignored as an empty segment. to avoid */
                    /* that, we need to explicitly produce segments based on x positions. */

                    /* rename variables to clear pairs */
                    float ya = y_top;
                    float x1 = (float) (x);
                    float x2 = (float) (x+1);
                    float x3 = xb;
                    float y3 = y_bottom;
                    float yb,y2;

                    yb = ((float)x - x0) / dx + y_top;
                    y2 = ((float)x+1 - x0) / dx + y_top;

                    if (x0 < x1 && x3 > x2) {         /* three segments descending down-right */
                        nk_tt__handle_clipped_edge(scanline,x,e, x0,ya, x1,yb);
                        nk_tt__handle_clipped_edge(scanline,x,e, x1,yb, x2,y2);
                        nk_tt__handle_clipped_edge(scanline,x,e, x2,y2, x3,y3);
                    } else if (x3 < x1 && x0 > x2) {  /* three segments descending down-left */
                        nk_tt__handle_clipped_edge(scanline,x,e, x0,ya, x2,y2);
                        nk_tt__handle_clipped_edge(scanline,x,e, x2,y2, x1,yb);
                        nk_tt__handle_clipped_edge(scanline,x,e, x1,yb, x3,y3);
                    } else if (x0 < x1 && x3 > x1) {  /* two segments across x, down-right */
                        nk_tt__handle_clipped_edge(scanline,x,e, x0,ya, x1,yb);
                        nk_tt__handle_clipped_edge(scanline,x,e, x1,yb, x3,y3);
                    } else if (x3 < x1 && x0 > x1) {  /* two segments across x, down-left */
                        nk_tt__handle_clipped_edge(scanline,x,e, x0,ya, x1,yb);
                        nk_tt__handle_clipped_edge(scanline,x,e, x1,yb, x3,y3);
                    } else if (x0 < x2 && x3 > x2) {  /* two segments across x+1, down-right */
                        nk_tt__handle_clipped_edge(scanline,x,e, x0,ya, x2,y2);
                        nk_tt__handle_clipped_edge(scanline,x,e, x2,y2, x3,y3);
                    } else if (x3 < x2 && x0 > x2) {  /* two segments across x+1, down-left */
                        nk_tt__handle_clipped_edge(scanline,x,e, x0,ya, x2,y2);
                        nk_tt__handle_clipped_edge(scanline,x,e, x2,y2, x3,y3);
                    } else {  /* one segment */
                        nk_tt__handle_clipped_edge(scanline,x,e, x0,ya, x3,y3);
                    }
                }
            }
        }
        e = e->next;
    }
}
NK_INTERN void
nk_tt__rasterize_sorted_edges(struct nk_tt__bitmap *result, struct nk_tt__edge *e,
    int n, int vsubsample, int off_x, int off_y, struct nk_allocator *alloc)
{
    /* directly AA rasterize edges w/o supersampling */
    struct nk_tt__hheap hh;
    struct nk_tt__active_edge *active = 0;
    int y,j=0, i;
    float scanline_data[129], *scanline, *scanline2;

    NK_UNUSED(vsubsample);
    nk_zero_struct(hh);
    hh.alloc = *alloc;

    if (result->w > 64)
        scanline = (float *) alloc->alloc(alloc->userdata,0, (nk_size)(result->w*2+1) * sizeof(float));
    else scanline = scanline_data;

    scanline2 = scanline + result->w;
    y = off_y;
    e[n].y0 = (float) (off_y + result->h) + 1;

    while (j < result->h)
    {
        /* find center of pixel for this scanline */
        float scan_y_top    = (float)y + 0.0f;
        float scan_y_bottom = (float)y + 1.0f;
        struct nk_tt__active_edge **step = &active;

        NK_MEMSET(scanline , 0, (nk_size)result->w*sizeof(scanline[0]));
        NK_MEMSET(scanline2, 0, (nk_size)(result->w+1)*sizeof(scanline[0]));

        /* update all active edges; */
        /* remove all active edges that terminate before the top of this scanline */
        while (*step) {
            struct nk_tt__active_edge * z = *step;
            if (z->ey <= scan_y_top) {
                *step = z->next; /* delete from list */
                NK_ASSERT(z->direction);
                z->direction = 0;
                nk_tt__hheap_free(&hh, z);
            } else {
                step = &((*step)->next); /* advance through list */
            }
        }

        /* insert all edges that start before the bottom of this scanline */
        while (e->y0 <= scan_y_bottom) {
            if (e->y0 != e->y1) {
                struct nk_tt__active_edge *z = nk_tt__new_active(&hh, e, off_x, scan_y_top);
                if (z != 0) {
                    NK_ASSERT(z->ey >= scan_y_top);
                    /* insert at front */
                    z->next = active;
                    active = z;
                }
            }
            ++e;
        }

        /* now process all active edges */
        if (active)
            nk_tt__fill_active_edges_new(scanline, scanline2+1, result->w, active, scan_y_top);

        {
            float sum = 0;
            for (i=0; i < result->w; ++i) {
                float k;
                int m;
                sum += scanline2[i];
                k = scanline[i] + sum;
                k = (float) NK_ABS(k) * 255.0f + 0.5f;
                m = (int) k;
                if (m > 255) m = 255;
                result->pixels[j*result->stride + i] = (unsigned char) m;
            }
        }
        /* advance all the edges */
        step = &active;
        while (*step) {
            struct nk_tt__active_edge *z = *step;
            z->fx += z->fdx; /* advance to position for current scanline */
            step = &((*step)->next); /* advance through list */
        }
        ++y;
        ++j;
    }
    nk_tt__hheap_cleanup(&hh);
    if (scanline != scanline_data)
        alloc->free(alloc->userdata, scanline);
}
NK_INTERN void
nk_tt__sort_edges_ins_sort(struct nk_tt__edge *p, int n)
{
    int i,j;
    #define NK_TT__COMPARE(a,b)  ((a)->y0 < (b)->y0)
    for (i=1; i < n; ++i) {
        struct nk_tt__edge t = p[i], *a = &t;
        j = i;
        while (j > 0) {
            struct nk_tt__edge *b = &p[j-1];
            int c = NK_TT__COMPARE(a,b);
            if (!c) break;
            p[j] = p[j-1];
            --j;
        }
        if (i != j)
            p[j] = t;
    }
}
NK_INTERN void
nk_tt__sort_edges_quicksort(struct nk_tt__edge *p, int n)
{
    /* threshold for transitioning to insertion sort */
    while (n > 12) {
        struct nk_tt__edge t;
        int c01,c12,c,m,i,j;

        /* compute median of three */
        m = n >> 1;
        c01 = NK_TT__COMPARE(&p[0],&p[m]);
        c12 = NK_TT__COMPARE(&p[m],&p[n-1]);

        /* if 0 >= mid >= end, or 0 < mid < end, then use mid */
        if (c01 != c12) {
            /* otherwise, we'll need to swap something else to middle */
            int z;
            c = NK_TT__COMPARE(&p[0],&p[n-1]);
            /* 0>mid && mid<n:  0>n => n; 0<n => 0 */
            /* 0<mid && mid>n:  0>n => 0; 0<n => n */
            z = (c == c12) ? 0 : n-1;
            t = p[z];
            p[z] = p[m];
            p[m] = t;
        }

        /* now p[m] is the median-of-three */
        /* swap it to the beginning so it won't move around */
        t = p[0];
        p[0] = p[m];
        p[m] = t;

        /* partition loop */
        i=1;
        j=n-1;
        for(;;) {
            /* handling of equality is crucial here */
            /* for sentinels & efficiency with duplicates */
            for (;;++i) {
                if (!NK_TT__COMPARE(&p[i], &p[0])) break;
            }
            for (;;--j) {
                if (!NK_TT__COMPARE(&p[0], &p[j])) break;
            }

            /* make sure we haven't crossed */
             if (i >= j) break;
             t = p[i];
             p[i] = p[j];
             p[j] = t;

            ++i;
            --j;

        }

        /* recurse on smaller side, iterate on larger */
        if (j < (n-i)) {
            nk_tt__sort_edges_quicksort(p,j);
            p = p+i;
            n = n-i;
        } else {
            nk_tt__sort_edges_quicksort(p+i, n-i);
            n = j;
        }
    }
}
NK_INTERN void
nk_tt__sort_edges(struct nk_tt__edge *p, int n)
{
   nk_tt__sort_edges_quicksort(p, n);
   nk_tt__sort_edges_ins_sort(p, n);
}
NK_INTERN void
nk_tt__rasterize(struct nk_tt__bitmap *result, struct nk_tt__point *pts,
    int *wcount, int windings, float scale_x, float scale_y,
    float shift_x, float shift_y, int off_x, int off_y, int invert,
    struct nk_allocator *alloc)
{
    float y_scale_inv = invert ? -scale_y : scale_y;
    struct nk_tt__edge *e;
    int n,i,j,k,m;
    int vsubsample = 1;
    /* vsubsample should divide 255 evenly; otherwise we won't reach full opacity */

    /* now we have to blow out the windings into explicit edge lists */
    n = 0;
    for (i=0; i < windings; ++i)
        n += wcount[i];

    e = (struct nk_tt__edge*)
       alloc->alloc(alloc->userdata, 0,(sizeof(*e) * (nk_size)(n+1)));
    if (e == 0) return;
    n = 0;

    m=0;
    for (i=0; i < windings; ++i)
    {
        struct nk_tt__point *p = pts + m;
        m += wcount[i];
        j = wcount[i]-1;
        for (k=0; k < wcount[i]; j=k++) {
            int a=k,b=j;
            /* skip the edge if horizontal */
            if (p[j].y == p[k].y)
                continue;

            /* add edge from j to k to the list */
            e[n].invert = 0;
            if (invert ? p[j].y > p[k].y : p[j].y < p[k].y) {
                e[n].invert = 1;
                a=j,b=k;
            }
            e[n].x0 = p[a].x * scale_x + shift_x;
            e[n].y0 = (p[a].y * y_scale_inv + shift_y) * (float)vsubsample;
            e[n].x1 = p[b].x * scale_x + shift_x;
            e[n].y1 = (p[b].y * y_scale_inv + shift_y) * (float)vsubsample;
            ++n;
        }
    }

    /* now sort the edges by their highest point (should snap to integer, and then by x) */
    /*STBTT_sort(e, n, sizeof(e[0]), nk_tt__edge_compare); */
    nk_tt__sort_edges(e, n);
    /* now, traverse the scanlines and find the intersections on each scanline, use xor winding rule */
    nk_tt__rasterize_sorted_edges(result, e, n, vsubsample, off_x, off_y, alloc);
    alloc->free(alloc->userdata, e);
}
NK_INTERN void
nk_tt__add_point(struct nk_tt__point *points, int n, float x, float y)
{
    if (!points) return; /* during first pass, it's unallocated */
    points[n].x = x;
    points[n].y = y;
}
NK_INTERN int
nk_tt__tesselate_curve(struct nk_tt__point *points, int *num_points,
    float x0, float y0, float x1, float y1, float x2, float y2,
    float objspace_flatness_squared, int n)
{
    /* tesselate until threshold p is happy...
     * @TODO warped to compensate for non-linear stretching */
    /* midpoint */
    float mx = (x0 + 2*x1 + x2)/4;
    float my = (y0 + 2*y1 + y2)/4;
    /* versus directly drawn line */
    float dx = (x0+x2)/2 - mx;
    float dy = (y0+y2)/2 - my;
    if (n > 16) /* 65536 segments on one curve better be enough! */
        return 1;

    /* half-pixel error allowed... need to be smaller if AA */
    if (dx*dx+dy*dy > objspace_flatness_squared) {
        nk_tt__tesselate_curve(points, num_points, x0,y0,
            (x0+x1)/2.0f,(y0+y1)/2.0f, mx,my, objspace_flatness_squared,n+1);
        nk_tt__tesselate_curve(points, num_points, mx,my,
            (x1+x2)/2.0f,(y1+y2)/2.0f, x2,y2, objspace_flatness_squared,n+1);
    } else {
        nk_tt__add_point(points, *num_points,x2,y2);
        *num_points = *num_points+1;
    }
    return 1;
}
NK_INTERN struct nk_tt__point*
nk_tt_FlattenCurves(struct nk_tt_vertex *vertices, int num_verts,
    float objspace_flatness, int **contour_lengths, int *num_contours,
    struct nk_allocator *alloc)
{
    /* returns number of contours */
    struct nk_tt__point *points=0;
    int num_points=0;
    float objspace_flatness_squared = objspace_flatness * objspace_flatness;
    int i;
    int n=0;
    int start=0;
    int pass;

    /* count how many "moves" there are to get the contour count */
    for (i=0; i < num_verts; ++i)
        if (vertices[i].type == NK_TT_vmove) ++n;

    *num_contours = n;
    if (n == 0) return 0;

    *contour_lengths = (int *)
        alloc->alloc(alloc->userdata,0, (sizeof(**contour_lengths) * (nk_size)n));
    if (*contour_lengths == 0) {
        *num_contours = 0;
        return 0;
    }

    /* make two passes through the points so we don't need to realloc */
    for (pass=0; pass < 2; ++pass)
    {
        float x=0,y=0;
        if (pass == 1) {
            points = (struct nk_tt__point *)
                alloc->alloc(alloc->userdata,0, (nk_size)num_points * sizeof(points[0]));
            if (points == 0) goto error;
        }
        num_points = 0;
        n= -1;

        for (i=0; i < num_verts; ++i)
        {
            switch (vertices[i].type) {
            case NK_TT_vmove:
                /* start the next contour */
                if (n >= 0)
                (*contour_lengths)[n] = num_points - start;
                ++n;
                start = num_points;

                x = vertices[i].x, y = vertices[i].y;
                nk_tt__add_point(points, num_points++, x,y);
                break;
            case NK_TT_vline:
               x = vertices[i].x, y = vertices[i].y;
               nk_tt__add_point(points, num_points++, x, y);
               break;
            case NK_TT_vcurve:
               nk_tt__tesselate_curve(points, &num_points, x,y,
                                        vertices[i].cx, vertices[i].cy,
                                        vertices[i].x,  vertices[i].y,
                                        objspace_flatness_squared, 0);
               x = vertices[i].x, y = vertices[i].y;
               break;
            default: break;
         }
      }
      (*contour_lengths)[n] = num_points - start;
   }
   return points;

error:
   alloc->free(alloc->userdata, points);
   alloc->free(alloc->userdata, *contour_lengths);
   *contour_lengths = 0;
   *num_contours = 0;
   return 0;
}
NK_INTERN void
nk_tt_Rasterize(struct nk_tt__bitmap *result, float flatness_in_pixels,
    struct nk_tt_vertex *vertices, int num_verts,
    float scale_x, float scale_y, float shift_x, float shift_y,
    int x_off, int y_off, int invert, struct nk_allocator *alloc)
{
    float scale = scale_x > scale_y ? scale_y : scale_x;
    int winding_count, *winding_lengths;
    struct nk_tt__point *windings = nk_tt_FlattenCurves(vertices, num_verts,
        flatness_in_pixels / scale, &winding_lengths, &winding_count, alloc);

    NK_ASSERT(alloc);
    if (windings) {
        nk_tt__rasterize(result, windings, winding_lengths, winding_count,
            scale_x, scale_y, shift_x, shift_y, x_off, y_off, invert, alloc);
        alloc->free(alloc->userdata, winding_lengths);
        alloc->free(alloc->userdata, windings);
    }
}
NK_INTERN void
nk_tt_MakeGlyphBitmapSubpixel(const struct nk_tt_fontinfo *info, unsigned char *output,
    int out_w, int out_h, int out_stride, float scale_x, float scale_y,
    float shift_x, float shift_y, int glyph, struct nk_allocator *alloc)
{
    int ix0,iy0;
    struct nk_tt_vertex *vertices;
    int num_verts = nk_tt_GetGlyphShape(info, alloc, glyph, &vertices);
    struct nk_tt__bitmap gbm;

    nk_tt_GetGlyphBitmapBoxSubpixel(info, glyph, scale_x, scale_y, shift_x,
        shift_y, &ix0,&iy0,0,0);
    gbm.pixels = output;
    gbm.w = out_w;
    gbm.h = out_h;
    gbm.stride = out_stride;

    if (gbm.w && gbm.h)
        nk_tt_Rasterize(&gbm, 0.35f, vertices, num_verts, scale_x, scale_y,
            shift_x, shift_y, ix0,iy0, 1, alloc);
    alloc->free(alloc->userdata, vertices);
}

/*-------------------------------------------------------------
 *                          Bitmap baking
 * --------------------------------------------------------------*/
NK_INTERN int
nk_tt_PackBegin(struct nk_tt_pack_context *spc, unsigned char *pixels,
    int pw, int ph, int stride_in_bytes, int padding, struct nk_allocator *alloc)
{
    int num_nodes = pw - padding;
    struct nk_rp_context *context = (struct nk_rp_context *)
        alloc->alloc(alloc->userdata,0, sizeof(*context));
    struct nk_rp_node *nodes = (struct nk_rp_node*)
        alloc->alloc(alloc->userdata,0, (sizeof(*nodes  ) * (nk_size)num_nodes));

    if (context == 0 || nodes == 0) {
        if (context != 0) alloc->free(alloc->userdata, context);
        if (nodes   != 0) alloc->free(alloc->userdata, nodes);
        return 0;
    }

    spc->width = pw;
    spc->height = ph;
    spc->pixels = pixels;
    spc->pack_info = context;
    spc->nodes = nodes;
    spc->padding = padding;
    spc->stride_in_bytes = (stride_in_bytes != 0) ? stride_in_bytes : pw;
    spc->h_oversample = 1;
    spc->v_oversample = 1;

    nk_rp_init_target(context, pw-padding, ph-padding, nodes, num_nodes);
    if (pixels)
        NK_MEMSET(pixels, 0, (nk_size)(pw*ph)); /* background of 0 around pixels */
    return 1;
}
NK_INTERN void
nk_tt_PackEnd(struct nk_tt_pack_context *spc, struct nk_allocator *alloc)
{
    alloc->free(alloc->userdata, spc->nodes);
    alloc->free(alloc->userdata, spc->pack_info);
}
NK_INTERN void
nk_tt_PackSetOversampling(struct nk_tt_pack_context *spc,
    unsigned int h_oversample, unsigned int v_oversample)
{
   NK_ASSERT(h_oversample <= NK_TT_MAX_OVERSAMPLE);
   NK_ASSERT(v_oversample <= NK_TT_MAX_OVERSAMPLE);
   if (h_oversample <= NK_TT_MAX_OVERSAMPLE)
      spc->h_oversample = h_oversample;
   if (v_oversample <= NK_TT_MAX_OVERSAMPLE)
      spc->v_oversample = v_oversample;
}
NK_INTERN void
nk_tt__h_prefilter(unsigned char *pixels, int w, int h, int stride_in_bytes,
    int kernel_width)
{
    unsigned char buffer[NK_TT_MAX_OVERSAMPLE];
    int safe_w = w - kernel_width;
    int j;

    for (j=0; j < h; ++j)
    {
        int i;
        unsigned int total;
        NK_MEMSET(buffer, 0, (nk_size)kernel_width);

        total = 0;

        /* make kernel_width a constant in common cases so compiler can optimize out the divide */
        switch (kernel_width) {
        case 2:
            for (i=0; i <= safe_w; ++i) {
                total += (unsigned int)(pixels[i] - buffer[i & NK_TT__OVER_MASK]);
                buffer[(i+kernel_width) & NK_TT__OVER_MASK] = pixels[i];
                pixels[i] = (unsigned char) (total / 2);
            }
            break;
        case 3:
            for (i=0; i <= safe_w; ++i) {
                total += (unsigned int)(pixels[i] - buffer[i & NK_TT__OVER_MASK]);
                buffer[(i+kernel_width) & NK_TT__OVER_MASK] = pixels[i];
                pixels[i] = (unsigned char) (total / 3);
            }
            break;
        case 4:
            for (i=0; i <= safe_w; ++i) {
                total += (unsigned int)pixels[i] - buffer[i & NK_TT__OVER_MASK];
                buffer[(i+kernel_width) & NK_TT__OVER_MASK] = pixels[i];
                pixels[i] = (unsigned char) (total / 4);
            }
            break;
        case 5:
            for (i=0; i <= safe_w; ++i) {
                total += (unsigned int)(pixels[i] - buffer[i & NK_TT__OVER_MASK]);
                buffer[(i+kernel_width) & NK_TT__OVER_MASK] = pixels[i];
                pixels[i] = (unsigned char) (total / 5);
            }
            break;
        default:
            for (i=0; i <= safe_w; ++i) {
                total += (unsigned int)(pixels[i] - buffer[i & NK_TT__OVER_MASK]);
                buffer[(i+kernel_width) & NK_TT__OVER_MASK] = pixels[i];
                pixels[i] = (unsigned char) (total / (unsigned int)kernel_width);
            }
            break;
        }

        for (; i < w; ++i) {
            NK_ASSERT(pixels[i] == 0);
            total -= (unsigned int)(buffer[i & NK_TT__OVER_MASK]);
            pixels[i] = (unsigned char) (total / (unsigned int)kernel_width);
        }
        pixels += stride_in_bytes;
    }
}
NK_INTERN void
nk_tt__v_prefilter(unsigned char *pixels, int w, int h, int stride_in_bytes,
    int kernel_width)
{
    unsigned char buffer[NK_TT_MAX_OVERSAMPLE];
    int safe_h = h - kernel_width;
    int j;

    for (j=0; j < w; ++j)
    {
        int i;
        unsigned int total;
        NK_MEMSET(buffer, 0, (nk_size)kernel_width);

        total = 0;

        /* make kernel_width a constant in common cases so compiler can optimize out the divide */
        switch (kernel_width) {
        case 2:
            for (i=0; i <= safe_h; ++i) {
                total += (unsigned int)(pixels[i*stride_in_bytes] - buffer[i & NK_TT__OVER_MASK]);
                buffer[(i+kernel_width) & NK_TT__OVER_MASK] = pixels[i*stride_in_bytes];
                pixels[i*stride_in_bytes] = (unsigned char) (total / 2);
            }
            break;
         case 3:
            for (i=0; i <= safe_h; ++i) {
                total += (unsigned int)(pixels[i*stride_in_bytes] - buffer[i & NK_TT__OVER_MASK]);
                buffer[(i+kernel_width) & NK_TT__OVER_MASK] = pixels[i*stride_in_bytes];
                pixels[i*stride_in_bytes] = (unsigned char) (total / 3);
            }
            break;
         case 4:
            for (i=0; i <= safe_h; ++i) {
                total += (unsigned int)(pixels[i*stride_in_bytes] - buffer[i & NK_TT__OVER_MASK]);
                buffer[(i+kernel_width) & NK_TT__OVER_MASK] = pixels[i*stride_in_bytes];
                pixels[i*stride_in_bytes] = (unsigned char) (total / 4);
            }
            break;
         case 5:
            for (i=0; i <= safe_h; ++i) {
                total += (unsigned int)(pixels[i*stride_in_bytes] - buffer[i & NK_TT__OVER_MASK]);
                buffer[(i+kernel_width) & NK_TT__OVER_MASK] = pixels[i*stride_in_bytes];
                pixels[i*stride_in_bytes] = (unsigned char) (total / 5);
            }
            break;
         default:
            for (i=0; i <= safe_h; ++i) {
                total += (unsigned int)(pixels[i*stride_in_bytes] - buffer[i & NK_TT__OVER_MASK]);
                buffer[(i+kernel_width) & NK_TT__OVER_MASK] = pixels[i*stride_in_bytes];
                pixels[i*stride_in_bytes] = (unsigned char) (total / (unsigned int)kernel_width);
            }
            break;
        }

        for (; i < h; ++i) {
            NK_ASSERT(pixels[i*stride_in_bytes] == 0);
            total -= (unsigned int)(buffer[i & NK_TT__OVER_MASK]);
            pixels[i*stride_in_bytes] = (unsigned char) (total / (unsigned int)kernel_width);
        }
        pixels += 1;
    }
}
NK_INTERN float
nk_tt__oversample_shift(int oversample)
{
    if (!oversample)
        return 0.0f;

    /* The prefilter is a box filter of width "oversample", */
    /* which shifts phase by (oversample - 1)/2 pixels in */
    /* oversampled space. We want to shift in the opposite */
    /* direction to counter this. */
    return (float)-(oversample - 1) / (2.0f * (float)oversample);
}
NK_INTERN int
nk_tt_PackFontRangesGatherRects(struct nk_tt_pack_context *spc,
    struct nk_tt_fontinfo *info, struct nk_tt_pack_range *ranges,
    int num_ranges, struct nk_rp_rect *rects)
{
    /* rects array must be big enough to accommodate all characters in the given ranges */
    int i,j,k;
    k = 0;

    for (i=0; i < num_ranges; ++i) {
        float fh = ranges[i].font_size;
        float scale = (fh > 0) ? nk_tt_ScaleForPixelHeight(info, fh):
            nk_tt_ScaleForMappingEmToPixels(info, -fh);
        ranges[i].h_oversample = (unsigned char) spc->h_oversample;
        ranges[i].v_oversample = (unsigned char) spc->v_oversample;
        for (j=0; j < ranges[i].num_chars; ++j) {
            int x0,y0,x1,y1;
            int codepoint = ranges[i].first_unicode_codepoint_in_range ?
                ranges[i].first_unicode_codepoint_in_range + j :
                ranges[i].array_of_unicode_codepoints[j];

            int glyph = nk_tt_FindGlyphIndex(info, codepoint);
            nk_tt_GetGlyphBitmapBoxSubpixel(info,glyph, scale * (float)spc->h_oversample,
                scale * (float)spc->v_oversample, 0,0, &x0,&y0,&x1,&y1);
            rects[k].w = (nk_rp_coord) (x1-x0 + spc->padding + (int)spc->h_oversample-1);
            rects[k].h = (nk_rp_coord) (y1-y0 + spc->padding + (int)spc->v_oversample-1);
            ++k;
        }
    }
    return k;
}
NK_INTERN int
nk_tt_PackFontRangesRenderIntoRects(struct nk_tt_pack_context *spc,
    struct nk_tt_fontinfo *info, struct nk_tt_pack_range *ranges,
    int num_ranges, struct nk_rp_rect *rects, struct nk_allocator *alloc)
{
    int i,j,k, return_value = 1;
    /* save current values */
    int old_h_over = (int)spc->h_oversample;
    int old_v_over = (int)spc->v_oversample;
    /* rects array must be big enough to accommodate all characters in the given ranges */

    k = 0;
    for (i=0; i < num_ranges; ++i)
    {
        float fh = ranges[i].font_size;
        float recip_h,recip_v,sub_x,sub_y;
        float scale = fh > 0 ? nk_tt_ScaleForPixelHeight(info, fh):
            nk_tt_ScaleForMappingEmToPixels(info, -fh);

        spc->h_oversample = ranges[i].h_oversample;
        spc->v_oversample = ranges[i].v_oversample;

        recip_h = 1.0f / (float)spc->h_oversample;
        recip_v = 1.0f / (float)spc->v_oversample;

        sub_x = nk_tt__oversample_shift((int)spc->h_oversample);
        sub_y = nk_tt__oversample_shift((int)spc->v_oversample);

        for (j=0; j < ranges[i].num_chars; ++j)
        {
            struct nk_rp_rect *r = &rects[k];
            if (r->was_packed)
            {
                struct nk_tt_packedchar *bc = &ranges[i].chardata_for_range[j];
                int advance, lsb, x0,y0,x1,y1;
                int codepoint = ranges[i].first_unicode_codepoint_in_range ?
                    ranges[i].first_unicode_codepoint_in_range + j :
                    ranges[i].array_of_unicode_codepoints[j];
                int glyph = nk_tt_FindGlyphIndex(info, codepoint);
                nk_rp_coord pad = (nk_rp_coord) spc->padding;

                /* pad on left and top */
                r->x = (nk_rp_coord)((int)r->x + (int)pad);
                r->y = (nk_rp_coord)((int)r->y + (int)pad);
                r->w = (nk_rp_coord)((int)r->w - (int)pad);
                r->h = (nk_rp_coord)((int)r->h - (int)pad);

                nk_tt_GetGlyphHMetrics(info, glyph, &advance, &lsb);
                nk_tt_GetGlyphBitmapBox(info, glyph, scale * (float)spc->h_oversample,
                        (scale * (float)spc->v_oversample), &x0,&y0,&x1,&y1);
                nk_tt_MakeGlyphBitmapSubpixel(info, spc->pixels + r->x + r->y*spc->stride_in_bytes,
                    (int)(r->w - spc->h_oversample+1), (int)(r->h - spc->v_oversample+1),
                    spc->stride_in_bytes, scale * (float)spc->h_oversample,
                    scale * (float)spc->v_oversample, 0,0, glyph, alloc);

                if (spc->h_oversample > 1)
                   nk_tt__h_prefilter(spc->pixels + r->x + r->y*spc->stride_in_bytes,
                        r->w, r->h, spc->stride_in_bytes, (int)spc->h_oversample);

                if (spc->v_oversample > 1)
                   nk_tt__v_prefilter(spc->pixels + r->x + r->y*spc->stride_in_bytes,
                        r->w, r->h, spc->stride_in_bytes, (int)spc->v_oversample);

                bc->x0       = (nk_ushort)  r->x;
                bc->y0       = (nk_ushort)  r->y;
                bc->x1       = (nk_ushort) (r->x + r->w);
                bc->y1       = (nk_ushort) (r->y + r->h);
                bc->xadvance = scale * (float)advance;
                bc->xoff     = (float)  x0 * recip_h + sub_x;
                bc->yoff     = (float)  y0 * recip_v + sub_y;
                bc->xoff2    = ((float)x0 + r->w) * recip_h + sub_x;
                bc->yoff2    = ((float)y0 + r->h) * recip_v + sub_y;
            } else {
                return_value = 0; /* if any fail, report failure */
            }
            ++k;
        }
    }
    /* restore original values */
    spc->h_oversample = (unsigned int)old_h_over;
    spc->v_oversample = (unsigned int)old_v_over;
    return return_value;
}
NK_INTERN void
nk_tt_GetPackedQuad(struct nk_tt_packedchar *chardata, int pw, int ph,
    int char_index, float *xpos, float *ypos, struct nk_tt_aligned_quad *q,
    int align_to_integer)
{
    float ipw = 1.0f / (float)pw, iph = 1.0f / (float)ph;
    struct nk_tt_packedchar *b = (struct nk_tt_packedchar*)(chardata + char_index);
    if (align_to_integer) {
        int tx = nk_ifloorf((*xpos + b->xoff) + 0.5f);
        int ty = nk_ifloorf((*ypos + b->yoff) + 0.5f);

        float x = (float)tx;
        float y = (float)ty;

        q->x0 = x;
        q->y0 = y;
        q->x1 = x + b->xoff2 - b->xoff;
        q->y1 = y + b->yoff2 - b->yoff;
    } else {
        q->x0 = *xpos + b->xoff;
        q->y0 = *ypos + b->yoff;
        q->x1 = *xpos + b->xoff2;
        q->y1 = *ypos + b->yoff2;
    }
    q->s0 = b->x0 * ipw;
    q->t0 = b->y0 * iph;
    q->s1 = b->x1 * ipw;
    q->t1 = b->y1 * iph;
    *xpos += b->xadvance;
}

/* -------------------------------------------------------------
 *
 *                          FONT BAKING
 *
 * --------------------------------------------------------------*/
struct nk_font_bake_data {
    struct nk_tt_fontinfo info;
    struct nk_rp_rect *rects;
    struct nk_tt_pack_range *ranges;
    nk_rune range_count;
};

struct nk_font_baker {
    struct nk_allocator alloc;
    struct nk_tt_pack_context spc;
    struct nk_font_bake_data *build;
    struct nk_tt_packedchar *packed_chars;
    struct nk_rp_rect *rects;
    struct nk_tt_pack_range *ranges;
};

NK_GLOBAL const nk_size nk_rect_align = NK_ALIGNOF(struct nk_rp_rect);
NK_GLOBAL const nk_size nk_range_align = NK_ALIGNOF(struct nk_tt_pack_range);
NK_GLOBAL const nk_size nk_char_align = NK_ALIGNOF(struct nk_tt_packedchar);
NK_GLOBAL const nk_size nk_build_align = NK_ALIGNOF(struct nk_font_bake_data);
NK_GLOBAL const nk_size nk_baker_align = NK_ALIGNOF(struct nk_font_baker);

NK_INTERN int
nk_range_count(const nk_rune *range)
{
    const nk_rune *iter = range;
    NK_ASSERT(range);
    if (!range) return 0;
    while (*(iter++) != 0);
    return (iter == range) ? 0 : (int)((iter - range)/2);
}
NK_INTERN int
nk_range_glyph_count(const nk_rune *range, int count)
{
    int i = 0;
    int total_glyphs = 0;
    for (i = 0; i < count; ++i) {
        int diff;
        nk_rune f = range[(i*2)+0];
        nk_rune t = range[(i*2)+1];
        NK_ASSERT(t >= f);
        diff = (int)((t - f) + 1);
        total_glyphs += diff;
    }
    return total_glyphs;
}
NK_API const nk_rune*
nk_font_default_glyph_ranges(void)
{
    NK_STORAGE const nk_rune ranges[] = {0x0020, 0x00FF, 0};
    return ranges;
}
NK_API const nk_rune*
nk_font_chinese_glyph_ranges(void)
{
    NK_STORAGE const nk_rune ranges[] = {
        0x0020, 0x00FF,
        0x3000, 0x30FF,
        0x31F0, 0x31FF,
        0xFF00, 0xFFEF,
        0x4e00, 0x9FAF,
        0
    };
    return ranges;
}
NK_API const nk_rune*
nk_font_cyrillic_glyph_ranges(void)
{
    NK_STORAGE const nk_rune ranges[] = {
        0x0020, 0x00FF,
        0x0400, 0x052F,
        0x2DE0, 0x2DFF,
        0xA640, 0xA69F,
        0
    };
    return ranges;
}
NK_API const nk_rune*
nk_font_korean_glyph_ranges(void)
{
    NK_STORAGE const nk_rune ranges[] = {
        0x0020, 0x00FF,
        0x3131, 0x3163,
        0xAC00, 0xD79D,
        0
    };
    return ranges;
}
NK_INTERN void
nk_font_baker_memory(nk_size *temp, int *glyph_count,
    struct nk_font_config *config_list, int count)
{
    int range_count = 0;
    int total_range_count = 0;
    struct nk_font_config *iter, *i;

    NK_ASSERT(config_list);
    NK_ASSERT(glyph_count);
    if (!config_list) {
        *temp = 0;
        *glyph_count = 0;
        return;
    }
    *glyph_count = 0;
    for (iter = config_list; iter; iter = iter->next) {
        i = iter;
        do {if (!i->range) iter->range = nk_font_default_glyph_ranges();
            range_count = nk_range_count(i->range);
            total_range_count += range_count;
            *glyph_count += nk_range_glyph_count(i->range, range_count);
        } while ((i = i->n) != iter);
    }
    *temp = (nk_size)*glyph_count * sizeof(struct nk_rp_rect);
    *temp += (nk_size)total_range_count * sizeof(struct nk_tt_pack_range);
    *temp += (nk_size)*glyph_count * sizeof(struct nk_tt_packedchar);
    *temp += (nk_size)count * sizeof(struct nk_font_bake_data);
    *temp += sizeof(struct nk_font_baker);
    *temp += nk_rect_align + nk_range_align + nk_char_align;
    *temp += nk_build_align + nk_baker_align;
}
NK_INTERN struct nk_font_baker*
nk_font_baker(void *memory, int glyph_count, int count, struct nk_allocator *alloc)
{
    struct nk_font_baker *baker;
    if (!memory) return 0;
    /* setup baker inside a memory block  */
    baker = (struct nk_font_baker*)NK_ALIGN_PTR(memory, nk_baker_align);
    baker->build = (struct nk_font_bake_data*)NK_ALIGN_PTR((baker + 1), nk_build_align);
    baker->packed_chars = (struct nk_tt_packedchar*)NK_ALIGN_PTR((baker->build + count), nk_char_align);
    baker->rects = (struct nk_rp_rect*)NK_ALIGN_PTR((baker->packed_chars + glyph_count), nk_rect_align);
    baker->ranges = (struct nk_tt_pack_range*)NK_ALIGN_PTR((baker->rects + glyph_count), nk_range_align);
    baker->alloc = *alloc;
    return baker;
}
NK_INTERN int
nk_font_bake_pack(struct nk_font_baker *baker,
    nk_size *image_memory, int *width, int *height, struct nk_recti *custom,
    const struct nk_font_config *config_list, int count,
    struct nk_allocator *alloc)
{
    NK_STORAGE const nk_size max_height = 1024 * 32;
    const struct nk_font_config *config_iter, *it;
    int total_glyph_count = 0;
    int total_range_count = 0;
    int range_count = 0;
    int i = 0;

    NK_ASSERT(image_memory);
    NK_ASSERT(width);
    NK_ASSERT(height);
    NK_ASSERT(config_list);
    NK_ASSERT(count);
    NK_ASSERT(alloc);

    if (!image_memory || !width || !height || !config_list || !count) return nk_false;
    for (config_iter = config_list; config_iter; config_iter = config_iter->next) {
        it = config_iter;
        do {range_count = nk_range_count(it->range);
            total_range_count += range_count;
            total_glyph_count += nk_range_glyph_count(it->range, range_count);
        } while ((it = it->n) != config_iter);
    }
    /* setup font baker from temporary memory */
    for (config_iter = config_list; config_iter; config_iter = config_iter->next) {
        it = config_iter;
        do {if (!nk_tt_InitFont(&baker->build[i++].info, (const unsigned char*)it->ttf_blob, 0))
            return nk_false;
        } while ((it = it->n) != config_iter);
    }
    *height = 0;
    *width = (total_glyph_count > 1000) ? 1024 : 512;
    nk_tt_PackBegin(&baker->spc, 0, (int)*width, (int)max_height, 0, 1, alloc);
    {
        int input_i = 0;
        int range_n = 0;
        int rect_n = 0;
        int char_n = 0;

        if (custom) {
            /* pack custom user data first so it will be in the upper left corner*/
            struct nk_rp_rect custom_space;
            nk_zero(&custom_space, sizeof(custom_space));
            custom_space.w = (nk_rp_coord)(custom->w);
            custom_space.h = (nk_rp_coord)(custom->h);

            nk_tt_PackSetOversampling(&baker->spc, 1, 1);
            nk_rp_pack_rects((struct nk_rp_context*)baker->spc.pack_info, &custom_space, 1);
            *height = NK_MAX(*height, (int)(custom_space.y + custom_space.h));

            custom->x = (short)custom_space.x;
            custom->y = (short)custom_space.y;
            custom->w = (short)custom_space.w;
            custom->h = (short)custom_space.h;
        }

        /* first font pass: pack all glyphs */
        for (input_i = 0, config_iter = config_list; input_i < count && config_iter;
            config_iter = config_iter->next) {
            it = config_iter;
            do {int n = 0;
                int glyph_count;
                const nk_rune *in_range;
                const struct nk_font_config *cfg = it;
                struct nk_font_bake_data *tmp = &baker->build[input_i++];

                /* count glyphs + ranges in current font */
                glyph_count = 0; range_count = 0;
                for (in_range = cfg->range; in_range[0] && in_range[1]; in_range += 2) {
                    glyph_count += (int)(in_range[1] - in_range[0]) + 1;
                    range_count++;
                }

                /* setup ranges  */
                tmp->ranges = baker->ranges + range_n;
                tmp->range_count = (nk_rune)range_count;
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
                nk_tt_PackSetOversampling(&baker->spc, cfg->oversample_h, cfg->oversample_v);
                n = nk_tt_PackFontRangesGatherRects(&baker->spc, &tmp->info,
                    tmp->ranges, (int)tmp->range_count, tmp->rects);
                nk_rp_pack_rects((struct nk_rp_context*)baker->spc.pack_info, tmp->rects, (int)n);

                /* texture height */
                for (i = 0; i < n; ++i) {
                    if (tmp->rects[i].was_packed)
                        *height = NK_MAX(*height, tmp->rects[i].y + tmp->rects[i].h);
                }
            } while ((it = it->n) != config_iter);
        }
        NK_ASSERT(rect_n == total_glyph_count);
        NK_ASSERT(char_n == total_glyph_count);
        NK_ASSERT(range_n == total_range_count);
    }
    *height = (int)nk_round_up_pow2((nk_uint)*height);
    *image_memory = (nk_size)(*width) * (nk_size)(*height);
    return nk_true;
}
NK_INTERN void
nk_font_bake(struct nk_font_baker *baker, void *image_memory, int width, int height,
    struct nk_font_glyph *glyphs, int glyphs_count,
    const struct nk_font_config *config_list, int font_count)
{
    int input_i = 0;
    nk_rune glyph_n = 0;
    const struct nk_font_config *config_iter;
    const struct nk_font_config *it;

    NK_ASSERT(image_memory);
    NK_ASSERT(width);
    NK_ASSERT(height);
    NK_ASSERT(config_list);
    NK_ASSERT(baker);
    NK_ASSERT(font_count);
    NK_ASSERT(glyphs_count);
    if (!image_memory || !width || !height || !config_list ||
        !font_count || !glyphs || !glyphs_count)
        return;

    /* second font pass: render glyphs */
    nk_zero(image_memory, (nk_size)((nk_size)width * (nk_size)height));
    baker->spc.pixels = (unsigned char*)image_memory;
    baker->spc.height = (int)height;
    for (input_i = 0, config_iter = config_list; input_i < font_count && config_iter;
        config_iter = config_iter->next) {
        it = config_iter;
        do {const struct nk_font_config *cfg = it;
            struct nk_font_bake_data *tmp = &baker->build[input_i++];
            nk_tt_PackSetOversampling(&baker->spc, cfg->oversample_h, cfg->oversample_v);
            nk_tt_PackFontRangesRenderIntoRects(&baker->spc, &tmp->info, tmp->ranges,
                (int)tmp->range_count, tmp->rects, &baker->alloc);
        } while ((it = it->n) != config_iter);
    } nk_tt_PackEnd(&baker->spc, &baker->alloc);

    /* third pass: setup font and glyphs */
    for (input_i = 0, config_iter = config_list; input_i < font_count && config_iter;
        config_iter = config_iter->next) {
        it = config_iter;
        do {nk_size i = 0;
            int char_idx = 0;
            nk_rune glyph_count = 0;
            const struct nk_font_config *cfg = it;
            struct nk_font_bake_data *tmp = &baker->build[input_i++];
            struct nk_baked_font *dst_font = cfg->font;

            float font_scale = nk_tt_ScaleForPixelHeight(&tmp->info, cfg->size);
            int unscaled_ascent, unscaled_descent, unscaled_line_gap;
            nk_tt_GetFontVMetrics(&tmp->info, &unscaled_ascent, &unscaled_descent,
                                    &unscaled_line_gap);

            /* fill baked font */
            if (!cfg->merge_mode) {
                dst_font->ranges = cfg->range;
                dst_font->height = cfg->size;
                dst_font->ascent = ((float)unscaled_ascent * font_scale);
                dst_font->descent = ((float)unscaled_descent * font_scale);
                dst_font->glyph_offset = glyph_n;
                // Need to zero this, or it will carry over from a previous
                // bake, and cause a segfault when accessing glyphs[].
                dst_font->glyph_count = 0;
            }

            /* fill own baked font glyph array */
            for (i = 0; i < tmp->range_count; ++i) {
                struct nk_tt_pack_range *range = &tmp->ranges[i];
                for (char_idx = 0; char_idx < range->num_chars; char_idx++)
                {
                    nk_rune codepoint = 0;
                    float dummy_x = 0, dummy_y = 0;
                    struct nk_tt_aligned_quad q;
                    struct nk_font_glyph *glyph;

                    /* query glyph bounds from stb_truetype */
                    const struct nk_tt_packedchar *pc = &range->chardata_for_range[char_idx];
                    if (!pc->x0 && !pc->x1 && !pc->y0 && !pc->y1) continue;
                    codepoint = (nk_rune)(range->first_unicode_codepoint_in_range + char_idx);
                    nk_tt_GetPackedQuad(range->chardata_for_range, (int)width,
                        (int)height, char_idx, &dummy_x, &dummy_y, &q, 0);

                    /* fill own glyph type with data */
                    glyph = &glyphs[dst_font->glyph_offset + dst_font->glyph_count + (unsigned int)glyph_count];
                    glyph->codepoint = codepoint;
                    glyph->x0 = q.x0; glyph->y0 = q.y0;
                    glyph->x1 = q.x1; glyph->y1 = q.y1;
                    glyph->y0 += (dst_font->ascent + 0.5f);
                    glyph->y1 += (dst_font->ascent + 0.5f);
                    glyph->w = glyph->x1 - glyph->x0 + 0.5f;
                    glyph->h = glyph->y1 - glyph->y0;

                    if (cfg->coord_type == NK_COORD_PIXEL) {
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
                    glyph_count++;
                }
            }
            dst_font->glyph_count += glyph_count;
            glyph_n += glyph_count;
        } while ((it = it->n) != config_iter);
    }
}
NK_INTERN void
nk_font_bake_custom_data(void *img_memory, int img_width, int img_height,
    struct nk_recti img_dst, const char *texture_data_mask, int tex_width,
    int tex_height, char white, char black)
{
    nk_byte *pixels;
    int y = 0;
    int x = 0;
    int n = 0;

    NK_ASSERT(img_memory);
    NK_ASSERT(img_width);
    NK_ASSERT(img_height);
    NK_ASSERT(texture_data_mask);
    NK_UNUSED(tex_height);
    if (!img_memory || !img_width || !img_height || !texture_data_mask)
        return;

    pixels = (nk_byte*)img_memory;
    for (y = 0, n = 0; y < tex_height; ++y) {
        for (x = 0; x < tex_width; ++x, ++n) {
            const int off0 = ((img_dst.x + x) + (img_dst.y + y) * img_width);
            const int off1 = off0 + 1 + tex_width;
            pixels[off0] = (texture_data_mask[n] == white) ? 0xFF : 0x00;
            pixels[off1] = (texture_data_mask[n] == black) ? 0xFF : 0x00;
        }
    }
}
NK_INTERN void
nk_font_bake_convert(void *out_memory, int img_width, int img_height,
    const void *in_memory)
{
    int n = 0;
    nk_rune *dst;
    const nk_byte *src;

    NK_ASSERT(out_memory);
    NK_ASSERT(in_memory);
    NK_ASSERT(img_width);
    NK_ASSERT(img_height);
    if (!out_memory || !in_memory || !img_height || !img_width) return;

    dst = (nk_rune*)out_memory;
    src = (const nk_byte*)in_memory;
    for (n = (int)(img_width * img_height); n > 0; n--)
        *dst++ = ((nk_rune)(*src++) << 24) | 0x00FFFFFF;
}

/* -------------------------------------------------------------
 *
 *                          FONT
 *
 * --------------------------------------------------------------*/
NK_INTERN float
nk_font_text_width(nk_handle handle, float height, const char *text, int len)
{
    nk_rune unicode;
    int text_len  = 0;
    float text_width = 0;
    int glyph_len = 0;
    float scale = 0;

    struct nk_font *font = (struct nk_font*)handle.ptr;
    NK_ASSERT(font);
    NK_ASSERT(font->glyphs);
    if (!font || !text || !len)
        return 0;

    scale = height/font->info.height;
    glyph_len = text_len = nk_utf_decode(text, &unicode, (int)len);
    if (!glyph_len) return 0;
    while (text_len <= (int)len && glyph_len) {
        const struct nk_font_glyph *g;
        if (unicode == NK_UTF_INVALID) break;

        /* query currently drawn glyph information */
        g = nk_font_find_glyph(font, unicode);
        text_width += g->xadvance * scale;

        /* offset next glyph */
        glyph_len = nk_utf_decode(text + text_len, &unicode, (int)len - text_len);
        text_len += glyph_len;
    }
    return text_width;
}
#ifdef NK_INCLUDE_VERTEX_BUFFER_OUTPUT
NK_INTERN void
nk_font_query_font_glyph(nk_handle handle, float height,
    struct nk_user_font_glyph *glyph, nk_rune codepoint, nk_rune next_codepoint)
{
    float scale;
    const struct nk_font_glyph *g;
    struct nk_font *font;

    NK_ASSERT(glyph);
    NK_UNUSED(next_codepoint);

    font = (struct nk_font*)handle.ptr;
    NK_ASSERT(font);
    NK_ASSERT(font->glyphs);
    if (!font || !glyph)
        return;

    scale = height/font->info.height;
    g = nk_font_find_glyph(font, codepoint);
    glyph->width = (g->x1 - g->x0) * scale;
    glyph->height = (g->y1 - g->y0) * scale;
    glyph->offset = nk_vec2(g->x0 * scale, g->y0 * scale);
    glyph->xadvance = (g->xadvance * scale);
    glyph->uv[0] = nk_vec2(g->u0, g->v0);
    glyph->uv[1] = nk_vec2(g->u1, g->v1);
}
#endif
NK_API const struct nk_font_glyph*
nk_font_find_glyph(struct nk_font *font, nk_rune unicode)
{
    int i = 0;
    int count;
    int total_glyphs = 0;
    const struct nk_font_glyph *glyph = 0;
    const struct nk_font_config *iter = 0;

    NK_ASSERT(font);
    NK_ASSERT(font->glyphs);
    NK_ASSERT(font->info.ranges);
    if (!font || !font->glyphs) return 0;

    glyph = font->fallback;
    iter = font->config;
    do {count = nk_range_count(iter->range);
        for (i = 0; i < count; ++i) {
            nk_rune f = iter->range[(i*2)+0];
            nk_rune t = iter->range[(i*2)+1];
            int diff = (int)((t - f) + 1);
            if (unicode >= f && unicode <= t)
                return &font->glyphs[((nk_rune)total_glyphs + (unicode - f))];
            total_glyphs += diff;
        }
    } while ((iter = iter->n) != font->config);
    return glyph;
}
NK_INTERN void
nk_font_init(struct nk_font *font, float pixel_height,
    nk_rune fallback_codepoint, struct nk_font_glyph *glyphs,
    const struct nk_baked_font *baked_font, nk_handle atlas)
{
    struct nk_baked_font baked;
    NK_ASSERT(font);
    NK_ASSERT(glyphs);
    NK_ASSERT(baked_font);
    if (!font || !glyphs || !baked_font)
        return;

    baked = *baked_font;
    font->fallback = 0;
    font->info = baked;
    font->scale = (float)pixel_height / (float)font->info.height;
    font->glyphs = &glyphs[baked_font->glyph_offset];
    font->texture = atlas;
    font->fallback_codepoint = fallback_codepoint;
    font->fallback = nk_font_find_glyph(font, fallback_codepoint);

    font->handle.height = font->info.height * font->scale;
    font->handle.width = nk_font_text_width;
    font->handle.userdata.ptr = font;
#ifdef NK_INCLUDE_VERTEX_BUFFER_OUTPUT
    font->handle.query = nk_font_query_font_glyph;
    font->handle.texture = font->texture;
#endif
}

/* ---------------------------------------------------------------------------
 *
 *                          DEFAULT FONT
 *
 * ProggyClean.ttf
 * Copyright (c) 2004, 2005 Tristan Grimmer
 * MIT license (see License.txt in http://www.upperbounds.net/download/ProggyClean.ttf.zip)
 * Download and more information at http://upperbounds.net
 *-----------------------------------------------------------------------------*/
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Woverlength-strings"
#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverlength-strings"
#endif

#ifdef NK_INCLUDE_DEFAULT_FONT

NK_GLOBAL const char nk_proggy_clean_ttf_compressed_data_base85[11980+1] =
    "7])#######hV0qs'/###[),##/l:$#Q6>##5[n42>c-TH`->>#/e>11NNV=Bv(*:.F?uu#(gRU.o0XGH`$vhLG1hxt9?W`#,5LsCp#-i>.r$<$6pD>Lb';9Crc6tgXmKVeU2cD4Eo3R/"
    "2*>]b(MC;$jPfY.;h^`IWM9<Lh2TlS+f-s$o6Q<BWH`YiU.xfLq$N;$0iR/GX:U(jcW2p/W*q?-qmnUCI;jHSAiFWM.R*kU@C=GH?a9wp8f$e.-4^Qg1)Q-GL(lf(r/7GrRgwV%MS=C#"
    "`8ND>Qo#t'X#(v#Y9w0#1D$CIf;W'#pWUPXOuxXuU(H9M(1<q-UE31#^-V'8IRUo7Qf./L>=Ke$$'5F%)]0^#0X@U.a<r:QLtFsLcL6##lOj)#.Y5<-R&KgLwqJfLgN&;Q?gI^#DY2uL"
    "i@^rMl9t=cWq6##weg>$FBjVQTSDgEKnIS7EM9>ZY9w0#L;>>#Mx&4Mvt//L[MkA#W@lK.N'[0#7RL_&#w+F%HtG9M#XL`N&.,GM4Pg;-<nLENhvx>-VsM.M0rJfLH2eTM`*oJMHRC`N"
    "kfimM2J,W-jXS:)r0wK#@Fge$U>`w'N7G#$#fB#$E^$#:9:hk+eOe--6x)F7*E%?76%^GMHePW-Z5l'&GiF#$956:rS?dA#fiK:)Yr+`&#0j@'DbG&#^$PG.Ll+DNa<XCMKEV*N)LN/N"
    "*b=%Q6pia-Xg8I$<MR&,VdJe$<(7G;Ckl'&hF;;$<_=X(b.RS%%)###MPBuuE1V:v&cX&#2m#(&cV]`k9OhLMbn%s$G2,B$BfD3X*sp5#l,$R#]x_X1xKX%b5U*[r5iMfUo9U`N99hG)"
    "tm+/Us9pG)XPu`<0s-)WTt(gCRxIg(%6sfh=ktMKn3j)<6<b5Sk_/0(^]AaN#(p/L>&VZ>1i%h1S9u5o@YaaW$e+b<TWFn/Z:Oh(Cx2$lNEoN^e)#CFY@@I;BOQ*sRwZtZxRcU7uW6CX"
    "ow0i(?$Q[cjOd[P4d)]>ROPOpxTO7Stwi1::iB1q)C_=dV26J;2,]7op$]uQr@_V7$q^%lQwtuHY]=DX,n3L#0PHDO4f9>dC@O>HBuKPpP*E,N+b3L#lpR/MrTEH.IAQk.a>D[.e;mc."
    "x]Ip.PH^'/aqUO/$1WxLoW0[iLA<QT;5HKD+@qQ'NQ(3_PLhE48R.qAPSwQ0/WK?Z,[x?-J;jQTWA0X@KJ(_Y8N-:/M74:/-ZpKrUss?d#dZq]DAbkU*JqkL+nwX@@47`5>w=4h(9.`G"
    "CRUxHPeR`5Mjol(dUWxZa(>STrPkrJiWx`5U7F#.g*jrohGg`cg:lSTvEY/EV_7H4Q9[Z%cnv;JQYZ5q.l7Zeas:HOIZOB?G<Nald$qs]@]L<J7bR*>gv:[7MI2k).'2($5FNP&EQ(,)"
    "U]W]+fh18.vsai00);D3@4ku5P?DP8aJt+;qUM]=+b'8@;mViBKx0DE[-auGl8:PJ&Dj+M6OC]O^((##]`0i)drT;-7X`=-H3[igUnPG-NZlo.#k@h#=Ork$m>a>$-?Tm$UV(?#P6YY#"
    "'/###xe7q.73rI3*pP/$1>s9)W,JrM7SN]'/4C#v$U`0#V.[0>xQsH$fEmPMgY2u7Kh(G%siIfLSoS+MK2eTM$=5,M8p`A.;_R%#u[K#$x4AG8.kK/HSB==-'Ie/QTtG?-.*^N-4B/ZM"
    "_3YlQC7(p7q)&](`6_c)$/*JL(L-^(]$wIM`dPtOdGA,U3:w2M-0<q-]L_?^)1vw'.,MRsqVr.L;aN&#/EgJ)PBc[-f>+WomX2u7lqM2iEumMTcsF?-aT=Z-97UEnXglEn1K-bnEO`gu"
    "Ft(c%=;Am_Qs@jLooI&NX;]0#j4#F14;gl8-GQpgwhrq8'=l_f-b49'UOqkLu7-##oDY2L(te+Mch&gLYtJ,MEtJfLh'x'M=$CS-ZZ%P]8bZ>#S?YY#%Q&q'3^Fw&?D)UDNrocM3A76/"
    "/oL?#h7gl85[qW/NDOk%16ij;+:1a'iNIdb-ou8.P*w,v5#EI$TWS>Pot-R*H'-SEpA:g)f+O$%%`kA#G=8RMmG1&O`>to8bC]T&$,n.LoO>29sp3dt-52U%VM#q7'DHpg+#Z9%H[K<L"
    "%a2E-grWVM3@2=-k22tL]4$##6We'8UJCKE[d_=%wI;'6X-GsLX4j^SgJ$##R*w,vP3wK#iiW&#*h^D&R?jp7+/u&#(AP##XU8c$fSYW-J95_-Dp[g9wcO&#M-h1OcJlc-*vpw0xUX&#"
    "OQFKNX@QI'IoPp7nb,QU//MQ&ZDkKP)X<WSVL(68uVl&#c'[0#(s1X&xm$Y%B7*K:eDA323j998GXbA#pwMs-jgD$9QISB-A_(aN4xoFM^@C58D0+Q+q3n0#3U1InDjF682-SjMXJK)("
    "h$hxua_K]ul92%'BOU&#BRRh-slg8KDlr:%L71Ka:.A;%YULjDPmL<LYs8i#XwJOYaKPKc1h:'9Ke,g)b),78=I39B;xiY$bgGw-&.Zi9InXDuYa%G*f2Bq7mn9^#p1vv%#(Wi-;/Z5h"
    "o;#2:;%d&#x9v68C5g?ntX0X)pT`;%pB3q7mgGN)3%(P8nTd5L7GeA-GL@+%J3u2:(Yf>et`e;)f#Km8&+DC$I46>#Kr]]u-[=99tts1.qb#q72g1WJO81q+eN'03'eM>&1XxY-caEnO"
    "j%2n8)),?ILR5^.Ibn<-X-Mq7[a82Lq:F&#ce+S9wsCK*x`569E8ew'He]h:sI[2LM$[guka3ZRd6:t%IG:;$%YiJ:Nq=?eAw;/:nnDq0(CYcMpG)qLN4$##&J<j$UpK<Q4a1]MupW^-"
    "sj_$%[HK%'F####QRZJ::Y3EGl4'@%FkiAOg#p[##O`gukTfBHagL<LHw%q&OV0##F=6/:chIm0@eCP8X]:kFI%hl8hgO@RcBhS-@Qb$%+m=hPDLg*%K8ln(wcf3/'DW-$.lR?n[nCH-"
    "eXOONTJlh:.RYF%3'p6sq:UIMA945&^HFS87@$EP2iG<-lCO$%c`uKGD3rC$x0BL8aFn--`ke%#HMP'vh1/R&O_J9'um,.<tx[@%wsJk&bUT2`0uMv7gg#qp/ij.L56'hl;.s5CUrxjO"
    "M7-##.l+Au'A&O:-T72L]P`&=;ctp'XScX*rU.>-XTt,%OVU4)S1+R-#dg0/Nn?Ku1^0f$B*P:Rowwm-`0PKjYDDM'3]d39VZHEl4,.j']Pk-M.h^&:0FACm$maq-&sgw0t7/6(^xtk%"
    "LuH88Fj-ekm>GA#_>568x6(OFRl-IZp`&b,_P'$M<Jnq79VsJW/mWS*PUiq76;]/NM_>hLbxfc$mj`,O;&%W2m`Zh:/)Uetw:aJ%]K9h:TcF]u_-Sj9,VK3M.*'&0D[Ca]J9gp8,kAW]"
    "%(?A%R$f<->Zts'^kn=-^@c4%-pY6qI%J%1IGxfLU9CP8cbPlXv);C=b),<2mOvP8up,UVf3839acAWAW-W?#ao/^#%KYo8fRULNd2.>%m]UK:n%r$'sw]J;5pAoO_#2mO3n,'=H5(et"
    "Hg*`+RLgv>=4U8guD$I%D:W>-r5V*%j*W:Kvej.Lp$<M-SGZ':+Q_k+uvOSLiEo(<aD/K<CCc`'Lx>'?;++O'>()jLR-^u68PHm8ZFWe+ej8h:9r6L*0//c&iH&R8pRbA#Kjm%upV1g:"
    "a_#Ur7FuA#(tRh#.Y5K+@?3<-8m0$PEn;J:rh6?I6uG<-`wMU'ircp0LaE_OtlMb&1#6T.#FDKu#1Lw%u%+GM+X'e?YLfjM[VO0MbuFp7;>Q&#WIo)0@F%q7c#4XAXN-U&VB<HFF*qL("
    "$/V,;(kXZejWO`<[5?\?ewY(*9=%wDc;,u<'9t3W-(H1th3+G]ucQ]kLs7df($/*JL]@*t7Bu_G3_7mp7<iaQjO@.kLg;x3B0lqp7Hf,^Ze7-##@/c58Mo(3;knp0%)A7?-W+eI'o8)b<"
    "nKnw'Ho8C=Y>pqB>0ie&jhZ[?iLR@@_AvA-iQC(=ksRZRVp7`.=+NpBC%rh&3]R:8XDmE5^V8O(x<<aG/1N$#FX$0V5Y6x'aErI3I$7x%E`v<-BY,)%-?Psf*l?%C3.mM(=/M0:JxG'?"
    "7WhH%o'a<-80g0NBxoO(GH<dM]n.+%q@jH?f.UsJ2Ggs&4<-e47&Kl+f//9@`b+?.TeN_&B8Ss?v;^Trk;f#YvJkl&w$]>-+k?'(<S:68tq*WoDfZu';mM?8X[ma8W%*`-=;D.(nc7/;"
    ")g:T1=^J$&BRV(-lTmNB6xqB[@0*o.erM*<SWF]u2=st-*(6v>^](H.aREZSi,#1:[IXaZFOm<-ui#qUq2$##Ri;u75OK#(RtaW-K-F`S+cF]uN`-KMQ%rP/Xri.LRcB##=YL3BgM/3M"
    "D?@f&1'BW-)Ju<L25gl8uhVm1hL$##*8###'A3/LkKW+(^rWX?5W_8g)a(m&K8P>#bmmWCMkk&#TR`C,5d>g)F;t,4:@_l8G/5h4vUd%&%950:VXD'QdWoY-F$BtUwmfe$YqL'8(PWX("
    "P?^@Po3$##`MSs?DWBZ/S>+4%>fX,VWv/w'KD`LP5IbH;rTV>n3cEK8U#bX]l-/V+^lj3;vlMb&[5YQ8#pekX9JP3XUC72L,,?+Ni&co7ApnO*5NK,((W-i:$,kp'UDAO(G0Sq7MVjJs"
    "bIu)'Z,*[>br5fX^:FPAWr-m2KgL<LUN098kTF&#lvo58=/vjDo;.;)Ka*hLR#/k=rKbxuV`>Q_nN6'8uTG&#1T5g)uLv:873UpTLgH+#FgpH'_o1780Ph8KmxQJ8#H72L4@768@Tm&Q"
    "h4CB/5OvmA&,Q&QbUoi$a_%3M01H)4x7I^&KQVgtFnV+;[Pc>[m4k//,]1?#`VY[Jr*3&&slRfLiVZJ:]?=K3Sw=[$=uRB?3xk48@aeg<Z'<$#4H)6,>e0jT6'N#(q%.O=?2S]u*(m<-"
    "V8J'(1)G][68hW$5'q[GC&5j`TE?m'esFGNRM)j,ffZ?-qx8;->g4t*:CIP/[Qap7/9'#(1sao7w-.qNUdkJ)tCF&#B^;xGvn2r9FEPFFFcL@.iFNkTve$m%#QvQS8U@)2Z+3K:AKM5i"
    "sZ88+dKQ)W6>J%CL<KE>`.d*(B`-n8D9oK<Up]c$X$(,)M8Zt7/[rdkqTgl-0cuGMv'?>-XV1q['-5k'cAZ69e;D_?$ZPP&s^+7])$*$#@QYi9,5P&#9r+$%CE=68>K8r0=dSC%%(@p7"
    ".m7jilQ02'0-VWAg<a/''3u.=4L$Y)6k/K:_[3=&jvL<L0C/2'v:^;-DIBW,B4E68:kZ;%?8(Q8BH=kO65BW?xSG&#@uU,DS*,?.+(o(#1vCS8#CHF>TlGW'b)Tq7VT9q^*^$$.:&N@@"
    "$&)WHtPm*5_rO0&e%K&#-30j(E4#'Zb.o/(Tpm$>K'f@[PvFl,hfINTNU6u'0pao7%XUp9]5.>%h`8_=VYbxuel.NTSsJfLacFu3B'lQSu/m6-Oqem8T+oE--$0a/k]uj9EwsG>%veR*"
    "hv^BFpQj:K'#SJ,sB-'#](j.Lg92rTw-*n%@/;39rrJF,l#qV%OrtBeC6/,;qB3ebNW[?,Hqj2L.1NP&GjUR=1D8QaS3Up&@*9wP?+lo7b?@%'k4`p0Z$22%K3+iCZj?XJN4Nm&+YF]u"
    "@-W$U%VEQ/,,>>#)D<h#`)h0:<Q6909ua+&VU%n2:cG3FJ-%@Bj-DgLr`Hw&HAKjKjseK</xKT*)B,N9X3]krc12t'pgTV(Lv-tL[xg_%=M_q7a^x?7Ubd>#%8cY#YZ?=,`Wdxu/ae&#"
    "w6)R89tI#6@s'(6Bf7a&?S=^ZI_kS&ai`&=tE72L_D,;^R)7[$s<Eh#c&)q.MXI%#v9ROa5FZO%sF7q7Nwb&#ptUJ:aqJe$Sl68%.D###EC><?-aF&#RNQv>o8lKN%5/$(vdfq7+ebA#"
    "u1p]ovUKW&Y%q]'>$1@-[xfn$7ZTp7mM,G,Ko7a&Gu%G[RMxJs[0MM%wci.LFDK)(<c`Q8N)jEIF*+?P2a8g%)$q]o2aH8C&<SibC/q,(e:v;-b#6[$NtDZ84Je2KNvB#$P5?tQ3nt(0"
    "d=j.LQf./Ll33+(;q3L-w=8dX$#WF&uIJ@-bfI>%:_i2B5CsR8&9Z&#=mPEnm0f`<&c)QL5uJ#%u%lJj+D-r;BoF&#4DoS97h5g)E#o:&S4weDF,9^Hoe`h*L+_a*NrLW-1pG_&2UdB8"
    "6e%B/:=>)N4xeW.*wft-;$'58-ESqr<b?UI(_%@[P46>#U`'6AQ]m&6/`Z>#S?YY#Vc;r7U2&326d=w&H####?TZ`*4?&.MK?LP8Vxg>$[QXc%QJv92.(Db*B)gb*BM9dM*hJMAo*c&#"
    "b0v=Pjer]$gG&JXDf->'StvU7505l9$AFvgYRI^&<^b68?j#q9QX4SM'RO#&sL1IM.rJfLUAj221]d##DW=m83u5;'bYx,*Sl0hL(W;;$doB&O/TQ:(Z^xBdLjL<Lni;''X.`$#8+1GD"
    ":k$YUWsbn8ogh6rxZ2Z9]%nd+>V#*8U_72Lh+2Q8Cj0i:6hp&$C/:p(HK>T8Y[gHQ4`4)'$Ab(Nof%V'8hL&#<NEdtg(n'=S1A(Q1/I&4([%dM`,Iu'1:_hL>SfD07&6D<fp8dHM7/g+"
    "tlPN9J*rKaPct&?'uBCem^jn%9_K)<,C5K3s=5g&GmJb*[SYq7K;TRLGCsM-$$;S%:Y@r7AK0pprpL<Lrh,q7e/%KWK:50I^+m'vi`3?%Zp+<-d+$L-Sv:@.o19n$s0&39;kn;S%BSq*"
    "$3WoJSCLweV[aZ'MQIjO<7;X-X;&+dMLvu#^UsGEC9WEc[X(wI7#2.(F0jV*eZf<-Qv3J-c+J5AlrB#$p(H68LvEA'q3n0#m,[`*8Ft)FcYgEud]CWfm68,(aLA$@EFTgLXoBq/UPlp7"
    ":d[/;r_ix=:TF`S5H-b<LI&HY(K=h#)]Lk$K14lVfm:x$H<3^Ql<M`$OhapBnkup'D#L$Pb_`N*g]2e;X/Dtg,bsj&K#2[-:iYr'_wgH)NUIR8a1n#S?Yej'h8^58UbZd+^FKD*T@;6A"
    "7aQC[K8d-(v6GI$x:T<&'Gp5Uf>@M.*J:;$-rv29'M]8qMv-tLp,'886iaC=Hb*YJoKJ,(j%K=H`K.v9HggqBIiZu'QvBT.#=)0ukruV&.)3=(^1`o*Pj4<-<aN((^7('#Z0wK#5GX@7"
    "u][`*S^43933A4rl][`*O4CgLEl]v$1Q3AeF37dbXk,.)vj#x'd`;qgbQR%FW,2(?LO=s%Sc68%NP'##Aotl8x=BE#j1UD([3$M(]UI2LX3RpKN@;/#f'f/&_mt&F)XdF<9t4)Qa.*kT"
    "LwQ'(TTB9.xH'>#MJ+gLq9-##@HuZPN0]u:h7.T..G:;$/Usj(T7`Q8tT72LnYl<-qx8;-HV7Q-&Xdx%1a,hC=0u+HlsV>nuIQL-5<N?)NBS)QN*_I,?&)2'IM%L3I)X((e/dl2&8'<M"
    ":^#M*Q+[T.Xri.LYS3v%fF`68h;b-X[/En'CR.q7E)p'/kle2HM,u;^%OKC-N+Ll%F9CF<Nf'^#t2L,;27W:0O@6##U6W7:$rJfLWHj$#)woqBefIZ.PK<b*t7ed;p*_m;4ExK#h@&]>"
    "_>@kXQtMacfD.m-VAb8;IReM3$wf0''hra*so568'Ip&vRs849'MRYSp%:t:h5qSgwpEr$B>Q,;s(C#$)`svQuF$##-D,##,g68@2[T;.XSdN9Qe)rpt._K-#5wF)sP'##p#C0c%-Gb%"
    "hd+<-j'Ai*x&&HMkT]C'OSl##5RG[JXaHN;d'uA#x._U;.`PU@(Z3dt4r152@:v,'R.Sj'w#0<-;kPI)FfJ&#AYJ&#//)>-k=m=*XnK$>=)72L]0I%>.G690a:$##<,);?;72#?x9+d;"
    "^V'9;jY@;)br#q^YQpx:X#Te$Z^'=-=bGhLf:D6&bNwZ9-ZD#n^9HhLMr5G;']d&6'wYmTFmL<LD)F^%[tC'8;+9E#C$g%#5Y>q9wI>P(9mI[>kC-ekLC/R&CH+s'B;K-M6$EB%is00:"
    "+A4[7xks.LrNk0&E)wILYF@2L'0Nb$+pv<(2.768/FrY&h$^3i&@+G%JT'<-,v`3;_)I9M^AE]CN?Cl2AZg+%4iTpT3<n-&%H%b<FDj2M<hH=&Eh<2Len$b*aTX=-8QxN)k11IM1c^j%"
    "9s<L<NFSo)B?+<-(GxsF,^-Eh@$4dXhN$+#rxK8'je'D7k`e;)2pYwPA'_p9&@^18ml1^[@g4t*[JOa*[=Qp7(qJ_oOL^('7fB&Hq-:sf,sNj8xq^>$U4O]GKx'm9)b@p7YsvK3w^YR-"
    "CdQ*:Ir<($u&)#(&?L9Rg3H)4fiEp^iI9O8KnTj,]H?D*r7'M;PwZ9K0E^k&-cpI;.p/6_vwoFMV<->#%Xi.LxVnrU(4&8/P+:hLSKj$#U%]49t'I:rgMi'FL@a:0Y-uA[39',(vbma*"
    "hU%<-SRF`Tt:542R_VV$p@[p8DV[A,?1839FWdF<TddF<9Ah-6&9tWoDlh]&1SpGMq>Ti1O*H&#(AL8[_P%.M>v^-))qOT*F5Cq0`Ye%+$B6i:7@0IX<N+T+0MlMBPQ*Vj>SsD<U4JHY"
    "8kD2)2fU/M#$e.)T4,_=8hLim[&);?UkK'-x?'(:siIfL<$pFM`i<?%W(mGDHM%>iWP,##P`%/L<eXi:@Z9C.7o=@(pXdAO/NLQ8lPl+HPOQa8wD8=^GlPa8TKI1CjhsCTSLJM'/Wl>-"
    "S(qw%sf/@%#B6;/U7K]uZbi^Oc^2n<bhPmUkMw>%t<)'mEVE''n`WnJra$^TKvX5B>;_aSEK',(hwa0:i4G?.Bci.(X[?b*($,=-n<.Q%`(X=?+@Am*Js0&=3bh8K]mL<LoNs'6,'85`"
    "0?t/'_U59@]ddF<#LdF<eWdF<OuN/45rY<-L@&#+fm>69=Lb,OcZV/);TTm8VI;?%OtJ<(b4mq7M6:u?KRdF<gR@2L=FNU-<b[(9c/ML3m;Z[$oF3g)GAWqpARc=<ROu7cL5l;-[A]%/"
    "+fsd;l#SafT/f*W]0=O'$(Tb<[)*@e775R-:Yob%g*>l*:xP?Yb.5)%w_I?7uk5JC+FS(m#i'k.'a0i)9<7b'fs'59hq$*5Uhv##pi^8+hIEBF`nvo`;'l0.^S1<-wUK2/Coh58KKhLj"
    "M=SO*rfO`+qC`W-On.=AJ56>>i2@2LH6A:&5q`?9I3@@'04&p2/LVa*T-4<-i3;M9UvZd+N7>b*eIwg:CC)c<>nO&#<IGe;__.thjZl<%w(Wk2xmp4Q@I#I9,DF]u7-P=.-_:YJ]aS@V"
    "?6*C()dOp7:WL,b&3Rg/.cmM9&r^>$(>.Z-I&J(Q0Hd5Q%7Co-b`-c<N(6r@ip+AurK<m86QIth*#v;-OBqi+L7wDE-Ir8K['m+DDSLwK&/.?-V%U_%3:qKNu$_b*B-kp7NaD'QdWQPK"
    "Yq[@>P)hI;*_F]u`Rb[.j8_Q/<&>uu+VsH$sM9TA%?)(vmJ80),P7E>)tjD%2L=-t#fK[%`v=Q8<FfNkgg^oIbah*#8/Qt$F&:K*-(N/'+1vMB,u()-a.VUU*#[e%gAAO(S>WlA2);Sa"
    ">gXm8YB`1d@K#n]76-a$U,mF<fX]idqd)<3,]J7JmW4`6]uks=4-72L(jEk+:bJ0M^q-8Dm_Z?0olP1C9Sa&H[d&c$ooQUj]Exd*3ZM@-WGW2%s',B-_M%>%Ul:#/'xoFM9QX-$.QN'>"
    "[%$Z$uF6pA6Ki2O5:8w*vP1<-1`[G,)-m#>0`P&#eb#.3i)rtB61(o'$?X3B</R90;eZ]%Ncq;-Tl]#F>2Qft^ae_5tKL9MUe9b*sLEQ95C&`=G?@Mj=wh*'3E>=-<)Gt*Iw)'QG:`@I"
    "wOf7&]1i'S01B+Ev/Nac#9S;=;YQpg_6U`*kVY39xK,[/6Aj7:'1Bm-_1EYfa1+o&o4hp7KN_Q(OlIo@S%;jVdn0'1<Vc52=u`3^o-n1'g4v58Hj&6_t7$##?M)c<$bgQ_'SY((-xkA#"
    "Y(,p'H9rIVY-b,'%bCPF7.J<Up^,(dU1VY*5#WkTU>h19w,WQhLI)3S#f$2(eb,jr*b;3Vw]*7NH%$c4Vs,eD9>XW8?N]o+(*pgC%/72LV-u<Hp,3@e^9UB1J+ak9-TN/mhKPg+AJYd$"
    "MlvAF_jCK*.O-^(63adMT->W%iewS8W6m2rtCpo'RS1R84=@paTKt)>=%&1[)*vp'u+x,VrwN;&]kuO9JDbg=pO$J*.jVe;u'm0dr9l,<*wMK*Oe=g8lV_KEBFkO'oU]^=[-792#ok,)"
    "i]lR8qQ2oA8wcRCZ^7w/Njh;?.stX?Q1>S1q4Bn$)K1<-rGdO'$Wr.Lc.CG)$/*JL4tNR/,SVO3,aUw'DJN:)Ss;wGn9A32ijw%FL+Z0Fn.U9;reSq)bmI32U==5ALuG&#Vf1398/pVo"
    "1*c-(aY168o<`JsSbk-,1N;$>0:OUas(3:8Z972LSfF8eb=c-;>SPw7.6hn3m`9^Xkn(r.qS[0;T%&Qc=+STRxX'q1BNk3&*eu2;&8q$&x>Q#Q7^Tf+6<(d%ZVmj2bDi%.3L2n+4W'$P"
    "iDDG)g,r%+?,$@?uou5tSe2aN_AQU*<h`e-GI7)?OK2A.d7_c)?wQ5AS@DL3r#7fSkgl6-++D:'A,uq7SvlB$pcpH'q3n0#_%dY#xCpr-l<F0NR@-##FEV6NTF6##$l84N1w?AO>'IAO"
    "URQ##V^Fv-XFbGM7Fl(N<3DhLGF%q.1rC$#:T__&Pi68%0xi_&[qFJ(77j_&JWoF.V735&T,[R*:xFR*K5>>#`bW-?4Ne_&6Ne_&6Ne_&n`kr-#GJcM6X;uM6X;uM(.a..^2TkL%oR(#"
    ";u.T%fAr%4tJ8&><1=GHZ_+m9/#H1F^R#SC#*N=BA9(D?v[UiFY>>^8p,KKF.W]L29uLkLlu/+4T<XoIB&hx=T1PcDaB&;HH+-AFr?(m9HZV)FKS8JCw;SD=6[^/DZUL`EUDf]GGlG&>"
    "w$)F./^n3+rlo+DB;5sIYGNk+i1t-69Jg--0pao7Sm#K)pdHW&;LuDNH@H>#/X-TI(;P>#,Gc>#0Su>#4`1?#8lC?#<xU?#@.i?#D:%@#HF7@#LRI@#P_[@#Tkn@#Xw*A#]-=A#a9OA#"
    "d<F&#*;G##.GY##2Sl##6`($#:l:$#>xL$#B.`$#F:r$#JF.%#NR@%#R_R%#Vke%#Zww%#_-4&#3^Rh%Sflr-k'MS.o?.5/sWel/wpEM0%3'/1)K^f1-d>G21&v(35>V`39V7A4=onx4"
    "A1OY5EI0;6Ibgr6M$HS7Q<)58C5w,;WoA*#[%T*#`1g*#d=#+#hI5+#lUG+#pbY+#tnl+#x$),#&1;,#*=M,#.I`,#2Ur,#6b.-#;w[H#iQtA#m^0B#qjBB#uvTB##-hB#'9$C#+E6C#"
    "/QHC#3^ZC#7jmC#;v)D#?,<D#C8ND#GDaD#KPsD#O]/E#g1A5#KA*1#gC17#MGd;#8(02#L-d3#rWM4#Hga1#,<w0#T.j<#O#'2#CYN1#qa^:#_4m3#o@/=#eG8=#t8J5#`+78#4uI-#"
    "m3B2#SB[8#Q0@8#i[*9#iOn8#1Nm;#^sN9#qh<9#:=x-#P;K2#$%X9#bC+.#Rg;<#mN=.#MTF.#RZO.#2?)4#Y#(/#[)1/#b;L/#dAU/#0Sv;#lY$0#n`-0#sf60#(F24#wrH0#%/e0#"
    "TmD<#%JSMFove:CTBEXI:<eh2g)B,3h2^G3i;#d3jD>)4kMYD4lVu`4m`:&5niUA5@(A5BA1]PBB:xlBCC=2CDLXMCEUtiCf&0g2'tN?PGT4CPGT4CPGT4CPGT4CPGT4CPGT4CPGT4CP"
    "GT4CPGT4CPGT4CPGT4CPGT4CPGT4CP-qekC`.9kEg^+F$kwViFJTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5KTB&5o,^<-28ZI'O?;xp"
    "O?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xpO?;xp;7q-#lLYI:xvD=#";

#endif /* NK_INCLUDE_DEFAULT_FONT */

#define NK_CURSOR_DATA_W 90
#define NK_CURSOR_DATA_H 27
NK_GLOBAL const char nk_custom_cursor_data[NK_CURSOR_DATA_W * NK_CURSOR_DATA_H + 1] =
{
    "..-         -XXXXXXX-    X    -           X           -XXXXXXX          -          XXXXXXX"
    "..-         -X.....X-   X.X   -          X.X          -X.....X          -          X.....X"
    "---         -XXX.XXX-  X...X  -         X...X         -X....X           -           X....X"
    "X           -  X.X  - X.....X -        X.....X        -X...X            -            X...X"
    "XX          -  X.X  -X.......X-       X.......X       -X..X.X           -           X.X..X"
    "X.X         -  X.X  -XXXX.XXXX-       XXXX.XXXX       -X.X X.X          -          X.X X.X"
    "X..X        -  X.X  -   X.X   -          X.X          -XX   X.X         -         X.X   XX"
    "X...X       -  X.X  -   X.X   -    XX    X.X    XX    -      X.X        -        X.X      "
    "X....X      -  X.X  -   X.X   -   X.X    X.X    X.X   -       X.X       -       X.X       "
    "X.....X     -  X.X  -   X.X   -  X..X    X.X    X..X  -        X.X      -      X.X        "
    "X......X    -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -         X.X   XX-XX   X.X         "
    "X.......X   -  X.X  -   X.X   -X.....................X-          X.X X.X-X.X X.X          "
    "X........X  -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -           X.X..X-X..X.X           "
    "X.........X -XXX.XXX-   X.X   -  X..X    X.X    X..X  -            X...X-X...X            "
    "X..........X-X.....X-   X.X   -   X.X    X.X    X.X   -           X....X-X....X           "
    "X......XXXXX-XXXXXXX-   X.X   -    XX    X.X    XX    -          X.....X-X.....X          "
    "X...X..X    ---------   X.X   -          X.X          -          XXXXXXX-XXXXXXX          "
    "X..X X..X   -       -XXXX.XXXX-       XXXX.XXXX       ------------------------------------"
    "X.X  X..X   -       -X.......X-       X.......X       -    XX           XX    -           "
    "XX    X..X  -       - X.....X -        X.....X        -   X.X           X.X   -           "
    "      X..X          -  X...X  -         X...X         -  X..X           X..X  -           "
    "       XX           -   X.X   -          X.X          - X...XXXXXXXXXXXXX...X -           "
    "------------        -    X    -           X           -X.....................X-           "
    "                    ----------------------------------- X...XXXXXXXXXXXXX...X -           "
    "                                                      -  X..X           X..X  -           "
    "                                                      -   X.X           X.X   -           "
    "                                                      -    XX           XX    -           "
};

#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop
#endif

NK_GLOBAL unsigned char *nk__barrier;
NK_GLOBAL unsigned char *nk__barrier2;
NK_GLOBAL unsigned char *nk__barrier3;
NK_GLOBAL unsigned char *nk__barrier4;
NK_GLOBAL unsigned char *nk__dout;

NK_INTERN unsigned int
nk_decompress_length(unsigned char *input)
{
    return (unsigned int)((input[8] << 24) + (input[9] << 16) + (input[10] << 8) + input[11]);
}
NK_INTERN void
nk__match(unsigned char *data, unsigned int length)
{
    /* INVERSE of memmove... write each byte before copying the next...*/
    NK_ASSERT (nk__dout + length <= nk__barrier);
    if (nk__dout + length > nk__barrier) { nk__dout += length; return; }
    if (data < nk__barrier4) { nk__dout = nk__barrier+1; return; }
    while (length--) *nk__dout++ = *data++;
}
NK_INTERN void
nk__lit(unsigned char *data, unsigned int length)
{
    NK_ASSERT (nk__dout + length <= nk__barrier);
    if (nk__dout + length > nk__barrier) { nk__dout += length; return; }
    if (data < nk__barrier2) { nk__dout = nk__barrier+1; return; }
    NK_MEMCPY(nk__dout, data, length);
    nk__dout += length;
}
NK_INTERN unsigned char*
nk_decompress_token(unsigned char *i)
{
    #define nk__in2(x)   ((i[x] << 8) + i[(x)+1])
    #define nk__in3(x)   ((i[x] << 16) + nk__in2((x)+1))
    #define nk__in4(x)   ((i[x] << 24) + nk__in3((x)+1))

    if (*i >= 0x20) { /* use fewer if's for cases that expand small */
        if (*i >= 0x80)       nk__match(nk__dout-i[1]-1, (unsigned int)i[0] - 0x80 + 1), i += 2;
        else if (*i >= 0x40)  nk__match(nk__dout-(nk__in2(0) - 0x4000 + 1), (unsigned int)i[2]+1), i += 3;
        else /* *i >= 0x20 */ nk__lit(i+1, (unsigned int)i[0] - 0x20 + 1), i += 1 + (i[0] - 0x20 + 1);
    } else { /* more ifs for cases that expand large, since overhead is amortized */
        if (*i >= 0x18)       nk__match(nk__dout-(unsigned int)(nk__in3(0) - 0x180000 + 1), (unsigned int)i[3]+1), i += 4;
        else if (*i >= 0x10)  nk__match(nk__dout-(unsigned int)(nk__in3(0) - 0x100000 + 1), (unsigned int)nk__in2(3)+1), i += 5;
        else if (*i >= 0x08)  nk__lit(i+2, (unsigned int)nk__in2(0) - 0x0800 + 1), i += 2 + (nk__in2(0) - 0x0800 + 1);
        else if (*i == 0x07)  nk__lit(i+3, (unsigned int)nk__in2(1) + 1), i += 3 + (nk__in2(1) + 1);
        else if (*i == 0x06)  nk__match(nk__dout-(unsigned int)(nk__in3(1)+1), i[4]+1u), i += 5;
        else if (*i == 0x04)  nk__match(nk__dout-(unsigned int)(nk__in3(1)+1), (unsigned int)nk__in2(4)+1u), i += 6;
    }
    return i;
}
NK_INTERN unsigned int
nk_adler32(unsigned int adler32, unsigned char *buffer, unsigned int buflen)
{
    const unsigned long ADLER_MOD = 65521;
    unsigned long s1 = adler32 & 0xffff, s2 = adler32 >> 16;
    unsigned long blocklen, i;

    blocklen = buflen % 5552;
    while (buflen) {
        for (i=0; i + 7 < blocklen; i += 8) {
            s1 += buffer[0]; s2 += s1;
            s1 += buffer[1]; s2 += s1;
            s1 += buffer[2]; s2 += s1;
            s1 += buffer[3]; s2 += s1;
            s1 += buffer[4]; s2 += s1;
            s1 += buffer[5]; s2 += s1;
            s1 += buffer[6]; s2 += s1;
            s1 += buffer[7]; s2 += s1;
            buffer += 8;
        }
        for (; i < blocklen; ++i) {
            s1 += *buffer++; s2 += s1;
        }

        s1 %= ADLER_MOD; s2 %= ADLER_MOD;
        buflen -= (unsigned int)blocklen;
        blocklen = 5552;
    }
    return (unsigned int)(s2 << 16) + (unsigned int)s1;
}
NK_INTERN unsigned int
nk_decompress(unsigned char *output, unsigned char *i, unsigned int length)
{
    unsigned int olen;
    if (nk__in4(0) != 0x57bC0000) return 0;
    if (nk__in4(4) != 0)          return 0; /* error! stream is > 4GB */
    olen = nk_decompress_length(i);
    nk__barrier2 = i;
    nk__barrier3 = i+length;
    nk__barrier = output + olen;
    nk__barrier4 = output;
    i += 16;

    nk__dout = output;
    for (;;) {
        unsigned char *old_i = i;
        i = nk_decompress_token(i);
        if (i == old_i) {
            if (*i == 0x05 && i[1] == 0xfa) {
                NK_ASSERT(nk__dout == output + olen);
                if (nk__dout != output + olen) return 0;
                if (nk_adler32(1, output, olen) != (unsigned int) nk__in4(2))
                    return 0;
                return olen;
            } else {
                NK_ASSERT(0); /* NOTREACHED */
                return 0;
            }
        }
        NK_ASSERT(nk__dout <= output + olen);
        if (nk__dout > output + olen)
            return 0;
    }
}
NK_INTERN unsigned int
nk_decode_85_byte(char c)
{
    return (unsigned int)((c >= '\\') ? c-36 : c-35);
}
NK_INTERN void
nk_decode_85(unsigned char* dst, const unsigned char* src)
{
    while (*src)
    {
        unsigned int tmp =
            nk_decode_85_byte((char)src[0]) +
            85 * (nk_decode_85_byte((char)src[1]) +
            85 * (nk_decode_85_byte((char)src[2]) +
            85 * (nk_decode_85_byte((char)src[3]) +
            85 * nk_decode_85_byte((char)src[4]))));

        /* we can't assume little-endianess. */
        dst[0] = (unsigned char)((tmp >> 0) & 0xFF);
        dst[1] = (unsigned char)((tmp >> 8) & 0xFF);
        dst[2] = (unsigned char)((tmp >> 16) & 0xFF);
        dst[3] = (unsigned char)((tmp >> 24) & 0xFF);

        src += 5;
        dst += 4;
    }
}

/* -------------------------------------------------------------
 *
 *                          FONT ATLAS
 *
 * --------------------------------------------------------------*/
NK_API struct nk_font_config
nk_font_config(float pixel_height)
{
    struct nk_font_config cfg;
    nk_zero_struct(cfg);
    cfg.ttf_blob = 0;
    cfg.ttf_size = 0;
    cfg.ttf_data_owned_by_atlas = 0;
    cfg.size = pixel_height;
    cfg.oversample_h = 3;
    cfg.oversample_v = 1;
    cfg.pixel_snap = 0;
    cfg.coord_type = NK_COORD_UV;
    cfg.spacing = nk_vec2(0,0);
    cfg.range = nk_font_default_glyph_ranges();
    cfg.merge_mode = 0;
    cfg.fallback_glyph = '?';
    cfg.font = 0;
    cfg.n = 0;
    return cfg;
}
#ifdef NK_INCLUDE_DEFAULT_ALLOCATOR
NK_API void
nk_font_atlas_init_default(struct nk_font_atlas *atlas)
{
    NK_ASSERT(atlas);
    if (!atlas) return;
    nk_zero_struct(*atlas);
    atlas->temporary.userdata.ptr = 0;
    atlas->temporary.alloc = nk_malloc;
    atlas->temporary.free = nk_mfree;
    atlas->permanent.userdata.ptr = 0;
    atlas->permanent.alloc = nk_malloc;
    atlas->permanent.free = nk_mfree;
}
#endif
NK_API void
nk_font_atlas_init(struct nk_font_atlas *atlas, struct nk_allocator *alloc)
{
    NK_ASSERT(atlas);
    NK_ASSERT(alloc);
    if (!atlas || !alloc) return;
    nk_zero_struct(*atlas);
    atlas->permanent = *alloc;
    atlas->temporary = *alloc;
}
NK_API void
nk_font_atlas_init_custom(struct nk_font_atlas *atlas,
    struct nk_allocator *permanent, struct nk_allocator *temporary)
{
    NK_ASSERT(atlas);
    NK_ASSERT(permanent);
    NK_ASSERT(temporary);
    if (!atlas || !permanent || !temporary) return;
    nk_zero_struct(*atlas);
    atlas->permanent = *permanent;
    atlas->temporary = *temporary;
}
NK_API void
nk_font_atlas_begin(struct nk_font_atlas *atlas)
{
    NK_ASSERT(atlas);
    NK_ASSERT(atlas->temporary.alloc && atlas->temporary.free);
    NK_ASSERT(atlas->permanent.alloc && atlas->permanent.free);
    if (!atlas || !atlas->permanent.alloc || !atlas->permanent.free ||
        !atlas->temporary.alloc || !atlas->temporary.free) return;
    if (atlas->glyphs) {
        atlas->permanent.free(atlas->permanent.userdata, atlas->glyphs);
        atlas->glyphs = 0;
    }
    if (atlas->pixel) {
        atlas->permanent.free(atlas->permanent.userdata, atlas->pixel);
        atlas->pixel = 0;
    }
}
NK_API struct nk_font*
nk_font_atlas_add(struct nk_font_atlas *atlas, const struct nk_font_config *config)
{
    struct nk_font *font = 0;
    struct nk_font_config *cfg;

    NK_ASSERT(atlas);
    NK_ASSERT(atlas->permanent.alloc);
    NK_ASSERT(atlas->permanent.free);
    NK_ASSERT(atlas->temporary.alloc);
    NK_ASSERT(atlas->temporary.free);

    NK_ASSERT(config);
    NK_ASSERT(config->ttf_blob);
    NK_ASSERT(config->ttf_size);
    NK_ASSERT(config->size > 0.0f);

    if (!atlas || !config || !config->ttf_blob || !config->ttf_size || config->size <= 0.0f||
        !atlas->permanent.alloc || !atlas->permanent.free ||
        !atlas->temporary.alloc || !atlas->temporary.free)
        return 0;

    /* allocate font config  */
    cfg = (struct nk_font_config*)
        atlas->permanent.alloc(atlas->permanent.userdata,0, sizeof(struct nk_font_config));
    NK_MEMCPY(cfg, config, sizeof(*config));
    cfg->n = cfg;
    cfg->p = cfg;

    if (!config->merge_mode) {
        /* insert font config into list */
        if (!atlas->config) {
            atlas->config = cfg;
            cfg->next = 0;
        } else {
            struct nk_font_config *i = atlas->config;
            while (i->next) i = i->next;
            i->next = cfg;
            cfg->next = 0;
        }
        /* allocate new font */
        font = (struct nk_font*)
            atlas->permanent.alloc(atlas->permanent.userdata,0, sizeof(struct nk_font));
        NK_ASSERT(font);
        nk_zero(font, sizeof(*font));
        if (!font) return 0;
        font->config = cfg;

        /* insert font into list */
        if (!atlas->fonts) {
            atlas->fonts = font;
            font->next = 0;
        } else {
            struct nk_font *i = atlas->fonts;
            while (i->next) i = i->next;
            i->next = font;
            font->next = 0;
        }
        cfg->font = &font->info;
    } else {
        /* extend previously added font */
        struct nk_font *f = 0;
        struct nk_font_config *c = 0;
        NK_ASSERT(atlas->font_num);
        f = atlas->fonts;
        c = f->config;
        cfg->font = &f->info;

        cfg->n = c;
        cfg->p = c->p;
        c->p->n = cfg;
        c->p = cfg;
    }
    /* create own copy of .TTF font blob */
    if (!config->ttf_data_owned_by_atlas) {
        cfg->ttf_blob = atlas->permanent.alloc(atlas->permanent.userdata,0, cfg->ttf_size);
        NK_ASSERT(cfg->ttf_blob);
        if (!cfg->ttf_blob) {
            atlas->font_num++;
            return 0;
        }
        NK_MEMCPY(cfg->ttf_blob, config->ttf_blob, cfg->ttf_size);
        cfg->ttf_data_owned_by_atlas = 1;
    }
    atlas->font_num++;
    return font;
}
NK_API struct nk_font*
nk_font_atlas_add_from_memory(struct nk_font_atlas *atlas, void *memory,
    nk_size size, float height, const struct nk_font_config *config)
{
    struct nk_font_config cfg;
    NK_ASSERT(memory);
    NK_ASSERT(size);

    NK_ASSERT(atlas);
    NK_ASSERT(atlas->temporary.alloc);
    NK_ASSERT(atlas->temporary.free);
    NK_ASSERT(atlas->permanent.alloc);
    NK_ASSERT(atlas->permanent.free);
    if (!atlas || !atlas->temporary.alloc || !atlas->temporary.free || !memory || !size ||
        !atlas->permanent.alloc || !atlas->permanent.free)
        return 0;

    cfg = (config) ? *config: nk_font_config(height);
    cfg.ttf_blob = memory;
    cfg.ttf_size = size;
    cfg.size = height;
    cfg.ttf_data_owned_by_atlas = 0;
    return nk_font_atlas_add(atlas, &cfg);
}
#ifdef NK_INCLUDE_STANDARD_IO
NK_API struct nk_font*
nk_font_atlas_add_from_file(struct nk_font_atlas *atlas, const char *file_path,
    float height, const struct nk_font_config *config)
{
    nk_size size;
    char *memory;
    struct nk_font_config cfg;

    NK_ASSERT(atlas);
    NK_ASSERT(atlas->temporary.alloc);
    NK_ASSERT(atlas->temporary.free);
    NK_ASSERT(atlas->permanent.alloc);
    NK_ASSERT(atlas->permanent.free);

    if (!atlas || !file_path) return 0;
    memory = nk_file_load(file_path, &size, &atlas->permanent);
    if (!memory) return 0;

    cfg = (config) ? *config: nk_font_config(height);
    cfg.ttf_blob = memory;
    cfg.ttf_size = size;
    cfg.size = height;
    cfg.ttf_data_owned_by_atlas = 1;
    return nk_font_atlas_add(atlas, &cfg);
}
#endif
NK_API struct nk_font*
nk_font_atlas_add_compressed(struct nk_font_atlas *atlas,
    void *compressed_data, nk_size compressed_size, float height,
    const struct nk_font_config *config)
{
    unsigned int decompressed_size;
    void *decompressed_data;
    struct nk_font_config cfg;

    NK_ASSERT(atlas);
    NK_ASSERT(atlas->temporary.alloc);
    NK_ASSERT(atlas->temporary.free);
    NK_ASSERT(atlas->permanent.alloc);
    NK_ASSERT(atlas->permanent.free);

    NK_ASSERT(compressed_data);
    NK_ASSERT(compressed_size);
    if (!atlas || !compressed_data || !atlas->temporary.alloc || !atlas->temporary.free ||
        !atlas->permanent.alloc || !atlas->permanent.free)
        return 0;

    decompressed_size = nk_decompress_length((unsigned char*)compressed_data);
    decompressed_data = atlas->permanent.alloc(atlas->permanent.userdata,0,decompressed_size);
    NK_ASSERT(decompressed_data);
    if (!decompressed_data) return 0;
    nk_decompress((unsigned char*)decompressed_data, (unsigned char*)compressed_data,
        (unsigned int)compressed_size);

    cfg = (config) ? *config: nk_font_config(height);
    cfg.ttf_blob = decompressed_data;
    cfg.ttf_size = decompressed_size;
    cfg.size = height;
    cfg.ttf_data_owned_by_atlas = 1;
    return nk_font_atlas_add(atlas, &cfg);
}
NK_API struct nk_font*
nk_font_atlas_add_compressed_base85(struct nk_font_atlas *atlas,
    const char *data_base85, float height, const struct nk_font_config *config)
{
    int compressed_size;
    void *compressed_data;
    struct nk_font *font;

    NK_ASSERT(atlas);
    NK_ASSERT(atlas->temporary.alloc);
    NK_ASSERT(atlas->temporary.free);
    NK_ASSERT(atlas->permanent.alloc);
    NK_ASSERT(atlas->permanent.free);

    NK_ASSERT(data_base85);
    if (!atlas || !data_base85 || !atlas->temporary.alloc || !atlas->temporary.free ||
        !atlas->permanent.alloc || !atlas->permanent.free)
        return 0;

    compressed_size = (((int)nk_strlen(data_base85) + 4) / 5) * 4;
    compressed_data = atlas->temporary.alloc(atlas->temporary.userdata,0, (nk_size)compressed_size);
    NK_ASSERT(compressed_data);
    if (!compressed_data) return 0;
    nk_decode_85((unsigned char*)compressed_data, (const unsigned char*)data_base85);
    font = nk_font_atlas_add_compressed(atlas, compressed_data,
                    (nk_size)compressed_size, height, config);
    atlas->temporary.free(atlas->temporary.userdata, compressed_data);
    return font;
}

#ifdef NK_INCLUDE_DEFAULT_FONT
NK_API struct nk_font*
nk_font_atlas_add_default(struct nk_font_atlas *atlas,
    float pixel_height, const struct nk_font_config *config)
{
    NK_ASSERT(atlas);
    NK_ASSERT(atlas->temporary.alloc);
    NK_ASSERT(atlas->temporary.free);
    NK_ASSERT(atlas->permanent.alloc);
    NK_ASSERT(atlas->permanent.free);
    return nk_font_atlas_add_compressed_base85(atlas,
        nk_proggy_clean_ttf_compressed_data_base85, pixel_height, config);
}
#endif
NK_API const void*
nk_font_atlas_bake(struct nk_font_atlas *atlas, int *width, int *height,
    enum nk_font_atlas_format fmt)
{
    int i = 0;
    void *tmp = 0;
    nk_size tmp_size, img_size;
    struct nk_font *font_iter;
    struct nk_font_baker *baker;

    NK_ASSERT(atlas);
    NK_ASSERT(atlas->temporary.alloc);
    NK_ASSERT(atlas->temporary.free);
    NK_ASSERT(atlas->permanent.alloc);
    NK_ASSERT(atlas->permanent.free);

    NK_ASSERT(width);
    NK_ASSERT(height);
    if (!atlas || !width || !height ||
        !atlas->temporary.alloc || !atlas->temporary.free ||
        !atlas->permanent.alloc || !atlas->permanent.free)
        return 0;

#ifdef NK_INCLUDE_DEFAULT_FONT
    /* no font added so just use default font */
    if (!atlas->font_num)
        atlas->default_font = nk_font_atlas_add_default(atlas, 13.0f, 0);
#endif
    NK_ASSERT(atlas->font_num);
    if (!atlas->font_num) return 0;

    /* allocate temporary baker memory required for the baking process */
    nk_font_baker_memory(&tmp_size, &atlas->glyph_count, atlas->config, atlas->font_num);
    tmp = atlas->temporary.alloc(atlas->temporary.userdata,0, tmp_size);
    NK_ASSERT(tmp);
    if (!tmp) goto failed;

    /* allocate glyph memory for all fonts */
    baker = nk_font_baker(tmp, atlas->glyph_count, atlas->font_num, &atlas->temporary);
    atlas->glyphs = (struct nk_font_glyph*)atlas->permanent.alloc(
        atlas->permanent.userdata,0, sizeof(struct nk_font_glyph)*(nk_size)atlas->glyph_count);
    NK_ASSERT(atlas->glyphs);
    if (!atlas->glyphs)
        goto failed;

    /* pack all glyphs into a tight fit space */
    atlas->custom.w = (NK_CURSOR_DATA_W*2)+1;
    atlas->custom.h = NK_CURSOR_DATA_H + 1;
    if (!nk_font_bake_pack(baker, &img_size, width, height, &atlas->custom,
        atlas->config, atlas->font_num, &atlas->temporary))
        goto failed;

    /* allocate memory for the baked image font atlas */
    atlas->pixel = atlas->temporary.alloc(atlas->temporary.userdata,0, img_size);
    NK_ASSERT(atlas->pixel);
    if (!atlas->pixel)
        goto failed;

    /* bake glyphs and custom white pixel into image */
    nk_font_bake(baker, atlas->pixel, *width, *height,
        atlas->glyphs, atlas->glyph_count, atlas->config, atlas->font_num);
    nk_font_bake_custom_data(atlas->pixel, *width, *height, atlas->custom,
            nk_custom_cursor_data, NK_CURSOR_DATA_W, NK_CURSOR_DATA_H, '.', 'X');

    if (fmt == NK_FONT_ATLAS_RGBA32) {
        /* convert alpha8 image into rgba32 image */
        void *img_rgba = atlas->temporary.alloc(atlas->temporary.userdata,0,
                            (nk_size)(*width * *height * 4));
        NK_ASSERT(img_rgba);
        if (!img_rgba) goto failed;
        nk_font_bake_convert(img_rgba, *width, *height, atlas->pixel);
        atlas->temporary.free(atlas->temporary.userdata, atlas->pixel);
        atlas->pixel = img_rgba;
    }
    atlas->tex_width = *width;
    atlas->tex_height = *height;

    /* initialize each font */
    for (font_iter = atlas->fonts; font_iter; font_iter = font_iter->next) {
        struct nk_font *font = font_iter;
        struct nk_font_config *config = font->config;
        nk_font_init(font, config->size, config->fallback_glyph, atlas->glyphs,
            config->font, nk_handle_ptr(0));
    }

    /* initialize each cursor */
    {NK_STORAGE const struct nk_vec2 nk_cursor_data[NK_CURSOR_COUNT][3] = {
        /* Pos      Size        Offset */
        {{ 0, 3},   {12,19},    { 0, 0}},
        {{13, 0},   { 7,16},    { 4, 8}},
        {{31, 0},   {23,23},    {11,11}},
        {{21, 0},   { 9, 23},   { 5,11}},
        {{55,18},   {23, 9},    {11, 5}},
        {{73, 0},   {17,17},    { 9, 9}},
        {{55, 0},   {17,17},    { 9, 9}}
    };
    for (i = 0; i < NK_CURSOR_COUNT; ++i) {
        struct nk_cursor *cursor = &atlas->cursors[i];
        cursor->img.w = (unsigned short)*width;
        cursor->img.h = (unsigned short)*height;
        cursor->img.region[0] = (unsigned short)(atlas->custom.x + nk_cursor_data[i][0].x);
        cursor->img.region[1] = (unsigned short)(atlas->custom.y + nk_cursor_data[i][0].y);
        cursor->img.region[2] = (unsigned short)nk_cursor_data[i][1].x;
        cursor->img.region[3] = (unsigned short)nk_cursor_data[i][1].y;
        cursor->size = nk_cursor_data[i][1];
        cursor->offset = nk_cursor_data[i][2];
    }}
    /* free temporary memory */
    atlas->temporary.free(atlas->temporary.userdata, tmp);
    return atlas->pixel;

failed:
    /* error so cleanup all memory */
    if (tmp) atlas->temporary.free(atlas->temporary.userdata, tmp);
    if (atlas->glyphs) {
        atlas->permanent.free(atlas->permanent.userdata, atlas->glyphs);
        atlas->glyphs = 0;
    }
    if (atlas->pixel) {
        atlas->temporary.free(atlas->temporary.userdata, atlas->pixel);
        atlas->pixel = 0;
    }
    return 0;
}
NK_API void
nk_font_atlas_end(struct nk_font_atlas *atlas, nk_handle texture,
    struct nk_draw_null_texture *null)
{
    int i = 0;
    struct nk_font *font_iter;
    NK_ASSERT(atlas);
    if (!atlas) {
        if (!null) return;
        null->texture = texture;
        null->uv = nk_vec2(0.5f,0.5f);
    }
    if (null) {
        null->texture = texture;
        null->uv.x = (atlas->custom.x + 0.5f)/(float)atlas->tex_width;
        null->uv.y = (atlas->custom.y + 0.5f)/(float)atlas->tex_height;
    }
    for (font_iter = atlas->fonts; font_iter; font_iter = font_iter->next) {
        font_iter->texture = texture;
#ifdef NK_INCLUDE_VERTEX_BUFFER_OUTPUT
        font_iter->handle.texture = texture;
#endif
    }
    for (i = 0; i < NK_CURSOR_COUNT; ++i)
        atlas->cursors[i].img.handle = texture;

    atlas->temporary.free(atlas->temporary.userdata, atlas->pixel);
    atlas->pixel = 0;
    atlas->tex_width = 0;
    atlas->tex_height = 0;
    atlas->custom.x = 0;
    atlas->custom.y = 0;
    atlas->custom.w = 0;
    atlas->custom.h = 0;
}
NK_API void
nk_font_atlas_cleanup(struct nk_font_atlas *atlas)
{
    NK_ASSERT(atlas);
    NK_ASSERT(atlas->temporary.alloc);
    NK_ASSERT(atlas->temporary.free);
    NK_ASSERT(atlas->permanent.alloc);
    NK_ASSERT(atlas->permanent.free);
    if (!atlas || !atlas->permanent.alloc || !atlas->permanent.free) return;
    if (atlas->config) {
        struct nk_font_config *iter;
        for (iter = atlas->config; iter; iter = iter->next) {
            struct nk_font_config *i;
            for (i = iter->n; i != iter; i = i->n) {
                atlas->permanent.free(atlas->permanent.userdata, i->ttf_blob);
                i->ttf_blob = 0;
            }
            atlas->permanent.free(atlas->permanent.userdata, iter->ttf_blob);
            iter->ttf_blob = 0;
        }
    }
}
NK_API void
nk_font_atlas_clear(struct nk_font_atlas *atlas)
{
    NK_ASSERT(atlas);
    NK_ASSERT(atlas->temporary.alloc);
    NK_ASSERT(atlas->temporary.free);
    NK_ASSERT(atlas->permanent.alloc);
    NK_ASSERT(atlas->permanent.free);
    if (!atlas || !atlas->permanent.alloc || !atlas->permanent.free) return;

    if (atlas->config) {
        struct nk_font_config *iter, *next;
        for (iter = atlas->config; iter; iter = next) {
            struct nk_font_config *i, *n;
            for (i = iter->n; i != iter; i = n) {
                n = i->n;
                if (i->ttf_blob)
                    atlas->permanent.free(atlas->permanent.userdata, i->ttf_blob);
                atlas->permanent.free(atlas->permanent.userdata, i);
            }
            next = iter->next;
            if (i->ttf_blob)
                atlas->permanent.free(atlas->permanent.userdata, iter->ttf_blob);
            atlas->permanent.free(atlas->permanent.userdata, iter);
        }
        atlas->config = 0;
    }
    if (atlas->fonts) {
        struct nk_font *iter, *next;
        for (iter = atlas->fonts; iter; iter = next) {
            next = iter->next;
            atlas->permanent.free(atlas->permanent.userdata, iter);
        }
        atlas->fonts = 0;
    }
    if (atlas->glyphs)
        atlas->permanent.free(atlas->permanent.userdata, atlas->glyphs);
    nk_zero_struct(*atlas);
}
#endif

