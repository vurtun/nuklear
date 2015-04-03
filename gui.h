/*
    Copyright (c) 2015
    vurtun <polygone@gmx.net>
    MIT licence
*/
#ifndef GUI_H_
#define GUI_H_

#define GUI_UTF_SIZE 4
#define GUI_INPUT_MAX 16

typedef int gui_int;
typedef unsigned int gui_uint;
typedef short gui_short;
typedef long gui_long;
typedef unsigned long gui_ulong;
typedef unsigned int gui_bool;
typedef unsigned int gui_flags;
typedef unsigned char gui_char;
typedef float gui_float;
typedef unsigned char gui_byte;
typedef unsigned int gui_flag;
typedef unsigned long gui_size;
typedef gui_char gui_glyph[GUI_UTF_SIZE];
typedef union {void* dx; gui_uint gl;} gui_texture;
typedef struct gui_panel gui_tab;
typedef struct gui_panel gui_group;
typedef struct gui_panel gui_shelf;

enum {gui_false, gui_true};
enum gui_heading {GUI_UP, GUI_RIGHT, GUI_DOWN, GUI_LEFT};
enum gui_direction {GUI_HORIZONTAL, GUI_VERTICAL};
struct gui_color {gui_byte r,g,b,a;};
struct gui_colorf {gui_float r,g,b,a;};
struct gui_texCoord {gui_float u,v;};
struct gui_vec2 {gui_float x,y;};
struct gui_rect {gui_float x,y,w,h;};
struct gui_key {gui_int down, clicked;};

struct gui_font_glyph {
    gui_ulong code;
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
    gui_long glyph_count;
    struct gui_font_glyph *glyphes;
    const struct gui_font_glyph *fallback;
};

struct gui_memory {
    void *memory;
    gui_size size;
    gui_size max_panels;
    gui_float vertex_percentage;
    gui_float command_percentage;
    gui_float clip_percentage;
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
    gui_uint mouse_clicked;
    struct gui_vec2 mouse_clicked_pos;
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
    struct gui_vertex *vertexes;
    gui_size vertex_size;
    struct gui_draw_command *commands;
    gui_size command_size;
};

struct gui_output {
    struct gui_draw_call_list **list;
    gui_size list_size;
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
    struct gui_color color;
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
    struct gui_color content;
    struct gui_color highlight;
    struct gui_color highlight_content;
};

enum gui_toggle_type {
    GUI_TOGGLE_CHECK,
    GUI_TOGGLE_OPTION
};

struct gui_toggle {
    gui_float x, y;
    gui_float w, h;
    gui_float pad_x, pad_y;
    gui_bool active;
    gui_size length;
    const gui_char *text;
    enum gui_toggle_type type;
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
    GUI_INPUT_BIN
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

enum gui_panel_colors {
    GUI_COLOR_TEXT,
    GUI_COLOR_PANEL,
    GUI_COLOR_BORDER,
    GUI_COLOR_TITLEBAR,
    GUI_COLOR_BUTTON,
    GUI_COLOR_BUTTON_BORDER,
    GUI_COLOR_BUTTON_HOVER,
    GUI_COLOR_BUTTON_HOVER_FONT,
    GUI_COLOR_CHECK,
    GUI_COLOR_CHECK_ACTIVE,
    GUI_COLOR_OPTION,
    GUI_COLOR_OPTION_ACTIVE,
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
    GUI_COLOR_SELECTOR,
    GUI_COLOR_SELECTOR_BORDER,
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
    struct gui_vec2 panel_padding;
    struct gui_vec2 panel_min_size;
    struct gui_vec2 item_spacing;
    struct gui_vec2 item_padding;
    struct gui_vec2 scaler_size;
    gui_float scrollbar_width;
    struct gui_color colors[GUI_COLOR_COUNT];
};

enum gui_panel_flags {
    GUI_PANEL_HEADER = 0x01,
    GUI_PANEL_BORDER = 0x02,
    GUI_PANEL_MINIMIZABLE = 0x4,
    GUI_PANEL_CLOSEABLE = 0x8,
    GUI_PANEL_HIDDEN = 0x10,
    GUI_PANEL_MOVEABLE = 0x20,
    GUI_PANEL_SCALEABLE = 0x40,
    /* intern */
    GUI_PANEL_SCROLLBAR = 0x100,
    GUI_PANEL_TAB = 0x200
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
    gui_bool minimized;
    struct gui_draw_buffer *out;
    const struct gui_font *font;
    const struct gui_input *in;
    const struct gui_config *config;
};

/* Input */
void gui_input_begin(struct gui_input*);
void gui_input_motion(struct gui_input*, gui_int x, gui_int y);
void gui_input_key(struct gui_input*, enum gui_keys, gui_int down);
void gui_input_button(struct gui_input*, gui_int x, gui_int y, gui_bool down);
void gui_input_char(struct gui_input*, const gui_glyph);
void gui_input_end(struct gui_input*);

/* Output */
void gui_output_begin(struct gui_draw_buffer*, const struct gui_memory*);
void gui_output_end(struct gui_draw_buffer*, struct gui_draw_call_list*,
                    struct gui_memory_status*);

/* Widgets */
void gui_widget_text(struct gui_draw_buffer*, const struct gui_text*,
                    const struct gui_font*);
gui_size gui_widget_text_wrap(struct gui_draw_buffer*, const struct gui_text*,
                    const struct gui_font*);
void gui_widget_image(struct gui_draw_buffer*, const struct gui_image*);
gui_bool gui_widget_button_text(struct gui_draw_buffer*, const struct gui_button*,
                    const char *text, gui_size len, const struct gui_font*,
                    const struct gui_input*);
gui_bool gui_widget_button_triangle(struct gui_draw_buffer*, struct gui_button*,
                    enum gui_heading, const struct gui_input*);
gui_bool gui_widget_button_icon(struct gui_draw_buffer*, struct gui_button*,
                    gui_texture, struct gui_texCoord from, struct gui_texCoord to,
                    const struct gui_input*);
gui_bool gui_widget_toggle(struct gui_draw_buffer*, const struct gui_toggle*,
                    const struct gui_font*, const struct gui_input*);
gui_float gui_widget_slider(struct gui_draw_buffer*, const struct gui_slider*,
                    const struct gui_input*);
gui_float gui_widget_slider_vertical(struct gui_draw_buffer*,
                    const struct gui_slider*, const struct gui_input*);
gui_size gui_widget_progress(struct gui_draw_buffer*, const struct gui_progress*,
                    const struct gui_input*);
gui_size gui_widget_progress_vertical(struct gui_draw_buffer *buf,
                    const struct gui_progress*, const struct gui_input*);
gui_bool gui_widget_input(struct gui_draw_buffer*,  gui_char*, gui_size*,
                    const struct gui_input_field*, const struct gui_font*,
                    const struct gui_input*);
gui_float gui_widget_scroll(struct gui_draw_buffer*, const struct gui_scroll*,
                    const struct gui_input*);
gui_int gui_widget_histo(struct gui_draw_buffer*, const struct gui_histo*,
                    const struct gui_input*);
gui_int gui_widget_plot(struct gui_draw_buffer*, const struct gui_plot*,
                    const struct gui_input*);

/* Panel */
void gui_default_config(struct gui_config*);
void gui_panel_init(struct gui_panel*, const struct gui_config*,
                    const struct gui_font*);
gui_bool gui_panel_begin(struct gui_panel*, struct gui_draw_buffer*,
                    const struct gui_input*, const char*,
                    gui_float x, gui_float y, gui_float w, gui_float h, gui_flags);
void gui_panel_layout(struct gui_panel*, gui_float height, gui_size cols);
void gui_panel_seperator(struct gui_panel*, gui_size cols);
void gui_panel_text(struct gui_panel*, const char *str, gui_size len);
gui_bool gui_panel_check(struct gui_panel*, const char*, gui_bool active);
gui_bool gui_panel_option(struct gui_panel*, const char*, gui_bool active);
gui_bool gui_panel_button_text(struct gui_panel*, const char*, enum gui_button_behavior);
gui_bool gui_panel_button_color(struct gui_panel*, const struct gui_color, enum gui_button_behavior);
gui_bool gui_panel_button_triangle(struct gui_panel*, enum gui_heading, enum gui_button_behavior);
gui_bool gui_panel_button_toggle(struct gui_panel*, const char*, gui_bool value);
gui_bool gui_panel_button_icon(struct gui_panel*, gui_texture,
                    struct gui_texCoord from, struct gui_texCoord to,
                    enum gui_button_behavior);
gui_float gui_panel_slider(struct gui_panel*, gui_float min, gui_float val,
                    gui_float max, gui_float step, enum gui_direction);
gui_size gui_panel_progress(struct gui_panel*, gui_size cur, gui_size max,
                    gui_bool modifyable, enum gui_direction);
gui_bool gui_panel_input(struct gui_panel*, gui_char *buffer, gui_size *len,
                    gui_size max, enum gui_input_filter, gui_bool active);
gui_size gui_panel_shell(struct gui_panel*, gui_char *buffer, gui_size *length,
                    gui_size max, gui_bool *active);
gui_bool gui_panel_spinner(struct gui_panel*, gui_int min, gui_int *value,
                    gui_int max, gui_int step, gui_bool active);
gui_size gui_panel_selector(struct gui_panel*, const char *items[],
                    gui_size item_count, gui_size item_current);
gui_int gui_panel_plot(struct gui_panel*, const gui_float *values,
                    gui_size value_count);
gui_int gui_panel_histo(struct gui_panel*, const gui_float *values,
                    gui_size value_count);
gui_bool gui_panel_tab_begin(struct gui_panel*, gui_tab*, const char *title);
void gui_panel_tab_end(struct gui_panel *panel, gui_tab *tab);
void gui_panel_group_begin(struct gui_panel*, gui_group*, const char *title);
void gui_panel_group_end(struct gui_panel*, gui_group* tab);
gui_size gui_panel_shelf_begin(struct gui_panel*, gui_shelf *shelf,
                    const char *tabs[], gui_size size, gui_size active);
void gui_panel_shelf_end(struct gui_panel *panel, gui_shelf *shelf);
void gui_panel_end(struct gui_panel*);

/* Context */
struct gui_context;
struct gui_context *gui_new(const struct gui_memory*, const struct gui_input*);
void gui_begin(struct gui_context*, gui_float width, gui_float height);
void gui_end(struct gui_context*, struct gui_output*, struct gui_memory_status*);
struct gui_panel *gui_panel_new(struct gui_context*, gui_float x, gui_float y, gui_float w,
                    gui_float h, const struct gui_config* , const struct gui_font*);
void gui_panel_del(struct gui_context*, struct gui_panel*);
gui_bool gui_begin_panel(struct gui_context*, struct gui_panel*, const char *title, gui_flags flags);
void gui_end_panel(struct gui_context*, struct gui_panel*, struct gui_memory_status*);

#endif
