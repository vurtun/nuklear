/*
    Copyright (c) 2015
    vurtun <polygone@gmx.net>
    MIT licence
*/
#ifndef GUI_H_
#define GUI_H_

#ifdef __cplusplus
extern "C" {
#endif

#define GUI_UTF_SIZE 4
#define GUI_INPUT_MAX 16
#define GUI_UTF_INVALID 0xFFFD

#ifdef GUI_USE_FIXED_TYPES
#include <stdint.h>
typedef int32_t gui_int;
typedef int32_t gui_bool;
typedef int16_t gui_short;
typedef int64_t gui_long;
typedef float gui_float;
typedef uint16_t gui_ushort;
typedef uint32_t gui_uint;
typedef uint64_t gui_ulong;
typedef uint32_t gui_flags;
typedef uint8_t gui_char;
typedef uint8_t gui_byte;
typedef uint32_t gui_flag;
typedef uint64_t gui_size;
#else
typedef int gui_int;
typedef int gui_bool;
typedef short gui_short;
typedef long gui_long;
typedef float gui_float;
typedef unsigned short gui_ushort;
typedef unsigned int gui_uint;
typedef unsigned long gui_ulong;
typedef unsigned int gui_flags;
typedef unsigned char gui_char;
typedef unsigned char gui_byte;
typedef unsigned int gui_flag;
typedef unsigned long gui_size;
#endif

enum {gui_false, gui_true};
enum gui_heading {GUI_UP, GUI_RIGHT, GUI_DOWN, GUI_LEFT};
struct gui_color {gui_byte r,g,b,a;};
struct gui_vec2 {gui_float x,y;};
struct gui_rect {gui_float x,y,w,h;};
struct gui_key {gui_bool down, clicked;};
struct gui_font;

typedef void* gui_image;
typedef gui_char gui_glyph[GUI_UTF_SIZE];
typedef gui_size(*gui_text_width_f)(void*,const gui_char*, gui_size);
typedef void(*gui_scissor)(void*, gui_float, gui_float, gui_float, gui_float);
typedef void(*gui_draw_line)(void*, gui_float, gui_float, gui_float, gui_float, struct gui_color);
typedef void(*gui_draw_rect)(void*, gui_float, gui_float, gui_float, gui_float, struct gui_color);
typedef void(*gui_draw_image)(void*, gui_float, gui_float, gui_float, gui_float, gui_image);
typedef void(*gui_draw_circle)(void*, gui_float, gui_float, gui_float, gui_float, struct gui_color);
typedef void(*gui_draw_triangle)(void*, gui_float, gui_float, gui_float, gui_float,
                            gui_float, gui_float, struct gui_color);
typedef void(*gui_draw_text)(void*, gui_float, gui_float, gui_float, gui_float,
                            const gui_char*, gui_size, const struct gui_font*,
                            struct gui_color, struct gui_color);
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

enum gui_text_align {
    GUI_TEXT_LEFT,
    GUI_TEXT_CENTERED,
    GUI_TEXT_RIGHT
};

struct gui_text {
    struct gui_vec2 padding;
    struct gui_color foreground;
    struct gui_color background;
};

enum gui_button_behavior {
    GUI_BUTTON_DEFAULT,
    GUI_BUTTON_REPEATER
};

struct gui_button {
    gui_float border;
    struct gui_vec2 padding;
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
    struct gui_vec2 padding;
    struct gui_color font;
    struct gui_color background;
    struct gui_color foreground;
    struct gui_color cursor;
};

struct gui_slider {
    struct gui_vec2 padding;
    struct gui_color background;
    struct gui_color foreground;
};

struct gui_scroll {
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
    struct gui_vec2 padding;
    gui_bool show_cursor;
    enum gui_input_filter filter;
    struct gui_color background;
    struct gui_color foreground;
    struct gui_color font;
};

enum gui_graph_type {
    GUI_GRAPH_LINES,
    GUI_GRAPH_HISTO,
    GUI_GRAPH_MAX
};

struct gui_graph {
    enum gui_graph_type type;
    gui_float x, y;
    gui_float w, h;
    gui_float min, max;
    struct gui_vec2 last;
    gui_size index;
    gui_size count;
};

enum gui_table_lines {
    GUI_TABLE_HHEADER = 0x01,
    GUI_TABLE_VHEADER = 0x02,
    GUI_TABLE_HBODY = 0x04,
    GUI_TABLE_VBODY = 0x08
};

struct gui_font {
    void *userdata;
    gui_float height;
    gui_text_width_f width;
};

struct gui_canvas {
    void *userdata;
    gui_size width;
    gui_size height;
    gui_scissor scissor;
    gui_draw_line draw_line;
    gui_draw_rect draw_rect;
    gui_draw_circle draw_circle;
    gui_draw_triangle draw_triangle;
    gui_draw_image draw_image;
    gui_draw_text draw_text;
};

struct gui_memory {
    void *memory;
    gui_size size;
};

struct gui_memory_status {
    gui_size size;
    gui_size allocated;
    gui_size needed;
    gui_size clipped_commands;
    gui_size clipped_memory;
};

struct gui_allocator {
    void *userdata;
    void*(*alloc)(void *usr, gui_size);
    void*(*realloc)(void *usr, void*, gui_size);
    void(*free)(void *usr, void*);
};

enum gui_command_type {
    GUI_COMMAND_NOP,
    GUI_COMMAND_SCISSOR,
    GUI_COMMAND_LINE,
    GUI_COMMAND_RECT,
    GUI_COMMAND_CIRCLE,
    GUI_COMMAND_TRIANGLE,
    GUI_COMMAND_TEXT
};

struct gui_command {
    enum gui_command_type type;
    gui_size offset;
};

struct gui_command_scissor {
    struct gui_command header;
    gui_short x, y;
    gui_ushort w, h;
};

struct gui_command_line {
    struct gui_command header;
    gui_short begin[2];
    gui_short end[2];
    struct gui_color color;
};

struct gui_command_rect {
    struct gui_command header;
    gui_short x, y;
    gui_ushort w, h;
    struct gui_color color;
};

struct gui_command_circle {
    struct gui_command header;
    gui_short x, y;
    gui_ushort w, h;
    struct gui_color color;
};

struct gui_command_image {
    struct gui_command header;
    gui_short x, y;
    gui_ushort w, h;
    gui_image img;
};

struct gui_command_triangle {
    struct gui_command header;
    gui_short a[2];
    gui_short b[2];
    gui_short c[2];
    struct gui_color color;
};

struct gui_command_text {
    struct gui_command header;
    void *font;
    gui_short x, y;
    gui_ushort w, h;
    struct gui_color bg;
    struct gui_color fg;
    gui_size length;
    gui_char string[1];
};

enum gui_buffer_flags {
    GUI_BUFFER_DEFAULT = 0,
    GUI_BUFFER_CLIPPING = 0x01,
    GUI_BUFFER_OWNER = 0x02,
    GUI_BUFFER_LOCKED = 0x04
};

struct gui_command_buffer {
    void *memory;
    gui_flags flags;
    struct gui_allocator allocator;
    struct gui_command *begin;
    struct gui_command *end;
    struct gui_rect clip;
    gui_size clipped_cmds;
    gui_size clipped_memory;
    gui_float grow_factor;
    gui_size sub_size;
    gui_size sub_cap;
    gui_size allocated;
    gui_size capacity;
    gui_size needed;
    gui_size count;
};

struct gui_command_list {
    struct gui_command_list *next;
    struct gui_command *begin;
    struct gui_command *end;
    gui_size count;
};

enum gui_panel_colors {
    GUI_COLOR_TEXT,
    GUI_COLOR_PANEL,
    GUI_COLOR_HEADER,
    GUI_COLOR_BORDER,
    GUI_COLOR_BUTTON,
    GUI_COLOR_BUTTON_BORDER,
    GUI_COLOR_BUTTON_HOVER,
    GUI_COLOR_BUTTON_TOGGLE,
    GUI_COLOR_BUTTON_HOVER_FONT,
    GUI_COLOR_CHECK,
    GUI_COLOR_CHECK_BACKGROUND,
    GUI_COLOR_CHECK_ACTIVE,
    GUI_COLOR_OPTION,
    GUI_COLOR_OPTION_BACKGROUND,
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
    GUI_COLOR_TABLE_LINES,
    GUI_COLOR_SCALER,
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
    GUI_PANEL_HIDDEN = 0x01,
    GUI_PANEL_BORDER = 0x02,
    GUI_PANEL_MINIMIZABLE = 0x4,
    GUI_PANEL_CLOSEABLE = 0x8,
    GUI_PANEL_MOVEABLE = 0x10,
    GUI_PANEL_SCALEABLE = 0x20,
    /* internal */
    GUI_PANEL_ACTIVE = 0x40,
    GUI_PANEL_SCROLLBAR = 0x80,
    GUI_PANEL_TAB = 0x100
};

struct gui_panel {
    gui_float x, y;
    gui_float w, h;
    gui_flags flags;
    gui_float offset;
    gui_bool minimized;
    struct gui_font font;
    const struct gui_config *config;
    struct gui_panel *next;
    struct gui_panel *prev;
};

struct gui_panel_layout {
    gui_float x, y, w, h;
    gui_float offset;
    gui_bool is_table;
    gui_flags tbl_flags;
    gui_bool valid;
    gui_float at_x;
    gui_float at_y;
    gui_size index;
    gui_float width, height;
    gui_float header_height;
    gui_float row_height;
    gui_size row_columns;
    struct gui_rect clip;
    struct gui_font font;
    const struct gui_config *config;
    const struct gui_input *input;
    const struct gui_canvas *canvas;
};

struct gui_pool_page {
    struct gui_pool_page *next;
    void *memory;
    gui_size capacity;
    gui_size count;
};

struct gui_pool {
    struct gui_allocator allocator;
    struct gui_pool_page base;
    struct gui_pool_page *pages;
    gui_size page_count;
    gui_size page_size;
    gui_size panel_size;
    gui_size panel_offset;
    struct gui_panel *free_list;
};

/* Input */
gui_size gui_utf_decode(const gui_char*, gui_long*, gui_size);
gui_size gui_utf_encode(gui_long, gui_char*, gui_size);
void gui_input_begin(struct gui_input*);
void gui_input_motion(struct gui_input*, gui_int x, gui_int y);
void gui_input_key(struct gui_input*, enum gui_keys, gui_bool down);
void gui_input_button(struct gui_input*, gui_int x, gui_int y, gui_bool down);
void gui_input_char(struct gui_input*, const gui_glyph);
void gui_input_end(struct gui_input*);


/* Buffer */
void gui_buffer_init(struct gui_command_buffer*, const struct gui_allocator*,
                    gui_size initial_size, gui_float grow_factor, gui_flag clipping);
void gui_buffer_init_fixed(struct gui_command_buffer*, const struct gui_memory*,
                    gui_flag clipping);
void gui_buffer_begin(struct gui_canvas *canvas, struct gui_command_buffer *buffer,
                    gui_size width, gui_size height);
void gui_buffer_lock(struct gui_canvas*, struct gui_command_buffer *buffer,
                    struct gui_command_buffer *sub, gui_flag clipping,
                    gui_size width, gui_size height);
void gui_buffer_unlock(struct gui_command_list*, struct gui_command_buffer *buf,
                    struct gui_command_buffer *sub, struct gui_canvas*,
                    struct gui_memory_status*);
void *gui_buffer_push(struct gui_command_buffer*,
                    enum gui_command_type, gui_size size);
void gui_buffer_push_scissor(struct gui_command_buffer*, gui_float, gui_float,
                    gui_float, gui_float);
void gui_buffer_push_line(struct gui_command_buffer*, gui_float, gui_float,
                    gui_float, gui_float, struct gui_color);
void gui_buffer_push_rect(struct gui_command_buffer*, gui_float, gui_float,
                    gui_float, gui_float, struct gui_color);
void gui_buffer_push_circle(struct gui_command_buffer*, gui_float, gui_float,
                    gui_float, gui_float, struct gui_color);
void gui_buffer_push_triangle(struct gui_command_buffer*, gui_float, gui_float,
                    gui_float, gui_float, gui_float, gui_float, struct gui_color);
void gui_buffer_push_image(struct gui_command_buffer*, gui_float, gui_float,
                    gui_float, gui_float, gui_image);
void gui_buffer_push_text(struct gui_command_buffer*, gui_float, gui_float,
                    gui_float, gui_float, const gui_char*, gui_size,
                    const struct gui_font*, struct gui_color, struct gui_color);
void gui_buffer_clear(struct gui_command_buffer*);
void gui_buffer_end(struct gui_command_list*, struct gui_command_buffer*,
                    struct gui_canvas*, struct gui_memory_status*);


/* List */
const struct gui_command* gui_list_begin(const struct gui_command_list*);
const struct gui_command* gui_list_next(const struct gui_command_list*,
                        const struct gui_command*);


/* Widgets */
void gui_text(const struct gui_canvas*, gui_float x, gui_float y, gui_float w, gui_float h,
                    const char *text, gui_size len, const struct gui_text*, enum gui_text_align,
                    const struct gui_font*);
gui_bool gui_button_text(const struct gui_canvas*, gui_float x, gui_float y,
                    gui_float w, gui_float h, const char*, enum gui_button_behavior,
                    const struct gui_button*, const struct gui_input*, const struct gui_font*);
gui_bool gui_button_image(const struct gui_canvas*, gui_float x, gui_float y,
                    gui_float w, gui_float h, gui_image, enum gui_button_behavior,
                    const struct gui_button*, const struct gui_input*);
gui_bool gui_button_triangle(const struct gui_canvas*, gui_float x, gui_float y,
                    gui_float w, gui_float h, enum gui_heading, enum gui_button_behavior,
                    const struct gui_button*, const struct gui_input*);
gui_bool gui_toggle(const struct gui_canvas*, gui_float x, gui_float y, gui_float w,
                    gui_float h, gui_bool, const char*, enum gui_toggle_type,
                    const struct gui_toggle*, const struct gui_input*, const struct gui_font*);
gui_float gui_slider(const struct gui_canvas*, gui_float x, gui_float y, gui_float w,
                    gui_float h, gui_float min, gui_float val, gui_float max, gui_float step,
                    const struct gui_slider*, const struct gui_input*);
gui_size gui_progress(const struct gui_canvas*, gui_float x, gui_float y, gui_float w,
                    gui_float h, gui_size value, gui_size max, gui_bool modifyable,
                    const struct gui_slider*, const struct gui_input*);
gui_size gui_edit(const struct gui_canvas*, gui_float x, gui_float y, gui_float w,
                    gui_float h, gui_char*, gui_size, gui_size max, gui_bool*,
                    const struct gui_input_field*, const struct gui_input*, const struct gui_font*);
gui_float gui_scroll(const struct gui_canvas*, gui_float x, gui_float y,
                    gui_float w, gui_float h, gui_float offset, gui_float target,
                    gui_float step, const struct gui_scroll*, const struct gui_input*);


/* Panel */
void gui_default_config(struct gui_config*);
void gui_panel_init(struct gui_panel*, gui_float x, gui_float y, gui_float w, gui_float h,
                    gui_flags, const struct gui_config *config, const struct gui_font*);
gui_bool gui_panel_begin(struct gui_panel_layout *layout, struct gui_panel*,
                    const char *title, const struct gui_canvas*, const struct gui_input*);
void gui_panel_row(struct gui_panel_layout*, gui_float height, gui_size cols);
void gui_panel_alloc_space(struct gui_rect*, struct gui_panel_layout*);
void gui_panel_seperator(struct gui_panel_layout*, gui_size cols);
void gui_panel_text(struct gui_panel_layout*, const char*, gui_size, enum gui_text_align);
void gui_panel_text_colored(struct gui_panel_layout*, const char*, gui_size, enum gui_text_align,
                    struct gui_color color);
void gui_panel_label(struct gui_panel_layout*, const char*, enum gui_text_align);
void gui_panel_label_colored(struct gui_panel_layout*, const char*, enum gui_text_align,
                    struct gui_color color);
gui_bool gui_panel_check(struct gui_panel_layout*, const char*, gui_bool active);
gui_bool gui_panel_option(struct gui_panel_layout*, const char*, gui_bool active);
gui_bool gui_panel_button_text(struct gui_panel_layout*, const char*, enum gui_button_behavior);
gui_bool gui_panel_button_color(struct gui_panel_layout*, const struct gui_color,
                    enum gui_button_behavior);
gui_bool gui_panel_button_triangle(struct gui_panel_layout*, enum gui_heading,
                    enum gui_button_behavior);
gui_bool gui_panel_button_image(struct gui_panel_layout*, gui_image image,
                    enum gui_button_behavior);
gui_bool gui_panel_button_toggle(struct gui_panel_layout*, const char*, gui_bool value);
gui_float gui_panel_slider(struct gui_panel_layout*, gui_float min, gui_float val,
                    gui_float max, gui_float step);
gui_size gui_panel_progress(struct gui_panel_layout*, gui_size cur, gui_size max,
                    gui_bool modifyable);
gui_size gui_panel_edit(struct gui_panel_layout*, gui_char *buffer, gui_size len,
                    gui_size max, gui_bool *active, enum gui_input_filter);
gui_bool gui_panel_shell(struct gui_panel_layout*, gui_char *buffer, gui_size *len,
                    gui_size max, gui_bool *active);
gui_int gui_panel_spinner(struct gui_panel_layout*, gui_int min, gui_int value,
                    gui_int max, gui_int step, gui_bool *active);
gui_size gui_panel_selector(struct gui_panel_layout*, const char *items[],
                    gui_size item_count, gui_size item_current);
void gui_panel_graph_begin(struct gui_panel_layout*, struct gui_graph*, enum gui_graph_type,
                    gui_size count, gui_float min_value, gui_float max_value);
gui_bool gui_panel_graph_push(struct gui_panel_layout *layout, struct gui_graph*, gui_float);
void gui_panel_graph_end(struct gui_panel_layout *layout, struct gui_graph*);
gui_int gui_panel_graph(struct gui_panel_layout*, enum gui_graph_type,
                    const gui_float *values, gui_size count);
gui_int gui_panel_graph_ex(struct gui_panel_layout*, enum gui_graph_type, gui_size count,
                    gui_float(*get_value)(void*, gui_size), void *userdata);
void gui_panel_table_begin(struct gui_panel_layout*, gui_flags flags,
                    gui_size row_height, gui_size cols);
void gui_panel_table_row(struct gui_panel_layout*);
void gui_panel_table_end(struct gui_panel_layout*);
gui_bool gui_panel_tab_begin(struct gui_panel_layout*, struct gui_panel_layout *tab,
                    const char*, gui_bool);
void gui_panel_tab_end(struct gui_panel_layout*, struct gui_panel_layout *tab);
void gui_panel_group_begin(struct gui_panel_layout*, struct gui_panel_layout *tab,
                    const char*,gui_float offset);
gui_float gui_panel_group_end(struct gui_panel_layout*, struct gui_panel_layout* tab);
gui_size gui_panel_shelf_begin(struct gui_panel_layout*, struct gui_panel_layout *shelf,
                    const char *tabs[], gui_size size, gui_size active, gui_float offset);
gui_float gui_panel_shelf_end(struct gui_panel_layout*, struct gui_panel_layout *shelf);
void gui_panel_end(struct gui_panel_layout*, struct gui_panel*);

#ifdef __cplusplus
}
#endif

#endif
