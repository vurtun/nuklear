/*
    Copyright (c) 2015
    vurtun <polygone@gmx.net>
    MIT licence
*/
#ifndef GUI_H_
#define GUI_H_
/*
 *  ------------- TODO-List ------------
 * - Input cursor is fucked!!!
 * - panel
 *      o Group (1)
 *      o Moveable
 *      o Scaleable
 *      o Tabs (2)
 *      o Icon
 *      o combobox
 *      o listView  (3)
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
enum gui_direction {GUI_UP, GUI_RIGHT, GUI_DOWN, GUI_LEFT};
struct gui_color {gui_byte r,g,b,a;};
struct gui_colorf {gui_float r,g,b,a;};
struct gui_texCoord {gui_float u,v;};
struct gui_vec2 {gui_float x,y;};
struct gui_rect {gui_float x,y,w,h;};
struct gui_key {gui_int down, clicked;};

struct gui_memory {
    void *memory;
    gui_size vertex_size;
    gui_size command_size;
    gui_size clip_size;
};

struct gui_memory_status {
    gui_size vertexes_allocated;
    gui_size vertexes_needed;
    gui_size commands_allocated;
    gui_size commands_needed;
    gui_size clips_allocated;
    gui_size clips_needed;
    gui_size allocated;
    gui_size needed;
};

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
    struct gui_vertex *vertexes;
    gui_size vertex_capacity;
    gui_size vertex_size;
    gui_size vertex_needed;
    struct gui_draw_command *commands;
    gui_size command_capacity;
    gui_size command_size;
    gui_size command_needed;
    struct gui_rect *clips;
    gui_size clip_capacity;
    gui_size clip_size;
    gui_size clip_needed;
};

struct gui_draw_call_list {
    void *memory;
    struct gui_vertex *vertexes;
    gui_size vertex_size;
    struct gui_draw_command *commands;
    gui_size command_size;
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

struct gui_text {
    gui_float x, y;
    gui_float w, h;
    gui_float pad_x, pad_y;
    const char *text;
    gui_size length;
    struct gui_color font;
    struct gui_color background;
};

struct gui_image {
    gui_float x, y;
    gui_float w, h;
    gui_float pad_x, pad_y;
    struct gui_texCoord uv[2];
    gui_texture texture;
    struct gui_color background;
};

enum gui_button_behavior {
    GUI_BUTTON_SWITCH,
    GUI_BUTTON_REPEATER
};

struct gui_button {
    gui_float x, y;
    gui_float w, h;
    gui_float pad_x, pad_y;
    enum gui_button_behavior behavior;
    struct gui_color background;
    struct gui_color foreground;
    struct gui_color highlight;
    struct gui_color font;
};

struct gui_toggle {
    gui_float x, y;
    gui_float w, h;
    gui_float pad_x, pad_y;
    gui_int active;
    gui_size length;
    const gui_char *text;
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
    gui_float offset;
    gui_float target;
    gui_float step;
    struct gui_color background;
    struct gui_color foreground;
    struct gui_color border;
};

enum gui_input_filter {
    GUI_INPUT_DEFAULT,
    GUI_INPUT_FLOAT,
    GUI_INPUT_DEC,
    GUI_INPUT_HEX,
    GUI_INPUT_OCT,
    GUI_INPUT_BIN,
};

struct gui_input_field {
    gui_float x, y;
    gui_float w, h;
    gui_float pad_x, pad_y;
    gui_size max;
    gui_bool active;
    gui_bool show_cursor;
    enum gui_input_filter filter;
    gui_bool password;
    struct gui_color background;
    struct gui_color foreground;
    struct gui_color font;
};

struct gui_spinner {
    gui_float x, y;
    gui_float w, h;
    gui_float pad_x, pad_y;
    gui_bool active;
    gui_int step;
    gui_int max;
    gui_int min;
    struct gui_color background;
    struct gui_color foreground;
    struct gui_color button;
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
    struct gui_color highlight;
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
    GUI_COLOR_SPINNER,
    GUI_COLOR_SPINNER_BORDER,
    GUI_COLOR_SPINNER_BUTTON,
    GUI_COLOR_HISTO,
    GUI_COLOR_HISTO_BARS,
    GUI_COLOR_HISTO_NEGATIVE,
    GUI_COLOR_HISTO_HIGHLIGHT,
    GUI_COLOR_PLOT,
    GUI_COLOR_PLOT_LINES,
    GUI_COLOR_PLOT_HIGHLIGHT,
    GUI_COLOR_SCROLLBAR,
    GUI_COLOR_SCROLLBAR_CURSOR,
    GUI_COLOR_SCROLLBAR_BORDER,
    GUI_COLOR_COUNT
};

struct gui_config {
    gui_float global_alpha;
    struct gui_vec2 panel_padding;
    struct gui_vec2 panel_min_size;
    struct gui_vec2 item_spacing;
    struct gui_vec2 item_padding;
    gui_float scrollbar_width;
    gui_float scroll_factor;
    struct gui_color colors[GUI_COLOR_COUNT];
};

enum gui_panel_flags {
    GUI_PANEL_TAB = 0x01,
    GUI_PANEL_HEADER = 0x02,
    GUI_PANEL_BORDER = 0x04,
    GUI_PANEL_MINIMIZABLE = 0x08,
    GUI_PANEL_CLOSEABLE = 0x10,
    GUI_PANEL_SCROLLBAR = 0x20
};

struct gui_panel {
    gui_flags flags;
    gui_float x, y;
    gui_float at_y;
    gui_float width, height;
    gui_size index;
    gui_float header_height;
    gui_float row_height;
    gui_size row_columns;
    gui_float offset;
    gui_int minimized;
    struct gui_draw_buffer *out;
    const struct gui_font *font;
    const struct gui_input *in;
    const struct gui_config *config;
};

/* Input */
void gui_input_begin(struct gui_input *in);
void gui_input_motion(struct gui_input *in, gui_int x, gui_int y);
void gui_input_key(struct gui_input *in, enum gui_keys key, gui_int down);
void gui_input_button(struct gui_input *in, gui_int x, gui_int y, gui_int down);
void gui_input_char(struct gui_input *in, gui_glyph glyph);
void gui_input_end(struct gui_input *in);

/* Output */
void gui_begin(struct gui_draw_buffer *buf, const struct gui_memory *memory);
void gui_end(struct gui_draw_buffer *buf, struct gui_draw_call_list *calls,
            struct gui_memory_status* status);

/* Widgets */
void gui_text(struct gui_draw_buffer *buf, const struct gui_text *text,
                    const struct gui_font *font);
gui_size gui_text_wrap(struct gui_draw_buffer *buf, const struct gui_text *text,
                    const struct gui_font *font);
void gui_image(struct gui_draw_buffer *buf, const struct gui_image *image);
gui_int gui_button_text(struct gui_draw_buffer *buf, const struct gui_button *button,
                    const char *text, gui_size len, const struct gui_font *font,
                    const struct gui_input *in);
gui_int gui_button_triangle(struct gui_draw_buffer *buffer, struct gui_button* button,
                    enum gui_direction direction, const struct gui_input *in);
gui_int gui_button_image(struct gui_draw_buffer *buffer, struct gui_button* button,
                    gui_texture tex, struct gui_texCoord from, struct gui_texCoord to,
                    const struct gui_input *in);
gui_int gui_toggle(struct gui_draw_buffer *buf, const struct gui_toggle *toggle,
                    const struct gui_font *font, const struct gui_input *in);
gui_float gui_slider(struct gui_draw_buffer *buf, const struct gui_slider *slider,
                    const struct gui_input *in);
gui_size gui_progress(struct gui_draw_buffer *buf, const struct gui_progress *prog,
                    const struct gui_input *in);
gui_int gui_input(struct gui_draw_buffer *buf,  gui_char *buffer, gui_size *length,
                    const struct gui_input_field *input,
                    const struct gui_font *font, const struct gui_input *in);
gui_size gui_command(struct gui_draw_buffer *buf,  gui_char *buffer, gui_size *length,
                    gui_int *active, const struct gui_input_field *input,
                    const struct gui_font *font, const struct gui_input *in);
gui_int gui_spinner(struct gui_draw_buffer *buf, gui_int *value,
                    const struct gui_spinner *spinner, const struct gui_font *font,
                    const struct gui_input *in);
gui_float gui_scroll(struct gui_draw_buffer *buf, const struct gui_scroll *scroll,
                    const struct gui_input *in);
gui_int gui_histo(struct gui_draw_buffer *buf, const struct gui_histo *histo,
                    const struct gui_input *in);
gui_int gui_plot(struct gui_draw_buffer *buf, const struct gui_plot *plot,
                    const struct gui_input *in);

/* Panel */
void gui_default_config(struct gui_config *config);
void gui_panel_init(struct gui_panel *panel, const struct gui_config *config,
                        const struct gui_font *font, const struct gui_input *input);
gui_int gui_panel_begin(struct gui_panel *panel, struct gui_draw_buffer *q,
                        const char *t, gui_flags f,
                        gui_float x, gui_float y, gui_float w, gui_float h);
void gui_panel_layout(struct gui_panel *panel, gui_float height, gui_size cols);
void gui_panel_seperator(struct gui_panel *panel, gui_size cols);
void gui_panel_text(struct gui_panel *panel, const char *str, gui_size len);
gui_int gui_panel_button_text(struct gui_panel *panel, const char *str, gui_size len,
                        enum gui_button_behavior behavior);
gui_int gui_panel_button_triangle(struct gui_panel *panel, enum gui_direction d,
                        enum gui_button_behavior behavior);
gui_int gui_panel_button_image(struct gui_panel *panel, gui_texture tex,
                        struct gui_texCoord from, struct gui_texCoord to,
                        enum gui_button_behavior behavior);
gui_int gui_panel_toggle(struct gui_panel *panel, const char *str, gui_size len,
                        gui_int active);
gui_float gui_panel_slider(struct gui_panel *panel, gui_float min, gui_float v,
                        gui_float max, gui_float step);
gui_size gui_panel_progress(struct gui_panel *panel, gui_size cur, gui_size max,
                        gui_bool modifyable);
gui_int gui_panel_input(struct gui_panel *panel, gui_char *buffer, gui_size *len,
                        gui_size max, enum gui_input_filter filter,  gui_bool active);
gui_size gui_panel_command(struct gui_panel *panel,  gui_char *buffer, gui_size *length,
                        gui_size max, gui_int *active);
gui_int gui_panel_spinner(struct gui_panel *panel, gui_int min, gui_int *value,
                        gui_int max, gui_int step, gui_int active);
gui_int gui_panel_plot(struct gui_panel *panel, const gui_float *values,
                        gui_size value_count);
gui_int gui_panel_histo(struct gui_panel *panel, const gui_float *values,
                        gui_size value_count);
void gui_panel_group_begin(struct gui_panel *panel, const char *t, struct gui_panel *tab);
void gui_panel_group_end(struct gui_panel *tab);
void gui_panel_end(struct gui_panel *panel);

#endif
