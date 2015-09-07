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
#include <math.h>
#include <assert.h>
#include <math.h>

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

/* macros */
#define DTIME       17
#define FONT_ATLAS_DEPTH 4

#define MAX_DRAW_COMMAND_MEMORY (4 * 1024)

#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#define MAX(a,b)    ((a) < (b) ? (b) : (a))
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#define LEN(a)      (sizeof(a)/sizeof(a)[0])
#define UNUSED(a)   ((void)(a))

#include "../gui.h"

static void clipboard_set(const char *text){SDL_SetClipboardText(text);}
static gui_bool clipboard_is_filled(void) {return SDL_HasClipboardText();}
static const char* clipboard_get(void){return SDL_GetClipboardText();}

#include "demo.c"

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

/* ==============================================================
 *
 *                      Font
 *
 * ===============================================================*/
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
font_get_text_width(gui_handle handle, const gui_char *t, gui_size l)
{
    long unicode;
    size_t text_width = 0;
    const struct font_glyph *glyph;
    size_t text_len = 0;
    size_t glyph_len;
    struct font *font = (struct font*)handle.ptr;
    assert(font);
    if (!t || !l) return 0;

    glyph_len = gui_utf_decode(t, &unicode, l);
    while (text_len < l && glyph_len) {
        if (unicode == GUI_UTF_INVALID) return 0;
        glyph = (unicode < font->glyph_count) ? &font->glyphes[unicode] : font->fallback;
        glyph = (glyph->code == 0) ? font->fallback : glyph;

        text_len += glyph_len;
        text_width += (gui_size)((float)glyph->xadvance * font->scale);
        glyph_len = gui_utf_decode(t + text_len, &unicode, l - text_len);
    }
    return text_width;
}

static void
font_query_font_glyph(gui_handle handle, struct gui_user_font_glyph *glyph,
    gui_long unicode, gui_long next_codepoint)
{
    const struct font_glyph *g;
    const struct font *font = (struct font*)handle.ptr;
    UNUSED(next_codepoint);
    if (unicode == GUI_UTF_INVALID) return;
    g = (unicode < font->glyph_count) ?
        &font->glyphes[unicode] :
        font->fallback;
    g = (g->code == 0) ? font->fallback : g;

    glyph->offset.x = g->xoff * font->scale;
    glyph->offset.y = g->yoff * font->scale;
    glyph->width = g->width * font->scale;
    glyph->height = g->height * font->scale;
    glyph->xadvance = g->xadvance * font->scale;
    glyph->uv[0] = gui_vec2(g->uv[0].u, g->uv[0].v);
    glyph->uv[1] = gui_vec2(g->uv[1].u, g->uv[1].v);
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
    struct font *font = (struct font*)calloc(sizeof(struct font), 1);

    atlas.dim = dim;
    atlas.range = range;
    atlas.size = atlas.dim * atlas.dim * FONT_ATLAS_DEPTH;
    atlas.memory = (gui_byte*)calloc((gui_size)atlas.size, 1);

    font->glyph_count = (unsigned int)atlas.range;
    font->glyphes = (struct font_glyph*)calloc(atlas.range, sizeof(struct font_glyph));
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

/* ==============================================================
 *
 *                      Draw
 *
 * ===============================================================*/
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
    GLuint null_tex;
    struct gui_draw_list draw_list;
    struct gui_draw_null_texture null;
    struct gui_buffer cmds;
};

#define glerror() glerror_(__FILE__, __LINE__)
static void
glerror_(const char *file, int line)
{
    const GLenum code = glGetError();
    if (code == GL_INVALID_ENUM)
        fprintf(stdout, "[GL] Error: (%s:%d) invalid value!\n", file, line);
    else if (code == GL_INVALID_OPERATION)
        fprintf(stdout, "[GL] Error: (%s:%d) invalid operation!\n", file, line);
    else if (code == GL_INVALID_FRAMEBUFFER_OPERATION)
        fprintf(stdout, "[GL] Error: (%s:%d) invalid frame op!\n", file, line);
    else if (code == GL_OUT_OF_MEMORY)
        fprintf(stdout, "[GL] Error: (%s:%d) out of memory!\n", file, line);
    else if (code == GL_STACK_UNDERFLOW)
        fprintf(stdout, "[GL] Error: (%s:%d) stack underflow!\n", file, line);
    else if (code == GL_STACK_OVERFLOW)
        fprintf(stdout, "[GL] Error: (%s:%d) stack overflow!\n", file, line);
}

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
    glerror();


    {
        /* buffer setup */
        size_t vs = sizeof(struct gui_draw_vertex);
        size_t vp = offsetof(struct gui_draw_vertex, position);
        size_t vt = offsetof(struct gui_draw_vertex, uv);
        size_t vc = offsetof(struct gui_draw_vertex, col);

        glGenBuffers(1, &dev->vbo);
        glGenBuffers(1, &dev->ebo);
        glGenVertexArrays(1, &dev->vao);

        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);
        glerror();

        glEnableVertexAttribArray((GLuint)dev->attrib_pos);
        glEnableVertexAttribArray((GLuint)dev->attrib_uv);
        glEnableVertexAttribArray((GLuint)dev->attrib_col);

        glVertexAttribPointer((GLuint)dev->attrib_pos, 2, GL_FLOAT, GL_FALSE, vs, (void*)vp);
        glVertexAttribPointer((GLuint)dev->attrib_uv, 2, GL_FLOAT, GL_FALSE, vs, (void*)vt);
        glVertexAttribPointer((GLuint)dev->attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, vs, (void*)vc);
        glerror();
    }

    {
        /* create default white texture which is needed to draw primitives. Any
         texture with a white pixel would do but since I do not have one I have
         to create one. */
        void *mem = calloc(64*64*4, 1);
        memset(mem, 255, 64*64*4);
        glGenTextures(1, &dev->null_tex);
        glBindTexture(GL_TEXTURE_2D, dev->null_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 64, 64, 0,
            GL_RGBA, GL_UNSIGNED_BYTE, mem);
        free(mem);
        glerror();
    }
    dev->null.texture.id = (gui_int)dev->null_tex;
    dev->null.uv = gui_vec2(0.0f, 0.0f);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glerror();
}

static void
device_shutdown(struct device *dev)
{
    glDetachShader(dev->prog, dev->vert_shdr);
    glDetachShader(dev->prog, dev->frag_shdr);
    glDeleteShader(dev->vert_shdr);
    glDeleteShader(dev->frag_shdr);
    glDeleteProgram(dev->prog);
    glDeleteTextures(1, &dev->null_tex);
    glDeleteBuffers(1, &dev->vbo);
    glDeleteBuffers(1, &dev->ebo);
}

static void
device_draw(struct device *dev, struct gui_command_queue *queue, int width, int height)
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
    glerror();

    /* setup global state */
    glViewport(0, 0, width, height);
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);
    glerror();

    /* setup program */
    glUseProgram(dev->prog);
    glUniform1i(dev->uniform_tex, 0);
    glUniformMatrix4fv(dev->uniform_proj, 1, GL_FALSE, &ortho[0][0]);
    glerror();

    {
        /* convert from command queue into draw list and draw to screen */
        struct gui_draw_list draw_list;
        const struct gui_draw_command *cmd;
        static const GLsizeiptr max_vertex_memory = 128 * 1024;
        static const GLsizeiptr max_element_memory = 32 * 1024;
        void *vertexes, *elements;
        const gui_draw_index *offset = NULL;

        /* allocate vertex and element buffer */
        memset(&draw_list, 0, sizeof(draw_list));
        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);
        glBufferData(GL_ARRAY_BUFFER, max_vertex_memory, NULL, GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, max_element_memory, NULL, GL_STREAM_DRAW);
        glerror();

        /* load draw vertexes & elements directly into vertex + element buffer */
        vertexes = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        elements = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        {
            struct gui_buffer vbuf, ebuf;
            gui_buffer_init_fixed(&vbuf, vertexes, (gui_size)max_vertex_memory);
            gui_buffer_init_fixed(&ebuf, elements, (gui_size)max_element_memory);
            gui_draw_list_init(&draw_list, &dev->cmds, &vbuf, &ebuf,
                sinf, cosf, dev->null, GUI_ANTI_ALIASING_ON);
            gui_draw_list_load(&draw_list, queue, 1.0f, 22);
        }
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);
        glerror();

        /* iterate and execute each draw command */
        gui_foreach_draw_command(cmd, &draw_list) {
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor((GLint)cmd->clip_rect.x,
                height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h),
                (GLint)cmd->clip_rect.w, (GLint)cmd->clip_rect.h);
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
            offset += cmd->elem_count;
            glerror();
        }

        gui_command_queue_clear(queue);
        gui_draw_list_clear(&draw_list);
    }

    /* restore old state */
    glUseProgram((GLuint)last_prog);
    glBindTexture(GL_TEXTURE_2D, (GLuint)last_tex);
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint)last_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, (GLuint)last_ebo);
    glBindVertexArray((GLuint)last_vao);
    glDisable(GL_SCISSOR_TEST);
    glerror();
}

/* ==============================================================
 *
 *                      APP
 *
 * ===============================================================*/
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
        gui_input_button(in, GUI_BUTTON_LEFT, x, y, down);
    if (evt->button.button == SDL_BUTTON_RIGHT)
        gui_input_button(in, GUI_BUTTON_RIGHT, x, y, down);
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
    void *mem;
    const char *font_path;
    SDL_Window *win;
    SDL_GLContext glContext;
    struct font *glfont;
    int win_width, win_height;
    unsigned int started;
    unsigned int dt;
    int width = 0, height = 0;

    /* GUI */
    struct device device;
    struct demo_gui gui;
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
    glfont = font_new(font_path, 10, 10, 255, FONT_ATLAS_DIM_256);
    glewExperimental = 1;
    if (glewInit() != GLEW_OK)
        die("Failed to setup GLEW\n");

    /* GUI */
    memset(&gui, 0, sizeof gui);
    gui_buffer_init_fixed(&gui.memory, calloc(MAX_MEMORY, 1), MAX_MEMORY);
    gui.font.userdata.ptr = glfont;
    gui.font.height = glfont->height;
    gui.font.width = font_get_text_width;
    gui.font.query = font_query_font_glyph;
    gui.font.texture.id = (gui_int)glfont->texture;
    mem = gui_buffer_alloc(&gui.memory, GUI_BUFFER_FRONT, MAX_DRAW_COMMAND_MEMORY, 0);
    gui_buffer_init_fixed(&device.cmds, mem, MAX_DRAW_COMMAND_MEMORY);

    init_demo(&gui);
    device_init(&device);

    while (gui.running) {
        /* Input */
        SDL_Event evt;
        started = SDL_GetTicks();
        gui_input_begin(&gui.input);
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_WINDOWEVENT) resize(&evt);
            else if (evt.type == SDL_QUIT) goto cleanup;
            else if (evt.type == SDL_KEYUP) key(&gui.input, &evt, gui_false);
            else if (evt.type == SDL_KEYDOWN) key(&gui.input, &evt, gui_true);
            else if (evt.type == SDL_MOUSEBUTTONDOWN) btn(&gui.input, &evt, gui_true);
            else if (evt.type == SDL_MOUSEBUTTONUP) btn(&gui.input, &evt, gui_false);
            else if (evt.type == SDL_MOUSEMOTION) motion(&gui.input, &evt);
            else if (evt.type == SDL_TEXTINPUT) text(&gui.input, &evt);
            else if (evt.type == SDL_MOUSEWHEEL)
                gui_input_scroll(&gui.input,(float)evt.wheel.y);
        }
        gui_input_end(&gui.input);

        /* GUI */
        SDL_GetWindowSize(win, &width, &height);
        gui.w = (gui_size)width;
        gui.h = (gui_size)height;
        run_demo(&gui);

        /* Draw */
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
        device_draw(&device, &gui.queue, width, height);
        SDL_GL_SwapWindow(win);

        /* Timing */
        dt = SDL_GetTicks() - started;
        if (dt < DTIME)
            SDL_Delay(DTIME - dt);
    }

cleanup:
    /* Cleanup */
    free(gui_buffer_memory(&gui.memory));
    font_del(glfont);
    device_shutdown(&device);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

