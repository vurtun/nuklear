/* nuklear - 1.40.8 - public domain */
#define  _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_VERTEX_BUFFER_OUTPUT
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_IMPLEMENTATION
#define NK_WAYLAND_GLES2_IMPLEMENTATION
#include "../../nuklear.h"
#include "nuklear_wayland_gles2.h"

#include <wayland-client.h>
#include <wayland-egl.h>
#include <xkbcommon/xkbcommon.h>
#include <EGL/egl.h>
#include <EGL/eglplatform.h>
#include <linux/input-event-codes.h>

#include "xdg-shell-client-protocol.h"
#include "relative-pointer-unstable-v1-client-protocol.h"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

#define UNUSED(a) (void)a
#define LEN(a) (sizeof(a)/sizeof(a)[0])

#define DOUBLE_CLICK_MS 200

/* ===============================================================
 *
 *                          EXAMPLE
 *
 * ===============================================================*/
/* This are some code examples to provide a small overview of what can be
 * done with this library. To try out an example uncomment the include
 * and the corresponding function. */
/*#include "../style.c"*/
/*#include "../calculator.c"*/
/*#include "../overview.c"*/
/*#include "../node_editor.c"*/

/* ===============================================================
 *
 *                          DEMO
 *
 * ===============================================================*/


/* Platform */
struct demo_state {
    struct nk_wayland *wayland;
    struct wl_display *wl_display;
    struct wl_compositor *compositor;
    struct wl_seat *seat;
    struct wl_list outputs;
    struct wl_data_device_manager *data_device_manager;
    struct xdg_wm_base *wm_base;
    struct wl_data_device *data_device;
    struct zwp_relative_pointer_manager_v1 *relative_manager;
    struct wl_pointer *pointer;
    struct zwp_relative_pointer_v1 *relative_pointer;
    struct wl_keyboard *keyboard;
    struct xkb_keymap *keymap;
    struct xkb_state *xkb_state;
    struct wl_surface *wl_surface;
    struct wl_egl_window *window;
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    struct wl_callback *frame_cb;
    int running;
    int32_t new_width;
    int32_t new_height;
    int32_t width;
    int32_t height;
    int32_t scale;
    uint32_t input_serial;
    uint32_t last_click;
    double last_mouse_x;
    double last_mouse_y;
    double mouse_frac_x;
    double mouse_frac_y;
    double wheel_x;
    double wheel_y;
    struct wl_data_source *source;
    struct wl_data_offer *offer;
    char *clipboard;
    size_t clipboard_length;
};

struct demo_output {
    struct demo_state *state;
    uint32_t name;
    struct wl_output *output;
    int32_t scale;
    int entered;
    struct wl_list link;
};

static void die(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputs("\n", stderr);
    exit(EXIT_FAILURE);
}

static void noop() {
    // do nothing
}

static int32_t get_scale(struct demo_state *state, int force) {
    int32_t scale = 1;
    struct demo_output *output;
    wl_list_for_each(output, &state->outputs, link) {
        if (force || output->entered) {
            scale = NK_MAX(scale, output->scale);
        }
    }
    return scale;
}

static void queue_redraw(struct demo_state *state);

static void bind_gl_context(struct demo_state *state) {
    if (!eglMakeCurrent(state->display, state->surface, state->surface,
                state->context)) die("Failed to attach EGL context");

    if(!eglBindAPI(EGL_OPENGL_ES_API)) die("Failed to bind GLES2 API");
}

static void draw_frame(struct demo_state *state, uint32_t time) {
    UNUSED(time);

    struct nk_context *ctx = nk_wayland_get_context(state->wayland);
    nk_input_end(ctx);
    bind_gl_context(state);

    /* GUI */
    if (nk_begin(ctx, "Demo", nk_rect(50, 50, 200, 200),
                NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
                NK_WINDOW_CLOSABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
    {
        nk_menubar_begin(ctx);
        nk_layout_row_begin(ctx, NK_STATIC, 25, 2);
        nk_layout_row_push(ctx, 45);
        if (nk_menu_begin_label(ctx, "FILE", NK_TEXT_LEFT, nk_vec2(120, 200))) {
            nk_layout_row_dynamic(ctx, 30, 1);
            nk_menu_item_label(ctx, "OPEN", NK_TEXT_LEFT);
            nk_menu_item_label(ctx, "CLOSE", NK_TEXT_LEFT);
            nk_menu_end(ctx);
        }
        nk_layout_row_push(ctx, 45);
        if (nk_menu_begin_label(ctx, "EDIT", NK_TEXT_LEFT, nk_vec2(120, 200))) {
            nk_layout_row_dynamic(ctx, 30, 1);
            nk_menu_item_label(ctx, "COPY", NK_TEXT_LEFT);
            nk_menu_item_label(ctx, "CUT", NK_TEXT_LEFT);
            nk_menu_item_label(ctx, "PASTE", NK_TEXT_LEFT);
            nk_menu_end(ctx);
        }
        nk_layout_row_end(ctx);
        nk_menubar_end(ctx);

        enum {EASY, HARD};
        static int op = EASY;
        static int property = 20;
        nk_layout_row_static(ctx, 30, 80, 1);
        if (nk_button_label(ctx, "button"))
            fprintf(stdout, "button pressed");
        nk_layout_row_dynamic(ctx, 30, 2);
        if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
        if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;
        nk_layout_row_dynamic(ctx, 25, 1);
        nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);
    }
    nk_end(ctx);

    /* -------------- EXAMPLES ---------------- */
    /*calculator(ctx);*/
    /*overview(ctx);*/
    /*node_editor(ctx);*/
    /* ----------------------------------------- */

    /* Draw */
    {float bg[4];
    nk_color_fv(bg, nk_rgba(28,48,62,255));
    int width = state->width * state->scale;
    int height = state->height * state->scale;
    glViewport(0, 0, width, height);
    glClear(GL_COLOR_BUFFER_BIT);
    glClearColor(bg[0], bg[1], bg[2], bg[3]);
    /* IMPORTANT: `nk_wayland_render` modifies some global OpenGL state
     * with blending, scissor, face culling, depth test and viewport and
     * defaults everything back into a default state.
     * Make sure to either a.) save and restore or b.) reset your own state after
     * rendering the UI. */
    nk_wayland_render(state->wayland, width, height,
            state->scale, NK_ANTI_ALIASING_ON,
            MAX_VERTEX_MEMORY, MAX_ELEMENT_MEMORY);
    eglSwapBuffers(state->display, state->surface);}

    nk_input_begin(ctx);
}

static void output_handle_done(void *data,
        struct wl_output *wl_output) {
    struct demo_output *output = data;
    if (output->link.next == NULL) {
        wl_list_insert(&output->state->outputs, &output->link);
    }
    if (output->state->wl_surface) {
        int32_t scale = get_scale(output->state, nk_false);
        if (scale != output->state->scale) {
            output->state->scale = scale;
            wl_surface_set_buffer_scale(output->state->wl_surface, scale);
            queue_redraw(output->state);
        }
    }

}

static void output_handle_scale(void *data,
        struct wl_output *wl_output,
        int32_t factor) {
    struct demo_output *output = data;
    output->scale = factor;
}

static const struct wl_output_listener output_listener = {
    noop,
    noop,
    output_handle_done,
    output_handle_scale
};

static void handle_motion(struct demo_state *state,
        wl_fixed_t surface_x,
        wl_fixed_t surface_y) {
    double x = wl_fixed_to_double(surface_x);
    double y = wl_fixed_to_double(surface_y);
    double dx = 0.;
    double dy = 0.;
    if (state->last_mouse_x >= 0. && state->last_mouse_y >= 0.) {
        dx = x - state->last_mouse_x + state->mouse_frac_x;
        dy = y - state->last_mouse_y + state->mouse_frac_y;
    }
    if (((int) dx) || ((int) dy)) {
        if (state->relative_pointer) {
            dx = 0.;
            dy = 0.;
        } else {
            state->mouse_frac_x = modf(dx, &dx);
            state->mouse_frac_y = modf(dy, &dy);
        }
        if (nk_wayland_handle_pointer_motion(state->wayland, x, y, dx, dy)) {
            queue_redraw(state);
        }
    }
    state->last_mouse_x = x;
    state->last_mouse_y = y;
}

static void pointer_handle_enter(void *data,
        struct wl_pointer *wl_pointer,
        uint32_t serial,
        struct wl_surface *surface,
        wl_fixed_t surface_x,
        wl_fixed_t surface_y) {
    struct demo_state *state = data;
    if (surface == state->wl_surface) {
        handle_motion(state, surface_x, surface_y);
    }
}

static void pointer_handle_leave(void *data,
        struct wl_pointer *pointer,
        uint32_t serial,
        struct wl_surface *surface) {
    struct demo_state *state = data;
    if (surface == state->wl_surface
            && nk_wayland_handle_pointer_motion(
                state->wayland, -1, -1, 0, 0)) {
        queue_redraw(state);
    }
}

static void pointer_handle_motion(void *data,
        struct wl_pointer *pointer,
        uint32_t time,
        wl_fixed_t surface_x,
        wl_fixed_t surface_y) {
    struct demo_state *state = data;
    handle_motion(state, surface_x, surface_y);
}

static void pointer_handle_button(void *data,
        struct wl_pointer *pointer,
        uint32_t serial,
        uint32_t time,
        uint32_t button,
        uint32_t pointer_state) {
    struct demo_state *state = data;
    state->input_serial = serial;
    int32_t b = -1;
    if (button == BTN_LEFT) {
        state->last_click = time;
        if (time - state->last_click < DOUBLE_CLICK_MS)
            nk_wayland_handle_pointer_button(state->wayland, NK_BUTTON_DOUBLE, pointer_state);
        nk_wayland_handle_pointer_button(state->wayland, NK_BUTTON_LEFT, pointer_state);
    } else {
        state->last_click = -1;
        if (button == BTN_MIDDLE)
            nk_wayland_handle_pointer_button(state->wayland, NK_BUTTON_MIDDLE, pointer_state);
        else if (button == BTN_RIGHT)
            nk_wayland_handle_pointer_button(state->wayland, NK_BUTTON_RIGHT, pointer_state);
        else return;
    }
    queue_redraw(state);
}

static void pointer_handle_axis(void *data,
        struct wl_pointer *pointer,
        uint32_t time,
        uint32_t axis,
        wl_fixed_t value) {
    struct demo_state *state = data;
    if (axis == WL_POINTER_AXIS_HORIZONTAL_SCROLL)
        state->wheel_x += wl_fixed_to_double(value);
    else if (axis == WL_POINTER_AXIS_VERTICAL_SCROLL)
        state->wheel_y += wl_fixed_to_double(value);
}

static void pointer_handle_frame(void *data,
        struct wl_pointer *pointer) {
    struct demo_state *state = data;
    if (state->wheel_x || state->wheel_y) {
        if (nk_wayland_handle_pointer_wheel(state->wayland,
                    state->wheel_x, state->wheel_y)) {
            queue_redraw(state);
        }
        state->wheel_x = 0.;
        state->wheel_y = 0.;
    }
}

static struct wl_pointer_listener pointer_listener = {
    pointer_handle_enter,
    pointer_handle_leave,
    pointer_handle_motion,
    pointer_handle_button,
    pointer_handle_axis,
    pointer_handle_frame,
    noop,
    noop,
    noop
};

static void relative_pointer_handle_relative_motion(void *data,
        struct zwp_relative_pointer_v1 *relative_pointer,
        uint32_t utime_hi,
        uint32_t utime_lo,
        wl_fixed_t dx_accel,
        wl_fixed_t dy_accel,
        wl_fixed_t dx_unaccel,
        wl_fixed_t dy_unaccel) {
    struct demo_state *state = data;
    double dx = wl_fixed_to_double(dx_accel) + state->mouse_frac_x;
    double dy = wl_fixed_to_double(dy_accel) + state->mouse_frac_y;
    state->mouse_frac_x = modf(dx, &dx);
    state->mouse_frac_y = modf(dy, &dy);
    if (nk_wayland_handle_pointer_motion(state->wayland, -1., -1., dx, dy)) {
        queue_redraw(state);
    }
}

static struct zwp_relative_pointer_v1_listener relative_pointer_listener = {
    relative_pointer_handle_relative_motion
};

static void keyboard_handle_keymap(void *data,
        struct wl_keyboard *keyboard,
        uint32_t format,
        int32_t fd,
        uint32_t size) {
    struct demo_state *state = data;
    char *keymap = mmap(NULL, size, PROT_READ, MAP_SHARED, fd, 0);
    if (keymap != MAP_FAILED) {
        struct xkb_context *context = xkb_context_new(0);
        if (!context) die("Failed to create xkb context");

        if (state->keymap) xkb_keymap_unref(state->keymap);
        state->keymap = xkb_keymap_new_from_string(context, keymap, format, 0);
        if (!state->keymap) die("Failed to create xkb keymap");
        munmap(keymap, size);

        xkb_state_unref(state->xkb_state);
        state->xkb_state = xkb_state_new(state->keymap);
        if (!state->xkb_state) die("Failed to create xkb state");

        xkb_context_unref(context);
    }
    close(fd);
}

static void keyboard_handle_key(void *data,
        struct wl_keyboard *keyboard,
        uint32_t serial,
        uint32_t time,
        uint32_t key,
        uint32_t keyboard_state) {
    struct demo_state *state = data;
    state->input_serial = serial;
    const xkb_keysym_t *syms;
    int sym_count = xkb_state_key_get_syms(state->xkb_state, key + 8, &syms);
    xkb_mod_mask_t xkb_mods = xkb_state_serialize_mods(state->xkb_state,
            XKB_STATE_MODS_EFFECTIVE);
    uint32_t mods = 0;
    if (xkb_mods & (1 << xkb_keymap_mod_get_index(state->keymap, XKB_MOD_NAME_SHIFT)))
        mods |= MOD_SHIFT;
    if (xkb_mods & (1 << xkb_keymap_mod_get_index(state->keymap, XKB_MOD_NAME_CAPS)))
        mods |= MOD_CAPS;
    if (xkb_mods & (1 << xkb_keymap_mod_get_index(state->keymap, XKB_MOD_NAME_CTRL)))
        mods |= MOD_CTRL;
    if (xkb_mods & (1 << xkb_keymap_mod_get_index(state->keymap, XKB_MOD_NAME_ALT)))
        mods |= MOD_ALT;
    if (xkb_mods & (1 << xkb_keymap_mod_get_index(state->keymap, XKB_MOD_NAME_NUM)))
        mods |= MOD_NUM;
    if (xkb_mods & (1 << xkb_keymap_mod_get_index(state->keymap, XKB_MOD_NAME_LOGO)))
        mods |= MOD_LOGO;

    for (int i = 0; i < sym_count; i++) {
        xkb_keysym_t sym = syms[i];
        if (nk_wayland_handle_key(state->wayland, sym, mods, keyboard_state)) {
            queue_redraw(state);
        }
    }


    if ((keyboard_state & WL_KEYBOARD_KEY_STATE_PRESSED)
            && !(mods & MOD_CTRL)) {
        char glyph[8] = "";
        int glyph_size = xkb_state_key_get_utf8(
                state->xkb_state, key + 8, glyph, sizeof(glyph));
        if (glyph_size) {
            glyph[glyph_size] = 0;
            nk_input_glyph(nk_wayland_get_context(state->wayland), glyph);
            queue_redraw(state);
        }
    }

}

static void keyboard_handle_modifiers(void *data,
        struct wl_keyboard *wl_keyboard,
        uint32_t serial,
        uint32_t mods_depressed,
        uint32_t mods_latched,
        uint32_t mods_locked,
        uint32_t group) {
    struct demo_state *state = data;
    state->input_serial = serial;
    xkb_state_update_mask(state->xkb_state,
            mods_depressed, mods_latched, mods_locked, group, 0, 0);
}

static struct wl_keyboard_listener keyboard_listener = {
    keyboard_handle_keymap,
    noop,
    noop,
    keyboard_handle_key,
    keyboard_handle_modifiers,
    noop
};

static void seat_handle_capabilities(void *data,
        struct wl_seat *seat,
        uint32_t capabilities) {
    struct demo_state *state = data;
    if ((capabilities & WL_SEAT_CAPABILITY_POINTER) && !state->pointer) {
        state->pointer = wl_seat_get_pointer(seat);
        wl_pointer_add_listener(state->pointer, &pointer_listener, state);
        if (state->relative_manager) {
            state->relative_pointer =
                zwp_relative_pointer_manager_v1_get_relative_pointer(
                        state->relative_manager, state->pointer);
            zwp_relative_pointer_v1_add_listener(state->relative_pointer,
                    &relative_pointer_listener, state);
        }
    } else if (!(capabilities & WL_SEAT_CAPABILITY_POINTER) && state->pointer) {
        wl_pointer_destroy(state->pointer);
        state->pointer = NULL;
        if (state->relative_pointer) {
            zwp_relative_pointer_v1_destroy(state->relative_pointer);
            state->relative_pointer = NULL;
        }
    }
    if ((capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && !state->keyboard) {
        state->keyboard = wl_seat_get_keyboard(seat);
        wl_keyboard_add_listener(state->keyboard, &keyboard_listener, state);
    } else if (!(capabilities & WL_SEAT_CAPABILITY_KEYBOARD) && state->keyboard) {
        wl_keyboard_destroy(state->keyboard);
        state->keyboard = NULL;
    }
}

static const struct wl_seat_listener seat_listener = {
    seat_handle_capabilities,
    noop
};

static void wm_base_handle_ping(void *data,
        struct xdg_wm_base *wm_base,
        uint32_t serial) {
    xdg_wm_base_pong(wm_base, serial);
}

static const struct xdg_wm_base_listener wm_base_listener = {
    wm_base_handle_ping
};

static void surface_handle_enter(void *data,
        struct wl_surface *surface,
        struct wl_output *wl_output) {
    struct demo_state *state = data;

    struct demo_output *output;
    wl_list_for_each(output, &state->outputs, link) {
        if (output->output == wl_output) {
            output->entered = nk_true;
            break;
        }
    }
}

static void surface_handle_leave(void *data,
        struct wl_surface *surface,
        struct wl_output *wl_output) {
    struct demo_state *state = data;
    struct demo_output *output;
    wl_list_for_each(output, &state->outputs, link) {
        if (output->output == wl_output) {
            output->entered = nk_false;
            break;
        }
    }
}

static const struct wl_surface_listener surface_listener = {
    surface_handle_enter,
    surface_handle_leave
};

static void surface_handle_configure(void *data,
        struct xdg_surface *surface,
        uint32_t serial) {
    struct demo_state *state = data;
    if (state->window
            && (state->new_width != state->width
                || state->new_height != state->height)) {
        state->width = state->new_width;
        state->height = state->new_height;
        wl_egl_window_resize(state->window,
                state->width * state->scale,
                state->height * state->scale, 0, 0);
        struct wl_region *region = wl_compositor_create_region(
                state->compositor);
        wl_region_add(region, 0, 0, state->width, state->height);
        wl_surface_set_opaque_region(state->wl_surface, region);
        wl_region_destroy(region);
        queue_redraw(state);
    }
    xdg_surface_ack_configure(surface, serial);
}

static const struct xdg_surface_listener xdg_surface_listener = {
    surface_handle_configure
};

static void toplevel_handle_configure(void *data,
        struct xdg_toplevel *xdg_toplevel,
        int32_t width,
        int32_t height,
        struct wl_array *states) {
    struct demo_state *state = data;
    if (width > 0 && height > 0) {
        state->new_width = width;
        state->new_height = height;
    }
}

static void toplevel_handle_close(void *data,
        struct xdg_toplevel *xdg_toplevel) {
    struct demo_state *state = data;
    state->running = nk_false;
}

static const struct xdg_toplevel_listener toplevel_listener = {
    toplevel_handle_configure,
    toplevel_handle_close
};

static void frame_ready(void *data,
        struct wl_callback *callback,
        uint32_t time) {
    struct demo_state *state = data;
    wl_callback_destroy(state->frame_cb);
    state->frame_cb = NULL;
    draw_frame(state, time);
}

static const struct wl_callback_listener frame_listener = {
    frame_ready
};

static void queue_redraw(struct demo_state *state) {
    if (state->frame_cb) return;
    state->frame_cb = wl_surface_frame(state->wl_surface);
    wl_callback_add_listener(state->frame_cb, &frame_listener, state);
    wl_surface_commit(state->wl_surface);
}

static const char *valid_mimetypes[] = {
    "text/plain;charset=utf-8",
    "text/plain",
    "UTF8_STRING",
    "STRING",
    "TEXT"
};

static void data_offer_handle_offer(void *data,
        struct wl_data_offer *data_offer,
        const char *mime_type) {
    const char *preferred = data;
    for (int i = 0; i < LEN(valid_mimetypes); i++) {
        if (strcmp(mime_type, valid_mimetypes[i]) == 0
                && (!preferred || preferred > valid_mimetypes[i])) {
            preferred = valid_mimetypes[i];
        }
    }
    wl_data_offer_set_user_data(data_offer, (void *) preferred);
}

static struct wl_data_offer_listener data_offer_listener = {
    data_offer_handle_offer,
    noop,
    noop
};

#define BUF_SIZE 4096

static char *read_all(int fd, size_t *length_out) {
    struct pollfd fds[1];
    fds[0].events = POLLIN;
    fds[0].fd = fd;
    if (poll(fds, LEN(fds), 1000) == -1) return NULL;
    char *contents = malloc(BUF_SIZE);
    if (contents == NULL) goto error;
    *length_out = 0;

    for (;;) {
        ssize_t count = read(fd, contents + *length_out, BUF_SIZE);
        if (count == -1) goto error;
        *length_out += count;
        if (count < BUF_SIZE) {
            contents = realloc(contents, *length_out);
            break;
        }
        contents = realloc(contents, *length_out + BUF_SIZE);
        if (contents == NULL) goto error;
    }
    goto success;
error:
    if (contents != NULL) {
        free(contents);
        contents = NULL;
    }
success:
    return contents;
}

static int set_pipe_flags(int fd) {
  int flags = fcntl(fd, F_GETFL);
  if (flags == -1) return -1;
  flags |= O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flags) == -1) return -1;
  flags = fcntl(fd, F_GETFD);
  if (flags == -1) return -1;
  flags |= O_CLOEXEC;
  if (fcntl(fd, F_SETFD, flags) == -1) return -1;
  return 0;
}

static void paste(nk_handle usr, struct nk_text_edit *edit) {
    struct demo_state *state = usr.ptr;
    if (state->source) {
        nk_textedit_paste(edit, state->clipboard, state->clipboard_length);
        return;
    }
    if (!state->offer) return;
    const char *preferred = wl_data_offer_get_user_data(state->offer);
    if (!preferred) return;
    int pipefds[2];
    if (pipe(pipefds) == -1) return;
    if (set_pipe_flags(pipefds[0]) == -1) return;
    if (set_pipe_flags(pipefds[1]) == -1) return;
    wl_data_offer_receive(state->offer, preferred, pipefds[1]);
    wl_display_flush(state->wl_display);
    close(pipefds[1]);
    size_t len;
    char *text = read_all(pipefds[0], &len);
    close(pipefds[0]);
    if (!text) return;
    nk_textedit_paste(edit, text, len);
    free(text);
}

static void data_source_handle_send(void *data,
        struct wl_data_source *data_source,
        const char *mime_type,
        int32_t fd) {
    struct demo_state *state = data;
    int result = write(fd, state->clipboard, state->clipboard_length + 1);
    UNUSED(result);
    close(fd);
}

static void data_source_handle_cancelled(void *data,
        struct wl_data_source *data_source) {
    struct demo_state *state = data;
    wl_data_source_destroy(state->source);
    wl_data_device_set_selection(state->data_device, NULL, 0);
    state->source = NULL;
}

static struct wl_data_source_listener data_source_listener = {
    noop,
    data_source_handle_send,
    data_source_handle_cancelled,
    noop,
    noop,
    noop
};

static void copy(nk_handle usr, const char *text, int len)
{
    struct demo_state *state = usr.ptr;
    if (!state->data_device_manager) return;
    if (!state->source) {
        state->source = wl_data_device_manager_create_data_source(
                state->data_device_manager);
        wl_data_source_add_listener(state->source,
                &data_source_listener, state);
        for (int i = 0; i < LEN(valid_mimetypes); i++) {
            wl_data_source_offer(state->source, valid_mimetypes[i]);
        }
        wl_data_device_set_selection(state->data_device, state->source,
                state->input_serial);
    }

    if (state->clipboard) free(state->clipboard);
    state->clipboard = (char *) malloc((size_t) len + 1);
    memcpy(state->clipboard, text, (size_t) len);
    state->clipboard[len] = '\0';
    state->clipboard_length = len;
}


static void data_device_handle_data_offer(void *data,
    struct wl_data_device *data_device,
    struct wl_data_offer *id) {
    struct demo_state *state = data;
    wl_data_offer_add_listener(id, &data_offer_listener, NULL);
}

static void data_device_handle_selection(void *data,
    struct wl_data_device *data_device,
    struct wl_data_offer *id) {
    struct demo_state *state = data;
    if (state->offer) {
        wl_data_offer_destroy(state->offer);
    }
    state->offer = id;
}

static struct wl_data_device_listener data_device_listener = {
    data_device_handle_data_offer,
    noop,
    noop,
    noop,
    noop,
    data_device_handle_selection
};

static void registry_handle_global(void *data,
        struct wl_registry *registry,
        uint32_t name,
        const char *interface,
        uint32_t version) {
    struct demo_state *state = data;
    if (strcmp(interface, wl_compositor_interface.name) == 0)
        state->compositor =
            wl_registry_bind(registry, name, &wl_compositor_interface, version);
    else if (strcmp(interface, wl_seat_interface.name) == 0 && !state->seat) {
        state->seat =
            wl_registry_bind(registry, name, &wl_seat_interface, version);
        wl_seat_add_listener(state->seat, &seat_listener, state);
    } else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
        state->wm_base =
            wl_registry_bind(registry, name, &xdg_wm_base_interface, version);
        xdg_wm_base_add_listener(state->wm_base, &wm_base_listener, state);
    } else if (strcmp(interface, wl_data_device_manager_interface.name) == 0)
        state->data_device_manager =
            wl_registry_bind(registry, name, &wl_data_device_manager_interface, version);
    else if (strcmp(interface, zwp_relative_pointer_manager_v1_interface.name) == 0)
        state->relative_manager =
            wl_registry_bind(registry, name, &zwp_relative_pointer_manager_v1_interface, version);
    else if (strcmp(interface, wl_output_interface.name) == 0) {
        struct demo_output *output = calloc(1, sizeof(*output));
        if (!output) die("out of memory");
        output->state = state;
        output->name = name;
        output->scale = 1;
        output->output =
            wl_registry_bind(registry, name, &wl_output_interface, version);
        wl_output_add_listener(output->output, &output_listener, output);
    }
    if (state->data_device_manager && state->seat && !state->data_device) {
        state->data_device = wl_data_device_manager_get_data_device(
                state->data_device_manager, state->seat);
        wl_data_device_add_listener(state->data_device,
                &data_device_listener, state);
    }
}

static void registry_handle_global_remove(void *data,
        struct wl_registry *registry,
        uint32_t name) {
    struct demo_state *state = data;
    struct demo_output *output;
    wl_list_for_each(output, &state->outputs, link) {
        if (output->name == name) {
            wl_list_remove(&output->link);
            wl_output_destroy(output->output);
            free(output);
            break;
        }
    }
}

static const struct wl_registry_listener registry_listener = {
    registry_handle_global,
    registry_handle_global_remove
};

int main(int argc, char* argv[])
{
    /* GUI */
    struct demo_state *state = calloc(1, sizeof(*state));
    if (!state) die("Out of memory");
    state->running = nk_true;
    state->width = WINDOW_WIDTH;
    state->height = WINDOW_HEIGHT;
    state->new_width = state->width;
    state->new_height = state->height;
    state->scale = 1;
    state->last_click = -1;
    state->last_mouse_x = -1.;
    state->last_mouse_y = -1.;
    state->wheel_x = 0.;
    state->wheel_y = 0.;
    wl_list_init(&state->outputs);

    {
        struct xkb_context *context = xkb_context_new(0);
        if (!context) die("Failed to create xkb context");
        struct xkb_rule_names names;
        names.rules = "evdev";
        names.model = "pc105";
        names.layout = "us";
        names.variant = "";
        names.options = "";
        state->keymap = xkb_keymap_new_from_names(context, &names, 0);
        if (!state->keymap) die("Failed to create xkb keymap");
        state->xkb_state = xkb_state_new(state->keymap);
        if (!state->xkb_state) die("Failed to create xkb state");
        xkb_context_unref(context);
    }

    state->wl_display = wl_display_connect(NULL);
    if (!state->wl_display) die("Can't connect to wayland display");
    struct wl_registry *registry = wl_display_get_registry(state->wl_display);
    wl_registry_add_listener(registry, &registry_listener, state);
    wl_display_dispatch(state->wl_display);
    wl_display_roundtrip(state->wl_display);

    if (!state->compositor) die("wl_compositor not found");
    if (!state->seat) die("wl_seat not found");
    if (!state->wm_base) die("xdg_wm_base not found");

    wl_display_roundtrip(state->wl_display);

    state->scale = get_scale(state, nk_true);

    state->wl_surface = wl_compositor_create_surface(state->compositor);
    wl_surface_add_listener(state->wl_surface, &surface_listener, state);
    struct xdg_surface *xdg_surface =
        xdg_wm_base_get_xdg_surface(state->wm_base, state->wl_surface);
    xdg_surface_add_listener(xdg_surface, &xdg_surface_listener, state);
    struct xdg_toplevel *toplevel = xdg_surface_get_toplevel(xdg_surface);
    xdg_toplevel_add_listener(toplevel, &toplevel_listener, state);
    wl_surface_set_buffer_scale(state->wl_surface, state->scale);
    xdg_toplevel_set_app_id(toplevel, "vurtun.nuklear.demo");
    xdg_toplevel_set_title(toplevel, "Demo");

    wl_surface_commit(state->wl_surface);
    wl_display_roundtrip(state->wl_display);

    state->window = wl_egl_window_create(state->wl_surface,
            state->width * state->scale, state->height * state->scale);
    if (!state->window) die("Failed to create EGL window");

    state->display = eglGetDisplay(state->wl_display);
    if (state->display == EGL_NO_DISPLAY) die("Failed to create EGL display");
    EGLint major, minor;
    if (!eglInitialize(state->display, &major, &minor))
        die("Failed to initialize EGL");
    if(!((major == 1 && minor >= 4) || major >= 2))
        die("EGL version is %d.%d, 1.4 or later is required", major, minor);

    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 0,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NATIVE_RENDERABLE, EGL_TRUE,
        EGL_NONE
    };
    EGLConfig config;
    EGLint num_configs;
    if (!eglChooseConfig(
                state->display, config_attribs, &config, 1, &num_configs)
            || num_configs == 0)
        die("Failed to choose EGL config");

    EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };
    state->context = eglCreateContext(state->display, config, EGL_NO_CONTEXT,
            context_attribs);
    if (state->context == EGL_NO_CONTEXT) die("Failed to create EGL context");

    state->surface = eglCreateWindowSurface(state->display, config,
            state->window, NULL);
    if (state->surface == EGL_NO_SURFACE) die("Failed to create EGL surface");

    bind_gl_context(state);
    state->wayland = nk_wayland_init(copy, paste, nk_handle_ptr(state));
    struct nk_context *ctx = nk_wayland_get_context(state->wayland);

    /* Load Fonts: if none of these are loaded a default font will be used  */
    /* Load Cursor: if you uncomment cursor loading please hide the cursor */
    {struct nk_font_atlas *atlas;
    nk_wayland_font_stash_begin(state->wayland, &atlas);
    /*struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "../../extra_font/DroidSans.ttf", 14, 0);*/
    /*struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "../../extra_font/Roboto-Regular.ttf", 16, 0);*/
    /*struct nk_font *future = nk_font_atlas_add_from_file(atlas, "../../extra_font/kenvector_future_thin.ttf", 13, 0);*/
    /*struct nk_font *clean = nk_font_atlas_add_from_file(atlas, "../../extra_font/ProggyClean.ttf", 12, 0);*/
    /*struct nk_font *tiny = nk_font_atlas_add_from_file(atlas, "../../extra_font/ProggyTiny.ttf", 10, 0);*/
    /*struct nk_font *cousine = nk_font_atlas_add_from_file(atlas, "../../extra_font/Cousine-Regular.ttf", 13, 0);*/
    nk_wayland_font_stash_end(state->wayland);
    /*nk_style_load_all_cursors(ctx, atlas->cursors);*/
    /*nk_style_set_font(ctx, &roboto->handle);*/}

    /* style.c */
    /*set_style(ctx, THEME_WHITE);*/
    /*set_style(ctx, THEME_RED);*/
    /*set_style(ctx, THEME_BLUE);*/
    /*set_style(ctx, THEME_DARK);*/

    nk_input_begin(ctx);
    draw_frame(state, 0);

    while (state->running && wl_display_dispatch(state->wl_display) != -1) {
        // do nothing
    }

    nk_input_end(ctx);

    nk_wayland_shutdown(state->wayland);
    eglDestroyContext(state->display, state->context);
    eglDestroySurface(state->display, state->surface);
    eglTerminate(state->display);
    wl_egl_window_destroy(state->window);
    xdg_toplevel_destroy(toplevel);
    xdg_surface_destroy(xdg_surface);
    wl_surface_destroy(state->wl_surface);
    if (state->keyboard)
        wl_keyboard_destroy(state->keyboard);
    if (state->relative_pointer)
        zwp_relative_pointer_v1_destroy(state->relative_pointer);
    if (state->pointer)
        wl_pointer_destroy(state->pointer);
    struct demo_output *output, *tmp;
    wl_list_for_each_safe(output, tmp, &state->outputs, link) {
        wl_list_remove(&output->link);
        wl_output_destroy(output->output);
        free(output);
    }
    if (state->clipboard)
        free(state->clipboard);
    if (state->source)
        wl_data_source_destroy(state->source);
    if (state->offer)
        wl_data_offer_destroy(state->offer);
    if (state->data_device)
        wl_data_device_destroy(state->data_device);
    if (state->data_device_manager)
        wl_data_device_manager_destroy(state->data_device_manager);
    xdg_wm_base_destroy(state->wm_base);
    if (state->relative_manager)
        zwp_relative_pointer_manager_v1_destroy(state->relative_manager);
    wl_seat_destroy(state->seat);
    wl_compositor_destroy(state->compositor);
    wl_registry_destroy(registry);
    wl_display_disconnect(state->wl_display);
    xkb_state_unref(state->xkb_state);
    xkb_keymap_unref(state->keymap);
    return 0;
}

