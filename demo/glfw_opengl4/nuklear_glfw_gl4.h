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
#ifndef NK_GLFW_GL4_H_
#define NK_GLFW_GL4_H_

#include <string.h>
#include <GLFW/glfw3.h>

enum nk_glfw_init_state{
    NK_GLFW3_DEFAULT = 0,
    NK_GLFW3_INSTALL_CALLBACKS
};

NK_API struct nk_context*   nk_glfw3_init(GLFWwindow *win, enum nk_glfw_init_state, int max_vertex_buffer, int max_element_buffer);
NK_API void                 nk_glfw3_shutdown(void);
NK_API void                 nk_glfw3_font_stash_begin(struct nk_font_atlas **atlas);
NK_API void                 nk_glfw3_font_stash_end(void);
NK_API void                 nk_glfw3_new_frame(void);
NK_API void                 nk_glfw3_render(enum nk_anti_aliasing);

NK_API void                 nk_glfw3_device_destroy(void);
NK_API void                 nk_glfw3_device_create(void);

NK_API void                 nk_glfw3_char_callback(GLFWwindow *win, unsigned int codepoint);
NK_API void                 nk_gflw3_scroll_callback(GLFWwindow *win, double xoff, double yoff);
NK_API void                 nk_glfw3_mouse_button_callback(GLFWwindow *win, int button, int action, int mods);

NK_API GLuint               nk_glfw3_get_tex_ogl_id(int tex_index);
NK_API GLuint64             nk_glfw3_get_tex_ogl_handle(int tex_index);
NK_API int                  nk_glfw3_create_texture(const void* image, int width, int height);
NK_API void                 nk_glfw3_destroy_texture(int tex_index);

#endif
/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#ifdef NK_GLFW_GL4_IMPLEMENTATION
#undef NK_GLFW_GL4_IMPLEMENTATION

#ifndef NK_GLFW_TEXT_MAX
#define NK_GLFW_TEXT_MAX 256
#endif
#ifndef NK_GLFW_DOUBLE_CLICK_LO
#define NK_GLFW_DOUBLE_CLICK_LO 0.02
#endif
#ifndef NK_GLFW_DOUBLE_CLICK_HI
#define NK_GLFW_DOUBLE_CLICK_HI 0.2
#endif
#ifndef NK_GLFW_MAX_TEXTURES
#define NK_GLFW_MAX_TEXTURES 256
#endif

struct nk_glfw_vertex {
    float position[2];
    float uv[2];
    nk_byte col[4];
};

struct nk_glfw_device {
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
    int font_tex_index;
    int max_vertex_buffer;
    int max_element_buffer;
    struct nk_glfw_vertex *vert_buffer;
    int *elem_buffer;
    GLsync buffer_sync;
    GLuint tex_ids[NK_GLFW_MAX_TEXTURES];
    GLuint64 tex_handles[NK_GLFW_MAX_TEXTURES];
};

static struct nk_glfw {
    GLFWwindow *win;
    int width, height;
    int display_width, display_height;
    struct nk_glfw_device ogl;
    struct nk_context ctx;
    struct nk_font_atlas atlas;
    struct nk_vec2 fb_scale;
    unsigned int text[NK_GLFW_TEXT_MAX];
    int text_len;
    struct nk_vec2 scroll;
    double last_button_click;
    int is_double_click_down;
    struct nk_vec2 double_click_pos;
} glfw;

#define NK_SHADER_VERSION "#version 450 core\n"
#define NK_SHADER_BINDLESS "#extension GL_ARB_bindless_texture : require\n"
#define NK_SHADER_64BIT "#extension GL_ARB_gpu_shader_int64 : require\n"

NK_API void
nk_glfw3_device_create()
{
    GLint status;
    static const GLchar *vertex_shader =
        NK_SHADER_VERSION
        NK_SHADER_BINDLESS
        NK_SHADER_64BIT
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
        NK_SHADER_BINDLESS
        NK_SHADER_64BIT
        "precision mediump float;\n"
        "uniform uint64_t Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main(){\n"
        "   sampler2D smp = sampler2D(Texture);\n"
        "   Out_Color = Frag_Color * texture(smp, Frag_UV.st);\n"
        "}\n";

    struct nk_glfw_device *dev = &glfw.ogl;
    nk_buffer_init_default(&dev->cmds);
    dev->prog = glCreateProgram();
    dev->vert_shdr = glCreateShader(GL_VERTEX_SHADER);
    dev->frag_shdr = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(dev->vert_shdr, 1, &vertex_shader, 0);
    glShaderSource(dev->frag_shdr, 1, &fragment_shader, 0);
    glCompileShader(dev->vert_shdr);
    glCompileShader(dev->frag_shdr);
    glGetShaderiv(dev->vert_shdr, GL_COMPILE_STATUS, &status);

    GLint len = 0;
    glGetShaderiv(dev->vert_shdr, GL_INFO_LOG_LENGTH, &len);
    if (len > 1) {
        char *log = (char*)calloc((size_t)len, sizeof(char));
        glGetShaderInfoLog(dev->vert_shdr, len, NULL, log);
        fprintf(stdout, "[GL]: failed to compile shader: %s", log);
        free(log);
    }

    glGetShaderiv(dev->frag_shdr, GL_INFO_LOG_LENGTH, &len);
    if (len > 1) {
        char *log = (char*)calloc((size_t)len, sizeof(char));
        glGetShaderInfoLog(dev->frag_shdr, len, NULL, log);
        fprintf(stdout, "[GL]: failed to compile shader: %s", log);
        free(log);
    }

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
        GLsizei vs = sizeof(struct nk_glfw_vertex);
        size_t vp = offsetof(struct nk_glfw_vertex, position);
        size_t vt = offsetof(struct nk_glfw_vertex, uv);
        size_t vc = offsetof(struct nk_glfw_vertex, col);

        GLuint pos = (GLuint)dev->attrib_pos;
        GLuint uv = (GLuint)dev->attrib_uv;
        GLuint col = (GLuint)dev->attrib_col;

        glCreateVertexArrays(1, &dev->vao);
        glCreateBuffers(1, &dev->vbo);
        glCreateBuffers(1, &dev->ebo);

        glEnableVertexArrayAttrib(dev->vao, pos);
        glEnableVertexArrayAttrib(dev->vao, uv);
        glEnableVertexArrayAttrib(dev->vao, col);

        glVertexArrayAttribBinding(dev->vao, pos, 0);
        glVertexArrayAttribBinding(dev->vao, uv, 0);
        glVertexArrayAttribBinding(dev->vao, col, 0);

        glVertexArrayAttribFormat(dev->vao, pos, 2, GL_FLOAT, GL_FALSE, vp);
        glVertexArrayAttribFormat(dev->vao, uv, 2, GL_FLOAT, GL_FALSE, vt);
        glVertexArrayAttribFormat(dev->vao, col, 4, GL_UNSIGNED_BYTE, GL_TRUE, vc);

        glVertexArrayElementBuffer(dev->vao, dev->ebo);
        glVertexArrayVertexBuffer(dev->vao, 0, dev->vbo, 0, vs);
    }

    /* Persistent mapped buffers */
    {GLsizei vb_size = dev->max_vertex_buffer;
    GLsizei eb_size = dev->max_element_buffer;
    GLbitfield flags = GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT;
    glNamedBufferStorage(dev->vbo, vb_size, 0, flags);
    glNamedBufferStorage(dev->ebo, eb_size, 0, flags);
    dev->vert_buffer = (struct nk_glfw_vertex*) glMapNamedBufferRange(dev->vbo, 0, vb_size, flags);
    dev->elem_buffer = (int*) glMapNamedBufferRange(dev->ebo, 0, eb_size, flags);}

    memset(dev->tex_ids, 0, sizeof(dev->tex_ids));
    memset(dev->tex_handles, 0, sizeof(dev->tex_handles));
}

NK_INTERN int
nk_glfw3_get_available_tex_index()
{
    int i = 0;
    struct nk_glfw_device *dev = &glfw.ogl;
    for (i = 0; i < NK_GLFW_MAX_TEXTURES; i++) {
        if (dev->tex_ids[i] == 0)
            return i;
    } assert(0); /* max textures reached */
    return -1;
}

NK_API GLuint
nk_glfw3_get_tex_ogl_id(int tex_index)
{
    struct nk_glfw_device *dev = &glfw.ogl;
    assert(tex_index >= 0 && tex_index < NK_GLFW_MAX_TEXTURES);
    return dev->tex_ids[tex_index];
}

NK_API GLuint64
nk_glfw3_get_tex_ogl_handle(int tex_index)
{
    struct nk_glfw_device *dev = &glfw.ogl;
    assert(tex_index >= 0 && tex_index < NK_GLFW_MAX_TEXTURES);
    return dev->tex_handles[tex_index];
}

NK_API int
nk_glfw3_create_texture(const void* image, int width, int height)
{
    GLuint id;
    GLsizei w = (GLsizei)width;
    GLsizei h = (GLsizei)height;
    struct nk_glfw_device *dev = &glfw.ogl;
    int tex_index = nk_glfw3_get_available_tex_index();
    if (tex_index < 0)
        return -1;

    glCreateTextures(GL_TEXTURE_2D, 1, &id);
    dev->tex_ids[tex_index] = id;

    glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureStorage2D(id, 1, GL_RGBA8, w, h);
    if (image)
        glTextureSubImage2D(id, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, image);
    else glClearTexImage(id, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);

    {GLuint64 handle = glGetTextureHandleARB(id);
    glMakeTextureHandleResidentARB(handle);
    dev->tex_handles[tex_index] = handle;
    return tex_index;}
}

NK_API void
nk_glfw3_destroy_texture(int tex_index)
{
    struct nk_glfw_device *dev = &glfw.ogl;
    GLuint id = nk_glfw3_get_tex_ogl_id(tex_index);
    if (id == 0) return;

    {GLuint64 handle = nk_glfw3_get_tex_ogl_handle(tex_index);
    glMakeTextureHandleNonResidentARB(handle);
    glDeleteTextures(1, &id);
    dev->tex_ids[tex_index] = 0;
    dev->tex_handles[tex_index] = 0;}
}

NK_INTERN void
nk_glfw3_device_upload_atlas(const void *image, int width, int height)
{
    struct nk_glfw_device *dev = &glfw.ogl;
    dev->font_tex_index = nk_glfw3_create_texture(image, width, height);
}

NK_API void
nk_glfw3_device_destroy(void)
{
    int i = 0;
    struct nk_glfw_device *dev = &glfw.ogl;
    glDetachShader(dev->prog, dev->vert_shdr);
    glDetachShader(dev->prog, dev->frag_shdr);
    glDeleteShader(dev->vert_shdr);
    glDeleteShader(dev->frag_shdr);
    glDeleteProgram(dev->prog);
    nk_glfw3_destroy_texture(dev->font_tex_index);

    for (i = 0; i < NK_GLFW_MAX_TEXTURES; i++)
        nk_glfw3_destroy_texture(i);
    glUnmapNamedBuffer(dev->vbo);
    glUnmapNamedBuffer(dev->ebo);
    glDeleteBuffers(1, &dev->vbo);
    glDeleteBuffers(1, &dev->ebo);
    glDeleteVertexArrays(1, &dev->vao);
    nk_buffer_free(&dev->cmds);
}

NK_INTERN void
nk_glfw3_wait_for_buffer_unlock()
{
    struct nk_glfw_device *dev = &glfw.ogl;
    if(!dev->buffer_sync)
        return;

    while (1) {
        GLenum wait = glClientWaitSync(dev->buffer_sync, GL_SYNC_FLUSH_COMMANDS_BIT, 1);
        if (wait == GL_ALREADY_SIGNALED || wait == GL_CONDITION_SATISFIED)
            return;
    }
}

NK_INTERN void
nk_glfw3_lock_buffer()
{
    struct nk_glfw_device *dev = &glfw.ogl;
    if(dev->buffer_sync) glDeleteSync(dev->buffer_sync);
    dev->buffer_sync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

NK_API void
nk_glfw3_render(enum nk_anti_aliasing AA)
{
    struct nk_glfw_device *dev = &glfw.ogl;
    struct nk_buffer vbuf, ebuf;
    GLfloat ortho[4][4] = {
        {2.0f, 0.0f, 0.0f, 0.0f},
        {0.0f,-2.0f, 0.0f, 0.0f},
        {0.0f, 0.0f,-1.0f, 0.0f},
        {-1.0f,1.0f, 0.0f, 1.0f},
    };
    ortho[0][0] /= (GLfloat)glfw.width;
    ortho[1][1] /= (GLfloat)glfw.height;

    /* setup global state */
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);

    /* setup program */
    glUseProgram(dev->prog);
    glUniformMatrix4fv(dev->uniform_proj, 1, GL_FALSE, &ortho[0][0]);
    glViewport(0,0,(GLsizei)glfw.display_width,(GLsizei)glfw.display_height);
    {
        /* convert from command queue into draw list and draw to screen */
        const struct nk_draw_command *cmd;
        void *vertices, *elements;
        const nk_draw_index *offset = NULL;

        glBindVertexArray(dev->vao);

        /* load draw vertices & elements directly into vertex + element buffer */
        vertices = dev->vert_buffer;
        elements = dev->elem_buffer;
        {
            /* Wait until GPU is done with buffer */
            nk_glfw3_wait_for_buffer_unlock();
            {
                /* fill convert configuration */
                struct nk_convert_config config;
                static const struct nk_draw_vertex_layout_element vertex_layout[] = {
                    {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_glfw_vertex, position)},
                    {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_glfw_vertex, uv)},
                    {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_glfw_vertex, col)},
                    {NK_VERTEX_LAYOUT_END}
                };
                NK_MEMSET(&config, 0, sizeof(config));
                config.vertex_layout = vertex_layout;
                config.vertex_size = sizeof(struct nk_glfw_vertex);
                config.vertex_alignment = NK_ALIGNOF(struct nk_glfw_vertex);
                config.null = dev->null;
                config.circle_segment_count = 22;
                config.curve_segment_count = 22;
                config.arc_segment_count = 22;
                config.global_alpha = 1.0f;
                config.shape_AA = AA;
                config.line_AA = AA;

                /* setup buffers to load vertices and elements */
                nk_buffer_init_fixed(&vbuf, vertices, (size_t)dev->max_vertex_buffer);
                nk_buffer_init_fixed(&ebuf, elements, (size_t)dev->max_element_buffer);
                nk_convert(&glfw.ctx, &dev->cmds, &vbuf, &ebuf, &config);
            }
        }

        /* iterate over and execute each draw command */
        nk_draw_foreach(cmd, &glfw.ctx, &dev->cmds)
        {
            int tex_index;
            GLuint64 tex_handle;
            if (!cmd->elem_count) continue;

            tex_index = cmd->texture.id;
            tex_handle = nk_glfw3_get_tex_ogl_handle(tex_index);

            /* tex handle must be made resident in each context that uses it */
            if (!glIsTextureHandleResidentARB(tex_handle))
                glMakeTextureHandleResidentARB(tex_handle);

            glUniformHandleui64ARB(dev->uniform_tex, tex_handle);
            glScissor(
                (GLint)(cmd->clip_rect.x * glfw.fb_scale.x),
                (GLint)((glfw.height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)) * glfw.fb_scale.y),
                (GLint)(cmd->clip_rect.w * glfw.fb_scale.x),
                (GLint)(cmd->clip_rect.h * glfw.fb_scale.y));
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
            offset += cmd->elem_count;
        }
        nk_clear(&glfw.ctx);
    }
    /* default OpenGL state */
    glUseProgram(0);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    /* Lock buffer until GPU has finished draw command */
    nk_glfw3_lock_buffer();
}

NK_API void
nk_glfw3_char_callback(GLFWwindow *win, unsigned int codepoint)
{
    (void)win;
    if (glfw.text_len < NK_GLFW_TEXT_MAX)
        glfw.text[glfw.text_len++] = codepoint;
}

NK_API void
nk_gflw3_scroll_callback(GLFWwindow *win, double xoff, double yoff)
{
    (void)win; (void)xoff;
    glfw.scroll.x += (float)xoff;
    glfw.scroll.y += (float)yoff;
}

NK_API void
nk_glfw3_mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    double x, y;
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    glfwGetCursorPos(window, &x, &y);
    if (action == GLFW_PRESS)  {
        double dt = glfwGetTime() - glfw.last_button_click;
        if (dt > NK_GLFW_DOUBLE_CLICK_LO && dt < NK_GLFW_DOUBLE_CLICK_HI) {
            glfw.is_double_click_down = nk_true;
            glfw.double_click_pos = nk_vec2((float)x, (float)y);
        }
        glfw.last_button_click = glfwGetTime();
    } else glfw.is_double_click_down = nk_false;
}

NK_INTERN void
nk_glfw3_clipboard_paste(nk_handle usr, struct nk_text_edit *edit)
{
    const char *text = glfwGetClipboardString(glfw.win);
    if (text) nk_textedit_paste(edit, text, nk_strlen(text));
    (void)usr;
}

NK_INTERN void
nk_glfw3_clipboard_copy(nk_handle usr, const char *text, int len)
{
    char *str = 0;
    (void)usr;
    if (!len) return;
    str = (char*)malloc((size_t)len+1);
    if (!str) return;
    memcpy(str, text, (size_t)len);
    str[len] = '\0';
    glfwSetClipboardString(glfw.win, str);
    free(str);
}

NK_API struct nk_context*
nk_glfw3_init(GLFWwindow *win, enum nk_glfw_init_state init_state,
              int max_vertex_buffer, int max_element_buffer)
{
    glfw.win = win;
    if (init_state == NK_GLFW3_INSTALL_CALLBACKS) {
        glfwSetScrollCallback(win, nk_gflw3_scroll_callback);
        glfwSetCharCallback(win, nk_glfw3_char_callback);
        glfwSetMouseButtonCallback(win, nk_glfw3_mouse_button_callback);
    }
    nk_init_default(&glfw.ctx, 0);
    glfw.ctx.clip.copy = nk_glfw3_clipboard_copy;
    glfw.ctx.clip.paste = nk_glfw3_clipboard_paste;
    glfw.ctx.clip.userdata = nk_handle_ptr(0);
    glfw.last_button_click = 0;

    {struct nk_glfw_device *dev = &glfw.ogl;
    dev->max_vertex_buffer = max_vertex_buffer;
    dev->max_element_buffer = max_element_buffer;
    nk_glfw3_device_create();}

    glfw.is_double_click_down = nk_false;
    glfw.double_click_pos = nk_vec2(0, 0);

    return &glfw.ctx;
}

NK_API void
nk_glfw3_font_stash_begin(struct nk_font_atlas **atlas)
{
    nk_font_atlas_init_default(&glfw.atlas);
    nk_font_atlas_begin(&glfw.atlas);
    *atlas = &glfw.atlas;
}

NK_API void
nk_glfw3_font_stash_end(void)
{
    const void *image; int w, h;
    image = nk_font_atlas_bake(&glfw.atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    nk_glfw3_device_upload_atlas(image, w, h);
    nk_font_atlas_end(&glfw.atlas, nk_handle_id((int)glfw.ogl.font_tex_index), &glfw.ogl.null);
    if (glfw.atlas.default_font)
        nk_style_set_font(&glfw.ctx, &glfw.atlas.default_font->handle);
}

NK_API void
nk_glfw3_new_frame(void)
{
    int i;
    double x, y;
    struct nk_context *ctx = &glfw.ctx;
    struct GLFWwindow *win = glfw.win;

    glfwGetWindowSize(win, &glfw.width, &glfw.height);
    glfwGetFramebufferSize(win, &glfw.display_width, &glfw.display_height);
    glfw.fb_scale.x = (float)glfw.display_width/(float)glfw.width;
    glfw.fb_scale.y = (float)glfw.display_height/(float)glfw.height;

    nk_input_begin(ctx);
    for (i = 0; i < glfw.text_len; ++i)
        nk_input_unicode(ctx, glfw.text[i]);

#ifdef NK_GLFW_GL4_MOUSE_GRABBING
    /* optional grabbing behavior */
    if (ctx->input.mouse.grab)
        glfwSetInputMode(glfw.win, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
    else if (ctx->input.mouse.ungrab)
        glfwSetInputMode(glfw.win, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
#endif

    nk_input_key(ctx, NK_KEY_DEL, glfwGetKey(win, GLFW_KEY_DELETE) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_ENTER, glfwGetKey(win, GLFW_KEY_ENTER) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_TAB, glfwGetKey(win, GLFW_KEY_TAB) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_BACKSPACE, glfwGetKey(win, GLFW_KEY_BACKSPACE) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_UP, glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_DOWN, glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_TEXT_START, glfwGetKey(win, GLFW_KEY_HOME) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_TEXT_END, glfwGetKey(win, GLFW_KEY_END) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_SCROLL_START, glfwGetKey(win, GLFW_KEY_HOME) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_SCROLL_END, glfwGetKey(win, GLFW_KEY_END) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_SCROLL_DOWN, glfwGetKey(win, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_SCROLL_UP, glfwGetKey(win, GLFW_KEY_PAGE_UP) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_SHIFT, glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS||
                                    glfwGetKey(win, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);

    if (glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(win, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
        nk_input_key(ctx, NK_KEY_COPY, glfwGetKey(win, GLFW_KEY_C) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_PASTE, glfwGetKey(win, GLFW_KEY_V) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_CUT, glfwGetKey(win, GLFW_KEY_X) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_UNDO, glfwGetKey(win, GLFW_KEY_Z) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_REDO, glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_LINE_START, glfwGetKey(win, GLFW_KEY_B) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_LINE_END, glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS);
    } else {
        nk_input_key(ctx, NK_KEY_LEFT, glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_RIGHT, glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_COPY, 0);
        nk_input_key(ctx, NK_KEY_PASTE, 0);
        nk_input_key(ctx, NK_KEY_CUT, 0);
        nk_input_key(ctx, NK_KEY_SHIFT, 0);
    }

    glfwGetCursorPos(win, &x, &y);
    nk_input_motion(ctx, (int)x, (int)y);
#ifdef NK_GLFW_GL4_MOUSE_GRABBING
    if (ctx->input.mouse.grabbed) {
        glfwSetCursorPos(glfw.win, ctx->input.mouse.prev.x, ctx->input.mouse.prev.y);
        ctx->input.mouse.pos.x = ctx->input.mouse.prev.x;
        ctx->input.mouse.pos.y = ctx->input.mouse.prev.y;
    }
#endif
    nk_input_button(ctx, NK_BUTTON_LEFT, (int)x, (int)y, glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    nk_input_button(ctx, NK_BUTTON_MIDDLE, (int)x, (int)y, glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS);
    nk_input_button(ctx, NK_BUTTON_RIGHT, (int)x, (int)y, glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
    nk_input_button(ctx, NK_BUTTON_DOUBLE, (int)glfw.double_click_pos.x, (int)glfw.double_click_pos.y, glfw.is_double_click_down);
    nk_input_scroll(ctx, glfw.scroll);
    nk_input_end(&glfw.ctx);
    glfw.text_len = 0;
    glfw.scroll = nk_vec2(0,0);
}

NK_API
void nk_glfw3_shutdown(void)
{
    nk_font_atlas_clear(&glfw.atlas);
    nk_free(&glfw.ctx);
    nk_glfw3_device_destroy();
    memset(&glfw, 0, sizeof(glfw));
}

#endif
