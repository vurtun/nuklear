#include "nuklear.h"
#include "nuklear_internal.h"

/* ===============================================================
 *
 *                              POOL
 *
 * ===============================================================*/
NK_LIB void
nk_pool_init(struct nk_pool *pool, struct nk_allocator *alloc,
    unsigned int capacity)
{
    nk_zero(pool, sizeof(*pool));
    pool->alloc = *alloc;
    pool->capacity = capacity;
    pool->type = NK_BUFFER_DYNAMIC;
    pool->pages = 0;
}
NK_LIB void
nk_pool_free(struct nk_pool *pool)
{
    struct nk_page *iter = pool->pages;
    if (!pool) return;
    if (pool->type == NK_BUFFER_FIXED) return;
    while (iter) {
        struct nk_page *next = iter->next;
        pool->alloc.free(pool->alloc.userdata, iter);
        iter = next;
    }
}
NK_LIB void
nk_pool_init_fixed(struct nk_pool *pool, void *memory, nk_size size)
{
    nk_zero(pool, sizeof(*pool));
    NK_ASSERT(size >= sizeof(struct nk_page));
    if (size < sizeof(struct nk_page)) return;
    pool->capacity = (unsigned)(size - sizeof(struct nk_page)) / sizeof(struct nk_page_element);
    pool->pages = (struct nk_page*)memory;
    pool->type = NK_BUFFER_FIXED;
    pool->size = size;
}
NK_LIB struct nk_page_element*
nk_pool_alloc(struct nk_pool *pool)
{
    if (!pool->pages || pool->pages->size >= pool->capacity) {
        /* allocate new page */
        struct nk_page *page;
        if (pool->type == NK_BUFFER_FIXED) {
            NK_ASSERT(pool->pages);
            if (!pool->pages) return 0;
            NK_ASSERT(pool->pages->size < pool->capacity);
            return 0;
        } else {
            nk_size size = sizeof(struct nk_page);
            size += NK_POOL_DEFAULT_CAPACITY * sizeof(union nk_page_data);
            page = (struct nk_page*)pool->alloc.alloc(pool->alloc.userdata,0, size);
            page->next = pool->pages;
            pool->pages = page;
            page->size = 0;
        }
    } return &pool->pages->win[pool->pages->size++];
}

