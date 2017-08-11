/* nuklear - 1.32.0 - public domain */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <math.h>
#include <limits.h>
#include <time.h>

#include <SDL2/SDL.h>
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_STANDARD_IO
#define NK_INCLUDE_STANDARD_VARARGS
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_INCLUDE_FONT_BAKING
#define NK_INCLUDE_DEFAULT_FONT
#define NK_SDL_SOFT_IMPLEMENTATION
#define NK_IMPLEMENTATION
#include "../../nuklear.h"
#include "sdl_software.h"



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
/*#include "../overview.c"*/
/*#include "../node_editor.c"*/

/* ===============================================================
 *
 *                          DEMO
 *
 * ===============================================================*/
int
main(int argc, char* argv[])
{
    /* Platform */
	SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "0");
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER | SDL_INIT_EVENTS);
	TTF_Init();
	TTF_Font * font = NULL;
	SDL_Window *win = SDL_CreateWindow("Demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
	SDL_Renderer *  ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
	///TTF_OpenFont()
	
	struct nk_color background;
    int win_width, win_height;
    int running = 1;

    /* GUI */
    struct nk_context *ctx;

    SDL_GetWindowSize(win, &win_width, &win_height);


    ctx = nk_sdl_soft_init(ren);
    /* Load Fonts: if none of these are loaded a default font will be used  */
    /* Load Cursor: if you uncomment cursor loading please hide the cursor */
	//{struct nk_font_atlas *atlas;
	//nk_sdl_font_stash_begin(&atlas); 
	
	/*struct nk_font *droid = nk_font_atlas_add_from_file(atlas, "../../../extra_font/DroidSans.ttf", 14, 0);*/
	/*struct nk_font *roboto = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Roboto-Regular.ttf", 16, 0);*/
	/*struct nk_font *future = nk_font_atlas_add_from_file(atlas, "../../../extra_font/kenvector_future_thin.ttf", 13, 0);*/
	/*struct nk_font *clean = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyClean.ttf", 12, 0);*/
	/*struct nk_font *tiny = nk_font_atlas_add_from_file(atlas, "../../../extra_font/ProggyTiny.ttf", 10, 0);*/
	/*struct nk_font *cousine = nk_font_atlas_add_from_file(atlas, "../../../extra_font/Cousine-Regular.ttf", 13, 0);*/
	//nk_sdl_font_stash_end(); }

	background= nk_rgb(111, 33, 44);
	background.a = 128;
    //background = nk_rgb(28,48,62);
	while (running)
	{
		/* Input */
		SDL_Event evt;
		nk_input_begin(ctx);
		while (SDL_PollEvent(&evt)) {
			if (evt.type == SDL_QUIT) goto cleanup;
			nk_sdl_handle_event(&evt);
		}
		nk_input_end(ctx);


		/* GUI */
		if (nk_begin(ctx, "Demo", nk_rect(50, 50, 200, 200),
			NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
			NK_WINDOW_CLOSABLE | NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
		{
			nk_menubar_begin(ctx);
			nk_layout_row_begin(ctx, NK_STATIC, 25, 2);
			nk_layout_row_push(ctx, 45);
			if (nk_menu_begin_label(ctx, "FILE", NK_TEXT_LEFT, nk_vec2(120, 200))) {
				nk_layout_row_dynamic(ctx, 30, 1);
				nk_menu_item_label(ctx, "OPEN", NK_TEXT_LEFT);
				nk_menu_item_label(ctx, "CLOSE", NK_TEXT_LEFT);
				nk_menu_end(ctx);
			}
			nk_layout_row_push(ctx, 45);
			if (nk_menu_begin_label(ctx, "EDIT", NK_TEXT_LEFT, nk_vec2(120, 200))) {
				nk_layout_row_dynamic(ctx, 30, 1);
				nk_menu_item_label(ctx, "COPY", NK_TEXT_LEFT);
				nk_menu_item_label(ctx, "CUT", NK_TEXT_LEFT);
				nk_menu_item_label(ctx, "PASTE", NK_TEXT_LEFT);
				nk_menu_end(ctx);
			}
			nk_layout_row_end(ctx);
			nk_menubar_end(ctx);

			enum { EASY, HARD };
			static int op = EASY;
			static int property = 20;
			nk_layout_row_static(ctx, 30, 80, 1);
			if (nk_button_label(ctx, "button"))
				fprintf(stdout, "button pressed\n");
			nk_layout_row_dynamic(ctx, 30, 2);
			if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
			if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;
			nk_layout_row_dynamic(ctx, 25, 1);
			nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);
		}
		nk_end(ctx);

		/* -------------- EXAMPLES ---------------- */
		/*calculator(ctx);*/
		/*overview(ctx);*/
		/*node_editor(ctx);*/
		/* ----------------------------------------- */

		/* Draw */
		SDL_SetRenderDrawColor(ren, background.r, background.b, background.g, background.a);
		SDL_RenderClear(ren);

		{float bg[4];
		nk_color_fv(bg, background);
		SDL_GetWindowSize(win, &win_width, &win_height);
		nk_sdl_soft_render();
		}
	
		SDL_RenderPresent(ren);

	}

cleanup:
    nk_sdl_soft_shutdown();
//    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}

