# Nuklear
This is a minimal state immediate mode graphical user interface toolkit
written in ANSI C and licensed under public domain. It was designed as a simple
embeddable user interface for application and does not have any dependencies,
a default renderbackend or OS window and input handling but instead provides a very modular
library approach by using simple input state for input and draw
commands describing primitive shapes as output. So instead of providing a
layered library that tries to abstract over a number of platform and
render backends it only focuses on the actual UI.

## Features
- Immediate mode graphical user interface toolkit
- Single header library
- Written in C89 (ANSI C)
- Small codebase (~15kLOC)
- Focus on portability, efficiency and simplicity
- No dependencies (not even the standard library if not wanted)
- Fully skinnable and customizable
- Low memory footprint with total memory control if needed or wanted
- UTF-8 support
- No global or hidden state
- Customizeable library modules (you can compile and use only what you need)
- Optional font baker and vertex buffer output

## Building
This library is self contained in one single header file and can be used either
in header only mode or in implementation mode. The header only mode is used
by default when included and allows including this header in other headers
and does not contain the actual implementation.

The implementation mode requires to define  the preprocessor macro
`NK_IMPLEMENTATION` in *one* .c/.cpp file before #includeing this file, e.g.:
```c
    #define NK_IMPLEMENTATION
    #include "nuklear.h"
```

## Gallery
![screenshot](https://cloud.githubusercontent.com/assets/8057201/11761525/ae06f0ca-a0c6-11e5-819d-5610b25f6ef4.gif)
![screen](https://cloud.githubusercontent.com/assets/8057201/13538240/acd96876-e249-11e5-9547-5ac0b19667a0.png)
![screen2](https://cloud.githubusercontent.com/assets/8057201/13538243/b04acd4c-e249-11e5-8fd2-ad7744a5b446.png)
![node](https://cloud.githubusercontent.com/assets/8057201/9976995/e81ac04a-5ef7-11e5-872b-acd54fbeee03.gif)
![skinning](https://cloud.githubusercontent.com/assets/8057201/14152357/25df939e-f6b3-11e5-8587-b19e863e0d1b.png)

## Example
```c
/* init gui state */
struct nk_context ctx;
nk_init_fixed(&ctx, calloc(1, MAX_MEMORY), MAX_MEMORY, &font);

enum {EASY, HARD};
int op = EASY;
float value = 0.6f;
int i =  20;

struct nk_panel layout;
nk_begin(&ctx, &layout, "Show", nk_rect(50, 50, 220, 220),
    NK_WINDOW_BORDER|NK_WINDOW_MOVEABLE|NK_WINDOW_CLOSEABLE);
{
    /* fixed widget pixel width */
    nk_layout_row_static(&ctx, 30, 80, 1);
    if (nk_button_text(&ctx, "button", NK_BUTTON_DEFAULT)) {
        /* event handling */
    }

    /* fixed widget window ratio width */
    nk_layout_row_dynamic(&ctx, 30, 2);
    if (nk_option(&ctx, "easy", op == EASY)) op = EASY;
    if (nk_option(&ctx, "hard", op == HARD)) op = HARD;

    /* custom widget pixel width */
    nk_layout_row_begin(&ctx, NK_STATIC, 30, 2);
    {
        nk_layout_row_push(&ctx, 50);
        nk_label(&ctx, "Volume:", NK_TEXT_LEFT);
        nk_layout_row_push(&ctx, 110);
        nk_slider_float(&ctx, 0, &value, 1.0f, 0.1f);
    }
    nk_layout_row_end(&ctx);
}
nk_end(ctx);
```
![example](https://cloud.githubusercontent.com/assets/8057201/10187981/584ecd68-675c-11e5-897c-822ef534a876.png)


##FAQ
---
#### Why single-file headers?
Windows doesn't have standard directories where libraries
live. That makes deploying libraries in Windows a lot more
painful than open source developers on Unix-derivates generally
realize. (It also makes library dependencies a lot worse in Windows.)

There's also a common problem in Windows where a library was built
against a different version of the runtime library, which causes
link conflicts and confusion. Shipping the libs as headers means
you normally just compile them straight into your project without
making libraries, thus sidestepping that problem.

Making them a single file makes it very easy to just
drop them into a project that needs them. (Of course you can
still put them in a proper shared library tree if you want.)

Why not two files, one a header and one an implementation?
The difference between 10 files and 9 files is not a big deal,
but the difference between 2 files and 1 file is a big deal.
You don't need to zip or tar the files up, you don't have to
remember to attach *two* files, etc.

#### Where is the documentation?
Each file has documentation, basic ussage description and
examples at the top of the file. In addition each API function,
struct and member variables are documented as well.
Finally each library has a corresponding test file inside the
test directory for additional working examples.

#### Why C?
Personally I primarily use C instead of C++ and since I want to
support both C and C++ and C++ is not useable from C I therefore focus
on C.

#### Why C89?
I use C89 instead of C99/C11 for its portability between different compilers
and accessiblity for other languages.

##CREDITS:
Developed by Micha Mettke and every direct or indirect contributor to the GitHub.


Embeds stb_texedit, stb_truetype and stb_rectpack by Sean Barret (public domain)
Embeds ProggyClean.ttf font by Tristan Grimmer (MIT license).


Big thank you to Omar Cornut (ocornut@github) for his imgui library and
giving me the inspiration for this library, Casey Muratori for handmade hero
and his original immediate mode graphical user interface idea and Sean
Barret for his amazing single header libraries which restored by faith
in libraries and brought me to create some of my own.

##LICENSE:
This software is dual-licensed to the public domain and under the following
license: you are granted a perpetual, irrevocable license to copy, modify,
publish and distribute this file as you see fit.

