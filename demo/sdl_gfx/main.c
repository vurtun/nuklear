/* nuklear - v1.00 - public domain */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <math.h>

#include <SDL/SDL.h>

/* these defines are both needed for the header
 * and source file. So if you split them remember
 * to copy them as well. */
#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#include "nuklear_sdl.h"
#include "nuklear_sdl.c"

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

int
main(void)
{
	static SDL_Surface *screen_surface;
	struct nk_color background;
	struct nk_colorf bg;
	int win_width, win_height;
	int running = 1;
	struct nk_context *ctx;
	/* float bg[4]; */

	/* SDL setup */
	if (SDL_Init(SDL_INIT_VIDEO) == -1) {
		printf( "Can't init SDL:  %s\n", SDL_GetError( ) );
		return 1;
	}
	screen_surface = SDL_SetVideoMode(WINDOW_WIDTH, WINDOW_HEIGHT, 32, SDL_SWSURFACE);
	if (screen_surface == NULL) {
		printf( "Can't set video mode: %s\n", SDL_GetError( ) );
		return 1;
	}

	if (!(ctx = nk_sdl_init(screen_surface))) {
		printf("nk_sdl_init() failed!");
		return 1;
	}

	background = nk_rgb(28,48,62);
	bg = nk_color_cf(background);
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
		{
		if (nk_begin(ctx, "Demo", nk_rect(50, 50, 210, 250),
			NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
			NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
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
			nk_layout_row_dynamic(ctx, 25, 1);
			nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);

			{
			nk_layout_row_dynamic(ctx, 20, 1);
			nk_label(ctx, "background:", NK_TEXT_LEFT);
			nk_layout_row_dynamic(ctx, 25, 1);
            if (nk_combo_begin_color(ctx, nk_rgb_cf(bg), nk_vec2(nk_widget_width(ctx),400))) {
				nk_layout_row_dynamic(ctx, 120, 1);
				bg = nk_color_picker(ctx, bg, NK_RGBA);
				nk_layout_row_dynamic(ctx, 25, 1);
                bg.r = nk_propertyf(ctx, "#R:", 0, bg.r, 1.0f, 0.01f,0.005f);
                bg.g = nk_propertyf(ctx, "#G:", 0, bg.g, 1.0f, 0.01f,0.005f);
                bg.b = nk_propertyf(ctx, "#B:", 0, bg.b, 1.0f, 0.01f,0.005f);
                bg.a = nk_propertyf(ctx, "#A:", 0, bg.a, 1.0f, 0.01f,0.005f);
				nk_combo_end(ctx);
			}}
		}
		nk_end(ctx);}

		/* Draw */
		/* nk_color_fv(bg, background); */
		nk_sdl_render(nk_rgb_cf(bg));
	}

cleanup:
	nk_sdl_shutdown();
	SDL_Quit();
	return 0;
}
