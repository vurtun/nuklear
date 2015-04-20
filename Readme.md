# GUI
This is a bloat-free stateless immediate mode graphical user interface toolkit
written in ANSI C. It was designed to be easily embeddable into graphical
application and does not have any direct dependencies. The main premise of this
toolkit is to be as stateless, simple but as powerful as possible with fast
streamlined user development speed in mind.

## Features
- Immediate mode graphical user interface toolkit
- Written in C89 (ANSI C)
- Small codebase (~2kLOC)
- Focus on portability and minimal internal state
- Suited for embedding into graphical applications
- No global hidden state
- No direct dependencies (not even libc!)
- No memory allocation needed
- Renderer and platform independent
- Configurable
- UTF-8 supported

## Functionality
+ Label
+ Buttons(Text, Triangle, Color, Toggle, Icon)
+ Slider
+ Progressbar
+ Checkbox
+ Radiobutton
+ Input field
+ Spinner
+ Selector
+ Linegraph
+ Histogram
+ Panels
+ Layouts(Tabs, Groups, Shelf)

## Limitations
- Does NOT provide window management
- Does NOT provide input handling
- Does NOT provide a renderer backend
- Does NOT implement a font library
- Does NOT provide overlapping panels  
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
support for overall changes. In addition without any state there is no need to
transfer state between your program, the gui state and the user which greatly
simplifies code. Further traits of immediate mode graphic user interfaces are a
code driven style, centralized flow control, easy extensibility and
understandablity.

## API
The API for this gui toolkit is divided into two different layers. There
is the widget layer and the panel layer. The widget layer provides a number of
classical widgets in functional immediate mode form without any kind of internal
state. Each widget can be placed anywhere on the screen but there is no directy
way provided to group widgets together. For this to change there is the panel
layer which is build on top of the widget layer and uses most of the widget API
internally to form groups of widgets into a layout.

### Widgets
The minimal widget API provides a basic number of widgets and is designed for
uses cases where only a small number of basic widgets are needed without any kind of
more complex layouts. In order for the GUI to work each widget needs a canvas to
draw to, positional and widgets specific data as well as user input
and returns the from the user input modified state of the widget.

```c
struct gui_input in = {0};
struct gui_canvas canvas = {...};
struct gui_button style = {...};

while (1) {
    gui_input_begin(&input);
    /* record input */
    gui_input_end(&input);
    if(gui_button_text(&canvas, 0, 0, 100, 30, &style, "ok", GUI_BUTTON_DEFAULT, &input))
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
in panels to tabs, groups and shelfs. The state of internal panels (tabs,
groups. shelf) is only needed over the course of the build up unlike normal
panels, which further emphasizes the minimal state mindset.

```c
struct gui_config config;
struct gui_input in = {0};
struct gui_panel panel = {0};
struct gui_canvas canvas = {...};
gui_default_config(&config);

while (1) {
    gui_input_begin(&input);
    /* record input */
    gui_input_end(&input);

    gui_panel_begin(&panel, "Demo", panel.x, panel.y, panel.width, panel.height,
        GUI_PANEL_CLOSEABLE|GUI_PANEL_MINIMIZABLE|GUI_PANEL_BORDER|
        GUI_PANEL_MOVEABLE|GUI_PANEL_SCALEABLE, &config, &canvas, &in);
    gui_panel_layout(&panel, 30, 1);
    if (gui_panel_button_text(&panel, "button", GUI_BUTTON_DEFAULT))
        fprintf(stdout, "button pressed!\n");
    value = gui_panel_slider(&panel, 0, value, 10, 1);
    progress = gui_panel_progress(&panel, progress, 100, gui_true);
    gui_panel_end(&panel);
}
```
## Canvas
The Canvas is the abstract drawing interface between the GUI toolkit
and the user and contains drawing callbacks for the primitives
scissor, line, rectangle, circle, triangle, bitmap and text which need to be
provided by the user. In addition to the drawing callbacks the canvas contains
font data and the width and height of the canvas drawing area.
Therefore the canvas is the heart of the toolkit and is probably the biggest
chunk of work to be done by the user.

## Configuration
The gui toolkit provides a number of different attributes that can be
configured, like spacing, padding, size and color.
While the widget API even expects you to provide the configuration
for each and every widget the panel layer provides you with a set of
attributes in the `gui_config` structure. The structure either needs to be
filled by the user or can be setup with some default values by the function
`gui_default_config`. Modification on the fly to the `gui_config` struct is in
true immedate mode fashion possible and supported.

## FAQ
#### Where is Widget X?
A number of basic widgets are provided but some of the more complex widgets like
comboboxes, tables and trees are not yet implemented. Maybe if I have more
time I will look into adding them. Except for comboboxes which are just
really hard to implement, but for a smaller datasets there is the selector
widget or you could combine a tab with a group and toggle buttons.

#### Where is the demo/example code
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

#### Why do you typedef your own types instead of using the standard types
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

The font management on the other hand it is litte bit more tricky. In the beginning
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
- [ImGui: The inspiration for this project](https://github.com/ocornut/imgui)
- [Nvidia's imgui toolkit](https://code.google.com/p/nvidia-widgets/)

# License
    (The MIT License)
