# Zahnrad
[![Coverity Status](https://scan.coverity.com/projects/5863/badge.svg)](https://scan.coverity.com/projects/5863)

This is a minimal state immediate mode graphical user interface toolkit
written in ANSI C and licensed under zlib. It was designed as a simple embeddable user interface for
application and does not have any direct dependencies. It does not have
a default renderbackend, os window and input handling but instead provides a very modular
library approach by providing a simple input state storage for input and draw
commands describing primitive shapes as output. So instead of providing a
layered library that tries to abstract over a number of platform and
render backends it only focuses on the actual UI.

## Features
- Immediate mode graphical user interface toolkit
- Written in C89 (ANSI C)
- Small codebase (~8kLOC)
- Focus on portability, efficiency, simplicity and minimal internal state
- No global or hidden state
- No direct dependencies
- Configurable style and colors
- UTF-8 support

## Optional
- Vertex buffer output 
- Font handling

## Building
The library is self-contained within four different files that only have to be
copied and compiled into your application. Files zahnrad.c and zahnrad.h make up
the core of the library, while stb_rect_pack.h and stb_truetype.h are
for a optional font handling implementation and can be removed if not needed.
- zahnrad.c
- zahnrad.h
- stb_rect_pack.h (optional)
- stb_truetype.h (optional)

There are no dependencies or a particular building process required. You just have
to compile the .c file and #include zahnrad.h into your project. To actually
run you have to provide the input state, configuration style and memory
for draw commands to the library. After the GUI was executed all draw commands
have to be either executed or optionally converted into a vertex buffer to
draw the GUI.

## Gallery
![screenshot](https://cloud.githubusercontent.com/assets/8057201/11300153/08473cca-8f8d-11e5-821b-b97228007b4d.png)
![demo](https://cloud.githubusercontent.com/assets/8057201/11282359/3325e3c6-8eff-11e5-86cb-cf02b0596087.png)
![node](https://cloud.githubusercontent.com/assets/8057201/9976995/e81ac04a-5ef7-11e5-872b-acd54fbeee03.gif)

## Example
```c
enum {EASY, HARD};
int option = EASY;
float value = 0.6f;

struct zr_context context;
zr_begin(&context, &window, "Show");
{
    zr_layout_row_static(&context, 30, 80, 1);
    if (zr_button_text(&context, "button", ZR_BUTTON_DEFAULT)) {
        /* event handling */
    }
    zr_layout_row_dynamic(&context, 30, 2);
    if (zr_option(&context, "easy", option == EASY)) option = EASY;
    if (zr_option(&context, "hard", option == HARD)) option = HARD;
    zr_label(&context, "Volume:", ZR_TEXT_LEFT);
    zr_slider_float(&context, 0, &value, 1.0f, 0.1f);
    zr_layout_row_end(&context);
}
zr_end(&context, &window);
```
![example](https://cloud.githubusercontent.com/assets/8057201/10187981/584ecd68-675c-11e5-897c-822ef534a876.png)

## Documentation
For code related documentation and information on how to link each single part to a whole I
would recommend reading the documentation inside the `zahnrad.h` header file wich has
information and descriptions about each part, struct, members and functions. In addition
I would especially recommend reading the examples and the demo.

If you want to dive into the example code I would recommand starting with `example/demo/demo.c`,
even if you don't know or want to use nanovg. It contains the usage code for every simple to use
core part of the library while providing a good looking UI. It also introduces a small set
of widgets while providing a simple example of how to do layouting.

As soon as you have a basic grip of how to use the library I would recommend looking at
`demo/demo.c`. It only contains the actual UI part of the GUI but offers example ussage
code for all widgets in the library. So it functions more as a reference to look how
a widget is supposed to be used.

On how to do the platform and render backend depended part I would recommend looking at
some example platform implementations for Win32 (`demo/win32/win32.c`) for windows and
X11 (`demo/x11.xlib.c`) for linux. Both provide the absolute minimum needed platform
dependend code without using any libraries. Finally for the most complex plaform demos
it is definitely worth it to read the OpenGL examples with `demo/sdl/sdl.c` for
zahnrad integration with SDL2, OpenGL and GLEW and `demo/glfw/glfw.c` for integration
with GLFW, OpenGL and GLEW. They also include usage code for the optional zahnrad font handling
and vertex buffer output.

The final two examples `example/filex/filex.c` and `example/nodedit/nodedit.c` both provide
actual example application use cases for this library. Filex is a simple file browser for Linux
and shows how to do window tiling. The node editor on the other hand is probably the more
interesting of the two since it shows how far you can bend this library to your
specific problem on hand.

