# Zahnrad
[![Coverity Status](https://scan.coverity.com/projects/5863/badge.svg)](https://scan.coverity.com/projects/5863)

This is a bloat free minimal state immediate mode graphical user interface toolkit
written in ANSI C. It was designed as a simple embeddable user interface for
application and does not have any direct dependencies. It does not have
a default renderbackend, os window and input handling but instead provides a very modular
library approach by providing a simple input state storage for input and draw
commands describing primitive shapes as output. So instead of providing a
layered library that tries to abstract over a number of platform and
render backends it only focuses on the actual GUI.

## Features
- Immediate mode graphical user interface toolkit
- Written in C89 (ANSI C)
- Small codebase (~8kLOC)
- Focus on portability, efficiency, simplicity and minimal internal state
- No global or hidden state
- No direct dependencies
- Configurable style and colors
- UTF-8 support
- Optional vertex buffer output and font handling

The library is self-contained within four different files that only have to be
copied and compiled into your application. Files zahnrad.c and zahnrad.h make up
the core of the library, while stb_rect_pack.h and stb_truetype.h are
for a optional font handling implementation and can be removed if not needed.
- zahnrad.c
- zahnrad.h
- stb_rect_pack.h
- stb_truetype.h

There are no dependencies or a particular building process required. You just have
to compile the .c file and #include zahnrad.h into your project. To actually
run you have to provide the input state, configuration style and memory
for draw commands to the library. After the GUI was executed all draw commands
have to be either executed or optionally converted into a vertex buffer to
draw the GUI.

## Gallery
![1](https://cloud.githubusercontent.com/assets/8057201/11033668/59ab5d04-86e5-11e5-8091-c56f16411565.png)
![2](https://cloud.githubusercontent.com/assets/8057201/11033664/5074a588-86e5-11e5-8308-8e1f4724ae85.png)
![3](https://cloud.githubusercontent.com/assets/8057201/11033654/3f0c5a5c-86e5-11e5-8529-4bb5ac3b357a.png)
![explorer](https://cloud.githubusercontent.com/assets/8057201/10718115/02a9ba08-7b6b-11e5-950f-adacdd637739.png)
![node](https://cloud.githubusercontent.com/assets/8057201/9976995/e81ac04a-5ef7-11e5-872b-acd54fbeee03.gif)

## Example
```c
/* allocate memory to hold draw commands */
struct zr_command_queue queue;
zr_command_queue_init_fixed(&queue, malloc(MEMORY_SIZE), MEMORY_SIZE);

/* setup configuration */
struct zr_style style;
struct zr_user_font font = {...};
zr_style_default(&style, ZR_DEFAULT_ALL, &font);

/* initialize window */
struct zr_window window;
zr_window_init(&window, zr_rect(50, 50, 220, 180),
    ZR_WINDOW_BORDER|ZR_WINDOW_MOVEABLE|ZR_WINDOW_SCALEABLE,
    &queue, &style, &input);

/* setup widget data */
enum {EASY, HARD};
zr_size option = EASY;
zr_float value = 0.6f;

struct zr_input input = {0};
while (1) {
    zr_input_begin(&input);
    /* record input */
    zr_input_end(&input);

    /* GUI */
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
        zr_layout_row_begin(&context, ZR_STATIC, 30, 2);
        {
            zr_layout_row_push(&context, 50);
            zr_label(&context, "Volume:", ZR_TEXT_LEFT);
            zr_layout_row_push(&context, 110);
            zr_slider_float(&context, 0, &value, 1.0f, 0.1f);
        }
        zr_layout_row_end(&context);
    }
    zr_end(&context, &window);

    /* draw */
    const struct zr_command *cmd;
    zr_foreach_command(cmd, &queue) {
        /* execute draw call command */
    }
    zr_command_queue_clear(&queue);
}
```
![example](https://cloud.githubusercontent.com/assets/8057201/10187981/584ecd68-675c-11e5-897c-822ef534a876.png)

## References
- [stb_rect_pack.h and stb_truetype.h by Sean Barret (public domain)](https:://github.com/nothings/stb/)
- [Tutorial from Jari Komppa about imgui libraries](http://www.johno.se/book/imgui.html)
- [Johannes 'johno' Norneby's article](http://iki.fi/sol/imgui/)
- [ImGui: The inspiration for this project from ocornut](https://github.com/ocornut/imgui)
- [Nvidia's imgui toolkit](https://code.google.com/p/nvidia-widgets/)

# License
    (The zlib License)
