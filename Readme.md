# GUI
This is a bloat-free stateless immediate mode graphical user interface toolkit
written in ANSI C. It was designed to be easily embeddable into graphical
application and does not have any direct dependencies. The main premise of this
toolkit is to be as stateless, simple but as powerful as possible with fast
streamlined user development speed in mind.

## Features
- Immediate mode graphical user interface toolkit
- Written in C89 (ANSI C)
- Small codebase (~2.5kLOC)
- Focus on portability and minimal internal state
- Suited for embedding into graphical applications
- No global hidden state
- No direct dependencies (not even libc!)
- Full memory management control
- Renderer and platform independent
- Configurable
- UTF-8 supported

## Functionality
+ Label
+ Buttons
+ Slider
+ Progressbar
+ Checkbox
+ Radiobutton
+ Input
+ Spinner
+ Selector
+ Linegraph
+ Histogram
+ Panel
+ Layouts

## Limitations
- Does NOT provide os window management
- Does NOT provide input handling
- Does NOT provide a renderer backend
- Does NOT implement a font library  
Summary: It is only responsible for the actual user interface

## IMGUIs
Immediate mode in contrast to classical retained mode GUIs store as little state as possible
by using procedural function calls as "widgets" instead of storing objects.
Each "widget" function call takes hereby all its neccessary data and immediatly returns
the through the user modified state back to the caller. Immediate mode graphical
user interfaces therefore combine drawing and input handling into one unit
instead of seperating them like retain mode GUIs.

Since there is no to minimal internal state in immediate mode user interfaces,
updates have to occur every frame which on one hand is more drawing expensive than classic
ratained GUI implementations but on the other hand grants a lot more flexibility and
support for overall layout changes. In addition without any state there is no need to
transfer state between your program, the gui and the user which greatly
simplifies code. Further traits of immediate mode graphic user interfaces are a
code driven style, centralized flow control, easy extensibility and
understandablity.

## Example
```c
gui_float value = 5.0f;
struct gui_input in = {0};
struct gui_config config;
struct gui_font font = {...};
struct gui_memory memory = {...};
struct gui_memory_status status;
struct gui_command_buffer buffer;
struct gui_command_list out;
struct gui_command_list list;
struct gui_panel panel;

gui_default_config(&config);
gui_panel_init(&panel, 50, 50, 100, 150,
    GUI_PANEL_BORDER|GUI_PANEL_MOVEABLE|
    GUI_PANEL_CLOSEABLE|GUI_PANEL_SCALEABLE|
    GUI_PANEL_MINIMIZABLE, &config, &font);
gui_output_init_fixed(buffer, &memory);

while (1) {
    gui_input_begin(&input);
    /* record input */
    gui_input_end(&input);

    struct gui_canvas canvas;
    struct gui_panel_layout layout;
    gui_output_begin(&canvas, &buffer, window_width, window_height);
    gui_panel_begin(&layout, &panel, "Demo", &canvas, &input);
    gui_panel_row(&layout, 30, 1);
    gui_panel_text(&layout, "label", 5, GUI_TEXT_CENTERED);
    if (gui_panel_button_text(&layout, "button", GUI_BUTTON_DEFAULT)) {
        /* event handling */
    }
    gui_panel_row(&layout, 30, 2);
    gui_panel_text(&layout, "value:", 6, GUI_TEXT_CENTERED);
    value = gui_panel_slider(&layout, 0, value, 10, 1);
    gui_panel_end(&layout, &panel);
    gui_output_end(&list, buffer, &status);

    struct gui_command *cmd = list.begin;
    while (cmd != list.end) {
        /* execute command */
        cmd = cmd->next;
    }
}
```
![gui screenshot](/screen.png?raw=true)

## API
The API for this gui toolkit is divided into two different layers. There
is the widget layer and the panel layer. The widget layer provides a number of
classical widgets in functional immediate mode form without any kind of internal
state. Each widget can be placed anywhere on the screen but there is no directy
way provided to group widgets together. For this to change there is the panel
layer which is build on top of the widget layer and uses most of the widget API
internally to form groups of widgets into a layout.

### Input
The input structure holds the user input over the course of the frame and
manages the complete modification of widget and panel state. Like the panel and
buffering the input is an immediate mode API and consist of an begin sequence
point with `gui_input_begin` and a end sequence point with `gui_input_end`.
All modifications to the input struct can only occur between both of these
sequence points while all outside modifcation provoke undefined behavior.

```c
struct gui_input input = {0};
while (1) {
    gui_input_begin(&input);
    /* record input */
    gui_input_end(&input);
}
```

### Canvas
The Canvas is the abstract drawing interface between the GUI toolkit
and the user and contains drawing callbacks for the primitives
scissor, line, rectangle, circle, triangle, bitmap and text which need to be
provided by the user. In addition to the drawing callbacks the canvas contains
font data and the width and height of the canvas drawing area.
Therefore the canvas is the heart of the toolkit and is probably the biggest
chunk of work to be done by the user.

### Font
Since there is no direct font implementation in the toolkit but font handling is
still an aspect of a gui implemenatation the gui struct was introduced. It only
contains the bare minimum of what is needed for font handling with a handle to
your font structure, the font height and a callback to calculate the width of a
given string.

### Configuration
The gui toolkit provides a number of different attributes that can be
configured, like spacing, padding, size and color.
While the widget API even expects you to provide the configuration
for each and every widget the panel layer provides you with a set of
attributes in the `gui_config` structure. The structure either needs to be
filled by the user or can be setup with some default values by the function
`gui_default_config`. Modification on the fly to the `gui_config` struct is in
true immedate mode fashion possible and supported.

### Buffering
For the purpose of defered drawing or the implementation of overlapping panels
the command buffering API was added. The command buffer hereby holds a queue of
drawing commands for a number of primitives like line, rectangle, circle,
triangle and text. The memory for the command buffer can be provided by the user
in three different ways. First a fixed memory block can be given over to the
command buffer which fills the memory up until no memory is left. The seond way
is still using a fixed size block but rellocating a new block of memory at the
end of the frame, if not enough memory was provided. The final way of memory
management is by providing allocator callbacks with alloc, realloc and free.
In true immediate mode fashion the buffering API is by base around a sequence
point API with an begin sequence point `gui_output_begin` and a end sequence
point with `gui_output_end` and modification of state between both points. Just
like the input API the buffer modification before the begining or after the end
sequence point is undefined behavior.

```c
struct gui_memory memory = {...};
struct gui_memory_status status = {0};
struct gui_command_list out = {0};
struct gui_command_buffer buffer = {0};
struct gui_canvas canvas = {0};
gui_output_init_fixed(buffer, &memory);

while (1) {
    gui_output_begin(&canvas, &buffer, window_width, window_height);
    /* add commands by using the canvas */
    gui_output_end(&list, buffer, &status);
}

```

### Widgets
The minimal widget API provides a basic number of widgets and is designed for
uses cases where only a small number of basic widgets are needed without any kind of
more complex layouts. In order for the GUI to work each widget needs a canvas to
draw to, positional and widgets specific data as well as user input
and returns the from the user input modified state of the widget.

```c
struct gui_input input = {0};
struct gui_font font = {...};
struct gui_canvas canvas = {...};
struct gui_button style = {...};

while (1) {
    if(gui_button_text(&canvas, 0, 0, 100, 30, "ok", GUI_BUTTON_DEFAULT, &style, &input, &font))
        fprintf(stdout, "button pressed!\n");
}
```

### Panels
To further extend the basic widget layer and remove some of the boilerplate
code the panel was introduced. The panel groups together a number of
widgets but in true immediate mode fashion does not save any widget state from
widgets that have been added to the panel. In addition the panel enables a
number of nice features for a group of widgets like panel movement, scaling,
closing and minimizing. An additional use for panels is to further group widgets
in panels to tabs, groups and shelfs.
The panel is divided into a persistent state struct with `struct gui_panel` with a number
of attributes which have a persistent life time outside the frame and the panel layout
`struct gui_panel_layout` with a transient frame life time. While the layout
state is constantly modifed over the course of the frame the panel struct is
only modifed at the immendiate mode sequence points `gui_panel_begin` and
`gui_panel_end`. Therefore all changes to the panel struct inside of both
sequence points has no effect on the current frame and is only visible in the
next frame.

```c
struct gui_panel panel;
struct gui_config config;
struct gui_font font = {...}
struct gui_input input = {0};
struct gui_canvas canvas = {...};
gui_default_config(&config);
gui_panel_init(&panel, 50, 50, 300, 200, 0, &config, &font);

while (1) {
    struct gui_panel_layout layout;
    gui_panel_begin(&layout, &panel, "Demo", &canvas, &in);
    gui_panel_row(&layout, 30, 1);
    if (gui_panel_button_text(&layout, "button", GUI_BUTTON_DEFAULT))
        fprintf(stdout, "button pressed!\n");
    value = gui_panel_slider(&layout, 0, value, 10, 1);
    progress = gui_panel_progress(&layout, progress, 100, gui_true);
    gui_panel_end(&layout, &panel);
}
```

## FAQ
#### Where is the demo/example code?
The demo and example code can be found in the demo folder. For now there is
only example code for Linux with X11 and Xlib but a Win32, OpenGL and Directx
demo is in the working.

#### Why did you use ANSI C and not C99 or C++?
Personally I stay out of all "discussions" about C vs C++ since they are totally
worthless and never brought anything good with it. The simple answer is I
personally love C and have nothing against people using C++ exspecially the new
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
plaform. But if your development environment provides the header file you can define
`GUI_USE_FIXED_SIZE_TYPES` to directly use the correct types.

#### Why is font/input/window management not provided?
As for window and input management it is a ton of work to abstract over
all possible platforms and there are already libraries like SDL or SFML or even
the platform itself which provide you with the functionality.
So instead of reinventing the wheel and trying to do everything the project tries
to be as indepenedent and out of the users way as possible.
This means in practice a litte bit more work on the users behalf but grants a
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
- [Casey Muratori's imgui panel design(1/2)](http://mollyrocket.com/casey/stream_0019.html)
- [Casey Muratori's imgui panel design(2/2)](http://mollyrocket.com/casey/stream_0020.html)
- [Casey Muratori: Designing and Evaluation Reusable Components](http://mollyrocket.com/casey/stream_0028.html)
- [ImGui: The inspiration for this project](https://github.com/ocornut/imgui)
- [Nvidia's imgui toolkit](https://code.google.com/p/nvidia-widgets/)

# License
    (The MIT License)
