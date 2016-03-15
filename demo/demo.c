#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>
#include <stdarg.h>

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
#define MAX_MEMORY  (64 * 1024)
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
    struct zr_image dir;
    struct zr_image copy;
    struct zr_image convert;
    struct zr_image del;
    struct zr_image edit;
    struct zr_image images[9];
    struct zr_image menu[6];

    struct zr_image desktop;
    struct zr_image home;
    struct zr_image computer;
    struct zr_image directory;

    struct zr_image default_file;
    struct zr_image text_file;
    struct zr_image music_file;
    struct zr_image font_file;
    struct zr_image img_file;
    struct zr_image movie_file;

};

enum theme {THEME_BLACK, THEME_WHITE, THEME_RED, THEME_BLUE, THEME_DARK};
struct demo {
    void *memory;
    struct zr_context ctx;
    struct icons icons;
    enum theme theme;
    struct zr_memory_status status;

    int show_filex;
    int show_simple;
    int show_demo;
    int show_node;
    int show_grid;
    int show_button;
    int show_basic;
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

/* ===============================================================
 *
 *                          CONTROL WINDOW
 *
 * ===============================================================*/
static void
set_style(struct zr_context *ctx, enum theme theme)
{
    if (theme == THEME_WHITE) {
        ctx->style.rounding[ZR_ROUNDING_SCROLLBAR] = 0;
        ctx->style.rounding[ZR_ROUNDING_PROPERTY] = 0;
        ctx->style.rounding[ZR_ROUNDING_BUTTON] = 0;

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
        ctx->style.rounding[ZR_ROUNDING_PROPERTY] = 0;
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
        ctx->style.colors[ZR_COLOR_HEADER] = zr_rgba(137, 182, 224, 220);
        ctx->style.colors[ZR_COLOR_BORDER] = zr_rgba(140, 159, 173, 255);
        ctx->style.colors[ZR_COLOR_BUTTON] = zr_rgba(137, 182, 224, 255);
        ctx->style.colors[ZR_COLOR_BUTTON_HOVER] = zr_rgba(142, 187, 229, 255);
        ctx->style.colors[ZR_COLOR_BUTTON_ACTIVE] = zr_rgba(147, 192, 234, 255);
        ctx->style.colors[ZR_COLOR_TOGGLE] = zr_rgba(177, 210, 210, 255);
        ctx->style.colors[ZR_COLOR_TOGGLE_HOVER] = zr_rgba(182, 215, 215, 255);
        ctx->style.colors[ZR_COLOR_TOGGLE_CURSOR] = zr_rgba(137, 182, 224, 255);
        ctx->style.colors[ZR_COLOR_SELECTABLE] = zr_rgba(147, 192, 234, 255);
        ctx->style.colors[ZR_COLOR_SELECTABLE_HOVER] = zr_rgba(150, 150, 150, 255);
        ctx->style.colors[ZR_COLOR_SELECTABLE_TEXT] = zr_rgba(70, 70, 70, 255);
        ctx->style.colors[ZR_COLOR_SLIDER] = zr_rgba(177, 210, 210, 255);
        ctx->style.colors[ZR_COLOR_SLIDER_CURSOR] = zr_rgba(137, 182, 224, 245);
        ctx->style.colors[ZR_COLOR_SLIDER_CURSOR_HOVER] = zr_rgba(142, 188, 229, 255);
        ctx->style.colors[ZR_COLOR_SLIDER_CURSOR_ACTIVE] = zr_rgba(147, 193, 234, 255);
        ctx->style.colors[ZR_COLOR_PROGRESS] = zr_rgba(177, 210, 210, 255);
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
        ctx->style.colors[ZR_COLOR_WINDOW] = zr_rgba(57, 67, 71, 215);
        ctx->style.colors[ZR_COLOR_HEADER] = zr_rgba(51, 51, 56, 220);
        ctx->style.colors[ZR_COLOR_BORDER] = zr_rgba(46, 46, 46, 255);
        ctx->style.colors[ZR_COLOR_BUTTON] = zr_rgba(48, 83, 111, 255);
        ctx->style.colors[ZR_COLOR_BUTTON_HOVER] = zr_rgba(58, 93, 121, 255);
        ctx->style.colors[ZR_COLOR_BUTTON_ACTIVE] = zr_rgba(63, 98, 126, 255);
        ctx->style.colors[ZR_COLOR_TOGGLE] = zr_rgba(50, 58, 61, 255);
        ctx->style.colors[ZR_COLOR_TOGGLE_HOVER] = zr_rgba(45, 53, 56, 255);
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
        if (zr_layout_push(ctx, ZR_LAYOUT_TAB, "Windows", ZR_MINIMIZED)) {
            zr_layout_row_dynamic(ctx, 25, 2);
            gui->show_simple = !zr_window_is_closed(ctx, "Show");
            gui->show_node = !zr_window_is_closed(ctx, "Node Editor");
            gui->show_demo = !zr_window_is_closed(ctx, "Demo");
#ifndef DEMO_DO_NOT_DRAW_IMAGES
            gui->show_filex = !zr_window_is_closed(ctx, "File Browser");
            gui->show_grid = !zr_window_is_closed(ctx, "Grid Demo");
            gui->show_basic = !zr_window_is_closed(ctx, "Basic Demo");
            gui->show_button = !zr_window_is_closed(ctx, "Button Demo");
#endif

            if (zr_checkbox(ctx, "Show", &gui->show_simple) && !gui->show_simple)
                zr_window_close(ctx, "Show");
            if (zr_checkbox(ctx, "Demo", &gui->show_demo) && !gui->show_demo)
                zr_window_close(ctx, "Demo");
            if (zr_checkbox(ctx, "Node Editor", &gui->show_node) && !gui->show_node)
                zr_window_close(ctx, "Node Editor");
#ifndef DEMO_DO_NOT_DRAW_IMAGES
            if (zr_checkbox(ctx, "Grid", &gui->show_grid) && !gui->show_grid)
                zr_window_close(ctx, "Grid Demo");
            if (zr_checkbox(ctx, "Basic", &gui->show_basic) && !gui->show_basic)
                zr_window_close(ctx, "Basic Demo");
            if (zr_checkbox(ctx, "Button", &gui->show_button) && !gui->show_button)
                zr_window_close(ctx, "Button Demo");
            if (zr_checkbox(ctx, "Filex", &gui->show_filex) && !gui->show_filex)
                zr_window_close(ctx, "File Browser");
#endif
            zr_layout_pop(ctx);
        }
        if (zr_layout_push(ctx, ZR_LAYOUT_TAB, "Metrics", ZR_MINIMIZED)) {
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
        if (zr_layout_push(ctx, ZR_LAYOUT_TAB, "Properties", ZR_MINIMIZED)) {
            zr_layout_row_dynamic(ctx, 22, 3);
            for (i = 0; i <= ZR_PROPERTY_SCROLLBAR_SIZE; ++i) {
                zr_label(ctx, zr_get_property_name((enum zr_style_properties)i), ZR_TEXT_LEFT);
                zr_property_float(ctx, "#X:", 0, &ctx->style.properties[i].x, 20, 1, 1);
                zr_property_float(ctx, "#Y:", 0, &ctx->style.properties[i].y, 20, 1, 1);
            }
            zr_layout_pop(ctx);
        }
        if (zr_layout_push(ctx, ZR_LAYOUT_TAB, "Rounding", ZR_MINIMIZED)) {
            zr_layout_row_dynamic(ctx, 22, 2);
            for (i = 0; i < ZR_ROUNDING_MAX; ++i) {
                zr_label(ctx, zr_get_rounding_name((enum zr_style_rounding)i), ZR_TEXT_LEFT);
                zr_property_float(ctx, "#R:", 0, &ctx->style.rounding[i], 20, 1, 1);
            }
            zr_layout_pop(ctx);
        }
        if (zr_layout_push(ctx, ZR_LAYOUT_TAB, "Color", ZR_MINIMIZED))
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

                    if (zr_combo_begin_color(ctx, &combo, ctx->style.colors[i], 400)) {
                        enum color_mode {COL_RGB, COL_HSV};
                        static int col_mode = COL_RGB;
                        #ifndef DEMO_DO_NOT_USE_COLOR_PICKER
                        zr_layout_row_dynamic(ctx, 120, 1);
                        ctx->style.colors[i] = zr_color_picker(ctx, ctx->style.colors[i], ZR_RGBA);
                        #endif

                        zr_layout_row_dynamic(ctx, 25, 2);
                        col_mode = zr_option(ctx, "RGB", col_mode == COL_RGB) ? COL_RGB : col_mode;
                        col_mode = zr_option(ctx, "HSV", col_mode == COL_HSV) ? COL_HSV : col_mode;
                        zr_layout_row_dynamic(ctx, 25, 1);
                        if (col_mode == COL_RGB) {
                            ctx->style.colors[i].r = (zr_byte)zr_propertyi(ctx, "#R:", 0, ctx->style.colors[i].r, 255, 1,1);
                            ctx->style.colors[i].g = (zr_byte)zr_propertyi(ctx, "#G:", 0, ctx->style.colors[i].g, 255, 1,1);
                            ctx->style.colors[i].b = (zr_byte)zr_propertyi(ctx, "#B:", 0, ctx->style.colors[i].b, 255, 1,1);
                            ctx->style.colors[i].a = (zr_byte)zr_propertyi(ctx, "#A:", 0, ctx->style.colors[i].a, 255, 1,1);
                        } else {
                            zr_byte tmp[4];
                            zr_color_hsva_bv(tmp, ctx->style.colors[i]);
                            tmp[0] = (zr_byte)zr_propertyi(ctx, "#H:", 0, tmp[0], 255, 1,1);
                            tmp[1] = (zr_byte)zr_propertyi(ctx, "#S:", 0, tmp[1], 255, 1,1);
                            tmp[2] = (zr_byte)zr_propertyi(ctx, "#V:", 0, tmp[2], 255, 1,1);
                            tmp[3] = (zr_byte)zr_propertyi(ctx, "#A:", 0, tmp[3], 255, 1,1);
                            ctx->style.colors[i] = zr_hsva_bv(tmp);
                        }
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

/* ===============================================================
 *
 *                          SIMPLE WINDOW
 *
 * ===============================================================*/
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

/* ===============================================================
 *
 *                          DEMO WINDOW
 *
 * ===============================================================*/
static void
demo_window(struct demo *gui, struct zr_context *ctx)
{
    struct zr_panel menu;

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
    struct zr_panel layout;

    /* window flags */
    window_flags = 0;
    ctx->style.header.align = header_align;
    if (border) window_flags |= ZR_WINDOW_BORDER;
    if (resize) window_flags |= ZR_WINDOW_SCALABLE;
    if (moveable) window_flags |= ZR_WINDOW_MOVABLE;
    if (no_scrollbar) window_flags |= ZR_WINDOW_NO_SCROLLBAR;
    if (minimizable) window_flags |= ZR_WINDOW_MINIMIZABLE;
    if (close) window_flags |= ZR_WINDOW_CLOSABLE;

    if (zr_begin(ctx, &layout, "Demo", zr_rect(10, 10, 400, 750), window_flags))
    {
        if (show_menu)
        {
            /* menubar */
            enum menu_states {MENU_DEFAULT, MENU_WINDOWS};
            static enum menu_states menu_state = MENU_DEFAULT;
            static zr_size mprog = 60;
            static int mslider = 10;
            static int mcheck = zr_true;

            zr_menubar_begin(ctx);
            zr_layout_row_begin(ctx, ZR_STATIC, 25, 2);
            zr_layout_row_push(ctx, 45);
            if (zr_menu_text_begin(ctx, &menu, "MENU", ZR_TEXT_DEFAULT_LEFT, 120))
            {
                zr_layout_row_dynamic(ctx, 25, 1);
                if (menu_state == MENU_DEFAULT) {
                    static size_t prog = 40;
                    static int slider = 10;
                    static int check = zr_true;

                    if (zr_menu_item(ctx, ZR_TEXT_DEFAULT_LEFT, "Hide"))
                        show_menu = zr_false;
                    if (zr_menu_item(ctx, ZR_TEXT_DEFAULT_LEFT, "About"))
                        show_app_about = zr_true;
                    zr_progress(ctx, &prog, 100, ZR_MODIFIABLE);
                    zr_slider_int(ctx, 0, &slider, 16, 1);
                    zr_checkbox(ctx, "check", &check);
                    if (zr_button_text_symbol(ctx, ZR_SYMBOL_TRIANGLE_RIGHT,
                            "Windows", ZR_TEXT_DEFAULT_LEFT, ZR_BUTTON_DEFAULT))
                        menu_state = MENU_WINDOWS;
                } else {
                    if (zr_selectable(ctx, "Show", ZR_TEXT_DEFAULT_LEFT, &gui->show_simple) && !gui->show_simple)
                        zr_window_close(ctx, "Show");
                    if (zr_selectable(ctx, "Node Editor", ZR_TEXT_DEFAULT_LEFT, &gui->show_node) && !gui->show_node)
                        zr_window_close(ctx, "Node Editor");
                    #ifndef DEMO_DO_NOT_DRAW_IMAGES
                    if (zr_selectable(ctx, "Grid", ZR_TEXT_DEFAULT_LEFT, &gui->show_grid) && !gui->show_grid)
                        zr_window_close(ctx, "Grid Demo");
                    if (zr_selectable(ctx, "Basic", ZR_TEXT_DEFAULT_LEFT, &gui->show_basic) && !gui->show_basic)
                        zr_window_close(ctx, "Basic Demo");
                    if (zr_selectable(ctx, "Button", ZR_TEXT_DEFAULT_LEFT, &gui->show_button) && !gui->show_button)
                        zr_window_close(ctx, "Button Demo");
                    #endif
                    if (zr_button_text_symbol(ctx, ZR_SYMBOL_TRIANGLE_LEFT,
                            "Back", ZR_TEXT_DEFAULT_RIGHT, ZR_BUTTON_DEFAULT))
                        menu_state = MENU_DEFAULT;
                }
                zr_menu_end(ctx);
            } else menu_state = MENU_DEFAULT;
            zr_layout_row_push(ctx, 70);
            zr_progress(ctx, &mprog, 100, ZR_MODIFIABLE);
            zr_slider_int(ctx, 0, &mslider, 16, 1);
            zr_checkbox(ctx, "check", &mcheck);
            zr_menubar_end(ctx);
        }

        if (show_app_about)
        {
            /* about popup */
            struct zr_panel popup;
            static struct zr_rect s = {20, 100, 300, 190};
            if (zr_popup_begin(ctx, &popup, ZR_POPUP_STATIC, "About", ZR_WINDOW_CLOSABLE, s))
            {
                zr_layout_row_dynamic(ctx, 20, 1);
                zr_label(ctx, "Zahnrad", ZR_TEXT_LEFT);
                zr_label(ctx, "By Micha Mettke", ZR_TEXT_LEFT);
                zr_label(ctx, "Zahnrad is licensed under the zlib License.",  ZR_TEXT_LEFT);
                zr_label(ctx, "See LICENSE for more information", ZR_TEXT_LEFT);
                zr_popup_end(ctx);
            } else show_app_about = zr_false;
        }

        /* window flags */
        if (zr_layout_push(ctx, ZR_LAYOUT_TAB, "Window", ZR_MINIMIZED)) {
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

        if (zr_layout_push(ctx, ZR_LAYOUT_TAB, "Widgets", ZR_MINIMIZED))
        {
            enum options {A,B,C};
            static int checkbox;
            static int option;
            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Text", ZR_MINIMIZED))
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

            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Button", ZR_MINIMIZED))
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

            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Basic", ZR_MINIMIZED))
            {
                /* Basic widgets */
                static int int_slider = 5;
                static float float_slider = 2.5f;
                static size_t prog_value = 40;
                static float property_float = 2;
                static int property_int = 10;
                static int property_neg = 10;

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
                zr_label(ctx, "Property neg:", ZR_TEXT_LEFT);
                zr_property_int(ctx, "Neg:", -10, &property_neg, 10, 1, 1);


                zr_layout_row_dynamic(ctx, 25, 1);
                zr_label(ctx, "Range:", ZR_TEXT_LEFT);
                zr_layout_row_dynamic(ctx, 25, 3);
                zr_property_float(ctx, "#min:", 0, &range_float_min, range_float_max, 1.0f, 0.2f);
                zr_property_float(ctx, "#float:", range_float_min, &range_float_value, range_float_max, 1.0f, 0.2f);
                zr_property_float(ctx, "#max:", range_float_min, &range_float_max, 100, 1.0f, 0.2f);

                zr_property_int(ctx, "#min:", INT_MIN, &range_int_min, range_int_max, 1, 10);
                zr_property_int(ctx, "#neg:", range_int_min, &range_int_value, range_int_max, 1, 10);
                zr_property_int(ctx, "#max:", range_int_min, &range_int_max, INT_MAX, 1, 10);

                zr_layout_pop(ctx);
            }

            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Selectable", ZR_MINIMIZED))
            {
                if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "List", ZR_MINIMIZED))
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
                if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Grid", ZR_MINIMIZED))
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

            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Combo", ZR_MINIMIZED))
            {
                /* Combobox Widgets
                 * In this library comboboxes are not limited to being a popup
                 * list of selectable text. Instead it is a abstract concept of
                 * having something that is *selected* or displayed, a popup window
                 * which opens if something needs to be modified and the content
                 * of the popup which causes the *selected* or displayed value to
                 * change or if wanted close the combobox.
                 *
                 * While strange at first handling comboboxes in a abstract way
                 * solves the problem of overloaded window content. For example
                 * changing a color value requires 4 value modifier (slider, property,...)
                 * for RGBA then you need a label and ways to display the current color.
                 * If you want to go fancy you even add rgb and hsv ratio boxes.
                 * While fine for one color if you have a lot of them it because
                 * tedious to look at and quite wasteful in space. You could add
                 * a popup which modifies the color but this does not solve the
                 * fact that it still requires a lot of cluttered space to do.
                 *
                 * In these kind of instance abstract comboboxes are quite handy. All
                 * value modifiers are hidden inside the combobox popup and only
                 * the color is shown if not open. This combines the clarity of the
                 * popup with the ease of use of just using the space for modifiers.
                 *
                 * Other instances are for example time and especially date picker,
                 * which only show the currently activated time/data and hide the
                 * selection logic inside the combobox popup.
                 */
                static float chart_selection = 8.0f;
                static int current_weapon = 0;
                static int check_values[5];
                static float position[3];
                static struct zr_color combo_color = {130, 50, 50, 255};
                static struct zr_color combo_color2 = {130, 180, 50, 255};
                static size_t prog_a =  20, prog_b = 40, prog_c = 10, prog_d = 90;
                static const char *weapons[] = {"Fist","Pistol","Shotgun","Plasma","BFG"};

                char buffer[64];
                size_t sum = 0;
                struct zr_panel combo;

                /* default combobox */
                zr_layout_row_static(ctx, 25, 200, 1);
                current_weapon = zr_combo(ctx, weapons, LEN(weapons), current_weapon, 25);

                /* slider color combobox */
                if (zr_combo_begin_color(ctx, &combo, combo_color, 200)) {
                    float ratios[] = {0.15f, 0.85f};
                    zr_layout_row(ctx, ZR_DYNAMIC, 30, 2, ratios);
                    zr_label(ctx, "R:", ZR_TEXT_LEFT);
                    combo_color.r = (zr_byte)zr_slide_int(ctx, 0, combo_color.r, 255, 5);
                    zr_label(ctx, "G:", ZR_TEXT_LEFT);
                    combo_color.g = (zr_byte)zr_slide_int(ctx, 0, combo_color.g, 255, 5);
                    zr_label(ctx, "B:", ZR_TEXT_LEFT);
                    combo_color.b = (zr_byte)zr_slide_int(ctx, 0, combo_color.b, 255, 5);
                    zr_label(ctx, "A:", ZR_TEXT_LEFT);
                    combo_color.a = (zr_byte)zr_slide_int(ctx, 0, combo_color.a , 255, 5);
                    zr_combo_end(ctx);
                }

                /* complex color combobox */
                if (zr_combo_begin_color(ctx, &combo, combo_color2, 400)) {
                    enum color_mode {COL_RGB, COL_HSV};
                    static int col_mode = COL_RGB;
                    #ifndef DEMO_DO_NOT_USE_COLOR_PICKER
                    zr_layout_row_dynamic(ctx, 120, 1);
                    combo_color2 = zr_color_picker(ctx, combo_color2, ZR_RGBA);
                    #endif

                    zr_layout_row_dynamic(ctx, 25, 2);
                    col_mode = zr_option(ctx, "RGB", col_mode == COL_RGB) ? COL_RGB : col_mode;
                    col_mode = zr_option(ctx, "HSV", col_mode == COL_HSV) ? COL_HSV : col_mode;

                    zr_layout_row_dynamic(ctx, 25, 1);
                    if (col_mode == COL_RGB) {
                        combo_color2.r = (zr_byte)zr_propertyi(ctx, "#R:", 0, combo_color2.r, 255, 1,1);
                        combo_color2.g = (zr_byte)zr_propertyi(ctx, "#G:", 0, combo_color2.g, 255, 1,1);
                        combo_color2.b = (zr_byte)zr_propertyi(ctx, "#B:", 0, combo_color2.b, 255, 1,1);
                        combo_color2.a = (zr_byte)zr_propertyi(ctx, "#A:", 0, combo_color2.a, 255, 1,1);
                    } else {
                        zr_byte tmp[4];
                        zr_color_hsva_bv(tmp, combo_color2);
                        tmp[0] = (zr_byte)zr_propertyi(ctx, "#H:", 0, tmp[0], 255, 1,1);
                        tmp[1] = (zr_byte)zr_propertyi(ctx, "#S:", 0, tmp[1], 255, 1,1);
                        tmp[2] = (zr_byte)zr_propertyi(ctx, "#V:", 0, tmp[2], 255, 1,1);
                        tmp[3] = (zr_byte)zr_propertyi(ctx, "#A:", 0, tmp[3], 255, 1,1);
                        combo_color2 = zr_hsva_bv(tmp);
                    }
                    zr_combo_end(ctx);
                }

                /* progressbar combobox */
                sum = prog_a + prog_b + prog_c + prog_d;
                sprintf(buffer, "%lu", sum);
                if (zr_combo_begin_text(ctx, &combo, buffer, 200)) {
                    zr_layout_row_dynamic(ctx, 30, 1);
                    zr_progress(ctx, &prog_a, 100, ZR_MODIFIABLE);
                    zr_progress(ctx, &prog_b, 100, ZR_MODIFIABLE);
                    zr_progress(ctx, &prog_c, 100, ZR_MODIFIABLE);
                    zr_progress(ctx, &prog_d, 100, ZR_MODIFIABLE);
                    zr_combo_end(ctx);
                }

                /* checkbox combobox */
                sum = (size_t)(check_values[0] + check_values[1] + check_values[2] + check_values[3] + check_values[4]);
                sprintf(buffer, "%lu", sum);
                if (zr_combo_begin_text(ctx, &combo, buffer, 200)) {
                    zr_layout_row_dynamic(ctx, 30, 1);
                    zr_checkbox(ctx, weapons[0], &check_values[0]);
                    zr_checkbox(ctx, weapons[1], &check_values[1]);
                    zr_checkbox(ctx, weapons[2], &check_values[2]);
                    zr_checkbox(ctx, weapons[3], &check_values[3]);
                    zr_combo_end(ctx);
                }

                /* complex text combobox */
                sprintf(buffer, "%.2f, %.2f, %.2f", position[0], position[1],position[2]);
                if (zr_combo_begin_text(ctx, &combo, buffer, 200)) {
                    zr_layout_row_dynamic(ctx, 25, 1);
                    zr_property_float(ctx, "#X:", -1024.0f, &position[0], 1024.0f, 1,0.5f);
                    zr_property_float(ctx, "#Y:", -1024.0f, &position[1], 1024.0f, 1,0.5f);
                    zr_property_float(ctx, "#Z:", -1024.0f, &position[2], 1024.0f, 1,0.5f);
                    zr_combo_end(ctx);
                }

                /* chart combobox */
                sprintf(buffer, "%.1f", chart_selection);
                if (zr_combo_begin_text(ctx, &combo, buffer, 250)) {
                    size_t i = 0;
                    static const float values[]={26.0f,13.0f,30.0f,15.0f,25.0f,10.0f,20.0f,40.0f, 12.0f, 8.0f, 22.0f, 28.0f, 5.0f};
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

                {
                    static int time_selected = 0;
                    static int date_selected = 0;
                    static struct tm sel_time;
                    static struct tm sel_date;
                    if (!time_selected || !date_selected) {
                        /* keep time and date updated if nothing is selected */
                        time_t cur_time = time(0);
                        struct tm *n = localtime(&cur_time);
                        if (!time_selected)
                            memcpy(&sel_time, n, sizeof(struct tm));
                        if (!date_selected)
                            memcpy(&sel_date, n, sizeof(struct tm));
                    }

                    /* time combobox */
                    sprintf(buffer, "%02d:%02d:%02d", sel_time.tm_hour, sel_time.tm_min, sel_time.tm_sec);
                    if (zr_combo_begin_text(ctx, &combo, buffer, 250)) {
                        time_selected = 1;
                        zr_layout_row_dynamic(ctx, 25, 1);
                        sel_time.tm_sec = zr_propertyi(ctx, "#S:", 0, sel_time.tm_sec, 60, 1, 1);
                        sel_time.tm_min = zr_propertyi(ctx, "#M:", 0, sel_time.tm_min, 60, 1, 1);
                        sel_time.tm_hour = zr_propertyi(ctx, "#H:", 0, sel_time.tm_hour, 23, 1, 1);
                        zr_combo_end(ctx);
                    }

                    /* date combobox */
                    zr_layout_row_static(ctx, 25, 350, 1);
                    sprintf(buffer, "%02d-%02d-%02d", sel_date.tm_mday, sel_date.tm_mon+1, sel_date.tm_year+1900);
                    if (zr_combo_begin_text(ctx, &combo, buffer, 400)) {
                        int i = 0;
                        const char *month[] = {"January", "February", "March", "Apil", "May", "June", "July", "August", "September", "Ocotober", "November", "December"};
                        const char *week_days[] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
                        const int month_days[] = {31,28,31,30,31,30,31,31,30,31,30,31};
                        int year = sel_date.tm_year+1900;
                        int leap_year = (!(year % 4) && ((year % 100))) || !(year % 400);
                        int days = (sel_date.tm_mon == 1) ?
                            month_days[sel_date.tm_mon] + leap_year:
                            month_days[sel_date.tm_mon];

                        /* header with month and year */
                        date_selected = 1;
                        zr_layout_row_begin(ctx, ZR_DYNAMIC, 20, 3);
                        zr_layout_row_push(ctx, 0.05f);
                        if (zr_button_symbol(ctx, ZR_SYMBOL_TRIANGLE_LEFT, ZR_BUTTON_DEFAULT)) {
                            if (sel_date.tm_mon == 0) {
                                sel_date.tm_mon = 11;
                                sel_date.tm_year = MAX(0, sel_date.tm_year-1);
                            } else sel_date.tm_mon--;
                        }
                        zr_layout_row_push(ctx, 0.9f);
                        sprintf(buffer, "%s %d", month[sel_date.tm_mon], year);
                        zr_label(ctx, buffer, ZR_TEXT_DEFAULT_CENTER);
                        zr_layout_row_push(ctx, 0.05f);
                        if (zr_button_symbol(ctx, ZR_SYMBOL_TRIANGLE_RIGHT, ZR_BUTTON_DEFAULT)) {
                            if (sel_date.tm_mon == 11) {
                                sel_date.tm_mon = 0;
                                sel_date.tm_year++;
                            } else sel_date.tm_mon++;
                        }
                        zr_layout_row_end(ctx);

                        /* good old week day formula (have to use double because precision) */
                        {int year_n = (sel_date.tm_mon < 2) ? year-1: year;
                        int y = year_n % 100;
                        int c = year_n / 100;
                        int y4 = (int)((float)y / 4);
                        int c4 = (int)((float)c / 4);
                        int m = (int)(2.6 * (double)(((sel_date.tm_mon + 10) % 12) + 1) - 0.2);
                        int week_day = (((1 + m + y + y4 + c4 - 2 * c) % 7) + 7) % 7;

                        /* weekdays  */
                        zr_layout_row_dynamic(ctx, 35, 7);
                        for (i = 0; i < (int)LEN(week_days); ++i)
                            zr_label(ctx, week_days[i], ZR_TEXT_DEFAULT_CENTER);

                        /* days  */
                        if (week_day > 0) zr_spacing(ctx, week_day);
                        for (i = 1; i <= days; ++i) {
                            sprintf(buffer, "%d", i);
                            if (zr_button_text(ctx, buffer, ZR_BUTTON_DEFAULT)) {
                                sel_date.tm_mday = i;
                                zr_combo_close(ctx);
                            }
                        }}
                        zr_combo_end(ctx);
                    }
                }

                zr_layout_pop(ctx);
            }

            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Input", ZR_MINIMIZED))
            {
                static const float ratio[] = {120, 150};
                static char field_buffer[64];
                static char box_buffer[512];
                static char text[9][64];
                static size_t text_len[9];
                static size_t field_len;
                static size_t box_len;
                zr_flags active;

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
                zr_layout_row_static(ctx, 180, 278, 1);
                zr_edit_string(ctx, ZR_EDIT_BOX, box_buffer, &box_len, 512, zr_filter_default);

                zr_layout_row(ctx, ZR_STATIC, 25, 2, ratio);
                active = zr_edit_string(ctx, ZR_EDIT_FIELD|ZR_EDIT_SIGCOMIT, text[7], &text_len[7], 64,  zr_filter_ascii);
                if (zr_button_text(ctx, "Submit", ZR_BUTTON_DEFAULT) ||
                    (active & ZR_EDIT_COMMITED))
                {
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

        if (zr_layout_push(ctx, ZR_LAYOUT_TAB, "Chart", ZR_MINIMIZED))
        {
            /* Chart Widgets
             * This library has two different rather simple charts. The line and the
             * column chart. Both provide a simple way of visualizing values and
             * have a retain mode and immedidate mode API version. For the retain
             * mode version `zr_plot` and `zr_plot_function` you either provide
             * an array or a callback to call to handle drawing the graph.
             * For the immediate mode version you start by calling `zr_chart_begin`
             * and need to provide min and max values for scaling on the Y-axis.
             * and then call `zr_chart_push` to push values into the chart.
             * Finally `zr_chart_end` needs to be called to end the process. */
            float id = 0;
            static int col_index = -1;
            static int line_index = -1;
            float step = (2*3.141592654f) / 32;

            int i;
            int index = -1;
            struct zr_rect bounds;

            /* line chart */
            id = 0;
            index = -1;
            zr_layout_row_dynamic(ctx, 100, 1);
            bounds = zr_widget_bounds(ctx);
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

            /* column chart */
            zr_layout_row_dynamic(ctx, 100, 1);
            bounds = zr_widget_bounds(ctx);
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
            zr_layout_pop(ctx);
        }

        if (zr_layout_push(ctx, ZR_LAYOUT_TAB, "Popup", ZR_MINIMIZED))
        {
            static struct zr_color color = {255,0,0, 255};
            static int select[4];
            static int popup_active;
            const struct zr_input *in = &ctx->input;
            struct zr_rect bounds;

            /* menu contextual */
            zr_layout_row_static(ctx, 30, 150, 1);
            bounds = zr_widget_bounds(ctx);
            zr_label(ctx, "Right click me for menu", ZR_TEXT_LEFT);

            if (zr_contextual_begin(ctx, &menu, 0, zr_vec2(100, 300), bounds)) {
                static size_t prog = 40;
                static int slider = 10;

                zr_layout_row_dynamic(ctx, 25, 1);
                zr_checkbox(ctx, "Menu", &show_menu);
                zr_progress(ctx, &prog, 100, ZR_MODIFIABLE);
                zr_slider_int(ctx, 0, &slider, 16, 1);
                if (zr_contextual_item(ctx, "About", ZR_TEXT_CENTERED))
                    show_app_about = zr_true;
                zr_selectable(ctx, select[0]?"Unselect":"Select", ZR_TEXT_LEFT, &select[0]);
                zr_selectable(ctx, select[1]?"Unselect":"Select", ZR_TEXT_LEFT, &select[1]);
                zr_selectable(ctx, select[2]?"Unselect":"Select", ZR_TEXT_LEFT, &select[2]);
                zr_selectable(ctx, select[3]?"Unselect":"Select", ZR_TEXT_LEFT, &select[3]);
                zr_contextual_end(ctx);
            }

            /* color contextual */
            zr_layout_row_begin(ctx, ZR_STATIC, 30, 2);
            zr_layout_row_push(ctx, 100);
            zr_label(ctx, "Right Click here:", ZR_TEXT_LEFT);
            zr_layout_row_push(ctx, 50);
            bounds = zr_widget_bounds(ctx);
            zr_button_color(ctx, color, ZR_BUTTON_DEFAULT);
            zr_layout_row_end(ctx);

            if (zr_contextual_begin(ctx, &menu, 0, zr_vec2(350, 60), bounds)) {
                zr_layout_row_dynamic(ctx, 30, 4);
                color.r = (zr_byte)zr_propertyi(ctx, "#r", 0, color.r, 255, 1, 1);
                color.g = (zr_byte)zr_propertyi(ctx, "#g", 0, color.g, 255, 1, 1);
                color.b = (zr_byte)zr_propertyi(ctx, "#b", 0, color.b, 255, 1, 1);
                color.a = (zr_byte)zr_propertyi(ctx, "#a", 0, color.a, 255, 1, 1);
                zr_contextual_end(ctx);
            }

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
            bounds = zr_widget_bounds(ctx);
            zr_label(ctx, "Hover me for tooltip", ZR_TEXT_LEFT);
            if (zr_input_is_mouse_hovering_rect(in, bounds))
                zr_tooltip(ctx, "This is a tooltip");

            zr_layout_pop(ctx);
        }

        if (zr_layout_push(ctx, ZR_LAYOUT_TAB, "Layout", ZR_MINIMIZED))
        {
            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Widget", ZR_MINIMIZED))
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

            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Group", ZR_MINIMIZED))
            {
                static int group_titlebar = zr_false;
                static int group_border = zr_true;
                static int group_no_scrollbar = zr_false;
                static int group_width = 320;
                static int group_height = 200;
                struct zr_panel tab;

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

                zr_layout_row_static(ctx, (float)group_height, group_width, 2);
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

            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Simple", ZR_MINIMIZED))
            {
                struct zr_panel tab;
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

            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Complex", ZR_MINIMIZED))
            {
                int i;
                struct zr_panel tab;
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

            if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Splitter", ZR_MINIMIZED))
            {
                const struct zr_input *in = &ctx->input;
                zr_layout_row_static(ctx, 20, 320, 1);
                zr_label(ctx, "Use slider and spinner to change tile size", ZR_TEXT_LEFT);
                zr_label(ctx, "Drag the space between tiles to change tile ratio", ZR_TEXT_LEFT);

                if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Vertical", ZR_MINIMIZED))
                {
                    static float a = 100, b = 100, c = 100;
                    struct zr_rect bounds;
                    struct zr_panel sub;

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
                    bounds = zr_widget_bounds(ctx);
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
                    bounds = zr_widget_bounds(ctx);
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

                    zr_layout_pop(ctx);
                }

                if (zr_layout_push(ctx, ZR_LAYOUT_NODE, "Horizontal", ZR_MINIMIZED))
                {
                    static float a = 100, b = 100, c = 100;
                    struct zr_panel sub;
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
                    bounds = zr_widget_bounds(ctx);
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

                    {
                        /* scaler */
                        zr_layout_row_dynamic(ctx, 8, 1);
                        bounds = zr_widget_bounds(ctx);
                        if ((zr_input_is_mouse_hovering_rect(in, bounds) ||
                            zr_input_is_mouse_prev_hovering_rect(in, bounds)) &&
                            zr_input_is_mouse_down(in, ZR_BUTTON_LEFT))
                        {
                            b = b + in->mouse.delta.y;
                            c = c - in->mouse.delta.y;
                        }
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
}

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
    if (zr_button_text_image(ctx, img->dir,
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
/* ===============================================================
 *
 *                          FILE BROWSER
 *
 * ===============================================================*/
/* sorry only works for posix systems right now because of the directory function */
enum file_groups {
    FILE_GROUP_DEFAULT,
    FILE_GROUP_TEXT,
    FILE_GROUP_MUSIC,
    FILE_GROUP_FONT,
    FILE_GROUP_IMAGE,
    FILE_GROUP_MOVIE,
    FILE_GROUP_MAX
};

enum file_types {
    FILE_DEFAULT,
    FILE_TEXT,
    FILE_C_SOURCE,
    FILE_CPP_SOURCE,
    FILE_HEADER,
    FILE_CPP_HEADER,
    FILE_MP3,
    FILE_WAV,
    FILE_OGG,
    FILE_TTF,
    FILE_BMP,
    FILE_PNG,
    FILE_JPEG,
    FILE_PCX,
    FILE_TGA,
    FILE_GIF,
    FILE_MAX
};

struct file_group {
    enum file_groups group;
    const char *name;
    struct zr_image *icon;
};

struct file {
    enum file_types type;
    const char *suffix;
    enum file_groups group;
};

struct media {
    int font;
    int icon_sheet;
    struct icons *icons;
    struct file_group group[FILE_GROUP_MAX];
    struct file files[FILE_MAX];
};

#define MAX_PATH_LEN 512
struct file_browser {
    /* path */
    char file[MAX_PATH_LEN];
    char home[MAX_PATH_LEN];
    char desktop[MAX_PATH_LEN];
    char directory[MAX_PATH_LEN];

    /* directory content */
    char **files;
    char **directories;
    size_t file_count;
    size_t dir_count;
    struct media media;
};

#ifndef DEMO_DO_NOT_DRAW_IMAGES
#ifdef __unix__

#include <dirent.h>
#include <unistd.h>

#ifndef _WIN32
# include <pwd.h>
#endif

static char*
str_duplicate(const char *src)
{
    char *ret;
    size_t len = strlen(src);
    if (!len) return 0;
    ret = (char*)malloc(len+1);
    if (!ret) return 0;
    memcpy(ret, src, len);
    ret[len] = '\0';
    return ret;
}

static void
dir_free_list(char **list, size_t size)
{
    size_t i;
    for (i = 0; i < size; ++i)
        free(list[i]);
    free(list);
}

static char**
dir_list(const char *dir, int return_subdirs, size_t *count)
{
    size_t n = 0;
    char buffer[MAX_PATH_LEN];
    char **results = NULL;
    const DIR *none = NULL;
    size_t capacity = 32;
    size_t size;
    DIR *z;

    assert(dir);
    assert(count);
    strncpy(buffer, dir, MAX_PATH_LEN);
    n = strlen(buffer);

    if (n > 0 && (buffer[n-1] != '/'))
        buffer[n++] = '/';

    size = 0;

    z = opendir(dir);
    if (z != none) {
        int nonempty = 1;
        struct dirent *data = readdir(z);
        nonempty = (data != NULL);
        if (!nonempty) return NULL;

        do {
            DIR *y;
            char *p;
            int is_subdir;
            if (data->d_name[0] == '.')
                continue;

            strncpy(buffer + n, data->d_name, MAX_PATH_LEN-n);
            y = opendir(buffer);
            is_subdir = (y != NULL);
            if (y != NULL) closedir(y);

            if ((return_subdirs && is_subdir) || (!is_subdir && !return_subdirs)){
                if (!size) {
                    results = (char**)calloc(sizeof(char*), capacity);
                } else if (size >= capacity) {
                    capacity = capacity * 2;
                    results = (char**)realloc(results, capacity * sizeof(char*));
                }
                p = str_duplicate(data->d_name);
                results[size++] = p;
            }
        } while ((data = readdir(z)) != NULL);
    }

    if (z) closedir(z);
    *count = size;
    return results;
}

static struct file_group
FILE_GROUP(enum file_groups group, const char *name, struct zr_image *icon)
{
    struct file_group fg;
    fg.group = group;
    fg.name = name;
    fg.icon = icon;
    return fg;
}

static struct file
FILE_DEF(enum file_types type, const char *suffix, enum file_groups group)
{
    struct file fd;
    fd.type = type;
    fd.suffix = suffix;
    fd.group = group;
    return fd;
}

static struct zr_image*
media_icon_for_file(struct media *media, const char *file)
{
    int i = 0;
    const char *s = file;
    char suffix[4];
    int found = 0;
    memset(suffix, 0, sizeof(suffix));

    /* extract suffix .xxx from file */
    while (*s++ != '\0') {
        if (found && i < 3)
            suffix[i++] = *s;

        if (*s == '.') {
            if (found){
                found = 0;
                break;
            }
            found = 1;
        }
    }

    /* check for all file definition of all groups for fitting suffix*/
    for (i = 0; i < FILE_MAX && found; ++i) {
        struct file *d = &media->files[i];
        {
            const char *f = d->suffix;
            s = suffix;
            while (f && *f && *s && *s == *f) {
                s++; f++;
            }

            /* found correct file definition so */
            if (f && *s == '\0' && *f == '\0')
                return media->group[d->group].icon;
        }
    }
    return &media->icons->default_file;
}

static void
media_init(struct media *media, struct icons *icons)
{
    /* file groups */
    media->icons = icons;
    media->group[FILE_GROUP_DEFAULT] = FILE_GROUP(FILE_GROUP_DEFAULT,"default",&icons->default_file);
    media->group[FILE_GROUP_TEXT] = FILE_GROUP(FILE_GROUP_TEXT, "textual", &icons->text_file);
    media->group[FILE_GROUP_MUSIC] = FILE_GROUP(FILE_GROUP_MUSIC, "music", &icons->music_file);
    media->group[FILE_GROUP_FONT] = FILE_GROUP(FILE_GROUP_FONT, "font", &icons->font_file);
    media->group[FILE_GROUP_IMAGE] = FILE_GROUP(FILE_GROUP_IMAGE, "image", &icons->img_file);
    media->group[FILE_GROUP_MOVIE] = FILE_GROUP(FILE_GROUP_MOVIE, "movie", &icons->movie_file);

    /* files */
    media->files[FILE_DEFAULT] = FILE_DEF(FILE_DEFAULT, NULL, FILE_GROUP_DEFAULT);
    media->files[FILE_TEXT] = FILE_DEF(FILE_TEXT, "txt", FILE_GROUP_TEXT);
    media->files[FILE_C_SOURCE] = FILE_DEF(FILE_C_SOURCE, "c", FILE_GROUP_TEXT);
    media->files[FILE_CPP_SOURCE] = FILE_DEF(FILE_CPP_SOURCE, "cpp", FILE_GROUP_TEXT);
    media->files[FILE_HEADER] = FILE_DEF(FILE_HEADER, "h", FILE_GROUP_TEXT);
    media->files[FILE_CPP_HEADER] = FILE_DEF(FILE_HEADER, "hpp", FILE_GROUP_TEXT);
    media->files[FILE_MP3] = FILE_DEF(FILE_MP3, "mp3", FILE_GROUP_MUSIC);
    media->files[FILE_WAV] = FILE_DEF(FILE_WAV, "wav", FILE_GROUP_MUSIC);
    media->files[FILE_OGG] = FILE_DEF(FILE_OGG, "ogg", FILE_GROUP_MUSIC);
    media->files[FILE_TTF] = FILE_DEF(FILE_TTF, "ttf", FILE_GROUP_FONT);
    media->files[FILE_BMP] = FILE_DEF(FILE_BMP, "bmp", FILE_GROUP_IMAGE);
    media->files[FILE_PNG] = FILE_DEF(FILE_PNG, "png", FILE_GROUP_IMAGE);
    media->files[FILE_JPEG] = FILE_DEF(FILE_JPEG, "jpg", FILE_GROUP_IMAGE);
    media->files[FILE_PCX] = FILE_DEF(FILE_PCX, "pcx", FILE_GROUP_IMAGE);
    media->files[FILE_TGA] = FILE_DEF(FILE_TGA, "tga", FILE_GROUP_IMAGE);
    media->files[FILE_GIF] = FILE_DEF(FILE_GIF, "gif", FILE_GROUP_IMAGE);
}

static void
file_browser_reload_directory_content(struct file_browser *browser, const char *path)
{
    strncpy(browser->directory, path, MAX_PATH_LEN);
    dir_free_list(browser->files, browser->file_count);
    dir_free_list(browser->directories, browser->dir_count);
    browser->files = dir_list(path, 0, &browser->file_count);
    browser->directories = dir_list(path, 1, &browser->dir_count);
}

static void
file_browser_init(struct file_browser *browser, struct icons *icons)
{
    memset(browser, 0, sizeof(*browser));
    media_init(&browser->media, icons);
    {
        /* load files and sub-directory list */
        const char *home = getenv("HOME");
#ifdef _WIN32
        if (!home) home = getenv("USERPROFILE");
#else
        if (!home) home = getpwuid(getuid())->pw_dir;
        {
            size_t l;
            strncpy(browser->home, home, MAX_PATH_LEN);
            l = strlen(browser->home);
            strcpy(browser->home + l, "/");
            strcpy(browser->directory, browser->home);
        }
#endif
        {
            size_t l;
            strcpy(browser->desktop, browser->home);
            l = strlen(browser->desktop);
            strcpy(browser->desktop + l, "desktop/");
        }
        browser->files = dir_list(browser->directory, 0, &browser->file_count);
        browser->directories = dir_list(browser->directory, 1, &browser->dir_count);
    }
}

static void
file_browser_free(struct file_browser *browser)
{
    if (browser->files)
        dir_free_list(browser->files, browser->file_count);
    if (browser->directories)
        dir_free_list(browser->directories, browser->dir_count);
    browser->files = NULL;
    browser->directories = NULL;
    memset(browser, 0, sizeof(*browser));
}

static int
file_browser_run(struct file_browser *browser, struct zr_context *ctx)
{
    int ret = 0;
    struct zr_panel layout;
    struct media *media = &browser->media;
    struct icons *icons = media->icons;
    struct zr_rect total_space;

    if (zr_begin(ctx, &layout, "File Browser", zr_rect(50, 50, 800, 600),
        ZR_WINDOW_BORDER|ZR_WINDOW_NO_SCROLLBAR|ZR_WINDOW_CLOSABLE|ZR_WINDOW_MOVABLE))
    {
        struct zr_panel sub;
        static float ratio[] = {0.25f, ZR_UNDEFINED};

        /* output path directory selector in the menubar */
        zr_menubar_begin(ctx);
        {
            char *d = browser->directory;
            char *begin = d + 1;
            zr_layout_row_dynamic(ctx, 25, 6);
            zr_push_property(ctx, ZR_PROPERTY_ITEM_SPACING, zr_vec2(0, 0));
            while (*d++) {
                if (*d == '/') {
                    *d = '\0';
                    if (zr_button_text(ctx, begin, ZR_BUTTON_DEFAULT)) {
                        *d++ = '/'; *d = '\0';
                        file_browser_reload_directory_content(browser, browser->directory);
                        break;
                    }
                    *d = '/';
                    begin = d + 1;
                }
            }
            zr_pop_property(ctx);
        }
        zr_menubar_end(ctx);

        /* window layout */
        total_space = zr_window_get_content_region(ctx);
        zr_layout_row(ctx, ZR_DYNAMIC, total_space.h, 2, ratio);
        zr_group_begin(ctx, &sub, "Special", ZR_WINDOW_NO_SCROLLBAR);
        {
            struct zr_image home = icons->home;
            struct zr_image desktop = icons->desktop;
            struct zr_image computer = icons->computer;

            zr_layout_row_dynamic(ctx, 40, 1);
            zr_push_property(ctx, ZR_PROPERTY_ITEM_SPACING, zr_vec2(0, 0));
            if (zr_button_text_image(ctx, home, "home", ZR_TEXT_CENTERED, ZR_BUTTON_DEFAULT))
                file_browser_reload_directory_content(browser, browser->home);
            if (zr_button_text_image(ctx,desktop,"desktop",ZR_TEXT_CENTERED, ZR_BUTTON_DEFAULT))
                file_browser_reload_directory_content(browser, browser->desktop);
            if (zr_button_text_image(ctx,computer,"computer",ZR_TEXT_CENTERED,ZR_BUTTON_DEFAULT))
                file_browser_reload_directory_content(browser, "/");
            zr_pop_property(ctx);
            zr_group_end(ctx);
        }

        /* output directory content window */
        zr_group_begin(ctx, &sub, "Content", 0);
        {
            int index = -1;
            size_t i = 0, j = 0, k = 0;
            size_t rows = 0, cols = 0;
            size_t count = browser->dir_count + browser->file_count;

            cols = 4;
            rows = count / cols;
            for (i = 0; i <= rows; i += 1) {
                {size_t n = j + cols;
                zr_layout_row_dynamic(ctx, 135, (int)cols);
                for (; j < count && j < n; ++j) {
                    /* draw one row of icons */
                    if (j < browser->dir_count) {
                        /* draw and execute directory buttons */
                        if (zr_button_image(ctx,icons->directory,ZR_BUTTON_DEFAULT))
                            index = (int)j;
                    } else {
                        /* draw and execute files buttons */
                        struct zr_image *icon;
                        size_t fileIndex = ((size_t)j - browser->dir_count);
                        icon = media_icon_for_file(media,browser->files[fileIndex]);
                        if (zr_button_image(ctx, *icon, ZR_BUTTON_DEFAULT)) {
                            strncpy(browser->file, browser->directory, MAX_PATH_LEN);
                            n = strlen(browser->file);
                            strncpy(browser->file + n, browser->files[fileIndex], MAX_PATH_LEN - n);
                            ret = 1;
                        }
                    }
                }}
                {size_t n = k + cols;
                zr_layout_row_dynamic(ctx, 20, (int)cols);
                for (; k < count && k < n; k++) {
                    /* draw one row of labels */
                    if (k < browser->dir_count) {
                        zr_label(ctx, browser->directories[k], ZR_TEXT_CENTERED);
                    } else {
                        size_t t = k-browser->dir_count;
                        zr_label(ctx,browser->files[t],ZR_TEXT_CENTERED);
                    }
                }}
            }

            if (index != -1) {
                size_t n = strlen(browser->directory);
                strncpy(browser->directory + n, browser->directories[index], MAX_PATH_LEN - n);
                n = strlen(browser->directory);
                if (n < MAX_PATH_LEN - 1) {
                    browser->directory[n] = '/';
                    browser->directory[n+1] = '\0';
                }
                file_browser_reload_directory_content(browser, browser->directory);
            }
            zr_group_end(ctx);
        }
    }
    zr_end(ctx);
    return ret;
}
#endif
#endif

/* ===============================================================
 *
 *                          NODE EDITOR
 *
 * ===============================================================*/
struct node {
    int ID;
    char name[32];
    struct zr_rect bounds;
    float value;
    struct zr_color color;
    int input_count;
    int output_count;
    struct node *next;
    struct node *prev;
};

struct node_link {
    int input_id;
    int input_slot;
    int output_id;
    int output_slot;
    struct zr_vec2 in;
    struct zr_vec2 out;
};

struct node_linking {
    int active;
    struct node *node;
    int input_id;
    int input_slot;
};

struct node_editor {
    struct node node_buf[32];
    struct node_link links[64];
    struct node *begin;
    struct node *end;
    int node_count;
    int link_count;
    struct zr_rect bounds;
    struct node *selected;
    int show_grid;
    struct zr_vec2 scrolling;
    struct node_linking linking;
};

static void
node_editor_push(struct node_editor *editor, struct node *node)
{
    if (!editor->begin) {
        node->next = NULL;
        node->prev = NULL;
        editor->begin = node;
        editor->end = node;
    } else {
        node->prev = editor->end;
        if (editor->end)
            editor->end->next = node;
        node->next = NULL;
        editor->end = node;
    }
}

static void
node_editor_pop(struct node_editor *editor, struct node *node)
{
    if (node->next)
        node->next->prev = node->prev;
    if (node->prev)
        node->prev->next = node->next;
    if (editor->end == node)
        editor->end = node->prev;
    if (editor->begin == node)
        editor->begin = node->next;
    node->next = NULL;
    node->prev = NULL;
}

static struct node*
node_editor_find(struct node_editor *editor, int ID)
{
    struct node *iter = editor->begin;
    while (iter) {
        if (iter->ID == ID)
            return iter;
        iter = iter->next;
    }
    return NULL;
}

static void
node_editor_add(struct node_editor *editor, const char *name, struct zr_rect bounds,
    struct zr_color col, int in_count, int out_count)
{
    static int IDs = 0;
    struct node *node;
    assert((zr_size)editor->node_count < LEN(editor->node_buf));
    node = &editor->node_buf[editor->node_count++];
    node->ID = IDs++;
    node->value = 0;
    node->color = zr_rgb(255, 0, 0);
    node->input_count = in_count;
    node->output_count = out_count;
    node->color = col;
    node->bounds = bounds;
    strcpy(node->name, name);
    node_editor_push(editor, node);
}

static void
node_editor_link(struct node_editor *editor, int in_id, int in_slot,
    int out_id, int out_slot)
{
    struct node_link *link;
    assert((zr_size)editor->link_count < LEN(editor->links));
    link = &editor->links[editor->link_count++];
    link->input_id = in_id;
    link->input_slot = in_slot;
    link->output_id = out_id;
    link->output_slot = out_slot;
}

static void
node_editor_demo(struct zr_context *ctx, struct node_editor *nodedit)
{
    int n = 0;
    struct zr_rect total_space;
    const struct zr_input *in = &ctx->input;
    struct zr_command_buffer *canvas;
    struct node *updated = 0;
    struct zr_panel layout;

    /* This is a simple node editor just to show a simple implementation and that
     * it is possible to achieve with this library. While all nodes inside this
     * example use a simple color modifier as content you could change them
     * to have your custom content depending on the node time.
     * Biggest difference to most usual implementation is that this example does
     * not has connectors on the right position of the proprety that it links.
     * This is mainly done out of lazyness and could be implemented as well but
     * requires calculating the position of all rows and add connectors.
     * In addition adding and removing nodes is quite limited at the
     * moment since it is based on a simple array. If this is to be converted
     * into something more serious it is probably best to extend it.*/

    if (zr_begin(ctx, &layout, "Node Editor", zr_rect(50, 50, 650, 650),
        ZR_WINDOW_BORDER|ZR_WINDOW_NO_SCROLLBAR|ZR_WINDOW_CLOSABLE|ZR_WINDOW_MOVABLE))
    {
        /* allocate complete window space */
        canvas = zr_window_get_canvas(ctx);
        total_space = zr_window_get_content_region(ctx);
        zr_layout_space_begin(ctx, ZR_STATIC, total_space.h, nodedit->node_count);
        {
            struct zr_panel node, menu;
            struct node *it = nodedit->begin;
            struct zr_rect size = zr_layout_space_bounds(ctx);

            if (nodedit->show_grid) {
                /* display grid */
                float x, y;
                const float grid_size = 32.0f;
                const struct zr_color grid_color = zr_rgb(50, 50, 50);
                for (x = (float)fmod(size.x - nodedit->scrolling.x, grid_size); x < size.w; x += grid_size)
                    zr_draw_line(canvas, x+size.x, size.y, x+size.x, size.y+size.h, grid_color);
                for (y = (float)fmod(size.y - nodedit->scrolling.y, grid_size); y < size.h; y += grid_size)
                    zr_draw_line(canvas, size.x, y+size.y, size.x+size.w, y+size.y, grid_color);
            }

            /* execute each node as a moveable group */
            while (it) {
                /* calculate scrolled node window position and size */
                zr_layout_space_push(ctx, zr_rect(it->bounds.x - nodedit->scrolling.x,
                    it->bounds.y - nodedit->scrolling.y, it->bounds.w, it->bounds.h));

                /* execute node window */
                if (zr_group_begin(ctx, &node, it->name, ZR_WINDOW_MOVABLE|ZR_WINDOW_NO_SCROLLBAR|ZR_WINDOW_BORDER|ZR_WINDOW_TITLE))
                {
                    /* always have last selected node on top */
                    if (zr_input_mouse_clicked(in, ZR_BUTTON_LEFT, node.bounds) &&
                        (!(it->prev && zr_input_mouse_clicked(in, ZR_BUTTON_LEFT,
                        zr_layout_space_rect_to_screen(ctx, node.bounds)))) &&
                        nodedit->end != it)
                    {
                        updated = it;
                    }

                    /* ================= NODE CONTENT =====================*/
                    zr_layout_row_dynamic(ctx, 25, 1);
                    zr_button_color(ctx, it->color, ZR_BUTTON_DEFAULT);
                    it->color.r = (zr_byte)zr_propertyi(ctx, "#R:", 0, it->color.r, 255, 1,1);
                    it->color.g = (zr_byte)zr_propertyi(ctx, "#G:", 0, it->color.g, 255, 1,1);
                    it->color.b = (zr_byte)zr_propertyi(ctx, "#B:", 0, it->color.b, 255, 1,1);
                    it->color.a = (zr_byte)zr_propertyi(ctx, "#A:", 0, it->color.a, 255, 1,1);
                    /* ====================================================*/
                    zr_group_end(ctx);
                }
                {
                    /* node connector and linking */
                    float space;
                    struct zr_rect bounds;
                    bounds = zr_layout_space_rect_to_local(ctx, node.bounds);
                    bounds.x += nodedit->scrolling.x;
                    bounds.y += nodedit->scrolling.y;
                    it->bounds = bounds;

                    /* output connector */
                    space = node.bounds.h / (float)((it->output_count) + 1);
                    for (n = 0; n < it->output_count; ++n) {
                        struct zr_rect circle;
                        circle.x = node.bounds.x + node.bounds.w-4;
                        circle.y = node.bounds.y + space * (float)(n+1);
                        circle.w = 8; circle.h = 8;
                        zr_draw_circle(canvas, circle, zr_rgb(100, 100, 100));

                        /* start linking process */
                        if (zr_input_has_mouse_click_down_in_rect(in, ZR_BUTTON_LEFT, circle, zr_true)) {
                            nodedit->linking.active = zr_true;
                            nodedit->linking.node = it;
                            nodedit->linking.input_id = it->ID;
                            nodedit->linking.input_slot = n;
                        }

                        /* draw curve from linked node slot to mouse position */
                        if (nodedit->linking.active && nodedit->linking.node == it &&
                            nodedit->linking.input_slot == n) {
                            struct zr_vec2 l0 = zr_vec2(circle.x + 3, circle.y + 3);
                            struct zr_vec2 l1 = in->mouse.pos;
                            zr_draw_curve(canvas, l0.x, l0.y, l0.x + 50.0f, l0.y,
                                l1.x - 50.0f, l1.y, l1.x, l1.y, zr_rgb(100, 100, 100));
                        }
                    }

                    /* input connector */
                    space = node.bounds.h / (float)((it->input_count) + 1);
                    for (n = 0; n < it->input_count; ++n) {
                        struct zr_rect circle;
                        circle.x = node.bounds.x-4;
                        circle.y = node.bounds.y + space * (float)(n+1);
                        circle.w = 8; circle.h = 8;
                        zr_draw_circle(canvas, circle, zr_rgb(100, 100, 100));
                        if (zr_input_is_mouse_released(in, ZR_BUTTON_LEFT) &&
                            zr_input_is_mouse_hovering_rect(in, circle) &&
                            nodedit->linking.active && nodedit->linking.node != it) {
                            nodedit->linking.active = zr_false;
                            node_editor_link(nodedit, nodedit->linking.input_id,
                                nodedit->linking.input_slot, it->ID, n);
                        }
                    }
                }
                it = it->next;
            }

            /* reset linking connection */
            if (nodedit->linking.active && zr_input_is_mouse_released(in, ZR_BUTTON_LEFT)) {
                nodedit->linking.active = zr_false;
                nodedit->linking.node = NULL;
                fprintf(stdout, "linking failed\n");
            }

            /* draw each link */
            for (n = 0; n < nodedit->link_count; ++n) {
                struct node_link *link = &nodedit->links[n];
                struct node *ni = node_editor_find(nodedit, link->input_id);
                struct node *no = node_editor_find(nodedit, link->output_id);
                float spacei = node.bounds.h / (float)((ni->output_count) + 1);
                float spaceo = node.bounds.h / (float)((no->input_count) + 1);
                struct zr_vec2 l0 = zr_layout_space_to_screen(ctx,
                    zr_vec2(ni->bounds.x + ni->bounds.w, 3.0f + ni->bounds.y + spacei * (float)(link->input_slot+1)));
                struct zr_vec2 l1 = zr_layout_space_to_screen(ctx,
                    zr_vec2(no->bounds.x, 3.0f + no->bounds.y + spaceo * (float)(link->output_slot+1)));

                l0.x -= nodedit->scrolling.x;
                l0.y -= nodedit->scrolling.y;
                l1.x -= nodedit->scrolling.x;
                l1.y -= nodedit->scrolling.y;
                zr_draw_curve(canvas, l0.x, l0.y, l0.x + 50.0f, l0.y,
                    l1.x - 50.0f, l1.y, l1.x, l1.y, zr_rgb(100, 100, 100));
            }

            if (updated) {
                /* reshuffle nodes to have least recently selected node on top */
                node_editor_pop(nodedit, updated);
                node_editor_push(nodedit, updated);
            }

            /* node selection */
            if (zr_input_mouse_clicked(in, ZR_BUTTON_LEFT, zr_layout_space_bounds(ctx))) {
                it = nodedit->begin;
                nodedit->selected = NULL;
                nodedit->bounds = zr_rect(in->mouse.pos.x, in->mouse.pos.y, 100, 200);
                while (it) {
                    struct zr_rect b = zr_layout_space_rect_to_screen(ctx, it->bounds);
                    b.x -= nodedit->scrolling.x;
                    b.y -= nodedit->scrolling.y;
                    if (zr_input_is_mouse_hovering_rect(in, b))
                        nodedit->selected = it;
                    it = it->next;
                }
            }

            /* contextual menu */
            if (zr_contextual_begin(ctx, &menu, 0, zr_vec2(100, 220), zr_window_get_bounds(ctx))) {
                const char *grid_option[] = {"Show Grid", "Hide Grid"};
                zr_layout_row_dynamic(ctx, 25, 1);
                if (zr_contextual_item(ctx, "New", ZR_TEXT_CENTERED))
                    node_editor_add(nodedit, "New", zr_rect(400, 260, 180, 220),
                            zr_rgb(255, 255, 255), 1, 2);
                if (zr_contextual_item(ctx, grid_option[nodedit->show_grid],ZR_TEXT_CENTERED))
                    nodedit->show_grid = !nodedit->show_grid;
                zr_contextual_end(ctx);
            }
        }
        zr_layout_space_end(ctx);

        /* window content scrolling */
        if (zr_input_is_mouse_hovering_rect(in, zr_window_get_bounds(ctx)) &&
            zr_input_is_mouse_down(in, ZR_BUTTON_MIDDLE)) {
            nodedit->scrolling.x += in->mouse.delta.x;
            nodedit->scrolling.y += in->mouse.delta.y;
        }
    }
    zr_end(ctx);
}

static void
node_editor_init(struct node_editor *editor)
{
    memset(editor, 0, sizeof(*editor));
    editor->begin = NULL;
    editor->end = NULL;
    node_editor_add(editor, "Source", zr_rect(40, 10, 180, 220), zr_rgb(255, 0, 0), 0, 1);
    node_editor_add(editor, "Source", zr_rect(40, 260, 180, 220), zr_rgb(0, 255, 0), 0, 1);
    node_editor_add(editor, "Combine", zr_rect(400, 100, 180, 220), zr_rgb(0,0,255), 2, 2);
    node_editor_link(editor, 0, 0, 2, 0);
    node_editor_link(editor, 1, 0, 2, 1);
    editor->show_grid = zr_true;
}

/* ===============================================================
 *
 *                          DEMO ENTRY
 *
 * ===============================================================*/
static int
run_demo(struct demo *gui)
{
    int ret = 1;
    static int init = 0;
    static struct node_editor nodedit;
    static struct file_browser filex;
    struct zr_context *ctx = &gui->ctx;

    if (!init) {
        gui->show_demo = 0;
        gui->show_node = 0;
        gui->show_simple = 0;

        gui->show_grid = 0;
        gui->show_basic = 0;
        gui->show_button = 0;

        memset(&nodedit, 0, sizeof(nodedit));
        node_editor_init(&nodedit);
#ifndef DEMO_DO_NOT_DRAW_IMAGES
#ifdef __unix__
        file_browser_init(&filex, &gui->icons);
#endif
#endif
        init = 1;
    }

    ret = control_window(ctx, gui);
    if (gui->show_demo)
        demo_window(gui, ctx);
    if (gui->show_simple)
        simple_window(ctx);
    if (gui->show_node)
        node_editor_demo(ctx, &nodedit);

#ifndef DEMO_DO_NOT_DRAW_IMAGES
#ifdef __unix__
    if (gui->show_filex)
        file_browser_run(&filex, ctx);
    if (!ret) file_browser_free(&filex);
#endif
    if (gui->show_grid)
        grid_demo(ctx);
    if (gui->show_button)
        button_demo(ctx, &gui->icons);
    if (gui->show_basic)
        basic_demo(ctx, &gui->icons);
#endif
    zr_buffer_info(&gui->status, &gui->ctx.memory);
    return ret;
}

