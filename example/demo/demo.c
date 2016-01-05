/*
    Copyright (c) 2016 Micha Mettke

    This software is provided 'as-is', without any express or implied
    warranty.  In no event will the authors be held liable for any damages
    arising from the use of this software.

    Permission is granted to anyone to use this software for any purpose,
    including commercial applications, and to alter it and redistribute it
    freely, subject to the following restrictions:

    1.  The origin of this software must not be misrepresented; you must not
        claim that you wrote the original software. If you use this software
        in a product, an acknowledgment in the product documentation would be
        appreciated but is not required.
    2.  Altered source versions must be plainly marked as such, and must not be
        misrepresented as being the original software.
    3.  This notice may not be removed or altered from any source distribution.
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#ifdef _WIN32
    #include <windows.h>
#endif

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#define NANOVG_GLES2_IMPLEMENTATION
#include "../../demo/nanovg/dep/nanovg.h"
#include "../../demo/nanovg/dep/nanovg_gl.h"
#include "../../demo/nanovg/dep/nanovg_gl_utils.h"
#include "../../demo/nanovg/dep/nanovg.c"

/* macros */
#define MAX_BUFFER  64
#define MAX_MEMORY  (32 * 1024)
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define DTIME       40
#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#define MAX(a,b)    ((a) < (b) ? (b) : (a))
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#define LEN(a)      (sizeof(a)/sizeof(a)[0])
#define UNUSED(a)   ((void)(a))

#include "../../zahnrad.h"

struct icons {
    int unchecked;
    int checked;
    int rocket;
    int cloud;
    int pen;
    int play;
    int pause;
    int stop;
    int prev;
    int next;
    int tools;
    int directory;
    int copy;
    int convert;
    int delete;
    int edit;
    int images[9];
    int menu[6];
};

static void
die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputs("\n", stderr);
    exit(EXIT_FAILURE);
}

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

static int
ui_piemenu(struct zr_context *ctx,
    struct zr_vec2 pos, float radius, int *icons, int item_count)
{
    int ret = -1;
    struct zr_rect total_space;
    struct zr_layout popup;
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

            struct zr_vec2 center = zr_vec2(bounds.x + bounds.w / 2.0f, center.y = bounds.y + bounds.h / 2.0f);
            struct zr_vec2 drag = zr_vec2(in->mouse.pos.x - center.x, in->mouse.pos.y - center.y);
            float angle = (float)atan2(drag.y, drag.x);
            if (angle < -0.0f) angle += 2.0f * 3.141592654f;
            active_item = (int)(angle/step);

            for (i = 0; i < item_count; ++i) {
                struct zr_image img;
                struct zr_rect content;
                float rx, ry, dx, dy, a;
                zr_draw_arc(out, center.x, center.y, (bounds.w/2.0f),
                    a_min, a_max, (active_item == i) ? zr_rgb(45,100,255) : zr_rgb(75,75,75));

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
                img = zr_image_id(icons[i]);
                zr_draw_image(out, content, &img);
                a_min = a_max; a_max += step;
            }
        }
        {
            /* inner circle */
            struct zr_rect inner;
            struct zr_image img;
            inner.x = bounds.x + bounds.w/2 - bounds.w/4;
            inner.y = bounds.y + bounds.h/2 - bounds.h/4;
            inner.w = bounds.w/2; inner.h = bounds.h/2;
            zr_draw_circle(out, inner, zr_rgb(45,45,45));

            /* active icon content */
            bounds.w = inner.w / 2.0f;
            bounds.h = inner.h / 2.0f;
            bounds.x = inner.x + inner.w/2 - bounds.w/2;
            bounds.y = inner.y + inner.h/2 - bounds.h/2;
            img = zr_image_id(icons[active_item]);
            zr_draw_image(out, bounds, &img);
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

static void
button_demo(struct zr_context *ctx, struct icons *img)
{
    struct zr_layout layout;
    struct zr_layout menu;
    static int option = 1;
    static int toggle0 = 1;
    static int toggle1 = 0;
    static int toggle2 = 1;
    int clicked = 0;

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
        if (zr_menu_icon_begin(ctx, &menu, "Music", zr_image_id(img->play), 120))
        {
            /* settings */
            zr_layout_row_dynamic(ctx, 25, 1);
            zr_menu_item_icon(ctx, zr_image_id(img->play), "Play", ZR_TEXT_RIGHT);
            zr_menu_item_icon(ctx, zr_image_id(img->stop), "Stop", ZR_TEXT_RIGHT);
            zr_menu_item_icon(ctx, zr_image_id(img->pause), "Pause", ZR_TEXT_RIGHT);
            zr_menu_item_icon(ctx, zr_image_id(img->next), "Next", ZR_TEXT_RIGHT);
            zr_menu_item_icon(ctx, zr_image_id(img->prev), "Prev", ZR_TEXT_RIGHT);
            zr_menu_end(ctx);
        }
        zr_button_image(ctx, zr_image_id(img->tools), ZR_BUTTON_DEFAULT);
        zr_button_image(ctx, zr_image_id(img->cloud), ZR_BUTTON_DEFAULT);
        zr_button_image(ctx, zr_image_id(img->pen), ZR_BUTTON_DEFAULT);
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
    if (zr_button_text_image(ctx, zr_image_id(img->rocket),
        "Styled", ZR_TEXT_CENTERED, ZR_BUTTON_DEFAULT))
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
    if (zr_button_text_image(ctx, (toggle0) ? zr_image_id(img->checked):
        zr_image_id(img->unchecked), "Toggle", ZR_TEXT_LEFT, ZR_BUTTON_DEFAULT))
        toggle0 = !toggle0;

    ui_widget(ctx, 35, 22);
    if (zr_button_text_image(ctx, (toggle1) ? zr_image_id(img->checked):
        zr_image_id(img->unchecked), "Toggle", ZR_TEXT_LEFT, ZR_BUTTON_DEFAULT))
        toggle1 = !toggle1;

    ui_widget(ctx, 35, 22);
    if (zr_button_text_image(ctx, (toggle2) ? zr_image_id(img->checked):
        zr_image_id(img->unchecked), "Toggle", ZR_TEXT_LEFT, ZR_BUTTON_DEFAULT))
        toggle2 = !toggle2;

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
    clicked = zr_input_mouse_clicked(&ctx->input, ZR_BUTTON_RIGHT, zr_window_get_bounds(ctx));
    if (zr_contextual_begin(ctx, &menu, ZR_WINDOW_NO_SCROLLBAR, zr_vec2(120, 200), clicked)) {
        ctx->style.font.height = 18;
        zr_layout_row_dynamic(ctx, 25, 1);
        if (zr_contextual_item_icon(ctx, zr_image_id(img->copy), "Clone", ZR_TEXT_RIGHT))
            fprintf(stdout, "pressed clone!\n");
        if (zr_contextual_item_icon(ctx, zr_image_id(img->delete), "Delete", ZR_TEXT_RIGHT))
            fprintf(stdout, "pressed delete!\n");
        if (zr_contextual_item_icon(ctx, zr_image_id(img->convert), "Convert", ZR_TEXT_RIGHT))
            fprintf(stdout, "pressed convert!\n");
        if (zr_contextual_item_icon(ctx, zr_image_id(img->edit), "Edit", ZR_TEXT_RIGHT))
            fprintf(stdout, "pressed edit!\n");
        zr_contextual_end(ctx);
    }
    zr_end(ctx);
}

static void
basic_demo(struct zr_context *ctx, struct icons *img)
{
    static int image_active;
    static int check0 = 1;
    static int check1 = 0;
    static int slider = 30;
    static size_t prog = 80;
    static int combo_active = 0;
    static int combo2_active = 0;
    static int selected_item = 0;
    static int selected_image = 2;
    static int selected_icon = 0;
    static const char *items[] = {"Item 0","item 1","item 2"};
    static int piemenu_active = 0;

    int i = 0;
    struct zr_layout layout;
    struct zr_layout combo;
    ctx->style.font.height = 20;
    zr_begin(ctx, &layout, "Basic Demo", zr_rect(320, 50, 275, 610),
        ZR_WINDOW_BORDER|ZR_WINDOW_MOVABLE|ZR_WINDOW_BORDER_HEADER|ZR_WINDOW_TITLE);

    /*------------------------------------------------
     *                  POPUP BUTTON
     *------------------------------------------------*/
    ui_header(ctx, "Popup & Scrollbar & Images");
    ui_widget(ctx, 35, 22);
    if (zr_button_text_image(ctx, zr_image_id(img->directory),
        "Images", ZR_TEXT_CENTERED, ZR_BUTTON_DEFAULT))
        image_active = !image_active;

    /*------------------------------------------------
     *                  SELECTED IMAGE
     *------------------------------------------------*/
    ui_header(ctx, "Selected Image");
    ui_widget_centered(ctx, 100, 22);
    zr_image(ctx, zr_image_id(img->images[selected_image]));

    /*------------------------------------------------
     *                  IMAGE POPUP
     *------------------------------------------------*/
    if (image_active) {
        struct zr_layout popup;
        if (zr_popup_begin(ctx, &popup, ZR_POPUP_STATIC, "Image Popup", 0, zr_rect(265, 0, 320, 220))) {
            zr_layout_row_static(ctx, 82, 82, 3);
            for (i = 0; i < 9; ++i) {
                if (zr_button_image(ctx, zr_image_id(img->images[i]), ZR_BUTTON_DEFAULT)) {
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
    if (zr_combo_begin_text(ctx, &combo, "items", items[selected_item], 200)) {
        zr_layout_row_dynamic(ctx, 35, 1);
        for (i = 0; i < 3; ++i)
            if (zr_combo_item(ctx, items[i], ZR_TEXT_LEFT))
                selected_item = i;
        zr_combo_end(ctx);
    }

    ui_widget(ctx, 40, 22);
    if (zr_combo_begin_icon(ctx, &combo, "pictures", items[selected_icon], zr_image_id(img->images[selected_icon]), 200)) {
        zr_layout_row_dynamic(ctx, 35, 1);
        for (i = 0; i < 3; ++i)
            if (zr_combo_item_icon(ctx, zr_image_id(img->images[i]), items[i], ZR_TEXT_RIGHT))
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
    zr_end(ctx);
}

static void
grid_demo(struct zr_context *ctx)
{
    static char text[3][64];
    static size_t text_len[3];
    static const char *items[] = {"Item 0","item 1","item 2"};
    static int selected_item = 0;
    static int check = 1;

    int i;
    struct zr_layout layout;
    struct zr_layout combo;

    ctx->style.font.height = 20;
    zr_begin(ctx, &layout, "Grid Demo", zr_rect(600, 350, 275, 250),
        ZR_WINDOW_TITLE|ZR_WINDOW_BORDER|ZR_WINDOW_MOVABLE|
        ZR_WINDOW_BORDER_HEADER|ZR_WINDOW_NO_SCROLLBAR);

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

    if (zr_combo_begin_text(ctx, &combo, "combo", items[selected_item], 200)) {
        zr_layout_row_dynamic(ctx, 30, 1);
        for (i = 0; i < 3; ++i)
            if (zr_combo_item(ctx, items[i], ZR_TEXT_LEFT))
                selected_item = i;
        zr_combo_end(ctx);
    }
    zr_end(ctx);
}

/* =================================================================
 *
 *                          APP
 *
 * =================================================================*/
static zr_size
font_get_width(zr_handle handle, float height, const char *text, zr_size len)
{
    zr_size width;
    float bounds[4];
    NVGcontext *ctx = (NVGcontext*)handle.ptr;
    nvgFontSize(ctx, (float)height);
    nvgTextBounds(ctx, 0, 0, text, &text[len], bounds);
    width = (zr_size)(bounds[2] - bounds[0]);
    return width;
}

static void
draw(NVGcontext *nvg, struct zr_context *ctx, int width, int height)
{
    const struct zr_command *cmd;
    glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_TEXTURE_2D);

    nvgBeginFrame(nvg, width, height, ((float)width/(float)height));
    zr_foreach(cmd, ctx) {
        switch (cmd->type) {
        case ZR_COMMAND_NOP: break;
        case ZR_COMMAND_SCISSOR: {
            const struct zr_command_scissor *s = zr_command(scissor, cmd);
            nvgScissor(nvg, s->x, s->y, s->w, s->h);
        } break;
        case ZR_COMMAND_LINE: {
            const struct zr_command_line *l = zr_command(line, cmd);
            nvgBeginPath(nvg);
            nvgMoveTo(nvg, l->begin.x, l->begin.y);
            nvgLineTo(nvg, l->end.x, l->end.y);
            nvgStrokeColor(nvg, nvgRGBA(l->color.r, l->color.g, l->color.b, l->color.a));
            nvgStroke(nvg);
        } break;
        case ZR_COMMAND_CURVE: {
            const struct zr_command_curve *q = zr_command(curve, cmd);
            nvgBeginPath(nvg);
            nvgMoveTo(nvg, q->begin.x, q->begin.y);
            nvgBezierTo(nvg, q->ctrl[0].x, q->ctrl[0].y, q->ctrl[1].x, q->ctrl[1].y, q->end.x, q->end.y);
            nvgStrokeColor(nvg, nvgRGBA(q->color.r, q->color.g, q->color.b, q->color.a));
            nvgStroke(nvg);
        } break;
        case ZR_COMMAND_RECT: {
            const struct zr_command_rect *r = zr_command(rect, cmd);
            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, r->x, r->y, r->w, r->h, r->rounding);
            nvgFillColor(nvg, nvgRGBA(r->color.r, r->color.g, r->color.b, r->color.a));
            nvgFill(nvg);
        } break;
        case ZR_COMMAND_CIRCLE: {
            const struct zr_command_circle *c = zr_command(circle, cmd);
            nvgBeginPath(nvg);
            nvgCircle(nvg, c->x + (c->w/2.0f), c->y + c->w/2.0f, c->w/2.0f);
            nvgFillColor(nvg, nvgRGBA(c->color.r, c->color.g, c->color.b, c->color.a));
            nvgFill(nvg);
        } break;
        case ZR_COMMAND_ARC: {
            const struct zr_command_arc *c = zr_command(arc, cmd);
            nvgBeginPath(nvg);
            nvgMoveTo(nvg, c->cx, c->cy);
            nvgArc(nvg, c->cx, c->cy, c->r, c->a[0], c->a[1], NVG_CW);
            nvgFillColor(nvg, nvgRGBA(c->color.r, c->color.g, c->color.b, c->color.a));
            nvgFill(nvg);
        } break;
        case ZR_COMMAND_TRIANGLE: {
            const struct zr_command_triangle *t = zr_command(triangle, cmd);
            nvgBeginPath(nvg);
            nvgMoveTo(nvg, t->a.x, t->a.y);
            nvgLineTo(nvg, t->b.x, t->b.y);
            nvgLineTo(nvg, t->c.x, t->c.y);
            nvgLineTo(nvg, t->a.x, t->a.y);
            nvgFillColor(nvg, nvgRGBA(t->color.r, t->color.g, t->color.b, t->color.a));
            nvgFill(nvg);
        } break;
        case ZR_COMMAND_TEXT: {
            const struct zr_command_text *t = zr_command(text, cmd);
            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, t->x, t->y, t->w, t->h, 0);
            nvgFillColor(nvg, nvgRGBA(t->background.r, t->background.g,
                t->background.b, t->background.a));
            nvgFill(nvg);

            nvgBeginPath(nvg);
            nvgFillColor(nvg, nvgRGBA(t->foreground.r, t->foreground.g,
                t->foreground.b, t->foreground.a));
            nvgFontSize(nvg, (float)t->height);
            nvgTextAlign(nvg, NVG_ALIGN_MIDDLE);
            nvgText(nvg, t->x, t->y + t->h * 0.5f, t->string, &t->string[t->length]);
            nvgFill(nvg);
        } break;
        case ZR_COMMAND_IMAGE: {
            const struct zr_command_image *i = zr_command(image, cmd);
            NVGpaint imgpaint;
            imgpaint = nvgImagePattern(nvg, i->x, i->y, i->w, i->h, 0, i->img.handle.id, 1.0f);
            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, i->x, i->y, i->w, i->h, 0);
            nvgFillPaint(nvg, imgpaint);
            nvgFill(nvg);
        } break;
        default: break;
        }
    }
    zr_clear(ctx);

    nvgResetScissor(nvg);
    nvgEndFrame(nvg);
    glPopAttrib();
}


static void
key(struct zr_context *ctx, SDL_Event *evt, int down)
{
    const Uint8* state = SDL_GetKeyboardState(NULL);
    SDL_Keycode sym = evt->key.keysym.sym;
    if (sym == SDLK_RSHIFT || sym == SDLK_LSHIFT)
        zr_input_key(ctx, ZR_KEY_SHIFT, down);
    else if (sym == SDLK_DELETE)
        zr_input_key(ctx, ZR_KEY_DEL, down);
    else if (sym == SDLK_RETURN)
        zr_input_key(ctx, ZR_KEY_ENTER, down);
    else if (sym == SDLK_BACKSPACE)
        zr_input_key(ctx, ZR_KEY_BACKSPACE, down);
    else if (sym == SDLK_LEFT)
        zr_input_key(ctx, ZR_KEY_LEFT, down);
    else if (sym == SDLK_RIGHT)
        zr_input_key(ctx, ZR_KEY_RIGHT, down);
    else if (sym == SDLK_c)
        zr_input_key(ctx, ZR_KEY_COPY, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_v)
        zr_input_key(ctx, ZR_KEY_PASTE, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_x)
        zr_input_key(ctx, ZR_KEY_CUT, down && state[SDL_SCANCODE_LCTRL]);
}

static void
motion(struct zr_context *ctx, SDL_Event *evt)
{
    const int x = evt->motion.x;
    const int y = evt->motion.y;
    zr_input_motion(ctx, x, y);
}

static void
btn(struct zr_context *ctx, SDL_Event *evt, int down)
{
    const int x = evt->button.x;
    const int y = evt->button.y;
    if (evt->button.button == SDL_BUTTON_LEFT)
        zr_input_button(ctx, ZR_BUTTON_LEFT, x, y, down);
    else if (evt->button.button == SDL_BUTTON_RIGHT)
        zr_input_button(ctx, ZR_BUTTON_RIGHT, x, y, down);
    else if (evt->button.button == SDL_BUTTON_MIDDLE)
        zr_input_button(ctx, ZR_BUTTON_MIDDLE, x, y, down);
}

static void
text(struct zr_context *ctx, SDL_Event *evt)
{
    zr_glyph glyph;
    memcpy(glyph, evt->text.text, ZR_UTF_SIZE);
    zr_input_glyph(ctx, glyph);
}

int
main(int argc, char *argv[])
{
    /* Platform */
    int x, y, width, height;
    const char *font_path;
    zr_size font_height;
    SDL_SysWMinfo info;
    SDL_Window *win;
    SDL_GLContext glContext;
    NVGcontext *vg = NULL;
    int running = zr_true;
    unsigned int started;
    unsigned int dt;
    int i = 0;
    int poll = 1;

    /* GUI */
    struct icons icons;
    struct zr_context ctx;
    void *memory;
    if (argc < 2) {
        fprintf(stdout,"Missing TTF Font file argument: demo <path>\n");
        exit(EXIT_FAILURE);
    }
    font_path = argv[1];
    font_height = 12;

    /* SDL */
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_EVENTS);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    win = SDL_CreateWindow("Demo",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN);
    glContext = SDL_GL_CreateContext(win);
    SDL_GetWindowSize(win, &width, &height);
    SDL_GetWindowPosition(win, &x, &y);
    SDL_VERSION(&info.version);
    assert(SDL_GetWindowWMInfo(win, &info));

    /* OpenGL */
    glewExperimental = 1;
    if (glewInit() != GLEW_OK)
        die("[GLEW] failed setup\n");
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    /* nanovg */
    vg = nvgCreateGLES2(NVG_ANTIALIAS);
    if (!vg) die("[NVG]: failed to init\n");
    nvgCreateFont(vg, "fixed", font_path);
    nvgFontFace(vg, "fixed");
    nvgFontSize(vg, font_height);
    nvgTextAlign(vg, NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);

    /* icons */
    icons.unchecked = nvgCreateImage(vg, "../icon/unchecked.png", 0);
    icons.checked = nvgCreateImage(vg, "../icon/checked.png", 0);
    icons.rocket = nvgCreateImage(vg, "../icon/rocket.png", 0);
    icons.cloud = nvgCreateImage(vg, "../icon/cloud.png", 0);
    icons.pen = nvgCreateImage(vg, "../icon/pen.png", 0);
    icons.play = nvgCreateImage(vg, "../icon/play.png", 0);
    icons.pause = nvgCreateImage(vg, "../icon/pause.png", 0);
    icons.stop = nvgCreateImage(vg, "../icon/stop.png", 0);
    icons.next =  nvgCreateImage(vg, "../icon/next.png", 0);
    icons.prev =  nvgCreateImage(vg, "../icon/prev.png", 0);
    icons.tools = nvgCreateImage(vg, "../icon/tools.png", 0);
    icons.directory = nvgCreateImage(vg, "../icon/directory.png", 0);
    icons.copy = nvgCreateImage(vg, "../icon/copy.png", 0);
    icons.convert = nvgCreateImage(vg, "../icon/export.png", 0);
    icons.delete = nvgCreateImage(vg, "../icon/delete.png", 0);
    icons.edit = nvgCreateImage(vg, "../icon/edit.png", 0);
    icons.menu[0] = nvgCreateImage(vg, "../icon/home.png", 0);
    icons.menu[1] = nvgCreateImage(vg, "../icon/phone.png", 0);
    icons.menu[2] = nvgCreateImage(vg, "../icon/plane.png", 0);
    icons.menu[3] = nvgCreateImage(vg, "../icon/wifi.png", 0);
    icons.menu[4] = nvgCreateImage(vg, "../icon/settings.png", 0);
    icons.menu[5] = nvgCreateImage(vg, "../icon/volume.png", 0);
    for (i = 0; i < 9; ++i) {
        char buffer[256];
        sprintf(buffer, "../images/image%d.jpg", (i+1));
        icons.images[i] = nvgCreateImage(vg, buffer, 0);
    }

    {
        /* GUI */
        struct zr_user_font font;
        memset(&ctx, 0, sizeof ctx);
        memory = malloc(MAX_MEMORY);

        font.userdata.ptr = vg;
        font.width = font_get_width;
        font.height = 20;
        zr_init_fixed(&ctx, memory, MAX_MEMORY, &font);
        ctx.style.rounding[ZR_ROUNDING_BUTTON] = 3;
    }

    while (running) {
        /* Input */
        int ret;
        SDL_Event evt;
        started = SDL_GetTicks();
        zr_input_begin(&ctx);

        if (!poll) {
            ret = SDL_WaitEvent(&evt);
            poll = 1;
        } else ret = SDL_PollEvent(&evt);

        while (ret) {
            if (evt.type == SDL_WINDOWEVENT &&
                evt.window.event == SDL_WINDOWEVENT_RESIZED)
                glViewport(0, 0, evt.window.data1, evt.window.data2);
            else if (evt.type == SDL_QUIT) goto cleanup;
            else if (evt.type == SDL_KEYUP)
                key(&ctx, &evt, zr_false);
            else if (evt.type == SDL_KEYDOWN)
                key(&ctx, &evt, zr_true);
            else if (evt.type == SDL_MOUSEBUTTONDOWN)
                btn(&ctx, &evt, zr_true);
            else if (evt.type == SDL_MOUSEBUTTONUP)
                btn(&ctx, &evt, zr_false);
            else if (evt.type == SDL_MOUSEMOTION)
                motion(&ctx, &evt);
            else if (evt.type == SDL_TEXTINPUT)
                text(&ctx, &evt);
            else if (evt.type == SDL_MOUSEWHEEL)
                zr_input_scroll(&ctx, evt.wheel.y);
            ret = SDL_PollEvent(&evt);
        }
        zr_input_end(&ctx);

        /* GUI */
        button_demo(&ctx, &icons);
        basic_demo(&ctx, &icons);
        grid_demo(&ctx);

        /* Draw */
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        SDL_GetWindowSize(win, &width, &height);
        draw(vg, &ctx, width, height);
        SDL_GL_SwapWindow(win);
        poll = ((poll+1) & 4);
    }

cleanup:
    /* Cleanup */
    nvgDeleteImage(vg, icons.unchecked);
    nvgDeleteImage(vg, icons.checked);
    nvgDeleteImage(vg, icons.rocket);
    nvgDeleteImage(vg, icons.cloud);
    nvgDeleteImage(vg, icons.pen);
    nvgDeleteImage(vg, icons.play);
    nvgDeleteImage(vg, icons.pause);
    nvgDeleteImage(vg, icons.stop);
    nvgDeleteImage(vg, icons.next);
    nvgDeleteImage(vg, icons.prev);
    nvgDeleteImage(vg, icons.tools);
    nvgDeleteImage(vg, icons.directory);
    for (i = 0; i < 9; ++i)
        nvgDeleteImage(vg, icons.images[i]);
    for (i = 0; i < 6; ++i)
        nvgDeleteImage(vg, icons.menu[i]);

    free(memory);
    nvgDeleteGLES2(vg);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
