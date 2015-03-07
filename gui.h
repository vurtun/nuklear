/*
    Copyright (c) 2015
    vurtun <polygone@gmx.net>
    MIT licence
*/
#ifndef TINY_GUI_H_
#define TINY_GUI_H_

#define GUI_UTF_SIZE 4
#define GUI_INPUT_MAX 16

typedef int gui_int;
typedef short gui_short;
typedef long gui_long;
typedef int gui_bool;
typedef unsigned int gui_flags;
typedef unsigned char gui_char;
typedef float gui_float;
typedef void* gui_texture;
typedef unsigned char gui_byte;
typedef unsigned int gui_flag;
typedef unsigned long gui_size;
typedef gui_char gui_glyph[GUI_UTF_SIZE];

enum {gui_false, gui_true};
struct gui_color {gui_byte r,g,b,a;};
struct gui_texCoord {gui_float u,v;};
struct gui_vec2 {gui_float x,y;};
struct gui_rect {gui_float x,y,w,h;};

struct gui_vertex {
    struct gui_vec2 pos;
    struct gui_texCoord uv;
    struct gui_color color;
};

struct gui_draw_command {
    struct gui_vertex *vertexes;
    gui_size vertex_count;
    gui_size vertex_write;
    struct gui_rect clip_rect;
    gui_texture texture;
};

struct gui_draw_queue {
    struct gui_draw_command *begin;
    struct gui_draw_command *end;
    gui_size vertex_count;
    gui_byte *memory;
    gui_size size;
    gui_size needed;
};

enum gui_keys {
    GUI_KEY_SHIFT,
    GUI_KEY_CTRL,
    GUI_KEY_DEL,
    GUI_KEY_ENTER,
    GUI_KEY_BACKSPACE,
    GUI_KEY_ESCAPE,
    GUI_KEY_SPACE,
    GUI_KEY_MAX
};

struct gui_input {
    gui_bool keys[GUI_KEY_MAX];
    gui_char text[GUI_INPUT_MAX];
    gui_size text_len;
    struct gui_vec2 mouse_pos;
    struct gui_vec2 mouse_prev;
    struct gui_vec2 mouse_delta;
    gui_bool mouse_down;
    gui_int mouse_clicked;
    struct gui_vec2 mouse_clicked_pos;
};

struct gui_font_glyph {
    gui_int code;
    gui_short xadvance;
    gui_short width, height;
    gui_float xoff, yoff;
    struct gui_texCoord uv[2];
};

struct gui_font {
    gui_short height;
    gui_float scale;
    gui_texture texture;
    struct gui_vec2 tex_size;
    struct gui_font_glyph *glyphes;
    gui_long glyph_count;
    const struct gui_font_glyph *fallback;
};

enum gui_panel_flags {
    GUI_PANEL_HEADER = 0x01,
    GUI_PANEL_MINIMIZABLE = 0x02,
    GUI_PANEL_CLOSEABLE = 0x04,
    GUI_PANEL_SCALEABLE = 0x08,
    GUI_PANEL_SCROLLBAR = 0x10
};

struct gui_panel {
    gui_flags flags;
    gui_int at_x, at_y;
};

void gui_input_begin(struct gui_input *in);
void gui_input_motion(struct gui_input *in, gui_int x, gui_int y);
void gui_input_key(struct gui_input *in, enum gui_keys key, gui_int down);
void gui_input_button(struct gui_input *in, gui_int x, gui_int y, gui_int down);
void gui_input_char(struct gui_input *in, gui_glyph glyph);
void gui_input_end(struct gui_input *in);

void gui_begin(struct gui_draw_queue *que, gui_byte *memory, gui_size size);
gui_size gui_end(struct gui_draw_queue *que);
const struct gui_draw_command *gui_next(const struct gui_draw_queue *que,
                                        const struct gui_draw_command*);

gui_int gui_button(struct gui_draw_queue *que, const struct gui_input *in,
                    const struct gui_font *font,
                    struct gui_color bg, struct gui_color fg,
                    gui_int x, gui_int y, gui_int w, gui_int h, gui_int pad,
                    const char *str, gui_int len);
gui_int gui_toggle(struct gui_draw_queue *que, const struct gui_input *in,
                    const struct gui_font *font,
                    struct gui_color bg, struct gui_color fg,
                    gui_int x, gui_int y, gui_int w, gui_int h, gui_int pad,
                    const char *str, gui_int len, gui_int active);
gui_int gui_slider(struct gui_draw_queue *que, const struct gui_input *in,
                    struct gui_color bg, struct gui_color fg,
                    gui_int x, gui_int y, gui_int w, gui_int h, gui_int pad,
                    gui_float min, gui_float v, gui_float max, gui_float step);
gui_int gui_progress(struct gui_draw_queue *que, const struct gui_input *in,
                    struct gui_color bg, struct gui_color fg,
                    gui_int x, gui_int y, gui_int w, gui_int h, gui_int pad,
                    gui_size cur, gui_size max, gui_bool modifyable);
gui_int gui_scroll(struct gui_draw_queue *que, const struct gui_input *in,
                    struct gui_color bg, struct gui_color fg,
                    gui_int x, gui_int y, gui_int w, gui_int h,
                    gui_int offset, gui_int dst);
gui_int gui_input(struct gui_draw_queue *que, const struct gui_input *in,
                    const struct gui_font *font,
                    struct gui_color bg, struct gui_color fg,
                    gui_int x, gui_int y, gui_int w, gui_int h,
                    gui_char *buffer, gui_int *len, gui_bool active);

#endif
