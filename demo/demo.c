#include "limits.h"

#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#define MAX(a,b)    ((a) < (b) ? (b) : (a))
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#define LEN(a)      (sizeof(a)/sizeof(a)[0])
#define UNUSED(a)   ((void)(a))

#define MAX_BUFFER  64
#define MAX_MEMORY  (32 * 1024)
#define MAX_COMMAND_MEMORY  (16 * 1024)
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

enum theme {THEME_BLACK, THEME_WHITE};

struct demo {
    void *memory;
    enum theme theme;
    struct zr_context ctx;
};

static void
zr_labelf(struct zr_context *ctx, enum zr_text_align align, const char *fmt, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    buffer[1023] = 0;
    zr_label(ctx, buffer, align);
    va_end(args);
}

static void
set_style(struct zr_context *ctx, enum theme theme)
{
    if (theme == THEME_WHITE) {
        ctx->style.colors[ZR_COLOR_TEXT] = zr_rgba(70, 70, 70, 255);
        ctx->style.colors[ZR_COLOR_TEXT_HOVERING] = zr_rgba(10, 10, 10, 255);
        ctx->style.colors[ZR_COLOR_TEXT_ACTIVE] = zr_rgba(20, 20, 20, 255);
        ctx->style.colors[ZR_COLOR_WINDOW] = zr_rgba(175, 175, 175, 255);
        ctx->style.colors[ZR_COLOR_HEADER] = zr_rgba(175, 175, 175, 255);
        ctx->style.colors[ZR_COLOR_BORDER] = zr_rgba(0, 0, 0, 255);
        ctx->style.colors[ZR_COLOR_BUTTON] = zr_rgba(185, 185, 185, 255);
        ctx->style.colors[ZR_COLOR_BUTTON_HOVER] = zr_rgba(170, 170, 170, 255);
        ctx->style.colors[ZR_COLOR_BUTTON_ACTIVE] = zr_rgba(160, 160, 160, 255);
        ctx->style.colors[ZR_COLOR_TOGGLE] = zr_rgba(150, 150, 150, 255);
        ctx->style.colors[ZR_COLOR_TOGGLE_HOVER] = zr_rgba(120, 120, 120, 255);
        ctx->style.colors[ZR_COLOR_TOGGLE_CURSOR] = zr_rgba(175, 175, 175, 255);
        ctx->style.colors[ZR_COLOR_SELECTABLE] = zr_rgba(190, 190, 190, 255);
        ctx->style.colors[ZR_COLOR_SELECTABLE_HOVER] = zr_rgba(150, 150, 150, 255);
        ctx->style.colors[ZR_COLOR_SELECTABLE_TEXT] = zr_rgba(70, 70, 70, 255);
        ctx->style.colors[ZR_COLOR_SLIDER] = zr_rgba(190, 190, 190, 255);
        ctx->style.colors[ZR_COLOR_SLIDER_CURSOR] = zr_rgba(80, 80, 80, 255);
        ctx->style.colors[ZR_COLOR_SLIDER_CURSOR_HOVER] = zr_rgba(70, 70, 70, 255);
        ctx->style.colors[ZR_COLOR_SLIDER_CURSOR_ACTIVE] = zr_rgba(60, 60, 60, 255);
        ctx->style.colors[ZR_COLOR_PROGRESS] = zr_rgba(190, 190, 190, 255);
        ctx->style.colors[ZR_COLOR_PROGRESS_CURSOR] = zr_rgba(80, 80, 80, 255);
        ctx->style.colors[ZR_COLOR_PROGRESS_CURSOR_HOVER] = zr_rgba(70, 70, 70, 255);
        ctx->style.colors[ZR_COLOR_PROGRESS_CURSOR_ACTIVE] = zr_rgba(60, 60, 60, 255);
        ctx->style.colors[ZR_COLOR_PROPERTY] = zr_rgba(175, 175, 175, 255);
        ctx->style.colors[ZR_COLOR_PROPERTY_HOVER] = zr_rgba(160, 160, 160, 255);
        ctx->style.colors[ZR_COLOR_PROPERTY_ACTIVE] = zr_rgba(165, 165, 165, 255);
        ctx->style.colors[ZR_COLOR_INPUT] = zr_rgba(150, 150, 150, 255);
        ctx->style.colors[ZR_COLOR_INPUT_CURSOR] = zr_rgba(0, 0, 0, 255);
        ctx->style.colors[ZR_COLOR_INPUT_TEXT] = zr_rgba(0, 0, 0, 255);
        ctx->style.colors[ZR_COLOR_COMBO] = zr_rgba(175, 175, 175, 255);
        ctx->style.colors[ZR_COLOR_HISTO] = zr_rgba(160, 160, 160, 255);
        ctx->style.colors[ZR_COLOR_HISTO_BARS] = zr_rgba(45, 45, 45, 255);
        ctx->style.colors[ZR_COLOR_HISTO_HIGHLIGHT] = zr_rgba( 255, 0, 0, 255);
        ctx->style.colors[ZR_COLOR_PLOT] = zr_rgba(160, 160, 160, 255);
        ctx->style.colors[ZR_COLOR_PLOT_LINES] = zr_rgba(45, 45, 45, 255);
        ctx->style.colors[ZR_COLOR_PLOT_HIGHLIGHT] = zr_rgba(255, 0, 0, 255);
        ctx->style.colors[ZR_COLOR_SCROLLBAR] = zr_rgba(180, 180, 180, 255);
        ctx->style.colors[ZR_COLOR_SCROLLBAR_CURSOR] = zr_rgba(140, 140, 140, 255);
        ctx->style.colors[ZR_COLOR_SCROLLBAR_CURSOR_HOVER] = zr_rgba(150, 150, 150, 255);
        ctx->style.colors[ZR_COLOR_SCROLLBAR_CURSOR_ACTIVE] = zr_rgba(160, 160, 160, 255);
        ctx->style.colors[ZR_COLOR_TABLE_LINES] = zr_rgba(100, 100, 100, 255);
        ctx->style.colors[ZR_COLOR_TAB_HEADER] = zr_rgba(180, 180, 180, 255);
        ctx->style.colors[ZR_COLOR_SCALER] = zr_rgba(100, 100, 100, 255);
    }
    else {
        zr_load_default_style(ctx, ZR_DEFAULT_ALL);
    }
}

static int
demo_window(struct zr_layout *layout, struct zr_context *ctx, enum theme *theme)
{
    struct zr_layout menu;

    /* window flags */
    static int show_menu = zr_true;
    static int titlebar = zr_true;
    static int border = zr_true;
    static int resize = zr_true;
    static int moveable = zr_true;
    static int no_scrollbar = zr_false;
    static zr_flags window_flags = 0;
    static int minimizable = zr_true;
    static int close = zr_true;

    /* popups */
    static enum zr_style_header_align header_align = ZR_HEADER_RIGHT;
    static int show_app_about = zr_false;
    static int show_close_popup = zr_false;
    static int show_color_picker_popup = zr_false;
    static int color_picker_index;
    static struct zr_color color_picker_color;

    /* window flags */
    window_flags = 0;
    ctx->style.header.align = header_align;
    if (border) window_flags |= ZR_WINDOW_BORDER;
    if (resize) window_flags |= ZR_WINDOW_SCALABLE;
    if (moveable) window_flags |= ZR_WINDOW_MOVABLE;
    if (no_scrollbar) window_flags |= ZR_WINDOW_NO_SCROLLBAR;
    if (minimizable) window_flags |= ZR_WINDOW_MINIMIZABLE;
    if (close) window_flags |= ZR_WINDOW_CLOSABLE;

    /* main window */
    if (zr_begin(ctx, layout, "Demo", zr_rect(30, 30, 400, 600), window_flags))
    {
        if (show_menu)
        {
            /* menubar */
            enum menu_states {MENU_DEFAULT, MENU_TEST};
            static zr_size mprog = 60;
            static int mslider = 10;
            static int mcheck = zr_true;

            zr_menubar_begin(ctx);
            zr_layout_row_begin(ctx, ZR_STATIC, 25, 2);
            zr_layout_row_push(ctx, 45);
            if (zr_menu_text_begin(ctx, &menu, "MENU", 120))
            {
                static size_t prog = 40;
                static int slider = 10;
                static int check = zr_true;

                zr_layout_row_dynamic(ctx, 25, 1);
                zr_progress(ctx, &prog, 100, ZR_MODIFIABLE);
                zr_slider_int(ctx, 0, &slider, 16, 1);
                zr_checkbox(ctx, "check", &check);

                if (zr_menu_item(ctx, ZR_TEXT_CENTERED, "Hide"))
                    show_menu = zr_false;
                if (zr_menu_item(ctx, ZR_TEXT_CENTERED, "About"))
                    show_app_about = zr_true;
                if (zr_menu_item(ctx, ZR_TEXT_CENTERED, "Quit"))
                    show_close_popup = zr_true;
                zr_menu_end(ctx);
            }
            zr_layout_row_push(ctx, 60);
            zr_progress(ctx, &mprog, 100, ZR_MODIFIABLE);
            zr_slider_int(ctx, 0, &mslider, 16, 1);
            zr_checkbox(ctx, "check", &mcheck);
            zr_menubar_end(ctx);
        }

        if (show_app_about)
        {
            /* about popup */
            struct zr_layout popup;
            static struct zr_rect s = {20, 100, 300, 190};
            if (zr_popup_begin(ctx, &popup, ZR_POPUP_STATIC, "About", ZR_WINDOW_CLOSABLE, s))
            {
                zr_layout_row_dynamic(ctx, 20, 1);
                zr_label(ctx, "Zahnrad", ZR_TEXT_LEFT);
                zr_label(ctx, "By Micha Mettke", ZR_TEXT_LEFT);
                zr_label(ctx, "Zahnrad is licensed under the MIT License.",  ZR_TEXT_LEFT);
                zr_label(ctx, "See LICENSE for more information", ZR_TEXT_LEFT);
                zr_popup_end(ctx);
            } else show_app_about = zr_false;
        }

        if (show_close_popup)
        {
            /* close popup */
            struct zr_layout popup;
            static const struct zr_rect s = {20, 150, 230, 150};
            if(zr_popup_begin(ctx, &popup, ZR_POPUP_STATIC, "Quit", ZR_WINDOW_CLOSABLE, s))
            {
                zr_layout_row_dynamic(ctx, 30, 1);
                zr_label(ctx, "Are you sure you want to exit?", ZR_TEXT_LEFT);
                zr_layout_row_dynamic(ctx, 30, 4);
                zr_spacing(ctx, 1);
                if (zr_button_text(ctx, "Yes", ZR_BUTTON_DEFAULT))
                    return 0;
                if (zr_button_text(ctx, "No", ZR_BUTTON_DEFAULT)) {
                    show_close_popup = zr_false;
                    zr_popup_close(ctx);
                }
                zr_popup_end(ctx);
            } else show_close_popup = zr_false;
        }

        if (show_color_picker_popup)
        {
            /* color picker popup */
            struct zr_layout popup;
            if (zr_popup_begin(ctx, &popup, ZR_POPUP_STATIC, "Color Picker",
                ZR_WINDOW_CLOSABLE, zr_rect(10, 100, 360, 280)))
            {
                zr_layout_row_dynamic(ctx, 30, 2);
                zr_label(ctx, zr_get_color_name((enum zr_style_colors)color_picker_index), ZR_TEXT_LEFT);
                zr_button_color(ctx, color_picker_color, ZR_BUTTON_DEFAULT);

                /* color selection */
                zr_layout_row_dynamic(ctx, 30, 1);
                color_picker_color.r = (zr_byte)zr_propertyi(ctx, "#R:", 0, color_picker_color.r, 255, 1,1);
                color_picker_color.g = (zr_byte)zr_propertyi(ctx, "#G:", 0, color_picker_color.g, 255, 1,1);
                color_picker_color.b = (zr_byte)zr_propertyi(ctx, "#B", 0, color_picker_color.b, 255, 1,1);
                color_picker_color.a = (zr_byte)zr_propertyi(ctx, "#A", 0, color_picker_color.a, 255, 1,1);

                zr_layout_row_dynamic(ctx, 30, 4);
                zr_spacing(ctx, 1);
                if (zr_button_text(ctx, "ok", ZR_BUTTON_DEFAULT)) {
                    zr_popup_close(ctx);
                    show_color_picker_popup = zr_false;
                    ctx->style.colors[color_picker_index] = color_picker_color;
                }
                if (zr_button_text(ctx, "cancel", ZR_BUTTON_DEFAULT)) {
                    zr_popup_close(ctx);
                    show_color_picker_popup = zr_false;
                }
                zr_popup_end(ctx);
            } else show_color_picker_popup = zr_false;
        }

        /* window flags */
        if (zr_layout_push(ctx, ZR_LAYOUT_TAB, "Window")) {
            zr_layout_row_dynamic(ctx, 30, 2);
            zr_checkbox(ctx, "Titlebar", &titlebar);
            zr_checkbox(ctx, "Menu", &show_menu);
            zr_checkbox(ctx, "Border", &border);
            zr_checkbox(ctx, "Resizable", &resize);
            zr_checkbox(ctx, "Moveable", &moveable);
            zr_checkbox(ctx, "No Scrollbar", &no_scrollbar);
            zr_checkbox(ctx, "Minimizable", &minimizable);
            zr_checkbox(ctx, "Closeable", &close);
            zr_layout_pop(ctx);
        }

        if (zr_layout_push(ctx, ZR_LAYOUT_TAB, "Style"))
        {
            /* style editor */
            static const char *themes[] = {"Black", "White"};
            enum theme old = *theme;
            struct zr_layout combo;

            /* theme */
            zr_layout_row_static(ctx, 30, 80, 2);
            zr_label(ctx, "Theme:", ZR_TEXT_LEFT);

            if (zr_combo_begin_text(ctx, &combo, "themes", themes[*theme], 200)) {
                zr_layout_row_dynamic(ctx, 25, 1);
                *theme = zr_combo_item(ctx, themes[THEME_BLACK], ZR_TEXT_CENTERED) ? THEME_BLACK : *theme;
                *theme = zr_combo_item(ctx, themes[THEME_WHITE], ZR_TEXT_CENTERED) ? THEME_WHITE : *theme;
                if (old != *theme) set_style(ctx, *theme);
                zr_combo_end(ctx);
            }

            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Properties")) {
                size_t i = 0;
                zr_layout_row_dynamic(ctx, 22, 3);
                for (i = 0; i <= ZR_PROPERTY_SCROLLBAR_SIZE; ++i) {
                    zr_label(ctx, zr_get_property_name((enum zr_style_properties)i), ZR_TEXT_LEFT);
                    zr_property_float(ctx, "#X:", 0, &ctx->style.properties[i].x, 20, 1, 1);
                    zr_property_float(ctx, "#Y:", 0, &ctx->style.properties[i].y, 20, 1, 1);
                }
                zr_layout_pop(ctx);
            }

            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Rounding")) {
                size_t i = 0;
                zr_layout_row_dynamic(ctx, 22, 2);
                for (i = 0; i < ZR_ROUNDING_MAX; ++i) {
                    zr_label(ctx, zr_get_rounding_name((enum zr_style_rounding)i), ZR_TEXT_LEFT);
                    zr_property_float(ctx, "#R:", 0, &ctx->style.rounding[i], 20, 1, 1);
                }
                zr_layout_pop(ctx);
            }

            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Colors")){
                size_t i = 0;
                struct zr_layout tab;
                zr_layout_row_dynamic(ctx, 20, 1);
                zr_label(ctx, "Click on the color to modify", ZR_TEXT_LEFT);
                zr_layout_row_dynamic(ctx, 400, 1);
                if (zr_group_begin(ctx, &tab, "Color_Picker", 0)) {
                    for (i = 0; i < ZR_COLOR_COUNT; ++i) {
                        zr_layout_row_dynamic(ctx, 22, 2);
                        zr_label(ctx, zr_get_color_name((enum zr_style_colors)i), ZR_TEXT_LEFT);
                        if (zr_button_color(ctx, ctx->style.colors[i], ZR_BUTTON_DEFAULT)) {
                            show_color_picker_popup = zr_true;
                            color_picker_index = (int)i;
                            color_picker_color = ctx->style.colors[i];
                        }
                    }
                    zr_group_end(ctx);
                }
                zr_layout_pop(ctx);
            }
            zr_layout_pop(ctx);
        }

        if (zr_layout_push(ctx, ZR_LAYOUT_TAB, "Widgets"))
        {
            enum options {A,B,C};
            static int checkbox;
            static int option;
            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Text"))
            {
                /* Text Widgets */
                zr_layout_row_dynamic(ctx, 20, 1);
                zr_label(ctx, "Label aligned left", ZR_TEXT_LEFT);
                zr_label(ctx, "Label aligned centered", ZR_TEXT_CENTERED);
                zr_label(ctx, "Label aligned right", ZR_TEXT_RIGHT);
                zr_label_colored(ctx, "Blue text", ZR_TEXT_LEFT, zr_rgb(0,0,255));
                zr_label_colored(ctx, "Yellow text", ZR_TEXT_LEFT, zr_rgb(255,255,0));
                zr_text(ctx, "Text without /0", 15, ZR_TEXT_RIGHT);

                zr_layout_row_static(ctx, 100, 200, 1);
                zr_label_wrap(ctx, "This is a very long line to hopefully get this text to be wrapped into multiple lines to show line wrapping");
                zr_layout_row_dynamic(ctx, 100, 1);
                zr_label_wrap(ctx, "This is another long text to show dynamic window changes on multiline text");
                zr_layout_pop(ctx);
            }

            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Button"))
            {
                /* Buttons Widgets */
                zr_layout_row_static(ctx, 30, 100, 3);
                if (zr_button_text(ctx, "Button", ZR_BUTTON_DEFAULT))
                    fprintf(stdout, "Button pressed!\n");
                if (zr_button_text(ctx, "Repeater", ZR_BUTTON_REPEATER))
                    fprintf(stdout, "Repeater is being pressed!\n");
                zr_button_color(ctx, zr_rgb(0,0,255), ZR_BUTTON_DEFAULT);

                zr_layout_row_static(ctx, 20, 20, 8);
                zr_button_symbol(ctx, ZR_SYMBOL_CIRCLE, ZR_BUTTON_DEFAULT);
                zr_button_symbol(ctx, ZR_SYMBOL_CIRCLE_FILLED, ZR_BUTTON_DEFAULT);
                zr_button_symbol(ctx, ZR_SYMBOL_RECT, ZR_BUTTON_DEFAULT);
                zr_button_symbol(ctx, ZR_SYMBOL_RECT_FILLED, ZR_BUTTON_DEFAULT);
                zr_button_symbol(ctx, ZR_SYMBOL_TRIANGLE_UP, ZR_BUTTON_DEFAULT);
                zr_button_symbol(ctx, ZR_SYMBOL_TRIANGLE_DOWN, ZR_BUTTON_DEFAULT);
                zr_button_symbol(ctx, ZR_SYMBOL_TRIANGLE_LEFT, ZR_BUTTON_DEFAULT);
                zr_button_symbol(ctx, ZR_SYMBOL_TRIANGLE_RIGHT, ZR_BUTTON_DEFAULT);

                zr_layout_row_static(ctx, 30, 100, 2);
                zr_button_text_symbol(ctx, ZR_SYMBOL_TRIANGLE_LEFT, "prev", ZR_TEXT_RIGHT, ZR_BUTTON_DEFAULT);
                zr_button_text_symbol(ctx, ZR_SYMBOL_TRIANGLE_RIGHT, "next", ZR_TEXT_LEFT, ZR_BUTTON_DEFAULT);
                zr_layout_pop(ctx);
            }

            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Basic"))
            {
                /* Basic widgets */
                static int int_slider = 5;
                static float float_slider = 2.5f;
                static size_t prog_value = 40;
                static float property_float = 2;
                static int property_int = 10;

                static float range_float_min = 0;
                static float range_float_max = 100;
                static float range_float_value = 50;
                static int range_int_min = 0;
                static int range_int_value = 2048;
                static int range_int_max = 4096;
                static const float ratio[] = {120, 150};

                zr_layout_row_static(ctx, 30, 100, 1);
                zr_checkbox(ctx, "Checkbox", &checkbox);

                zr_layout_row_static(ctx, 30, 80, 3);
                option = zr_option(ctx, "optionA", option == A) ? A : option;
                option = zr_option(ctx, "optionB", option == B) ? B : option;
                option = zr_option(ctx, "optionC", option == C) ? C : option;


                zr_layout_row(ctx, ZR_STATIC, 30, 2, ratio);
                zr_labelf(ctx, ZR_TEXT_LEFT, "Slider int");
                zr_slider_int(ctx, 0, &int_slider, 10, 1);

                zr_label(ctx, "Slider float", ZR_TEXT_LEFT);
                zr_slider_float(ctx, 0, &float_slider, 5.0, 0.5f);
                zr_labelf(ctx, ZR_TEXT_LEFT, "Progressbar" , prog_value);
                zr_progress(ctx, &prog_value, 100, ZR_MODIFIABLE);

                zr_layout_row(ctx, ZR_STATIC, 25, 2, ratio);
                zr_label(ctx, "Property float:", ZR_TEXT_LEFT);
                zr_property_float(ctx, "Float:", 0, &property_float, 64.0f, 0.1f, 0.2f);
                zr_label(ctx, "Property int:", ZR_TEXT_LEFT);
                zr_property_int(ctx, "Int:", 0, &property_int, 100.0f, 1, 1);

                zr_layout_row_dynamic(ctx, 25, 1);
                zr_label(ctx, "Range:", ZR_TEXT_LEFT);
                zr_layout_row_dynamic(ctx, 25, 3);
                zr_property_float(ctx, "#min:", 0, &range_float_min, range_float_max, 1.0f, 0.2f);
                zr_property_float(ctx, "#float:", range_float_min, &range_float_value, range_float_max, 1.0f, 0.2f);
                zr_property_float(ctx, "#max:", range_float_min, &range_float_max, 100, 1.0f, 0.2f);

                zr_property_int(ctx, "#min:", INT_MIN, &range_int_min, range_int_value, 1, 10);
                zr_property_int(ctx, "#int:", range_int_min, &range_int_value, range_int_max, 1, 10);
                zr_property_int(ctx, "#max:", range_int_value, &range_int_max, INT_MAX, 1, 10);

                zr_layout_pop(ctx);
            }

            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Selectable"))
            {
                if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "List"))
                {
                    static int selected[4] = {zr_false, zr_false, zr_true, zr_false};
                    zr_layout_row_static(ctx, 18, 100, 1);
                    zr_selectable(ctx, "Selectable", ZR_TEXT_LEFT, &selected[0]);
                    zr_selectable(ctx, "Selectable", ZR_TEXT_LEFT, &selected[1]);
                    zr_label(ctx, "Not Selectable", ZR_TEXT_LEFT);
                    zr_selectable(ctx, "Selectable", ZR_TEXT_LEFT, &selected[2]);
                    zr_selectable(ctx, "Selectable", ZR_TEXT_LEFT, &selected[3]);
                    zr_layout_pop(ctx);
                }
                if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Grid"))
                {
                    int i;
                    static int selected[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
                    zr_layout_row_static(ctx, 50, 50, 4);
                    for (i = 0; i < 16; ++i) {
                        if (zr_selectable(ctx, "Z", ZR_TEXT_CENTERED, &selected[i])) {
                            int x = (i % 4), y = i / 4;
                            if (x > 0) selected[i - 1] ^= 1;
                            if (x < 3) selected[i + 1] ^= 1;
                            if (y > 0) selected[i - 4] ^= 1;
                            if (y < 3) selected[i + 4] ^= 1;
                        }
                    }
                    zr_layout_pop(ctx);
                }
                zr_layout_pop(ctx);
            }

            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Combo"))
            {
                /* Combobox Widgets */
                static float chart_selection = 8.0f;
                static const char *weapons[] = {"Fist","Pistol","Shotgun","Plasma","BFG"};
                static size_t current_weapon = 0;
                static int check_values[5];
                static struct zr_color combo_color = {130, 50, 50, 255};
                static size_t x =  20, y = 40, z = 10, w = 90;

                char buffer[32];
                size_t sum = 0;
                struct zr_layout combo;

                /* default combobox */
                zr_layout_row_static(ctx, 30, 200, 1);
                if (zr_combo_begin_text(ctx, &combo, "Weapons", weapons[current_weapon], 200)) {
                    size_t i = 0;
                    zr_layout_row_dynamic(ctx, 25, 1);
                    for (i = 0; i < LEN(weapons); ++i) {
                        if (zr_combo_item(ctx, weapons[i], ZR_TEXT_LEFT))
                            current_weapon = i;
                    }
                    zr_combo_end(ctx);
                }

                /* slider color combobox */
                if (zr_combo_begin_color(ctx, &combo, "Color", combo_color, 200)) {
                    float ratios[] = {0.15f, 0.85f};
                    zr_layout_row(ctx, ZR_DYNAMIC, 30, 2, ratios);
                    zr_label(ctx, "R", ZR_TEXT_LEFT);
                    combo_color.r = (zr_byte)zr_slide_int(ctx, 0, combo_color.r, 255, 5);
                    zr_label(ctx, "G", ZR_TEXT_LEFT);
                    combo_color.g = (zr_byte)zr_slide_int(ctx, 0, combo_color.g, 255, 5);
                    zr_label(ctx, "B", ZR_TEXT_LEFT);
                    combo_color.b = (zr_byte)zr_slide_int(ctx, 0, combo_color.b, 255, 5);
                    zr_label(ctx, "A", ZR_TEXT_LEFT);
                    combo_color.a = (zr_byte)zr_slide_int(ctx, 0, combo_color.a , 255, 5);
                    zr_combo_end(ctx);
                }

                /* progressbar combobox */
                sum = x + y + z + w;
                sprintf(buffer, "%lu", sum);
                if (zr_combo_begin_text(ctx, &combo, "Progressbar", buffer, 200)) {
                    zr_layout_row_dynamic(ctx, 30, 1);
                    zr_progress(ctx, &x, 100, ZR_MODIFIABLE);
                    zr_progress(ctx, &y, 100, ZR_MODIFIABLE);
                    zr_progress(ctx, &z, 100, ZR_MODIFIABLE);
                    zr_progress(ctx, &w, 100, ZR_MODIFIABLE);
                    zr_combo_end(ctx);
                }

                /* checkbox combobox */
                sum = (size_t)(check_values[0] + check_values[1] + check_values[2] + check_values[3] + check_values[4]);
                sprintf(buffer, "%lu", sum);
                if (zr_combo_begin_text(ctx, &combo, "Checkbox", buffer, 200)) {
                    zr_layout_row_dynamic(ctx, 30, 1);
                    zr_checkbox(ctx, weapons[0], &check_values[0]);
                    zr_checkbox(ctx, weapons[1], &check_values[1]);
                    zr_checkbox(ctx, weapons[2], &check_values[2]);
                    zr_checkbox(ctx, weapons[3], &check_values[3]);
                    zr_combo_end(ctx);
                }

                /* chart combobox */
                sprintf(buffer, "%.1f", chart_selection);
                if (zr_combo_begin_text(ctx, &combo, "Charts", buffer, 250)) {
                    size_t i = 0;
                    static const float values[]={30.0f,15.0f,25.0f,10.0f,20.0f,40.0f};
                    zr_layout_row_dynamic(ctx, 150, 1);
                    zr_chart_begin(ctx, ZR_CHART_COLUMN, LEN(values), 0, 50);
                    for (i = 0; i < LEN(values); ++i) {
                        zr_flags res = zr_chart_push(ctx, values[i]);
                        if (res & ZR_CHART_CLICKED) {
                            chart_selection = values[i];
                            zr_combo_close(ctx);
                        }
                    }
                    zr_chart_end(ctx);
                    zr_combo_end(ctx);
                }
                zr_layout_pop(ctx);
            }

            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Input"))
            {
                static const float ratio[] = {120, 150};
                static char field_buffer[64];
                static char box_buffer[512];
                static char text[9][64];
                static size_t text_len[9];
                static size_t field_len;
                static size_t box_len;
                int active;

                zr_layout_row(ctx, ZR_STATIC, 25, 2, ratio);
                zr_label(ctx, "Default:", ZR_TEXT_LEFT);

                zr_edit_string(ctx, ZR_EDIT_CURSOR, text[0], &text_len[0], 64, zr_filter_default);
                zr_label(ctx, "Int:", ZR_TEXT_LEFT);
                zr_edit_string(ctx, ZR_EDIT_CURSOR, text[1], &text_len[1], 64, zr_filter_decimal);
                zr_label(ctx, "Float:", ZR_TEXT_LEFT);
                zr_edit_string(ctx, ZR_EDIT_CURSOR, text[2], &text_len[2], 64, zr_filter_float);
                zr_label(ctx, "Hex:", ZR_TEXT_LEFT);
                zr_edit_string(ctx, ZR_EDIT_CURSOR, text[4], &text_len[4], 64, zr_filter_hex);
                zr_label(ctx, "Octal:", ZR_TEXT_LEFT);
                zr_edit_string(ctx, ZR_EDIT_CURSOR, text[5], &text_len[5], 64, zr_filter_oct);
                zr_label(ctx, "Binary:", ZR_TEXT_LEFT);
                zr_edit_string(ctx, ZR_EDIT_CURSOR, text[6], &text_len[6], 64, zr_filter_binary);

                zr_label(ctx, "Password:", ZR_TEXT_LEFT);
                {
                    size_t i = 0;
                    size_t old_len = text_len[8];
                    char buffer[64];
                    for (i = 0; i < text_len[8]; ++i) buffer[i] = '*';
                    zr_edit_string(ctx, ZR_EDIT_SIMPLE, buffer, &text_len[8], 64, zr_filter_default);
                    if (old_len < text_len[8])
                        memcpy(&text[8][old_len], &buffer[old_len], text_len[8] - old_len);
                }

                zr_label(ctx, "Field:", ZR_TEXT_LEFT);
                zr_edit_string(ctx, ZR_EDIT_FIELD, field_buffer, &field_len, 64, zr_filter_default);

                zr_label(ctx, "Box:", ZR_TEXT_LEFT);
                zr_layout_row_static(ctx, 75, 278, 1);
                zr_edit_string(ctx, ZR_EDIT_BOX, box_buffer, &box_len, 512, zr_filter_default);

                zr_layout_row(ctx, ZR_STATIC, 25, 2, ratio);
                active = zr_edit_string(ctx, ZR_EDIT_FIELD, text[7], &text_len[7], 64,  zr_filter_ascii);
                if (zr_button_text(ctx, "Submit", ZR_BUTTON_DEFAULT)
                    ||(active && zr_input_is_key_pressed(&ctx->input, ZR_KEY_ENTER))) {
                    text[7][text_len[7]] = '\n';
                    text_len[7]++;
                    memcpy(&box_buffer[box_len], &text[7], text_len[7]);
                    box_len += text_len[7];
                    text_len[7] = 0;
                }
                zr_layout_row_end(ctx);
                zr_layout_pop(ctx);
            }
            zr_layout_pop(ctx);
        }

        if (zr_layout_push(ctx, ZR_LAYOUT_TAB, "Chart"))
        {
            float id = 0;
            static int col_index = -1;
            static int line_index = -1;
            float step = (2*3.141592654f) / 32;

            int i;
            int index = -1;
            struct zr_rect bounds;

            /* column chart */
            zr_layout_row_dynamic(ctx, 100, 1);
            zr_layout_peek(&bounds, ctx);
            zr_chart_begin(ctx, ZR_CHART_COLUMN, 32, 0.0f, 1.0f);
            for (i = 0; i < 32; ++i) {
                zr_flags res = zr_chart_push(ctx, (float)fabs(sin(id)));
                if (res & ZR_CHART_HOVERING)
                    index = (int)i;
                if (res & ZR_CHART_CLICKED)
                    col_index = (int)i;
                id += step;
            }
            zr_chart_end(ctx);

            if (index != -1) {
                char buffer[ZR_MAX_NUMBER_BUFFER];
                sprintf(buffer, "Value: %.2f", (float)fabs(sin(step * (float)index)));
                zr_tooltip(ctx, buffer);
            }
            if (col_index != -1) {
                zr_layout_row_dynamic(ctx, 20, 1);
                zr_labelf(ctx, ZR_TEXT_LEFT, "Selected value: %.2f", (float)fabs(sin(step * (float)col_index)));
            }

            /* line chart */
            id = 0;
            index = -1;
            zr_layout_row_dynamic(ctx, 100, 1);
            zr_layout_peek(&bounds, ctx);
            zr_chart_begin(ctx, ZR_CHART_LINES, 32, -1.0f, 1.0f);
            for (i = 0; i < 32; ++i) {
                zr_flags res = zr_chart_push(ctx, (float)cos(id));
                if (res & ZR_CHART_HOVERING)
                    index = (int)i;
                if (res & ZR_CHART_CLICKED)
                    line_index = (int)i;
                id += step;
            }
            zr_chart_end(ctx);

            if (index != -1) {
                char buffer[ZR_MAX_NUMBER_BUFFER];
                float val = (float)cos((float)index*step);
                sprintf(buffer, "Value: %.2f", val);
                zr_tooltip(ctx, buffer);
            }
            if (line_index != -1) {
                zr_layout_row_dynamic(ctx, 20, 1);
                zr_labelf(ctx, ZR_TEXT_LEFT, "Selected value: %.2f", (float)cos((float)index*step));
            }
            zr_layout_pop(ctx);
        }

        if (zr_layout_push(ctx, ZR_LAYOUT_TAB, "Popup"))
        {
            enum {NONE=0, MENU=1, COLOR=2};
            static int active = MENU;
            static struct zr_color color = {255,0,0, 255};
            static int select[4];
            static int popup_active;

            const struct zr_input *in = &ctx->input;
            struct zr_rect menu_bounds, color_bounds, bounds;
            int is_menu, is_color;
            struct zr_vec2 size;

            /* contextual */
            zr_layout_row_static(ctx, 30, 150, 1);
            zr_layout_peek(&menu_bounds, ctx);
            zr_label(ctx, "Right click me for menu", ZR_TEXT_LEFT);

            /* color contextual */
            zr_layout_row_begin(ctx, ZR_STATIC, 30, 2);
            zr_layout_row_push(ctx, 100);
            zr_label(ctx, "Right Click here:", ZR_TEXT_LEFT);
            zr_layout_row_push(ctx, 50);
            zr_layout_peek(&color_bounds, ctx);
            zr_button_color(ctx, color, ZR_BUTTON_DEFAULT);
            zr_layout_row_end(ctx);

            /* This is a shitty hack to accomplish multiple contextual menus */
            is_menu = zr_input_mouse_clicked(in, ZR_BUTTON_RIGHT, menu_bounds);
            is_color = zr_input_mouse_clicked(in, ZR_BUTTON_RIGHT, color_bounds);
            size = (is_color || active == COLOR) ? zr_vec2(350, 60): zr_vec2(100, 200);
            if (zr_contextual_begin(ctx, &menu, 0, size, is_color|is_menu)) {
                if (is_menu || active == MENU) {
                    static size_t prog = 40;
                    static int slider = 10;

                    active = MENU;
                    zr_layout_row_dynamic(ctx, 25, 1);
                    zr_checkbox(ctx, "Menu", &show_menu);
                    zr_progress(ctx, &prog, 100, ZR_MODIFIABLE);
                    zr_slider_int(ctx, 0, &slider, 16, 1);
                    zr_selectable(ctx, select[0]?"Unselect":"Select", ZR_TEXT_LEFT, &select[0]);
                    zr_selectable(ctx, select[1]?"Unselect":"Select", ZR_TEXT_LEFT, &select[1]);
                    zr_selectable(ctx, select[2]?"Unselect":"Select", ZR_TEXT_LEFT, &select[2]);
                    zr_selectable(ctx, select[3]?"Unselect":"Select", ZR_TEXT_LEFT, &select[3]);
                    if (zr_contextual_item(ctx, "About", ZR_TEXT_CENTERED))
                        show_app_about = zr_true;
                    if (zr_contextual_item(ctx, "Quit", ZR_TEXT_CENTERED))
                        show_close_popup = zr_true;
                } else if (is_color || active == COLOR){
                    active = COLOR;
                    zr_layout_row_dynamic(ctx, 30, 4);
                    color.r = (zr_byte)zr_propertyi(ctx, "#r", 0, color.r, 255, 1, 1);
                    color.g = (zr_byte)zr_propertyi(ctx, "#g", 0, color.g, 255, 1, 1);
                    color.b = (zr_byte)zr_propertyi(ctx, "#b", 0, color.b, 255, 1, 1);
                    color.a = (zr_byte)zr_propertyi(ctx, "#a", 0, color.a, 255, 1, 1);
                }
                zr_contextual_end(ctx);
            } else active = NONE;

            /* popup */
            zr_layout_row_begin(ctx, ZR_STATIC, 30, 2);
            zr_layout_row_push(ctx, 100);
            zr_label(ctx, "Popup:", ZR_TEXT_LEFT);
            zr_layout_row_push(ctx, 50);
            if (zr_button_text(ctx, "Popup", ZR_BUTTON_DEFAULT))
                popup_active = 1;
            zr_layout_row_end(ctx);

            if (popup_active)
            {
                static struct zr_rect s = {20, 100, 220, 150};
                if (zr_popup_begin(ctx, &menu, ZR_POPUP_STATIC, "Error", ZR_WINDOW_DYNAMIC, s))
                {
                    zr_layout_row_dynamic(ctx, 25, 1);
                    zr_label(ctx, "A terrible error as occured", ZR_TEXT_LEFT);
                    zr_layout_row_dynamic(ctx, 25, 2);
                    if (zr_button_text(ctx, "OK", ZR_BUTTON_DEFAULT)) {
                        popup_active = 0;
                        zr_popup_close(ctx);
                    }
                    if (zr_button_text(ctx, "Cancel", ZR_BUTTON_DEFAULT)) {
                        popup_active = 0;
                        zr_popup_close(ctx);
                    }
                    zr_popup_end(ctx);
                } else popup_active = zr_false;
            }

            /* tooltip */
            zr_layout_row_static(ctx, 30, 150, 1);
            zr_layout_peek(&bounds, ctx);
            zr_label(ctx, "Hover me for tooltip", ZR_TEXT_LEFT);
            if (zr_input_is_mouse_hovering_rect(in, bounds))
                zr_tooltip(ctx, "This is a tooltip");

            zr_layout_pop(ctx);
        }

        if (zr_layout_push(ctx, ZR_LAYOUT_TAB, "Layout"))
        {
            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Widget"))
            {
                float ratio_two[] = {0.2f, 0.6f, 0.2f};
                float width_two[] = {100, 200, 50};

                zr_layout_row_dynamic(ctx, 30, 1);
                zr_label(ctx, "Dynamic fixed column layout with generated position and size:", ZR_TEXT_LEFT);
                zr_layout_row_dynamic(ctx, 30, 3);
                zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT);
                zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT);
                zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT);

                zr_layout_row_dynamic(ctx, 30, 1);
                zr_label(ctx, "static fixed column layout with generated position and size:", ZR_TEXT_LEFT);
                zr_layout_row_static(ctx, 30, 100, 3);
                zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT);
                zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT);
                zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT);

                zr_layout_row_dynamic(ctx, 30, 1);
                zr_label(ctx, "Dynamic array-based custom column layout with generated position and custom size:",ZR_TEXT_LEFT);
                zr_layout_row(ctx, ZR_DYNAMIC, 30, 3, ratio_two);
                zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT);
                zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT);
                zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT);

                zr_layout_row_dynamic(ctx, 30, 1);
                zr_label(ctx, "Static array-based custom column layout with generated position and custom size:",ZR_TEXT_LEFT );
                zr_layout_row(ctx, ZR_STATIC, 30, 3, width_two);
                zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT);
                zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT);
                zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT);

                zr_layout_row_dynamic(ctx, 30, 1);
                zr_label(ctx, "Dynamic immediate mode custom column layout with generated position and custom size:",ZR_TEXT_LEFT);
                zr_layout_row_begin(ctx, ZR_DYNAMIC, 30, 3);
                zr_layout_row_push(ctx, 0.2f);
                zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT);
                zr_layout_row_push(ctx, 0.6f);
                zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT);
                zr_layout_row_push(ctx, 0.2f);
                zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT);
                zr_layout_row_end(ctx);

                zr_layout_row_dynamic(ctx, 30, 1);
                zr_label(ctx, "Static immmediate mode custom column layout with generated position and custom size:", ZR_TEXT_LEFT);
                zr_layout_row_begin(ctx, ZR_STATIC, 30, 3);
                zr_layout_row_push(ctx, 100);
                zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT);
                zr_layout_row_push(ctx, 200);
                zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT);
                zr_layout_row_push(ctx, 50);
                zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT);
                zr_layout_row_end(ctx);

                zr_layout_row_dynamic(ctx, 30, 1);
                zr_label(ctx, "Static free space with custom position and custom size:", ZR_TEXT_LEFT);
                zr_layout_space_begin(ctx, ZR_STATIC, 120, 4);
                zr_layout_space_push(ctx, zr_rect(100, 0, 100, 30));
                zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT);
                zr_layout_space_push(ctx, zr_rect(0, 15, 100, 30));
                zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT);
                zr_layout_space_push(ctx, zr_rect(200, 15, 100, 30));
                zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT);
                zr_layout_space_push(ctx, zr_rect(100, 30, 100, 30));
                zr_button_text(ctx, "button", ZR_BUTTON_DEFAULT);
                zr_layout_space_end(ctx);
                zr_layout_pop(ctx);
            }

            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Group"))
            {
                static int group_titlebar = zr_false;
                static int group_border = zr_true;
                static int group_no_scrollbar = zr_false;
                static int group_width = 320;
                static int group_height = 200;
                struct zr_layout tab;

                zr_flags group_flags = 0;
                if (group_border) group_flags |= ZR_WINDOW_BORDER;
                if (group_no_scrollbar) group_flags |= ZR_WINDOW_NO_SCROLLBAR;
                if (group_titlebar) group_flags |= ZR_WINDOW_TITLE;

                zr_layout_row_dynamic(ctx, 30, 3);
                zr_checkbox(ctx, "Titlebar", &group_titlebar);
                zr_checkbox(ctx, "Border", &group_border);
                zr_checkbox(ctx, "No Scrollbar", &group_no_scrollbar);

                zr_layout_row_begin(ctx, ZR_STATIC, 22, 2);
                zr_layout_row_push(ctx, 50);
                zr_label(ctx, "size:", ZR_TEXT_LEFT);
                zr_layout_row_push(ctx, 130);
                zr_property_int(ctx, "#Width:", 100, &group_width, 500, 10, 1);
                zr_layout_row_push(ctx, 130);
                zr_property_int(ctx, "#Height:", 100, &group_height, 500, 10, 1);
                zr_layout_row_end(ctx);

                zr_layout_row_static(ctx, (float)group_height, (size_t)group_width, 2);
                if (zr_group_begin(ctx, &tab, "Group", group_flags)) {
                    int i = 0;
                    static int selected[16];
                    zr_layout_row_static(ctx, 18, 100, 1);
                    for (i = 0; i < 16; ++i)
                        zr_selectable(ctx, (selected[i]) ? "Selected": "Unselected", ZR_TEXT_CENTERED, &selected[i]);
                    zr_group_end(ctx);
                }
                zr_layout_pop(ctx);
            }

            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Simple"))
            {
                struct zr_layout tab;
                zr_layout_row_dynamic(ctx, 300, 2);
                if (zr_group_begin(ctx, &tab, "Group_Without_Border", 0)) {
                    int i = 0;
                    char buffer[64];
                    zr_layout_row_static(ctx, 18, 150, 1);
                    for (i = 0; i < 64; ++i) {
                        sprintf(buffer, "0x%02x", i);
                        zr_labelf(ctx, ZR_TEXT_LEFT, "%s: scrollable region", buffer);
                    }
                    zr_group_end(ctx);
                }
                if (zr_group_begin(ctx, &tab, "Group_With_Border", ZR_WINDOW_BORDER)) {
                    int i = 0;
                    char buffer[64];
                    zr_layout_row_dynamic(ctx, 25, 2);
                    for (i = 0; i < 64; ++i) {
                        sprintf(buffer, "%08d", ((((i%7)*10)^32))+(64+(i%2)*2));
                        zr_button_text(ctx, buffer, ZR_BUTTON_DEFAULT);
                    }
                    zr_group_end(ctx);
                }
                zr_layout_pop(ctx);
            }

            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Complex"))
            {
                int i;
                struct zr_layout tab;
                zr_layout_space_begin(ctx, ZR_STATIC, 500, 64);
                zr_layout_space_push(ctx, zr_rect(0,0,150,500));
                if (zr_group_begin(ctx, &tab, "Group_left", ZR_WINDOW_BORDER)) {
                    static int selected[32];
                    zr_layout_row_static(ctx, 18, 100, 1);
                    for (i = 0; i < 32; ++i)
                        zr_selectable(ctx, (selected[i]) ? "Selected": "Unselected", ZR_TEXT_CENTERED, &selected[i]);
                    zr_group_end(ctx);
                }

                zr_layout_space_push(ctx, zr_rect(160,0,150,240));
                if (zr_group_begin(ctx, &tab, "Group_top", ZR_WINDOW_BORDER)) {
                    zr_layout_row_dynamic(ctx, 25, 1);
                    zr_button_text(ctx, "#FFAA", ZR_BUTTON_DEFAULT);
                    zr_button_text(ctx, "#FFBB", ZR_BUTTON_DEFAULT);
                    zr_button_text(ctx, "#FFCC", ZR_BUTTON_DEFAULT);
                    zr_button_text(ctx, "#FFDD", ZR_BUTTON_DEFAULT);
                    zr_button_text(ctx, "#FFEE", ZR_BUTTON_DEFAULT);
                    zr_button_text(ctx, "#FFFF", ZR_BUTTON_DEFAULT);
                    zr_group_end(ctx);
                }

                zr_layout_space_push(ctx, zr_rect(160,250,150,250));
                if (zr_group_begin(ctx, &tab, "Group_buttom", ZR_WINDOW_BORDER)) {
                    zr_layout_row_dynamic(ctx, 25, 1);
                    zr_button_text(ctx, "#FFAA", ZR_BUTTON_DEFAULT);
                    zr_button_text(ctx, "#FFBB", ZR_BUTTON_DEFAULT);
                    zr_button_text(ctx, "#FFCC", ZR_BUTTON_DEFAULT);
                    zr_button_text(ctx, "#FFDD", ZR_BUTTON_DEFAULT);
                    zr_button_text(ctx, "#FFEE", ZR_BUTTON_DEFAULT);
                    zr_button_text(ctx, "#FFFF", ZR_BUTTON_DEFAULT);
                    zr_group_end(ctx);
                }

                zr_layout_space_push(ctx, zr_rect(320,0,150,150));
                if (zr_group_begin(ctx, &tab, "Group_right_top", ZR_WINDOW_BORDER)) {
                    static int selected[4];
                    zr_layout_row_static(ctx, 18, 100, 1);
                    for (i = 0; i < 4; ++i)
                        zr_selectable(ctx, (selected[i]) ? "Selected": "Unselected", ZR_TEXT_CENTERED, &selected[i]);
                    zr_group_end(ctx);
                }

                zr_layout_space_push(ctx, zr_rect(320,160,150,150));
                if (zr_group_begin(ctx, &tab, "Group_right_center", ZR_WINDOW_BORDER)) {
                    static int selected[4];
                    zr_layout_row_static(ctx, 18, 100, 1);
                    for (i = 0; i < 4; ++i)
                        zr_selectable(ctx, (selected[i]) ? "Selected": "Unselected", ZR_TEXT_CENTERED, &selected[i]);
                    zr_group_end(ctx);
                }

                zr_layout_space_push(ctx, zr_rect(320,320,150,150));
                if (zr_group_begin(ctx, &tab, "Group_right_bottom", ZR_WINDOW_BORDER)) {
                    static int selected[4];
                    zr_layout_row_static(ctx, 18, 100, 1);
                    for (i = 0; i < 4; ++i)
                        zr_selectable(ctx, (selected[i]) ? "Selected": "Unselected", ZR_TEXT_CENTERED, &selected[i]);
                    zr_group_end(ctx);
                }
                zr_layout_space_end(ctx);
                zr_layout_pop(ctx);
            }

            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Splitter"))
            {
                const struct zr_input *in = &ctx->input;
                zr_layout_row_static(ctx, 20, 320, 1);
                zr_label(ctx, "Use slider and spinner to change tile size", ZR_TEXT_LEFT);
                zr_label(ctx, "Drag the space between tiles to change tile ratio", ZR_TEXT_LEFT);

                if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Vertical"))
                {
                    static float a = 100, b = 100, c = 100;
                    struct zr_rect bounds;
                    struct zr_layout sub;

                    float row_layout[5];
                    row_layout[0] = a;
                    row_layout[1] = 8;
                    row_layout[2] = b;
                    row_layout[3] = 8;
                    row_layout[4] = c;

                    /* header */
                    zr_layout_row_static(ctx, 30, 100, 2);
                    zr_label(ctx, "left:", ZR_TEXT_LEFT);
                    zr_slider_float(ctx, 10.0f, &a, 200.0f, 10.0f);

                    zr_label(ctx, "middle:", ZR_TEXT_LEFT);
                    zr_slider_float(ctx, 10.0f, &b, 200.0f, 10.0f);

                    zr_label(ctx, "right:", ZR_TEXT_LEFT);
                    zr_slider_float(ctx, 10.0f, &c, 200.0f, 10.0f);

                    /* tiles */
                    zr_layout_row(ctx, ZR_STATIC, 200, 5, row_layout);
                    zr_push_property(ctx, ZR_PROPERTY_ITEM_SPACING, zr_vec2(0, 4));

                    /* left space */
                    if (zr_group_begin(ctx, &sub, "left", ZR_WINDOW_NO_SCROLLBAR|ZR_WINDOW_BORDER|ZR_WINDOW_NO_SCROLLBAR)) {
                        zr_layout_row_dynamic(ctx, 25, 1);
                        zr_button_text(ctx, "#FFAA", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFBB", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFCC", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFDD", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFEE", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFFF", ZR_BUTTON_DEFAULT);
                        zr_group_end(ctx);
                    }

                    /* scaler */
                    zr_layout_peek(&bounds, ctx);
                    zr_spacing(ctx, 1);
                    if ((zr_input_is_mouse_hovering_rect(in, bounds) ||
                        zr_input_is_mouse_prev_hovering_rect(in, bounds)) &&
                        zr_input_is_mouse_down(in, ZR_BUTTON_LEFT))
                    {
                        a = row_layout[0] + in->mouse.delta.x;
                        b = row_layout[2] - in->mouse.delta.x;
                    }

                    /* middle space */
                    if (zr_group_begin(ctx, &sub, "center", ZR_WINDOW_BORDER|ZR_WINDOW_NO_SCROLLBAR)) {
                        zr_layout_row_dynamic(ctx, 25, 1);
                        zr_button_text(ctx, "#FFAA", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFBB", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFCC", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFDD", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFEE", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFFF", ZR_BUTTON_DEFAULT);
                        zr_group_end(ctx);
                    }

                    /* scaler */
                    zr_layout_peek(&bounds, ctx);
                    zr_spacing(ctx, 1);
                    if ((zr_input_is_mouse_hovering_rect(in, bounds) ||
                        zr_input_is_mouse_prev_hovering_rect(in, bounds)) &&
                        zr_input_is_mouse_down(in, ZR_BUTTON_LEFT))
                    {
                        b = (row_layout[2] + in->mouse.delta.x);
                        c = (row_layout[4] - in->mouse.delta.x);
                    }

                    /* right space */
                    if (zr_group_begin(ctx, &sub, "right", ZR_WINDOW_BORDER|ZR_WINDOW_NO_SCROLLBAR)) {
                        zr_layout_row_dynamic(ctx, 25, 1);
                        zr_button_text(ctx, "#FFAA", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFBB", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFCC", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFDD", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFEE", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFFF", ZR_BUTTON_DEFAULT);
                        zr_group_end(ctx);
                    }

                    zr_pop_property(ctx);
                    zr_layout_pop(ctx);
                }

                if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Horizontal"))
                {
                    static float a = 100, b = 100, c = 100;
                    struct zr_layout sub;
                    struct zr_rect bounds;

                    /* header */
                    zr_layout_row_static(ctx, 30, 100, 2);
                    zr_label(ctx, "top:", ZR_TEXT_LEFT);
                    zr_slider_float(ctx, 10.0f, &a, 200.0f, 10.0f);

                    zr_label(ctx, "middle:", ZR_TEXT_LEFT);
                    zr_slider_float(ctx, 10.0f, &b, 200.0f, 10.0f);

                    zr_label(ctx, "bottom:", ZR_TEXT_LEFT);
                    zr_slider_float(ctx, 10.0f, &c, 200.0f, 10.0f);

                    /* top space */
                    zr_layout_row_dynamic(ctx, a, 1);
                    if (zr_group_begin(ctx, &sub, "top", ZR_WINDOW_NO_SCROLLBAR|ZR_WINDOW_BORDER)) {
                        zr_layout_row_dynamic(ctx, 25, 3);
                        zr_button_text(ctx, "#FFAA", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFBB", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFCC", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFDD", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFEE", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFFF", ZR_BUTTON_DEFAULT);
                        zr_group_end(ctx);
                    }

                    /* scaler */
                    zr_layout_row_dynamic(ctx, 8, 1);
                    zr_layout_peek(&bounds, ctx);
                    zr_spacing(ctx, 1);
                    if ((zr_input_is_mouse_hovering_rect(in, bounds) ||
                        zr_input_is_mouse_prev_hovering_rect(in, bounds)) &&
                        zr_input_is_mouse_down(in, ZR_BUTTON_LEFT))
                    {
                        a = a + in->mouse.delta.y;
                        b = b - in->mouse.delta.y;
                    }

                    /* middle space */
                    zr_layout_row_dynamic(ctx, b, 1);
                    if (zr_group_begin(ctx, &sub, "middle", ZR_WINDOW_NO_SCROLLBAR|ZR_WINDOW_BORDER)) {
                        zr_layout_row_dynamic(ctx, 25, 3);
                        zr_button_text(ctx, "#FFAA", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFBB", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFCC", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFDD", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFEE", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFFF", ZR_BUTTON_DEFAULT);
                        zr_group_end(ctx);
                    }

                    /* scaler */
                    zr_layout_row_dynamic(ctx, 8, 1);
                    zr_layout_peek(&bounds, ctx);
                    zr_spacing(ctx, 1);
                    if ((zr_input_is_mouse_hovering_rect(in, bounds) ||
                        zr_input_is_mouse_prev_hovering_rect(in, bounds)) &&
                        zr_input_is_mouse_down(in, ZR_BUTTON_LEFT))
                    {
                        b = b + in->mouse.delta.y;
                        c = c - in->mouse.delta.y;
                    }

                    /* bottom space */
                    zr_layout_row_dynamic(ctx, c, 1);
                    if (zr_group_begin(ctx, &sub, "bottom", ZR_WINDOW_NO_SCROLLBAR|ZR_WINDOW_BORDER)) {
                        zr_layout_row_dynamic(ctx, 25, 3);
                        zr_button_text(ctx, "#FFAA", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFBB", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFCC", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFDD", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFEE", ZR_BUTTON_DEFAULT);
                        zr_button_text(ctx, "#FFFF", ZR_BUTTON_DEFAULT);
                        zr_group_end(ctx);
                    }
                    zr_layout_pop(ctx);
                }
                zr_layout_pop(ctx);
            }
            zr_layout_pop(ctx);
        }

    }
    zr_end(ctx);
    return !zr_window_is_closed(ctx, "Demo");
}

static int
run_demo(struct demo *gui)
{
    static enum theme theme = THEME_BLACK;
    struct zr_layout layout;
    struct zr_context *ctx = &gui->ctx;

    /* extensive demo window */
    if (!demo_window(&layout, ctx, &theme))
        return 0;

    /* simple demo window */
    if (zr_begin(ctx, &layout, "Show", zr_rect(20, 20, 200, 200),
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

    /* metrics window */
    if (zr_begin(ctx, &layout, "Metrics", zr_rect(50, 600, 250, 300),
        ZR_WINDOW_BORDER|ZR_WINDOW_MOVABLE|ZR_WINDOW_SCALABLE|
        ZR_WINDOW_CLOSABLE|ZR_WINDOW_MINIMIZABLE))
    {
        struct zr_memory_status status;
        zr_buffer_info(&status, &ctx->memory);

        zr_layout_row_dynamic(ctx, 20, 2);
        zr_label(ctx,"Total:", ZR_TEXT_LEFT);
        zr_labelf(ctx, ZR_TEXT_LEFT, "%lu", status.size);
        zr_label(ctx,"Used:", ZR_TEXT_LEFT);
        zr_labelf(ctx, ZR_TEXT_LEFT, "%lu", status.allocated);
        zr_label(ctx,"Required:", ZR_TEXT_LEFT);
        zr_labelf(ctx, ZR_TEXT_LEFT, "%lu", status.needed);
        zr_label(ctx,"Calls:", ZR_TEXT_LEFT);
        zr_labelf(ctx, ZR_TEXT_LEFT, "%lu", status.calls);
    }
    zr_end(ctx);
    return 1;
}

