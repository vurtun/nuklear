#include "limits.h"

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#endif
#ifndef CLAMP
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#endif
#define LEN(a)      (sizeof(a)/sizeof(a)[0])
#define UNUSED(a)   ((void)(a))

#define MAX_BUFFER  64
#define MAX_MEMORY  (32 * 1024)
#define MAX_COMMAND_MEMORY  (16 * 1024)
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

struct icons {
    struct zr_image unchecked;
    struct zr_image checked;
    struct zr_image rocket;
    struct zr_image cloud;
    struct zr_image pen;
    struct zr_image play;
    struct zr_image pause;
    struct zr_image stop;
    struct zr_image prev;
    struct zr_image next;
    struct zr_image tools;
    struct zr_image directory;
    struct zr_image copy;
    struct zr_image convert;
    struct zr_image delete;
    struct zr_image edit;
    struct zr_image images[9];
    struct zr_image menu[6];
};

enum theme {THEME_BLACK, THEME_WHITE, THEME_RED, THEME_BLUE, THEME_DARK};
struct demo {
    void *memory;
    struct zr_context ctx;
    struct icons icons;
    enum theme theme;
    struct zr_memory_status status;
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
    } else if (theme == THEME_RED) {
        ctx->style.rounding[ZR_ROUNDING_SCROLLBAR] = 0;
        ctx->style.properties[ZR_PROPERTY_SCROLLBAR_SIZE] = zr_vec2(10,10);
        ctx->style.colors[ZR_COLOR_TEXT] = zr_rgba(190, 190, 190, 255);
        ctx->style.colors[ZR_COLOR_TEXT_HOVERING] = zr_rgba(195, 195, 195, 255);
        ctx->style.colors[ZR_COLOR_TEXT_ACTIVE] = zr_rgba(200, 200, 200, 255);
        ctx->style.colors[ZR_COLOR_WINDOW] = zr_rgba(30, 33, 40, 215);
        ctx->style.colors[ZR_COLOR_HEADER] = zr_rgba(181, 45, 69, 220);
        ctx->style.colors[ZR_COLOR_BORDER] = zr_rgba(51, 55, 67, 255);
        ctx->style.colors[ZR_COLOR_BUTTON] = zr_rgba(181, 45, 69, 255);
        ctx->style.colors[ZR_COLOR_BUTTON_HOVER] = zr_rgba(190, 50, 70, 255);
        ctx->style.colors[ZR_COLOR_BUTTON_ACTIVE] = zr_rgba(195, 55, 75, 255);
        ctx->style.colors[ZR_COLOR_TOGGLE] = zr_rgba(51, 55, 67, 255);
        ctx->style.colors[ZR_COLOR_TOGGLE_HOVER] = zr_rgba(45, 60, 60, 255);
        ctx->style.colors[ZR_COLOR_TOGGLE_CURSOR] = zr_rgba(181, 45, 69, 255);
        ctx->style.colors[ZR_COLOR_SELECTABLE] = zr_rgba(181, 45, 69, 255);
        ctx->style.colors[ZR_COLOR_SELECTABLE_HOVER] = zr_rgba(181, 45, 69, 255);
        ctx->style.colors[ZR_COLOR_SELECTABLE_TEXT] = zr_rgba(190, 190, 190, 255);
        ctx->style.colors[ZR_COLOR_SLIDER] = zr_rgba(51, 55, 67, 255);
        ctx->style.colors[ZR_COLOR_SLIDER_CURSOR] = zr_rgba(181, 45, 69, 255);
        ctx->style.colors[ZR_COLOR_SLIDER_CURSOR_HOVER] = zr_rgba(186, 50, 74, 255);
        ctx->style.colors[ZR_COLOR_SLIDER_CURSOR_ACTIVE] = zr_rgba(191, 55, 79, 255);
        ctx->style.colors[ZR_COLOR_PROGRESS] = zr_rgba(51, 55, 67, 255);
        ctx->style.colors[ZR_COLOR_PROGRESS_CURSOR] = zr_rgba(181, 45, 69, 255);
        ctx->style.colors[ZR_COLOR_PROGRESS_CURSOR_HOVER] = zr_rgba(186, 50, 74, 255);
        ctx->style.colors[ZR_COLOR_PROGRESS_CURSOR_ACTIVE] = zr_rgba(191, 55, 79, 255);
        ctx->style.colors[ZR_COLOR_PROPERTY] = zr_rgba(51, 55, 67, 255);
        ctx->style.colors[ZR_COLOR_PROPERTY_HOVER] = zr_rgba(55, 60, 72, 255);
        ctx->style.colors[ZR_COLOR_PROPERTY_ACTIVE] = zr_rgba(60, 65, 77, 255);
        ctx->style.colors[ZR_COLOR_INPUT] = zr_rgba(51, 55, 67, 225);
        ctx->style.colors[ZR_COLOR_INPUT_CURSOR] = zr_rgba(190, 190, 190, 255);
        ctx->style.colors[ZR_COLOR_INPUT_TEXT] = zr_rgba(190, 190, 190, 255);
        ctx->style.colors[ZR_COLOR_COMBO] = zr_rgba(51, 55, 67, 255);
        ctx->style.colors[ZR_COLOR_HISTO] = zr_rgba(51, 55, 67, 255);
        ctx->style.colors[ZR_COLOR_HISTO_BARS] = zr_rgba(170, 40, 60, 255);
        ctx->style.colors[ZR_COLOR_HISTO_HIGHLIGHT] = zr_rgba( 255, 0, 0, 255);
        ctx->style.colors[ZR_COLOR_PLOT] = zr_rgba(51, 55, 67, 255);
        ctx->style.colors[ZR_COLOR_PLOT_LINES] = zr_rgba(170, 40, 60, 255);
        ctx->style.colors[ZR_COLOR_PLOT_HIGHLIGHT] = zr_rgba(255, 0, 0, 255);
        ctx->style.colors[ZR_COLOR_SCROLLBAR] = zr_rgba(30, 33, 40, 255);
        ctx->style.colors[ZR_COLOR_SCROLLBAR_CURSOR] = zr_rgba(64, 84, 95, 255);
        ctx->style.colors[ZR_COLOR_SCROLLBAR_CURSOR_HOVER] = zr_rgba(70, 90, 100, 255);
        ctx->style.colors[ZR_COLOR_SCROLLBAR_CURSOR_ACTIVE] = zr_rgba(75, 95, 105, 255);
        ctx->style.colors[ZR_COLOR_TABLE_LINES] = zr_rgba(100, 100, 100, 255);
        ctx->style.colors[ZR_COLOR_TAB_HEADER] = zr_rgba(181, 45, 69, 220);
        ctx->style.colors[ZR_COLOR_SCALER] = zr_rgba(100, 100, 100, 255);
    } else if (theme == THEME_BLUE) {
        ctx->style.rounding[ZR_ROUNDING_SCROLLBAR] = 0;
        ctx->style.properties[ZR_PROPERTY_SCROLLBAR_SIZE] = zr_vec2(10,10);
        ctx->style.colors[ZR_COLOR_TEXT] = zr_rgba(20, 20, 20, 255);
        ctx->style.colors[ZR_COLOR_TEXT_HOVERING] = zr_rgba(195, 195, 195, 255);
        ctx->style.colors[ZR_COLOR_TEXT_ACTIVE] = zr_rgba(200, 200, 200, 255);
        ctx->style.colors[ZR_COLOR_WINDOW] = zr_rgba(202, 212, 214, 215);
        ctx->style.colors[ZR_COLOR_HEADER] = zr_rgba(246, 246, 246, 220);
        ctx->style.colors[ZR_COLOR_BORDER] = zr_rgba(140, 159, 173, 255);
        ctx->style.colors[ZR_COLOR_BUTTON] = zr_rgba(137, 182, 224, 255);
        ctx->style.colors[ZR_COLOR_BUTTON_HOVER] = zr_rgba(142, 187, 229, 255);
        ctx->style.colors[ZR_COLOR_BUTTON_ACTIVE] = zr_rgba(147, 192, 234, 255);
        ctx->style.colors[ZR_COLOR_TOGGLE] = zr_rgba(210, 210, 210, 255);
        ctx->style.colors[ZR_COLOR_TOGGLE_HOVER] = zr_rgba(245, 245, 245, 255);
        ctx->style.colors[ZR_COLOR_TOGGLE_CURSOR] = zr_rgba(142, 187, 229, 255);
        ctx->style.colors[ZR_COLOR_SELECTABLE] = zr_rgba(147, 192, 234, 255);
        ctx->style.colors[ZR_COLOR_SELECTABLE_HOVER] = zr_rgba(150, 150, 150, 255);
        ctx->style.colors[ZR_COLOR_SELECTABLE_TEXT] = zr_rgba(70, 70, 70, 255);
        ctx->style.colors[ZR_COLOR_SLIDER] = zr_rgba(210, 210, 210, 255);
        ctx->style.colors[ZR_COLOR_SLIDER_CURSOR] = zr_rgba(137, 182, 224, 245);
        ctx->style.colors[ZR_COLOR_SLIDER_CURSOR_HOVER] = zr_rgba(142, 188, 229, 255);
        ctx->style.colors[ZR_COLOR_SLIDER_CURSOR_ACTIVE] = zr_rgba(147, 193, 234, 255);
        ctx->style.colors[ZR_COLOR_PROGRESS] = zr_rgba(210, 210, 210, 255);
        ctx->style.colors[ZR_COLOR_PROGRESS_CURSOR] = zr_rgba(137, 182, 224, 255);
        ctx->style.colors[ZR_COLOR_PROGRESS_CURSOR_HOVER] = zr_rgba(142, 188, 229, 255);
        ctx->style.colors[ZR_COLOR_PROGRESS_CURSOR_ACTIVE] = zr_rgba(147, 193, 234, 255);
        ctx->style.colors[ZR_COLOR_PROPERTY] = zr_rgba(210, 210, 210, 255);
        ctx->style.colors[ZR_COLOR_PROPERTY_HOVER] = zr_rgba(235, 235, 235, 255);
        ctx->style.colors[ZR_COLOR_PROPERTY_ACTIVE] = zr_rgba(230, 230, 230, 255);
        ctx->style.colors[ZR_COLOR_INPUT] = zr_rgba(210, 210, 210, 225);
        ctx->style.colors[ZR_COLOR_INPUT_CURSOR] = zr_rgba(20, 20, 20, 255);
        ctx->style.colors[ZR_COLOR_INPUT_TEXT] = zr_rgba(20, 20, 20, 255);
        ctx->style.colors[ZR_COLOR_COMBO] = zr_rgba(210, 210, 210, 255);
        ctx->style.colors[ZR_COLOR_HISTO] = zr_rgba(210, 210, 210, 255);
        ctx->style.colors[ZR_COLOR_HISTO_BARS] = zr_rgba(137, 182, 224, 255);
        ctx->style.colors[ZR_COLOR_HISTO_HIGHLIGHT] = zr_rgba( 255, 0, 0, 255);
        ctx->style.colors[ZR_COLOR_PLOT] = zr_rgba(210, 210, 210, 255);
        ctx->style.colors[ZR_COLOR_PLOT_LINES] = zr_rgba(137, 182, 224, 255);
        ctx->style.colors[ZR_COLOR_PLOT_HIGHLIGHT] = zr_rgba(255, 0, 0, 255);
        ctx->style.colors[ZR_COLOR_SCROLLBAR] = zr_rgba(190, 200, 200, 255);
        ctx->style.colors[ZR_COLOR_SCROLLBAR_CURSOR] = zr_rgba(64, 84, 95, 255);
        ctx->style.colors[ZR_COLOR_SCROLLBAR_CURSOR_HOVER] = zr_rgba(70, 90, 100, 255);
        ctx->style.colors[ZR_COLOR_SCROLLBAR_CURSOR_ACTIVE] = zr_rgba(75, 95, 105, 255);
        ctx->style.colors[ZR_COLOR_TABLE_LINES] = zr_rgba(100, 100, 100, 255);
        ctx->style.colors[ZR_COLOR_TAB_HEADER] = zr_rgba(156, 193, 220, 255);
        ctx->style.colors[ZR_COLOR_SCALER] = zr_rgba(100, 100, 100, 255);
    } else if (theme == THEME_DARK) {
        ctx->style.rounding[ZR_ROUNDING_SCROLLBAR] = 0;
        ctx->style.properties[ZR_PROPERTY_SCROLLBAR_SIZE] = zr_vec2(10,10);
        ctx->style.colors[ZR_COLOR_TEXT] = zr_rgba(210, 210, 210, 255);
        ctx->style.colors[ZR_COLOR_TEXT_HOVERING] = zr_rgba(195, 195, 195, 255);
        ctx->style.colors[ZR_COLOR_TEXT_ACTIVE] = zr_rgba(200, 200, 200, 255);
        ctx->style.colors[ZR_COLOR_WINDOW] = zr_rgba(45, 53, 56, 215);
        ctx->style.colors[ZR_COLOR_HEADER] = zr_rgba(46, 46, 46, 220);
        ctx->style.colors[ZR_COLOR_BORDER] = zr_rgba(46, 46, 46, 255);
        ctx->style.colors[ZR_COLOR_BUTTON] = zr_rgba(48, 83, 111, 255);
        ctx->style.colors[ZR_COLOR_BUTTON_HOVER] = zr_rgba(53, 88, 116, 255);
        ctx->style.colors[ZR_COLOR_BUTTON_ACTIVE] = zr_rgba(58, 93, 121, 255);
        ctx->style.colors[ZR_COLOR_TOGGLE] = zr_rgba(50, 58, 61, 255);
        ctx->style.colors[ZR_COLOR_TOGGLE_HOVER] = zr_rgba(55, 63, 66, 255);
        ctx->style.colors[ZR_COLOR_TOGGLE_CURSOR] = zr_rgba(48, 83, 111, 255);
        ctx->style.colors[ZR_COLOR_SELECTABLE] = zr_rgba(48, 83, 111, 255);
        ctx->style.colors[ZR_COLOR_SELECTABLE_HOVER] = zr_rgba(48, 83, 111, 255);
        ctx->style.colors[ZR_COLOR_SELECTABLE_TEXT] = zr_rgba(210, 210, 210, 255);
        ctx->style.colors[ZR_COLOR_SLIDER] = zr_rgba(50, 58, 61, 255);
        ctx->style.colors[ZR_COLOR_SLIDER_CURSOR] = zr_rgba(48, 83, 111, 245);
        ctx->style.colors[ZR_COLOR_SLIDER_CURSOR_HOVER] = zr_rgba(53, 88, 116, 255);
        ctx->style.colors[ZR_COLOR_SLIDER_CURSOR_ACTIVE] = zr_rgba(58, 93, 121, 255);
        ctx->style.colors[ZR_COLOR_PROGRESS] = zr_rgba(50, 58, 61, 255);
        ctx->style.colors[ZR_COLOR_PROGRESS_CURSOR] = zr_rgba(48, 83, 111, 255);
        ctx->style.colors[ZR_COLOR_PROGRESS_CURSOR_HOVER] = zr_rgba(53, 88, 116, 255);
        ctx->style.colors[ZR_COLOR_PROGRESS_CURSOR_ACTIVE] = zr_rgba(58, 93, 121, 255);
        ctx->style.colors[ZR_COLOR_PROPERTY] = zr_rgba(50, 58, 61, 255);
        ctx->style.colors[ZR_COLOR_PROPERTY_HOVER] = zr_rgba(55, 63, 66, 255);
        ctx->style.colors[ZR_COLOR_PROPERTY_ACTIVE] = zr_rgba(60, 68, 71, 255);
        ctx->style.colors[ZR_COLOR_INPUT] = zr_rgba(50, 58, 61, 225);
        ctx->style.colors[ZR_COLOR_INPUT_CURSOR] = zr_rgba(210, 210, 210, 255);
        ctx->style.colors[ZR_COLOR_INPUT_TEXT] = zr_rgba(210, 210, 210, 255);
        ctx->style.colors[ZR_COLOR_COMBO] = zr_rgba(50, 58, 61, 255);
        ctx->style.colors[ZR_COLOR_HISTO] = zr_rgba(50, 58, 61, 255);
        ctx->style.colors[ZR_COLOR_HISTO_BARS] = zr_rgba(48, 83, 111, 255);
        ctx->style.colors[ZR_COLOR_HISTO_HIGHLIGHT] = zr_rgba(255, 0, 0, 255);
        ctx->style.colors[ZR_COLOR_PLOT] = zr_rgba(50, 58, 61, 255);
        ctx->style.colors[ZR_COLOR_PLOT_LINES] = zr_rgba(48, 83, 111, 255);
        ctx->style.colors[ZR_COLOR_PLOT_HIGHLIGHT] = zr_rgba(255, 0, 0, 255);
        ctx->style.colors[ZR_COLOR_SCROLLBAR] = zr_rgba(50, 58, 61, 255);
        ctx->style.colors[ZR_COLOR_SCROLLBAR_CURSOR] = zr_rgba(48, 83, 111, 255);
        ctx->style.colors[ZR_COLOR_SCROLLBAR_CURSOR_HOVER] = zr_rgba(53, 88, 116, 255);
        ctx->style.colors[ZR_COLOR_SCROLLBAR_CURSOR_ACTIVE] = zr_rgba(58, 93, 121, 255);
        ctx->style.colors[ZR_COLOR_TABLE_LINES] = zr_rgba(100, 100, 100, 255);
        ctx->style.colors[ZR_COLOR_TAB_HEADER] = zr_rgba(48, 83, 111, 255);
        ctx->style.colors[ZR_COLOR_SCALER] = zr_rgba(100, 100, 100, 255);
    } else {
        zr_load_default_style(ctx, ZR_DEFAULT_ALL);
    }
}

static int
control_window(struct zr_context *ctx, struct demo *gui)
{
    int i;
    struct zr_panel layout;
    if (zr_begin(ctx, &layout, "Control", zr_rect(0, 0, 350, 520),
        ZR_WINDOW_CLOSABLE|ZR_WINDOW_MINIMIZABLE|ZR_WINDOW_MOVABLE|
        ZR_WINDOW_SCALABLE|ZR_WINDOW_BORDER))
    {
        /* Style */
        if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Metrics", ZR_MINIMIZED)) {
            zr_layout_row_dynamic(ctx, 20, 2);
            zr_label(ctx,"Total:", ZR_TEXT_LEFT);
            zr_labelf(ctx, ZR_TEXT_LEFT, "%lu", gui->status.size);
            zr_label(ctx,"Used:", ZR_TEXT_LEFT);
            zr_labelf(ctx, ZR_TEXT_LEFT, "%lu", gui->status.allocated);
            zr_label(ctx,"Required:", ZR_TEXT_LEFT);
            zr_labelf(ctx, ZR_TEXT_LEFT, "%lu", gui->status.needed);
            zr_label(ctx,"Calls:", ZR_TEXT_LEFT);
            zr_labelf(ctx, ZR_TEXT_LEFT, "%lu", gui->status.calls);
            zr_layout_pop(ctx);
        }
        if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Properties", ZR_MINIMIZED)) {
            zr_layout_row_dynamic(ctx, 22, 3);
            for (i = 0; i <= ZR_PROPERTY_SCROLLBAR_SIZE; ++i) {
                zr_label(ctx, zr_get_property_name((enum zr_style_properties)i), ZR_TEXT_LEFT);
                zr_property_float(ctx, "#X:", 0, &ctx->style.properties[i].x, 20, 1, 1);
                zr_property_float(ctx, "#Y:", 0, &ctx->style.properties[i].y, 20, 1, 1);
            }
            zr_layout_pop(ctx);
        }
        if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Rounding", ZR_MINIMIZED)) {
            zr_layout_row_dynamic(ctx, 22, 2);
            for (i = 0; i < ZR_ROUNDING_MAX; ++i) {
                zr_label(ctx, zr_get_rounding_name((enum zr_style_rounding)i), ZR_TEXT_LEFT);
                zr_property_float(ctx, "#R:", 0, &ctx->style.rounding[i], 20, 1, 1);
            }
            zr_layout_pop(ctx);
        }
        if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Color", ZR_MINIMIZED))
        {
            struct zr_panel tab, combo;
            enum theme old = gui->theme;
            static const char *themes[] = {"Black", "White", "Red", "Blue", "Dark", "Grey"};

            zr_layout_row_dynamic(ctx,  25, 2);
            zr_label(ctx, "THEME:", ZR_TEXT_LEFT);
            if (zr_combo_begin_text(ctx, &combo, themes[gui->theme], 300)) {
                zr_layout_row_dynamic(ctx, 25, 1);
                gui->theme = zr_combo_item(ctx, themes[THEME_BLACK], ZR_TEXT_CENTERED) ? THEME_BLACK : gui->theme;
                gui->theme = zr_combo_item(ctx, themes[THEME_WHITE], ZR_TEXT_CENTERED) ? THEME_WHITE : gui->theme;
                gui->theme = zr_combo_item(ctx, themes[THEME_RED], ZR_TEXT_CENTERED) ? THEME_RED : gui->theme;
                gui->theme = zr_combo_item(ctx, themes[THEME_BLUE], ZR_TEXT_CENTERED) ? THEME_BLUE : gui->theme;
                gui->theme = zr_combo_item(ctx, themes[THEME_DARK], ZR_TEXT_CENTERED) ? THEME_DARK : gui->theme;
                if (old != gui->theme) set_style(ctx, gui->theme);
                zr_combo_end(ctx);
            }

            zr_layout_row_dynamic(ctx, 300, 1);
            if (zr_group_begin(ctx, &tab, "Colors", 0))
            {
                for (i = 0; i < ZR_COLOR_COUNT; ++i) {
                    zr_layout_row_dynamic(ctx, 25, 2);
                    zr_label(ctx, zr_get_color_name((enum zr_style_colors)i), ZR_TEXT_LEFT);
                    if (zr_combo_begin_color(ctx, &combo, ctx->style.colors[i], 200)) {
                        zr_layout_row_dynamic(ctx, 25, 1);
                        ctx->style.colors[i].r = (zr_byte)zr_propertyi(ctx, "#R:", 0, ctx->style.colors[i].r, 255, 1,1);
                        ctx->style.colors[i].g = (zr_byte)zr_propertyi(ctx, "#G:", 0, ctx->style.colors[i].g, 255, 1,1);
                        ctx->style.colors[i].b = (zr_byte)zr_propertyi(ctx, "#B:", 0, ctx->style.colors[i].b, 255, 1,1);
                        ctx->style.colors[i].a = (zr_byte)zr_propertyi(ctx, "#A:", 0, ctx->style.colors[i].a, 255, 1,1);
                        zr_combo_end(ctx);
                    }
                }
                zr_group_end(ctx);
            }
            zr_layout_pop(ctx);
        }
    }
    zr_end(ctx);
    return !zr_window_is_closed(ctx, "Control");
}

#include "simple.c"
#include "overview.c"
#include "extended.c"
#include "nodedit.c"

static int
run_demo(struct demo *gui)
{
    int ret = 1;
    static int init = 0;
    static struct node_editor nodedit;
    struct zr_context *ctx = &gui->ctx;

    if (!init) {
        memset(&nodedit, 0, sizeof(nodedit));
        node_editor_init(&nodedit);
        init = 1;
    }

    /* windows */
    simple_window(ctx);
    demo_window(ctx);
    node_editor_demo(ctx, &nodedit);
#ifndef DEMO_DO_NOT_DRAW_IMAGES
    grid_demo(ctx);
    button_demo(ctx, &gui->icons);
    basic_demo(ctx, &gui->icons);
#endif

    ret = control_window(ctx, gui);
    zr_buffer_info(&gui->status, &gui->ctx.memory);
    return ret;
}

