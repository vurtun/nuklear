/*
    Copyright (c) 2015
    vurtun <polygone@gmx.net>
    MIT licence

    GUI
    -----------------------
    This two file provide both the interface and implementation for a bloat free
    minimal state immediate mode graphical user interface toolkit. The Toolkit
    does not have any library or runtine dependencies like libc but does not
    handle os window/input management, have a render backend or a font library which
    need to be provided by the user.
*/
#ifndef GUI_H_
#define GUI_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Constants */
#define GUI_UTF_INVALID 0xFFFD
#define GUI_UTF_SIZE 4
/* describes the number of bytes a glyph consists of*/
#define GUI_INPUT_MAX 16
/* defines the max number of bytes to be added as text input in one frame */
#define GUI_MAX_COLOR_STACK 32
/* defines the number of temporary configuration color changes that can be stored */
#define GUI_MAX_ATTRIB_STACK 32
/* defines the number of temporary configuration attribute changes that can be stored */

/* Compiler switches */
#define GUI_COMPILE_WITH_FIXED_TYPES 1
/* setting this define to 1 adds the <stdint.h> header for fixed sized types
 * if 0 each type has to be set to the correct size*/
#define GUI_COMPILE_WITH_STD_ASSERT 1
/* setting this define to 1 adds the <assert.h> header for the assert macro
  IMPORTANT: it also adds clib so only use it if wanted */

#if GUI_COMPILE_WITH_FIXED_TYPES
#include <stdint.h>
typedef char gui_char;
typedef int32_t gui_int;
typedef int32_t gui_bool;
typedef int16_t gui_short;
typedef int64_t gui_long;
typedef float gui_float;
typedef double gui_double;
typedef uint16_t gui_ushort;
typedef uint32_t gui_uint;
typedef uint64_t gui_ulong;
typedef uint32_t gui_flags;
typedef gui_flags gui_state;
typedef uint8_t gui_byte;
typedef uint32_t gui_flag;
typedef uint64_t gui_size;
typedef uintptr_t gui_ptr;
#else
typedef int gui_int;
typedef int gui_bool;
typedef char gui_char;
typedef short gui_short;
typedef long gui_long;
typedef float gui_float;
typedef double gui_double;
typedef unsigned short gui_ushort;
typedef unsigned int gui_uint;
typedef unsigned long gui_ulong;
typedef unsigned int gui_flags;
typedef gui_flags gui_state;
typedef unsigned char gui_byte;
typedef unsigned int gui_flag;
typedef unsigned long gui_size;
typedef unsigned long gui_ptr;
#endif

#if GUI_COMPILE_WITH_STD_ASSERT
#ifndef GUI_ASSERT
#include <assert.h>
#define GUI_ASSERT(expr) assert(expr)
#endif
#else
#define GUI_ASSERT(expr)
#endif

/* Utilities */
enum {gui_false, gui_true};
enum gui_heading {GUI_UP, GUI_RIGHT, GUI_DOWN, GUI_LEFT};
struct gui_color {gui_byte r,g,b,a;};
struct gui_vec2 {gui_float x,y;};
struct gui_vec2i {gui_short x, y;};
struct gui_rect {gui_float x,y,w,h;};
struct gui_key {gui_bool down, clicked;};
typedef gui_char gui_glyph[GUI_UTF_SIZE];
typedef union {void *ptr; gui_int id;} gui_handle;
struct gui_image {gui_handle handle; struct gui_rect region;};
enum gui_widget_states {GUI_INACTIVE = gui_false, GUI_AYOUT_ACTIVE = gui_true};
enum gui_collapse_states {GUI_MINIMIZED = gui_false, GUI_MAXIMIZED = gui_true};

/* Callbacks */
struct gui_font;
struct gui_edit_box;
typedef gui_bool(*gui_filter)(gui_long unicode);
typedef gui_size(*gui_text_width_f)(gui_handle, const gui_char*, gui_size);
typedef void(*gui_paste_f)(gui_handle, struct gui_edit_box*);
typedef void(*gui_copy_f)(gui_handle, const char*, gui_size size);
/*
 * ==============================================================
 *
 *                          Utility
 *
 * ===============================================================
 */
/*  Utility
    ----------------------------
    The utility API provides mainly a number of object construction function
    for some gui specific objects like image handle, vector, color and rectangle.

    USAGE
    ----------------------------
    Utility function API
    gui_get_null_rect   -- returns a default clipping rectangle
    gui_utf_decode      -- decodes a utf-8 glyph into u32 unicode glyph and len
    gui_utf_encode      -- encodes a u32 unicode glyph into a utf-8 glyph
    gui_image_ptr       -- create a image handle from pointer
    gui_image_id        -- create a image handle from integer id
    gui_subimage_ptr    -- create a sub-image handle from pointer and region
    gui_subimage_id     -- create a sub-image handle from integer id and region
    gui_rect_is_valid   -- check if a rectangle inside the image command is valid
    gui_rect            -- creates a rectangle from x,y-Position and width and height
    gui_vec2            -- creates a 2D vector, in the best case should not be needed by the user
    gui_rgba            -- create a gui color struct from rgba color code
    gui_rgb             -- create a gui color struct from rgb color code
*/
struct gui_rect gui_get_null_rect(void);
gui_size gui_utf_decode(const gui_char*, gui_long*, gui_size);
gui_size gui_utf_encode(gui_long, gui_char*, gui_size);
gui_size gui_utf_len(const gui_char*, gui_size len);
struct gui_image gui_image_ptr(void*);
struct gui_image gui_image_id(gui_int);
struct gui_image gui_subimage_ptr(void*, struct gui_rect);
struct gui_image gui_subimage_id(gui_int, struct gui_rect);
gui_bool gui_rect_is_valid(const struct gui_rect r);
gui_bool gui_image_is_subimage(const struct gui_image* img);
struct gui_rect gui_rect(gui_float x, gui_float y, gui_float w, gui_float h);
struct gui_vec2 gui_vec2(gui_float x, gui_float y);
struct gui_color gui_rgba(gui_byte r, gui_byte g, gui_byte b, gui_byte a);
struct gui_color gui_rgb(gui_byte r, gui_byte g, gui_byte b);
#define gui_ptr_add(t, p, i) ((t*)((void*)((gui_byte*)(p) + (i))))
#define gui_ptr_sub(t, p, i) ((t*)((void*)((gui_byte*)(p) - (i))))
#define gui_ptr_add_const(t, p, i) ((const t*)((const void*)((const gui_byte*)(p) + (i))))
#define gui_ptr_sub_const(t, p, i) ((const t*)((const void*)((const gui_byte*)(p) - (i))))

/*
 * ==============================================================
 *
 *                          Input
 *
 * ===============================================================
 */
/*  INPUT
    ----------------------------
    The input API is responsible for holding input by keeping track of
    mouse, key and text input state. The core of the API is the persistent
    gui_input struct which holds the input state while running.
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

    Input query function API
    gui_input_is_mouse_click_in_rect    - checks for up/down click in a rectangle
    gui_input_is_mouse_hovering_rect    - checks if the mouse hovers over a rectangle
    gui_input_mouse_clicked             - checks if mouse hovers + down + clicked in rectangle
    gui_input_is_mouse_down             - checks if the current mouse button is down
    gui_input_is_mouse_released         - checks if mouse button previously released
    gui_input_is_key_pressed            - checks if key was up and now is down
    gui_input_is_key_released           - checks if key was down and is now up
    gui_input_is_key_down               - checks if key is currently down
*/
/* every key that is being used inside the library */
enum gui_keys {
    GUI_KEY_SHIFT,
    GUI_KEY_DEL,
    GUI_KEY_ENTER,
    GUI_KEY_BACKSPACE,
    GUI_KEY_SPACE,
    GUI_KEY_COPY,
    GUI_KEY_CUT,
    GUI_KEY_PASTE,
    GUI_KEY_LEFT,
    GUI_KEY_RIGHT,
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
    /* mouse travelling distance from last to current frame */
    gui_bool mouse_down;
    /* current mouse button state */
    gui_uint mouse_clicked;
    /* number of mouse button state transistions between frames */
    gui_float scroll_delta;
    /* number of steps in the up or down scroll direction */
    struct gui_vec2 mouse_clicked_pos;
    /* mouse position of the last mouse button state change */
};

void gui_input_begin(struct gui_input*);
/*  this function sets the input state to writeable */
void gui_input_motion(struct gui_input*, gui_int x, gui_int y);
/*  this function updates the current mouse position
    Input:
    - local os window X position
    - local os window Y position
*/
void gui_input_key(struct gui_input*, enum gui_keys, gui_bool down);
/*  this function updates the current state of a key
    Input:
    - key identifier
    - the new state of the key
*/
void gui_input_button(struct gui_input*, gui_int x, gui_int y, gui_bool down);
/*  this function updates the current state of the button
    Input:
    - local os window X position
    - local os window Y position
    - the new state of the button
*/
void gui_input_scroll(struct gui_input*, gui_float y);
/*  this function updates the current page scroll delta
    Input:
    - vector with each direction (< 0 down > 0 up and scroll distance)
*/
void gui_input_glyph(struct gui_input*, const gui_glyph);
/*  this function adds a utf-8 glpyh into the internal text frame buffer
    Input:
    - utf8 glyph to add to the text buffer
*/
void gui_input_char(struct gui_input*, char);
/*  this function adds char into the internal text frame buffer
    Input:
    - character to add to the text buffer
*/
void gui_input_end(struct gui_input*);
/*  this function sets the input state to readable */
gui_bool gui_input_is_mouse_click_in_rect(const struct gui_input*, struct gui_rect);
/*  this function returns true if a mouse click inside a rectangle occured */
gui_bool gui_input_is_mouse_hovering_rect(const struct gui_input*, struct gui_rect);
/*  this function returns true if the mouse hovers over a rectangle */
gui_bool gui_input_mouse_clicked(const struct gui_input*, struct gui_rect);
/*  this function returns true if a mouse click inside a rectangle occured
    and the mouse still hovers over the rectangle*/
gui_bool gui_input_is_mouse_down(const struct gui_input*);
/*  this function returns true if the current mouse button is down */
gui_bool gui_input_is_mouse_released(const struct gui_input*);
/*  this function returns true if the mouse button was previously pressed but
    was now released */
gui_bool gui_input_is_key_pressed(const struct gui_input*, enum gui_keys);
/*  this function returns true if the given key was up and is now pressed */
gui_bool gui_input_is_key_released(const struct gui_input*, enum gui_keys);
/*  this function returns true if the given key was down and is now up */
gui_bool gui_input_is_key_down(const struct gui_input*, enum gui_keys);
/*  this function returns true if the given key was down and is now up */
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
    of the user. The memory is provided in three different ways.
    The first way is to use a fixed size block of memory to be filled up.
    Biggest advantage is a simple memory model. Downside is that if the buffer
    is full there is no way to accesses more memory, which fits target application
    with a GUI with roughly known memory consumptions.
    The second way to mnamge memory is by extending the fixed size block by querying
    information from the buffer about the used size and needed size and allocate new
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
    To instantiate the buffer you either have to call the fixed size or allocator
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
    gui_buffer_info         -- provides buffer memory information
    gui_buffer_alloc        -- allocates a block of memory from the buffer
    gui_buffer_clear        -- resets the buffer back to an empty state
    gui_buffer_free         -- frees all memory if the buffer is dynamic
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
    /* dynamically growing buffer */
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
    /* number of allocation calls */
};

void gui_buffer_init(struct gui_buffer*, const struct gui_allocator*,
                        gui_size initial_size, gui_float grow_factor);
/*  this function initializes a growing buffer
    Input:
    - allocator holding your own alloctator and memory allocation callbacks
    - initial size of the buffer
    - factor to grow the buffer by if the buffer is full
    Output:
    - dynamically growing buffer
*/
void gui_buffer_init_fixed(struct gui_buffer*, void *memory, gui_size size);
/*  this function initializes a fixed size buffer
    Input:
    - fixed size previously allocated memory block
    - size of the memory block
    Output:
    - fixed size buffer
*/
void gui_buffer_info(struct gui_memory_status*, struct gui_buffer*);
/*  this function requests memory information from a buffer
    Input:
    - buffer to get the inforamtion from
    Output:
    - buffer memory information
*/
void *gui_buffer_alloc(struct gui_buffer*, gui_size size, gui_size align);
/*  this functions allocated a aligned memory block from a buffer
    Input:
    - buffer to allocate memory from
    - size of the requested memory block
    - alignment requirement for the memory block
    Output:
    - memory block with given size and alignment requirement
*/
void gui_buffer_clear(struct gui_buffer*);
/*  this functions resets the buffer back into an empty state */
void gui_buffer_free(struct gui_buffer*);
/*  this functions frees all memory inside a dynamically growing buffer */
/*
 * ==============================================================
 *
 *                          Command Buffer
 *
 * ===============================================================
 */
/*  COMMAND BUFFER
    ----------------------------
    The command buffer API queues draw calls as commands into a buffer and
    therefore abstracts over drawing routines and enables defered drawing.
    The API offers a number of drawing primitives like lines, rectangles, circles,
    triangles, images, text and clipping rectangles, that have to be drawn by the user.
    Therefore the command buffer is the main toolkit output besides the actual widget output.
    The actual draw command execution is done by the user and is build up in a
    interpreter like fashion by iterating over all commands and executing each
    command differently depending on the command type.

    USAGE
    ----------------------------
    To use the command buffer you first have to initiate the command buffer with a
    buffer. After the initilization you can add primitives by
    calling the appropriate gui_command_buffer_XXX for each primitive.
    To iterate over each commands inside the buffer gui_foreach_command is
    provided. Finally to reuse the buffer after the frame use the
    gui_command_buffer_reset function. If used without a command queue the command
    buffer has be cleared after each frame to reset the buffer back into a
    empty state.

    command buffer function API
    gui_command_buffer_init         -- initializes a command buffer with a buffer
    gui_command_buffer_clear        -- resets the command buffer and internal buffer
    gui_command_buffer_reset        -- resets the command buffer but not the buffer

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
    gui_foreach_buffer_command      -- iterates over all commands in a queue
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
    gui_size next;
    /* absolute base pointer offset to the next command */
};

struct gui_command_scissor {
    struct gui_command header;
    gui_short x, y;
    gui_ushort w, h;
};

struct gui_command_line {
    struct gui_command header;
    struct gui_vec2i begin;
    struct gui_vec2i end;
    struct gui_color color;
};

struct gui_command_rect {
    struct gui_command header;
    gui_uint rounding;
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
    struct gui_image img;
};

struct gui_command_triangle {
    struct gui_command header;
    struct gui_vec2i a;
    struct gui_vec2i b;
    struct gui_vec2i c;
    struct gui_color color;
};

struct gui_command_text {
    struct gui_command header;
    gui_handle font;
    struct gui_color background;
    struct gui_color foreground;
    gui_short x, y;
    gui_ushort w, h;
    gui_size length;
    gui_char string[1];
};

enum gui_command_clipping {
    GUI_NOCLIP = gui_false,
    GUI_CLIP = gui_true
};

struct gui_command_queue;
struct gui_command_buffer {
    struct gui_buffer *base;
    /* memory buffer to store the command */
    struct gui_rect clip;
    /* current clipping rectangle */
    gui_bool use_clipping;
    /* flag if the command buffer should clip commands */
    struct gui_command_queue *queue;
    struct gui_command_buffer *next;
    struct gui_command_buffer *prev;
    gui_size begin, end, last;
};

void gui_command_buffer_init(struct gui_command_buffer*, struct gui_buffer*,
                                enum gui_command_clipping);
/*  this function intializes the command buffer
    Input:
    - memory buffer to store the commands into
    - clipping flag for removing non-visible draw commands
*/
void gui_command_buffer_reset(struct gui_command_buffer*);
/*  this function clears the command buffer but not the internal memory buffer */
void gui_command_buffer_clear(struct gui_command_buffer*);
/*  this function clears the command buffer and internal memory buffer */
void *gui_command_buffer_push(struct gui_command_buffer*, gui_uint type, gui_size size);
/*  this function push enqueues a command into the buffer
    Input:
    - buffer to push the command into
    - type of the command
    - amount of memory that is needed for the specified command
*/
void gui_command_buffer_push_scissor(struct gui_command_buffer*, struct gui_rect);
/*  this function push a clip rectangle command into the buffer
    Input:
    - buffer to push the clip rectangle command into
    - x,y and width and height of the clip rectangle
*/
void gui_command_buffer_push_line(struct gui_command_buffer*, gui_float, gui_float,
                                    gui_float, gui_float, struct gui_color);
/*  this function pushes a line draw command into the buffer
    Input:
    - buffer to push the clip rectangle command into
    - starting position of the line
    - ending position of the line
    - color of the line to draw
*/
void
gui_command_buffer_push_rect(struct gui_command_buffer*, struct gui_rect,
                                gui_float rounding, struct gui_color color);
/*  this function pushes a rectangle draw command into the buffer
    Input:
    - buffer to push the draw rectangle command into
    - rectangle bounds
    - rectangle edge rounding
    - color of the rectangle to draw
*/
void gui_command_buffer_push_circle(struct gui_command_buffer*, struct gui_rect,
                                    struct gui_color c);
/*  this function pushes a circle draw command into the buffer
    Input:
    - buffer to push the circle draw command into
    - rectangle bounds of the circle
    - color of the circle to draw
*/
void gui_command_buffer_push_triangle(struct gui_command_buffer*, gui_float, gui_float,
                                        gui_float, gui_float, gui_float, gui_float,
                                        struct gui_color);
/*  this function pushes a triangle draw command into the buffer
    Input:
    - buffer to push the draw triangle command into
    - (x,y) coordinates of all three points
    - rectangle diameter of the circle
    - color of the triangle to draw
*/
void gui_command_buffer_push_image(struct gui_command_buffer*, struct gui_rect,
                                    struct gui_image*);
/*  this function pushes a image draw command into the buffer
    Input:
    - buffer to push the draw image command into
    - bounds of the image to draw with position, width and height
    - rectangle diameter of the circle
    - color of the triangle to draw
*/
void gui_command_buffer_push_text(struct gui_command_buffer*, struct gui_rect,
                                    const gui_char*, gui_size, const struct gui_font*,
                                    struct gui_color, struct gui_color);
/*  this function pushes a text draw command into the buffer
    Input:
    - buffer to push the draw text command into
    - top left position of the text with x,y position
    - maixmal size of the text to draw with width and height
    - color of the triangle to draw
*/
#define gui_command(t, c) ((const struct gui_command_##t*)c)
/*  this is a small helper makro to cast from a general to a specific command */
#define gui_foreach_buffer_command(i, b)\
    for((i)=gui_command_buffer_begin(b); (i)!=NULL; (i)=gui_command_buffer_next(b,i))
/*  this loop header allow to iterate over each command in a command buffer */
const struct gui_command *gui_command_buffer_begin(struct gui_command_buffer*);
/*  this function returns the first command in the command buffer */
const struct gui_command *gui_command_buffer_next(struct gui_command_buffer*,
                                                struct gui_command*);
/*  this function returns the next command of a given command*/
/*
 * ==============================================================
 *
 *                          Command Queue
 *
 * ===============================================================
 */
/*  COMMAND QUEUE
    ----------------------------
    The command queue extends the command buffer with the possiblity to use
    more than one command buffer on one memory buffer and still only need
    to iterate over one command list. Therefore it is possible to have mutliple
    panels without having to manage each panels individual memory. This greatly
    simplifies and reduces the amount of code needed with just using command buffers.

    Internally the command queue has a list of command buffers which can be
    modified to create a certain sequence, for example the `gui_panel_begin`
    function changes the list to create overlapping panels, while the
    `gui_panel_begin_tiled` function makes sure that its command buffers will
    always be drawn first since panel in tiled layouts are always in the background.

    USAGE
    ----------------------------
    The command queue owns a memory buffer internaly that needs to be initialized
    either as a fixed size or dynamic buffer with functions `gui_commmand_queue_init'
    or `gui_command_queue_init_fixed`. Panels are automaticall added to the command
    queue in the `gui_panel_init` with the `gui_command_queue_add` function
    but removing a panel requires a manual call of `gui_command_queue_remove`.
    Internally the panel calls the `gui_command_queue_start` and
    `gui_commanmd_queue_finish` function that setup and finilize a command buffer for
    command queuing. Finally to iterate over all commands in all command buffers
    the iterator API is provided. It allows to iterate over each command in a
    foreach loop.

    command queue function API
    gui_command_queue_init          -- initializes a dynamic command queue
    gui_command_queue_init_fixed    -- initializes a static command queue
    gui_command_queue_clear         -- frees all memory if the command queue is dynamic
    gui_command_queue_insert_font   -- adds a command buffer in the front of the queue
    gui_command_queue_insert_back   -- adds a command buffer in the back of the queue
    gui_command_queue_remove        -- removes a command buffer from the queue
    gui_command_queue_start         -- begins the command buffer filling process
    gui_command_queue_finish        -- ends the command buffer filling process
    gui_command_queue_start_child   -- begins the child command buffer filling process
    gui_command_queue_finish_child  -- ends the child command buffer filling process

    command iterator function API
    gui_command_queue_begin         -- returns the first command in a queue
    gui_command_queue_next          -- returns the next command in a queue or NULL
    gui_foreach_command             -- iterates over all commands in a queue
*/
struct gui_command_buffer_list {
    gui_size count;
    /* number of panels inside the stack */
    struct gui_command_buffer *begin;
    /* first panel inside the panel which will be drawn first */
    struct gui_command_buffer *end;
    /* currently active panel which will be drawn last */
};

struct gui_command_sub_buffer {
    gui_size begin;
    /* begin of the subbuffer */
    gui_size parent_last;
    /* last entry before the sub buffer*/
    gui_size last;
    /* last entry in the sub buffer*/
    gui_size end;
    /* end of the subbuffer */
    gui_size next;
};

struct gui_command_sub_buffer_stack {
    gui_size count;
    /* number of subbuffers */
    gui_size begin;
    /* buffer offset of the first subbuffer*/
    gui_size end;
    /* buffer offset of the last subbuffer*/
    gui_size size;
    /* real size of the buffer */
};

struct gui_command_queue {
    struct gui_buffer buffer;
    /* memory buffer the hold all commands */
    struct gui_command_buffer_list list;
    /* list of each memory buffer inside the queue */
    struct gui_command_sub_buffer_stack stack;
    /* subbuffer stack for overlapping child panels in panels */
    gui_bool build;
    /* flag indicating if a complete command list was build inside the queue*/
};

void gui_command_queue_init(struct gui_command_queue*, const struct gui_allocator*,
                            gui_size initial_size, gui_float grow_factor);
/*  this function initializes a growing command queue
    Input:
    - allocator holding your own alloctator and memory allocation callbacks
    - initial size of the buffer
    - factor to grow the buffer by if the buffer is full
*/
void gui_command_queue_init_fixed(struct gui_command_queue*, void*, gui_size);
/*  this function initializes a fixed size command queue
    Input:
    - fixed size previously allocated memory block
    - size of the memory block
*/
void gui_command_queue_insert_back(struct gui_command_queue*, struct gui_command_buffer*);
/*  this function adds a command buffer into the back of the queue
    Input:
    - command buffer to add into the queue
*/
void gui_command_queue_insert_front(struct gui_command_queue*, struct gui_command_buffer*);
/*  this function adds a command buffer into the beginning of the queue
    Input:
    - command buffer to add into the queue
*/
void gui_command_queue_remove(struct gui_command_queue*, struct gui_command_buffer*);
/*  this function removes a command buffer from the queue
    Input:
    - command buffer to remove from the queue
*/
void gui_command_queue_start(struct gui_command_queue*, struct gui_command_buffer*);
/*  this function sets up the command buffer to be filled up
    Input:
    - command buffer to fill with commands
*/
void gui_command_queue_finish(struct gui_command_queue*, struct gui_command_buffer*);
/*  this function finishes the command buffer fill up process
    Input:
    - the now filled command buffer
*/
gui_bool gui_command_queue_start_child(struct gui_command_queue*, struct gui_command_buffer*);
/*  this function sets up a child buffer inside a command buffer to be filled up
    Input:
    - command buffer to begin the child buffer in
    Output:
    - gui_true if successful gui_false otherwise
*/
void gui_command_queue_finish_child(struct gui_command_queue*, struct gui_command_buffer*);
/*  this function finishes the child buffer inside the command buffer fill up process
    Input:
    - the command buffer to create the child command buffer in
*/
void gui_command_queue_free(struct gui_command_queue*);
/*  this function clears the internal buffer if it is a dynamic buffer */
void gui_command_queue_clear(struct gui_command_queue*);
/*  this function reset the internal buffer and has to be called every frame */
#define gui_foreach_command(i, q)\
    for((i)=gui_command_queue_begin(q); (i)!=NULL; (i)=gui_command_queue_next(q,i))
/*  this function iterates over each command inside the command queue
    Input:
    - iterator gui_command pointer to iterate over all commands
    - queue to iterate over
*/
void gui_command_queue_build(struct gui_command_queue*);
/*  this function builds the internal queue commmand list out of all buffers.
 *  Only needs be called if gui_command_queue_begin is called in parallel */
const struct gui_command *gui_command_queue_begin(struct gui_command_queue*);
/*  this function returns the first command in the command queue */
const struct gui_command* gui_command_queue_next(struct gui_command_queue*,
                                                const struct gui_command*);
/*  this function returns the next command of a given command*/
/*
 * ===============================================================
 *
 *                          Edit Box
 *
 * ===============================================================
 */
/*  EDIT BOX
    ----------------------------
    The Editbox is for text input with either a fixed or dynamically growing
    buffer. It extends the basic functionality of basic input over `gui_edit`
    and `gui_panel_edit` with basic copy and paste functionality and the possiblity
    to use a extending buffer.

    USAGE
    ----------------------------
    The Editbox first needs to be initialized either with a fixed size
    memory block or a allocator. After that it can be used by either the
    `gui_editobx` or `gui_panel_editbox` function. In addition symbols can be
    added and removed with `gui_edit_box_add` and `gui_edit_box_remove`.

    Widget function API
    gui_edit_box_init         -- initialize a dynamically growing edit box
    gui_edit_box_init_fixed   -- initialize a statically edit box
    gui_edit_box_reset        -- resets the edit box back to the beginning
    gui_edit_box_clear        -- frees all memory of a dynamic edit box
    gui_edit_box_add          -- adds a symbol to the editbox
    gui_edit_box_remove       -- removes a symbol from the editbox
    gui_edit_box_get          -- returns the string inside the editbox
    gui_edit_box_len          -- returns the length of the string inside the edditbox
*/
struct gui_clipboard {
    gui_handle userdata;
    /* user memory for callback */
    gui_paste_f paste;
    /* paste callback for the edit box  */
    gui_copy_f copy;
    /* copy callback for the edit box  */
};

struct gui_selection {
    gui_bool active;
    /* current selection state */
    gui_size begin;
    /* text selection beginning glyph index */
    gui_size end;
    /* text selection ending glyph index */
};

typedef struct gui_buffer gui_edit_buffer;
struct gui_edit_box {
    gui_edit_buffer buffer;
    /* glyph buffer to add text into */
    gui_state active;
    /* flag indicating if the buffer is currently being modified  */
    gui_size cursor;
    /* current glyph (not byte) cursor position */
    gui_size glyphes;
    /* number of glyphes inside the edit box */
    struct gui_clipboard clip;
    /* copy paste callbacks */
    gui_filter filter;
    /* input filter callback */
    struct gui_selection sel;
    /* text selection */
};

/* filter function */
gui_bool gui_filter_default(gui_long unicode);
gui_bool gui_filter_ascii(gui_long unicode);
gui_bool gui_filter_float(gui_long unicode);
gui_bool gui_filter_decimal(gui_long unicode);
gui_bool gui_filter_hex(gui_long unicode);
gui_bool gui_filter_oct(gui_long unicode);
gui_bool gui_filter_binary(gui_long unicode);

/* editbox */
void gui_edit_box_init(struct gui_edit_box*, struct gui_allocator*, gui_size initial,
                        gui_float grow_fac, const struct gui_clipboard*, gui_filter);
/*  this function initializes the editbox a growing buffer
    Input:
    - allocator implementation
    - initital buffer size
    - buffer growing factor
    - clipboard implementation for copy&paste or NULL of not needed
    - character filtering callback to limit input or NULL of not needed
*/
void gui_edit_box_init_fixed(struct gui_edit_box*, void *memory, gui_size size,
                                const struct gui_clipboard*, gui_filter);
/*  this function initializes the editbox a static buffer
    Input:
    - memory block to fill
    - sizeo of the memory block
    - clipboard implementation for copy&paste or NULL of not needed
    - character filtering callback to limit input or NULL of not needed
*/
void gui_edit_box_clear(struct gui_edit_box*);
/*  this function resets the buffer and sets everything back into a clean state */
void gui_edit_box_free(struct gui_edit_box*);
/*  this function frees all internal memory in a dynamically growing buffer */
void gui_edit_box_info(struct gui_memory_status*, struct gui_edit_box*);
/* this function returns information about the memory in use  */
void gui_edit_box_add(struct gui_edit_box*, const char*, gui_size);
/*  this function adds text at the current cursor position
    Input:
    - string buffer or glyph to copy/add to the buffer
    - length of the string buffer or glyph
*/
void gui_edit_box_remove(struct gui_edit_box*);
/*  removes the glyph at the current cursor position */
gui_char *gui_edit_box_get(struct gui_edit_box*);
/* returns the string buffer inside the edit box */
const gui_char *gui_edit_box_get_const(struct gui_edit_box*);
/* returns the constant string buffer inside the edit box */
void gui_edit_box_at(struct gui_edit_box*, gui_size pos, gui_glyph, gui_size*);
/*  this function returns the glyph at a given offset
    Input:
    - glyph offset inside the buffer
    Output:
    - utf8 glyph at the given position
    - byte length of the glyph
*/
void gui_edit_box_at_cursor(struct gui_edit_box*, gui_glyph, gui_size*);
/*  this function returns the glyph at the cursor
    Output:
    - utf8 glyph at the cursor position
    - byte length of the glyph
*/
gui_char gui_edit_box_at_char(struct gui_edit_box*, gui_size pos);
/*  this function returns the character at a given byte position
    Input:
    - character offset inside the buffer
    Output:
    - character at the given position
*/
void gui_edit_box_set_cursor(struct gui_edit_box*, gui_size pos);
/*  this function sets the cursor at a given glyph position
    Input:
    - glyph offset inside the buffer
*/
gui_size gui_edit_box_get_cursor(struct gui_edit_box *eb);
/*  this function returns the cursor glyph position
    Output:
    - cursor glyph offset inside the buffer
*/
gui_size gui_edit_box_len_char(struct gui_edit_box*);
/*  this function returns length of the buffer in bytes
    Output:
    - string buffer byte length
*/
gui_size gui_edit_box_len(struct gui_edit_box*);
/*  this function returns length of the buffer in glyphes
    Output:
    - string buffer glyph length
*/
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
    be managed by the user. It is the basis for the panel API and implements
    the functionality for almost all widgets in the panel API. The widget API
    hereby is more flexible than the panel API in placing and styling but
    requires more work for the user and has no concept for groups of widgets.

    USAGE
    ----------------------------
    Most widgets take an input struct, font and widget specific data and a command
    buffer to push draw command into and return the updated widget state.
    Important to note is that there is no actual state that is being stored inside
    the toolkit so everything has to be stored byte the user.

    Widget function API
    gui_text                -- draws a string inside a box
    gui_button_text         -- button widget with text content
    gui_button_image        -- button widget with icon content
    gui_button_triangle     -- button widget with triangle content
    gui_button_text_triangle-- button widget with triangle and text content
    gui_button_text_image   -- button widget with image and text content
    gui_toggle              -- either a checkbox or radiobutton widget
    gui_slider              -- floating point slider widget
    gui_progress            -- unsigned integer progressbar widget
    gui_editbox             -- Editbox widget for complex user input
    gui_edit                -- Editbox wiget for basic user input
    gui_edit_filtered       -- Editbox with utf8 gylph filter capabilities
    gui_spinner_int         -- integer spinner widget
    gui_spinner_float       -- float spinner widget
    gui_selector            -- string selector widget
    gui_scroll              -- scrollbar widget imeplementation
*/
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

enum gui_button_behavior {
    GUI_BUTTON_DEFAULT,
    /* button only returns on activation */
    GUI_BUTTON_REPEATER,
    /* button returns as long as the button is pressed */
    GUI_BUTTON_MAX
};

struct gui_text {
    struct gui_vec2 padding;
    /* padding between bounds and text */
    struct gui_color foreground;
    /*text color */
    struct gui_color background;
    /* text background color */
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
    enum gui_text_align alignment;
    /* text alignment in the button */
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
    /* progressbar rectangle rounding */
    struct gui_vec2 padding;
    /* padding between bounds and content */
    struct gui_color background;
    /* progressbar background color */
    struct gui_color foreground;
    /* progressbar cursor color */
};

struct gui_slider {
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

struct gui_scrollbar {
    gui_float rounding;
    /* scrollbar rectangle rounding */
    struct gui_color highlight;
    /* button highlight color  */
    struct gui_color highlight_content;
    /* button content highlight color */
    struct gui_color background;
    /* scrollbar background color */
    struct gui_color foreground;
    /* scrollbar cursor color */
    struct gui_color border;
    /* scrollbar border color */
    gui_bool has_scrolling;
    /* flag if the scrollbar can be updated by scrolling */
};

enum gui_input_filter {
    GUI_INPUT_DEFAULT,
    /* everything goes */
    GUI_INPUT_ASCII,
    /* ASCII characters (0-127)*/
    GUI_INPUT_FLOAT,
    /* only float point numbers */
    GUI_INPUT_DEC,
    /* only integer numbers */
    GUI_INPUT_HEX,
    /* only hex numbers */
    GUI_INPUT_OCT,
    /* only octal numbers */
    GUI_INPUT_BIN
    /* only binary numbers */
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
    /* selector background color */
    struct gui_color border;
    /* selector border color */
    struct gui_color text;
    /* selector text color */
    struct gui_color text_bg;
    /* selector text background color */
    struct gui_vec2 padding;
    /* padding between bounds and content*/
};

void gui_text(struct gui_command_buffer*, struct gui_rect,
                const char*, gui_size, const struct gui_text*,
                enum gui_text_align, const struct gui_font*);
/*  this function executes a text widget with text alignment
    Input:
    - output command buffer for drawing
    - text bounds
    - string to draw
    - length of the string
    - visual widget style structure describing the text
    - text alignment with either left, center and right
    - font structure for text drawing
*/
gui_bool gui_button_text(struct gui_command_buffer*, struct gui_rect,
                        const char*, enum gui_button_behavior,
                        const struct gui_button*, const struct gui_input*,
                        const struct gui_font*);
/*  this function executes a text button widget
    Input:
    - output command buffer for drawing
    - text button widget bounds
    - button text
    - button behavior with either repeating or transition state event
    - visual widget style structure describing the button
    - input structure to update the button with
    - font structure for text drawing
    Output:
    - returns gui_true if the button was pressed gui_false otherwise
*/
gui_bool gui_button_image(struct gui_command_buffer*, struct gui_rect,
                            struct gui_image, enum gui_button_behavior,
                            const struct gui_button*, const struct gui_input*);
/*  this function executes a image button widget
    Input:
    - output command buffer for drawing
    - (x,y) position
    - (width, height) size
    - user provided image handle which is either a pointer or a id
    - button behavior with either repeating or transition state event
    - visual widget style structure describing the button
    - input structure to update the button with
    Output:
    - returns gui_true if the button was pressed gui_false otherwise
*/
gui_bool gui_button_triangle(struct gui_command_buffer*, struct gui_rect,
                            enum gui_heading, enum gui_button_behavior,
                            const struct gui_button*, const struct gui_input*);
/*  this function executes a triangle button widget
    Input:
    - output command buffer for drawing
    - triangle button bounds
    - triangle direction with either left, top, right xor bottom
    - button behavior with either repeating or transition state event
    - visual widget style structure describing the button
    - input structure to update the button with
    Output:
    - returns gui_true if the button was pressed gui_false otherwise
*/
gui_bool gui_button_text_triangle(struct gui_command_buffer*, struct gui_rect,
                                enum gui_heading, const char*, enum gui_text_align,
                                enum gui_button_behavior, const struct gui_button*,
                                const struct gui_font*, const struct gui_input*);
/*  this function executes a button with text and a triangle widget
    Input:
    - output command buffer for drawing
    - bounds of the text triangle widget
    - triangle direction with either left, top, right xor bottom
    - button text
    - text alignment with either left, center and right
    - button behavior with either repeating or transition state event
    - visual widget style structure describing the button
    - font structure for text drawing
    - input structure to update the button with
    Output:
    - returns gui_true if the button was pressed gui_false otherwise
*/
gui_bool gui_button_text_image(struct gui_command_buffer*, struct gui_rect,
                            struct gui_image, const char*, enum gui_text_align,
                            enum gui_button_behavior, const struct gui_button*,
                            const struct gui_font*, const struct gui_input*);
/*  this function executes a button widget with text and an icon
    Input:
    - output command buffer for drawing
    - bounds of the text image widgets
    - user provided image handle which is either a pointer or a id
    - button text
    - text alignment with either left, center and right
    - button behavior with either repeating or transition state event
    - visual widget style structure describing the button
    - font structure for text drawing
    - input structure to update the button with
    Output:
    - returns gui_true if the button was pressed gui_false otherwise
*/
gui_bool gui_toggle(struct gui_command_buffer*, struct gui_rect,
                    gui_bool active, const char *string, enum gui_toggle_type,
                    const struct gui_toggle*, const struct gui_input*,
                    const struct gui_font*);
/*  this function executes a toggle (checkbox, radiobutton) widget
    Input:
    - output command buffer for drawing
    - bounds of the toggle
    - active or inactive flag describing the state of the toggle
    - visual widget style structure describing the toggle
    - input structure to update the toggle with
    - font structure for text drawing
    Output:
    - returns the update state of the toggle
*/
gui_float gui_slider(struct gui_command_buffer*, struct gui_rect,
                    gui_float min, gui_float val, gui_float max, gui_float step,
                    const struct gui_slider *s, const struct gui_input *in);
/*  this function executes a slider widget
    Input:
    - output command buffer for drawing
    - bounds of the slider
    - minimal slider value that will not be underflown
    - slider value to be updated by the user
    - maximal slider value that will not be overflown
    - step interval the value will be updated with
    - visual widget style structure describing the slider
    - input structure to update the slider with
    Output:
    - returns the from the user input updated value
*/
gui_size gui_progress(struct gui_command_buffer*, struct gui_rect,
                        gui_size value, gui_size max, gui_bool modifyable,
                        const struct gui_progress*, const struct gui_input*);
/*  this function executes a progressbar widget
    Input:
    - output command buffer for drawing
    - (x,y) position
    - (width, height) size
    - progressbar value to be updated by the user
    - maximal progressbar value that will not be overflown
    - flag if the progressbar is modifyable by the user
    - visual widget style structure describing the progressbar
    - input structure to update the slider with
    Output:
    - returns the from the user input updated value
*/
void gui_editbox(struct gui_command_buffer*, struct gui_rect,
                struct gui_edit_box*, const struct gui_edit*,
                const struct gui_input*, const struct gui_font*);
/*  this function executes a editbox widget
    Input:
    - output command buffer for drawing
    - (x,y) position
    - (width, height) size
    - edit box structure containing the state to update
    - visual widget style structure describing the editbox
    - input structure to update the editbox with
    - font structure for text drawing
*/
gui_size gui_edit(struct gui_command_buffer*, struct gui_rect, gui_char*, gui_size,
                    gui_size max, gui_state*, gui_size *cursor, const struct gui_edit*,
                    enum gui_input_filter filter, const struct gui_input*,
                    const struct gui_font*);
/*  this function executes a editbox widget
    Input:
    - output command buffer for drawing
    - (x,y) position
    - (width, height) size
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
gui_size gui_edit_filtered(struct gui_command_buffer*, struct gui_rect,
                            gui_char*, gui_size, gui_size max, gui_state*, gui_size *cursor,
                            const struct gui_edit*, gui_filter filter,
                            const struct gui_input*, const struct gui_font*);
/*  this function executes a editbox widget
    Input:
    - output command buffer for drawing
    - (x,y) position
    - (width, height) size
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
gui_int gui_spinner_int(struct gui_command_buffer*, struct gui_rect,
                        const struct gui_spinner*, gui_int min, gui_int value,
                        gui_int max, gui_int step, gui_state *active,
                        const struct gui_input*, const struct gui_font*);
/*  this function executes a integer spinner widget
    Input:
    - output command buffer for draw commands
    - bounds of the spinner widget
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
gui_float gui_spinner_float(struct gui_command_buffer*, struct gui_rect,
                            const struct gui_spinner*, gui_float, gui_float,
                            gui_float max, gui_float, gui_state*,
                            const struct gui_input*, const struct gui_font*);
/*  this function executes a integer spinner widget
    Input:
    - output command buffer for draw commands
    - bounds of the spinner widget
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
gui_size gui_selector(struct gui_command_buffer*, struct gui_rect,
                        const struct gui_selector*, const char *items[],
                        gui_size item_count, gui_size item_current,
                        const struct gui_input*, const struct gui_font*);
/*  this function executes a selector widget
    Input:
    - output command buffer for draw commands
    - (x,y) position
    - (width, height) size
    - visual widget style structure describing the selector
    - selection of string array to select from
    - size of the selection array
    - input structure to update the slider with
    - font structure for text drawing
    Output:
    - returns the from the user input updated spinner value
*/
gui_float gui_scrollbar_vertical(struct gui_command_buffer*, struct gui_rect,
                                gui_float offset, gui_float target,
                                gui_float step, const struct gui_scrollbar*,
                                const struct gui_input*);
/*  this function executes a vertical scrollbar widget
    Input:
    - output command buffer for draw commands
    - (x,y) position
    - (width, height) size
    - scrollbar offset in source pixel
    - destination pixel size
    - step pixel size if the scrollbar up- or down button is pressed
    - visual widget style structure describing the selector
    - input structure to update the slider with
    Output:
    - returns the from the user input updated scrollbar offset in pixels
*/
gui_float gui_scrollbar_horizontal(struct gui_command_buffer*, struct gui_rect,
                                    gui_float offset, gui_float target,
                                    gui_float step, const struct gui_scrollbar*,
                                    const struct gui_input*);
/*  this function executes a horizontal scrollbar widget
    Input:
    - output command buffer for draw commands
    - (x,y) position
    - (width, height) size
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
    The panel configuration consists of style, color and rectangle roundingo
    information that is used for the general style and looks of panel.
    In addition for temporary modification the configuration structure consists
    of a stack for pushing and pop either color or property values.

    USAGE
    ----------------------------
    To use the configuration file you either initialize every value yourself besides
    the internal stack which needs to be initialized to zero or use the default
    configuration by calling the function gui_config_default.
    To add and remove temporary configuration states the gui_config_push_xxxx
    for adding and gui_config_pop_xxxx for removing either color or property values
    from the stack. To reset all previously modified values the gui_config_reset_xxx
    were added.

    Configuration function API
    gui_config_default              -- initializes a default panel configuration
    gui_config_set_font             -- changes the used font
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
    GUI_COLOR_SLIDER_CURSOR,
    GUI_COLOR_PROGRESS,
    GUI_COLOR_PROGRESS_CURSOR,
    GUI_COLOR_INPUT,
    GUI_COLOR_INPUT_CURSOR,
    GUI_COLOR_INPUT_TEXT,
    GUI_COLOR_SELECTOR,
    GUI_COLOR_SELECTOR_TRIANGLE,
    GUI_COLOR_SELECTOR_TEXT,
    GUI_COLOR_SELECTOR_BUTTON,
    GUI_COLOR_HISTO,
    GUI_COLOR_HISTO_BARS,
    GUI_COLOR_HISTO_NEGATIVE,
    GUI_COLOR_HISTO_HIGHLIGHT,
    GUI_COLOR_PLOT,
    GUI_COLOR_PLOT_LINES,
    GUI_COLOR_PLOT_HIGHLIGHT,
    GUI_COLOR_SCROLLBAR,
    GUI_COLOR_SCROLLBAR_CURSOR,
    GUI_COLOR_TABLE_LINES,
    GUI_COLOR_TAB_HEADER,
    GUI_COLOR_SHELF,
    GUI_COLOR_SHELF_TEXT,
    GUI_COLOR_SHELF_ACTIVE,
    GUI_COLOR_SHELF_ACTIVE_TEXT,
    GUI_COLOR_SCALER,
    GUI_COLOR_LAYOUT_SCALER,
    GUI_COLOR_COUNT
};

enum gui_config_rounding {
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
    GUI_PROPERTY_SCROLLBAR_SIZE,
    GUI_PROPERTY_SIZE,
    GUI_PROPERTY_MAX
};

struct gui_saved_property {
    enum gui_config_properties type;
    /* identifier of the current modified property */
    struct gui_vec2 value;
    /* property value that has been saveed */
};

struct gui_saved_color {
    enum gui_config_colors type;
    /* identifier of the current modified color */
    struct gui_color value;
    /* color value that has been saveed */
};

enum gui_config_components {
    GUI_DEFAULT_COLOR = 0x01,
    /* default all colors inside the configuration struct */
    GUI_DEFAULT_PROPERTIES = 0x02,
    /* default all properites inside the configuration struct */
    GUI_DEFAULT_ROUNDING = 0x04,
    /* default all rounding values inside the configuration struct */
    GUI_DEFAULT_ALL = 0xFFFF
    /* default the complete configuration struct */
};

struct gui_config_stack  {
    gui_size property;
    /* current property stack pushing index */
    struct gui_saved_property properties[GUI_MAX_ATTRIB_STACK];
    /* saved property stack */
    struct gui_saved_color colors[GUI_MAX_COLOR_STACK];
    /* saved color stack */
    gui_size color;
    /* current color stack pushing index */
};

struct gui_config {
    struct gui_font font;
    /* the from the user provided font */
    gui_float scaler_width;
    /* width of the tiled panel scaler */
    gui_float rounding[GUI_ROUNDING_MAX];
    /* rectangle widget rounding */
    struct gui_vec2 properties[GUI_PROPERTY_MAX];
    /* configuration properties to modify the panel style */
    struct gui_color colors[GUI_COLOR_COUNT];
    /* configuration color to modify the panel color */
    struct gui_config_stack stack;
    /* modification stack */
};

void gui_config_default(struct gui_config*, gui_flags, const struct gui_font*);
/*  this function load the panel configuration with default values
    Input:
    - configuration flags indicating which part of the configuration should be loaded with default values
    - user font reference structure describing the font used inside the panel
    Output:
    - configuration structure holding the default panel style
*/
void gui_config_set_font(struct gui_config*, const struct gui_font*);
/*  this function changes the used font and can be used even inside a frame
    Input:
    - user font reference structure describing the font used inside the panel
*/
struct gui_vec2 gui_config_property(const struct gui_config*,
                                            enum gui_config_properties);
/*  this function accesses a configuration property over an identifier
    Input:
    - Configuration the get the property from
    - Configuration property idenfifier describing the property to get
    Output:
    - Property value that has been asked for
*/
struct gui_color gui_config_color(const struct gui_config*, enum gui_config_colors);
/*  this function accesses a configuration color over an identifier
    Input:
    - Configuration the get the color from
    - Configuration color idenfifier describing the color to get
    Output:
    - color value that has been asked for
*/
void gui_config_push_property(struct gui_config*, enum gui_config_properties,
                                struct gui_vec2);
/*  this function temporarily changes a property in a stack like fashion to be reseted later
    Input:
    - Configuration structure to push the change to
    - Property idenfifier to change
    - new value of the property
*/
void gui_config_push_color(struct gui_config*, enum gui_config_colors,
                            struct gui_color);
/*  this function temporarily changes a color in a stack like fashion to be reseted later
    Input:
    - Configuration structure to push the change to
    - color idenfifier to change
    - new color
*/
void gui_config_pop_color(struct gui_config*);
/*  this function reverts back a previously pushed temporary color change
    Input:
    - Configuration structure to pop the change from and to
*/
void gui_config_pop_property(struct gui_config*);
/*  this function reverts back a previously pushed temporary property change
    Input:
    - Configuration structure to pop the change from and to
*/
void gui_config_reset_colors(struct gui_config*);
/*  this function reverts back all previously pushed temporary color changes
    Input:
    - Configuration structure to pop the change from and to
*/
void gui_config_reset_properties(struct gui_config*);
/*  this function reverts back all previously pushed temporary color changes
    Input:
    - Configuration structure to pop the change from and to
*/
void gui_config_reset(struct gui_config*);
/*  this function reverts back all previously pushed temporary color and
 *  property changes
    Input:
    - Configuration structure to pop the change from and to
*/
/*
 * ==============================================================
 *
 *                          Panel
 *
 * ===============================================================
    PANEL
    ----------------------------
    A panel provides functionality on groupes of widgets like
    moving, scaleing, minimizing, controlled positioning, scrolling, tabs, trees,
    tables and shelfs. The panel API is hereby not responsible for providing widgets
    but instead uses the widget API under the hood and only controlles position
    and style of each widget. The actual drawing and updating is done by the
    widget API.
    The API is just like the widget API an immediate mode API and almost all
    data outside of the actual panel is lost after each fraem.
    Therefore there is no concept of storing widget state but instead the user
    has to manage all state.
    From a data point of view the panel takes in panel data, widget data, configuration
    data and a memory block and outputs the updated widget data plus a command
    buffer with a number of draw commands for each frame. This was done to provide
    a easy way to abstract over a big number of platforms, renter backends, font
    implementations.

    ----------                                  -------------
    | config |          -------------           |           |
    | panel  |          |           |           | widget    |
    | memory | ------\  |   GUI     |  -------> |-----------|
    | widget | ------/  |           |           | commands  |
    | Input  |          -------------           |           |
    ----------                                  -------------

    The panel can be divided into a header, menubar and body. The header
    provides functionality like closing or minimizing while the menubar
    is space under the header that is independent of scrolling and therefore
    always stays at the top of the panel. The menubar and body can be filled with
    a number of different provided widgets while the header only supports its
    own set of icons.

    USAGE
    ----------------------------
    Main functions      -- main panel setup and modification functions
    Header functions    -- functions to create and setup a panel header and menubar
    Layout functions    -- API that provides different ways to place widgets in the panel
    Widget functions    -- immediate mode widgets functions to till the panel with
    Complex functions   -- Widget with more complex behavior and requirements
    Group functions     -- Widget grouping functions

*/
enum gui_widget_state {
    GUI_WIDGET_INVALID, /* The widget cannot be seen and is completly out of view */
    GUI_WIDGET_VALID, /* The widget is completly inside the panel and can be updated + drawn */
    GUI_WIDGET_ROM /* The widget is partially visible and cannot be updated */
};

enum gui_panel_flags {
    GUI_PANEL_HIDDEN = 0x01,
    /* Hiddes the panel and stops any panel interaction and drawing can be set
     * by user input or by closing the panel */
    GUI_PANEL_BORDER = 0x02,
    /* Draws a border around the panel to visually seperate the panel from the
     * background */
    GUI_PANEL_BORDER_HEADER = 0x04,
    /* Draws a border between panel header and body */
    GUI_PANEL_MOVEABLE = 0x08,
    /* The moveable flag inidicates that a panel can be move by user input by
     * dragging the panel header */
    GUI_PANEL_SCALEABLE = 0x10,
    /* The scaleable flag indicates that a panel can be scaled by user input
     * by dragging a scaler icon at the button of the panel */
    GUI_PANEL_MINIMIZED = 0x20,
    /* marks the panel as minimized */
    GUI_PANEL_ROM = 0x40,
    /* sets the panel into a read only mode and does not allow input changes */
    GUI_PANEL_DYNAMIC = 0x80,
    /* special type of panel which grows up in height while being filled to a
     * certain maximum height. It is mainly used for combo boxes but can be
     * used to create perfectly fitting panels as well */
    GUI_PANEL_ACTIVE = 0x10000,
    /* INTERNAL ONLY!: marks the panel as active, used by the panel stack */
    GUI_PANEL_TAB = 0x20000,
    /* INTERNAL ONLY!: Marks the panel as an subpanel of another panel(Groups/Tabs/Shelf)*/
    GUI_PANEL_COMBO_MENU = 0x40000,
    /* INTERNAL ONLY!: Marks the panel as an combo box or menu */
    GUI_PANEL_REMOVE_ROM = 0x80000
    /* INTERNAL ONLY!: removes the read only mode at the end of the panel */
};

struct gui_panel {
    gui_float x, y;
    /* position in the os window */
    gui_float w, h;
    /* size with width and height of the panel */
    gui_flags flags;
    /* panel flags modifing its behavior */
    struct gui_vec2 offset;
    /* flag indicating if the panel is collapsed */
    const struct gui_config *config;
    /* configuration reference describing the panel style */
    struct gui_command_buffer buffer;
    /* output command buffer queuing all drawing calls */
    struct gui_command_queue *queue;
    /* output command queue which hold the command buffer */
    const struct gui_input *input;
    /* input state for updating the panel and all its widgets */
};

enum gui_panel_row_layout_type {
    GUI_PANEL_LAYOUT_DYNAMIC_FIXED,
    /* fixed widget ratio width panel layout */
    GUI_PANEL_LAYOUT_DYNAMIC_ROW,
    /* immediate mode widget specific widget width ratio layout */
    GUI_PANEL_LAYOUT_DYNAMIC_FREE,
    /* free ratio based placing of widget in a local space  */
    GUI_PANEL_LAYOUT_DYNAMIC,
    /* retain mode widget specific widget ratio width*/
    GUI_PANEL_LAYOUT_STATIC_FIXED,
    /* fixed widget pixel width panel layout */
    GUI_PANEL_LAYOUT_STATIC_ROW,
    /* immediate mode widget specific widget pixel width layout */
    GUI_PANEL_LAYOUT_STATIC_FREE,
    /* free pixel based placing of widget in a local space  */
    GUI_PANEL_LAYOUT_STATIC
    /* retain mode widget specific widget pixel width layout */
};

enum gui_panel_layout_node_type {
    GUI_LAYOUT_NODE,
    /* a node is a space which can be minimized or maximized */
    GUI_LAYOUT_TAB
    /* a tab is a node with a header */
};

#define GUI_UNDEFINED (-1.0f)
struct gui_panel_row_layout {
    enum gui_panel_row_layout_type type;
    /* type of the row layout */
    gui_size index;
    /* index of the current widget in the current panel row */
    gui_float height;
    /* height of the current row */
    gui_size columns;
    /* number of columns in the current row */
    const gui_float *ratio;
    /* row widget width ratio */
    gui_float item_width, item_height;
    /* current width of very item */
    gui_float item_offset;
    /* x positon offset of the current item */
    gui_float filled;
    /* total fill ratio */
    struct gui_rect item;
    /* item bounds */
    struct gui_rect clip;
    /* temporary clipping rect */
};

struct gui_panel_header {
    gui_float x, y, w, h;
    /* header bounds */
    gui_float front, back;
    /* visual header filling deque */
};

struct gui_panel_menu {
    gui_float x, y, w, h;
    /* menu bounds */
    struct gui_vec2 offset;
    /* saved panel scrollbar offset */
};

struct gui_panel_layout {
    gui_flags flags;
    /* panel flags modifing its behavior */
    gui_float x, y, w, h;
    /* position and size of the panel in the os window */
    struct gui_vec2 offset;
    /* panel scrollbar offset */
    gui_bool is_table;
    /* flag indicating if the panel is currently creating a table */
    gui_flags tbl_flags;
    /* flags describing the line drawing for every row in the table */
    gui_bool valid;
    /* flag inidicating if the panel is visible */
    gui_float at_x, at_y, max_x;
    /* index position of the current widget row and column  */
    gui_float width, height;
    /* size of the actual useable space inside the panel */
    gui_float footer_h;
    /* height of the panel footer space */
    struct gui_rect clip;
    /* panel clipping rect */
    struct gui_panel_header header;
    /* panel header bounds */
    struct gui_panel_menu menu;
    /* panel menubar bounds */
    struct gui_panel_row_layout row;
    /* currently used panel row layout */
    const struct gui_config *config;
    /* configuration data describing the visual style of the panel */
    const struct gui_input *input;
    /* current input state for updating the panel and all its widgets */
    struct gui_command_buffer *buffer;
    /* command draw call output command buffer */
    struct gui_command_queue *queue;
    /* command draw call output command buffer */
};

/*
 * --------------------------------------------------------------
 *                          MAIN
 * --------------------------------------------------------------
    MAIN
    The Main Panel function API is used to initialize a panel, create
    a stack based panel layout, control the build up process and to
    modify the panel state. The modification of the panel is only allowed
    outside both sequence points `gui_panel_begin` and `gui_panel_end`, therefore
    all modification inside them is undefined behavior.

    USAGE
    To work a panel needs to be initialized by calling `gui_panel_init` with
    a reference to a valid command buffer and configuration structure.
    The references have to be valid over the life time of the panel and can be
    changed by setter functions. In addition to being initialized
    the panel needs a panel layout in every frame to fill and use as temporary
    state to fill the panel with widgets.
    The panel layout hereby does NOT have to be kept around outside of the build
    up process and multiple panels can share one panel layout. The reason why
    panel and layout are split is to seperate the temporary changing state
    of the panel layout from the persistent state of the panel. In addition
    the panel only needs a fraction of the needed space and state of the panel layout.

    panel function API
    gui_panel_init          -- initializes the panel with position, size and flags
    gui_panel_begin         -- begin sequence point in the panel layout build up process
    gui_panel_begin_tiled   -- extends gui_panel_begin by adding the panel into a tiled layout
    gui_panel_end           -- end squeunce point which finializes the panel build up
    gui_panel_set_config    -- updates the used panel configuration
    gui_panel_add_flag      -- adds a behavior flag to the panel
    gui_panel_remove_flag   -- removes a behavior flag from the panel
    gui_panel_has_flag      -- check if a given behavior flag is set in the panel
    gui_panel_is_minimized  -- return wether the panel is minimized
 */
void gui_panel_init(struct gui_panel *panel, gui_float x, gui_float y, gui_float w,
                    gui_float h, gui_flags flags, struct gui_command_queue*,
                    const struct gui_config*, const struct gui_input *in);
/*  this function initilizes and setups the panel
    Input:
    - bounds of the panel with x,y position and width and height
    - panel flags for modified panel behavior
    - reference to a output command queue to push draw calls into
    - configuration file containing the style, color and font for the panel
    Output:
    - a newly initialized panel
*/
void gui_panel_begin(struct gui_panel_layout*, struct gui_panel*);
/*  this function begins the panel build up process
    Input:
    - input structure holding all user generated state changes
    Output:
    - panel layout to fill up with widgets
*/
void gui_panel_end(struct gui_panel_layout*, struct gui_panel*);
/*  this function ends the panel layout build up process and updates the panel */
void gui_panel_set_config(struct gui_panel*, const struct gui_config*);
/*  this function updateds the panel configuration pointer */
void gui_panel_set_buffer(struct gui_panel*, struct gui_command_buffer*);
/*  this function updateds the used panel command buffer */
void gui_panel_add_flag(struct gui_panel*, gui_flags);
/*  this function adds panel flags to the panel
    Input:
    - panel flags to add the panel
*/
void gui_panel_remove_flag(struct gui_panel*, gui_flags);
/*  this function removes panel flags from the panel
    Input:
    - panel flags to remove from the panel
*/
gui_bool gui_panel_has_flag(struct gui_panel*, gui_flags);
/*  this function checks if a panel has given flag(s)
    Input:
    - panel flags to check for
*/
gui_bool gui_panel_is_minimized(struct gui_panel*);
/*  this function checks if the panel is minimized */
/*
 * --------------------------------------------------------------
 *                          HEADER
 * --------------------------------------------------------------
    HEADER
    The header API is for adding a window space at the top of the panel for
    buttons, icons and panel title. It is useful for toggling the visiblity
    aswell as minmized state of the panel. The header can be filled with buttons
    and icons from the left and as well as the right side and allows therefore
    a wide range of header layouts.

    USAGE
    To create a header you have to call one of two API after the panel layout
    has been created with `gui_panel_begin`. The first and easiest way is to
    just call `gui_panel_header` which provides a basic header with
    customizable buttons as well as title but notification if a button is pressed.
    The layout supported is hereby limited and custom button and icons cannot be
    added. To achieve that you have to use the more extensive header API.
    You start by calling `gui_panel_header_begin` after `gui_panel_begin` and
    call the different `gui_panel_header_xxx` functions to add icons or the title
    either at the left or right side of the panel. Each function returns if the
    icon or button has been pressed or in the case of the toggle the current state.
    Finally if all button/icons/toggles have been added the process is finished
    by calling `gui_panel_header_end`.

    panel header function API
    gui_panel_header_begin          -- begins the header build up process
    gui_panel_header_button         -- adds a button into the header
    gui_panel_header_button_icon    -- adds a image button into the header
    gui_panel_header_toggle         -- adds a toggle button into the header
    gui_panel_header_flag           -- adds a panel flag toggle button
    gui_panel_header_title          -- adds the title of the panel into the header
    gui_panel_header_end            -- finishes the header build up process
    gui_panel_header                -- short cut version of the header build up process
    gui_panel_menubar_begin         -- marks the beginning of the menubar building process
    gui_panel_menubar_end           -- marks the end the menubar build up process
*/
enum gui_panel_header_flags {
    GUI_CLOSEABLE = 0x01,
    /* adds a closeable icon into the header */
    GUI_MINIMIZABLE = 0x02,
    /* adds a minimize icon into the header */
    GUI_SCALEABLE = 0x04,
    /* adds a scaleable flag icon into the header */
    GUI_MOVEABLE = 0x08
    /* adds a moveable flag icon into the header */
};

enum gui_panel_header_symbol {
    GUI_SYMBOL_X,
    GUI_SYMBOL_UNDERSCORE,
    GUI_SYMBOL_CIRCLE,
    GUI_SYMBOL_CIRCLE_FILLED,
    GUI_SYMBOL_RECT,
    GUI_SYMBOL_RECT_FILLED,
    GUI_SYMBOL_TRIANGLE_UP,
    GUI_SYMBOL_TRIANGLE_DOWN,
    GUI_SYMBOL_TRIANGLE_LEFT,
    GUI_SYMBOL_TRIANGLE_RIGHT,
    GUI_SYMBOL_PLUS,
    GUI_SYMBOL_MINUS,
    GUI_SYMBOL_IMAGE,
    GUI_SYMBOL_MAX
};

enum gui_panel_header_align {
    GUI_HEADER_LEFT,
    /* header elements are added at the left side of the header */
    GUI_HEADER_RIGHT
    /* header elements are added at the right side of the header */
};

gui_flags gui_panel_header(struct gui_panel_layout*, const char *title,
                            gui_flags show, gui_flags notify,
                            enum gui_panel_header_align);
/*  this function is a shorthand for the header build up process
    flag by the user
    Input:
    - title of the header or NULL if not needed
    - flags indicating which icons should be drawn to the header
    - flags indicating which icons should notify if clicked
*/
void gui_panel_header_begin(struct gui_panel_layout*);
/*  this function begins the panel header build up process */
gui_bool gui_panel_header_button(struct gui_panel_layout *layout,
                                enum gui_panel_header_symbol symbol,
                                enum gui_panel_header_align);
/*  this function adds a header button icon
    Input:
    -
    - symbol that shall be shown in the header as a icon
    Output:
    - gui_true if the button was pressed gui_false otherwise
*/
gui_bool gui_panel_header_button_icon(struct gui_panel_layout*, struct gui_image,
                                    enum gui_panel_header_align);
/*  this function adds a header image button icon
    Input:
    - symbol that shall be shown in the header as a icon
    Output:
    - gui_true if the button was pressed gui_false otherwise
*/
gui_bool gui_panel_header_toggle(struct gui_panel_layout*,
                                enum gui_panel_header_symbol inactive,
                                enum gui_panel_header_symbol active,
                                enum gui_panel_header_align, gui_bool state);
/*  this function adds a header toggle button
    Input:
    - symbol that will be drawn if the toggle is inactive
    - symbol that will be drawn if the toggle is active
    - state of the toggle with either active or inactive
    Output:
    - updated state of the toggle
*/
gui_bool gui_panel_header_flag(struct gui_panel_layout *layout,
                                enum gui_panel_header_symbol inactive,
                                enum gui_panel_header_symbol active,
                                enum gui_panel_header_align,
                                enum gui_panel_flags flag);
/*  this function adds a header toggle button for modifing a certain panel flag
    Input:
    - symbol that will be drawn if the flag is inactive
    - symbol that will be drawn if the flag is active
    - panel flag whose state will be display by the toggle button
    Output:
    - gui_true if the button was pressed gui_false otherwise
*/
void gui_panel_header_title(struct gui_panel_layout*, const char*,
                                enum gui_panel_header_align);
/*  this function adds a title to the panel header
    Input:
    - title of the header
*/
void gui_panel_header_end(struct gui_panel_layout*);
/*  this function ends the panel header build up process */
void gui_panel_menubar_begin(struct gui_panel_layout*);
/*  this function begins the panel menubar build up process */
void gui_panel_menubar_end(struct gui_panel_layout*);
/*  this function ends the panel menubar build up process */
/*
 * --------------------------------------------------------------
 *                          LAYOUT
 * --------------------------------------------------------------
    LAYOUT
    The layout API is for positioning of widget inside a panel. In general there
    are three different ways to position widget. The first one is a table with
    fixed size columns. This like the other three comes in two flavors. First
    the scaleable width as a ratio of the panel width and the other is a
    non-scaleable fixed pixel value for static panels.
    Since sometimes widgets with different sizes in a row is needed another set
    of row layout has been added. The first set is for dynamically size widgets
    in an immediate mode API which sets each size of a widget directly before
    it is called or a retain mode API which stores the size of every widget as
    an array.
    The final way to position widgets is by allocating a fixed space from
    the panel and directly positioning each widget with position and size.
    This requires the least amount of work for the API and the most for the user,
    but offers the most positioning freedom.

    panel scaling layout function API
    gui_panel_row               -- user defined widget row layout
    gui_panel_row_dynamic       -- scaling fixed column row layout
    gui_panel_row_static        -- fixed width fixed column row layout
    gui_panel_row_begin         -- begins the row build up process
    gui_panel_row_push          -- pushes the next widget width
    gui_panel_row_end           -- ends the row build up process
    gui_panel_row_space_begin   -- creates a free placing space in the panel
    gui_panel_row_space_widget  -- pushes a widget into the space
    gui_panel_row_space_end     -- finishes the free drawingp process

    panel tree layout function API
    gui_panel_layout_push       -- pushes a new node/collapseable header/tab
    gui_panel_layout_pop        -- pops the the previously added node

*/
enum gui_panel_row_layout_format {
    GUI_DYNAMIC, /* row layout which scales with the panel */
    GUI_STATIC /* row layout with fixed pixel width */
};

void gui_panel_row_dynamic(struct gui_panel_layout*, gui_float height, gui_size cols);
/*  this function sets the row layout to dynamically fixed size widget
    Input:
    - height of the row that will be filled
    - number of widget inside the row that will divide the space
*/
void gui_panel_row_static(struct gui_panel_layout*, gui_float row_height,
                        gui_size item_width, gui_size cols);
/*  this function sets the row layout to static fixed size widget
    Input:
    - height of the row that will be filled
    - width in pixel measurement of each widget in the row
    - number of widget inside the row that will divide the space
*/
void gui_panel_row_begin(struct gui_panel_layout*,
                        enum gui_panel_row_layout_format,
                        gui_float row_height, gui_size cols);
/*  this function start a new scaleable row that can be filled with different
    sized widget
    Input:
    - scaleable or fixed row format
    - height of the row that will be filled
    - number of widget inside the row that will divide the space
*/
void gui_panel_row_push(struct gui_panel_layout*, gui_float value);
/*  this function pushes a widget into the previously start row with the given
    panel width ratio or pixel width
    Input:
    - value with either a ratio for GUI_DYNAMIC or a pixel width for GUI_STATIC layout
*/
void gui_panel_row_end(struct gui_panel_layout*);
/*  this function ends the previously started scaleable row */
void gui_panel_row(struct gui_panel_layout*,  enum gui_panel_row_layout_format,
                    gui_float height, gui_size cols, const gui_float *ratio);
/*  this function sets the row layout as an array of ratios/width for
    every widget that will be inserted into that row
    Input:
    - scaleable or fixed row format
    - height of the row and there each widget inside
    - number of widget inside the row
    - panel ratio/pixel width array for each widget
*/
void gui_panel_row_space_begin(struct gui_panel_layout*,
                                enum gui_panel_row_layout_format,
                                gui_float height, gui_size widget_count);
/*  this functions starts a space where widgets can be added
    at any given position and the user has to make sure no overlap occures
    Input:
    - height of the row and therefore each widget inside
    - number of widget that will be added into that space
*/
void gui_panel_row_space_push(struct gui_panel_layout*, struct gui_rect);
/*  this functions pushes the position and size of the next widget that will
    be added into the previously allocated panel space
    Input:
    - rectangle with position and size as a ratio of the next widget to add
*/
void gui_panel_row_space_end(struct gui_panel_layout*);
/*  this functions finishes the scaleable space filling process */
gui_bool gui_panel_layout_push(struct gui_panel_layout*, enum gui_panel_layout_node_type,
                                const char *title, gui_state*);
/*  this functions pushes either a tree node, collapseable header or tab into
 *  the current panel layout
    Input:
    - title of the node to push into the panel
    - type of then node with either default node, collapseable header or tab
    - state of the node with either GUI_MINIMIZED or GUI_MAXIMIZED
    Output:
    - returns the updated state as either gui_true if open and gui_false otherwise
    - updates the state of the node pointer to the updated state
*/
void gui_panel_layout_pop(struct gui_panel_layout*);
/*  this functions ends the previously added node */
/*
 * --------------------------------------------------------------
 *                          WIDGETS
 * --------------------------------------------------------------
    WIDGET
    The layout API uses the layout API to provide and add widget to the panel.
    IMPORTANT: the widget API does NOT work without a layout so if you have
    visual glitches then the problem probably stems from not using the layout
    correctly. The panel widget API does not implement any widget itself, instead
    it uses the general Widget API under the hood and is only responsible for
    calling the correct widget API function with correct position, size and style.
    All widgets do NOT store any state instead everything has to be managed by
    the user.

    USAGE
    To use the Widget API you first have to call one of the layout API funtions
    to setup the widget. After that you can just call one of the widget functions
    at it will automaticall update the widget state as well as `draw` the widget
    by adding draw command into the panel command buffer.

    Panel widget API
    gui_panel_widget                -- base function for all widgets to allocate space on the panel
    gui_panel_spacing               -- create a column seperator and is basically an empty widget
    gui_panel_text                  -- text widget for printing text with length
    gui_panel_text_colored          -- colored text widget for printing colored text width length
    gui_panel_label                 -- text widget for printing zero terminated strings
    gui_panel_label_colored         -- text widget for printing colored zero terminiated strings
    gui_panel_button_text           -- button widget with text content
    gui_panel_button_color          -- colored button widget without content
    gui_panel_button_triangle       -- button with triangle pointing either up-/down-/left- or right
    gui_panel_button_image          -- button widget width icon content
    gui_panel_button_toggle         -- toggle button with either active or inactive state
    gui_panel_button_text_image     -- button widget with text and icon
    gui_panel_button_text_triangle  -- button widget with text and a triangle
    gui_panle_image                 -- image widget for outputing a image to a panel
    gui_panel_check                 -- add a checkbox widget with either active or inactive state
    gui_panel_option                -- radiobutton widget with either active or inactive state
    gui_panel_option_group          -- radiobutton group for automatic single selection
    gui_panel_slider                -- slider widget with min,max,step value
    gui_panel_progress              -- progressbar widget
    gui_panel_edit                  -- edit textbox widget for text input
    gui_panel_edit_filtered         -- edit textbox widget for text input with filter input
    gui_panel_editbox               -- edit textbox with cursor, clipboard and filter
    gui_panel_spinner_int           -- spinner widget with either keyboard or mouse modification
    gui_panel_spinner_float         -- spinner widget with either keyboard or mouse modification
    gui_panel_selector              -- selector widget for combobox like selection of types
*/
enum gui_widget_state gui_panel_widget(struct gui_rect*, struct gui_panel_layout*);
/*  this function represents the base of every widget and calculates the bounds
 *  and allocated space for a widget inside a panel.
    Output:
    - allocated space for a widget to draw into
    - gui_true if the widget is visible and should be updated gui_false if not
*/
void gui_panel_spacing(struct gui_panel_layout*, gui_size cols);
/*  this function creates a seperator to fill space
    Input:
    - number of columns or widget to jump over
*/
void gui_panel_text(struct gui_panel_layout*, const char*, gui_size,
                    enum gui_text_align);
/*  this function creates a bounded non terminated text widget with either
    left, centered or right alignment
    Input:
    - string pointer to text that should be drawn
    - number of bytes the text is long
    - text alignment with either left, centered or right alignment
*/
void gui_panel_text_colored(struct gui_panel_layout*, const char*, gui_size,
                            enum gui_text_align, struct gui_color);
/*  this function creates a bounded non terminated color text widget with either
    left, centered or right alignment
    Input:
    - string pointer to text that should be drawn
    - number of bytes the text is long
    - text alignment with either left, centered or right alignment
    - color the text should be drawn
*/
void gui_panel_label(struct gui_panel_layout*, const char*, enum gui_text_align);
/*  this function creates a zero terminated text widget with either
    left, centered or right alignment
    Input:
    - string pointer to text that should be drawn
    - text alignment with either left, centered or right alignment
*/
void gui_panel_label_colored(struct gui_panel_layout*, const char*,
                            enum gui_text_align, struct gui_color);
/*  this function creates a zero terminated colored text widget with either
    left, centered or right alignment
    Input:
    - string pointer to text that should be drawn
    - text alignment with either left, centered or right alignment
    - color the label should be drawn
*/
void gui_panel_image(struct gui_panel_layout*, struct gui_image);
/*  this function creates an image widget
    Input:
    - string pointer to text that should be drawn
*/
gui_bool gui_panel_check(struct gui_panel_layout*, const char*, gui_bool active);
/*  this function creates a checkbox widget with either active or inactive state
    Input:
    - checkbox label describing the content
    - state of the checkbox with either active or inactive
    Output:
    - from user input updated state of the checkbox
*/
gui_bool gui_panel_option(struct gui_panel_layout*, const char*, gui_bool active);
/*  this function creates a radiobutton widget with either active or inactive state
    Input:
    - radiobutton label describing the content
    - state of the radiobutton with either active or inactive
    Output:
    - from user input updated state of the radiobutton
*/
gui_size gui_panel_option_group(struct gui_panel_layout*, const char**,
                                gui_size cnt, gui_size cur);
/*  this function creates a radiobutton group widget with only one active radiobutton
    Input:
    - radiobutton label array describing the content of each radiobutton
    - number of radiobuttons
    - index of the current active radiobutton
    Output:
    - the from user input updated index of the active radiobutton
*/
gui_bool gui_panel_button_text(struct gui_panel_layout*, const char*,
                                enum gui_button_behavior);
/*  this function creates a text button
    Input:
    - button label describing the button
    - string label
    - button behavior with either default or repeater behavior
    Output:
    - gui_true if the button was transistioned from unpressed to pressed with
        default button behavior or pressed if repeater behavior.
*/
gui_bool gui_panel_button_color(struct gui_panel_layout*, struct gui_color,
                                enum gui_button_behavior);
/*  this function creates a colored button without content
    Input:
    - color the button should be drawn with
    - button behavior with either default or repeater behavior
    Output:
    - gui_true if the button was transistioned from unpressed to pressed with
        default button behavior or pressed if repeater behavior.
*/
gui_bool gui_panel_button_triangle(struct gui_panel_layout*, enum gui_heading,
                                    enum gui_button_behavior);
/*  this function creates a button with a triangle pointing in one of four directions
    Input:
    - triangle direction with either up, down, left or right direction
    - button behavior with either default or repeater behavior
    Output:
    - gui_true if the button was transistioned from unpressed to pressed with
        default button behavior or pressed if repeater behavior.
*/
gui_bool gui_panel_button_image(struct gui_panel_layout*, struct gui_image img,
                                enum gui_button_behavior);
/*  this function creates a button with an icon as content
    Input:
    - icon image handle to draw into the button
    - button behavior with either default or repeater behavior
    Output:
    - gui_true if the button was transistioned from unpressed to pressed with
        default button behavior or pressed if repeater behavior.
*/
gui_bool gui_panel_button_text_triangle(struct gui_panel_layout*, enum gui_heading,
                                    const char*, enum gui_text_align,
                                    enum gui_button_behavior);
/*  this function creates a button with a triangle pointing in one of four directions and text
    Input:
    - triangle direction with either up, down, left or right direction
    - button label describing the button
    - text alignment with either left, centered or right alignment
    - button behavior with either default or repeater behavior
    Output:
    - gui_true if the button was transistioned from unpressed to pressed with
        default button behavior or pressed if repeater behavior.
*/
gui_bool gui_panel_button_text_image(struct gui_panel_layout *layout, struct gui_image img,
                                    const char *text, enum gui_text_align align,
                                    enum gui_button_behavior behavior);
/*  this function creates a button with an icon and text
    Input:
    - image or subimage to use as an icon
    - button label describing the button
    - text alignment with either left, centered or right alignment
    - button behavior with either default or repeater behavior
    Output:
    - gui_true if the button was transistioned from unpressed to pressed with
        default button behavior or pressed if repeater behavior.
*/
gui_bool gui_panel_button_toggle(struct gui_panel_layout*, const char*,gui_bool value);
/*  this function creates a toggle button which is either active or inactive
    Input:
    - label describing the toggle button
    - current state of the toggle
    Output:
    - from user input updated toggle state
*/
gui_float gui_panel_slider(struct gui_panel_layout*, gui_float min, gui_float val,
                            gui_float max, gui_float step);
/*  this function creates a slider for value manipulation
    Input:
    - minimal slider value that will not be underflown
    - slider value which shall be updated
    - maximal slider value that will not be overflown
    - step intervall to change the slider with
    Output:
    - the from user input updated slider value
*/
gui_size gui_panel_progress(struct gui_panel_layout*, gui_size cur, gui_size max,
                            gui_bool modifyable);
/*  this function creates an either user or program controlled progressbar
    Input:
    - current progressbar value
    - maximal progressbar value that will not be overflown
    - flag indicating if the progressbar should be changeable by user input
    Output:
    - the from user input updated progressbar value if modifyable progressbar
*/
void gui_panel_editbox(struct gui_panel_layout*, struct gui_edit_box*);
/*  this function creates an editbox with copy & paste functionality and text buffering */
gui_size gui_panel_edit(struct gui_panel_layout*, gui_char *buffer, gui_size len,
                        gui_size max, gui_state *active, gui_size *cursor,
                        enum gui_input_filter);
/*  this function creates an editbox to updated/insert user text input
    Input:
    - buffer to fill with user input
    - current length of the buffer in bytes
    - maximal number of bytes the buffer can be filled with
    - state of the editbox with active as currently modified by the user
    - filter type to limit the glyph the user can input into the editbox
    Output:
    - length of the buffer after user input update
    - current state of the editbox with active(gui_true) or inactive(gui_false)
*/
gui_size gui_panel_edit_filtered(struct gui_panel_layout*, gui_char *buffer,
                                gui_size len, gui_size max,  gui_state *active,
                                gui_size *cursor, gui_filter);
/*  this function creates an editbox to updated/insert filtered user text input
    Input:
    - buffer to fill with user input
    - current length of the buffer in bytes
    - maximal number of bytes the buffer can be filled with
    - state of the editbox with active as currently modified by the user
    - filter callback to limit the glyphes the user can input into the editbox
    Output:
    - length of the buffer after user input update
    - current state of the editbox with active(gui_true) or inactive(gui_false)
*/
gui_int gui_panel_spinner_int(struct gui_panel_layout*, gui_int min, gui_int value,
                                gui_int max, gui_int step, gui_state *active);
/*  this function creates a integer spinner widget
    Input:
    - min value that will not be underflown
    - current spinner value to be updated by user input
    - max value that will not be overflown
    - spinner value modificaton stepping intervall
    - current state of the spinner with active as currently modfied by user input
    Output:
    - the from user input updated spinner value
    - current state of the editbox with active(gui_true) or inactive(gui_false)
*/
gui_float gui_panel_spinner_float(struct gui_panel_layout*, gui_float min, gui_float value,
                                gui_float max, gui_float step, gui_state *active);
/*  this function creates a float spinner widget
    Input:
    - min value that will not be underflown
    - current spinner value to be updated by user input
    - max value that will not be overflown
    - spinner value modificaton stepping intervall
    - current state of the spinner with active as currently modfied by user input
    Output:
    - the from user input updated spinner value
    - current state of the editbox with active(gui_true) or inactive(gui_false)
*/
gui_size gui_panel_selector(struct gui_panel_layout*, const char *items[],
                            gui_size item_count, gui_size item_current);
/*  this function creates a string selector widget
    Input:
    - string array contaning a selection
    - number of items inside the selection array
    - index of the currenetly selected item inside the array
    Output:
    - the from user selection selected array index of the active item
*/
/*
 * -------------------------------------------------------------
 *                          GROUP
 * --------------------------------------------------------------
 *
    GROUP
    A group represents a panel inside a panel. The group thereby has a fixed height
    but just like a normal panel has a scrollbar. It main promise is to group together
    a group of widgets into a small space inside a panel and to provide a scrollable
    space inside a panel.

    USAGE
    To create a group you first have to allocate space in a panel. This is done
    by the panel row layout API and works the same as widgets. After that the
    `gui_panel_group_begin` has to be called with the parent layout to create
    the group in and a group layout to create a new panel inside the panel.
    Just like a panel layout structures the group layout only has a lifetime
    between the `gui_panel_group_begin` and `gui_panel_group_end` and does
    not have to be persistent.

    Panel group API
    gui_panel_group_begin   -- adds a scrollable fixed space inside the panel
    gui_panel_group_end     -- ends the scrollable space
*/
void gui_panel_group_begin(struct gui_panel_layout*, struct gui_panel_layout *tab,
                            const char *title, struct gui_vec2);
/*  this function adds a grouped subpanel into the parent panel
    IMPORTANT: You need to set the height of the group with panel_row_layout
    Input:
    - group title to write into the header
    - group scrollbar offset
    Output:
    - group layout to fill with widgets
*/
struct gui_vec2 gui_panel_group_end(struct gui_panel_layout*, struct gui_panel_layout* tab);
/*  this function finishes the previously started group layout
    Output:
    - The from user input updated group scrollbar pixel offset
*/
/*
 * -------------------------------------------------------------
 *                          SHELF
 * --------------------------------------------------------------
    SHELF
    A shelf extends the concept of a group as an panel inside a panel
    with the possibility to decide which content should be drawn into the group.
    This is achieved by tabs on the top of the group panel with one selected
    tab. The selected tab thereby defines which content should be drawn inside
    the group panel by an index it returns. So you just have to check the returned
    index and depending on it draw the wanted content.

    Panel shelf API
    gui_panel_shelf_begin   -- begins a shelf with a number of selectable tabs
    gui_panel_shelf_end     -- ends a previously started shelf build up process

*/
gui_size gui_panel_shelf_begin(struct gui_panel_layout*, struct gui_panel_layout*,
                                const char *tabs[], gui_size size,
                                gui_size active, struct gui_vec2 offset);
/*  this function adds a shelf subpanel into the parent panel
    IMPORTANT: You need to set the height of the shelf with panel_row_layout
    Input:
    - all possible selectible tabs of the shelf with names as a string array
    - number of seletectible tabs
    - current active tab array index
    - scrollbar pixel offset for the shelf
    Output:
    - group layout to fill with widgets
    - the from user input updated current shelf tab index
*/
struct gui_vec2 gui_panel_shelf_end(struct gui_panel_layout*, struct gui_panel_layout*);
/*  this function finishes the previously started shelf layout
    Input:
    - previously started group layout
    Output:
    - The from user input updated shelf scrollbar pixel offset
*/
/*
 * -------------------------------------------------------------
 *                          POPUP
 * --------------------------------------------------------------
    POPUP
    The popup extends the normal panel with an overlapping blocking
    panel that needs to be closed before the underlining main panel can
    be used again. Therefore popups are designed for messages,tooltips and
    are used to create the combo box. Internally the popup creates a subbuffer
    inside a command queue that will be drawn after the complete parent panel.

    USAGE
    To create an popup the `gui_panel_popup_begin` function needs to be called
    with to the parent panel local position and size and the wanted type with
    static or dynamic panel. A static panel has a fixed size and behaves like a
    normal panel inside a panel, but a dynamic panel only takes up as much
    height as needed up to a given maximum height. Dynamic panels are for example
    combo boxes while static panel make sense for messsages or tooltips.
    To close a popup you can use the `gui_panel_pop_close` function which takes
    care of the closing process. Finally if the popup panel was completly created
    the `gui_panel_popup_end` function finializes the popup.

    Panel popup API
    gui_panel_popup_begin   -- adds a popup inside a panel
    gui_panel_popup_close   -- closes the popup panel
    gui_panel_popup_end     -- ends the popup building process
*/
enum gui_popup_type {
    GUI_POPUP_STATIC, /* static fixed height non growing popup */
    GUI_POPUP_DYNAMIC /* dynamically growing popup with maximum height */
};

gui_flags gui_panel_popup_begin(struct gui_panel_layout *parent,
                                struct gui_panel_layout *popup,
                                enum gui_popup_type, struct gui_rect bounds,
                                struct gui_vec2 offset);
/*  this function adds a grouped subpanel into the parent panel
    Input:
    - popup position and size of the popup (NOTE: local position)
    - scrollbar pixel offsets for the popup
    Output:
    - popup layout to fill with widgets
*/
void gui_panel_popup_close(struct gui_panel_layout *popup);
/*  this functions closes a previously opened popup */
struct gui_vec2 gui_panel_popup_end(struct gui_panel_layout *parent,
                                    struct gui_panel_layout *popup);
/*  this function finishes the previously started popup layout
    Output:
    - The from user input updated popup scrollbar pixel offset
*/
/*
 * -------------------------------------------------------------
 *                          GRAPH
 * --------------------------------------------------------------
    GRAPH
    The graph widget provided a way to visualize data in either a line or
    column graph.

    USAGE
    To create a graph three different ways are provided. The first one
    is an immediate mode API which allows the push values one by one
    into the graph. The second one is a retain mode function which takes
    an array of float values and converts them into a graph. The final
    function is based on a callback and is mainly a good option if you
    want to draw a mathematical function like for example sine or cosine.

    graph widget API
    gui_panel_graph_begin   -- immediate mode graph building begin sequence point
    gui_panel_graph_push    -- push a value into a graph
    gui_panel_graph_end     -- immediate mode graph building end sequence point
    gui_panel_graph         -- retained mode graph with array of values
    gui_panel_graph_ex      -- ratained mode graph with getter callback
*/
enum gui_graph_type {
    GUI_GRAPH_LINES,
    /* Line graph with each data point being connected with its previous and next node */
    GUI_GRAPH_COLUMN,
    /* Column graph/Histogram with value represented as bars */
    GUI_GRAPH_MAX
};

struct gui_graph {
    gui_bool valid;
    /* graph valid flag to make sure that the graph is visible */
    enum gui_graph_type type;
    /* graph type with either line or column graph */
    gui_float x, y;
    /* graph canvas space position */
    gui_float w, h;
    /* graph canvas space size */
    gui_float min, max;
    /* min and max value for correct scaling of values */
    struct gui_vec2 last;
    /* last line graph point to connect to. Only used by the line graph */
    gui_size index;
    /* current graph value index*/
    gui_size count;
    /* number of values inside the graph */
};

void gui_panel_graph_begin(struct gui_panel_layout*, struct gui_graph*,
                            enum gui_graph_type, gui_size count,
                            gui_float min, gui_float max);
/*  this function begins a graph building widget
    Input:
    - type of the graph with either lines or bars
    - minimal graph value for the lower bounds of the graph
    - maximal graph value for the upper bounds of the graph
    Output:
    - graph stack object that can be filled with values
*/
gui_bool gui_panel_graph_push(struct gui_panel_layout*,struct gui_graph*,gui_float);
/*  this function pushes a value inside the pushed graph
    Input:
    - value data point to fill into the graph either as point or as bar
*/
void gui_panel_graph_end(struct gui_panel_layout *layout, struct gui_graph*);
/*  this function pops the graph from being used
*/
gui_int gui_panel_graph(struct gui_panel_layout*, enum gui_graph_type,
                        const gui_float *values, gui_size count, gui_size offset);
/*  this function create a graph with given type from an array of value
    Input:
    - type of the graph with either line or bar graph
    - graph values in continues array form
    - number of graph values
    - offset into the value array from which to begin drawing
*/
gui_int gui_panel_graph_callback(struct gui_panel_layout*, enum gui_graph_type,
                            gui_size count, gui_float(*get_value)(void*, gui_size),
                            void *userdata);
/*  this function create a graph with given type from callback providing the
    graph with values
    Input:
    - type of the graph with either line or bar graph
    - number of values inside the graph
    - callback to access the values inside your datastrucutre
    - userdata to pull the graph values from
*/
/*
 * --------------------------------------------------------------
 *                          COMBO BOX
 * --------------------------------------------------------------
    COMBO BOX
    The combo box is a minimizable popup panel and extends the old school
    text combo box with the possibility to fill combo boxes with any kind of widgets.
    The combo box is internall implemented with a dynamic popup panel
    and can only be as height as the panel allows.
    There are two different ways to create a combo box. The first one is a
    standart text combo box which has it own function `gui_panel_combo`. The second
    way is the more complex immediate mode API which allows to create
    any kind of content inside the combo box. In case of the second API it is
    additionally possible and sometimes wanted to close the combo box popup
    panel. This can be achived with `gui_panel_combo_close`.

    combo box API
    gui_panel_combo_begin   -- begins the combo box popup panel
    gui_panel_combo_close   -- closes the previously opened combo box
    gui_panel_combo_end     -- ends the combo box build up process
    gui_panel_combo         -- shorthand version for a text based combo box
*/
void gui_panel_combo(struct gui_panel_layout*, const char **entries,
                    gui_size count, gui_size *current, gui_size row_height,
                    gui_state *active, struct gui_vec2 *scrollbar);
/*  this function creates a standart text based combobox
    Input:
    - parent panel layout the combo box will be placed into
    - string array of all items inside the combo box
    - number of items inside the string array
    - the index of the currently selected item
    - the height of every widget inside the combobox
    - the current state of the combobox
    - the scrollbar offset of the panel scrollbar
    Output:
    - updated currently selected index
    - updated state of the combo box
*/
void gui_panel_combo_begin(struct gui_panel_layout *parent,
                        struct gui_panel_layout *combo, const char *selected,
                        gui_state *active, struct gui_vec2 offset);
/*  this function begins the combobox build up process
    Input:
    - parent panel layout the combo box will be placed into
    - ouput combo box panel layout which will be needed to fill the combo box
    - title of the combo box or in the case of the text combo box the selected item
    - the current state of the combobox with either gui_true (active) or gui_false else
    - the current scrollbar offset of the combo box popup panel
*/
void gui_panel_combo_close(struct gui_panel_layout *combo);
/*  this function closes a opened combobox */
struct gui_vec2 gui_panel_combo_end(struct gui_panel_layout *parent,
                                    struct gui_panel_layout *comob);
/*  this function ends the combobox build up process */
/*
 * --------------------------------------------------------------
 *                          MENU
 * --------------------------------------------------------------
    MENU
    The menu widget provides a overlapping popup panel which can
    be opened/closed by clicking on the menu button. It is normally
    placed at the top of the panel and is independent of the parent
    scrollbar offset. But if needed the menu can even be placed inside the panel.

    menu widget API
    gui_panel_menu_begin    -- begins the menu item build up processs
    gui_panel_menu_push     -- adds a item into the menu
    gui_panel_menu_end      -- ends the menu item build up process
    gui_panel_menu          -- shorthand retain mode array version
*/
#define GUI_NONE (-1)
gui_int gui_panel_menu(struct gui_panel_layout*, const gui_char *title,
                    const char **entries, gui_size count, gui_size row_height,
                    gui_float width, gui_state *active, struct gui_vec2 scrollbar);
/*  this function creates a standart text based combobox
    Input:
    - parent panel layout the combo box will be placed into
    - string array of all items inside the menu
    - number of menu items inside the string array
    - the height of every widget inside the combobox
    - the current state of the menu
    - the scrollbar offset of the panel scrollbar
    Output:
    - updated state of the menu
    - index of the selected menu item or -1 otherwise
*/
void gui_panel_menu_begin(struct gui_panel_layout *parent,
                        struct gui_panel_layout *menu, const char *title,
                        gui_float width, gui_state *active, struct gui_vec2 offset);
/*  this function begins the menu build up process
    Input:
    - parent panel layout the menu will be placed into
    - ouput menu panel layout
    - title of the menu to
    - the current state of the menu with either gui_true (open) or gui_false else
    - the current scrollbar offset of the menu popup panel
*/
void gui_panel_menu_close(struct gui_panel_layout *menu);
/*  this function closes a opened menu */
struct gui_vec2 gui_panel_menu_end(struct gui_panel_layout *parent,
                            struct gui_panel_layout *menu);
/*  this function ends the menu build up process */
/*
 * --------------------------------------------------------------
 *                          TREE
 * --------------------------------------------------------------
    TREE
    The tree widget is standart immediate mode API and divides tree nodes into
    parent nodes and leafes. Nodes have immediate mode function points, while
    leafes are just normal functions. In addition there is a icon version for each
    of the two node types which allows you to add images into a tree node.
    The tree widget supports in contrast to the tree layout a back channel
    for each node and leaf. This allows to return commands back to the user
    to declare what to do with the tree node. This includes cloning which is
    copying the selected node and pasting it in the same parent node, cuting
    which removes nodes from its parents and copyies it into a paste buffer,
    pasting to take all nodes inside the paste buffer and copy it into a node and
    finally removing a tree node.

    tree widget API
    gui_panel_tree_begin            -- begins the tree build up processs
    gui_panel_tree_begin_node       -- adds and opens a normal node to the tree
    gui_panel_tree_begin_node_icon  -- adds a opens a node with an icon to the tree
    gui_panel_tree_end_node         -- ends and closes a previously added node
    gui_panel_tree_leaf             -- adds a leaf node to a prev opened node
    gui_panel_tree_leaf_icon        -- adds a leaf icon node to a prev opended node
    gui_panel_tree_end              -- ends the tree build up process
*/
enum gui_tree_nodes_states {
    GUI_NODE_ACTIVE = 0x01,
    /* the node is currently opened */
    GUI_NODE_SELECTED = 0x02
    /* the node has been seleted by the user */
};

enum gui_tree_node_operation {
    GUI_NODE_NOP,
    /* node did not receive a command */
    GUI_NODE_CUT,
    /* cut the node from the current tree and add into a buffer */
    GUI_NODE_CLONE,
    /* copy current node and add copy into the parent node */
    GUI_NODE_PASTE,
    /* paste all node in the buffer into the tree */
    GUI_NODE_DELETE
    /* remove the node from the parent tree */
};

struct gui_tree {
    struct gui_panel_layout group;
    /* panel to add the tree into  */
    gui_float x_off, at_x;
    /* current x position of the next node */
    gui_int skip;
    /* flag that indicates that a node will be skipped */
    gui_int depth;
    /* current depth of the tree */
};

void gui_panel_tree_begin(struct gui_panel_layout*, struct gui_tree*,
                            const char*, gui_float row_height,
                            struct gui_vec2 scrollbar);
/*  this function begins the tree building process
    Input:
    - title describing the tree or NULL
    - height of every node inside the panel
    - scrollbar offset
    Output:
    - tree build up state structure
*/
enum gui_tree_node_operation gui_panel_tree_begin_node(struct gui_tree*, const char*,
                                                        gui_state*);
/*  this function begins a parent node
    Input:
    - title of the node
    - current node state
    Output:
    - operation identifier what should be done with this node
*/
enum gui_tree_node_operation gui_panel_tree_begin_node_icon(struct gui_tree*,
                                                    const char*, struct gui_image,
                                                    gui_state*);
/*  this function begins a text icon parent node
    Input:
    - title of the node
    - icon of the node
    - current node state
    Output:
    - operation identifier what should be done with this node
*/
void gui_panel_tree_end_node(struct gui_tree*);
/*  this function ends a parent node */
enum gui_tree_node_operation gui_panel_tree_leaf(struct gui_tree*, const char*,
                                                    gui_state*);
/*  this function pushes a leaf node to the tree
    Input:
    - title of the node
    - current leaf node state
    Output:
    - operation identifier what should be done with this node
*/
enum gui_tree_node_operation gui_panel_tree_leaf_icon(struct gui_tree*,
                                                    const char*, struct gui_image,
                                                    gui_state*);
/*  this function pushes a leaf icon node to the tree
    Input:
    - title of the node
    - icon of the node
    - current leaf node state
    Output:
    - operation identifier what should be done with this node
*/
struct gui_vec2 gui_panel_tree_end(struct gui_panel_layout*, struct gui_tree*);
/*  this function ends a the tree building process */
/*
 * -------------------------------------------------------------
 *                          TABLE
 * --------------------------------------------------------------
    TABLE
    Temporary table widget. Needs to be rewritten to be actually useful.

    Table widget API
    gui_panel_table_begin           -- begin table build up process
    gui_panel_table_row             -- seperates tables rows
    gui_panel_table_end             -- ends the table build up process
*/
enum gui_table_lines {
    GUI_TABLE_HHEADER = 0x01,
    /* Horizontal table header lines */
    GUI_TABLE_VHEADER = 0x02,
    /* Vertical table header lines */
    GUI_TABLE_HBODY = 0x04,
    /* Horizontal table body lines */
    GUI_TABLE_VBODY = 0x08
    /* Vertical table body lines */
};

void gui_panel_table_begin(struct gui_panel_layout*, gui_flags flags,
                            gui_size row_height, gui_size cols);
/*  this function set the panel to a table state which enable you to create a
    table with the standart panel row layout
    Input:
    - table row and column line seperator flags
    - height of each table row
    - number of columns inside the table
*/
void gui_panel_table_row(struct gui_panel_layout*);
/*  this function add a row with line seperator into asa table marked table
*/
void gui_panel_table_end(struct gui_panel_layout*);
/*  this function finished the table build up process and reverts the panel back
    to its normal state.
*/
/*
 * ==============================================================
 *
 *                          Window Layout
 *
 * ===============================================================
 */
/*  LAYOUT
    ----------------------------
    The tiled layout provides a way to divide the screen into slots which
    again can be divided into either horizontal or vertical panels or another
    tiled layout. This is especially usefull for more complex application which
    need more than just fixed or overlapping panels. There are five slots
    (Top, Left, Center, Right, Bottom) in the layout which are either be
    scaleable or static and occupy a certain percentage of the screen.

    USAGE
    ----------------------------
    To use the tile layout you first have to define the bounds of the layout,
    which slots of the layout is going to be used, how many panels are contained
    inside each slot as well as if a slot can be scaled or is static.
    This is done by calling `gui_layout_slot` for scaleable and `gui_layout_slot_locked`
    for non-scaleable slot in between the `gui_layout_begin` and `gui_layout_end` call,
    for each used layout slot. After that each panel will have to take the tiled
    layout as argument in the `gui_panel_begin_tiled` function call.

    -----------------------------
    |           Top             |
    -----------------------------
    |       |           |       |
    | Left  |   Center  | Right |
    |       |           |       |
    -----------------------------
    |          Bottom           |
    -----------------------------

    definition function API
    gui_layout_begin                - begins the layout definition process
    gui_layout_slot_locked          - adds a non scaleable slot
    gui_layout_slot                 - adds a scaleable slot
    gui_layout_end                  - ends the definition process

    update function API
    gui_layout_set_size             - updates the size of the layaout
    gui_layout_set_pos              - updates the position of the layout
    gui_layout_set_state            - activate or deactivate user input
    gui_layout_load                 - position a child layout into parent layout slot
    gui_layout_remove               - removes a panel from the layout
    gui_layout_clear                - removes all panels from the layout
    gui_layout_slot_bounds          - queries the space of a given slot
    gui_layout_slot_panel_bounds    - queries the space of a panel in a slot
*/
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
    /* panels in slots are added left to right */
    GUI_LAYOUT_VERTICAL
    /* panels in slots are added top to bottom */
};

enum gui_layout_slot_state {
    GUI_UNLOCKED,
    /* SLOT is scaleable */
    GUI_LOCKED
    /* SLOT is static */
};

struct gui_layout_slot {
    gui_size capacity;
    /* number of panels inside the slot */
    gui_float value;
    /* temporary storage for the layout build up process */
    struct gui_vec2 ratio;
    /* horizontal and vertical window ratio */
    struct gui_vec2 offset;
    /* position of the slot in the window */
    enum gui_layout_format format;
    /* panel filling layout */
    enum gui_layout_slot_state state;
    /* scaleable state */
};

struct gui_layout {
    gui_float scaler_width;
    /* width of the scaling line between slots */
    struct gui_rect bounds;
    /* bounds of the layout inside the window */
    gui_state active;
    /* flag indicating if the layout is from the user modifyable */
    struct gui_layout_slot slots[GUI_SLOT_MAX];
    /* each slot inside the panel layout */
};

void gui_panel_begin_tiled(struct gui_panel_layout*, struct gui_panel*,
    struct gui_layout*, enum gui_layout_slot_index, gui_size index);
/*  this function begins a tiled panel build up process
    Input:
    - slot the panel will be placed inside the tiled layout
    - panel slot index inside the slot
    - input structure holding all user generated state changes
*/
void gui_layout_begin(struct gui_layout*, struct gui_rect bounds,
                    gui_state);
/*  this function start the definition of the layout slots
    Input:
    - position (width/height) of the layout in the window
    - size (width/height) of the layout in the window
    - layout state with either active as user updateable or inactive for blocked
*/
void gui_layout_slot_locked(struct gui_layout*, enum gui_layout_slot_index,
                            gui_float ratio, enum gui_layout_format,
                            gui_size entry_count);
/*  this function activates a non scaleable slot inside a scaleable layout
    Input:
        - index of the slot to be activated
        - percentage of the screen that is being occupied
        - panel filling format either horizntal or vertical
        - number of panels the slot will be filled with
*/
void gui_layout_slot(struct gui_layout*, enum gui_layout_slot_index, gui_float ratio,
                    enum gui_layout_format, gui_size entry_count);
/*  this function activates a slot inside the layout
    Input:
        - index of the slot to be activated
        - percentage of the screen that is being occupied
        - panel filling format either horizntal or vertical
        - number of panels the slot will be filled with
*/
void gui_layout_end(struct gui_layout*);
/*  this function ends the definition of the layout slots */
void gui_layout_load(struct gui_layout*child, struct gui_layout *parent,
                        enum gui_layout_slot_index, gui_size index);
/*  this function places a child layout into a parent slot panel index
    Input:
        - child layout that will be filled
        - parent layout that provided the position/size and state for the child
        - the slot index the child layout will be placed into
        - the panel index in the slot the child layout will be placed into
*/
void gui_layout_set_size(struct gui_layout*, gui_size width, gui_size height);
/*  this function updates the size of the layout
    Input:
        - size (width/height) of the layout in the window
*/
void gui_layout_set_pos(struct gui_layout*, gui_size x, gui_size y);
/*  this function updates the position of the layout
    Input:
        - position (x/y) of the layout in the window
*/
void gui_layout_set_state(struct gui_layout*, gui_state);
/*  this function changes the user modifiable layout state
    Input:
        - new state of the layout with either active or inactive
*/
void gui_layout_slot_bounds(struct gui_rect *bounds, const struct gui_layout*,
                            enum gui_layout_slot_index);
/*  this function returns the complete space occupied by a given slot
    Input:
        - index of the slot to be queried
    Output:
        - bounds of the slot as a rectangle (x,y,w,h)
*/
void gui_layout_slot_panel_bounds(struct gui_rect *bounds, const struct gui_layout*,
                                enum gui_layout_slot_index, gui_size entry);
/*  this function returns the space occupied by a given panel and slot
    Input:
        - slot index to be queried
        - panel index to be queried
    Output:
        - bounds of the panel inside the slot as a rectangle (x,y,w,h)
*/

#ifdef __cplusplus
}
#endif

#endif /* GUI_H_ */
