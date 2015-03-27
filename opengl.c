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

#include "gui.h"

/* macros */
#define LEN(a) (sizeof(a)/sizeof(a)[0])
#define UNUSED(v) (void)v
#define WIN_WIDTH 800
#define WIN_HEIGHT 600
#define MAX_MEMORY (128 * 1024)
#define MAX_PANELS 4
#define DTIME 33
#define MAX_BUFFER 64

/* functions */
static void die(const char*,...);
static void* xcalloc(size_t nmemb, size_t size);
static long timestamp(void);
static void sleep_for(long ms);
static char* ldfile(const char*, size_t*);

static void kpress(struct gui_input*, SDL_Event*);
static void bpress(struct gui_input*, SDL_Event*);
static void brelease(struct gui_input*, SDL_Event*);
static void bmotion(struct gui_input*, SDL_Event*);
static GLuint ldbmp(gui_byte*, uint32_t*, uint32_t*);

/* gobals */
static void
die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    SDL_Quit();
    exit(1);
}

static void*
xcalloc(size_t nmemb, size_t size)
{
    void *p = calloc(nmemb, size);
    if (!p) die("out of memory\n");
    return p;
}

static void
kpress(struct gui_input *in, SDL_Event* e)
{
    SDL_Keycode code = e->key.keysym.sym;
    if (code == SDLK_LCTRL || code == SDLK_RCTRL)
        gui_input_key(in, GUI_KEY_CTRL, gui_true);
    else if (code == SDLK_LSHIFT || code == SDLK_RSHIFT)
        gui_input_key(in, GUI_KEY_SHIFT, gui_true);
    else if (code == SDLK_DELETE)
        gui_input_key(in, GUI_KEY_DEL, gui_true);
    else if (code == SDLK_RETURN)
        gui_input_key(in, GUI_KEY_ENTER, gui_true);
    else if (code == SDLK_SPACE)
        gui_input_key(in, GUI_KEY_SPACE, gui_true);
    else if (code == SDLK_BACKSPACE)
        gui_input_key(in, GUI_KEY_BACKSPACE, gui_true);
}

static void
krelease(struct gui_input *in, SDL_Event* e)
{
    SDL_Keycode code = e->key.keysym.sym;
    if (code == SDLK_LCTRL || code == SDLK_RCTRL)
        gui_input_key(in, GUI_KEY_CTRL, gui_false);
    else if (code == SDLK_LSHIFT || code == SDLK_RSHIFT)
        gui_input_key(in, GUI_KEY_SHIFT, gui_false);
    else if (code == SDLK_DELETE)
        gui_input_key(in, GUI_KEY_DEL, gui_false);
    else if (code == SDLK_RETURN)
        gui_input_key(in, GUI_KEY_ENTER, gui_false);
    else if (code == SDLK_SPACE)
        gui_input_key(in, GUI_KEY_SPACE, gui_false);
    else if (code == SDLK_BACKSPACE)
        gui_input_key(in, GUI_KEY_BACKSPACE, gui_false);
}

static void
bpress(struct gui_input *in, SDL_Event *evt)
{
    const int x = evt->button.x;
    const int y = evt->button.y;
    if (evt->button.button == SDL_BUTTON_LEFT)
        gui_input_button(in, x, y, gui_true);
}

static void
brelease(struct gui_input *in, SDL_Event *evt)
{
    const int x = evt->button.x;
    const int y = evt->button.y;
    if (evt->button.button == SDL_BUTTON_LEFT)
        gui_input_button(in, x, y, gui_false);
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

static char*
ldfile(const char* path, size_t* siz)
{
    char *buf;
    FILE *fd = fopen(path, "rb");
    if (!fd) die("Failed to open file: %s\n");
    fseek(fd, 0, SEEK_END);
    *siz = (size_t)ftell(fd);
    fseek(fd, 0, SEEK_SET);
    buf = xcalloc(*siz, 1);
    fread(buf, *siz, 1, fd);
    fclose(fd);
    return buf;
}

static GLuint
ldbmp(gui_byte *data, uint32_t *width, uint32_t *height)
{
    /* texture */
    GLuint texture;
    gui_byte *header;
    gui_byte *target;
    gui_byte *writer;
    gui_byte *reader;
    uint32_t ioff;
    uint32_t j;
    int32_t i;

    header = data;
    if (!width || !height)
        die("[BMP]: width or height is NULL!");
    if (header[0] != 'B' || header[1] != 'M')
        die("[BMP]: invalid file");

    memcpy(width, &header[0x12], sizeof(uint32_t));
    memcpy(height, &header[0x16], sizeof(uint32_t));
    memcpy(&ioff, &header[0x0A], sizeof(uint32_t));
    if (*width <= 0 || *height <= 0)
        die("[BMP]: invalid image size");

    data = data + ioff;
    reader = data;
    target = xcalloc(*width * *height * 4, 1);
    for (i = (int32_t)*height-1; i >= 0; i--) {
        writer = target + (i * (int32_t)*width * 4);
        for (j = 0; j < *width; j++) {
            *writer++ = *(reader + (j * 4) + 1);
            *writer++ = *(reader + (j * 4) + 2);
            *writer++ = *(reader + (j * 4) + 3);
            *writer++ = *(reader + (j * 4) + 0);
        }
        reader += *width * 4;
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)*width, (GLsizei)*height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, target);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    free(target);
    return texture;
}

static struct gui_font*
ldfont(const char *name, unsigned char height)
{
    size_t mem;
    size_t size;
    size_t i = 0;
    gui_byte *iter;
    gui_texture tex;
    short max_height = 0;
    struct gui_font *font;
    uint32_t img_width, img_height;
    struct gui_font_glyph *glyphes;

    uint16_t num;
    uint16_t indexes;
    uint16_t tex_width;
    uint16_t tex_height;

    gui_byte *buffer = (gui_byte*)ldfile(name, &size);
    memcpy(&num, buffer, sizeof(uint16_t));
    memcpy(&indexes, &buffer[0x02], sizeof(uint16_t));
    memcpy(&tex_width, &buffer[0x04], sizeof(uint16_t));
    memcpy(&tex_height, &buffer[0x06], sizeof(uint16_t));

    iter = &buffer[0x08];
    mem = sizeof(struct gui_font_glyph) * ((size_t)indexes + 1);
    glyphes = xcalloc(mem, 1);
    for(i = 0; i < num; ++i) {
        uint16_t id, x, y, w, h;
        float xoff, yoff, xadv;

        memcpy(&id, iter, sizeof(uint16_t));
        memcpy(&x, &iter[0x02], sizeof(uint16_t));
        memcpy(&y, &iter[0x04], sizeof(uint16_t));
        memcpy(&w, &iter[0x06], sizeof(uint16_t));
        memcpy(&h, &iter[0x08], sizeof(uint16_t));
        memcpy(&xoff, &iter[10], sizeof(float));
        memcpy(&yoff, &iter[14], sizeof(float));
        memcpy(&xadv, &iter[18], sizeof(float));

        glyphes[id].code = id;
        glyphes[id].width = (short)w;
        glyphes[id].height = (short)h;
        glyphes[id].xoff  = xoff;
        glyphes[id].yoff = yoff;
        glyphes[id].xadvance = xadv;
        glyphes[id].uv[0].u = (float)x/(float)tex_width;
        glyphes[id].uv[0].v = (float)y/(float)tex_height;
        glyphes[id].uv[1].u = (float)(x+w)/(float)tex_width;
        glyphes[id].uv[1].v = (float)(y+h)/(float)tex_height;
        if (glyphes[id].height > max_height) max_height = glyphes[id].height;
        iter += 22;
    }

    font = xcalloc(sizeof(struct gui_font), 1);
    font->height = height;
    font->scale = (float)height/(float)max_height;
    font->texture.gl = ldbmp(iter, &img_width, &img_height);
    font->tex_size.x = tex_width;
    font->tex_size.y = tex_height;
    font->fallback = &glyphes['?'];
    font->glyphes = glyphes;
    font->glyph_count = indexes + 1;
    free(buffer);
    return font;
}

static void
delfont(struct gui_font *font)
{
    if (!font) return;
    if (font->glyphes)
        free(font->glyphes);
    free(font);
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

    struct gui_context *ctx;
    struct gui_font *font;
    struct gui_panel *panel;
    struct gui_panel *subpanel;
    struct gui_config config;
    struct gui_memory memory;
    struct gui_input input;
    struct gui_output output;

    gui_char cmd_buf[MAX_BUFFER];
    gui_size cmd_len = 0;
    gui_bool cmd_act = gui_false;
    gui_bool check = gui_false;
    gui_int option = 0;
    gui_float slider = 2.0f;
    gui_size prog = 60;
    gui_int spinner = 100;
    gui_bool spin_act = gui_false;
    const gui_float values[] = {8.0f, 15.0f, 20.0f, 12.0f, 30.0f};
    const char *items[] = {"Fist", "Pistol", "Shotgun", "Railgun", "BFG"};
    gui_size item_cur = 0;
    gui_float list_off = 0.0f;
    gui_bool list_sel[5];

    /* Window */
    UNUSED(argc); UNUSED(argv);
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
        die("[SDL] unabled to initialize\n");
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    win = SDL_CreateWindow("clone",
        0, 0, WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN);
    if (!win) die("[SDL] unable to create window\n");
    glContext = SDL_GL_CreateContext(win);
    glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);

    /* GUI */
    memset(&input, 0, sizeof(input));
    memory.max_panels = MAX_PANELS;
    memory.memory = xcalloc(MAX_MEMORY , 1);
    memory.size = MAX_MEMORY;
    memory.vertex_percentage = 0.80f;
    memory.command_percentage = 0.19f;
    memory.clip_percentage = 0.01f;

    ctx = gui_new(&memory, &input);
    font = ldfont("mono.sdf", 16);
    gui_default_config(&config);
    config.colors[GUI_COLOR_TEXT].r = 255;
    config.colors[GUI_COLOR_TEXT].g = 255;
    config.colors[GUI_COLOR_TEXT].b = 255;
    config.colors[GUI_COLOR_TEXT].a = 255;
    panel = gui_panel_new(ctx, 20, 20, 200, 400, &config, font);
    subpanel = gui_panel_new(ctx, 250, 20, 200, 200, &config, font);

    running = gui_true;
    while (running) {
        SDL_Event ev;
        started = SDL_GetTicks();
        gui_input_begin(&input);
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_WINDOWEVENT) resize(&ev);
            else if (ev.type == SDL_MOUSEMOTION) bmotion(&input, &ev);
            else if (ev.type == SDL_MOUSEBUTTONDOWN) bpress(&input, &ev);
            else if (ev.type == SDL_MOUSEBUTTONUP) brelease(&input, &ev);
            else if (ev.type == SDL_KEYDOWN) kpress( &input, &ev);
            else if (ev.type == SDL_KEYUP) krelease(&input, &ev);
            else if (ev.type == SDL_TEXTINPUT) text(&input, &ev);
        }
        gui_input_end(&input);
        SDL_GetWindowSize(win, &width, &height);

        /* ------------------------- GUI --------------------------*/
        gui_begin(ctx, (gui_float)width, (gui_float)height);
        running = gui_begin_panel(ctx, panel, "Demo",
            GUI_PANEL_HEADER|GUI_PANEL_CLOSEABLE|GUI_PANEL_MINIMIZABLE|
            GUI_PANEL_MOVEABLE|GUI_PANEL_SCROLLBAR|GUI_PANEL_SCALEABLE);
        gui_panel_layout(panel, 30, 1);
        if (gui_panel_button_text(panel, "button", 6, GUI_BUTTON_SWITCH))
            fprintf(stdout, "button pressed!\n");
        check = gui_panel_check(panel, "advanced", 8, check);
        gui_panel_layout(panel, 30, 2);
        if (gui_panel_option(panel, "easy", 4, option == 0)) option = 0;
        if (gui_panel_option(panel, "hard", 4, option == 1)) option = 1;
        gui_panel_layout(panel, 30, 1);
        slider = gui_panel_slider(panel, 0, slider, 10, 1.0f, GUI_HORIZONTAL);
        prog = gui_panel_progress(panel, prog, 100, gui_true, GUI_HORIZONTAL);
        gui_panel_command(panel, cmd_buf, &cmd_len, MAX_BUFFER, &cmd_act);
        spin_act = gui_panel_spinner(panel, 0, &spinner, 1024, 10, spin_act);
        item_cur = gui_panel_selector(panel, items, LEN(items), item_cur);
        gui_panel_layout(panel, 100, 1);
        gui_panel_plot(panel, values, LEN(values));
        gui_panel_histo(panel, values, LEN(values));
        list_off = gui_panel_list(panel, list_sel, items, LEN(items), list_off, 30);
        gui_end_panel(ctx, panel, NULL);

        gui_begin_panel(ctx, subpanel, "Error",
            GUI_PANEL_HEADER|GUI_PANEL_BORDER|GUI_PANEL_MOVEABLE);
        gui_panel_layout(subpanel, 30, 2);
        if (gui_panel_button_text(subpanel, "ok", 2, GUI_BUTTON_SWITCH))
            fprintf(stdout, "ok pressed!\n");
        if (gui_panel_button_text(subpanel, "cancel", 6, GUI_BUTTON_SWITCH))
            fprintf(stdout, "cancel pressed!\n");
        gui_end_panel(ctx, subpanel, NULL);
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

