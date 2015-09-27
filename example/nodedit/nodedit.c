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

struct node {
    int ID;
    char name[32];
    struct zr_rect bounds;
    zr_float value;
    struct zr_color color;
    zr_int input_count;
    zr_int output_count;
    struct node *next;
    struct node *prev;
};

struct node_link {
    int input_id;
    int input_slot;
    int output_id;
    int output_slot;
    struct zr_vec2 in;
    struct zr_vec2 out;
};

struct node_linking {
    zr_bool active;
    struct node *node;
    int input_id;
    int input_slot;
};

struct node_editor {
    struct node node_buf[32];
    struct node_link links[64];
    struct node *begin;
    struct node *end;
    int node_count;
    int link_count;
    zr_state menu;
    struct zr_rect bounds;
    struct node *selected;
    zr_bool show_grid;
    struct zr_vec2 scrolling;
    struct node_linking linking;
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
 *                      NODE EDITOR
 *
 * =================================================================
 */
static void
node_editor_push(struct node_editor *editor, struct node *node)
{
    if (!editor->begin) {
        node->next = NULL;
        node->prev = NULL;
        editor->begin = node;
        editor->end = node;
    } else {
        node->prev = editor->end;
        if (editor->end)
            editor->end->next = node;
        node->next = NULL;
        editor->end = node;
    }
}

static void
node_editor_pop(struct node_editor *editor, struct node *node)
{
    if (node->next)
        node->next->prev = node->prev;
    if (node->prev)
        node->prev->next = node->next;
    if (editor->end == node)
        editor->end = node->prev;
    if (editor->begin == node)
        editor->begin = node->next;
    node->next = NULL;
    node->prev = NULL;
}

static struct node*
node_editor_find(struct node_editor *editor, int ID)
{
    struct node *iter = editor->begin;
    while (iter) {
        if (iter->ID == ID)
            return iter;
        iter = iter->next;
    }
    return NULL;
}

static void
node_editor_add(struct node_editor *editor, const char *name, struct zr_rect bounds,
    struct zr_color col, int in_count, int out_count)
{
    static int IDs = 0;
    struct node *node;
    assert((zr_size)editor->node_count < LEN(editor->node_buf));
    node = &editor->node_buf[editor->node_count++];
    node->ID = IDs++;
    node->value = 0;
    node->color = zr_rgb(255, 0, 0);
    node->input_count = in_count;
    node->output_count = out_count;
    node->color = col;
    strcpy(node->name, name);
    node->bounds = bounds;
    node_editor_push(editor, node);
}

static void
node_editor_link(struct node_editor *editor, int in_id, int in_slot,
    int out_id, int out_slot)
{
    static int IDs = 0;
    struct node_link *link;
    assert((zr_size)editor->link_count < LEN(editor->links));
    link = &editor->links[editor->link_count++];
    link->input_id = in_id;
    link->input_slot = in_slot;
    link->output_id = out_id;
    link->output_slot = out_slot;
}

static void
node_editor_draw(struct zr_context *layout, struct node_editor *nodedit,
    struct zr_style *config)
{
    int i = 0, n = 0;
    struct zr_rect total_space;
    const struct zr_input *in = zr_input(layout);
    struct zr_command_buffer *canvas = zr_canvas(layout);
    struct node *updated = 0;

    total_space = zr_space(layout);
    zr_layout_row_space_begin(layout, ZR_STATIC, total_space.h, (zr_size)nodedit->node_count);
    {
        struct zr_context node;
        struct node *it = nodedit->begin;
        struct zr_rect size = zr_layout_row_space_bounds(layout);

        if (nodedit->show_grid) {
            /* display grid */
            zr_float x, y;
            const zr_float grid_size = 32.0f;
            const struct zr_color grid_color = zr_rgb(50, 50, 50);
            for (x = fmod(size.x - nodedit->scrolling.x, grid_size); x < size.w; x += grid_size)
                zr_command_buffer_push_line(canvas, x+size.x, size.y, x+size.x, size.y+size.h, grid_color);
            for (y = fmod(size.y - nodedit->scrolling.y, grid_size); y < size.h; y += grid_size)
                zr_command_buffer_push_line(canvas, size.x, y+size.y, size.x+size.w, y+size.y, grid_color);
        }

        /* execute each node */
        zr_style_push_color(config, ZR_COLOR_WINDOW, zr_rgb(48, 48, 48));
        while (it) {
            /* draw node */
            zr_layout_row_space_push(layout, zr_rect(it->bounds.x - nodedit->scrolling.x,
                it->bounds.y - nodedit->scrolling.y, it->bounds.w, it->bounds.h));
            zr_group_begin(layout, &node, it->name,
                ZR_WINDOW_MOVEABLE|ZR_WINDOW_NO_SCROLLBAR, zr_vec2(0,0));
            {
                struct zr_color color;
                const char *str[] = {"R:", "G:", "B:", "A:"};
                zr_float ratio[] = {0.25f, 0.75f};
                zr_byte *iter = &it->color.r;

                /* always have last selected node on top */
                if (zr_input_mouse_clicked(in, ZR_BUTTON_LEFT, node.bounds) &&
                    (!(it->prev && zr_input_mouse_clicked(in, ZR_BUTTON_LEFT,
                    zr_layout_row_space_rect_to_screen(layout, node.bounds)))) &&
                    nodedit->end != it)
                {
                    updated = it;
                }

                /* ================= NODE CONTENT =====================*/
                zr_layout_row_dynamic(&node, 30, 1);
                color = it->color;
                zr_button_color(&node, color, ZR_BUTTON_DEFAULT);
                zr_layout_row(&node, ZR_DYNAMIC, 30, 2, ratio);
                for (n = 0; n < 3; ++n) {
                    zr_float t = *iter;
                    zr_label(&node, str[n], ZR_TEXT_LEFT);
                    t = zr_slider(&node, 0, t, 255, 10);
                    *iter = (zr_byte)t;
                    iter++;
                }
                /* ====================================================*/
            }
            zr_group_end(layout, &node);

            {
                /* node connector and linking */
                zr_float space;
                struct zr_rect bounds;
                bounds = zr_layout_row_space_rect_to_local(layout, node.bounds);
                bounds.x += nodedit->scrolling.x;
                bounds.y += nodedit->scrolling.y;
                it->bounds = bounds;

                /* output connector */
                space = node.bounds.h / ((it->output_count) + 1);
                for (n = 0; n < it->output_count; ++n) {
                    struct zr_rect circle;
                    circle.x = node.bounds.x + node.bounds.w-4;
                    circle.y = node.bounds.y + space * (n+1);
                    circle.w = 8; circle.h = 8;
                    zr_command_buffer_push_circle(canvas, circle, zr_rgb(100, 100, 100));

                    /* start linking process */
                    if (zr_input_is_mouse_click_in_rect(in, ZR_BUTTON_LEFT, circle)) {
                        nodedit->linking.active = zr_true;
                        nodedit->linking.node = it;
                        nodedit->linking.input_id = it->ID;
                        nodedit->linking.input_slot = n;
                    }

                    /* draw curve from linked node slot to mouse position */
                    if (nodedit->linking.active && nodedit->linking.node == it &&
                        nodedit->linking.input_slot == n) {
                        struct zr_vec2 l0 = zr_vec2(circle.x + 3, circle.y + 3);
                        struct zr_vec2 l1 = in->mouse.pos;
                        zr_command_buffer_push_curve(canvas, l0.x, l0.y, l0.x + 50.0f, l0.y,
                            l1.x - 50.0f, l1.y, l1.x, l1.y, zr_rgb(100, 100, 100));
                    }
                }

                /* input connector */
                space = node.bounds.h / ((it->input_count) + 1);
                for (n = 0; n < it->input_count; ++n) {
                    struct zr_rect circle;
                    circle.x = node.bounds.x-4;
                    circle.y = node.bounds.y + space * (n+1);
                    circle.w = 8; circle.h = 8;
                    zr_command_buffer_push_circle(canvas, circle, zr_rgb(100, 100, 100));
                    if (zr_input_is_mouse_released(in, ZR_BUTTON_LEFT) &&
                        zr_input_is_mouse_hovering_rect(in, circle) &&
                        nodedit->linking.active && nodedit->linking.node != it) {
                        nodedit->linking.active = zr_false;
                        node_editor_link(nodedit, nodedit->linking.input_id,
                            nodedit->linking.input_slot, it->ID, n);
                    }
                }
            }
            it = it->next;
        }

        /* reset linking connection */
        if (nodedit->linking.active && zr_input_is_mouse_released(in, ZR_BUTTON_LEFT)) {
            nodedit->linking.active = zr_false;
            nodedit->linking.node = NULL;
            fprintf(stdout, "linking failed\n");
        }

        /* draw each link */
        for (n = 0; n < nodedit->link_count; ++n) {
            struct node_link *link = &nodedit->links[n];
            struct node *ni = node_editor_find(nodedit, link->input_id);
            struct node *no = node_editor_find(nodedit, link->output_id);
            zr_float spacei = node.bounds.h / ((ni->output_count) + 1);
            zr_float spaceo = node.bounds.h / ((no->input_count) + 1);
            struct zr_vec2 l0 = zr_layout_row_space_to_screen(layout,
                zr_vec2(ni->bounds.x + ni->bounds.w, 3+ni->bounds.y + spacei * (link->input_slot+1)));
            struct zr_vec2 l1 = zr_layout_row_space_to_screen(layout,
                zr_vec2(no->bounds.x, 3 + no->bounds.y + spaceo * (link->output_slot+1)));

            l0.x -= nodedit->scrolling.x;
            l0.y -= nodedit->scrolling.y;
            l1.x -= nodedit->scrolling.x;
            l1.y -= nodedit->scrolling.y;
            zr_command_buffer_push_curve(canvas, l0.x, l0.y, l0.x + 50.0f, l0.y,
                l1.x - 50.0f, l1.y, l1.x, l1.y, zr_rgb(100, 100, 100));
        }
        zr_style_pop_color(config);

        if (updated) {
            /* reshuffle nodes to have last recently selected node on the top */
            node_editor_pop(nodedit, updated);
            node_editor_push(nodedit, updated);
        }

        {
            /* right click popup context menu activation */
            if (nodedit->menu == ZR_INACTIVE &&
                zr_input_mouse_clicked(in, ZR_BUTTON_RIGHT, zr_layout_row_space_bounds(layout))) {
                it = nodedit->begin;
                nodedit->selected = NULL;
                nodedit->menu = ZR_ACTIVE;
                nodedit->bounds = zr_rect(in->mouse.pos.x, in->mouse.pos.y, 100, 200);
                while (it) {
                    struct zr_rect b = zr_layout_row_space_rect_to_screen(layout, it->bounds);
                    b.x -= nodedit->scrolling.x;
                    b.y -= nodedit->scrolling.y;
                    if (zr_input_is_mouse_hovering_rect(in, b))
                        nodedit->selected = it;
                    it = it->next;
                }
            }

            /* popup context menu */
            if (nodedit->menu == ZR_ACTIVE) {
                struct zr_context menu;
                const char *grid_option[] = {"Show Grid", "Hide Grid"};
                zr_contextual_begin(layout, &menu, ZR_WINDOW_NO_SCROLLBAR, &nodedit->menu,
                    nodedit->bounds);
                zr_layout_row_dynamic(&menu, 25, 1);
                if (!nodedit->selected) {
                    /* menu content if no selected node */
                    if (zr_contextual_item(&menu, "New", ZR_TEXT_CENTERED)) {
                        node_editor_add(nodedit, "New", zr_rect(400, 260, 180, 220),
                                zr_rgb(255, 255, 255), 1, 2);
                    }
                    if (zr_contextual_item(&menu, grid_option[nodedit->show_grid],ZR_TEXT_CENTERED))
                        nodedit->show_grid = !nodedit->show_grid;
                } else {
                    /* menu content if selected node */
                    if (zr_contextual_item(&menu, "Delete", ZR_TEXT_CENTERED))
                        fprintf(stdout, "pressed delete!\n");
                    if (zr_contextual_item(&menu, "Rename", ZR_TEXT_CENTERED))
                        fprintf(stdout, "pressed rename!\n");
                    if (zr_contextual_item(&menu, "Copy", ZR_TEXT_CENTERED))
                        fprintf(stdout, "pressed copy!\n");
                }
                nodedit->menu = zr_contextual_end(layout, &menu);
            }
        }
    }
    zr_layout_row_space_end(layout);
    if (zr_input_is_mouse_hovering_rect(in, layout->bounds) &&
        zr_input_is_mouse_down(in, ZR_BUTTON_MIDDLE)) {
        nodedit->scrolling.x += in->mouse.delta.x;
        nodedit->scrolling.y += in->mouse.delta.y;
    }
}

static void
node_editor_init(struct node_editor *editor)
{
    memset(editor, 0, sizeof(*editor));
    editor->begin = NULL;
    editor->end = NULL;
    node_editor_add(editor, "Source", zr_rect(40, 10, 180, 220), zr_rgb(255, 0, 0), 0, 1);
    node_editor_add(editor, "Source", zr_rect(40, 260, 180, 220), zr_rgb(0, 255, 0), 0, 1);
    node_editor_add(editor, "Combine", zr_rect(400, 100, 180, 220), zr_rgb(0,0,255), 2, 2);
    node_editor_link(editor, 0, 0, 2, 0);
    node_editor_link(editor, 1, 0, 2, 1);
    editor->show_grid = zr_true;
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
            nvgStrokeWidth(nvg, 3);
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
    struct node_editor nodedit;
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
    node_editor_init(&nodedit);

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
            if (zr_header(&layout, "Node Editor", ZR_CLOSEABLE, ZR_CLOSEABLE, ZR_HEADER_RIGHT))
                goto cleanup;

            /* borderless os window dragging */
            incursor = zr_input_is_mouse_hovering_rect(&gui.input,
                zr_rect(layout.bounds.x, layout.bounds.y, layout.bounds.w, layout.header.h));
            if (zr_input_is_mouse_down(&gui.input, ZR_BUTTON_LEFT) && incursor) {
                x += gui.input.mouse.delta.x;
                y += gui.input.mouse.delta.y;
                SDL_SetWindowPosition(win, x, y);
            }
            node_editor_draw(&layout, &nodedit, &gui.config);
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
