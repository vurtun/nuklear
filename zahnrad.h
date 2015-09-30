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
#ifndef ZR_H_
#define ZR_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * ==============================================================
 *
 *                          Constants
 *
 * ===============================================================
 */
#define ZR_UTF_INVALID 0xFFFD
#define ZR_UTF_SIZE 4
/* describes the number of bytes a glyph consists of*/
#define ZR_INPUT_MAX 16
/* defines the max number of bytes to be added as text input in one frame */
#define ZR_MAX_COLOR_STACK 32
/* defines the number of temporary configuration color changes that can be stored */
#define ZR_MAX_ATTRIB_STACK 32
/* defines the number of temporary configuration attribute changes that can be stored */

/*
 * ==============================================================
 *
 *                      Compiler switches
 *
 * ===============================================================
 */
#define ZR_COMPILE_WITH_FIXED_TYPES 1
/* setting this define to 1 adds header <stdint.h> for fixed sized types
 * if 0 each type has to be set to the correct size*/
#define ZR_COMPILE_WITH_ASSERT 1
/* setting this define to 1 adds the <assert.h> header for the assert macro
  IMPORTANT: it also adds clib so only use it if wanted */
#define ZR_COMPILE_WITH_VERTEX_BUFFER 1
/* setting this define to 1 adds a vertex draw command list backend to this library,
  which allows you to convert queue commands into vertex draw commands.
  If you do not want or need a default backend you can set this flag to zero
  and the module of the library will not be compiled */
#define ZR_COMPILE_WITH_FONT 1
/* setting this define to 1 adds the `stb_truetype` and `stb_rect_pack` header
 to this library and provides a default font for font loading and rendering.
 If you already have font handling or do not want to use this font handler
 you can just set this define to zero and the font module will not be compiled
 and the two headers will not be needed. */
/*
 * ==============================================================
 *
 *                          Basic Types
 *
 * ===============================================================
 */
#if ZR_COMPILE_WITH_FIXED_TYPES
#include <stdint.h>
typedef char zr_char;
typedef int32_t zr_int;
typedef int32_t zr_bool;
typedef int16_t zr_short;
typedef int64_t zr_long;
typedef float zr_float;
typedef double zr_double;
typedef uint16_t zr_ushort;
typedef uint32_t zr_uint;
typedef uint64_t zr_ulong;
typedef uint32_t zr_flags;
typedef zr_flags zr_state;
typedef uint8_t zr_byte;
typedef uint32_t zr_flag;
typedef uint64_t zr_size;
typedef uintptr_t zr_ptr;
#else
typedef int zr_int;
typedef int zr_bool;
typedef char zr_char;
typedef short zr_short;
typedef long zr_long;
typedef float zr_float;
typedef double zr_double;
typedef unsigned short zr_ushort;
typedef unsigned int zr_uint;
typedef unsigned long zr_ulong;
typedef unsigned int zr_flags;
typedef zr_flags zr_state;
typedef unsigned char zr_byte;
typedef unsigned int zr_flag;
typedef unsigned long zr_size;
typedef unsigned long zr_ptr;
#endif

#if ZR_COMPILE_WITH_ASSERT
#ifndef ZR_ASSERT
#include <assert.h>
#define ZR_ASSERT(expr) assert(expr)
#endif
#else
#define ZR_ASSERT(expr)
#endif

/* Callbacks */
struct zr_user_font;
struct zr_edit_box;
struct zr_user_font_glyph;
/*
 * ==============================================================
 *
 *                          Utility
 *
 * ===============================================================
 */
/*  Utility
    ----------------------------
    The utility API provides a number of object construction function
    for some gui specific objects like image handle, vector, color and rectangle.

    USAGE
    ----------------------------
    Utility function API
    zr_get_null_rect   -- returns a default clipping rectangle
    zr_utf_decode      -- decodes a utf-8 glyph into u32 unicode glyph and len
    zr_utf_encode      -- encodes a u32 unicode glyph into a utf-8 glyph
    zr_image_ptr       -- create a image handle from pointer
    zr_image_id        -- create a image handle from integer id
    zr_subimage_ptr    -- create a sub-image handle from pointer and region
    zr_subimage_id     -- create a sub-image handle from integer id and region
    zr_rect_is_valid   -- check if a rectangle inside the image command is valid
    zr_rect            -- creates a rectangle from x,y-Position and width and height
    zr_vec2            -- creates a 2D floating point vector
    zr_rgba            -- create a gui color struct from rgba color code
    zr_rgb             -- create a gui color struct from rgb color code
*/
enum {zr_false, zr_true};
enum zr_heading {ZR_UP, ZR_RIGHT, ZR_DOWN, ZR_LEFT};
struct zr_color {zr_byte r,g,b,a;};
struct zr_vec2 {zr_float x,y;};
struct zr_vec2i {zr_short x, y;};
struct zr_rect {zr_float x,y,w,h;};
struct zr_recti {zr_ushort x,y,w,h;};
typedef zr_char zr_glyph[ZR_UTF_SIZE];
typedef union {void *ptr; zr_int id;} zr_handle;
struct zr_image {zr_handle handle; zr_ushort w, h; zr_ushort region[4];};

/* -----------------------  POINTER ---------------------------------*/
#define zr_ptr_add(t, p, i) ((t*)((void*)((zr_byte*)(p) + (i))))
#define zr_ptr_sub(t, p, i) ((t*)((void*)((zr_byte*)(p) - (i))))
#define zr_ptr_add_const(t, p, i) ((const t*)((const void*)((const zr_byte*)(p) + (i))))
#define zr_ptr_sub_const(t, p, i) ((const t*)((const void*)((const zr_byte*)(p) - (i))))
/* -----------------------  MATH ---------------------------------*/
struct zr_rect zr_get_null_rect(void);
struct zr_rect zr_rect(zr_float x, zr_float y, zr_float w, zr_float h);
struct zr_vec2 zr_vec2(zr_float x, zr_float y);
/* -----------------------  COLOR ---------------------------------*/
struct zr_color zr_rgba(zr_byte r, zr_byte g, zr_byte b, zr_byte a);
struct zr_color zr_rgb(zr_byte r, zr_byte g, zr_byte b);
struct zr_color zr_rgba_f(zr_float r, zr_float g, zr_float b, zr_float a);
struct zr_color zr_rgb_f(zr_float r, zr_float g, zr_float b);
struct zr_color zr_hsv(zr_float h, zr_float s, zr_float v);
struct zr_color zr_rgba32(zr_uint);
zr_uint zr_color32(struct zr_color);
void zr_colorf(zr_float *r, zr_float *g, zr_float *b, zr_float *a, struct zr_color);
void zr_color_hsv(zr_float *out_h, zr_float *out_s, zr_float *out_v, struct zr_color);
/* -----------------------  IMAGE ---------------------------------*/
zr_handle zr_handle_ptr(void*);
zr_handle zr_handle_id(zr_int);
struct zr_image zr_image_ptr(void*);
struct zr_image zr_image_id(zr_int);
struct zr_image zr_subimage_ptr(void*, zr_ushort w, zr_ushort h, struct zr_rect);
struct zr_image zr_subimage_id(zr_int, zr_ushort w, zr_ushort h, struct zr_rect);
zr_bool zr_image_is_subimage(const struct zr_image* img);
/* -----------------------  UTF-8 ---------------------------------*/
zr_size zr_utf_decode(const zr_char*, zr_long*, zr_size);
zr_size zr_utf_encode(zr_long, zr_char*, zr_size);
zr_size zr_utf_len(const zr_char*, zr_size len);
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
    zr_input struct which holds the input state while running.
    It is important to note that no direct os or window handling is done by the input
    API, instead all the input state has to be provided from the user. This in one hand
    expects more work from the user and complicates the usage but on the other hand
    provides simple abstraction over a big number of platforms, libraries and other
    already provided functionality.

    USAGE
    ----------------------------
    To instantiate the Input API the zr_input structure has to be zeroed at
    the beginning of the program by either using memset or setting it to {0},
    since the internal state is persistent over all frames.

    To modify the internal input state you have to first set the zr_input struct
    into a modifiable state with zr_input_begin. After the zr_input struct is
    now ready you can add input state changes until everything is up to date.
    Finally to revert back into a read state you have to call zr_input_end.

    Input function API
    zr_input_begin         -- begins the modification state
    zr_input_motion        -- notifies of a cursor motion update
    zr_input_key           -- notifies of a keyboard key update
    zr_input_button        -- notifies of a action event
    zr_input_char          -- adds a text glyph to zr_input
    zr_input_end           -- ends the modification state

    Input query function API
    zr_input_is_mouse_click_in_rect    - checks for up/down click in a rectangle
    zr_input_is_mouse_hovering_rect    - checks if the mouse hovers over a rectangle
    zr_input_mouse_clicked             - checks if hover + down + clicked in rectangle
    zr_input_is_mouse_down             - checks if the current mouse button is down
    zr_input_is_mouse_released         - checks if mouse button previously released
    zr_input_is_key_pressed            - checks if key was up and now is down
    zr_input_is_key_released           - checks if key was down and is now up
    zr_input_is_key_down               - checks if key is currently down
*/
/* every key that is being used inside the library */
enum zr_keys {
    ZR_KEY_SHIFT,
    ZR_KEY_DEL,
    ZR_KEY_ENTER,
    ZR_KEY_BACKSPACE,
    ZR_KEY_SPACE,
    ZR_KEY_COPY,
    ZR_KEY_CUT,
    ZR_KEY_PASTE,
    ZR_KEY_LEFT,
    ZR_KEY_RIGHT,
    ZR_KEY_MAX
};

/* every used mouse button */
enum zr_buttons {
    ZR_BUTTON_LEFT,
    ZR_BUTTON_MIDDLE,
    ZR_BUTTON_RIGHT,
    ZR_BUTTON_MAX
};

struct zr_mouse_button {
    zr_bool down;
    /* current button state */
    zr_uint clicked;
    /* button state change */
    struct zr_vec2 clicked_pos;
    /* mouse position of last state change */
};

struct zr_mouse {
    struct zr_mouse_button buttons[ZR_BUTTON_MAX];
    /* mouse button states */
    struct zr_vec2 pos;
    /* current mouse position */
    struct zr_vec2 prev;
    /* mouse position in the last frame */
    struct zr_vec2 delta;
    /* mouse travelling distance from last to current frame */
    zr_float scroll_delta;
    /* number of steps in the up or down scroll direction */
};

struct zr_key {zr_bool down; zr_uint clicked;};
struct zr_keyboard {
    struct zr_key keys[ZR_KEY_MAX];
    /* state of every used key */
    zr_char text[ZR_INPUT_MAX];
    /* utf8 text input frame buffer */
    zr_size text_len;
    /* text input frame buffer length in bytes */
};

struct zr_input {
    struct zr_keyboard keyboard;
    /* current keyboard key + text input state */
    struct zr_mouse mouse;
    /* current mouse button and position state */
};

void zr_input_begin(struct zr_input*);
/*  this function sets the input state to writeable */
void zr_input_motion(struct zr_input*, zr_int x, zr_int y);
/*  this function updates the current mouse position
    Input:
    - local os window X position
    - local os window Y position
*/
void zr_input_key(struct zr_input*, enum zr_keys, zr_bool down);
/*  this function updates the current state of a key
    Input:
    - key identifier
    - the new state of the key
*/
void zr_input_button(struct zr_input*, enum zr_buttons, zr_int x, zr_int y, zr_bool down);
/*  this function updates the current state of the button
    Input:
    - mouse button identifier
    - local os window X position
    - local os window Y position
    - the new state of the button
*/
void zr_input_scroll(struct zr_input*, zr_float y);
/*  this function updates the current page scroll delta
    Input:
    - vector with each direction (< 0 down > 0 up and scroll distance)
*/
void zr_input_glyph(struct zr_input*, const zr_glyph);
/*  this function adds a utf-8 glpyh into the internal text frame buffer
    Input:
    - utf8 glyph to add to the text buffer
*/
void zr_input_char(struct zr_input*, char);
/* this function adds char into the internal text frame buffer
    Input:
    - character to add to the text buffer
*/
void zr_input_end(struct zr_input*);
/* this function sets the input state to readable */
zr_bool zr_input_has_mouse_click_in_rect(const struct zr_input*,enum zr_buttons,
                                            struct zr_rect);
/* this function returns true if a mouse click inside a rectangle occured in prev frames */
zr_bool zr_input_has_mouse_click_down_in_rect(const struct zr_input*, enum zr_buttons,
                                                struct zr_rect, zr_bool down);
/*  this function returns true if a mouse down click inside a rectangle occured in prev frames */
zr_bool zr_input_is_mouse_click_in_rect(const struct zr_input*, enum zr_buttons,
                                            struct zr_rect);
/* this function returns true if a mouse click inside a rectangle occured and was just clicked */
zr_bool zr_input_is_mouse_prev_hovering_rect(const struct zr_input*, struct zr_rect);
/*  this function returns true if the mouse hovers over a rectangle */
zr_bool zr_input_is_mouse_hovering_rect(const struct zr_input*, struct zr_rect);
/*  this function returns true if the mouse hovers over a rectangle */
zr_bool zr_input_mouse_clicked(const struct zr_input*, enum zr_buttons, struct zr_rect);
/*  this function returns true if a mouse click inside a rectangle occured
    and the mouse still hovers over the rectangle*/
zr_bool zr_input_is_mouse_down(const struct zr_input*, enum zr_buttons);
/*  this function returns true if the current mouse button is down */
zr_bool zr_input_is_mouse_pressed(const struct zr_input*, enum zr_buttons);
/*  this function returns true if the current mouse button is down and state change*/
zr_bool zr_input_is_mouse_released(const struct zr_input*, enum zr_buttons);
/*  this function returns true if the mouse button was previously pressed but
    was now released */
zr_bool zr_input_is_key_pressed(const struct zr_input*, enum zr_keys);
/*  this function returns true if the given key was up and is now pressed */
zr_bool zr_input_is_key_released(const struct zr_input*, enum zr_keys);
/*  this function returns true if the given key was down and is now up */
zr_bool zr_input_is_key_down(const struct zr_input*, enum zr_keys);
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
    A basic (double)-buffer API with linear allocation and resetting as only
    freeing policy. The buffers main purpose is to control all memory management inside
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
    The user hereby registers callbacks to be called to allocate, free and reallocate
    memory if needed. While this solves most allocation problems it causes some
    loss of flow control on the user side.

    USAGE
    ----------------------------
    To instantiate the buffer you either have to call the fixed size or allocator
    initialization function and provide a memory block in the first case and
    an allocator in the second case.
    To allocate memory from the buffer you would call zr_buffer_alloc with a request
    memory block size as well as an alignment for the block. Finally to reset the memory
    at the end of the frame and when the memory buffer inside the buffer is no longer
    needed you would call zr_buffer_reset. To free all memory that has been allocated
    by an allocator if the buffer is no longer being used you have to call
    zr_buffer_clear.

    Buffer function API
    zr_buffer_init         -- initializes a dynamic buffer
    zr_buffer_init_fixed   -- initializes a static buffer
    zr_buffer_info         -- provides buffer memory information
    zr_buffer_alloc        -- allocates a block of memory from the buffer
    zr_buffer_clear        -- resets the buffer back to an empty state
    zr_buffer_free         -- frees all memory if the buffer is dynamic
    zr_buffer_mark         -- marks the current buffer size for later resetting
    zr_buffer_reset        -- resets the buffer either back to zero or up to marker if set
    zr_buffer_memory       -- returns the internal memory
    zr_buffer_total        -- returns the current total size of the memory
*/
struct zr_memory_status {
    void *memory;
    /* pointer to the currently used memory block inside the referenced buffer */
    zr_uint type;
    /* type of the buffer which is either fixed size or dynamic */
    zr_size size;
    /* total size of the memory block */
    zr_size allocated;
    /* allocated amount of memory */
    zr_size needed;
    /* memory size that would have been allocated if enough memory was present */
    zr_size calls;
    /* number of allocation calls referencing this buffer */
};

struct zr_allocator {
    zr_handle userdata;
    /* handle to your own allocator */
    void*(*alloc)(zr_handle, zr_size);
    /* allocation function pointer */
    /* reallocation pointer of a previously allocated memory block */
    void(*free)(zr_handle, void*);
    /* callback function pointer to finally free all allocated memory */
};

enum zr_buffer_type {
    ZR_BUFFER_FIXED,
    /* fixed size memory buffer */
    ZR_BUFFER_DYNAMIC
    /* dynamically growing buffer */
};

enum zr_buffer_allocation_type {
    ZR_BUFFER_FRONT,
    /* allocate memory from the front of the buffer */
    ZR_BUFFER_BACK,
    /* allocate memory from the back of the buffer */
    ZR_BUFFER_MAX
};

struct zr_buffer_marker {
    zr_bool active;
    /* flag indiciation if the marker was set */
    zr_size offset;
    /* offset of the marker inside the buffer */
};

struct zr_memory {void *ptr;zr_size size;};
struct zr_buffer {
    struct zr_buffer_marker marker[ZR_BUFFER_MAX];
    /* buffer marker to free a buffer to a certain offset */
    struct zr_allocator pool;
    /* allocator callback for dynamic buffers */
    enum zr_buffer_type type;
    /* memory type management type */
    struct zr_memory memory;
    /* memory and size of the current memory block */
    zr_float grow_factor;
    /* growing factor for dynamic memory management */
    zr_size allocated;
    /* total amount of memory allocated */
    zr_size needed;
    /* total amount of memory allocated if enough memory would have been present */
    zr_size calls;
    /* number of allocation calls */
    zr_size size;
    /* current size of the buffer */
};

void zr_buffer_init(struct zr_buffer*, const struct zr_allocator*,
                        zr_size initial_size, zr_float grow_factor);
/*  this function initializes a growing buffer
    Input:
    - allocator holding your own alloctator and memory allocation callbacks
    - initial size of the buffer
    - factor to grow the buffer by if the buffer is full
    Output:
    - dynamically growing buffer
*/
void zr_buffer_init_fixed(struct zr_buffer*, void *memory, zr_size size);
/*  this function initializes a fixed sized buffer
    Input:
    - fixed size previously allocated memory block
    - size of the memory block
    Output:
    - fixed size buffer
*/
void zr_buffer_info(struct zr_memory_status*, struct zr_buffer*);
/*  this function requests memory information from a buffer
    Input:
    - buffer to get the inforamtion from
    Output:
    - buffer memory information
*/
zr_bool zr_buffer_push(struct zr_buffer*,enum zr_buffer_allocation_type type,
                        void *data, zr_size size, zr_size align);
/*  this functions allocates a aligned memory block from a buffer and fill
    the block with data
    Input:
    - buffer to allocate memory from
    - size of the requested memory block
    - alignment requirement for the memory block
    Output:
    - 'zr_true' if succesfully pushed 'zr_false' otherwise
*/
void zr_buffer_mark(struct zr_buffer*, enum zr_buffer_allocation_type);
/* sets a marker either for the back or front buffer for later resetting */
void zr_buffer_reset(struct zr_buffer*, enum zr_buffer_allocation_type);
/* resets the buffer back to the previously set marker or if not set the begining */
void zr_buffer_clear(struct zr_buffer*);
/*  this functions resets the buffer back into an empty state */
void zr_buffer_free(struct zr_buffer*);
/*  this functions frees all memory inside a dynamically growing buffer */
void *zr_buffer_memory(struct zr_buffer*);
/* returns the memory inside the buffer */
zr_size zr_buffer_total(struct zr_buffer*);
/* returns the total size of the buffer */
/*
 * ==============================================================
 *
 *                      Command Buffer
 *
 * ===============================================================
 */
/*  COMMAND BUFFER
    ----------------------------
    The command buffer API queues draw calls as commands into a buffer and
    therefore abstracts over drawing routines and enables defered drawing.
    The API offers a number of drawing primitives like lines, rectangles, circles,
    triangles, images, text and clipping rectangles, that have to be drawn by the user.
    Therefore the command buffer is the main toolkit output besides the actual
    widget output.
    The actual draw command execution is done by the user and is build up in a
    interpreter like fashion by iterating over all commands and executing each
    command depending on the command type.

    USAGE
    ----------------------------
    To use the command buffer you first have to initiate the command buffer with a
    buffer. After the initilization you can add primitives by
    calling the appropriate zr_command_buffer_XXX for each primitive.
    To iterate over each commands inside the buffer zr_foreach_buffer_command is
    provided. Finally to reuse the buffer after the frame use the
    zr_command_buffer_reset function. If used without a command queue the command
    buffer has be cleared after each frame to reset the buffer back into a
    empty state.

    command buffer function API
    zr_command_buffer_init         -- initializes a command buffer with a buffer
    zr_command_buffer_clear        -- resets the command buffer and internal buffer
    zr_command_buffer_reset        -- resets the command buffer but not the buffer

    command queue function API
    zr_command_buffer_push         -- pushes and enqueues a command into the buffer
    zr_command_buffer_push_scissor -- pushes a clipping rectangle into the command buffer
    zr_command_buffer_push_line    -- pushes a line into the command buffer
    zr_command_buffer_push_rect    -- pushes a rectange into the command buffer
    zr_command_buffer_push_circle  -- pushes a circle into the command buffer
    zr_command_buffer_push_triangle-- pushes a triangle command into the buffer
    zr_command_buffer_push_cruve   -- pushes a bezier cruve command into the buffer
    zr_command_buffer_push_image   -- pushes a image draw command into the buffer
    zr_command_buffer_push_text    -- pushes a text draw command into the buffer

    command iterator function API
    zr_command_buffer_begin        -- returns the first command in a buffer
    zr_command_buffer_next         -- returns the next command in a buffer
    zr_foreach_buffer_command      -- iterates over all commands in a buffer
*/
/* command type of every used drawing primitive */
enum zr_command_type {
    ZR_COMMAND_NOP,
    ZR_COMMAND_SCISSOR,
    ZR_COMMAND_LINE,
    ZR_COMMAND_CURVE,
    ZR_COMMAND_RECT,
    ZR_COMMAND_CIRCLE,
    ZR_COMMAND_ARC,
    ZR_COMMAND_TRIANGLE,
    ZR_COMMAND_TEXT,
    ZR_COMMAND_IMAGE
};

/* command base and header of every comand inside the buffer */
struct zr_command {
    enum zr_command_type type;
    /* the type of the current command */
    zr_size next;
    /* absolute base pointer offset to the next command */
};

struct zr_command_scissor {
    struct zr_command header;
    zr_short x, y;
    zr_ushort w, h;
};

struct zr_command_line {
    struct zr_command header;
    struct zr_vec2i begin;
    struct zr_vec2i end;
    struct zr_color color;
};

struct zr_command_curve {
    struct zr_command header;
    struct zr_vec2i begin;
    struct zr_vec2i end;
    struct zr_vec2i ctrl[2];
    struct zr_color color;
};

struct zr_command_rect {
    struct zr_command header;
    zr_uint rounding;
    zr_short x, y;
    zr_ushort w, h;
    struct zr_color color;
};

struct zr_command_circle {
    struct zr_command header;
    zr_short x, y;
    zr_ushort w, h;
    struct zr_color color;
};

struct zr_command_arc {
    struct zr_command header;
    zr_short cx, cy;
    zr_ushort r;
    zr_float a[2];
    struct zr_color color;
};

struct zr_command_triangle {
    struct zr_command header;
    struct zr_vec2i a;
    struct zr_vec2i b;
    struct zr_vec2i c;
    struct zr_color color;
};

struct zr_command_image {
    struct zr_command header;
    zr_short x, y;
    zr_ushort w, h;
    struct zr_image img;
};

struct zr_command_text {
    struct zr_command header;
    const struct zr_user_font *font;
    struct zr_color background;
    struct zr_color foreground;
    zr_short x, y;
    zr_ushort w, h;
    zr_size length;
    zr_char string[1];
};

enum zr_command_clipping {
    ZR_NOCLIP = zr_false,
    ZR_CLIP = zr_true
};

struct zr_command_stats {
    zr_uint lines;
    /* number of lines inside the buffer */
    zr_uint rectangles;
    /* number of rectangles in the buffer */
    zr_uint circles;
    /* number of circles in the buffer */
    zr_uint triangles;
    /* number of triangles in the buffer */
    zr_uint images;
    /* number of images in the buffer */
    zr_uint text;
    /* number of text commands in the buffer */
    zr_uint glyphes;
    /* number of text glyphes in the buffer */
};

struct zr_command_queue;
struct zr_command_buffer {
    struct zr_buffer *base;
    /* memory buffer to store the command */
    struct zr_rect clip;
    /* current clipping rectangle */
    zr_bool use_clipping;
    /* flag if the command buffer should clip commands */
    struct zr_command_stats stats;
    /* stats about the content of the buffer */
    struct zr_command_queue *queue;
    struct zr_command_buffer *next;
    struct zr_command_buffer *prev;
    zr_size begin, end, last;
    /* INTERNAL: references into a command queue */
};

void zr_command_buffer_init(struct zr_command_buffer*, struct zr_buffer*,
                                enum zr_command_clipping);
/*  this function intializes the command buffer
    Input:
    - memory buffer to store the commands into
    - clipping flag for removing non-visible draw commands
*/
void zr_command_buffer_reset(struct zr_command_buffer*);
/*  this function clears the command buffer but not the internal memory buffer */
void zr_command_buffer_clear(struct zr_command_buffer*);
/*  this function clears the command buffer and internal memory buffer */
void *zr_command_buffer_push(struct zr_command_buffer*, zr_uint type, zr_size size);
/*  this function push enqueues a command into the buffer
    Input:
    - buffer to push the command into
    - type of the command
    - amount of memory that is needed for the specified command
*/
void zr_command_buffer_push_scissor(struct zr_command_buffer*, struct zr_rect);
/*  this function push a clip rectangle command into the buffer
    Input:
    - buffer to push the clip rectangle command into
    - x,y and width and height of the clip rectangle
*/
void zr_command_buffer_push_line(struct zr_command_buffer*, zr_float, zr_float,
                                    zr_float, zr_float, struct zr_color);
/*  this function pushes a line draw command into the buffer
    Input:
    - buffer to push the clip rectangle command into
    - starting point of the line
    - ending point of the line
    - color of the line to draw
*/
void zr_command_buffer_push_curve(struct zr_command_buffer*, zr_float, zr_float,
                                    zr_float, zr_float,  zr_float, zr_float,
                                    zr_float, zr_float, struct zr_color);
/*  this function pushes a quad bezier line draw command into the buffer
    Input:
    - buffer to push the clip rectangle command into
    - starting point (x,y) of the line
    - first control point (x,y) of the line
    - second control point (x,y) of the line
    - ending point (x,y) of the line
    - color of the line to draw
*/
void zr_command_buffer_push_rect(struct zr_command_buffer*, struct zr_rect,
                                zr_float rounding, struct zr_color color);
/*  this function pushes a rectangle draw command into the buffer
    Input:
    - buffer to push the draw rectangle command into
    - rectangle bounds
    - rectangle edge rounding
    - color of the rectangle to draw
*/
void zr_command_buffer_push_circle(struct zr_command_buffer*, struct zr_rect,
                                    struct zr_color c);
/*  this function pushes a circle draw command into the buffer
    Input:
    - buffer to push the circle draw command into
    - rectangle bounds of the circle
    - color of the circle to draw
*/
void zr_command_buffer_push_arc(struct zr_command_buffer*, zr_float cx,
                                zr_float cy, zr_float radius, zr_float a_min,
                                zr_float a_max, struct zr_color);
/*  this function pushes an arc draw command into the buffer
    Input:
    - buffer to push the circle draw command into
    - center position (x,y) of the arc
    - start and end angle of the arc
    - color of the arc
*/
void zr_command_buffer_push_triangle(struct zr_command_buffer*, zr_float, zr_float,
                                        zr_float, zr_float, zr_float, zr_float,
                                        struct zr_color);
/*  this function pushes a triangle draw command into the buffer
    Input:
    - buffer to push the draw triangle command into
    - (x,y) coordinates of all three points
    - rectangle diameter of the circle
    - color of the triangle to draw
*/
void zr_command_buffer_push_image(struct zr_command_buffer*, struct zr_rect,
                                    struct zr_image*);
/*  this function pushes a image draw command into the buffer
    Input:
    - buffer to push the draw image command into
    - bounds of the image to draw with position, width and height
    - rectangle diameter of the circle
    - color of the triangle to draw
*/
void zr_command_buffer_push_text(struct zr_command_buffer*, struct zr_rect,
                                    const zr_char*, zr_size, const struct zr_user_font*,
                                    struct zr_color, struct zr_color);
/*  this function pushes a text draw command into the buffer
    Input:
    - buffer to push the draw text command into
    - top left position of the text with x,y position
    - maixmal size of the text to draw with width and height
    - color of the triangle to draw
*/
#define zr_command(t, c) ((const struct zr_command_##t*)c)
/*  this is a small helper makro to cast from a general to a specific command */
#define zr_foreach_buffer_command(i, b)\
    for((i)=zr_command_buffer_begin(b); (i)!=NULL; (i)=zr_command_buffer_next(b,i))
/*  this loop header allow to iterate over each command in a command buffer */
const struct zr_command *zr_command_buffer_begin(struct zr_command_buffer*);
/*  this function returns the first command in the command buffer */
const struct zr_command *zr_command_buffer_next(struct zr_command_buffer*,
                                                struct zr_command*);
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
    to iterate over one command list. Therefore it is possible to have multiple
    windows without having to manage each windows individual memory. This greatly
    simplifies and reduces the amount of code needed with just using command buffers.

    Internally the command queue has a list of command buffers which can be
    modified to create a certain sequence, for example the `zr_begin`
    function changes the list to create overlapping windows, while `zr_begin_tiled`
    always draws all its windows in the background by pushing its buffers to the
    beginning of the list.

    USAGE
    ----------------------------
    The command queue owns a memory buffer internaly that needs to be initialized
    either as a fixed size or dynamic buffer with functions `zr_commmand_queue_init'
    or `zr_command_queue_init_fixed`. Windows are automaticall added to the command
    queue in the `zr_window_init` with the `zr_command_queue_add` function
    but removing a window requires a manual call of `zr_command_queue_remove`.
    Internally the window calls the `zr_command_queue_start` and
    `zr_commanmd_queue_finish` function that setup and finilize a command buffer for
    command queuing. Finally to iterate over all commands in all command buffers
    the iterator API is provided. It allows to iterate over each command in a
    foreach loop.

    command queue function API
    zr_command_queue_init          -- initializes a dynamic command queue
    zr_command_queue_init_fixed    -- initializes a static command queue
    zr_command_queue_clear         -- frees all memory if the command queue is dynamic
    zr_command_queue_insert_font   -- adds a command buffer in the front of the queue
    zr_command_queue_insert_back   -- adds a command buffer in the back of the queue
    zr_command_queue_remove        -- removes a command buffer from the queue
    zr_command_queue_start         -- begins the command buffer filling process
    zr_command_queue_start_child   -- begins the child command buffer filling process
    zr_command_queue_finish_child  -- ends the child command buffer filling process
    zr_command_queue_finish        -- ends the command buffer filling process

    command iterator function API
    zr_command_queue_begin         -- returns the first command in a queue
    zr_command_queue_next          -- returns the next command in a queue or NULL
    zr_foreach_command             -- iterates over all commands in a queue
*/
struct zr_command_buffer_list {
    zr_size count;
    /* number of windows inside the stack */
    struct zr_command_buffer *begin;
    /* first window inside the window which will be drawn first */
    struct zr_command_buffer *end;
    /* currently active window which will be drawn last */
};

struct zr_command_sub_buffer {
    zr_size begin;
    /* begin of the subbuffer */
    zr_size parent_last;
    /* last entry before the sub buffer*/
    zr_size last;
    /* last entry in the sub buffer*/
    zr_size end;
    /* end of the subbuffer */
    zr_size next;
};

struct zr_command_sub_buffer_stack {
    zr_size count;
    /* number of subbuffers */
    zr_size begin;
    /* buffer offset of the first subbuffer*/
    zr_size end;
    /* buffer offset of the last subbuffer*/
    zr_size size;
    /* real size of the buffer */
};

struct zr_command_queue {
    struct zr_buffer buffer;
    /* memory buffer the hold all commands */
    struct zr_command_buffer_list list;
    /* list of each memory buffer inside the queue */
    struct zr_command_sub_buffer_stack stack;
    /* subbuffer stack for overlapping popup windows in windows */
    zr_bool build;
    /* flag indicating if a complete command list was build inside the queue*/
};

void zr_command_queue_init(struct zr_command_queue*, const struct zr_allocator*,
                            zr_size initial_size, zr_float grow_factor);
/*  this function initializes a growing command queue
    Input:
    - allocator holding your own alloctator and memory allocation callbacks
    - initial size of the buffer
    - factor to grow the buffer by if the buffer is full
*/
void zr_command_queue_init_fixed(struct zr_command_queue*, void*, zr_size);
/*  this function initializes a fixed size command queue
    Input:
    - fixed size previously allocated memory block
    - size of the memory block
*/
void zr_command_queue_insert_back(struct zr_command_queue*, struct zr_command_buffer*);
/*  this function adds a command buffer into the back of the queue
    Input:
    - command buffer to add into the queue
*/
void zr_command_queue_insert_front(struct zr_command_queue*, struct zr_command_buffer*);
/*  this function adds a command buffer into the beginning of the queue
    Input:
    - command buffer to add into the queue
*/
void zr_command_queue_remove(struct zr_command_queue*, struct zr_command_buffer*);
/*  this function removes a command buffer from the queue
    Input:
    - command buffer to remove from the queue
*/
void zr_command_queue_start(struct zr_command_queue*, struct zr_command_buffer*);
/*  this function sets up the command buffer to be filled up
    Input:
    - command buffer to fill with commands
*/
void zr_command_queue_finish(struct zr_command_queue*, struct zr_command_buffer*);
/*  this function finishes the command buffer fill up process
    Input:
    - the now filled command buffer
*/
zr_bool zr_command_queue_start_child(struct zr_command_queue*,
                                        struct zr_command_buffer*);
/*  this function sets up a child buffer inside a command buffer to be filled up
    Input:
    - command buffer to begin the child buffer in
    Output:
    - zr_true if successful zr_false otherwise
*/
void zr_command_queue_finish_child(struct zr_command_queue*, struct zr_command_buffer*);
/*  this function finishes the child buffer inside the command buffer fill up process
    Input:
    - the command buffer to create the child command buffer in
*/
void zr_command_queue_free(struct zr_command_queue*);
/*  this function clears the internal buffer if it is a dynamic buffer */
void zr_command_queue_clear(struct zr_command_queue*);
/*  this function reset the internal buffer and has to be called every frame */
#define zr_foreach_command(i, q)\
    for((i)=zr_command_queue_begin(q); (i)!=0; (i)=zr_command_queue_next(q,i))
/*  this function iterates over each command inside the command queue
    Input:
    - iterator zr_command pointer to iterate over all commands
    - queue to iterate over
*/
void zr_command_queue_build(struct zr_command_queue*);
/*  this function builds the internal queue commmand list out of all buffers.
 *  Only needs be called if zr_command_queue_begin is called in parallel */
const struct zr_command *zr_command_queue_begin(struct zr_command_queue*);
/*  this function returns the first command in the command queue */
const struct zr_command* zr_command_queue_next(struct zr_command_queue*,
                                                const struct zr_command*);
/*  this function returns the next command of a given command*/
/*
 * ===============================================================
 *
 *                          Draw List
 *
 * ===============================================================
 */
#if ZR_COMPILE_WITH_VERTEX_BUFFER
/*  DRAW LIST
    ----------------------------
    The optional draw list provides a way to convert the gui library
    drawing command output into a hardware accessible format.
    This consists of vertex, element and draw batch command buffer output which
    can be interpreted by render frameworks (OpenGL, DirectX, ...).
    In addition to just provide a way to convert commands the draw list has
    a primitives and stateful path drawing API, which allows to draw into the
    draw list even with anti-aliasing.

    The draw list consist internaly of three user provided buffers that will be
    filled with data. The first buffer is the the draw command and temporary
    path buffer which permanetly stores all draw batch commands. The vertex and
    element buffer are the same buffers as their hardware renderer counter-parts,
    in fact it is even possible to directly map one of these buffers and fill
    them with data.

    The reason why the draw list is optional or is not the default library output
    is that basic commands provide an easy way to abstract over other libraries
    which already provide a drawing API and do not need or want the output the
    draw list provides. In addition it is way easier and takes less memory
    to serialize commands decribing primitives than vertex data.

    USAGE
    ----------------------------
    To actually use the draw list you first need the initialize the draw list
    by providing three buffers to be filled while drawing. The reason
    buffers need to be provided and not memory or an allocator is to provide
    more fine grained control over the memory inside the draw list, which in term
    requires more work from the user.

    After the draw list has been initialized you can fill the draw list by
    a.) converting the content of a command queue into the draw list format
    b.) adding primtive filled shapes or only the outline of rectangles, circle, etc.
    c.) using the stateful path API for fine grained drawing control

    Finaly for the drawing process you have to iterate over each draw command
    inside the `zr_draw_list` by using the function `zr_foreach_draw_command`
    which contains drawing state like clip rectangle, current texture and a number
    of elements to draw with the current state.

    draw list buffer functions
    zr_draw_list_init              - initializes a command buffer with memory
    zr_draw_list_clear             - clears and resets the buffer
    zr_draw_list_load              - load the draw buffer from a command queue
    zr_draw_list_begin             - returns the first command in the draw buffer
    zr_draw_list_next              - returns the next command in the draw buffer or NULL
    zr_draw_list_is_empty          - returns if the buffer has no vertexes or commands
    zr_foreach_draw_command        - iterates over all commands in the draw buffer

    draw list primitives drawing functions
    zr_draw_list_add_line          - pushes a line into the draw list
    zr_draw_list_add_rect          - pushes a rectangle into the draw list
    zr_draw_list_add_rect_filled   - pushes a filled rectangle into the draw list
    zr_draw_list_add_triangle      - pushes a triangle into the draw list
    zr_draw_list_add_triangle_filled -pushes a filled triangle into the draw list
    zr_draw_list_add_circle        - pushes circle into the draw list
    zr_draw_list_add_circle_filled - pushes a filled circle into the draw list
    zr_draw_list_add_text          - pushes text into the draw list
    zr_draw_list_add_image         - pushes an image into the draw list

    stateful path functions
    zr_draw_list_path_clear        - resets the current path
    zr_draw_list_path_line_to      - adds a point into the path
    zr_draw_list_path_arc_to       - adds a arc into the path
    zr_draw_list_path_curve_to     - adds a bezier curve into the path
    zr_draw_list_path_rect_to      - adds a rectangle into the path
    zr_draw_list_path_fill         - fills the path as a convex polygon
    zr_draw_list_path_stroke       - connects each point in the path
*/
typedef zr_ushort zr_draw_index;
typedef zr_uint zr_draw_vertex_color;
typedef zr_float(*zr_sin_f)(zr_float);
typedef zr_float(*zr_cos_f)(zr_float);

enum zr_anti_aliasing {
    ZR_ANTI_ALIASING_OFF = zr_false,
    /* renderes all primitives without anit-aliasing  */
    ZR_ANTI_ALIASING_ON
    /* renderes all primitives with anit-aliasing  */
};

struct zr_draw_vertex {
    struct zr_vec2 position;
    struct zr_vec2 uv;
    zr_draw_vertex_color col;
};

enum zr_draw_list_stroke {
    ZR_STROKE_OPEN = zr_false,
    /* build up path has no connection back to the beginning */
    ZR_STROKE_CLOSED = zr_true
    /* build up path has a connection back to the beginning */
};

struct zr_draw_command {
    zr_uint elem_count;
    /* number of elements in the current draw batch */
    struct zr_rect clip_rect;
    /* current screen clipping rectangle */
    zr_handle texture;
    /* current texture to set */
};

struct zr_draw_null_texture {
    zr_handle texture;
    /* texture handle to a texture with a white pixel */
    struct zr_vec2 uv;
    /* coordinates to the white pixel in the texture  */
};

struct zr_draw_list {
    enum zr_anti_aliasing AA;
    /* flag indicating if anti-aliasing should be used to render primtives */
    struct zr_draw_null_texture null;
    /* texture with white pixel for easy primitive drawing */
    struct zr_rect clip_rect;
    /* current clipping rectangle */
    zr_cos_f cos; zr_sin_f sin;
    /* cosine/sine calculation callback since this library does not use libc  */
    struct zr_buffer *buffer;
    /* buffer to store draw commands and temporarily store path */
    struct zr_buffer *vertexes;
    /* buffer to store each draw vertex */
    struct zr_buffer *elements;
    /* buffer to store each draw element index */
    zr_uint element_count;
    /* total number of elements inside the elements buffer */
    zr_uint vertex_count;
    /* total number of vertexes inside the vertex buffer */
    zr_size cmd_offset;
    /* offset to the first command in the buffer */
    zr_uint cmd_count;
    /* number of commands inside the buffer  */
    zr_uint path_count;
    /* current number of points inside the path */
    zr_uint path_offset;
    /* offset to the first point in the buffer */
};
/* ---------------------------------------------------------------
 *                          MAIN
 * ---------------------------------------------------------------*/
void zr_draw_list_init(struct zr_draw_list*, struct zr_buffer *cmds,
                        struct zr_buffer *vertexes, struct zr_buffer *elements,
                        zr_sin_f, zr_cos_f, struct zr_draw_null_texture,
                        enum zr_anti_aliasing AA);
/*  this function initializes the draw list
    Input:
    - command memory buffer to fill with commands and provided temporary memory for path
    - vertex memory buffer to fill with draw vertexeses
    - element memory buffer to fill with draw indexes
    - sine function callback since this library does not use clib (default: just use sinf)
    - cosine function callback since this library does not use clib (default: just use cosf)
    - special null structure which holds a texture with a white pixel
    - Anti-aliasing flag for turning on/off anti-aliased primitives drawing
*/
void zr_draw_list_load(struct zr_draw_list*, struct zr_command_queue *queue,
                        zr_float line_thickness, zr_uint curve_segments);
/*  this function loads the draw list from command queue commands
    Input:
    - queue with draw commands to fill the draw list with
    - line thickness for all lines and non-filled primitives
    - number of segments used for drawing circles and curves
*/
void zr_draw_list_clear(struct zr_draw_list *list);
/*  this function clears all buffers inside the draw list */
zr_bool zr_draw_list_is_empty(struct zr_draw_list *list);
/*  this function returns if the draw list is empty */
#define zr_foreach_draw_command(i, q)\
    for((i)=zr_draw_list_begin(q); (i)!=NULL; (i)=zr_draw_list_next(q,i))
/*  this function iterates over each draw command inside the draw list
    Input:
    - iterator zr_draw_command pointer to iterate over all commands
    - draw list to iterate over
*/
const struct zr_draw_command *zr_draw_list_begin(const struct zr_draw_list *list);
/*  this function returns the first draw command in the draw list */
const struct zr_draw_command* zr_draw_list_next(const struct zr_draw_list *list,
                                                    const struct zr_draw_command*);
/*  this function returns the next draw command of a given draw command*/
/* ---------------------------------------------------------------
 *                          PRIMITIVES
 * ---------------------------------------------------------------*/
void zr_draw_list_add_clip(struct zr_draw_list*, struct zr_rect);
/*  this function pushes a new clipping rectangle into the draw list
    Input:
    - new clipping rectangle
*/
void zr_draw_list_add_line(struct zr_draw_list*, struct zr_vec2 a,
                            struct zr_vec2 b, struct zr_color col,
                            zr_float thickness);
/*  this function pushes a new clipping rectangle into the draw list
    Input:
    - beginning point of the line
    - end point of the line
    - used line color
    - line thickness in pixel
*/
void zr_draw_list_add_rect(struct zr_draw_list*, struct zr_rect rect,
                            struct zr_color col, zr_float rounding,
                            zr_float line_thickness);
/*  this function pushes a rectangle into the draw list
    Input:
    - rectangle to render into the draw list
    - rectangle outline color
    - rectangle edge rounding (0 == no edge rounding)
    - rectangle outline thickness
*/
void zr_draw_list_add_rect_filled(struct zr_draw_list*, struct zr_rect rect,
                                    struct zr_color col, zr_float rounding);
/*  this function pushes a filled rectangle into the draw list
    Input:
    - rectangle to render into the draw list
    - color to fill the rectangle with
    - rectangle edge rounding (0 == no edge rounding)
*/
void zr_draw_list_add_triangle(struct zr_draw_list*, struct zr_vec2 a,
                                struct zr_vec2 b, struct zr_vec2 c,
                                struct zr_color, zr_float line_thickness);
/*  this function pushes a triangle into the draw list
    Input:
    - first point of the triangle
    - second point of the triangle
    - third point of the triangle
    - color of the triangle outline
    - triangle outline line thickness
*/
void zr_draw_list_add_triangle_filled(struct zr_draw_list*, struct zr_vec2 a,
                                        struct zr_vec2 b, struct zr_vec2 c,
                                        struct zr_color col);
/*  this function pushes a filled triangle into the draw list
    Input:
    - first point of the triangle
    - second point of the triangle
    - third point of the triangle
    - color to fill the triangle with
*/
void zr_draw_list_add_circle(struct zr_draw_list*, struct zr_vec2 center,
                            zr_float radius, struct zr_color col, zr_uint segs,
                            zr_float line_thickness);
/*  this function pushes a circle outline into the draw list
    Input:
    - center point of the circle
    - circle radius
    - circle outlone color
    - number of segement that make up the circle
*/
void zr_draw_list_add_circle_filled(struct zr_draw_list*, struct zr_vec2 center,
                                    zr_float radius, struct zr_color col, zr_uint);
/*  this function pushes a filled circle into the draw list
    Input:
    - center point of the circle
    - circle radius
    - color to fill the circle with
    - number of segement that make up the circle
*/
void zr_draw_list_add_curve(struct zr_draw_list*, struct zr_vec2 p0,
                            struct zr_vec2 cp0, struct zr_vec2 cp1, struct zr_vec2 p1,
                            struct zr_color col, zr_uint segments, zr_float thickness);
/*  this function pushes a bezier curve into the draw list
    Input:
    - beginning point of the curve
    - first curve control point
    - second curve control point
    - end point of the curve
    - color of the curve
    - number of segement that make up the circle
    - line thickness of the curve
*/
void zr_draw_list_add_text(struct zr_draw_list*, const struct zr_user_font*,
                            struct zr_rect, const zr_char*, zr_size length,
                            struct zr_color bg, struct zr_color fg);
/*  this function renders text
    Input:
    - user font to draw the text with
    - the rectangle the text will be drawn into
    - string text to draw
    - length of the string to draw
    - text background color
    - text color
*/
void zr_draw_list_add_image(struct zr_draw_list *list, struct zr_image texture,
                            struct zr_rect rect, struct zr_color color);
/*  this function renders an image
    Input:
    - image texture handle to draw
    - rectangle with position and size of the image
    - color to blend with the image
*/
/* ---------------------------------------------------------------
 *                          PATH
 * ---------------------------------------------------------------*/
void zr_draw_list_path_clear(struct zr_draw_list*);
/* clears the statefull drawing path previously build */
void zr_draw_list_path_line_to(struct zr_draw_list*, struct zr_vec2 pos);
/* adds a point into the path that will make up a line or a convex polygon */
void zr_draw_list_path_arc_to(struct zr_draw_list*, struct zr_vec2 center,
                                zr_float radius, zr_float a_min, zr_float a_max,
                                zr_uint segments);
/* adds an arc made up of a number of segments into the path */
void zr_draw_list_path_rect_to(struct zr_draw_list*, struct zr_vec2 a,
                                struct zr_vec2 b, zr_float rounding);
/* adds a rectangle into the path */
void zr_draw_list_path_curve_to(struct zr_draw_list*, struct zr_vec2 p1,
                                struct zr_vec2 p2, struct zr_vec2 p3,
                                zr_uint num_segments);
/* adds a bezier curve into the path where the first point has to be already in the path */
void zr_draw_list_path_fill(struct zr_draw_list*, struct zr_color);
/* uses all points inside the path to draw a convex polygon  */
void zr_draw_list_path_stroke(struct zr_draw_list*, struct zr_color,
                                zr_bool closed, zr_float thickness);
/* uses all points inside the path to draw a line/outline */
#endif
/*
 * ==============================================================
 *
 *                          Font
 *
 * ===============================================================
 */
/*  FONT
    ----------------------------
    Font handling in this library can be achived in three different ways.
    The first and simplest ways is by just using your font handling mechanism
    and provide a simple callback for text string width calculation with
    `zr_user_font`. This requires the default drawing output
    and is not possible for the optional vertex buffer output.

    The second way of font handling is by using the same `zr_user_font` struct
    to referencing a font as before but providing a second callback for
    `zr_user_font_glyph` querying which is used for text drawing in the optional vertex
    buffer output. In addition to the callback it is also required to provide
    a texture atlas from the font to draw.

    The final and most complex way is to use the optional font baker
    and font handling function, which requires two additional headers for
    TTF font baking. While the previous two methods did no need any functions
    outside callbacks and are therefore rather simple to handle, the final
    font handling method quite complex and you need to handle the complex
    font baking API. The reason why it is complex is because there are multible
    ways of using the API. For example it must be possible to use the font
    for default command output as well as vertex buffer output. So for example
    texture coordinates can either be UV for vertex buffer output or absolute pixel
    for drawing function based on pixels. Furthermore it is possible to incoperate
    custom user data into the resulting baked image (for example a white pixel for the
    vertex buffer output). In addition and probably the most complex aspect of
    the baking API was to incoperate baking of multible fonts into one image.

    In general the font baking API can be understood as having a number of
    loaded in memory TTF-fonts, font baking configuration and optional custom
    render data as input, while the output is made of font specific data, a big
    glyph array of all baked glyphes and the baked image. The API
    was designed that way to have a typical file format and not
    a perfectly ready in memory library instance of a font. The reason is more
    control and seperates the font baking code from the in library used font format.

    USAGE
    ----------------------------
    font baking functions
    zr_font_bake_memory            -- calculates the needed font baking memory
    zr_font_bake_pack              -- packs all glyphes and calculates needed image memory
    zr_font_bake                   -- renders all glyphes inside an image and setups glyphes
    zr_font_bake_custom_data       -- renders custom user data into the image
    zr_font_bake_convert           -- converts the baked image from alpha8 to rgba8

    font functions
    zr_font_init                   -- initilizes the font
    zr_font_ref                    -- create a user font out of the font
    zr_font_find_glyph             -- finds and returns a glyph from the font
*/
typedef zr_size(*zr_text_width_f)(zr_handle, const zr_char*, zr_size);
typedef void(*zr_query_font_glyph_f)(zr_handle, struct zr_user_font_glyph*,
        zr_long codepoint, zr_long next_codepoint);

#if ZR_COMPILE_WITH_VERTEX_BUFFER
struct zr_user_font_glyph {
    struct zr_vec2 uv[2];
    /* texture coordinates */
    struct zr_vec2 offset;
    /* offset between top left and glyph */
    zr_float width, height;
    /* size of the glyph  */
    zr_float xadvance;
    /* offset to the next glyph */
};
#endif

struct zr_user_font {
    zr_handle userdata;
    /* user provided font handle */
    zr_float height;
    /* max height of the font */
    zr_text_width_f width;
    /* font string width in pixel callback */
#if ZR_COMPILE_WITH_VERTEX_BUFFER
    zr_query_font_glyph_f query;
    /* font glyph callback to query drawing info */
    zr_handle texture;
    /* texture handle to the used font atlas or texture */
#endif
};

#ifdef ZR_COMPILE_WITH_FONT
enum zr_font_coord_type {
    ZR_COORD_UV,
    /* texture coordinates inside font glyphes are clamped between 0.0f and 1.0f */
    ZR_COORD_PIXEL
    /* texture coordinates inside font glyphes are in absolute pixel */
};

struct zr_baked_font {
    zr_float height;
    /* height of the font  */
    zr_float ascent, descent;
    /* font glyphes ascent and descent  */
    zr_long glyph_offset;
    /* glyph array offset inside the font glyph baking output array  */
    zr_long glyph_count;
    /* number of glyphes of this font inside the glyph baking array output */
    const zr_long *ranges;
    /* font codepoint ranges as pairs of (from/to) and 0 as last element */
};

struct zr_font_config {
    void *ttf_blob;
    /* pointer to loaded TTF file memory block */
    zr_size ttf_size;
    /* size of the loaded TTF file memory block */
    zr_float size;
    /* bake pixel height of the font */
    zr_uint oversample_h, oversample_v;
    /* rasterize at hight quality for sub-pixel position */
    zr_bool pixel_snap;
    /* align very character to pixel boundry (if true set oversample (1,1)) */
    enum zr_font_coord_type coord_type;
    /* baked glyph texture coordinate format with either pixel or UV coordinates */
    struct zr_vec2 spacing;
    /* extra pixel spacing between glyphs  */
    const zr_long *range;
    /* list of unicode ranges (2 values per range, zero terminated) */
    struct zr_baked_font *font;
    /* font to setup in the baking process  */
};

struct zr_font_glyph {
    zr_long codepoint;
    /* unicode codepoint */
    zr_float xadvance;
    /* xoffset to the next character  */
    zr_float x0, y0, x1, y1;
    /* glyph bounding points in pixel inside the glyph image with top left and bottom right */
    zr_float u0, v0, u1, v1;
    /* texture coordinates either in pixel or clamped (0.0 - 1.0) */
};

struct zr_font {
    zr_float size;
    /* pixel height of the font */
    zr_float scale;
    /* scale factor for different font size */
    zr_float ascent, descent;
    /* font ascent and descent  */
    struct zr_font_glyph *glyphes;
    /* font glyph array  */
    const struct zr_font_glyph *fallback;
    /* fallback glyph */
    zr_long fallback_codepoint;
    /* fallback glyph codepoint */
    zr_long glyph_count;
    /* font glyph array size */
    const zr_long *ranges;
    /* glyph unicode ranges in the font */
    zr_handle atlas;
    /* font image atlas handle */
};

/* some language glyph codepoint ranges */
const zr_long *zr_font_default_glyph_ranges(void);
const zr_long *zr_font_chinese_glyph_ranges(void);
const zr_long *zr_font_cyrillic_glyph_ranges(void);
const zr_long *zr_font_korean_glyph_ranges(void);

/* ---------------------------------------------------------------
 *                          Baking
 * ---------------------------------------------------------------*/
/* font baking functions (need to be called sequentially top to bottom) */
void zr_font_bake_memory(zr_size *temporary_memory, zr_size *glyph_count,
                            struct zr_font_config*, zr_size count);
/*  this function calculates the needed memory for the baking process
    Input:
    - array of configuration for every font that should be baked into one image
    - number of configuration fonts in the array
    Output:
    - amount of memory needed in the baking process
    - total number of glyphes that need to be allocated
*/
zr_bool zr_font_bake_pack(zr_size *img_memory, zr_size *img_width, zr_size *img_height,
                            struct zr_recti *custom_space,
                            void *temporary_memory, zr_size temporary_size,
                            const struct zr_font_config*, zr_size font_count);
/*  this function packs together all glyphes and optional space into one
    total image space and returns the needed image width and height.
    Input:
    - NULL or custom space inside the image with width and height (will be updated!)
    - temporary memory block that will be used in the baking process
    - size of the temporary memory block
    - array of configuration for every font that should be baked into one image
    - number of configuration fonts in the array
    Output:
    - calculated resulting size of the image in bytes
    - pixel width of the resulting image
    - pixel height of the resulting image
    - custom space bounds with position and size inside image which can be filled by the user
*/
void zr_font_bake(void *image_memory, zr_size image_width, zr_size image_height,
                    void *temporary_memory, zr_size temporary_memory_size,
                    struct zr_font_glyph*, zr_size glyphes_count,
                    const struct zr_font_config*, zr_size font_count);
/*  this function bakes all glyphes into the pre-allocated image and
    fills a glyph array with information.
    Input:
    - image memory buffer to bake the glyph into
    - pixel width/height of the image
    - temporary memory block that will be used in the baking process
    - size of the temporary memory block
    Output:
    - image filled with glyphes
    - filled glyph array
*/
void zr_font_bake_custom_data(void *img_memory, zr_size img_width, zr_size img_height,
                                struct zr_recti img_dst, const char *texture_data_mask,
                                zr_size tex_width, zr_size tex_height,
                                zr_char white, zr_char black);
/*  this function bakes custom data in string format with white, black and zero
    alpha pixels into the font image. The zero alpha pixel is represented as
    any character beside the black and zero pixel character.
    Input:
    - image memory buffer to bake the custom data into
    - image size (width/height) of the image in pixels
    - custom texture data in string format
    - texture size (width/height) of the custom image content
    - character representing a white pixel in the texture data format
    - character representing a black pixel in the texture data format
    Output:
    - image filled with custom texture data
*/
void zr_font_bake_convert(void *out_memory, zr_ushort img_width, zr_ushort img_height,
                            const void *in_memory);
/*  this function converts the alpha8 baking input image into a
    preallocated rgba8 output image.
    Input:
    - image pixel size (width, height)
    - memory block containing the alpha8 image
    Output:
    - rgba8 output image
*/
/* ---------------------------------------------------------------
 *                          Font
 * ---------------------------------------------------------------*/
void zr_font_init(struct zr_font*, zr_float pixel_height,
                    zr_long fallback_codepoint, struct zr_font_glyph*,
                    const struct zr_baked_font*, zr_handle atlas);
/*  this function initializes a font. IMPORTANT: The font only references
 *  its glyphes since it allows to have multible font glyph in one big array.
    Input:
    - pixel height of the font can be different than the baked font height
    - unicode fallback codepoint for a glyph that will be used if a glyph is requested
        that does not exist in the font
    - glyph array of all glyphes inside the font
    - number of glyphes inside the glyph array
    - font information for this font from the baking process
    - handle to the baked font image atlas
*/
struct zr_user_font zr_font_ref(struct zr_font*);
/*  this function
    Output:
    - gui font handle used in the library
*/
const struct zr_font_glyph* zr_font_find_glyph(struct zr_font*, zr_long unicode);
/*  this function
    Input:
    - unicode glyph codepoint of the glyph
    Output:
    - either the glyph with the given codepoint or a fallback glyph if does not exist
*/
#endif
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
    buffer. It extends the basic functionality of basic input over `zr_edit`
    and `zr_edit` with basic copy and paste functionality and the possiblity
    to use a extending buffer.

    USAGE
    ----------------------------
    The Editbox first needs to be initialized either with a fixed size
    memory block or a allocator. After that it can be used by either the
    `zr_editobx` or `zr_editbox` function. In addition symbols can be
    added and removed with `zr_edit_box_add` and `zr_edit_box_remove`.

    Widget function API
    zr_edit_box_init       -- initialize a dynamically growing edit box
    zr_edit_box_init_fixed -- initialize a fixed size edit box
    zr_edit_box_reset      -- resets the edit box back to the beginning
    zr_edit_box_clear      -- frees all memory of a dynamic edit box
    zr_edit_box_add        -- adds a symbol to the editbox
    zr_edit_box_remove     -- removes a symbol from the editbox
    zr_edit_box_get        -- returns the string inside the editbox
    zr_edit_box_get_const  -- returns the const string inside the editbox
    zr_edit_box_free       -- frees all memory in a dynamic editbox
    zr_edit_box_info       -- fills a memory info struct with data
    zr_edit_box_at         -- returns the glyph at the given position
    zr_edit_box_at_cursor  -- returns the glyph at the cursor position
    zr_edit_box_at_char    -- returns the char at the given position
    zr_edit_box_set_cursor -- sets the cursor to a given glyph
    zr_edit_box_get_cursor -- returns the position of the cursor
    zr_edit_box_len_char   -- returns the length of the string in bytes
    zr_edit_box_len        -- returns the length of the string in glyphes
*/
typedef zr_bool(*zr_filter)(zr_long unicode);
typedef void(*zr_paste_f)(zr_handle, struct zr_edit_box*);
typedef void(*zr_copy_f)(zr_handle, const char*, zr_size size);

struct zr_clipboard {
    zr_handle userdata;
    /* user memory for callback */
    zr_paste_f paste;
    /* paste callback for the edit box  */
    zr_copy_f copy;
    /* copy callback for the edit box  */
};

struct zr_selection {
    zr_bool active;
    /* current selection state */
    zr_size begin;
    /* text selection beginning glyph index */
    zr_size end;
    /* text selection ending glyph index */
};

typedef struct zr_buffer zr_edit_buffer;
struct zr_edit_box {
    zr_edit_buffer buffer;
    /* glyph buffer to add text into */
    zr_state active;
    /* flag indicating if the buffer is currently being modified  */
    zr_size cursor;
    /* current glyph (not byte) cursor position */
    zr_size glyphes;
    /* number of glyphes inside the edit box */
    struct zr_clipboard clip;
    /* copy paste callbacks */
    zr_filter filter;
    /* input filter callback */
    struct zr_selection sel;
    /* text selection */
};

/* filter function */
zr_bool zr_filter_default(zr_long unicode);
zr_bool zr_filter_ascii(zr_long unicode);
zr_bool zr_filter_float(zr_long unicode);
zr_bool zr_filter_decimal(zr_long unicode);
zr_bool zr_filter_hex(zr_long unicode);
zr_bool zr_filter_oct(zr_long unicode);
zr_bool zr_filter_binary(zr_long unicode);

/* editbox */
void zr_edit_box_init(struct zr_edit_box*, struct zr_allocator*, zr_size initial,
                        zr_float grow_fac, const struct zr_clipboard*, zr_filter);
/*  this function initializes the editbox a growing buffer
    Input:
    - allocator implementation
    - initital buffer size
    - buffer growing factor
    - clipboard implementation for copy&paste or NULL of not needed
    - character filtering callback to limit input or NULL of not needed
*/
void zr_edit_box_init_fixed(struct zr_edit_box*, void *memory, zr_size size,
                                const struct zr_clipboard*, zr_filter);
/*  this function initializes the editbox a static buffer
    Input:
    - memory block to fill
    - sizeo of the memory block
    - clipboard implementation for copy&paste or NULL of not needed
    - character filtering callback to limit input or NULL of not needed
*/
void zr_edit_box_clear(struct zr_edit_box*);
/*  this function resets the buffer and sets everything back into a clean state */
void zr_edit_box_free(struct zr_edit_box*);
/*  this function frees all internal memory in a dynamically growing buffer */
void zr_edit_box_info(struct zr_memory_status*, struct zr_edit_box*);
/* this function returns information about the memory in use  */
void zr_edit_box_add(struct zr_edit_box*, const char*, zr_size);
/*  this function adds text at the current cursor position
    Input:
    - string buffer or glyph to copy/add to the buffer
    - length of the string buffer or glyph
*/
void zr_edit_box_remove(struct zr_edit_box*);
/*  removes the glyph at the current cursor position */
zr_char *zr_edit_box_get(struct zr_edit_box*);
/* returns the string buffer inside the edit box */
const zr_char *zr_edit_box_get_const(struct zr_edit_box*);
/* returns the constant string buffer inside the edit box */
void zr_edit_box_at(struct zr_edit_box*, zr_size pos, zr_glyph, zr_size*);
/*  this function returns the glyph at a given offset
    Input:
    - glyph offset inside the buffer
    Output:
    - utf8 glyph at the given position
    - byte length of the glyph
*/
void zr_edit_box_at_cursor(struct zr_edit_box*, zr_glyph, zr_size*);
/*  this function returns the glyph at the cursor
    Output:
    - utf8 glyph at the cursor position
    - byte length of the glyph
*/
zr_char zr_edit_box_at_char(struct zr_edit_box*, zr_size pos);
/*  this function returns the character at a given byte position
    Input:
    - character offset inside the buffer
    Output:
    - character at the given position
*/
void zr_edit_box_set_cursor(struct zr_edit_box*, zr_size pos);
/*  this function sets the cursor at a given glyph position
    Input:
    - glyph offset inside the buffer
*/
zr_size zr_edit_box_get_cursor(struct zr_edit_box *eb);
/*  this function returns the cursor glyph position
    Output:
    - cursor glyph offset inside the buffer
*/
zr_size zr_edit_box_len_char(struct zr_edit_box*);
/*  this function returns length of the buffer in bytes
    Output:
    - string buffer byte length
*/
zr_size zr_edit_box_len(struct zr_edit_box*);
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
    be managed by the user. It is the basis for the window API and implements
    the functionality for almost all widgets in the window API. The widget API
    hereby is more flexible than the window API in placing and styling but
    requires more work for the user and has no concept for groups of widgets.

    USAGE
    ----------------------------
    Most widgets take an input struct, font and widget specific data and a command
    buffer to push draw command into and return the updated widget state.
    Important to note is that there is no actual state that is being stored inside
    the toolkit so everything has to be stored byte the user.

    Widget function API
    zr_widget_text                 -- draws a string inside a box
    zr_widget_button_text          -- button widget with text content
    zr_widget_button_image         -- button widget with icon content
    zr_widget_button_triangle      -- button widget with triangle content
    zr_widget_button_text_triangle -- button widget with triangle and text content
    zr_widget_button_text_image    -- button widget with image and text content
    zr_widget_toggle               -- either a checkbox or radiobutton widget
    zr_widget_slider               -- floating point slider widget
    zr_widget_progress             -- unsigned integer progressbar widget
    zr_widget_editbox              -- Editbox widget for complex user input
    zr_widget_edit                 -- Editbox wiget for basic user input
    zr_widget_edit_filtered        -- Editbox with utf8 gylph filter capabilities
    zr_widget_spinner              -- integer spinner widget
    zr_widget_scrollbarv           -- vertical scrollbar widget imeplementation
    zr_widget_scrollbarh           -- horizontal scrollbar widget imeplementation
*/
enum zr_text_align {
    ZR_TEXT_LEFT,
    ZR_TEXT_CENTERED,
    ZR_TEXT_RIGHT
};

enum zr_button_behavior {
    ZR_BUTTON_DEFAULT,
    /* button only returns on activation */
    ZR_BUTTON_REPEATER,
    /* button returns as long as the button is pressed */
    ZR_BUTTON_BEHAVIOR_MAX
};

struct zr_text {
    struct zr_vec2 padding;
    /* padding between bounds and text */
    struct zr_color background;
    /* text background color */
    struct zr_color text;
    /* text color */
};

struct zr_button {
    zr_float border_width;
    /* size of the border */
    zr_float rounding;
    /* buttong rectangle rounding */
    struct zr_vec2 padding;
    /* padding between bounds and content */
    struct zr_color border;
    /* button border color */
    struct zr_color normal;
    /* normal button background color */
    struct zr_color hover;
    /* hover button background color */
    struct zr_color active;
    /* hover button background color */
};

struct zr_button_text {
    struct zr_button base;
    /* basic button style */
    enum zr_text_align alignment;
    /* text alignment in the button */
    struct zr_color normal;
    /* normal text color */
    struct zr_color hover;
    /* hovering text border color */
    struct zr_color active;
    /* active text border color */
};

enum zr_symbol {
    ZR_SYMBOL_X,
    ZR_SYMBOL_UNDERSCORE,
    ZR_SYMBOL_CIRCLE,
    ZR_SYMBOL_CIRCLE_FILLED,
    ZR_SYMBOL_RECT,
    ZR_SYMBOL_RECT_FILLED,
    ZR_SYMBOL_TRIANGLE_UP,
    ZR_SYMBOL_TRIANGLE_DOWN,
    ZR_SYMBOL_TRIANGLE_LEFT,
    ZR_SYMBOL_TRIANGLE_RIGHT,
    ZR_SYMBOL_PLUS,
    ZR_SYMBOL_MINUS,
    ZR_SYMBOL_MAX
};

struct zr_button_symbol {
    struct zr_button base;
    /* basic button style */
    struct zr_color normal;
    /* normal triangle color */
    struct zr_color hover;
    /* hovering triangle color */
    struct zr_color active;
    /* active triangle color */
};

struct zr_button_icon {
    struct zr_button base;
    /* basic button style */
    struct zr_vec2 padding;
    /* additional image padding */
};

enum zr_toggle_type {
    ZR_TOGGLE_CHECK,
    /* checkbox toggle */
    ZR_TOGGLE_OPTION
    /* radiobutton toggle */
};

struct zr_toggle {
    zr_float rounding;
    /* checkbox rectangle rounding */
    struct zr_vec2 padding;
    /* padding between bounds and content */
    struct zr_color font;
    /* text background  */
    struct zr_color background;
    /* text color background */
    struct zr_color normal;
    /* toggle cursor background normal color*/
    struct zr_color hover;
    /* toggle cursor background hove color*/
    struct zr_color cursor;
    /* toggle cursor color*/
};

struct zr_progress {
    zr_float rounding;
    /* progressbar rectangle rounding */
    struct zr_vec2 padding;
    /* padding between bounds and content */
    struct zr_color border;
    /* progressbar cursor color */
    struct zr_color background;
    /* progressbar background color */
    struct zr_color normal;
    /* progressbar normal cursor color */
    struct zr_color hover;
    /* progressbar hovering cursor color */
    struct zr_color active;
    /* progressbar active cursor color */
};

struct zr_slider {
    struct zr_vec2 padding;
    /* padding between bounds and content */
    struct zr_color border;
    /* slider cursor border color */
    struct zr_color bg;
    /* slider background color */
    struct zr_color normal;
    /* slider cursor color */
    struct zr_color hover;
    /* slider cursor color */
    struct zr_color active;
    /* slider cursor color */
    zr_float rounding;
    /* slider rounding */
};

struct zr_scrollbar {
    zr_float rounding;
    /* scrollbar rectangle rounding */
    struct zr_color border;
    /* scrollbar border color */
    struct zr_color background;
    /* scrollbar background color */
    struct zr_color normal;
    /* scrollbar cursor color */
    struct zr_color hover;
    /* scrollbar cursor color */
    struct zr_color active;
    /* scrollbar cursor color */
    zr_bool has_scrolling;
    /* flag if the scrollbar can be updated by scrolling */
};

enum zr_input_filter {
    ZR_INPUT_DEFAULT,
    /* everything goes */
    ZR_INPUT_ASCII,
    /* ASCII characters (0-127)*/
    ZR_INPUT_FLOAT,
    /* only float point numbers */
    ZR_INPUT_DEC,
    /* only integer numbers */
    ZR_INPUT_HEX,
    /* only hex numbers */
    ZR_INPUT_OCT,
    /* only octal numbers */
    ZR_INPUT_BIN
    /* only binary numbers */
};

struct zr_edit {
    zr_float border_size;
    /* editbox border line size */
    zr_float rounding;
    /* editbox rectangle rounding */
    struct zr_vec2 padding;
    /* padding between bounds and content*/
    zr_bool show_cursor;
    /* flag indication if the cursor should be drawn */
    struct zr_color background;
    /* editbox background */
    struct zr_color border;
    /* editbox border color */
    struct zr_color cursor;
    /* editbox cursor color */
    struct zr_color text;
    /* editbox text color */
};

struct zr_spinner {
    struct zr_button_symbol button;
    /* button style */
    struct zr_color color;
    /* spinner background color  */
    struct zr_color border;
    /* spinner border color */
    struct zr_color text;
    /* spinner text color */
    struct zr_vec2 padding;
    /* spinner padding between bounds and content*/
    zr_bool show_cursor;
    /* flag indicating if the cursor should be drawn */
};

void zr_widget_text(struct zr_command_buffer*, struct zr_rect,
                const char*, zr_size, const struct zr_text*,
                enum zr_text_align, const struct zr_user_font*);
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
zr_bool zr_widget_button_text(struct zr_command_buffer*, struct zr_rect,
                                const char*, enum zr_button_behavior,
                                const struct zr_button_text*,
                                const struct zr_input*, const struct zr_user_font*);
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
    - returns zr_true if the button was pressed zr_false otherwise
*/
zr_bool zr_widget_button_image(struct zr_command_buffer*, struct zr_rect,
                                struct zr_image, enum zr_button_behavior b,
                                const struct zr_button_icon*, const struct zr_input*);
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
    - returns zr_true if the button was pressed zr_false otherwise
*/
zr_bool zr_widget_button_symbol(struct zr_command_buffer*, struct zr_rect,
                                enum zr_symbol, enum zr_button_behavior,
                                const struct zr_button_symbol*, const struct zr_input*,
                                const struct zr_user_font*);
/*  this function executes a triangle button widget
    Input:
    - output command buffer for drawing
    - triangle button bounds
    - triangle direction with either left, top, right xor bottom
    - button behavior with either repeating or transition state event
    - visual widget style structure describing the button
    - input structure to update the button with
    Output:
    - returns zr_true if the button was pressed zr_false otherwise
*/
zr_bool zr_widget_button_text_symbol(struct zr_command_buffer*, struct zr_rect,
                                        enum zr_symbol, const char*, enum zr_text_align,
                                        enum zr_button_behavior,
                                        const struct zr_button_text*,
                                        const struct zr_user_font*,
                                        const struct zr_input*);
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
    - returns zr_true if the button was pressed zr_false otherwise
*/
zr_bool zr_widget_button_text_image(struct zr_command_buffer*, struct zr_rect,
                                    struct zr_image, const char*, enum zr_text_align,
                                    enum zr_button_behavior,
                                    const struct zr_button_text*,
                                    const struct zr_user_font*, const struct zr_input*);
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
    - returns zr_true if the button was pressed zr_false otherwise
*/
void zr_widget_toggle(struct zr_command_buffer*, struct zr_rect,
                        zr_bool*, const char *string, enum zr_toggle_type,
                        const struct zr_toggle*, const struct zr_input*,
                        const struct zr_user_font*);
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
zr_float zr_widget_slider(struct zr_command_buffer*, struct zr_rect,
                        zr_float min, zr_float val, zr_float max, zr_float step,
                        const struct zr_slider *s, const struct zr_input *in);
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
zr_size zr_widget_progress(struct zr_command_buffer*, struct zr_rect,
                            zr_size value, zr_size max, zr_bool modifyable,
                            const struct zr_progress*, const struct zr_input*);
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
void zr_widget_editbox(struct zr_command_buffer*, struct zr_rect,
                        struct zr_edit_box*, const struct zr_edit*,
                        const struct zr_input*, const struct zr_user_font*);
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
zr_size zr_widget_edit(struct zr_command_buffer*, struct zr_rect, zr_char*, zr_size,
                        zr_size max, zr_state*, zr_size *cursor, const struct zr_edit*,
                        enum zr_input_filter filter, const struct zr_input*,
                        const struct zr_user_font*);
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
zr_size zr_widget_edit_filtered(struct zr_command_buffer*, struct zr_rect,
                                zr_char*, zr_size, zr_size max, zr_state*,
                                zr_size *cursor, const struct zr_edit*,
                                zr_filter filter, const struct zr_input*,
                                const struct zr_user_font*);
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
zr_int zr_widget_spinner(struct zr_command_buffer*, struct zr_rect,
                        const struct zr_spinner*, zr_int min, zr_int value,
                        zr_int max, zr_int step, zr_state *active,
                        const struct zr_input*, const struct zr_user_font*);
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
zr_float zr_widget_scrollbarv(struct zr_command_buffer*, struct zr_rect,
                                zr_float offset, zr_float target,
                                zr_float step, const struct zr_scrollbar*,
                                const struct zr_input*);
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
zr_float zr_widget_scrollbarh(struct zr_command_buffer*, struct zr_rect,
                                zr_float offset, zr_float target,
                                zr_float step, const struct zr_scrollbar*,
                                const struct zr_input*);
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
 *                          Style
 *
 * ===============================================================
 */
/*  STYLE
    ----------------------------
    The window style consists of properties, color and rectangle rounding
    information that is used for the general style and looks of window.
    In addition for temporary modification the configuration structure consists
    of a stack for pushing and pop either color or property values.

    USAGE
    ----------------------------
    To use the configuration file you either initialize every value yourself besides
    the internal stack which needs to be initialized to zero or use the default
    configuration by calling the function zr_config_default.
    To add and remove temporary configuration states the zr_config_push_xxxx
    for adding and zr_config_pop_xxxx for removing either color or property values
    from the stack. To reset all previously modified values the zr_config_reset_xxx
    were added.

    Configuration function API
    zr_style_default          -- initializes a default style
    zr_style_set_font         -- changes the used font
    zr_style_property         -- returns the property value from an id
    zr_style_color            -- returns the color value from an id
    zr_style_push_property    -- push old property onto stack and sets a new value
    zr_style_push_color       -- push old color onto stack and sets a new value
    zr_style_pop_color        -- resets an old color value from the internal stack
    zr_style_pop_property     -- resets an old property value from the internal stack
    zr_style_reset_colors     -- reverts back all temporary color changes from the style
    zr_style_reset_properties -- reverts back all temporary property changes
    zr_style_reset            -- reverts back all temporary all changes from the config
*/
enum zr_style_colors {
    ZR_COLOR_TEXT,
    ZR_COLOR_TEXT_HOVERING,
    ZR_COLOR_TEXT_ACTIVE,
    ZR_COLOR_WINDOW,
    ZR_COLOR_HEADER,
    ZR_COLOR_BORDER,
    ZR_COLOR_BUTTON,
    ZR_COLOR_BUTTON_HOVER,
    ZR_COLOR_BUTTON_ACTIVE,
    ZR_COLOR_TOGGLE,
    ZR_COLOR_TOGGLE_HOVER,
    ZR_COLOR_TOGGLE_CURSOR,
    ZR_COLOR_SLIDER,
    ZR_COLOR_SLIDER_CURSOR,
    ZR_COLOR_SLIDER_CURSOR_HOVER,
    ZR_COLOR_SLIDER_CURSOR_ACTIVE,
    ZR_COLOR_PROGRESS,
    ZR_COLOR_PROGRESS_CURSOR,
    ZR_COLOR_PROGRESS_CURSOR_HOVER,
    ZR_COLOR_PROGRESS_CURSOR_ACTIVE,
    ZR_COLOR_INPUT,
    ZR_COLOR_INPUT_CURSOR,
    ZR_COLOR_INPUT_TEXT,
    ZR_COLOR_SPINNER,
    ZR_COLOR_SPINNER_TRIANGLE,
    ZR_COLOR_HISTO,
    ZR_COLOR_HISTO_BARS,
    ZR_COLOR_HISTO_NEGATIVE,
    ZR_COLOR_HISTO_HIGHLIGHT,
    ZR_COLOR_PLOT,
    ZR_COLOR_PLOT_LINES,
    ZR_COLOR_PLOT_HIGHLIGHT,
    ZR_COLOR_SCROLLBAR,
    ZR_COLOR_SCROLLBAR_CURSOR,
    ZR_COLOR_SCROLLBAR_CURSOR_HOVER,
    ZR_COLOR_SCROLLBAR_CURSOR_ACTIVE,
    ZR_COLOR_TABLE_LINES,
    ZR_COLOR_TAB_HEADER,
    ZR_COLOR_SHELF,
    ZR_COLOR_SHELF_TEXT,
    ZR_COLOR_SHELF_ACTIVE,
    ZR_COLOR_SHELF_ACTIVE_TEXT,
    ZR_COLOR_SCALER,
    ZR_COLOR_COUNT
};

enum zr_style_rounding {
    ZR_ROUNDING_BUTTON,
    ZR_ROUNDING_SLIDER,
    ZR_ROUNDING_PROGRESS,
    ZR_ROUNDING_CHECK,
    ZR_ROUNDING_INPUT,
    ZR_ROUNDING_GRAPH,
    ZR_ROUNDING_SCROLLBAR,
    ZR_ROUNDING_MAX
};

enum zr_style_properties {
    ZR_PROPERTY_ITEM_SPACING,
    ZR_PROPERTY_ITEM_PADDING,
    ZR_PROPERTY_PADDING,
    ZR_PROPERTY_SCALER_SIZE,
    ZR_PROPERTY_SCROLLBAR_SIZE,
    ZR_PROPERTY_SIZE,
    ZR_PROPERTY_MAX
};

struct zr_saved_property {
    enum zr_style_properties type;
    /* identifier of the current modified property */
    struct zr_vec2 value;
    /* property value that has been saveed */
};

struct zr_saved_color {
    enum zr_style_colors type;
    /* identifier of the current modified color */
    struct zr_color value;
    /* color value that has been saveed */
};

enum zr_style_components {
    ZR_DEFAULT_COLOR = 0x01,
    /* default all colors inside the configuration struct */
    ZR_DEFAULT_PROPERTIES = 0x02,
    /* default all properites inside the configuration struct */
    ZR_DEFAULT_ROUNDING = 0x04,
    /* default all rounding values inside the configuration struct */
    ZR_DEFAULT_ALL = 0xFFFF
    /* default the complete configuration struct */
};

struct zr_style_mod_stack  {
    zr_size property;
    /* current property stack pushing index */
    struct zr_saved_property properties[ZR_MAX_ATTRIB_STACK];
    /* saved property stack */
    struct zr_saved_color colors[ZR_MAX_COLOR_STACK];
    /* saved color stack */
    zr_size color;
    /* current color stack pushing index */
};

struct zr_style {
    struct zr_user_font font;
    /* the from the user provided font */
    zr_float rounding[ZR_ROUNDING_MAX];
    /* rectangle widget rounding */
    struct zr_vec2 properties[ZR_PROPERTY_MAX];
    /* configuration properties to modify the style */
    struct zr_color colors[ZR_COLOR_COUNT];
    /* configuration color to modify color */
    struct zr_style_mod_stack stack;
    /* modification stack */
};

void zr_style_default(struct zr_style*, zr_flags, const struct zr_user_font*);
/*  this function load the window configuration with default values
    Input:
    - config flags which part of the configuration should be loaded with default values
    - user font reference structure describing the font used inside the window
    Output:
    - configuration structure holding the default window style
*/
void zr_style_set_font(struct zr_style*, const struct zr_user_font*);
/*  this function changes the used font and can be used even inside a frame
    Input:
    - user font reference structure describing the font used inside the window
*/
struct zr_vec2 zr_style_property(const struct zr_style*,
                                            enum zr_style_properties);
/*  this function accesses a configuration property over an identifier
    Input:
    - Configuration the get the property from
    - Configuration property idenfifier describing the property to get
    Output:
    - Property value that has been asked for
*/
struct zr_color zr_style_color(const struct zr_style*, enum zr_style_colors);
/*  this function accesses a configuration color over an identifier
    Input:
    - Configuration the get the color from
    - Configuration color idenfifier describing the color to get
    Output:
    - color value that has been asked for
*/
void zr_style_push_property(struct zr_style*, enum zr_style_properties, struct zr_vec2);
/*  this function temporarily changes a property in a stack to be reseted later
    Input:
    - Configuration structure to push the change to
    - Property idenfifier to change
    - new value of the property
*/
void zr_style_push_color(struct zr_style*, enum zr_style_colors, struct zr_color);
/*  this function temporarily changes a color in a stack like fashion to be reseted later
    Input:
    - Configuration structure to push the change to
    - color idenfifier to change
    - new color
*/
void zr_style_pop_color(struct zr_style*);
/*  this function reverts back a previously pushed temporary color change
    Input:
    - Configuration structure to pop the change from and to
*/
void zr_style_pop_property(struct zr_style*);
/*  this function reverts back a previously pushed temporary property change
    Input:
    - Configuration structure to pop the change from and to
*/
void zr_style_reset_colors(struct zr_style*);
/*  this function reverts back all previously pushed temporary color changes
    Input:
    - Configuration structure to pop the change from and to
*/
void zr_style_reset_properties(struct zr_style*);
/*  this function reverts back all previously pushed temporary color changes
    Input:
    - Configuration structure to pop the change from and to
*/
void zr_style_reset(struct zr_style*);
/*  this function reverts back all previously pushed temporary color and
 *  property changes
    Input:
    - Configuration structure to pop the change from and to
*/
/*
 * ==============================================================
 *
 *                          Tiling
 *
 * ===============================================================
    TILING
    ----------------------------
    Tiling provides a way to divide a space into fixed slots which again can be
    divided into either horizontal or vertical spaces. The tiled layout consists
    of five slots (Top, Left, Center, Right and Bottom) what is also known
    as a Border layout. Since slots can either be filled or empty, horizontally or
    vertically fillable, have either none, one or many subspaces and can even
    have a tiled layout inside it is possible to achive a great number of layout.

    In addition it is possible to define the space inside the tiled layout either
    in pixel or as a ratio. Ratio based layout are great for scaling but
    are less usefull in cases where scaling destroys the UI. Pixel based layouts
    provided a layout which always look the same but are less flexible.

    The tiled layout is used in the library inside windows as a widget layout
    and for window placing inside the screen with each case having its own functions
    to handle and use the tiled layout.

    USAGE
    ----------------------------
    To use the tiled layout you have to define which slot inside the layout
    should be active, how the slot should be filled and how much space the
    it takes. To define each slot you first have to call `zr_tiled_begin`
    to start the layout slot definiton and to end the definition
    the corresponding function `zr_tiled_end` has to be called.
    Between both sequence points you can define each slot with `zr_tiled_slot`.

    -----------------------------
    |           TOP             |
    -----------------------------
    |       |           |       |
    | LEFT  |   CENTER  | RIGHT |
    |       |           |       |
    -----------------------------
    |           BOTTOM          |
    -----------------------------

    Tiling API
    zr_tiled_begin      -- starts the definition of a tiled layout
    zr_tiled_begin_local-- starts the definition inside the first depth of a window
    zr_tiled_slot       -- activates and setups a slot inside the tile layout
    zr_tiled_end        -- ends the definiition of the tiled layout slots
    zr_tiled_slot_bounds-- returns the bounds of a slot in the tiled layout
    zr_tiled_bounds     -- returns the bounds of a widget in the tiled layout
    zr_tiled_load       -- loads another tiled layout from slot
*/
enum zr_tiled_layout_slot_index {
    ZR_SLOT_TOP,
    ZR_SLOT_BOTTOM,
    ZR_SLOT_LEFT,
    ZR_SLOT_CENTER,
    ZR_SLOT_RIGHT,
    ZR_SLOT_MAX
};

enum zr_layout_format {
    ZR_DYNAMIC, /* row layout which scales with the window */
    ZR_STATIC /* row layout with fixed pixel width */
};

enum zr_tiled_slot_format {
    ZR_SLOT_HORIZONTAL,
    /* widgets in slots are added left to right */
    ZR_SLOT_VERTICAL
    /* widgets in slots are added top to bottom */
};

struct zr_tiled_slot {
    zr_uint capacity;
    /* number of widget inside the slot */
    zr_float value;
    /* temp value for layout build up */
    struct zr_vec2 size;
    /* horizontal and vertical window (ratio/width) */
    struct zr_vec2 pos;
    /* position of the slot in the window */
    enum zr_tiled_slot_format format;
    /* layout filling format  */
};

struct zr_tiled_layout {
    struct zr_tiled_slot slots[ZR_SLOT_MAX];
    /* tiled layout slots */
    enum zr_layout_format fmt;
    /* row layout format */
    struct zr_rect bounds;
    /* bounding rect of the layout */
    struct zr_vec2 spacing;
};

void zr_tiled_begin(struct zr_tiled_layout*, enum zr_layout_format,
                    struct zr_rect bounds, struct zr_vec2 spacing);
/*  this functions begins the definitions of a tiled layout
    Input:
    - layout format with either dynamic ratio based or fixed pixel based slots
    - pixel position and size of a window inside the screen
    - spacing between slot entries
*/
void zr_tiled_begin_local(struct zr_tiled_layout*, enum zr_layout_format,
                        zr_float width, zr_float height);
/*  this functions begins the definitions of a tiled local layout.
 *  IMPORTANT:  is only used for the first depth of a tiled window widget layout
                all other types of tiled layouts need to use `zr_tiled_begin`.
    Input:
    - layout format with either dynamic ratio based or fixed pixel based slots
    - pixel width of the tiled layout space (IMPORTANT: not used for dynamic tiled layouts)
    - pixel height of the tiled layout space
*/
void zr_tiled_slot(struct zr_tiled_layout *layout,
                    enum zr_tiled_layout_slot_index, zr_float ratio,
                    enum zr_tiled_slot_format, zr_uint widget_count);
/*  this functions defines a slot in the tiled layout which then can be filled
 *  with widgets
    Input:
    - slot identifier
    - either ratio or pixel size of the slot
    - slot filling format with either horizontal or vertical filling
    - number of widgets inside the slot
*/
void zr_tiled_end(struct zr_tiled_layout*);
/*  this functions ends the definitions of the tiled layout slots */
void zr_tiled_slot_bounds(struct zr_rect*, const struct zr_tiled_layout*,
                            enum zr_tiled_layout_slot_index);
/*  this functions queries the bounds (position + size) of a tiled layout slot
    Input:
    - slot identifier
    Output:
    - rectangle with position and size of the slot
*/
void zr_tiled_bounds(struct zr_rect*, const struct zr_tiled_layout*,
                        enum zr_tiled_layout_slot_index, zr_uint);
/*  this functions queries the bounds (position + size) of a tiled layout slot entry
    Input:
    - slot identifier
    - index of the widget inside the slot
    Output:
    - rectangle with position and size of the slot entry
*/
void zr_tiled_load(struct zr_tiled_layout *parent, struct zr_tiled_layout *child,
                    enum zr_layout_format fmt, enum zr_tiled_layout_slot_index slot,
                    zr_uint index);
/*  this functions load a tiled layout from another tiled layout slot
    Input:
    - slot filling format with either horizontal or vertical filling
    - slot identifier
    - index of the widget inside the slot
    Output:
    - loaded child tiled layout inside the parent tiled layout
*/
/*
 * ==============================================================
 *
 *                          Window
 *
 * ===============================================================
    WINDOW
    The window groups widgets together and allows collective operation
    on these widgets like movement, scrolling, window minimizing and closing.
    Windows are divided into a persistent state window struct and a temporary
    context which is used each frame to fill the window. All direct build up
    function therefore work on the context and not on the actual window.
    Each window is linked inside a queue which in term allows for an easy
    way to buffer output commands but requires that the window is unlinked
    from the queue if removed.

    USAGE
    The window needs to be initialized by `zr_window_init` and can be updated
    by all the `zr_window_set_xxx` function. Important to note is that each
    window is linked inside a queue by an internal memory buffer. So if you want
    to remove the window you first have to remove the window from the queue
    or if you want to change to queue use `zr_window_queue_set`.

    window function API
    ------------------
    zr_window_init         -- initializes the window with position, size and flags
    zr_window_unlink       -- remove the window from the command queue
    zr_window_set_config   -- updates the used window configuration
    zr_window_add_flag     -- adds a behavior flag to the window
    zr_window_remove_flag  -- removes a behavior flag from the window
    zr_window_has_flag     -- check if a given behavior flag is set in the window
    zr_window_is_minimized -- return wether the window is minimized

    APIs
    -----------------
    Window Context API  -- The context is temporary state that is used every frame to build a window
    Window Header API   -- Responsible for creating a header at the top of a window
    Window Layout API   -- The window layout is responsible for placing widget in the window
    Window Widget API   -- Different widget that can be placed inside the window
    Window Tree API     -- Tree widget that allows to visualize and mofify a tree
    Window Combobox API -- Combobox widget for collapsable popup content
    Window Group API    -- Create a subwindow inside a window which again can be filled with widgets
    Window Shelf API    -- Group window with tabs which can be filled with widget
    Window Popup API    -- Popup window with either non-blocking or blocking capabilities
    Window Menu API     -- Popup menus with currently one single depth
*/
enum zr_window_flags {
    ZR_WINDOW_HIDDEN = 0x01,
    /* Hiddes the window and stops any window interaction and drawing can be set
     * by user input or by closing the window */
    ZR_WINDOW_BORDER = 0x02,
    /* Draws a border around the window to visually seperate the window from the
     * background */
    ZR_WINDOW_BORDER_HEADER = 0x04,
    /* Draws a border between window header and body */
    ZR_WINDOW_MOVEABLE = 0x08,
    /* The moveable flag inidicates that a window can be move by user input by
     * dragging the window header */
    ZR_WINDOW_SCALEABLE = 0x10,
    /* The scaleable flag indicates that a window can be scaled by user input
     * by dragging a scaler icon at the button of the window */
    ZR_WINDOW_MINIMIZED = 0x20,
    /* marks the window as minimized */
    ZR_WINDOW_ROM = 0x40,
    /* sets the window into a read only mode and does not allow input changes */
    ZR_WINDOW_DYNAMIC = 0x80,
    /* special type of window which grows up in height while being filled to a
     * certain maximum height. It is mainly used for combo boxes but can be
     * used to create perfectly fitting windows as well */
    ZR_WINDOW_ACTIVE = 0x10000,
    /* INTERNAL ONLY!: marks the window as active, used by the window stack */
    ZR_WINDOW_TAB = 0x20000,
    /* INTERNAL ONLY!: Marks the window as subwindow of another window(Groups/Tabs/Shelf)*/
    ZR_WINDOW_COMBO_MENU = 0x40000,
    /* INTERNAL ONLY!: Marks the window as an combo box or menu */
    ZR_WINDOW_REMOVE_ROM = 0x80000,
    /* INTERNAL ONLY!: removes the read only mode at the end of the window */
    ZR_WINDOW_NO_SCROLLBAR = 0x100000
    /* INTERNAL ONLY!: removes the scrollbar from the window */
};

struct zr_window {
    struct zr_rect bounds;
    /* size with width and height and position of the window */
    zr_flags flags;
    /* window flags modifing its behavior */
    struct zr_vec2 offset;
    /* scrollbar x- and y-offset */
    const struct zr_style *style;
    /* configuration reference describing the window style */
    struct zr_command_buffer buffer;
    /* output command buffer queuing all drawing calls */
    struct zr_command_queue *queue;
    /* output command queue which hold the command buffer */
    const struct zr_input *input;
    /* input state for updating the window and all its widgets */
};

void zr_window_init(struct zr_window*, struct zr_rect bounds, zr_flags flags,
                    struct zr_command_queue*, const struct zr_style*,
                    const struct zr_input *in);
/*  this function initilizes and setups the window
    Input:
    - bounds of the window with x,y position and width and height
    - window flags for modified window behavior
    - reference to a output command queue to push draw calls into
    - configuration file containing the style, color and font for the window
    Output:
    - a newly initialized window
*/
void zr_window_unlink(struct zr_window*);
/*  this function unlinks the window from its queue */
void zr_window_add_flag(struct zr_window*, zr_flags);
/*  this function adds window flags to the window */
void zr_window_remove_flag(struct zr_window*, zr_flags);
/*  this function removes window flags from the window */
zr_bool zr_window_has_flag(struct zr_window*, zr_flags);
/*  this function checks if a window has given flag(s) */
zr_bool zr_window_is_minimized(struct zr_window*);
/*  this function checks if the window is minimized */
/* --------------------------------------------------------------
 *                          Context
 * --------------------------------------------------------------
    CONTEXT
    The context is temporary window state that is used in the window drawing
    and build up process. The reason the window and context are divided is that
    the context has far more state which is not needed outside of the build up
    phase. The context is not only useful for normal windows. It is used for
    more complex widget or layout as well. This includes a window inside a window,
    popup windows, menus, comboboxes, etc. In each case the context allows to
    fill a space with widgets. Therefore the context provides the base
    and key stone of the flexibility in the library. The context is used
    for all APIs for the window.

    USAGE
    The context from a window is created by `zr_begin` and finilized by
    `zr_end`. Between these two sequence points the context can be used
    to setup the window with widgets. Other widgets which also use a context
    have each their own `zr_xxx_begin` and `zr_xxx_end` function pair and act
    the same as a window context.

    context function API
    ------------------
    zr_begin   -- starts the window build up process by filling a context with window state
    zr_end     -- ends the window build up process and update the window state
    zr_canvas  -- returns the currently used drawing command buffer
    zr_input   -- returns the from the context used input struct
    zr_queue   -- returns the queue of the window
    zr_space   -- returns the drawable space inside the window
*/
enum zr_widget_states {
    ZR_INACTIVE = zr_false,
    ZR_ACTIVE = zr_true
};

enum zr_collapse_states {
    ZR_MINIMIZED = zr_false,
    ZR_MAXIMIZED = zr_true
};

enum zr_widget_state {
    ZR_WIDGET_INVALID,
    /* The widget cannot be seen and is completly out of view */
    ZR_WIDGET_VALID,
    /* The widget is completly inside the window can be updated */
    ZR_WIDGET_ROM
    /* The widget is partially visible and cannot be updated */
};

enum zr_row_layout_type {
    /* INTERNAL */
    ZR_LAYOUT_DYNAMIC_FIXED,
    /* fixed widget ratio width window layout */
    ZR_LAYOUT_DYNAMIC_ROW,
    /* immediate mode widget specific widget width ratio layout */
    ZR_LAYOUT_DYNAMIC_FREE,
    /* free ratio based placing of widget in a local space  */
    ZR_LAYOUT_DYNAMIC_TILED,
    /* dynamic Border layout */
    ZR_LAYOUT_DYNAMIC,
    /* retain mode widget specific widget ratio width*/
    ZR_LAYOUT_STATIC_FIXED,
    /* fixed widget pixel width window layout */
    ZR_LAYOUT_STATIC_ROW,
    /* immediate mode widget specific widget pixel width layout */
    ZR_LAYOUT_STATIC_FREE,
    /* free pixel based placing of widget in a local space  */
    ZR_LAYOUT_STATIC_TILED,
    /* static Border layout */
    ZR_LAYOUT_STATIC
    /* retain mode widget specific widget pixel width layout */
};

#define ZR_UNDEFINED (-1.0f)
struct zr_row_layout {
    enum zr_row_layout_type type;
    /* type of the row layout */
    zr_size index;
    /* index of the current widget in the current window row */
    zr_float height;
    /* height of the current row */
    zr_size columns;
    /* number of columns in the current row */
    const zr_float *ratio;
    /* row widget width ratio */
    zr_float item_width, item_height;
    /* current width of very item */
    zr_float item_offset;
    /* x positon offset of the current item */
    zr_float filled;
    /* total fill ratio */
    struct zr_rect item;
    /* item bounds */
    struct zr_rect clip;
    /* temporary clipping rect */
};

struct zr_header {
    zr_float x, y, w, h;
    /* header bounds */
    zr_float front, back;
    /* visual header filling deque */
};

struct zr_menu {
    zr_float x, y, w, h;
    /* menu bounds */
    struct zr_vec2 offset;
    /* saved window scrollbar offset */
};

struct zr_context {
    zr_flags flags;
    /* window flags modifing its behavior */
    struct zr_rect bounds;
    /* position and size of the window in the os window */
    struct zr_vec2 offset;
    /* window scrollbar offset */
    zr_bool valid;
    /* flag inidicating if the window is visible */
    zr_float at_x, at_y, max_x;
    /* index position of the current widget row and column  */
    zr_float width, height;
    /* size of the actual useable space inside the window */
    zr_float footer_h;
    /* height of the window footer space */
    struct zr_rect clip;
    /* window clipping rect */
    struct zr_header header;
    /* window header bounds */
    struct zr_menu menu;
    /* window menubar bounds */
    struct zr_row_layout row;
    /* currently used window row layout */
    const struct zr_style *style;
    /* configuration data describing the visual style of the window */
    const struct zr_input *input;
    /* current input state for updating the window and all its widgets */
    struct zr_command_buffer *buffer;
    /* command draw call output command buffer */
    struct zr_command_queue *queue;
    /* command draw call output command buffer */
};

zr_flags zr_begin(struct zr_context*, struct zr_window*);
/*  this function begins the window build up process by creating a context to fill
    Input:
    - input structure holding all user generated state changes
    Output:
    - window context to fill up with widgets
    - ZR_WINDOW_MOVABLE if window was moved
*/
zr_flags zr_begin_tiled(struct zr_context*, struct zr_window*, struct zr_tiled_layout*,
                    enum zr_tiled_layout_slot_index slot, zr_uint index);
/*  this function begins the window build up process by creating a context to fill
    and placing the window inside a tiled layout on the screen.
    Input:
    - input structure holding all user generated state changes
    Output:
    - window context to fill up with widgets
    - ZR_WINDOW_MOVABLE if window was moved
*/
zr_flags zr_end(struct zr_context*, struct zr_window*);
/*  this function ends the window layout build up process and updates the window
    and placing the window inside a tiled layout on the screen.
    Output:
    - ZR_WINDOW_SCALEABLE if window was scaled
*/
struct zr_rect zr_space(struct zr_context*);
/* this function returns the drawable space inside the window */
struct zr_command_buffer* zr_canvas(struct zr_context*);
/* this functions returns the currently used draw command buffer */
const struct zr_input *zr_input(struct zr_context*);
/* this functions returns the currently used input */
struct zr_command_queue *zr_queue(struct zr_context*);
/* this functions returns the currently used queue */
/* --------------------------------------------------------------
 *                          HEADER
 * --------------------------------------------------------------
    HEADER
    The header API is for adding a window space at the top of the window for
    buttons, icons and window title. It is useful for toggling the visiblity
    aswell as minmized state of the window. The header can be filled with buttons
    and icons from the left and as well as the right side and allows therefore
    a wide range of header layouts.

    USAGE
    To create a header you have to call one of two API after the window layout
    has been created with `zr_begin`. The first and easiest way is to
    just call `zr_header` which provides a basic header with
    with title and button and buton pressed notification if a button was pressed.
    The layout supported is hereby limited and custom button and icons cannot be
    added. To achieve that you have to use the more extensive header API.
    You start by calling `zr_header_begin` after `zr_begin` and
    call the different `zr_header_xxx` functions to add icons or the title
    either at the left or right side of the window. Each function returns if the
    icon or button has been pressed or in the case of the toggle the current state.
    Finally if all button/icons/toggles have been added the process is finished
    by calling `zr_header_end`.

    window header function API
    zr_header_begin          -- begins the header build up process
    zr_header_button         -- adds a button into the header
    zr_header_button_icon    -- adds a image button into the header
    zr_header_toggle         -- adds a toggle button into the header
    zr_header_flag           -- adds a window flag toggle button
    zr_header_title          -- adds the title of the window into the header
    zr_header_end            -- finishes the header build up process
    zr_header                -- short cut version of the header build up process
    zr_menubar_begin         -- marks the beginning of the menubar building process
    zr_menubar_end           -- marks the end the menubar build up process
*/
enum zr_header_flags {
    ZR_CLOSEABLE = 0x01,
    /* adds a closeable icon into the header */
    ZR_MINIMIZABLE = 0x02,
    /* adds a minimize icon into the header */
    ZR_SCALEABLE = 0x04,
    /* adds a scaleable flag icon into the header */
    ZR_MOVEABLE = 0x08
    /* adds a moveable flag icon into the header */
};

enum zr_header_align {
    ZR_HEADER_LEFT,
    /* header elements are added at the left side of the header */
    ZR_HEADER_RIGHT
    /* header elements are added at the right side of the header */
};

zr_flags zr_header(struct zr_context*, const char *title, zr_flags show,
                    zr_flags notify, enum zr_header_align);
/*  this function is a shorthand for the header build up process
    flag by the user
    Input:
    - title of the header or NULL if not needed
    - flags indicating which icons should be drawn to the header
    - flags indicating which icons should notify if clicked
*/
void zr_header_begin(struct zr_context*);
/*  this function begins the window header build up process */
zr_bool zr_header_button(struct zr_context *layout, enum zr_symbol symbol,
                            enum zr_header_align);
/*  this function adds a header button icon
    Input:
    -
    - symbol that shall be shown in the header as a icon
    Output:
    - zr_true if the button was pressed zr_false otherwise
*/
zr_bool zr_header_button_icon(struct zr_context*, struct zr_image,
                                    enum zr_header_align);
/*  this function adds a header image button icon
    Input:
    - symbol that shall be shown in the header as a icon
    Output:
    - zr_true if the button was pressed zr_false otherwise
*/
zr_bool zr_header_toggle(struct zr_context*, enum zr_symbol inactive,
                            enum zr_symbol active, enum zr_header_align,
                            zr_bool state);
/*  this function adds a header toggle button
    Input:
    - symbol that will be drawn if the toggle is inactive
    - symbol that will be drawn if the toggle is active
    - state of the toggle with either active or inactive
    Output:
    - updated state of the toggle
*/
zr_bool zr_header_flag(struct zr_context *layout, enum zr_symbol inactive,
                        enum zr_symbol active, enum zr_header_align,
                        enum zr_window_flags flag);
/*  this function adds a header toggle button for modifing a certain window flag
    Input:
    - symbol that will be drawn if the flag is inactive
    - symbol that will be drawn if the flag is active
    - window flag whose state will be display by the toggle button
    Output:
    - zr_true if the button was pressed zr_false otherwise
*/
void zr_header_title(struct zr_context*, const char*, enum zr_header_align);
/*  this function adds a title to the window header
    Input:
    - title of the header
*/
void zr_header_end(struct zr_context*);
/*  this function ends the window header build up process */
void zr_menubar_begin(struct zr_context*);
/*  this function begins the window menubar build up process */
void zr_menubar_end(struct zr_context*);
/*  this function ends the window menubar build up process */

/* --------------------------------------------------------------
 *                          Layout
 * --------------------------------------------------------------
    LAYOUT
    The layout API is for positioning of widget inside a window context. In general there
    are four different ways to position widget. The first one is a table with
    fixed size columns. This like the other three comes in two flavors. First
    the scaleable width as a ratio of the window width and the other is a
    non-scaleable fixed pixel value for static windows.
    Since sometimes widgets with different sizes in a row is needed another set
    of row layout has been added. The first set is for dynamically size widgets
    in an immediate mode API which sets each size of a widget directly before
    it is called or a retain mode API which stores the size of every widget as
    an array.
    The third way to position widgets is by allocating a fixed space from
    the window and directly positioning each widget with position and size.
    This requires the least amount of work for the API and the most for the user,
    but offers the most positioning freedom.
    The final row layout is a tiled layout which divides a space in the panel
    into a Top, Left, Center, Right and Bottom slot. Each slot can be filled
    with widgets either horizontally or vertically.

    fixed width widget layout API
    zr_layout_row_dynamic              -- scaling fixed column row layout
    zr_layout_row_static               -- fixed width fixed column row layout

    custom width widget layout API
    zr_layout_row                      -- user defined widget row layout
    zr_layout_row_begin                -- begins the row build up process
    zr_layout_row_push                 -- pushes the next widget width
    zr_layout_row_end                  -- ends the row build up process

    tiled layout widget placing API
    zr_layout_row_tiled_begin          -- begins tiled layout based placing of widgets
    zr_layout_row_tiled_push           -- pushes a widget into a slot in the tiled layout
    zr_layout_row_tiled_end            -- ends tiled layout based placing of widgets

    custom widget placing API
    zr_layout_row_space_begin          -- creates a free placing space in the window
    zr_layout_row_space_push           -- pushes a widget into the space
    zr_layout_row_space_end            -- finishes the free drawingp process
    zr_layout_row_space_bounds         -- totally allocated space in window
    zr_layout_row_space_to_screen      -- converts from local space to screen
    zr_layout_row_space_to_local       -- converts from screen to local space
    zr_layout_row_space_rect_to_screen -- converts rect from local space to screen
    zr_layout_row_space_rect_to_local  -- converts rect from screen to local space

    window tree layout function API
    zr_layout_push                     -- pushes a new node/collapseable header/tab
    zr_layout_pop                      -- pops the the previously added node
*/
enum zr_layout_node_type {
    ZR_LAYOUT_NODE,
    /* a node is a space which can be minimized or maximized */
    ZR_LAYOUT_TAB
    /* a tab is a node with a header */
};

void zr_layout_peek(struct zr_rect *bounds, struct zr_context*);
/*  this function peeks at the next widget position and size that will be
 *  allocated from the window without actually allocation the space
    output:
    - widget position and size of the next allocate space in the panel
*/
/* ------------------------------ Fixed ----------------------------------- */
void zr_layout_row_dynamic(struct zr_context*, zr_float height, zr_size cols);
/*  this function sets the row layout to dynamically fixed size widget
    Input:
    - height of the row that will be filled
    - number of widget inside the row that will divide the space
*/
void zr_layout_row_static(struct zr_context*, zr_float row_height,
                        zr_size item_width, zr_size cols);
/*  this function sets the row layout to static fixed size widget
    Input:
    - height of the row that will be filled
    - width in pixel measurement of each widget in the row
    - number of widget inside the row that will divide the space
*/
/* ------------------------------ Custom ----------------------------------- */
void zr_layout_row_begin(struct zr_context*, enum zr_layout_format,
                        zr_float row_height, zr_size cols);
/*  this function start a new scaleable row that can be filled with different
    sized widget
    Input:
    - scaleable or fixed row format
    - height of the row that will be filled
    - number of widget inside the row that will divide the space
*/
void zr_layout_row_push(struct zr_context*, zr_float value);
/*  this function pushes a widget into the previously start row with the given
    window width ratio or pixel width
    Input:
    - value with either a ratio for ZR_DYNAMIC or a pixel width for ZR_STATIC layout
*/
void zr_layout_row_end(struct zr_context*);
/*  this function ends the previously started scaleable row */
void zr_layout_row(struct zr_context*, enum zr_layout_format, zr_float height,
                    zr_size cols, const zr_float *ratio);
/*  this function sets the row layout as an array of ratios/width for
    every widget that will be inserted into that row
    Input:
    - scaleable or fixed row format
    - height of the row and there each widget inside
    - number of widget inside the row
    - window ratio/pixel width array for each widget
*/
/* ------------------------------ User ----------------------------------- */
void zr_layout_row_space_begin(struct zr_context*, enum zr_layout_format,
                                zr_float height, zr_size widget_count);
/*  this functions starts a space where widgets can be added
    at any given position and the user has to make sure no overlap occures
    Input:
    - height of the row and therefore each widget inside
    - number of widget that will be added into that space
*/
struct zr_rect zr_layout_row_space_bounds(struct zr_context*);
/*  this functions returns the complete bounds of the space in the panel */
void zr_layout_row_space_push(struct zr_context*, struct zr_rect);
/*  this functions pushes the position and size of the next widget that will
    be added into the previously allocated window space
    Input:
    - rectangle with position and size as a ratio of the next widget to add
*/
struct zr_vec2 zr_layout_row_space_to_screen(struct zr_context*, struct zr_vec2);
/*  this functions calculates a position from local space to screen space
    Input:
    - position in local layout space
    Output:
    - position in screen space
*/
struct zr_vec2 zr_layout_row_space_to_local(struct zr_context*, struct zr_vec2);
/*  this functions calculates a position from screen space to local space
    Input:
    - position in screen layout space
    Output:
    - position in local layout space
*/
struct zr_rect zr_layout_row_space_rect_to_screen(struct zr_context*, struct zr_rect);
/*  this functions calculates a rectange from local space to screen space
    Input:
    - rectangle in local layout space
    Output:
    - rectangle in screen space
*/
struct zr_rect zr_layout_row_space_rect_to_local(struct zr_context*, struct zr_rect);
/*  this functions calculates a rectangle from screen space to local space
    Input:
    - rectangle in screen space
    Output:
    - rectangle in local space
*/
void zr_layout_row_space_end(struct zr_context*);
/*  this functions finishes the scaleable space filling process */
/* ------------------------------ Tiled ----------------------------------- */
void zr_layout_row_tiled_begin(struct zr_context*, struct zr_tiled_layout*);
/*  this functions begins the tiled layout
    Input:
    - row height of the complete layout to allocate from the window
*/
void zr_layout_row_tiled_push(struct zr_context*, struct zr_tiled_layout*,
                            enum zr_tiled_layout_slot_index, zr_uint index);
/*  this functions pushes a widget into a tiled layout slot
    Input:
    - slot identifier
    - widget index in the slot
*/
void zr_layout_row_tiled_end(struct zr_context*);
/*  this functions ends the tiled layout */
/* ------------------------------ Tree ----------------------------------- */
zr_bool zr_layout_push(struct zr_context*, enum zr_layout_node_type,
                        const char *title, zr_state*);
/*  this functions pushes either a tree node or collapseable header into
 *  the current window layout
    Input:
    - title of the node to push into the window
    - type of then node with either default node, collapseable header or tab
    - state of the node with either ZR_MINIMIZED or ZR_MAXIMIZED
    Output:
    - returns the updated state as either zr_true if open and zr_false otherwise
    - updates the state of the node pointer to the updated state
*/
void zr_layout_pop(struct zr_context*);
/*  this functions ends the previously added node */
/* --------------------------------------------------------------
 *                          WIDGETS
 * --------------------------------------------------------------
    WIDGET
    The layout API uses the layout API to provide and add widget to the window.
    IMPORTANT: the widget API does NOT work without a layout so if you have
    visual glitches then the problem probably stems from not using the layout
    correctly. The window widget API does not implement any widget itself, instead
    it uses the general Widget API under the hood and is only responsible for
    calling the correct widget API function with correct position, size and style.
    All widgets do NOT store any state instead everything has to be managed by
    the user.

    USAGE
    To use the Widget API you first have to call one of the layout API funtions
    to setup the widget. After that you can just call one of the widget functions
    at it will automaticall update the widget state as well as `draw` the widget
    by adding draw command into the window command buffer.

    window widgets API
    zr_widget                -- base function for all widgets to allocate space
    zr_widget_fitting        -- special base function for widget without padding/spacing
    zr_spacing               -- column seperator and is basically an empty widget
    zr_seperator             -- adds either a horizontal or vertical seperator
    zr_text                  -- text widget for printing text with length
    zr_text_colored          -- colored text widget for printing string by length
    zr_label                 -- text widget for printing zero terminated strings
    zr_label_colored         -- widget for printing colored zero terminiated strings
    zr_button_text           -- button widget with text content
    zr_button_toggle         -- button toggle widget with text content
    zr_button_color          -- colored button widget without content
    zr_button_symbol         -- button with triangle either up-/down-/left- or right
    zr_button_image          -- button widget width icon content
    zr_button_text_image     -- button widget with text and icon
    zr_button_text_symbol    -- button widget with text and a triangle
    zr_button_fitting        -- button widget without border and fitting space
    zr_image                 -- image widget for outputing a image to a window
    zr_check                 -- add a checkbox widget
    zr_option                -- radiobutton widget
    zr_option_group          -- radiobutton group for automatic single selection
    zr_slider                -- slider widget with min,max,step value
    zr_progress              -- progressbar widget
    zr_edit                  -- edit textbox widget for text input
    zr_edit_filtered         -- edit textbox widget for text input with filter input
    zr_editbox               -- edit textbox with cursor, clipboard and filter
    zr_spinner               -- spinner widget with keyboard or mouse modification
*/
enum zr_widget_state zr_widget(struct zr_rect*, struct zr_context*);
/*  this function represents the base of every widget and calculates the bounds
 *  and allocates space for a widget inside a window.
    Output:
    - allocated space for a widget to draw into
    - state of widget the widget with invisible, renderable and render + updateable
*/
enum zr_widget_state zr_widget_fitting(struct zr_rect*, struct zr_context*);
/*  this function represents calculates the bounds of a perfectly and completly
 *  fitting widget inside the window and allocates the space inside a window.
    Output:
    - allocated space for a widget to draw into
    - state of widget the widget with invisible, renderable and render + updateable
*/
void zr_spacing(struct zr_context*, zr_size cols);
/*  this function creates a seperator to fill space
    Input:
    - number of columns or widget to jump over
*/
void zr_seperator(struct zr_context*);
/*  this function creates a seperator line */
void zr_text(struct zr_context*, const char*, zr_size, enum zr_text_align);
/*  this function creates a bounded non terminated text widget with either
    left, centered or right alignment
    Input:
    - string pointer to text that should be drawn
    - number of bytes the text is long
    - text alignment with either left, centered or right alignment
*/
void zr_text_colored(struct zr_context*, const char*, zr_size, enum zr_text_align,
                    struct zr_color);
/*  this function creates a bounded non terminated color text widget with either
    left, centered or right alignment
    Input:
    - string pointer to text that should be drawn
    - number of bytes the text is long
    - text alignment with either left, centered or right alignment
    - color the text should be drawn
*/
void zr_label(struct zr_context*, const char*, enum zr_text_align);
/*  this function creates a zero terminated text widget with either
    left, centered or right alignment
    Input:
    - string pointer to text that should be drawn
    - text alignment with either left, centered or right alignment
*/
void zr_label_colored(struct zr_context*, const char*, enum zr_text_align, struct zr_color);
/*  this function creates a zero terminated colored text widget with either
    left, centered or right alignment
    Input:
    - string pointer to text that should be drawn
    - text alignment with either left, centered or right alignment
    - color the label should be drawn
*/
void zr_image(struct zr_context*, struct zr_image);
/*  this function creates an image widget
    Input:
    - string pointer to text that should be drawn
*/
zr_bool zr_check(struct zr_context*, const char*, zr_bool active);
/*  this function creates a checkbox widget with either active or inactive state
    Input:
    - checkbox label describing the content
    - state of the checkbox with either active or inactive
    Output:
    - from user input updated state of the checkbox
*/
zr_bool zr_option(struct zr_context*, const char*, zr_bool active);
/*  this function creates a radiobutton widget with either active or inactive state
    Input:
    - radiobutton label describing the content
    - state of the radiobutton with either active or inactive
    Output:
    - from user input updated state of the radiobutton
*/
zr_size zr_option_group(struct zr_context*, const char**, zr_size cnt, zr_size cur);
/*  this function creates a radiobutton group widget with only one active radiobutton
    Input:
    - radiobutton label array describing the content of each radiobutton
    - number of radiobuttons
    - index of the current active radiobutton
    Output:
    - the from user input updated index of the active radiobutton
*/
zr_bool zr_button_text(struct zr_context*, const char*, enum zr_button_behavior);
/*  this function creates a text button
    Input:
    - button label describing the button
    - string label
    - button behavior with either default or repeater behavior
    Output:
    - zr_true if the button was transistioned from unpressed to pressed with
        default button behavior or pressed if repeater behavior.
*/
zr_bool zr_button_color(struct zr_context*, struct zr_color, enum zr_button_behavior);
/*  this function creates a colored button without content
    Input:
    - color the button should be drawn with
    - button behavior with either default or repeater behavior
    Output:
    - zr_true if the button was transistioned from unpressed to pressed with
        default button behavior or pressed if repeater behavior.
*/
zr_bool zr_button_symbol(struct zr_context*, enum zr_symbol, enum zr_button_behavior);
/*  this function creates a button with a triangle pointing in one of four directions
    Input:
    - triangle direction with either up, down, left or right direction
    - button behavior with either default or repeater behavior
    Output:
    - zr_true if the button was transistioned from unpressed to pressed with
        default button behavior or pressed if repeater behavior.
*/
zr_bool zr_button_image(struct zr_context*, struct zr_image img, enum zr_button_behavior);
/*  this function creates a button with an icon as content
    Input:
    - icon image handle to draw into the button
    - button behavior with either default or repeater behavior
    Output:
    - zr_true if the button was transistioned from unpressed to pressed with
        default button behavior or pressed if repeater behavior.
*/
zr_bool zr_button_text_symbol(struct zr_context*, enum zr_symbol, const char*,
                                enum zr_text_align, enum zr_button_behavior);
/*  this function creates a button with a triangle and text
    Input:
    - symbol to draw with the text
    - button label describing the button
    - text alignment with either left, centered or right alignment
    - button behavior with either default or repeater behavior
    Output:
    - zr_true if the button was transistioned from unpressed to pressed with
        default button behavior or pressed if repeater behavior.
*/
zr_bool zr_button_text_image(struct zr_context *layout, struct zr_image img,
                            const char *text, enum zr_text_align align,
                            enum zr_button_behavior behavior);
/*  this function creates a button with an icon and text
    Input:
    - image or subimage to use as an icon
    - button label describing the button
    - text alignment with either left, centered or right alignment
    - button behavior with either default or repeater behavior
    Output:
    - zr_true if the button was transistioned from unpressed to pressed with
        default button behavior or pressed if repeater behavior.
*/
zr_bool zr_button_toggle(struct zr_context*, const char*,zr_bool value);
/*  this function creates a toggle button which is either active or inactive
    Input:
    - label describing the toggle button
    - current state of the toggle
    Output:
    - from user input updated toggle state
*/
zr_bool zr_button_toggle_fitting(struct zr_context*, const char*,zr_bool value);
/*  this function creates a fitting toggle button which is either active or inactive
    Input:
    - label describing the toggle button
    - current state of the toggle
    Output:
    - from user input updated toggle state
*/
zr_float zr_slider(struct zr_context*, zr_float min, zr_float val, zr_float max,
                    zr_float step);
/*  this function creates a slider for value manipulation
    Input:
    - minimal slider value that will not be underflown
    - slider value which shall be updated
    - maximal slider value that will not be overflown
    - step intervall to change the slider with
    Output:
    - the from user input updated slider value
*/
zr_size zr_progress(struct zr_context*, zr_size cur, zr_size max, zr_bool modifyable);
/*  this function creates an either user or program controlled progressbar
    Input:
    - current progressbar value
    - maximal progressbar value that will not be overflown
    - flag indicating if the progressbar should be changeable by user input
    Output:
    - the from user input updated progressbar value if modifyable progressbar
*/
void zr_editbox(struct zr_context*, struct zr_edit_box*);
/*  this function creates an editbox with copy & paste functionality and text buffering */
zr_size zr_edit(struct zr_context*, zr_char *buffer, zr_size len, zr_size max,
                zr_state *active, zr_size *cursor, enum zr_input_filter);
/*  this function creates an editbox to updated/insert user text input
    Input:
    - buffer to fill with user input
    - current length of the buffer in bytes
    - maximal number of bytes the buffer can be filled with
    - state of the editbox with active as currently modified by the user
    - filter type to limit the glyph the user can input into the editbox
    Output:
    - length of the buffer after user input update
    - current state of the editbox with active(zr_true) or inactive(zr_false)
*/
zr_size zr_edit_filtered(struct zr_context*, zr_char *buffer, zr_size len,
                        zr_size max,  zr_state *active, zr_size *cursor, zr_filter);
/*  this function creates an editbox to updated/insert filtered user text input
    Input:
    - buffer to fill with user input
    - current length of the buffer in bytes
    - maximal number of bytes the buffer can be filled with
    - state of the editbox with active as currently modified by the user
    - filter callback to limit the glyphes the user can input into the editbox
    Output:
    - length of the buffer after user input update
    - current state of the editbox with active(zr_true) or inactive(zr_false)
*/
zr_int zr_spinner(struct zr_context*, zr_int min, zr_int value, zr_int max,
                    zr_int step, zr_state *active);
/*  this function creates a integer spinner widget
    Input:
    - min value that will not be underflown
    - current spinner value to be updated by user input
    - max value that will not be overflown
    - spinner value modificaton stepping intervall
    - current state of the spinner with active as currently modfied by user input
    Output:
    - the from user input updated spinner value
    - current state of the editbox with active(zr_true) or inactive(zr_false)
*/
/* --------------------------------------------------------------
 *                          COMBO BOX
 * --------------------------------------------------------------
    COMBO BOX
    The combo box is a minimizable popup window and extends the old school
    text combo box with the possibility to fill combo boxes with any kind of widgets.
    The combo box is internall implemented with a dynamic popup window
    and can only be as height as the window allows.
    There are two different ways to create a combo box. The first one is a
    standart text combo box which has it own function `zr_combo`. The second
    way is the more complex immediate mode API which allows to create
    any kind of content inside the combo box. In case of the second API it is
    additionally possible and sometimes wanted to close the combo box popup
    window This can be achived with `zr_combo_close`.

    combo box API
    zr_combo_begin          -- begins the combo box popup window
    zr_combo_item           -- adds a text item into the combobox
    zr_combo_item_icon      -- adds a text image item into the combobox
    zr_combo_item_symbol    -- adds a text symbol item into the combobox
    zr_combo_close          -- closes the previously opened combo box
    zr_combo_end            -- ends the combo box build up process
*/
void zr_combo_begin(struct zr_context *parent, struct zr_context *combo,
                    const char *selected, zr_state *active);
/*  this function begins the combobox build up process
    Input:
    - parent window layout the combo box will be placed into
    - ouput combo box window layout which will be needed to fill the combo box
    - title of the combo box or in the case of the text combo box the selected item
    - the current state of the combobox with either zr_true (active) or zr_false else
    - the current scrollbar offset of the combo box popup window
*/
zr_bool zr_combo_item(struct zr_context *menu, enum zr_text_align align, const char*);
/*  this function execute a combo box item
    Input:
    - title of the item
    Output
    - `zr_true` if has been clicked `zr_false` otherwise
*/
zr_bool zr_combo_item_icon(struct zr_context *menu, struct zr_image,
                                const char*, enum zr_text_align align);
/*  this function execute combo box icon item
    Input:
    - icon to draw into the combo box item
    - text alignment of the title
    - title of the item
    Output
    - `zr_true` if has been clicked `zr_false` otherwise
*/
zr_bool zr_combo_item_symbol(struct zr_context *menu, enum zr_symbol symbol,
                                    const char*, enum zr_text_align align);
/*  this function execute combo box symbol item
    Input:
    - symbol to draw into the combo box item
    - text alignment of the title
    - title of the item
    Output
    - `zr_true` if has been clicked `zr_false` otherwise
*/
void zr_combo_close(struct zr_context *combo);
/*  this function closes a opened combobox */
void zr_combo_end(struct zr_context *parent, struct zr_context *combo, zr_state*);
/*  this function ends the combobox build up process */
/* --------------------------------------------------------------
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
    zr_graph_begin   -- immediate mode graph building begin sequence point
    zr_graph_push    -- push a value into a graph
    zr_graph_end     -- immediate mode graph building end sequence point
    zr_graph         -- retained mode graph with array of values
    zr_graph_ex      -- ratained mode graph with getter callback
*/
enum zr_graph_type {
    ZR_GRAPH_LINES,
    /* Line graph with each data point being connected with its previous and next node */
    ZR_GRAPH_COLUMN,
    /* Column graph/Histogram with value represented as bars */
    ZR_GRAPH_MAX
};

struct zr_graph {
    zr_bool valid;
    /* graph valid flag to make sure that the graph is visible */
    enum zr_graph_type type;
    /* graph type with either line or column graph */
    zr_float x, y;
    /* graph canvas space position */
    zr_float w, h;
    /* graph canvas space size */
    zr_float min, max;
    /* min and max value for correct scaling of values */
    struct zr_vec2 last;
    /* last line graph point to connect to. Only used by the line graph */
    zr_size index;
    /* current graph value index*/
    zr_size count;
    /* number of values inside the graph */
};

void zr_graph_begin(struct zr_context*, struct zr_graph*, enum zr_graph_type,
                    zr_size count, zr_float min, zr_float max);
/*  this function begins a graph building widget
    Input:
    - type of the graph with either lines or bars
    - minimal graph value for the lower bounds of the graph
    - maximal graph value for the upper bounds of the graph
    Output:
    - graph stack object that can be filled with values
*/
zr_bool zr_graph_push(struct zr_context*,struct zr_graph*,zr_float);
/*  this function pushes a value inside the pushed graph
    Input:
    - value data point to fill into the graph either as point or as bar
*/
void zr_graph_end(struct zr_context *layout, struct zr_graph*);
/*  this function ends the graph */
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
    zr_tree_begin            -- begins the tree build up processs
    zr_tree_begin_node       -- adds and opens a normal node to the tree
    zr_tree_begin_node_icon  -- adds a opens a node with an icon to the tree
    zr_tree_end_node         -- ends and closes a previously added node
    zr_tree_leaf             -- adds a leaf node to a prev opened node
    zr_tree_leaf_icon        -- adds a leaf icon node to a prev opended node
    zr_tree_end              -- ends the tree build up process
*/
enum zr_tree_nodes_states {
    ZR_NODE_ACTIVE = 0x01,
    /* the node is currently opened */
    ZR_NODE_SELECTED = 0x02
    /* the node has been seleted by the user */
};

enum zr_tree_node_operation {
    ZR_NODE_NOP,
    /* node did not receive a command */
    ZR_NODE_CUT,
    /* cut the node from the current tree and add into a buffer */
    ZR_NODE_CLONE,
    /* copy current node and add copy into the parent node */
    ZR_NODE_PASTE,
    /* paste all node in the buffer into the tree */
    ZR_NODE_DELETE
    /* remove the node from the parent tree */
};

struct zr_tree {
    struct zr_context group;
    /* window add the tree into  */
    zr_float x_off, at_x;
    /* current x position of the next node */
    zr_int skip;
    /* flag that indicates that a node will be skipped */
    zr_int depth;
    /* current depth of the tree */
};

void zr_tree_begin(struct zr_context*, struct zr_tree*, const char *title,
                    zr_float row_height, struct zr_vec2 scrollbar);
/*  this function begins the tree building process
    Input:
    - title describing the tree or NULL
    - height of every node inside the window
    - scrollbar offset
    Output:
    - tree build up state structure
*/
enum zr_tree_node_operation zr_tree_begin_node(struct zr_tree*, const char*,
                                                zr_state*);
/*  this function begins a parent node
    Input:
    - title of the node
    - current node state
    Output:
    - operation identifier what should be done with this node
*/
enum zr_tree_node_operation zr_tree_begin_node_icon(struct zr_tree*,
                                                    const char*, struct zr_image,
                                                    zr_state*);
/*  this function begins a text icon parent node
    Input:
    - title of the node
    - icon of the node
    - current node state
    Output:
    - operation identifier what should be done with this node
*/
void zr_tree_end_node(struct zr_tree*);
/*  this function ends a parent node */
enum zr_tree_node_operation zr_tree_leaf(struct zr_tree*, const char*,
                                                    zr_state*);
/*  this function pushes a leaf node to the tree
    Input:
    - title of the node
    - current leaf node state
    Output:
    - operation identifier what should be done with this node
*/
enum zr_tree_node_operation zr_tree_leaf_icon(struct zr_tree*,
                                                const char*, struct zr_image,
                                                zr_state*);
/*  this function pushes a leaf icon node to the tree
    Input:
    - title of the node
    - icon of the node
    - current leaf node state
    Output:
    - operation identifier what should be done with this node
*/
void zr_tree_end(struct zr_context*, struct zr_tree*, struct zr_vec2 *scrollbar);
/*  this function ends a the tree building process */
/* --------------------------------------------------------------
 *                          POPUP
 * --------------------------------------------------------------
    POPUP
    The popup extends the normal window with an overlapping blocking
    window that needs to be closed before the underlining main window can
    be used again. Therefore popups are designed for messages,tooltips and
    are used to create the combo box. Internally the popup creates a subbuffer
    inside a command queue that will be drawn after the complete parent window.

    USAGE
    To create an popup the `zr_window_popup_begin` function needs to be called
    with the parent window local position and size and the wanted type with
    static or dynamic window. A static window has a fixed size and behaves like a
    normal window inside a window, but a dynamic window only takes up as much
    height as needed up to a given maximum height. Dynamic windows are for example
    combo boxes while static window make sense for messsages or tooltips.
    To close a popup you can use the `zr_pop_close` function which takes
    care of the closing process. Finally `zr_popup_end` finializes the popup.

    window blocking popup API
    zr_popup_begin         -- adds a popup inside a window
    zr_popup_close         -- closes the popup window
    zr_popup_end           -- ends the popup building process

    window non-blocking popup API
    zr_popup_menu_begin    -- begin a popup context menu
    zr_popup_menu_close    -- closes a popup context menu
    zr_popup_menu_end      -- ends the popup building process
*/
enum zr_popup_type {
    ZR_POPUP_STATIC,
    /* static fixed height non growing popup */
    ZR_POPUP_DYNAMIC
    /* dynamically growing popup with maximum height */
};

zr_flags zr_popup_begin(struct zr_context *parent, struct zr_context *popup,
                            enum zr_popup_type, zr_flags, struct zr_rect bounds,
                            struct zr_vec2 offset);
/*  this function adds a overlapping blocking popup menu
    Input:
    - type of the popup as either growing or static
    - additonal popup window flags
    - popup position and size of the popup (NOTE: local position)
    - scrollbar offset of wanted
    Output:
    - popup layout to fill with widgets
*/
void zr_popup_close(struct zr_context *popup);
/*  this functions closes a previously opened popup */
void zr_popup_end(struct zr_context *parent, struct zr_context *popup,
                struct zr_vec2 *scrollbar);
/*  this function finishes the previously started popup layout
    Output:
    - The from user input updated popup scrollbar pixel offset
*/
/* --------------------------------------------------------------
 *                      CONTEXTUAL
 * --------------------------------------------------------------
    CONTEXTUAL
    A contextual menu is a dynamic non-blocking popup window. It was mainly
    designed to create a typical right-click menu with items but can be filled with
    any content. The reason special menu items were added was to make the content
    fit the menu. Since the contextual menu is non-blocking in contrast to
    normal popups a click outside of the menu results in a closed menu. In addition
    if one of the menu items function is called a call to one of the items results
    in a closed menu as well. The final method of closing the contextual menu is
    by hand by calling the close function.

    contextual API
    zr_contextual_begin         -- begins the contextual menu popup window
    zr_contextual_item          -- adds a text item into the contextual menu
    zr_contextual_item_icon     -- adds a text image item into the contextual menu
    zr_contextual_item_symbol   -- adds a text symbol item into the contextual menu
    zr_contextual_close         -- closes the previously opened contextual menu
    zr_contextual_end           -- ends the contextual menu build up process
*/
void zr_contextual_begin(struct zr_context *parent, struct zr_context *popup,
                        zr_flags flags, zr_state *active, struct zr_rect body);
/*  this function adds a context menu popup
    Input:
    - type of the popup as either growing or static
    - additonal popup window flags
    - popup position and size of the popup (NOTE: local position)
    - scrollbar offset of wanted
    Output:
    - popup layout to fill with widgets
*/
zr_bool zr_contextual_item(struct zr_context *menu, const char*, enum zr_text_align align);
/*  this function execute contextual menu item
    Input:
    - text alignment of the title
    - title of the item
    Output
    - `zr_true` if has been clicked `zr_false` otherwise
*/
zr_bool zr_contextual_item_icon(struct zr_context *menu, struct zr_image,
                                const char*, enum zr_text_align align);
/*  this function execute contextual menu item
    Input:
    - icon to draw into the menu item
    - text alignment of the title
    - title of the item
    Output
    - `zr_true` if has been clicked `zr_false` otherwise
*/
zr_bool zr_contextual_item_symbol(struct zr_context *menu, enum zr_symbol symbol,
                                    const char*, enum zr_text_align align);
/*  this function execute contextual menu item
    Input:
    - symbol to draw into the menu item
    - text alignment of the title
    - title of the item
    Output
    - `zr_true` if has been clicked `zr_false` otherwise
*/
void zr_contextual_close(struct zr_context *popup);
/*  this functions closes the context menu
     Output:
    - update state of the context menu
*/
void zr_contextual_end(struct zr_context *parent, struct zr_context *popup, zr_state*);
/*  this functions closes a previously opened context menu */
/*----------------------------------------------------------------
 *                          MENU
 * --------------------------------------------------------------
    MENU
    The menu widget provides a overlapping popup window which can
    be opened/closed by clicking on the menu button. It is normally
    placed at the top of the window and is independent of the parent
    scrollbar offset. But if needed the menu can even be placed inside the window.
    At the moment the menu only allows a single depth but that will change
    in the future.

    menu widget API
    zr_menu_begin       -- begins the menu item build up processs
    zr_menu_item        -- adds a item into the menu
    zr_menu_item_icon   -- adds a text + image item into the menu
    zr_menu_item_symbol -- adds a text + symbol item into the menu
    zr_menu_close       -- closes the menu
    zr_menu_end         -- ends the menu item build up process
*/
void zr_menu_begin(struct zr_context *parent,
                        struct zr_context *menu, const char *title,
                        zr_float width, zr_state *active);
/*  this function begins the menu build up process
    Input:
    - parent window layout the menu will be placed into
    - ouput menu window layout
    - title of the menu to
    - the current state of the menu with either zr_true (open) or zr_false else
*/
zr_bool zr_menu_item(struct zr_context *menu, enum zr_text_align align, const char*);
/*  this function execute a menu item
    Input:
    - title of the item
    Output
    - `zr_true` if has been clicked `zr_false` otherwise
*/
zr_bool zr_menu_item_icon(struct zr_context *menu, struct zr_image,
                                const char*, enum zr_text_align align);
/*  this function execute menu text icon item
    Input:
    - icon to draw into the menu item
    - text alignment of the title
    - title of the item
    Output
    - `zr_true` if has been clicked `zr_false` otherwise
*/
zr_bool zr_menu_item_symbol(struct zr_context *menu, enum zr_symbol symbol,
                            const char*, enum zr_text_align align);
/*  this function execute menu text symbol item
    Input:
    - symbol to draw into the menu item
    - text alignment of the title
    - title of the item
    Output
    - `zr_true` if has been clicked `zr_false` otherwise
*/
zr_state zr_menu_close(struct zr_context *menu);
/*  this function closes a opened menu */
void zr_menu_end(struct zr_context *parent, struct zr_context *menu);
/*  this function ends the menu build up process */
/* --------------------------------------------------------------
 *                          TOOLTIP
 * --------------------------------------------------------------
    TOOLTIP
    The tooltip widget can be used to provide the user with information
    by creating a popup window under the mouse cursor which decribes a function.
    To use it you should first test if the mouse hovers over the thing you want
    to provide information for before calling this.

    tooltip widget API
    zr_tooltip          -- creates a simple tooltip popup under the mouse cursor
    zr_tooltip_begin    -- begins a start window popup to be filled
    zr_tooltip_end      -- ends the window popup
*/
void zr_tooltip(struct zr_context*, const char *text);
/*  this function create a simple text tooltip window under the mouse cursor,
    Input:
    - output text to display inside the tooltip
*/
void zr_tooltip_begin(struct zr_context *parent, struct zr_context *tip,
                        zr_float width);
/*  this function begins a popup tooltip window under the mouse cursor
    Input:
    - width of the tooltip window
*/
void zr_tooltip_end(struct zr_context *parent, struct zr_context *tip);
/*  this function ends the tooltip window */
/* --------------------------------------------------------------
 *                          Group
 * --------------------------------------------------------------
 *
    GROUP
    A group window represents a window inside a window. The group thereby has a fixed height
    but just like a normal window has a scrollbar. It main promise is to group together
    a group of widgets into a small space inside a window and to provide a scrollable
    space inside a window.

    USAGE
    To create a group you first have to allocate space in a window. This is done
    by the group row layout API and works the same as widgets. After that the
    `zr_group_begin` has to be called with the parent layout to create
    the group in and a group layout to create a new window inside the window.
    Just like a window layout structures the group layout only has a lifetime
    between the `zr_group_begin` and `zr_group_end` and does
    not have to be persistent.

    group window API
    zr_group_begin -- adds a scrollable fixed space inside the window
    zr_group_begin -- ends the scrollable space
*/
void zr_group_begin(struct zr_context*, struct zr_context *tab,
                    const char *title, zr_flags, struct zr_vec2);
/*  this function adds a grouped group window into the parent window
    IMPORTANT: You need to set the height of the group with zr_row_layout
    Input:
    - group title to write into the header
    - group scrollbar offset
    Output:
    - group layout to fill with widgets
*/
void zr_group_end(struct zr_context*, struct zr_context*, struct zr_vec2 *scrollbar);
/*  this function finishes the previously started group layout
    Output:
    - The from user input updated group scrollbar pixel offset
*/
/* --------------------------------------------------------------
 *                          SHELF
 * --------------------------------------------------------------
    SHELF
    A shelf extends the concept of a group as an window inside a window
    with the possibility to decide which content should be drawn into the group.
    This is achieved by tabs on the top of the group window with one selected
    tab. The selected tab thereby defines which content should be drawn inside
    the group window by an index it returns. So you just have to check the returned
    index and depending on it draw the wanted content.

    shelf API
    zr_shelf_begin   -- begins a shelf with a number of selectable tabs
    zr_shelf_end     -- ends a previously started shelf build up process

*/
zr_size zr_shelf_begin(struct zr_context*, struct zr_context*,
                        const char *tabs[], zr_size size,
                        zr_size active, struct zr_vec2 offset);
/*  this function adds a shelf child window into the parent window
    IMPORTANT: You need to set the height of the shelf with zr_row_layout
    Input:
    - all possible selectible tabs of the shelf with names as a string array
    - number of seletectible tabs
    - current active tab array index
    - scrollbar pixel offset for the shelf
    Output:
    - group layout to fill with widgets
    - the from user input updated current shelf tab index
*/
void zr_shelf_end(struct zr_context *p, struct zr_context *s, struct zr_vec2 *scrollbar);
/*  this function finishes the previously started shelf layout
    Input:
    - previously started group layout
    Output:
    - The from user input updated shelf scrollbar pixel offset
*/

#ifdef __cplusplus
}
#endif

#endif /* ZR_H_ */
