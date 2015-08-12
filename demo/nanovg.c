/*
    Copyright (c) 2015
    vurtun <polygone@gmx.net>
    MIT licence
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
    #include <windows.h>
#endif

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>

#define NANOVG_GLES2_IMPLEMENTATION
#include "nanovg/nanovg.h"
#include "nanovg/nanovg_gl.h"
#include "nanovg/nanovg_gl_utils.h"

/* macros */
#define DTIME       16
#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#define MAX(a,b)    ((a) < (b) ? (b) : (a))
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#define LEN(a)      (sizeof(a)/sizeof(a)[0])
#define UNUSED(a)   ((void)(a))

#include "../gui.h"

static void
clipboard_set(const char *text)
{SDL_SetClipboardText(text);}

static gui_bool
clipboard_is_filled(void)
{return SDL_HasClipboardText();}

static const char*
clipboard_get(void)
{return SDL_GetClipboardText();}

#include "demo.c"

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

static gui_size
font_get_width(gui_handle handle, const gui_char *text, gui_size len)
{
    gui_size width;
    float bounds[4];
    NVGcontext *ctx = (NVGcontext*)handle.ptr;
    nvgTextBounds(ctx, 0, 0, text, &text[len], bounds);
    width = (gui_size)(bounds[2] - bounds[0]);
    return width;
}

static void
draw_rect(NVGcontext *ctx, float x, float y, float w, float h, float r, struct gui_color c)
{
    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, x, y, w, h, r);
    nvgFillColor(ctx, nvgRGBA(c.r, c.g, c.b, c.a));
    nvgFill(ctx);
}

static void
draw_text(NVGcontext *ctx, float x, float y, float w, float h,
    struct gui_color c, struct gui_color bg, const gui_char *string, gui_size len)
{
    draw_rect(ctx, x,y,w,h,0, bg);
    nvgBeginPath(ctx);
    nvgFillColor(ctx, nvgRGBA(c.r, c.g, c.b, c.a));
    nvgTextAlign(ctx, NVG_ALIGN_MIDDLE);
    nvgText(ctx, x, y + h * 0.5f, string, &string[len]);
    nvgFill(ctx);
}

static void
draw_line(NVGcontext *ctx, float x0, float y0, float x1, float y1, struct gui_color c)
{
    nvgBeginPath(ctx);
    nvgMoveTo(ctx, x0, y0);
    nvgLineTo(ctx, x1, y1);
    nvgFillColor(ctx, nvgRGBA(c.r, c.g, c.b, c.a));
    nvgFill(ctx);
}

static void
draw_circle(NVGcontext *ctx, float x, float y, float r, struct gui_color c)
{
    nvgBeginPath(ctx);
    nvgCircle(ctx, x + r, y + r, r);
    nvgFillColor(ctx, nvgRGBA(c.r, c.g, c.b, c.a));
    nvgFill(ctx);
}

static void
draw_triangle(NVGcontext *ctx, float x0, float y0, float x1, float y1,
    float x2, float y2, struct gui_color c)
{
    nvgBeginPath(ctx);
    nvgMoveTo(ctx, x0, y0);
    nvgLineTo(ctx, x1, y1);
    nvgLineTo(ctx, x2, y2);
    nvgLineTo(ctx, x0, y0);
    nvgFillColor(ctx, nvgRGBA(c.r, c.g, c.b, c.a));
    nvgFill(ctx);
}

static void
draw_image(NVGcontext *ctx, gui_handle img, float x, float y, float w, float h, float r)
{
    NVGpaint imgpaint;
    imgpaint = nvgImagePattern(ctx, x, y, w, h, 0, img.id, 1.0f);
    nvgBeginPath(ctx);
    nvgRoundedRect(ctx, x, y, w, h, r);
    nvgFillPaint(ctx, imgpaint);
    nvgFill(ctx);
}

static void
draw(NVGcontext *nvg, struct gui_command_queue *queue, int width, int height)
{
    const struct gui_command *cmd;
    glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_TEXTURE_2D);

    nvgBeginFrame(nvg, width, height, ((float)width/(float)height));
    gui_foreach_command(cmd, queue) {
        switch (cmd->type) {
        case GUI_COMMAND_NOP: break;
        case GUI_COMMAND_SCISSOR: {
            const struct gui_command_scissor *s = gui_command(scissor, cmd);
            nvgScissor(nvg, s->x, s->y, s->w, s->h);
        } break;
        case GUI_COMMAND_LINE: {
            const struct gui_command_line *l = gui_command(line, cmd);
            draw_line(nvg, l->begin.x, l->begin.y, l->end.x, l->end.y, l->color);
        } break;
        case GUI_COMMAND_RECT: {
            const struct gui_command_rect *r = gui_command(rect, cmd);
            draw_rect(nvg, r->x, r->y, r->w, r->h, r->rounding, r->color);
        } break;
        case GUI_COMMAND_CIRCLE: {
            const struct gui_command_circle *c = gui_command(circle, cmd);
            draw_circle(nvg, c->x, c->y, (float)c->w / 2.0f, c->color);
        } break;
        case GUI_COMMAND_TRIANGLE: {
            const struct gui_command_triangle *t = gui_command(triangle, cmd);
            draw_triangle(nvg, t->a.x, t->a.y, t->b.x, t->b.y, t->c.x,
                    t->c.y, t->color);
        } break;
        case GUI_COMMAND_TEXT: {
            const struct gui_command_text *t = gui_command(text, cmd);
            draw_text(nvg, t->x, t->y, t->w, t->h, t->foreground, t->background, t->string, t->length);
        } break;
        case GUI_COMMAND_IMAGE: {
            const struct gui_command_image *i = gui_command(image, cmd);
            draw_image(nvg, i->img.handle, i->x, i->y, i->w, i->h, 1);
        } break;
        case GUI_COMMAND_MAX:
        default: break;
        }
    }
    gui_command_queue_clear(queue);

    nvgResetScissor(nvg);
    nvgEndFrame(nvg);
    glPopAttrib();
}

static void
key(struct gui_input *in, SDL_Event *evt, gui_bool down)
{
    const Uint8* state = SDL_GetKeyboardState(NULL);
    SDL_Keycode sym = evt->key.keysym.sym;
    if (sym == SDLK_RSHIFT || sym == SDLK_LSHIFT)
        gui_input_key(in, GUI_KEY_SHIFT, down);
    else if (sym == SDLK_DELETE)
        gui_input_key(in, GUI_KEY_DEL, down);
    else if (sym == SDLK_RETURN)
        gui_input_key(in, GUI_KEY_ENTER, down);
    else if (sym == SDLK_SPACE)
        gui_input_key(in, GUI_KEY_SPACE, down);
    else if (sym == SDLK_BACKSPACE)
        gui_input_key(in, GUI_KEY_BACKSPACE, down);
    else if (sym == SDLK_LEFT)
        gui_input_key(in, GUI_KEY_LEFT, down);
    else if (sym == SDLK_RIGHT)
        gui_input_key(in, GUI_KEY_RIGHT, down);
    else if (sym == SDLK_c)
        gui_input_key(in, GUI_KEY_COPY, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_v)
        gui_input_key(in, GUI_KEY_PASTE, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_x)
        gui_input_key(in, GUI_KEY_CUT, down && state[SDL_SCANCODE_LCTRL]);
}

static void
motion(struct gui_input *in, SDL_Event *evt)
{
    const gui_int x = evt->motion.x;
    const gui_int y = evt->motion.y;
    gui_input_motion(in, x, y);
}

static void
btn(struct gui_input *in, SDL_Event *evt, gui_bool down)
{
    const gui_int x = evt->button.x;
    const gui_int y = evt->button.y;
    if (evt->button.button == SDL_BUTTON_LEFT)
        gui_input_button(in, x, y, down);
}

static void
text(struct gui_input *in, SDL_Event *evt)
{
    gui_glyph glyph;
    memcpy(glyph, evt->text.text, GUI_UTF_SIZE);
    gui_input_glyph(in, glyph);
}

static void
resize(SDL_Event *evt)
{
    if (evt->window.event != SDL_WINDOWEVENT_RESIZED) return;
    glViewport(0, 0, evt->window.data1, evt->window.data2);
}

int
main(int argc, char *argv[])
{
    /* Platform */
    int width, height;
    const char *font_path;
    gui_size font_height;
    SDL_Window *win;
    SDL_GLContext glContext;
    NVGcontext *vg = NULL;
    unsigned int started;
    unsigned int dt;

    /* GUI */
    struct gui_input in;
    struct gui_font font;
    struct demo_gui gui;

    if (argc < 2) {
        fprintf(stdout,"Missing TTF Font file argument: gui <path>\n");
        exit(EXIT_FAILURE);
    }
    font_path = argv[1];
    font_height = 10;

    /* SDL */
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_EVENTS);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    win = SDL_CreateWindow("Demo",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN);
    glContext = SDL_GL_CreateContext(win);
    SDL_GetWindowSize(win, &width, &height);

    /* OpenGL */
    glewExperimental = 1;
    if (glewInit() != GLEW_OK)
        die("[GLEW] failed setup\n");
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    /* nanovg */
    vg = nvgCreateGLES2(NVG_ANTIALIAS|NVG_DEBUG);
    if (!vg) die("[NVG]: failed to init\n");
    nvgCreateFont(vg, "fixed", font_path);
    nvgFontFace(vg, "fixed");
    nvgFontSize(vg, font_height);
    nvgTextAlign(vg, NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);

    /* GUI */
    memset(&in, 0, sizeof in);
    memset(&gui, 0, sizeof gui);
    gui.memory = malloc(MAX_MEMORY);
    font.userdata.ptr = vg;
    nvgTextMetrics(vg, NULL, NULL, &font.height);
    font.width = font_get_width;
    gui.input = &in;
    init_demo(&gui, &font);

    while (gui.running) {
        /* Input */
        SDL_Event evt;
        started = SDL_GetTicks();
        gui_input_begin(&in);
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_WINDOWEVENT) resize(&evt);
            else if (evt.type == SDL_QUIT) goto cleanup;
            else if (evt.type == SDL_KEYUP) key(&in, &evt, gui_false);
            else if (evt.type == SDL_KEYDOWN) key(&in, &evt, gui_true);
            else if (evt.type == SDL_MOUSEBUTTONDOWN) btn(&in, &evt, gui_true);
            else if (evt.type == SDL_MOUSEBUTTONUP) btn(&in, &evt, gui_false);
            else if (evt.type == SDL_MOUSEMOTION) motion(&in, &evt);
            else if (evt.type == SDL_TEXTINPUT) text(&in, &evt);
            else if (evt.type == SDL_MOUSEWHEEL) gui_input_scroll(&in, evt.wheel.y);
        }
        gui_input_end(&in);

        /* GUI */
        SDL_GetWindowSize(win, &width, &height);
        run_demo(&gui);

        /* Draw */
        glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        draw(vg, &gui.queue, width, height);
        SDL_GL_SwapWindow(win);

        /* Timing */
        dt = SDL_GetTicks() - started;
        if (dt < DTIME)
            SDL_Delay(DTIME - dt);
    }

cleanup:
    /* Cleanup */
    free(gui.memory);
    nvgDeleteGLES2(vg);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

