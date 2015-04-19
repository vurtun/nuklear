# GUI

WORK IN PROGRESS: I do not garantee that everything works right now

## Features
- Immediate mode graphical user interface toolkit
- Written in C89 (ANSI C)
- Small codebase with roughly 2kLOC
- Focus on portability and minimal internal state
- Suited for embedding into graphical applications
- No global hidden state
- No direct dependencies (not even libc!)
- No memory allocation
- Renderer and platform independent
- Configurable
- UTF-8 supported

## Limitations
- Does NOT provide window management
- Does NOT provide input handling
- Does NOT provide a renderer backend
- Does NOT implement a font library  
Summary: It is only responsible for the actual user interface

#### Widgets
```c
struct gui_input in = {0};
struct gui_canvas canvas = {...};
struct gui_button style = {...};
gui_default_config(&config);

while (1) {
    gui_input_begin(&input);
    /* record input */
    gui_input_end(&input);

    if (gui_button_text(&canvas, 50, 50, 100, 30, &style, "button", GUI_BUTTON_DEFAULT, &input))
        fprintf(stdout, "button pressed!\n");
}
```

#### Panels
```c
struct gui_input in = {0};
struct gui_config config;
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
    gui_panel_end(&panel);
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

## References
- [Tutorial from Jari Komppa about imgui libraries](http://www.johno.se/book/imgui.html)
- [Johannes 'johno' Norneby's article](http://iki.fi/sol/imgui/)
- [Casey Muratori's original introduction to imgui's](http:://mollyrocket.com/861?node=861)
- [Casey Muratori's imgui panel design 1/2](http://mollyrocket.com/casey/stream_0019.html)
- [Casey Muratori's imgui panel design 2/2](http://mollyrocket.com/casey/stream_0020.html)
- [ImGui: The inspiration for this project](https://github.com/ocornut/imgui)

# License
    (The MIT License)
