# Zahnrad
[![Coverity Status](https://scan.coverity.com/projects/5863/badge.svg)](https://scan.coverity.com/projects/5863)

This is a minimal state immediate mode graphical user interface toolkit
written in ANSI C and licensed under zlib. It was designed as a simple embeddable user interface for
application and does not have any direct dependencies,
a default renderbackend or OS window and input handling but instead provides a very modular
library approach by using simple input state for input and draw
commands describing primitive shapes as output. So instead of providing a
layered library that tries to abstract over a number of platform and
render backends it only focuses on the actual UI.

## Features
- Immediate mode graphical user interface toolkit
- Written in C89 (ANSI C)
- Small codebase (~9kLOC)
- Focus on portability, efficiency, simplicity and minimal internal state
- No dependencies (not even the standard library)
- No global or hidden state
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
![screenshot](https://cloud.githubusercontent.com/assets/8057201/11761525/ae06f0ca-a0c6-11e5-819d-5610b25f6ef4.gif)
![demo](https://cloud.githubusercontent.com/assets/8057201/11282359/3325e3c6-8eff-11e5-86cb-cf02b0596087.png)
![node](https://cloud.githubusercontent.com/assets/8057201/9976995/e81ac04a-5ef7-11e5-872b-acd54fbeee03.gif)

## Example
```c
/* init gui state */
struct zr_context ctx;
zr_init_fixed(&ctx, calloc(1, MAX_MEMORY), MAX_MEMORY, &font, sinf, cosf);

enum {EASY, HARD};
int op = EASY;
float value = 0.6f;
int i =  20;

while (1) {
    zr_input_begin(&ctx.input);
    /* generate input  */
    zr_input_end(&ctx.input);

    zr_begin(&ctx, "Show", zr_rect(50, 50, 220, 220),
        ZR_WINDOW_BORDER|ZR_WINDOW_MOVEABLE|ZR_WINDOW_SCALEABLE|
        ZR_WINDOW_CLOSEABLE|ZR_WINDOW_MINIMIZABLE);
    {
        /* fixed widget pixel width */
        zr_layout_row_static(&ctx, 30, 80, 1);
        if (zr_button_text(&ctx, "button", ZR_BUTTON_DEFAULT)) {
            /* event handling */
        }

        /* fixed widget window ration width */
        zr_layout_row_dynamic(&ctx, 30, 2);
        if (zr_option(&ctx, "easy", op == EASY)) op = EASY;
        if (zr_option(&ctx, "hard", op == HARD)) op = HARD;

        zr_layout_row_dynamic(&ctx, 30, 1);
        zr_property_int(&ctx, "Compression:", 0, &i, 100, 10, 1);

        /* custom widget pixel width */
        zr_layout_row_begin(&ctx, ZR_STATIC, 30, 2);
        {
            zr_layout_row_push(&ctx, 50);
            zr_label(&ctx, "Volume:", ZR_TEXT_LEFT);
            zr_layout_row_push(&ctx, 110);
            zr_slider_float(&ctx, 0, &value, 1.0f, 0.1f);
        }
        zr_layout_row_end(&ctx);
    }
    zr_end(ctx);

    struct zr_command *cmd;
    zr_foreach(cmd, &ctx) {
        /* execute draw command */
    }
}
```
![example](https://cloud.githubusercontent.com/assets/8057201/10187981/584ecd68-675c-11e5-897c-822ef534a876.png)

## Documentation
Zahnrad currently relies heavily on documentation provided inside the `zahnrad.h` header file, consisting
of descriptions and important information about modules, data types and functions.
While being quite limited in delivering information about the general high-level libray composition it
should still offer some understanding about the inner workings and stand as a practical usage reference. 

## Examples
A number of usage examples can be found inside the `example` and `demo` folder which should yield a
basic overview how to embed the libray into different platforms with varying APIs and provided functionality
and hopefully offer a basic understanding of zahnrad's UI API.
In general it is advised to start by reading `example/demo`. It consists of a basic embedding example into
SDL, OpenGL and [NanoVG](https://github.com/memononen/nanovg) with a very simple set of used widgets and layouting.

As soon as a basic understanding of the library is accumulated it is recommended to look into the `demo/` folder with your platform
of choice. For now a basic platform layer was implemented for Linux(X11), Windows(win32) and OpenGL with SDL and GLFW. 
Both platform specific demos (X11, win32) use their respectable window, input, draw and font API and don't have any
outside dependencies which should qualify them as the first platform to compile, run and test.
For hardware supported rendering, font both the SDL and GLFW version use zahnrad's internal vertex buffer output
and font baker.

Up until now you should hopefully have a basic grip of how to use zahnrad UI API and be able to embed zahnrad into
your plaform. From here on `demo/demo.c` should provide a basic reference on how use most widgets and layouting.
Finally for some small actual working example apps both `example/filex` with implementation of a linux only
file browser and `example/nodedit` with a basic node editor skeleton are provided. Especially the `nodedit` example
should show how far you can bend the this library to your own needs.

