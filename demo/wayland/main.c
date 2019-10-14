#define _GNU_SOURCE // for O_TMPFILE
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_INCLUDE_SOFTWARE_FONT


#include <wayland-client.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>
#include "../../nuklear.h"
#include "nuklear_raw_wayland.h"


#define DTIME           20


//WAYLAND OUTPUT INTERFACE
static void nk_wayland_output_cb_geometry(void *data, struct wl_output *wl_output, int x, int y, int w, int h, int subpixel, const char *make, const char *model, int transform)
{
    printf("wl_output geometry x=%d, y=%d, w=%d, h=%d make=%s, model=%s \n", x,y,w,h, make, model);
}

static void nk_wayland_output_cb_mode(void *data, struct wl_output *wl_output, unsigned int flags, int w, int h, int refresh)
{
}

static void nk_wayland_output_cb_done(void *data, struct wl_output *output)
{

}

static void nk_wayland_output_cb_scale(void *data, struct wl_output *output, int scale)
{

}

static const struct wl_output_listener nk_wayland_output_listener =
{
   &nk_wayland_output_cb_geometry,
   &nk_wayland_output_cb_mode,
   &nk_wayland_output_cb_done,
   &nk_wayland_output_cb_scale
};
//-------------------------------------------------------------------- endof WAYLAND OUTPUT INTERFACE

//WAYLAND POINTER INTERFACE (mouse/touchpad)
static void nk_wayland_pointer_enter (void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface, wl_fixed_t surface_x, wl_fixed_t surface_y) 
{
}

static void nk_wayland_pointer_leave (void *data, struct wl_pointer *pointer, uint32_t serial, struct wl_surface *surface) 
{
}

static void nk_wayland_pointer_motion (void *data, struct wl_pointer *pointer, uint32_t time, wl_fixed_t x, wl_fixed_t y) 
{
    struct nk_wayland* win = (struct nk_wayland*)data;
    win->mouse_pointer_x = wl_fixed_to_int(x);
    win->mouse_pointer_y = wl_fixed_to_int(y);
    
    nk_input_motion(&(win->ctx), win->mouse_pointer_x, win->mouse_pointer_y);
}

static void nk_wayland_pointer_button (void *data, struct wl_pointer *pointer, uint32_t serial, uint32_t time, uint32_t button, uint32_t state) 
{
    struct nk_wayland* win = (struct nk_wayland*)data;
    
    if (button == 272){ //left mouse button
        if (state == WL_POINTER_BUTTON_STATE_PRESSED) {
           // printf("nk_input_button x=%d, y=%d press: 1 \n", win->mouse_pointer_x, win->mouse_pointer_y);
            nk_input_button(&(win->ctx), NK_BUTTON_LEFT, win->mouse_pointer_x, win->mouse_pointer_y, 1);
            
        } else if (state == WL_POINTER_BUTTON_STATE_RELEASED) {
            nk_input_button(&(win->ctx), NK_BUTTON_LEFT, win->mouse_pointer_x, win->mouse_pointer_y, 0);
        }
    }
}

static void nk_wayland_pointer_axis (void *data, struct wl_pointer *pointer, uint32_t time, uint32_t axis, wl_fixed_t value) 
{
}

static struct wl_pointer_listener nk_wayland_pointer_listener = 
{
    &nk_wayland_pointer_enter, 
    &nk_wayland_pointer_leave, 
    &nk_wayland_pointer_motion, 
    &nk_wayland_pointer_button, 
    &nk_wayland_pointer_axis
};
//-------------------------------------------------------------------- endof WAYLAND POINTER INTERFACE

//WAYLAND KEYBOARD INTERFACE
static void nk_wayland_keyboard_keymap (void *data, struct wl_keyboard *keyboard, uint32_t format, int32_t fd, uint32_t size) 
{
}

static void nk_wayland_keyboard_enter (void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface, struct wl_array *keys) 
{	
}

static void nk_wayland_keyboard_leave (void *data, struct wl_keyboard *keyboard, uint32_t serial, struct wl_surface *surface) 
{
}

static void nk_wayland_keyboard_key (void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t time, uint32_t key, uint32_t state) 
{
    printf("key: %d \n", key);
}

static void nk_wayland_keyboard_modifiers (void *data, struct wl_keyboard *keyboard, uint32_t serial, uint32_t mods_depressed, uint32_t mods_latched, uint32_t mods_locked, uint32_t group) 
{
}

static struct wl_keyboard_listener nk_wayland_keyboard_listener = 
{
    &nk_wayland_keyboard_keymap, 
    &nk_wayland_keyboard_enter, 
    &nk_wayland_keyboard_leave, 
    &nk_wayland_keyboard_key, 
    &nk_wayland_keyboard_modifiers
};
//-------------------------------------------------------------------- endof WAYLAND KEYBOARD INTERFACE

//WAYLAND SEAT INTERFACE
static void seat_capabilities (void *data, struct wl_seat *seat, uint32_t capabilities) 
{
     struct nk_wayland* win = (struct nk_wayland*)data;
     
	if (capabilities & WL_SEAT_CAPABILITY_POINTER) {
		struct wl_pointer *pointer = wl_seat_get_pointer (seat);
		wl_pointer_add_listener (pointer, &nk_wayland_pointer_listener, win);
	}
	if (capabilities & WL_SEAT_CAPABILITY_KEYBOARD) {
		struct wl_keyboard *keyboard = wl_seat_get_keyboard (seat);
		wl_keyboard_add_listener (keyboard, &nk_wayland_keyboard_listener, win);
	}
}

static struct wl_seat_listener seat_listener =
{
    &seat_capabilities
};
//-------------------------------------------------------------------- endof WAYLAND SEAT INTERFACE

// WAYLAND SHELL INTERFACE
static void nk_wayland_shell_surface_ping (void *data, struct wl_shell_surface *shell_surface, uint32_t serial) 
{
	wl_shell_surface_pong (shell_surface, serial);
}

static void nk_wayland_shell_surface_configure (void *data, struct wl_shell_surface *shell_surface, uint32_t edges, int32_t width, int32_t height) 
{
}

static void nk_wayland_shell_surface_popup_done (void *data, struct wl_shell_surface *shell_surface) 
{	
}

static struct wl_shell_surface_listener nk_wayland_shell_surface_listener = 
{
    &nk_wayland_shell_surface_ping, 
    &nk_wayland_shell_surface_configure, 
    &nk_wayland_shell_surface_popup_done
};
//--------------------------------------------------------------------- endof WAYLAND SHELL INTERFACE


// WAYLAND REGISTRY INTERFACE
static void nk_wayland_registry_add_object (void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) 
{
    struct nk_wayland* win = (struct nk_wayland*)data;
    
    //printf("looking for %s interface \n", interface);
	if (!strcmp(interface,"wl_compositor")) {
		win->compositor = wl_registry_bind (registry, name, &wl_compositor_interface, 1);
        
	} else if (!strcmp(interface,"wl_shell")) {
		win->shell = wl_registry_bind (registry, name, &wl_shell_interface, 1);
        
	} else if (!strcmp(interface,"wl_shm")) {
		win->wl_shm = wl_registry_bind (registry, name, &wl_shm_interface, 1);
        
	} else if (!strcmp(interface,"wl_seat")) {
		win->seat = wl_registry_bind (registry, name, &wl_seat_interface, 1);
		wl_seat_add_listener (win->seat, &seat_listener, win);
        
	} else if (!strcmp(interface, "wl_output")) {
        struct wl_output *wl_output = wl_registry_bind(registry, name, &wl_output_interface, 1);
        wl_output_add_listener(wl_output, &nk_wayland_output_listener, NULL);
    }
}

static void nk_wayland_registry_remove_object (void *data, struct wl_registry *registry, uint32_t name) 
{
}

static struct wl_registry_listener nk_wayland_registry_listener = 
{
    &nk_wayland_registry_add_object, 
    &nk_wayland_registry_remove_object
};
//------------------------------------------------------------------------------------------------ endof WAYLAND REGISTRY INTERFACE


static void nk_wayland_init(struct nk_wayland* win)
{
    const void *tex;
    
    win->font_tex.pixels = win->tex_scratch;
    win->font_tex.format = NK_FONT_ATLAS_ALPHA8;
    win->font_tex.w = win->font_tex.h = 0;

    if (0 == nk_init_default(&(win->ctx), 0)) {
        return;
    }

    nk_font_atlas_init_default(&(win->atlas));
    nk_font_atlas_begin(&(win->atlas));
    tex = nk_font_atlas_bake(&(win->atlas), &(win->font_tex.w), &(win->font_tex.h), win->font_tex.format);
    if (!tex) {
        return;
    }

    switch(win->font_tex.format) {
    case NK_FONT_ATLAS_ALPHA8:
        win->font_tex.pitch = win->font_tex.w * 1;
        break;
    case NK_FONT_ATLAS_RGBA32:
        win->font_tex.pitch = win->font_tex.w * 4;
        break;
    };
    /* Store the font texture in tex scratch memory */
    memcpy(win->font_tex.pixels, tex, win->font_tex.pitch * win->font_tex.h);
    nk_font_atlas_end(&(win->atlas), nk_handle_ptr(NULL), NULL);
    if (win->atlas.default_font)
        nk_style_set_font(&(win->ctx), &(win->atlas.default_font->handle));
    nk_style_load_all_cursors(&(win->ctx), win->atlas.cursors);
    nk_wayland_scissor(win, 0, 0, win->width, win->height);
    
}

static void nk_wayland_deinit(struct nk_wayland *win) 
{
	wl_shell_surface_destroy (win->shell_surface);
	wl_surface_destroy (win->surface);
}

static long timestamp(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) < 0) return 0;
    return (long)((long)tv.tv_sec * 1000 + (long)tv.tv_usec/1000);
}

static void sleep_for(long t)
{
    struct timespec req;
    const time_t sec = (int)(t/1000);
    const long ms = t - (sec * 1000);
    req.tv_sec = sec;
    req.tv_nsec = ms * 1000000L;
    while(-1 == nanosleep(&req, &req));
}

static void nk_wayland_surf_clear(struct nk_wayland* win)
{
    int x, y;
    int pix_idx;
    
    for (y = 0; y < win->height; y++){
        for (x = 0; x < win->width; x++){ 
            pix_idx = y * win->width + x;
            win->data[pix_idx] = 0xFF000000;
        }
    }
}

//This causes the screen to refresh 
static const struct wl_callback_listener frame_listener;

static void redraw(void *data, struct wl_callback *callback, uint32_t time)
{
  //  printf("redrawing.. 1\n");
    struct nk_wayland* win = (struct nk_wayland*)data;
    struct nk_color col_red = {0xFF,0x00,0x00,0xA0}; //r,g,b,a
    struct nk_color col_green = {0x00,0xFF,0x00,0xA0}; //r,g,b,a
    wl_callback_destroy(win->frame_callback);
    wl_surface_damage(win->surface, 0, 0, WIDTH, HEIGHT); 
    


    win->frame_callback = wl_surface_frame(win->surface);
    wl_surface_attach(win->surface, win->front_buffer, 0, 0);
    wl_callback_add_listener(win->frame_callback, &frame_listener, win);
    wl_surface_commit(win->surface);
    
}


static const struct wl_callback_listener frame_listener = {
    redraw
};

int main () 
{
    long dt;
    long started;
    struct nk_wayland nk_wayland_ctx;
    struct wl_backend *backend;
    struct wl_registry *registry;
    int running = 1;
    
    //1. Initialize display
	nk_wayland_ctx.display = wl_display_connect (NULL);
    if (nk_wayland_ctx.display == NULL) {
        printf("no wayland display found. do you have wayland composer running? \n");
        return -1;
    }
    
	registry = wl_display_get_registry (nk_wayland_ctx.display);
	wl_registry_add_listener (registry, &nk_wayland_registry_listener, &nk_wayland_ctx);
	wl_display_roundtrip (nk_wayland_ctx.display);
	
    
    //2. Create Window
    nk_wayland_ctx.width = WIDTH;
	nk_wayland_ctx.height = HEIGHT;
	nk_wayland_ctx.surface = wl_compositor_create_surface (nk_wayland_ctx.compositor);
	nk_wayland_ctx.shell_surface = wl_shell_get_shell_surface (nk_wayland_ctx.shell, nk_wayland_ctx.surface);
	wl_shell_surface_add_listener (nk_wayland_ctx.shell_surface, &nk_wayland_shell_surface_listener, &nk_wayland_ctx);
	wl_shell_surface_set_toplevel (nk_wayland_ctx.shell_surface);
	
    
    nk_wayland_ctx.frame_callback = wl_surface_frame(nk_wayland_ctx.surface);
    wl_callback_add_listener(nk_wayland_ctx.frame_callback, &frame_listener, &nk_wayland_ctx);
    
	size_t size = WIDTH * HEIGHT * 4;
	char *xdg_runtime_dir = getenv ("XDG_RUNTIME_DIR");
	int fd = open (xdg_runtime_dir, O_TMPFILE|O_RDWR|O_EXCL, 0600);
	ftruncate (fd, size);
	nk_wayland_ctx.data = mmap (NULL, size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	struct wl_shm_pool *pool = wl_shm_create_pool (nk_wayland_ctx.wl_shm, fd, size);
	nk_wayland_ctx.front_buffer = wl_shm_pool_create_buffer (pool, 0, WIDTH, HEIGHT, WIDTH*4, WL_SHM_FORMAT_XRGB8888);
	wl_shm_pool_destroy (pool);
	close (fd);
    
    
    
    
    //3. Clear window and start rendering loop    
	nk_wayland_surf_clear(&nk_wayland_ctx);
    wl_surface_attach (nk_wayland_ctx.surface, nk_wayland_ctx.front_buffer, 0, 0);
    wl_surface_commit (nk_wayland_ctx.surface);
    
    nk_wayland_init(&nk_wayland_ctx);
    
    
    //4. rendering UI
    while (running) {
        started = timestamp(); 
      
        // GUI 
        if (nk_begin(&(nk_wayland_ctx.ctx), "Demo", nk_rect(50, 50, 200, 200),
            NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|
            NK_WINDOW_CLOSABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE)) {
            enum {EASY, HARD};
            static int op = EASY;
            static int property = 20;

            nk_layout_row_static(&(nk_wayland_ctx.ctx), 30, 80, 1);
            if (nk_button_label(&(nk_wayland_ctx.ctx), "button")){
                printf("button pressed\n");
            }
            nk_layout_row_dynamic(&(nk_wayland_ctx.ctx), 30, 2);
            if (nk_option_label(&(nk_wayland_ctx.ctx), "easy", op == EASY)) op = EASY;
            if (nk_option_label(&(nk_wayland_ctx.ctx), "hard", op == HARD)) op = HARD;
            nk_layout_row_dynamic(&(nk_wayland_ctx.ctx), 25, 1);
            nk_property_int(&(nk_wayland_ctx.ctx), "Compression:", 0, &property, 100, 10, 1);
        }
        nk_end(&(nk_wayland_ctx.ctx));
        
        if (nk_window_is_closed(&(nk_wayland_ctx.ctx), "Demo")) break;

        // -------------- EXAMPLES ---------------- 
        //#ifdef INCLUDE_CALCULATOR
        //  calculator(&rawfb->ctx);
        //#endif
       // #ifdef INCLUDE_OVERVIEW
       //   overview(&rawfb->ctx);
       // #endif
       // #ifdef INCLUDE_NODE_EDITOR
       //   node_editor(&rawfb->ctx);
        //#endif
        // ----------------------------------------- 

        // Draw framebuffer 
        nk_wayland_render(&nk_wayland_ctx, nk_rgb(30,30,30), 1);
        
        
        //handle wayland stuff (send display to FB & get inputs)
        nk_input_begin(&(nk_wayland_ctx.ctx));
        wl_display_dispatch(nk_wayland_ctx.display);
        nk_input_end(&(nk_wayland_ctx.ctx));
        
        // Timing 
        dt = timestamp() - started;
        if (dt < DTIME)
            sleep_for(DTIME - dt);
    }
	
	nk_wayland_deinit (&nk_wayland_ctx);
	wl_display_disconnect (nk_wayland_ctx.display);
	return 0;
}
