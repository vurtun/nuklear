# Zahnrad
[![Coverity Status](https://scan.coverity.com/projects/5863/badge.svg)](https://scan.coverity.com/projects/5863)

This is a minimal state immediate mode graphical user interface toolkit
written in ANSI C. It was designed as a simple embeddable user interface for
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
- vertex buffer output 
- font handling

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
zr_size option = EASY;
zr_float value = 0.6f;

struct zr_context context;
zr_begin(&context, &window);
{
    zr_header(&context, "Show", ZR_CLOSEABLE, 0, ZR_HEADER_LEFT);
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

# License
    (The zlib License)
