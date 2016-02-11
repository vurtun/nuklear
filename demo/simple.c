static void
simple_window(struct zr_context *ctx)
{
    /* simple demo window */
    struct zr_panel layout;
    if (zr_begin(ctx, &layout, "Show", zr_rect(420, 350, 200, 200),
        ZR_WINDOW_BORDER|ZR_WINDOW_MOVABLE|ZR_WINDOW_SCALABLE|
        ZR_WINDOW_CLOSABLE|ZR_WINDOW_MINIMIZABLE|ZR_WINDOW_TITLE))
    {
        enum {EASY, HARD};
        static int op = EASY;
        static int property = 20;

        zr_layout_row_static(ctx, 30, 80, 1);
        if (zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT)) {
            /* event handling */
        }
        zr_layout_row_dynamic(ctx, 30, 2);
        if (zr_option(ctx, "easy", op == EASY)) op = EASY;
        if (zr_option(ctx, "hard", op == HARD)) op = HARD;

        zr_layout_row_dynamic(ctx, 22, 1);
        zr_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);
    }
    zr_end(ctx);
}

