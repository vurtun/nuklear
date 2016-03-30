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
#include <GLFW/glfw3.h>

/* macros */
#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

#include "../../zahnrad.h"
#include "../demo.c"

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wsign-conversion"
#pragma clang diagnostic ignored "-Wfloat-equal"
#pragma clang diagnostic ignored "-Wbad-function-cast"
#pragma clang diagnostic ignored "-Wcast-qual"
#pragma clang diagnostic ignored "-Wshadow"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wdeclaration-after-statement"
#pragma clang diagnostic ignored "-Wunused-function"
#pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wbad-function-cast"
#pragma GCC diagnostic ignored "-Wcast-qual"
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"
#pragma GCC diagnostic ignored "-Wtype-limits"
#pragma GCC diagnostic ignored "-Wswitch-default"
#pragma GCC diagnostic ignored "-Wunused-function"
#elif _MSC_VER
#pragma warning (push)
#pragma warning (disable: 4456)
#endif

#define STB_IMAGE_IMPLEMENTATION
#include "../stb_image.h"

#ifdef __clang__
#pragma clang diagnostic pop
#elif defined(__GNUC__) || defined(__GNUG__)
#pragma GCC diagnostic pop
#elif _MSC_VER
#pragma warning (pop)
#endif

static int mouse_pos_x = 0;
static int mouse_pos_y = 0;
static struct demo gui;

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

static struct zr_image
icon_load(const char *filename)
{
    int x,y,n;
    GLuint tex;
    unsigned char *data = stbi_load(filename, &x, &y, &n, 0);
    if (!data) die("[SDL]: failed to load image: %s", filename);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, x, y, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    stbi_image_free(data);
    return zr_image_id((int)tex);
}

struct device {
    struct zr_buffer cmds;
    struct zr_draw_null_texture null;
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

static void
device_upload_atlas(struct device *dev, const void *image, int width, int height)
{
    glGenTextures(1, &dev->font_tex);
    glBindTexture(GL_TEXTURE_2D, dev->font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image);
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
        void *vertices, *elements;
        const zr_draw_index *offset = NULL;

        /* allocate vertex and element buffer */
        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        glBufferData(GL_ARRAY_BUFFER, MAX_VERTEX_MEMORY, NULL, GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, MAX_ELEMENT_MEMORY, NULL, GL_STREAM_DRAW);

        /* load draw vertices & elements directly into vertex + element buffer */
        vertices = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        elements = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        {
            struct zr_buffer vbuf, ebuf;

            /* fill converting configuration */
            struct zr_convert_config config;
            memset(&config, 0, sizeof(config));
            config.global_alpha = 1.0f;
            config.shape_AA = AA;
            config.line_AA = AA;
            config.circle_segment_count = 22;
            config.curve_segment_count = 22;
            config.arc_segment_count = 22;
            config.null = dev->null;

            /* setup buffers to load vertices and elements */
            zr_buffer_init_fixed(&vbuf, vertices, MAX_VERTEX_MEMORY);
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
error_callback(int error, const char *description)
{
    fprintf(stderr, "Error %d: %s\n", error, description);
}

static void
input_key(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    int down = action == GLFW_PRESS;
    UNUSED(window);
    UNUSED(scancode);
    if (key == GLFW_KEY_RIGHT_SHIFT || key == GLFW_KEY_LEFT_SHIFT)
        zr_input_key(&gui.ctx, ZR_KEY_SHIFT, down);
    else if (key == GLFW_KEY_TAB)
        zr_input_key(&gui.ctx, ZR_KEY_TAB, down);
    else if (key == GLFW_KEY_DELETE)
        zr_input_key(&gui.ctx, ZR_KEY_DEL, down);
    else if (key == GLFW_KEY_ENTER)
        zr_input_key(&gui.ctx, ZR_KEY_ENTER, down);
    else if (key == GLFW_KEY_BACKSPACE)
        zr_input_key(&gui.ctx, ZR_KEY_BACKSPACE, down);
    else if (key == GLFW_KEY_LEFT)
        zr_input_key(&gui.ctx, ZR_KEY_LEFT, down);
    else if (key == GLFW_KEY_RIGHT)
        zr_input_key(&gui.ctx, ZR_KEY_RIGHT, down);
    else if (key == GLFW_KEY_C)
        zr_input_key(&gui.ctx, ZR_KEY_COPY, down && (mods & GLFW_MOD_CONTROL));
    else if (key == GLFW_KEY_V)
        zr_input_key(&gui.ctx, ZR_KEY_PASTE, down && (mods & GLFW_MOD_CONTROL));
    else if (key == GLFW_KEY_X)
        zr_input_key(&gui.ctx, ZR_KEY_CUT, down && (mods & GLFW_MOD_CONTROL));
}

static void
input_motion(GLFWwindow *window, double xpos, double ypos)
{
    const int x = (int)xpos;
    const int y = (int)ypos;
    UNUSED(window);
    mouse_pos_x = x;
    mouse_pos_y = y;
    zr_input_motion(&gui.ctx, x, y);
}

static void
input_button(GLFWwindow *window, int button, int action, int mods)
{
    int x = mouse_pos_x;
    int y = mouse_pos_y;
    UNUSED(window);
    UNUSED(mods);
    if (button == 0)
        zr_input_button(&gui.ctx, ZR_BUTTON_LEFT, x, y, action == GLFW_PRESS);
    if (button == 1)
        zr_input_button(&gui.ctx, ZR_BUTTON_RIGHT, x, y, action == GLFW_PRESS);
    if (button == 2)
        zr_input_button(&gui.ctx, ZR_BUTTON_MIDDLE, x, y, action == GLFW_PRESS);
}

static void
input_text(GLFWwindow *window, unsigned int codepoint)
{
    UNUSED(window);
    zr_input_unicode(&gui.ctx, codepoint);
}

static void
input_scroll(GLFWwindow *window, double xoffset, double yoffset)
{
    UNUSED(window);
    UNUSED(xoffset);
    zr_input_scroll(&gui.ctx, (float)yoffset);
}

int
main(int argc, char *argv[])
{
    /* Platform */
    int i;
    static GLFWwindow *win;
    const char *font_path;
    int width = 0, height = 0;
    int running = 1;

    /* GUI */
    struct device device;
    struct zr_font *font;
    struct zr_font_atlas atlas;
    font_path = (argc > 1) ? argv[1]: 0;

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
        fprintf(stdout, "[GFLW] failed to init!\n");
        return 1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    win = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "Demo", NULL, NULL);
    glfwMakeContextCurrent(win);

    glfwSetCursorPosCallback(win, input_motion);
    glfwSetMouseButtonCallback(win, input_button);
    glfwSetKeyCallback(win, input_key);
    glfwSetCharCallback(win, input_text);
    glfwSetScrollCallback(win, input_scroll);

    /* OpenGL */
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glewExperimental = 1;
    if (glewInit() != GLEW_OK)
        die("Failed to setup GLEW\n");

    device_init(&device);
    {
        /* Font */
        const void *image;
        int w, h;

        zr_font_atlas_init_default(&atlas);
        zr_font_atlas_begin(&atlas);
        if (font_path) font = zr_font_atlas_add_from_file(&atlas, font_path, 14.0f, NULL);
        else font = zr_font_atlas_add_default(&atlas, 14.0f, NULL);
        image = zr_font_atlas_bake(&atlas, &w, &h, ZR_FONT_ATLAS_RGBA32);
        device_upload_atlas(&device, image, w, h);
        zr_font_atlas_end(&atlas, zr_handle_id((int)device.font_tex), &device.null);

        /* GUI */
        memset(&gui, 0, sizeof(gui));
        zr_buffer_init_default(&device.cmds);
        zr_init_default(&gui.ctx, &font->handle);
    }

    /* icons */
    glEnable(GL_TEXTURE_2D);
    gui.icons.unchecked = icon_load("../../icon/unchecked.png");
    gui.icons.checked = icon_load("../../icon/checked.png");
    gui.icons.rocket = icon_load("../../icon/rocket.png");
    gui.icons.cloud = icon_load("../../icon/cloud.png");
    gui.icons.pen = icon_load("../../icon/pen.png");
    gui.icons.play = icon_load("../../icon/play.png");
    gui.icons.pause = icon_load("../../icon/pause.png");
    gui.icons.stop = icon_load("../../icon/stop.png");
    gui.icons.next =  icon_load("../../icon/next.png");
    gui.icons.prev =  icon_load("../../icon/prev.png");
    gui.icons.tools = icon_load("../../icon/tools.png");
    gui.icons.dir = icon_load("../../icon/directory.png");
    gui.icons.copy = icon_load("../../icon/copy.png");
    gui.icons.convert = icon_load("../../icon/export.png");
    gui.icons.del = icon_load("../../icon/delete.png");
    gui.icons.edit = icon_load("../../icon/edit.png");
    gui.icons.menu[0] = icon_load("../../icon/home.png");
    gui.icons.menu[1] = icon_load("../../icon/phone.png");
    gui.icons.menu[2] = icon_load("../../icon/plane.png");
    gui.icons.menu[3] = icon_load("../../icon/wifi.png");
    gui.icons.menu[4] = icon_load("../../icon/settings.png");
    gui.icons.menu[5] = icon_load("../../icon/volume.png");

    gui.icons.home = icon_load("../../icon/home.png");
    gui.icons.directory = icon_load("../../icon/directory.png");
    gui.icons.computer = icon_load("../../icon/computer.png");
    gui.icons.desktop = icon_load("../../icon/desktop.png");
    gui.icons.default_file = icon_load("../../icon/default.png");
    gui.icons.text_file = icon_load("../../icon/text.png");
    gui.icons.music_file = icon_load("../../icon/music.png");
    gui.icons.font_file =  icon_load("../../icon/font.png");
    gui.icons.img_file = icon_load("../../icon/img.png");
    gui.icons.movie_file = icon_load("../../icon/movie.png");

    for (i = 0; i < 9; ++i) {
        char buffer[256];
        sprintf(buffer, "../../images/image%d.png", (i+1));
        gui.icons.images[i] = icon_load(buffer);
    }

    while (!glfwWindowShouldClose(win) && running) {
        /* Input */
        zr_input_begin(&gui.ctx);
        glfwPollEvents();
        zr_input_end(&gui.ctx);

        /* GUI */
        glfwGetWindowSize(win, &width, &height);
        running = run_demo(&gui);

        /* Draw */
        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        device_draw(&device, &gui.ctx, width, height, ZR_ANTI_ALIASING_ON);
        glfwSwapBuffers(win);
    }

    /* Cleanup */
    glDeleteTextures(1,(const GLuint*)&gui.icons.unchecked.handle.id);
    glDeleteTextures(1,(const GLuint*)&gui.icons.checked.handle.id);
    glDeleteTextures(1,(const GLuint*)&gui.icons.rocket.handle.id);
    glDeleteTextures(1,(const GLuint*)&gui.icons.cloud.handle.id);
    glDeleteTextures(1,(const GLuint*)&gui.icons.pen.handle.id);
    glDeleteTextures(1,(const GLuint*)&gui.icons.play.handle.id);
    glDeleteTextures(1,(const GLuint*)&gui.icons.pause.handle.id);
    glDeleteTextures(1,(const GLuint*)&gui.icons.stop.handle.id);
    glDeleteTextures(1,(const GLuint*)&gui.icons.next.handle.id);
    glDeleteTextures(1,(const GLuint*)&gui.icons.prev.handle.id);
    glDeleteTextures(1,(const GLuint*)&gui.icons.tools.handle.id);
    glDeleteTextures(1,(const GLuint*)&gui.icons.dir.handle.id);
    glDeleteTextures(1,(const GLuint*)&gui.icons.del.handle.id);

    glDeleteTextures(1,(const GLuint*)&gui.icons.home.handle.id);
    glDeleteTextures(1,(const GLuint*)&gui.icons.directory.handle.id);
    glDeleteTextures(1,(const GLuint*)&gui.icons.computer.handle.id);
    glDeleteTextures(1,(const GLuint*)&gui.icons.desktop.handle.id);
    glDeleteTextures(1,(const GLuint*)&gui.icons.default_file.handle.id);
    glDeleteTextures(1,(const GLuint*)&gui.icons.text_file.handle.id);
    glDeleteTextures(1,(const GLuint*)&gui.icons.music_file.handle.id);
    glDeleteTextures(1,(const GLuint*)&gui.icons.font_file.handle.id);
    glDeleteTextures(1,(const GLuint*)&gui.icons.img_file.handle.id);
    glDeleteTextures(1,(const GLuint*)&gui.icons.movie_file.handle.id);

    for (i = 0; i < 9; ++i)
        glDeleteTextures(1, (const GLuint*)&gui.icons.images[i].handle.id);
    for (i = 0; i < 6; ++i)
        glDeleteTextures(1, (const GLuint*)&gui.icons.menu[i].handle.id);

    zr_font_atlas_clear(&atlas);
    zr_free(&gui.ctx);
    zr_buffer_free(&device.cmds);
    device_shutdown(&device);
    glfwTerminate();
    return 0;
}

