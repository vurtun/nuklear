/*
    Copyright (c) 2015
    vurtun <polygone@gmx.net>
    MIT licence
*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>

#include "../gui.h"

/* macros */
#define LEN(a) (sizeof(a)/sizeof(a)[0])
#define UNUSED(v) (void)v
#define WIN_WIDTH 800
#define WIN_HEIGHT 600
#define MAX_MEMORY (256 * 1024)
#define MAX_PANELS 8
#define DTIME 33
#define MAX_BUFFER 64


#include "example.c"

/* functions */
static void kpress(struct gui_input*, SDL_Event*);
static void bpress(struct gui_input*, SDL_Event*);
static void brelease(struct gui_input*, SDL_Event*);
static void bmotion(struct gui_input*, SDL_Event*);

/* gobals */
static void
key(struct gui_input *in, SDL_Event* e, gui_bool down)
{
    SDL_Keycode code = e->key.keysym.sym;
    if (code == SDLK_LCTRL || code == SDLK_RCTRL)
        gui_input_key(in, GUI_KEY_CTRL, down);
    else if (code == SDLK_LSHIFT || code == SDLK_RSHIFT)
        gui_input_key(in, GUI_KEY_SHIFT, down);
    else if (code == SDLK_DELETE)
        gui_input_key(in, GUI_KEY_DEL, down);
    else if (code == SDLK_RETURN)
        gui_input_key(in, GUI_KEY_ENTER, down);
    else if (code == SDLK_SPACE)
        gui_input_key(in, GUI_KEY_SPACE, down);
    else if (code == SDLK_BACKSPACE)
        gui_input_key(in, GUI_KEY_BACKSPACE, down);
}

static void
button(struct gui_input *in, SDL_Event *evt, gui_bool down)
{
    const int x = evt->button.x;
    const int y = evt->button.y;
    if (evt->button.button == SDL_BUTTON_LEFT)
        gui_input_button(in, x, y, down);
}

static void
bmotion(struct gui_input *in, SDL_Event *evt)
{
    const gui_int x = evt->motion.x;
    const gui_int y = evt->motion.y;
    gui_input_motion(in, x, y);
}

static void
text(struct gui_input *in, SDL_Event *evt)
{
    gui_input_char(in, (const gui_char*)evt->text.text);
}

static void
resize(SDL_Event* evt)
{
    if (evt->window.event == SDL_WINDOWEVENT_RESIZED)
        glViewport(0, 0, evt->window.data1, evt->window.data2);
}

static void
draw(int width, int height, struct gui_draw_call_list **list, gui_size count)
{
    gui_size i = 0;
    gui_size n = 0;
    const gui_byte *vertexes;
    const struct gui_draw_command *cmd;
    static const size_t v = sizeof(struct gui_vertex);
    static const size_t p = offsetof(struct gui_vertex, pos);
    static const size_t t = offsetof(struct gui_vertex, uv);
    static const size_t c = offsetof(struct gui_vertex, color);

    if (!list) return;
    glPushAttrib(GL_ENABLE_BIT | GL_COLOR_BUFFER_BIT | GL_TRANSFORM_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0f, width, height, 0.0f, 0.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    for (n = 0; n < count; ++n) {
        GLint offset = 0;
        vertexes = (const gui_char*)list[n]->vertexes;
        glVertexPointer(2, GL_FLOAT, (GLsizei)v, (const void*)(vertexes + p));
        glTexCoordPointer(2, GL_FLOAT, (GLsizei)v, (const void*)(vertexes + t));
        glColorPointer(4, GL_UNSIGNED_BYTE, (GLsizei)v, (const void*)(vertexes + c));

        for (i = 0; i < list[n]->command_size; ++i) {
            int x,y,w,h;
            cmd = &list[n]->commands[i];
            x = (int)cmd->clip_rect.x;
            w = (int)cmd->clip_rect.w;
            h = (int)cmd->clip_rect.h;
            y = height - (int)(cmd->clip_rect.y + cmd->clip_rect.h);
            glScissor(x, y, w, h);
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.gl);
            glDrawArrays(GL_TRIANGLES, offset, (GLsizei)cmd->vertex_count);
            offset += (GLint)cmd->vertex_count;
        }
    }

    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

int
main(int argc, char *argv[])
{
    int width, height;
    uint32_t dt, started;
    gui_bool running;
    SDL_Window *win;
    SDL_GLContext glContext;

    struct gui_memory memory;
    struct gui_config config;
    struct gui_input input;
    struct gui_output output;
    struct gui_font *font;
    struct gui_context *ctx;
    struct gui_panel *panel;
    struct gui_panel *message;
    struct demo demo;

    /* Window */
    UNUSED(argc); UNUSED(argv);
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    win = SDL_CreateWindow("clone",
        0, 0, WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN);
    glContext = SDL_GL_CreateContext(win);
    glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);

    /* GUI */
    memset(&input, 0, sizeof(input));
    memory.max_panels = 8;
    memory.max_depth = 4;
    memory.memory = calloc(MAX_MEMORY , 1);
    memory.size = MAX_MEMORY;
    memory.vertex_percentage = 0.80f;
    memory.command_percentage = 0.19f;

    memset(&demo, 0, sizeof(demo));
    demo.tab.minimized = gui_true;
    demo.spinner = 250;
    demo.slider = 2.0f;
    demo.prog = 60;

    font = ldfont("mono.sdf", 16);
    ctx = gui_new(&memory, &input);
    gui_default_config(&config);
    config.colors[GUI_COLOR_TEXT].r = 255;
    config.colors[GUI_COLOR_TEXT].g = 255;
    config.colors[GUI_COLOR_TEXT].b = 255;
    config.colors[GUI_COLOR_TEXT].a = 255;
    panel = gui_panel_new(ctx, 50, 50, 500, 300, &config, font);
    message = gui_panel_new(ctx, 150, 150, 200, 100, &config, font);

    running = gui_true;
    while (running) {
        SDL_Event ev;
        started = SDL_GetTicks();
        gui_input_begin(&input);
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_WINDOWEVENT) resize(&ev);
            else if (ev.type == SDL_MOUSEMOTION) bmotion(&input, &ev);
            else if (ev.type == SDL_MOUSEBUTTONDOWN) button(&input, &ev, gui_true);
            else if (ev.type == SDL_MOUSEBUTTONUP) button(&input, &ev, gui_false);
            else if (ev.type == SDL_KEYDOWN) key( &input, &ev, gui_true);
            else if (ev.type == SDL_KEYUP) key(&input, &ev, gui_false);
            else if (ev.type == SDL_TEXTINPUT) text(&input, &ev);
            else if (ev.type == SDL_QUIT) running = gui_false;
        }
        gui_input_end(&input);
        SDL_GetWindowSize(win, &width, &height);

        /* ------------------------- GUI --------------------------*/
        gui_begin(ctx, (gui_float)width, (gui_float)height);
        running = demo_panel(ctx, panel, &demo);
        message_panel(ctx, message);
        gui_end(ctx, &output, NULL);
        /* ---------------------------------------------------------*/

        /* Draw */
        glClearColor(120.0f/255.0f, 120.0f/255.0f, 120.0f/255.0f, 120.0f/255.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        draw(width, height, output.list, output.list_size);
        SDL_GL_SwapWindow(win);

        /* Timing */
        dt = SDL_GetTicks() - started;
        if (dt < DTIME)
            SDL_Delay(DTIME - dt);
    }

    /* Cleanup */
    free(memory.memory);
    delfont(font);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

