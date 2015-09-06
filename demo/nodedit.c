/*
    Copyright (c) 2015
    vurtun <polygone@gmx.net>
    MIT licence
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#ifdef _WIN32
    #include <windows.h>
#endif

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <SDL2/SDL.h>

#define NANOVG_GLES2_IMPLEMENTATION
#include "nanovg/nanovg.h"
#include "nanovg/nanovg_gl.h"
#include "nanovg/nanovg_gl_utils.h"

/* macros */
#define MAX_BUFFER  64
#define MAX_MEMORY  (16 * 1024)
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800
#define DTIME       33
#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#define MAX(a,b)    ((a) < (b) ? (b) : (a))
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#define LEN(a)      (sizeof(a)/sizeof(a)[0])
#define UNUSED(a)   ((void)(a))

#include "../gui.h"

struct node {
    int ID;
    char name[32];
    struct gui_rect bounds;
    gui_float value;
    struct gui_color color;
    gui_int input_count;
    gui_int output_count;
    struct node *next;
    struct node *prev;
};

struct node_link {
    int input_id;
    int input_slot;
    int output_id;
    int output_slot;
    struct gui_vec2 in;
    struct gui_vec2 out;
};

struct node_editor {
    struct node node_buf[32];
    struct node_link links[64];
    struct node *begin;
    struct node *end;
    int node_count;
    int link_count;
    gui_state menu;
    struct gui_rect bounds;
    struct node *selected;
};

struct gui {
    void *memory;
    struct gui_window panel;
    struct gui_input input;
    struct gui_command_queue queue;
    struct gui_style config;
    struct gui_user_font font;
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
node_editor_add(struct node_editor *editor, const char *name, struct gui_rect bounds,
    struct gui_color col, int in_count, int out_count)
{
    static int IDs = 0;
    struct node *node;
    assert((gui_size)editor->node_count < LEN(editor->node_buf));
    node = &editor->node_buf[editor->node_count++];
    node->ID = IDs++;
    node->value = 0;
    node->color = gui_rgb(255, 0, 0);
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
    assert((gui_size)editor->link_count < LEN(editor->links));
    link = &editor->links[editor->link_count++];
    link->input_id = in_id;
    link->input_slot = in_slot;
    link->output_id = out_id;
    link->output_slot = out_slot;
}

static void
node_editor_draw(struct gui_window *win, struct node_editor *nodedit,
    struct gui_style *config)
{
    struct node *updated = 0;
    struct gui_context layout;
    gui_begin(&layout, win);
    {
        int i = 0, n = 0;
        gui_header(&layout, "Node Editor", GUI_CLOSEABLE, 0, GUI_HEADER_LEFT);
        gui_layout_row_space_begin(&layout, GUI_STATIC, 500, (gui_size)nodedit->node_count);
        {
            /* execute each node */
            struct gui_context node;
            struct node *it = nodedit->begin;
            const struct gui_input *in = gui_input(&layout);
            struct gui_command_buffer *canvas = gui_canvas(&layout);
            gui_style_push_color(config, GUI_COLOR_WINDOW, gui_rgb(52, 52, 52));
            while (it) {
                /* draw node */
                gui_layout_row_space_push(&layout, it->bounds);
                gui_group_begin(&layout, &node, it->name,
                    GUI_WINDOW_MOVEABLE, gui_vec2(0,0));
                {
                    struct gui_color color;
                    const char *str[] = {"R:", "G:", "B:", "A:"};
                    gui_float ratio[] = {0.25f, 0.75f};
                    gui_byte *iter = &it->color.r;

                    /* always have last selected node on top */
                    if (gui_input_mouse_clicked(in, GUI_BUTTON_LEFT, node.bounds) &&
                        (!(it->prev && gui_input_mouse_clicked(in, GUI_BUTTON_LEFT,
                        gui_layout_row_space_rect_to_screen(&layout, node.bounds)))) &&
                        nodedit->end != it)
                    {
                        updated = it;
                    }

                    /* color */
                    gui_layout_row_dynamic(&node, 30, 1);
                    color = it->color;
                    gui_button_color(&node, color, GUI_BUTTON_DEFAULT);

                    /* color picker */
                    gui_layout_row(&node, GUI_DYNAMIC, 30, 2, ratio);
                    for (n = 0; n < 3; ++n) {
                        gui_float t = *iter;
                        gui_label(&node, str[n], GUI_TEXT_LEFT);
                        t = gui_slider(&node, 0, t, 255, 10);
                        *iter = (gui_byte)t;
                        iter++;
                    }
                }
                gui_group_end(&layout, &node);

                {
                    /* draw node connector */
                    gui_float space;
                    struct gui_rect bounds;
                    bounds = gui_layout_row_space_rect_to_local(&layout, node.bounds);
                    it->bounds = bounds;
                    space = node.bounds.h / ((it->input_count) + 1);

                    /* output connector */
                    for (n = 0; n < it->output_count; ++n) {
                        gui_command_buffer_push_circle(canvas,
                            gui_rect((node.bounds.x + node.bounds.w)-4,
                            node.bounds.y + space * (n+1), 8, 8),
                            gui_rgb(100, 100, 100));
                    }

                    /* input connector */
                    for (n = 0; n < it->input_count; ++n) {
                        gui_command_buffer_push_circle(canvas,
                            gui_rect(node.bounds.x-4, node.bounds.y + space * (n+1), 8, 8),
                            gui_rgb(100, 100, 100));
                    }
                }
                it = it->next;
            }

            /* draw each link */
            for (n = 0; n < nodedit->link_count; ++n) {
                struct node_link *link = &nodedit->links[n];
                struct node *ni = node_editor_find(nodedit, link->input_id);
                struct node *no = node_editor_find(nodedit, link->output_id);
                gui_float spacei = node.bounds.h / ((ni->input_count) + 1);
                gui_float spaceo = node.bounds.h / ((no->input_count) + 1);
                struct gui_vec2 l0 = gui_layout_row_space_to_screen(&layout,
                    gui_vec2(ni->bounds.x + ni->bounds.w, 3+ni->bounds.y + spacei * (link->input_slot+1)));
                struct gui_vec2 l1 = gui_layout_row_space_to_screen(&layout,
                    gui_vec2(no->bounds.x, 3 + no->bounds.y + spaceo * (link->output_slot+1)));
                gui_command_buffer_push_curve(canvas, l0.x, l0.y, l0.x + 50.0f, l0.y,
                    l1.x - 50.0f, l1.y, l1.x, l1.y, gui_rgb(100, 100, 100));
            }
            gui_style_pop_color(config);
            if (updated) {
                node_editor_pop(nodedit, updated);
                node_editor_push(nodedit, updated);
            }

            {
                /* right click popup context menu activation */
                if (nodedit->menu == GUI_INACTIVE &&
                    gui_input_mouse_clicked(in, GUI_BUTTON_RIGHT,
                        gui_layout_row_space_bounds(&layout))) {
                    it = nodedit->begin;
                    nodedit->selected = NULL;
                    nodedit->menu = GUI_ACTIVE;
                    nodedit->bounds = gui_rect(in->mouse.pos.x, in->mouse.pos.y, 100, 200);
                    while (it) {
                        if (gui_input_is_mouse_hovering_rect(in,
                            gui_layout_row_space_rect_to_screen(&layout, it->bounds)))
                            nodedit->selected = it;
                        it = it->next;
                    }
                }

                /* popup context menu */
                if (nodedit->menu == GUI_ACTIVE) {
                    struct gui_context menu;
                    gui_popup_nonblock_begin(&layout, &menu, GUI_WINDOW_NO_SCROLLBAR, &nodedit->menu,
                        nodedit->bounds);
                    gui_layout_row_dynamic(&menu, 25, 1);
                    if (!nodedit->selected) {
                        if (gui_button_fitting(&menu, "New", GUI_TEXT_CENTERED, GUI_BUTTON_DEFAULT)) {
                            fprintf(stdout, "pressed new!\n");
                            nodedit->menu = gui_popup_nonblock_close(&menu);
                            node_editor_add(nodedit, "New", gui_rect(400, 260, 180, 220),
                                    gui_rgb(255, 255, 255), 2, 2);
                        }
                    } else {
                        if (gui_button_fitting(&menu, "Delete", GUI_TEXT_CENTERED, GUI_BUTTON_DEFAULT)) {
                            fprintf(stdout, "pressed delete!\n");
                            nodedit->menu = gui_popup_nonblock_close(&menu);
                        }
                        if (gui_button_fitting(&menu, "Rename", GUI_TEXT_CENTERED, GUI_BUTTON_DEFAULT)) {
                            fprintf(stdout, "pressed rename!\n");
                            nodedit->menu = gui_popup_nonblock_close(&menu);
                        }
                        if (gui_button_fitting(&menu, "Copy", GUI_TEXT_CENTERED, GUI_BUTTON_DEFAULT)) {
                            fprintf(stdout, "pressed copy!\n");
                            nodedit->menu = gui_popup_nonblock_close(&menu);
                        }
                    }
                    gui_popup_nonblock_end(&layout, &menu);
                }
            }
        }
        gui_layout_row_space_end(&layout);
    }
    gui_end(&layout, win);
}

static void
node_editor_init(struct node_editor *editor)
{
    memset(editor, 0, sizeof(*editor));
    editor->begin = NULL;
    editor->end = NULL;
    node_editor_add(editor, "MainTex", gui_rect(40, 10, 180, 220), gui_rgb(255, 0, 0), 1, 1);
    node_editor_add(editor, "BumpMap", gui_rect(40, 260, 180, 220), gui_rgb(0, 255, 0), 1, 1);
    node_editor_add(editor, "Combine", gui_rect(400, 100, 180, 220), gui_rgb(0,0,255), 2, 2);
    node_editor_link(editor, 0, 0, 2, 0);
    node_editor_link(editor, 1, 0, 2, 1);
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

static gui_size
font_get_width(gui_handle handle, const gui_char *text, gui_size len)
{
    gui_size width;
    float bounds[4];
    NVGcontext *ctx = (NVGcontext*)handle.ptr;
    nvgTextBounds(ctx, 0, 0, text, &text[len], bounds);
    width = (gui_size)(bounds[2] - bounds[0]);
    return width;
}

static void
draw(NVGcontext *nvg, struct gui_command_queue *queue, int width, int height)
{
    const struct gui_command *cmd;
    glPushAttrib(GL_ENABLE_BIT|GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_TEXTURE_2D);

    nvgBeginFrame(nvg, width, height, ((float)width/(float)height));
    gui_foreach_command(cmd, queue) {
        switch (cmd->type) {
        case GUI_COMMAND_NOP: break;
        case GUI_COMMAND_SCISSOR: {
            const struct gui_command_scissor *s = gui_command(scissor, cmd);
            nvgScissor(nvg, s->x, s->y, s->w, s->h);
        } break;
        case GUI_COMMAND_LINE: {
            const struct gui_command_line *l = gui_command(line, cmd);
            nvgBeginPath(nvg);
            nvgMoveTo(nvg, l->begin.x, l->begin.y);
            nvgLineTo(nvg, l->end.x, l->end.y);
            nvgFillColor(nvg, nvgRGBA(l->color.r, l->color.g, l->color.b, l->color.a));
            nvgFill(nvg);
        } break;
        case GUI_COMMAND_CURVE: {
            const struct gui_command_curve *q = gui_command(curve, cmd);
            nvgBeginPath(nvg);
            nvgMoveTo(nvg, q->begin.x, q->begin.y);
            nvgBezierTo(nvg, q->ctrl[0].x, q->ctrl[0].y, q->ctrl[1].x,
                q->ctrl[1].y, q->end.x, q->end.y);
            nvgStrokeColor(nvg, nvgRGBA(q->color.r, q->color.g, q->color.b, q->color.a));
            nvgStroke(nvg);
        } break;
        case GUI_COMMAND_RECT: {
            const struct gui_command_rect *r = gui_command(rect, cmd);
            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, r->x, r->y, r->w, r->h, r->rounding);
            nvgFillColor(nvg, nvgRGBA(r->color.r, r->color.g, r->color.b, r->color.a));
            nvgFill(nvg);
        } break;
        case GUI_COMMAND_CIRCLE: {
            const struct gui_command_circle *c = gui_command(circle, cmd);
            nvgBeginPath(nvg);
            nvgCircle(nvg, c->x + (c->w/2.0f), c->y + c->w/2.0f, c->w/2.0f);
            nvgFillColor(nvg, nvgRGBA(c->color.r, c->color.g, c->color.b, c->color.a));
            nvgFill(nvg);
        } break;
        case GUI_COMMAND_TRIANGLE: {
            const struct gui_command_triangle *t = gui_command(triangle, cmd);
            nvgBeginPath(nvg);
            nvgMoveTo(nvg, t->a.x, t->a.y);
            nvgLineTo(nvg, t->b.x, t->b.y);
            nvgLineTo(nvg, t->c.x, t->c.y);
            nvgLineTo(nvg, t->a.x, t->a.y);
            nvgFillColor(nvg, nvgRGBA(t->color.r, t->color.g, t->color.b, t->color.a));
            nvgFill(nvg);
        } break;
        case GUI_COMMAND_TEXT: {
            const struct gui_command_text *t = gui_command(text, cmd);
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
        case GUI_COMMAND_IMAGE: {
            const struct gui_command_image *i = gui_command(image, cmd);
            NVGpaint imgpaint;
            imgpaint = nvgImagePattern(nvg, i->x, i->y, i->w, i->h, 0, i->img.handle.id, 1.0f);
            nvgBeginPath(nvg);
            nvgRoundedRect(nvg, i->x, i->y, i->w, i->h, 0);
            nvgFillPaint(nvg, imgpaint);
            nvgFill(nvg);
        } break;
        case GUI_COMMAND_MAX:
        default: break;
        }
    }
    gui_command_queue_clear(queue);

    nvgResetScissor(nvg);
    nvgEndFrame(nvg);
    glPopAttrib();
}

static void
key(struct gui_input *in, SDL_Event *evt, gui_bool down)
{
    const Uint8* state = SDL_GetKeyboardState(NULL);
    SDL_Keycode sym = evt->key.keysym.sym;
    if (sym == SDLK_RSHIFT || sym == SDLK_LSHIFT)
        gui_input_key(in, GUI_KEY_SHIFT, down);
    else if (sym == SDLK_DELETE)
        gui_input_key(in, GUI_KEY_DEL, down);
    else if (sym == SDLK_RETURN)
        gui_input_key(in, GUI_KEY_ENTER, down);
    else if (sym == SDLK_SPACE)
        gui_input_key(in, GUI_KEY_SPACE, down);
    else if (sym == SDLK_BACKSPACE)
        gui_input_key(in, GUI_KEY_BACKSPACE, down);
    else if (sym == SDLK_LEFT)
        gui_input_key(in, GUI_KEY_LEFT, down);
    else if (sym == SDLK_RIGHT)
        gui_input_key(in, GUI_KEY_RIGHT, down);
    else if (sym == SDLK_c)
        gui_input_key(in, GUI_KEY_COPY, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_v)
        gui_input_key(in, GUI_KEY_PASTE, down && state[SDL_SCANCODE_LCTRL]);
    else if (sym == SDLK_x)
        gui_input_key(in, GUI_KEY_CUT, down && state[SDL_SCANCODE_LCTRL]);
}

static void
motion(struct gui_input *in, SDL_Event *evt)
{
    const gui_int x = evt->motion.x;
    const gui_int y = evt->motion.y;
    gui_input_motion(in, x, y);
}

static void
btn(struct gui_input *in, SDL_Event *evt, gui_bool down)
{
    const gui_int x = evt->button.x;
    const gui_int y = evt->button.y;
    if (evt->button.button == SDL_BUTTON_LEFT)
        gui_input_button(in, GUI_BUTTON_LEFT, x, y, down);
    else if (evt->button.button == SDL_BUTTON_RIGHT)
        gui_input_button(in, GUI_BUTTON_RIGHT, x, y, down);
}

static void
text(struct gui_input *in, SDL_Event *evt)
{
    gui_glyph glyph;
    memcpy(glyph, evt->text.text, GUI_UTF_SIZE);
    gui_input_glyph(in, glyph);
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
    int width, height;
    const char *font_path;
    gui_size font_height;
    SDL_Window *win;
    SDL_GLContext glContext;
    NVGcontext *vg = NULL;
    unsigned int started;
    unsigned int dt;
    gui_bool running = gui_true;

    /* GUI */
    struct node_editor nodedit;
    struct gui gui;

    if (argc < 2) {
        fprintf(stdout,"Missing TTF Font file argument: gui <path>\n");
        exit(EXIT_FAILURE);
    }
    font_path = argv[1];
    font_height = 10;

    /* SDL */
    SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER|SDL_INIT_EVENTS);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    win = SDL_CreateWindow("Demo",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_OPENGL|SDL_WINDOW_SHOWN);
    glContext = SDL_GL_CreateContext(win);
    SDL_GetWindowSize(win, &width, &height);

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
    gui_command_queue_init_fixed(&gui.queue, gui.memory, MAX_MEMORY);

    gui.font.userdata.ptr = vg;
    nvgTextMetrics(vg, NULL, NULL, &gui.font.height);
    gui.font.width = font_get_width;
    gui_style_default(&gui.config, GUI_DEFAULT_ALL, &gui.font);

    gui_window_init(&gui.panel, gui_rect(50, 50, 700, 600),
        GUI_WINDOW_BORDER|GUI_WINDOW_MOVEABLE,
        &gui.queue, &gui.config, &gui.input);
    node_editor_init(&nodedit);

    while (running) {
        /* Input */
        SDL_Event evt;
        started = SDL_GetTicks();
        gui_input_begin(&gui.input);
        while (SDL_PollEvent(&evt)) {
            if (evt.type == SDL_WINDOWEVENT) resize(&evt);
            else if (evt.type == SDL_QUIT) goto cleanup;
            else if (evt.type == SDL_KEYUP) key(&gui.input, &evt, gui_false);
            else if (evt.type == SDL_KEYDOWN) key(&gui.input, &evt, gui_true);
            else if (evt.type == SDL_MOUSEBUTTONDOWN) btn(&gui.input, &evt, gui_true);
            else if (evt.type == SDL_MOUSEBUTTONUP) btn(&gui.input, &evt, gui_false);
            else if (evt.type == SDL_MOUSEMOTION) motion(&gui.input, &evt);
            else if (evt.type == SDL_TEXTINPUT) text(&gui.input, &evt);
            else if (evt.type == SDL_MOUSEWHEEL) gui_input_scroll(&gui.input, evt.wheel.y);
        }
        gui_input_end(&gui.input);

        /* GUI */
        glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        SDL_GetWindowSize(win, &width, &height);
        node_editor_draw(&gui.panel, &nodedit, &gui.config);
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

