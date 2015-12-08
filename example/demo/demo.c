/*
    Copyright (c) 2015 Micha Mettke

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
#define MAX_MEMORY  (16 * 1024)
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
};

struct gui {
    void *memory;
    struct zr_window button_demo;
    struct zr_window basic_demo;
    struct zr_window grid_demo;
    struct zr_input input;
    struct zr_command_queue queue;
    struct zr_style config;
    struct zr_user_font font;
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
ui_header(struct zr_context *layout, struct zr_style *config, const char *title)
{
    zr_style_reset_font_height(config);
    zr_style_push_font_height(config, 18);
    zr_layout_row_dynamic(layout, 20, 1);
    zr_label(layout, "Push buttons", ZR_TEXT_LEFT);
}

static void
ui_widget(struct zr_context *layout, struct zr_style *config, float height, float font_height)
{
    static const float ratio[] = {0.15f, 0.85f};
    zr_style_reset_font_height(config);
    zr_style_push_font_height(config, font_height);
    zr_layout_row(layout, ZR_DYNAMIC, height, 2, ratio);
    zr_spacing(layout, 1);
}

static void
ui_widget_centered(struct zr_context *layout, struct zr_style *config, float height, float font_height)
{
    static const float ratio[] = {0.15f, 0.50f, 0.35f};
    zr_style_reset_font_height(config);
    zr_style_push_font_height(config, font_height);
    zr_layout_row(layout, ZR_DYNAMIC, height, 3, ratio);
    zr_spacing(layout, 1);
}

static void
button_demo(struct zr_window *window, struct zr_style *config, struct icons *img)
{
    struct zr_context layout;
    struct zr_context menu;
    static int option = 1;
    static int toggle0 = 1;
    static int toggle1 = 0;
    static int toggle2 = 1;
    static int music_active = 0;
    static int contextual_active = 0;
    static struct zr_rect contextual_bounds;

    config->font.height = 20;
    zr_begin(&layout, window, "Button Demo");
    /*------------------------------------------------
     *                  MENU
     *------------------------------------------------*/
    zr_menubar_begin(&layout);
    {
        /* toolbar */
        zr_layout_row_static(&layout, 40, 40, 4);
        zr_menu_icon_begin(&layout, &menu, img->play, 120, &music_active);
        {
            /* settings */
            zr_layout_row_dynamic(&menu, 25, 1);
            if (zr_menu_item_icon(&menu, img->play, "Play", ZR_TEXT_RIGHT))
                zr_menu_close(&menu, &music_active);
            if (zr_menu_item_icon(&menu, img->stop, "Stop", ZR_TEXT_RIGHT))
                zr_menu_close(&menu, &music_active);
            if (zr_menu_item_icon(&menu, img->pause, "Pause", ZR_TEXT_RIGHT))
                zr_menu_close(&menu, &music_active);
            if (zr_menu_item_icon(&menu, img->next, "Next", ZR_TEXT_RIGHT))
                zr_menu_close(&menu, &music_active);
            if (zr_menu_item_icon(&menu, img->next, "Prev", ZR_TEXT_RIGHT))
                zr_menu_close(&menu, &music_active);
        }
        zr_menu_end(&layout, &menu);
        zr_button_image(&layout, img->tools, ZR_BUTTON_DEFAULT);
        zr_button_image(&layout, img->cloud, ZR_BUTTON_DEFAULT);
        zr_button_image(&layout, img->pen, ZR_BUTTON_DEFAULT);
    }
    zr_menubar_end(&layout);

    /*------------------------------------------------
     *                  BUTTON
     *------------------------------------------------*/
    ui_header(&layout, config, "Push buttons");
    ui_widget(&layout, config, 35, 22);
    if (zr_button_text(&layout, "Push me", ZR_BUTTON_DEFAULT))
        fprintf(stdout, "pushed!\n");

    ui_widget(&layout, config, 35, 22);
    if (zr_button_text_image(&layout, img->rocket,
        "Styled", ZR_TEXT_CENTERED, ZR_BUTTON_DEFAULT))
        fprintf(stdout, "rocket!\n");

    /*------------------------------------------------
     *                  REPEATER
     *------------------------------------------------*/
    ui_header(&layout, config, "Repeater");
    ui_widget(&layout, config, 35, 22);
    if (zr_button_text(&layout, "Press me", ZR_BUTTON_REPEATER))
        fprintf(stdout, "pressed!\n");

    /*------------------------------------------------
     *                  TOGGLE
     *------------------------------------------------*/
    ui_header(&layout, config, "Toggle buttons");
    ui_widget(&layout, config, 35, 22);
    if (zr_button_text_image(&layout, (toggle0) ? img->checked: img->unchecked,
        "Toggle", ZR_TEXT_LEFT, ZR_BUTTON_DEFAULT)) toggle0 = !toggle0;

    ui_widget(&layout, config, 35, 22);
    if (zr_button_text_image(&layout, (toggle1) ? img->checked: img->unchecked,
        "Toggle", ZR_TEXT_LEFT, ZR_BUTTON_DEFAULT)) toggle1 = !toggle1;

    ui_widget(&layout, config, 35, 22);
    if (zr_button_text_image(&layout, (toggle2) ? img->checked: img->unchecked,
        "Toggle", ZR_TEXT_LEFT, ZR_BUTTON_DEFAULT)) toggle2 = !toggle2;

    /*------------------------------------------------
     *                  RADIO
     *------------------------------------------------*/
    ui_header(&layout, config, "Radio buttons");
    ui_widget(&layout, config, 35, 22);
    if (zr_button_text_symbol(&layout, (option == 0)?ZR_SYMBOL_CIRCLE_FILLED:ZR_SYMBOL_CIRCLE,
            "Select", ZR_TEXT_LEFT, ZR_BUTTON_DEFAULT)) option = 0;
    ui_widget(&layout, config, 35, 22);
    if (zr_button_text_symbol(&layout, (option == 1)?ZR_SYMBOL_CIRCLE_FILLED:ZR_SYMBOL_CIRCLE,
            "Select", ZR_TEXT_LEFT, ZR_BUTTON_DEFAULT)) option = 1;
    ui_widget(&layout, config, 35, 22);
    if (zr_button_text_symbol(&layout, (option == 2)?ZR_SYMBOL_CIRCLE_FILLED:ZR_SYMBOL_CIRCLE,
            "Select", ZR_TEXT_LEFT, ZR_BUTTON_DEFAULT)) option = 2;

    /*------------------------------------------------
     *                  CONTEXTUAL
     *------------------------------------------------*/
    if (zr_input_mouse_clicked(zr_input(&layout), ZR_BUTTON_RIGHT, layout.bounds)) {
        const struct zr_input *in = zr_input(&layout);
        contextual_bounds = zr_rect(in->mouse.pos.x, in->mouse.pos.y, 120, 200);
        contextual_active = ZR_ACTIVE;
    }
    if (contextual_active) {
        config->font.height = 18;
        zr_contextual_begin(&layout, &menu, ZR_WINDOW_NO_SCROLLBAR, &contextual_active, contextual_bounds);
        zr_layout_row_dynamic(&menu, 25, 1);
        if (zr_contextual_item_icon(&menu, img->copy, "Clone", ZR_TEXT_RIGHT))
            fprintf(stdout, "pressed clone!\n");
        if (zr_contextual_item_icon(&menu, img->delete, "Delete", ZR_TEXT_RIGHT))
            fprintf(stdout, "pressed delete!\n");
        if (zr_contextual_item_icon(&menu, img->convert, "Convert", ZR_TEXT_RIGHT))
            fprintf(stdout, "pressed convert!\n");
        if (zr_contextual_item_icon(&menu, img->edit, "Edit", ZR_TEXT_RIGHT))
            fprintf(stdout, "pressed edit!\n");
        zr_contextual_end(&layout, &menu, &contextual_active);
    }
    zr_end(&layout, window);
}

static void
basic_demo(struct zr_window *window, struct zr_style *config, struct icons *img)
{
    static int image_active;
    static int check0 = 1;
    static int check1 = 0;
    static int slider = 30;
    static size_t prog = 80;
    static int combo_active = 0;
    static int selected_item = 0;
    static int selected_image = 2;
    static const char *items[] = {"Item 0","item 1","item 2"};

    int i = 0;
    struct zr_context layout;
    struct zr_context combo;
    config->font.height = 20;
    zr_begin(&layout, window, "Basic Demo");

    /*------------------------------------------------
     *                  POPUP BUTTON
     *------------------------------------------------*/
    ui_header(&layout, config, "Popup & Scrollbar & Images");
    ui_widget(&layout, config, 35, 22);
    if (zr_button_text_image(&layout, img->directory,
        "Images", ZR_TEXT_CENTERED, ZR_BUTTON_DEFAULT))
        image_active = !image_active;

    /*------------------------------------------------
     *                  SELECTED IMAGE
     *------------------------------------------------*/
    ui_header(&layout, config, "Selected Image");
    ui_widget_centered(&layout, config, 100, 22);
    zr_image(&layout, img->images[selected_image]);

    /*------------------------------------------------
     *                  IMAGE POPUP
     *------------------------------------------------*/
    if (image_active) {
        struct zr_context popup;
        static struct zr_vec2 scrollbar;
        zr_popup_begin(&layout, &popup, ZR_POPUP_STATIC, 0, 0,
            zr_rect(265, 0, 300, 200), scrollbar);
        zr_layout_row_static(&popup, 82, 82, 3);
        for (i = 0; i < 9; ++i) {
            if (zr_button_image(&popup, img->images[i], ZR_BUTTON_DEFAULT)) {
                selected_image = i;
                image_active = 0;
                zr_popup_close(&popup);
            }
        }
        zr_popup_end(&layout, &popup, &scrollbar);
    }

    /*------------------------------------------------
     *                  COMBOBOX
     *------------------------------------------------*/
    ui_header(&layout, config, "Combo box");
    ui_widget(&layout, config, 40, 22);
    zr_combo_begin(&layout, &combo, items[selected_item], &combo_active);
    zr_layout_row_dynamic(&combo, 35, 1);
    for (i = 0; i < 3; ++i)
        if (zr_combo_item(&combo, items[i], ZR_TEXT_LEFT))
            selected_item = i;
    zr_combo_end(&layout, &combo, &combo_active);

    /*------------------------------------------------
     *                  CHECKBOX
     *------------------------------------------------*/
    ui_header(&layout, config, "Checkbox");
    ui_widget(&layout, config, 30, 22);
    zr_checkbox(&layout, "Flag 1", &check0);
    ui_widget(&layout, config, 30, 22);
    zr_checkbox(&layout, "Flag 2", &check1);

    /*------------------------------------------------
     *                  PROGRESSBAR
     *------------------------------------------------*/
    ui_header(&layout, config, "Progressbar");
    ui_widget(&layout, config, 35, 22);
    zr_progress(&layout, &prog, 100, zr_true);

    /*------------------------------------------------
     *                  SLIDER
     *------------------------------------------------*/
    ui_header(&layout, config, "Slider");
    ui_widget(&layout, config, 35, 22);
    zr_slider_int(&layout, 0, &slider, 100, 10);
    zr_end(&layout, window);
}

static void
grid_demo(struct zr_window *window, struct zr_style *config)
{
    static char text[3][64];
    static size_t text_len[3];
    static int text_active[3];
    static size_t text_cursor[3];
    static const char *items[] = {"Item 0","item 1","item 2"};
    static int selected_item = 0;
    static int combo_active = 0;
    static int check = 1;

    int i;
    struct zr_context layout;
    struct zr_context combo;

    config->font.height = 20;
    zr_begin(&layout, window, "Grid Demo");

    config->font.height = 18;
    zr_layout_row_dynamic(&layout, 30, 2);
    zr_label(&layout, "Floating point:", ZR_TEXT_RIGHT);
    zr_edit(&layout, text[0], &text_len[0], 64, &text_active[0], &text_cursor[0], ZR_INPUT_FLOAT);
    zr_label(&layout, "Hexadeximal:", ZR_TEXT_RIGHT);
    zr_edit(&layout, text[1], &text_len[1], 64, &text_active[1], &text_cursor[1], ZR_INPUT_HEX);
    zr_label(&layout, "Binary:", ZR_TEXT_RIGHT);
    zr_edit(&layout, text[2], &text_len[2], 64, &text_active[2], &text_cursor[2], ZR_INPUT_BIN);
    zr_label(&layout, "Checkbox:", ZR_TEXT_RIGHT);
    zr_checkbox(&layout, "Check me", &check);
    zr_label(&layout, "Combobox:", ZR_TEXT_RIGHT);
    zr_combo_begin(&layout, &combo, items[selected_item], &combo_active);
    zr_layout_row_dynamic(&combo, 30, 1);
    for (i = 0; i < 3; ++i)
        if (zr_combo_item(&combo, items[i], ZR_TEXT_LEFT))
            selected_item = i;
    zr_combo_end(&layout, &combo, &combo_active);
    zr_end(&layout, window);
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
draw(NVGcontext *nvg, struct zr_command_queue *queue, int width, int height)
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
    zr_foreach_command(cmd, queue) {
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
            nvgFillColor(nvg, nvgRGBA(l->color.r, l->color.g, l->color.b, l->color.a));
            nvgFill(nvg);
        } break;
        case ZR_COMMAND_CURVE: {
            const struct zr_command_curve *q = zr_command(curve, cmd);
            nvgBeginPath(nvg);
            nvgMoveTo(nvg, q->begin.x, q->begin.y);
            nvgBezierTo(nvg, q->ctrl[0].x, q->ctrl[0].y, q->ctrl[1].x,
                q->ctrl[1].y, q->end.x, q->end.y);
            nvgStrokeColor(nvg, nvgRGBA(q->color.r, q->color.g, q->color.b, q->color.a));
            nvgStrokeWidth(nvg, 3);
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
            const float font_height = t->height;
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
    zr_command_queue_clear(queue);

    nvgResetScissor(nvg);
    nvgEndFrame(nvg);
    glPopAttrib();
}

static void
key(struct zr_input *in, SDL_Event *evt, int down)
{
    const Uint8* state = SDL_GetKeyboardState(NULL);
    SDL_Keycode sym = evt->key.keysym.sym;
    if (sym == SDLK_RSHIFT || sym == SDLK_LSHIFT)
        zr_input_key(in, ZR_KEY_SHIFT, down);
    else if (sym == SDLK_DELETE)
        zr_input_key(in, ZR_KEY_DEL, down);
    else if (sym == SDLK_RETURN)
        zr_input_key(in, ZR_KEY_ENTER, down);
    else if (sym == SDLK_BACKSPACE)
        zr_input_key(in, ZR_KEY_BACKSPACE, down);
    else if (sym == SDLK_LEFT)
        zr_input_key(in, ZR_KEY_LEFT, down);
    else if (sym == SDLK_RIGHT)
        zr_input_key(in, ZR_KEY_RIGHT, down);
    else if (sym == SDLK_c)
        zr_input_key(in, ZR_KEY_COPY, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_v)
        zr_input_key(in, ZR_KEY_PASTE, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_x)
        zr_input_key(in, ZR_KEY_CUT, down && state[SDL_SCANCODE_LCTRL]);
}

static void
motion(struct zr_input *in, SDL_Event *evt)
{
    const int x = evt->motion.x;
    const int y = evt->motion.y;
    zr_input_motion(in, x, y);
}

static void
btn(struct zr_input *in, SDL_Event *evt, int down)
{
    const int x = evt->button.x;
    const int y = evt->button.y;
    if (evt->button.button == SDL_BUTTON_LEFT)
        zr_input_button(in, ZR_BUTTON_LEFT, x, y, down);
    else if (evt->button.button == SDL_BUTTON_RIGHT)
        zr_input_button(in, ZR_BUTTON_RIGHT, x, y, down);
    else if (evt->button.button == SDL_BUTTON_MIDDLE)
        zr_input_button(in, ZR_BUTTON_MIDDLE, x, y, down);
}

static void
text(struct zr_input *in, SDL_Event *evt)
{
    zr_glyph glyph;
    memcpy(glyph, evt->text.text, ZR_UTF_SIZE);
    zr_input_glyph(in, glyph);
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

    /* images */
    struct icons icons;
    int unchecked;
    int checked;
    int rocket;
    int cloud;
    int pen;
    int play;
    int stop;
    int pause;
    int next;
    int prev;
    int tools;
    int directory;
    int images[9];
    int copy;
    int convert;
    int delete;
    int edit;

    /* GUI */
    struct gui gui;
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

    /* images */
    unchecked = nvgCreateImage(vg, "../icon/unchecked.png", 0);
    checked = nvgCreateImage(vg, "../icon/checked.png", 0);
    rocket = nvgCreateImage(vg, "../icon/rocket.png", 0);
    cloud = nvgCreateImage(vg, "../icon/cloud.png", 0);
    pen = nvgCreateImage(vg, "../icon/pen.png", 0);
    play = nvgCreateImage(vg, "../icon/play.png", 0);
    pause = nvgCreateImage(vg, "../icon/pause.png", 0);
    stop = nvgCreateImage(vg, "../icon/stop.png", 0);
    next =  nvgCreateImage(vg, "../icon/next.png", 0);
    prev =  nvgCreateImage(vg, "../icon/prev.png", 0);
    tools = nvgCreateImage(vg, "../icon/tools.png", 0);
    directory = nvgCreateImage(vg, "../icon/directory.png", 0);
    copy = nvgCreateImage(vg, "../icon/copy.png", 0);
    convert = nvgCreateImage(vg, "../icon/export.png", 0);
    delete = nvgCreateImage(vg, "../icon/delete.png", 0);
    edit = nvgCreateImage(vg, "../icon/edit.png", 0);
    for (i = 0; i < 9; ++i) {
        char buffer[64];
        sprintf(buffer, "../images/image%d.jpg", (i+1));
        images[i] = nvgCreateImage(vg, buffer, 0);
    }

    /* icons */
    icons.unchecked = zr_image_id(unchecked);
    icons.checked = zr_image_id(checked);
    icons.rocket = zr_image_id(rocket);
    icons.cloud = zr_image_id(cloud);
    icons.pen = zr_image_id(pen);
    icons.play = zr_image_id(play);
    icons.tools = zr_image_id(tools);
    icons.directory = zr_image_id(directory);
    icons.pause = zr_image_id(pause);
    icons.stop = zr_image_id(stop);
    icons.prev = zr_image_id(prev);
    icons.next = zr_image_id(next);
    icons.copy = zr_image_id(copy);
    icons.convert = zr_image_id(convert);
    icons.delete = zr_image_id(delete);
    icons.edit = zr_image_id(edit);

    for (i = 0; i < 9; ++i)
        icons.images[i] = zr_image_id(images[i]);

    /* GUI */
    memset(&gui, 0, sizeof gui);
    gui.memory = malloc(MAX_MEMORY);
    zr_command_queue_init_fixed(&gui.queue, gui.memory, MAX_MEMORY);

    gui.font.userdata.ptr = vg;
    nvgTextMetrics(vg, NULL, NULL, &gui.font.height);
    gui.font.width = font_get_width;
    gui.font.height = 20;
    zr_style_default(&gui.config, ZR_DEFAULT_ALL, &gui.font);
    gui.config.rounding[ZR_ROUNDING_BUTTON] = 3;

    zr_window_init(&gui.button_demo, zr_rect(50, 50, 250, 600),
        ZR_WINDOW_BORDER|ZR_WINDOW_MOVEABLE|ZR_WINDOW_BORDER_HEADER,
        &gui.queue, &gui.config, &gui.input);
    zr_window_init(&gui.basic_demo, zr_rect(320, 50, 275, 600),
        ZR_WINDOW_BORDER|ZR_WINDOW_MOVEABLE|ZR_WINDOW_BORDER_HEADER,
        &gui.queue, &gui.config, &gui.input);
    zr_window_init(&gui.grid_demo, zr_rect(600, 350, 275, 250),
        ZR_WINDOW_BORDER|ZR_WINDOW_MOVEABLE|ZR_WINDOW_BORDER_HEADER|ZR_WINDOW_NO_SCROLLBAR,
        &gui.queue, &gui.config, &gui.input);


    while (running) {
        /* Input */
        SDL_Event evt;
        started = SDL_GetTicks();
        zr_input_begin(&gui.input);
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_WINDOWEVENT &&
                evt.window.event == SDL_WINDOWEVENT_RESIZED)
                glViewport(0, 0, evt.window.data1, evt.window.data2);
            else if (evt.type == SDL_QUIT) goto cleanup;
            else if (evt.type == SDL_KEYUP) key(&gui.input, &evt, zr_false);
            else if (evt.type == SDL_KEYDOWN) key(&gui.input, &evt, zr_true);
            else if (evt.type == SDL_MOUSEBUTTONDOWN) btn(&gui.input, &evt, zr_true);
            else if (evt.type == SDL_MOUSEBUTTONUP) btn(&gui.input, &evt, zr_false);
            else if (evt.type == SDL_MOUSEMOTION) motion(&gui.input, &evt);
            else if (evt.type == SDL_TEXTINPUT) text(&gui.input, &evt);
            else if (evt.type == SDL_MOUSEWHEEL) zr_input_scroll(&gui.input, evt.wheel.y);
        }
        zr_input_end(&gui.input);

        /* GUI */
        button_demo(&gui.button_demo, &gui.config, &icons);
        basic_demo(&gui.basic_demo, &gui.config, &icons);
        grid_demo(&gui.grid_demo, &gui.config);

        /* Draw */
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        SDL_GetWindowSize(win, &width, &height);
        draw(vg, &gui.queue, width, height);
        SDL_GL_SwapWindow(win);

        /* Timing */
        dt = SDL_GetTicks() - started;
        if (dt < DTIME)
            SDL_Delay(DTIME - dt);
    }

cleanup:
    /* Cleanup */
    nvgDeleteImage(vg, unchecked);
    nvgDeleteImage(vg, checked);
    nvgDeleteImage(vg, rocket);
    nvgDeleteImage(vg, cloud);
    nvgDeleteImage(vg, pen);
    nvgDeleteImage(vg, play);
    nvgDeleteImage(vg, pause);
    nvgDeleteImage(vg, stop);
    nvgDeleteImage(vg, next);
    nvgDeleteImage(vg, prev);
    nvgDeleteImage(vg, tools);
    nvgDeleteImage(vg, directory);

    free(gui.memory);
    nvgDeleteGLES2(vg);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
