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
#include <math.h>
#include <assert.h>
#include <math.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>

/* macros */
#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

#include "../../zahnrad.h"
#include "../demo.c"

/* ==============================================================
 *
 *                      Utility
 *
 * ===============================================================*/
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

static char*
file_load(const char* path, size_t* siz)
{
    char *buf;
    FILE *fd = fopen(path, "rb");
    if (!fd) die("Failed to open file: %s\n", path);
    fseek(fd, 0, SEEK_END);
    *siz = (size_t)ftell(fd);
    fseek(fd, 0, SEEK_SET);
    buf = (char*)calloc(*siz, 1);
    fread(buf, *siz, 1, fd);
    fclose(fd);
    return buf;
}

struct device {
    GLuint vbo, vao, ebo;
    GLuint prog;
    GLuint vert_shdr;
    GLuint frag_shdr;
    GLint attrib_pos;
    GLint attrib_uv;
    GLint attrib_col;
    GLint uniform_tex;
    GLint uniform_proj;
    GLuint font_tex;
    struct zr_draw_null_texture null;
    struct zr_buffer cmds;
};

static void
device_init(struct device *dev)
{
    GLint status;
    static const GLchar *vertex_shader =
        "#version 300 es\n"
        "uniform mat4 ProjMtx;\n"
        "in vec2 Position;\n"
        "in vec2 TexCoord;\n"
        "in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "void main() {\n"
        "   Frag_UV = TexCoord;\n"
        "   Frag_Color = Color;\n"
        "   gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
        "}\n";
    static const GLchar *fragment_shader =
        "#version 300 es\n"
        "precision mediump float;\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main(){\n"
        "   Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    dev->prog = glCreateProgram();
    dev->vert_shdr = glCreateShader(GL_VERTEX_SHADER);
    dev->frag_shdr = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(dev->vert_shdr, 1, &vertex_shader, 0);
    glShaderSource(dev->frag_shdr, 1, &fragment_shader, 0);
    glCompileShader(dev->vert_shdr);
    glCompileShader(dev->frag_shdr);
    glGetShaderiv(dev->vert_shdr, GL_COMPILE_STATUS, &status);
    assert(status == GL_TRUE);
    glGetShaderiv(dev->frag_shdr, GL_COMPILE_STATUS, &status);
    assert(status == GL_TRUE);
    glAttachShader(dev->prog, dev->vert_shdr);
    glAttachShader(dev->prog, dev->frag_shdr);
    glLinkProgram(dev->prog);
    glGetProgramiv(dev->prog, GL_LINK_STATUS, &status);
    assert(status == GL_TRUE);

    dev->uniform_tex = glGetUniformLocation(dev->prog, "Texture");
    dev->uniform_proj = glGetUniformLocation(dev->prog, "ProjMtx");
    dev->attrib_pos = glGetAttribLocation(dev->prog, "Position");
    dev->attrib_uv = glGetAttribLocation(dev->prog, "TexCoord");
    dev->attrib_col = glGetAttribLocation(dev->prog, "Color");

    {
        /* buffer setup */
        GLsizei vs = sizeof(struct zr_draw_vertex);
        size_t vp = offsetof(struct zr_draw_vertex, position);
        size_t vt = offsetof(struct zr_draw_vertex, uv);
        size_t vc = offsetof(struct zr_draw_vertex, col);

        glGenBuffers(1, &dev->vbo);
        glGenBuffers(1, &dev->ebo);
        glGenVertexArrays(1, &dev->vao);

        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        glEnableVertexAttribArray((GLuint)dev->attrib_pos);
        glEnableVertexAttribArray((GLuint)dev->attrib_uv);
        glEnableVertexAttribArray((GLuint)dev->attrib_col);

        glVertexAttribPointer((GLuint)dev->attrib_pos, 2, GL_FLOAT, GL_FALSE, vs, (void*)vp);
        glVertexAttribPointer((GLuint)dev->attrib_uv, 2, GL_FLOAT, GL_FALSE, vs, (void*)vt);
        glVertexAttribPointer((GLuint)dev->attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void*)vc);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

static struct zr_user_font
font_bake_and_upload(struct device *dev, struct zr_font *font,
    const char *path, unsigned int font_height, const zr_rune *range)
{
    int glyph_count;
    int img_width, img_height;
    struct zr_font_glyph *glyphes;
    struct zr_baked_font baked_font;
    struct zr_user_font user_font;
    struct zr_recti custom;

    memset(&baked_font, 0, sizeof(baked_font));
    memset(&user_font, 0, sizeof(user_font));
    memset(&custom, 0, sizeof(custom));

    {
        /* bake and upload font texture */
        void *img, *tmp;
        size_t ttf_size;
        size_t tmp_size, img_size;
        const char *custom_data = "....";
        struct zr_font_config config;
        char *ttf_blob = file_load(path, &ttf_size);
        if (!ttf_blob)
            die("[Font]: %s is not a file or cannot be found!\n", path);

        /* setup font configuration */
        memset(&config, 0, sizeof(config));
        config.ttf_blob = ttf_blob;
        config.ttf_size = ttf_size;
        config.font = &baked_font;
        config.coord_type = ZR_COORD_UV;
        config.range = range;
        config.pixel_snap = zr_false;
        config.size = (float)font_height;
        config.spacing = zr_vec2(0,0);
        config.oversample_h = 1;
        config.oversample_v = 1;

        /* query needed amount of memory for the font baking process */
        zr_font_bake_memory(&tmp_size, &glyph_count, &config, 1);
        glyphes = (struct zr_font_glyph*)calloc(sizeof(struct zr_font_glyph), (size_t)glyph_count);
        tmp = calloc(1, tmp_size);

        /* pack all glyphes and return needed image width, height and memory size*/
        custom.w = 2; custom.h = 2;
        if (!zr_font_bake_pack(&img_size, &img_width,&img_height,&custom,tmp,tmp_size,&config, 1))
            die("[Font]: failed to load font!\n");

        /* bake all glyphes and custom white pixel into image */
        img = calloc(1, img_size);
        zr_font_bake(img, img_width, img_height, tmp, tmp_size, glyphes, glyph_count, &config, 1);
        zr_font_bake_custom_data(img, img_width, img_height, custom, custom_data, 2, 2, '.', 'X');
        {
            /* convert alpha8 image into rgba8 image */
            void *img_rgba = calloc(4, (size_t)(img_height * img_width));
            zr_font_bake_convert(img_rgba, img_width, img_height, img);
            free(img);
            img = img_rgba;
        }
        {
            /* upload baked font image */
            glGenTextures(1, &dev->font_tex);
            glBindTexture(GL_TEXTURE_2D, dev->font_tex);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)img_width, (GLsizei)img_height, 0,
                        GL_RGBA, GL_UNSIGNED_BYTE, img);
        }
        free(ttf_blob);
        free(tmp);
        free(img);
    }

    /* default white pixel in a texture which is needed to draw primitives */
    dev->null.texture.id = (int)dev->font_tex;
    dev->null.uv = zr_vec2((custom.x + 0.5f)/(float)img_width,
                            (custom.y + 0.5f)/(float)img_height);

    /* setup font with glyphes. IMPORTANT: the font only references the glyphes
      this was done to have the possibility to have multible fonts with one
      total glyph array. Not quite sure if it is a good thing since the
      glyphes have to be freed as well. */
    zr_font_init(font, (float)font_height, '?', glyphes, &baked_font, dev->null.texture);
    user_font = zr_font_ref(font);
    return user_font;
}

static void
device_shutdown(struct device *dev)
{
    glDetachShader(dev->prog, dev->vert_shdr);
    glDetachShader(dev->prog, dev->frag_shdr);
    glDeleteShader(dev->vert_shdr);
    glDeleteShader(dev->frag_shdr);
    glDeleteProgram(dev->prog);
    glDeleteTextures(1, &dev->font_tex);
    glDeleteBuffers(1, &dev->vbo);
    glDeleteBuffers(1, &dev->ebo);
    glDeleteVertexArrays(1, &dev->vao);
}

static void
device_draw(struct device *dev, struct zr_context *ctx, int width, int height,
    enum zr_anti_aliasing AA)
{
    GLint last_prog, last_tex;
    GLint last_ebo, last_vbo, last_vao;
    GLfloat ortho[4][4] = {
        {2.0f, 0.0f, 0.0f, 0.0f},
        {0.0f,-2.0f, 0.0f, 0.0f},
        {0.0f, 0.0f,-1.0f, 0.0f},
        {-1.0f,1.0f, 0.0f, 1.0f},
    };
    ortho[0][0] /= (GLfloat)width;
    ortho[1][1] /= (GLfloat)height;

    /* save previous opengl state */
    glGetIntegerv(GL_CURRENT_PROGRAM, &last_prog);
    glGetIntegerv(GL_TEXTURE_BINDING_2D, &last_tex);
    glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &last_vao);
    glGetIntegerv(GL_ELEMENT_ARRAY_BUFFER_BINDING, &last_ebo);
    glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &last_vbo);

    /* setup global state */
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    /* setup program */
    glUseProgram(dev->prog);
    glUniform1i(dev->uniform_tex, 0);
    glUniformMatrix4fv(dev->uniform_proj, 1, GL_FALSE, &ortho[0][0]);

    {
        /* convert from command queue into draw list and draw to screen */
        const struct zr_draw_command *cmd;
        void *vertexes, *elements;
        const zr_draw_index *offset = NULL;

        /* allocate vertex and element buffer */
        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        glBufferData(GL_ARRAY_BUFFER, MAX_VERTEX_MEMORY, NULL, GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_ELEMENT_MEMORY, NULL, GL_STREAM_DRAW);

        /* load draw vertexes & elements directly into vertex + element buffer */
        vertexes = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        elements = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        {
            struct zr_buffer vbuf, ebuf;

            /* fill converting configuration */
            struct zr_convert_config config;
            memset(&config, 0, sizeof(config));
            config.shape_AA = AA;
            config.line_AA = AA;
            config.circle_segment_count = 22;
            config.line_thickness = 1.0f;
            config.null = dev->null;

            /* setup buffers to load vertexes and elements */
            zr_buffer_init_fixed(&vbuf, vertexes, MAX_VERTEX_MEMORY);
            zr_buffer_init_fixed(&ebuf, elements, MAX_ELEMENT_MEMORY);
            zr_convert(ctx, &dev->cmds, &vbuf, &ebuf, &config);
        }
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

        /* iterate over and execute each draw command */
        zr_draw_foreach(cmd, ctx, &dev->cmds) {
            if (!cmd->elem_count) continue;
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor((GLint)cmd->clip_rect.x,
                height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h),
                (GLint)cmd->clip_rect.w, (GLint)cmd->clip_rect.h);
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
            offset += cmd->elem_count;
        }
        zr_clear(ctx);
    }

    /* restore old state */
    glUseProgram((GLuint)last_prog);
    glBindTexture(GL_TEXTURE_2D, (GLuint)last_tex);
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint)last_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (GLuint)last_ebo);
    glBindVertexArray((GLuint)last_vao);
    glDisable(GL_SCISSOR_TEST);
}

static void
input_key(struct zr_context *ctx, SDL_Event *evt, int down)
{
    const Uint8* state = SDL_GetKeyboardState(NULL);
    SDL_Keycode sym = evt->key.keysym.sym;
    if (sym == SDLK_RSHIFT || sym == SDLK_LSHIFT)
        zr_input_key(ctx, ZR_KEY_SHIFT, down);
    else if (sym == SDLK_DELETE)
        zr_input_key(ctx, ZR_KEY_DEL, down);
    else if (sym == SDLK_RETURN)
        zr_input_key(ctx, ZR_KEY_ENTER, down);
    else if (sym == SDLK_TAB)
        zr_input_key(ctx, ZR_KEY_TAB, down);
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
input_motion(struct zr_context *ctx, SDL_Event *evt)
{
    const int x = evt->motion.x;
    const int y = evt->motion.y;
    zr_input_motion(ctx, x, y);
}

static void
input_button(struct zr_context *ctx, SDL_Event *evt, int down)
{
    const int x = evt->button.x;
    const int y = evt->button.y;
    if (evt->button.button == SDL_BUTTON_LEFT)
        zr_input_button(ctx, ZR_BUTTON_LEFT, x, y, down);
    if (evt->button.button == SDL_BUTTON_RIGHT)
        zr_input_button(ctx, ZR_BUTTON_RIGHT, x, y, down);
}

static void
input_text(struct zr_context *ctx, SDL_Event *evt)
{
    zr_glyph glyph;
    memcpy(glyph, evt->text.text, ZR_UTF_SIZE);
    zr_input_glyph(ctx, glyph);
}

static void
resize(SDL_Event *evt)
{
    if (evt->window.event != SDL_WINDOWEVENT_RESIZED) return;
    glViewport(0, 0, evt->window.data1, evt->window.data2);
}

static void* mem_alloc(zr_handle unused, size_t size)
{UNUSED(unused); return calloc(1, size);}
static void mem_free(zr_handle unused, void *ptr)
{UNUSED(unused); free(ptr);}

int
main(int argc, char *argv[])
{
    /* Platform */
    const char *font_path;
    SDL_Window *win;
    SDL_GLContext glContext;
    int win_width, win_height;
    int width = 0, height = 0;
    int running = 1;

    /* GUI */
    struct device device;
    struct demo gui;
    struct zr_font font;

    font_path = argv[1];
    if (argc < 2)
        die("Missing TTF Font file argument!");

    /* SDL */
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_EVENTS);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    win = SDL_CreateWindow("Demo",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN);
    glContext = SDL_GL_CreateContext(win);
    SDL_GetWindowSize(win, &win_width, &win_height);

    /* OpenGL */
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glewExperimental = 1;
    if (glewInit() != GLEW_OK)
        die("Failed to setup GLEW\n");

    {
        /* GUI */
        struct zr_user_font usrfnt;
        struct zr_allocator alloc;
        alloc.userdata.ptr = NULL;
        alloc.alloc = mem_alloc;
        alloc.free = mem_free;
        zr_buffer_init(&device.cmds, &alloc, 1024);
        usrfnt = font_bake_and_upload(&device, &font, font_path, 14,
                        zr_font_default_glyph_ranges());
        zr_init(&gui.ctx, &alloc, &usrfnt);
    }

    device_init(&device);
    while (running) {
        /* Input */
        SDL_Event evt;
        zr_input_begin(&gui.ctx);
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_WINDOWEVENT) resize(&evt);
            else if (evt.type == SDL_QUIT) goto cleanup;
            else if (evt.type == SDL_KEYUP)
                input_key(&gui.ctx, &evt, zr_false);
            else if (evt.type == SDL_KEYDOWN)
                input_key(&gui.ctx, &evt, zr_true);
            else if (evt.type == SDL_MOUSEBUTTONDOWN)
                input_button(&gui.ctx, &evt, zr_true);
            else if (evt.type == SDL_MOUSEBUTTONUP)
                input_button(&gui.ctx, &evt, zr_false);
            else if (evt.type == SDL_MOUSEMOTION)
                input_motion(&gui.ctx, &evt);
            else if (evt.type == SDL_TEXTINPUT)
                input_text(&gui.ctx, &evt);
            else if (evt.type == SDL_MOUSEWHEEL)
                zr_input_scroll(&gui.ctx,(float)evt.wheel.y);
        }
        zr_input_end(&gui.ctx);

        /* GUI */
        SDL_GetWindowSize(win, &width, &height);
        running = run_demo(&gui);

        /* Draw */
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        device_draw(&device, &gui.ctx, width, height, ZR_ANTI_ALIASING_ON);
        SDL_GL_SwapWindow(win);
    }

cleanup:
    /* Cleanup */
    free(font.glyphs);
    zr_free(&gui.ctx);
    zr_buffer_free(&device.cmds);
    device_shutdown(&device);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

