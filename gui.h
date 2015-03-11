/*
    Copyright (c) 2015
    vurtun <polygone@gmx.net>
    MIT licence
*/
#ifndef GUI_H_
#define GUI_H_

/*
 *  ------------- TODO-List ------------
 * - fix input bug
 * - plot input handling
 * - fine tune progressbar input handling
 * - fine tune slider input handling
 * - make additional widgets hoverable
 * - panel clip rects
 * - panel
 *      o flags
 *      o widgets
 *      o combobox
 *      o listView
 *      o treeView
 *      o textBox
 * ---------------------------------------
 */
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
struct gui_key {gui_int down, clicked;};

struct gui_vertex {
    struct gui_vec2 pos;
    struct gui_texCoord uv;
    struct gui_color color;
};

struct gui_draw_command {
    gui_size vertex_count;
    struct gui_rect clip_rect;
    gui_texture texture;
};

struct gui_draw_buffer {
    gui_byte *vertexes;
    gui_byte *begin;
    gui_byte *end;
    gui_byte *memory;
    gui_size size;
    gui_size vertex_write;
    gui_size vertex_count;
    gui_size command_count;
    gui_size allocated;
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
    struct gui_key keys[GUI_KEY_MAX];
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
    gui_float xadvance;
    gui_short width, height;
    gui_float xoff, yoff;
    struct gui_texCoord uv[2];
};

struct gui_font {
    gui_float height;
    gui_float scale;
    gui_texture texture;
    struct gui_vec2 tex_size;
    struct gui_font_glyph *glyphes;
    gui_long glyph_count;
    const struct gui_font_glyph *fallback;
};

struct gui_button {
    gui_float x, y;
    gui_float w, h;
    gui_float pad_x, pad_y;
    const char *text;
    gui_size length;
    struct gui_color font;
    struct gui_color background;
    struct gui_color foreground;
    struct gui_color highlight;
};

struct gui_toggle {
    gui_float x, y;
    gui_float w, h;
    gui_float pad_x, pad_y;
    gui_int active;
    gui_size length;
    const char *text;
    struct gui_color font;
    struct gui_color background;
    struct gui_color foreground;
};

struct gui_slider {
    gui_float x, y;
    gui_float w, h;
    gui_float pad_x, pad_y;
    gui_float min, max;
    gui_float value;
    gui_float step;
    struct gui_color background;
    struct gui_color foreground;
};

struct gui_progress {
    gui_float x, y;
    gui_float w, h;
    gui_float pad_x, pad_y;
    gui_size current;
    gui_size max;
    gui_bool modifyable;
    struct gui_color background;
    struct gui_color foreground;
};

struct gui_scroll {
    gui_float x, y;
    gui_float w, h;
    gui_size offset;
    gui_size target;
    gui_size step;
    struct gui_color background;
    struct gui_color foreground;
};

struct gui_input_field {
    gui_float x, y;
    gui_float w, h;
    gui_float pad_x, pad_y;
    gui_char *buffer;
    gui_size *length;
    gui_size max;
    gui_bool active;
    struct gui_color background;
    struct gui_color foreground;
    struct gui_color font;
};

struct gui_plot {
    gui_float x, y;
    gui_float w, h;
    gui_float pad_x;
    gui_float pad_y;
    gui_size value_count;
    const gui_float *values;
    struct gui_color background;
    struct gui_color foreground;
};

struct gui_histo {
    gui_float x, y;
    gui_float w, h;
    gui_float pad_x, pad_y;
    gui_size value_count;
    const gui_float *values;
    struct gui_color background;
    struct gui_color foreground;
    struct gui_color negative;
    struct gui_color highlight;
};

enum gui_colors {
    GUI_COLOR_TEXT,
    GUI_COLOR_PANEL,
    GUI_COLOR_BORDER,
    GUI_COLOR_TITLEBAR,
    GUI_COLOR_BUTTON,
    GUI_COLOR_BUTTON_HOVER,
    GUI_COLOR_BUTTON_BORDER,
    GUI_COLOR_TOGGLE,
    GUI_COLOR_TOGGLE_ACTIVE,
    GUI_COLOR_SCROLL,
    GUI_COLOR_SCROLL_CURSOR,
    GUI_COLOR_SLIDER,
    GUI_COLOR_SLIDER_CURSOR,
    GUI_COLOR_PROGRESS,
    GUI_COLOR_PROGRESS_CURSOR,
    GUI_COLOR_INPUT,
    GUI_COLOR_INPUT_BORDER,
    GUI_COLOR_HISTO,
    GUI_COLOR_HISTO_BARS,
    GUI_COLOR_HISTO_NEGATIVE,
    GUI_COLOR_HISTO_HIGHLIGHT,
    GUI_COLOR_PLOT,
    GUI_COLOR_PLOT_LINES,
    GUI_COLOR_COUNT
};

struct gui_config {
    gui_float global_alpha;
    gui_float header_height;
    struct gui_vec2 panel_padding;
    struct gui_vec2 panel_min_size;
    struct gui_vec2 item_spacing;
    struct gui_vec2 item_padding;
    gui_int scrollbar_width;
    gui_int scroll_factor;
    struct gui_color colors[GUI_COLOR_COUNT];
};

enum gui_panel_flags {
    GUI_PANEL_BORDER = 0x01,
    GUI_PANEL_TITLEBAR = 0x02,
    GUI_PANEL_MINIMIZABLE = 0x04,
    GUI_PANEL_CLOSEABLE = 0x08,
    GUI_PANEL_SCALEABLE = 0x10,
    GUI_PANEL_SCROLLBAR = 0x20
};

struct gui_panel {
    gui_flags flags;
    gui_float x, y;
    gui_float at_y;
    gui_float width, height;
    gui_size index;
    gui_float row_height;
    gui_size row_columns;
    struct gui_draw_buffer *out;
    const struct gui_font *font;
    const struct gui_input *in;
    const struct gui_config *config;
};

/* Utility */
struct gui_color gui_make_color(gui_byte r, gui_byte g, gui_byte b, gui_byte a);
struct gui_vec2 gui_make_vec2(gui_float x, gui_float y);
void gui_default_config(struct gui_config *config);

/* Input */
void gui_input_begin(struct gui_input *in);
void gui_input_motion(struct gui_input *in, gui_int x, gui_int y);
void gui_input_key(struct gui_input *in, enum gui_keys key, gui_int down);
void gui_input_button(struct gui_input *in, gui_int x, gui_int y, gui_int down);
void gui_input_char(struct gui_input *in, gui_glyph glyph);
void gui_input_end(struct gui_input *in);

/* Output */
void gui_begin(struct gui_draw_buffer *buf, gui_byte *memory, gui_size size);
gui_size gui_end(struct gui_draw_buffer *buf);
gui_int gui_get_command(struct gui_draw_command *cmd,
                        const struct gui_draw_buffer *buf, gui_size index);

/* Widgets */
gui_int gui_button(struct gui_draw_buffer *buf, const struct gui_button *button,
                    const struct gui_font *font, const struct gui_input *in);
gui_int gui_toggle(struct gui_draw_buffer *buf, const struct gui_toggle *toggle,
                    const struct gui_font *font, const struct gui_input *in);
gui_float gui_slider(struct gui_draw_buffer *buf, const struct gui_slider *slider,
                    const struct gui_input *in);
gui_size gui_progress(struct gui_draw_buffer *buf, const struct gui_progress *prog,
                    const struct gui_input *in);
gui_size gui_scroll(struct gui_draw_buffer *buf, const struct gui_scroll *scroll,
                    const struct gui_input *in);
gui_int gui_input(struct gui_draw_buffer *buf, const struct gui_input_field *f,
                    const struct gui_font *font, const struct gui_input *in);
gui_int gui_histo(struct gui_draw_buffer *buf, const struct gui_histo *histo,
                    const struct gui_input *in);
void gui_plot(struct gui_draw_buffer *buf, const struct gui_plot *plot);

/* Panel */
void gui_panel_init(struct gui_panel *panel, const struct gui_config *config,
                    const struct gui_font *font, const struct gui_input *input);
gui_int gui_panel_begin(struct gui_panel *panel, struct gui_draw_buffer *q,
                        const char *t, gui_flags f,
                        gui_float x, gui_float y, gui_float w, gui_float h);
void gui_panel_row(struct gui_panel *panel, gui_float height, gui_size cols);
void gui_panel_space(struct gui_panel *panel, gui_int cols);
gui_int gui_panel_button(struct gui_panel *panel, const char *str, gui_size len);
gui_int gui_panel_toggle(struct gui_panel *panel, const char *str, gui_size len,
                        gui_int active);
gui_float gui_panel_slider(struct gui_panel *panel, gui_float min, gui_float v,
                        gui_float max, gui_float step);
gui_size gui_panel_progress(struct gui_panel *panel, gui_size cur, gui_size max,
                        gui_bool modifyable);
gui_int gui_panel_input(struct gui_panel *panel, gui_char *buffer, gui_size *len,
                        gui_size max, gui_bool active);
void gui_panel_plot(struct gui_panel *panel, const gui_float *values,
                        gui_size value_count);
gui_int gui_panel_histo(struct gui_panel *panel, const gui_float *values,
                        gui_size value_count);
void gui_panel_end(struct gui_panel *panel);

#endif
