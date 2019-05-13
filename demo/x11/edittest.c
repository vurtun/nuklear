static struct nk_text_edit gEditBuf1;
static struct nk_text_edit gEditBuf2;
static char gEditBufMem1[1024];
static char gEditBufMem2[1024];

static void
edittest(struct nk_context *ctx)
{
    static int initialized = 0;
    int width, height;
    if (!initialized) {
        initialized = 1;
        nk_textedit_init_fixed(&gEditBuf1, gEditBufMem1, 1000);
        nk_textedit_init_fixed(&gEditBuf2, gEditBufMem2, 1000);
    }

    width  = 200;
    height = 130;
    if (nk_begin(ctx, "EditTest", nk_rect(10, 10, width, height),NK_WINDOW_MOVABLE|NK_WINDOW_TITLE|NK_WINDOW_NO_SCROLLBAR)) {
        nk_layout_row_static(ctx, 30, width-40, 1);
        nk_edit_buffer(ctx, NK_EDIT_SIMPLE, &gEditBuf1, nk_filter_default);
        nk_layout_row_static(ctx, 30, width-40, 1);
        nk_edit_buffer(ctx, NK_EDIT_SIMPLE, &gEditBuf2, nk_filter_default);
    }
    nk_end(ctx);
}
