#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xlocale.h>

#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glext.h>
#include <GL/glxext.h>

#define NK_IMPLEMENTATION
#include "nuklear_x11.h"
#include "../../nuklear.h"

#ifndef FALSE
#define FALSE 0
#endif
#ifndef TRUE
#define TRUE 1
#endif

#if NK_X11_LOAD_OPENGL_EXTENSIONS
/* GL_ARB_vertex_buffer_object */
typedef void(*qglGenBuffers)(GLsizei, GLuint*);
typedef void(*qglBindBuffer)(GLenum, GLuint);
typedef void(*qglBufferData)(GLenum, GLsizeiptr, const GLvoid*, GLenum);
typedef void(*qglBufferSubData)(GLenum, GLintptr, GLsizeiptr, const GLvoid*);
typedef void*(*qglMapBuffer)(GLenum, GLenum);
typedef GLboolean(*qglUnmapBuffer)(GLenum);
typedef void(*qglDeleteBuffers)(GLsizei, GLuint*);
/* GL_ARB_vertex_array_object */
typedef void (*qglGenVertexArrays)(GLsizei, GLuint*);
typedef void (*qglBindVertexArray)(GLuint);
typedef void (*qglDeleteVertexArrays)(GLsizei, const GLuint*);
/* GL_ARB_vertex_program / GL_ARB_fragment_program */
typedef void(*qglVertexAttribPointer)(GLuint, GLint, GLenum, GLboolean, GLsizei, const GLvoid*);
typedef void(*qglEnableVertexAttribArray)(GLuint);
typedef void(*qglDisableVertexAttribArray)(GLuint);
/* GL_ARB_framebuffer_object */
typedef void(*qglGenerateMipmap)(GLenum target);
/* GLSL/OpenGL 2.0 core */
typedef GLuint(*qglCreateShader)(GLenum);
typedef void(*qglShaderSource)(GLuint, GLsizei, const GLchar**, const GLint*);
typedef void(*qglCompileShader)(GLuint);
typedef void(*qglGetShaderiv)(GLuint, GLenum, GLint*);
typedef void(*qglGetShaderInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void(*qglDeleteShader)(GLuint);
typedef GLuint(*qglCreateProgram)(void);
typedef void(*qglAttachShader)(GLuint, GLuint);
typedef void(*qglDetachShader)(GLuint, GLuint);
typedef void(*qglLinkProgram)(GLuint);
typedef void(*qglUseProgram)(GLuint);
typedef void(*qglGetProgramiv)(GLuint, GLenum, GLint*);
typedef void(*qglGetProgramInfoLog)(GLuint, GLsizei, GLsizei*, GLchar*);
typedef void(*qglDeleteProgram)(GLuint);
typedef GLint(*qglGetUniformLocation)(GLuint, const GLchar*);
typedef GLint(*qglGetAttribLocation)(GLuint, const GLchar*);
typedef void(*qglUniform1i)(GLint, GLint);
typedef void(*qglUniform1f)(GLint, GLfloat);
typedef void(*qglUniformMatrix3fv)(GLint, GLsizei, GLboolean, const GLfloat*);
typedef void(*qglUniformMatrix4fv)(GLint, GLsizei, GLboolean, const GLfloat*);

static qglGenBuffers glGenBuffers;
static qglBindBuffer glBindBuffer;
static qglBufferData glBufferData;
static qglBufferSubData glBufferSubData;
static qglMapBuffer glMapBuffer;
static qglUnmapBuffer glUnmapBuffer;
static qglDeleteBuffers glDeleteBuffers;
static qglGenVertexArrays glGenVertexArrays;
static qglBindVertexArray glBindVertexArray;
static qglDeleteVertexArrays glDeleteVertexArrays;
static qglVertexAttribPointer glVertexAttribPointer;
static qglEnableVertexAttribArray glEnableVertexAttribArray;
static qglDisableVertexAttribArray glDisableVertexAttribArray;
static qglGenerateMipmap glGenerateMipmap;
static qglCreateShader glCreateShader;
static qglShaderSource glShaderSource;
static qglCompileShader glCompileShader;
static qglGetShaderiv glGetShaderiv;
static qglGetShaderInfoLog glGetShaderInfoLog;
static qglDeleteShader glDeleteShader;
static qglCreateProgram glCreateProgram;
static qglAttachShader glAttachShader;
static qglDetachShader glDetachShader;
static qglLinkProgram glLinkProgram;
static qglUseProgram glUseProgram;
static qglGetProgramiv glGetProgramiv;
static qglGetProgramInfoLog glGetProgramInfoLog;
static qglDeleteProgram glDeleteProgram;
static qglGetUniformLocation glGetUniformLocation;
static qglGetAttribLocation glGetAttribLocation;
static qglUniform1i glUniform1i;
static qglUniform1f glUniform1f;
static qglUniformMatrix3fv glUniformMatrix3fv;
static qglUniformMatrix4fv glUniformMatrix4fv;

enum graphics_card_vendors {
    VENDOR_UNKNOWN,
    VENDOR_NVIDIA,
    VENDOR_AMD,
    VENDOR_INTEL
};

struct opengl_info {
    /* info */
    const char *vendor_str;
    const char *version_str;
    const char *extensions_str;
    const char *renderer_str;
    const char *glsl_version_str;
    enum graphics_card_vendors vendor;
    /* version */
    float version;
    int major_version;
    int minor_version;
    /* extensions */
    int glsl_available;
    int vertex_buffer_obj_available;
    int vertex_array_obj_available;
    int map_buffer_range_available;
    int fragment_program_available;
    int frame_buffer_object_available;
};
#endif

struct nk_x11_device {
#if NK_X11_LOAD_OPENGL_EXTENSIONS
    struct opengl_info info;
#endif
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

static struct nk_x11 {
    struct nk_x11_device ogl;
    struct nk_context ctx;
    struct nk_font_atlas atlas;
    Display *dpy;
    Window win;
} x11;

#ifdef __APPLE__
  #define NK_SHADER_VERSION "#version 150\n"
#else
  #define NK_SHADER_VERSION "#version 300 es\n"
#endif

#if NK_X11_LOAD_OPENGL_EXTENSIONS
NK_INTERN int
nk_x11_stricmpn(const char *a, const char *b, int len)
{
    int i = 0;
    for (i = 0; i < len && a[i] && b[i]; ++i)
        if (a[i] != b[i]) return 1;
    if (i != len) return 1;
    return 0;
}

NK_INTERN int
nk_x11_check_extension(struct opengl_info *GL, const char *ext)
{
    const char *start, *where, *term;
    where = strchr(ext, ' ');
    if (where || *ext == '\0')
        return FALSE;

    for (start = GL->extensions_str;;) {
        where = strstr((const char*)start, ext);
        if (!where) break;
        term = where + strlen(ext);
        if (where == start || *(where - 1) == ' ') {
            if (*term == ' ' || *term == '\0')
                return TRUE;
        }
        start = term;
    }
    return FALSE;
}

#define GL_EXT(name) (q##name)nk_gl_ext(#name)
NK_INTERN __GLXextFuncPtr
nk_gl_ext(const char *name)
{
    __GLXextFuncPtr func;
    func = glXGetProcAddress((const GLubyte*)name);
    if (!func) {
        fprintf(stdout, "[GL]: failed to load extension: %s", name);
        return NULL;
    }
    return func;
}

NK_INTERN int
nk_load_opengl(struct opengl_info *gl)
{
    int failed = FALSE;
    gl->version_str = (const char*)glGetString(GL_VERSION);
    glGetIntegerv(GL_MAJOR_VERSION, &gl->major_version);
    glGetIntegerv(GL_MINOR_VERSION, &gl->minor_version);
    if (gl->major_version < 2) {
        fprintf(stderr, "[GL]: Graphics card does not fullfill minimum OpenGL 2.0 support\n");
        return 0;
    }

    gl->version = (float)gl->major_version + (float)gl->minor_version * 0.1f;
    gl->renderer_str = (const char*)glGetString(GL_RENDERER);
    gl->extensions_str = (const char*)glGetString(GL_EXTENSIONS);
    gl->glsl_version_str = (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION);
    gl->vendor_str = (const char*)glGetString(GL_VENDOR);
    if (!nk_x11_stricmpn(gl->vendor_str, "ATI", 4) ||
        !nk_x11_stricmpn(gl->vendor_str, "AMD", 4))
        gl->vendor = VENDOR_AMD;
    else if (!nk_x11_stricmpn(gl->vendor_str, "NVIDIA", 6))
        gl->vendor = VENDOR_NVIDIA;
    else if (!nk_x11_stricmpn(gl->vendor_str, "Intel", 5))
        gl->vendor = VENDOR_INTEL;
    else gl->vendor = VENDOR_UNKNOWN;

    /* Extensions */
    gl->glsl_available = (gl->version >= 2.0f);
    if (gl->glsl_available) {
        /* GLSL core in OpenGL > 2 */
        glCreateShader = GL_EXT(glCreateShader);
        glShaderSource = GL_EXT(glShaderSource);
        glCompileShader = GL_EXT(glCompileShader);
        glGetShaderiv = GL_EXT(glGetShaderiv);
        glGetShaderInfoLog = GL_EXT(glGetShaderInfoLog);
        glDeleteShader = GL_EXT(glDeleteShader);
        glCreateProgram = GL_EXT(glCreateProgram);
        glAttachShader = GL_EXT(glAttachShader);
        glDetachShader = GL_EXT(glDetachShader);
        glLinkProgram = GL_EXT(glLinkProgram);
        glUseProgram = GL_EXT(glUseProgram);
        glGetProgramiv = GL_EXT(glGetProgramiv);
        glGetProgramInfoLog = GL_EXT(glGetProgramInfoLog);
        glDeleteProgram = GL_EXT(glDeleteProgram);
        glGetUniformLocation = GL_EXT(glGetUniformLocation);
        glGetAttribLocation = GL_EXT(glGetAttribLocation);
        glUniform1i = GL_EXT(glUniform1i);
        glUniform1f = GL_EXT(glUniform1f);
        glUniformMatrix3fv = GL_EXT(glUniformMatrix3fv);
        glUniformMatrix4fv = GL_EXT(glUniformMatrix4fv);
    }
    gl->vertex_buffer_obj_available = nk_x11_check_extension(gl, "GL_ARB_vertex_buffer_object");
    if (gl->vertex_buffer_obj_available) {
        /* GL_ARB_vertex_buffer_object */
        glGenBuffers = GL_EXT(glGenBuffers);
        glBindBuffer = GL_EXT(glBindBuffer);
        glBufferData = GL_EXT(glBufferData);
        glBufferSubData = GL_EXT(glBufferSubData);
        glMapBuffer = GL_EXT(glMapBuffer);
        glUnmapBuffer = GL_EXT(glUnmapBuffer);
        glDeleteBuffers = GL_EXT(glDeleteBuffers);
    }
    gl->fragment_program_available = nk_x11_check_extension(gl, "GL_ARB_fragment_program");
    if (gl->fragment_program_available) {
        /* GL_ARB_vertex_program / GL_ARB_fragment_program  */
        glVertexAttribPointer = GL_EXT(glVertexAttribPointer);
        glEnableVertexAttribArray = GL_EXT(glEnableVertexAttribArray);
        glDisableVertexAttribArray = GL_EXT(glDisableVertexAttribArray);
    }
    gl->vertex_array_obj_available = nk_x11_check_extension(gl, "GL_ARB_vertex_array_object");
    if (gl->vertex_array_obj_available) {
        /* GL_ARB_vertex_array_object */
        glGenVertexArrays = GL_EXT(glGenVertexArrays);
        glBindVertexArray = GL_EXT(glBindVertexArray);
        glDeleteVertexArrays = GL_EXT(glDeleteVertexArrays);
    }
    gl->frame_buffer_object_available = nk_x11_check_extension(gl, "GL_ARB_framebuffer_object");
    if (gl->frame_buffer_object_available) {
        /* GL_ARB_framebuffer_object */
        glGenerateMipmap = GL_EXT(glGenerateMipmap);
    }
    if (!gl->vertex_buffer_obj_available) {
        fprintf(stdout, "[GL] Error: GL_ARB_vertex_buffer_object is not available!\n");
        failed = TRUE;
    }
    if (!gl->fragment_program_available) {
        fprintf(stdout, "[GL] Error: GL_ARB_fragment_program is not available!\n");
        failed = TRUE;
    }
    if (!gl->vertex_array_obj_available) {
        fprintf(stdout, "[GL] Error: GL_ARB_vertex_array_object is not available!\n");
        failed = TRUE;
    }
    if (!gl->frame_buffer_object_available) {
        fprintf(stdout, "[GL] Error: GL_ARB_framebuffer_object is not available!\n");
        failed = TRUE;
    }
    return !failed;
}
#endif

NK_API int
nk_x11_device_create(void)
{
    GLint status;
    static const GLchar *vertex_shader =
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

    struct nk_x11_device *dev = &x11.ogl;
#if NK_X11_LOAD_OPENGL_EXTENSIONS
    if (!nk_load_opengl(&dev->info)) return 0;
#endif
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
        GLsizei vs = sizeof(struct nk_draw_vertex);
        size_t vp = offsetof(struct nk_draw_vertex, position);
        size_t vt = offsetof(struct nk_draw_vertex, uv);
        size_t vc = offsetof(struct nk_draw_vertex, col);

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
    return 1;
}

NK_INTERN void
nk_x11_device_upload_atlas(const void *image, int width, int height)
{
    struct nk_x11_device *dev = &x11.ogl;
    glGenTextures(1, &dev->font_tex);
    glBindTexture(GL_TEXTURE_2D, dev->font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, image);
}

NK_API void
nk_x11_device_destroy(void)
{
    struct nk_x11_device *dev = &x11.ogl;
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
nk_x11_render(enum nk_anti_aliasing AA, int max_vertex_buffer, int max_element_buffer)
{
    int width, height;
    XWindowAttributes attr;
    struct nk_x11_device *dev = &x11.ogl;
    GLfloat ortho[4][4] = {
        {2.0f, 0.0f, 0.0f, 0.0f},
        {0.0f,-2.0f, 0.0f, 0.0f},
        {0.0f, 0.0f,-1.0f, 0.0f},
        {-1.0f,1.0f, 0.0f, 1.0f},
    };
    XGetWindowAttributes(x11.dpy, x11.win, &attr);
    width = attr.width;
    height = attr.height;

    ortho[0][0] /= (GLfloat)width;
    ortho[1][1] /= (GLfloat)height;

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
    glViewport(0,0,(GLsizei)width,(GLsizei)height);
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

        /* load draw vertices & elements directly into vertex + element buffer */
        vertices = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        elements = glMapBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_WRITE_ONLY);
        {
            /* fill converting configuration */
            struct nk_convert_config config;
            memset(&config, 0, sizeof(config));
            config.global_alpha = 1.0f;
            config.shape_AA = AA;
            config.line_AA = AA;
            config.circle_segment_count = 22;
            config.curve_segment_count = 22;
            config.arc_segment_count = 22;
            config.null = dev->null;

            /* setup buffers to load vertices and elements */
            {struct nk_buffer vbuf, ebuf;
            nk_buffer_init_fixed(&vbuf, vertices, (size_t)max_vertex_buffer);
            nk_buffer_init_fixed(&ebuf, elements, (size_t)max_element_buffer);
            nk_convert(&x11.ctx, &dev->cmds, &vbuf, &ebuf, &config);}
        }
        glUnmapBuffer(GL_ARRAY_BUFFER);
        glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

        /* iterate over and execute each draw command */
        nk_draw_foreach(cmd, &x11.ctx, &dev->cmds)
        {
            if (!cmd->elem_count) continue;
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor(
                (GLint)(cmd->clip_rect.x),
                (GLint)((height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h))),
                (GLint)(cmd->clip_rect.w),
                (GLint)(cmd->clip_rect.h));
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, offset);
            offset += cmd->elem_count;
        }
        nk_clear(&x11.ctx);
    }

    /* default OpenGL state */
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
}

NK_API void
nk_x11_font_stash_begin(struct nk_font_atlas **atlas)
{
    nk_font_atlas_init_default(&x11.atlas);
    nk_font_atlas_begin(&x11.atlas);
    *atlas = &x11.atlas;
}

NK_API void
nk_x11_font_stash_end(void)
{
    const void *image; int w, h;
    image = nk_font_atlas_bake(&x11.atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    nk_x11_device_upload_atlas(image, w, h);
    nk_font_atlas_end(&x11.atlas, nk_handle_id((int)x11.ogl.font_tex), &x11.ogl.null);
    if (x11.atlas.default_font)
        nk_style_set_font(&x11.ctx, &x11.atlas.default_font->handle);
}

NK_API void
nk_x11_handle_event(XEvent *evt)
{
    struct nk_context *ctx = &x11.ctx;
    if (evt->type == KeyPress || evt->type == KeyRelease)
    {
        /* Key handler */
        int ret, down = (evt->type == KeyPress);
        KeySym *code = XGetKeyboardMapping(x11.dpy, (KeyCode)evt->xkey.keycode, 1, &ret);
        if (*code == XK_Shift_L || *code == XK_Shift_R) nk_input_key(ctx, NK_KEY_SHIFT, down);
        else if (*code == XK_Delete)    nk_input_key(ctx, NK_KEY_DEL, down);
        else if (*code == XK_Return)    nk_input_key(ctx, NK_KEY_ENTER, down);
        else if (*code == XK_Tab)       nk_input_key(ctx, NK_KEY_TAB, down);
        else if (*code == XK_Left)      nk_input_key(ctx, NK_KEY_LEFT, down);
        else if (*code == XK_Right)     nk_input_key(ctx, NK_KEY_RIGHT, down);
        else if (*code == XK_BackSpace) nk_input_key(ctx, NK_KEY_BACKSPACE, down);
        else if (*code == XK_Home)  nk_input_key(ctx, NK_KEY_TEXT_START, down);
        else if (*code == XK_End)  nk_input_key(ctx, NK_KEY_TEXT_END, down);
        else if (*code == XK_space && !down) nk_input_char(ctx, ' ');
        else {
            if (*code == 'c' && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_COPY, down);
            else if (*code == 'v' && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_PASTE, down);
            else if (*code == 'x' && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_CUT, down);
            else if (*code == 'z' && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_TEXT_UNDO, down);
            else if (*code == 'r' && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_TEXT_REDO, down);
            else if (*code == XK_Left && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
            else if (*code == XK_Right && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
            else if (*code == 'b' && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down);
            else if (*code == 'e' && (evt->xkey.state & ControlMask))
                nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down);
            else if (!down) {
                char buf[32];
                KeySym keysym = 0;
                if (XLookupString((XKeyEvent*)evt, buf, 32, &keysym, NULL) != NoSymbol)
                    nk_input_glyph(ctx, buf);
            }
        }
        XFree(code);
    } else if (evt->type == ButtonPress || evt->type == ButtonRelease) {
        /* Button handler */
        int down = (evt->type == ButtonPress);
        const int x = evt->xbutton.x, y = evt->xbutton.y;
        if (evt->xbutton.button == Button1)
            nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
        if (evt->xbutton.button == Button2)
            nk_input_button(ctx, NK_BUTTON_MIDDLE, x, y, down);
        else if (evt->xbutton.button == Button3)
            nk_input_button(ctx, NK_BUTTON_RIGHT, x, y, down);
        else if (evt->xbutton.button == Button4)
            nk_input_scroll(ctx, 1.0f);
        else if (evt->xbutton.button == Button5)
            nk_input_scroll(ctx, -1.0f);

    } else if (evt->type == MotionNotify) {
        /* Mouse motion handler */
        const int x = evt->xmotion.x, y = evt->xmotion.y;
        nk_input_motion(ctx, x, y);
    } else if (evt->type == KeymapNotify)
        XRefreshKeyboardMapping(&evt->xmapping);
}

NK_API struct nk_context*
nk_x11_init(Display *dpy, Window win)
{
    if (!setlocale(LC_ALL,"")) return 0;
    if (!XSupportsLocale()) return 0;
    if (!XSetLocaleModifiers("@im=none")) return 0;
    if (!nk_x11_device_create()) return 0;
    x11.dpy = dpy;
    x11.win = win;
    nk_init_default(&x11.ctx, 0);
    return &x11.ctx;
}

NK_API void
nk_x11_shutdown(void)
{
    nk_font_atlas_clear(&x11.atlas);
    nk_free(&x11.ctx);
    nk_x11_device_destroy();
    memset(&x11, 0, sizeof(x11));
}

