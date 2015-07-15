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

#ifndef GUI_ASSERT
/* remove or change if not wanted */
#include <assert.h>
#define GUI_ASSERT(expr) assert(expr)
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

/*
Since the gui uses ANSI C which does not guarantee to have fixed types, you need
to set the appropriate size of each type. However if your developer environment
supports fixed size types by the <stdint> header you can just uncomment the define
to automatically set the correct size for each type in the library:
#define GUI_USE_FIXED_TYPES
*/

#ifdef GUI_USE_FIXED_TYPES
#include <stdint.h>
typedef char gui_char;
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
typedef uintptr_t gui_ptr;
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
typedef unsigned long gui_ptr;
#endif

/* Utilities */
enum {gui_false, gui_true};
enum gui_heading {GUI_UP, GUI_RIGHT, GUI_DOWN, GUI_LEFT};
struct gui_color {gui_byte r,g,b,a;};
struct gui_vec2 {gui_float x,y;};
struct gui_rect {gui_float x,y,w,h;};
struct gui_key {gui_bool down, clicked;};
typedef gui_char gui_glyph[GUI_UTF_SIZE];
typedef union {void *ptr; gui_int id;} gui_handle;
struct gui_image {gui_handle handle; struct gui_rect region;};

/* Callbacks */
struct gui_font;
typedef gui_bool(*gui_filter)(gui_long unicode);
typedef gui_size(*gui_text_width_f)(gui_handle, const gui_char*, gui_size);
typedef gui_size(*gui_paste_f)(gui_handle, char *buffer, gui_size max);
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
    gui_get_null_rect()     -- returns a default clipping rectangle
    gui_utf_decode()        -- decodes a utf-8 glyph into u32 unicode glyph and len
    gui_utf_encode()        -- encodes a u32 unicode glyph into a utf-8 glyph
    gui_image_ptr()         -- create a image handle from pointer
    gui_image_id()          -- create a image handle from integer id
    gui_subimage_ptr()      -- create a sub-image handle from pointer and region
    gui_subimage_id()       -- create a sub-image handle from integer id and region
    gui_rect_is_valid()     -- check if a rectangle inside the image command is valid
    gui_rect()              -- creates a rectangle from x,y-Position and width and height
    gui_vec2()              -- creates a 2D vector, in the best case should not be needed by the user
    gui_rgba()              -- create a gui color struct from rgba color code
    gui_rgb()               -- create a gui color struct from rgb color code
*/
struct gui_rect gui_get_null_rect(void);
gui_size gui_utf_decode(const gui_char*, gui_long*, gui_size);
gui_size gui_utf_encode(gui_long, gui_char*, gui_size);
gui_size gui_utf_len(const gui_char*, gui_size len);
struct gui_image gui_image_ptr(void*);
struct gui_image gui_image_id(gui_int);
struct gui_image gui_subimage_ptr(void*, struct gui_rect);
struct gui_image gui_subimage_id(gui_int, struct gui_rect);
gui_bool gui_rect_is_valid(const struct gui_rect*);
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

*/
/* every key that is being used inside the library */
enum gui_keys {
    GUI_KEY_SHIFT,
    GUI_KEY_DEL,
    GUI_KEY_ENTER,
    GUI_KEY_BACKSPACE,
    GUI_KEY_SPACE,
    GUI_KEY_COPY,
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
    /* number of allcation calls */
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
void gui_buffer_reset(struct gui_buffer*);
/*  this functions resets the buffer back into an empty state */
void gui_buffer_clear(struct gui_buffer*);
/*  this functions frees all memory inside a dynamically growing buffer */
/*
 * ==============================================================
 *
 *                          Commands
 *
 * ===============================================================
 */
/*  COMMAND QUEUE
    ----------------------------
    The command buffer API enqueues draw calls as commands in to a buffer and
    therefore abstracts over drawing routines and enables defered drawing.
    The API offers a number of drawing primitives like lines, rectangles, circles,
    triangles, images, text and clipping rectangles, that have to be drawn by the user.
    Therefore the command buffer is the main toolkit output besides the actual widget output.
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
    struct gui_image img;
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
    {gui_buffer_init(&(b)->base, a, i, g), (b)->use_clipping=c; (b)->clip=gui_get_null_rect();}
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
    {gui_buffer_init_fixed(&(b)->base, m ,s); (b)->use_clipping=c; \
        (b)->clip=gui_get_null_rect();}
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
#define gui_command_buffer_info(status, b)\
    gui_buffer_info((status), &(b)->base)
/*  this function requests memory information from a buffer
    Input:
    - buffer to get the inforamtion from
    Output:
    - buffer memory information
*/
void *gui_command_buffer_push(struct gui_command_buffer*, gui_uint type, gui_size size);
/*  this function push enqueues a command into the buffer
    Input:
    - buffer to push the command into
    - type of the command
    - amount of memory that is needed for the specified command
*/
void gui_command_buffer_push_scissor(struct gui_command_buffer*, gui_float,
                                        gui_float, gui_float, gui_float);
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
void gui_command_buffer_push_rect(struct gui_command_buffer *buffer, gui_float x,
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
void gui_command_buffer_push_circle(struct gui_command_buffer*, gui_float, gui_float,
                                            gui_float, gui_float, struct gui_color);
/*  this function pushes a circle draw command into the buffer
    Input:
    - buffer to push the circle draw command into
    - x position of the top left of the circle
    - y position of the top left of the circle
    - rectangle diameter of the circle
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
void gui_command_buffer_push_image(struct gui_command_buffer*, gui_float,
                                    gui_float, gui_float, gui_float,
                                    struct gui_image*);
/*  this function pushes a image draw command into the buffer
    Input:
    - buffer to push the draw image command into
    - position of the image with x,y position
    - size of the image to draw with width and height
    - rectangle diameter of the circle
    - color of the triangle to draw
*/
void gui_command_buffer_push_text(struct gui_command_buffer*, gui_float, gui_float,
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

#define gui_command(t, c) ((const struct gui_command_##t*)c)
#define gui_command_buffer_begin(b)\
    ((struct gui_command*)(b)->base.memory.ptr)
#define gui_command_buffer_end(b)\
    (gui_ptr_add_const(struct gui_command, (b)->base.memory.ptr, (b)->base.allocated))
#define gui_command_buffer_next(b, c)\
    ((gui_ptr_add_const(struct gui_command,c,c->offset) < gui_command_buffer_end(b))?\
     gui_ptr_add_const(struct gui_command,c,c->offset):NULL)
#define gui_foreach_command(i, b)\
    for((i)=gui_command_buffer_begin(b); (i)!=NULL; (i)=gui_command_buffer_next(b,i))

/*
 * ==============================================================
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
    gui_edit_box_init()         -- initialize a dynamically growing edit box
    gui_edit_box_init_fixed()   -- initialize a statically edit box
    gui_edit_box_reset()        -- resets the edit box back to the beginning
    gui_edit_box_clear()        -- frees all memory of a dynamic edit box
    gui_edit_box_add()          -- adds a symbol to the editbox
    gui_edit_box_remove()       -- removes a symbol from the editbox
    gui_edit_box_get()          -- returns the string inside the editbox
    gui_edit_box_len()          -- returns the length of the string inside the edditbox
*/
struct gui_clipboard {
    gui_handle userdata;
    gui_paste_f paste;
    gui_copy_f copy;
};

typedef struct gui_buffer gui_edit_buffer;
struct gui_edit_box {
    gui_edit_buffer buffer;
    gui_bool active;
    gui_size cursor;
    gui_size glyphes;
    struct gui_clipboard clip;
    gui_filter filter;
};

/* filter function */
gui_bool gui_filter_input_default(gui_long unicode);
gui_bool gui_filter_input_ascii(gui_long unicode);
gui_bool gui_filter_input_float(gui_long unicode);
gui_bool gui_filter_input_decimal(gui_long unicode);
gui_bool gui_filter_input_hex(gui_long unicode);
gui_bool gui_filter_input_oct(gui_long unicode);
gui_bool gui_filter_input_binary(gui_long unicode);

/* editbox */
void gui_edit_box_init(struct gui_edit_box*, struct gui_allocator*, gui_size initial,
                        gui_float grow_fac, const struct gui_clipboard*, gui_filter);
void gui_edit_box_init_fixed(struct gui_edit_box*, void *memory, gui_size size,
                        const struct gui_clipboard*, gui_filter);
#define gui_edit_box_reset(b)\
    do {gui_buffer_reset(&(b)->buffer); (b)->cursor = (b)->glyphes = 0;} while(0);
#define gui_edit_box_clear(b) gui_buffer_clear(&(b)->buffer)
#define gui_edit_box_info(status, b)\
    gui_buffer_info((status), &(b)->buffer)
void gui_edit_box_add(struct gui_edit_box*, const char*, gui_size);
void gui_edit_box_remove(struct gui_edit_box*);
gui_char *gui_edit_box_get(struct gui_edit_box*);
const gui_char *gui_edit_box_get_const(struct gui_edit_box*);
void gui_edit_box_at(struct gui_edit_box*, gui_size pos, gui_glyph, gui_size*);
void gui_edit_box_at_cursor(struct gui_edit_box*, gui_glyph, gui_size*);
gui_char gui_edit_box_at_byte(struct gui_edit_box*, gui_size pos);
void gui_edit_box_set_cursor(struct gui_edit_box*, gui_size pos);
gui_size gui_edit_box_get_cursor(struct gui_edit_box *eb);
gui_size gui_edit_box_len_byte(struct gui_edit_box*);
gui_size gui_edit_box_len(struct gui_edit_box*);
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
    Most widgets takes an input struct, font and widget specific data and a command
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
    gui_spinner             -- unsigned integer spinner widget
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
    /* button only returns on activation */
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

struct gui_scroll {
    gui_float rounding;
    /* scrollbar rectangle rounding */
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

void gui_text(struct gui_command_buffer*, gui_float, gui_float, gui_float, gui_float,
                    const char *text, gui_size len, const struct gui_text*,
                    enum gui_text_align, const struct gui_font*);
/*  this function executes a text widget with text alignment
    Input:
    - output command buffer for drawing
    - (x,y) position
    - (width, height) size
    - string to draw
    - length of the string
    - visual widget style structure describing the text
    - text alignment with either left, center and right
    - font structure for text drawing
*/
gui_bool gui_button_text(struct gui_command_buffer*, gui_float x, gui_float y,
                        gui_float w, gui_float h, const char*,
                        enum gui_button_behavior, const struct gui_button*,
                        const struct gui_input*, const struct gui_font*);
/*  this function executes a text button widget
    Input:
    - output command buffer for drawing
    - (x,y) position
    - (width, height) size
    - button text
    - button behavior with either repeating or transition state event
    - visual widget style structure describing the button
    - input structure to update the button with
    - font structure for text drawing
    Output:
    - returns gui_true if the button was pressed gui_false otherwise
*/
gui_bool gui_button_image(struct gui_command_buffer*, gui_float x, gui_float y,
                            gui_float w, gui_float h, struct gui_image img,
                            enum gui_button_behavior, const struct gui_button*,
                            const struct gui_input*);
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
gui_bool gui_button_triangle(struct gui_command_buffer*, gui_float x, gui_float y,
                            gui_float w, gui_float h, enum gui_heading,
                            enum gui_button_behavior, const struct gui_button*,
                            const struct gui_input*);
/*  this function executes a triangle button widget
    Input:
    - output command buffer for drawing
    - (x,y) position
    - (width, height) size
    - triangle direction with either left, top, right xor bottom
    - button behavior with either repeating or transition state event
    - visual widget style structure describing the button
    - input structure to update the button with
    Output:
    - returns gui_true if the button was pressed gui_false otherwise
*/
gui_bool gui_button_text_triangle(struct gui_command_buffer*, gui_float x, gui_float y,
                                    gui_float w, gui_float h, enum gui_heading,
                                    const char*,enum gui_text_align,
                                    enum gui_button_behavior,
                                    const struct gui_button*, const struct gui_font*,
                                    const struct gui_input*);
/*  this function executes a button with text and a triangle widget
    Input:
    - output command buffer for drawing
    - (x,y) position
    - (width, height) size
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
gui_bool gui_button_text_image(struct gui_command_buffer *out, gui_float x, gui_float y,
                            gui_float w, gui_float h, struct gui_image img,
                            const char* text, enum gui_text_align align,
                            enum gui_button_behavior behavior,
                            const struct gui_button *button, const struct gui_font *f,
                            const struct gui_input *i);
/*  this function executes a button widget with text and an icon
    Input:
    - output command buffer for drawing
    - (x,y) position
    - (width, height) size
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
gui_bool gui_toggle(struct gui_command_buffer*, gui_float x, gui_float y, gui_float w,
                    gui_float h, gui_bool, const char*, enum gui_toggle_type,
                    const struct gui_toggle*, const struct gui_input*,
                    const struct gui_font*);
/*  this function executes a toggle (checkbox, radiobutton) widget
    Input:
    - output command buffer for drawing
    - (x,y) position
    - (width, height) size
    - active or inactive flag describing the state of the toggle
    - visual widget style structure describing the toggle
    - input structure to update the toggle with
    - font structure for text drawing
    Output:
    - returns the update state of the toggle
*/
gui_float gui_slider(struct gui_command_buffer*, gui_float x, gui_float y, gui_float,
                    gui_float h, gui_float min, gui_float val, gui_float max,
                    gui_float step, const struct gui_slider*, const struct gui_input*);
/*  this function executes a slider widget
    Input:
    - output command buffer for drawing
    - (x,y) position
    - (width, height) size
    - minimal slider value that will not be underflown
    - slider value to be updated by the user
    - maximal slider value that will not be overflown
    - step interval the value will be updated with
    - visual widget style structure describing the slider
    - input structure to update the slider with
    Output:
    - returns the from the user input updated value
*/
gui_size gui_progress(struct gui_command_buffer*, gui_float x, gui_float y,
                        gui_float w, gui_float h, gui_size value, gui_size max,
                        gui_bool modifyable, const struct gui_progress*,
                        const struct gui_input*);
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
void gui_editbox(struct gui_command_buffer*, gui_float x, gui_float y, gui_float w,
                gui_float h, struct gui_edit_box*, const struct gui_edit*,
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
gui_size gui_edit(struct gui_command_buffer*, gui_float x, gui_float y, gui_float w,
                    gui_float h, gui_char*, gui_size, gui_size max, gui_bool*,
                    const struct gui_edit*, enum gui_input_filter filter,
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
    - glyph input filter type to only let specified glyph through
    - input structure to update the editbox with
    - font structure for text drawing
    Output:
    - state of the editbox with either active or inactive
    - returns the size of the buffer in bytes after the modification
*/
gui_size gui_edit_filtered(struct gui_command_buffer*, gui_float x, gui_float y,
                            gui_float w, gui_float h, gui_char*, gui_size,
                            gui_size max, gui_bool*, const struct gui_edit*,
                            gui_filter filter, const struct gui_input*,
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
    - glyph input filter callback to only let specified glyph through
    - input structure to update the editbox with
    - font structure for text drawing
    Output:
    - state of the editbox with either active or inactive
    - returns the size of the buffer in bytes after the modification
*/
gui_int gui_spinner(struct gui_command_buffer*, gui_float x, gui_float y, gui_float w,
                    gui_float h, const struct gui_spinner*, gui_int min, gui_int value,
                    gui_int max, gui_int step, gui_bool *active,
                    const struct gui_input*, const struct gui_font*);
/*  this function executes a spinner widget
    Input:
    - output command buffer for draw commands
    - (x,y) position
    - (width, height) size
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
gui_size gui_selector(struct gui_command_buffer*, gui_float x, gui_float y,
                        gui_float w, gui_float h, const struct gui_selector*,
                        const char *items[], gui_size item_count,
                        gui_size item_current, const struct gui_input*,
                        const struct gui_font*);
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
gui_float gui_scroll(struct gui_command_buffer*, gui_float x, gui_float y,
                    gui_float w, gui_float h, gui_float offset, gui_float target,
                    gui_float step, const struct gui_scroll*, const struct gui_input*);
/*  this function executes a scrollbar widget
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
    GUI_PROPERTY_SCROLLBAR_WIDTH,
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
    GUI_DEFAULT_PROPERTIES = 0x02,
    GUI_DEFAULT_ROUNDING = 0x04,
    GUI_DEFAULT_ALL = 0xFFFF
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
                                gui_float, gui_float);
/*  this function temporarily changes a property in a stack like fashion to be reseted later
    Input:
    - Configuration structure to push the change to
    - Property idenfifier to change
    - first value of the property most of the time the x position
    - second value of the property most of the time the y position
*/
void gui_config_push_color(struct gui_config*, enum gui_config_colors,
                            gui_byte, gui_byte, gui_byte, gui_byte);
/*  this function temporarily changes a color in a stack like fashion to be reseted later
    Input:
    - Configuration structure to push the change to
    - color idenfifier to change
    - red color component
    - green color component
    - blue color component
    - alpha color component
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
 */
/*  PANEL
    ----------------------------
    The Panel function API is based on the widget API and is almost in its
    entirety based on positioning of groups of widgets. Almost each widget inside the
    panel API uses the widget API for drawing and manipulation/input logic
    but offers a uniform style over a single configuration structure as well as
    widget group base moving, spacing and structuring. The panel references
    a basic configuration file, an output commmand buffer and input structure which
    need to share the same or greater life time than the panel since they are relied
    on by the panel.

    USAGE
    ----------------------------
    To setup the Panel API you have to initiate the panel first with position, size
    and behavior flags. The flags inside the panel describe the behavior of the panel
    and can be set or modified directly over the public panel struture
    or at the beginning in the initialization phase. Just like the flags the position
    and size of the panel is made directly modifiable at any given time given single
    threaded access while changes are only visible outside the layout buildup process.

    To finally use the panel a panel layout has to be created over gui_panle_begin_xxx
    which sets up the panel layout build up process. The panel layout has to be kept
    valid over the course of the build process until gui_panel_end is called, which
    makes the layout perfectly fit for either a stack object or a single instance for
    every panel. The begin sequence pont of the panel layout also gives the opportunity to
    add the panel either into a panel stack for overlapping panels or a tiled border
    layout for automated window independend panel positing and sizing.

    To add widgets into the panel layout a number of basic widget are provided
    which can be added by calling the appropriate function inside both panel
    layout sequene points gui_panel_begin and gui_panel_end. All calls outside
    both sequence points are invalid and can cause undefined behavior.

    Since the panel has no information about the structuring of widgets a
    row layout has to be set with row height and number of columns which can
    be changed and set by calling the gui_panel_row function.
    IMPORTANT: !IF YOUR LAYOUT IS WRONG FIRST CHECK IF YOU CALLED gui_panel_row CORRECTLY XOR AT ALL!

    Panel function API
    gui_panel_init          -- initializes the panel with position, size and flags
    gui_panel_begin         -- begin sequence point in the panel layout build up process
    gui_panel_begin_stacked -- extends gui_panel_begin by adding the panel into a panel stack
    gui_panel_begin_tiled   -- extends gui_panel_begin by adding the panel into a tiled layout
    gui_panel_row           -- defines the current row layout with row height and number of columns
    gui_panel_row_begin     -- begins the row build up process
    gui_panel_row_push_widget-- pushes the next added widget width as a ratio of the provided space
    gui_panel_row_end       -- ends the row build up process
    gui_panel_row_columns   -- returns the number of possible columns with a given width in a table layout
    gui_panel_pixel_to_ratio-- convert a widget width from pixel into a panel space ratio
    gui_panel_widget        -- base function for all widgets to allocate space on the panel
    gui_panel_spacing       -- create a column seperator and is basically an empty widget
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
    gui_panel_editbox       -- edit textbox for text input with cursor, clipboard and filter
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
    /* Horizontal table header lines */
    GUI_TABLE_VHEADER = 0x02,
    /* Vertical table header lines */
    GUI_TABLE_HBODY = 0x04,
    /* Horizontal table body lines */
    GUI_TABLE_VBODY = 0x08
    /* Vertical table body lines */
};

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

enum gui_panel_tab {
    GUI_MAXIMIZED = gui_false,
    /* Flag indicating that the panel tab is open */
    GUI_MINIMIZED = gui_true
    /* Flag indicating that the panel tab is closed */
};

enum gui_panel_flags {
    GUI_PANEL_HIDDEN = 0x01,
    /* Hiddes the panel and stops any panel interaction and drawing can be set
     * by user input or by closing the panel */
    GUI_PANEL_BORDER = 0x02,
    /* Draws a border around the panel to visually seperate the panel from the
     * background */
    GUI_PANEL_MINIMIZABLE = 0x04,
    /* Enables the panel to be minimized/collapsed and adds a minimizing icon
     * in the panel header to be clicked by GUI user */
    GUI_PANEL_CLOSEABLE = 0x08,
    /* Enables the panel to be closed, hidden and made non interactive for the
     * user by adding a closing icon in the panel header */
    GUI_PANEL_MOVEABLE = 0x10,
    /* The moveable flag inidicates that a panel can be move by user input by
     * dragging the panel header */
    GUI_PANEL_SCALEABLE = 0x20,
    /* The scaleable flag indicates that a panel can be scaled by user input
     * by dragging a scaler icon at the button of the panel */
    GUI_PANEL_NO_HEADER = 0x40,
    /* To remove the header from the panel and invalidate all panel header flags
     * the NO HEADER flags was added */
    GUI_PANEL_BORDER_HEADER = 0x80,
    /* Draws a border inside the panel for the panel header seperating the body
     * and header of the panel */
    GUI_PANEL_ACTIVE = 0x100,
    /* INTERNAL ONLY!: marks the panel as active, used by the panel stack */
    GUI_PANEL_SCROLLBAR = 0x200,
    /* INTERNAL ONLY!: adds a scrollbar to the panel which enables fixed size
     * panels with unlimited amount of space to fill */
    GUI_PANEL_TAB = 0x400,
    /* INTERNAL ONLY!: Marks the panel as an subpanel of another panel(Groups/Tabs/Shelf)*/
    GUI_PANEL_DO_NOT_RESET = 0x800
    /* INTERNAL ONLY!: requires that the panel does not resets the command buffer */
};

struct gui_panel {
    gui_float x, y;
    /* position in the os window */
    gui_float w, h;
    /* size with width and height of the panel */
    gui_flags flags;
    /* panel flags modifing its behavior */
    gui_float offset;
    /* panel scrollbar offset in pixel */
    gui_bool minimized;
    /* flag indicating if the panel is collapsed */
    const struct gui_config *config;
    /* configuration reference describing the panel style */
    struct gui_command_buffer *buffer;
    /* output command buffer queuing all drawing calls */
    struct gui_panel* next;
    /* next panel pointer for the panel stack*/
    struct gui_panel* prev;
    /* prev panel pointer for the panel stack*/
};

enum gui_panel_row_layout_type {
    GUI_PANEL_ROW_LAYOUT_TABLE,
    /* table like row layout with fixed widget width */
    GUI_PANEL_ROW_LAYOUT_RATIO
    /* freely defineable widget width */
};

#define GUI_UNDEFINED (-1.0f)
struct gui_panel_row_layout {
    enum gui_panel_row_layout_type type;
    /* type of the row layout */
    gui_float height;
    /* height of the current row */
    gui_size columns;
    /* number of columns in the current row */
    const gui_float *ratio;
    /* row widget width ratio */
    gui_float item_ratio;
    /* current with of very item */
    gui_float item_offset;
    /* x positon offset of the current item */
    gui_float filled;
    /* total fill ratio */
};

struct gui_panel_layout {
    gui_float x, y, w, h;
    /* position and size of the panel in the os window */
    gui_float offset;
    /* panel scrollbar offset */
    gui_bool is_table;
    /* flag indicating if the panel is currently creating a table */
    gui_flags tbl_flags;
    /* flags describing the line drawing for every row in the table */
    gui_bool valid;
    /* flag inidicating if the panel is visible */
    gui_float at_x, at_y;
    /* index position of the current widget row and column  */
    gui_float width, height;
    /* size of the actual useable space inside the panel */
    gui_float header_height;
    /* height of the panel header space */
    gui_size index;
    /* index of the current widget in the current panel row */
    struct gui_panel_row_layout row;
    /* currently used panel row layout */
    struct gui_rect clip;
    /* panel clipping rect needed by scrolling */
    const struct gui_config *config;
    /* configuration data describing the visual style of the panel */
    const struct gui_input *input;
    /* current input state for updating the panel and all its widgets */
    struct gui_command_buffer *buffer;
    /* command draw call output command buffer */
};

struct gui_layout;
void gui_panel_init(struct gui_panel*, gui_float x, gui_float y, gui_float w,
                    gui_float h, gui_flags, struct gui_command_buffer*,
                    const struct gui_config*);
/*  this function initilizes and setups the panel
    Input:
    - bounds of the panel with x,y position and width and height
    - panel flags for modified panel behavior
    - reference to a output command buffer to push draw calls to
    - configuration file containing the style, color and font for the panel
    Output:
    - a newly initialized panel
*/
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
gui_bool gui_panel_begin(struct gui_panel_layout *layout, struct gui_panel*,
                        const char *title, const struct gui_input*);
/*  this function begins the panel build up process
    Input:
    - title of the panel visible in th header
    - input structure holding all user generated state changes
    Output:
    - panel layout to fill up with widgets
*/
struct gui_stack;
gui_bool gui_panel_begin_stacked(struct gui_panel_layout*, struct gui_panel*,
                                struct gui_stack*, const char*,
                                const struct gui_input*);
/*  this function begins the panel build up process and push the panel into a panel stack
    Input:
    - panel stack to push the panel into
    - title of the panel visible in th header
    - input structure holding all user generated state changes
    Output:
    - panel layout to fill up with widgets
*/
gui_bool gui_panel_begin_tiled(struct gui_panel_layout*, struct gui_panel*,
                                struct gui_layout*, gui_uint slot, gui_size index,
                                const char*, const struct gui_input*);
/*  this function begins the panel build up process and push the panel into a tiled
 *  layout container
    Input:
    - tiled layout container to push the panel into
    - slot inside the panel with either top,button,center,left or right position
    - index inside the slot to position the panel into
    - title of the panel visible in th header
    - input structure holding all user generated state changes
    Output:
    - panel layout to fill up with widgets
*/
void gui_panel_row(struct gui_panel_layout*, gui_float height, gui_size cols);
/*  this function set the current panel row layout
    Input:
    - panel row layout height in pixel
    - panel row layout column count
*/
gui_float gui_panel_pixel_to_ratio(struct gui_panel_layout *layout, gui_size pixel);
/*  converts the width of a widget into a percentage of the panel
    Input:
    - widget width in pixel
    Output:
    - widget width in panel space percentage
*/
void gui_panel_row_begin(struct gui_panel_layout*, gui_float height);
/*  this function start the row build up process
    Input:
    - row height inhereted by all widget inside the row
*/
void gui_panel_row_push_widget(struct gui_panel_layout*, gui_float ratio);
/*  this function directly sets the width ratio of the next added widget
    Input:
    - ratio percentage value (0.0f-1.0f) of the needed row space
*/
void gui_panel_row_end(struct gui_panel_layout*);
/* this function ends the row build up process */
void gui_panel_row_templated(struct gui_panel_layout*, gui_float height,
                            gui_size cols, const gui_float *ratio);
/*  this function set the current panel row layout as a array of ratios
    Input:
    - panel row layout height in pixel
    - panel row layout column count
    - array with percentage size values for each column
*/
gui_size gui_panel_row_columns(const struct gui_panel_layout *layout,
                                gui_size widget_size);
/*  this function calculates the possible number of widget with the same width in the
    current row layout.
    Input:
    - size of all widgets that need to fit into the current panel row layout
    Output:
    - panel layout to fill up with widgets
*/
gui_bool gui_panel_widget(struct gui_rect*, struct gui_panel_layout*);
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
                        gui_size max, gui_bool *active, enum gui_input_filter);
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
                                gui_size len, gui_size max,
                                gui_bool *active, gui_filter);
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
gui_int gui_panel_spinner(struct gui_panel_layout*, gui_int min, gui_int value,
                            gui_int max, gui_int step, gui_bool *active);
/*  this function creates a spinner widget
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
gui_int gui_panel_graph_ex(struct gui_panel_layout*, enum gui_graph_type,
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
gui_bool gui_panel_tab_begin(struct gui_panel_layout*, struct gui_panel_layout *tab,
                            const char*, gui_bool);
/*  this function adds a tab subpanel into the parent panel
    Input:
    - tab title to write into the header
    - state of the tab with either collapsed(GUI_MINIMIZED) or open state
    Output:
    - tab layout to fill with widgets
    - wether the tab is currently collapsed(gui_true) or open(gui_false)
*/
void gui_panel_tab_end(struct gui_panel_layout*, struct gui_panel_layout *tab);
/*  this function finishes the previously started tab and allocated the needed
    tab space in the parent panel
*/
void gui_panel_group_begin(struct gui_panel_layout*, struct gui_panel_layout *tab,
                            const char*, gui_float offset);
/*  this function adds a grouped subpanel into the parent panel
    IMPORTANT: You need to set the height of the group with panel_row_layout
    Input:
    - group title to write into the header
    - group scrollbar offset
    Output:
    - group layout to fill with widgets
*/
gui_float gui_panel_group_end(struct gui_panel_layout*, struct gui_panel_layout* tab);
/*  this function finishes the previously started group layout
    Output:
    - The from user input updated group scrollbar pixel offset
*/
gui_size gui_panel_shelf_begin(struct gui_panel_layout*, struct gui_panel_layout*,
                                const char *tabs[], gui_size size,
                                gui_size active, gui_float offset);
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
gui_float gui_panel_shelf_end(struct gui_panel_layout*, struct gui_panel_layout*);
/*  this function finishes the previously started shelf layout
    Input:
    - previously started group layout
    Output:
    - The from user input updated shelf scrollbar pixel offset
*/
void gui_panel_end(struct gui_panel_layout*, struct gui_panel*);
/*  this function ends the panel layout build up process and updates the panel
*/
/*
 * ==============================================================
 *
 *                          Stack
 *
 * ===============================================================
 */
struct gui_stack {
    gui_size count;
    /* number of panels inside the stack */
    struct gui_panel *begin;
    /* first panel inside the panel which will be drawn first */
    struct gui_panel *end;
    /* currently active panel which will be drawn last */
};

void gui_stack_clear(struct gui_stack*);
/* this function clears and reset the stack back to an empty state */
void gui_stack_push(struct gui_stack*, struct gui_panel*);
/* this function add a panel into the stack if the panel is not already inside
 * the stack */
void gui_stack_pop(struct gui_stack*, struct gui_panel*);
/* this function removes a panel from the stack */
#define gui_foreach_panel(i, s) for (i = (s)->begin; i != 0; i = (i)->next)
/* iterates over each panel inside the stack */

/*
 * ==============================================================
 *
 *                          Layout
 *
 * ===============================================================
 */
/*
-----------------------------
|           Top             |
-----------------------------
|       |           |       |
| Left  |   Center  | Right |
|       |           |       |
-----------------------------
|          Bottom           |
-----------------------------
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

enum gui_layout_state {
    GUI_LAYOUT_DEACTIVATE,
    /* all panels inside the layout can NOT be modified by user input */
    GUI_LAYOUT_ACTIVATE
    /* all panels inside the layout can be modified by user input */
};

enum gui_layout_flags {
    GUI_LAYOUT_INACTIVE = 0x01,
    /* tiled layout is inactive and cannot be updated by the user */
    GUI_LAYOUT_SCALEABLE = 0x02
    /* tiled layout ratio can be changed by the user */
};

struct gui_layout {
    gui_float scaler_width;
    /* width of the scaling line between slots */
    gui_size x, y;
    /* position of the layout inside the window */
    gui_size width, height;
    /* size of the layout inside the window */
    gui_flags flags;
    /* flag indicating if the layout is from the user modifyable */
    struct gui_stack stack;
    /* panel stack of all panels inside the layout */
    struct gui_layout_slot slots[GUI_SLOT_MAX];
    /* each slot inside the panel layout */
};

void gui_layout_begin(struct gui_layout*, gui_size x, gui_size y,
                        gui_size width, gui_size height, gui_flags);
/*  this function start the definition of the layout slots
    Input:
    - position (width/height) of the layout in the window
    - size (width/height) of the layout in the window
    - layout flag settings
*/
void gui_layout_slot(struct gui_layout*, enum gui_layout_slot_index, gui_float ratio,
                    enum gui_layout_format, gui_size panel_count);
/*  this function activates a slot inside the layout
    Input:
        - index of the slot to be activated
        - percentage of the screen that is being occupied
        - panel filling format either horizntal or vertical
        - number of panels the slot will be filled with
*/
void gui_layout_slot_locked(struct gui_layout*, enum gui_layout_slot_index, gui_float ratio,
                            enum gui_layout_format, gui_size panel_count);
/*  this function activates a non scaleable slot inside a scaleable layout
    Input:
        - index of the slot to be activated
        - percentage of the screen that is being occupied
        - panel filling format either horizntal or vertical
        - number of panels the slot will be filled with
*/
void gui_layout_end(struct gui_layout*);
/*  this function ends the definition of the layout slots */
void gui_layout_update_pos(struct gui_layout*, gui_size x, gui_size y);
/*  this function updates the position of the layout
    Input:
        - position (x/y) of the layout in the window
*/

void gui_layout_update_size(struct gui_layout*, gui_size width, gui_size height);
/*  this function updates the size of the layout
    Input:
        - size (width/height) of the layout in the window
*/
void gui_layout_update_state(struct gui_layout*, gui_uint state);
/*  this function changes the user modifiable layout state
    Input:
        - new state of the layout with either active or inactive
*/

#ifdef __cplusplus
}
#endif

#endif /* GUI_H_ */
