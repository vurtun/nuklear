#include "nuklear.h"
#include "nuklear_internal.h"

/* ===============================================================
 *
 *                              TABLE
 *
 * ===============================================================*/
NK_LIB struct nk_table*
nk_create_table(struct nk_context *ctx)
{
    struct nk_page_element *elem;
    elem = nk_create_page_element(ctx);
    if (!elem) return 0;
    nk_zero_struct(*elem);
    return &elem->data.tbl;
}
NK_LIB void
nk_free_table(struct nk_context *ctx, struct nk_table *tbl)
{
    union nk_page_data *pd = NK_CONTAINER_OF(tbl, union nk_page_data, tbl);
    struct nk_page_element *pe = NK_CONTAINER_OF(pd, struct nk_page_element, data);
    nk_free_page_element(ctx, pe);
}
NK_LIB void
nk_push_table(struct nk_window *win, struct nk_table *tbl)
{
    if (!win->tables) {
        win->tables = tbl;
        tbl->next = 0;
        tbl->prev = 0;
        tbl->size = 0;
        win->table_count = 1;
        return;
    }
    win->tables->prev = tbl;
    tbl->next = win->tables;
    tbl->prev = 0;
    tbl->size = 0;
    win->tables = tbl;
    win->table_count++;
}
NK_LIB void
nk_remove_table(struct nk_window *win, struct nk_table *tbl)
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
NK_LIB nk_uint*
nk_add_value(struct nk_context *ctx, struct nk_window *win,
            nk_hash name, nk_uint value)
{
    NK_ASSERT(ctx);
    NK_ASSERT(win);
    if (!win || !ctx) return 0;
    if (!win->tables || win->tables->size >= NK_VALUE_PAGE_CAPACITY) {
        struct nk_table *tbl = nk_create_table(ctx);
        NK_ASSERT(tbl);
        if (!tbl) return 0;
        nk_push_table(win, tbl);
    }
    win->tables->seq = win->seq;
    win->tables->keys[win->tables->size] = name;
    win->tables->values[win->tables->size] = value;
    return &win->tables->values[win->tables->size++];
}
NK_LIB nk_uint*
nk_find_value(struct nk_window *win, nk_hash name)
{
    struct nk_table *iter = win->tables;
    while (iter) {
        unsigned int i = 0;
        unsigned int size = iter->size;
        for (i = 0; i < size; ++i) {
            if (iter->keys[i] == name) {
                iter->seq = win->seq;
                return &iter->values[i];
            }
        } size = NK_VALUE_PAGE_CAPACITY;
        iter = iter->next;
    }
    return 0;
}

