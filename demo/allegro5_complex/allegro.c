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

#include <allegro5/allegro.h>
#include <allegro5/allegro_primitives.h>

/* macros */
#define MAX_VERTEX_MEMORY 512 * 1024
#define MAX_ELEMENT_MEMORY 128 * 1024

#include "../../zahnrad.h"
#include "../demo.c"

struct device {
    ALLEGRO_DISPLAY *display;
    ALLEGRO_EVENT_QUEUE *queue;
    ALLEGRO_BITMAP *texture;
    ALLEGRO_VERTEX_DECL *vertex_decl;
    struct zr_draw_null_texture null;
    struct zr_buffer cmds;
    struct zr_font font;
    void *vertex_buffer;
    void *element_buffer;
};

struct allegro_vertex {
    struct zr_vec2 pos;
    struct zr_vec2 uv;
    ALLEGRO_COLOR col;
};

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

static struct zr_user_font
device_init(struct device *dev,
    const char *path, unsigned int font_height, const zr_rune *range)
{
    int glyph_count;
    int img_width, img_height;
    struct zr_font_glyph *glyphes;
    struct zr_baked_font baked_font;
    struct zr_user_font user_font;
    struct zr_recti custom;

    ALLEGRO_VERTEX_ELEMENT elems[] = {
        {ALLEGRO_PRIM_POSITION, ALLEGRO_PRIM_FLOAT_2, offsetof(struct allegro_vertex, pos)},
        {ALLEGRO_PRIM_TEX_COORD, ALLEGRO_PRIM_FLOAT_2, offsetof(struct allegro_vertex, uv)},
        {ALLEGRO_PRIM_COLOR_ATTR, 0, offsetof(struct allegro_vertex, col)},
        {0,0,0}
    };
    dev->vertex_decl = al_create_vertex_decl(elems, sizeof(struct allegro_vertex));

    memset(&baked_font, 0, sizeof(baked_font));
    memset(&user_font, 0, sizeof(user_font));
    memset(&custom, 0, sizeof(custom));
    {
        /* bake and upload font texture */
        void *img, *tmp;
        size_t ttf_size;
        size_t tmp_size, img_size;
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
        config.size = (float)font_height;
        config.spacing = zr_vec2(0,0);
        config.oversample_h = 1;
        config.oversample_v = 1;

        /* query needed amount of memory for the font baking process */
        zr_font_bake_memory(&tmp_size, &glyph_count, &config, 1);
        glyphes = (struct zr_font_glyph*)calloc(sizeof(struct zr_font_glyph), (size_t)glyph_count);
        tmp = calloc(1, tmp_size);

        /* pack all glyphes and return needed image width, height and memory size*/
        custom.w = 2; custom.h = 2;
        if (!zr_font_bake_pack(&img_size, &img_width,&img_height,&custom,tmp,tmp_size,&config, 1))
            die("[Font]: failed to load font!\n");

        /* bake all glyphes and custom white pixel into image */
        img = calloc(1, img_size);
        zr_font_bake(img, img_width, img_height, tmp, tmp_size, glyphes, glyph_count, &config, 1);
        zr_font_bake_custom_data(img, img_width, img_height, custom, custom_data, 2, 2, '.', 'X');
        {
            /* convert alpha8 image into rgba8 image */
            void *img_rgba = calloc(4, (size_t)(img_height * img_width));
            zr_font_bake_convert(img_rgba, img_width, img_height, img);
            free(img);
            img = img_rgba;
        }

        {
            /* create allegro font bitmap */
            ALLEGRO_BITMAP *bitmap = 0;
            int flags = al_get_new_bitmap_flags();
            int fmt = al_get_new_bitmap_format();
            al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP|ALLEGRO_MIN_LINEAR|ALLEGRO_MAG_LINEAR);
            al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE);
            bitmap = al_create_bitmap(img_width, img_height);
            al_set_new_bitmap_flags(flags);
            al_set_new_bitmap_format(fmt);
            assert(bitmap);

            {
                /* copy font texture into bitmap */
                ALLEGRO_LOCKED_REGION * locked_img;
                locked_img = al_lock_bitmap(bitmap, al_get_bitmap_format(bitmap), ALLEGRO_LOCK_WRITEONLY);
                assert(locked_img);
                memcpy(locked_img->data, img, sizeof(uint32_t)*(size_t)(img_width*img_height));
                al_unlock_bitmap(bitmap);
            }

            /* convert software texture into hardware texture */
            dev->texture = al_clone_bitmap(bitmap);
            al_destroy_bitmap(bitmap);
            assert(dev->texture);
        }
        free(ttf_blob);
        free(tmp);
        free(img);
    }

    /* default white pixel in a texture which is needed to draw primitives */
    dev->null.texture.ptr = dev->texture;
    dev->null.uv = zr_vec2((custom.x + 0.5f)/(float)img_width,
                            (custom.y + 0.5f)/(float)img_height);

    /* setup font with glyphes. IMPORTANT: the font only references the glyphes
      this was done to have the possibility to have multible fonts with one
      total glyph array. Not quite sure if it is a good thing since the
      glyphes have to be freed as well. */
    zr_font_init(&dev->font, (float)font_height, '?', glyphes, &baked_font, dev->null.texture);
    user_font = zr_font_ref(&dev->font);

    /* allocate memory for drawing process */
    dev->vertex_buffer = calloc(MAX_VERTEX_MEMORY, 1);
    dev->element_buffer = calloc(MAX_ELEMENT_MEMORY, 1);
    return user_font;
}

static void
device_shutdown(struct device *dev)
{
    free(dev->font.glyphs);
    free(dev->vertex_buffer);
    free(dev->element_buffer);
    zr_buffer_free(&dev->cmds);
}

static void
device_draw(struct device *dev, struct zr_context *ctx, enum zr_anti_aliasing AA)
{
    int op, src, dst;
    al_get_blender(&op, &src, &dst);
    al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);

    {
        const struct zr_draw_command *cmd;
        struct zr_buffer vbuf, ebuf;
        int offset = 0;
        struct allegro_vertex *vertices = 0;
        int *indicies = 0;

        /* fill converting configuration */
        struct zr_convert_config config;
        memset(&config, 0, sizeof(config));
        config.global_alpha = 1.0f;
        config.shape_AA = AA;
        config.line_AA = AA;
        config.circle_segment_count = 22;
        config.line_thickness = 1.0f;
        config.null = dev->null;

        /* convert from command into hardware format */
        zr_buffer_init_fixed(&vbuf, dev->vertex_buffer, MAX_VERTEX_MEMORY);
        zr_buffer_init_fixed(&ebuf, dev->element_buffer, MAX_ELEMENT_MEMORY);
            zr_convert(ctx, &dev->cmds, &vbuf, &ebuf, &config);

        {
            /* <sign> allegro does not support 32-bit packed color */
            unsigned int i = 0;
            struct zr_draw_vertex *verts = (struct zr_draw_vertex*)dev->vertex_buffer;
            vertices = calloc(sizeof(struct allegro_vertex), ctx->canvas.vertex_count);
            for (i = 0; i < ctx->canvas.vertex_count; ++i) {
                zr_byte *c;
                vertices[i].pos = verts[i].position;
                vertices[i].uv = verts[i].uv;
                c = (zr_byte*)&verts[i].col;
                vertices[i].col = al_map_rgba(c[0], c[1], c[2], c[3]);
            }
        }
        {
            /* <massive sign> allegro does not support 16-bit indicies:
             * @OPT: define zr_draw_index as int to fix this issue. */
            unsigned int i = 0;
            zr_draw_index *elements = (zr_draw_index*)dev->element_buffer;
            indicies = calloc(sizeof(int), ctx->canvas.element_count);
            for (i = 0; i < ctx->canvas.element_count; ++i)
                indicies[i] = elements[i];
        }

        /* iterate over and execute each draw command */
        zr_draw_foreach(cmd, ctx, &dev->cmds)
        {
            ALLEGRO_BITMAP *texture = cmd->texture.ptr;
            if (!cmd->elem_count) continue;
            al_set_clipping_rectangle((int)cmd->clip_rect.x, (int)cmd->clip_rect.y,
                (int)cmd->clip_rect.w, (int)cmd->clip_rect.h);
            al_draw_indexed_prim(vertices, dev->vertex_decl, texture, &indicies[offset],
                (int)cmd->elem_count, ALLEGRO_PRIM_TRIANGLE_LIST);
            offset += cmd->elem_count;
        }

        free(vertices);
        free(indicies);
        zr_clear(ctx);
    }
    al_set_blender(op, src, dst);
    al_set_clipping_rectangle(0,0,al_get_display_width(dev->display),
        al_get_display_height(dev->display));
}

static void
input_key(struct zr_context *ctx, ALLEGRO_EVENT *evt, int down)
{
    int sym = evt->keyboard.keycode;
    if (sym == ALLEGRO_KEY_RSHIFT || sym == ALLEGRO_KEY_LSHIFT)
        zr_input_key(ctx, ZR_KEY_SHIFT, down);
    else if (sym == ALLEGRO_KEY_DELETE)
        zr_input_key(ctx, ZR_KEY_DEL, down);
    else if (sym == ALLEGRO_KEY_ENTER)
        zr_input_key(ctx, ZR_KEY_ENTER, down);
    else if (sym == ALLEGRO_KEY_TAB)
        zr_input_key(ctx, ZR_KEY_TAB, down);
    else if (sym == ALLEGRO_KEY_BACKSPACE)
        zr_input_key(ctx, ZR_KEY_BACKSPACE, down);
    else if (sym == ALLEGRO_KEY_LEFT)
        zr_input_key(ctx, ZR_KEY_LEFT, down);
    else if (sym == ALLEGRO_KEY_RIGHT)
        zr_input_key(ctx, ZR_KEY_RIGHT, down);
    else if (sym == ALLEGRO_KEY_C)
        zr_input_key(ctx, ZR_KEY_COPY, down && evt->keyboard.modifiers & ALLEGRO_KEYMOD_CTRL);
    else if (sym == ALLEGRO_KEY_V)
        zr_input_key(ctx, ZR_KEY_PASTE, down && evt->keyboard.modifiers & ALLEGRO_KEYMOD_CTRL);
    else if (sym == ALLEGRO_KEY_X)
        zr_input_key(ctx, ZR_KEY_CUT, down && evt->keyboard.modifiers & ALLEGRO_KEYMOD_CTRL);
}

static void
input_button(struct zr_context *ctx, ALLEGRO_EVENT *evt, int down)
{
    const int x = evt->mouse.x;
    const int y = evt->mouse.y;
    if (evt->mouse.button == 1)
        zr_input_button(ctx, ZR_BUTTON_LEFT, x, y, down);
    if (evt->mouse.button == 2)
        zr_input_button(ctx, ZR_BUTTON_RIGHT, x, y, down);
}

static void* mem_alloc(zr_handle unused, size_t size)
{UNUSED(unused); return calloc(1, size);}
static void mem_free(zr_handle unused, void *ptr)
{UNUSED(unused); free(ptr);}

int
main(int argc, char *argv[])
{
    struct device dev;
    struct demo gui;
    int running = 1;
    const char *font_path = argv[1];
    if (argc < 2)
        die("Missing TTF Font file argument!");

    /* Allegro */
    al_init();
    al_install_keyboard();
    al_install_mouse();
    al_init_primitives_addon();
    dev.display = al_create_display(WINDOW_WIDTH, WINDOW_HEIGHT);
    al_set_window_title(dev.display, "Zahnrad");
    dev.queue = al_create_event_queue();
    al_register_event_source(dev.queue, al_get_display_event_source(dev.display));
    al_register_event_source(dev.queue, al_get_keyboard_event_source());
    al_register_event_source(dev.queue, al_get_mouse_event_source());

    {
        /* GUI */
        struct zr_user_font usrfnt;
        struct zr_allocator alloc;
        alloc.userdata.ptr = NULL;
        alloc.alloc = mem_alloc;
        alloc.free = mem_free;
        zr_buffer_init(&dev.cmds, &alloc, 1024);

        usrfnt = device_init(&dev, font_path, 14,
            zr_font_default_glyph_ranges());
        zr_init(&gui.ctx, &alloc, &usrfnt);
    }

    while (running) {
        /* Input */
        ALLEGRO_EVENT evt;
        zr_input_begin(&gui.ctx);
        while (al_get_next_event(dev.queue, &evt)) {
            if (evt.type == ALLEGRO_EVENT_DISPLAY_CLOSE) goto cleanup;
            else if (evt.type == ALLEGRO_EVENT_KEY_UP && evt.keyboard.display == dev.display)
                input_key(&gui.ctx, &evt, zr_false);
            else if (evt.type == ALLEGRO_EVENT_KEY_DOWN && evt.keyboard.display == dev.display)
                input_key(&gui.ctx, &evt, zr_true);
            else if (evt.type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN)
                input_button(&gui.ctx, &evt, zr_true);
            else if (evt.type == ALLEGRO_EVENT_MOUSE_BUTTON_UP)
                input_button(&gui.ctx, &evt, zr_false);
            else if (evt.type == ALLEGRO_EVENT_MOUSE_AXES) {
                zr_input_motion(&gui.ctx, evt.mouse.x, evt.mouse.y);
            } else if (evt.type == ALLEGRO_EVENT_KEY_CHAR) {
                if (evt.keyboard.display == dev.display)
                    if (evt.keyboard.unichar > 0 && evt.keyboard.unichar < 0x10000)
                        zr_input_unicode(&gui.ctx, (zr_rune)evt.keyboard.unichar);
            }
        }
        zr_input_end(&gui.ctx);

        /* GUI */
        running = run_demo(&gui);

        /* Draw */
        al_clear_to_color(al_map_rgba_f(0.2f, 0.2f, 0.2f, 1.0f));
        device_draw(&dev, &gui.ctx, ZR_ANTI_ALIASING_ON);
        al_flip_display();
    }

cleanup:
    /* Cleanup */
    if (dev.texture)
        al_destroy_bitmap(dev.texture);
    if (dev.queue)
        al_destroy_event_queue(dev.queue);
    if (dev.display)
        al_destroy_display(dev.display);
    device_shutdown(&dev);
    zr_free(&gui.ctx);
    return 0;
}

