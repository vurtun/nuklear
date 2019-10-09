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
#ifndef NK_XLIB_GL2_H_
#define NK_XLIB_GL2_H_

#include <X11/Xlib.h>
NK_API struct nk_context*   nk_x11_init(Display *dpy, Window win);
NK_API void                 nk_x11_font_stash_begin(struct nk_font_atlas **atlas);
NK_API void                 nk_x11_font_stash_end(void);
NK_API int                  nk_x11_handle_event(XEvent *evt);
NK_API void                 nk_x11_render(enum nk_anti_aliasing, int max_vertex_buffer, int max_element_buffer);
NK_API void                 nk_x11_shutdown(void);

#endif
/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#ifdef NK_XLIB_GL2_IMPLEMENTATION
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/time.h>
#include <unistd.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xlocale.h>

#include <GL/gl.h>

#ifndef NK_X11_DOUBLE_CLICK_LO
#define NK_X11_DOUBLE_CLICK_LO 20
#endif
#ifndef NK_X11_DOUBLE_CLICK_HI
#define NK_X11_DOUBLE_CLICK_HI 200
#endif

struct nk_x11_vertex {
    float position[2];
    float uv[2];
    nk_byte col[4];
};

struct nk_x11_device {
    struct nk_buffer cmds;
    struct nk_draw_null_texture null;
    GLuint font_tex;
};

static struct nk_x11 {
    struct nk_x11_device ogl;
    struct nk_context ctx;
    struct nk_font_atlas atlas;
    Cursor cursor;
    Display *dpy;
    Window win;
    long last_button_click;
} x11;

NK_INTERN long
nk_timestamp(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0) return 0;
    return (long)((long)tv.tv_sec * 1000 + (long)tv.tv_usec/1000);
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
nk_x11_render(enum nk_anti_aliasing AA, int max_vertex_buffer, int max_element_buffer)
{
    /* setup global state */
    struct nk_x11_device *dev = &x11.ogl;
    int width, height;

    XWindowAttributes attr;
    XGetWindowAttributes(x11.dpy, x11.win, &attr);
    width = attr.width;
    height = attr.height;

    glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT|GL_TRANSFORM_BIT);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glEnable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    /* setup viewport/project */
    glViewport(0,0,(GLsizei)width,(GLsizei)height);
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();
    glLoadIdentity();
    glOrtho(0.0f, width, height, 0.0f, -1.0f, 1.0f);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    {
        GLsizei vs = sizeof(struct nk_x11_vertex);
        size_t vp = offsetof(struct nk_x11_vertex, position);
        size_t vt = offsetof(struct nk_x11_vertex, uv);
        size_t vc = offsetof(struct nk_x11_vertex, col);

        /* convert from command queue into draw list and draw to screen */
        const struct nk_draw_command *cmd;
        const nk_draw_index *offset = NULL;
        struct nk_buffer vbuf, ebuf;

        /* fill convert configuration */
        struct nk_convert_config config;
        static const struct nk_draw_vertex_layout_element vertex_layout[] = {
            {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_x11_vertex, position)},
            {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_x11_vertex, uv)},
            {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_x11_vertex, col)},
            {NK_VERTEX_LAYOUT_END}
        };
        NK_MEMSET(&config, 0, sizeof(config));
        config.vertex_layout = vertex_layout;
        config.vertex_size = sizeof(struct nk_x11_vertex);
        config.vertex_alignment = NK_ALIGNOF(struct nk_x11_vertex);
        config.null = dev->null;
        config.circle_segment_count = 22;
        config.curve_segment_count = 22;
        config.arc_segment_count = 22;
        config.global_alpha = 1.0f;
        config.shape_AA = AA;
        config.line_AA = AA;

        /* convert shapes into vertexes */
        nk_buffer_init_default(&vbuf);
        nk_buffer_init_default(&ebuf);
        nk_convert(&x11.ctx, &dev->cmds, &vbuf, &ebuf, &config);

        /* setup vertex buffer pointer */
        {const void *vertices = nk_buffer_memory_const(&vbuf);
        glVertexPointer(2, GL_FLOAT, vs, (const void*)((const nk_byte*)vertices + vp));
        glTexCoordPointer(2, GL_FLOAT, vs, (const void*)((const nk_byte*)vertices + vt));
        glColorPointer(4, GL_UNSIGNED_BYTE, vs, (const void*)((const nk_byte*)vertices + vc));}

        /* iterate over and execute each draw command */
        offset = (const nk_draw_index*)nk_buffer_memory_const(&ebuf);
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
        nk_buffer_free(&vbuf);
        nk_buffer_free(&ebuf);
    }

    /* default OpenGL state */
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);

    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, 0);
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glPopAttrib();

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

NK_API int
nk_x11_handle_event(XEvent *evt)
{
    struct nk_context *ctx = &x11.ctx;

    /* optional grabbing behavior */
    if (ctx->input.mouse.grab) {
        XDefineCursor(x11.dpy, x11.win, x11.cursor);
        ctx->input.mouse.grab = 0;
    } else if (ctx->input.mouse.ungrab) {
        XWarpPointer(x11.dpy, None, x11.win, 0, 0, 0, 0,
            (int)ctx->input.mouse.pos.x, (int)ctx->input.mouse.pos.y);
        XUndefineCursor(x11.dpy, x11.win);
        ctx->input.mouse.ungrab = 0;
    }

    if (evt->type == KeyPress || evt->type == KeyRelease)
    {
        /* Key handler */
        int ret, down = (evt->type == KeyPress);
        KeySym *code = XGetKeyboardMapping(x11.dpy, (KeyCode)evt->xkey.keycode, 1, &ret);
        if (*code == XK_Shift_L || *code == XK_Shift_R) nk_input_key(ctx, NK_KEY_SHIFT, down);
        else if (*code == XK_Control_L || *code == XK_Control_R) nk_input_key(ctx, NK_KEY_CTRL, down);
        else if (*code == XK_Delete)    nk_input_key(ctx, NK_KEY_DEL, down);
        else if (*code == XK_Return)    nk_input_key(ctx, NK_KEY_ENTER, down);
        else if (*code == XK_Tab)       nk_input_key(ctx, NK_KEY_TAB, down);
        else if (*code == XK_Left)      nk_input_key(ctx, NK_KEY_LEFT, down);
        else if (*code == XK_Right)     nk_input_key(ctx, NK_KEY_RIGHT, down);
        else if (*code == XK_Up)      nk_input_key(ctx, NK_KEY_UP, down);
        else if (*code == XK_Down)     nk_input_key(ctx, NK_KEY_DOWN, down);
        else if (*code == XK_BackSpace) nk_input_key(ctx, NK_KEY_BACKSPACE, down);
        else if (*code == XK_space && !down) nk_input_char(ctx, ' ');
        else if (*code == XK_Page_Up)   nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
        else if (*code == XK_Page_Down) nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
        else if (*code == XK_Home) {
            nk_input_key(ctx, NK_KEY_TEXT_START, down);
            nk_input_key(ctx, NK_KEY_SCROLL_START, down);
        } else if (*code == XK_End) {
            nk_input_key(ctx, NK_KEY_TEXT_END, down);
            nk_input_key(ctx, NK_KEY_SCROLL_END, down);
        } else {
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
            else {
                if (*code == 'i')
                    nk_input_key(ctx, NK_KEY_TEXT_INSERT_MODE, down);
                else if (*code == 'r')
                    nk_input_key(ctx, NK_KEY_TEXT_REPLACE_MODE, down);
                if (down) {
                    char buf[32];
                    KeySym keysym = 0;
                    if (XLookupString((XKeyEvent*)evt, buf, 32, &keysym, NULL) != NoSymbol)
                        nk_input_glyph(ctx, buf);
                }
            }
        }
        XFree(code);
        return 1;
    } else if (evt->type == ButtonPress || evt->type == ButtonRelease) {
        /* Button handler */
        int down = (evt->type == ButtonPress);
        const int x = evt->xbutton.x, y = evt->xbutton.y;
        if (evt->xbutton.button == Button1) {
            if (down) { /* Double-Click Button handler */
                long dt = nk_timestamp() - x11.last_button_click;
                if (dt > NK_X11_DOUBLE_CLICK_LO && dt < NK_X11_DOUBLE_CLICK_HI)
                    nk_input_button(ctx, NK_BUTTON_DOUBLE, x, y, nk_true);
                x11.last_button_click = nk_timestamp();
            } else nk_input_button(ctx, NK_BUTTON_DOUBLE, x, y, nk_false);
            nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
        } else if (evt->xbutton.button == Button2)
            nk_input_button(ctx, NK_BUTTON_MIDDLE, x, y, down);
        else if (evt->xbutton.button == Button3)
            nk_input_button(ctx, NK_BUTTON_RIGHT, x, y, down);
        else if (evt->xbutton.button == Button4)
            nk_input_scroll(ctx, nk_vec2(0,1.0f));
        else if (evt->xbutton.button == Button5)
            nk_input_scroll(ctx, nk_vec2(0,-1.0f));
        else return 0;
        return 1;
    } else if (evt->type == MotionNotify) {
        /* Mouse motion handler */
        const int x = evt->xmotion.x, y = evt->xmotion.y;
        nk_input_motion(ctx, x, y);
        if (ctx->input.mouse.grabbed) {
            ctx->input.mouse.pos.x = ctx->input.mouse.prev.x;
            ctx->input.mouse.pos.y = ctx->input.mouse.prev.y;
            XWarpPointer(x11.dpy, None, x11.win, 0, 0, 0, 0, (int)ctx->input.mouse.pos.x, (int)ctx->input.mouse.pos.y);
        }
        return 1;
    } else if (evt->type == KeymapNotify) {
        XRefreshKeyboardMapping(&evt->xmapping);
        return 1;
    }
    return 0;
}

NK_API struct nk_context*
nk_x11_init(Display *dpy, Window win)
{
    x11.dpy = dpy;
    x11.win = win;

    if (!setlocale(LC_ALL,"")) return 0;
    if (!XSupportsLocale()) return 0;
    if (!XSetLocaleModifiers("@im=none")) return 0;

    /* create invisible cursor */
    {static XColor dummy; char data[1] = {0};
    Pixmap blank = XCreateBitmapFromData(dpy, win, data, 1, 1);
    if (blank == None) return 0;
    x11.cursor = XCreatePixmapCursor(dpy, blank, blank, &dummy, &dummy, 0, 0);
    XFreePixmap(dpy, blank);}

    nk_buffer_init_default(&x11.ogl.cmds);
    nk_init_default(&x11.ctx, 0);
    return &x11.ctx;
}

NK_API void
nk_x11_shutdown(void)
{
    struct nk_x11_device *dev = &x11.ogl;
    nk_font_atlas_clear(&x11.atlas);
    nk_free(&x11.ctx);
    glDeleteTextures(1, &dev->font_tex);
    nk_buffer_free(&dev->cmds);
    XFreeCursor(x11.dpy, x11.cursor);
    memset(&x11, 0, sizeof(x11));
}

#endif
