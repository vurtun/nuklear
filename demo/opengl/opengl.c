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
#include <math.h>
#include <assert.h>
#include <math.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>

/* macros */
#define DTIME       33
#define MAX_DRAW_COMMAND_MEMORY (4 * 1024)

#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#define MAX(a,b)    ((a) < (b) ? (b) : (a))
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#define LEN(a)      (sizeof(a)/sizeof(a)[0])
#define UNUSED(a)   ((void)(a))

#include "../../zahnrad.h"

static void clipboard_set(const char *text){SDL_SetClipboardText(text);}
static zr_bool clipboard_is_filled(void) {return SDL_HasClipboardText();}
static const char* clipboard_get(void){return SDL_GetClipboardText();}

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
    const char *path, unsigned int font_height, const zr_long *range)
{
    zr_size glyph_count;
    zr_size img_width, img_height;
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
        zr_size tmp_size, img_size;
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
        config.size = (zr_float)font_height;
        config.spacing = zr_vec2(0,0);
        config.oversample_h = 1;
        config.oversample_v = 1;

        /* query needed amount of memory for the font baking process */
        zr_font_bake_memory(&tmp_size, &glyph_count, &config, 1);
        glyphes = (struct zr_font_glyph*)calloc(sizeof(struct zr_font_glyph), glyph_count);
        tmp = calloc(1, tmp_size);

        /* pack all glyphes and return needed image width height and memory size*/
        custom.w = 2; custom.h = 2;
        if (!zr_font_bake_pack(&img_size, &img_width,&img_height,&custom,tmp,tmp_size,&config, 1))
            die("[Font]: failed to load font!\n");

        /* bake all glyphes and custom white pixel into image */
        img = calloc(1, img_size);
        zr_font_bake(img, img_width, img_height, tmp, tmp_size, glyphes, glyph_count, &config, 1);
        zr_font_bake_custom_data(img, img_width, img_height, custom, custom_data, 2, 2, '.', 'X');
        {
            /* convert alpha8 image into rgba8 image */
            void *img_rgba = calloc(4, img_height * img_width);
            zr_font_bake_convert(img_rgba, (zr_ushort)img_width, (zr_ushort)img_height, img);
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
    dev->null.texture.id = (zr_int)dev->font_tex;
    dev->null.uv = zr_vec2((custom.x + 0.5f)/(zr_float)img_width,
                            (custom.y + 0.5f)/(zr_float)img_height);
    /* setup font with glyphes. IMPORTANT: the font only references the glyphes
      this was done to have the possibility to have multible fonts with one
      total glyph array. Not quite sure if it is a good thing since the
      glyphes have to be freed as well. */
    zr_font_init(font, (zr_float)font_height, '?', glyphes, &baked_font, dev->null.texture);
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
}

/* this is stupid but needed for C89 since sinf and cosf do not exist */
static zr_float fsin(zr_float f) {return (zr_float)sin(f);}
static zr_float fcos(zr_float f) {return (zr_float)cos(f);}

static void
device_draw(struct device *dev, struct zr_command_queue *queue, int width, int height)
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
    glViewport(0, 0, width, height);
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
        struct zr_draw_list draw_list;
        const struct zr_draw_command *cmd;
        static const GLsizeiptr max_vertex_memory = 128 * 1024;
        static const GLsizeiptr max_element_memory = 32 * 1024;
        void *vertexes, *elements;
        const zr_draw_index *offset = NULL;

        /* allocate vertex and element buffer */
        memset(&draw_list, 0, sizeof(draw_list));
        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        glBufferData(GL_ARRAY_BUFFER, max_vertex_memory, NULL, GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, max_element_memory, NULL, GL_STREAM_DRAW);

        /* load draw vertexes & elements directly into vertex + element buffer */
        vertexes = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        elements = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        {
            struct zr_buffer vbuf, ebuf;
            zr_buffer_init_fixed(&vbuf, vertexes, (zr_size)max_vertex_memory);
            zr_buffer_init_fixed(&ebuf, elements, (zr_size)max_element_memory);
            zr_draw_list_init(&draw_list, &dev->cmds, &vbuf, &ebuf,
                fsin, fcos, dev->null, ZR_ANTI_ALIASING_ON);
            zr_draw_list_load(&draw_list, queue, 1.0f, 22);
        }
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

        /* iterate and execute each draw command */
        zr_foreach_draw_command(cmd, &draw_list) {
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor((GLint)cmd->clip_rect.x,
                height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h),
                (GLint)cmd->clip_rect.w, (GLint)cmd->clip_rect.h);
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
            offset += cmd->elem_count;
        }

        zr_command_queue_clear(queue);
        zr_draw_list_clear(&draw_list);
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
    if (evt->button.button == SDL_BUTTON_RIGHT)
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


static void* mem_alloc(zr_handle unused, zr_size size)
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

    /* GUI */
    struct zr_allocator alloc;
    struct device device;
    struct demo_gui gui;
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

    /* GUI */
    alloc.userdata.ptr = NULL;
    alloc.alloc = mem_alloc;
    alloc.free = mem_free;
    memset(&gui, 0, sizeof gui);
    zr_buffer_init(&device.cmds, &alloc, 1024, 2.0f);
    zr_command_queue_init(&gui.queue, &alloc, 1024, 2.0f);
    gui.font = font_bake_and_upload(&device, &font, font_path, 14,
                                    zr_font_default_glyph_ranges());

    init_demo(&gui);
    device_init(&device);

    while (gui.running) {
        /* Input */
        SDL_Event evt;
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
        gui.w = (zr_size)width;
        gui.h = (zr_size)height;
        run_demo(&gui);

        /* Draw */
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
        device_draw(&device, &gui.queue, width, height);
        SDL_GL_SwapWindow(win);
    }

cleanup:
    /* Cleanup */
    free(font.glyphes);
    zr_command_queue_free(&gui.queue);
    zr_buffer_free(&device.cmds);
    device_shutdown(&device);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

