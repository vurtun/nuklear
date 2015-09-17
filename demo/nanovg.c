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
#define DTIME       33
#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#define MAX(a,b)    ((a) < (b) ? (b) : (a))
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#define LEN(a)      (sizeof(a)/sizeof(a)[0])
#define UNUSED(a)   ((void)(a))

#include "../zahnrad.h"

static void
clipboard_set(const char *text)
{SDL_SetClipboardText(text);}

static zr_bool
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

static zr_size
font_get_width(zr_handle handle, const zr_char *text, zr_size len)
{
    zr_size width;
    float bounds[4];
    NVGcontext *ctx = (NVGcontext*)handle.ptr;
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
        case ZR_COMMAND_MAX:
        default: break;
        }
    }
    zr_command_queue_clear(queue);

    nvgResetScissor(nvg);
    nvgEndFrame(nvg);
    glPopAttrib();
}

static void
key(struct zr_input *in, SDL_Event *evt, zr_bool down)
{
    const Uint8* state = SDL_GetKeyboardState(NULL);
    SDL_Keycode sym = evt->key.keysym.sym;
    if (sym == SDLK_RSHIFT || sym == SDLK_LSHIFT)
        zr_input_key(in, ZR_KEY_SHIFT, down);
    else if (sym == SDLK_DELETE)
        zr_input_key(in, ZR_KEY_DEL, down);
    else if (sym == SDLK_RETURN)
        zr_input_key(in, ZR_KEY_ENTER, down);
    else if (sym == SDLK_SPACE)
        zr_input_key(in, ZR_KEY_SPACE, down);
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
    const zr_int x = evt->motion.x;
    const zr_int y = evt->motion.y;
    zr_input_motion(in, x, y);
}

static void
btn(struct zr_input *in, SDL_Event *evt, zr_bool down)
{
    const zr_int x = evt->button.x;
    const zr_int y = evt->button.y;
    if (evt->button.button == SDL_BUTTON_LEFT)
        zr_input_button(in, ZR_BUTTON_LEFT, x, y, down);
    else if (evt->button.button == SDL_BUTTON_LEFT)
        zr_input_button(in, ZR_BUTTON_RIGHT, x, y, down);
}

static void
text(struct zr_input *in, SDL_Event *evt)
{
    zr_glyph glyph;
    memcpy(glyph, evt->text.text, ZR_UTF_SIZE);
    zr_input_glyph(in, glyph);
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
    zr_size font_height;
    SDL_Window *win;
    SDL_GLContext glContext;
    NVGcontext *vg = NULL;
    unsigned int started;
    unsigned int dt;

    /* GUI */
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
    memset(&gui, 0, sizeof gui);
    zr_buffer_init_fixed(&gui.memory, calloc(MAX_MEMORY, 1), MAX_MEMORY);
    gui.font.userdata = zr_handle_ptr(vg);
    gui.font.width = font_get_width;
    nvgTextMetrics(vg, NULL, NULL, &gui.font.height);
    init_demo(&gui);

    while (gui.running) {
        /* Input */
        SDL_Event evt;
        started = SDL_GetTicks();
        zr_input_begin(&gui.input);
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_WINDOWEVENT) resize(&evt);
            else if (evt.type == SDL_QUIT) goto cleanup;
            else if (evt.type == SDL_KEYUP) key(&gui.input, &evt, zr_false);
            else if (evt.type == SDL_KEYDOWN) key(&gui.input, &evt, zr_true);
            else if (evt.type == SDL_MOUSEBUTTONDOWN) btn(&gui.input, &evt, zr_true);
            else if (evt.type == SDL_MOUSEBUTTONUP) btn(&gui.input, &evt, zr_false);
            else if (evt.type == SDL_MOUSEMOTION) motion(&gui.input, &evt);
            else if (evt.type == SDL_TEXTINPUT) text(&gui.input, &evt);
            else if (evt.type == SDL_MOUSEWHEEL)
                zr_input_scroll(&gui.input,(float)evt.wheel.y);
        }
        zr_input_end(&gui.input);

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
    free(zr_buffer_memory(&gui.memory));
    nvgDeleteGLES2(vg);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

