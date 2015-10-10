/*
    Copyright (c) 2015 Micha Mettke

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
#include <assert.h>
#include <math.h>

#ifdef _WIN32
    #include <windows.h>
#endif

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>

#define NANOVG_GLES2_IMPLEMENTATION
#include "../../demo/nanovg/dep/nanovg.h"
#include "../../demo/nanovg/dep/nanovg_gl.h"
#include "../../demo/nanovg/dep/nanovg_gl_utils.h"
#include "../../demo/nanovg/dep/nanovg.c"

/* macros */
#define MAX_BUFFER  64
#define MAX_MEMORY  (16 * 1024)
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define DTIME       40
#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#define MAX(a,b)    ((a) < (b) ? (b) : (a))
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#define LEN(a)      (sizeof(a)/sizeof(a)[0])
#define UNUSED(a)   ((void)(a))

#include "../../zahnrad.h"

#define GATE_MAP(GATE)\
    GATE(NOT, "NOT", 1, 1, "1")\
    GATE(AND, "AND", 2, 1, "&")\
    GATE(OR, "OR", 2, 1, ">=1")\
    GATE(NAND, "NAND", 2, 1, "!&")\
    GATE(NOR, "NOR", 2, 1, ">1")\
    GATE(XOR, "XOR", 2, 1, "=1")\
    GATE(XNOR, "XNOR", 2, 1, "!=1")

enum gate_type {
#define GATE(id, name, in, out, sym) GATE_##id,
    GATE_MAP(GATE)
#undef GATE
    GATE_MAX
};

#define MAX_NAME 64
struct gate_config {
    enum gate_type type;
    const char *name;
    zr_int input_count;
    zr_int output_count;
    const char *sym;
};

struct gate {
    int ID;
    const struct gate_config *config;
    struct zr_rect bounds;
    struct gate *next;
    struct gate *prev;
};

static struct gate_config gate_configs[] = {
#define GATE(id, name, in, out, sym) {GATE_##id, name, in, out, sym},
    GATE_MAP(GATE)
#undef GATE
    GATE_MAX
};

struct gate_link {
    int input_id;
    int input_slot;
    int output_id;
    int output_slot;
};

struct gate_linking {
    zr_bool active;
    struct gate *gate;
    int input_id;
    int input_slot;
};

struct gate_editor {
    struct gate gate_buf[32];
    struct gate_link links[64];
    struct gate *begin;
    struct gate *end;
    int gate_count;
    int link_count;
    zr_state menu;
    struct zr_rect bounds;
    struct gate *selected;
    zr_bool show_grid;
    zr_bool show_header;
    struct zr_vec2 scrolling;
    struct gate_linking linking;
    struct zr_vec2 gate_size;
    struct zr_rect toolbar_bounds;
    struct zr_vec2 toolbar_scrollbar;
};

struct gui {
    void *memory;
    struct zr_window window;
    struct zr_input input;
    struct zr_command_queue queue;
    struct zr_style config;
    struct zr_user_font font;
};

/* =================================================================
 *
 *                      gate EDITOR
 *
 * =================================================================
 */
static void
gate_editor_push(struct gate_editor *editor, struct gate *gate)
{
    if (!editor->begin) {
        gate->next = NULL;
        gate->prev = NULL;
        editor->begin = gate;
        editor->end = gate;
    } else {
        gate->prev = editor->end;
        if (editor->end)
            editor->end->next = gate;
        gate->next = NULL;
        editor->end = gate;
    }
}

static void
gate_editor_pop(struct gate_editor *editor, struct gate *gate)
{
    if (gate->next)
        gate->next->prev = gate->prev;
    if (gate->prev)
        gate->prev->next = gate->next;
    if (editor->end == gate)
        editor->end = gate->prev;
    if (editor->begin == gate)
        editor->begin = gate->next;
    gate->next = NULL;
    gate->prev = NULL;
}

static struct gate*
gate_editor_find(struct gate_editor *editor, int ID)
{
    struct gate *iter = editor->begin;
    while (iter) {
        if (iter->ID == ID)
            return iter;
        iter = iter->next;
    }
    return NULL;
}

static void
gate_editor_add(struct gate_editor *editor, enum gate_type type, struct zr_rect bounds)
{
    static int IDs = 0;
    struct gate *gate;
    assert((zr_size)editor->gate_count < LEN(editor->gate_buf));
    gate = &editor->gate_buf[editor->gate_count++];
    gate->ID = IDs++;
    gate->bounds = bounds;
    gate->config = &gate_configs[type];
    gate_editor_push(editor, gate);
}

static void
gate_editor_link(struct gate_editor *editor, int in_id, int in_slot,
    int out_id, int out_slot)
{
    static int IDs = 0;
    struct gate_link *link;
    assert((zr_size)editor->link_count < LEN(editor->links));
    link = &editor->links[editor->link_count++];
    link->input_id = in_id;
    link->input_slot = in_slot;
    link->output_id = out_id;
    link->output_slot = out_slot;
}

static zr_bool
gate_button(struct zr_context *layout, enum gate_type type, enum zr_button_behavior behavior)
{
    zr_bool ret;
    struct zr_rect bounds, content;
    struct zr_button button;
    struct zr_vec2 item_padding;
    const struct zr_style *config;
    zr_size n;
    zr_float space, saved;

    const struct zr_input *i;
    enum zr_widget_state state;
    config = layout->style;
    state = zr_widget(&bounds, layout);
    if (!state) return zr_false;
    i = (state == ZR_WIDGET_ROM || layout->flags & ZR_WINDOW_ROM) ? 0 : layout->input;

    /* execute basic button */
    item_padding = zr_style_property(config, ZR_PROPERTY_ITEM_PADDING);
    button.rounding = 0;
    button.normal = config->colors[ZR_COLOR_BUTTON];
    button.hover = config->colors[ZR_COLOR_BUTTON_HOVER];
    button.active = config->colors[ZR_COLOR_BUTTON_ACTIVE];
    button.border = config->colors[ZR_COLOR_BORDER];
    button.border_width = 0;
    button.padding.x = item_padding.x;
    button.padding.y = item_padding.y;
    ret = zr_widget_button(layout->buffer, bounds, &button, i, behavior, &content);

    /* draw logical gate */
    content.x += item_padding.x;
    content.y += item_padding.y;
    content.w -= 2 * item_padding.x;
    content.h -= 2 * item_padding.y;
    zr_command_buffer_push_rect(layout->buffer, content, 0, zr_rgb(60, 60, 60));

    /* draw input connectors */
    space = content.h / ((gate_configs[type].input_count) + 1);
    for (n = 0; n < gate_configs[type].input_count; ++n) {
        struct zr_rect circle;
        circle.x = content.x-4;
        circle.y = content.y + space * (n+1);
        circle.w = 8; circle.h = 8;
        zr_command_buffer_push_circle(layout->buffer, circle, zr_rgb(100, 100, 100));
    }

    /* draw output connectors */
    space = content.h / ((gate_configs[type].output_count) + 1);
    for (n = 0; n < gate_configs[type].output_count; ++n) {
        struct zr_rect circle;
        circle.x = content.x + content.w-4;
        circle.y = content.y + space * (n+1);
        circle.w = 8; circle.h = 8;
        zr_command_buffer_push_circle(layout->buffer, circle, zr_rgb(100, 100, 100));
    }

    saved = content.x + content.w;
    content.w -= 2 * item_padding.x;
    content.h -= 2 * item_padding.y;
    content.x += content.w / 2 - item_padding.x;
    content.y += item_padding.y;
    content.w = saved-4 - content.x;
    zr_command_buffer_push_text(layout->buffer, content, gate_configs[type].sym,
        strlen(gate_configs[type].sym), &config->font, zr_rgb(60,60,60), zr_rgb(200,200,200));
    return ret;
}

static void
gate_editor_draw(struct zr_context *layout, struct gate_editor *gatedit,
    struct zr_style *config)
{
    int i = 0, n = 0;
    struct zr_rect total_space;
    const struct zr_input *in = zr_input(layout);
    struct zr_command_buffer *canvas = zr_canvas(layout);
    struct gate *updated = 0;

    /* allocate complete window space */
    total_space = zr_space(layout);
    zr_layout_row_space_begin(layout, ZR_STATIC, total_space.h, (zr_size)gatedit->gate_count);
    {
        struct zr_context gate;
        struct gate *it = gatedit->begin;
        struct zr_rect size = zr_layout_row_space_bounds(layout);

        if (gatedit->show_grid) {
            /* display grid */
            zr_float x, y;
            const zr_float grid_size = 22.0f;
            const struct zr_color grid_color = zr_rgb(50, 50, 50);
            for (x = fmod(size.x - gatedit->scrolling.x, grid_size); x < size.w; x += grid_size)
                zr_command_buffer_push_line(canvas, x+size.x, size.y, x+size.x, size.y+size.h, grid_color);
            for (y = fmod(size.y - gatedit->scrolling.y, grid_size); y < size.h; y += grid_size)
                zr_command_buffer_push_line(canvas, size.x, y+size.y, size.x+size.w, y+size.y, grid_color);
        }

        /* ================================== GATE ============================================ */
        zr_style_push_color(config, ZR_COLOR_WINDOW, zr_rgb(48, 48, 48));
        while (it) {
            /* draw gate */
            zr_layout_row_space_push(layout, zr_rect(it->bounds.x - gatedit->scrolling.x,
                it->bounds.y - gatedit->scrolling.y, it->bounds.w, it->bounds.h));
            zr_group_begin(layout, &gate, (gatedit->show_header) ? it->config->name : NULL,
                ZR_WINDOW_MOVEABLE|ZR_WINDOW_NO_SCROLLBAR, zr_vec2(0,0));
            {
                /* always have last selected gate on top */
                if (zr_input_mouse_clicked(in, ZR_BUTTON_LEFT, gate.bounds) &&
                    (!(it->prev && zr_input_mouse_clicked(in, ZR_BUTTON_LEFT,
                    zr_layout_row_space_rect_to_screen(layout, gate.bounds)))) &&
                    gatedit->end != it)
                {
                    updated = it;
                }

                /* logic gate content */
                zr_layout_row_dynamic(&gate, (gatedit->show_header) ? 40 : 70, 1);
                zr_label(&gate, it->config->sym, ZR_TEXT_CENTERED);
            }
            zr_group_end(layout, &gate, NULL);

            {
                /* gate connector and linking */
                zr_float space;
                struct zr_rect bounds;
                bounds = zr_layout_row_space_rect_to_local(layout, gate.bounds);
                bounds.x += gatedit->scrolling.x;
                bounds.y += gatedit->scrolling.y;
                it->bounds = bounds;

                /* output connector */
                space = gate.bounds.h / ((it->config->output_count) + 1);
                for (n = 0; n < it->config->output_count; ++n) {
                    struct zr_rect circle;
                    circle.x = gate.bounds.x + gate.bounds.w-4;
                    circle.y = gate.bounds.y + space * (n+1);
                    circle.w = 8; circle.h = 8;
                    zr_command_buffer_push_circle(canvas, circle, zr_rgb(100, 100, 100));

                    /* start linking process */
                    if (zr_input_is_mouse_click_in_rect(in, ZR_BUTTON_LEFT, circle)) {
                        gatedit->linking.active = zr_true;
                        gatedit->linking.gate = it;
                        gatedit->linking.input_id = it->ID;
                        gatedit->linking.input_slot = n;
                    }

                    /* draw curve from linked gate slot to mouse position */
                    if (gatedit->linking.active && gatedit->linking.gate == it &&
                        gatedit->linking.input_slot == n) {
                        struct zr_vec2 l0 = zr_vec2(circle.x + 3, circle.y + 3);
                        struct zr_vec2 l1 = in->mouse.pos;
                        zr_command_buffer_push_curve(canvas, l0.x, l0.y, l0.x + 50.0f, l0.y,
                            l1.x - 50.0f, l1.y, l1.x, l1.y, zr_rgb(100, 100, 100));
                    }
                }

                /* input connector */
                space = gate.bounds.h / ((it->config->input_count) + 1);
                for (n = 0; n < it->config->input_count; ++n) {
                    struct zr_rect circle;
                    circle.x = gate.bounds.x-4;
                    circle.y = gate.bounds.y + space * (n+1);
                    circle.w = 8; circle.h = 8;
                    zr_command_buffer_push_circle(canvas, circle, zr_rgb(100, 100, 100));
                    if (zr_input_is_mouse_released(in, ZR_BUTTON_LEFT) &&
                        zr_input_is_mouse_hovering_rect(in, circle) &&
                        gatedit->linking.active && gatedit->linking.gate != it) {
                        gatedit->linking.active = zr_false;
                        gate_editor_link(gatedit, gatedit->linking.input_id,
                            gatedit->linking.input_slot, it->ID, n);
                    }
                }
            }
            it = it->next;
        }

        /* reset linking connection */
        if (gatedit->linking.active && zr_input_is_mouse_released(in, ZR_BUTTON_LEFT)) {
            gatedit->linking.active = zr_false;
            gatedit->linking.gate = NULL;
            fprintf(stdout, "linking failed\n");
        }

        /* draw each link */
        for (n = 0; n < gatedit->link_count; ++n) {
            struct gate_link *link = &gatedit->links[n];
            struct gate *ni = gate_editor_find(gatedit, link->input_id);
            struct gate *no = gate_editor_find(gatedit, link->output_id);
            zr_float spacei = gate.bounds.h / ((ni->config->output_count) + 1);
            zr_float spaceo = gate.bounds.h / ((no->config->input_count) + 1);
            struct zr_vec2 l0 = zr_layout_row_space_to_screen(layout,
                zr_vec2(ni->bounds.x + ni->bounds.w, 3+ni->bounds.y + spacei * (link->input_slot+1)));
            struct zr_vec2 l1 = zr_layout_row_space_to_screen(layout,
                zr_vec2(no->bounds.x, 3 + no->bounds.y + spaceo * (link->output_slot+1)));

            l0.x -= gatedit->scrolling.x;
            l0.y -= gatedit->scrolling.y;
            l1.x -= gatedit->scrolling.x;
            l1.y -= gatedit->scrolling.y;
            zr_command_buffer_push_curve(canvas, l0.x, l0.y, l0.x + 50.0f, l0.y,
                l1.x - 50.0f, l1.y, l1.x, l1.y, zr_rgb(100, 100, 100));
        }
        zr_style_pop_color(config);

        if (updated) {
            /* reshuffle gates to have last recently selected gate on the top */
            gate_editor_pop(gatedit, updated);
            gate_editor_push(gatedit, updated);
        }

        /* ================================== MENU ============================================ */
        /* gate selection + right click popup context menu activation */
        if (gatedit->menu == ZR_INACTIVE &&
            zr_input_mouse_clicked(in, ZR_BUTTON_RIGHT, zr_layout_row_space_bounds(layout))) {
            it = gatedit->begin;
            gatedit->selected = NULL;
            gatedit->menu = ZR_ACTIVE;
            gatedit->bounds = zr_rect(in->mouse.pos.x, in->mouse.pos.y, 100, 400);
            while (it) {
                struct zr_rect b = zr_layout_row_space_rect_to_screen(layout, it->bounds);
                b.x -= gatedit->scrolling.x;
                b.y -= gatedit->scrolling.y;
                if (zr_input_is_mouse_hovering_rect(in, b))
                    gatedit->selected = it;
                it = it->next;
            }
        }

        /* popup context menu */
        if (gatedit->menu == ZR_ACTIVE) {
            struct zr_context menu;
            const char *grid_option[] = {"Show Grid", "Hide Grid"};
            const char *header_option[] = {"Show Header", "Hide Header"};
            zr_contextual_begin(layout, &menu, ZR_WINDOW_NO_SCROLLBAR, &gatedit->menu, gatedit->bounds);
            {
                zr_layout_row_dynamic(&menu, 25, 1);
                if (!gatedit->selected) {
                    /* TODO(micha): probably want this to be at mouse position */
                    struct zr_rect bounds;
                    bounds.x = 400;
                    bounds.y = 260;
                    bounds.w = gatedit->gate_size.x;
                    bounds.h = gatedit->gate_size.y;

                    /* menu content if no selected gate */
                    if (zr_contextual_item(&menu, "NOT", ZR_TEXT_CENTERED))
                        gate_editor_add(gatedit, GATE_NOT, bounds);
                    if (zr_contextual_item(&menu, "AND", ZR_TEXT_CENTERED))
                        gate_editor_add(gatedit, GATE_AND, bounds);
                    if (zr_contextual_item(&menu, "OR", ZR_TEXT_CENTERED))
                        gate_editor_add(gatedit, GATE_OR, bounds);
                    if (zr_contextual_item(&menu, "NAND", ZR_TEXT_CENTERED))
                        gate_editor_add(gatedit, GATE_NAND, bounds);
                    if (zr_contextual_item(&menu, "NOR", ZR_TEXT_CENTERED))
                        gate_editor_add(gatedit, GATE_NOR, bounds);
                    if (zr_contextual_item(&menu, "XOR", ZR_TEXT_CENTERED))
                        gate_editor_add(gatedit, GATE_XOR, bounds);
                    if (zr_contextual_item(&menu, "XNOR", ZR_TEXT_CENTERED))
                        gate_editor_add(gatedit, GATE_XNOR, bounds);
                    if (zr_contextual_item(&menu, grid_option[gatedit->show_grid],ZR_TEXT_CENTERED))
                        gatedit->show_grid = !gatedit->show_grid;
                    if (zr_contextual_item(&menu, header_option[gatedit->show_header],ZR_TEXT_CENTERED))
                        gatedit->show_header = !gatedit->show_header;
                } else {
                    /* menu content if selected gate */
                    if (zr_contextual_item(&menu, "Delete", ZR_TEXT_CENTERED))
                        fprintf(stdout, "pressed delete!\n");
                    if (zr_contextual_item(&menu, "Copy", ZR_TEXT_CENTERED))
                        fprintf(stdout, "pressed copy!\n");
                }
            }
            zr_contextual_end(layout, &menu, &gatedit->menu);
        }
        /* ================================ TOOLBAR ========================================== */
        zr_layout_row_space_push(layout, gatedit->toolbar_bounds);
        zr_group_begin(layout, &gate, "Toolbar", ZR_WINDOW_MOVEABLE|ZR_WINDOW_NO_SCROLLBAR,
            gatedit->toolbar_scrollbar);
        {
            /* TODO(micha): probably want this to be draggable */
            struct zr_rect bounds;
            bounds.x = 400;
            bounds.y = 260;
            bounds.w = gatedit->gate_size.x;
            bounds.h = gatedit->gate_size.y;
            zr_size type = 0;

            zr_layout_row_dynamic(&gate, 60, 1);
            for (type = 0; type < GATE_MAX; ++type) {
                if (gate_button(&gate, type, ZR_BUTTON_DEFAULT))
                    gate_editor_add(gatedit, type, bounds);
            }
        }
        gatedit->toolbar_bounds = zr_layout_row_space_rect_to_local(layout, gate.bounds);
        zr_group_end(layout, &gate, &gatedit->toolbar_scrollbar);
        /* =================================================================================== */
    }
    zr_layout_row_space_end(layout);

    if (zr_input_is_mouse_hovering_rect(in, layout->bounds) &&
        zr_input_is_mouse_down(in, ZR_BUTTON_MIDDLE)) {
        gatedit->scrolling.x += in->mouse.delta.x;
        gatedit->scrolling.y += in->mouse.delta.y;
    }
}

static void
gate_editor_init(struct gate_editor *editor)
{
    zr_float w,h;
    memset(editor, 0, sizeof(*editor));
    editor->begin = NULL;
    editor->end = NULL;
    w = h = 100;
    editor->gate_size = zr_vec2(w, h);
    editor->toolbar_bounds = zr_rect(20, 50, 100, 520);
    gate_editor_add(editor, GATE_AND, zr_rect(40, 10, w, h));
    gate_editor_add(editor, GATE_AND, zr_rect(40, 150, w, h));
    gate_editor_add(editor, GATE_OR, zr_rect(200, 70, w, h));
    gate_editor_link(editor, 0, 0, 2, 0);
    gate_editor_link(editor, 1, 0, 2, 1);
    editor->show_grid = zr_true;
    editor->show_header = zr_true;
}

/* =================================================================
 *
 *                          APP
 *
 * =================================================================
 */
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

static zr_size
font_get_width(zr_handle handle, const zr_char *text, zr_size len)
{
    zr_size width;
    float bounds[4];
    NVGcontext *ctx = (NVGcontext*)handle.ptr;
    nvgTextBounds(ctx, 0, 0, text, &text[len], bounds);
    width = (zr_size)(bounds[2] - bounds[0]);
    return width;
}

static void
draw(NVGcontext *nvg, struct zr_command_queue *queue, int width, int height)
{
    const struct zr_command *cmd;
    glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_TEXTURE_2D);

    nvgBeginFrame(nvg, width, height, ((float)width/(float)height));
    zr_foreach_command(cmd, queue) {
        switch (cmd->type) {
        case ZR_COMMAND_NOP: break;
        case ZR_COMMAND_SCISSOR: {
            const struct zr_command_scissor *s = zr_command(scissor, cmd);
            nvgScissor(nvg, s->x, s->y, s->w, s->h);
        } break;
        case ZR_COMMAND_LINE: {
            const struct zr_command_line *l = zr_command(line, cmd);
            nvgBeginPath(nvg);
            nvgMoveTo(nvg, l->begin.x, l->begin.y);
            nvgLineTo(nvg, l->end.x, l->end.y);
            nvgFillColor(nvg, nvgRGBA(l->color.r, l->color.g, l->color.b, l->color.a));
            nvgFill(nvg);
        } break;
        case ZR_COMMAND_CURVE: {
            const struct zr_command_curve *q = zr_command(curve, cmd);
            nvgBeginPath(nvg);
            nvgMoveTo(nvg, q->begin.x, q->begin.y);
            nvgBezierTo(nvg, q->ctrl[0].x, q->ctrl[0].y, q->ctrl[1].x,
                q->ctrl[1].y, q->end.x, q->end.y);
            nvgStrokeColor(nvg, nvgRGBA(q->color.r, q->color.g, q->color.b, q->color.a));
            nvgStrokeWidth(nvg, 2);
            nvgStroke(nvg);
        } break;
        case ZR_COMMAND_RECT: {
            const struct zr_command_rect *r = zr_command(rect, cmd);
            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, r->x, r->y, r->w, r->h, r->rounding);
            nvgFillColor(nvg, nvgRGBA(r->color.r, r->color.g, r->color.b, r->color.a));
            nvgFill(nvg);
        } break;
        case ZR_COMMAND_CIRCLE: {
            const struct zr_command_circle *c = zr_command(circle, cmd);
            nvgBeginPath(nvg);
            nvgCircle(nvg, c->x + (c->w/2.0f), c->y + c->w/2.0f, c->w/2.0f);
            nvgFillColor(nvg, nvgRGBA(c->color.r, c->color.g, c->color.b, c->color.a));
            nvgFill(nvg);
        } break;
        case ZR_COMMAND_TRIANGLE: {
            const struct zr_command_triangle *t = zr_command(triangle, cmd);
            nvgBeginPath(nvg);
            nvgMoveTo(nvg, t->a.x, t->a.y);
            nvgLineTo(nvg, t->b.x, t->b.y);
            nvgLineTo(nvg, t->c.x, t->c.y);
            nvgLineTo(nvg, t->a.x, t->a.y);
            nvgFillColor(nvg, nvgRGBA(t->color.r, t->color.g, t->color.b, t->color.a));
            nvgFill(nvg);
        } break;
        case ZR_COMMAND_TEXT: {
            const struct zr_command_text *t = zr_command(text, cmd);
            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, t->x, t->y, t->w, t->h, 0);
            nvgFillColor(nvg, nvgRGBA(t->background.r, t->background.g,
                t->background.b, t->background.a));
            nvgFill(nvg);

            nvgBeginPath(nvg);
            nvgFillColor(nvg, nvgRGBA(t->foreground.r, t->foreground.g,
                t->foreground.b, t->foreground.a));
            nvgTextAlign(nvg, NVG_ALIGN_MIDDLE);
            nvgText(nvg, t->x, t->y + t->h * 0.5f, t->string, &t->string[t->length]);
            nvgFill(nvg);
        } break;
        case ZR_COMMAND_IMAGE: {
            const struct zr_command_image *i = zr_command(image, cmd);
            NVGpaint imgpaint;
            imgpaint = nvgImagePattern(nvg, i->x, i->y, i->w, i->h, 0, i->img.handle.id, 1.0f);
            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, i->x, i->y, i->w, i->h, 0);
            nvgFillPaint(nvg, imgpaint);
            nvgFill(nvg);
        } break;
        default: break;
        }
    }
    zr_command_queue_clear(queue);

    nvgResetScissor(nvg);
    nvgEndFrame(nvg);
    glPopAttrib();
}

static void
key(struct zr_input *in, SDL_Event *evt, zr_bool down)
{
    const Uint8* state = SDL_GetKeyboardState(NULL);
    SDL_Keycode sym = evt->key.keysym.sym;
    if (sym == SDLK_RSHIFT || sym == SDLK_LSHIFT)
        zr_input_key(in, ZR_KEY_SHIFT, down);
    else if (sym == SDLK_DELETE)
        zr_input_key(in, ZR_KEY_DEL, down);
    else if (sym == SDLK_RETURN)
        zr_input_key(in, ZR_KEY_ENTER, down);
    else if (sym == SDLK_SPACE)
        zr_input_key(in, ZR_KEY_SPACE, down);
    else if (sym == SDLK_BACKSPACE)
        zr_input_key(in, ZR_KEY_BACKSPACE, down);
    else if (sym == SDLK_LEFT)
        zr_input_key(in, ZR_KEY_LEFT, down);
    else if (sym == SDLK_RIGHT)
        zr_input_key(in, ZR_KEY_RIGHT, down);
    else if (sym == SDLK_c)
        zr_input_key(in, ZR_KEY_COPY, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_v)
        zr_input_key(in, ZR_KEY_PASTE, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_x)
        zr_input_key(in, ZR_KEY_CUT, down && state[SDL_SCANCODE_LCTRL]);
}

static void
motion(struct zr_input *in, SDL_Event *evt)
{
    const zr_int x = evt->motion.x;
    const zr_int y = evt->motion.y;
    zr_input_motion(in, x, y);
}

static void
btn(struct zr_input *in, SDL_Event *evt, zr_bool down)
{
    const zr_int x = evt->button.x;
    const zr_int y = evt->button.y;
    if (evt->button.button == SDL_BUTTON_LEFT)
        zr_input_button(in, ZR_BUTTON_LEFT, x, y, down);
    else if (evt->button.button == SDL_BUTTON_RIGHT)
        zr_input_button(in, ZR_BUTTON_RIGHT, x, y, down);
    else if (evt->button.button == SDL_BUTTON_MIDDLE)
        zr_input_button(in, ZR_BUTTON_MIDDLE, x, y, down);
}

static void
text(struct zr_input *in, SDL_Event *evt)
{
    zr_glyph glyph;
    memcpy(glyph, evt->text.text, ZR_UTF_SIZE);
    zr_input_glyph(in, glyph);
}

static void
resize(SDL_Event *evt)
{
    if (evt->window.event != SDL_WINDOWEVENT_RESIZED) return;
    glViewport(0, 0, evt->window.data1, evt->window.data2);
}

int
main(int argc, char *argv[])
{
    /* Platform */
    int x, y, width, height;
    const char *font_path;
    zr_size font_height;
    SDL_SysWMinfo info;
    SDL_Window *win;
    SDL_GLContext glContext;
    NVGcontext *vg = NULL;
    zr_bool running = zr_true;
    unsigned int started;
    unsigned int dt;

    /* GUI */
    struct gate_editor gatedit;
    struct gui gui;

    if (argc < 2) {
        fprintf(stdout,"Missing TTF Font file argument: gui <path>\n");
        exit(EXIT_FAILURE);
    }
    font_path = argv[1];
    font_height = 12;

    /* SDL */
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_EVENTS);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    win = SDL_CreateWindow("Demo",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN|SDL_WINDOW_BORDERLESS);
    glContext = SDL_GL_CreateContext(win);
    SDL_GetWindowSize(win, &width, &height);
    SDL_GetWindowPosition(win, &x, &y);
    SDL_VERSION(&info.version);
    assert(SDL_GetWindowWMInfo(win, &info));

    /* OpenGL */
    glewExperimental = 1;
    if (glewInit() != GLEW_OK)
        die("[GLEW] failed setup\n");
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    /* nanovg */
    vg = nvgCreateGLES2(NVG_ANTIALIAS|NVG_DEBUG);
    if (!vg) die("[NVG]: failed to init\n");
    nvgCreateFont(vg, "fixed", font_path);
    nvgFontFace(vg, "fixed");
    nvgFontSize(vg, font_height);
    nvgTextAlign(vg, NVG_ALIGN_LEFT|NVG_ALIGN_MIDDLE);

    /* GUI */
    memset(&gui, 0, sizeof gui);
    gui.memory = malloc(MAX_MEMORY);
    zr_command_queue_init_fixed(&gui.queue, gui.memory, MAX_MEMORY);

    gui.font.userdata.ptr = vg;
    nvgTextMetrics(vg, NULL, NULL, &gui.font.height);
    gui.font.width = font_get_width;
    zr_style_default(&gui.config, ZR_DEFAULT_ALL, &gui.font);

    zr_window_init(&gui.window, zr_rect(0, 0, width, height),
        ZR_WINDOW_BORDER|ZR_WINDOW_NO_SCROLLBAR,
        &gui.queue, &gui.config, &gui.input);
    gate_editor_init(&gatedit);

    while (running) {
        /* Input */
        SDL_Event evt;
        started = SDL_GetTicks();
        zr_input_begin(&gui.input);
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_WINDOWEVENT) resize(&evt);
            else if (evt.type == SDL_QUIT) goto cleanup;
            else if (evt.type == SDL_KEYUP) key(&gui.input, &evt, zr_false);
            else if (evt.type == SDL_KEYDOWN) key(&gui.input, &evt, zr_true);
            else if (evt.type == SDL_MOUSEBUTTONDOWN) btn(&gui.input, &evt, zr_true);
            else if (evt.type == SDL_MOUSEBUTTONUP) btn(&gui.input, &evt, zr_false);
            else if (evt.type == SDL_MOUSEMOTION) motion(&gui.input, &evt);
            else if (evt.type == SDL_TEXTINPUT) text(&gui.input, &evt);
            else if (evt.type == SDL_MOUSEWHEEL) zr_input_scroll(&gui.input, evt.wheel.y);
        }
        zr_input_end(&gui.input);

        {
            zr_bool incursor;
            struct zr_context layout;
            struct zr_rect bounds = gui.window.bounds;

            zr_begin(&layout, &gui.window);
            if (zr_header(&layout, "Gate Editor", ZR_CLOSEABLE, ZR_CLOSEABLE, ZR_HEADER_RIGHT))
                goto cleanup;

            /* borderless os window dragging */
            incursor = zr_input_is_mouse_hovering_rect(&gui.input,
                zr_rect(layout.bounds.x, layout.bounds.y, layout.bounds.w, layout.header.h));
            if (zr_input_is_mouse_down(&gui.input, ZR_BUTTON_LEFT) && incursor) {
                x += gui.input.mouse.delta.x;
                y += gui.input.mouse.delta.y;
                SDL_SetWindowPosition(win, x, y);
            }
            gate_editor_draw(&layout, &gatedit, &gui.config);
            zr_end(&layout, &gui.window);
        }

        /* GUI */
        glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        SDL_GetWindowSize(win, &width, &height);
        draw(vg, &gui.queue, width, height);
        SDL_GL_SwapWindow(win);

        /* Timing */
        dt = SDL_GetTicks() - started;
        if (dt < DTIME)
            SDL_Delay(DTIME - dt);
    }

cleanup:
    /* Cleanup */
    free(gui.memory);
    nvgDeleteGLES2(vg);
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
