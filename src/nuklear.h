#ifndef NK_NUKLEAR_H_
#define NK_NUKLEAR_H_

#ifdef __cplusplus
extern "C" {
#endif
/*
 * ==============================================================
 *
 *                          CONSTANTS
 *
 * ===============================================================
 */
#define NK_UNDEFINED (-1.0f)
#define NK_UTF_INVALID 0xFFFD /* internal invalid utf8 rune */
#define NK_UTF_SIZE 4 /* describes the number of bytes a glyph consists of*/
#ifndef NK_INPUT_MAX
  #define NK_INPUT_MAX 16
#endif
#ifndef NK_MAX_NUMBER_BUFFER
  #define NK_MAX_NUMBER_BUFFER 64
#endif
#ifndef NK_SCROLLBAR_HIDING_TIMEOUT
  #define NK_SCROLLBAR_HIDING_TIMEOUT 4.0f
#endif
/*
 * ==============================================================
 *
 *                          HELPER
 *
 * ===============================================================
 */
#ifndef NK_API
  #ifdef NK_PRIVATE
    #if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199409L))
      #define NK_API static inline
    #elif defined(__cplusplus)
      #define NK_API static inline
    #else
      #define NK_API static
    #endif
  #else
    #define NK_API extern
  #endif
#endif
#ifndef NK_LIB
  #ifdef NK_SINGLE_FILE
    #define NK_LIB static
  #else
    #define NK_LIB extern
  #endif
#endif

#define NK_INTERN static
#define NK_STORAGE static
#define NK_GLOBAL static

#define NK_FLAG(x) (1 << (x))
#define NK_STRINGIFY(x) #x
#define NK_MACRO_STRINGIFY(x) NK_STRINGIFY(x)
#define NK_STRING_JOIN_IMMEDIATE(arg1, arg2) arg1 ## arg2
#define NK_STRING_JOIN_DELAY(arg1, arg2) NK_STRING_JOIN_IMMEDIATE(arg1, arg2)
#define NK_STRING_JOIN(arg1, arg2) NK_STRING_JOIN_DELAY(arg1, arg2)

#ifdef _MSC_VER
  #define NK_UNIQUE_NAME(name) NK_STRING_JOIN(name,__COUNTER__)
#else
  #define NK_UNIQUE_NAME(name) NK_STRING_JOIN(name,__LINE__)
#endif

#ifndef NK_STATIC_ASSERT
  #define NK_STATIC_ASSERT(exp) typedef char NK_UNIQUE_NAME(_dummy_array)[(exp)?1:-1]
#endif

#ifndef NK_FILE_LINE
#ifdef _MSC_VER
  #define NK_FILE_LINE __FILE__ ":" NK_MACRO_STRINGIFY(__COUNTER__)
#else
  #define NK_FILE_LINE __FILE__ ":" NK_MACRO_STRINGIFY(__LINE__)
#endif
#endif

#define NK_MIN(a,b) ((a) < (b) ? (a) : (b))
#define NK_MAX(a,b) ((a) < (b) ? (b) : (a))
#define NK_CLAMP(i,v,x) (NK_MAX(NK_MIN(v,x), i))

#ifdef NK_INCLUDE_STANDARD_VARARGS
  #if defined(_MSC_VER) && (_MSC_VER >= 1600) /* VS 2010 and above */
    #include <sal.h>
    #define NK_PRINTF_FORMAT_STRING _Printf_format_string_
  #else
    #define NK_PRINTF_FORMAT_STRING
  #endif
  #if defined(__GNUC__)
    #define NK_PRINTF_VARARG_FUNC(fmtargnumber) __attribute__((format(__printf__, fmtargnumber, fmtargnumber+1)))
    #define NK_PRINTF_VALIST_FUNC(fmtargnumber) __attribute__((format(__printf__, fmtargnumber, 0)))
  #else
    #define NK_PRINTF_VARARG_FUNC(fmtargnumber)
    #define NK_PRINTF_VALIST_FUNC(fmtargnumber)
  #endif
#endif

/*
 * ===============================================================
 *
 *                          BASIC
 *
 * ===============================================================
 */
#ifdef NK_INCLUDE_FIXED_TYPES
 #include <stdint.h>
 #define NK_INT8 int8_t
 #define NK_UINT8 uint8_t
 #define NK_INT16 int16_t
 #define NK_UINT16 uint16_t
 #define NK_INT32 int32_t
 #define NK_UINT32 uint32_t
 #define NK_SIZE_TYPE uintptr_t
 #define NK_POINTER_TYPE uintptr_t
#else
  #ifndef NK_INT8
    #define NK_INT8 signed char
  #endif
  #ifndef NK_UINT8
    #define NK_UINT8 unsigned char
  #endif
  #ifndef NK_INT16
    #define NK_INT16 signed short
  #endif
  #ifndef NK_UINT16
    #define NK_UINT16 unsigned short
  #endif
  #ifndef NK_INT32
    #if defined(_MSC_VER)
      #define NK_INT32 __int32
    #else
      #define NK_INT32 signed int
    #endif
  #endif
  #ifndef NK_UINT32
    #if defined(_MSC_VER)
      #define NK_UINT32 unsigned __int32
    #else
      #define NK_UINT32 unsigned int
    #endif
  #endif
  #ifndef NK_SIZE_TYPE
    #if defined(_WIN64) && defined(_MSC_VER)
      #define NK_SIZE_TYPE unsigned __int64
    #elif (defined(_WIN32) || defined(WIN32)) && defined(_MSC_VER)
      #define NK_SIZE_TYPE unsigned __int32
    #elif defined(__GNUC__) || defined(__clang__)
      #if defined(__x86_64__) || defined(__ppc64__)
        #define NK_SIZE_TYPE unsigned long
      #else
        #define NK_SIZE_TYPE unsigned int
      #endif
    #else
      #define NK_SIZE_TYPE unsigned long
    #endif
  #endif
  #ifndef NK_POINTER_TYPE
    #if defined(_WIN64) && defined(_MSC_VER)
      #define NK_POINTER_TYPE unsigned __int64
    #elif (defined(_WIN32) || defined(WIN32)) && defined(_MSC_VER)
      #define NK_POINTER_TYPE unsigned __int32
    #elif defined(__GNUC__) || defined(__clang__)
      #if defined(__x86_64__) || defined(__ppc64__)
        #define NK_POINTER_TYPE unsigned long
      #else
        #define NK_POINTER_TYPE unsigned int
      #endif
    #else
      #define NK_POINTER_TYPE unsigned long
    #endif
  #endif
#endif

typedef NK_INT8 nk_char;
typedef NK_UINT8 nk_uchar;
typedef NK_UINT8 nk_byte;
typedef NK_INT16 nk_short;
typedef NK_UINT16 nk_ushort;
typedef NK_INT32 nk_int;
typedef NK_UINT32 nk_uint;
typedef NK_SIZE_TYPE nk_size;
typedef NK_POINTER_TYPE nk_ptr;

typedef nk_uint nk_hash;
typedef nk_uint nk_flags;
typedef nk_uint nk_rune;

/* Make sure correct type size:
 * This will fire with a negative subscript error if the type sizes
 * are set incorrectly by the compiler, and compile out if not */
NK_STATIC_ASSERT(sizeof(nk_short) == 2);
NK_STATIC_ASSERT(sizeof(nk_ushort) == 2);
NK_STATIC_ASSERT(sizeof(nk_uint) == 4);
NK_STATIC_ASSERT(sizeof(nk_int) == 4);
NK_STATIC_ASSERT(sizeof(nk_byte) == 1);
NK_STATIC_ASSERT(sizeof(nk_flags) >= 4);
NK_STATIC_ASSERT(sizeof(nk_rune) >= 4);
NK_STATIC_ASSERT(sizeof(nk_size) >= sizeof(void*));
NK_STATIC_ASSERT(sizeof(nk_ptr) >= sizeof(void*));

/* ============================================================================
 *
 *                                  API
 *
 * =========================================================================== */
struct nk_buffer;
struct nk_allocator;
struct nk_command_buffer;
struct nk_draw_command;
struct nk_convert_config;
struct nk_style_item;
struct nk_text_edit;
struct nk_draw_list;
struct nk_user_font;
struct nk_panel;
struct nk_context;
struct nk_draw_vertex_layout_element;
struct nk_style_button;
struct nk_style_toggle;
struct nk_style_selectable;
struct nk_style_slide;
struct nk_style_progress;
struct nk_style_scrollbar;
struct nk_style_edit;
struct nk_style_property;
struct nk_style_chart;
struct nk_style_combo;
struct nk_style_tab;
struct nk_style_window_header;
struct nk_style_window;

enum {nk_false, nk_true};
struct nk_color {nk_byte r,g,b,a;};
struct nk_colorf {float r,g,b,a;};
struct nk_vec2 {float x,y;};
struct nk_vec2i {short x, y;};
struct nk_rect {float x,y,w,h;};
struct nk_recti {short x,y,w,h;};
typedef char nk_glyph[NK_UTF_SIZE];
typedef union {void *ptr; int id;} nk_handle;
struct nk_image {nk_handle handle;unsigned short w,h;unsigned short region[4];};
struct nk_cursor {struct nk_image img; struct nk_vec2 size, offset;};
struct nk_scroll {nk_uint x, y;};

enum nk_heading         {NK_UP, NK_RIGHT, NK_DOWN, NK_LEFT};
enum nk_button_behavior {NK_BUTTON_DEFAULT, NK_BUTTON_REPEATER};
enum nk_modify          {NK_FIXED = nk_false, NK_MODIFIABLE = nk_true};
enum nk_orientation     {NK_VERTICAL, NK_HORIZONTAL};
enum nk_collapse_states {NK_MINIMIZED = nk_false, NK_MAXIMIZED = nk_true};
enum nk_show_states     {NK_HIDDEN = nk_false, NK_SHOWN = nk_true};
enum nk_chart_type      {NK_CHART_LINES, NK_CHART_COLUMN, NK_CHART_MAX};
enum nk_chart_event     {NK_CHART_HOVERING = 0x01, NK_CHART_CLICKED = 0x02};
enum nk_color_format    {NK_RGB, NK_RGBA};
enum nk_popup_type      {NK_POPUP_STATIC, NK_POPUP_DYNAMIC};
enum nk_layout_format   {NK_DYNAMIC, NK_STATIC};
enum nk_tree_type       {NK_TREE_NODE, NK_TREE_TAB};

typedef void*(*nk_plugin_alloc)(nk_handle, void *old, nk_size);
typedef void (*nk_plugin_free)(nk_handle, void *old);
typedef int(*nk_plugin_filter)(const struct nk_text_edit*, nk_rune unicode);
typedef void(*nk_plugin_paste)(nk_handle, struct nk_text_edit*);
typedef void(*nk_plugin_copy)(nk_handle, const char*, int len);

struct nk_allocator {
    nk_handle userdata;
    nk_plugin_alloc alloc;
    nk_plugin_free free;
};
enum nk_symbol_type {
    NK_SYMBOL_NONE,
    NK_SYMBOL_X,
    NK_SYMBOL_UNDERSCORE,
    NK_SYMBOL_CIRCLE_SOLID,
    NK_SYMBOL_CIRCLE_OUTLINE,
    NK_SYMBOL_RECT_SOLID,
    NK_SYMBOL_RECT_OUTLINE,
    NK_SYMBOL_TRIANGLE_UP,
    NK_SYMBOL_TRIANGLE_DOWN,
    NK_SYMBOL_TRIANGLE_LEFT,
    NK_SYMBOL_TRIANGLE_RIGHT,
    NK_SYMBOL_PLUS,
    NK_SYMBOL_MINUS,
    NK_SYMBOL_MAX
};
/* =============================================================================
 *
 *                                  CONTEXT
 *
 * =============================================================================*/
/*/// ### Context
/// Contexts are the main entry point and the majestro of nuklear and contain all required state.
/// They are used for window, memory, input, style, stack, commands and time management and need
/// to be passed into all nuklear GUI specific functions.
///
/// #### Usage
/// To use a context it first has to be initialized which can be achieved by calling
/// one of either `nk_init_default`, `nk_init_fixed`, `nk_init`, `nk_init_custom`.
/// Each takes in a font handle and a specific way of handling memory. Memory control
/// hereby ranges from standard library to just specifying a fixed sized block of memory
/// which nuklear has to manage itself from.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// struct nk_context ctx;
/// nk_init_xxx(&ctx, ...);
/// while (1) {
///     // [...]
///     nk_clear(&ctx);
/// }
/// nk_free(&ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// #### Reference
/// Function            | Description
/// --------------------|-------------------------------------------------------
/// __nk_init_default__ | Initializes context with standard library memory allocation (malloc,free)
/// __nk_init_fixed__   | Initializes context from single fixed size memory block
/// __nk_init__         | Initializes context with memory allocator callbacks for alloc and free
/// __nk_init_custom__  | Initializes context from two buffers. One for draw commands the other for window/panel/table allocations
/// __nk_clear__        | Called at the end of the frame to reset and prepare the context for the next frame
/// __nk_free__         | Shutdown and free all memory allocated inside the context
/// __nk_set_user_data__| Utility function to pass user data to draw command
 */
#ifdef NK_INCLUDE_DEFAULT_ALLOCATOR
/*/// #### nk_init_default
/// Initializes a `nk_context` struct with a default standard library allocator.
/// Should be used if you don't want to be bothered with memory management in nuklear.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_init_default(struct nk_context *ctx, const struct nk_user_font *font);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|---------------------------------------------------------------
/// __ctx__     | Must point to an either stack or heap allocated `nk_context` struct
/// __font__    | Must point to a previously initialized font handle for more info look at font documentation
///
/// Returns either `false(0)` on failure or `true(1)` on success.
///
*/
NK_API int nk_init_default(struct nk_context*, const struct nk_user_font*);
#endif
/*/// #### nk_init_fixed
/// Initializes a `nk_context` struct from single fixed size memory block
/// Should be used if you want complete control over nuklear's memory management.
/// Especially recommended for system with little memory or systems with virtual memory.
/// For the later case you can just allocate for example 16MB of virtual memory
/// and only the required amount of memory will actually be committed.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_init_fixed(struct nk_context *ctx, void *memory, nk_size size, const struct nk_user_font *font);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// !!! Warning
///     make sure the passed memory block is aligned correctly for `nk_draw_commands`.
///
/// Parameter   | Description
/// ------------|--------------------------------------------------------------
/// __ctx__     | Must point to an either stack or heap allocated `nk_context` struct
/// __memory__  | Must point to a previously allocated memory block
/// __size__    | Must contain the total size of __memory__
/// __font__    | Must point to a previously initialized font handle for more info look at font documentation
///
/// Returns either `false(0)` on failure or `true(1)` on success.
*/
NK_API int nk_init_fixed(struct nk_context*, void *memory, nk_size size, const struct nk_user_font*);
/*/// #### nk_init
/// Initializes a `nk_context` struct with memory allocation callbacks for nuklear to allocate
/// memory from. Used internally for `nk_init_default` and provides a kitchen sink allocation
/// interface to nuklear. Can be useful for cases like monitoring memory consumption.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_init(struct nk_context *ctx, struct nk_allocator *alloc, const struct nk_user_font *font);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|---------------------------------------------------------------
/// __ctx__     | Must point to an either stack or heap allocated `nk_context` struct
/// __alloc__   | Must point to a previously allocated memory allocator
/// __font__    | Must point to a previously initialized font handle for more info look at font documentation
///
/// Returns either `false(0)` on failure or `true(1)` on success.
*/
NK_API int nk_init(struct nk_context*, struct nk_allocator*, const struct nk_user_font*);
/*/// #### nk_init_custom
/// Initializes a `nk_context` struct from two different either fixed or growing
/// buffers. The first buffer is for allocating draw commands while the second buffer is
/// used for allocating windows, panels and state tables.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_init_custom(struct nk_context *ctx, struct nk_buffer *cmds, struct nk_buffer *pool, const struct nk_user_font *font);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|---------------------------------------------------------------
/// __ctx__     | Must point to an either stack or heap allocated `nk_context` struct
/// __cmds__    | Must point to a previously initialized memory buffer either fixed or dynamic to store draw commands into
/// __pool__    | Must point to a previously initialized memory buffer either fixed or dynamic to store windows, panels and tables
/// __font__    | Must point to a previously initialized font handle for more info look at font documentation
///
/// Returns either `false(0)` on failure or `true(1)` on success.
*/
NK_API int nk_init_custom(struct nk_context*, struct nk_buffer *cmds, struct nk_buffer *pool, const struct nk_user_font*);
/*/// #### nk_clear
/// Resets the context state at the end of the frame. This includes mostly
/// garbage collector tasks like removing windows or table not called and therefore
/// used anymore.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_clear(struct nk_context *ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to a previously initialized `nk_context` struct
*/
NK_API void nk_clear(struct nk_context*);
/*/// #### nk_free
/// Frees all memory allocated by nuklear. Not needed if context was
/// initialized with `nk_init_fixed`.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_free(struct nk_context *ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to a previously initialized `nk_context` struct
*/
NK_API void nk_free(struct nk_context*);
#ifdef NK_INCLUDE_COMMAND_USERDATA
/*/// #### nk_set_user_data
/// Sets the currently passed userdata passed down into each draw command.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_set_user_data(struct nk_context *ctx, nk_handle data);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|--------------------------------------------------------------
/// __ctx__     | Must point to a previously initialized `nk_context` struct
/// __data__    | Handle with either pointer or index to be passed into every draw commands
*/
NK_API void nk_set_user_data(struct nk_context*, nk_handle handle);
#endif
/* =============================================================================
 *
 *                                  INPUT
 *
 * =============================================================================*/
/*/// ### Input
/// The input API is responsible for holding the current input state composed of
/// mouse, key and text input states.
/// It is worth noting that no direct OS or window handling is done in nuklear.
/// Instead all input state has to be provided by platform specific code. This on one hand
/// expects more work from the user and complicates usage but on the other hand
/// provides simple abstraction over a big number of platforms, libraries and other
/// already provided functionality.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// nk_input_begin(&ctx);
/// while (GetEvent(&evt)) {
///     if (evt.type == MOUSE_MOVE)
///         nk_input_motion(&ctx, evt.motion.x, evt.motion.y);
///     else if (evt.type == [...]) {
///         // [...]
///     }
/// } nk_input_end(&ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// #### Usage
/// Input state needs to be provided to nuklear by first calling `nk_input_begin`
/// which resets internal state like delta mouse position and button transistions.
/// After `nk_input_begin` all current input state needs to be provided. This includes
/// mouse motion, button and key pressed and released, text input and scrolling.
/// Both event- or state-based input handling are supported by this API
/// and should work without problems. Finally after all input state has been
/// mirrored `nk_input_end` needs to be called to finish input process.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// struct nk_context ctx;
/// nk_init_xxx(&ctx, ...);
/// while (1) {
///     Event evt;
///     nk_input_begin(&ctx);
///     while (GetEvent(&evt)) {
///         if (evt.type == MOUSE_MOVE)
///             nk_input_motion(&ctx, evt.motion.x, evt.motion.y);
///         else if (evt.type == [...]) {
///             // [...]
///         }
///     }
///     nk_input_end(&ctx);
///     // [...]
///     nk_clear(&ctx);
/// } nk_free(&ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// #### Reference
/// Function            | Description
/// --------------------|-------------------------------------------------------
/// __nk_input_begin__  | Begins the input mirroring process. Needs to be called before all other `nk_input_xxx` calls
/// __nk_input_motion__ | Mirrors mouse cursor position
/// __nk_input_key__    | Mirrors key state with either pressed or released
/// __nk_input_button__ | Mirrors mouse button state with either pressed or released
/// __nk_input_scroll__ | Mirrors mouse scroll values
/// __nk_input_char__   | Adds a single ASCII text character into an internal text buffer
/// __nk_input_glyph__  | Adds a single multi-byte UTF-8 character into an internal text buffer
/// __nk_input_unicode__| Adds a single unicode rune into an internal text buffer
/// __nk_input_end__    | Ends the input mirroring process by calculating state changes. Don't call any `nk_input_xxx` function referenced above after this call
*/
enum nk_keys {
    NK_KEY_NONE,
    NK_KEY_SHIFT,
    NK_KEY_CTRL,
    NK_KEY_DEL,
    NK_KEY_ENTER,
    NK_KEY_TAB,
    NK_KEY_BACKSPACE,
    NK_KEY_COPY,
    NK_KEY_CUT,
    NK_KEY_PASTE,
    NK_KEY_UP,
    NK_KEY_DOWN,
    NK_KEY_LEFT,
    NK_KEY_RIGHT,
    /* Shortcuts: text field */
    NK_KEY_TEXT_INSERT_MODE,
    NK_KEY_TEXT_REPLACE_MODE,
    NK_KEY_TEXT_RESET_MODE,
    NK_KEY_TEXT_LINE_START,
    NK_KEY_TEXT_LINE_END,
    NK_KEY_TEXT_START,
    NK_KEY_TEXT_END,
    NK_KEY_TEXT_UNDO,
    NK_KEY_TEXT_REDO,
    NK_KEY_TEXT_SELECT_ALL,
    NK_KEY_TEXT_WORD_LEFT,
    NK_KEY_TEXT_WORD_RIGHT,
    /* Shortcuts: scrollbar */
    NK_KEY_SCROLL_START,
    NK_KEY_SCROLL_END,
    NK_KEY_SCROLL_DOWN,
    NK_KEY_SCROLL_UP,
    NK_KEY_MAX
};
enum nk_buttons {
    NK_BUTTON_LEFT,
    NK_BUTTON_MIDDLE,
    NK_BUTTON_RIGHT,
    NK_BUTTON_DOUBLE,
    NK_BUTTON_MAX
};
/*/// #### nk_input_begin
/// Begins the input mirroring process by resetting text, scroll
/// mouse, previous mouse position and movement as well as key state transitions,
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_input_begin(struct nk_context*);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to a previously initialized `nk_context` struct
*/
NK_API void nk_input_begin(struct nk_context*);
/*/// #### nk_input_motion
/// Mirrors current mouse position to nuklear
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_input_motion(struct nk_context *ctx, int x, int y);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to a previously initialized `nk_context` struct
/// __x__       | Must hold an integer describing the current mouse cursor x-position
/// __y__       | Must hold an integer describing the current mouse cursor y-position
*/
NK_API void nk_input_motion(struct nk_context*, int x, int y);
/*/// #### nk_input_key
/// Mirrors the state of a specific key to nuklear
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_input_key(struct nk_context*, enum nk_keys key, int down);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to a previously initialized `nk_context` struct
/// __key__     | Must be any value specified in enum `nk_keys` that needs to be mirrored
/// __down__    | Must be 0 for key is up and 1 for key is down
*/
NK_API void nk_input_key(struct nk_context*, enum nk_keys, int down);
/*/// #### nk_input_button
/// Mirrors the state of a specific mouse button to nuklear
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_input_button(struct nk_context *ctx, enum nk_buttons btn, int x, int y, int down);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to a previously initialized `nk_context` struct
/// __btn__     | Must be any value specified in enum `nk_buttons` that needs to be mirrored
/// __x__       | Must contain an integer describing mouse cursor x-position on click up/down
/// __y__       | Must contain an integer describing mouse cursor y-position on click up/down
/// __down__    | Must be 0 for key is up and 1 for key is down
*/
NK_API void nk_input_button(struct nk_context*, enum nk_buttons, int x, int y, int down);
/*/// #### nk_input_scroll
/// Copies the last mouse scroll value to nuklear. Is generally
/// a scroll value. So does not have to come from mouse and could also originate
/// TODO finish this sentence
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_input_scroll(struct nk_context *ctx, struct nk_vec2 val);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to a previously initialized `nk_context` struct
/// __val__     | vector with both X- as well as Y-scroll value
*/
NK_API void nk_input_scroll(struct nk_context*, struct nk_vec2 val);
/*/// #### nk_input_char
/// Copies a single ASCII character into an internal text buffer
/// This is basically a helper function to quickly push ASCII characters into
/// nuklear.
///
/// !!! Note
///     Stores up to NK_INPUT_MAX bytes between `nk_input_begin` and `nk_input_end`.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_input_char(struct nk_context *ctx, char c);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to a previously initialized `nk_context` struct
/// __c__       | Must be a single ASCII character preferable one that can be printed
*/
NK_API void nk_input_char(struct nk_context*, char);
/*/// #### nk_input_glyph
/// Converts an encoded unicode rune into UTF-8 and copies the result into an
/// internal text buffer.
///
/// !!! Note
///     Stores up to NK_INPUT_MAX bytes between `nk_input_begin` and `nk_input_end`.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_input_glyph(struct nk_context *ctx, const nk_glyph g);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to a previously initialized `nk_context` struct
/// __g__       | UTF-32 unicode codepoint
*/
NK_API void nk_input_glyph(struct nk_context*, const nk_glyph);
/*/// #### nk_input_unicode
/// Converts a unicode rune into UTF-8 and copies the result
/// into an internal text buffer.
/// !!! Note
///     Stores up to NK_INPUT_MAX bytes between `nk_input_begin` and `nk_input_end`.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_input_unicode(struct nk_context*, nk_rune rune);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to a previously initialized `nk_context` struct
/// __rune__    | UTF-32 unicode codepoint
*/
NK_API void nk_input_unicode(struct nk_context*, nk_rune);
/*/// #### nk_input_end
/// End the input mirroring process by resetting mouse grabbing
/// state to ensure the mouse cursor is not grabbed indefinitely.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_input_end(struct nk_context *ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to a previously initialized `nk_context` struct
*/
NK_API void nk_input_end(struct nk_context*);
/* =============================================================================
 *
 *                                  DRAWING
 *
 * =============================================================================*/
/*/// ### Drawing
/// This library was designed to be render backend agnostic so it does
/// not draw anything to screen directly. Instead all drawn shapes, widgets
/// are made of, are buffered into memory and make up a command queue.
/// Each frame therefore fills the command buffer with draw commands
/// that then need to be executed by the user and his own render backend.
/// After that the command buffer needs to be cleared and a new frame can be
/// started. It is probably important to note that the command buffer is the main
/// drawing API and the optional vertex buffer API only takes this format and
/// converts it into a hardware accessible format.
///
/// #### Usage
/// To draw all draw commands accumulated over a frame you need your own render
/// backend able to draw a number of 2D primitives. This includes at least
/// filled and stroked rectangles, circles, text, lines, triangles and scissors.
/// As soon as this criterion is met you can iterate over each draw command
/// and execute each draw command in a interpreter like fashion:
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// const struct nk_command *cmd = 0;
/// nk_foreach(cmd, &ctx) {
///     switch (cmd->type) {
///     case NK_COMMAND_LINE:
///         your_draw_line_function(...)
///         break;
///     case NK_COMMAND_RECT
///         your_draw_rect_function(...)
///         break;
///     case //...:
///         //[...]
///     }
/// }
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// In program flow context draw commands need to be executed after input has been
/// gathered and the complete UI with windows and their contained widgets have
/// been executed and before calling `nk_clear` which frees all previously
/// allocated draw commands.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// struct nk_context ctx;
/// nk_init_xxx(&ctx, ...);
/// while (1) {
///     Event evt;
///     nk_input_begin(&ctx);
///     while (GetEvent(&evt)) {
///         if (evt.type == MOUSE_MOVE)
///             nk_input_motion(&ctx, evt.motion.x, evt.motion.y);
///         else if (evt.type == [...]) {
///             [...]
///         }
///     }
///     nk_input_end(&ctx);
///     //
///     // [...]
///     //
///     const struct nk_command *cmd = 0;
///     nk_foreach(cmd, &ctx) {
///     switch (cmd->type) {
///     case NK_COMMAND_LINE:
///         your_draw_line_function(...)
///         break;
///     case NK_COMMAND_RECT
///         your_draw_rect_function(...)
///         break;
///     case ...:
///         // [...]
///     }
///     nk_clear(&ctx);
/// }
/// nk_free(&ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// You probably noticed that you have to draw all of the UI each frame which is
/// quite wasteful. While the actual UI updating loop is quite fast rendering
/// without actually needing it is not. So there are multiple things you could do.
///
/// First is only update on input. This of course is only an option if your
/// application only depends on the UI and does not require any outside calculations.
/// If you actually only update on input make sure to update the UI two times each
/// frame and call `nk_clear` directly after the first pass and only draw in
/// the second pass. In addition it is recommended to also add additional timers
/// to make sure the UI is not drawn more than a fixed number of frames per second.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// struct nk_context ctx;
/// nk_init_xxx(&ctx, ...);
/// while (1) {
///     // [...wait for input ]
///     // [...do two UI passes ...]
///     do_ui(...)
///     nk_clear(&ctx);
///     do_ui(...)
///     //
///     // draw
///     const struct nk_command *cmd = 0;
///     nk_foreach(cmd, &ctx) {
///     switch (cmd->type) {
///     case NK_COMMAND_LINE:
///         your_draw_line_function(...)
///         break;
///     case NK_COMMAND_RECT
///         your_draw_rect_function(...)
///         break;
///     case ...:
///         //[...]
///     }
///     nk_clear(&ctx);
/// }
/// nk_free(&ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// The second probably more applicable trick is to only draw if anything changed.
/// It is not really useful for applications with continuous draw loop but
/// quite useful for desktop applications. To actually get nuklear to only
/// draw on changes you first have to define `NK_ZERO_COMMAND_MEMORY` and
/// allocate a memory buffer that will store each unique drawing output.
/// After each frame you compare the draw command memory inside the library
/// with your allocated buffer by memcmp. If memcmp detects differences
/// you have to copy the command buffer into the allocated buffer
/// and then draw like usual (this example uses fixed memory but you could
/// use dynamically allocated memory).
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// //[... other defines ...]
/// #define NK_ZERO_COMMAND_MEMORY
/// #include "nuklear.h"
/// //
/// // setup context
/// struct nk_context ctx;
/// void *last = calloc(1,64*1024);
/// void *buf = calloc(1,64*1024);
/// nk_init_fixed(&ctx, buf, 64*1024);
/// //
/// // loop
/// while (1) {
///     // [...input...]
///     // [...ui...]
///     void *cmds = nk_buffer_memory(&ctx.memory);
///     if (memcmp(cmds, last, ctx.memory.allocated)) {
///         memcpy(last,cmds,ctx.memory.allocated);
///         const struct nk_command *cmd = 0;
///         nk_foreach(cmd, &ctx) {
///             switch (cmd->type) {
///             case NK_COMMAND_LINE:
///                 your_draw_line_function(...)
///                 break;
///             case NK_COMMAND_RECT
///                 your_draw_rect_function(...)
///                 break;
///             case ...:
///                 // [...]
///             }
///         }
///     }
///     nk_clear(&ctx);
/// }
/// nk_free(&ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Finally while using draw commands makes sense for higher abstracted platforms like
/// X11 and Win32 or drawing libraries it is often desirable to use graphics
/// hardware directly. Therefore it is possible to just define
/// `NK_INCLUDE_VERTEX_BUFFER_OUTPUT` which includes optional vertex output.
/// To access the vertex output you first have to convert all draw commands into
/// vertexes by calling `nk_convert` which takes in your preferred vertex format.
/// After successfully converting all draw commands just iterate over and execute all
/// vertex draw commands:
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// // fill configuration
/// struct your_vertex
/// {
///     float pos[2]; // important to keep it to 2 floats
///     float uv[2];
///     unsigned char col[4];
/// };
/// struct nk_convert_config cfg = {};
/// static const struct nk_draw_vertex_layout_element vertex_layout[] = {
///     {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct your_vertex, pos)},
///     {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct your_vertex, uv)},
///     {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct your_vertex, col)},
///     {NK_VERTEX_LAYOUT_END}
/// };
/// cfg.shape_AA = NK_ANTI_ALIASING_ON;
/// cfg.line_AA = NK_ANTI_ALIASING_ON;
/// cfg.vertex_layout = vertex_layout;
/// cfg.vertex_size = sizeof(struct your_vertex);
/// cfg.vertex_alignment = NK_ALIGNOF(struct your_vertex);
/// cfg.circle_segment_count = 22;
/// cfg.curve_segment_count = 22;
/// cfg.arc_segment_count = 22;
/// cfg.global_alpha = 1.0f;
/// cfg.null = dev->null;
/// //
/// // setup buffers and convert
/// struct nk_buffer cmds, verts, idx;
/// nk_buffer_init_default(&cmds);
/// nk_buffer_init_default(&verts);
/// nk_buffer_init_default(&idx);
/// nk_convert(&ctx, &cmds, &verts, &idx, &cfg);
/// //
/// // draw
/// nk_draw_foreach(cmd, &ctx, &cmds) {
/// if (!cmd->elem_count) continue;
///     //[...]
/// }
/// nk_buffer_free(&cms);
/// nk_buffer_free(&verts);
/// nk_buffer_free(&idx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// #### Reference
/// Function            | Description
/// --------------------|-------------------------------------------------------
/// __nk__begin__       | Returns the first draw command in the context draw command list to be drawn
/// __nk__next__        | Increments the draw command iterator to the next command inside the context draw command list
/// __nk_foreach__      | Iterates over each draw command inside the context draw command list
/// __nk_convert__      | Converts from the abstract draw commands list into a hardware accessible vertex format
/// __nk_draw_begin__   | Returns the first vertex command in the context vertex draw list to be executed
/// __nk__draw_next__   | Increments the vertex command iterator to the next command inside the context vertex command list
/// __nk__draw_end__    | Returns the end of the vertex draw list
/// __nk_draw_foreach__ | Iterates over each vertex draw command inside the vertex draw list
*/
enum nk_anti_aliasing {NK_ANTI_ALIASING_OFF, NK_ANTI_ALIASING_ON};
enum nk_convert_result {
    NK_CONVERT_SUCCESS = 0,
    NK_CONVERT_INVALID_PARAM = 1,
    NK_CONVERT_COMMAND_BUFFER_FULL = NK_FLAG(1),
    NK_CONVERT_VERTEX_BUFFER_FULL = NK_FLAG(2),
    NK_CONVERT_ELEMENT_BUFFER_FULL = NK_FLAG(3)
};
struct nk_draw_null_texture {
    nk_handle texture; /* texture handle to a texture with a white pixel */
    struct nk_vec2 uv; /* coordinates to a white pixel in the texture  */
};
struct nk_convert_config {
    float global_alpha; /* global alpha value */
    enum nk_anti_aliasing line_AA; /* line anti-aliasing flag can be turned off if you are tight on memory */
    enum nk_anti_aliasing shape_AA; /* shape anti-aliasing flag can be turned off if you are tight on memory */
    unsigned circle_segment_count; /* number of segments used for circles: default to 22 */
    unsigned arc_segment_count; /* number of segments used for arcs: default to 22 */
    unsigned curve_segment_count; /* number of segments used for curves: default to 22 */
    struct nk_draw_null_texture null; /* handle to texture with a white pixel for shape drawing */
    const struct nk_draw_vertex_layout_element *vertex_layout; /* describes the vertex output format and packing */
    nk_size vertex_size; /* sizeof one vertex for vertex packing */
    nk_size vertex_alignment; /* vertex alignment: Can be obtained by NK_ALIGNOF */
};
/*/// #### nk__begin
/// Returns a draw command list iterator to iterate all draw
/// commands accumulated over one frame.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// const struct nk_command* nk__begin(struct nk_context*);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | must point to an previously initialized `nk_context` struct at the end of a frame
///
/// Returns draw command pointer pointing to the first command inside the draw command list
*/
NK_API const struct nk_command* nk__begin(struct nk_context*);
/*/// #### nk__next
/// Returns draw command pointer pointing to the next command inside the draw command list
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// const struct nk_command* nk__next(struct nk_context*, const struct nk_command*);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct at the end of a frame
/// __cmd__     | Must point to an previously a draw command either returned by `nk__begin` or `nk__next`
///
/// Returns draw command pointer pointing to the next command inside the draw command list
*/
NK_API const struct nk_command* nk__next(struct nk_context*, const struct nk_command*);
/*/// #### nk_foreach
/// Iterates over each draw command inside the context draw command list
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// #define nk_foreach(c, ctx)
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct at the end of a frame
/// __cmd__     | Command pointer initialized to NULL
///
/// Iterates over each draw command inside the context draw command list
*/
#define nk_foreach(c, ctx) for((c) = nk__begin(ctx); (c) != 0; (c) = nk__next(ctx,c))
#ifdef NK_INCLUDE_VERTEX_BUFFER_OUTPUT
/*/// #### nk_convert
/// Converts all internal draw commands into vertex draw commands and fills
/// three buffers with vertexes, vertex draw commands and vertex indices. The vertex format
/// as well as some other configuration values have to be configured by filling out a
/// `nk_convert_config` struct.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// nk_flags nk_convert(struct nk_context *ctx, struct nk_buffer *cmds,
///     struct nk_buffer *vertices, struct nk_buffer *elements, const struct nk_convert_config*);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct at the end of a frame
/// __cmds__    | Must point to a previously initialized buffer to hold converted vertex draw commands
/// __vertices__| Must point to a previously initialized buffer to hold all produced vertices
/// __elements__| Must point to a previously initialized buffer to hold all produced vertex indices
/// __config__  | Must point to a filled out `nk_config` struct to configure the conversion process
///
/// Returns one of enum nk_convert_result error codes
///
/// Parameter                       | Description
/// --------------------------------|-----------------------------------------------------------
/// NK_CONVERT_SUCCESS              | Signals a successful draw command to vertex buffer conversion
/// NK_CONVERT_INVALID_PARAM        | An invalid argument was passed in the function call
/// NK_CONVERT_COMMAND_BUFFER_FULL  | The provided buffer for storing draw commands is full or failed to allocate more memory
/// NK_CONVERT_VERTEX_BUFFER_FULL   | The provided buffer for storing vertices is full or failed to allocate more memory
/// NK_CONVERT_ELEMENT_BUFFER_FULL  | The provided buffer for storing indicies is full or failed to allocate more memory
*/
NK_API nk_flags nk_convert(struct nk_context*, struct nk_buffer *cmds, struct nk_buffer *vertices, struct nk_buffer *elements, const struct nk_convert_config*);
/*/// #### nk__draw_begin
/// Returns a draw vertex command buffer iterator to iterate over the vertex draw command buffer
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// const struct nk_draw_command* nk__draw_begin(const struct nk_context*, const struct nk_buffer*);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct at the end of a frame
/// __buf__     | Must point to an previously by `nk_convert` filled out vertex draw command buffer
///
/// Returns vertex draw command pointer pointing to the first command inside the vertex draw command buffer
*/
NK_API const struct nk_draw_command* nk__draw_begin(const struct nk_context*, const struct nk_buffer*);
/*/// #### nk__draw_end
/// Returns the vertex draw command at the end of the vertex draw command buffer
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// const struct nk_draw_command* nk__draw_end(const struct nk_context *ctx, const struct nk_buffer *buf);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct at the end of a frame
/// __buf__     | Must point to an previously by `nk_convert` filled out vertex draw command buffer
///
/// Returns vertex draw command pointer pointing to the end of the last vertex draw command inside the vertex draw command buffer
*/
NK_API const struct nk_draw_command* nk__draw_end(const struct nk_context*, const struct nk_buffer*);
/*/// #### nk__draw_next
/// Increments the vertex draw command buffer iterator
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// const struct nk_draw_command* nk__draw_next(const struct nk_draw_command*, const struct nk_buffer*, const struct nk_context*);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __cmd__     | Must point to an previously either by `nk__draw_begin` or `nk__draw_next` returned vertex draw command
/// __buf__     | Must point to an previously by `nk_convert` filled out vertex draw command buffer
/// __ctx__     | Must point to an previously initialized `nk_context` struct at the end of a frame
///
/// Returns vertex draw command pointer pointing to the end of the last vertex draw command inside the vertex draw command buffer
*/
NK_API const struct nk_draw_command* nk__draw_next(const struct nk_draw_command*, const struct nk_buffer*, const struct nk_context*);
/*/// #### nk_draw_foreach
/// Iterates over each vertex draw command inside a vertex draw command buffer
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// #define nk_draw_foreach(cmd,ctx, b)
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __cmd__     | `nk_draw_command`iterator set to NULL
/// __buf__     | Must point to an previously by `nk_convert` filled out vertex draw command buffer
/// __ctx__     | Must point to an previously initialized `nk_context` struct at the end of a frame
*/
#define nk_draw_foreach(cmd,ctx, b) for((cmd)=nk__draw_begin(ctx, b); (cmd)!=0; (cmd)=nk__draw_next(cmd, b, ctx))
#endif
/* =============================================================================
 *
 *                                  WINDOW
 *
 * =============================================================================
/// ### Window
/// Windows are the main persistent state used inside nuklear and are life time
/// controlled by simply "retouching" (i.e. calling) each window each frame.
/// All widgets inside nuklear can only be added inside the function pair `nk_begin_xxx`
/// and `nk_end`. Calling any widgets outside these two functions will result in an
/// assert in debug or no state change in release mode.<br /><br />
///
/// Each window holds frame persistent state like position, size, flags, state tables,
/// and some garbage collected internal persistent widget state. Each window
/// is linked into a window stack list which determines the drawing and overlapping
/// order. The topmost window thereby is the currently active window.<br /><br />
///
/// To change window position inside the stack occurs either automatically by
/// user input by being clicked on or programmatically by calling `nk_window_focus`.
/// Windows by default are visible unless explicitly being defined with flag
/// `NK_WINDOW_HIDDEN`, the user clicked the close button on windows with flag
/// `NK_WINDOW_CLOSABLE` or if a window was explicitly hidden by calling
/// `nk_window_show`. To explicitly close and destroy a window call `nk_window_close`.<br /><br />
///
/// #### Usage
/// To create and keep a window you have to call one of the two `nk_begin_xxx`
/// functions to start window declarations and `nk_end` at the end. Furthermore it
/// is recommended to check the return value of `nk_begin_xxx` and only process
/// widgets inside the window if the value is not 0. Either way you have to call
/// `nk_end` at the end of window declarations. Furthermore, do not attempt to
/// nest `nk_begin_xxx` calls which will hopefully result in an assert or if not
/// in a segmentation fault.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// if (nk_begin_xxx(...) {
///     // [... widgets ...]
/// }
/// nk_end(ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// In the grand concept window and widget declarations need to occur after input
/// handling and before drawing to screen. Not doing so can result in higher
/// latency or at worst invalid behavior. Furthermore make sure that `nk_clear`
/// is called at the end of the frame. While nuklear's default platform backends
/// already call `nk_clear` for you if you write your own backend not calling
/// `nk_clear` can cause asserts or even worse undefined behavior.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// struct nk_context ctx;
/// nk_init_xxx(&ctx, ...);
/// while (1) {
///     Event evt;
///     nk_input_begin(&ctx);
///     while (GetEvent(&evt)) {
///         if (evt.type == MOUSE_MOVE)
///             nk_input_motion(&ctx, evt.motion.x, evt.motion.y);
///         else if (evt.type == [...]) {
///             nk_input_xxx(...);
///         }
///     }
///     nk_input_end(&ctx);
///
///     if (nk_begin_xxx(...) {
///         //[...]
///     }
///     nk_end(ctx);
///
///     const struct nk_command *cmd = 0;
///     nk_foreach(cmd, &ctx) {
///     case NK_COMMAND_LINE:
///         your_draw_line_function(...)
///         break;
///     case NK_COMMAND_RECT
///         your_draw_rect_function(...)
///         break;
///     case //...:
///         //[...]
///     }
///     nk_clear(&ctx);
/// }
/// nk_free(&ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// #### Reference
/// Function                            | Description
/// ------------------------------------|----------------------------------------
/// nk_begin                            | Starts a new window; needs to be called every frame for every window (unless hidden) or otherwise the window gets removed
/// nk_begin_titled                     | Extended window start with separated title and identifier to allow multiple windows with same name but not title
/// nk_end                              | Needs to be called at the end of the window building process to process scaling, scrollbars and general cleanup
//
/// nk_window_find                      | Finds and returns the window with give name
/// nk_window_get_bounds                | Returns a rectangle with screen position and size of the currently processed window.
/// nk_window_get_position              | Returns the position of the currently processed window
/// nk_window_get_size                  | Returns the size with width and height of the currently processed window
/// nk_window_get_width                 | Returns the width of the currently processed window
/// nk_window_get_height                | Returns the height of the currently processed window
/// nk_window_get_panel                 | Returns the underlying panel which contains all processing state of the current window
/// nk_window_get_content_region        | Returns the position and size of the currently visible and non-clipped space inside the currently processed window
/// nk_window_get_content_region_min    | Returns the upper rectangle position of the currently visible and non-clipped space inside the currently processed window
/// nk_window_get_content_region_max    | Returns the upper rectangle position of the currently visible and non-clipped space inside the currently processed window
/// nk_window_get_content_region_size   | Returns the size of the currently visible and non-clipped space inside the currently processed window
/// nk_window_get_canvas                | Returns the draw command buffer. Can be used to draw custom widgets
/// nk_window_get_scroll                | Gets the scroll offset of the current window
/// nk_window_has_focus                 | Returns if the currently processed window is currently active
/// nk_window_is_collapsed              | Returns if the window with given name is currently minimized/collapsed
/// nk_window_is_closed                 | Returns if the currently processed window was closed
/// nk_window_is_hidden                 | Returns if the currently processed window was hidden
/// nk_window_is_active                 | Same as nk_window_has_focus for some reason
/// nk_window_is_hovered                | Returns if the currently processed window is currently being hovered by mouse
/// nk_window_is_any_hovered            | Return if any window currently hovered
/// nk_item_is_any_active               | Returns if any window or widgets is currently hovered or active
//
/// nk_window_set_bounds                | Updates position and size of the currently processed window
/// nk_window_set_position              | Updates position of the currently process window
/// nk_window_set_size                  | Updates the size of the currently processed window
/// nk_window_set_focus                 | Set the currently processed window as active window
/// nk_window_set_scroll                | Sets the scroll offset of the current window
//
/// nk_window_close                     | Closes the window with given window name which deletes the window at the end of the frame
/// nk_window_collapse                  | Collapses the window with given window name
/// nk_window_collapse_if               | Collapses the window with given window name if the given condition was met
/// nk_window_show                      | Hides a visible or reshows a hidden window
/// nk_window_show_if                   | Hides/shows a window depending on condition
*/
/*
/// #### nk_panel_flags
/// Flag                        | Description
/// ----------------------------|----------------------------------------
/// NK_WINDOW_BORDER            | Draws a border around the window to visually separate window from the background
/// NK_WINDOW_MOVABLE           | The movable flag indicates that a window can be moved by user input or by dragging the window header
/// NK_WINDOW_SCALABLE          | The scalable flag indicates that a window can be scaled by user input by dragging a scaler icon at the button of the window
/// NK_WINDOW_CLOSABLE          | Adds a closable icon into the header
/// NK_WINDOW_MINIMIZABLE       | Adds a minimize icon into the header
/// NK_WINDOW_NO_SCROLLBAR      | Removes the scrollbar from the window
/// NK_WINDOW_TITLE             | Forces a header at the top at the window showing the title
/// NK_WINDOW_SCROLL_AUTO_HIDE  | Automatically hides the window scrollbar if no user interaction: also requires delta time in `nk_context` to be set each frame
/// NK_WINDOW_BACKGROUND        | Always keep window in the background
/// NK_WINDOW_SCALE_LEFT        | Puts window scaler in the left-bottom corner instead right-bottom
/// NK_WINDOW_NO_INPUT          | Prevents window of scaling, moving or getting focus
///
/// #### nk_collapse_states
/// State           | Description
/// ----------------|-----------------------------------------------------------
/// __NK_MINIMIZED__| UI section is collased and not visibile until maximized
/// __NK_MAXIMIZED__| UI section is extended and visibile until minimized
/// <br /><br />
*/
enum nk_panel_flags {
    NK_WINDOW_BORDER            = NK_FLAG(0),
    NK_WINDOW_MOVABLE           = NK_FLAG(1),
    NK_WINDOW_SCALABLE          = NK_FLAG(2),
    NK_WINDOW_CLOSABLE          = NK_FLAG(3),
    NK_WINDOW_MINIMIZABLE       = NK_FLAG(4),
    NK_WINDOW_NO_SCROLLBAR      = NK_FLAG(5),
    NK_WINDOW_TITLE             = NK_FLAG(6),
    NK_WINDOW_SCROLL_AUTO_HIDE  = NK_FLAG(7),
    NK_WINDOW_BACKGROUND        = NK_FLAG(8),
    NK_WINDOW_SCALE_LEFT        = NK_FLAG(9),
    NK_WINDOW_NO_INPUT          = NK_FLAG(10)
};
/*/// #### nk_begin
/// Starts a new window; needs to be called every frame for every
/// window (unless hidden) or otherwise the window gets removed
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_begin(struct nk_context *ctx, const char *title, struct nk_rect bounds, nk_flags flags);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __title__   | Window title and identifier. Needs to be persistent over frames to identify the window
/// __bounds__  | Initial position and window size. However if you do not define `NK_WINDOW_SCALABLE` or `NK_WINDOW_MOVABLE` you can set window position and size every frame
/// __flags__   | Window flags defined in the nk_panel_flags section with a number of different window behaviors
///
/// Returns `true(1)` if the window can be filled up with widgets from this point
/// until `nk_end` or `false(0)` otherwise for example if minimized
*/
NK_API int nk_begin(struct nk_context *ctx, const char *title, struct nk_rect bounds, nk_flags flags);
/*/// #### nk_begin_titled
/// Extended window start with separated title and identifier to allow multiple
/// windows with same title but not name
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_begin_titled(struct nk_context *ctx, const char *name, const char *title, struct nk_rect bounds, nk_flags flags);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __name__    | Window identifier. Needs to be persistent over frames to identify the window
/// __title__   | Window title displayed inside header if flag `NK_WINDOW_TITLE` or either `NK_WINDOW_CLOSABLE` or `NK_WINDOW_MINIMIZED` was set
/// __bounds__  | Initial position and window size. However if you do not define `NK_WINDOW_SCALABLE` or `NK_WINDOW_MOVABLE` you can set window position and size every frame
/// __flags__   | Window flags defined in the nk_panel_flags section with a number of different window behaviors
///
/// Returns `true(1)` if the window can be filled up with widgets from this point
/// until `nk_end` or `false(0)` otherwise for example if minimized
*/
NK_API int nk_begin_titled(struct nk_context *ctx, const char *name, const char *title, struct nk_rect bounds, nk_flags flags);
/*/// #### nk_end
/// Needs to be called at the end of the window building process to process scaling, scrollbars and general cleanup.
/// All widget calls after this functions will result in asserts or no state changes
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_end(struct nk_context *ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
*/
NK_API void nk_end(struct nk_context *ctx);
/*/// #### nk_window_find
/// Finds and returns a window from passed name
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// struct nk_window *nk_window_find(struct nk_context *ctx, const char *name);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __name__    | Window identifier
///
/// Returns a `nk_window` struct pointing to the identified window or NULL if
/// no window with the given name was found
*/
NK_API struct nk_window *nk_window_find(struct nk_context *ctx, const char *name);
/*/// #### nk_window_get_bounds
/// Returns a rectangle with screen position and size of the currently processed window
///
/// !!! WARNING
///     Only call this function between calls `nk_begin_xxx` and `nk_end`
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// struct nk_rect nk_window_get_bounds(const struct nk_context *ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
///
/// Returns a `nk_rect` struct with window upper left window position and size
*/
NK_API struct nk_rect nk_window_get_bounds(const struct nk_context *ctx);
/*/// #### nk_window_get_position
/// Returns the position of the currently processed window.
///
/// !!! WARNING
///     Only call this function between calls `nk_begin_xxx` and `nk_end`
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// struct nk_vec2 nk_window_get_position(const struct nk_context *ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
///
/// Returns a `nk_vec2` struct with window upper left position
*/
NK_API struct nk_vec2 nk_window_get_position(const struct nk_context *ctx);
/*/// #### nk_window_get_size
/// Returns the size with width and height of the currently processed window.
///
/// !!! WARNING
///     Only call this function between calls `nk_begin_xxx` and `nk_end`
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// struct nk_vec2 nk_window_get_size(const struct nk_context *ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
///
/// Returns a `nk_vec2` struct with window width and height
*/
NK_API struct nk_vec2 nk_window_get_size(const struct nk_context*);
/*/// #### nk_window_get_width
/// Returns the width of the currently processed window.
///
/// !!! WARNING
///     Only call this function between calls `nk_begin_xxx` and `nk_end`
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// float nk_window_get_width(const struct nk_context *ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
///
/// Returns the current window width
*/
NK_API float nk_window_get_width(const struct nk_context*);
/*/// #### nk_window_get_height
/// Returns the height of the currently processed window.
///
/// !!! WARNING
///     Only call this function between calls `nk_begin_xxx` and `nk_end`
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// float nk_window_get_height(const struct nk_context *ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
///
/// Returns the current window height
*/
NK_API float nk_window_get_height(const struct nk_context*);
/*/// #### nk_window_get_panel
/// Returns the underlying panel which contains all processing state of the current window.
///
/// !!! WARNING
///     Only call this function between calls `nk_begin_xxx` and `nk_end`
/// !!! WARNING
///     Do not keep the returned panel pointer around, it is only valid until `nk_end`
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// struct nk_panel* nk_window_get_panel(struct nk_context *ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
///
/// Returns a pointer to window internal `nk_panel` state.
*/
NK_API struct nk_panel* nk_window_get_panel(struct nk_context*);
/*/// #### nk_window_get_content_region
/// Returns the position and size of the currently visible and non-clipped space
/// inside the currently processed window.
///
/// !!! WARNING
///     Only call this function between calls `nk_begin_xxx` and `nk_end`
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// struct nk_rect nk_window_get_content_region(struct nk_context *ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
///
/// Returns `nk_rect` struct with screen position and size (no scrollbar offset)
/// of the visible space inside the current window
*/
NK_API struct nk_rect nk_window_get_content_region(struct nk_context*);
/*/// #### nk_window_get_content_region_min
/// Returns the upper left position of the currently visible and non-clipped
/// space inside the currently processed window.
///
/// !!! WARNING
///     Only call this function between calls `nk_begin_xxx` and `nk_end`
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// struct nk_vec2 nk_window_get_content_region_min(struct nk_context *ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
///
/// returns `nk_vec2` struct with  upper left screen position (no scrollbar offset)
/// of the visible space inside the current window
*/
NK_API struct nk_vec2 nk_window_get_content_region_min(struct nk_context*);
/*/// #### nk_window_get_content_region_max
/// Returns the lower right screen position of the currently visible and
/// non-clipped space inside the currently processed window.
///
/// !!! WARNING
///     Only call this function between calls `nk_begin_xxx` and `nk_end`
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// struct nk_vec2 nk_window_get_content_region_max(struct nk_context *ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
///
/// Returns `nk_vec2` struct with lower right screen position (no scrollbar offset)
/// of the visible space inside the current window
*/
NK_API struct nk_vec2 nk_window_get_content_region_max(struct nk_context*);
/*/// #### nk_window_get_content_region_size
/// Returns the size of the currently visible and non-clipped space inside the
/// currently processed window
///
/// !!! WARNING
///     Only call this function between calls `nk_begin_xxx` and `nk_end`
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// struct nk_vec2 nk_window_get_content_region_size(struct nk_context *ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
///
/// Returns `nk_vec2` struct with size the visible space inside the current window
*/
NK_API struct nk_vec2 nk_window_get_content_region_size(struct nk_context*);
/*/// #### nk_window_get_canvas
/// Returns the draw command buffer. Can be used to draw custom widgets
/// !!! WARNING
///     Only call this function between calls `nk_begin_xxx` and `nk_end`
/// !!! WARNING
///     Do not keep the returned command buffer pointer around it is only valid until `nk_end`
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// struct nk_command_buffer* nk_window_get_canvas(struct nk_context *ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
///
/// Returns a pointer to window internal `nk_command_buffer` struct used as
/// drawing canvas. Can be used to do custom drawing.
*/
NK_API struct nk_command_buffer* nk_window_get_canvas(struct nk_context*);
/*/// #### nk_window_get_scroll
/// Gets the scroll offset for the current window
/// !!! WARNING
///     Only call this function between calls `nk_begin_xxx` and `nk_end`
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_window_get_scroll(struct nk_context *ctx, nk_uint *offset_x, nk_uint *offset_y);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter    | Description
/// -------------|-----------------------------------------------------------
/// __ctx__      | Must point to an previously initialized `nk_context` struct
/// __offset_x__ | A pointer to the x offset output (or NULL to ignore)
/// __offset_y__ | A pointer to the y offset output (or NULL to ignore)
*/
NK_API void nk_window_get_scroll(struct nk_context*, nk_uint *offset_x, nk_uint *offset_y);
/*/// #### nk_window_has_focus
/// Returns if the currently processed window is currently active
/// !!! WARNING
///     Only call this function between calls `nk_begin_xxx` and `nk_end`
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_window_has_focus(const struct nk_context *ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
///
/// Returns `false(0)` if current window is not active or `true(1)` if it is
*/
NK_API int nk_window_has_focus(const struct nk_context*);
/*/// #### nk_window_is_hovered
/// Return if the current window is being hovered
/// !!! WARNING
///     Only call this function between calls `nk_begin_xxx` and `nk_end`
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_window_is_hovered(struct nk_context *ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
///
/// Returns `true(1)` if current window is hovered or `false(0)` otherwise
*/
NK_API int nk_window_is_hovered(struct nk_context*);
/*/// #### nk_window_is_collapsed
/// Returns if the window with given name is currently minimized/collapsed
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_window_is_collapsed(struct nk_context *ctx, const char *name);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __name__    | Identifier of window you want to check if it is collapsed
///
/// Returns `true(1)` if current window is minimized and `false(0)` if window not
/// found or is not minimized
*/
NK_API int nk_window_is_collapsed(struct nk_context *ctx, const char *name);
/*/// #### nk_window_is_closed
/// Returns if the window with given name was closed by calling `nk_close`
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_window_is_closed(struct nk_context *ctx, const char *name);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __name__    | Identifier of window you want to check if it is closed
///
/// Returns `true(1)` if current window was closed or `false(0)` window not found or not closed
*/
NK_API int nk_window_is_closed(struct nk_context*, const char*);
/*/// #### nk_window_is_hidden
/// Returns if the window with given name is hidden
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_window_is_hidden(struct nk_context *ctx, const char *name);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __name__    | Identifier of window you want to check if it is hidden
///
/// Returns `true(1)` if current window is hidden or `false(0)` window not found or visible
*/
NK_API int nk_window_is_hidden(struct nk_context*, const char*);
/*/// #### nk_window_is_active
/// Same as nk_window_has_focus for some reason
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_window_is_active(struct nk_context *ctx, const char *name);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __name__    | Identifier of window you want to check if it is active
///
/// Returns `true(1)` if current window is active or `false(0)` window not found or not active
*/
NK_API int nk_window_is_active(struct nk_context*, const char*);
/*/// #### nk_window_is_any_hovered
/// Returns if the any window is being hovered
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_window_is_any_hovered(struct nk_context*);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
///
/// Returns `true(1)` if any window is hovered or `false(0)` otherwise
*/
NK_API int nk_window_is_any_hovered(struct nk_context*);
/*/// #### nk_item_is_any_active
/// Returns if the any window is being hovered or any widget is currently active.
/// Can be used to decide if input should be processed by UI or your specific input handling.
/// Example could be UI and 3D camera to move inside a 3D space.
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_item_is_any_active(struct nk_context*);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
///
/// Returns `true(1)` if any window is hovered or any item is active or `false(0)` otherwise
*/
NK_API int nk_item_is_any_active(struct nk_context*);
/*/// #### nk_window_set_bounds
/// Updates position and size of window with passed in name
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_window_set_bounds(struct nk_context*, const char *name, struct nk_rect bounds);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __name__    | Identifier of the window to modify both position and size
/// __bounds__  | Must point to a `nk_rect` struct with the new position and size
*/
NK_API void nk_window_set_bounds(struct nk_context*, const char *name, struct nk_rect bounds);
/*/// #### nk_window_set_position
/// Updates position of window with passed name
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_window_set_position(struct nk_context*, const char *name, struct nk_vec2 pos);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __name__    | Identifier of the window to modify both position
/// __pos__     | Must point to a `nk_vec2` struct with the new position
*/
NK_API void nk_window_set_position(struct nk_context*, const char *name, struct nk_vec2 pos);
/*/// #### nk_window_set_size
/// Updates size of window with passed in name
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_window_set_size(struct nk_context*, const char *name, struct nk_vec2);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __name__    | Identifier of the window to modify both window size
/// __size__    | Must point to a `nk_vec2` struct with new window size
*/
NK_API void nk_window_set_size(struct nk_context*, const char *name, struct nk_vec2);
/*/// #### nk_window_set_focus
/// Sets the window with given name as active
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_window_set_focus(struct nk_context*, const char *name);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __name__    | Identifier of the window to set focus on
*/
NK_API void nk_window_set_focus(struct nk_context*, const char *name);
/*/// #### nk_window_set_scroll
/// Sets the scroll offset for the current window
/// !!! WARNING
///     Only call this function between calls `nk_begin_xxx` and `nk_end`
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_window_set_scroll(struct nk_context *ctx, nk_uint offset_x, nk_uint offset_y);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter    | Description
/// -------------|-----------------------------------------------------------
/// __ctx__      | Must point to an previously initialized `nk_context` struct
/// __offset_x__ | The x offset to scroll to
/// __offset_y__ | The y offset to scroll to
*/
NK_API void nk_window_set_scroll(struct nk_context*, nk_uint offset_x, nk_uint offset_y);
/*/// #### nk_window_close
/// Closes a window and marks it for being freed at the end of the frame
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_window_close(struct nk_context *ctx, const char *name);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __name__    | Identifier of the window to close
*/
NK_API void nk_window_close(struct nk_context *ctx, const char *name);
/*/// #### nk_window_collapse
/// Updates collapse state of a window with given name
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_window_collapse(struct nk_context*, const char *name, enum nk_collapse_states state);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __name__    | Identifier of the window to close
/// __state__   | value out of nk_collapse_states section
*/
NK_API void nk_window_collapse(struct nk_context*, const char *name, enum nk_collapse_states state);
/*/// #### nk_window_collapse_if
/// Updates collapse state of a window with given name if given condition is met
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_window_collapse_if(struct nk_context*, const char *name, enum nk_collapse_states, int cond);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __name__    | Identifier of the window to either collapse or maximize
/// __state__   | value out of nk_collapse_states section the window should be put into
/// __cond__    | condition that has to be met to actually commit the collapse state change
*/
NK_API void nk_window_collapse_if(struct nk_context*, const char *name, enum nk_collapse_states, int cond);
/*/// #### nk_window_show
/// updates visibility state of a window with given name
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_window_show(struct nk_context*, const char *name, enum nk_show_states);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __name__    | Identifier of the window to either collapse or maximize
/// __state__   | state with either visible or hidden to modify the window with
*/
NK_API void nk_window_show(struct nk_context*, const char *name, enum nk_show_states);
/*/// #### nk_window_show_if
/// Updates visibility state of a window with given name if a given condition is met
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_window_show_if(struct nk_context*, const char *name, enum nk_show_states, int cond);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __name__    | Identifier of the window to either hide or show
/// __state__   | state with either visible or hidden to modify the window with
/// __cond__    | condition that has to be met to actually commit the visbility state change
*/
NK_API void nk_window_show_if(struct nk_context*, const char *name, enum nk_show_states, int cond);
/* =============================================================================
 *
 *                                  LAYOUT
 *
 * =============================================================================
/// ### Layouting
/// Layouting in general describes placing widget inside a window with position and size.
/// While in this particular implementation there are five different APIs for layouting
/// each with different trade offs between control and ease of use. <br /><br />
///
/// All layouting methods in this library are based around the concept of a row.
/// A row has a height the window content grows by and a number of columns and each
/// layouting method specifies how each widget is placed inside the row.
/// After a row has been allocated by calling a layouting functions and then
/// filled with widgets will advance an internal pointer over the allocated row. <br /><br />
///
/// To actually define a layout you just call the appropriate layouting function
/// and each subsequent widget call will place the widget as specified. Important
/// here is that if you define more widgets then columns defined inside the layout
/// functions it will allocate the next row without you having to make another layouting <br /><br />
/// call.
///
/// Biggest limitation with using all these APIs outside the `nk_layout_space_xxx` API
/// is that you have to define the row height for each. However the row height
/// often depends on the height of the font. <br /><br />
///
/// To fix that internally nuklear uses a minimum row height that is set to the
/// height plus padding of currently active font and overwrites the row height
/// value if zero. <br /><br />
///
/// If you manually want to change the minimum row height then
/// use nk_layout_set_min_row_height, and use nk_layout_reset_min_row_height to
/// reset it back to be derived from font height. <br /><br />
///
/// Also if you change the font in nuklear it will automatically change the minimum
/// row height for you and. This means if you change the font but still want
/// a minimum row height smaller than the font you have to repush your value. <br /><br />
///
/// For actually more advanced UI I would even recommend using the `nk_layout_space_xxx`
/// layouting method in combination with a cassowary constraint solver (there are
/// some versions on github with permissive license model) to take over all control over widget
/// layouting yourself. However for quick and dirty layouting using all the other layouting
/// functions should be fine.
///
/// #### Usage
/// 1.  __nk_layout_row_dynamic__<br /><br />
///     The easiest layouting function is `nk_layout_row_dynamic`. It provides each
///     widgets with same horizontal space inside the row and dynamically grows
///     if the owning window grows in width. So the number of columns dictates
///     the size of each widget dynamically by formula:
///
///     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
///     widget_width = (window_width - padding - spacing) * (1/colum_count)
///     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
///     Just like all other layouting APIs if you define more widget than columns this
///     library will allocate a new row and keep all layouting parameters previously
///     defined.
///
///     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
///     if (nk_begin_xxx(...) {
///         // first row with height: 30 composed of two widgets
///         nk_layout_row_dynamic(&ctx, 30, 2);
///         nk_widget(...);
///         nk_widget(...);
///         //
///         // second row with same parameter as defined above
///         nk_widget(...);
///         nk_widget(...);
///         //
///         // third row uses 0 for height which will use auto layouting
///         nk_layout_row_dynamic(&ctx, 0, 2);
///         nk_widget(...);
///         nk_widget(...);
///     }
///     nk_end(...);
///     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// 2.  __nk_layout_row_static__<br /><br />
///     Another easy layouting function is `nk_layout_row_static`. It provides each
///     widget with same horizontal pixel width inside the row and does not grow
///     if the owning window scales smaller or bigger.
///
///     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
///     if (nk_begin_xxx(...) {
///         // first row with height: 30 composed of two widgets with width: 80
///         nk_layout_row_static(&ctx, 30, 80, 2);
///         nk_widget(...);
///         nk_widget(...);
///         //
///         // second row with same parameter as defined above
///         nk_widget(...);
///         nk_widget(...);
///         //
///         // third row uses 0 for height which will use auto layouting
///         nk_layout_row_static(&ctx, 0, 80, 2);
///         nk_widget(...);
///         nk_widget(...);
///     }
///     nk_end(...);
///     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// 3.  __nk_layout_row_xxx__<br /><br />
///     A little bit more advanced layouting API are functions `nk_layout_row_begin`,
///     `nk_layout_row_push` and `nk_layout_row_end`. They allow to directly
///     specify each column pixel or window ratio in a row. It supports either
///     directly setting per column pixel width or widget window ratio but not
///     both. Furthermore it is a immediate mode API so each value is directly
///     pushed before calling a widget. Therefore the layout is not automatically
///     repeating like the last two layouting functions.
///
///     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
///     if (nk_begin_xxx(...) {
///         // first row with height: 25 composed of two widgets with width 60 and 40
///         nk_layout_row_begin(ctx, NK_STATIC, 25, 2);
///         nk_layout_row_push(ctx, 60);
///         nk_widget(...);
///         nk_layout_row_push(ctx, 40);
///         nk_widget(...);
///         nk_layout_row_end(ctx);
///         //
///         // second row with height: 25 composed of two widgets with window ratio 0.25 and 0.75
///         nk_layout_row_begin(ctx, NK_DYNAMIC, 25, 2);
///         nk_layout_row_push(ctx, 0.25f);
///         nk_widget(...);
///         nk_layout_row_push(ctx, 0.75f);
///         nk_widget(...);
///         nk_layout_row_end(ctx);
///         //
///         // third row with auto generated height: composed of two widgets with window ratio 0.25 and 0.75
///         nk_layout_row_begin(ctx, NK_DYNAMIC, 0, 2);
///         nk_layout_row_push(ctx, 0.25f);
///         nk_widget(...);
///         nk_layout_row_push(ctx, 0.75f);
///         nk_widget(...);
///         nk_layout_row_end(ctx);
///     }
///     nk_end(...);
///     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// 4.  __nk_layout_row__<br /><br />
///     The array counterpart to API nk_layout_row_xxx is the single nk_layout_row
///     functions. Instead of pushing either pixel or window ratio for every widget
///     it allows to define it by array. The trade of for less control is that
///     `nk_layout_row` is automatically repeating. Otherwise the behavior is the
///     same.
///
///     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
///     if (nk_begin_xxx(...) {
///         // two rows with height: 30 composed of two widgets with width 60 and 40
///         const float size[] = {60,40};
///         nk_layout_row(ctx, NK_STATIC, 30, 2, ratio);
///         nk_widget(...);
///         nk_widget(...);
///         nk_widget(...);
///         nk_widget(...);
///         //
///         // two rows with height: 30 composed of two widgets with window ratio 0.25 and 0.75
///         const float ratio[] = {0.25, 0.75};
///         nk_layout_row(ctx, NK_DYNAMIC, 30, 2, ratio);
///         nk_widget(...);
///         nk_widget(...);
///         nk_widget(...);
///         nk_widget(...);
///         //
///         // two rows with auto generated height composed of two widgets with window ratio 0.25 and 0.75
///         const float ratio[] = {0.25, 0.75};
///         nk_layout_row(ctx, NK_DYNAMIC, 30, 2, ratio);
///         nk_widget(...);
///         nk_widget(...);
///         nk_widget(...);
///         nk_widget(...);
///     }
///     nk_end(...);
///     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// 5.  __nk_layout_row_template_xxx__<br /><br />
///     The most complex and second most flexible API is a simplified flexbox version without
///     line wrapping and weights for dynamic widgets. It is an immediate mode API but
///     unlike `nk_layout_row_xxx` it has auto repeat behavior and needs to be called
///     before calling the templated widgets.
///     The row template layout has three different per widget size specifier. The first
///     one is the `nk_layout_row_template_push_static`  with fixed widget pixel width.
///     They do not grow if the row grows and will always stay the same.
///     The second size specifier is `nk_layout_row_template_push_variable`
///     which defines a minimum widget size but it also can grow if more space is available
///     not taken by other widgets.
///     Finally there are dynamic widgets with `nk_layout_row_template_push_dynamic`
///     which are completely flexible and unlike variable widgets can even shrink
///     to zero if not enough space is provided.
///
///     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
///     if (nk_begin_xxx(...) {
///         // two rows with height: 30 composed of three widgets
///         nk_layout_row_template_begin(ctx, 30);
///         nk_layout_row_template_push_dynamic(ctx);
///         nk_layout_row_template_push_variable(ctx, 80);
///         nk_layout_row_template_push_static(ctx, 80);
///         nk_layout_row_template_end(ctx);
///         //
///         // first row
///         nk_widget(...); // dynamic widget can go to zero if not enough space
///         nk_widget(...); // variable widget with min 80 pixel but can grow bigger if enough space
///         nk_widget(...); // static widget with fixed 80 pixel width
///         //
///         // second row same layout
///         nk_widget(...);
///         nk_widget(...);
///         nk_widget(...);
///     }
///     nk_end(...);
///     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// 6.  __nk_layout_space_xxx__<br /><br />
///     Finally the most flexible API directly allows you to place widgets inside the
///     window. The space layout API is an immediate mode API which does not support
///     row auto repeat and directly sets position and size of a widget. Position
///     and size hereby can be either specified as ratio of allocated space or
///     allocated space local position and pixel size. Since this API is quite
///     powerful there are a number of utility functions to get the available space
///     and convert between local allocated space and screen space.
///
///     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
///     if (nk_begin_xxx(...) {
///         // static row with height: 500 (you can set column count to INT_MAX if you don't want to be bothered)
///         nk_layout_space_begin(ctx, NK_STATIC, 500, INT_MAX);
///         nk_layout_space_push(ctx, nk_rect(0,0,150,200));
///         nk_widget(...);
///         nk_layout_space_push(ctx, nk_rect(200,200,100,200));
///         nk_widget(...);
///         nk_layout_space_end(ctx);
///         //
///         // dynamic row with height: 500 (you can set column count to INT_MAX if you don't want to be bothered)
///         nk_layout_space_begin(ctx, NK_DYNAMIC, 500, INT_MAX);
///         nk_layout_space_push(ctx, nk_rect(0.5,0.5,0.1,0.1));
///         nk_widget(...);
///         nk_layout_space_push(ctx, nk_rect(0.7,0.6,0.1,0.1));
///         nk_widget(...);
///     }
///     nk_end(...);
///     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// #### Reference
/// Function                                | Description
/// ----------------------------------------|------------------------------------
/// nk_layout_set_min_row_height            | Set the currently used minimum row height to a specified value
/// nk_layout_reset_min_row_height          | Resets the currently used minimum row height to font height
/// nk_layout_widget_bounds                 | Calculates current width a static layout row can fit inside a window
/// nk_layout_ratio_from_pixel              | Utility functions to calculate window ratio from pixel size
//
/// nk_layout_row_dynamic                   | Current layout is divided into n same sized growing columns
/// nk_layout_row_static                    | Current layout is divided into n same fixed sized columns
/// nk_layout_row_begin                     | Starts a new row with given height and number of columns
/// nk_layout_row_push                      | Pushes another column with given size or window ratio
/// nk_layout_row_end                       | Finished previously started row
/// nk_layout_row                           | Specifies row columns in array as either window ratio or size
//
/// nk_layout_row_template_begin            | Begins the row template declaration
/// nk_layout_row_template_push_dynamic     | Adds a dynamic column that dynamically grows and can go to zero if not enough space
/// nk_layout_row_template_push_variable    | Adds a variable column that dynamically grows but does not shrink below specified pixel width
/// nk_layout_row_template_push_static      | Adds a static column that does not grow and will always have the same size
/// nk_layout_row_template_end              | Marks the end of the row template
//
/// nk_layout_space_begin                   | Begins a new layouting space that allows to specify each widgets position and size
/// nk_layout_space_push                    | Pushes position and size of the next widget in own coordinate space either as pixel or ratio
/// nk_layout_space_end                     | Marks the end of the layouting space
//
/// nk_layout_space_bounds                  | Callable after nk_layout_space_begin and returns total space allocated
/// nk_layout_space_to_screen               | Converts vector from nk_layout_space coordinate space into screen space
/// nk_layout_space_to_local                | Converts vector from screen space into nk_layout_space coordinates
/// nk_layout_space_rect_to_screen          | Converts rectangle from nk_layout_space coordinate space into screen space
/// nk_layout_space_rect_to_local           | Converts rectangle from screen space into nk_layout_space coordinates
*/
/*/// #### nk_layout_set_min_row_height
/// Sets the currently used minimum row height.
/// !!! WARNING
///     The passed height needs to include both your preferred row height
///     as well as padding. No internal padding is added.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_layout_set_min_row_height(struct nk_context*, float height);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_begin_xxx`
/// __height__  | New minimum row height to be used for auto generating the row height
*/
NK_API void nk_layout_set_min_row_height(struct nk_context*, float height);
/*/// #### nk_layout_reset_min_row_height
/// Reset the currently used minimum row height back to `font_height + text_padding + padding`
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_layout_reset_min_row_height(struct nk_context*);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_begin_xxx`
*/
NK_API void nk_layout_reset_min_row_height(struct nk_context*);
/*/// #### nk_layout_widget_bounds
/// Returns the width of the next row allocate by one of the layouting functions
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// struct nk_rect nk_layout_widget_bounds(struct nk_context*);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_begin_xxx`
///
/// Return `nk_rect` with both position and size of the next row
*/
NK_API struct nk_rect nk_layout_widget_bounds(struct nk_context*);
/*/// #### nk_layout_ratio_from_pixel
/// Utility functions to calculate window ratio from pixel size
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// float nk_layout_ratio_from_pixel(struct nk_context*, float pixel_width);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_begin_xxx`
/// __pixel__   | Pixel_width to convert to window ratio
///
/// Returns `nk_rect` with both position and size of the next row
*/
NK_API float nk_layout_ratio_from_pixel(struct nk_context*, float pixel_width);
/*/// #### nk_layout_row_dynamic
/// Sets current row layout to share horizontal space
/// between @cols number of widgets evenly. Once called all subsequent widget
/// calls greater than @cols will allocate a new row with same layout.
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_layout_row_dynamic(struct nk_context *ctx, float height, int cols);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_begin_xxx`
/// __height__  | Holds height of each widget in row or zero for auto layouting
/// __columns__ | Number of widget inside row
*/
NK_API void nk_layout_row_dynamic(struct nk_context *ctx, float height, int cols);
/*/// #### nk_layout_row_static
/// Sets current row layout to fill @cols number of widgets
/// in row with same @item_width horizontal size. Once called all subsequent widget
/// calls greater than @cols will allocate a new row with same layout.
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_layout_row_static(struct nk_context *ctx, float height, int item_width, int cols);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_begin_xxx`
/// __height__  | Holds height of each widget in row or zero for auto layouting
/// __width__   | Holds pixel width of each widget in the row
/// __columns__ | Number of widget inside row
*/
NK_API void nk_layout_row_static(struct nk_context *ctx, float height, int item_width, int cols);
/*/// #### nk_layout_row_begin
/// Starts a new dynamic or fixed row with given height and columns.
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_layout_row_begin(struct nk_context *ctx, enum nk_layout_format fmt, float row_height, int cols);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_begin_xxx`
/// __fmt__     | either `NK_DYNAMIC` for window ratio or `NK_STATIC` for fixed size columns
/// __height__  | holds height of each widget in row or zero for auto layouting
/// __columns__ | Number of widget inside row
*/
NK_API void nk_layout_row_begin(struct nk_context *ctx, enum nk_layout_format fmt, float row_height, int cols);
/*/// #### nk_layout_row_push
/// Specifies either window ratio or width of a single column
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_layout_row_push(struct nk_context*, float value);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_begin_xxx`
/// __value__   | either a window ratio or fixed width depending on @fmt in previous `nk_layout_row_begin` call
*/
NK_API void nk_layout_row_push(struct nk_context*, float value);
/*/// #### nk_layout_row_end
/// Finished previously started row
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_layout_row_end(struct nk_context*);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_begin_xxx`
*/
NK_API void nk_layout_row_end(struct nk_context*);
/*/// #### nk_layout_row
/// Specifies row columns in array as either window ratio or size
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_layout_row(struct nk_context*, enum nk_layout_format, float height, int cols, const float *ratio);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_begin_xxx`
/// __fmt__     | Either `NK_DYNAMIC` for window ratio or `NK_STATIC` for fixed size columns
/// __height__  | Holds height of each widget in row or zero for auto layouting
/// __columns__ | Number of widget inside row
*/
NK_API void nk_layout_row(struct nk_context*, enum nk_layout_format, float height, int cols, const float *ratio);
/*/// #### nk_layout_row_template_begin
/// Begins the row template declaration
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_layout_row_template_begin(struct nk_context*, float row_height);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_begin_xxx`
/// __height__  | Holds height of each widget in row or zero for auto layouting
*/
NK_API void nk_layout_row_template_begin(struct nk_context*, float row_height);
/*/// #### nk_layout_row_template_push_dynamic
/// Adds a dynamic column that dynamically grows and can go to zero if not enough space
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_layout_row_template_push_dynamic(struct nk_context*);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_begin_xxx`
/// __height__  | Holds height of each widget in row or zero for auto layouting
*/
NK_API void nk_layout_row_template_push_dynamic(struct nk_context*);
/*/// #### nk_layout_row_template_push_variable
/// Adds a variable column that dynamically grows but does not shrink below specified pixel width
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_layout_row_template_push_variable(struct nk_context*, float min_width);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_begin_xxx`
/// __width__   | Holds the minimum pixel width the next column must always be
*/
NK_API void nk_layout_row_template_push_variable(struct nk_context*, float min_width);
/*/// #### nk_layout_row_template_push_static
/// Adds a static column that does not grow and will always have the same size
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_layout_row_template_push_static(struct nk_context*, float width);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_begin_xxx`
/// __width__   | Holds the absolute pixel width value the next column must be
*/
NK_API void nk_layout_row_template_push_static(struct nk_context*, float width);
/*/// #### nk_layout_row_template_end
/// Marks the end of the row template
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_layout_row_template_end(struct nk_context*);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_begin_xxx`
*/
NK_API void nk_layout_row_template_end(struct nk_context*);
/*/// #### nk_layout_space_begin
/// Begins a new layouting space that allows to specify each widgets position and size.
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_layout_space_begin(struct nk_context*, enum nk_layout_format, float height, int widget_count);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_begin_xxx`
/// __fmt__     | Either `NK_DYNAMIC` for window ratio or `NK_STATIC` for fixed size columns
/// __height__  | Holds height of each widget in row or zero for auto layouting
/// __columns__ | Number of widgets inside row
*/
NK_API void nk_layout_space_begin(struct nk_context*, enum nk_layout_format, float height, int widget_count);
/*/// #### nk_layout_space_push
/// Pushes position and size of the next widget in own coordinate space either as pixel or ratio
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_layout_space_push(struct nk_context *ctx, struct nk_rect bounds);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_layout_space_begin`
/// __bounds__  | Position and size in laoyut space local coordinates
*/
NK_API void nk_layout_space_push(struct nk_context*, struct nk_rect bounds);
/*/// #### nk_layout_space_end
/// Marks the end of the layout space
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_layout_space_end(struct nk_context*);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_layout_space_begin`
*/
NK_API void nk_layout_space_end(struct nk_context*);
/*/// #### nk_layout_space_bounds
/// Utility function to calculate total space allocated for `nk_layout_space`
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// struct nk_rect nk_layout_space_bounds(struct nk_context*);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_layout_space_begin`
///
/// Returns `nk_rect` holding the total space allocated
*/
NK_API struct nk_rect nk_layout_space_bounds(struct nk_context*);
/*/// #### nk_layout_space_to_screen
/// Converts vector from nk_layout_space coordinate space into screen space
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// struct nk_vec2 nk_layout_space_to_screen(struct nk_context*, struct nk_vec2);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_layout_space_begin`
/// __vec__     | Position to convert from layout space into screen coordinate space
///
/// Returns transformed `nk_vec2` in screen space coordinates
*/
NK_API struct nk_vec2 nk_layout_space_to_screen(struct nk_context*, struct nk_vec2);
/*/// #### nk_layout_space_to_local
/// Converts vector from layout space into screen space
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// struct nk_vec2 nk_layout_space_to_local(struct nk_context*, struct nk_vec2);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_layout_space_begin`
/// __vec__     | Position to convert from screen space into layout coordinate space
///
/// Returns transformed `nk_vec2` in layout space coordinates
*/
NK_API struct nk_vec2 nk_layout_space_to_local(struct nk_context*, struct nk_vec2);
/*/// #### nk_layout_space_rect_to_screen
/// Converts rectangle from screen space into layout space
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// struct nk_rect nk_layout_space_rect_to_screen(struct nk_context*, struct nk_rect);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_layout_space_begin`
/// __bounds__  | Rectangle to convert from layout space into screen space
///
/// Returns transformed `nk_rect` in screen space coordinates
*/
NK_API struct nk_rect nk_layout_space_rect_to_screen(struct nk_context*, struct nk_rect);
/*/// #### nk_layout_space_rect_to_local
/// Converts rectangle from layout space into screen space
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// struct nk_rect nk_layout_space_rect_to_local(struct nk_context*, struct nk_rect);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after call `nk_layout_space_begin`
/// __bounds__  | Rectangle to convert from layout space into screen space
///
/// Returns transformed `nk_rect` in layout space coordinates
*/
NK_API struct nk_rect nk_layout_space_rect_to_local(struct nk_context*, struct nk_rect);
/* =============================================================================
 *
 *                                  GROUP
 *
 * =============================================================================
/// ### Groups
/// Groups are basically windows inside windows. They allow to subdivide space
/// in a window to layout widgets as a group. Almost all more complex widget
/// layouting requirements can be solved using groups and basic layouting
/// fuctionality. Groups just like windows are identified by an unique name and
/// internally keep track of scrollbar offsets by default. However additional
/// versions are provided to directly manage the scrollbar.
///
/// #### Usage
/// To create a group you have to call one of the three `nk_group_begin_xxx`
/// functions to start group declarations and `nk_group_end` at the end. Furthermore it
/// is required to check the return value of `nk_group_begin_xxx` and only process
/// widgets inside the window if the value is not 0.
/// Nesting groups is possible and even encouraged since many layouting schemes
/// can only be achieved by nesting. Groups, unlike windows, need `nk_group_end`
/// to be only called if the corosponding `nk_group_begin_xxx` call does not return 0:
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// if (nk_group_begin_xxx(ctx, ...) {
///     // [... widgets ...]
///     nk_group_end(ctx);
/// }
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// In the grand concept groups can be called after starting a window
/// with `nk_begin_xxx` and before calling `nk_end`:
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// struct nk_context ctx;
/// nk_init_xxx(&ctx, ...);
/// while (1) {
///     // Input
///     Event evt;
///     nk_input_begin(&ctx);
///     while (GetEvent(&evt)) {
///         if (evt.type == MOUSE_MOVE)
///             nk_input_motion(&ctx, evt.motion.x, evt.motion.y);
///         else if (evt.type == [...]) {
///             nk_input_xxx(...);
///         }
///     }
///     nk_input_end(&ctx);
///     //
///     // Window
///     if (nk_begin_xxx(...) {
///         // [...widgets...]
///         nk_layout_row_dynamic(...);
///         if (nk_group_begin_xxx(ctx, ...) {
///             //[... widgets ...]
///             nk_group_end(ctx);
///         }
///     }
///     nk_end(ctx);
///     //
///     // Draw
///     const struct nk_command *cmd = 0;
///     nk_foreach(cmd, &ctx) {
///     switch (cmd->type) {
///     case NK_COMMAND_LINE:
///         your_draw_line_function(...)
///         break;
///     case NK_COMMAND_RECT
///         your_draw_rect_function(...)
///         break;
///     case ...:
///         // [...]
///     }
///     nk_clear(&ctx);
/// }
/// nk_free(&ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
/// #### Reference
/// Function                        | Description
/// --------------------------------|-------------------------------------------
/// nk_group_begin                  | Start a new group with internal scrollbar handling
/// nk_group_begin_titled           | Start a new group with separeted name and title and internal scrollbar handling
/// nk_group_end                    | Ends a group. Should only be called if nk_group_begin returned non-zero
/// nk_group_scrolled_offset_begin  | Start a new group with manual separated handling of scrollbar x- and y-offset
/// nk_group_scrolled_begin         | Start a new group with manual scrollbar handling
/// nk_group_scrolled_end           | Ends a group with manual scrollbar handling. Should only be called if nk_group_begin returned non-zero
/// nk_group_get_scroll             | Gets the scroll offset for the given group
/// nk_group_set_scroll             | Sets the scroll offset for the given group
*/
/*/// #### nk_group_begin
/// Starts a new widget group. Requires a previous layouting function to specify a pos/size.
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_group_begin(struct nk_context*, const char *title, nk_flags);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __title__   | Must be an unique identifier for this group that is also used for the group header
/// __flags__   | Window flags defined in the nk_panel_flags section with a number of different group behaviors
///
/// Returns `true(1)` if visible and fillable with widgets or `false(0)` otherwise
*/
NK_API int nk_group_begin(struct nk_context*, const char *title, nk_flags);
/*/// #### nk_group_begin_titled
/// Starts a new widget group. Requires a previous layouting function to specify a pos/size.
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_group_begin_titled(struct nk_context*, const char *name, const char *title, nk_flags);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __id__      | Must be an unique identifier for this group
/// __title__   | Group header title
/// __flags__   | Window flags defined in the nk_panel_flags section with a number of different group behaviors
///
/// Returns `true(1)` if visible and fillable with widgets or `false(0)` otherwise
*/
NK_API int nk_group_begin_titled(struct nk_context*, const char *name, const char *title, nk_flags);
/*/// #### nk_group_end
/// Ends a widget group
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_group_end(struct nk_context*);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
*/
NK_API void nk_group_end(struct nk_context*);
/*/// #### nk_group_scrolled_offset_begin
/// starts a new widget group. requires a previous layouting function to specify
/// a size. Does not keep track of scrollbar.
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_group_scrolled_offset_begin(struct nk_context*, nk_uint *x_offset, nk_uint *y_offset, const char *title, nk_flags flags);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __x_offset__| Scrollbar x-offset to offset all widgets inside the group horizontally.
/// __y_offset__| Scrollbar y-offset to offset all widgets inside the group vertically
/// __title__   | Window unique group title used to both identify and display in the group header
/// __flags__   | Window flags from the nk_panel_flags section
///
/// Returns `true(1)` if visible and fillable with widgets or `false(0)` otherwise
*/
NK_API int nk_group_scrolled_offset_begin(struct nk_context*, nk_uint *x_offset, nk_uint *y_offset, const char *title, nk_flags flags);
/*/// #### nk_group_scrolled_begin
/// Starts a new widget group. requires a previous
/// layouting function to specify a size. Does not keep track of scrollbar.
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_group_scrolled_begin(struct nk_context*, struct nk_scroll *off, const char *title, nk_flags);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __off__     | Both x- and y- scroll offset. Allows for manual scrollbar control
/// __title__   | Window unique group title used to both identify and display in the group header
/// __flags__   | Window flags from nk_panel_flags section
///
/// Returns `true(1)` if visible and fillable with widgets or `false(0)` otherwise
*/
NK_API int nk_group_scrolled_begin(struct nk_context*, struct nk_scroll *off, const char *title, nk_flags);
/*/// #### nk_group_scrolled_end
/// Ends a widget group after calling nk_group_scrolled_offset_begin or nk_group_scrolled_begin.
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_group_scrolled_end(struct nk_context*);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
*/
NK_API void nk_group_scrolled_end(struct nk_context*);
/*/// #### nk_group_get_scroll
/// Gets the scroll position of the given group.
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_group_get_scroll(struct nk_context*, const char *id, nk_uint *x_offset, nk_uint *y_offset);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter    | Description
/// -------------|-----------------------------------------------------------
/// __ctx__      | Must point to an previously initialized `nk_context` struct
/// __id__       | The id of the group to get the scroll position of
/// __x_offset__ | A pointer to the x offset output (or NULL to ignore)
/// __y_offset__ | A pointer to the y offset output (or NULL to ignore)
*/
NK_API void nk_group_get_scroll(struct nk_context*, const char *id, nk_uint *x_offset, nk_uint *y_offset);
/*/// #### nk_group_set_scroll
/// Sets the scroll position of the given group.
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_group_set_scroll(struct nk_context*, const char *id, nk_uint x_offset, nk_uint y_offset);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter    | Description
/// -------------|-----------------------------------------------------------
/// __ctx__      | Must point to an previously initialized `nk_context` struct
/// __id__       | The id of the group to scroll
/// __x_offset__ | The x offset to scroll to
/// __y_offset__ | The y offset to scroll to
*/
NK_API void nk_group_set_scroll(struct nk_context*, const char *id, nk_uint x_offset, nk_uint y_offset);
/* =============================================================================
 *
 *                                  TREE
 *
 * =============================================================================
/// ### Tree
/// Trees represent two different concept. First the concept of a collapsable
/// UI section that can be either in a hidden or visibile state. They allow the UI
/// user to selectively minimize the current set of visible UI to comprehend.
/// The second concept are tree widgets for visual UI representation of trees.<br /><br />
///
/// Trees thereby can be nested for tree representations and multiple nested
/// collapsable UI sections. All trees are started by calling of the
/// `nk_tree_xxx_push_tree` functions and ended by calling one of the
/// `nk_tree_xxx_pop_xxx()` functions. Each starting functions takes a title label
/// and optionally an image to be displayed and the initial collapse state from
/// the nk_collapse_states section.<br /><br />
///
/// The runtime state of the tree is either stored outside the library by the caller
/// or inside which requires a unique ID. The unique ID can either be generated
/// automatically from `__FILE__` and `__LINE__` with function `nk_tree_push`,
/// by `__FILE__` and a user provided ID generated for example by loop index with
/// function `nk_tree_push_id` or completely provided from outside by user with
/// function `nk_tree_push_hashed`.
///
/// #### Usage
/// To create a tree you have to call one of the seven `nk_tree_xxx_push_xxx`
/// functions to start a collapsable UI section and `nk_tree_xxx_pop` to mark the
/// end.
/// Each starting function will either return `false(0)` if the tree is collapsed
/// or hidden and therefore does not need to be filled with content or `true(1)`
/// if visible and required to be filled.
///
/// !!! Note
///     The tree header does not require and layouting function and instead
///     calculates a auto height based on the currently used font size
///
/// The tree ending functions only need to be called if the tree content is
/// actually visible. So make sure the tree push function is guarded by `if`
/// and the pop call is only taken if the tree is visible.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// if (nk_tree_push(ctx, NK_TREE_TAB, "Tree", NK_MINIMIZED)) {
///     nk_layout_row_dynamic(...);
///     nk_widget(...);
///     nk_tree_pop(ctx);
/// }
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// #### Reference
/// Function                    | Description
/// ----------------------------|-------------------------------------------
/// nk_tree_push                | Start a collapsable UI section with internal state management
/// nk_tree_push_id             | Start a collapsable UI section with internal state management callable in a look
/// nk_tree_push_hashed         | Start a collapsable UI section with internal state management with full control over internal unique ID use to store state
/// nk_tree_image_push          | Start a collapsable UI section with image and label header
/// nk_tree_image_push_id       | Start a collapsable UI section with image and label header and internal state management callable in a look
/// nk_tree_image_push_hashed   | Start a collapsable UI section with image and label header and internal state management with full control over internal unique ID use to store state
/// nk_tree_pop                 | Ends a collapsable UI section
//
/// nk_tree_state_push          | Start a collapsable UI section with external state management
/// nk_tree_state_image_push    | Start a collapsable UI section with image and label header and external state management
/// nk_tree_state_pop           | Ends a collapsabale UI section
///
/// #### nk_tree_type
/// Flag            | Description
/// ----------------|----------------------------------------
/// NK_TREE_NODE    | Highlighted tree header to mark a collapsable UI section
/// NK_TREE_TAB     | Non-highighted tree header closer to tree representations
*/
/*/// #### nk_tree_push
/// Starts a collapsable UI section with internal state management
/// !!! WARNING
///     To keep track of the runtime tree collapsable state this function uses
///     defines `__FILE__` and `__LINE__` to generate a unique ID. If you want
///     to call this function in a loop please use `nk_tree_push_id` or
///     `nk_tree_push_hashed` instead.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// #define nk_tree_push(ctx, type, title, state)
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __type__    | Value from the nk_tree_type section to visually mark a tree node header as either a collapseable UI section or tree node
/// __title__   | Label printed in the tree header
/// __state__   | Initial tree state value out of nk_collapse_states
///
/// Returns `true(1)` if visible and fillable with widgets or `false(0)` otherwise
*/
#define nk_tree_push(ctx, type, title, state) nk_tree_push_hashed(ctx, type, title, state, NK_FILE_LINE,nk_strlen(NK_FILE_LINE),__LINE__)
/*/// #### nk_tree_push_id
/// Starts a collapsable UI section with internal state management callable in a look
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// #define nk_tree_push_id(ctx, type, title, state, id)
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __type__    | Value from the nk_tree_type section to visually mark a tree node header as either a collapseable UI section or tree node
/// __title__   | Label printed in the tree header
/// __state__   | Initial tree state value out of nk_collapse_states
/// __id__      | Loop counter index if this function is called in a loop
///
/// Returns `true(1)` if visible and fillable with widgets or `false(0)` otherwise
*/
#define nk_tree_push_id(ctx, type, title, state, id) nk_tree_push_hashed(ctx, type, title, state, NK_FILE_LINE,nk_strlen(NK_FILE_LINE),id)
/*/// #### nk_tree_push_hashed
/// Start a collapsable UI section with internal state management with full
/// control over internal unique ID used to store state
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_tree_push_hashed(struct nk_context*, enum nk_tree_type, const char *title, enum nk_collapse_states initial_state, const char *hash, int len,int seed);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __type__    | Value from the nk_tree_type section to visually mark a tree node header as either a collapseable UI section or tree node
/// __title__   | Label printed in the tree header
/// __state__   | Initial tree state value out of nk_collapse_states
/// __hash__    | Memory block or string to generate the ID from
/// __len__     | Size of passed memory block or string in __hash__
/// __seed__    | Seeding value if this function is called in a loop or default to `0`
///
/// Returns `true(1)` if visible and fillable with widgets or `false(0)` otherwise
*/
NK_API int nk_tree_push_hashed(struct nk_context*, enum nk_tree_type, const char *title, enum nk_collapse_states initial_state, const char *hash, int len,int seed);
/*/// #### nk_tree_image_push
/// Start a collapsable UI section with image and label header
/// !!! WARNING
///     To keep track of the runtime tree collapsable state this function uses
///     defines `__FILE__` and `__LINE__` to generate a unique ID. If you want
///     to call this function in a loop please use `nk_tree_image_push_id` or
///     `nk_tree_image_push_hashed` instead.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// #define nk_tree_image_push(ctx, type, img, title, state)
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
//
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __type__    | Value from the nk_tree_type section to visually mark a tree node header as either a collapseable UI section or tree node
/// __img__     | Image to display inside the header on the left of the label
/// __title__   | Label printed in the tree header
/// __state__   | Initial tree state value out of nk_collapse_states
///
/// Returns `true(1)` if visible and fillable with widgets or `false(0)` otherwise
*/
#define nk_tree_image_push(ctx, type, img, title, state) nk_tree_image_push_hashed(ctx, type, img, title, state, NK_FILE_LINE,nk_strlen(NK_FILE_LINE),__LINE__)
/*/// #### nk_tree_image_push_id
/// Start a collapsable UI section with image and label header and internal state
/// management callable in a look
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// #define nk_tree_image_push_id(ctx, type, img, title, state, id)
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __type__    | Value from the nk_tree_type section to visually mark a tree node header as either a collapseable UI section or tree node
/// __img__     | Image to display inside the header on the left of the label
/// __title__   | Label printed in the tree header
/// __state__   | Initial tree state value out of nk_collapse_states
/// __id__      | Loop counter index if this function is called in a loop
///
/// Returns `true(1)` if visible and fillable with widgets or `false(0)` otherwise
*/
#define nk_tree_image_push_id(ctx, type, img, title, state, id) nk_tree_image_push_hashed(ctx, type, img, title, state, NK_FILE_LINE,nk_strlen(NK_FILE_LINE),id)
/*/// #### nk_tree_image_push_hashed
/// Start a collapsable UI section with internal state management with full
/// control over internal unique ID used to store state
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_tree_image_push_hashed(struct nk_context*, enum nk_tree_type, struct nk_image, const char *title, enum nk_collapse_states initial_state, const char *hash, int len,int seed);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct
/// __type__    | Value from the nk_tree_type section to visually mark a tree node header as either a collapseable UI section or tree node
/// __img__     | Image to display inside the header on the left of the label
/// __title__   | Label printed in the tree header
/// __state__   | Initial tree state value out of nk_collapse_states
/// __hash__    | Memory block or string to generate the ID from
/// __len__     | Size of passed memory block or string in __hash__
/// __seed__    | Seeding value if this function is called in a loop or default to `0`
///
/// Returns `true(1)` if visible and fillable with widgets or `false(0)` otherwise
*/
NK_API int nk_tree_image_push_hashed(struct nk_context*, enum nk_tree_type, struct nk_image, const char *title, enum nk_collapse_states initial_state, const char *hash, int len,int seed);
/*/// #### nk_tree_pop
/// Ends a collapsabale UI section
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_tree_pop(struct nk_context*);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after calling `nk_tree_xxx_push_xxx`
*/
NK_API void nk_tree_pop(struct nk_context*);
/*/// #### nk_tree_state_push
/// Start a collapsable UI section with external state management
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_tree_state_push(struct nk_context*, enum nk_tree_type, const char *title, enum nk_collapse_states *state);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after calling `nk_tree_xxx_push_xxx`
/// __type__    | Value from the nk_tree_type section to visually mark a tree node header as either a collapseable UI section or tree node
/// __title__   | Label printed in the tree header
/// __state__   | Persistent state to update
///
/// Returns `true(1)` if visible and fillable with widgets or `false(0)` otherwise
*/
NK_API int nk_tree_state_push(struct nk_context*, enum nk_tree_type, const char *title, enum nk_collapse_states *state);
/*/// #### nk_tree_state_image_push
/// Start a collapsable UI section with image and label header and external state management
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_tree_state_image_push(struct nk_context*, enum nk_tree_type, struct nk_image, const char *title, enum nk_collapse_states *state);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after calling `nk_tree_xxx_push_xxx`
/// __img__     | Image to display inside the header on the left of the label
/// __type__    | Value from the nk_tree_type section to visually mark a tree node header as either a collapseable UI section or tree node
/// __title__   | Label printed in the tree header
/// __state__   | Persistent state to update
///
/// Returns `true(1)` if visible and fillable with widgets or `false(0)` otherwise
*/
NK_API int nk_tree_state_image_push(struct nk_context*, enum nk_tree_type, struct nk_image, const char *title, enum nk_collapse_states *state);
/*/// #### nk_tree_state_pop
/// Ends a collapsabale UI section
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_tree_state_pop(struct nk_context*);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter   | Description
/// ------------|-----------------------------------------------------------
/// __ctx__     | Must point to an previously initialized `nk_context` struct after calling `nk_tree_xxx_push_xxx`
*/
NK_API void nk_tree_state_pop(struct nk_context*);

#define nk_tree_element_push(ctx, type, title, state, sel) nk_tree_element_push_hashed(ctx, type, title, state, sel, NK_FILE_LINE,nk_strlen(NK_FILE_LINE),__LINE__)
#define nk_tree_element_push_id(ctx, type, title, state, sel, id) nk_tree_element_push_hashed(ctx, type, title, state, sel, NK_FILE_LINE,nk_strlen(NK_FILE_LINE),id)
NK_API int nk_tree_element_push_hashed(struct nk_context*, enum nk_tree_type, const char *title, enum nk_collapse_states initial_state, int *selected, const char *hash, int len, int seed);
NK_API int nk_tree_element_image_push_hashed(struct nk_context*, enum nk_tree_type, struct nk_image, const char *title, enum nk_collapse_states initial_state, int *selected, const char *hash, int len,int seed);
NK_API void nk_tree_element_pop(struct nk_context*);

/* =============================================================================
 *
 *                                  LIST VIEW
 *
 * ============================================================================= */
struct nk_list_view {
/* public: */
    int begin, end, count;
/* private: */
    int total_height;
    struct nk_context *ctx;
    nk_uint *scroll_pointer;
    nk_uint scroll_value;
};
NK_API int nk_list_view_begin(struct nk_context*, struct nk_list_view *out, const char *id, nk_flags, int row_height, int row_count);
NK_API void nk_list_view_end(struct nk_list_view*);
/* =============================================================================
 *
 *                                  WIDGET
 *
 * ============================================================================= */
enum nk_widget_layout_states {
    NK_WIDGET_INVALID, /* The widget cannot be seen and is completely out of view */
    NK_WIDGET_VALID, /* The widget is completely inside the window and can be updated and drawn */
    NK_WIDGET_ROM /* The widget is partially visible and cannot be updated */
};
enum nk_widget_states {
    NK_WIDGET_STATE_MODIFIED    = NK_FLAG(1),
    NK_WIDGET_STATE_INACTIVE    = NK_FLAG(2), /* widget is neither active nor hovered */
    NK_WIDGET_STATE_ENTERED     = NK_FLAG(3), /* widget has been hovered on the current frame */
    NK_WIDGET_STATE_HOVER       = NK_FLAG(4), /* widget is being hovered */
    NK_WIDGET_STATE_ACTIVED     = NK_FLAG(5),/* widget is currently activated */
    NK_WIDGET_STATE_LEFT        = NK_FLAG(6), /* widget is from this frame on not hovered anymore */
    NK_WIDGET_STATE_HOVERED     = NK_WIDGET_STATE_HOVER|NK_WIDGET_STATE_MODIFIED, /* widget is being hovered */
    NK_WIDGET_STATE_ACTIVE      = NK_WIDGET_STATE_ACTIVED|NK_WIDGET_STATE_MODIFIED /* widget is currently activated */
};
NK_API enum nk_widget_layout_states nk_widget(struct nk_rect*, const struct nk_context*);
NK_API enum nk_widget_layout_states nk_widget_fitting(struct nk_rect*, struct nk_context*, struct nk_vec2);
NK_API struct nk_rect nk_widget_bounds(struct nk_context*);
NK_API struct nk_vec2 nk_widget_position(struct nk_context*);
NK_API struct nk_vec2 nk_widget_size(struct nk_context*);
NK_API float nk_widget_width(struct nk_context*);
NK_API float nk_widget_height(struct nk_context*);
NK_API int nk_widget_is_hovered(struct nk_context*);
NK_API int nk_widget_is_mouse_clicked(struct nk_context*, enum nk_buttons);
NK_API int nk_widget_has_mouse_click_down(struct nk_context*, enum nk_buttons, int down);
NK_API void nk_spacing(struct nk_context*, int cols);
/* =============================================================================
 *
 *                                  TEXT
 *
 * ============================================================================= */
enum nk_text_align {
    NK_TEXT_ALIGN_LEFT        = 0x01,
    NK_TEXT_ALIGN_CENTERED    = 0x02,
    NK_TEXT_ALIGN_RIGHT       = 0x04,
    NK_TEXT_ALIGN_TOP         = 0x08,
    NK_TEXT_ALIGN_MIDDLE      = 0x10,
    NK_TEXT_ALIGN_BOTTOM      = 0x20
};
enum nk_text_alignment {
    NK_TEXT_LEFT        = NK_TEXT_ALIGN_MIDDLE|NK_TEXT_ALIGN_LEFT,
    NK_TEXT_CENTERED    = NK_TEXT_ALIGN_MIDDLE|NK_TEXT_ALIGN_CENTERED,
    NK_TEXT_RIGHT       = NK_TEXT_ALIGN_MIDDLE|NK_TEXT_ALIGN_RIGHT
};
NK_API void nk_text(struct nk_context*, const char*, int, nk_flags);
NK_API void nk_text_colored(struct nk_context*, const char*, int, nk_flags, struct nk_color);
NK_API void nk_text_wrap(struct nk_context*, const char*, int);
NK_API void nk_text_wrap_colored(struct nk_context*, const char*, int, struct nk_color);
NK_API void nk_label(struct nk_context*, const char*, nk_flags align);
NK_API void nk_label_colored(struct nk_context*, const char*, nk_flags align, struct nk_color);
NK_API void nk_label_wrap(struct nk_context*, const char*);
NK_API void nk_label_colored_wrap(struct nk_context*, const char*, struct nk_color);
NK_API void nk_image(struct nk_context*, struct nk_image);
NK_API void nk_image_color(struct nk_context*, struct nk_image, struct nk_color);
#ifdef NK_INCLUDE_STANDARD_VARARGS
NK_API void nk_labelf(struct nk_context*, nk_flags, NK_PRINTF_FORMAT_STRING const char*, ...) NK_PRINTF_VARARG_FUNC(3);
NK_API void nk_labelf_colored(struct nk_context*, nk_flags, struct nk_color, NK_PRINTF_FORMAT_STRING const char*,...) NK_PRINTF_VARARG_FUNC(4);
NK_API void nk_labelf_wrap(struct nk_context*, NK_PRINTF_FORMAT_STRING const char*,...) NK_PRINTF_VARARG_FUNC(2);
NK_API void nk_labelf_colored_wrap(struct nk_context*, struct nk_color, NK_PRINTF_FORMAT_STRING const char*,...) NK_PRINTF_VARARG_FUNC(3);
NK_API void nk_labelfv(struct nk_context*, nk_flags, NK_PRINTF_FORMAT_STRING const char*, va_list) NK_PRINTF_VALIST_FUNC(3);
NK_API void nk_labelfv_colored(struct nk_context*, nk_flags, struct nk_color, NK_PRINTF_FORMAT_STRING const char*, va_list) NK_PRINTF_VALIST_FUNC(4);
NK_API void nk_labelfv_wrap(struct nk_context*, NK_PRINTF_FORMAT_STRING const char*, va_list) NK_PRINTF_VALIST_FUNC(2);
NK_API void nk_labelfv_colored_wrap(struct nk_context*, struct nk_color, NK_PRINTF_FORMAT_STRING const char*, va_list) NK_PRINTF_VALIST_FUNC(3);
NK_API void nk_value_bool(struct nk_context*, const char *prefix, int);
NK_API void nk_value_int(struct nk_context*, const char *prefix, int);
NK_API void nk_value_uint(struct nk_context*, const char *prefix, unsigned int);
NK_API void nk_value_float(struct nk_context*, const char *prefix, float);
NK_API void nk_value_color_byte(struct nk_context*, const char *prefix, struct nk_color);
NK_API void nk_value_color_float(struct nk_context*, const char *prefix, struct nk_color);
NK_API void nk_value_color_hex(struct nk_context*, const char *prefix, struct nk_color);
#endif
/* =============================================================================
 *
 *                                  BUTTON
 *
 * ============================================================================= */
NK_API int nk_button_text(struct nk_context*, const char *title, int len);
NK_API int nk_button_label(struct nk_context*, const char *title);
NK_API int nk_button_color(struct nk_context*, struct nk_color);
NK_API int nk_button_symbol(struct nk_context*, enum nk_symbol_type);
NK_API int nk_button_image(struct nk_context*, struct nk_image img);
NK_API int nk_button_symbol_label(struct nk_context*, enum nk_symbol_type, const char*, nk_flags text_alignment);
NK_API int nk_button_symbol_text(struct nk_context*, enum nk_symbol_type, const char*, int, nk_flags alignment);
NK_API int nk_button_image_label(struct nk_context*, struct nk_image img, const char*, nk_flags text_alignment);
NK_API int nk_button_image_text(struct nk_context*, struct nk_image img, const char*, int, nk_flags alignment);
NK_API int nk_button_text_styled(struct nk_context*, const struct nk_style_button*, const char *title, int len);
NK_API int nk_button_label_styled(struct nk_context*, const struct nk_style_button*, const char *title);
NK_API int nk_button_symbol_styled(struct nk_context*, const struct nk_style_button*, enum nk_symbol_type);
NK_API int nk_button_image_styled(struct nk_context*, const struct nk_style_button*, struct nk_image img);
NK_API int nk_button_symbol_text_styled(struct nk_context*,const struct nk_style_button*, enum nk_symbol_type, const char*, int, nk_flags alignment);
NK_API int nk_button_symbol_label_styled(struct nk_context *ctx, const struct nk_style_button *style, enum nk_symbol_type symbol, const char *title, nk_flags align);
NK_API int nk_button_image_label_styled(struct nk_context*,const struct nk_style_button*, struct nk_image img, const char*, nk_flags text_alignment);
NK_API int nk_button_image_text_styled(struct nk_context*,const struct nk_style_button*, struct nk_image img, const char*, int, nk_flags alignment);
NK_API void nk_button_set_behavior(struct nk_context*, enum nk_button_behavior);
NK_API int nk_button_push_behavior(struct nk_context*, enum nk_button_behavior);
NK_API int nk_button_pop_behavior(struct nk_context*);
/* =============================================================================
 *
 *                                  CHECKBOX
 *
 * ============================================================================= */
NK_API int nk_check_label(struct nk_context*, const char*, int active);
NK_API int nk_check_text(struct nk_context*, const char*, int,int active);
NK_API unsigned nk_check_flags_label(struct nk_context*, const char*, unsigned int flags, unsigned int value);
NK_API unsigned nk_check_flags_text(struct nk_context*, const char*, int, unsigned int flags, unsigned int value);
NK_API int nk_checkbox_label(struct nk_context*, const char*, int *active);
NK_API int nk_checkbox_text(struct nk_context*, const char*, int, int *active);
NK_API int nk_checkbox_flags_label(struct nk_context*, const char*, unsigned int *flags, unsigned int value);
NK_API int nk_checkbox_flags_text(struct nk_context*, const char*, int, unsigned int *flags, unsigned int value);
/* =============================================================================
 *
 *                                  RADIO BUTTON
 *
 * ============================================================================= */
NK_API int nk_radio_label(struct nk_context*, const char*, int *active);
NK_API int nk_radio_text(struct nk_context*, const char*, int, int *active);
NK_API int nk_option_label(struct nk_context*, const char*, int active);
NK_API int nk_option_text(struct nk_context*, const char*, int, int active);
/* =============================================================================
 *
 *                                  SELECTABLE
 *
 * ============================================================================= */
NK_API int nk_selectable_label(struct nk_context*, const char*, nk_flags align, int *value);
NK_API int nk_selectable_text(struct nk_context*, const char*, int, nk_flags align, int *value);
NK_API int nk_selectable_image_label(struct nk_context*,struct nk_image,  const char*, nk_flags align, int *value);
NK_API int nk_selectable_image_text(struct nk_context*,struct nk_image, const char*, int, nk_flags align, int *value);
NK_API int nk_selectable_symbol_label(struct nk_context*,enum nk_symbol_type,  const char*, nk_flags align, int *value);
NK_API int nk_selectable_symbol_text(struct nk_context*,enum nk_symbol_type, const char*, int, nk_flags align, int *value);

NK_API int nk_select_label(struct nk_context*, const char*, nk_flags align, int value);
NK_API int nk_select_text(struct nk_context*, const char*, int, nk_flags align, int value);
NK_API int nk_select_image_label(struct nk_context*, struct nk_image,const char*, nk_flags align, int value);
NK_API int nk_select_image_text(struct nk_context*, struct nk_image,const char*, int, nk_flags align, int value);
NK_API int nk_select_symbol_label(struct nk_context*,enum nk_symbol_type,  const char*, nk_flags align, int value);
NK_API int nk_select_symbol_text(struct nk_context*,enum nk_symbol_type, const char*, int, nk_flags align, int value);

/* =============================================================================
 *
 *                                  SLIDER
 *
 * ============================================================================= */
NK_API float nk_slide_float(struct nk_context*, float min, float val, float max, float step);
NK_API int nk_slide_int(struct nk_context*, int min, int val, int max, int step);
NK_API int nk_slider_float(struct nk_context*, float min, float *val, float max, float step);
NK_API int nk_slider_int(struct nk_context*, int min, int *val, int max, int step);
/* =============================================================================
 *
 *                                  PROGRESSBAR
 *
 * ============================================================================= */
NK_API int nk_progress(struct nk_context*, nk_size *cur, nk_size max, int modifyable);
NK_API nk_size nk_prog(struct nk_context*, nk_size cur, nk_size max, int modifyable);

/* =============================================================================
 *
 *                                  COLOR PICKER
 *
 * ============================================================================= */
NK_API struct nk_colorf nk_color_picker(struct nk_context*, struct nk_colorf, enum nk_color_format);
NK_API int nk_color_pick(struct nk_context*, struct nk_colorf*, enum nk_color_format);
/* =============================================================================
 *
 *                                  PROPERTIES
 *
 * =============================================================================
/// ### Properties
/// Properties are the main value modification widgets in Nuklear. Changing a value
/// can be achieved by dragging, adding/removing incremental steps on button click
/// or by directly typing a number.
///
/// #### Usage
/// Each property requires a unique name for identifaction that is also used for
/// displaying a label. If you want to use the same name multiple times make sure
/// add a '#' before your name. The '#' will not be shown but will generate a
/// unique ID. Each propery also takes in a minimum and maximum value. If you want
/// to make use of the complete number range of a type just use the provided
/// type limits from `limits.h`. For example `INT_MIN` and `INT_MAX` for
/// `nk_property_int` and `nk_propertyi`. In additional each property takes in
/// a increment value that will be added or subtracted if either the increment
/// decrement button is clicked. Finally there is a value for increment per pixel
/// dragged that is added or subtracted from the value.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int value = 0;
/// struct nk_context ctx;
/// nk_init_xxx(&ctx, ...);
/// while (1) {
///     // Input
///     Event evt;
///     nk_input_begin(&ctx);
///     while (GetEvent(&evt)) {
///         if (evt.type == MOUSE_MOVE)
///             nk_input_motion(&ctx, evt.motion.x, evt.motion.y);
///         else if (evt.type == [...]) {
///             nk_input_xxx(...);
///         }
///     }
///     nk_input_end(&ctx);
///     //
///     // Window
///     if (nk_begin_xxx(...) {
///         // Property
///         nk_layout_row_dynamic(...);
///         nk_property_int(ctx, "ID", INT_MIN, &value, INT_MAX, 1, 1);
///     }
///     nk_end(ctx);
///     //
///     // Draw
///     const struct nk_command *cmd = 0;
///     nk_foreach(cmd, &ctx) {
///     switch (cmd->type) {
///     case NK_COMMAND_LINE:
///         your_draw_line_function(...)
///         break;
///     case NK_COMMAND_RECT
///         your_draw_rect_function(...)
///         break;
///     case ...:
///         // [...]
///     }
///     nk_clear(&ctx);
/// }
/// nk_free(&ctx);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// #### Reference
/// Function            | Description
/// --------------------|-------------------------------------------
/// nk_property_int     | Integer property directly modifing a passed in value
/// nk_property_float   | Float property directly modifing a passed in value
/// nk_property_double  | Double property directly modifing a passed in value
/// nk_propertyi        | Integer property returning the modified int value
/// nk_propertyf        | Float property returning the modified float value
/// nk_propertyd        | Double property returning the modified double value
///
*/
/*/// #### nk_property_int
/// Integer property directly modifing a passed in value
/// !!! WARNING
///     To generate a unique property ID using the same label make sure to insert
///     a `#` at the beginning. It will not be shown but guarantees correct behavior.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_property_int(struct nk_context *ctx, const char *name, int min, int *val, int max, int step, float inc_per_pixel);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter           | Description
/// --------------------|-----------------------------------------------------------
/// __ctx__             | Must point to an previously initialized `nk_context` struct after calling a layouting function
/// __name__            | String used both as a label as well as a unique identifier
/// __min__             | Minimum value not allowed to be underflown
/// __val__             | Integer pointer to be modified
/// __max__             | Maximum value not allowed to be overflown
/// __step__            | Increment added and subtracted on increment and decrement button
/// __inc_per_pixel__   | Value per pixel added or subtracted on dragging
*/
NK_API void nk_property_int(struct nk_context*, const char *name, int min, int *val, int max, int step, float inc_per_pixel);
/*/// #### nk_property_float
/// Float property directly modifing a passed in value
/// !!! WARNING
///     To generate a unique property ID using the same label make sure to insert
///     a `#` at the beginning. It will not be shown but guarantees correct behavior.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_property_float(struct nk_context *ctx, const char *name, float min, float *val, float max, float step, float inc_per_pixel);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter           | Description
/// --------------------|-----------------------------------------------------------
/// __ctx__             | Must point to an previously initialized `nk_context` struct after calling a layouting function
/// __name__            | String used both as a label as well as a unique identifier
/// __min__             | Minimum value not allowed to be underflown
/// __val__             | Float pointer to be modified
/// __max__             | Maximum value not allowed to be overflown
/// __step__            | Increment added and subtracted on increment and decrement button
/// __inc_per_pixel__   | Value per pixel added or subtracted on dragging
*/
NK_API void nk_property_float(struct nk_context*, const char *name, float min, float *val, float max, float step, float inc_per_pixel);
/*/// #### nk_property_double
/// Double property directly modifing a passed in value
/// !!! WARNING
///     To generate a unique property ID using the same label make sure to insert
///     a `#` at the beginning. It will not be shown but guarantees correct behavior.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// void nk_property_double(struct nk_context *ctx, const char *name, double min, double *val, double max, double step, double inc_per_pixel);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter           | Description
/// --------------------|-----------------------------------------------------------
/// __ctx__             | Must point to an previously initialized `nk_context` struct after calling a layouting function
/// __name__            | String used both as a label as well as a unique identifier
/// __min__             | Minimum value not allowed to be underflown
/// __val__             | Double pointer to be modified
/// __max__             | Maximum value not allowed to be overflown
/// __step__            | Increment added and subtracted on increment and decrement button
/// __inc_per_pixel__   | Value per pixel added or subtracted on dragging
*/
NK_API void nk_property_double(struct nk_context*, const char *name, double min, double *val, double max, double step, float inc_per_pixel);
/*/// #### nk_propertyi
/// Integer property modifing a passed in value and returning the new value
/// !!! WARNING
///     To generate a unique property ID using the same label make sure to insert
///     a `#` at the beginning. It will not be shown but guarantees correct behavior.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// int nk_propertyi(struct nk_context *ctx, const char *name, int min, int val, int max, int step, float inc_per_pixel);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter           | Description
/// --------------------|-----------------------------------------------------------
/// __ctx__             | Must point to an previously initialized `nk_context` struct after calling a layouting function
/// __name__            | String used both as a label as well as a unique identifier
/// __min__             | Minimum value not allowed to be underflown
/// __val__             | Current integer value to be modified and returned
/// __max__             | Maximum value not allowed to be overflown
/// __step__            | Increment added and subtracted on increment and decrement button
/// __inc_per_pixel__   | Value per pixel added or subtracted on dragging
///
/// Returns the new modified integer value
*/
NK_API int nk_propertyi(struct nk_context*, const char *name, int min, int val, int max, int step, float inc_per_pixel);
/*/// #### nk_propertyf
/// Float property modifing a passed in value and returning the new value
/// !!! WARNING
///     To generate a unique property ID using the same label make sure to insert
///     a `#` at the beginning. It will not be shown but guarantees correct behavior.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// float nk_propertyf(struct nk_context *ctx, const char *name, float min, float val, float max, float step, float inc_per_pixel);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter           | Description
/// --------------------|-----------------------------------------------------------
/// __ctx__             | Must point to an previously initialized `nk_context` struct after calling a layouting function
/// __name__            | String used both as a label as well as a unique identifier
/// __min__             | Minimum value not allowed to be underflown
/// __val__             | Current float value to be modified and returned
/// __max__             | Maximum value not allowed to be overflown
/// __step__            | Increment added and subtracted on increment and decrement button
/// __inc_per_pixel__   | Value per pixel added or subtracted on dragging
///
/// Returns the new modified float value
*/
NK_API float nk_propertyf(struct nk_context*, const char *name, float min, float val, float max, float step, float inc_per_pixel);
/*/// #### nk_propertyd
/// Float property modifing a passed in value and returning the new value
/// !!! WARNING
///     To generate a unique property ID using the same label make sure to insert
///     a `#` at the beginning. It will not be shown but guarantees correct behavior.
///
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~c
/// float nk_propertyd(struct nk_context *ctx, const char *name, double min, double val, double max, double step, double inc_per_pixel);
/// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
///
/// Parameter           | Description
/// --------------------|-----------------------------------------------------------
/// __ctx__             | Must point to an previously initialized `nk_context` struct after calling a layouting function
/// __name__            | String used both as a label as well as a unique identifier
/// __min__             | Minimum value not allowed to be underflown
/// __val__             | Current double value to be modified and returned
/// __max__             | Maximum value not allowed to be overflown
/// __step__            | Increment added and subtracted on increment and decrement button
/// __inc_per_pixel__   | Value per pixel added or subtracted on dragging
///
/// Returns the new modified double value
*/
NK_API double nk_propertyd(struct nk_context*, const char *name, double min, double val, double max, double step, float inc_per_pixel);
/* =============================================================================
 *
 *                                  TEXT EDIT
 *
 * ============================================================================= */
enum nk_edit_flags {
    NK_EDIT_DEFAULT                 = 0,
    NK_EDIT_READ_ONLY               = NK_FLAG(0),
    NK_EDIT_AUTO_SELECT             = NK_FLAG(1),
    NK_EDIT_SIG_ENTER               = NK_FLAG(2),
    NK_EDIT_ALLOW_TAB               = NK_FLAG(3),
    NK_EDIT_NO_CURSOR               = NK_FLAG(4),
    NK_EDIT_SELECTABLE              = NK_FLAG(5),
    NK_EDIT_CLIPBOARD               = NK_FLAG(6),
    NK_EDIT_CTRL_ENTER_NEWLINE      = NK_FLAG(7),
    NK_EDIT_NO_HORIZONTAL_SCROLL    = NK_FLAG(8),
    NK_EDIT_ALWAYS_INSERT_MODE      = NK_FLAG(9),
    NK_EDIT_MULTILINE               = NK_FLAG(10),
    NK_EDIT_GOTO_END_ON_ACTIVATE    = NK_FLAG(11)
};
enum nk_edit_types {
    NK_EDIT_SIMPLE  = NK_EDIT_ALWAYS_INSERT_MODE,
    NK_EDIT_FIELD   = NK_EDIT_SIMPLE|NK_EDIT_SELECTABLE|NK_EDIT_CLIPBOARD,
    NK_EDIT_BOX     = NK_EDIT_ALWAYS_INSERT_MODE| NK_EDIT_SELECTABLE| NK_EDIT_MULTILINE|NK_EDIT_ALLOW_TAB|NK_EDIT_CLIPBOARD,
    NK_EDIT_EDITOR  = NK_EDIT_SELECTABLE|NK_EDIT_MULTILINE|NK_EDIT_ALLOW_TAB| NK_EDIT_CLIPBOARD
};
enum nk_edit_events {
    NK_EDIT_ACTIVE      = NK_FLAG(0), /* edit widget is currently being modified */
    NK_EDIT_INACTIVE    = NK_FLAG(1), /* edit widget is not active and is not being modified */
    NK_EDIT_ACTIVATED   = NK_FLAG(2), /* edit widget went from state inactive to state active */
    NK_EDIT_DEACTIVATED = NK_FLAG(3), /* edit widget went from state active to state inactive */
    NK_EDIT_COMMITED    = NK_FLAG(4) /* edit widget has received an enter and lost focus */
};
NK_API nk_flags nk_edit_string(struct nk_context*, nk_flags, char *buffer, int *len, int max, nk_plugin_filter);
NK_API nk_flags nk_edit_string_zero_terminated(struct nk_context*, nk_flags, char *buffer, int max, nk_plugin_filter);
NK_API nk_flags nk_edit_buffer(struct nk_context*, nk_flags, struct nk_text_edit*, nk_plugin_filter);
NK_API void nk_edit_focus(struct nk_context*, nk_flags flags);
NK_API void nk_edit_unfocus(struct nk_context*);
/* =============================================================================
 *
 *                                  CHART
 *
 * ============================================================================= */
NK_API int nk_chart_begin(struct nk_context*, enum nk_chart_type, int num, float min, float max);
NK_API int nk_chart_begin_colored(struct nk_context*, enum nk_chart_type, struct nk_color, struct nk_color active, int num, float min, float max);
NK_API void nk_chart_add_slot(struct nk_context *ctx, const enum nk_chart_type, int count, float min_value, float max_value);
NK_API void nk_chart_add_slot_colored(struct nk_context *ctx, const enum nk_chart_type, struct nk_color, struct nk_color active, int count, float min_value, float max_value);
NK_API nk_flags nk_chart_push(struct nk_context*, float);
NK_API nk_flags nk_chart_push_slot(struct nk_context*, float, int);
NK_API void nk_chart_end(struct nk_context*);
NK_API void nk_plot(struct nk_context*, enum nk_chart_type, const float *values, int count, int offset);
NK_API void nk_plot_function(struct nk_context*, enum nk_chart_type, void *userdata, float(*value_getter)(void* user, int index), int count, int offset);
/* =============================================================================
 *
 *                                  POPUP
 *
 * ============================================================================= */
NK_API int nk_popup_begin(struct nk_context*, enum nk_popup_type, const char*, nk_flags, struct nk_rect bounds);
NK_API void nk_popup_close(struct nk_context*);
NK_API void nk_popup_end(struct nk_context*);
NK_API void nk_popup_get_scroll(struct nk_context*, nk_uint *offset_x, nk_uint *offset_y);
NK_API void nk_popup_set_scroll(struct nk_context*, nk_uint offset_x, nk_uint offset_y);
/* =============================================================================
 *
 *                                  COMBOBOX
 *
 * ============================================================================= */
NK_API int nk_combo(struct nk_context*, const char **items, int count, int selected, int item_height, struct nk_vec2 size);
NK_API int nk_combo_separator(struct nk_context*, const char *items_separated_by_separator, int separator, int selected, int count, int item_height, struct nk_vec2 size);
NK_API int nk_combo_string(struct nk_context*, const char *items_separated_by_zeros, int selected, int count, int item_height, struct nk_vec2 size);
NK_API int nk_combo_callback(struct nk_context*, void(*item_getter)(void*, int, const char**), void *userdata, int selected, int count, int item_height, struct nk_vec2 size);
NK_API void nk_combobox(struct nk_context*, const char **items, int count, int *selected, int item_height, struct nk_vec2 size);
NK_API void nk_combobox_string(struct nk_context*, const char *items_separated_by_zeros, int *selected, int count, int item_height, struct nk_vec2 size);
NK_API void nk_combobox_separator(struct nk_context*, const char *items_separated_by_separator, int separator,int *selected, int count, int item_height, struct nk_vec2 size);
NK_API void nk_combobox_callback(struct nk_context*, void(*item_getter)(void*, int, const char**), void*, int *selected, int count, int item_height, struct nk_vec2 size);
/* =============================================================================
 *
 *                                  ABSTRACT COMBOBOX
 *
 * ============================================================================= */
NK_API int nk_combo_begin_text(struct nk_context*, const char *selected, int, struct nk_vec2 size);
NK_API int nk_combo_begin_label(struct nk_context*, const char *selected, struct nk_vec2 size);
NK_API int nk_combo_begin_color(struct nk_context*, struct nk_color color, struct nk_vec2 size);
NK_API int nk_combo_begin_symbol(struct nk_context*,  enum nk_symbol_type,  struct nk_vec2 size);
NK_API int nk_combo_begin_symbol_label(struct nk_context*, const char *selected, enum nk_symbol_type, struct nk_vec2 size);
NK_API int nk_combo_begin_symbol_text(struct nk_context*, const char *selected, int, enum nk_symbol_type, struct nk_vec2 size);
NK_API int nk_combo_begin_image(struct nk_context*, struct nk_image img,  struct nk_vec2 size);
NK_API int nk_combo_begin_image_label(struct nk_context*, const char *selected, struct nk_image, struct nk_vec2 size);
NK_API int nk_combo_begin_image_text(struct nk_context*,  const char *selected, int, struct nk_image, struct nk_vec2 size);
NK_API int nk_combo_item_label(struct nk_context*, const char*, nk_flags alignment);
NK_API int nk_combo_item_text(struct nk_context*, const char*,int, nk_flags alignment);
NK_API int nk_combo_item_image_label(struct nk_context*, struct nk_image, const char*, nk_flags alignment);
NK_API int nk_combo_item_image_text(struct nk_context*, struct nk_image, const char*, int,nk_flags alignment);
NK_API int nk_combo_item_symbol_label(struct nk_context*, enum nk_symbol_type, const char*, nk_flags alignment);
NK_API int nk_combo_item_symbol_text(struct nk_context*, enum nk_symbol_type, const char*, int, nk_flags alignment);
NK_API void nk_combo_close(struct nk_context*);
NK_API void nk_combo_end(struct nk_context*);
/* =============================================================================
 *
 *                                  CONTEXTUAL
 *
 * ============================================================================= */
NK_API int nk_contextual_begin(struct nk_context*, nk_flags, struct nk_vec2, struct nk_rect trigger_bounds);
NK_API int nk_contextual_item_text(struct nk_context*, const char*, int,nk_flags align);
NK_API int nk_contextual_item_label(struct nk_context*, const char*, nk_flags align);
NK_API int nk_contextual_item_image_label(struct nk_context*, struct nk_image, const char*, nk_flags alignment);
NK_API int nk_contextual_item_image_text(struct nk_context*, struct nk_image, const char*, int len, nk_flags alignment);
NK_API int nk_contextual_item_symbol_label(struct nk_context*, enum nk_symbol_type, const char*, nk_flags alignment);
NK_API int nk_contextual_item_symbol_text(struct nk_context*, enum nk_symbol_type, const char*, int, nk_flags alignment);
NK_API void nk_contextual_close(struct nk_context*);
NK_API void nk_contextual_end(struct nk_context*);
/* =============================================================================
 *
 *                                  TOOLTIP
 *
 * ============================================================================= */
NK_API void nk_tooltip(struct nk_context*, const char*);
#ifdef NK_INCLUDE_STANDARD_VARARGS
NK_API void nk_tooltipf(struct nk_context*, NK_PRINTF_FORMAT_STRING const char*, ...) NK_PRINTF_VARARG_FUNC(2);
NK_API void nk_tooltipfv(struct nk_context*, NK_PRINTF_FORMAT_STRING const char*, va_list) NK_PRINTF_VALIST_FUNC(2);
#endif
NK_API int nk_tooltip_begin(struct nk_context*, float width);
NK_API void nk_tooltip_end(struct nk_context*);
/* =============================================================================
 *
 *                                  MENU
 *
 * ============================================================================= */
NK_API void nk_menubar_begin(struct nk_context*);
NK_API void nk_menubar_end(struct nk_context*);
NK_API int nk_menu_begin_text(struct nk_context*, const char* title, int title_len, nk_flags align, struct nk_vec2 size);
NK_API int nk_menu_begin_label(struct nk_context*, const char*, nk_flags align, struct nk_vec2 size);
NK_API int nk_menu_begin_image(struct nk_context*, const char*, struct nk_image, struct nk_vec2 size);
NK_API int nk_menu_begin_image_text(struct nk_context*, const char*, int,nk_flags align,struct nk_image, struct nk_vec2 size);
NK_API int nk_menu_begin_image_label(struct nk_context*, const char*, nk_flags align,struct nk_image, struct nk_vec2 size);
NK_API int nk_menu_begin_symbol(struct nk_context*, const char*, enum nk_symbol_type, struct nk_vec2 size);
NK_API int nk_menu_begin_symbol_text(struct nk_context*, const char*, int,nk_flags align,enum nk_symbol_type, struct nk_vec2 size);
NK_API int nk_menu_begin_symbol_label(struct nk_context*, const char*, nk_flags align,enum nk_symbol_type, struct nk_vec2 size);
NK_API int nk_menu_item_text(struct nk_context*, const char*, int,nk_flags align);
NK_API int nk_menu_item_label(struct nk_context*, const char*, nk_flags alignment);
NK_API int nk_menu_item_image_label(struct nk_context*, struct nk_image, const char*, nk_flags alignment);
NK_API int nk_menu_item_image_text(struct nk_context*, struct nk_image, const char*, int len, nk_flags alignment);
NK_API int nk_menu_item_symbol_text(struct nk_context*, enum nk_symbol_type, const char*, int, nk_flags alignment);
NK_API int nk_menu_item_symbol_label(struct nk_context*, enum nk_symbol_type, const char*, nk_flags alignment);
NK_API void nk_menu_close(struct nk_context*);
NK_API void nk_menu_end(struct nk_context*);
/* =============================================================================
 *
 *                                  STYLE
 *
 * ============================================================================= */
enum nk_style_colors {
    NK_COLOR_TEXT,
    NK_COLOR_WINDOW,
    NK_COLOR_HEADER,
    NK_COLOR_BORDER,
    NK_COLOR_BUTTON,
    NK_COLOR_BUTTON_HOVER,
    NK_COLOR_BUTTON_ACTIVE,
    NK_COLOR_TOGGLE,
    NK_COLOR_TOGGLE_HOVER,
    NK_COLOR_TOGGLE_CURSOR,
    NK_COLOR_SELECT,
    NK_COLOR_SELECT_ACTIVE,
    NK_COLOR_SLIDER,
    NK_COLOR_SLIDER_CURSOR,
    NK_COLOR_SLIDER_CURSOR_HOVER,
    NK_COLOR_SLIDER_CURSOR_ACTIVE,
    NK_COLOR_PROPERTY,
    NK_COLOR_EDIT,
    NK_COLOR_EDIT_CURSOR,
    NK_COLOR_COMBO,
    NK_COLOR_CHART,
    NK_COLOR_CHART_COLOR,
    NK_COLOR_CHART_COLOR_HIGHLIGHT,
    NK_COLOR_SCROLLBAR,
    NK_COLOR_SCROLLBAR_CURSOR,
    NK_COLOR_SCROLLBAR_CURSOR_HOVER,
    NK_COLOR_SCROLLBAR_CURSOR_ACTIVE,
    NK_COLOR_TAB_HEADER,
    NK_COLOR_COUNT
};
enum nk_style_cursor {
    NK_CURSOR_ARROW,
    NK_CURSOR_TEXT,
    NK_CURSOR_MOVE,
    NK_CURSOR_RESIZE_VERTICAL,
    NK_CURSOR_RESIZE_HORIZONTAL,
    NK_CURSOR_RESIZE_TOP_LEFT_DOWN_RIGHT,
    NK_CURSOR_RESIZE_TOP_RIGHT_DOWN_LEFT,
    NK_CURSOR_COUNT
};
NK_API void nk_style_default(struct nk_context*);
NK_API void nk_style_from_table(struct nk_context*, const struct nk_color*);
NK_API void nk_style_load_cursor(struct nk_context*, enum nk_style_cursor, const struct nk_cursor*);
NK_API void nk_style_load_all_cursors(struct nk_context*, struct nk_cursor*);
NK_API const char* nk_style_get_color_by_name(enum nk_style_colors);
NK_API void nk_style_set_font(struct nk_context*, const struct nk_user_font*);
NK_API int nk_style_set_cursor(struct nk_context*, enum nk_style_cursor);
NK_API void nk_style_show_cursor(struct nk_context*);
NK_API void nk_style_hide_cursor(struct nk_context*);

NK_API int nk_style_push_font(struct nk_context*, const struct nk_user_font*);
NK_API int nk_style_push_float(struct nk_context*, float*, float);
NK_API int nk_style_push_vec2(struct nk_context*, struct nk_vec2*, struct nk_vec2);
NK_API int nk_style_push_style_item(struct nk_context*, struct nk_style_item*, struct nk_style_item);
NK_API int nk_style_push_flags(struct nk_context*, nk_flags*, nk_flags);
NK_API int nk_style_push_color(struct nk_context*, struct nk_color*, struct nk_color);

NK_API int nk_style_pop_font(struct nk_context*);
NK_API int nk_style_pop_float(struct nk_context*);
NK_API int nk_style_pop_vec2(struct nk_context*);
NK_API int nk_style_pop_style_item(struct nk_context*);
NK_API int nk_style_pop_flags(struct nk_context*);
NK_API int nk_style_pop_color(struct nk_context*);
/* =============================================================================
 *
 *                                  COLOR
 *
 * ============================================================================= */
NK_API struct nk_color nk_rgb(int r, int g, int b);
NK_API struct nk_color nk_rgb_iv(const int *rgb);
NK_API struct nk_color nk_rgb_bv(const nk_byte* rgb);
NK_API struct nk_color nk_rgb_f(float r, float g, float b);
NK_API struct nk_color nk_rgb_fv(const float *rgb);
NK_API struct nk_color nk_rgb_cf(struct nk_colorf c);
NK_API struct nk_color nk_rgb_hex(const char *rgb);

NK_API struct nk_color nk_rgba(int r, int g, int b, int a);
NK_API struct nk_color nk_rgba_u32(nk_uint);
NK_API struct nk_color nk_rgba_iv(const int *rgba);
NK_API struct nk_color nk_rgba_bv(const nk_byte *rgba);
NK_API struct nk_color nk_rgba_f(float r, float g, float b, float a);
NK_API struct nk_color nk_rgba_fv(const float *rgba);
NK_API struct nk_color nk_rgba_cf(struct nk_colorf c);
NK_API struct nk_color nk_rgba_hex(const char *rgb);

NK_API struct nk_colorf nk_hsva_colorf(float h, float s, float v, float a);
NK_API struct nk_colorf nk_hsva_colorfv(float *c);
NK_API void nk_colorf_hsva_f(float *out_h, float *out_s, float *out_v, float *out_a, struct nk_colorf in);
NK_API void nk_colorf_hsva_fv(float *hsva, struct nk_colorf in);

NK_API struct nk_color nk_hsv(int h, int s, int v);
NK_API struct nk_color nk_hsv_iv(const int *hsv);
NK_API struct nk_color nk_hsv_bv(const nk_byte *hsv);
NK_API struct nk_color nk_hsv_f(float h, float s, float v);
NK_API struct nk_color nk_hsv_fv(const float *hsv);

NK_API struct nk_color nk_hsva(int h, int s, int v, int a);
NK_API struct nk_color nk_hsva_iv(const int *hsva);
NK_API struct nk_color nk_hsva_bv(const nk_byte *hsva);
NK_API struct nk_color nk_hsva_f(float h, float s, float v, float a);
NK_API struct nk_color nk_hsva_fv(const float *hsva);

/* color (conversion nuklear --> user) */
NK_API void nk_color_f(float *r, float *g, float *b, float *a, struct nk_color);
NK_API void nk_color_fv(float *rgba_out, struct nk_color);
NK_API struct nk_colorf nk_color_cf(struct nk_color);
NK_API void nk_color_d(double *r, double *g, double *b, double *a, struct nk_color);
NK_API void nk_color_dv(double *rgba_out, struct nk_color);

NK_API nk_uint nk_color_u32(struct nk_color);
NK_API void nk_color_hex_rgba(char *output, struct nk_color);
NK_API void nk_color_hex_rgb(char *output, struct nk_color);

NK_API void nk_color_hsv_i(int *out_h, int *out_s, int *out_v, struct nk_color);
NK_API void nk_color_hsv_b(nk_byte *out_h, nk_byte *out_s, nk_byte *out_v, struct nk_color);
NK_API void nk_color_hsv_iv(int *hsv_out, struct nk_color);
NK_API void nk_color_hsv_bv(nk_byte *hsv_out, struct nk_color);
NK_API void nk_color_hsv_f(float *out_h, float *out_s, float *out_v, struct nk_color);
NK_API void nk_color_hsv_fv(float *hsv_out, struct nk_color);

NK_API void nk_color_hsva_i(int *h, int *s, int *v, int *a, struct nk_color);
NK_API void nk_color_hsva_b(nk_byte *h, nk_byte *s, nk_byte *v, nk_byte *a, struct nk_color);
NK_API void nk_color_hsva_iv(int *hsva_out, struct nk_color);
NK_API void nk_color_hsva_bv(nk_byte *hsva_out, struct nk_color);
NK_API void nk_color_hsva_f(float *out_h, float *out_s, float *out_v, float *out_a, struct nk_color);
NK_API void nk_color_hsva_fv(float *hsva_out, struct nk_color);
/* =============================================================================
 *
 *                                  IMAGE
 *
 * ============================================================================= */
NK_API nk_handle nk_handle_ptr(void*);
NK_API nk_handle nk_handle_id(int);
NK_API struct nk_image nk_image_handle(nk_handle);
NK_API struct nk_image nk_image_ptr(void*);
NK_API struct nk_image nk_image_id(int);
NK_API int nk_image_is_subimage(const struct nk_image* img);
NK_API struct nk_image nk_subimage_ptr(void*, unsigned short w, unsigned short h, struct nk_rect sub_region);
NK_API struct nk_image nk_subimage_id(int, unsigned short w, unsigned short h, struct nk_rect sub_region);
NK_API struct nk_image nk_subimage_handle(nk_handle, unsigned short w, unsigned short h, struct nk_rect sub_region);
/* =============================================================================
 *
 *                                  MATH
 *
 * ============================================================================= */
NK_API nk_hash nk_murmur_hash(const void *key, int len, nk_hash seed);
NK_API void nk_triangle_from_direction(struct nk_vec2 *result, struct nk_rect r, float pad_x, float pad_y, enum nk_heading);

NK_API struct nk_vec2 nk_vec2(float x, float y);
NK_API struct nk_vec2 nk_vec2i(int x, int y);
NK_API struct nk_vec2 nk_vec2v(const float *xy);
NK_API struct nk_vec2 nk_vec2iv(const int *xy);

NK_API struct nk_rect nk_get_null_rect(void);
NK_API struct nk_rect nk_rect(float x, float y, float w, float h);
NK_API struct nk_rect nk_recti(int x, int y, int w, int h);
NK_API struct nk_rect nk_recta(struct nk_vec2 pos, struct nk_vec2 size);
NK_API struct nk_rect nk_rectv(const float *xywh);
NK_API struct nk_rect nk_rectiv(const int *xywh);
NK_API struct nk_vec2 nk_rect_pos(struct nk_rect);
NK_API struct nk_vec2 nk_rect_size(struct nk_rect);
/* =============================================================================
 *
 *                                  STRING
 *
 * ============================================================================= */
NK_API int nk_strlen(const char *str);
NK_API int nk_stricmp(const char *s1, const char *s2);
NK_API int nk_stricmpn(const char *s1, const char *s2, int n);
NK_API int nk_strtoi(const char *str, const char **endptr);
NK_API float nk_strtof(const char *str, const char **endptr);
NK_API double nk_strtod(const char *str, const char **endptr);
NK_API int nk_strfilter(const char *text, const char *regexp);
NK_API int nk_strmatch_fuzzy_string(char const *str, char const *pattern, int *out_score);
NK_API int nk_strmatch_fuzzy_text(const char *txt, int txt_len, const char *pattern, int *out_score);
/* =============================================================================
 *
 *                                  UTF-8
 *
 * ============================================================================= */
NK_API int nk_utf_decode(const char*, nk_rune*, int);
NK_API int nk_utf_encode(nk_rune, char*, int);
NK_API int nk_utf_len(const char*, int byte_len);
NK_API const char* nk_utf_at(const char *buffer, int length, int index, nk_rune *unicode, int *len);
/* ===============================================================
 *
 *                          FONT
 *
 * ===============================================================*/
/*  Font handling in this library was designed to be quite customizable and lets
    you decide what you want to use and what you want to provide. There are three
    different ways to use the font atlas. The first two will use your font
    handling scheme and only requires essential data to run nuklear. The next
    slightly more advanced features is font handling with vertex buffer output.
    Finally the most complex API wise is using nuklear's font baking API.

    1.) Using your own implementation without vertex buffer output
    --------------------------------------------------------------
    So first up the easiest way to do font handling is by just providing a
    `nk_user_font` struct which only requires the height in pixel of the used
    font and a callback to calculate the width of a string. This way of handling
    fonts is best fitted for using the normal draw shape command API where you
    do all the text drawing yourself and the library does not require any kind
    of deeper knowledge about which font handling mechanism you use.
    IMPORTANT: the `nk_user_font` pointer provided to nuklear has to persist
    over the complete life time! I know this sucks but it is currently the only
    way to switch between fonts.

        float your_text_width_calculation(nk_handle handle, float height, const char *text, int len)
        {
            your_font_type *type = handle.ptr;
            float text_width = ...;
            return text_width;
        }

        struct nk_user_font font;
        font.userdata.ptr = &your_font_class_or_struct;
        font.height = your_font_height;
        font.width = your_text_width_calculation;

        struct nk_context ctx;
        nk_init_default(&ctx, &font);

    2.) Using your own implementation with vertex buffer output
    --------------------------------------------------------------
    While the first approach works fine if you don't want to use the optional
    vertex buffer output it is not enough if you do. To get font handling working
    for these cases you have to provide two additional parameters inside the
    `nk_user_font`. First a texture atlas handle used to draw text as subimages
    of a bigger font atlas texture and a callback to query a character's glyph
    information (offset, size, ...). So it is still possible to provide your own
    font and use the vertex buffer output.

        float your_text_width_calculation(nk_handle handle, float height, const char *text, int len)
        {
            your_font_type *type = handle.ptr;
            float text_width = ...;
            return text_width;
        }
        void query_your_font_glyph(nk_handle handle, float font_height, struct nk_user_font_glyph *glyph, nk_rune codepoint, nk_rune next_codepoint)
        {
            your_font_type *type = handle.ptr;
            glyph.width = ...;
            glyph.height = ...;
            glyph.xadvance = ...;
            glyph.uv[0].x = ...;
            glyph.uv[0].y = ...;
            glyph.uv[1].x = ...;
            glyph.uv[1].y = ...;
            glyph.offset.x = ...;
            glyph.offset.y = ...;
        }

        struct nk_user_font font;
        font.userdata.ptr = &your_font_class_or_struct;
        font.height = your_font_height;
        font.width = your_text_width_calculation;
        font.query = query_your_font_glyph;
        font.texture.id = your_font_texture;

        struct nk_context ctx;
        nk_init_default(&ctx, &font);

    3.) Nuklear font baker
    ------------------------------------
    The final approach if you do not have a font handling functionality or don't
    want to use it in this library is by using the optional font baker.
    The font baker APIs can be used to create a font plus font atlas texture
    and can be used with or without the vertex buffer output.

    It still uses the `nk_user_font` struct and the two different approaches
    previously stated still work. The font baker is not located inside
    `nk_context` like all other systems since it can be understood as more of
    an extension to nuklear and does not really depend on any `nk_context` state.

    Font baker need to be initialized first by one of the nk_font_atlas_init_xxx
    functions. If you don't care about memory just call the default version
    `nk_font_atlas_init_default` which will allocate all memory from the standard library.
    If you want to control memory allocation but you don't care if the allocated
    memory is temporary and therefore can be freed directly after the baking process
    is over or permanent you can call `nk_font_atlas_init`.

    After successfully initializing the font baker you can add Truetype(.ttf) fonts from
    different sources like memory or from file by calling one of the `nk_font_atlas_add_xxx`.
    functions. Adding font will permanently store each font, font config and ttf memory block(!)
    inside the font atlas and allows to reuse the font atlas. If you don't want to reuse
    the font baker by for example adding additional fonts you can call
    `nk_font_atlas_cleanup` after the baking process is over (after calling nk_font_atlas_end).

    As soon as you added all fonts you wanted you can now start the baking process
    for every selected glyph to image by calling `nk_font_atlas_bake`.
    The baking process returns image memory, width and height which can be used to
    either create your own image object or upload it to any graphics library.
    No matter which case you finally have to call `nk_font_atlas_end` which
    will free all temporary memory including the font atlas image so make sure
    you created our texture beforehand. `nk_font_atlas_end` requires a handle
    to your font texture or object and optionally fills a `struct nk_draw_null_texture`
    which can be used for the optional vertex output. If you don't want it just
    set the argument to `NULL`.

    At this point you are done and if you don't want to reuse the font atlas you
    can call `nk_font_atlas_cleanup` to free all truetype blobs and configuration
    memory. Finally if you don't use the font atlas and any of it's fonts anymore
    you need to call `nk_font_atlas_clear` to free all memory still being used.

        struct nk_font_atlas atlas;
        nk_font_atlas_init_default(&atlas);
        nk_font_atlas_begin(&atlas);
        nk_font *font = nk_font_atlas_add_from_file(&atlas, "Path/To/Your/TTF_Font.ttf", 13, 0);
        nk_font *font2 = nk_font_atlas_add_from_file(&atlas, "Path/To/Your/TTF_Font2.ttf", 16, 0);
        const void* img = nk_font_atlas_bake(&atlas, &img_width, &img_height, NK_FONT_ATLAS_RGBA32);
        nk_font_atlas_end(&atlas, nk_handle_id(texture), 0);

        struct nk_context ctx;
        nk_init_default(&ctx, &font->handle);
        while (1) {

        }
        nk_font_atlas_clear(&atlas);

    The font baker API is probably the most complex API inside this library and
    I would suggest reading some of my examples `example/` to get a grip on how
    to use the font atlas. There are a number of details I left out. For example
    how to merge fonts, configure a font with `nk_font_config` to use other languages,
    use another texture coordinate format and a lot more:

        struct nk_font_config cfg = nk_font_config(font_pixel_height);
        cfg.merge_mode = nk_false or nk_true;
        cfg.range = nk_font_korean_glyph_ranges();
        cfg.coord_type = NK_COORD_PIXEL;
        nk_font *font = nk_font_atlas_add_from_file(&atlas, "Path/To/Your/TTF_Font.ttf", 13, &cfg);

*/
struct nk_user_font_glyph;
typedef float(*nk_text_width_f)(nk_handle, float h, const char*, int len);
typedef void(*nk_query_font_glyph_f)(nk_handle handle, float font_height,
                                    struct nk_user_font_glyph *glyph,
                                    nk_rune codepoint, nk_rune next_codepoint);

#if defined(NK_INCLUDE_VERTEX_BUFFER_OUTPUT) || defined(NK_INCLUDE_SOFTWARE_FONT)
struct nk_user_font_glyph {
    struct nk_vec2 uv[2];
    /* texture coordinates */
    struct nk_vec2 offset;
    /* offset between top left and glyph */
    float width, height;
    /* size of the glyph  */
    float xadvance;
    /* offset to the next glyph */
};
#endif

struct nk_user_font {
    nk_handle userdata;
    /* user provided font handle */
    float height;
    /* max height of the font */
    nk_text_width_f width;
    /* font string width in pixel callback */
#ifdef NK_INCLUDE_VERTEX_BUFFER_OUTPUT
    nk_query_font_glyph_f query;
    /* font glyph callback to query drawing info */
    nk_handle texture;
    /* texture handle to the used font atlas or texture */
#endif
};

#ifdef NK_INCLUDE_FONT_BAKING
enum nk_font_coord_type {
    NK_COORD_UV, /* texture coordinates inside font glyphs are clamped between 0-1 */
    NK_COORD_PIXEL /* texture coordinates inside font glyphs are in absolute pixel */
};

struct nk_font;
struct nk_baked_font {
    float height;
    /* height of the font  */
    float ascent, descent;
    /* font glyphs ascent and descent  */
    nk_rune glyph_offset;
    /* glyph array offset inside the font glyph baking output array  */
    nk_rune glyph_count;
    /* number of glyphs of this font inside the glyph baking array output */
    const nk_rune *ranges;
    /* font codepoint ranges as pairs of (from/to) and 0 as last element */
};

struct nk_font_config {
    struct nk_font_config *next;
    /* NOTE: only used internally */
    void *ttf_blob;
    /* pointer to loaded TTF file memory block.
     * NOTE: not needed for nk_font_atlas_add_from_memory and nk_font_atlas_add_from_file. */
    nk_size ttf_size;
    /* size of the loaded TTF file memory block
     * NOTE: not needed for nk_font_atlas_add_from_memory and nk_font_atlas_add_from_file. */

    unsigned char ttf_data_owned_by_atlas;
    /* used inside font atlas: default to: 0*/
    unsigned char merge_mode;
    /* merges this font into the last font */
    unsigned char pixel_snap;
    /* align every character to pixel boundary (if true set oversample (1,1)) */
    unsigned char oversample_v, oversample_h;
    /* rasterize at hight quality for sub-pixel position */
    unsigned char padding[3];

    float size;
    /* baked pixel height of the font */
    enum nk_font_coord_type coord_type;
    /* texture coordinate format with either pixel or UV coordinates */
    struct nk_vec2 spacing;
    /* extra pixel spacing between glyphs  */
    const nk_rune *range;
    /* list of unicode ranges (2 values per range, zero terminated) */
    struct nk_baked_font *font;
    /* font to setup in the baking process: NOTE: not needed for font atlas */
    nk_rune fallback_glyph;
    /* fallback glyph to use if a given rune is not found */
    struct nk_font_config *n;
    struct nk_font_config *p;
};

struct nk_font_glyph {
    nk_rune codepoint;
    float xadvance;
    float x0, y0, x1, y1, w, h;
    float u0, v0, u1, v1;
};

struct nk_font {
    struct nk_font *next;
    struct nk_user_font handle;
    struct nk_baked_font info;
    float scale;
    struct nk_font_glyph *glyphs;
    const struct nk_font_glyph *fallback;
    nk_rune fallback_codepoint;
    nk_handle texture;
    struct nk_font_config *config;
};

enum nk_font_atlas_format {
    NK_FONT_ATLAS_ALPHA8,
    NK_FONT_ATLAS_RGBA32
};

struct nk_font_atlas {
    void *pixel;
    int tex_width;
    int tex_height;

    struct nk_allocator permanent;
    struct nk_allocator temporary;

    struct nk_recti custom;
    struct nk_cursor cursors[NK_CURSOR_COUNT];

    int glyph_count;
    struct nk_font_glyph *glyphs;
    struct nk_font *default_font;
    struct nk_font *fonts;
    struct nk_font_config *config;
    int font_num;
};

/* some language glyph codepoint ranges */
NK_API const nk_rune *nk_font_default_glyph_ranges(void);
NK_API const nk_rune *nk_font_chinese_glyph_ranges(void);
NK_API const nk_rune *nk_font_cyrillic_glyph_ranges(void);
NK_API const nk_rune *nk_font_korean_glyph_ranges(void);

#ifdef NK_INCLUDE_DEFAULT_ALLOCATOR
NK_API void nk_font_atlas_init_default(struct nk_font_atlas*);
#endif
NK_API void nk_font_atlas_init(struct nk_font_atlas*, struct nk_allocator*);
NK_API void nk_font_atlas_init_custom(struct nk_font_atlas*, struct nk_allocator *persistent, struct nk_allocator *transient);
NK_API void nk_font_atlas_begin(struct nk_font_atlas*);
NK_API struct nk_font_config nk_font_config(float pixel_height);
NK_API struct nk_font *nk_font_atlas_add(struct nk_font_atlas*, const struct nk_font_config*);
#ifdef NK_INCLUDE_DEFAULT_FONT
NK_API struct nk_font* nk_font_atlas_add_default(struct nk_font_atlas*, float height, const struct nk_font_config*);
#endif
NK_API struct nk_font* nk_font_atlas_add_from_memory(struct nk_font_atlas *atlas, void *memory, nk_size size, float height, const struct nk_font_config *config);
#ifdef NK_INCLUDE_STANDARD_IO
NK_API struct nk_font* nk_font_atlas_add_from_file(struct nk_font_atlas *atlas, const char *file_path, float height, const struct nk_font_config*);
#endif
NK_API struct nk_font *nk_font_atlas_add_compressed(struct nk_font_atlas*, void *memory, nk_size size, float height, const struct nk_font_config*);
NK_API struct nk_font* nk_font_atlas_add_compressed_base85(struct nk_font_atlas*, const char *data, float height, const struct nk_font_config *config);
NK_API const void* nk_font_atlas_bake(struct nk_font_atlas*, int *width, int *height, enum nk_font_atlas_format);
NK_API void nk_font_atlas_end(struct nk_font_atlas*, nk_handle tex, struct nk_draw_null_texture*);
NK_API const struct nk_font_glyph* nk_font_find_glyph(struct nk_font*, nk_rune unicode);
NK_API void nk_font_atlas_cleanup(struct nk_font_atlas *atlas);
NK_API void nk_font_atlas_clear(struct nk_font_atlas*);

#endif

/* ==============================================================
 *
 *                          MEMORY BUFFER
 *
 * ===============================================================*/
/*  A basic (double)-buffer with linear allocation and resetting as only
    freeing policy. The buffer's main purpose is to control all memory management
    inside the GUI toolkit and still leave memory control as much as possible in
    the hand of the user while also making sure the library is easy to use if
    not as much control is needed.
    In general all memory inside this library can be provided from the user in
    three different ways.

    The first way and the one providing most control is by just passing a fixed
    size memory block. In this case all control lies in the hand of the user
    since he can exactly control where the memory comes from and how much memory
    the library should consume. Of course using the fixed size API removes the
    ability to automatically resize a buffer if not enough memory is provided so
    you have to take over the resizing. While being a fixed sized buffer sounds
    quite limiting, it is very effective in this library since the actual memory
    consumption is quite stable and has a fixed upper bound for a lot of cases.

    If you don't want to think about how much memory the library should allocate
    at all time or have a very dynamic UI with unpredictable memory consumption
    habits but still want control over memory allocation you can use the dynamic
    allocator based API. The allocator consists of two callbacks for allocating
    and freeing memory and optional userdata so you can plugin your own allocator.

    The final and easiest way can be used by defining
    NK_INCLUDE_DEFAULT_ALLOCATOR which uses the standard library memory
    allocation functions malloc and free and takes over complete control over
    memory in this library.
*/
struct nk_memory_status {
    void *memory;
    unsigned int type;
    nk_size size;
    nk_size allocated;
    nk_size needed;
    nk_size calls;
};

enum nk_allocation_type {
    NK_BUFFER_FIXED,
    NK_BUFFER_DYNAMIC
};

enum nk_buffer_allocation_type {
    NK_BUFFER_FRONT,
    NK_BUFFER_BACK,
    NK_BUFFER_MAX
};

struct nk_buffer_marker {
    int active;
    nk_size offset;
};

struct nk_memory {void *ptr;nk_size size;};
struct nk_buffer {
    struct nk_buffer_marker marker[NK_BUFFER_MAX];
    /* buffer marker to free a buffer to a certain offset */
    struct nk_allocator pool;
    /* allocator callback for dynamic buffers */
    enum nk_allocation_type type;
    /* memory management type */
    struct nk_memory memory;
    /* memory and size of the current memory block */
    float grow_factor;
    /* growing factor for dynamic memory management */
    nk_size allocated;
    /* total amount of memory allocated */
    nk_size needed;
    /* totally consumed memory given that enough memory is present */
    nk_size calls;
    /* number of allocation calls */
    nk_size size;
    /* current size of the buffer */
};

#ifdef NK_INCLUDE_DEFAULT_ALLOCATOR
NK_API void nk_buffer_init_default(struct nk_buffer*);
#endif
NK_API void nk_buffer_init(struct nk_buffer*, const struct nk_allocator*, nk_size size);
NK_API void nk_buffer_init_fixed(struct nk_buffer*, void *memory, nk_size size);
NK_API void nk_buffer_info(struct nk_memory_status*, struct nk_buffer*);
NK_API void nk_buffer_push(struct nk_buffer*, enum nk_buffer_allocation_type type, const void *memory, nk_size size, nk_size align);
NK_API void nk_buffer_mark(struct nk_buffer*, enum nk_buffer_allocation_type type);
NK_API void nk_buffer_reset(struct nk_buffer*, enum nk_buffer_allocation_type type);
NK_API void nk_buffer_clear(struct nk_buffer*);
NK_API void nk_buffer_free(struct nk_buffer*);
NK_API void *nk_buffer_memory(struct nk_buffer*);
NK_API const void *nk_buffer_memory_const(const struct nk_buffer*);
NK_API nk_size nk_buffer_total(struct nk_buffer*);

/* ==============================================================
 *
 *                          STRING
 *
 * ===============================================================*/
/*  Basic string buffer which is only used in context with the text editor
 *  to manage and manipulate dynamic or fixed size string content. This is _NOT_
 *  the default string handling method. The only instance you should have any contact
 *  with this API is if you interact with an `nk_text_edit` object inside one of the
 *  copy and paste functions and even there only for more advanced cases. */
struct nk_str {
    struct nk_buffer buffer;
    int len; /* in codepoints/runes/glyphs */
};

#ifdef NK_INCLUDE_DEFAULT_ALLOCATOR
NK_API void nk_str_init_default(struct nk_str*);
#endif
NK_API void nk_str_init(struct nk_str*, const struct nk_allocator*, nk_size size);
NK_API void nk_str_init_fixed(struct nk_str*, void *memory, nk_size size);
NK_API void nk_str_clear(struct nk_str*);
NK_API void nk_str_free(struct nk_str*);

NK_API int nk_str_append_text_char(struct nk_str*, const char*, int);
NK_API int nk_str_append_str_char(struct nk_str*, const char*);
NK_API int nk_str_append_text_utf8(struct nk_str*, const char*, int);
NK_API int nk_str_append_str_utf8(struct nk_str*, const char*);
NK_API int nk_str_append_text_runes(struct nk_str*, const nk_rune*, int);
NK_API int nk_str_append_str_runes(struct nk_str*, const nk_rune*);

NK_API int nk_str_insert_at_char(struct nk_str*, int pos, const char*, int);
NK_API int nk_str_insert_at_rune(struct nk_str*, int pos, const char*, int);

NK_API int nk_str_insert_text_char(struct nk_str*, int pos, const char*, int);
NK_API int nk_str_insert_str_char(struct nk_str*, int pos, const char*);
NK_API int nk_str_insert_text_utf8(struct nk_str*, int pos, const char*, int);
NK_API int nk_str_insert_str_utf8(struct nk_str*, int pos, const char*);
NK_API int nk_str_insert_text_runes(struct nk_str*, int pos, const nk_rune*, int);
NK_API int nk_str_insert_str_runes(struct nk_str*, int pos, const nk_rune*);

NK_API void nk_str_remove_chars(struct nk_str*, int len);
NK_API void nk_str_remove_runes(struct nk_str *str, int len);
NK_API void nk_str_delete_chars(struct nk_str*, int pos, int len);
NK_API void nk_str_delete_runes(struct nk_str*, int pos, int len);

NK_API char *nk_str_at_char(struct nk_str*, int pos);
NK_API char *nk_str_at_rune(struct nk_str*, int pos, nk_rune *unicode, int *len);
NK_API nk_rune nk_str_rune_at(const struct nk_str*, int pos);
NK_API const char *nk_str_at_char_const(const struct nk_str*, int pos);
NK_API const char *nk_str_at_const(const struct nk_str*, int pos, nk_rune *unicode, int *len);

NK_API char *nk_str_get(struct nk_str*);
NK_API const char *nk_str_get_const(const struct nk_str*);
NK_API int nk_str_len(struct nk_str*);
NK_API int nk_str_len_char(struct nk_str*);

/*===============================================================
 *
 *                      TEXT EDITOR
 *
 * ===============================================================*/
/* Editing text in this library is handled by either `nk_edit_string` or
 * `nk_edit_buffer`. But like almost everything in this library there are multiple
 * ways of doing it and a balance between control and ease of use with memory
 * as well as functionality controlled by flags.
 *
 * This library generally allows three different levels of memory control:
 * First of is the most basic way of just providing a simple char array with
 * string length. This method is probably the easiest way of handling simple
 * user text input. Main upside is complete control over memory while the biggest
 * downside in comparison with the other two approaches is missing undo/redo.
 *
 * For UIs that require undo/redo the second way was created. It is based on
 * a fixed size nk_text_edit struct, which has an internal undo/redo stack.
 * This is mainly useful if you want something more like a text editor but don't want
 * to have a dynamically growing buffer.
 *
 * The final way is using a dynamically growing nk_text_edit struct, which
 * has both a default version if you don't care where memory comes from and an
 * allocator version if you do. While the text editor is quite powerful for its
 * complexity I would not recommend editing gigabytes of data with it.
 * It is rather designed for uses cases which make sense for a GUI library not for
 * an full blown text editor.
 */
#ifndef NK_TEXTEDIT_UNDOSTATECOUNT
#define NK_TEXTEDIT_UNDOSTATECOUNT     99
#endif

#ifndef NK_TEXTEDIT_UNDOCHARCOUNT
#define NK_TEXTEDIT_UNDOCHARCOUNT      999
#endif

struct nk_text_edit;
struct nk_clipboard {
    nk_handle userdata;
    nk_plugin_paste paste;
    nk_plugin_copy copy;
};

struct nk_text_undo_record {
   int where;
   short insert_length;
   short delete_length;
   short char_storage;
};

struct nk_text_undo_state {
   struct nk_text_undo_record undo_rec[NK_TEXTEDIT_UNDOSTATECOUNT];
   nk_rune undo_char[NK_TEXTEDIT_UNDOCHARCOUNT];
   short undo_point;
   short redo_point;
   short undo_char_point;
   short redo_char_point;
};

enum nk_text_edit_type {
    NK_TEXT_EDIT_SINGLE_LINE,
    NK_TEXT_EDIT_MULTI_LINE
};

enum nk_text_edit_mode {
    NK_TEXT_EDIT_MODE_VIEW,
    NK_TEXT_EDIT_MODE_INSERT,
    NK_TEXT_EDIT_MODE_REPLACE
};

struct nk_text_edit {
    struct nk_clipboard clip;
    struct nk_str string;
    nk_plugin_filter filter;
    struct nk_vec2 scrollbar;

    int cursor;
    int select_start;
    int select_end;
    unsigned char mode;
    unsigned char cursor_at_end_of_line;
    unsigned char initialized;
    unsigned char has_preferred_x;
    unsigned char single_line;
    unsigned char active;
    unsigned char padding1;
    float preferred_x;
    struct nk_text_undo_state undo;
};

/* filter function */
NK_API int nk_filter_default(const struct nk_text_edit*, nk_rune unicode);
NK_API int nk_filter_ascii(const struct nk_text_edit*, nk_rune unicode);
NK_API int nk_filter_float(const struct nk_text_edit*, nk_rune unicode);
NK_API int nk_filter_decimal(const struct nk_text_edit*, nk_rune unicode);
NK_API int nk_filter_hex(const struct nk_text_edit*, nk_rune unicode);
NK_API int nk_filter_oct(const struct nk_text_edit*, nk_rune unicode);
NK_API int nk_filter_binary(const struct nk_text_edit*, nk_rune unicode);

/* text editor */
#ifdef NK_INCLUDE_DEFAULT_ALLOCATOR
NK_API void nk_textedit_init_default(struct nk_text_edit*);
#endif
NK_API void nk_textedit_init(struct nk_text_edit*, struct nk_allocator*, nk_size size);
NK_API void nk_textedit_init_fixed(struct nk_text_edit*, void *memory, nk_size size);
NK_API void nk_textedit_free(struct nk_text_edit*);
NK_API void nk_textedit_text(struct nk_text_edit*, const char*, int total_len);
NK_API void nk_textedit_delete(struct nk_text_edit*, int where, int len);
NK_API void nk_textedit_delete_selection(struct nk_text_edit*);
NK_API void nk_textedit_select_all(struct nk_text_edit*);
NK_API int nk_textedit_cut(struct nk_text_edit*);
NK_API int nk_textedit_paste(struct nk_text_edit*, char const*, int len);
NK_API void nk_textedit_undo(struct nk_text_edit*);
NK_API void nk_textedit_redo(struct nk_text_edit*);

/* ===============================================================
 *
 *                          DRAWING
 *
 * ===============================================================*/
/*  This library was designed to be render backend agnostic so it does
    not draw anything to screen. Instead all drawn shapes, widgets
    are made of, are buffered into memory and make up a command queue.
    Each frame therefore fills the command buffer with draw commands
    that then need to be executed by the user and his own render backend.
    After that the command buffer needs to be cleared and a new frame can be
    started. It is probably important to note that the command buffer is the main
    drawing API and the optional vertex buffer API only takes this format and
    converts it into a hardware accessible format.

    To use the command queue to draw your own widgets you can access the
    command buffer of each window by calling `nk_window_get_canvas` after
    previously having called `nk_begin`:

        void draw_red_rectangle_widget(struct nk_context *ctx)
        {
            struct nk_command_buffer *canvas;
            struct nk_input *input = &ctx->input;
            canvas = nk_window_get_canvas(ctx);

            struct nk_rect space;
            enum nk_widget_layout_states state;
            state = nk_widget(&space, ctx);
            if (!state) return;

            if (state != NK_WIDGET_ROM)
                update_your_widget_by_user_input(...);
            nk_fill_rect(canvas, space, 0, nk_rgb(255,0,0));
        }

        if (nk_begin(...)) {
            nk_layout_row_dynamic(ctx, 25, 1);
            draw_red_rectangle_widget(ctx);
        }
        nk_end(..)

    Important to know if you want to create your own widgets is the `nk_widget`
    call. It allocates space on the panel reserved for this widget to be used,
    but also returns the state of the widget space. If your widget is not seen and does
    not have to be updated it is '0' and you can just return. If it only has
    to be drawn the state will be `NK_WIDGET_ROM` otherwise you can do both
    update and draw your widget. The reason for separating is to only draw and
    update what is actually necessary which is crucial for performance.
*/
enum nk_command_type {
    NK_COMMAND_NOP,
    NK_COMMAND_SCISSOR,
    NK_COMMAND_LINE,
    NK_COMMAND_CURVE,
    NK_COMMAND_RECT,
    NK_COMMAND_RECT_FILLED,
    NK_COMMAND_RECT_MULTI_COLOR,
    NK_COMMAND_CIRCLE,
    NK_COMMAND_CIRCLE_FILLED,
    NK_COMMAND_ARC,
    NK_COMMAND_ARC_FILLED,
    NK_COMMAND_TRIANGLE,
    NK_COMMAND_TRIANGLE_FILLED,
    NK_COMMAND_POLYGON,
    NK_COMMAND_POLYGON_FILLED,
    NK_COMMAND_POLYLINE,
    NK_COMMAND_TEXT,
    NK_COMMAND_IMAGE,
    NK_COMMAND_CUSTOM
};

/* command base and header of every command inside the buffer */
struct nk_command {
    enum nk_command_type type;
    nk_size next;
#ifdef NK_INCLUDE_COMMAND_USERDATA
    nk_handle userdata;
#endif
};

struct nk_command_scissor {
    struct nk_command header;
    short x, y;
    unsigned short w, h;
};

struct nk_command_line {
    struct nk_command header;
    unsigned short line_thickness;
    struct nk_vec2i begin;
    struct nk_vec2i end;
    struct nk_color color;
};

struct nk_command_curve {
    struct nk_command header;
    unsigned short line_thickness;
    struct nk_vec2i begin;
    struct nk_vec2i end;
    struct nk_vec2i ctrl[2];
    struct nk_color color;
};

struct nk_command_rect {
    struct nk_command header;
    unsigned short rounding;
    unsigned short line_thickness;
    short x, y;
    unsigned short w, h;
    struct nk_color color;
};

struct nk_command_rect_filled {
    struct nk_command header;
    unsigned short rounding;
    short x, y;
    unsigned short w, h;
    struct nk_color color;
};

struct nk_command_rect_multi_color {
    struct nk_command header;
    short x, y;
    unsigned short w, h;
    struct nk_color left;
    struct nk_color top;
    struct nk_color bottom;
    struct nk_color right;
};

struct nk_command_triangle {
    struct nk_command header;
    unsigned short line_thickness;
    struct nk_vec2i a;
    struct nk_vec2i b;
    struct nk_vec2i c;
    struct nk_color color;
};

struct nk_command_triangle_filled {
    struct nk_command header;
    struct nk_vec2i a;
    struct nk_vec2i b;
    struct nk_vec2i c;
    struct nk_color color;
};

struct nk_command_circle {
    struct nk_command header;
    short x, y;
    unsigned short line_thickness;
    unsigned short w, h;
    struct nk_color color;
};

struct nk_command_circle_filled {
    struct nk_command header;
    short x, y;
    unsigned short w, h;
    struct nk_color color;
};

struct nk_command_arc {
    struct nk_command header;
    short cx, cy;
    unsigned short r;
    unsigned short line_thickness;
    float a[2];
    struct nk_color color;
};

struct nk_command_arc_filled {
    struct nk_command header;
    short cx, cy;
    unsigned short r;
    float a[2];
    struct nk_color color;
};

struct nk_command_polygon {
    struct nk_command header;
    struct nk_color color;
    unsigned short line_thickness;
    unsigned short point_count;
    struct nk_vec2i points[1];
};

struct nk_command_polygon_filled {
    struct nk_command header;
    struct nk_color color;
    unsigned short point_count;
    struct nk_vec2i points[1];
};

struct nk_command_polyline {
    struct nk_command header;
    struct nk_color color;
    unsigned short line_thickness;
    unsigned short point_count;
    struct nk_vec2i points[1];
};

struct nk_command_image {
    struct nk_command header;
    short x, y;
    unsigned short w, h;
    struct nk_image img;
    struct nk_color col;
};

typedef void (*nk_command_custom_callback)(void *canvas, short x,short y,
    unsigned short w, unsigned short h, nk_handle callback_data);
struct nk_command_custom {
    struct nk_command header;
    short x, y;
    unsigned short w, h;
    nk_handle callback_data;
    nk_command_custom_callback callback;
};

struct nk_command_text {
    struct nk_command header;
    const struct nk_user_font *font;
    struct nk_color background;
    struct nk_color foreground;
    short x, y;
    unsigned short w, h;
    float height;
    int length;
    char string[1];
};

enum nk_command_clipping {
    NK_CLIPPING_OFF = nk_false,
    NK_CLIPPING_ON = nk_true
};

struct nk_command_buffer {
    struct nk_buffer *base;
    struct nk_rect clip;
    int use_clipping;
    nk_handle userdata;
    nk_size begin, end, last;
};

/* shape outlines */
NK_API void nk_stroke_line(struct nk_command_buffer *b, float x0, float y0, float x1, float y1, float line_thickness, struct nk_color);
NK_API void nk_stroke_curve(struct nk_command_buffer*, float, float, float, float, float, float, float, float, float line_thickness, struct nk_color);
NK_API void nk_stroke_rect(struct nk_command_buffer*, struct nk_rect, float rounding, float line_thickness, struct nk_color);
NK_API void nk_stroke_circle(struct nk_command_buffer*, struct nk_rect, float line_thickness, struct nk_color);
NK_API void nk_stroke_arc(struct nk_command_buffer*, float cx, float cy, float radius, float a_min, float a_max, float line_thickness, struct nk_color);
NK_API void nk_stroke_triangle(struct nk_command_buffer*, float, float, float, float, float, float, float line_thichness, struct nk_color);
NK_API void nk_stroke_polyline(struct nk_command_buffer*, float *points, int point_count, float line_thickness, struct nk_color col);
NK_API void nk_stroke_polygon(struct nk_command_buffer*, float*, int point_count, float line_thickness, struct nk_color);

/* filled shades */
NK_API void nk_fill_rect(struct nk_command_buffer*, struct nk_rect, float rounding, struct nk_color);
NK_API void nk_fill_rect_multi_color(struct nk_command_buffer*, struct nk_rect, struct nk_color left, struct nk_color top, struct nk_color right, struct nk_color bottom);
NK_API void nk_fill_circle(struct nk_command_buffer*, struct nk_rect, struct nk_color);
NK_API void nk_fill_arc(struct nk_command_buffer*, float cx, float cy, float radius, float a_min, float a_max, struct nk_color);
NK_API void nk_fill_triangle(struct nk_command_buffer*, float x0, float y0, float x1, float y1, float x2, float y2, struct nk_color);
NK_API void nk_fill_polygon(struct nk_command_buffer*, float*, int point_count, struct nk_color);

/* misc */
NK_API void nk_draw_image(struct nk_command_buffer*, struct nk_rect, const struct nk_image*, struct nk_color);
NK_API void nk_draw_text(struct nk_command_buffer*, struct nk_rect, const char *text, int len, const struct nk_user_font*, struct nk_color, struct nk_color);
NK_API void nk_push_scissor(struct nk_command_buffer*, struct nk_rect);
NK_API void nk_push_custom(struct nk_command_buffer*, struct nk_rect, nk_command_custom_callback, nk_handle usr);

/* ===============================================================
 *
 *                          INPUT
 *
 * ===============================================================*/
struct nk_mouse_button {
    int down;
    unsigned int clicked;
    struct nk_vec2 clicked_pos;
};
struct nk_mouse {
    struct nk_mouse_button buttons[NK_BUTTON_MAX];
    struct nk_vec2 pos;
    struct nk_vec2 prev;
    struct nk_vec2 delta;
    struct nk_vec2 scroll_delta;
    unsigned char grab;
    unsigned char grabbed;
    unsigned char ungrab;
};

struct nk_key {
    int down;
    unsigned int clicked;
};
struct nk_keyboard {
    struct nk_key keys[NK_KEY_MAX];
    char text[NK_INPUT_MAX];
    int text_len;
};

struct nk_input {
    struct nk_keyboard keyboard;
    struct nk_mouse mouse;
};

NK_API int nk_input_has_mouse_click(const struct nk_input*, enum nk_buttons);
NK_API int nk_input_has_mouse_click_in_rect(const struct nk_input*, enum nk_buttons, struct nk_rect);
NK_API int nk_input_has_mouse_click_down_in_rect(const struct nk_input*, enum nk_buttons, struct nk_rect, int down);
NK_API int nk_input_is_mouse_click_in_rect(const struct nk_input*, enum nk_buttons, struct nk_rect);
NK_API int nk_input_is_mouse_click_down_in_rect(const struct nk_input *i, enum nk_buttons id, struct nk_rect b, int down);
NK_API int nk_input_any_mouse_click_in_rect(const struct nk_input*, struct nk_rect);
NK_API int nk_input_is_mouse_prev_hovering_rect(const struct nk_input*, struct nk_rect);
NK_API int nk_input_is_mouse_hovering_rect(const struct nk_input*, struct nk_rect);
NK_API int nk_input_mouse_clicked(const struct nk_input*, enum nk_buttons, struct nk_rect);
NK_API int nk_input_is_mouse_down(const struct nk_input*, enum nk_buttons);
NK_API int nk_input_is_mouse_pressed(const struct nk_input*, enum nk_buttons);
NK_API int nk_input_is_mouse_released(const struct nk_input*, enum nk_buttons);
NK_API int nk_input_is_key_pressed(const struct nk_input*, enum nk_keys);
NK_API int nk_input_is_key_released(const struct nk_input*, enum nk_keys);
NK_API int nk_input_is_key_down(const struct nk_input*, enum nk_keys);

/* ===============================================================
 *
 *                          DRAW LIST
 *
 * ===============================================================*/
#ifdef NK_INCLUDE_VERTEX_BUFFER_OUTPUT
/*  The optional vertex buffer draw list provides a 2D drawing context
    with antialiasing functionality which takes basic filled or outlined shapes
    or a path and outputs vertexes, elements and draw commands.
    The actual draw list API is not required to be used directly while using this
    library since converting the default library draw command output is done by
    just calling `nk_convert` but I decided to still make this library accessible
    since it can be useful.

    The draw list is based on a path buffering and polygon and polyline
    rendering API which allows a lot of ways to draw 2D content to screen.
    In fact it is probably more powerful than needed but allows even more crazy
    things than this library provides by default.
*/
#ifdef NK_UINT_DRAW_INDEX
typedef nk_uint nk_draw_index;
#else
typedef nk_ushort nk_draw_index;
#endif
enum nk_draw_list_stroke {
    NK_STROKE_OPEN = nk_false,
    /* build up path has no connection back to the beginning */
    NK_STROKE_CLOSED = nk_true
    /* build up path has a connection back to the beginning */
};

enum nk_draw_vertex_layout_attribute {
    NK_VERTEX_POSITION,
    NK_VERTEX_COLOR,
    NK_VERTEX_TEXCOORD,
    NK_VERTEX_ATTRIBUTE_COUNT
};

enum nk_draw_vertex_layout_format {
    NK_FORMAT_SCHAR,
    NK_FORMAT_SSHORT,
    NK_FORMAT_SINT,
    NK_FORMAT_UCHAR,
    NK_FORMAT_USHORT,
    NK_FORMAT_UINT,
    NK_FORMAT_FLOAT,
    NK_FORMAT_DOUBLE,

NK_FORMAT_COLOR_BEGIN,
    NK_FORMAT_R8G8B8 = NK_FORMAT_COLOR_BEGIN,
    NK_FORMAT_R16G15B16,
    NK_FORMAT_R32G32B32,

    NK_FORMAT_R8G8B8A8,
    NK_FORMAT_B8G8R8A8,
    NK_FORMAT_R16G15B16A16,
    NK_FORMAT_R32G32B32A32,
    NK_FORMAT_R32G32B32A32_FLOAT,
    NK_FORMAT_R32G32B32A32_DOUBLE,

    NK_FORMAT_RGB32,
    NK_FORMAT_RGBA32,
NK_FORMAT_COLOR_END = NK_FORMAT_RGBA32,
    NK_FORMAT_COUNT
};

#define NK_VERTEX_LAYOUT_END NK_VERTEX_ATTRIBUTE_COUNT,NK_FORMAT_COUNT,0
struct nk_draw_vertex_layout_element {
    enum nk_draw_vertex_layout_attribute attribute;
    enum nk_draw_vertex_layout_format format;
    nk_size offset;
};

struct nk_draw_command {
    unsigned int elem_count;
    /* number of elements in the current draw batch */
    struct nk_rect clip_rect;
    /* current screen clipping rectangle */
    nk_handle texture;
    /* current texture to set */
#ifdef NK_INCLUDE_COMMAND_USERDATA
    nk_handle userdata;
#endif
};

struct nk_draw_list {
    struct nk_rect clip_rect;
    struct nk_vec2 circle_vtx[12];
    struct nk_convert_config config;

    struct nk_buffer *buffer;
    struct nk_buffer *vertices;
    struct nk_buffer *elements;

    unsigned int element_count;
    unsigned int vertex_count;
    unsigned int cmd_count;
    nk_size cmd_offset;

    unsigned int path_count;
    unsigned int path_offset;

    enum nk_anti_aliasing line_AA;
    enum nk_anti_aliasing shape_AA;

#ifdef NK_INCLUDE_COMMAND_USERDATA
    nk_handle userdata;
#endif
};

/* draw list */
NK_API void nk_draw_list_init(struct nk_draw_list*);
NK_API void nk_draw_list_setup(struct nk_draw_list*, const struct nk_convert_config*, struct nk_buffer *cmds, struct nk_buffer *vertices, struct nk_buffer *elements, enum nk_anti_aliasing line_aa,enum nk_anti_aliasing shape_aa);

/* drawing */
#define nk_draw_list_foreach(cmd, can, b) for((cmd)=nk__draw_list_begin(can, b); (cmd)!=0; (cmd)=nk__draw_list_next(cmd, b, can))
NK_API const struct nk_draw_command* nk__draw_list_begin(const struct nk_draw_list*, const struct nk_buffer*);
NK_API const struct nk_draw_command* nk__draw_list_next(const struct nk_draw_command*, const struct nk_buffer*, const struct nk_draw_list*);
NK_API const struct nk_draw_command* nk__draw_list_end(const struct nk_draw_list*, const struct nk_buffer*);

/* path */
NK_API void nk_draw_list_path_clear(struct nk_draw_list*);
NK_API void nk_draw_list_path_line_to(struct nk_draw_list*, struct nk_vec2 pos);
NK_API void nk_draw_list_path_arc_to_fast(struct nk_draw_list*, struct nk_vec2 center, float radius, int a_min, int a_max);
NK_API void nk_draw_list_path_arc_to(struct nk_draw_list*, struct nk_vec2 center, float radius, float a_min, float a_max, unsigned int segments);
NK_API void nk_draw_list_path_rect_to(struct nk_draw_list*, struct nk_vec2 a, struct nk_vec2 b, float rounding);
NK_API void nk_draw_list_path_curve_to(struct nk_draw_list*, struct nk_vec2 p2, struct nk_vec2 p3, struct nk_vec2 p4, unsigned int num_segments);
NK_API void nk_draw_list_path_fill(struct nk_draw_list*, struct nk_color);
NK_API void nk_draw_list_path_stroke(struct nk_draw_list*, struct nk_color, enum nk_draw_list_stroke closed, float thickness);

/* stroke */
NK_API void nk_draw_list_stroke_line(struct nk_draw_list*, struct nk_vec2 a, struct nk_vec2 b, struct nk_color, float thickness);
NK_API void nk_draw_list_stroke_rect(struct nk_draw_list*, struct nk_rect rect, struct nk_color, float rounding, float thickness);
NK_API void nk_draw_list_stroke_triangle(struct nk_draw_list*, struct nk_vec2 a, struct nk_vec2 b, struct nk_vec2 c, struct nk_color, float thickness);
NK_API void nk_draw_list_stroke_circle(struct nk_draw_list*, struct nk_vec2 center, float radius, struct nk_color, unsigned int segs, float thickness);
NK_API void nk_draw_list_stroke_curve(struct nk_draw_list*, struct nk_vec2 p0, struct nk_vec2 cp0, struct nk_vec2 cp1, struct nk_vec2 p1, struct nk_color, unsigned int segments, float thickness);
NK_API void nk_draw_list_stroke_poly_line(struct nk_draw_list*, const struct nk_vec2 *pnts, const unsigned int cnt, struct nk_color, enum nk_draw_list_stroke, float thickness, enum nk_anti_aliasing);

/* fill */
NK_API void nk_draw_list_fill_rect(struct nk_draw_list*, struct nk_rect rect, struct nk_color, float rounding);
NK_API void nk_draw_list_fill_rect_multi_color(struct nk_draw_list*, struct nk_rect rect, struct nk_color left, struct nk_color top, struct nk_color right, struct nk_color bottom);
NK_API void nk_draw_list_fill_triangle(struct nk_draw_list*, struct nk_vec2 a, struct nk_vec2 b, struct nk_vec2 c, struct nk_color);
NK_API void nk_draw_list_fill_circle(struct nk_draw_list*, struct nk_vec2 center, float radius, struct nk_color col, unsigned int segs);
NK_API void nk_draw_list_fill_poly_convex(struct nk_draw_list*, const struct nk_vec2 *points, const unsigned int count, struct nk_color, enum nk_anti_aliasing);

/* misc */
NK_API void nk_draw_list_add_image(struct nk_draw_list*, struct nk_image texture, struct nk_rect rect, struct nk_color);
NK_API void nk_draw_list_add_text(struct nk_draw_list*, const struct nk_user_font*, struct nk_rect, const char *text, int len, float font_height, struct nk_color);
#ifdef NK_INCLUDE_COMMAND_USERDATA
NK_API void nk_draw_list_push_userdata(struct nk_draw_list*, nk_handle userdata);
#endif

#endif

/* ===============================================================
 *
 *                          GUI
 *
 * ===============================================================*/
enum nk_style_item_type {
    NK_STYLE_ITEM_COLOR,
    NK_STYLE_ITEM_IMAGE
};

union nk_style_item_data {
    struct nk_image image;
    struct nk_color color;
};

struct nk_style_item {
    enum nk_style_item_type type;
    union nk_style_item_data data;
};

struct nk_style_text {
    struct nk_color color;
    struct nk_vec2 padding;
};

struct nk_style_button {
    /* background */
    struct nk_style_item normal;
    struct nk_style_item hover;
    struct nk_style_item active;
    struct nk_color border_color;

    /* text */
    struct nk_color text_background;
    struct nk_color text_normal;
    struct nk_color text_hover;
    struct nk_color text_active;
    nk_flags text_alignment;

    /* properties */
    float border;
    float rounding;
    struct nk_vec2 padding;
    struct nk_vec2 image_padding;
    struct nk_vec2 touch_padding;

    /* optional user callbacks */
    nk_handle userdata;
    void(*draw_begin)(struct nk_command_buffer*, nk_handle userdata);
    void(*draw_end)(struct nk_command_buffer*, nk_handle userdata);
};

struct nk_style_toggle {
    /* background */
    struct nk_style_item normal;
    struct nk_style_item hover;
    struct nk_style_item active;
    struct nk_color border_color;

    /* cursor */
    struct nk_style_item cursor_normal;
    struct nk_style_item cursor_hover;

    /* text */
    struct nk_color text_normal;
    struct nk_color text_hover;
    struct nk_color text_active;
    struct nk_color text_background;
    nk_flags text_alignment;

    /* properties */
    struct nk_vec2 padding;
    struct nk_vec2 touch_padding;
    float spacing;
    float border;

    /* optional user callbacks */
    nk_handle userdata;
    void(*draw_begin)(struct nk_command_buffer*, nk_handle);
    void(*draw_end)(struct nk_command_buffer*, nk_handle);
};

struct nk_style_selectable {
    /* background (inactive) */
    struct nk_style_item normal;
    struct nk_style_item hover;
    struct nk_style_item pressed;

    /* background (active) */
    struct nk_style_item normal_active;
    struct nk_style_item hover_active;
    struct nk_style_item pressed_active;

    /* text color (inactive) */
    struct nk_color text_normal;
    struct nk_color text_hover;
    struct nk_color text_pressed;

    /* text color (active) */
    struct nk_color text_normal_active;
    struct nk_color text_hover_active;
    struct nk_color text_pressed_active;
    struct nk_color text_background;
    nk_flags text_alignment;

    /* properties */
    float rounding;
    struct nk_vec2 padding;
    struct nk_vec2 touch_padding;
    struct nk_vec2 image_padding;

    /* optional user callbacks */
    nk_handle userdata;
    void(*draw_begin)(struct nk_command_buffer*, nk_handle);
    void(*draw_end)(struct nk_command_buffer*, nk_handle);
};

struct nk_style_slider {
    /* background */
    struct nk_style_item normal;
    struct nk_style_item hover;
    struct nk_style_item active;
    struct nk_color border_color;

    /* background bar */
    struct nk_color bar_normal;
    struct nk_color bar_hover;
    struct nk_color bar_active;
    struct nk_color bar_filled;

    /* cursor */
    struct nk_style_item cursor_normal;
    struct nk_style_item cursor_hover;
    struct nk_style_item cursor_active;

    /* properties */
    float border;
    float rounding;
    float bar_height;
    struct nk_vec2 padding;
    struct nk_vec2 spacing;
    struct nk_vec2 cursor_size;

    /* optional buttons */
    int show_buttons;
    struct nk_style_button inc_button;
    struct nk_style_button dec_button;
    enum nk_symbol_type inc_symbol;
    enum nk_symbol_type dec_symbol;

    /* optional user callbacks */
    nk_handle userdata;
    void(*draw_begin)(struct nk_command_buffer*, nk_handle);
    void(*draw_end)(struct nk_command_buffer*, nk_handle);
};

struct nk_style_progress {
    /* background */
    struct nk_style_item normal;
    struct nk_style_item hover;
    struct nk_style_item active;
    struct nk_color border_color;

    /* cursor */
    struct nk_style_item cursor_normal;
    struct nk_style_item cursor_hover;
    struct nk_style_item cursor_active;
    struct nk_color cursor_border_color;

    /* properties */
    float rounding;
    float border;
    float cursor_border;
    float cursor_rounding;
    struct nk_vec2 padding;

    /* optional user callbacks */
    nk_handle userdata;
    void(*draw_begin)(struct nk_command_buffer*, nk_handle);
    void(*draw_end)(struct nk_command_buffer*, nk_handle);
};

struct nk_style_scrollbar {
    /* background */
    struct nk_style_item normal;
    struct nk_style_item hover;
    struct nk_style_item active;
    struct nk_color border_color;

    /* cursor */
    struct nk_style_item cursor_normal;
    struct nk_style_item cursor_hover;
    struct nk_style_item cursor_active;
    struct nk_color cursor_border_color;

    /* properties */
    float border;
    float rounding;
    float border_cursor;
    float rounding_cursor;
    struct nk_vec2 padding;

    /* optional buttons */
    int show_buttons;
    struct nk_style_button inc_button;
    struct nk_style_button dec_button;
    enum nk_symbol_type inc_symbol;
    enum nk_symbol_type dec_symbol;

    /* optional user callbacks */
    nk_handle userdata;
    void(*draw_begin)(struct nk_command_buffer*, nk_handle);
    void(*draw_end)(struct nk_command_buffer*, nk_handle);
};

struct nk_style_edit {
    /* background */
    struct nk_style_item normal;
    struct nk_style_item hover;
    struct nk_style_item active;
    struct nk_color border_color;
    struct nk_style_scrollbar scrollbar;

    /* cursor  */
    struct nk_color cursor_normal;
    struct nk_color cursor_hover;
    struct nk_color cursor_text_normal;
    struct nk_color cursor_text_hover;

    /* text (unselected) */
    struct nk_color text_normal;
    struct nk_color text_hover;
    struct nk_color text_active;

    /* text (selected) */
    struct nk_color selected_normal;
    struct nk_color selected_hover;
    struct nk_color selected_text_normal;
    struct nk_color selected_text_hover;

    /* properties */
    float border;
    float rounding;
    float cursor_size;
    struct nk_vec2 scrollbar_size;
    struct nk_vec2 padding;
    float row_padding;
};

struct nk_style_property {
    /* background */
    struct nk_style_item normal;
    struct nk_style_item hover;
    struct nk_style_item active;
    struct nk_color border_color;

    /* text */
    struct nk_color label_normal;
    struct nk_color label_hover;
    struct nk_color label_active;

    /* symbols */
    enum nk_symbol_type sym_left;
    enum nk_symbol_type sym_right;

    /* properties */
    float border;
    float rounding;
    struct nk_vec2 padding;

    struct nk_style_edit edit;
    struct nk_style_button inc_button;
    struct nk_style_button dec_button;

    /* optional user callbacks */
    nk_handle userdata;
    void(*draw_begin)(struct nk_command_buffer*, nk_handle);
    void(*draw_end)(struct nk_command_buffer*, nk_handle);
};

struct nk_style_chart {
    /* colors */
    struct nk_style_item background;
    struct nk_color border_color;
    struct nk_color selected_color;
    struct nk_color color;

    /* properties */
    float border;
    float rounding;
    struct nk_vec2 padding;
};

struct nk_style_combo {
    /* background */
    struct nk_style_item normal;
    struct nk_style_item hover;
    struct nk_style_item active;
    struct nk_color border_color;

    /* label */
    struct nk_color label_normal;
    struct nk_color label_hover;
    struct nk_color label_active;

    /* symbol */
    struct nk_color symbol_normal;
    struct nk_color symbol_hover;
    struct nk_color symbol_active;

    /* button */
    struct nk_style_button button;
    enum nk_symbol_type sym_normal;
    enum nk_symbol_type sym_hover;
    enum nk_symbol_type sym_active;

    /* properties */
    float border;
    float rounding;
    struct nk_vec2 content_padding;
    struct nk_vec2 button_padding;
    struct nk_vec2 spacing;
};

struct nk_style_tab {
    /* background */
    struct nk_style_item background;
    struct nk_color border_color;
    struct nk_color text;

    /* button */
    struct nk_style_button tab_maximize_button;
    struct nk_style_button tab_minimize_button;
    struct nk_style_button node_maximize_button;
    struct nk_style_button node_minimize_button;
    enum nk_symbol_type sym_minimize;
    enum nk_symbol_type sym_maximize;

    /* properties */
    float border;
    float rounding;
    float indent;
    struct nk_vec2 padding;
    struct nk_vec2 spacing;
};

enum nk_style_header_align {
    NK_HEADER_LEFT,
    NK_HEADER_RIGHT
};
struct nk_style_window_header {
    /* background */
    struct nk_style_item normal;
    struct nk_style_item hover;
    struct nk_style_item active;

    /* button */
    struct nk_style_button close_button;
    struct nk_style_button minimize_button;
    enum nk_symbol_type close_symbol;
    enum nk_symbol_type minimize_symbol;
    enum nk_symbol_type maximize_symbol;

    /* title */
    struct nk_color label_normal;
    struct nk_color label_hover;
    struct nk_color label_active;

    /* properties */
    enum nk_style_header_align align;
    struct nk_vec2 padding;
    struct nk_vec2 label_padding;
    struct nk_vec2 spacing;
};

struct nk_style_window {
    struct nk_style_window_header header;
    struct nk_style_item fixed_background;
    struct nk_color background;

    struct nk_color border_color;
    struct nk_color popup_border_color;
    struct nk_color combo_border_color;
    struct nk_color contextual_border_color;
    struct nk_color menu_border_color;
    struct nk_color group_border_color;
    struct nk_color tooltip_border_color;
    struct nk_style_item scaler;

    float border;
    float combo_border;
    float contextual_border;
    float menu_border;
    float group_border;
    float tooltip_border;
    float popup_border;
    float min_row_height_padding;

    float rounding;
    struct nk_vec2 spacing;
    struct nk_vec2 scrollbar_size;
    struct nk_vec2 min_size;

    struct nk_vec2 padding;
    struct nk_vec2 group_padding;
    struct nk_vec2 popup_padding;
    struct nk_vec2 combo_padding;
    struct nk_vec2 contextual_padding;
    struct nk_vec2 menu_padding;
    struct nk_vec2 tooltip_padding;
};

struct nk_style {
    const struct nk_user_font *font;
    const struct nk_cursor *cursors[NK_CURSOR_COUNT];
    const struct nk_cursor *cursor_active;
    struct nk_cursor *cursor_last;
    int cursor_visible;

    struct nk_style_text text;
    struct nk_style_button button;
    struct nk_style_button contextual_button;
    struct nk_style_button menu_button;
    struct nk_style_toggle option;
    struct nk_style_toggle checkbox;
    struct nk_style_selectable selectable;
    struct nk_style_slider slider;
    struct nk_style_progress progress;
    struct nk_style_property property;
    struct nk_style_edit edit;
    struct nk_style_chart chart;
    struct nk_style_scrollbar scrollh;
    struct nk_style_scrollbar scrollv;
    struct nk_style_tab tab;
    struct nk_style_combo combo;
    struct nk_style_window window;
};

NK_API struct nk_style_item nk_style_item_image(struct nk_image img);
NK_API struct nk_style_item nk_style_item_color(struct nk_color);
NK_API struct nk_style_item nk_style_item_hide(void);

/*==============================================================
 *                          PANEL
 * =============================================================*/
#ifndef NK_MAX_LAYOUT_ROW_TEMPLATE_COLUMNS
#define NK_MAX_LAYOUT_ROW_TEMPLATE_COLUMNS 16
#endif
#ifndef NK_CHART_MAX_SLOT
#define NK_CHART_MAX_SLOT 4
#endif

enum nk_panel_type {
    NK_PANEL_NONE       = 0,
    NK_PANEL_WINDOW     = NK_FLAG(0),
    NK_PANEL_GROUP      = NK_FLAG(1),
    NK_PANEL_POPUP      = NK_FLAG(2),
    NK_PANEL_CONTEXTUAL = NK_FLAG(4),
    NK_PANEL_COMBO      = NK_FLAG(5),
    NK_PANEL_MENU       = NK_FLAG(6),
    NK_PANEL_TOOLTIP    = NK_FLAG(7)
};
enum nk_panel_set {
    NK_PANEL_SET_NONBLOCK = NK_PANEL_CONTEXTUAL|NK_PANEL_COMBO|NK_PANEL_MENU|NK_PANEL_TOOLTIP,
    NK_PANEL_SET_POPUP = NK_PANEL_SET_NONBLOCK|NK_PANEL_POPUP,
    NK_PANEL_SET_SUB = NK_PANEL_SET_POPUP|NK_PANEL_GROUP
};

struct nk_chart_slot {
    enum nk_chart_type type;
    struct nk_color color;
    struct nk_color highlight;
    float min, max, range;
    int count;
    struct nk_vec2 last;
    int index;
};

struct nk_chart {
    int slot;
    float x, y, w, h;
    struct nk_chart_slot slots[NK_CHART_MAX_SLOT];
};

enum nk_panel_row_layout_type {
    NK_LAYOUT_DYNAMIC_FIXED = 0,
    NK_LAYOUT_DYNAMIC_ROW,
    NK_LAYOUT_DYNAMIC_FREE,
    NK_LAYOUT_DYNAMIC,
    NK_LAYOUT_STATIC_FIXED,
    NK_LAYOUT_STATIC_ROW,
    NK_LAYOUT_STATIC_FREE,
    NK_LAYOUT_STATIC,
    NK_LAYOUT_TEMPLATE,
    NK_LAYOUT_COUNT
};
struct nk_row_layout {
    enum nk_panel_row_layout_type type;
    int index;
    float height;
    float min_height;
    int columns;
    const float *ratio;
    float item_width;
    float item_height;
    float item_offset;
    float filled;
    struct nk_rect item;
    int tree_depth;
    float templates[NK_MAX_LAYOUT_ROW_TEMPLATE_COLUMNS];
};

struct nk_popup_buffer {
    nk_size begin;
    nk_size parent;
    nk_size last;
    nk_size end;
    int active;
};

struct nk_menu_state {
    float x, y, w, h;
    struct nk_scroll offset;
};

struct nk_panel {
    enum nk_panel_type type;
    nk_flags flags;
    struct nk_rect bounds;
    nk_uint *offset_x;
    nk_uint *offset_y;
    float at_x, at_y, max_x;
    float footer_height;
    float header_height;
    float border;
    unsigned int has_scrolling;
    struct nk_rect clip;
    struct nk_menu_state menu;
    struct nk_row_layout row;
    struct nk_chart chart;
    struct nk_command_buffer *buffer;
    struct nk_panel *parent;
};

/*==============================================================
 *                          WINDOW
 * =============================================================*/
#ifndef NK_WINDOW_MAX_NAME
#define NK_WINDOW_MAX_NAME 64
#endif

struct nk_table;
enum nk_window_flags {
    NK_WINDOW_PRIVATE       = NK_FLAG(11),
    NK_WINDOW_DYNAMIC       = NK_WINDOW_PRIVATE,
    /* special window type growing up in height while being filled to a certain maximum height */
    NK_WINDOW_ROM           = NK_FLAG(12),
    /* sets window widgets into a read only mode and does not allow input changes */
    NK_WINDOW_NOT_INTERACTIVE = NK_WINDOW_ROM|NK_WINDOW_NO_INPUT,
    /* prevents all interaction caused by input to either window or widgets inside */
    NK_WINDOW_HIDDEN        = NK_FLAG(13),
    /* Hides window and stops any window interaction and drawing */
    NK_WINDOW_CLOSED        = NK_FLAG(14),
    /* Directly closes and frees the window at the end of the frame */
    NK_WINDOW_MINIMIZED     = NK_FLAG(15),
    /* marks the window as minimized */
    NK_WINDOW_REMOVE_ROM    = NK_FLAG(16)
    /* Removes read only mode at the end of the window */
};

struct nk_popup_state {
    struct nk_window *win;
    enum nk_panel_type type;
    struct nk_popup_buffer buf;
    nk_hash name;
    int active;
    unsigned combo_count;
    unsigned con_count, con_old;
    unsigned active_con;
    struct nk_rect header;
};

struct nk_edit_state {
    nk_hash name;
    unsigned int seq;
    unsigned int old;
    int active, prev;
    int cursor;
    int sel_start;
    int sel_end;
    struct nk_scroll scrollbar;
    unsigned char mode;
    unsigned char single_line;
};

struct nk_property_state {
    int active, prev;
    char buffer[NK_MAX_NUMBER_BUFFER];
    int length;
    int cursor;
    int select_start;
    int select_end;
    nk_hash name;
    unsigned int seq;
    unsigned int old;
    int state;
};

struct nk_window {
    unsigned int seq;
    nk_hash name;
    char name_string[NK_WINDOW_MAX_NAME];
    nk_flags flags;

    struct nk_rect bounds;
    struct nk_scroll scrollbar;
    struct nk_command_buffer buffer;
    struct nk_panel *layout;
    float scrollbar_hiding_timer;

    /* persistent widget state */
    struct nk_property_state property;
    struct nk_popup_state popup;
    struct nk_edit_state edit;
    unsigned int scrolled;

    struct nk_table *tables;
    unsigned int table_count;

    /* window list hooks */
    struct nk_window *next;
    struct nk_window *prev;
    struct nk_window *parent;
};

/*==============================================================
 *                          STACK
 * =============================================================*/
/* The style modifier stack can be used to temporarily change a
 * property inside `nk_style`. For example if you want a special
 * red button you can temporarily push the old button color onto a stack
 * draw the button with a red color and then you just pop the old color
 * back from the stack:
 *
 *      nk_style_push_style_item(ctx, &ctx->style.button.normal, nk_style_item_color(nk_rgb(255,0,0)));
 *      nk_style_push_style_item(ctx, &ctx->style.button.hover, nk_style_item_color(nk_rgb(255,0,0)));
 *      nk_style_push_style_item(ctx, &ctx->style.button.active, nk_style_item_color(nk_rgb(255,0,0)));
 *      nk_style_push_vec2(ctx, &cx->style.button.padding, nk_vec2(2,2));
 *
 *      nk_button(...);
 *
 *      nk_style_pop_style_item(ctx);
 *      nk_style_pop_style_item(ctx);
 *      nk_style_pop_style_item(ctx);
 *      nk_style_pop_vec2(ctx);
 *
 * Nuklear has a stack for style_items, float properties, vector properties,
 * flags, colors, fonts and for button_behavior. Each has it's own fixed size stack
 * which can be changed at compile time.
 */
#ifndef NK_BUTTON_BEHAVIOR_STACK_SIZE
#define NK_BUTTON_BEHAVIOR_STACK_SIZE 8
#endif

#ifndef NK_FONT_STACK_SIZE
#define NK_FONT_STACK_SIZE 8
#endif

#ifndef NK_STYLE_ITEM_STACK_SIZE
#define NK_STYLE_ITEM_STACK_SIZE 16
#endif

#ifndef NK_FLOAT_STACK_SIZE
#define NK_FLOAT_STACK_SIZE 32
#endif

#ifndef NK_VECTOR_STACK_SIZE
#define NK_VECTOR_STACK_SIZE 16
#endif

#ifndef NK_FLAGS_STACK_SIZE
#define NK_FLAGS_STACK_SIZE 32
#endif

#ifndef NK_COLOR_STACK_SIZE
#define NK_COLOR_STACK_SIZE 32
#endif

#define NK_CONFIGURATION_STACK_TYPE(prefix, name, type)\
    struct nk_config_stack_##name##_element {\
        prefix##_##type *address;\
        prefix##_##type old_value;\
    }
#define NK_CONFIG_STACK(type,size)\
    struct nk_config_stack_##type {\
        int head;\
        struct nk_config_stack_##type##_element elements[size];\
    }

#define nk_float float
NK_CONFIGURATION_STACK_TYPE(struct nk, style_item, style_item);
NK_CONFIGURATION_STACK_TYPE(nk ,float, float);
NK_CONFIGURATION_STACK_TYPE(struct nk, vec2, vec2);
NK_CONFIGURATION_STACK_TYPE(nk ,flags, flags);
NK_CONFIGURATION_STACK_TYPE(struct nk, color, color);
NK_CONFIGURATION_STACK_TYPE(const struct nk, user_font, user_font*);
NK_CONFIGURATION_STACK_TYPE(enum nk, button_behavior, button_behavior);

NK_CONFIG_STACK(style_item, NK_STYLE_ITEM_STACK_SIZE);
NK_CONFIG_STACK(float, NK_FLOAT_STACK_SIZE);
NK_CONFIG_STACK(vec2, NK_VECTOR_STACK_SIZE);
NK_CONFIG_STACK(flags, NK_FLAGS_STACK_SIZE);
NK_CONFIG_STACK(color, NK_COLOR_STACK_SIZE);
NK_CONFIG_STACK(user_font, NK_FONT_STACK_SIZE);
NK_CONFIG_STACK(button_behavior, NK_BUTTON_BEHAVIOR_STACK_SIZE);

struct nk_configuration_stacks {
    struct nk_config_stack_style_item style_items;
    struct nk_config_stack_float floats;
    struct nk_config_stack_vec2 vectors;
    struct nk_config_stack_flags flags;
    struct nk_config_stack_color colors;
    struct nk_config_stack_user_font fonts;
    struct nk_config_stack_button_behavior button_behaviors;
};

/*==============================================================
 *                          CONTEXT
 * =============================================================*/
#define NK_VALUE_PAGE_CAPACITY \
    (((NK_MAX(sizeof(struct nk_window),sizeof(struct nk_panel)) / sizeof(nk_uint))) / 2)

struct nk_table {
    unsigned int seq;
    unsigned int size;
    nk_hash keys[NK_VALUE_PAGE_CAPACITY];
    nk_uint values[NK_VALUE_PAGE_CAPACITY];
    struct nk_table *next, *prev;
};

union nk_page_data {
    struct nk_table tbl;
    struct nk_panel pan;
    struct nk_window win;
};

struct nk_page_element {
    union nk_page_data data;
    struct nk_page_element *next;
    struct nk_page_element *prev;
};

struct nk_page {
    unsigned int size;
    struct nk_page *next;
    struct nk_page_element win[1];
};

struct nk_pool {
    struct nk_allocator alloc;
    enum nk_allocation_type type;
    unsigned int page_count;
    struct nk_page *pages;
    struct nk_page_element *freelist;
    unsigned capacity;
    nk_size size;
    nk_size cap;
};

struct nk_context {
/* public: can be accessed freely */
    struct nk_input input;
    struct nk_style style;
    struct nk_buffer memory;
    struct nk_clipboard clip;
    nk_flags last_widget_state;
    enum nk_button_behavior button_behavior;
    struct nk_configuration_stacks stacks;
    float delta_time_seconds;

/* private:
    should only be accessed if you
    know what you are doing */
#ifdef NK_INCLUDE_VERTEX_BUFFER_OUTPUT
    struct nk_draw_list draw_list;
#endif
#ifdef NK_INCLUDE_COMMAND_USERDATA
    nk_handle userdata;
#endif
    /* text editor objects are quite big because of an internal
     * undo/redo stack. Therefore it does not make sense to have one for
     * each window for temporary use cases, so I only provide *one* instance
     * for all windows. This works because the content is cleared anyway */
    struct nk_text_edit text_edit;
    /* draw buffer used for overlay drawing operation like cursor */
    struct nk_command_buffer overlay;

    /* windows */
    int build;
    int use_pool;
    struct nk_pool pool;
    struct nk_window *begin;
    struct nk_window *end;
    struct nk_window *active;
    struct nk_window *current;
    struct nk_page_element *freelist;
    unsigned int count;
    unsigned int seq;
};

/* ==============================================================
 *                          MATH
 * =============================================================== */
#define NK_PI 3.141592654f
#define NK_UTF_INVALID 0xFFFD
#define NK_MAX_FLOAT_PRECISION 2

#define NK_UNUSED(x) ((void)(x))
#define NK_SATURATE(x) (NK_MAX(0, NK_MIN(1.0f, x)))
#define NK_LEN(a) (sizeof(a)/sizeof(a)[0])
#define NK_ABS(a) (((a) < 0) ? -(a) : (a))
#define NK_BETWEEN(x, a, b) ((a) <= (x) && (x) < (b))
#define NK_INBOX(px, py, x, y, w, h)\
    (NK_BETWEEN(px,x,x+w) && NK_BETWEEN(py,y,y+h))
#define NK_INTERSECT(x0, y0, w0, h0, x1, y1, w1, h1) \
    (!(((x1 > (x0 + w0)) || ((x1 + w1) < x0) || (y1 > (y0 + h0)) || (y1 + h1) < y0)))
#define NK_CONTAINS(x, y, w, h, bx, by, bw, bh)\
    (NK_INBOX(x,y, bx, by, bw, bh) && NK_INBOX(x+w,y+h, bx, by, bw, bh))

#define nk_vec2_sub(a, b) nk_vec2((a).x - (b).x, (a).y - (b).y)
#define nk_vec2_add(a, b) nk_vec2((a).x + (b).x, (a).y + (b).y)
#define nk_vec2_len_sqr(a) ((a).x*(a).x+(a).y*(a).y)
#define nk_vec2_muls(a, t) nk_vec2((a).x * (t), (a).y * (t))

#define nk_ptr_add(t, p, i) ((t*)((void*)((nk_byte*)(p) + (i))))
#define nk_ptr_add_const(t, p, i) ((const t*)((const void*)((const nk_byte*)(p) + (i))))
#define nk_zero_struct(s) nk_zero(&s, sizeof(s))

/* ==============================================================
 *                          ALIGNMENT
 * =============================================================== */
/* Pointer to Integer type conversion for pointer alignment */
#if defined(__PTRDIFF_TYPE__) /* This case should work for GCC*/
# define NK_UINT_TO_PTR(x) ((void*)(__PTRDIFF_TYPE__)(x))
# define NK_PTR_TO_UINT(x) ((nk_size)(__PTRDIFF_TYPE__)(x))
#elif !defined(__GNUC__) /* works for compilers other than LLVM */
# define NK_UINT_TO_PTR(x) ((void*)&((char*)0)[x])
# define NK_PTR_TO_UINT(x) ((nk_size)(((char*)x)-(char*)0))
#elif defined(NK_USE_FIXED_TYPES) /* used if we have <stdint.h> */
# define NK_UINT_TO_PTR(x) ((void*)(uintptr_t)(x))
# define NK_PTR_TO_UINT(x) ((uintptr_t)(x))
#else /* generates warning but works */
# define NK_UINT_TO_PTR(x) ((void*)(x))
# define NK_PTR_TO_UINT(x) ((nk_size)(x))
#endif

#define NK_ALIGN_PTR(x, mask)\
    (NK_UINT_TO_PTR((NK_PTR_TO_UINT((nk_byte*)(x) + (mask-1)) & ~(mask-1))))
#define NK_ALIGN_PTR_BACK(x, mask)\
    (NK_UINT_TO_PTR((NK_PTR_TO_UINT((nk_byte*)(x)) & ~(mask-1))))

#define NK_OFFSETOF(st,m) ((nk_ptr)&(((st*)0)->m))
#define NK_CONTAINER_OF(ptr,type,member)\
    (type*)((void*)((char*)(1 ? (ptr): &((type*)0)->member) - NK_OFFSETOF(type, member)))

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
template<typename T> struct nk_alignof;
template<typename T, int size_diff> struct nk_helper{enum {value = size_diff};};
template<typename T> struct nk_helper<T,0>{enum {value = nk_alignof<T>::value};};
template<typename T> struct nk_alignof{struct Big {T x; char c;}; enum {
    diff = sizeof(Big) - sizeof(T), value = nk_helper<Big, diff>::value};};
#define NK_ALIGNOF(t) (nk_alignof<t>::value)
#elif defined(_MSC_VER)
#define NK_ALIGNOF(t) (__alignof(t))
#else
#define NK_ALIGNOF(t) ((char*)(&((struct {char c; t _h;}*)0)->_h) - (char*)0)
#endif

#endif /* NK_NUKLEAR_H_ */
