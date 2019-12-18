/*
 * Nuklear - 1.40.8 - public domain
 * no warrenty implied; use at your own risk.
 * authored from 2015-2017 by Micha Mettke
 * OpenGL ES 2.0 from 2017 by Dmitry Hrabrov a.k.a. DeXPeriX
 * Wayland from 2019 by Jason Francis
 */
/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */
#ifndef NK_WAYLAND_GLES2_H_
#define NK_WAYLAND_GLES2_H_

enum mod_mask {
    MOD_SHIFT = 1 << 0,
    MOD_CAPS = 1 << 1,
    MOD_CTRL = 1 << 2,
    MOD_ALT = 1 << 3,
    MOD_NUM = 1 << 4,
    MOD_LOGO = 1 << 5
};

struct nk_wayland;
typedef uint32_t xkb_keysym_t;

NK_API struct nk_wayland *  nk_wayland_init(nk_plugin_copy copy, nk_plugin_paste paste, nk_handle userdata);
NK_API void                 nk_wayland_font_stash_begin(struct nk_wayland *wayland, struct nk_font_atlas **atlas);
NK_API void                 nk_wayland_font_stash_end(struct nk_wayland *wayland);
NK_API int                  nk_wayland_handle_key(struct nk_wayland *wayland, xkb_keysym_t sym, uint32_t mod, int down);
NK_API int                  nk_wayland_handle_pointer_button(struct nk_wayland *wayland, enum nk_buttons button, int down);
NK_API int                  nk_wayland_handle_pointer_motion(struct nk_wayland *wayland, int x, int y, int dx, int dy);
NK_API int                  nk_wayland_handle_pointer_wheel(struct nk_wayland *wayland, double x, double y);
NK_API void                 nk_wayland_render(struct nk_wayland *wayland, int display_width, int display_height, double scale, enum nk_anti_aliasing , int max_vertex_buffer, int max_element_buffer);
NK_API void                 nk_wayland_shutdown(struct nk_wayland *wayland);
NK_API void                 nk_wayland_device_destroy(struct nk_wayland *wayland);
NK_API void                 nk_wayland_device_create(struct nk_wayland *wayland);
NK_API struct nk_context *  nk_wayland_get_context(struct nk_wayland *wayland);

#endif

/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#ifdef NK_WAYLAND_GLES2_IMPLEMENTATION

#include <string.h>
#include <stddef.h>
#include <GLES2/gl2.h>
#include <xkbcommon/xkbcommon-keysyms.h>

struct nk_wayland_device {
    struct nk_buffer cmds;
    struct nk_draw_null_texture null;
    GLuint vbo, ebo;
    GLuint prog;
    GLuint vert_shdr;
    GLuint frag_shdr;
    GLint attrib_pos;
    GLint attrib_uv;
    GLint attrib_col;
    GLint uniform_tex;
    GLint uniform_proj;
    GLuint font_tex;
    GLsizei vs;
    size_t vp, vt, vc;
};

struct nk_wayland_vertex {
    GLfloat position[2];
    GLfloat uv[2];
    nk_byte col[4];
};

struct nk_wayland {
    struct nk_wayland_device ogl;
    struct nk_context ctx;
    struct nk_font_atlas atlas;
};

#define NK_SHADER_VERSION "#version 100\n"


NK_API void
nk_wayland_device_create(struct nk_wayland *wayland)
{
    GLint status;
    static const GLchar *vertex_shader =
        NK_SHADER_VERSION
        "uniform mat4 ProjMtx;\n"
        "attribute vec2 Position;\n"
        "attribute vec2 TexCoord;\n"
        "attribute vec4 Color;\n"
        "varying vec2 Frag_UV;\n"
        "varying vec4 Frag_Color;\n"
        "void main() {\n"
        "   Frag_UV = TexCoord;\n"
        "   Frag_Color = Color;\n"
        "   gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
        "}\n";
    static const GLchar *fragment_shader =
        NK_SHADER_VERSION
        "precision mediump float;\n"
        "uniform sampler2D Texture;\n"
        "varying vec2 Frag_UV;\n"
        "varying vec4 Frag_Color;\n"
        "void main(){\n"
        "   gl_FragColor = Frag_Color * texture2D(Texture, Frag_UV);\n"
        "}\n";

    struct nk_wayland_device *dev = &wayland->ogl;
    
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
        dev->vs = sizeof(struct nk_wayland_vertex);
        dev->vp = offsetof(struct nk_wayland_vertex, position);
        dev->vt = offsetof(struct nk_wayland_vertex, uv);
        dev->vc = offsetof(struct nk_wayland_vertex, col);
        
        /* Allocate buffers */
        glGenBuffers(1, &dev->vbo);
        glGenBuffers(1, &dev->ebo);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

NK_INTERN void
nk_wayland_device_upload_atlas(struct nk_wayland *wayland, const void *image, int width, int height)
{
    struct nk_wayland_device *dev = &wayland->ogl;
    glGenTextures(1, &dev->font_tex);
    glBindTexture(GL_TEXTURE_2D, dev->font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image);
}

NK_API void
nk_wayland_device_destroy(struct nk_wayland *wayland)
{
    struct nk_wayland_device *dev = &wayland->ogl;
    glDetachShader(dev->prog, dev->vert_shdr);
    glDetachShader(dev->prog, dev->frag_shdr);
    glDeleteShader(dev->vert_shdr);
    glDeleteShader(dev->frag_shdr);
    glDeleteProgram(dev->prog);
    glDeleteTextures(1, &dev->font_tex);
    glDeleteBuffers(1, &dev->vbo);
    glDeleteBuffers(1, &dev->ebo);
    nk_buffer_free(&dev->cmds);
}

NK_API void
nk_wayland_render(struct nk_wayland *wayland, int display_width, int display_height, double scale_factor, enum nk_anti_aliasing AA, int max_vertex_buffer, int max_element_buffer)
{
    struct nk_wayland_device *dev = &wayland->ogl;
    int width, height;
    struct nk_vec2 scale = { scale_factor, scale_factor };
    GLfloat ortho[4][4] = {
        {2.0f, 0.0f, 0.0f, 0.0f},
        {0.0f,-2.0f, 0.0f, 0.0f},
        {0.0f, 0.0f,-1.0f, 0.0f},
        {-1.0f,1.0f, 0.0f, 1.0f},
    };

    width = display_width / scale.x;
    height = display_height / scale.y;
    ortho[0][0] /= (GLfloat)width;
    ortho[1][1] /= (GLfloat)height;

    /* setup global state */
    glViewport(0,0,display_width,display_height);
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

        /* Bind buffers */
        glBindBuffer(GL_ARRAY_BUFFER, dev->vbo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, dev->ebo);
        
        {
            /* buffer setup */
            glEnableVertexAttribArray((GLuint)dev->attrib_pos);
            glEnableVertexAttribArray((GLuint)dev->attrib_uv);
            glEnableVertexAttribArray((GLuint)dev->attrib_col);

            glVertexAttribPointer((GLuint)dev->attrib_pos, 2, GL_FLOAT, GL_FALSE, dev->vs, (void*)dev->vp);
            glVertexAttribPointer((GLuint)dev->attrib_uv, 2, GL_FLOAT, GL_FALSE, dev->vs, (void*)dev->vt);
            glVertexAttribPointer((GLuint)dev->attrib_col, 4, GL_UNSIGNED_BYTE, GL_TRUE, dev->vs, (void*)dev->vc);
        }

        glBufferData(GL_ARRAY_BUFFER, max_vertex_buffer, NULL, GL_STREAM_DRAW);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, max_element_buffer, NULL, GL_STREAM_DRAW);

        /* load vertices/elements directly into vertex/element buffer */
        vertices = malloc((size_t)max_vertex_buffer);
        elements = malloc((size_t)max_element_buffer);
        if (vertices && elements)
        {
            {
                /* fill convert configuration */
                struct nk_convert_config config;
                static const struct nk_draw_vertex_layout_element vertex_layout[] = {
                    {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_wayland_vertex, position)},
                    {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_wayland_vertex, uv)},
                    {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_wayland_vertex, col)},
                    {NK_VERTEX_LAYOUT_END}
                };
                NK_MEMSET(&config, 0, sizeof(config));
                config.vertex_layout = vertex_layout;
                config.vertex_size = sizeof(struct nk_wayland_vertex);
                config.vertex_alignment = NK_ALIGNOF(struct nk_wayland_vertex);
                config.null = dev->null;
                config.circle_segment_count = 22;
                config.curve_segment_count = 22;
                config.arc_segment_count = 22;
                config.global_alpha = 1.0f;
                config.shape_AA = AA;
                config.line_AA = AA;

                /* setup buffers to load vertices and elements */
                {struct nk_buffer vbuf, ebuf;
                    nk_buffer_init_fixed(&vbuf, vertices, (nk_size)max_vertex_buffer);
                    nk_buffer_init_fixed(&ebuf, elements, (nk_size)max_element_buffer);
                    nk_convert(&wayland->ctx, &dev->cmds, &vbuf, &ebuf, &config);}
            }
            glBufferSubData(GL_ARRAY_BUFFER, 0, (size_t)max_vertex_buffer, vertices);
            glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, (size_t)max_element_buffer, elements);
            free(vertices);
            free(elements);

            /* iterate over and execute each draw command */
            nk_draw_foreach(cmd, &wayland->ctx, &dev->cmds) {
                if (!cmd->elem_count) continue;
                glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
                glScissor((GLint)(cmd->clip_rect.x * scale.x),
                        (GLint)((height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)) * scale.y),
                        (GLint)(cmd->clip_rect.w * scale.x),
                        (GLint)(cmd->clip_rect.h * scale.y));
                glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
                offset += cmd->elem_count;
            }
        }
        nk_clear(&wayland->ctx);
    }

    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
}

NK_API struct nk_wayland*
nk_wayland_init(nk_plugin_copy copy, nk_plugin_paste paste, nk_handle userdata)
{
    struct nk_wayland *wayland = calloc(1, sizeof(*wayland));
    nk_init_default(&wayland->ctx, 0);
    wayland->ctx.clip.copy = copy;
    wayland->ctx.clip.paste = paste;
    wayland->ctx.clip.userdata = userdata;
    nk_wayland_device_create(wayland);
    return wayland;
}

NK_API void
nk_wayland_font_stash_begin(struct nk_wayland *wayland, struct nk_font_atlas **atlas)
{
    nk_font_atlas_init_default(&wayland->atlas);
    nk_font_atlas_begin(&wayland->atlas);
    *atlas = &wayland->atlas;
}

NK_API void
nk_wayland_font_stash_end(struct nk_wayland *wayland)
{
    const void *image; int w, h;
    image = nk_font_atlas_bake(&wayland->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    nk_wayland_device_upload_atlas(wayland, image, w, h);
    nk_font_atlas_end(&wayland->atlas, nk_handle_id((int)wayland->ogl.font_tex), &wayland->ogl.null);
    if (wayland->atlas.default_font)
        nk_style_set_font(&wayland->ctx, &wayland->atlas.default_font->handle);

}

NK_API int
nk_wayland_handle_key(struct nk_wayland *wayland, xkb_keysym_t sym, uint32_t mod, int down)
{
    struct nk_context *ctx = &wayland->ctx;
    if (sym == XKB_KEY_Shift_R || sym == XKB_KEY_Shift_L)
        nk_input_key(ctx, NK_KEY_SHIFT, down);
    else if (sym == XKB_KEY_Delete)
        nk_input_key(ctx, NK_KEY_DEL, down);
    else if (sym == XKB_KEY_Return)
        nk_input_key(ctx, NK_KEY_ENTER, down);
    else if (sym == XKB_KEY_Tab)
        nk_input_key(ctx, NK_KEY_TAB, down);
    else if (sym == XKB_KEY_BackSpace)
        nk_input_key(ctx, NK_KEY_BACKSPACE, down);
    else if (sym == XKB_KEY_Home) {
        nk_input_key(ctx, NK_KEY_TEXT_START, down);
        nk_input_key(ctx, NK_KEY_SCROLL_START, down);
    } else if (sym == XKB_KEY_End) {
        nk_input_key(ctx, NK_KEY_TEXT_END, down);
        nk_input_key(ctx, NK_KEY_SCROLL_END, down);
    } else if (sym == XKB_KEY_Page_Down) {
        nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
    } else if (sym == XKB_KEY_Page_Up) {
        nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
    } else if (sym == XKB_KEY_z && (mod & MOD_CTRL))
        nk_input_key(ctx, NK_KEY_TEXT_UNDO, down);
    else if (sym == XKB_KEY_r && (mod & MOD_CTRL))
        nk_input_key(ctx, NK_KEY_TEXT_REDO, down);
    else if (sym == XKB_KEY_c && (mod & MOD_CTRL))
        nk_input_key(ctx, NK_KEY_COPY, down);
    else if (sym == XKB_KEY_v && (mod & MOD_CTRL))
        nk_input_key(ctx, NK_KEY_PASTE, down);
    else if (sym == XKB_KEY_x && (mod & MOD_CTRL))
        nk_input_key(ctx, NK_KEY_CUT, down);
    else if (sym == XKB_KEY_b && (mod & MOD_CTRL))
        nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down);
    else if (sym == XKB_KEY_e && (mod & MOD_CTRL))
        nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down);
    else if (sym == XKB_KEY_Up)
        nk_input_key(ctx, NK_KEY_UP, down);
    else if (sym == XKB_KEY_Down)
        nk_input_key(ctx, NK_KEY_DOWN, down);
    else if (sym == XKB_KEY_Left) {
        if (mod & MOD_CTRL)
            nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
        else nk_input_key(ctx, NK_KEY_LEFT, down);
    } else if (sym == XKB_KEY_Right) {
        if (mod & MOD_CTRL)
            nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
        else nk_input_key(ctx, NK_KEY_RIGHT, down);
    } else {
        return 0;
    }
    return 1;
}

NK_API int
nk_wayland_handle_pointer_button(struct nk_wayland *wayland, enum nk_buttons button, int down)
{
    struct nk_context *ctx = &wayland->ctx;
    const int x = ctx->input.mouse.prev.x, y = ctx->input.mouse.prev.y;
    nk_input_button(ctx, button, x, y, down);
    return 1;
}

NK_API int
nk_wayland_handle_pointer_motion(struct nk_wayland *wayland, int x, int y, int dx, int dy)
{
    struct nk_context *ctx = &wayland->ctx;
    if (!ctx->input.mouse.grabbed && x >= 0 && y >= 0) {
        nk_input_motion(ctx, x, y);
        int ddx = ctx->input.mouse.delta.x, ddy = ctx->input.mouse.delta.y;
        int px = ctx->input.mouse.prev.x, py = ctx->input.mouse.prev.y;
        return 1;
    } else if (ctx->input.mouse.grabbed && (dx || dy)) {
        float px = ctx->input.mouse.prev.x, py = ctx->input.mouse.prev.y;
        nk_input_motion(ctx, px + dx, py + dy);
        return 1;
    }
    return 0;
}

NK_API int
nk_wayland_handle_pointer_wheel(struct nk_wayland *wayland, double x, double y)
{
    struct nk_context *ctx = &wayland->ctx;
    nk_input_scroll(ctx, nk_vec2(x, y));
    return 1;
}

NK_API
void nk_wayland_shutdown(struct nk_wayland *wayland)
{
    nk_font_atlas_clear(&wayland->atlas);
    nk_free(&wayland->ctx);
    nk_wayland_device_destroy(wayland);
    free(wayland);
}

NK_API
struct nk_context *nk_wayland_get_context(struct nk_wayland *wayland)
{
    return &wayland->ctx;
}

#endif
