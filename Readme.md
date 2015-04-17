# GUI

WORK IN PROGRESS: I do not garantee that everything works right now

## Features
- Immediate mode graphical user interface toolkit
- Written in C89 (ANSI C)
- Small (~2.5kLOC)
- Focus on portability and minimal internal state
- Suited for embedding into graphical applications
- No global hidden state
- No direct dependencies (not even libc!)
- Renderer and platform independent
- Configurable
- UTF-8 supported

## Limitations
- Does NOT provide platform independent window management
- Does NOT provide platform independent input handling
- Does NOT provide a renderer backend
- Does NOT implement a font library
Summary: It is only responsible for the actual user interface

## Layer
The gui toolkit consists of three levels of abstraction. First the basic widget layer
for a as pure functional as possible set of widgets functions without
any kind of internal state, with the tradeoff off of a lot of boilerplate code.
Second the panel layer for a static grouping of widgets into a panel with a reduced need for
a lot of the boilerplate code but takes away some freedom of widget placing and
introduces the state of the panel.
Finally there is the context layer which represent the complete window and
enables moveable, scaleable and overlapping panels, but needs complete control
over the panel management and therefore needs the most amount of internal state.
Each higher level of abstraction uses the lower level(s) internally to build
on but offers a little bit different API.

#### Widgets
The widget layer provides the most basic way of creating graphical user interface
elements. It consist only of functions and only operates on the given data. Each
widgets takes in the current input state, font and the basic element configuration
and returns an updated draw buffer and the state of the element.
With each widget the buffer gets filled with a number of primitives that need be
drawn to screen. The main reason for the command buffer to queue up the draw
calls instead of just using a callback to directly draw the primitive lies in
the context which needs control over the drawing order. For a more limited scope
of functionality it would have been better to just use draw callbacks and to not
rely on memory allocation at all. The API will change if I find a way to
combine the buffer needed by the context with the drawing callbacks.

```c
struct gui_input input = {0};
struct gui_command_buffer buffer;
struct gui_command_list list;
struct gui_memory_status status;
const struct gui_font font = {...};
const struct gui_buffer button = {...};
struct gui_memory memory = {...};

while (1) {
    gui_input_begin(&input);
    /* record input */
    gui_input_end(&input);

    gui_output_begin(&buffer, &memory);
    if (gui_widget_button(&buffer, &button, "button", &font, &input))
        fprintf(stdout, "button pressed!\n");
    gui_output_end(&buffer, &status, &list);
    /* execute command list */
}
```

#### Panels
Panels provide an easy way to group together a number of widgets and reduce
some of the boilerplate code of the widget layer. Most of the boilerplate code
gets reduced by introducing a configration structure to provide a common look.
Instead of having a fixed layout and owning and holding widgets like in classic
graphical user interfaces, panels use an immediate mode approach of just setting
the layout of one row of the panel at a time and filling each row with widgets.
Therefore the only state that is being modfied over the course of setting up the
panel is an index descriping the current position of the next widget and the
current height and number of columns of the current row. In addition panels
provide a number of grouping functionality for widgets with groups, tabs and
shelfs and provide a minimizing and closing functionality.

```c
struct gui_config config;
struct gui_command_buffer buffer;
struct gui_command_list list;
struct gui_memory_status status;
const struct gui_font font = {...};
struct gui_memory memory = {...};
struct gui_panel panel = {0};
gui_default_config(&config);
gui_panel_init(&panel, &config, &font);

while (1) {
    gui_input_begin(&input);
    /* record input */
    gui_input_end(&input);

    gui_output_begin(&buffer, &memory);
    gui_panel_begin(&panel, &buffer, &input, "Demo", 50, 50, 400, 300, 0);
    gui_panel_layout(&panel, 30, 1);
    if (gui_panel_button_text(&panel, "button", GUI_BUTTON_DEFAULT))
        fprintf(stdout, "button pressed!\n");
    gui_panel_end(&panel);
    gui_output_end(&buffer, &status, &list);
    /* execute command list */
}
```

#### Context
The context extends the panel functionality with moving, scaling and overlapping
panels which are quite a bit more complicated than just minimzing and closing of
panels. For panel overlapping to work as intented the context needs complete control
over all created panels to control the drawing order. OVerall the expense to
provide overlapping panels is quite hight since draw calls, the context and
all panels need to be managed and allocated.

```c
struct gui_config config;
struct gui_output output;
struct gui_memory_status status;
const struct gui_font font = {...};
struct gui_memory memory = {...};
struct gui_panel *panel;
struct gui_context *ctx;
gui_default_config(&config);
ctx = gui_new(&memory, &input);
panel = gui_new_panel(ctx, 50, 50, 400, 300, &config, &font);

while (1) {
    gui_input_begin(&input);
    /* record input */
    gui_input_end(&input);

    gui_begin(ctx, 800, 600);
    gui_begin_panel(ctx, panel, "demo", 0);
    gui_panel_layout(&panel, 30, 1);
    if (gui_panel_button_text(&panel, "button", GUI_BUTTON_DEFAULT))
        fprintf(stdout, "button pressed!\n");
    gui_end_panel(ctx, &panel, NULL);
    gui_end(ctx, &output, NULL);
    /* execute output lists */
}
```

## Configuration
The gui toolkit provides a number of different attributes that can be
configured, like spacing, padding, size and color.
While the widget API even expects you to provide the configuration
for each and every widget the higher layers provide you with a set of
attributes in the `gui_config` structure. The structure either needs to be
filled by the user or can be setup with some default values by the function
`gui_default_config`. Modification on the fly to the `gui_config` is in
true immedate mode fashion possible and supported.

## FAQ
#### Where is Widget X?
A number of basic widgets are provided but some of the more complex widgets like
comboboxes, tables and trees are not yet implemented. Maybe if I have more
time I will look into adding them, except for comboboxes which are just
really hard to implement.

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

#### Why do you typedef our own types instead of using the standard types
This Project uses ANSI C which does not have the header file `<stdint.h>`
and therefore does not provide the fixed sized types that I need. Therefore
I defined my own types which need to be set to the correct size for each
plaform. But if your development environment provides the header file you can define
`GUI_USE_FIXED_SIZE_TYPES` to directly use the correct types.

#### Why is font/input/window management not provided
As for window and input management it is a ton of work to abstract over
all possible platforms and there are already libraries like SDL or SFML or even
the platform itself which provide you with the functionality.
So instead of reinventing the wheel and trying to do everything the project tries
to be as indepenedent and out of the users way as possible.
This means in practice a litte bit more work on the users behalf but grants a
lot more freedom especially because the toolkit is designed to be embeddable.

The font management on the other hand is a more tricky subject. In the beginning
the toolkit had some basic font handling but it got later removed. This is mainly
a question of if font handling should be part of a gui toolkit or not. As for a
framework the question would definitely be yes but for a toolkit library the
question is not as easy. In the end the project does not have font handling
since there are already a number of font handling libraries in existence or even the
platform (Xlib, Win32) itself already provides a solution.

#### Why do you use fixed size memory management
This is one of the more controversial decision in the toolkit and it comes down
to some preference that I personally build up. There are two general
ways to allocate memory, the standard way of callbacks and preallocation.
Personally I am not a big fan of callbacks even though they have their use cases
for abstraction purposes but are greatly overused in my experience.
Memory callbacks are an edge case for me and definitly shine in cases where a lot
of unpredictable allocation with varying life cycles take place. This toolkit on
the other hand has a relative stable memory allocation behavior. In the worse
case on the highst abstraction layer only the context, panels and the command
buffer need memory. In addition the general memory consumption is not that high
and could even be described as insignificant for the modern memory size. For a
system with a low amount of memory it is even better since there is only a small
limited amount of memory which is easier to optimize for as a fixed amount of memory than
a number of unrelated allocation calls.

## References
- [Tutorial from Jari Komppa about imgui libraries](http://www.johno.se/book/imgui.html)
- [Johannes 'johno' Norneby's article](http://iki.fi/sol/imgui/)
- [Casey Muratori's original introduction to imgui's](http:://mollyrocket.com/861?node=861)
- [Casey Muratori's imgui panel design 1/2](http://mollyrocket.com/casey/stream_0019.html)
- [Casey Muratori's imgui panel design 2/2](http://mollyrocket.com/casey/stream_0020.html)
- [ImGui: The inspiration for this project](https://github.com/ocornut/imgui)

# License
    (The MIT License)
