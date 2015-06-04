/*
    Copyright (c) 2015
    vurtun <polygone@gmx.net>
    MIT licence
*/
#ifndef GUI_H_
#define GUI_H_

#ifdef GUI_STATIC
#define GUI_API static
#else
#define GUI_API extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Constants */
#define GUI_UTF_SIZE 4
/* describes the number of bytes a glyph consists of*/
#define GUI_INPUT_MAX 16
/* defines the max number of bytes to be added as input in one frame */
#define GUI_MAX_COLOR_STACK 32
/* defines the number of temporary configuration color changes that can be stored */
#define GUI_MAX_ATTRIB_STACK 32
/* defines the number of temporary configuration attribute changes that can be stored */

/*
Since the gui uses ANSI C which does not guarantee to have fixed types, you need
to set the appropriate size of each type. However if your developer environment
supports fixed size types over the <stdint> header you can just use
#define GUI_USE_FIXED_TYPES
to automatically set the correct size for each type in the library.
*/
#ifdef GUI_USE_FIXED_TYPES
#include <stdint.h>
typedef int8_t gui_char;
typedef int32_t gui_int;
typedef int32_t gui_bool;
typedef int16_t gui_short;
typedef int64_t gui_long;
typedef float gui_float;
typedef uint16_t gui_ushort;
typedef uint32_t gui_uint;
typedef uint64_t gui_ulong;
typedef uint32_t gui_flags;
typedef uint8_t gui_byte;
typedef uint32_t gui_flag;
typedef uint64_t gui_size;
#else
typedef int gui_int;
typedef int gui_bool;
typedef char gui_char;
typedef short gui_short;
typedef long gui_long;
typedef float gui_float;
typedef unsigned short gui_ushort;
typedef unsigned int gui_uint;
typedef unsigned long gui_ulong;
typedef unsigned int gui_flags;
typedef unsigned char gui_byte;
typedef unsigned int gui_flag;
typedef unsigned long gui_size;
#endif

/* Utilities */
enum {gui_false, gui_true};
enum gui_heading {GUI_UP, GUI_RIGHT, GUI_DOWN, GUI_LEFT};
struct gui_color {gui_byte r,g,b,a;};
struct gui_vec2 {gui_float x,y;};
struct gui_rect {gui_float x,y,w,h;};
struct gui_key {gui_bool down, clicked;};
typedef union {void *ptr; gui_int id;} gui_handle;
typedef gui_char gui_glyph[GUI_UTF_SIZE];

/* Callbacks */
struct gui_font;
typedef gui_bool(*gui_filter)(gui_long unicode);
typedef gui_size(*gui_text_width_f)(gui_handle, const gui_char*, gui_size);

/*
 * ==============================================================
 *
 *                          Input
 *
 * ===============================================================
 */
/*  INPUT
    ----------------------------
    The input API is responsible for holding input state by keeping track of
    mouse, key and text input state. The core of the API is the persistent
    gui_input struct which holds the input state over the runtime of the gui.
    It is important to note that no direct os or window handling is done by the input
    API, instead all the input state has to be provided from the user. This in one hand
    expects more work from the user and complicates the usage but on the other hand
    provides simple abstraction over a big number of platforms, libraries and other
    already provided functionality.

    USAGE
    ----------------------------
    To instantiate the Input API the gui_input structure has to be zeroed at
    the beginning of the program by either using memset or setting it to {0},
    since the internal state is persistent over all frames.

    To modify the internal input state you have to first set the gui_input struct
    into a modifiable state with gui_input_begin. After the gui_input struct is
    now ready you can add input state changes until everything is up to date.
    Finally to revert back into a read state you have to call gui_input_end.

    Input function API
    gui_input_begin()       -- begins the modification state
    gui_input_motion()      -- notifies of a cursor motion update
    gui_input_key()         -- notifies of a keyboard key update
    gui_input_button()      -- notifies of a action event
    gui_input_char()        -- adds a text glyph to gui_input
    gui_input_end()         -- ends the modification state

*/
#if 0
/* Example */
#define GUI_IMPLEMENTATION
#include "gui.h"
int main(void)
{
    struct gui_input input;
    memset(&input, 0, sizeof(input));
    while (1) {
        gui_input_begin(&in);
        if (Key_state_changed)
            gui_input_key(&input, key, is_down);
        else if (button_state_changed)
            gui_input_button(&in, mouse_x, mouse_y, is_down);
        else if (mouse_position_changed)
            gui_input_motion(&input, mouse_x, mouse_y);
        else if (text_input)
            gui_input_char(&input, glyph);
        gui_input_end(&in);
    }
    return 0;
}
#endif

/* every key that is being used inside the library */
enum gui_keys {
    GUI_KEY_SHIFT,
    GUI_KEY_DEL,
    GUI_KEY_ENTER,
    GUI_KEY_BACKSPACE,
    GUI_KEY_ESCAPE,
    GUI_KEY_SPACE,
    GUI_KEY_MAX
};

struct gui_input {
    struct gui_key keys[GUI_KEY_MAX];
    /* state of every used key */
    gui_char text[GUI_INPUT_MAX];
    /* utf8 text input frame buffer */
    gui_size text_len;
    /* text input frame buffer length in bytes */
    struct gui_vec2 mouse_pos;
    /* current mouse position */
    struct gui_vec2 mouse_prev;
    /* mouse position in the last frame */
    struct gui_vec2 mouse_delta;
    /* mouse travelling distance over the last and current frame */
    gui_bool mouse_down;
    /* current mouse button state */
    gui_uint mouse_clicked;
    /* number of mouse button state transistion between frames */
    struct gui_vec2 mouse_clicked_pos;
    /* mouse position of the last mouse button state change */
};

GUI_API void gui_input_begin(struct gui_input*);
/*  this function sets the input state to writeable
    Input:
    - Input structure to set modfifiable
*/
GUI_API void gui_input_motion(struct gui_input*, gui_int x, gui_int y);
/*  this function updates the current mouse position
    Input:
    - Input structure to update to mouse state
    - local os window X position inside of the mouse
    - local os window Y position inside of the mouse
*/
GUI_API void gui_input_key(struct gui_input*, enum gui_keys, gui_bool down);
/*  this function updates the current state of a key
    Input:
    - Input structure to update to key state
    - key identifies whose state has been changed
    - the new state of the key
*/
GUI_API void gui_input_button(struct gui_input*, gui_int x, gui_int y, gui_bool down);
/*  this function updates the current state of the button
    Input:
    - Input structure to update to key state
    - local os window X position inside of the mouse
    - local os window Y position inside of the mouse
    - the new state of the button
*/
GUI_API void gui_input_char(struct gui_input*, const gui_glyph);
/*  this function adds a utf8 glpyh into the internal text frame buffer
    Input:
    - Input structure to add the glyph to
    - utf8 glyph to add
*/
GUI_API void gui_input_end(struct gui_input*);
/*  this function sets the input state to readable
    Input:
    - Input structure to set readable
*/

/*
 * ==============================================================
 *
 *                          Buffer
 *
 * ===============================================================
 */
/*  BUFFER
    ----------------------------
    A basic buffer API with linear allocation and resetting as only freeing policy.
    The buffer main purpose is to control all memory management inside
    the GUI toolkit and still leave memory control as much as possible in the hand
    of the user. The memory control is herby achivable over three different ways
    of memory handlong from the user.
    The first way is to use a fixed size block of memory to be filled up.
    Biggest advantage of using a fixed size block is a simple memory model.
    Downside is that if the buffer is full no way of accessing more memory is
    available, which fits target application with roughly known memory consumptions.
    The second way to mnamge memory is by extending the fixed size block by querying
    the buffer for information about the used size and needed size and allocate new
    memory if the buffer is full. While this approach is still better than just using
    a fixed size memory block the reallocation still has one invalid frame as consquence
    since the used memory information is only available at the end of the frame which leads
    to the last way of handling memory.
    The last and most complicated way of handling memory is by allocator callbacks.
    The user herby registers callbacks to be called to allocate, free and reallocate
    memory if needed. While this solves most allocation problems it causes some
    loss of flow control on the user side.

    USAGE
    ----------------------------
    To instantiate the Buffer you either have to call the fixed size or allocator
    initialization function and provide a memory block in the first case and
    an allocator in the second case.
    To allocate memory from the buffer you would call gui_buffer_alloc with a request
    memory block size aswell as an alignment for the block. Finally to reset the memory
    at the end of the frame and when the memory buffer inside the buffer is no longer
    needed you would call gui_buffer_reset. To free all memory that has been allocated
    by an allocator if the buffer is no longer being used you have to call gui_buffer_clear.

    Buffer function API
    gui_buffer_init         -- initializes a dynamic buffer
    gui_buffer_init_fixed   -- initializes a static buffer
    gui_buffer_info         -- provides memory information about a buffer
    gui_buffer_alloc        -- allocates a block of memory from the buffer
    gui_buffer_reset        -- resets the buffer back to an empty state
    gui_buffer_clear        -- frees all memory if the buffer is dynamic

*/
struct gui_memory_status {
    void *memory;
    /* pointer to the currently used memory block inside the referenced buffer */
    gui_uint type;
    /* type of the buffer which is either fixed size or dynamic */
    gui_size size;
    /* total size of the memory block */
    gui_size allocated;
    /* allocated amount of memory */
    gui_size needed;
    /* memory size that would have been allocated if enough memory was present */
    gui_size calls;
    /* number of allocation calls referencing this buffer */
};

struct gui_allocator {
    gui_handle userdata;
    /* handle to your own allocator */
    void*(*alloc)(gui_handle, gui_size);
    /* allocation function pointer */
    void*(*realloc)(gui_handle, void*, gui_size);
    /* reallocation pointer of a previously allocated memory block */
    void(*free)(gui_handle, void*);
    /* callback function pointer to finally free all allocated memory */
};

enum gui_buffer_type {
    GUI_BUFFER_FIXED,
    /* fixed size memory buffer */
    GUI_BUFFER_DYNAMIC
    /* dynically growing buffer */
};

struct gui_memory {void *ptr;gui_size size;};
struct gui_buffer {
    struct gui_allocator pool;
    /* allocator callback for dynamic buffers */
    enum gui_buffer_type type;
    /* memory type management type */
    struct gui_memory memory;
    /* memory and size of the current memory block */
    gui_float grow_factor;
    /* growing factor for dynamic memory management */
    gui_size allocated;
    /* total amount of memory allocated */
    gui_size needed;
    /* total amount of memory allocated if enough memory would have been present */
    gui_size calls;
    /* number of allcation calls */
};

GUI_API void gui_buffer_init(struct gui_buffer*, const struct gui_allocator*,
                            gui_size initial_size, gui_float grow_factor);
/*  this function initializes a growing buffer
    Input:
    - allocator holding your own alloctator and memory allocation callbacks
    - initial size of the buffer
    - factor to grow the buffer with if the buffer is full
    Output:
    - dynamically growing buffer
*/
GUI_API void gui_buffer_init_fixed(struct gui_buffer*, void *memory, gui_size size);
/*  this function initializes a fixed size buffer
    Input:
    - fixed size previously allocated memory block
    - size of the memory block
    Output:
    - fixed size buffer
*/
GUI_API void gui_buffer_info(struct gui_memory_status*, struct gui_buffer*);
/*  this function requests memory information from a buffer
    Input:
    - buffer to get the inforamtion from
    Output:
    - buffer memory information
*/
GUI_API void *gui_buffer_alloc(struct gui_buffer*, gui_size size, gui_size align);
/*  this functions allocated a aligned memory block from a buffer
    Input:
    - buffer to allocate memory from
    - size of the requested memory block
    - alignment requirement for the memory block
    Output:
    - memory block with given size and alignment requirement
*/
GUI_API void gui_buffer_reset(struct gui_buffer*);
/*  this functions resets the buffer back into a empty state
    Input:
    - buffer to reset
*/
GUI_API void gui_buffer_clear(struct gui_buffer*);
/*  this functions frees all memory inside a dynamically growing buffer
    Input:
    - buffer to clear
*/
#if 0
/* Example fixed size buffer */
#define GUI_IMPLEMENTATION
#include "gui.h"
int main(void)
{
    struct gui_buffer buffer;
    void *memory = calloc(4*1024);
    gui_buffer_init_fixed(&buffer, memory, 4*1024);
    void *ptr = gui_buffer_alloc(&buffer, 256, 4);
    gui_buffer_reset(&buffer);
    return 0;
}
#endif

#if 0
/* Example fixed size buffer */
#define GUI_IMPLEMENTATION
#include "gui.h"

int main(void)
{
    struct gui_allocator alloc;
    alloc.userdata = your_allocator;
    alloc.alloc = your_allocation_callback;
    alloc.realloc = your_reallocation_callback;
    alloc.free = your_free_callback;

    struct gui_buffer buffer;
    gui_buffer_init(&buffer, &alloc, 4*1024, 2.7f);
    void *ptr = gui_buffer_alloc(&buffer, 256, 4);
    gui_buffer_reset(&buffer);
    gui_buffer_clear(&buffer);
    return 0;
}
#endif

/*
 * ==============================================================
 *
 *                          Commands
 *
 * ===============================================================
 */
/*  DRAW COMMAND QUEUE
    ----------------------------
    The command buffer API enqueues draw calls as commands and therefore abstracts
    over drawing routines and enables defered drawing. The API offers a number of
    drawing primitives like lines, rectangles, circles, triangles, images, text and
    clipping rectangles, that have to be drawn by the user. Therefore the command
    buffer is the main toolkit output besides the actual widget output.
    The actual draw command execution is done by the user and is build up in a
    interpreter like fashion by iterating over all commands and executing each
    command differently depending on the command type.

    USAGE
    ----------------------------
    To use the command buffer you first have to initiate the buffer either as fixed
    size or growing buffer. After the initilization you can add primitives by
    calling the appropriate gui_command_buffer_XXX for each primitive.
    To iterate over each commands inside the buffer gui_foreach_command is
    provided. Finally to reuse the buffer after the frame use the
    gui_command_buffer_reset function.

    command buffer function API
    gui_command_buffer_init         -- initializes a dynamic command buffer
    gui_command_buffer_init_fixed   -- initializes a static command buffer
    gui_command_buffer_reset        -- resets the command buffer back to an empty state
    gui_command_buffer_clear        -- frees all memory if the command buffer is dynamic

    command queue function API
    gui_command_buffer_push         -- pushes and enqueues a command into the buffer
    gui_command_buffer_push_scissor -- pushes a clipping rectangle into the command queue
    gui_command_buffer_push_line    -- pushes a line into the command queue
    gui_command_buffer_push_rect    -- pushes a rectange into the command queue
    gui_command_buffer_push_circle  -- pushes a circle into the command queue
    gui_command_buffer_push_triangle-- pushes a triangle command into the queue
    gui_command_buffer_push_image   -- pushes a image draw command into the queue
    gui_command_buffer_push_text    -- pushes a text draw command into the queue

    command iterator function API
    gui_command_buffer_begin        -- returns the first command in a queue
    gui_command_buffer_next         -- returns the next command in a queue
    gui_foreach_command             -- iterates over all commands in a queue

*/

/* command type of every used drawing primitive */
enum gui_command_type {
    GUI_COMMAND_NOP,
    GUI_COMMAND_SCISSOR,
    GUI_COMMAND_LINE,
    GUI_COMMAND_RECT,
    GUI_COMMAND_CIRCLE,
    GUI_COMMAND_TRIANGLE,
    GUI_COMMAND_TEXT,
    GUI_COMMAND_IMAGE,
    GUI_COMMAND_MAX
};

/* command base and header of every comand inside the buffer */
struct gui_command {
    enum gui_command_type type;
    /* the type of the current command */
    gui_size offset;
    /* offset to the next command */
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
    gui_byte color[4];
};

struct gui_command_rect {
    struct gui_command header;
    gui_uint r;
    gui_short x, y;
    gui_ushort w, h;
    gui_byte color[4];
};

struct gui_command_circle {
    struct gui_command header;
    gui_short x, y;
    gui_ushort w, h;
    gui_byte color[4];
};

struct gui_command_image {
    struct gui_command header;
    gui_short x, y;
    gui_ushort w, h;
    gui_handle img;
};

struct gui_command_triangle {
    struct gui_command header;
    gui_short a[2];
    gui_short b[2];
    gui_short c[2];
    gui_byte color[4];
};

struct gui_command_text {
    struct gui_command header;
    gui_handle font;
    gui_byte bg[4];
    gui_byte fg[4];
    gui_short x, y;
    gui_ushort w, h;
    gui_size length;
    gui_char string[1];
};

enum gui_command_clipping {
    GUI_NOCLIP = gui_false,
    GUI_CLIP = gui_true
};

struct gui_command_buffer {
    struct gui_buffer base;
    /* memory buffer */
    struct gui_rect clip;
    /* current clipping rectangle */
    gui_bool use_clipping;
    /* flag if the command buffer should clip commands */
};

#define gui_command_buffer_init(b, a, i, g, c)\
    {gui_buffer_init(&(b)->base, a, i, g), (b)->use_clipping=c; (b)->clip=gui_null_rect;}
/*  this function initializes a growing buffer
    Input:
    - allocator holding your own alloctator and memory allocation callbacks
    - initial size of the buffer
    - factor to grow the buffer with if the buffer is full
    - clipping flag for draw command clipping
    Output:
    - dynamically growing command buffer
*/
#define gui_command_buffer_init_fixed(b, m, s,c)\
    {gui_buffer_init_fixed(&(b)->base, m ,s); (b)->use_clipping=c; (b)->clip=gui_null_rect;}
/*  this function initializes a fixed size buffer
    Input:
    - memory block to fill
    - size of the memory block
    - clipping flag for draw command clipping
    Output:
    - fixed size command buffer
*/
#define gui_command_buffer_reset(b)\
    gui_buffer_reset(&(b)->base)
/*  this function resets the buffer back to an empty state
    Input:
    - buffer to reset
*/
#define gui_command_buffer_clear(b)\
    gui_buffer_clear(&(b)->base)
/*  this function frees the memory of a dynamic buffer
    Input:
    - buffer to reset
*/
GUI_API void *gui_command_buffer_push(struct gui_command_buffer*, gui_uint type, gui_size size);
/*  this function push enqueues a command into the buffer
    Input:
    - buffer to push the command into
    - type of the command
    - amount of memory that is needed for the specified command
*/
GUI_API void gui_command_buffer_push_scissor(struct gui_command_buffer*, gui_float,
                                                gui_float, gui_float, gui_float);
/*  this function push a clip rectangle command into the buffer
    Input:
    - buffer to push the clip rectangle command into
    - x,y and width and height of the clip rectangle
*/
GUI_API void gui_command_buffer_push_line(struct gui_command_buffer*, gui_float, gui_float,
                                            gui_float, gui_float, struct gui_color);
/*  this function pushes a line draw command into the buffer
    Input:
    - buffer to push the clip rectangle command into
    - starting position of the line
    - ending position of the line
    - color of the line to draw
*/
GUI_API void gui_command_buffer_push_rect(struct gui_command_buffer *buffer, gui_float x,
                                        gui_float y, gui_float w, gui_float h,
                                        gui_float r, struct gui_color c);
/*  this function pushes a rectangle draw command into the buffer
    Input:
    - buffer to push the draw rectangle command into
    - rectangle position
    - rectangle size
    - rectangle rounding
    - color of the rectangle to draw
*/
GUI_API void gui_command_buffer_push_circle(struct gui_command_buffer*, gui_float, gui_float,
                                            gui_float, gui_float, struct gui_color);
/*  this function pushes a circle draw command into the buffer
    Input:
    - buffer to push the circle draw command into
    - x position of the top left of the circle
    - y position of the top left of the circle
    - rectangle diameter of the circle
    - color of the circle to draw
*/
GUI_API void gui_command_buffer_push_triangle(struct gui_command_buffer*, gui_float, gui_float,
                                                gui_float, gui_float, gui_float, gui_float,
                                                struct gui_color);
/*  this function pushes a triangle draw command into the buffer
    Input:
    - buffer to push the draw triangle command into
    - (x,y) coordinates of all three points
    - rectangle diameter of the circle
    - color of the triangle to draw
*/
GUI_API void gui_command_buffer_push_image(struct gui_command_buffer*, gui_float,
                                            gui_float, gui_float, gui_float, gui_handle);
/*  this function pushes a image draw command into the buffer
    Input:
    - buffer to push the draw image command into
    - position of the image with x,y position
    - size of the image to draw with width and height
    - rectangle diameter of the circle
    - color of the triangle to draw
*/
GUI_API void gui_command_buffer_push_text(struct gui_command_buffer*, gui_float, gui_float,
                                            gui_float, gui_float, const gui_char*, gui_size,
                                            const struct gui_font*, struct gui_color,
                                            struct gui_color);
/*  this function pushes a text draw command into the buffer
    Input:
    - buffer to push the draw text command into
    - top left position of the text with x,y position
    - maixmal size of the text to draw with width and height
    - color of the triangle to draw
*/
#define gui_ptr_add(t, p, i) ((t*)((void*)((gui_size)(p) + (i))))
#define gui_ptr_sub(t, p, i) ((t*)((void*)((gui_size)(p) - (i))))
#define gui_command(t, c) ((const struct gui_command_##t*)c)
#define gui_command_buffer_begin(b)\
    ((const struct gui_command*)(b)->base.memory.ptr)
#define gui_command_buffer_end(b)\
    (gui_ptr_add(const struct gui_command, (b)->base.memory.ptr, (b)->base.allocated))
#define gui_command_buffer_next(b, c)\
    ((gui_ptr_add(const struct gui_command,c,c->offset)<gui_command_buffer_end(b))?\
     gui_ptr_add(const struct gui_command,c,c->offset):NULL)
#define gui_foreach_command(i, b)\
    for((i)=gui_command_buffer_begin(b); (i)!=NULL; (i)=gui_command_buffer_next(b,i))

/*
 * ==============================================================
 *
 *                          Widgets
 *
 * ===============================================================
 */
/*  WIDGETS
    ----------------------------
    The Widget API supports a number of basic widgets like buttons, sliders or
    editboxes and stores no widget state. Instead the state of each widget has to
    be managed by the user.

    USAGE
    ----------------------------
    Most widgets takes an input struct, font and widget specific data and a command
    buffer to push draw command into and return the updated widget state.
    Important to note is that there is no actual state that is being stored inside
    the toolkit so everything has to be stored byte the user.

    Widget function API
    gui_text                -- draws a string inside a box
    gui_button_text         -- button widget with text content
    gui_button_image        -- button widget with icon content
    gui_button_triangle     -- button widget with triangle content
    gui_toggle              -- either a checkbox or radiobutton widget
    gui_slider              -- floating point slider widget
    gui_progress            -- unsigned integer progressbar widget
    gui_edit                -- Editbox wiget for user input
    gui_edit_filtered       -- Editbox with utf8 gylph filter capabilities
    gui_spinner             -- unsigned integer spinner widget
    gui_selector            -- string selector widget
    gui_scroll              -- scrollbar widget imeplementation
*/
/* Example */
#if 0
gui_command_buffer buffer;
void *memory = malloc(MEMORY_SIZE)
gui_buffer_init_fixed(buffer, memory, MEMORY_SIZE);

struct gui_font font = {...};
const struct gui_slider slider = {...};
const struct gui_progress progress = {...};
gui_float value = 5.0f
gui_size prog = 20;

struct gui_input input = {0};
while (1) {
    gui_input_begin(&input);
    /* record input */
    gui_input_end(&input);

    gui_command_buffer_reset(&buffer);
    value = gui_slider(&buffer, 50, 50, 100, 30, 0, value, 10, 1, &slider, &input);
    prog = gui_progress(&buffer, 50, 100, 100, 30, prog, 100, gui_false, &progress, &input);

    const struct gui_command *cmd;
    gui_foreach_command(cmd, buffer) {
        /* execute draw call command */
    }
}
#endif

struct gui_font {
    gui_handle userdata;
    /* user provided font handle */
    gui_float height;
    /* max height of the font */
    gui_text_width_f width;
    /* font string width in pixel callback */
};

enum gui_text_align {
    GUI_TEXT_LEFT,
    GUI_TEXT_CENTERED,
    GUI_TEXT_RIGHT
};

struct gui_text {
    struct gui_vec2 padding;
    /* padding between bounds and text */
    struct gui_color foreground;
    /*text color */
    struct gui_color background;
    /* text background color */
};

enum gui_button_behavior {
    GUI_BUTTON_DEFAULT,
    /* buton only returns on activation */
    GUI_BUTTON_REPEATER,
    /* button returns as long as the button is pressed */
    GUI_BUTTON_MAX
};

struct gui_button {
    gui_float border;
    /* size of the border */
    gui_float rounding;
    /* buttong rectangle rounding */
    struct gui_vec2 padding;
    /* padding between bounds and content */
    struct gui_color background;
    /* button color */
    struct gui_color foreground;
    /* button border color */
    struct gui_color content;
    /* button content color */
    struct gui_color highlight;
    /* background color if mouse is over */
    struct gui_color highlight_content;
    /* content color if mouse is over */
};

enum gui_toggle_type {
    GUI_TOGGLE_CHECK,
    /* checkbox toggle */
    GUI_TOGGLE_OPTION
    /* radiobutton toggle */
};

struct gui_toggle {
    gui_float rounding;
    /* checkbox rectangle rounding */
    struct gui_vec2 padding;
    /* padding between bounds and content */
    struct gui_color font;
    /* text color */
    struct gui_color background;
    /* toggle background color*/
    struct gui_color foreground;
    /* toggle foreground color*/
    struct gui_color cursor;
    /* toggle cursor color*/
};

struct gui_progress {
    gui_float rounding;
    /* prgressbar rectangle rounding */
    struct gui_vec2 padding;
    /* padding between bounds and content */
    struct gui_color background;
    /* progressbar background color */
    struct gui_color foreground;
    /* pgressbar cursor color */
};

enum gui_slider_cursor {
    GUI_SLIDER_RECT,
    GUI_SLIDER_CIRCLE
};

struct gui_slider {
    enum gui_slider_cursor cursor;
    /* slider cursor shape */
    struct gui_vec2 padding;
    /* padding between bounds and content */
    struct gui_color bar;
    /* slider background bar color */
    struct gui_color border;
    /* slider cursor border color */
    struct gui_color bg;
    /* slider background color */
    struct gui_color fg;
    /* slider cursor color */
};

struct gui_scroll {
    gui_float rounding;
    /* scrolbar rectangle rounding */
    struct gui_color background;
    /* scrollbar background color */
    struct gui_color foreground;
    /* scrollbar cursor color */
    struct gui_color border;
    /* scrollbar border color */
};

enum gui_input_filter {
    GUI_INPUT_DEFAULT,
    GUI_INPUT_ASCII,
    GUI_INPUT_FLOAT,
    GUI_INPUT_DEC,
    GUI_INPUT_HEX,
    GUI_INPUT_OCT,
    GUI_INPUT_BIN
};

struct gui_edit {
    gui_float border_size;
    /* editbox border line size */
    gui_float rounding;
    /* editbox rectangle rounding */
    struct gui_vec2 padding;
    /* padding between bounds and content*/
    gui_bool show_cursor;
    /* flag indication if the cursor should be drawn */
    struct gui_color background;
    /* editbox background */
    struct gui_color border;
    /* editbox border color */
    struct gui_color cursor;
    /* editbox cursor color */
    struct gui_color text;
    /* editbox text color */
};

struct gui_spinner {
    gui_size border_button;
    /* border line width */
    struct gui_color button_color;
    /* spinner button background color */
    struct gui_color button_border;
    /* spinner button border color */
    struct gui_color button_triangle;
    /* spinner up and down button triangle color  */
    struct gui_color color;
    /* spinner background color  */
    struct gui_color border;
    /* spinner border color */
    struct gui_color text;
    /* spinner text color */
    struct gui_vec2 padding;
    /* spinner padding between bounds and content*/
    gui_bool show_cursor;
    /* flag indicating if the cursor should be drawn */
};

struct gui_selector {
    gui_size border_button;
    /* border line width */
    struct gui_color button_color;
    /* selector button background color */
    struct gui_color button_border;
    /* selector button border color */
    struct gui_color button_triangle;
    /* selector button content color */
    struct gui_color color;
    /* spinner background color */
    struct gui_color border;
    /* spinner border color */
    struct gui_color text;
    /* spinner text color */
    struct gui_color text_bg;
    /* spinner text background color */
    struct gui_vec2 padding;
    /* padding between bounds and content*/
};

GUI_API void gui_text(struct gui_command_buffer*, gui_float, gui_float, gui_float, gui_float,
                    const char *text, gui_size len, const struct gui_text*, enum gui_text_align,
                    const struct gui_font*);
/*  this function execute a text widget with alignment
    Input:
    - output command buffer for draw commands
    - (x,y) coordinates for the button bounds
    - (width, height) size for the button bounds
    - string to draw
    - length of the string
    - visual widget style structure describing the text
    - text alignment with either left, center and right
    - font structure for text drawing
*/
GUI_API gui_bool gui_button_text(struct gui_command_buffer*, gui_float x, gui_float y,
                                gui_float w, gui_float h, const char*, enum gui_button_behavior,
                                const struct gui_button*, const struct gui_input*,
                                const struct gui_font*);
/*  this function executes a text button widget
    Input:
    - output command buffer for draw commands
    - (x,y) coordinates for the button bounds
    - (width, height) size for the button bounds
    - button text
    - button behavior with either repeating or transition state event
    - visual widget style structure describing the button
    - input structure to update the button with
    - font structure for text drawing
    Output:
    - returns gui_true if the button was pressed gui_false otherwise
*/
GUI_API gui_bool gui_button_image(struct gui_command_buffer*, gui_float x, gui_float y,
                                gui_float w, gui_float h, gui_handle img, enum gui_button_behavior,
                                const struct gui_button*, const struct gui_input*);
/*  this function executes a image button widget
    Input:
    - output command buffer for draw commands
    - (x,y) coordinates for the button bounds
    - (width, height) size for the button bounds
    - user provided image handle which is either a pointer or a id
    - button behavior with either repeating or transition state event
    - visual widget style structure describing the button
    - input structure to update the button with
    Output:
    - returns gui_true if the button was pressed gui_false otherwise
*/
GUI_API gui_bool gui_button_triangle(struct gui_command_buffer*, gui_float x, gui_float y,
                                    gui_float w, gui_float h, enum gui_heading,
                                    enum gui_button_behavior, const struct gui_button*,
                                    const struct gui_input*);
/*  this function executes a triangle button widget
    Input:
    - output command buffer for draw commands
    - (x,y) coordinates for the button bounds
    - (width, height) size for the button bounds
    - triangle direction with either left, top, right xor bottom
    - button behavior with either repeating or transition state event
    - visual widget style structure describing the button
    - input structure to update the button with
    Output:
    - returns gui_true if the button was pressed gui_false otherwise
*/
GUI_API gui_bool gui_toggle(struct gui_command_buffer*, gui_float x, gui_float y, gui_float w,
                    gui_float h, gui_bool, const char*, enum gui_toggle_type,
                    const struct gui_toggle*, const struct gui_input*, const struct gui_font*);
/*  this function executes a toggle (checkbox, radiobutton) widget
    Input:
    - output command buffer for draw commands
    - (x,y) coordinates for the button bounds
    - (width, height) size for the button bounds
    - active or inactive flag describing the state of the toggle
    - visual widget style structure describing the toggle
    - input structure to update the toggle with
    - font structure for text drawing
    Output:
    - returns the update state of the toggle
*/
GUI_API gui_float gui_slider(struct gui_command_buffer*, gui_float x, gui_float y, gui_float,
                            gui_float h, gui_float min, gui_float val, gui_float max, gui_float step,
                            const struct gui_slider*, const struct gui_input*);
/*  this function executes a slider widget
    Input:
    - output command buffer for draw commands
    - (x,y) coordinates for the button bounds
    - (width, height) size for the button bounds
    - minimal slider value that will not be underflown
    - slider value to be updated by the user
    - maximal slider value that will not be overflown
    - step interval the value will be updated with
    - visual widget style structure describing the slider
    - input structure to update the slider with
    Output:
    - returns the from the user input updated value
*/
GUI_API gui_size gui_progress(struct gui_command_buffer*, gui_float x, gui_float y,
                            gui_float w, gui_float h, gui_size value, gui_size max,
                            gui_bool modifyable, const struct gui_progress*,
                            const struct gui_input*);
/*  this function executes a progressbar widget
    Input:
    - output command buffer for draw commands
    - (x,y) coordinates for the button bounds
    - (width, height) size for the button bounds
    - progressbar value to be updated by the user
    - maximal progressbar value that will not be overflown
    - flag if the progressbar is modifyable by the user
    - visual widget style structure describing the progressbar
    - input structure to update the slider with
    Output:
    - returns the from the user input updated value
*/
GUI_API gui_size gui_edit(struct gui_command_buffer*, gui_float x, gui_float y, gui_float w,
                        gui_float h, gui_char*, gui_size, gui_size max, gui_bool*,
                        const struct gui_edit*, enum gui_input_filter filter,
                        const struct gui_input*, const struct gui_font*);
/*  this function executes a editbox widget
    Input:
    - output command buffer for draw commands
    - (x,y) coordinates for the button bounds
    - (width, height) size for the button bounds
    - char buffer to add or remove glyphes from/to
    - buffer text length in bytes
    - maximal buffer size
    - current state of the editbox with either active or inactive
    - visual widget style structure describing the editbox
    - glyph input filter type to only let specified glyph through
    - input structure to update the editbox with
    - font structure for text drawing
    Output:
    - state of the editbox with either active or inactive
    - returns the size of the buffer in bytes after the modification
*/
GUI_API gui_size gui_edit_filtered(struct gui_command_buffer*, gui_float x, gui_float y,
                                    gui_float w, gui_float h, gui_char*, gui_size,
                                    gui_size max, gui_bool*, const struct gui_edit*,
                                    gui_filter filter, const struct gui_input*,
                                    const struct gui_font*);
/*  this function executes a editbox widget
    Input:
    - output command buffer for draw commands
    - (x,y) coordinates for the button bounds
    - (width, height) size for the button bounds
    - char buffer to add or remove glyphes from/to
    - buffer text length in bytes
    - maximal buffer size
    - current state of the editbox with either active or inactive
    - visual widget style structure describing the editbox
    - glyph input filter callback to only let specified glyph through
    - input structure to update the editbox with
    - font structure for text drawing
    Output:
    - state of the editbox with either active or inactive
    - returns the size of the buffer in bytes after the modification
*/
GUI_API gui_int gui_spinner(struct gui_command_buffer*, gui_float x, gui_float y, gui_float w,
                            gui_float h, const struct gui_spinner*, gui_int min, gui_int value,
                            gui_int max, gui_int step, gui_bool *active, const struct gui_input*,
                            const struct gui_font*);
/*  this function executes a spinner widget
    Input:
    - output command buffer for draw commands
    - (x,y) coordinates for the button bounds
    - (width, height) size for the button bounds
    - visual widget style structure describing the spinner
    - minimal spinner value that will no be underflown
    - spinner value that will be updated
    - maximal spinner value that will no be overflown
    - spinner input state with either active or inactive
    - input structure to update the slider with
    - font structure for text drawing
    Output:
    - returns the from the user input updated spinner value
*/
GUI_API gui_size gui_selector(struct gui_command_buffer*, gui_float x, gui_float y,
                                gui_float w, gui_float h, const struct gui_selector*,
                                const char *items[], gui_size item_count,
                                gui_size item_current, const struct gui_input*,
                                const struct gui_font*);
/*  this function executes a selector widget
    Input:
    - output command buffer for draw commands
    - (x,y) coordinates for the button bounds
    - (width, height) size for the button bounds
    - visual widget style structure describing the selector
    - selection of string array to select from
    - size of the selection array
    - input structure to update the slider with
    - font structure for text drawing
    Output:
    - returns the from the user input updated spinner value
*/
GUI_API gui_float gui_scroll(struct gui_command_buffer*, gui_float x, gui_float y,
                            gui_float w, gui_float h, gui_float offset, gui_float target,
                            gui_float step, const struct gui_scroll*, const struct gui_input*);
/*  this function executes a scrollbar widget
    Input:
    - output command buffer for draw commands
    - (x,y) coordinates for the button bounds
    - (width, height) size for the button bounds
    - scrollbar offset in source pixel
    - destination pixel size
    - step pixel size if the scrollbar up- or down button is pressed
    - visual widget style structure describing the selector
    - input structure to update the slider with
    Output:
    - returns the from the user input updated scrollbar offset in pixels
*/
/*
 * ==============================================================
 *
 *                          Config
 *
 * ===============================================================
 */
/*  CONFIG
    ----------------------------
    The panel configuration consists of style information that is used for
    the general style and looks of panel. In addition for temporary modification
    the configuration structure consists of a stack for pushing and pop either
    color or property values.

    USAGE
    ----------------------------
    To use the configuration file you either initial every value yourself besides
    the internal stack which needs to be initialized to zero or use the default
    configuration by calling the function gui_config_default.
    To add and remove temporary configuration states the gui_config_push_xxxx
    for adding and gui_config_pop_xxxx for removing either color or property values
    from the stack. To reset all previously modified values the gui_config_reset_xxx
    were added.

    Configuration function API
    gui_config_default              -- initializes a default panel configuration
    gui_config_property             -- returns the property value from an id
    gui_config_color                -- returns the color value from an id
    gui_config_push_property        -- push an old property onto a interal stack and sets a new value
    gui_config_push_color           -- push an old color onto a internal stack and sets a new value
    gui_config_pop_color            -- resets an old color value from the internal stack
    gui_config_pop_property         -- resets an old property value from the internal stack
    gui_config_reset_colors         -- reverts back all temporary color changes from the config
    gui_config_reset_properties     -- reverts back all temporary property changes from the config
    gui_config_reset                -- reverts back all temporary all changes from the config
*/
enum gui_config_colors {
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
    GUI_COLOR_SLIDER,
    GUI_COLOR_SLIDER_BAR,
    GUI_COLOR_SLIDER_BORDER,
    GUI_COLOR_SLIDER_CURSOR,
    GUI_COLOR_PROGRESS,
    GUI_COLOR_PROGRESS_CURSOR,
    GUI_COLOR_INPUT,
    GUI_COLOR_INPUT_CURSOR,
    GUI_COLOR_INPUT_BORDER,
    GUI_COLOR_INPUT_TEXT,
    GUI_COLOR_SPINNER,
    GUI_COLOR_SPINNER_BORDER,
    GUI_COLOR_SPINNER_TRIANGLE,
    GUI_COLOR_SPINNER_TEXT,
    GUI_COLOR_SELECTOR,
    GUI_COLOR_SELECTOR_BORDER,
    GUI_COLOR_SELECTOR_TRIANGLE,
    GUI_COLOR_SELECTOR_TEXT,
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
    GUI_COLOR_SHELF,
    GUI_COLOR_SHELF_TEXT,
    GUI_COLOR_SHELF_ACTIVE,
    GUI_COLOR_SHELF_ACTIVE_TEXT,
    GUI_COLOR_SCALER,
    GUI_COLOR_COUNT
};

enum gui_config_rounding {
    GUI_ROUNDING_PANEL,
    GUI_ROUNDING_BUTTON,
    GUI_ROUNDING_CHECK,
    GUI_ROUNDING_PROGRESS,
    GUI_ROUNDING_INPUT,
    GUI_ROUNDING_GRAPH,
    GUI_ROUNDING_SCROLLBAR,
    GUI_ROUNDING_MAX
};

enum gui_config_properties {
    GUI_PROPERTY_ITEM_SPACING,
    GUI_PROPERTY_ITEM_PADDING,
    GUI_PROPERTY_PADDING,
    GUI_PROPERTY_SCALER_SIZE,
    GUI_PROPERTY_SCROLLBAR_WIDTH,
    GUI_PROPERTY_SIZE,
    GUI_PROPERTY_MAX
};

struct gui_saved_property {
    enum gui_config_properties type;
    struct gui_vec2 value;
};

struct gui_saved_color {
    enum gui_config_colors type;
    struct gui_color value;
};

enum gui_config_components {
    GUI_DEFAULT_COLOR = 0x01,
    GUI_DEFAULT_PROPERTIES = 0x02,
    GUI_DEFAULT_ROUNDING = 0x04,
    GUI_DEFAULT_ALL = 0xFFFF
};

struct gui_config_stack  {
    gui_size property;
    struct gui_saved_property properties[GUI_MAX_ATTRIB_STACK];
    struct gui_saved_color colors[GUI_MAX_COLOR_STACK];
    gui_size color;
};

struct gui_config {
    struct gui_font font;
    enum gui_slider_cursor slider_cursor;
    gui_float rounding[GUI_ROUNDING_MAX];
    struct gui_vec2 properties[GUI_PROPERTY_MAX];
    struct gui_color colors[GUI_COLOR_COUNT];
    struct gui_config_stack stack;
};

GUI_API void gui_config_default(struct gui_config*, gui_flags, const struct gui_font*);
GUI_API struct gui_vec2 gui_config_property(const struct gui_config*,
                    enum gui_config_properties);
GUI_API struct gui_color gui_config_color(const struct gui_config*, enum gui_config_colors);
GUI_API void gui_config_push_property(struct gui_config*, enum gui_config_properties,
                    gui_float, gui_float);
GUI_API void gui_config_push_color(struct gui_config*, enum gui_config_colors,
                    gui_byte, gui_byte, gui_byte, gui_byte);
GUI_API void gui_config_pop_color(struct gui_config*);
GUI_API void gui_config_pop_property(struct gui_config*);
GUI_API void gui_config_reset_colors(struct gui_config*);
GUI_API void gui_config_reset_properties(struct gui_config*);
GUI_API void gui_config_reset(struct gui_config*);

/*
 * ==============================================================
 *
 *                          Panel
 *
 * ===============================================================
 */
/*  PANEL
    ----------------------------

    USAGE
    ----------------------------
    Panel function API
    gui_panel_init          -- initializes the panel with position, size and flags
    gui_panel_begin         -- begin sequence point in the panel layout build up process
    gui_panel_begin_stacked -- extends gui_panel_begin by adding the panel into a panel stack
    gui_panel_begin_tiled   -- extends gui_panel_begin by adding the pnale into a tiled layout
    gui_panel_row           -- defines the current row layout with row height and number of columns
    gui_panel_widget        -- base function for all widgets to allocate space on the panel and check if valid
    gui_panel_spacing       -- create a column seperator and is basically an empty widget filler
    gui_panel_text          -- text widget for printing text with length
    gui_panel_text_colored  -- colored text widget for printing colored text width length
    gui_panel_label         -- text widget for printing zero terminated strings
    gui_panel_label_colored -- text wicget for printing colored zero terminiated strings
    gui_panel_check         -- add a checkbox widget with either active or inactive state
    gui_panel_option        -- radiobutton widget with either active or inactive state
    gui_panel_option_group  -- radiobutton group with automates the process of having only one active
    gui_panel_button_text   -- button widget with text content
    gui_panel_button_color  -- colored button widget without content
    gui_panel_button_triangle --button with triangle pointing either up-/down-/left- or right
    gui_panel_button_image  -- button widget width icon content
    gui_panel_button_toggle -- toggle button with either active or inactive state
    gui_panel_slider        -- slider widget with min and max value as well as stepping range
    gui_panel_progress      -- either modifyable or static progressbar
    gui_panel_edit          -- edit textbox widget for text input
    gui_panel_edit_filtered -- edit textbox widget for text input with filter input
    gui_panel_spinner       -- spinner widget with either keyboard or mouse modification
    gui_panel_selector      -- selector widget for combobox like selection of types
    gui_panel_graph_begin   -- immediate mode graph building begin sequence point
    gui_panel_graph_push    -- push a value into a graph
    gui_panel_graph_end     -- immediate mode graph building end sequence point
    gui_panel_graph         -- retained mode graph with array of values
    gui_panel_graph_ex      -- ratained mode graph with getter callback
    gui_panel_end           -- end squeunce point which finializes the panel build up
*/
enum gui_table_lines {
    GUI_TABLE_HHEADER = 0x01,
    GUI_TABLE_VHEADER = 0x02,
    GUI_TABLE_HBODY = 0x04,
    GUI_TABLE_VBODY = 0x08
};

enum gui_graph_type {
    GUI_GRAPH_LINES,
    GUI_GRAPH_COLUMN,
    GUI_GRAPH_MAX
};

struct gui_graph {
    gui_bool valid;
    enum gui_graph_type type;
    gui_float x, y;
    gui_float w, h;
    gui_float min, max;
    struct gui_vec2 last;
    gui_size index;
    gui_size count;
};

enum gui_panel_tab {
    GUI_MAXIMIZED = gui_false,
    GUI_MINIMIZED = gui_true
};

enum gui_panel_flags {
    GUI_PANEL_HIDDEN = 0x01,
    GUI_PANEL_BORDER = 0x02,
    GUI_PANEL_MINIMIZABLE = 0x04,
    GUI_PANEL_CLOSEABLE = 0x08,
    GUI_PANEL_MOVEABLE = 0x10,
    GUI_PANEL_SCALEABLE = 0x20,
    GUI_PANEL_NO_HEADER = 0x40,
    GUI_PANEL_BORDER_HEADER = 0x80,
    /* internal */
    GUI_PANEL_ACTIVE = 0x100,
    GUI_PANEL_SCROLLBAR = 0x200,
    GUI_PANEL_TAB = 0x400
};

struct gui_panel {
    gui_float x, y;
    gui_float w, h;
    gui_flags flags;
    gui_float offset;
    gui_bool minimized;
    const struct gui_config *config;
    struct gui_command_buffer *buffer;
    struct gui_panel* next;
    struct gui_panel* prev;
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
    const struct gui_config *config;
    const struct gui_input *input;
    struct gui_command_buffer *buffer;
};

/* Panel */
struct gui_stack;
struct gui_layout;
GUI_API void gui_panel_init(struct gui_panel*, gui_float x, gui_float y, gui_float w,
                    gui_float h, gui_flags, struct gui_command_buffer*,const struct gui_config*);
GUI_API gui_bool gui_panel_begin(struct gui_panel_layout *layout, struct gui_panel*,
                    const char *title, const struct gui_input*);
GUI_API gui_bool gui_panel_begin_stacked(struct gui_panel_layout*, struct gui_panel*,
                    struct gui_stack*, const char*, const struct gui_input*);
GUI_API gui_bool gui_panel_begin_tiled(struct gui_panel_layout*, struct gui_panel*,
                    struct gui_layout*, gui_uint slot, gui_size index,
                    const char*, const struct gui_input*);
GUI_API gui_size gui_panel_row_columns(const struct gui_panel_layout *layout,
                    gui_size widget_size);
GUI_API void gui_panel_row(struct gui_panel_layout*, gui_float height, gui_size cols);
GUI_API gui_bool gui_panel_widget(struct gui_rect*, struct gui_panel_layout*);
GUI_API void gui_panel_spacing(struct gui_panel_layout*, gui_size cols);
GUI_API void gui_panel_text(struct gui_panel_layout*, const char*, gui_size,
                    enum gui_text_align);
GUI_API void gui_panel_text_colored(struct gui_panel_layout*, const char*, gui_size,
                    enum gui_text_align, struct gui_color);
GUI_API void gui_panel_label(struct gui_panel_layout*, const char*, enum gui_text_align);
GUI_API void gui_panel_label_colored(struct gui_panel_layout*, const char*,
                    enum gui_text_align, struct gui_color);
GUI_API gui_bool gui_panel_check(struct gui_panel_layout*, const char*, gui_bool active);
GUI_API gui_bool gui_panel_option(struct gui_panel_layout*, const char*, gui_bool active);
GUI_API gui_size gui_panel_option_group(struct gui_panel_layout*, const char**,
                    gui_size cnt, gui_size cur);
GUI_API gui_bool gui_panel_button_text(struct gui_panel_layout*, const char*,
                    enum gui_button_behavior);
GUI_API gui_bool gui_panel_button_color(struct gui_panel_layout*, struct gui_color,
                    enum gui_button_behavior);
GUI_API gui_bool gui_panel_button_triangle(struct gui_panel_layout*, enum gui_heading,
                    enum gui_button_behavior);
GUI_API gui_bool gui_panel_button_image(struct gui_panel_layout*, gui_handle img,
                    enum gui_button_behavior);
GUI_API gui_bool gui_panel_button_toggle(struct gui_panel_layout*, const char*,gui_bool value);
GUI_API gui_float gui_panel_slider(struct gui_panel_layout*, gui_float min, gui_float val,
                    gui_float max, gui_float step);
GUI_API gui_size gui_panel_progress(struct gui_panel_layout*, gui_size cur, gui_size max,
                    gui_bool modifyable);
GUI_API gui_size gui_panel_edit(struct gui_panel_layout*, gui_char *buffer, gui_size len,
                    gui_size max, gui_bool *active, enum gui_input_filter);
GUI_API gui_size gui_panel_edit_filtered(struct gui_panel_layout*, gui_char *buffer,
                    gui_size len, gui_size max, gui_bool *active, gui_filter);
GUI_API gui_int gui_panel_spinner(struct gui_panel_layout*, gui_int min, gui_int value,
                    gui_int max, gui_int step, gui_bool *active);
GUI_API gui_size gui_panel_selector(struct gui_panel_layout*, const char *items[],
                    gui_size item_count, gui_size item_current);
GUI_API void gui_panel_graph_begin(struct gui_panel_layout*, struct gui_graph*,
                    enum gui_graph_type, gui_size count, gui_float min, gui_float max);
GUI_API gui_bool gui_panel_graph_push(struct gui_panel_layout*,struct gui_graph*,gui_float);
GUI_API void gui_panel_graph_end(struct gui_panel_layout *layout, struct gui_graph*);
GUI_API gui_int gui_panel_graph(struct gui_panel_layout*, enum gui_graph_type,
                    const gui_float *values, gui_size count, gui_size offset);
GUI_API gui_int gui_panel_graph_ex(struct gui_panel_layout*, enum gui_graph_type,
                    gui_size count, gui_float(*get_value)(void*, gui_size), void *userdata);
GUI_API void gui_panel_table_begin(struct gui_panel_layout*, gui_flags flags,
                    gui_size row_height, gui_size cols);
GUI_API void gui_panel_table_row(struct gui_panel_layout*);
GUI_API void gui_panel_table_end(struct gui_panel_layout*);
GUI_API gui_bool gui_panel_tab_begin(struct gui_panel_layout*, struct gui_panel_layout *tab,
                    const char*, gui_bool);
GUI_API void gui_panel_tab_end(struct gui_panel_layout*, struct gui_panel_layout *tab);
GUI_API void gui_panel_group_begin(struct gui_panel_layout*, struct gui_panel_layout *tab,
                    const char*, gui_float offset);
GUI_API gui_float gui_panel_group_end(struct gui_panel_layout*, struct gui_panel_layout* tab);
GUI_API gui_size gui_panel_shelf_begin(struct gui_panel_layout*, struct gui_panel_layout*,
                    const char *tabs[], gui_size size, gui_size active, gui_float offset);
GUI_API gui_float gui_panel_shelf_end(struct gui_panel_layout*, struct gui_panel_layout*);
GUI_API void gui_panel_end(struct gui_panel_layout*, struct gui_panel*);


/*
 * ==============================================================
 *
 *                          Stack
 *
 * ===============================================================
 */
struct gui_stack {
    gui_size count;
    struct gui_panel *begin;
    struct gui_panel *end;
};

GUI_API void gui_stack_clear(struct gui_stack*);
GUI_API void gui_stack_push(struct gui_stack*, struct gui_panel*);
GUI_API void gui_stack_pop(struct gui_stack*, struct gui_panel*);
#define gui_foreach_panel(i, s) for (i = (s)->begin; i != NULL; i = (i)->next)

/*
 * ==============================================================
 *
 *                          Layout
 *
 * ===============================================================
 */
enum gui_layout_state {
    GUI_LAYOUT_INACTIVE,
    GUI_LAYOUT_ACTIVE
};

enum gui_layout_slot_index {
    GUI_SLOT_TOP,
    GUI_SLOT_BOTTOM,
    GUI_SLOT_LEFT,
    GUI_SLOT_CENTER,
    GUI_SLOT_RIGHT,
    GUI_SLOT_MAX
};

enum gui_layout_format {
    GUI_LAYOUT_HORIZONTAL,
    GUI_LAYOUT_VERTICAL
};

struct gui_layout_config {
    gui_float left;
    gui_float right;
    gui_float centerh;
    gui_float centerv;
    gui_float bottom;
    gui_float top;
};

struct gui_layout_slot {
    gui_size capacity;
    struct gui_vec2 ratio;
    struct gui_vec2 offset;
    enum gui_layout_format format;
};

struct gui_layout {
    gui_flags flags;
    gui_size width, height;
    enum gui_layout_state state;
    struct gui_stack stack;
    struct gui_layout_slot slots[GUI_SLOT_MAX];
};

GUI_API void gui_layout_init(struct gui_layout*, const struct gui_layout_config*,
                    gui_size width, gui_size height);
GUI_API void gui_layout_set_size(struct gui_layout*, gui_size width, gui_size height);
GUI_API void gui_layout_slot(struct gui_layout*, enum gui_layout_slot_index,
                    enum gui_layout_format, gui_size panel_count);



#ifdef GUI_IMPLEMENTATION

#ifndef GUI_ASSERT
#define GUI_ASSERT(expr)
#endif

#ifndef NULL
#define NULL ((void*)0)
#endif
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#endif
#ifndef CLAMP
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#endif

#define GUI_UTF_INVALID 0xFFFD
#define GUI_MAX_NUMBER_BUFFER 64
#define GUI_SATURATE(x) (MAX(0, MIN(1.0f, x)))
#define GUI_LEN(a) (sizeof(a)/sizeof(a)[0])
#define GUI_ABS(a) (((a) < 0) ? -(a) : (a))
#define GUI_BETWEEN(x, a, b) ((a) <= (x) && (x) <= (b))
#define GUI_INBOX(px, py, x, y, w, h) (GUI_BETWEEN(px, x, x+w) && GUI_BETWEEN(py, y, y+h))
#define GUI_INTERSECT(x0, y0, w0, h0, x1, y1, w1, h1) \
    (!(((x1 > (x0 + w0)) || ((x1 + w1) < x0) || (y1 > (y0 + h0)) || (y1 + h1) < y0)))

#define gui_vec2_mov(to,from) (to).x = (from).x, (to).y = (from).y
#define gui_vec2_sub(r,a,b) do {(r).x=(a).x-(b).x; (r).y=(a).y-(b).y;} while(0)
#define gui_color_to_array(ar, c)\
    (ar)[0] = (c).r, (ar)[1] = (c).g, (ar)[2] = (c).b, (ar)[3] = (c).a

#define GUI_ALIGNOF(t) ((char*)(&((struct {char c; t _h;}*)0)->_h) - (char*)0)
#define GUI_ALIGN_PTR(x, mask) (void*)((gui_size)((gui_byte*)(x) + (mask-1)) & ~(mask-1))
#define GUI_ALIGN(x, mask) ((x) + (mask-1)) & ~(mask-1)
#define GUI_OFFSETOF(st, m) ((gui_size)(&((st *)0)->m))

static const struct gui_rect gui_null_rect = {-9999.0f, 9999.0f, 9999.0f, 9999.0f};
static const gui_byte gui_utfbyte[GUI_UTF_SIZE+1] = {0x80, 0, 0xC0, 0xE0, 0xF0};
static const gui_byte gui_utfmask[GUI_UTF_SIZE+1] = {0xC0, 0x80, 0xE0, 0xF0, 0xF8};
static const long gui_utfmin[GUI_UTF_SIZE+1] = {0, 0, 0x80, 0x800, 0x100000};
static const long gui_utfmax[GUI_UTF_SIZE+1] = {0x10FFFF, 0x7F, 0x7FF, 0xFFFF, 0x10FFFF};

/*
 * ==============================================================
 *
 *                          Utility
 *
 * ===============================================================
 */
GUI_API struct gui_color
gui_rgba(gui_byte r, gui_byte g, gui_byte b, gui_byte a)
{
    struct gui_color ret;
    ret.r = r; ret.g = g;
    ret.b = b; ret.a = a;
    return ret;
}

GUI_API struct gui_color
gui_rgb(gui_byte r, gui_byte g, gui_byte b)
{
    struct gui_color ret;
    ret.r = r; ret.g = g;
    ret.b = b; ret.a = 255;
    return ret;
}

static struct gui_vec2
gui_vec2(gui_float x, gui_float y)
{
    struct gui_vec2 ret;
    ret.x = x; ret.y = y;
    return ret;
}

static void*
gui_memcopy(void *dst, const void *src, gui_size size)
{
    gui_size i = 0;
    char *d = dst;
    const char *s = src;
    for (i = 0; i < size; ++i)
        d[i] = s[i];
    return dst;
}

static void
gui_zero(void *dst, gui_size size)
{
    gui_size i;
    char *d = dst;
    for (i = 0; i < size; ++i) d[i] = 0;
}

static gui_size
gui_strsiz(const char *str)
{
    gui_size siz = 0;
    while (str && *str++ != '\0') siz++;
    return siz;
}

static gui_int
gui_strtoi(gui_int *number, const char *buffer, gui_size len)
{
    gui_size i;
    if (!number || !buffer)
        return 0;
    *number = 0;
    for (i = 0; i < len; ++i)
        *number = *number * 10 + (buffer[i] - '0');
    return 1;
}

static gui_size
gui_itos(char *buffer, gui_int num)
{
    static const char digit[] = "0123456789";
    gui_int shifter;
    gui_size len = 0;
    char *p = buffer;
    if (!buffer)
        return 0;

    if (num < 0) {
        num = GUI_ABS(num);
        *p++ = '-';
    }
    shifter = num;

    do {
        ++p;
        shifter = shifter/10;
    } while (shifter);
    *p = '\0';

    len = (gui_size)(p - buffer);
    do {
        *--p = digit[num % 10];
        num = num / 10;
    } while (num);
    return len;
}

static void
gui_unify(struct gui_rect *clip, const struct gui_rect *a, gui_float x0, gui_float y0,
    gui_float x1, gui_float y1)
{
    clip->x = MAX(a->x, x0) - 1;
    clip->y = MAX(a->y, y0) - 1;
    clip->w = MIN(a->x + a->w, x1) - clip->x+ 2;
    clip->h = MIN(a->y + a->h, y1) - clip->y + 2;
    clip->w = MAX(0, clip->w);
    clip->h = MAX(0, clip->h);
}

static void
gui_triangle_from_direction(struct gui_vec2 *result, gui_float x, gui_float y,
    gui_float w, gui_float h, gui_float pad_x, gui_float pad_y,
    enum gui_heading direction)
{
    gui_float w_half, h_half;
    GUI_ASSERT(result);

    w = MAX(4 * pad_x, w);
    h = MAX(4 * pad_y, h);
    w = w - 2 * pad_x;
    h = h - 2 * pad_y;

    x = x + pad_x;
    y = y + pad_y;

    w_half = w / 2.0f;
    h_half = h / 2.0f;

    if (direction == GUI_UP) {
        result[0] = gui_vec2(x + w_half, y);
        result[1] = gui_vec2(x, y + h);
        result[2] = gui_vec2(x + w, y + h);
    } else if (direction == GUI_RIGHT) {
        result[0] = gui_vec2(x, y);
        result[1] = gui_vec2(x, y + h);
        result[2] = gui_vec2(x + w, y + h_half);
    } else if (direction == GUI_DOWN) {
        result[0] = gui_vec2(x, y);
        result[1] = gui_vec2(x + w_half, y + h);
        result[2] = gui_vec2(x + w, y);
    } else {
        result[0] = gui_vec2(x, y + h_half);
        result[1] = gui_vec2(x + w, y + h);
        result[2] = gui_vec2(x + w, y);
    }
}

/*
 * ==============================================================
 *
 *                          Input
 *
 * ===============================================================
 */
static gui_size
gui_utf_validate(long *u, gui_size i)
{
    if (!u) return 0;
    if (!GUI_BETWEEN(*u, gui_utfmin[i], gui_utfmax[i]) ||
         GUI_BETWEEN(*u, 0xD800, 0xDFFF))
            *u = GUI_UTF_INVALID;
    for (i = 1; *u > gui_utfmax[i]; ++i);
    return i;
}

static gui_long
gui_utf_decode_byte(gui_char c, gui_size *i)
{
    if (!i) return 0;
    for(*i = 0; *i < GUI_LEN(gui_utfmask); ++(*i)) {
        if ((c & gui_utfmask[*i]) == gui_utfbyte[*i])
            return c & ~gui_utfmask[*i];
    }
    return 0;
}

static gui_size
gui_utf_decode(const gui_char *c, gui_long *u, gui_size clen)
{
    gui_size i, j, len, type=0;
    gui_long udecoded;

    *u = GUI_UTF_INVALID;
    if (!c || !u) return 0;
    if (!clen) return 0;
    udecoded = gui_utf_decode_byte(c[0], &len);
    if (!GUI_BETWEEN(len, 1, GUI_UTF_SIZE))
        return 1;

    for (i = 1, j = 1; i < clen && j < len; ++i, ++j) {
        udecoded = (udecoded << 6) | gui_utf_decode_byte(c[i], &type);
        if (type != 0)
            return j;
    }
    if (j < len)
        return 0;
    *u = udecoded;
    gui_utf_validate(u, len);
    return len;
}

static gui_char
gui_utf_encode_byte(gui_long u, gui_size i)
{
    return (gui_char)(gui_utfbyte[i] | (u & ~gui_utfmask[i]));
}

static gui_size
gui_utf_encode(gui_long u, gui_char *c, gui_size clen)
{
    gui_size len, i;
    len = gui_utf_validate(&u, 0);
    if (clen < len || !len)
        return 0;
    for (i = len - 1; i != 0; --i) {
        c[i] = gui_utf_encode_byte(u, 0);
        u >>= 6;
    }
    c[0] = gui_utf_encode_byte(u, len);
    return len;
}

void
gui_input_begin(struct gui_input *in)
{
    gui_size i;
    GUI_ASSERT(in);
    if (!in) return;
    in->mouse_clicked = 0;
    in->text_len = 0;
    gui_vec2_mov(in->mouse_prev, in->mouse_pos);
    for (i = 0; i < GUI_KEY_MAX; i++)
        in->keys[i].clicked = 0;
}

void
gui_input_motion(struct gui_input *in, gui_int x, gui_int y)
{
    GUI_ASSERT(in);
    if (!in) return;
    in->mouse_pos.x = (gui_float)x;
    in->mouse_pos.y = (gui_float)y;
}

void
gui_input_key(struct gui_input *in, enum gui_keys key, gui_bool down)
{
    GUI_ASSERT(in);
    if (!in) return;
    if (in->keys[key].down == down) return;
    in->keys[key].down = down;
    in->keys[key].clicked++;
}

void
gui_input_button(struct gui_input *in, gui_int x, gui_int y, gui_bool down)
{
    GUI_ASSERT(in);
    if (!in) return;
    if (in->mouse_down == down) return;
    in->mouse_clicked_pos.x = (gui_float)x;
    in->mouse_clicked_pos.y = (gui_float)y;
    in->mouse_down = down;
    in->mouse_clicked++;
}

void
gui_input_char(struct gui_input *in, const gui_glyph glyph)
{
    gui_size len = 0;
    gui_long unicode;
    GUI_ASSERT(in);
    if (!in) return;
    len = gui_utf_decode(glyph, &unicode, GUI_UTF_SIZE);
    if (len && ((in->text_len + len) < GUI_INPUT_MAX)) {
        gui_utf_encode(unicode, &in->text[in->text_len], GUI_INPUT_MAX - in->text_len);
        in->text_len += len;
    }
}

void
gui_input_end(struct gui_input *in)
{
    GUI_ASSERT(in);
    if (!in) return;
    gui_vec2_sub(in->mouse_delta, in->mouse_pos, in->mouse_prev);
}

/*
 * ==============================================================
 *
 *                          Buffer
 *
 * ===============================================================
 */
void
gui_buffer_init(struct gui_buffer *b, const struct gui_allocator *a,
    gui_size initial_size, gui_float grow_factor)
{
    GUI_ASSERT(buffer);
    GUI_ASSERT(memory);
    GUI_ASSERT(initial_size);
    if (!b || !a || !initial_size) return;

    gui_zero(b, sizeof(*b));
    b->type = GUI_BUFFER_DYNAMIC;
    b->memory.ptr = a->alloc(a->userdata, initial_size);
    b->memory.size = initial_size;
    b->grow_factor = grow_factor;
    b->pool = *a;
}

void
gui_buffer_init_fixed(struct gui_buffer *b, void *m, gui_size size)
{
    GUI_ASSERT(b);
    GUI_ASSERT(m);
    GUI_ASSERT(size);
    if (!b || !m || !size) return;

    gui_zero(b, sizeof(*b));
    b->type = GUI_BUFFER_FIXED;
    b->memory.ptr = m;
    b->memory.size = size;
}

void*
gui_buffer_alloc(struct gui_buffer *b, gui_size size, gui_size align)
{
    gui_size cap;
    gui_size alignment;
    void *unaligned;
    void *memory;

    GUI_ASSERT(b);
    if (!b || !size) return NULL;
    b->needed += size;

    unaligned = gui_ptr_add(void, b->memory.ptr, b->allocated);
    memory = GUI_ALIGN_PTR(unaligned, align);
    alignment = (gui_size)((gui_byte*)memory - (gui_byte*)unaligned);

    if ((b->allocated + size + alignment) >= b->memory.size) {
        void *temp;
        if (b->type != GUI_BUFFER_DYNAMIC || !b->pool.realloc)
            return NULL;

        cap = (gui_size)((gui_float)b->memory.size * b->grow_factor);
        temp = b->pool.realloc(b->pool.userdata, b->memory.ptr, cap);
        if (!temp) return NULL;

        b->memory.ptr = temp;
        b->memory.size = cap;
        unaligned = gui_ptr_add(gui_byte, b->memory.ptr, b->allocated);
        memory = GUI_ALIGN_PTR(unaligned, align);
        alignment = (gui_size)((gui_byte*)memory - (gui_byte*)unaligned);
    }

    b->allocated += size + alignment;
    b->needed += alignment;
    b->calls++;
    return memory;
}

void
gui_buffer_reset(struct gui_buffer *b)
{
    GUI_ASSERT(b);
    if (!b) return;
    b->allocated = 0;
    b->calls = 0;
}

void
gui_buffer_clear(struct gui_buffer *b)
{
    GUI_ASSERT(buffer);
    if (!b || !b->memory.ptr) return;
    if (b->type == GUI_BUFFER_FIXED) return;
    if (!b->pool.free) return;
    b->pool.free(b->pool.userdata, b->memory.ptr);
}

void
gui_buffer_info(struct gui_memory_status *s, struct gui_buffer *b)
{
    GUI_ASSERT(b);
    GUI_ASSERT(s);
    if (!s || !b) return;
    s->allocated = b->allocated;
    s->size =  b->memory.size;
    s->needed = b->needed;
    s->memory = b->memory.ptr;
    s->calls = b->calls;
}

/*
 * ==============================================================
 *
 *                      Command buffer
 *
 * ===============================================================
 */
void*
gui_command_buffer_push(struct gui_command_buffer* b,
    enum gui_command_type t, gui_size size)
{
    static const gui_size align = GUI_ALIGNOF(struct gui_command);
    struct gui_command *cmd;
    gui_size alignment;
    void *unaligned;
    void *memory;
    GUI_ASSERT(b);
    if (!b) return NULL;

    cmd = gui_buffer_alloc(&b->base, size, align);
    if (!cmd) return NULL;

    unaligned = (gui_byte*)cmd + size;
    memory = GUI_ALIGN_PTR(unaligned, align);
    alignment = (gui_size)((gui_byte*)memory - (gui_byte*)unaligned);

    cmd->type = t;
    cmd->offset = size + alignment;
    return cmd;
}

void
gui_command_buffer_push_scissor(struct gui_command_buffer *b, gui_float x, gui_float y,
    gui_float w, gui_float h)
{
    struct gui_command_scissor *cmd;
    GUI_ASSERT(b);
    if (!b) return;

    b->clip.x = x;
    b->clip.y = y;
    b->clip.w = w;
    b->clip.h = h;

    cmd = gui_command_buffer_push(b, GUI_COMMAND_SCISSOR, sizeof(*cmd));
    if (!cmd) return;
    cmd->x = (gui_short)x;
    cmd->y = (gui_short)y;
    cmd->w = (gui_ushort)MAX(0, w);
    cmd->h = (gui_ushort)MAX(0, h);
}

void
gui_command_buffer_push_line(struct gui_command_buffer *b, gui_float x0, gui_float y0,
    gui_float x1, gui_float y1, struct gui_color c)
{
    struct gui_command_line *cmd;
    GUI_ASSERT(buffer);
    if (!b) return;

    cmd = gui_command_buffer_push(b, GUI_COMMAND_LINE, sizeof(*cmd));
    if (!cmd) return;
    cmd->begin[0] = (gui_short)x0;
    cmd->begin[1] = (gui_short)y0;
    cmd->end[0] = (gui_short)x1;
    cmd->end[1] = (gui_short)y1;
    gui_color_to_array(cmd->color, c);
}

void
gui_command_buffer_push_rect(struct gui_command_buffer *b, gui_float x, gui_float y,
    gui_float w, gui_float h, gui_float rounding, struct gui_color c)
{
    struct gui_command_rect *cmd;
    GUI_ASSERT(buffer);
    if (!b) return;

    if (b->use_clipping) {
        const struct gui_rect *clip = &b->clip;
        if (!GUI_INTERSECT(x, y, w, h, clip->x, clip->y, clip->w, clip->h))
            return;
    }

    cmd = gui_command_buffer_push(b, GUI_COMMAND_RECT, sizeof(*cmd));
    if (!cmd) return;
    cmd->r = (gui_uint)rounding;
    cmd->x = (gui_short)x;
    cmd->y = (gui_short)y;
    cmd->w = (gui_ushort)MAX(0, w);
    cmd->h = (gui_ushort)MAX(0, h);
    gui_color_to_array(cmd->color, c);
}

void
gui_command_buffer_push_circle(struct gui_command_buffer *b, gui_float x, gui_float y,
    gui_float w, gui_float h, struct gui_color c)
{
    struct gui_command_circle *cmd;
    GUI_ASSERT(b);
    if (!b) return;

    if (b->use_clipping) {
        const struct gui_rect *clip = &b->clip;
        if (!GUI_INTERSECT(x, y, w, h, clip->x, clip->y, clip->w, clip->h))
            return;
    }

    cmd = gui_command_buffer_push(b, GUI_COMMAND_CIRCLE, sizeof(*cmd));
    if (!cmd) return;
    cmd->x = (gui_short)x;
    cmd->y = (gui_short)y;
    cmd->w = (gui_ushort)MAX(w, 0);
    cmd->h = (gui_ushort)MAX(h, 0);
    gui_color_to_array(cmd->color, c);
}

void
gui_command_buffer_push_triangle(struct gui_command_buffer *b, gui_float x0, gui_float y0,
    gui_float x1, gui_float y1, gui_float x2, gui_float y2, struct gui_color c)
{
    struct gui_command_triangle *cmd;
    GUI_ASSERT(buffer);
    if (!b) return;

    if (b->use_clipping) {
        const struct gui_rect *clip = &b->clip;
        if (!GUI_INBOX(x0, y0, clip->x, clip->y, clip->w, clip->h) ||
            !GUI_INBOX(x1, y1, clip->x, clip->y, clip->w, clip->h) ||
            !GUI_INBOX(x2, y2, clip->x, clip->y, clip->w, clip->h))
            return;
    }

    cmd = gui_command_buffer_push(b, GUI_COMMAND_TRIANGLE, sizeof(*cmd));
    if (!cmd) return;
    cmd->a[0] = (gui_short)x0;
    cmd->a[1] = (gui_short)y0;
    cmd->b[0] = (gui_short)x1;
    cmd->b[1] = (gui_short)y1;
    cmd->c[0] = (gui_short)x2;
    cmd->c[1] = (gui_short)y2;
    gui_color_to_array(cmd->color, c);
}

void
gui_command_buffer_push_image(struct gui_command_buffer *b, gui_float x, gui_float y,
    gui_float w, gui_float h, gui_handle img)
{
    struct gui_command_image *cmd;
    GUI_ASSERT(b);
    if (!b) return;

    if (b->use_clipping) {
        const struct gui_rect *c = &b->clip;
        if (!GUI_INTERSECT(x, y, w, h, c->x, c->y, c->w, c->h))
            return;
    }

    cmd = gui_command_buffer_push(b, GUI_COMMAND_IMAGE, sizeof(*cmd));
    if (!cmd) return;
    cmd->x = (gui_short)x;
    cmd->y = (gui_short)y;
    cmd->w = (gui_ushort)MAX(0, w);
    cmd->h = (gui_ushort)MAX(0, h);
    cmd->img = img;
}

void
gui_command_buffer_push_text(struct gui_command_buffer *b, gui_float x, gui_float y,
    gui_float w, gui_float h, const gui_char *string, gui_size length,
    const struct gui_font *font, struct gui_color bg, struct gui_color fg)
{
    struct gui_command_text *cmd;
    GUI_ASSERT(buffer);
    GUI_ASSERT(font);
    if (!b || !string || !length) return;

    if (b->use_clipping) {
        const struct gui_rect *c = &b->clip;
        if (!GUI_INTERSECT(x, y, w, h, c->x, c->y, c->w, c->h))
            return;
    }

    cmd = gui_command_buffer_push(b, GUI_COMMAND_TEXT, sizeof(*cmd) + length + 1);
    if (!cmd) return;
    cmd->x = (gui_short)x;
    cmd->y = (gui_short)y;
    cmd->w = (gui_ushort)w;
    cmd->h = (gui_ushort)h;
    gui_color_to_array(cmd->bg, bg);
    gui_color_to_array(cmd->fg, fg);
    cmd->font = font->userdata;
    cmd->length = length;
    gui_memcopy(cmd->string, string, length);
    cmd->string[length] = '\0';
}

/*
 * ==============================================================
 *
 *                          Widgets
 *
 * ===============================================================
 */
void
gui_text(struct gui_command_buffer *o, gui_float x, gui_float y, gui_float w, gui_float h,
    const char *string, gui_size len, const struct gui_text *t,
    enum gui_text_align a, const struct gui_font *f)
{
    struct gui_rect label;
    gui_size text_width;

    GUI_ASSERT(o);
    GUI_ASSERT(t);
    if (!o || !t) return;

    label.x = 0; label.w = 0;
    label.y = y + t->padding.y;
    label.h = MAX(0, h - 2 * t->padding.y);

    text_width = f->width(f->userdata, (const gui_char*)string, len);
    if (a == GUI_TEXT_LEFT) {
        label.x = x + t->padding.x;
        label.w = MAX(0, w - 2 * t->padding.x);
    } else if (a == GUI_TEXT_CENTERED) {
        label.w = 2 * t->padding.x + (gui_float)text_width;
        label.x = (x + t->padding.x + ((w - 2 * t->padding.x)/2)) - (label.w/2);
        label.x = MAX(x + t->padding.x, label.x);
        label.w = MIN(x + w, label.x + label.w) - label.x;
    } else if (a == GUI_TEXT_RIGHT) {
        label.x = MAX(x, (x + w) - (2 * t->padding.x + (gui_float)text_width));
        label.w = (gui_float)text_width + 2 * t->padding.x;
    } else return;

    gui_command_buffer_push_rect(o, x, y, w, h, 0, t->background);
    gui_command_buffer_push_text(o, label.x, label.y, label.w, label.h,
        (const gui_char*)string, len, f, t->background, t->foreground);
}

static gui_bool
gui_do_button(struct gui_command_buffer *o, gui_float x, gui_float y, gui_float w,
    gui_float h, const struct gui_button *b, const struct gui_input *i,
    enum gui_button_behavior behavior)
{
    gui_bool ret = gui_false;
    struct gui_color background;

    GUI_ASSERT(o);
    GUI_ASSERT(b);
    if (!o || !b)
        return gui_false;

    background = b->background;
    if (i && GUI_INBOX(i->mouse_pos.x, i->mouse_pos.y, x, y, w, h)) {
        background = b->highlight;
        if (GUI_INBOX(i->mouse_clicked_pos.x, i->mouse_clicked_pos.y, x, y, w, h)) {
            ret = (behavior != GUI_BUTTON_DEFAULT) ? i->mouse_down:
                (i->mouse_down && i->mouse_clicked);
        }
    }

    gui_command_buffer_push_rect(o, x, y, w, h, b->rounding, b->foreground);
    gui_command_buffer_push_rect(o, x + b->border, y + b->border,
        w - 2 * b->border, h - 2 * b->border, b->rounding, background);
    return ret;
}

gui_bool
gui_button_text(struct gui_command_buffer *o, gui_float x, gui_float y,
    gui_float w, gui_float h, const char *string, enum gui_button_behavior behavior,
    const struct gui_button *b, const struct gui_input *i,
    const struct gui_font *f)
{
    gui_bool ret = gui_false;
    gui_float button_w, button_h;
    struct gui_text t;
    struct gui_color font_color;
    struct gui_color bg_color;
    struct gui_rect inner;

    GUI_ASSERT(b);
    GUI_ASSERT(o);
    GUI_ASSERT(string);
    if (!o || !b)
        return gui_false;

    font_color = b->content;
    bg_color = b->background;
    button_w = MAX(w, 2 * b->padding.x);
    button_h = MAX(h, f->height + 2 * b->padding.y);
    if (i && GUI_INBOX(i->mouse_pos.x, i->mouse_pos.y, x, y, button_w, button_h)) {
        font_color = b->highlight_content;
        bg_color = b->highlight;
    }
    ret = gui_do_button(o, x, y, button_w, button_h, b, i, behavior);

    inner.x = x + b->border + b->rounding;
    inner.y = y + b->border;
    inner.w = button_w - (2 * b->border + 2 * b->rounding);
    inner.h = button_h - (2 * b->border);

    t.padding.x = b->padding.x;
    t.padding.y = b->padding.y;
    t.background = bg_color;
    t.foreground = font_color;
    gui_text(o, inner.x, inner.y, inner.w, inner.h, string, gui_strsiz(string),
        &t, GUI_TEXT_CENTERED, f);
    return ret;
}

gui_bool
gui_button_triangle(struct gui_command_buffer *out, gui_float x, gui_float y,
    gui_float w, gui_float h, enum gui_heading heading, enum gui_button_behavior bh,
    const struct gui_button *b, const struct gui_input *in)
{
    gui_bool pressed;
    struct gui_color col;
    struct gui_vec2 points[3];

    GUI_ASSERT(b);
    GUI_ASSERT(out);
    if (!out || !b)
        return gui_false;

    pressed = gui_do_button(out, x, y, w, h, b, in, bh);
    gui_triangle_from_direction(points, x, y, w, h, b->padding.x, b->padding.y, heading);
    col = (in && GUI_INBOX(in->mouse_pos.x, in->mouse_pos.y, x, y, w, h)) ?
        b->highlight_content : b->content;
    gui_command_buffer_push_triangle(out,  points[0].x, points[0].y,
        points[1].x, points[1].y, points[2].x, points[2].y, col);
    return pressed;
}

gui_bool
gui_button_image(struct gui_command_buffer *out, gui_float x, gui_float y,
    gui_float w, gui_float h, gui_handle img, enum gui_button_behavior b,
    const struct gui_button *button, const struct gui_input *in)
{
    gui_bool pressed;
    gui_float img_x, img_y, img_w, img_h;

    GUI_ASSERT(button);
    GUI_ASSERT(out);
    if (!out || !button)
        return gui_false;

    pressed = gui_do_button(out, x, y, w, h, button, in, b);
    img_x = x + button->padding.x;
    img_y = y + button->padding.y;
    img_w = w - 2 * button->padding.x;
    img_h = h - 2 * button->padding.y;
    gui_command_buffer_push_image(out, img_x, img_y, img_w, img_h, img);
    return pressed;
}

gui_bool
gui_toggle(struct gui_command_buffer *out, gui_float x, gui_float y, gui_float w,
    gui_float h, gui_bool active, const char *string, enum gui_toggle_type type,
    const struct gui_toggle *toggle, const struct gui_input *in,
    const struct gui_font *font)
{
    gui_bool toggle_active;
    gui_float select_size;
    gui_float toggle_w, toggle_h;
    gui_float select_x, select_y;
    gui_float cursor_x, cursor_y;
    gui_float cursor_pad, cursor_size;

    GUI_ASSERT(toggle);
    GUI_ASSERT(out);
    if (!out || !toggle)
        return 0;

    toggle_w = MAX(w, font->height + 2 * toggle->padding.x);
    toggle_h = MAX(h, font->height + 2 * toggle->padding.y);
    toggle_active = active;

    select_x = x + toggle->padding.x;
    select_y = y + toggle->padding.y;
    select_size = font->height + 2 * toggle->padding.y;

    cursor_pad = (type == GUI_TOGGLE_OPTION) ?
        (gui_float)(gui_int)(select_size / 4):
        (gui_float)(gui_int)(select_size / 8);
    cursor_size = select_size - cursor_pad * 2;
    cursor_x = select_x + cursor_pad;
    cursor_y = select_y + cursor_pad;

    if (in && !in->mouse_down && in->mouse_clicked)
        if (GUI_INBOX(in->mouse_clicked_pos.x, in->mouse_clicked_pos.y,
                cursor_x, cursor_y, cursor_size, cursor_size))
                toggle_active = !toggle_active;

    if (type == GUI_TOGGLE_CHECK)
        gui_command_buffer_push_rect(out, select_x, select_y, select_size,
           select_size, toggle->rounding, toggle->foreground);
    else gui_command_buffer_push_circle(out, select_x, select_y, select_size,
            select_size, toggle->foreground);

    if (toggle_active) {
        if (type == GUI_TOGGLE_CHECK)
            gui_command_buffer_push_rect(out, cursor_x, cursor_y, cursor_size,
                cursor_size, toggle->rounding, toggle->cursor);
        else gui_command_buffer_push_circle(out, cursor_x, cursor_y,
                cursor_size, cursor_size, toggle->cursor);
    }

    if (font && string) {
        struct gui_text text;
        gui_float inner_x, inner_y;
        gui_float inner_w, inner_h;

        inner_x = x + select_size + toggle->padding.x * 2;
        inner_y = (y + (select_size / 2)) - (font->height / 2);
        inner_w = (x + toggle_w) - (inner_x + toggle->padding.x);
        inner_h = (y + toggle_h) - (inner_y + toggle->padding.y);

        text.padding.x = 0;
        text.padding.y = 0;
        text.background = toggle->background;
        text.foreground = toggle->font;
        gui_text(out, inner_x, inner_y, inner_w, inner_h, string, gui_strsiz(string),
            &text, GUI_TEXT_LEFT, font);
    }
    return toggle_active;
}

gui_float
gui_slider(struct gui_command_buffer *out, gui_float x, gui_float y, gui_float w, gui_float h,
    gui_float min, gui_float val, gui_float max, gui_float step,
    const struct gui_slider *s, const struct gui_input *in)
{
    gui_float slider_range;
    gui_float slider_min, slider_max;
    gui_float slider_value, slider_steps;
    gui_float slider_w, slider_h;
    gui_float cursor_offset;
    struct gui_rect cursor;
    struct gui_rect bar;

    GUI_ASSERT(s);
    GUI_ASSERT(out);
    if (!out || !s)
        return 0;

    slider_w = MAX(w, 2 * s->padding.x);
    slider_h = MAX(h, 2 * s->padding.y);
    slider_max = MAX(min, max);
    slider_min = MIN(min, max);
    slider_value = CLAMP(slider_min, val, slider_max);
    slider_range = slider_max - slider_min;
    slider_steps = slider_range / step;

    cursor_offset = (slider_value - slider_min) / step;
    cursor.w = (slider_w - 2 * s->padding.x) / (slider_steps + 1);
    cursor.h = slider_h - 2 * s->padding.y;
    cursor.x = x + s->padding.x + (cursor.w * cursor_offset);
    cursor.y = y + s->padding.y;

    bar.x = x + s->padding.x;
    bar.y = (cursor.y + cursor.h/2) - cursor.h/8;
    bar.w = slider_w - 2 * s->padding.x;
    bar.h = cursor.h/4;

    if (in && in->mouse_down &&
        GUI_INBOX(in->mouse_pos.x, in->mouse_pos.y, x, y, slider_w, slider_h) &&
        GUI_INBOX(in->mouse_clicked_pos.x,in->mouse_clicked_pos.y, x, y, slider_w, slider_h))
    {
        const float d = in->mouse_pos.x - (cursor.x + cursor.w / 2.0f);
        const float pxstep = (slider_w - 2 * s->padding.x) / slider_steps;
        if (GUI_ABS(d) >= pxstep) {
            const gui_float steps = (gui_float)((gui_int)(GUI_ABS(d) / pxstep));
            slider_value += (d > 0) ? (step * steps) : -(step * steps);
            slider_value = CLAMP(slider_min, slider_value, slider_max);
            cursor.x = x + s->padding.x + (cursor.w * (slider_value - slider_min));
        }
    }

    gui_command_buffer_push_rect(out, x, y, slider_w, slider_h, 0, s->bg);
    gui_command_buffer_push_rect(out, bar.x, bar.y, bar.w, bar.h,0, s->bar);
    if (s->cursor == GUI_SLIDER_RECT) {
        gui_command_buffer_push_rect(out,cursor.x, cursor.y, cursor.w, cursor.h,0, s->border);
        gui_command_buffer_push_rect(out,cursor.x+1,cursor.y+1,cursor.w-2,cursor.h-2,0, s->fg);
    } else {
        gui_float c_pos = cursor.x - cursor.h/4;
        gui_command_buffer_push_circle(out,c_pos,cursor.y,cursor.h,cursor.h,s->border);
        gui_command_buffer_push_circle(out,c_pos + 1,cursor.y+1,cursor.h-2,cursor.h-2,s->fg);
    }
    return slider_value;
}

gui_size
gui_progress(struct gui_command_buffer *out, gui_float x, gui_float y,
    gui_float w, gui_float h, gui_size value, gui_size max, gui_bool modifyable,
    const struct gui_progress *prog, const struct gui_input *in)
{
    gui_float cursor_x, cursor_y;
    gui_float cursor_w, cursor_h;
    gui_float prog_w, prog_h;
    gui_float prog_scale;
    gui_size prog_value;

    GUI_ASSERT(prog);
    GUI_ASSERT(out);
    if (!out || !prog) return 0;

    prog_w = MAX(w, 2 * prog->padding.x + 1);
    prog_h = MAX(h, 2 * prog->padding.y + 1);
    prog_value = MIN(value, max);

    if (in && modifyable && in->mouse_down &&
        GUI_INBOX(in->mouse_pos.x, in->mouse_pos.y, x, y, prog_w, prog_h)){
        gui_float ratio = (gui_float)(in->mouse_pos.x - x) / (gui_float)prog_w;
        prog_value = (gui_size)((gui_float)max * ratio);
    }

    if (!max) return prog_value;
    prog_value = MIN(prog_value, max);
    prog_scale = (gui_float)prog_value / (gui_float)max;

    cursor_h = prog_h - 2 * prog->padding.y;
    cursor_w = (prog_w - 2 * prog->padding.x) * prog_scale;
    cursor_x = x + prog->padding.x;
    cursor_y = y + prog->padding.y;

    gui_command_buffer_push_rect(out, x, y, prog_w, prog_h,prog->rounding, prog->background);
    gui_command_buffer_push_rect(out, cursor_x, cursor_y, cursor_w, cursor_h,
        prog->rounding, prog->foreground);
    return prog_value;
}

static gui_bool
gui_filter_input_default(gui_long unicode)
{
    (void)unicode;
    return gui_true;
}

static gui_bool
gui_filter_input_ascii(gui_long unicode)
{
    if (unicode < 0 || unicode > 128)
        return gui_false;
    return gui_true;
}

static gui_bool
gui_filter_input_float(gui_long unicode)
{
    if ((unicode < '0' || unicode > '9') && unicode != '.' && unicode != '-')
        return gui_false;
    return gui_true;
}

static gui_bool
gui_filter_input_decimal(gui_long unicode)
{
    if (unicode < '0' || unicode > '9')
        return gui_false;
    return gui_true;
}

static gui_bool
gui_filter_input_hex(gui_long unicode)
{
    if ((unicode < '0' || unicode > '9') &&
        (unicode < 'a' || unicode > 'f') &&
        (unicode < 'A' || unicode > 'B'))
        return gui_false;
    return gui_true;
}

static gui_bool
gui_filter_input_oct(gui_long unicode)
{
    if (unicode < '0' || unicode > '7')
        return gui_false;
    return gui_true;
}

static gui_bool
gui_filter_input_binary(gui_long unicode)
{
    if (unicode < '0' || unicode > '1')
        return gui_false;
    return gui_true;
}

static gui_size
gui_buffer_input(gui_char *buffer, gui_size length, gui_size max,
    gui_filter filter, const struct gui_input *i)
{
    gui_long unicode;
    gui_size src_len = 0;
    gui_size text_len = 0, glyph_len = 0;
    GUI_ASSERT(buffer);
    GUI_ASSERT(i);

    glyph_len = gui_utf_decode(i->text, &unicode, i->text_len);
    while (glyph_len && ((text_len+glyph_len) <= i->text_len) && (length+src_len)<max) {
        if (filter(unicode)) {
            gui_size n = 0;
            for (n = 0; n < glyph_len; n++)
                buffer[length++] = i->text[text_len + n];
            text_len += glyph_len;
        }
        src_len = src_len + glyph_len;
        glyph_len = gui_utf_decode(i->text + src_len, &unicode, i->text_len - src_len);
    }
    return text_len;
}

gui_size
gui_edit_filtered(struct gui_command_buffer *out, gui_float x, gui_float y, gui_float w,
    gui_float h, gui_char *buffer, gui_size len, gui_size max, gui_bool *active,
    const struct gui_edit *field, gui_filter filter, const struct gui_input *in,
    const struct gui_font *font)
{
    gui_float input_w, input_h;
    gui_bool input_active;

    GUI_ASSERT(out);
    GUI_ASSERT(buffer);
    GUI_ASSERT(field);
    if (!out || !buffer || !field)
        return 0;

    input_w = MAX(w, 2 * field->padding.x);
    input_h = MAX(h, font->height);
    input_active = *active;

    gui_command_buffer_push_rect(out, x, y, input_w, input_h,field->rounding, field->border);
    gui_command_buffer_push_rect(out, x + field->border_size, y + field->border_size,
        input_w - 2 * field->border_size, input_h - 2 * field->border_size,
        field->rounding, field->background);
    if (in && in->mouse_clicked && in->mouse_down)
        input_active = GUI_INBOX(in->mouse_pos.x, in->mouse_pos.y, x, y, input_w, input_h);

    if (input_active && in) {
        const struct gui_key *bs = &in->keys[GUI_KEY_BACKSPACE];
        const struct gui_key *del = &in->keys[GUI_KEY_DEL];
        const struct gui_key *esc = &in->keys[GUI_KEY_ESCAPE];
        const struct gui_key *enter = &in->keys[GUI_KEY_ENTER];
        const struct gui_key *space = &in->keys[GUI_KEY_SPACE];

        if ((del->down && del->clicked) || (bs->down && bs->clicked))
            if (len > 0) len = len - 1;
        if ((enter->down && enter->clicked) || (esc->down && esc->clicked))
            input_active = gui_false;
        if ((space->down && space->clicked) && (len < max))
            buffer[len++] = ' ';
        if (in->text_len && len < max)
            len += gui_buffer_input(buffer, len, max, filter, in);
    }

    if (font && buffer) {
        gui_size offset = 0;
        gui_float label_x, label_y, label_h;
        gui_float label_w = input_w - 2 * field->padding.x - 2 * field->border_size;
        gui_size cursor_w =(gui_size)font->width(font->userdata,(const gui_char*)"X", 1);

        gui_size text_len = len;
        gui_size text_width = font->width(font->userdata, buffer, text_len);
        while (text_len && (text_width + cursor_w) > (gui_size)label_w) {
            gui_long unicode;
            offset += gui_utf_decode(&buffer[offset], &unicode, text_len);
            text_len = len - offset;
            text_width = font->width(font->userdata, &buffer[offset], text_len);
        }

        label_x = x + field->padding.x + field->border_size;
        label_y = y + field->padding.y + field->border_size;
        label_h = input_h - (2 * field->padding.y + 2 * field->border_size);
        gui_command_buffer_push_text(out , label_x, label_y, label_w, label_h,
            &buffer[offset], text_len, font, field->background, field->text);

        if (input_active && field->show_cursor) {
            gui_command_buffer_push_rect(out, label_x + (gui_float)text_width, label_y,
                (gui_float)cursor_w, label_h,0, field->cursor);
        }
    }
    *active = input_active;
    return len;

}

gui_size
gui_edit(struct gui_command_buffer *out, gui_float x, gui_float y, gui_float w,
    gui_float h, gui_char *buffer, gui_size len, gui_size max, gui_bool *active,
    const struct gui_edit *field, enum gui_input_filter f,
    const struct gui_input *in, const struct gui_font *font)
{
    static const gui_filter filter[] = {
        gui_filter_input_default,
        gui_filter_input_ascii,
        gui_filter_input_float,
        gui_filter_input_decimal,
        gui_filter_input_hex,
        gui_filter_input_oct,
        gui_filter_input_binary,
    };
    return gui_edit_filtered(out, x, y, w, h, buffer, len, max, active,
                field,filter[f], in, font);
}

gui_float
gui_scroll(struct gui_command_buffer *out, gui_float x, gui_float y,
    gui_float w, gui_float h, gui_float offset, gui_float target,
    gui_float step, const struct gui_scroll *s, const struct gui_input *i)
{
    gui_bool button_up_pressed;
    gui_bool button_down_pressed;
    struct gui_button button;

    gui_float scroll_step;
    gui_float scroll_offset;
    gui_float scroll_off, scroll_ratio;
    gui_float scroll_y, scroll_w, scroll_h;

    gui_float cursor_x, cursor_y;
    gui_float cursor_w, cursor_h;
    gui_bool inscroll, incursor;

    GUI_ASSERT(out);
    GUI_ASSERT(s);
    if (!out || !s) return 0;

    scroll_w = MAX(w, 0);
    scroll_h = MAX(h, 2 * scroll_w);
    gui_command_buffer_push_rect(out , x, y, scroll_w, scroll_h, s->rounding, s->background);
    if (target <= scroll_h) return 0;

    button.border = 1;
    button.rounding = 0;
    button.padding.x = scroll_w / 4;
    button.padding.y = scroll_w / 4;
    button.background = s->background;
    button.foreground =  s->foreground;
    button.content = s->foreground;
    button.highlight = s->background;
    button.highlight_content = s->foreground;

    button_up_pressed = gui_button_triangle(out, x, y, scroll_w, scroll_w,
         GUI_UP, GUI_BUTTON_DEFAULT, &button, i);
    button_down_pressed = gui_button_triangle(out, x, y + scroll_h - scroll_w,
        scroll_w, scroll_w, GUI_DOWN, GUI_BUTTON_DEFAULT, &button, i);

    scroll_h = scroll_h - 2 * scroll_w;
    scroll_y = y + scroll_w;
    scroll_step = MIN(step, scroll_h);
    scroll_offset = MIN(offset, target - scroll_h);
    scroll_ratio = scroll_h / target;
    scroll_off = scroll_offset / target;

    cursor_h = scroll_ratio * scroll_h;
    cursor_y = scroll_y + (scroll_off * scroll_h);
    cursor_w = scroll_w;
    cursor_x = x;

    if (i) {
        const struct gui_vec2 mouse_pos = i->mouse_pos;
        const struct gui_vec2 mouse_prev = i->mouse_prev;
        inscroll = GUI_INBOX(mouse_pos.x,mouse_pos.y, x, y, scroll_w, scroll_h);
        incursor = GUI_INBOX(mouse_prev.x,mouse_prev.y,cursor_x,cursor_y,cursor_w, cursor_h);
        if (i->mouse_down && inscroll && incursor) {
            /* update cursor over mouse dragging */
            const gui_float pixel = i->mouse_delta.y;
            const gui_float delta =  (pixel / scroll_h) * target;
            scroll_offset = CLAMP(0, scroll_offset + delta, target - scroll_h);
            cursor_y += pixel;
        } else if (button_up_pressed || button_down_pressed) {
            /* update cursor over up/down button */
            scroll_offset = (button_down_pressed) ?
                MIN(scroll_offset + scroll_step, target - scroll_h):
                MAX(0, scroll_offset - scroll_step);
            scroll_off = scroll_offset / target;
            cursor_y = scroll_y + (scroll_off * scroll_h);
        }
    }

    gui_command_buffer_push_rect(out, cursor_x, cursor_y, cursor_w,
        cursor_h, s->rounding, s->foreground);
    return scroll_offset;
}

gui_int
gui_spinner(struct gui_command_buffer *out, gui_float x, gui_float y, gui_float w,
    gui_float h, const struct gui_spinner *s, gui_int min, gui_int value,
    gui_int max, gui_int step, gui_bool *active, const struct gui_input *in,
    const struct gui_font *font)
{
    char string[GUI_MAX_NUMBER_BUFFER];
    gui_size len, old_len;
    gui_bool is_active;

    struct gui_button button;
    gui_float button_x, button_y;
    gui_float button_w, button_h;
    gui_bool button_up_clicked, button_down_clicked;

    struct gui_edit field;
    gui_float field_x, field_y;
    gui_float field_w, field_h;

    value = CLAMP(min, value, max);
    len = gui_itos(string, value);
    is_active = (active) ? *active : gui_false;
    old_len = len;

    button_y = y;
    button_h = h / 2;
    button_w = h - s->padding.x;
    button_x = x + w - button_w;
    button.rounding = 0;
    button.border = (gui_float)s->border_button;
    button.padding.x = MAX(3, (button_h - font->height) / 2);
    button.padding.y = MAX(3, (button_h - font->height) / 2);
    button.background = s->button_color;
    button.foreground = s->button_border;
    button.content = s->button_triangle;
    button.highlight = s->button_color;
    button.highlight_content = s->button_triangle;
    button_up_clicked = gui_button_triangle(out, button_x, button_y, button_w, button_h,
        GUI_UP, GUI_BUTTON_DEFAULT, &button, in);

    button_y = y + button_h;
    button_down_clicked = gui_button_triangle(out, button_x, button_y, button_w, button_h,
        GUI_DOWN, GUI_BUTTON_DEFAULT, &button, in);
    if (button_up_clicked || button_down_clicked) {
        value += (button_up_clicked) ? step : -step;
        value = CLAMP(min, value, max);
    }

    field_x = x;
    field_y = y;
    field_h = h;
    field_w = w - (button_w - button.border * 2);
    field.border_size = 1;
    field.rounding = 0;
    field.padding.x = s->padding.x;
    field.padding.y = s->padding.y;
    field.show_cursor = s->show_cursor;
    field.background = s->color;
    field.border = s->border;
    field.text = s->text;
    len = gui_edit(out, field_x, field_y, field_w, field_h, (gui_char*)string,
            len, GUI_MAX_NUMBER_BUFFER, &is_active, &field,GUI_INPUT_FLOAT, in, font);

    if (old_len != len)
        gui_strtoi(&value, string, len);
    if (active) *active = is_active;
    return value;
}

gui_size
gui_selector(struct gui_command_buffer *out, gui_float x, gui_float y, gui_float w,
    gui_float h, const struct gui_selector *s, const char *items[],
    gui_size item_count, gui_size item_current, const struct gui_input *input,
    const struct gui_font *font)
{
    gui_size text_len;
    gui_float label_x, label_y;
    gui_float label_w, label_h;

    struct gui_button button;
    gui_bool button_up_clicked;
    gui_bool button_down_clicked;
    gui_float button_x, button_y;
    gui_float button_w, button_h;

    GUI_ASSERT(items);
    GUI_ASSERT(item_count);
    GUI_ASSERT(item_current < item_count);

    gui_command_buffer_push_rect(out, x, y, w, h, 0, s->border);
    gui_command_buffer_push_rect(out, x+1, y+1, w-2, h-2, 0, s->color);

    button_y = y;
    button_h = h / 2;
    button_w = h - s->padding.x;
    button_x = x + w - button_w;
    button.rounding = 0;
    button.border = (gui_float)s->border_button;
    button.padding.x = MAX(3, (button_h - font->height) / 2);
    button.padding.y = MAX(3, (button_h - font->height) / 2);
    button.background = s->button_color;
    button.foreground = s->button_border;
    button.content = s->button_triangle;
    button.highlight = s->button_color;
    button.highlight_content = s->button_triangle;
    button_up_clicked = gui_button_triangle(out, button_x, button_y, button_w,
        button_h, GUI_UP, GUI_BUTTON_DEFAULT, &button, input);

    button_y = y + button_h;
    button_down_clicked = gui_button_triangle(out, button_x, button_y, button_w,
        button_h, GUI_DOWN, GUI_BUTTON_DEFAULT, &button,  input);
    item_current = (button_down_clicked && item_current > 0) ?
        item_current-1 : (button_up_clicked && item_current < item_count-1) ?
        item_current+1 : item_current;

    label_x = x + s->padding.x;
    label_y = y + s->padding.y;
    label_w = w - (button_w + 2 * s->padding.x);
    label_h = h - 2 * s->padding.y;
    text_len = gui_strsiz(items[item_current]);
    gui_command_buffer_push_text(out, label_x, label_y, label_w, label_h,
        (const gui_char*)items[item_current], text_len, font,
        s->text_bg, s->text);
    return item_current;
}

/*
 * ==============================================================
 *
 *                          Config
 *
 * ===============================================================
 */
static void
gui_config_default_properties(struct gui_config *config)
{
    config->slider_cursor = GUI_SLIDER_CIRCLE;
    config->properties[GUI_PROPERTY_SCROLLBAR_WIDTH] = gui_vec2(16, 16);
    config->properties[GUI_PROPERTY_PADDING] = gui_vec2(15.0f, 10.0f);
    config->properties[GUI_PROPERTY_SIZE] = gui_vec2(64.0f, 64.0f);
    config->properties[GUI_PROPERTY_ITEM_SPACING] = gui_vec2(10.0f, 4.0f);
    config->properties[GUI_PROPERTY_ITEM_PADDING] = gui_vec2(4.0f, 4.0f);
    config->properties[GUI_PROPERTY_SCALER_SIZE] = gui_vec2(16.0f, 16.0f);
}

static void
gui_config_default_rounding(struct gui_config *config)
{
    config->rounding[GUI_ROUNDING_PANEL] = 4.0f;
    config->rounding[GUI_ROUNDING_BUTTON] = 2.0f;
    config->rounding[GUI_ROUNDING_CHECK] = 2.0f;
    config->rounding[GUI_ROUNDING_PROGRESS] = 4.0f;
    config->rounding[GUI_ROUNDING_INPUT] = 2.0f;
    config->rounding[GUI_ROUNDING_GRAPH] = 4.0f;
    config->rounding[GUI_ROUNDING_SCROLLBAR] = 8.0f;
}

static void
gui_config_default_color(struct gui_config *config)
{
    config->colors[GUI_COLOR_TEXT] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_PANEL] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_HEADER] = gui_rgba(40, 40, 40, 255);
    config->colors[GUI_COLOR_BORDER] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_BUTTON] = gui_rgba(50, 50, 50, 255);
    config->colors[GUI_COLOR_BUTTON_HOVER] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_BUTTON_TOGGLE] = gui_rgba(75, 75, 75, 255);
    config->colors[GUI_COLOR_BUTTON_HOVER_FONT] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_BUTTON_BORDER] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_CHECK] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_CHECK_BACKGROUND] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_CHECK_ACTIVE] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_OPTION] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_OPTION_BACKGROUND] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_OPTION_ACTIVE] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_SLIDER] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_SLIDER_BAR] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_SLIDER_BORDER] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_SLIDER_CURSOR] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_PROGRESS] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_PROGRESS_CURSOR] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_INPUT] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_INPUT_CURSOR] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_INPUT_BORDER] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_INPUT_TEXT] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_SPINNER] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_SPINNER_BORDER] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_SPINNER_TRIANGLE] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_SPINNER_TEXT] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_SELECTOR] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_SELECTOR_BORDER] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_SELECTOR_TRIANGLE] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_SELECTOR_TEXT] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_HISTO] = gui_rgba(120, 120, 120, 255);
    config->colors[GUI_COLOR_HISTO_BARS] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_HISTO_NEGATIVE] = gui_rgba(255, 255, 255, 255);
    config->colors[GUI_COLOR_HISTO_HIGHLIGHT] = gui_rgba( 255, 0, 0, 255);
    config->colors[GUI_COLOR_PLOT] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_PLOT_LINES] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_PLOT_HIGHLIGHT] = gui_rgba(255, 0, 0, 255);
    config->colors[GUI_COLOR_SCROLLBAR] = gui_rgba(40, 40, 40, 255);
    config->colors[GUI_COLOR_SCROLLBAR_CURSOR] = gui_rgba(70, 70, 70, 255);
    config->colors[GUI_COLOR_SCROLLBAR_BORDER] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_TABLE_LINES] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_SHELF] = gui_rgba(45, 45, 45, 255);
    config->colors[GUI_COLOR_SHELF_TEXT] = gui_rgba(150, 150, 150, 255);
    config->colors[GUI_COLOR_SHELF_ACTIVE] = gui_rgba(30, 30, 30, 255);
    config->colors[GUI_COLOR_SHELF_ACTIVE_TEXT] = gui_rgba(100, 100, 100, 255);
    config->colors[GUI_COLOR_SCALER] = gui_rgba(100, 100, 100, 255);
}

void
gui_config_default(struct gui_config *config, gui_flags flags, const struct gui_font *font)
{
    GUI_ASSERT(config);
    if (!config) return;
    gui_zero(config, sizeof(*config));
    config->font = *font;

    if (flags & GUI_DEFAULT_COLOR)
        gui_config_default_color(config);
    if (flags & GUI_DEFAULT_PROPERTIES)
        gui_config_default_properties(config);
    if (flags & GUI_DEFAULT_ROUNDING)
        gui_config_default_rounding(config);
}

struct gui_vec2
gui_config_property(const struct gui_config *config, enum gui_config_properties index)
{
    static struct gui_vec2 zero;
    GUI_ASSERT(config);
    if (!config) return zero;
    return config->properties[index];
}

struct gui_color
gui_config_color(const struct gui_config *config, enum gui_config_colors index)
{
    static struct gui_color zero;
    GUI_ASSERT(config);
    if (!config) return zero;
    return config->colors[index];
}

void
gui_config_push_color(struct gui_config *config, enum gui_config_colors index,
    gui_byte r, gui_byte g, gui_byte b, gui_byte a)
{
    struct gui_saved_color *c;
    GUI_ASSERT(config);
    if (!config) return;
    if (config->stack.color >= GUI_MAX_COLOR_STACK) return;
    c = &config->stack.colors[config->stack.color++];
    c->value = config->colors[index];
    c->type = index;
    config->colors[index].r = r;
    config->colors[index].g = g;
    config->colors[index].b = b;
    config->colors[index].a = a;
}

void
gui_config_push_property(struct gui_config *config, enum gui_config_properties index,
    gui_float x, gui_float y)
{
    struct gui_saved_property *p;
    GUI_ASSERT(config);
    if (!config) return;
    if (config->stack.property >= GUI_MAX_ATTRIB_STACK) return;
    p = &config->stack.properties[config->stack.property++];
    p->value = config->properties[index];
    p->type = index;
    config->properties[index].x = x;
    config->properties[index].y = y;
}

void
gui_config_pop_color(struct gui_config *config)
{
    struct gui_saved_color *c;
    GUI_ASSERT(config);
    if (!config) return;
    if (!config->stack.color) return;
    c = &config->stack.colors[--config->stack.color];
    config->colors[c->type] = c->value;
}

void
gui_config_pop_property(struct gui_config *config)
{
    struct gui_saved_property *p;
    GUI_ASSERT(config);
    if (!config) return;
    if (!config->stack.property) return;
    p = &config->stack.properties[--config->stack.property];
    config->properties[p->type] = p->value;
}

void
gui_config_reset_colors(struct gui_config *config)
{
    GUI_ASSERT(config);
    if (!config) return;
    while (config->stack.color)
        gui_config_pop_color(config);
}

void
gui_config_reset_properties(struct gui_config *config)
{
    GUI_ASSERT(config);
    if (!config) return;
    while (config->stack.property)
        gui_config_pop_property(config);
}

void
gui_config_reset(struct gui_config *config)
{
    GUI_ASSERT(config);
    if (!config) return;
    gui_config_reset_colors(config);
    gui_config_reset_properties(config);
}

/*
 * ==============================================================
 *
 *                          Panel
 *
 * ===============================================================
 */
void
gui_panel_init(struct gui_panel *panel, gui_float x, gui_float y, gui_float w,
    gui_float h, gui_flags flags, struct gui_command_buffer *buffer,
    const struct gui_config *config)
{
    GUI_ASSERT(panel);
    GUI_ASSERT(config);
    if (!panel || !config)
        return;

    panel->x = x;
    panel->y = y;
    panel->w = w;
    panel->h = h;
    panel->flags = flags;
    panel->config = config;
    panel->buffer = buffer;
    panel->offset = 0;
    panel->minimized = gui_false;
}

gui_bool
gui_panel_begin(struct gui_panel_layout *l, struct gui_panel *p,
    const char *text, const struct gui_input *i)
{
    const struct gui_config *c;
    struct gui_command_buffer *out;
    const struct gui_color *header;
    gui_float mouse_x, mouse_y;
    gui_float prev_x, prev_y;
    gui_float clicked_x, clicked_y;
    gui_float header_x = 0, header_w = 0;
    gui_float footer_h;
    gui_bool ret = gui_true;

    float scrollbar_width;
    struct gui_vec2 panel_size;
    struct gui_vec2 item_padding;
    struct gui_vec2 item_spacing;
    struct gui_vec2 panel_padding;
    struct gui_vec2 scaler_size;

    GUI_ASSERT(p);
    GUI_ASSERT(l);
    GUI_ASSERT(p->buffer);

    /* check arguments */
    if (!p || !p->buffer || !l)
        return gui_false;
    if (!(p->flags & GUI_PANEL_TAB))
        gui_command_buffer_reset(p->buffer);
    if (p->flags & GUI_PANEL_HIDDEN) {
        gui_zero(l, sizeof(*l));
        l->valid = gui_false;
        l->config = p->config;
        l->buffer = p->buffer;
        return gui_false;
    }

    c = p->config;
    scrollbar_width = gui_config_property(c, GUI_PROPERTY_SCROLLBAR_WIDTH).x;
    panel_size = gui_config_property(c, GUI_PROPERTY_SIZE);
    panel_padding = gui_config_property(c, GUI_PROPERTY_PADDING);
    item_padding = gui_config_property(c, GUI_PROPERTY_ITEM_PADDING);
    item_spacing = gui_config_property(c, GUI_PROPERTY_ITEM_SPACING);
    scaler_size = gui_config_property(c, GUI_PROPERTY_SCALER_SIZE);

    l->header_height = c->font.height + 3 * item_padding.y;
    l->header_height += panel_padding.y;

    mouse_x = (i) ? i->mouse_pos.x : -1;
    mouse_y = (i) ? i->mouse_pos.y : -1;
    prev_x = (i) ? i->mouse_prev.x : -1;
    prev_y = (i) ? i->mouse_prev.y : -1;
    clicked_x = (i) ? i->mouse_clicked_pos.x : -1;
    clicked_y = (i) ? i->mouse_clicked_pos.y : -1;

    if (p->flags & GUI_PANEL_MOVEABLE) {
        gui_bool incursor;
        const gui_float move_x = p->x;
        const gui_float move_y = p->y;
        const gui_float move_w = p->w;
        const gui_float move_h = l->header_height;

        incursor = i && GUI_INBOX(i->mouse_prev.x,i->mouse_prev.y,move_x,move_y,move_w,move_h);
        if (i && i->mouse_down && incursor) {
            p->x = MAX(0, p->x + i->mouse_delta.x);
            p->y = MAX(0, p->y + i->mouse_delta.y);
        }
    }

    if (p->flags & GUI_PANEL_SCALEABLE) {
        gui_bool incursor;
        gui_float scaler_w = MAX(0, scaler_size.x - item_padding.x);
        gui_float scaler_h = MAX(0, scaler_size.y - item_padding.y);
        gui_float scaler_x = (p->x + p->w) - (item_padding.x + scaler_w);
        gui_float scaler_y = p->y + p->h - scaler_size.y;

        incursor = i && GUI_INBOX(prev_x, prev_y, scaler_x, scaler_y, scaler_w, scaler_h);
        if (i && i->mouse_down && incursor) {
            gui_float min_x = panel_size.x;
            gui_float min_y = panel_size.y;
            p->w = MAX(min_x, p->w + i->mouse_delta.x);
            p->h = MAX(min_y, p->h + i->mouse_delta.y);
        }
    }

    out = p->buffer;
    l->x = p->x;
    l->y = p->y;
    l->w = p->w;
    l->h = p->h;
    l->at_x = p->x;
    l->at_y = p->y;
    l->width = p->w;
    l->height = p->h;
    l->config = p->config;
    l->buffer = p->buffer;
    l->input = i;
    l->index = 0;
    l->row_columns = 0;
    l->row_height = 0;
    l->offset = p->offset;

    if (!(p->flags & GUI_PANEL_NO_HEADER)) {
        header = &c->colors[GUI_COLOR_HEADER];
        header_x = p->x + panel_padding.x;
        header_w = p->w - 2 * panel_padding.x;
        gui_command_buffer_push_rect(out, p->x, p->y, p->w, l->header_height, 0, *header);
    } else l->header_height = 1;
    l->row_height = l->header_height + 2 * item_spacing.y;

    footer_h = scaler_size.y + item_padding.y;
    if ((p->flags & GUI_PANEL_SCALEABLE) &&
        (p->flags & GUI_PANEL_SCROLLBAR) &&
        !p->minimized) {
        gui_float footer_x, footer_y, footer_w;
        footer_x = p->x;
        footer_w = p->w;
        footer_y = p->y + p->h - footer_h;
        gui_command_buffer_push_rect(out, footer_x, footer_y, footer_w, footer_h,
            0, c->colors[GUI_COLOR_PANEL]);
    }

    if (!(p->flags & GUI_PANEL_TAB)) {
        p->flags |= GUI_PANEL_SCROLLBAR;
        if (i && i->mouse_down) {
            if (!GUI_INBOX(clicked_x, clicked_y, p->x, p->y, p->w, p->h))
                p->flags &= (gui_flag)~GUI_PANEL_ACTIVE;
            else p->flags |= GUI_PANEL_ACTIVE;
        }
    }

    l->clip.x = p->x;
    l->clip.w = p->w;
    l->clip.y = p->y + l->header_height - 1;
    if (p->flags & GUI_PANEL_SCROLLBAR) {
        if (p->flags & GUI_PANEL_SCALEABLE)
            l->clip.h = p->h - (footer_h + l->header_height);
        else l->clip.h = p->h - l->header_height;
            l->clip.h -= (panel_padding.y + item_padding.y);
    } else l->clip.h = gui_null_rect.h;

    if ((p->flags & GUI_PANEL_CLOSEABLE) && (!(p->flags & GUI_PANEL_NO_HEADER))) {
        const gui_char *X = (const gui_char*)"x";
        const gui_size text_width = c->font.width(c->font.userdata, X, 1);
        const gui_float close_x = header_x;
        const gui_float close_y = p->y + panel_padding.y;
        const gui_float close_w = (gui_float)text_width + 2 * item_padding.x;
        const gui_float close_h = c->font.height + 2 * item_padding.y;
        gui_command_buffer_push_text(out, close_x, close_y, close_w, close_h,
            X, 1, &c->font, c->colors[GUI_COLOR_HEADER], c->colors[GUI_COLOR_TEXT]);

        header_w -= close_w;
        header_x += close_h - item_padding.x;
        if (i && GUI_INBOX(mouse_x, mouse_y, close_x, close_y, close_w, close_h)) {
            if (GUI_INBOX(clicked_x, clicked_y, close_x, close_y, close_w, close_h)) {
                ret = !(i->mouse_down && i->mouse_clicked);
                if (!ret) p->flags |= GUI_PANEL_HIDDEN;
            }
        }
    }

    if ((p->flags & GUI_PANEL_MINIMIZABLE) && (!(p->flags & GUI_PANEL_NO_HEADER))) {
        gui_size text_width;
        gui_float min_x, min_y, min_w, min_h;
        const gui_char *score = (p->minimized) ?
            (const gui_char*)"+":
            (const gui_char*)"-";

        text_width = c->font.width(c->font.userdata, score, 1);
        min_x = header_x;
        min_y = p->y + panel_padding.y;
        min_w = (gui_float)text_width + 3 * item_padding.x;
        min_h = c->font.height + 2 * item_padding.y;
        gui_command_buffer_push_text(out, min_x, min_y, min_w, min_h,
            score, 1, &c->font, c->colors[GUI_COLOR_HEADER],
            c->colors[GUI_COLOR_TEXT]);

        header_w -= min_w;
        header_x += min_w - item_padding.x;
        if (i && GUI_INBOX(mouse_x, mouse_y, min_x, min_y, min_w, min_h)) {
            if (GUI_INBOX(clicked_x, clicked_y, min_x, min_y, min_w, min_h))
                if (i->mouse_down && i->mouse_clicked)
                    p->minimized = !p->minimized;
        }
    }
    l->valid = !(p->minimized || (p->flags & GUI_PANEL_HIDDEN));

    if (text && !(p->flags & GUI_PANEL_NO_HEADER)) {
        const gui_size text_len = gui_strsiz(text);
        const gui_float label_x = header_x + item_padding.x;
        const gui_float label_y = p->y + panel_padding.y;
        const gui_float label_w = header_w - (3 * item_padding.x);
        const gui_float label_h = c->font.height + 2 * item_padding.y;
        gui_command_buffer_push_text(out, label_x, label_y, label_w, label_h,
            (const gui_char*)text, text_len, &c->font, c->colors[GUI_COLOR_HEADER],
            c->colors[GUI_COLOR_TEXT]);
    }

    if (p->flags & GUI_PANEL_SCROLLBAR) {
        const struct gui_color *color = &c->colors[GUI_COLOR_PANEL];
        l->width = p->w - scrollbar_width;
        l->height = p->h - (l->header_height + 2 * item_spacing.y);
        if (p->flags & GUI_PANEL_SCALEABLE) l->height -= footer_h;
        if (l->valid)
            gui_command_buffer_push_rect(out, p->x, p->y + l->header_height,
                p->w, p->h - l->header_height, 0, *color);
    }

    if (p->flags & GUI_PANEL_BORDER) {
        const struct gui_color *color = &c->colors[GUI_COLOR_BORDER];
        const gui_float width = (p->flags & GUI_PANEL_SCROLLBAR) ?
                l->width + scrollbar_width : l->width;

        gui_command_buffer_push_line(out, p->x, p->y,
                p->x + p->w, p->y, *color);
        gui_command_buffer_push_line(out, p->x, p->y, p->x,
                p->y + l->header_height, c->colors[GUI_COLOR_BORDER]);
        gui_command_buffer_push_line(out, p->x + width, p->y, p->x + width,
                p->y + l->header_height, c->colors[GUI_COLOR_BORDER]);
        if (p->flags & GUI_PANEL_BORDER_HEADER)
            gui_command_buffer_push_line(out, p->x, p->y + l->header_height,
                p->x + p->w, p->y + l->header_height, c->colors[GUI_COLOR_BORDER]);
    }
    gui_command_buffer_push_scissor(out, l->clip.x, l->clip.y, l->clip.w, l->clip.h);
    return ret;
}

gui_bool
gui_panel_begin_stacked(struct gui_panel_layout *l, struct gui_panel* p,
    struct gui_stack *s, const char *title, const struct gui_input *i)
{
    gui_bool inpanel;
    GUI_ASSERT(l);
    GUI_ASSERT(s);
    if (!l || !s) return gui_false;

    inpanel = GUI_INBOX(i->mouse_prev.x, i->mouse_prev.y, p->x, p->y, p->w, p->h);
    if (i->mouse_down && i->mouse_clicked && inpanel && p != s->end) {
        const struct gui_panel *iter = p->next;
        while (iter) {
            const struct gui_panel *cur = iter;
            if (GUI_INBOX(i->mouse_prev.x,i->mouse_prev.y,cur->x,cur->y,cur->w,cur->h) &&
              !cur->minimized) break;
            iter = iter->next;
        }

        if (!iter) {
            gui_stack_pop(s, p);
            gui_stack_push(s, p);
        }
    }
    return gui_panel_begin(l, p, title, (s->end == p) ? i : NULL);
}

gui_bool
gui_panel_begin_tiled(struct gui_panel_layout *tile, struct gui_panel *panel,
    struct gui_layout *layout, enum gui_layout_slot_index slot, gui_size index,
    const char *title, const struct gui_input *in)
{
    struct gui_rect bounds;
    struct gui_layout_slot *s;

    GUI_ASSERT(panel);
    GUI_ASSERT(tile);
    GUI_ASSERT(layout);

    if (!layout || !panel || !tile) return gui_false;
    if (slot >= GUI_SLOT_MAX) return gui_false;
    if (index >= layout->slots[slot].capacity) return gui_false;

    panel->flags &= (gui_flags)~GUI_PANEL_MINIMIZABLE;
    panel->flags &= (gui_flags)~GUI_PANEL_CLOSEABLE;
    panel->flags &= (gui_flags)~GUI_PANEL_MOVEABLE;
    panel->flags &= (gui_flags)~GUI_PANEL_SCALEABLE;

    s = &layout->slots[slot];
    bounds.x = s->offset.x * (gui_float)layout->width;
    bounds.y = s->offset.y * (gui_float)layout->height;
    bounds.w = s->ratio.x * (gui_float)layout->width;
    bounds.h = s->ratio.y * (gui_float)layout->height;

    if (s->format == GUI_LAYOUT_HORIZONTAL) {
        panel->h = bounds.h;
        panel->y = bounds.y;
        panel->x = bounds.x + (gui_float)index * panel->w;
        panel->w = bounds.w / (gui_float)s->capacity;
    } else {
        panel->x = bounds.x;
        panel->w = bounds.w;
        panel->h = bounds.h / (gui_float)s->capacity;
        panel->y = bounds.y + (gui_float)index * panel->h;
    }

    gui_stack_push(&layout->stack, panel);
    return gui_panel_begin(tile, panel, title, (layout->state) ? in : NULL);
}

void
gui_panel_row(struct gui_panel_layout *layout, gui_float height, gui_size cols)
{
    const struct gui_config *config;
    const struct gui_color *color;
    struct gui_command_buffer *out;
    struct gui_vec2 item_spacing;
    struct gui_vec2 panel_padding;

    GUI_ASSERT(layout);
    GUI_ASSERT(layout->config);
    GUI_ASSERT(layout->buffer);
    if (!layout) return;
    if (!layout->valid) return;

    config = layout->config;
    out = layout->buffer;
    color = &config->colors[GUI_COLOR_PANEL];
    item_spacing = gui_config_property(config, GUI_PROPERTY_ITEM_SPACING);
    panel_padding = gui_config_property(config, GUI_PROPERTY_PADDING);

    layout->index = 0;
    layout->at_y += layout->row_height;
    layout->row_columns = cols;
    layout->row_height = height + item_spacing.y;
    gui_command_buffer_push_rect(out,  layout->at_x, layout->at_y,
        layout->width, height + panel_padding.y, 0, *color);
}

gui_size
gui_panel_row_columns(const struct gui_panel_layout *l, gui_size widget_size)
{
    struct gui_vec2 spacing;
    struct gui_vec2 padding;
    gui_size cols = 0, size;
    GUI_ASSERT(l);
    GUI_ASSERT(widget_size);
    if (!l || !widget_size)
        return 0;

    spacing = gui_config_property(l->config, GUI_PROPERTY_ITEM_SPACING);
    padding = gui_config_property(l->config, GUI_PROPERTY_PADDING);
    cols = (gui_size)(l->width) / widget_size;
    size = (cols * (gui_size)spacing.x) + 2 * (gui_size)padding.x + widget_size * cols;
    while ((size > l->width) && --cols)
        size = (cols*(gui_size)spacing.x) + 2*(gui_size)padding.x + widget_size * cols;
    return cols;
}

void
gui_panel_spacing(struct gui_panel_layout *l, gui_size cols)
{
    gui_size add;
    GUI_ASSERT(l);
    GUI_ASSERT(l->config);
    GUI_ASSERT(l->buffer);
    if (!l) return;
    if (!l->valid) return;

    add = (l->index + cols) % l->row_columns;
    if (l->index + cols > l->row_columns) {
        gui_size i;
        const struct gui_config *c = l->config;
        struct gui_vec2 item_spacing = gui_config_property(c, GUI_PROPERTY_ITEM_SPACING);
        const gui_float row_height = l->row_height - item_spacing.y;
        gui_size rows = (l->index + cols) / l->row_columns;
        for (i = 0; i < rows; ++i)
            gui_panel_row(l, row_height, l->row_columns);
    }
    l->index += add;
}

static void
gui_panel_alloc_space(struct gui_rect *bounds, struct gui_panel_layout *layout)
{
    const struct gui_config *config;
    gui_float panel_padding, panel_spacing, panel_space;
    gui_float item_offset, item_width, item_spacing;
    struct gui_vec2 spacing;
    struct gui_vec2 padding;

    GUI_ASSERT(layout);
    GUI_ASSERT(layout->config);
    GUI_ASSERT(bounds);
    if (!layout || !layout->config || !bounds)
        return;

    config = layout->config;
    spacing = gui_config_property(config, GUI_PROPERTY_ITEM_SPACING);
    padding = gui_config_property(config, GUI_PROPERTY_PADDING);

    if (layout->index >= layout->row_columns) {
        const gui_float row_height = layout->row_height - spacing.y;
        gui_panel_row(layout, row_height, layout->row_columns);
    }

    panel_padding = 2 * padding.x;
    panel_spacing = (gui_float)(layout->row_columns - 1) * spacing.x;
    panel_space  = layout->width - panel_padding - panel_spacing;

    item_width = panel_space / (gui_float)layout->row_columns;
    item_offset = (gui_float)layout->index * item_width;
    item_spacing = (gui_float)layout->index * spacing.x;

    bounds->x = layout->at_x + item_offset + item_spacing + padding.x;
    bounds->y = layout->at_y - layout->offset;
    bounds->w = item_width;
    bounds->h = layout->row_height - spacing.y;
    layout->index++;
}

gui_bool
gui_panel_widget(struct gui_rect *bounds, struct gui_panel_layout *layout)
{
    struct gui_rect *c = NULL;
    GUI_ASSERT(layout);
    GUI_ASSERT(layout->config);
    GUI_ASSERT(layout->buffer);
    if (!layout || !layout->config || !layout->buffer) return gui_false;
    if (!layout->valid) return gui_false;

    gui_panel_alloc_space(bounds, layout);
    c = &layout->clip;
    if (!GUI_INTERSECT(c->x, c->y, c->w, c->h, bounds->x, bounds->y, bounds->w, bounds->h))
        return gui_false;
    return gui_true;
}

void
gui_panel_text_colored(struct gui_panel_layout *layout, const char *str, gui_size len,
    enum gui_text_align alignment, struct gui_color color)
{
    struct gui_rect bounds;
    struct gui_text text;
    const struct gui_config *config;
    struct gui_vec2 item_padding;

    GUI_ASSERT(layout);
    GUI_ASSERT(layout->config);
    GUI_ASSERT(layout->buffer);
    GUI_ASSERT(str && len);

    if (!layout || !layout->config || !layout->buffer) return;
    if (!layout->valid) return;
    gui_panel_alloc_space(&bounds, layout);
    config = layout->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);

    text.padding.x = item_padding.x;
    text.padding.y = item_padding.y;
    text.foreground = color;
    text.background = config->colors[GUI_COLOR_PANEL];
    gui_text(layout->buffer,bounds.x,bounds.y,bounds.w,bounds.h, str, len,
        &text, alignment, &config->font);
}

void
gui_panel_text(struct gui_panel_layout *l, const char *str, gui_size len,
    enum gui_text_align alignment)
{
    gui_panel_text_colored(l, str, len, alignment,l->config->colors[GUI_COLOR_TEXT]);
}

void
gui_panel_label_colored(struct gui_panel_layout *layout, const char *text,
    enum gui_text_align align, struct gui_color color)
{
    gui_size len = gui_strsiz(text);
    gui_panel_text_colored(layout, text, len, align, color);
}

void
gui_panel_label(struct gui_panel_layout *layout, const char *text,
    enum gui_text_align align)
{
    gui_size len = gui_strsiz(text);
    gui_panel_text(layout, text, len, align);
}

static gui_bool
gui_panel_button(struct gui_button *button, struct gui_rect *bounds,
    struct gui_panel_layout *layout)
{
    const struct gui_config *config;
    struct gui_vec2 item_padding;
    if (!gui_panel_widget(bounds, layout)) return gui_false;
    config = layout->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);

    button->border = 1;
    button->rounding = config->rounding[GUI_ROUNDING_BUTTON];
    button->padding.x = item_padding.x;
    button->padding.y = item_padding.y;
    return gui_true;
}

gui_bool
gui_panel_button_text(struct gui_panel_layout *layout, const char *str,
    enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;
    if (!gui_panel_button(&button, &bounds, layout))
        return gui_false;

    config = layout->config;
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
    button.content = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
    button.highlight_content = config->colors[GUI_COLOR_BUTTON_HOVER_FONT];
    button.rounding = config->rounding[GUI_ROUNDING_BUTTON];
    return gui_button_text(layout->buffer, bounds.x, bounds.y, bounds.w, bounds.h,
            str, behavior, &button, layout->input, &config->font);
}

gui_bool
gui_panel_button_color(struct gui_panel_layout *layout,
   struct gui_color color, enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button button;
    if (!gui_panel_button(&button, &bounds, layout))
        return gui_false;

    button.background = color;
    button.foreground = color;
    button.highlight = color;
    button.highlight_content = color;
    button.rounding = layout->config->rounding[GUI_ROUNDING_BUTTON];
    return gui_do_button(layout->buffer, bounds.x, bounds.y, bounds.w, bounds.h,
                &button, layout->input, behavior);
}

gui_bool
gui_panel_button_triangle(struct gui_panel_layout *layout, enum gui_heading heading,
    enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;
    if (!gui_panel_button(&button, &bounds, layout))
        return gui_false;

    config = layout->config;
    button.rounding = config->rounding[GUI_ROUNDING_BUTTON];
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
    button.content = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
    button.highlight_content = config->colors[GUI_COLOR_BUTTON_HOVER_FONT];
    return gui_button_triangle(layout->buffer, bounds.x, bounds.y, bounds.w,
            bounds.h, heading, behavior, &button, layout->input);
}

gui_bool
gui_panel_button_image(struct gui_panel_layout *layout, gui_handle image,
    enum gui_button_behavior behavior)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;
    if (!gui_panel_button(&button, &bounds, layout))
        return gui_false;

    config = layout->config;
    button.rounding = config->rounding[GUI_ROUNDING_BUTTON];
    button.background = config->colors[GUI_COLOR_BUTTON];
    button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
    button.content = config->colors[GUI_COLOR_TEXT];
    button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
    button.highlight_content = config->colors[GUI_COLOR_BUTTON_HOVER_FONT];
    return gui_button_image(layout->buffer, bounds.x, bounds.y, bounds.w,
            bounds.h, image, behavior, &button, layout->input);
}

gui_bool
gui_panel_button_toggle(struct gui_panel_layout *layout, const char *str, gui_bool value)
{
    struct gui_rect bounds;
    struct gui_button button;
    const struct gui_config *config;
    GUI_ASSERT(str);
    if (!gui_panel_button(&button, &bounds, layout))
        return value;

    config = layout->config;
    button.rounding = config->rounding[GUI_ROUNDING_BUTTON];
    if (!value) {
        button.background = config->colors[GUI_COLOR_BUTTON];
        button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
        button.content = config->colors[GUI_COLOR_TEXT];
        button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
        button.highlight_content = config->colors[GUI_COLOR_BUTTON];
    } else {
        button.background = config->colors[GUI_COLOR_BUTTON_TOGGLE];
        button.foreground = config->colors[GUI_COLOR_BUTTON_BORDER];
        button.content = config->colors[GUI_COLOR_TEXT];
        button.highlight = config->colors[GUI_COLOR_BUTTON_HOVER];
        button.highlight_content = config->colors[GUI_COLOR_BUTTON];
    }
    if (gui_button_text(layout->buffer, bounds.x, bounds.y, bounds.w, bounds.h,
        str, GUI_BUTTON_DEFAULT, &button, layout->input, &config->font)) value = !value;
    return value;
}

static gui_bool
gui_panel_toggle_base(struct gui_toggle *toggle, struct gui_rect *bounds,
    struct gui_panel_layout *layout)
{
    const struct gui_config *config;
    struct gui_vec2 item_padding;
    if (!gui_panel_widget(bounds, layout))
        return gui_false;

    config = layout->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    toggle->rounding = 0;
    toggle->padding.x = item_padding.x;
    toggle->padding.y = item_padding.y;
    toggle->font = config->colors[GUI_COLOR_TEXT];
    return gui_true;
}

gui_bool
gui_panel_check(struct gui_panel_layout *layout, const char *text, gui_bool is_active)
{
    struct gui_rect bounds;
    struct gui_toggle toggle;
    const struct gui_config *config;
    if (!text || !gui_panel_toggle_base(&toggle, &bounds, layout))
        return is_active;

    config = layout->config;
    toggle.rounding = config->rounding[GUI_ROUNDING_CHECK];
    toggle.cursor = config->colors[GUI_COLOR_CHECK_ACTIVE];
    toggle.background = config->colors[GUI_COLOR_CHECK_BACKGROUND];
    toggle.foreground = config->colors[GUI_COLOR_CHECK];
    return gui_toggle(layout->buffer, bounds.x, bounds.y, bounds.w, bounds.h,
                is_active, text, GUI_TOGGLE_CHECK, &toggle, layout->input, &config->font);
}

gui_bool
gui_panel_option(struct gui_panel_layout *layout, const char *text, gui_bool is_active)
{
    struct gui_rect bounds;
    struct gui_toggle toggle;
    const struct gui_config *config;
    if (!text || !gui_panel_toggle_base(&toggle, &bounds, layout))
        return is_active;

    config = layout->config;
    toggle.cursor = config->colors[GUI_COLOR_OPTION_ACTIVE];
    toggle.background = config->colors[GUI_COLOR_OPTION_BACKGROUND];
    toggle.foreground = config->colors[GUI_COLOR_OPTION];
    return gui_toggle(layout->buffer, bounds.x, bounds.y, bounds.w, bounds.h,
        is_active, text, GUI_TOGGLE_OPTION, &toggle, layout->input, &config->font);
}

gui_size
gui_panel_option_group(struct gui_panel_layout *layout, const char **options,
    gui_size count, gui_size current)
{
    gui_size i;
    GUI_ASSERT(layout && options && count);
    if (!layout || !options || !count) return current;
    for (i = 0; i < count; ++i) {
        if (gui_panel_option(layout, options[i], i == current))
            current = i;
    }
    return current;
}

gui_float
gui_panel_slider(struct gui_panel_layout *layout, gui_float min_value, gui_float value,
    gui_float max_value, gui_float value_step)
{
    struct gui_rect bounds;
    struct gui_slider slider;
    const struct gui_config *config;
    struct gui_vec2 item_padding;
    if (!gui_panel_widget(&bounds, layout))
        return value;

    config = layout->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    slider.cursor = config->slider_cursor;
    slider.padding.x = item_padding.x;
    slider.padding.y = item_padding.y;
    slider.bg = config->colors[GUI_COLOR_SLIDER];
    slider.fg = config->colors[GUI_COLOR_SLIDER_CURSOR];
    slider.bar = config->colors[GUI_COLOR_SLIDER_BAR];
    slider.border = config->colors[GUI_COLOR_SLIDER_BORDER];
    return gui_slider(layout->buffer, bounds.x, bounds.y, bounds.w, bounds.h,
                min_value, value, max_value, value_step, &slider, layout->input);
}

gui_size
gui_panel_progress(struct gui_panel_layout *layout, gui_size cur_value, gui_size max_value,
    gui_bool is_modifyable)
{
    struct gui_rect bounds;
    struct gui_progress prog;
    const struct gui_config *config;
    struct gui_vec2 item_padding;
    if (!gui_panel_widget(&bounds, layout))
        return cur_value;

    config = layout->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    prog.rounding = config->rounding[GUI_ROUNDING_PROGRESS];
    prog.padding.x = item_padding.x;
    prog.padding.y = item_padding.y;
    prog.background = config->colors[GUI_COLOR_PROGRESS];
    prog.foreground = config->colors[GUI_COLOR_PROGRESS_CURSOR];
    return gui_progress(layout->buffer, bounds.x, bounds.y, bounds.w, bounds.h,
            cur_value, max_value, is_modifyable, &prog, layout->input);
}

static gui_bool
gui_panel_edit_base(struct gui_rect *bounds, struct gui_edit *field,
    struct gui_panel_layout *layout)
{
    const struct gui_config *config;
    struct gui_vec2 item_padding;
    if (!gui_panel_widget(bounds, layout))
        return gui_false;

    config = layout->config;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    field->border_size = 1;
    field->rounding = config->rounding[GUI_ROUNDING_INPUT];
    field->padding.x = item_padding.x;
    field->padding.y = item_padding.y;
    field->show_cursor = gui_true;
    field->background = config->colors[GUI_COLOR_INPUT];
    field->border = config->colors[GUI_COLOR_INPUT_BORDER];
    field->cursor = config->colors[GUI_COLOR_INPUT_CURSOR];
    field->text = config->colors[GUI_COLOR_INPUT_TEXT];
    return gui_true;
}

gui_size
gui_panel_edit(struct gui_panel_layout *layout, gui_char *buffer, gui_size len,
    gui_size max, gui_bool *active, enum gui_input_filter filter)
{
    struct gui_rect bounds;
    struct gui_edit field;
    const struct gui_config *config = layout->config;
    if (!gui_panel_edit_base(&bounds, &field, layout)) return len;
    return gui_edit(layout->buffer, bounds.x, bounds.y, bounds.w, bounds.h,
        buffer, len, max, active, &field, filter, layout->input, &config->font);
}

gui_size
gui_panel_edit_filtered(struct gui_panel_layout *layout, gui_char *buffer, gui_size len,
    gui_size max, gui_bool *active, gui_filter filter)
{
    struct gui_rect bounds;
    struct gui_edit field;
    const struct gui_config *config = layout->config;
    if (!gui_panel_edit_base(&bounds, &field, layout)) return len;
    return gui_edit_filtered(layout->buffer, bounds.x, bounds.y, bounds.w, bounds.h,
        buffer, len, max, active, &field, filter, layout->input, &config->font);
}

gui_int
gui_panel_spinner(struct gui_panel_layout *layout, gui_int min, gui_int value,
    gui_int max, gui_int step, gui_bool *active)
{
    struct gui_rect bounds;
    struct gui_spinner spinner;
    const struct gui_config *config;
    struct gui_command_buffer *out;
    struct gui_vec2 item_padding;

    if (!gui_panel_widget(&bounds, layout))
        return value;

    config = layout->config;
    out = layout->buffer;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);

    spinner.border_button = 1;
    spinner.button_color = config->colors[GUI_COLOR_BUTTON];
    spinner.button_border = config->colors[GUI_COLOR_BUTTON_BORDER];
    spinner.button_triangle = config->colors[GUI_COLOR_SPINNER_TRIANGLE];
    spinner.padding.x = item_padding.x;
    spinner.padding.y = item_padding.y;
    spinner.color = config->colors[GUI_COLOR_SPINNER];
    spinner.border = config->colors[GUI_COLOR_SPINNER_BORDER];
    spinner.text = config->colors[GUI_COLOR_SPINNER_TEXT];
    spinner.show_cursor = gui_false;
    return gui_spinner(out, bounds.x, bounds.y, bounds.w, bounds.h, &spinner,
        min, value, max, step, active, layout->input, &layout->config->font);
}

gui_size
gui_panel_selector(struct gui_panel_layout *layout, const char *items[],
    gui_size item_count, gui_size item_current)
{
    struct gui_rect bounds;
    struct gui_selector selector;
    const struct gui_config *config;
    struct gui_command_buffer *out;

    GUI_ASSERT(items);
    GUI_ASSERT(item_count);
    GUI_ASSERT(item_current < item_count);
    if (!gui_panel_widget(&bounds, layout))
        return item_current;

    out = layout->buffer;
    config = layout->config;

    selector.border_button = 1;
    selector.button_color = config->colors[GUI_COLOR_BUTTON];
    selector.button_border = config->colors[GUI_COLOR_BUTTON_BORDER];
    selector.button_triangle = config->colors[GUI_COLOR_SELECTOR_TRIANGLE];
    selector.color = config->colors[GUI_COLOR_SELECTOR];
    selector.border = config->colors[GUI_COLOR_SELECTOR_BORDER];
    selector.text = config->colors[GUI_COLOR_TEXT];
    selector.text_bg = config->colors[GUI_COLOR_PANEL];
    selector.padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    return gui_selector(out, bounds.x, bounds.y, bounds.w, bounds.h, &selector,
        items, item_count, item_current, layout->input, &layout->config->font);
}

void
gui_panel_graph_begin(struct gui_panel_layout *layout, struct gui_graph *graph,
    enum gui_graph_type type, gui_size count, gui_float min_value, gui_float max_value)
{
    struct gui_rect bounds = {0, 0, 0, 0};
    const struct gui_config *config;
    struct gui_command_buffer *out;
    struct gui_color color;
    struct gui_vec2 item_padding;
    if (!gui_panel_widget(&bounds, layout)) {
        gui_zero(graph, sizeof(*graph));
        return;
    }

    config = layout->config;
    out = layout->buffer;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    color = (type == GUI_GRAPH_LINES) ?
        config->colors[GUI_COLOR_PLOT]: config->colors[GUI_COLOR_HISTO];
    gui_command_buffer_push_rect(out, bounds.x, bounds.y, bounds.w, bounds.h,
        config->rounding[GUI_ROUNDING_GRAPH], color);

    graph->valid = gui_true;
    graph->type = type;
    graph->index = 0;
    graph->count = count;
    graph->min = min_value;
    graph->max = max_value;
    graph->x = bounds.x + item_padding.x;
    graph->y = bounds.y + item_padding.y;
    graph->w = bounds.w - 2 * item_padding.x;
    graph->h = bounds.h - 2 * item_padding.y;
    graph->w = MAX(graph->w, 2 * item_padding.x);
    graph->h = MAX(graph->h, 2 * item_padding.y);
    graph->last.x = 0; graph->last.y = 0;
}

static gui_bool
gui_panel_graph_push_line(struct gui_panel_layout *l,
    struct gui_graph *g, gui_float value)
{
    struct gui_command_buffer *out = l->buffer;
    const struct gui_config *config = l->config;
    const struct gui_input *i = l->input;
    struct gui_color color = config->colors[GUI_COLOR_PLOT_LINES];
    gui_bool selected = gui_false;
    gui_float step, range, ratio;
    struct gui_vec2 cur;

    GUI_ASSERT(g);
    GUI_ASSERT(l);
    GUI_ASSERT(out);
    if (!g || !l || !g->valid || g->index >= g->count)
        return gui_false;

    step = g->w / (gui_float)g->count;
    range = g->max - g->min;
    ratio = (value - g->min) / range;

    if (g->index == 0) {
        g->last.x = g->x;
        g->last.y = (g->y + g->h) - ratio * (gui_float)g->h;
        if (i && GUI_INBOX(i->mouse_pos.x, i->mouse_pos.y, g->last.x-3, g->last.y-3, 6, 6)){
            selected = (i->mouse_down && i->mouse_clicked) ? gui_true: gui_false;
            color = config->colors[GUI_COLOR_PLOT_HIGHLIGHT];
        }
        gui_command_buffer_push_rect(out, g->last.x - 3, g->last.y - 3, 6, 6, 0, color);
        g->index++;
        return selected;
    }

    cur.x = g->x + (gui_float)(step * (gui_float)g->index);
    cur.y = (g->y + g->h) - (ratio * (gui_float)g->h);
    gui_command_buffer_push_line(out, g->last.x, g->last.y, cur.x, cur.y,
        config->colors[GUI_COLOR_PLOT_LINES]);

    if (i && GUI_INBOX(i->mouse_pos.x, i->mouse_pos.y, cur.x-3, cur.y-3, 6, 6)) {
        selected = (i->mouse_down && i->mouse_clicked) ? gui_true: gui_false;
        color = config->colors[GUI_COLOR_PLOT_HIGHLIGHT];
    } else color = config->colors[GUI_COLOR_PLOT_LINES];
    gui_command_buffer_push_rect(out, cur.x - 3, cur.y - 3, 6, 6, 0, color);

    g->last.x = cur.x;
    g->last.y = cur.y;
    g->index++;
    return selected;
}

static gui_bool
gui_panel_graph_push_column(struct gui_panel_layout *layout,
    struct gui_graph *graph, gui_float value)
{
    struct gui_command_buffer *out = layout->buffer;
    const struct gui_config *config = layout->config;
    const struct gui_input *in = layout->input;
    struct gui_vec2 item_padding;
    struct gui_color color;

    gui_float ratio;
    gui_bool selected = gui_false;
    gui_float item_x, item_y;
    gui_float item_w = 0.0f, item_h = 0.0f;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);

    if (!graph->valid || graph->index >= graph->count)
        return gui_false;
    if (graph->count) {
        gui_float padding = (gui_float)(graph->count-1) * item_padding.x;
        item_w = (graph->w - padding) / (gui_float)(graph->count);
    }

    ratio = GUI_ABS(value) / graph->max;
    color = (value < 0) ? config->colors[GUI_COLOR_HISTO_NEGATIVE]:
        config->colors[GUI_COLOR_HISTO_BARS];

    item_h = graph->h * ratio;
    item_y = (graph->y + graph->h) - item_h;
    item_x = graph->x + ((gui_float)graph->index * item_w);
    item_x = item_x + ((gui_float)graph->index * item_padding.x);

    if (in && GUI_INBOX(in->mouse_pos.x, in->mouse_pos.y, item_x, item_y, item_w, item_h)) {
        selected = (in->mouse_down && in->mouse_clicked) ? (gui_int)graph->index: selected;
        color = config->colors[GUI_COLOR_HISTO_HIGHLIGHT];
    }
    gui_command_buffer_push_rect(out, item_x, item_y, item_w, item_h, 0, color);
    graph->index++;
    return selected;
}

gui_bool
gui_panel_graph_push(struct gui_panel_layout *layout, struct gui_graph *graph,
    gui_float value)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(graph);
    if (!layout || !graph || !layout->valid) return gui_false;
    switch (graph->type) {
    case GUI_GRAPH_LINES:
        return gui_panel_graph_push_line(layout, graph, value);
    case GUI_GRAPH_COLUMN:
        return gui_panel_graph_push_column(layout, graph, value);
    case GUI_GRAPH_MAX:
    default: return gui_false;
    }
}

void
gui_panel_graph_end(struct gui_panel_layout *layout, struct gui_graph *graph)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(graph);
    if (!layout || !graph) return;
    graph->type = GUI_GRAPH_MAX;
    graph->index = 0;
    graph->count = 0;
    graph->min = 0;
    graph->max = 0;
    graph->x = 0;
    graph->y = 0;
    graph->w = 0;
    graph->h = 0;
}

gui_int
gui_panel_graph(struct gui_panel_layout *layout, enum gui_graph_type type,
    const gui_float *values, gui_size count, gui_size offset)
{
    gui_size i;
    gui_int index = -1;
    gui_float min_value;
    gui_float max_value;
    struct gui_graph graph;

    GUI_ASSERT(layout);
    GUI_ASSERT(values);
    GUI_ASSERT(count);
    if (!layout || !layout->valid || !values || !count)
        return -1;

    max_value = values[0];
    min_value = values[0];
    for (i = offset; i < count; ++i) {
        if (values[i] > max_value)
            max_value = values[i];
        if (values[i] < min_value)
            min_value = values[i];
    }

    gui_panel_graph_begin(layout, &graph, type, count, min_value, max_value);
    for (i = offset; i < count; ++i) {
        if (gui_panel_graph_push(layout, &graph, values[i]))
            index = (gui_int)i;
    }
    gui_panel_graph_end(layout, &graph);
    return index;
}

gui_int
gui_panel_graph_ex(struct gui_panel_layout *layout, enum gui_graph_type type,
    gui_size count, gui_float(*get_value)(void*, gui_size), void *userdata)
{
    gui_size i;
    gui_int index = -1;
    gui_float min_value;
    gui_float max_value;
    struct gui_graph graph;

    GUI_ASSERT(layout);
    GUI_ASSERT(get_value);
    GUI_ASSERT(count);
    if (!layout || !layout->valid || !get_value || !count)
        return -1;

    max_value = get_value(userdata, 0);
    min_value = max_value;
    for (i = 1; i < count; ++i) {
        gui_float value = get_value(userdata, i);
        if (value > max_value)
            max_value = value;
        if (value < min_value)
            min_value = value;
    }

    gui_panel_graph_begin(layout, &graph, type, count, min_value, max_value);
    for (i = 0; i < count; ++i) {
        gui_float value = get_value(userdata, i);
        if (gui_panel_graph_push(layout, &graph, value))
            index = (gui_int)i;
    }
    gui_panel_graph_end(layout, &graph);
    return index;
}

static void
gui_panel_table_hline(struct gui_panel_layout *l, gui_size row_height)
{
    struct gui_command_buffer *out = l->buffer;
    const struct gui_config *c = l->config;
    const struct gui_vec2 item_padding = gui_config_property(c, GUI_PROPERTY_ITEM_PADDING);
    const struct gui_vec2 item_spacing = gui_config_property(c, GUI_PROPERTY_ITEM_SPACING);

    gui_float y = l->at_y + (gui_float)row_height - l->offset;
    gui_float x = l->at_x + item_spacing.x + item_padding.x;
    gui_float w = l->width - (2 * item_spacing.x + 2 * item_padding.x);
    gui_command_buffer_push_line(out, x, y, x + w, y, c->colors[GUI_COLOR_TABLE_LINES]);
}

static void
gui_panel_table_vline(struct gui_panel_layout *layout, gui_size cols)
{
    gui_size i;
    struct gui_command_buffer *out = layout->buffer;
    const struct gui_config *config = layout->config;
    for (i = 0; i < cols - 1; ++i) {
        gui_float y, h;
        struct gui_rect bounds;

        gui_panel_alloc_space(&bounds, layout);
        y = layout->at_y + layout->row_height;
        h = layout->row_height;
        gui_command_buffer_push_line(out,  bounds.x + bounds.w, y,
            bounds.x + bounds.w, h, config->colors[GUI_COLOR_TABLE_LINES]);
    }
    layout->index -= i;
}

void
gui_panel_table_begin(struct gui_panel_layout *layout, gui_flags flags,
    gui_size row_height, gui_size cols)
{
    GUI_ASSERT(layout);
    if (!layout || !layout->valid) return;

    layout->is_table = gui_true;
    layout->tbl_flags = flags;
    gui_panel_row(layout, (gui_float)row_height, cols);
    if (layout->tbl_flags & GUI_TABLE_HHEADER)
        gui_panel_table_hline(layout, row_height);
    if (layout->tbl_flags & GUI_TABLE_VHEADER)
        gui_panel_table_vline(layout, cols);
}

void
gui_panel_table_row(struct gui_panel_layout *layout)
{
    const struct gui_config *config;
    struct gui_vec2 item_spacing;
    GUI_ASSERT(layout);
    if (!layout) return;

    config = layout->config;
    item_spacing = gui_config_property(config, GUI_PROPERTY_ITEM_SPACING);
    gui_panel_row(layout, layout->row_height - item_spacing.y, layout->row_columns);
    if (layout->tbl_flags & GUI_TABLE_HBODY)
        gui_panel_table_hline(layout, (gui_size)(layout->row_height - item_spacing.y));
    if (layout->tbl_flags & GUI_TABLE_VBODY)
        gui_panel_table_vline(layout, layout->row_columns);
}

void
gui_panel_table_end(struct gui_panel_layout *layout)
{
    layout->is_table = gui_false;
}

gui_bool
gui_panel_tab_begin(struct gui_panel_layout *parent, struct gui_panel_layout *tab,
    const char *title, gui_bool minimized)
{
    struct gui_rect bounds;
    struct gui_command_buffer *out;
    struct gui_panel panel;
    struct gui_rect clip;
    gui_flags flags;

    GUI_ASSERT(parent);
    GUI_ASSERT(tab);
    if (!parent || !tab) return gui_true;
    out = parent->buffer;
    gui_zero(tab, sizeof(*tab));
    tab->valid = !minimized;
    if (!parent->valid) {
        tab->valid = gui_false;
        tab->config = parent->config;
        tab->buffer = parent->buffer;
        tab->input = parent->input;
        return minimized;
    }

    parent->index = 0;
    gui_panel_row(parent, 0, 1);
    gui_panel_alloc_space(&bounds, parent);

    flags = GUI_PANEL_BORDER|GUI_PANEL_MINIMIZABLE|GUI_PANEL_TAB|GUI_PANEL_BORDER_HEADER;
    gui_panel_init(&panel,bounds.x,bounds.y,bounds.w,gui_null_rect.h,flags,out,parent->config);
    panel.minimized = minimized;

    gui_panel_begin(tab, &panel, title, parent->input);
    gui_unify(&clip, &parent->clip, tab->clip.x, tab->clip.y, tab->clip.x + tab->clip.w,
        tab->clip.y + tab->clip.h);
    gui_command_buffer_push_scissor(out, clip.x, clip.y, clip.w, clip.h);
    return panel.minimized;
}

void
gui_panel_tab_end(struct gui_panel_layout *p, struct gui_panel_layout *t)
{
    struct gui_panel panel;
    struct gui_command_buffer *out;
    struct gui_vec2 item_spacing;
    GUI_ASSERT(t);
    GUI_ASSERT(p);
    if (!p || !t || !p->valid)
        return;

    panel.x = t->at_x;
    panel.y = t->y;
    panel.flags = GUI_PANEL_BORDER|GUI_PANEL_MINIMIZABLE|GUI_PANEL_TAB;
    gui_panel_end(t, &panel);

    item_spacing = gui_config_property(p->config, GUI_PROPERTY_ITEM_SPACING);
    p->at_y += t->height + item_spacing.y;
    out = p->buffer;
    gui_command_buffer_push_scissor(out, p->clip.x, p->clip.y, p->clip.w, p->clip.h);
}

void
gui_panel_group_begin(struct gui_panel_layout *p, struct gui_panel_layout *g,
    const char *title, gui_float offset)
{
    gui_flags flags;
    struct gui_rect bounds;
    struct gui_command_buffer *out;
    struct gui_rect clip;
    struct gui_panel panel;
    const struct gui_rect *c;

    GUI_ASSERT(p);
    GUI_ASSERT(g);
    if (!p || !g) return;
    if (!p->valid)
        goto failed;

    gui_panel_alloc_space(&bounds, p);
    gui_zero(g, sizeof(*g));
    c = &p->clip;
    if (!GUI_INTERSECT(c->x, c->y, c->w, c->h, bounds.x, bounds.y, bounds.w, bounds.h))
        goto failed;

    out = p->buffer;
    flags = GUI_PANEL_BORDER|GUI_PANEL_SCROLLBAR|GUI_PANEL_TAB;
    gui_panel_init(&panel, bounds.x,bounds.y,bounds.w,bounds.h,flags, out, p->config);

    gui_panel_begin(g, &panel, title, p->input);
    g->offset = offset;
    gui_unify(&clip, &p->clip, g->clip.x, g->clip.y, g->clip.x + g->clip.w,
        g->clip.y + g->clip.h);
    gui_command_buffer_push_scissor(out, clip.x, clip.y, clip.w, clip.h);
    return;

failed:
    g->valid = gui_false;
    g->config = p->config;
    g->buffer = p->buffer;
    g->input = p->input;
}

gui_float
gui_panel_group_end(struct gui_panel_layout *p, struct gui_panel_layout *g)
{
    struct gui_command_buffer *out;
    struct gui_rect clip;
    struct gui_panel pan;

    GUI_ASSERT(p);
    GUI_ASSERT(g);
    if (!p || !g) return 0;
    if (!p->valid) return 0;

    out = p->buffer;
    pan.x = g->at_x;
    pan.y = g->y;
    pan.flags = GUI_PANEL_BORDER|GUI_PANEL_MINIMIZABLE|GUI_PANEL_TAB|GUI_PANEL_SCROLLBAR;

    gui_unify(&clip, &p->clip, g->clip.x, g->clip.y, g->x + g->w, g->y + g->h);
    gui_command_buffer_push_scissor(out, clip.x, clip.y, clip.w, clip.h);
    gui_panel_end(g, &pan);
    gui_command_buffer_push_scissor(out, p->clip.x, p->clip.y, p->clip.w, p->clip.h);
    return pan.offset;
}

gui_size
gui_panel_shelf_begin(struct gui_panel_layout *parent, struct gui_panel_layout *shelf,
    const char *tabs[], gui_size size, gui_size active, gui_float offset)
{
    const struct gui_config *config;
    struct gui_command_buffer *out;
    const struct gui_font *font;
    struct gui_vec2 item_spacing;
    struct gui_vec2 item_padding;
    struct gui_vec2 panel_padding;

    struct gui_rect bounds;
    struct gui_rect clip;
    struct gui_rect *c;
    struct gui_panel panel;
    struct gui_button button;

    gui_float header_x, header_y;
    gui_float header_w, header_h;
    gui_float item_width;
    gui_flags flags;
    gui_size i;

    GUI_ASSERT(parent);
    GUI_ASSERT(tabs);
    GUI_ASSERT(shelf);
    GUI_ASSERT(active < size);
    if (!parent || !shelf || !tabs || active >= size)
        return active;

    if (!parent->valid)
        goto failed;

    config = parent->config;
    out = parent->buffer;
    font = &config->font;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    item_spacing = gui_config_property(config, GUI_PROPERTY_ITEM_SPACING);
    panel_padding = gui_config_property(config, GUI_PROPERTY_PADDING);

    gui_panel_alloc_space(&bounds, parent);
    gui_zero(shelf, sizeof(*shelf));
    c = &parent->clip;
    if (!GUI_INTERSECT(c->x, c->y, c->w, c->h, bounds.x, bounds.y, bounds.w, bounds.h))
        goto failed;

    header_x = bounds.x;
    header_y = bounds.y;
    header_w = bounds.w;
    header_h = panel_padding.y + 3 * item_padding.y + config->font.height;
    item_width = (header_w - (gui_float)size) / (gui_float)size;

    button.border = 1;
    button.rounding = 0;
    button.padding.x = item_padding.x;
    button.padding.y = item_padding.y;
    button.foreground = config->colors[GUI_COLOR_BORDER];

    for (i = 0; i < size; i++) {
        gui_float button_x, button_y;
        gui_float button_w, button_h;
        gui_size text_width = font->width(font->userdata,
            (const gui_char*)tabs[i], gui_strsiz(tabs[i]));
        text_width = text_width + (gui_size)(2 * item_spacing.x);

        button_y = header_y;
        button_h = header_h;
        button_x = header_x;
        button_w = MIN(item_width, (gui_float)text_width);
        header_x += MIN(item_width, (gui_float)text_width);

        if ((button_x + button_w) >= (bounds.x + bounds.w)) break;
        if (active != i) {
            button_y += item_padding.y;
            button_h -= item_padding.y;
            button.background = config->colors[GUI_COLOR_SHELF];
            button.content = config->colors[GUI_COLOR_SHELF_TEXT];
            button.highlight = config->colors[GUI_COLOR_SHELF];
            button.highlight_content = config->colors[GUI_COLOR_SHELF_TEXT];
        } else {
            button.background = config->colors[GUI_COLOR_SHELF_ACTIVE];
            button.content = config->colors[GUI_COLOR_SHELF_ACTIVE_TEXT];
            button.highlight = config->colors[GUI_COLOR_SHELF_ACTIVE];
            button.highlight_content = config->colors[GUI_COLOR_SHELF_ACTIVE_TEXT];
        }

        if (gui_button_text(out, button_x, button_y, button_w, button_h,
            tabs[i], GUI_BUTTON_DEFAULT, &button, parent->input, &config->font))
            active = i;
    }

    bounds.y += header_h;
    bounds.h -= header_h;

    flags = GUI_PANEL_BORDER|GUI_PANEL_SCROLLBAR|GUI_PANEL_TAB|GUI_PANEL_NO_HEADER;
    gui_panel_init(&panel, bounds.x, bounds.y, bounds.w, bounds.h, flags, out, config);
    gui_panel_begin(shelf, &panel, NULL, parent->input);
    shelf->offset = offset;

    gui_unify(&clip, &parent->clip, shelf->clip.x, shelf->clip.y,
        shelf->clip.x + shelf->clip.w, shelf->clip.y + shelf->clip.h);
    gui_command_buffer_push_scissor(out, clip.x, clip.y, clip.w, clip.h);
    return active;

failed:
    shelf->valid = gui_false;
    shelf->config = parent->config;
    shelf->buffer = parent->buffer;
    shelf->input = parent->input;
    return active;
}

gui_float
gui_panel_shelf_end(struct gui_panel_layout *p, struct gui_panel_layout *s)
{
    struct gui_command_buffer *out;
    struct gui_rect clip;
    struct gui_panel pan;

    GUI_ASSERT(p);
    GUI_ASSERT(s);
    if (!p || !s) return 0;
    if (!p->valid) return 0;

    out = p->buffer;
    pan.x = s->at_x; pan.y = s->y;
    pan.flags = GUI_PANEL_BORDER|GUI_PANEL_MINIMIZABLE|GUI_PANEL_TAB|GUI_PANEL_SCROLLBAR;

    gui_unify(&clip, &p->clip, s->clip.x, s->clip.y, s->x + s->w, s->y + s->h);
    gui_command_buffer_push_scissor(out, clip.x, clip.y, clip.w, clip.h);
    gui_panel_end(s, &pan);
    gui_command_buffer_push_scissor(out, p->clip.x, p->clip.y, p->clip.w, p->clip.h);
    return pan.offset;
}

void
gui_panel_end(struct gui_panel_layout *layout, struct gui_panel *panel)
{
    const struct gui_config *config;
    struct gui_command_buffer *out;
    gui_float scrollbar_width;
    struct gui_vec2 item_padding;
    struct gui_vec2 item_spacing;
    struct gui_vec2 panel_padding;
    struct gui_vec2 scaler_size;

    GUI_ASSERT(layout);
    GUI_ASSERT(panel);
    if (!panel || !layout) return;
    layout->at_y += layout->row_height;

    config = layout->config;
    out = layout->buffer;
    item_padding = gui_config_property(config, GUI_PROPERTY_ITEM_PADDING);
    item_spacing = gui_config_property(config, GUI_PROPERTY_ITEM_SPACING);
    panel_padding = gui_config_property(config, GUI_PROPERTY_PADDING);
    scrollbar_width = gui_config_property(config, GUI_PROPERTY_SCROLLBAR_WIDTH).x;
    scaler_size = gui_config_property(config, GUI_PROPERTY_SCALER_SIZE);
    if (!(panel->flags & GUI_PANEL_TAB))
        gui_command_buffer_push_scissor(out, layout->x,layout->y,layout->w+1,layout->h+1);

    if (panel->flags & GUI_PANEL_SCROLLBAR && layout->valid) {
        struct gui_scroll scroll;
        gui_float panel_y;
        gui_float scroll_x, scroll_y;
        gui_float scroll_w, scroll_h;
        gui_float scroll_target, scroll_offset, scroll_step;

        scroll_x = layout->at_x + layout->width;
        scroll_y = (panel->flags & GUI_PANEL_BORDER) ? layout->y + 1 : layout->y;
        scroll_y += layout->header_height;
        scroll_w = scrollbar_width;
        scroll_h = layout->height;
        scroll_offset = layout->offset;
        scroll_step = layout->height * 0.25f;
        scroll.rounding = config->rounding[GUI_ROUNDING_SCROLLBAR];
        scroll.background = config->colors[GUI_COLOR_SCROLLBAR];
        scroll.foreground = config->colors[GUI_COLOR_SCROLLBAR_CURSOR];
        scroll.border = config->colors[GUI_COLOR_SCROLLBAR_BORDER];
        if (panel->flags & GUI_PANEL_BORDER) scroll_h -= 1;
        scroll_target = (layout->at_y-layout->y)-(layout->header_height+2*item_spacing.y);
        panel->offset = gui_scroll(out, scroll_x, scroll_y, scroll_w, scroll_h,
                                    scroll_offset, scroll_target, scroll_step,
                                    &scroll, layout->input);

        panel_y = layout->y + layout->height + layout->header_height - panel_padding.y;
        gui_command_buffer_push_rect(out, layout->x,panel_y,layout->width,panel_padding.y,
            0, config->colors[GUI_COLOR_PANEL]);
    } else layout->height = layout->at_y - layout->y;

    if ((panel->flags & GUI_PANEL_SCALEABLE) && layout->valid) {
        struct gui_color col = config->colors[GUI_COLOR_SCALER];
        gui_float scaler_w = MAX(0, scaler_size.x - item_padding.x);
        gui_float scaler_h = MAX(0, scaler_size.y - item_padding.y);
        gui_float scaler_x = (layout->x + layout->w) - (item_padding.x + scaler_w);
        gui_float scaler_y = layout->y + layout->h - scaler_size.y;
        gui_command_buffer_push_triangle(out, scaler_x + scaler_w, scaler_y,
            scaler_x + scaler_w, scaler_y + scaler_h, scaler_x, scaler_y + scaler_h, col);
    }

    if (panel->flags & GUI_PANEL_BORDER) {
        const gui_float width = (panel->flags & GUI_PANEL_SCROLLBAR) ?
                layout->width + scrollbar_width : layout->width;
        const gui_float padding_y = (!layout->valid) ?
                panel->y + layout->header_height:
                (panel->flags & GUI_PANEL_SCROLLBAR) ?
                panel->y + layout->h :
                panel->y + layout->height + item_padding.y;

        gui_command_buffer_push_line(out, panel->x, padding_y, panel->x + width,
                padding_y, config->colors[GUI_COLOR_BORDER]);
        gui_command_buffer_push_line(out, panel->x, panel->y, panel->x,
                padding_y, config->colors[GUI_COLOR_BORDER]);
        gui_command_buffer_push_line(out, panel->x + width, panel->y, panel->x + width,
                padding_y, config->colors[GUI_COLOR_BORDER]);
    }
    gui_command_buffer_push_scissor(out, 0, 0, gui_null_rect.w, gui_null_rect.h);
}

/*
 * ==============================================================
 *
 *                          Stack
 *
 * ===============================================================
 */
void
gui_stack_clear(struct gui_stack *stack)
{
    stack->begin = NULL;
    stack->end = NULL;
    stack->count = 0;
}

void
gui_stack_push(struct gui_stack *stack, struct gui_panel *panel)
{
    struct gui_panel *iter;
    GUI_ASSERT(stack);
    GUI_ASSERT(panel);
    if (!stack || !panel) return;
    gui_foreach_panel(iter, stack)
        if (iter == panel) return;

    if (!stack->begin) {
        panel->next = NULL;
        panel->prev = NULL;
        stack->begin = panel;
        stack->end = panel;
        stack->count = 1;
        return;
    }

    stack->end->next = panel;
    panel->prev = stack->end;
    panel->next = NULL;
    stack->end = panel;
    stack->count++;
}

void
gui_stack_pop(struct gui_stack *stack, struct gui_panel*panel)
{
    GUI_ASSERT(stack);
    GUI_ASSERT(panel);
    if (!stack || !panel) return;

    if (panel->prev)
        panel->prev->next = panel->next;
    if (panel->next)
        panel->next->prev = panel->prev;
    if (stack->begin == panel)
        stack->begin = panel->next;
    if (stack->end == panel)
        stack->end = panel->prev;
    panel->next = NULL;
    panel->prev = NULL;
}

/*
 * ==============================================================
 *
 *                          Layout
 *
 * ===============================================================
 */
void
gui_layout_init(struct gui_layout *layout, const struct gui_layout_config *config,
    gui_size width, gui_size height)
{
    gui_float left, right;
    gui_float centerh, centerv;
    gui_float bottom, top;

    GUI_ASSERT(layout);
    GUI_ASSERT(config);
    if (!layout || !config) return;

    gui_zero(layout, sizeof(*layout));
    layout->state = GUI_LAYOUT_ACTIVE;
    layout->width = width;
    layout->height = height;

    left = GUI_SATURATE(config->left);
    right = GUI_SATURATE(config->right);
    centerh = GUI_SATURATE(config->centerh);
    centerv = GUI_SATURATE(config->centerv);
    bottom = GUI_SATURATE(config->bottom);
    top = GUI_SATURATE(config->top);

    layout->slots[GUI_SLOT_TOP].ratio = gui_vec2(1.0f, top);
    layout->slots[GUI_SLOT_LEFT].ratio = gui_vec2(left, centerv);
    layout->slots[GUI_SLOT_BOTTOM].ratio = gui_vec2(1.0f, bottom);
    layout->slots[GUI_SLOT_CENTER].ratio = gui_vec2(centerh, centerv);
    layout->slots[GUI_SLOT_RIGHT].ratio = gui_vec2(right, centerv);

    layout->slots[GUI_SLOT_TOP].offset = gui_vec2(0.0f, 0.0f);
    layout->slots[GUI_SLOT_LEFT].offset = gui_vec2(0.0f, top);
    layout->slots[GUI_SLOT_BOTTOM].offset = gui_vec2(0.0f, top + centerv);
    layout->slots[GUI_SLOT_CENTER].offset = gui_vec2(left, top);
    layout->slots[GUI_SLOT_RIGHT].offset = gui_vec2(left + centerh, top);
}

void
gui_layout_set_size(struct gui_layout *layout, gui_size width, gui_size height)
{
    GUI_ASSERT(layout);
    if (!layout) return;
    layout->width = width;
    layout->height = height;
}

void
gui_layout_slot(struct gui_layout *layout, enum gui_layout_slot_index slot,
    enum gui_layout_format format, gui_size count)
{
    GUI_ASSERT(layout);
    GUI_ASSERT(count);
    GUI_ASSERT(slot >= GUI_SLOT_TOP && slot < GUI_SLOT_MAX);
    if (!layout || !count) return;
    layout->slots[slot].capacity = count;
    layout->slots[slot].format = format;
}

#endif
#ifdef __cplusplus
}
#endif
#endif /* GUI_H_ */
