#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <math.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "../gui.h"

/* macros */
#define MAX_BUFFER  64
#define MAX_MEMORY  (8 * 1024)
#define MAX_PANELS  4
#define WIN_WIDTH   800
#define WIN_HEIGHT  600
#define DTIME       16
#define FONT_ATLAS_DEPTH 4
#define CIRCLE_SEGMENTS 22

#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#define MAX(a,b)    ((a) < (b) ? (b) : (a))
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#define LEN(a)      (sizeof(a)/sizeof(a)[0])
#define UNUSED(a)   ((void)(a))

struct texCoord {
    float u;
    float v;
};

enum font_atlas_dimension {
    FONT_ATLAS_DIM_64 = 64,
    FONT_ATLAS_DIM_128 = 128,
    FONT_ATLAS_DIM_256 = 256,
    FONT_ATLAS_DIM_512 = 512,
    FONT_ATLAS_DIM_1024 = 1024,
    FONT_ATLAS_DIM_2048 = 2048
};

struct font_atlas {
    enum font_atlas_dimension dim;
    gui_size range;
    gui_size size;
    gui_byte *memory;
};

struct font_glyph {
    unsigned int code;
    float xadvance;
    short width, height;
    float xoff, yoff;
    struct texCoord uv[2];
};

struct font {
    float height;
    float scale;
    GLuint texture;
    unsigned int glyph_count;
    struct font_glyph *glyphes;
    const struct font_glyph *fallback;
};

struct demo {
    gui_char in_buf[MAX_BUFFER];
    gui_size in_len;
    gui_bool in_act;
    gui_bool check;
    gui_int option;
    gui_float slider;
    gui_size prog;
    gui_int spinner;
    gui_bool spin_act;
    gui_size item_cur;
    gui_size cur;
    gui_bool tab_min;
    gui_float group_off;
    gui_float shelf_off;
    gui_bool toggle;
};

static char*
file_load(const char* path, size_t* siz)
{
    char *buf;
    FILE *fd = fopen(path, "rb");
    if (!fd) {
        fprintf(stderr, "Failed to open file: %s\n", path);
        exit(EXIT_FAILURE);
    }
    fseek(fd, 0, SEEK_END);
    *siz = (size_t)ftell(fd);
    fseek(fd, 0, SEEK_SET);
    buf = calloc(*siz, 1);
    fread(buf, *siz, 1, fd);
    fclose(fd);
    return buf;
}

static void
font_load_glyph(unsigned int code, struct font_glyph *glyph, FT_GlyphSlot slot)
{
    glyph->code = code;
    glyph->width = (short)slot->bitmap.width;
    glyph->height = (short)slot->bitmap.rows;
    glyph->xoff = (float)slot->bitmap_left;
    glyph->yoff = (float)slot->bitmap_top;
    glyph->xadvance = (float)(slot->advance.x >> 6);
}

static void
font_load_glyphes(FT_Face face, struct font *font, size_t range)
{
    size_t i;
    int ft_err;
    for (i = 0; i < range; ++i) {
        unsigned int index = FT_Get_Char_Index(face, i);
        if (!index) continue;
        ft_err = FT_Load_Glyph(face, index, 0);
        if (ft_err) continue;
        ft_err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        if (ft_err) continue;
        font_load_glyph(index, &font->glyphes[i], face->glyph);
    }
}

static void
font_pack_glyphes(struct font *font, float width, float height, size_t range)
{
    size_t i;
    float xoff = 0, yoff = 0;
    float max_height = 0.0f;
    for (i = 0; i < range; ++i) {
        struct font_glyph *glyph = &font->glyphes[i];
        if ((xoff + glyph->width) > width) {
            yoff += max_height;
            max_height = 0.0f;
            xoff = 0.0f;
        }

        glyph->uv[0].u = xoff / width;
        glyph->uv[0].v = yoff / height;
        glyph->uv[1].u = (xoff + glyph->width) / width;
        glyph->uv[1].v = (yoff + glyph->height) / height;
        if (glyph->height > max_height)
            max_height = glyph->height;
        xoff += glyph->width;
    }
}

static void
font_atlas_blit(struct font_atlas *atlas, FT_GlyphSlot glyph,
        size_t off_x, size_t off_y)
{
    size_t y, x;
    size_t width = glyph->bitmap.width;
    size_t height = glyph->bitmap.rows;
    const size_t pitch = atlas->dim * FONT_ATLAS_DEPTH;
    for (y = 0; y < height; y++) {
        size_t x_off = off_x * FONT_ATLAS_DEPTH;
        size_t y_off = (off_y + y) * pitch;
        unsigned char *dst = &atlas->memory[y_off + x_off];
        for (x = 0; x < width; ++x) {
            dst[0] = 255;
            dst[1] = 255;
            dst[2] = 255;
            dst[3] = glyph->bitmap.buffer[y * width + x];
            dst += FONT_ATLAS_DEPTH;
        }
    }
}

static void
font_bake_glyphes(FT_Face face, struct font_atlas *atlas,
    const struct font *font)
{
    size_t i;
    int ft_err;
    for (i = 0; i < atlas->range; ++i) {
        size_t x, y;
        struct font_glyph *glyph = &font->glyphes[i];
        unsigned int index = FT_Get_Char_Index(face, i);

        if (!index) continue;
        ft_err = FT_Load_Glyph(face, index, 0);
        if (ft_err) continue;
        ft_err = FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        if (ft_err) continue;

        x = (gui_size)(glyph->uv[0].u * (gui_float)atlas->dim);
        y = (gui_size)(glyph->uv[0].v * (gui_float)atlas->dim);
        font_atlas_blit(atlas, face->glyph, x, y);
    }
}

static int
font_load(struct font *font, struct font_atlas *atlas, unsigned int height,
    const unsigned char *data, size_t size)
{
    int ret = 1;
    FT_Library ft_lib;
    FT_Face ft_face;

    assert(font);
    assert(atlas);
    assert(font->glyphes);
    assert(atlas->memory);

    if (!font || !atlas)
        return gui_false;
    if (!font->glyphes || !atlas->memory)
        return gui_false;
    if (FT_Init_FreeType(&ft_lib))
        return gui_false;
    if (FT_New_Memory_Face(ft_lib, data, (FT_Long)size, 0, &ft_face))
        goto cleanup;
    if (FT_Select_Charmap(ft_face, FT_ENCODING_UNICODE))
        goto failed;
    if (FT_Set_Char_Size(ft_face, height << 6, height << 6, 96, 96))
        goto failed;

    font_load_glyphes(ft_face, font, atlas->range);
    font_pack_glyphes(font, atlas->dim, atlas->dim, atlas->range);
    font_bake_glyphes(ft_face, atlas, font);

failed:
    FT_Done_Face(ft_face);
cleanup:
    FT_Done_FreeType(ft_lib);
    return ret;
}

static gui_size
font_get_text_width(void *handle, const gui_char *t, gui_size l)
{
    long unicode;
    size_t text_width = 0;
    const struct font_glyph *glyph;
    size_t text_len = 0;
    size_t glyph_len;
    struct font *font = handle;
    assert(font);
    if (!t || !l) return 0;

    glyph_len = gui_utf_decode(t, &unicode, l);
    while (text_len <= l && glyph_len) {
        if (unicode == GUI_UTF_INVALID) return 0;
        glyph = (unicode < font->glyph_count) ? &font->glyphes[unicode] : font->fallback;
        glyph = (glyph->code == 0) ? font->fallback : glyph;

        text_width += (gui_size)((float)glyph->xadvance * font->scale);
        glyph_len = gui_utf_decode(t + text_len, &unicode, l - text_len);
        text_len += glyph_len;
    }
    return text_width;
}

static void
font_draw_text(const struct font *font, float x, float y, float h,
    struct gui_color color, const unsigned char *text, size_t len)
{
    struct gui_rect clip;
    size_t text_len;
    long unicode;
    const struct font_glyph *g;

    text_len = gui_utf_decode(text, &unicode, len);
    glBindTexture(GL_TEXTURE_2D, font->texture);
    glColor4ub(color.r, color.g, color.b, color.a);
    glBegin(GL_QUADS);
    while (text_len <= len) {
        float gx, gy, gh, gw, char_width = 0;
        if (unicode == GUI_UTF_INVALID) break;
        g = (unicode < font->glyph_count) ?
            &font->glyphes[unicode] :
            font->fallback;
        g = (g->code == 0) ? font->fallback : g;


        /* th = font->ascent + font->descent;*/
        /*ty = (int)y + ((int)h / 2) - (th / 2) + font->ascent;*/
        gw = (float)g->width * font->scale;
        gh = (float)g->height * font->scale;
        gx = x + g->xoff * font->scale;
        /*gy = y + (font->height - g->yoff * font->scale);*/
        gy = y + (h / 2) - (g->yoff * font->scale);
        char_width = g->xadvance * font->scale;

        glTexCoord2f(g->uv[0].u, g->uv[0].v);
        glVertex2f(gx, gy);
        glTexCoord2f(g->uv[1].u, g->uv[0].v);
        glVertex2f(gx + gw, gy);
        glTexCoord2f(g->uv[1].u, g->uv[1].v);
        glVertex2f(gx + gw, gy + gh);
        glTexCoord2f(g->uv[0].u, g->uv[1].v);
        glVertex2f(gx, gy + gh);
        text_len += gui_utf_decode(text + text_len, &unicode, len - text_len);
        x += char_width;
    }
    glEnd();
    glBindTexture(GL_TEXTURE_2D, 0);
}

static void
font_del(struct font *font)
{
    glDeleteTextures(1, &font->texture);
    free(font->glyphes);
    free(font);
}

static struct font*
font_new(const char *path, unsigned int font_height, unsigned int bake_height,
    size_t range, enum font_atlas_dimension dim)
{
    gui_byte *ttf_blob;
    gui_size ttf_blob_size;
    struct font_atlas atlas;
    struct font *font = calloc(sizeof(struct font), 1);

    atlas.dim = dim;
    atlas.range = range;
    atlas.size = atlas.dim * atlas.dim * FONT_ATLAS_DEPTH;
    atlas.memory = calloc((gui_size)atlas.size, 1);

    font->glyph_count = (unsigned int)atlas.range;
    font->glyphes = calloc(atlas.range, sizeof(struct font_glyph));
    font->fallback = &font->glyphes['?'];
    font->scale = (float)font_height / (gui_float)bake_height;
    font->height = (float)font_height;

    ttf_blob = (unsigned char*)file_load(path, &ttf_blob_size);
    if (!font_load(font, &atlas, bake_height, ttf_blob, ttf_blob_size))
        goto failed;

    glGenTextures(1, &font->texture);
    glBindTexture(GL_TEXTURE_2D, font->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, dim, dim, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, atlas.memory);

    free(atlas.memory);
    free(ttf_blob);
    return font;

failed:
    free(atlas.memory);
    free(ttf_blob);
    font_del(font);
    return NULL;
}

static void
draw_line(float x0, float y0, float x1, float y1, struct gui_color color)
{
    glColor4ub(color.r, color.g, color.b, color.a);
    glBegin(GL_LINES);
    glVertex2f(x0, y0);
    glVertex2f(x1, y1);
    glEnd();
}

static void
draw_rect(float x, float y, float w, float h, struct gui_color color)
{
    glColor4ub(color.r, color.g, color.b, color.a);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
}

static void
draw_circle(float x, float y, float r, struct gui_color color)
{
    int i;
    float a0 = 0.0f;
    const float a_step = (2 * 3.141592654f)/22.0f;
    x += r; y += r;
    glColor4ub(color.r, color.g, color.b, color.a);
    glBegin(GL_TRIANGLES);
    for (i = 0; i < CIRCLE_SEGMENTS; i++) {
        const float a1 = ((i + 1) == CIRCLE_SEGMENTS) ? 0.0f : a0 + a_step;
        const float p0x = x + (float)cos(a0) * r;
        const float p0y = y + (float)sin(a0) * r;
        const float p1x = x + (float)cos(a1) * r;
        const float p1y = y + (float)sin(a1) * r;

        glVertex2f(p0x, p0y);
        glVertex2f(p1x, p1y);
        glVertex2f(x, y);
        a0 = a1;
    }
    glEnd();
}

static void
demo_panel(struct gui_panel_layout *panel, struct demo *demo)
{
    gui_int i = 0;
    enum {HISTO, PLOT};
    const char *shelfs[] = {"Histogram", "Lines"};
    const gui_float values[] = {8.0f, 15.0f, 20.0f, 12.0f, 30.0f};
    const char *items[] = {"Fist", "Pistol", "Shotgun", "Railgun", "BFG"};
    const char *options[] = {"easy", "normal", "hard", "hell", "doom", "godlike"};
    struct gui_panel_layout tab;

    /* Tabs */
    demo->tab_min = gui_panel_tab_begin(panel, &tab, "Difficulty", demo->tab_min);
    gui_panel_row(&tab, 30, 3);
    for (i = 0; i < (gui_int)LEN(options); i++) {
        if (gui_panel_option(&tab, options[i], demo->option == i))
            demo->option = i;
    }
    gui_panel_tab_end(panel, &tab);

    /* Shelf */
    gui_panel_row(panel, 200, 2);
    demo->cur = gui_panel_shelf_begin(panel,&tab,shelfs,LEN(shelfs),demo->cur,demo->shelf_off);
    gui_panel_row(&tab, 100, 1);
    if (demo->cur == HISTO) {
        gui_panel_histo(&tab, values, LEN(values));
    } else {
        gui_panel_plot(&tab, values, LEN(values));
    }
    demo->shelf_off = gui_panel_shelf_end(panel, &tab);

    /* Group */
    gui_panel_group_begin(panel, &tab, "Options", demo->group_off);
    gui_panel_row(&tab, 30, 1);
    if (gui_panel_button_text(&tab, "button", GUI_BUTTON_DEFAULT))
        fprintf(stdout, "button pressed!\n");
    demo->toggle = gui_panel_button_toggle(&tab, "toggle", demo->toggle);
    demo->check = gui_panel_check(&tab, "advanced", demo->check);
    demo->slider = gui_panel_slider(&tab, 0, demo->slider, 10, 1.0f);
    demo->prog = gui_panel_progress(&tab, demo->prog, 100, gui_true);
    demo->item_cur = gui_panel_selector(&tab, items, LEN(items), demo->item_cur);
    demo->spinner = gui_panel_spinner(&tab, 0, demo->spinner, 250, 10, &demo->spin_act);
    demo->in_len = gui_panel_input(&tab, demo->in_buf, demo->in_len,
                        MAX_BUFFER, &demo->in_act, GUI_INPUT_DEFAULT);
    demo->group_off = gui_panel_group_end(panel, &tab);
}

static void
draw(struct gui_command_list *list, int width, int height)
{
    static const struct gui_color col = {255, 0, 0, 255};
    struct gui_command *cmd;
    if (!list->count) return;

    glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_TRANSFORM_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_TEXTURE_2D);

    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0f, width, height, 0.0f, 0.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    cmd = list->begin;
    while (cmd != list->end) {
        switch (cmd->type) {
        case GUI_COMMAND_NOP: break;
        case GUI_COMMAND_SCISSOR: {
            struct gui_command_scissor *s = (void*)cmd;
            glScissor(s->x, height - (s->y + s->h), s->w, s->h);
        } break;
        case GUI_COMMAND_LINE: {
            struct gui_command_line *l = (void*)cmd;
            draw_line(l->begin[0], l->begin[1], l->end[0], l->end[1], l->color);
        } break;
        case GUI_COMMAND_RECT: {
            struct gui_command_rect *r = (void*)cmd;
            draw_rect(r->x, r->y, r->w, r->h, r->color);
        } break;
        case GUI_COMMAND_CIRCLE: {
            unsigned i;
            struct gui_command_circle *c = (void*)cmd;
            draw_circle(c->x, c->y, (float)c->w / 2.0f, c->color);
        } break;
        case GUI_COMMAND_TRIANGLE: {
            struct gui_command_triangle *t = (void*)cmd;
            glColor4ub(t->color.r, t->color.g, t->color.b, t->color.a);
            glBegin(GL_TRIANGLES);
            glVertex2f(t->a[0], t->a[1]);
            glVertex2f(t->b[0], t->b[1]);
            glVertex2f(t->c[0], t->c[1]);
            glEnd();
        } break;
        case GUI_COMMAND_TEXT: {
            struct gui_command_text *t = (void*)cmd;
            /*draw_rect(t->x, t->y, t->w, t->h, t->fg);*/
            font_draw_text(t->font, t->x, t->y, t->h, t->fg, t->string, t->length);
        } break;
        default: break;
        }
        cmd = cmd->next;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();
}

static void
key(struct gui_input *in, SDL_Event *evt, gui_bool down)
{
    SDL_Keycode sym = evt->key.keysym.sym;
    if (sym == SDLK_LCTRL || sym == SDLK_RCTRL)
        gui_input_key(in, GUI_KEY_CTRL, down);
    else if (sym == SDLK_RSHIFT || sym == SDLK_LSHIFT)
        gui_input_key(in, GUI_KEY_SHIFT, down);
    else if (sym == SDLK_DELETE)
        gui_input_key(in, GUI_KEY_DEL, down);
    else if (sym == SDLK_RETURN)
        gui_input_key(in, GUI_KEY_ENTER, down);
    else if (sym == SDLK_SPACE)
        gui_input_key(in, GUI_KEY_SPACE, down);
    else if (sym == SDLK_BACKSPACE)
        gui_input_key(in, GUI_KEY_BACKSPACE, down);
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
    gui_input_char(in, glyph);
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
    const char *font_path;
    SDL_Window *win;
    SDL_GLContext glContext;
    struct font *glfont;
    gui_bool running = gui_true;
    int win_width, win_height;
    unsigned int started;
    unsigned int dt;
    int width = 0, height = 0;
    struct demo demo;

    /* GUI */
    struct gui_input in;
    struct gui_font font;
    struct gui_memory memory;
    struct gui_memory_status status;
    struct gui_config config;
    struct gui_canvas canvas;
    struct gui_command_buffer buffer;
    struct gui_command_list list;
    struct gui_panel panel;
    struct gui_panel_layout layout;

    font_path = argv[1];
    if (argc < 2) {
        fprintf(stdout, "Missing TTF Font file argument!");
        exit(EXIT_FAILURE);
    }

    /* SDL */
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_EVENTS);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    win = SDL_CreateWindow("Demo",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WIN_WIDTH, WIN_HEIGHT, SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN);
    glContext = SDL_GL_CreateContext(win);
    SDL_GetWindowSize(win, &win_width, &win_height);
    glViewport(0, 0, WIN_WIDTH, WIN_HEIGHT);
    glfont = font_new(font_path, 10, 16, 255, FONT_ATLAS_DIM_256);

    /* GUI */
    memset(&in, 0, sizeof in);
    memory.memory = calloc(MAX_MEMORY, 1);
    memory.size = MAX_MEMORY;
    gui_buffer_init_fixed(&buffer, &memory, GUI_CLIP);

    font.userdata = glfont;
    font.height = glfont->height;
    font.width = font_get_text_width;
    gui_default_config(&config);
    gui_panel_init(&panel, 50, 50, 450, 310,
        GUI_PANEL_BORDER|GUI_PANEL_MOVEABLE|
        GUI_PANEL_CLOSEABLE|GUI_PANEL_SCALEABLE|
        GUI_PANEL_MINIMIZABLE, &config, &font);

    /* Demo */
    memset(&demo, 0, sizeof(demo));
    demo.tab_min = gui_true;
    demo.spinner = 100;
    demo.slider = 2.0f;
    demo.prog = 60;

    while (running) {
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
        }
        gui_input_end(&in);

        /* GUI */
        SDL_GetWindowSize(win, &width, &height);
        gui_buffer_begin(&canvas, &buffer, (gui_size)width, (gui_size)height);
        running = gui_panel_begin(&layout, &panel , "Demo", &canvas, &in);
        demo_panel(&layout, &demo);
        gui_panel_end(&layout, &panel);
        gui_buffer_end(&list, &buffer, &canvas, &status);

        /* Draw */
        glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        draw(&list, width, height);
        SDL_GL_SwapWindow(win);

        /* Timing */
        dt = SDL_GetTicks() - started;
        if (dt < DTIME)
            SDL_Delay(DTIME - dt);
    }

cleanup:
    /* Cleanup */
    free(memory.memory);
    font_del(glfont);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

