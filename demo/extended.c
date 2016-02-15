/* ===============================================================
 *
 *                          CUSTOM WIDGET
 *
 * ===============================================================*/
static int
ui_piemenu(struct zr_context *ctx, struct zr_vec2 pos, float radius,
            struct zr_image *icons, int item_count)
{
    int ret = -1;
    struct zr_rect total_space;
    struct zr_panel popup;
    struct zr_rect bounds;
    int active_item = 0;

    /* hide popup background */
    struct zr_color border;
    zr_push_color(ctx, ZR_COLOR_WINDOW, zr_rgba(0,0,0,0));
    border = ctx->style.colors[ZR_COLOR_BORDER];
    zr_push_color(ctx, ZR_COLOR_BORDER, zr_rgba(0,0,0,0));

    /* pie menu popup */
    total_space  = zr_window_get_content_region(ctx);
    zr_popup_begin(ctx, &popup,  ZR_POPUP_STATIC, "piemenu", ZR_WINDOW_NO_SCROLLBAR,
        zr_rect(pos.x - total_space.x - radius, pos.y - radius - total_space.y,
        2*radius,2*radius));

    total_space = zr_window_get_content_region(ctx);
    zr_layout_row_dynamic(ctx, total_space.h, 1);
    {
        int i = 0;
        struct zr_command_buffer* out = zr_window_get_canvas(ctx);
        const struct zr_input *in = &ctx->input;
        {
            /* allocate complete popup space for the menu */
            enum zr_widget_state state;
            total_space = zr_window_get_content_region(ctx);
            total_space.x = total_space.y = 0;
            state = zr_widget(&bounds, ctx);
        }

        /* outer circle */
        zr_draw_circle(out, bounds, zr_rgb(50,50,50));
        {
            /* circle buttons */
            float step = (2 * 3.141592654f) / (float)(MAX(1,item_count));
            float a_min = 0; float a_max = step;

            struct zr_vec2 center = zr_vec2(bounds.x + bounds.w / 2.0f, bounds.y + bounds.h / 2.0f);
            struct zr_vec2 drag = zr_vec2(in->mouse.pos.x - center.x, in->mouse.pos.y - center.y);
            float angle = (float)atan2(drag.y, drag.x);
            if (angle < -0.0f) angle += 2.0f * 3.141592654f;
            active_item = (int)(angle/step);

            for (i = 0; i < item_count; ++i) {
                struct zr_rect content;
                float rx, ry, dx, dy, a;
                zr_draw_arc(out, center.x, center.y, (bounds.w/2.0f),
                    a_min, a_max, (active_item == i) ? zr_rgb(45,100,255): zr_rgb(60,60,60));

                /* seperator line */
                rx = bounds.w/2.0f; ry = 0;
                dx = rx * (float)cos(a_min) - ry * (float)sin(a_min);
                dy = rx * (float)sin(a_min) + ry * (float)cos(a_min);
                zr_draw_line(out, center.x, center.y,
                    center.x + dx, center.y + dy, zr_rgb(50,50,50));

                /* button content */
                a = a_min + (a_max - a_min)/2.0f;
                rx = bounds.w/2.5f; ry = 0;
                content.w = 30; content.h = 30;
                content.x = center.x + ((rx * (float)cos(a) - ry * (float)sin(a)) - content.w/2.0f);
                content.y = center.y + (rx * (float)sin(a) + ry * (float)cos(a) - content.h/2.0f);
                zr_draw_image(out, content, &icons[i]);
                a_min = a_max; a_max += step;
            }
        }
        {
            /* inner circle */
            struct zr_rect inner;
            inner.x = bounds.x + bounds.w/2 - bounds.w/4;
            inner.y = bounds.y + bounds.h/2 - bounds.h/4;
            inner.w = bounds.w/2; inner.h = bounds.h/2;
            zr_draw_circle(out, inner, zr_rgb(45,45,45));

            /* active icon content */
            bounds.w = inner.w / 2.0f;
            bounds.h = inner.h / 2.0f;
            bounds.x = inner.x + inner.w/2 - bounds.w/2;
            bounds.y = inner.y + inner.h/2 - bounds.h/2;
            zr_draw_image(out, bounds, &icons[active_item]);
        }
    }
    zr_layout_space_end(ctx);
    zr_popup_end(ctx);

    zr_reset_colors(ctx);
    zr_reset_properties(ctx);

    if (!zr_input_is_mouse_down(&ctx->input, ZR_BUTTON_RIGHT))
        return active_item;
    else return ret;
}

/* ===============================================================
 *
 *                          GRID
 *
 * ===============================================================*/
static void
grid_demo(struct zr_context *ctx)
{
    static char text[3][64];
    static size_t text_len[3];
    static const char *items[] = {"Item 0","item 1","item 2"};
    static int selected_item = 0;
    static int check = 1;
    struct zr_panel layout;

    int i;
    struct zr_panel combo;
    ctx->style.font.height = 20;
    if (zr_begin(ctx, &layout, "Grid Demo", zr_rect(600, 350, 275, 250),
        ZR_WINDOW_TITLE|ZR_WINDOW_BORDER|ZR_WINDOW_MOVABLE|
        ZR_WINDOW_BORDER_HEADER|ZR_WINDOW_NO_SCROLLBAR))
    {
        ctx->style.font.height = 18;
        zr_layout_row_dynamic(ctx, 30, 2);
        zr_label(ctx, "Floating point:", ZR_TEXT_RIGHT);
        zr_edit_string(ctx, ZR_EDIT_FIELD, text[0], &text_len[0], 64, zr_filter_float);
        zr_label(ctx, "Hexadeximal:", ZR_TEXT_RIGHT);
        zr_edit_string(ctx, ZR_EDIT_FIELD, text[1], &text_len[1], 64, zr_filter_hex);
        zr_label(ctx, "Binary:", ZR_TEXT_RIGHT);
        zr_edit_string(ctx, ZR_EDIT_FIELD, text[2], &text_len[2], 64, zr_filter_binary);
        zr_label(ctx, "Checkbox:", ZR_TEXT_RIGHT);
        zr_checkbox(ctx, "Check me", &check);
        zr_label(ctx, "Combobox:", ZR_TEXT_RIGHT);

        if (zr_combo_begin_text(ctx, &combo, items[selected_item], 200)) {
            zr_layout_row_dynamic(ctx, 30, 1);
            for (i = 0; i < 3; ++i)
                if (zr_combo_item(ctx, items[i], ZR_TEXT_LEFT))
                    selected_item = i;
            zr_combo_end(ctx);
        }
    }
    zr_end(ctx);
    ctx->style.font.height = 14;
}

/* ===============================================================
 *
 *                          BUTTON DEMO
 *
 * ===============================================================*/
static void
ui_header(struct zr_context *ctx, const char *title)
{
    zr_reset_font_height(ctx);
    zr_push_font_height(ctx, 18);
    zr_layout_row_dynamic(ctx, 20, 1);
    zr_label(ctx, title, ZR_TEXT_LEFT);
}

static void
ui_widget(struct zr_context *ctx, float height, float font_height)
{
    static const float ratio[] = {0.15f, 0.85f};
    zr_reset_font_height(ctx);
    zr_push_font_height(ctx, font_height);
    zr_layout_row(ctx, ZR_DYNAMIC, height, 2, ratio);
    zr_spacing(ctx, 1);
}

static void
ui_widget_centered(struct zr_context *ctx, float height, float font_height)
{
    static const float ratio[] = {0.15f, 0.50f, 0.35f};
    zr_reset_font_height(ctx);
    zr_push_font_height(ctx, font_height);
    zr_layout_row(ctx, ZR_DYNAMIC, height, 3, ratio);
    zr_spacing(ctx, 1);
}

static void
button_demo(struct zr_context *ctx, struct icons *img)
{
    struct zr_panel layout;
    struct zr_panel menu;
    static int option = 1;
    static int toggle0 = 1;
    static int toggle1 = 0;
    static int toggle2 = 1;

    ctx->style.font.height = 20;
    zr_begin(ctx, &layout, "Button Demo", zr_rect(50,50,255,610),
        ZR_WINDOW_BORDER|ZR_WINDOW_MOVABLE|ZR_WINDOW_BORDER_HEADER|ZR_WINDOW_TITLE);

    /*------------------------------------------------
     *                  MENU
     *------------------------------------------------*/
    zr_menubar_begin(ctx);
    {
        /* toolbar */
        zr_layout_row_static(ctx, 40, 40, 4);
        if (zr_menu_icon_begin(ctx, &menu, "Music", img->play, 120))
        {
            /* settings */
            zr_layout_row_dynamic(ctx, 25, 1);
            zr_menu_item_icon(ctx, img->play, "Play", ZR_TEXT_RIGHT);
            zr_menu_item_icon(ctx, img->stop, "Stop", ZR_TEXT_RIGHT);
            zr_menu_item_icon(ctx, img->pause, "Pause", ZR_TEXT_RIGHT);
            zr_menu_item_icon(ctx, img->next, "Next", ZR_TEXT_RIGHT);
            zr_menu_item_icon(ctx, img->prev, "Prev", ZR_TEXT_RIGHT);
            zr_menu_end(ctx);
        }
        zr_button_image(ctx, img->tools, ZR_BUTTON_DEFAULT);
        zr_button_image(ctx, img->cloud, ZR_BUTTON_DEFAULT);
        zr_button_image(ctx, img->pen, ZR_BUTTON_DEFAULT);
    }
    zr_menubar_end(ctx);

    /*------------------------------------------------
     *                  BUTTON
     *------------------------------------------------*/
    ui_header(ctx, "Push buttons");
    ui_widget(ctx, 35, 22);
    if (zr_button_text(ctx, "Push me", ZR_BUTTON_DEFAULT))
        fprintf(stdout, "pushed!\n");
    ui_widget(ctx, 35, 22);
    if (zr_button_text_image(ctx, img->rocket, "Styled", ZR_TEXT_CENTERED, ZR_BUTTON_DEFAULT))
        fprintf(stdout, "rocket!\n");

    /*------------------------------------------------
     *                  REPEATER
     *------------------------------------------------*/
    ui_header(ctx, "Repeater");
    ui_widget(ctx, 35, 22);
    if (zr_button_text(ctx, "Press me", ZR_BUTTON_REPEATER))
        fprintf(stdout, "pressed!\n");

    /*------------------------------------------------
     *                  TOGGLE
     *------------------------------------------------*/
    ui_header(ctx, "Toggle buttons");
    ui_widget(ctx, 35, 22);
    if (zr_button_text_image(ctx, (toggle0) ? img->checked: img->unchecked,
        "Toggle", ZR_TEXT_LEFT, ZR_BUTTON_DEFAULT)) toggle0 = !toggle0;

    ui_widget(ctx, 35, 22);
    if (zr_button_text_image(ctx, (toggle1) ? img->checked: img->unchecked,
        "Toggle", ZR_TEXT_LEFT, ZR_BUTTON_DEFAULT)) toggle1 = !toggle1;

    ui_widget(ctx, 35, 22);
    if (zr_button_text_image(ctx, (toggle2) ? img->checked: img->unchecked,
        "Toggle", ZR_TEXT_LEFT, ZR_BUTTON_DEFAULT)) toggle2 = !toggle2;

    /*------------------------------------------------
     *                  RADIO
     *------------------------------------------------*/
    ui_header(ctx, "Radio buttons");
    ui_widget(ctx, 35, 22);
    if (zr_button_text_symbol(ctx, (option == 0)?ZR_SYMBOL_CIRCLE_FILLED:ZR_SYMBOL_CIRCLE,
            "Select", ZR_TEXT_LEFT, ZR_BUTTON_DEFAULT)) option = 0;
    ui_widget(ctx, 35, 22);
    if (zr_button_text_symbol(ctx, (option == 1)?ZR_SYMBOL_CIRCLE_FILLED:ZR_SYMBOL_CIRCLE,
            "Select", ZR_TEXT_LEFT, ZR_BUTTON_DEFAULT)) option = 1;
    ui_widget(ctx, 35, 22);
    if (zr_button_text_symbol(ctx, (option == 2)?ZR_SYMBOL_CIRCLE_FILLED:ZR_SYMBOL_CIRCLE,
            "Select", ZR_TEXT_LEFT, ZR_BUTTON_DEFAULT)) option = 2;

    /*------------------------------------------------
     *                  CONTEXTUAL
     *------------------------------------------------*/
    if (zr_contextual_begin(ctx, &menu, ZR_WINDOW_NO_SCROLLBAR, zr_vec2(120, 200), zr_window_get_bounds(ctx))) {
        ctx->style.font.height = 18;
        zr_layout_row_dynamic(ctx, 25, 1);
        if (zr_contextual_item_icon(ctx, img->copy, "Clone", ZR_TEXT_RIGHT))
            fprintf(stdout, "pressed clone!\n");
        if (zr_contextual_item_icon(ctx, img->del, "Delete", ZR_TEXT_RIGHT))
            fprintf(stdout, "pressed delete!\n");
        if (zr_contextual_item_icon(ctx, img->convert, "Convert", ZR_TEXT_RIGHT))
            fprintf(stdout, "pressed convert!\n");
        if (zr_contextual_item_icon(ctx, img->edit, "Edit", ZR_TEXT_RIGHT))
            fprintf(stdout, "pressed edit!\n");
        zr_contextual_end(ctx);
    }
    ctx->style.font.height = 14;
    zr_end(ctx);
}

/* ===============================================================
 *
 *                          BASIC DEMO
 *
 * ===============================================================*/
static void
basic_demo(struct zr_context *ctx, struct icons *img)
{
    static int image_active;
    static int check0 = 1;
    static int check1 = 0;
    static int slider = 30;
    static size_t prog = 80;
    static int selected_item = 0;
    static int selected_image = 3;
    static int selected_icon = 0;
    static const char *items[] = {"Item 0","item 1","item 2"};
    static int piemenu_active = 0;

    int i = 0;
    struct zr_panel layout;
    struct zr_panel combo;
    ctx->style.font.height = 20;
    zr_begin(ctx, &layout, "Basic Demo", zr_rect(320, 50, 275, 610),
        ZR_WINDOW_BORDER|ZR_WINDOW_MOVABLE|ZR_WINDOW_BORDER_HEADER|ZR_WINDOW_TITLE);

    /*------------------------------------------------
     *                  POPUP BUTTON
     *------------------------------------------------*/
    ui_header(ctx, "Popup & Scrollbar & Images");
    ui_widget(ctx, 35, 22);
    if (zr_button_text_image(ctx, img->directory,
        "Images", ZR_TEXT_CENTERED, ZR_BUTTON_DEFAULT))
        image_active = !image_active;

    /*------------------------------------------------
     *                  SELECTED IMAGE
     *------------------------------------------------*/
    ui_header(ctx, "Selected Image");
    ui_widget_centered(ctx, 100, 22);
    zr_image(ctx, img->images[selected_image]);

    /*------------------------------------------------
     *                  IMAGE POPUP
     *------------------------------------------------*/
    if (image_active) {
        struct zr_panel popup;
        if (zr_popup_begin(ctx, &popup, ZR_POPUP_STATIC, "Image Popup", 0, zr_rect(265, 0, 320, 220))) {
            zr_layout_row_static(ctx, 82, 82, 3);
            for (i = 0; i < 9; ++i) {
                if (zr_button_image(ctx, img->images[i], ZR_BUTTON_DEFAULT)) {
                    selected_image = i;
                    image_active = 0;
                    zr_popup_close(ctx);
                }
            }
            zr_popup_end(ctx);
        }
    }
    /*------------------------------------------------
     *                  COMBOBOX
     *------------------------------------------------*/
    ui_header(ctx, "Combo box");
    ui_widget(ctx, 40, 22);
    if (zr_combo_begin_text(ctx, &combo, items[selected_item], 200)) {
        zr_layout_row_dynamic(ctx, 35, 1);
        for (i = 0; i < 3; ++i)
            if (zr_combo_item(ctx, items[i], ZR_TEXT_LEFT))
                selected_item = i;
        zr_combo_end(ctx);
    }

    ui_widget(ctx, 40, 22);
    if (zr_combo_begin_icon(ctx, &combo, items[selected_icon], img->images[selected_icon], 200)) {
        zr_layout_row_dynamic(ctx, 35, 1);
        for (i = 0; i < 3; ++i)
            if (zr_combo_item_icon(ctx, img->images[i], items[i], ZR_TEXT_RIGHT))
                selected_icon = i;
        zr_combo_end(ctx);
    }

    /*------------------------------------------------
     *                  CHECKBOX
     *------------------------------------------------*/
    ui_header(ctx, "Checkbox");
    ui_widget(ctx, 30, 22);
    zr_checkbox(ctx, "Flag 1", &check0);
    ui_widget(ctx, 30, 22);
    zr_checkbox(ctx, "Flag 2", &check1);

    /*------------------------------------------------
     *                  PROGRESSBAR
     *------------------------------------------------*/
    ui_header(ctx, "Progressbar");
    ui_widget(ctx, 35, 22);
    zr_progress(ctx, &prog, 100, zr_true);

    /*------------------------------------------------
     *                  SLIDER
     *------------------------------------------------*/
    ui_header(ctx, "Slider");
    ui_widget(ctx, 35, 22);
    zr_slider_int(ctx, 0, &slider, 100, 10);

    /*------------------------------------------------
     *                  PIEMENU
     *------------------------------------------------*/
    if (zr_input_is_mouse_down(&ctx->input, ZR_BUTTON_RIGHT) &&
        zr_input_is_mouse_hovering_rect(&ctx->input, layout.bounds))
        piemenu_active = 1;

    if (piemenu_active) {
        int ret = ui_piemenu(ctx, zr_vec2(WINDOW_WIDTH/2-140, WINDOW_HEIGHT/2-140), 140, &img->menu[0], 6);
        if (ret != -1) {
            fprintf(stdout, "piemenu selected: %d\n", ret);
            piemenu_active = 0;
        }
    }
    ctx->style.font.height = 14;
    zr_end(ctx);
}


