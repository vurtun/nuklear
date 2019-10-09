/*
 * Nuklear - 1.32.0 - public domain
 * no warrenty implied; use at your own risk.
 * authored from 2015-2016 by Micha Mettke
 */
/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */
#ifndef NK_SFML_GL3_H_
#define NK_SFML_GL3_H_

/* Feel free to edit here and include your own extension wrangler */
#include <glad/glad.h>
/* I use GLAD but you can use GLEW or what you like */

#include <SFML/Window.hpp>

NK_API struct nk_context*   nk_sfml_init(sf::Window* window);
NK_API void                 nk_sfml_font_stash_begin(struct nk_font_atlas** atlas);
NK_API void                 nk_sfml_font_stash_end(void);
NK_API int                  nk_sfml_handle_event(sf::Event* event);
NK_API void                 nk_sfml_render(enum nk_anti_aliasing, int max_vertex_buffer, int max_element_buffer);
NK_API void                 nk_sfml_shutdown(void);

NK_API void                 nk_sfml_device_create(void);
NK_API void                 nk_sfml_device_destroy(void);

#endif
/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
 #ifdef NK_SFML_GL3_IMPLEMENTATION

#include <string>

struct nk_sfml_device {
    struct nk_buffer cmds;
    struct nk_draw_null_texture null;
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
struct nk_sfml_vertex {
    float position[2];
    float uv[2];
    nk_byte col[4];
};
static struct nk_sfml {
    sf::Window* window;
    struct nk_sfml_device ogl;
    struct nk_context ctx;
    struct nk_font_atlas atlas;
} sfml;

#ifdef __APPLE__
  #define NK_SHADER_VERSION "#version 150\n"
#else
  #define NK_SHADER_VERSION "#version 300 es\n"
#endif

NK_API void
nk_sfml_device_create(void)
{
    GLint status;
    static const GLchar* vertex_shader =
        NK_SHADER_VERSION
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
        NK_SHADER_VERSION
        "precision mediump float;\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main(){\n"
        "   Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    struct nk_sfml_device* dev = &sfml.ogl;
    nk_buffer_init_default(&dev->cmds);

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
        GLsizei vs = sizeof(struct nk_sfml_vertex);
        size_t vp = NK_OFFSETOF(struct nk_sfml_vertex, position);
        size_t vt = NK_OFFSETOF(struct nk_sfml_vertex, uv);
        size_t vc = NK_OFFSETOF(struct nk_sfml_vertex, col);

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

NK_API void
nk_sfml_device_destroy(void)
{
    struct nk_sfml_device* dev = &sfml.ogl;

    glDetachShader(dev->prog, dev->vert_shdr);
    glDetachShader(dev->prog, dev->frag_shdr);
    glDeleteShader(dev->vert_shdr);
    glDeleteShader(dev->vert_shdr);
    glDeleteProgram(dev->prog);
    glDeleteTextures(1, &dev->font_tex);
    glDeleteBuffers(1, &dev->vbo);
    glDeleteBuffers(1, &dev->ebo);
    nk_buffer_free(&dev->cmds);
}

NK_INTERN void
nk_sfml_device_upload_atlas(const void* image, int width, int height)
{
    struct nk_sfml_device* dev = &sfml.ogl;
    glGenTextures(1, &dev->font_tex);
    glBindTexture(GL_TEXTURE_2D, dev->font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image);
}

NK_API void
nk_sfml_render(enum nk_anti_aliasing AA, int max_vertex_buffer, int max_element_buffer)
{
    /* setup global state */
    struct nk_sfml_device* dev = &sfml.ogl;
    int window_width = sfml.window->getSize().x;
    int window_height = sfml.window->getSize().y;
    GLfloat ortho[4][4] = {
        {2.0f, 0.0f, 0.0f, 0.0f},
        {0.0f,-2.0f, 0.0f, 0.0f},
        {0.0f, 0.0f,-1.0f, 0.0f},
        {-1.0f,1.0f, 0.0f, 1.0f},
    };
    ortho[0][0] /= (GLfloat)window_width;
    ortho[1][1] /= (GLfloat)window_height;

    glViewport(0, 0, window_width, window_height);
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
        const struct nk_draw_command *cmd;
        void *vertices, *elements;
        const nk_draw_index *offset = NULL;

        /* allocate vertex and element buffer */
        glBindVertexArray(dev->vao);
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);

        glBufferData(GL_ARRAY_BUFFER, max_vertex_buffer, NULL, GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, max_element_buffer, NULL, GL_STREAM_DRAW);

        /* load vertices/elements directly into vertex/element buffer */
        vertices = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        elements = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        {
            /* fill convert configuration */
            struct nk_convert_config config;
            static const struct nk_draw_vertex_layout_element vertex_layout[] =  {
                {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_sfml_vertex, position)},
                {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_sfml_vertex, uv)},
                {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_sfml_vertex, col)},
                {NK_VERTEX_LAYOUT_END}
            };

            NK_MEMSET(&config, 0, sizeof(config));
            config.vertex_layout = vertex_layout;
            config.vertex_size = sizeof(struct nk_sfml_vertex);
            config.vertex_alignment = NK_ALIGNOF(struct nk_sfml_vertex);
            config.null = dev->null;
            config.circle_segment_count = 22;
            config.curve_segment_count = 22;
            config.arc_segment_count = 22;
            config.global_alpha = 1.0f;
            config.shape_AA = AA;
            config.line_AA = AA;

            /* setup buffers to load vertices and elements */
            struct nk_buffer vbuf, ebuf;
            nk_buffer_init_fixed(&vbuf, vertices, (nk_size)max_vertex_buffer);
            nk_buffer_init_fixed(&ebuf, elements, (nk_size)max_element_buffer);
            nk_convert(&sfml.ctx, &dev->cmds, &vbuf, &ebuf, &config);
        }
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

        /* iterate over and execute each draw command */
        nk_draw_foreach(cmd, &sfml.ctx, &dev->cmds)
        {
            if (!cmd->elem_count) continue;
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor(
                (GLint)(cmd->clip_rect.x),
                (GLint)((window_height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h))),
                (GLint)(cmd->clip_rect.w),
                (GLint)(cmd->clip_rect.h));
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
            offset += cmd->elem_count;
        }
        nk_clear(&sfml.ctx);
    }
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
}

static void
nk_sfml_clipboard_paste(nk_handle usr, struct nk_text_edit* edit)
{
#if 0
    /* Not Implemented in SFML */
    (void)usr;
    sf::Clipboard clipboard(sfml.window);
    const char* text = clipboard.getText();
    if(text) nk_textedit_paste(edit, text, nk_strlen(text));
#endif
}
static void
nk_sfml_clipboard_copy(nk_handle usr, const char* text, int len)
{
#if 0
    char* str = 0;
    (void)usr;
    if(!len) return;
    str = (char*)malloc((size_t)len+1);
    if(!str) return;
    memcpy(str, text, (size_t)len);
    str[len] = '\0';

    /* Not Implemented in SFML */
    sf::Clipboard clipboard(sfml.window);
    clipboard.setText(str);
    free(str);
#endif
}

NK_API struct nk_context*
nk_sfml_init(sf::Window* window)
{
    sfml.window = window;
    nk_init_default(&sfml.ctx, 0);
    sfml.ctx.clip.copy = nk_sfml_clipboard_copy;
    sfml.ctx.clip.paste = nk_sfml_clipboard_paste;
    sfml.ctx.clip.userdata = nk_handle_ptr(0);
    nk_sfml_device_create();
    return &sfml.ctx;
}

NK_API void
nk_sfml_font_stash_begin(struct nk_font_atlas** atlas)
{
    nk_font_atlas_init_default(&sfml.atlas);
    nk_font_atlas_begin(&sfml.atlas);
    *atlas = &sfml.atlas;
}

NK_API void
nk_sfml_font_stash_end()
{
    const void* image;
    int w, h;
    image = nk_font_atlas_bake(&sfml.atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    nk_sfml_device_upload_atlas(image, w, h);
    nk_font_atlas_end(&sfml.atlas, nk_handle_id((int)sfml.ogl.font_tex), &sfml.ogl.null);
    if(sfml.atlas.default_font)
        nk_style_set_font(&sfml.ctx, &sfml.atlas.default_font->handle);
}

NK_API int
nk_sfml_handle_event(sf::Event* evt)
{
    struct nk_context* ctx = &sfml.ctx;
    /* optional grabbing behavior */
    if(ctx->input.mouse.grab)
        ctx->input.mouse.grab = 0;
    else if(ctx->input.mouse.ungrab) {
        int x = (int)ctx->input.mouse.prev.x;
        int y = (int)ctx->input.mouse.prev.y;
        sf::Mouse::setPosition(sf::Vector2i(x, y), *sfml.window);
        ctx->input.mouse.ungrab = 0;
    }
    if(evt->type == sf::Event::KeyReleased || evt->type == sf::Event::KeyPressed)
    {
        int down = evt->type == sf::Event::KeyPressed;
        sf::Keyboard::Key key = evt->key.code;
        if(key == sf::Keyboard::RShift || key == sf::Keyboard::LShift)
            nk_input_key(ctx, NK_KEY_SHIFT, down);
        else if(key == sf::Keyboard::Delete)
            nk_input_key(ctx, NK_KEY_DEL, down);
        else if(key == sf::Keyboard::Return)
            nk_input_key(ctx, NK_KEY_ENTER, down);
        else if(key == sf::Keyboard::Tab)
            nk_input_key(ctx, NK_KEY_TAB, down);
        else if(key == sf::Keyboard::BackSpace)
            nk_input_key(ctx, NK_KEY_BACKSPACE, down);
        else if(key == sf::Keyboard::Home) {
            nk_input_key(ctx, NK_KEY_TEXT_START, down);
            nk_input_key(ctx, NK_KEY_SCROLL_START, down);
        } else if(key == sf::Keyboard::End) {
            nk_input_key(ctx, NK_KEY_TEXT_END, down);
            nk_input_key(ctx, NK_KEY_SCROLL_END, down);
        } else if(key == sf::Keyboard::PageDown)
            nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
        else if(key == sf::Keyboard::PageUp)
            nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
        else if(key == sf::Keyboard::Z)
            nk_input_key(ctx, NK_KEY_TEXT_UNDO, down && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl));
        else if(key == sf::Keyboard::R)
            nk_input_key(ctx, NK_KEY_TEXT_REDO, down && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl));
        else if(key == sf::Keyboard::C)
            nk_input_key(ctx, NK_KEY_COPY, down && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl));
        else if(key == sf::Keyboard::V)
            nk_input_key(ctx, NK_KEY_PASTE, down && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl));
        else if(key == sf::Keyboard::X)
            nk_input_key(ctx, NK_KEY_CUT, down && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl));
        else if(key == sf::Keyboard::B)
            nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl));
        else if(key == sf::Keyboard::E)
            nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl));
        else if(key == sf::Keyboard::Up)
            nk_input_key(ctx, NK_KEY_UP, down);
        else if(key == sf::Keyboard::Down)
            nk_input_key(ctx, NK_KEY_DOWN, down);
        else if(key == sf::Keyboard::Left) {
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
                nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
            else nk_input_key(ctx, NK_KEY_LEFT, down);
        } else if(key == sf::Keyboard::Right) {
            if(sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
                nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
            else nk_input_key(ctx, NK_KEY_RIGHT, down);
        } else return 0;
        return 1;
    } else if(evt->type == sf::Event::MouseButtonPressed || evt->type == sf::Event::MouseButtonReleased) {
        int down = evt->type == sf::Event::MouseButtonPressed;
        const int x = evt->mouseButton.x, y = evt->mouseButton.y;
        if(evt->mouseButton.button == sf::Mouse::Left)
            nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
        if(evt->mouseButton.button == sf::Mouse::Middle)
            nk_input_button(ctx, NK_BUTTON_MIDDLE, x, y, down);
        if(evt->mouseButton.button == sf::Mouse::Right)
            nk_input_button(ctx, NK_BUTTON_RIGHT, x, y, down);
        else return 0;
        return 1;
    } else if(evt->type == sf::Event::MouseMoved) {
        nk_input_motion(ctx, evt->mouseMove.x, evt->mouseMove.y);
        return 1;
    } else if(evt->type == sf::Event::TouchBegan || evt->type == sf::Event::TouchEnded) {
        int down = evt->type == sf::Event::TouchBegan;
        const int x = evt->touch.x, y = evt->touch.y;
        nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
        return 1;
    } else if(evt->type == sf::Event::TouchMoved) {
        if(ctx->input.mouse.grabbed) {
            int x = (int)ctx->input.mouse.prev.x;
            int y = (int)ctx->input.mouse.prev.y;
            nk_input_motion(ctx, x + evt->touch.x, y + evt->touch.y);
        } else nk_input_motion(ctx, evt->touch.x, evt->touch.y);
        return 1;
    } else if(evt->type == sf::Event::TextEntered) {
        nk_input_unicode(ctx, evt->text.unicode);
        return 1;
    } else if(evt->type == sf::Event::MouseWheelScrolled) {
        nk_input_scroll(ctx, nk_vec2(0,evt->mouseWheelScroll.delta));
        return 1;
    }
    return 0;
}

NK_API
void nk_sfml_shutdown()
{
    nk_font_atlas_clear(&sfml.atlas);
    nk_free(&sfml.ctx);
    nk_sfml_device_destroy();
    memset(&sfml, 0, sizeof(sfml));
}

#endif
