/*
 * Nuklear - v1.00 - public domain
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
#ifndef NK_ALLEGRO_H_
#define NK_ALLEGRO_H_

#include <allegro5/allegro.h>
NK_API struct nk_context* nk_allegro_init(ALLEGRO_DISPLAY *win, int max_vertex_memory, int max_element_memory);
NK_API void nk_allegro_font_stash_begin(struct nk_font_atlas **atlas);
NK_API void nk_allegro_font_stash_end(void);

NK_API void nk_allegro_handle_event(ALLEGRO_EVENT *evt);
NK_API void nk_allegro_render(enum nk_anti_aliasing);
NK_API void nk_allegro_shutdown(void);

NK_API void nk_allegro_device_destroy(void);
NK_API void nk_allegro_device_create(int max_vertex_memory, int max_elemnt_memory);

#endif

/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#ifdef NK_ALLEGRO_IMPLEMENTATION

#include <allegro5/allegro_primitives.h>

struct nk_allegro_device {
    ALLEGRO_BITMAP *texture;
    ALLEGRO_VERTEX_DECL *vertex_decl;
    struct nk_draw_null_texture null;
    struct nk_buffer cmds;
    void *vertex_buffer;
    void *element_buffer;
    int max_vertex_memory;
    int max_element_memory;
};

struct nk_allegro_vertex {
    struct nk_vec2 pos;
    struct nk_vec2 uv;
    ALLEGRO_COLOR col;
};

static struct {
    ALLEGRO_DISPLAY *win;
    struct nk_allegro_device dev;
    struct nk_context ctx;
    struct nk_font_atlas atlas;
} allegro;


NK_API void
nk_allegro_device_create(int max_vertex_memory, int max_element_memory)
{
    struct nk_allegro_device *dev = &allegro.dev;
    ALLEGRO_VERTEX_ELEMENT elems[] = {
        {ALLEGRO_PRIM_POSITION, ALLEGRO_PRIM_FLOAT_2, offsetof(struct nk_allegro_vertex, pos)},
        {ALLEGRO_PRIM_TEX_COORD, ALLEGRO_PRIM_FLOAT_2, offsetof(struct nk_allegro_vertex, uv)},
        {ALLEGRO_PRIM_COLOR_ATTR, 0, offsetof(struct nk_allegro_vertex, col)},
        {0,0,0}
    };
    dev->vertex_decl = al_create_vertex_decl(elems, sizeof(struct nk_allegro_vertex));
    dev->vertex_buffer = calloc((size_t)max_vertex_memory, 1);
    dev->element_buffer = calloc((size_t)max_element_memory, 1);
    dev->max_vertex_memory = max_vertex_memory;
    dev->max_element_memory = max_element_memory;
    nk_buffer_init_default(&dev->cmds);
}

static void
nk_allegro_device_upload_atlas(const void *image, int width, int height)
{
    /* create allegro font bitmap */
    ALLEGRO_BITMAP *bitmap = 0;
    struct nk_allegro_device *dev = &allegro.dev;
    int flags = al_get_new_bitmap_flags();
    int fmt = al_get_new_bitmap_format();
    al_set_new_bitmap_flags(ALLEGRO_MEMORY_BITMAP|ALLEGRO_MIN_LINEAR|ALLEGRO_MAG_LINEAR);
    al_set_new_bitmap_format(ALLEGRO_PIXEL_FORMAT_ABGR_8888_LE);
    bitmap = al_create_bitmap(width, height);
    al_set_new_bitmap_flags(flags);
    al_set_new_bitmap_format(fmt);
    assert(bitmap);

    {/* copy font texture into bitmap */
    ALLEGRO_LOCKED_REGION * locked_img;
    locked_img = al_lock_bitmap(bitmap, al_get_bitmap_format(bitmap), ALLEGRO_LOCK_WRITEONLY);
    assert(locked_img);
    memcpy(locked_img->data, image, sizeof(uint32_t)*(size_t)(width*height));
    al_unlock_bitmap(bitmap);}

    /* convert software texture into hardware texture */
    dev->texture = al_clone_bitmap(bitmap);
    al_destroy_bitmap(bitmap);
    assert(dev->texture);
}

NK_API void
nk_allegro_render(enum nk_anti_aliasing AA)
{
    int op, src, dst;
    struct nk_allegro_device *dev = &allegro.dev;
    struct nk_context *ctx = &allegro.ctx;
    al_get_blender(&op, &src, &dst);
    al_set_blender(ALLEGRO_ADD, ALLEGRO_ALPHA, ALLEGRO_INVERSE_ALPHA);

    {
        const struct nk_draw_command *cmd;
        struct nk_buffer vbuf, ebuf;
        int offset = 0;
        struct nk_allegro_vertex *vertices = 0;
        int *indices = 0;

        /* fill converting configuration */
        struct nk_convert_config config;
        memset(&config, 0, sizeof(config));
        config.global_alpha = 1.0f;
        config.shape_AA = AA;
        config.line_AA = AA;
        config.circle_segment_count = 22;
        config.arc_segment_count = 22;
        config.curve_segment_count = 22;
        config.null = dev->null;

        /* convert from command into hardware format */
        nk_buffer_init_fixed(&vbuf, dev->vertex_buffer, (nk_size)dev->max_vertex_memory);
        nk_buffer_init_fixed(&ebuf, dev->element_buffer, (nk_size)dev->max_element_memory);
            nk_convert(ctx, &dev->cmds, &vbuf, &ebuf, &config);

        {
            /* <sign> allegro does not support 32-bit packed color */
            unsigned int i = 0;
            struct nk_draw_vertex *verts = (struct nk_draw_vertex*)dev->vertex_buffer;
            vertices = (struct nk_allegro_vertex*)calloc(sizeof(struct nk_allegro_vertex), ctx->draw_list.vertex_count);
            for (i = 0; i < ctx->draw_list.vertex_count; ++i) {
                nk_byte *c;
                vertices[i].pos = verts[i].position;
                vertices[i].uv = verts[i].uv;
                c = (nk_byte*)&verts[i].col;
                vertices[i].col = al_map_rgba(c[0], c[1], c[2], c[3]);
            }
        }
        {
            /* <massive sign> allegro does not support 16-bit indices:
             * @OPT: define nk_draw_index as int to fix this issue. */
            unsigned int i = 0;
            nk_draw_index *elements = (nk_draw_index*)dev->element_buffer;
            indices = (int*)calloc(sizeof(int), ctx->draw_list.element_count);
            for (i = 0; i < ctx->draw_list.element_count; ++i)
                indices[i] = elements[i];
        }

        /* iterate over and execute each draw command */
        nk_draw_foreach(cmd, ctx, &dev->cmds)
        {
            ALLEGRO_BITMAP *texture = (ALLEGRO_BITMAP*)cmd->texture.ptr;
            if (!cmd->elem_count) continue;
            al_set_clipping_rectangle((int)cmd->clip_rect.x, (int)cmd->clip_rect.y,
                (int)cmd->clip_rect.w, (int)cmd->clip_rect.h);
            al_draw_indexed_prim(vertices, dev->vertex_decl, texture, &indices[offset],
                (int)cmd->elem_count, ALLEGRO_PRIM_TRIANGLE_LIST);
            offset += cmd->elem_count;
        }

        free(vertices);
        free(indices);
        nk_clear(ctx);
    }
    al_set_blender(op, src, dst);
    al_set_clipping_rectangle(0,0, al_get_display_width(allegro.win),
        al_get_display_height(allegro.win));
}

NK_API void
nk_allegro_device_destroy(void)
{
    struct nk_allegro_device *dev = &allegro.dev;
    free(dev->vertex_buffer);
    free(dev->element_buffer);
    nk_buffer_free(&dev->cmds);
}


NK_API struct nk_context*
nk_allegro_init(ALLEGRO_DISPLAY *win, int max_vertex_memory, int max_element_memory)
{
    allegro.win = win;
    nk_init_default(&allegro.ctx, 0);
    nk_allegro_device_create(max_vertex_memory, max_element_memory);
    return &allegro.ctx;
}

NK_API void
nk_allegro_font_stash_begin(struct nk_font_atlas **atlas)
{
    nk_font_atlas_init_default(&allegro.atlas);
    nk_font_atlas_begin(&allegro.atlas);
    *atlas = &allegro.atlas;
}

NK_API void
nk_allegro_font_stash_end(void)
{
    const void *image; int w, h;
    image = nk_font_atlas_bake(&allegro.atlas, &w, &h, NK_FONT_ATLAS_RGBA32);
    nk_allegro_device_upload_atlas(image, w, h);
    nk_font_atlas_end(&allegro.atlas, nk_handle_ptr(allegro.dev.texture), &allegro.dev.null);
    if (allegro.atlas.default_font)
        nk_style_set_font(&allegro.ctx, &allegro.atlas.default_font->handle);
}


NK_API void
nk_allegro_handle_event(ALLEGRO_EVENT *evt)
{
    struct nk_context *ctx = &allegro.ctx;
    if ((evt->type == ALLEGRO_EVENT_KEY_UP ||
            evt->type == ALLEGRO_EVENT_KEY_DOWN) &&
            evt->keyboard.display == allegro.win)
    {
        /* key handler */
        int down = (evt->type == ALLEGRO_EVENT_KEY_UP);
        int sym = evt->keyboard.keycode;
        if (sym == ALLEGRO_KEY_RSHIFT || sym == ALLEGRO_KEY_LSHIFT)
            nk_input_key(ctx, NK_KEY_SHIFT, down);
        else if (sym == ALLEGRO_KEY_DELETE)
            nk_input_key(ctx, NK_KEY_DEL, down);
        else if (sym == ALLEGRO_KEY_ENTER)
            nk_input_key(ctx, NK_KEY_ENTER, down);
        else if (sym == ALLEGRO_KEY_TAB)
            nk_input_key(ctx, NK_KEY_TAB, down);
        else if (sym == ALLEGRO_KEY_BACKSPACE)
            nk_input_key(ctx, NK_KEY_BACKSPACE, down);
        else if (sym == ALLEGRO_KEY_LEFT) {
            if (evt->keyboard.modifiers & ALLEGRO_KEYMOD_CTRL)
                nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
            else nk_input_key(ctx, NK_KEY_LEFT, down);
        } else if (sym == ALLEGRO_KEY_RIGHT) {
            if (evt->keyboard.modifiers & ALLEGRO_KEYMOD_CTRL)
                nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
            else nk_input_key(ctx, NK_KEY_RIGHT, down);
        } else if (sym == ALLEGRO_KEY_HOME) {
            nk_input_key(ctx, NK_KEY_TEXT_START, down);
            nk_input_key(ctx, NK_KEY_SCROLL_START, down);
        } else if (sym == ALLEGRO_KEY_END) {
            nk_input_key(ctx, NK_KEY_TEXT_END, down);
            nk_input_key(ctx, NK_KEY_SCROLL_END, down);
        } else if (sym == ALLEGRO_KEY_C)
            nk_input_key(ctx, NK_KEY_COPY, down && evt->keyboard.modifiers & ALLEGRO_KEYMOD_CTRL);
        else if (sym == ALLEGRO_KEY_PGUP)
            nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
        else if (sym == ALLEGRO_KEY_PGDN)
            nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
        else if (sym == ALLEGRO_KEY_V)
            nk_input_key(ctx, NK_KEY_PASTE, down && evt->keyboard.modifiers & ALLEGRO_KEYMOD_CTRL);
        else if (sym == ALLEGRO_KEY_X)
            nk_input_key(ctx, NK_KEY_CUT, down && evt->keyboard.modifiers & ALLEGRO_KEYMOD_CTRL);
        else if (sym == ALLEGRO_KEY_Z)
            nk_input_key(ctx, NK_KEY_TEXT_UNDO, down && evt->keyboard.modifiers & ALLEGRO_KEYMOD_CTRL);
        else if (sym == ALLEGRO_KEY_R)
            nk_input_key(ctx, NK_KEY_TEXT_REDO, down && evt->keyboard.modifiers & ALLEGRO_KEYMOD_CTRL);
        else if (sym == ALLEGRO_KEY_X)
            nk_input_key(ctx, NK_KEY_CUT, down && evt->keyboard.modifiers & ALLEGRO_KEYMOD_CTRL);
        else if (sym == ALLEGRO_KEY_B)
            nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down && evt->keyboard.modifiers & ALLEGRO_KEYMOD_CTRL);
        else if (sym == ALLEGRO_KEY_E)
            nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down && evt->keyboard.modifiers & ALLEGRO_KEYMOD_CTRL);

    } else if (evt->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN ||
                evt->type == ALLEGRO_EVENT_MOUSE_BUTTON_UP) {
        /* button handler */
        int down = evt->type == ALLEGRO_EVENT_MOUSE_BUTTON_DOWN;
        const int x = evt->mouse.x, y = evt->mouse.y;
        if (evt->mouse.button == 1)
            nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
        if (evt->mouse.button == 2)
            nk_input_button(ctx, NK_BUTTON_RIGHT, x, y, down);
    } else if (evt->type == ALLEGRO_EVENT_MOUSE_AXES) {
        /* mouse motion */
        nk_input_motion(ctx, evt->mouse.x, evt->mouse.y);
    } else if (evt->type == ALLEGRO_EVENT_KEY_CHAR) {
        /* text input */
        if (evt->keyboard.display == allegro.win)
            if (evt->keyboard.unichar > 32 && evt->keyboard.unichar < 0x10000)
                nk_input_unicode(ctx, (nk_rune)evt->keyboard.unichar);
    }
}

NK_API void
nk_allegro_shutdown(void)
{
    if (allegro.dev.texture)
        al_destroy_bitmap(allegro.dev.texture);
    free(allegro.dev.vertex_buffer);
    free(allegro.dev.element_buffer);
}

#endif
