# GUI
[![Coverity Status](https://scan.coverity.com/projects/5863/badge.svg)](https://scan.coverity.com/projects/5863)

This is a bloat free minimal state immediate mode graphical user interface toolkit
written in ANSI C. It was designed as a embeddable user interface for graphical
application and does not have any direct dependencies.

## Features
- Immediate mode graphical user interface toolkit
- Written in C89 (ANSI C)
- Small codebase (~6kLOC)
- Focus on portability, efficiency, simplicity and minimal internal state
- Suited for embedding into graphical applications
- No global or hidden state
- No direct dependencies (not even libc!)
- Full memory management control
- Renderer and platform independent
- Configurable style and colors
- UTF-8 support

## Limitations
- Does NOT provide os window/input management
- Does NOT provide a renderer backend
- Does NOT implement a font library  
Summary: It is only responsible for the actual user interface

## Target applications
- Graphical tools/editors
- Library testbeds
- Game engine debugging UI
- Graphical overlay

## Gallery
![gui demo](/screen/demo.png?raw=true)
![gui explorer](/screen/explorer.png?raw=true)
![gui screenshot](/screen/screenshot.png?raw=true)

## Example
```c
/* allocate memory to hold the draw commands */
struct gui_command_queue queue;
void *memory = malloc(MEMORY_SIZE);
gui_command_queue_init_fixed(&buffer, memory, MEMORY_SIZE);

/* setup configuration */
struct gui_font font;
struct gui_config config;
font.userdata.ptr = your_font_data;
font.height = your_font_data.height;
font.width = your_font_string_width_callback_function;
gui_config_default(&config, GUI_DEFAULT_ALL, &font);

/* initialize panel */
struct gui_panel panel;
gui_panel_init(&panel, 50, 50, 220, 180,
    GUI_PANEL_BORDER|GUI_PANEL_MOVEABLE|GUI_PANEL_SCALEABLE,
    &queue, &config, &input);

/* setup widget data */
enum {EASY, HARD};
gui_size option = EASY;
gui_size item = 0;

struct gui_input input = {0};
while (1) {
    gui_input_begin(&input);
    /* record input */
    gui_input_end(&input);

    /* GUI */
    struct gui_panel_layout layout;
    gui_panel_begin(&layout, &panel);
    {
        const char *items[] = {"Fist", "Pistol", "Railgun", "BFG"};
        gui_panel_header(&layout, "Demo", GUI_CLOSEABLE, 0, GUI_HEADER_LEFT);
        gui_panel_row_static(&layout, 30, 80, 1);
        if (gui_panel_button_text(&layout, "button", GUI_BUTTON_DEFAULT)) {
            /* event handling */
        }
        gui_panel_row_dynamic(&layout, 30, 2);
        if (gui_panel_option(&layout, "easy", option == EASY)) option = EASY;
        if (gui_panel_option(&layout, "hard", option == HARD)) option = HARD;
        gui_panel_label(&layout, "Weapon:", GUI_TEXT_LEFT);
        item = gui_panel_selector(&layout, items, LEN(items), item);
    }
    gui_panel_end(&layout, &panel);

    /* draw */
    const struct gui_command *cmd;
    gui_foreach_command(cmd, &queue) {
        /* execute draw call command */
        switch (cmd->type) {
        case GUI_COMMAND_SCISSOR:
            /*...*/
        case GUI_COMMAND_LINE:
            /*...*/
        case GUI_COMMAND_RECT:
            /*...*/
        case GUI_COMMAND_CIRCLE:
            /*...*/
        case GUI_COMMAND_TRIANGLE:
            /*...*/
        case GUI_COMMAND_TEXT:
            /*...*/
        case GUI_COMMAND_IMAGE:
            /*...*/
        }
    }
    gui_command_queue_clear(&queue);
}
```
![gui screenshot](/screen/screen.png?raw=true)

## IMGUIs
Immediate mode in contrast to classical retained mode GUIs store as little state as possible
by using procedural function calls as "widgets" instead of storing objects.
Each "widget" function call takes hereby all its necessary data and immediately returns
the through the user modified state back to the caller. Immediate mode graphical
user interfaces therefore combine drawing and input handling into one unit
instead of separating them like retain mode GUIs.

Since there is no to minimal internal state in immediate mode user interfaces,
updates have to occur every frame which on one hand is more drawing expensive than classic
retained GUI implementations but on the other hand grants a lot more flexibility and
support for overall layout changes. In addition without any state there is no
duplicated state between your program, the gui and the user which greatly
simplifies code. Further traits of immediate mode graphic user interfaces are a
code driven style, centralized flow control, easy extensibility and
understandability.

### Input
The `gui_input` struct holds the user input over the course of the frame and
manages the complete modification of widget and panel state. To fill the
structure with data over the frame there are a number of functions provided for
key, motion, button and text input. The input is hereby completly independent of
the underlying platform or way of input so even touch or other ways of input are
possible.
Like the panel and the buffer, input is based on an immediate mode API and
consist of an begin sequence with `gui_input_begin` and a end sequence point
with `gui_input_end`. All modifications can only occur between both of these
sequence points while all outside modification provoke undefined behavior.

```c
struct gui_input input = {0};
while (1) {
    gui_input_begin(&input);
    if (/*mouse moved*/)
        gui_input_motion(&input, mouse.x, mouse.y);
    if (/*key pressed*/)
        gui_input_key(&input, key, gui_true);
    if (/*key released*/)
        gui_input_key(&input, key, gui_false);
    if (/*mouse button pressed*/)
        gui_input_button(&input, mouse.x, mouse.y, gui_true);
    if (/*mouse button released */)
        gui_input_button(&input, mouse.x, mouse.y, gui_false);
    gui_input_end(&input);
}
```

### Configuration
The gui toolkit provides a number of different attributes that can be
configured, like spacing, padding, size and color.
While the widget API even expects you to provide the configuration
for each and every widget the panel layer provides you with a set of
attributes in the `gui_config` structure. The structure either needs to be
filled by the user or can be setup with some default values by the function
`gui_config_default`. Modification on the fly to the `gui_config` struct is in
true immediate mode fashion possible and supported.

```c
struct gui_config {
    gui_float rounding[GUI_ROUNDING_MAX];
    struct gui_vec2 properties[GUI_PROPERTY_MAX];
    struct gui_color colors[GUI_COLOR_COUNT];
};
```
In addition to modifing the `gui_config` struct directly the configration API
enables you to temporarily change a property or color and revert back directly
after the change is no longer needed. The number of temporary changes are
limited but can be changed with constants `GUI_MAX_COLOR_STACK` and
`GUI_MAX_ATTRIB_STACK`.


```c
gui_config_push_color(config, GUI_COLORS_PANEL, 255, 0, 0, 255);
gui_config_push_attribute(config, GUI_ATTRIBUTE_PADDING, 10.0f, 5.0f);
/* use the configuration data */
gui_config_pop_attribute(config);
gui_config_pop_color(config);
```

Since there is no direct font implementation in the toolkit but font handling is
still an aspect of a gui implementation, the `gui_font` struct was introduced. It only
contains the bare minimum of what is needed for font handling.
For widgets the `gui_font` data has to be persistent while the
panel hold the font internally. Important to node is that the font does not hold
your font data but merely references it so you have to make sure that the font
always points to a valid object.

```c
struct gui_font {
    gui_handle userdata;
    gui_float height;
    gui_text_width_f width;
};

font.userdata.ptr = your_font_data;
font.height = your_font_data.height;
font.width = your_font_string_width_callback_function;
```

### Memory
Almost all memory as well as object management for the toolkit
is left to the user for maximum control. In fact a big subset of the toolkit can
be used without any heap allocation at all. The only place where heap allocation
is needed at all is for buffering draw calls. While the standart way of
memory allocation in that case for libraries is to just provide allocator callbacks
which is implemented aswell with the `gui_allocator`
structure, there are two addition ways to provided memory. The
first one is to just providing a static fixed size memory block to fill up which
is handy for UIs with roughly known memory requirements. The other way of memory
managment is to extend the fixed size block with the abiltiy to resize your block
at the end of the frame if there is not enough memory.
For the purpose of resizable fixed size memory blocks and for general
information about memory consumption the `gui_memory_info` structure was
added. It contains information about the allocated amount of data in the current
frame as well as the needed amount if not enough memory was provided.

```c
/* fixed size queue */
void *memory = malloc(size);
gui_command_queue queue;
gui_command_queue_init_fixed(&queue, memory, MEMORY_SIZE, GUI_CLIP);
```

```c
/* dynamically growing queue */
struct gui_allocator alloc;
alloc.userdata = your_allocator;
alloc.alloc = your_allocation_callback;
alloc.relloac = your_reallocation_callback;
alloc.free = your_free_callback;

struct gui_command_queue queue;
const gui_size initial_size = 4*1024;
const gui_float grow_factor = 2.0f;
gui_command_queue_init(&queue, &alloc, initial_size, grow_factor);
```

### Tiling
A tiled layout allows to divide the screen into regions called
slots in this case the top, left, center, right and bottom slot. Each slot occupies a
certain percentage on the screen and can be filled with panels either
horizontally or vertically. The combination of slots, ratio and multiple panels
per slots support a rich set of vertical, horizontal and mixed layouts.

```c
struct gui_command_queue queue;
void *memory = malloc(MEMORY_SIZE);
gui_command_queue_init_fixed(&queue, memory, MEMORY_SIZE, GUI_CLIP);

struct gui_config config;
struct gui_font font = {...}
gui_config_default(&config, GUI_DEFAULT_ALL, &font);

/* setup tiled window layout */
struct gui_layout tiled;
gui_layout_begin(&tiled, gui_rect(0, 0, window_width, window_height), GUI_LAYOUT_ACTIVE);
gui_layout_slot(&tiled, GUI_SLOT_LEFT, 0.5f, GUI_LAYOUT_VERTICAL, 1);
gui_layout_slot(&tiled, GUI_SLOT_CENTER, 0.5f, GUI_LAYOUT_VERTICAL, 1);
gui_layout_end(&tiled);

/* setup panels */
struct gui_input input = {0};
struct gui_panel left;
struct gui_panel center;
gui_panel_init(&left, 0, 0, 0, 0, 0, &config, &queue, &input);
gui_panel_init(&center, 0, 0, 0, 0, 0, &config, &queue, &input);

while (1) {
    gui_input_begin(&input);
    /* record input */
    gui_input_end(&input);

    /* GUI */
    struct gui_panel_layout layout;
    gui_panel_begin_tiled(&layout, &left, &tiled, GUI_SLOT_LEFT, 0);
    gui_panel_row_dynamic(&layout, 30, 1);
    if (gui_panel_button_text(&layout, "button0", GUI_BUTTON_DEFAULT))
        fprintf(stdout, "button pressed!\n");
    gui_panel_end(&layout, &left);

    gui_panel_begin_tiled(&layout, &center, &tiled, GUI_SLOT_CENTER, 0);
    gui_panel_row_dynamic(&layout, 30, 1);
    if (gui_panel_button_text(&layout, "button1", GUI_BUTTON_DEFAULT))
        fprintf(stdout, "button pressed!\n");
    gui_panel_end(&layout, &center);

    /* draw each panel */
    const struct gui_command *cmd
    gui_foreach_command(cmd, queue) {
        /* execute draw call command */
    }
}
```

## FAQ
#### Where is the demo/example code?
The demo and example code can be found in the demo folder.
There is demo code for Linux(X11), Windows(win32) and OpenGL(SDL2, freetype).
As for now there will be no DirectX demo since I don't have experience
programming using DirectX but you are more than welcome to provide one.

#### Why did you use ANSI C and not C99 or C++?
Personally I stay out of all "discussions" about C vs C++ since they are totally
worthless and never brought anything good with it. The simple answer is I
personally love C and have nothing against people using C++ especially the new
iterations with C++11 and C++14.
While this hopefully settles my view on C vs C++ there is still ANSI C vs C99.
While for personal projects I only use C99 with all its niceties, libraries are
a little bit different. Libraries are designed to reach the highest number of
users possible which brings me to ANSI C as the most portable version.
In addition not all C compiler like the MSVC
compiler fully support C99, which finalized my decision to use ANSI C.

#### Why do you typedef your own types instead of using the standard types?
This Project uses ANSI C which does not have the header file `<stdint.h>`
and therefore does not provide the fixed sized types that I need. Therefore
I defined my own types which need to be set to the correct size for each
platform. But if your development environment provides the header file you can define
`GUI_USE_FIXED_SIZE_TYPES` to directly use the correct types.

#### Why is font/input/window management not provided?
As for window and input management it is a ton of work to abstract over
all possible platforms and there are already libraries like SDL or SFML or even
the platform itself which provide you with the functionality.
So instead of reinventing the wheel and trying to do everything the project tries
to be as independent and out of the users way as possible.
This means in practice a little bit more work on the users behalf but grants a
lot more freedom especially because the toolkit is designed to be embeddable.

The font management on the other hand is litte bit more tricky. In the beginning
the toolkit had some basic font handling but I removed it later. This is mainly
a question of if font handling should be part of a gui toolkit or not. As for a
framework the question would definitely be yes but for a toolkit library the
question is not as easy. In the end the project does not have font handling
since there are already a number of font handling libraries in existence or even the
platform (Xlib, Win32) itself already provides a solution.

## References
- [Tutorial from Jari Komppa about imgui libraries](http://www.johno.se/book/imgui.html)
- [Johannes 'johno' Norneby's article](http://iki.fi/sol/imgui/)
- [Casey Muratori's original introduction to imgui's](http:://mollyrocket.com/861?node=861)
- [Casey Muratori's imgui panel design (1/2)](http://mollyrocket.com/casey/stream_0019.html)
- [Casey Muratori's imgui panel design (2/2)](http://mollyrocket.com/casey/stream_0020.html)
- [Casey Muratori: Designing and Evaluation Reusable Components](http://mollyrocket.com/casey/stream_0028.html)
- [ImGui: The inspiration for this project](https://github.com/ocornut/imgui)
- [Nvidia's imgui toolkit](https://code.google.com/p/nvidia-widgets/)

# License
    (The MIT License)
