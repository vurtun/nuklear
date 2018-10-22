/* nuklear - 1.32.0 - public domain */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <limits.h>
#include <time.h>

#include <allegro5/allegro.h>

#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#define NK_ALLEGRO5_IMPLEMENTATION
#include "../../nuklear.h"
#include "nuklear_allegro5.h"


#define UNUSED(a) (void)a
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) < (b) ? (b) : (a))
#define LEN(a) (sizeof(a)/sizeof(a)[0])

/* ===============================================================
 *
 *                          EXAMPLE
 *
 * ===============================================================*/
/* This are some code examples to provide a small overview of what can be
 * done with this library. To try out an example uncomment the include
 * and the corresponding function. */
/*#include "../style.c"*/
/*#include "../calculator.c"*/
#include "../overview.c"
/*#include "../node_editor.c"*/

/* ===============================================================
 *
 *                          DEMO
 *
 * ===============================================================*/
static void error_callback(int e, const char *d)
{printf("Error %d: %s\n", e, d);}

int main(void)
{
    /* Platform */
    ALLEGRO_DISPLAY *display = NULL;
    ALLEGRO_EVENT_QUEUE *event_queue = NULL;

    if (!al_init()) {
        fprintf(stdout, "failed to initialize allegro5!\n");
        exit(1);
    }

    al_install_mouse();
    al_set_mouse_wheel_precision(150);
    al_install_keyboard();

    al_set_new_display_flags(ALLEGRO_WINDOWED|ALLEGRO_RESIZABLE|ALLEGRO_OPENGL);
    al_set_new_display_option(ALLEGRO_SAMPLE_BUFFERS, 1, ALLEGRO_SUGGEST);
    al_set_new_display_option(ALLEGRO_SAMPLES, 8, ALLEGRO_SUGGEST);
    display = al_create_display(WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!display) {
        fprintf(stdout, "failed to create display!\n");
        exit(1);
    }

    event_queue = al_create_event_queue();
    if (!event_queue) {
        fprintf(stdout, "failed to create event_queue!\n");
        al_destroy_display(display);
        exit(1);
    }

    al_register_event_source(event_queue, al_get_display_event_source(display));
    al_register_event_source(event_queue, al_get_mouse_event_source());
    al_register_event_source(event_queue, al_get_keyboard_event_source());

    NkAllegro5Font *font;
    font = nk_allegro5_font_create_from_file("../../../extra_font/Roboto-Regular.ttf", 12, 0);
    struct nk_context *ctx;

    ctx = nk_allegro5_init(font, display, WINDOW_WIDTH, WINDOW_HEIGHT);

    /* style.c */
    /*set_style(ctx, THEME_WHITE);*/
    /*set_style(ctx, THEME_RED);*/
    /*set_style(ctx, THEME_BLUE);*/
    /*set_style(ctx, THEME_DARK);*/

    while(1)
    {
        ALLEGRO_EVENT ev;
        ALLEGRO_TIMEOUT timeout;
        al_init_timeout(&timeout, 0.06);

        bool get_event = al_wait_for_event_until(event_queue, &ev, &timeout);

        if (get_event && ev.type == ALLEGRO_EVENT_DISPLAY_CLOSE) {
            break;
        }

        /* Very Important: Always do nk_input_begin / nk_input_end even if
           there are no events, otherwise internal nuklear state gets messed up */
        nk_input_begin(ctx);
        if (get_event) {
            while (get_event) {
                nk_allegro5_handle_event(&ev);
                get_event = al_get_next_event(event_queue, &ev);
            }
        }
        nk_input_end(ctx);

        /* GUI */
        if (nk_begin(ctx, "Demo", nk_rect(50, 50, 200, 200),
            NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
            NK_WINDOW_CLOSABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
        {
            enum {EASY, HARD};
            static int op = EASY;
            static int property = 20;

            nk_layout_row_static(ctx, 30, 80, 1);
            if (nk_button_label(ctx, "button"))
                fprintf(stdout, "button pressed\n");
            nk_layout_row_dynamic(ctx, 30, 2);
            if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
            if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;
            nk_layout_row_dynamic(ctx, 22, 1);
            nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);
        }
        nk_end(ctx);

        /* -------------- EXAMPLES ---------------- */
        /*calculator(ctx);*/
        overview(ctx);
        /*node_editor(ctx);*/
        /* ----------------------------------------- */

        /* Draw */
        al_clear_to_color(al_map_rgb(19, 43, 81));
        /* IMPORTANT: `nk_allegro5_render` changes the target backbuffer
        to the display set at initialization and does not restore it.
        Change it if you want to draw somewhere else. */
        nk_allegro5_render();
        al_flip_display();
    }

    nk_allegro5_font_del(font);
    nk_allegro5_shutdown();
    al_destroy_display(display);
    al_destroy_event_queue(event_queue);
    return 0;
}

