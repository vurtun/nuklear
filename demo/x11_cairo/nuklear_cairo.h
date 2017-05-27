/*
 * Nuklear - v1.00 - public domain
 * no warrenty implied; use at your own risk.
 * authored 2016 by Hanspeter Portner
 */

/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */
#ifndef NK_CAIRO_H_
#define NK_CAIRO_H_

NK_API void                 nk_cairo_init(struct nk_context *ctx, struct nk_user_font *font, cairo_t *cairo);
NK_API int                  nk_cairo_handle_event(struct nk_context *ctx, Display *dpy, XEvent *evt);
NK_API void                 nk_cairo_clear(cairo_t *cairo, int w, int h, struct nk_color clear);
NK_API void                 nk_cairo_render(struct nk_context *ctx, cairo_t *cairo);
NK_API void                 nk_cairo_shutdown(struct nk_context *ctx);
NK_API struct nk_image      nk_cairo_png_load(const char *filename);
NK_API void                 nk_cairo_png_unload(struct nk_image image);

#endif
/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
* ===============================================================
 */
#ifdef NK_CAIRO_IMPLEMENTATION

#ifndef M_PI
#	define M_PI 3.141592654
#endif

NK_INTERN float
nk_cairo_get_text_width(nk_handle handle, float height, const char *text, int len)
{
	cairo_t *cairo = handle.ptr;
	cairo_text_extents_t extents;

	cairo_select_font_face(cairo, "cairo:monospace", CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
	cairo_set_font_size(cairo, height);

	if(text[len] == '\0') /* is zero terminated */
	{
		cairo_text_extents(cairo, text, &extents);
	}
	else
	{
		char *str = malloc((len + 1)*sizeof(char));

		if(!str)
			return 0;

		strncpy(str, text, len);
		str[len] = '\0';

		cairo_text_extents(cairo, str, &extents);

		free(str);
	}

	return extents.x_advance;
}

NK_INTERN void
nk_cairo_get_color(struct nk_color color, float *r, float *g, float *b, float *a)
{
	*r = (float)color.r / 0xff;
	*g = (float)color.g / 0xff;
	*b = (float)color.b / 0xff;
	*a = (float)color.a / 0xff;
}

NK_INTERN void
nk_cairo_set_color(cairo_t *cairo, struct nk_color color)
{
	float r, g, b, a;
	nk_cairo_get_color(color, &r, &g, &b, &a);
	cairo_set_source_rgba(cairo, r, g, b, a);
}

NK_INTERN void
nk_cairo_mesh_pattern_set_corner_color(cairo_pattern_t *pat, int idx, struct nk_color color)
{
	float r, g, b, a;
	nk_cairo_get_color(color, &r, &g, &b, &a);
	cairo_mesh_pattern_set_corner_color_rgba(pat, idx, r, g, b, a);
}

NK_INTERN void
nk_cairo_rectangle(cairo_t *cairo, double x, double y, double w, double h, double r)
{
	cairo_new_sub_path(cairo);
	cairo_arc(cairo, x + w - r, y + r, r,     -M_PI/2, 0.0);
	cairo_arc(cairo, x + w - r, y + h - r, r,  0.0,    M_PI/2);
	cairo_arc(cairo, x + r,     y + h - r, r,  M_PI/2, M_PI);
	cairo_arc(cairo, x + r,     y + r, r,      M_PI,   3*M_PI/2);
	cairo_close_path(cairo);
}

NK_INTERN void
nk_cairo_circle(cairo_t *cairo, double x, double y, double w, double h)
{
	const double w2 = w/2;
	const double h2 = h/2;

	cairo_save(cairo);
	cairo_translate(cairo, x + w2, y + h2);
	cairo_scale(cairo, w2, h2);
	cairo_arc(cairo, 0.0, 0.0, 1.0, 0.0, 2*M_PI);
	cairo_restore(cairo);
}

NK_INTERN void
nk_cairo_stroke(cairo_t *cairo, unsigned short line_thickness, struct nk_color color)
{
	cairo_set_line_width(cairo, line_thickness);
	nk_cairo_set_color(cairo, color);
	cairo_stroke(cairo);
}

NK_INTERN void
nk_cairo_fill(cairo_t *cairo, struct nk_color color)
{
	nk_cairo_set_color(cairo, color);
	cairo_fill(cairo);
}

NK_API void
nk_cairo_clear(cairo_t *cairo, int w, int h, struct nk_color clear)
{
	cairo_rectangle(cairo, 0, 0, w, h);
	nk_cairo_fill(cairo, clear);
}

NK_API void
nk_cairo_init(struct nk_context *ctx, struct nk_user_font *font, cairo_t *cairo)
{
	font->userdata = nk_handle_ptr(cairo);
	font->height = 12;
	font->width = nk_cairo_get_text_width;

	nk_init_default(ctx, font);
}

NK_API int
nk_cairo_handle_event(struct nk_context *ctx, Display *dpy, XEvent *evt)
{
	if( (evt->type == KeyPress) || (evt->type == KeyRelease) )
	{
		/* Key handler */
		int down = evt->type == KeyPress;
		int ret;
		KeySym *code = XGetKeyboardMapping(dpy, (KeyCode)evt->xkey.keycode, 1, &ret);

		if(*code == XK_Shift_L || *code == XK_Shift_R)
			nk_input_key(ctx, NK_KEY_SHIFT, down);
		else if(*code == XK_Delete)
			nk_input_key(ctx, NK_KEY_DEL, down);
		else if(*code == XK_Return)
			nk_input_key(ctx, NK_KEY_ENTER, down);
		else if(*code == XK_Tab)
			nk_input_key(ctx, NK_KEY_TAB, down);
		else if(*code == XK_Left)
			nk_input_key(ctx, NK_KEY_LEFT, down);
		else if(*code == XK_Right)
			nk_input_key(ctx, NK_KEY_RIGHT, down);
		else if(*code == XK_BackSpace)
			nk_input_key(ctx, NK_KEY_BACKSPACE, down);
		else if(*code == XK_space && !down)
			nk_input_char(ctx, ' ');
		else if(*code == XK_Page_Up)
			nk_input_key(ctx, NK_KEY_SCROLL_UP, down);
		else if(*code == XK_Page_Down)
			nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
		else if(*code == XK_Home)
		{
			nk_input_key(ctx, NK_KEY_TEXT_START, down);
			nk_input_key(ctx, NK_KEY_SCROLL_START, down);
		}
		else if(*code == XK_End)
		{
			nk_input_key(ctx, NK_KEY_TEXT_END, down);
			nk_input_key(ctx, NK_KEY_SCROLL_END, down);
		}
		else
		{
			if(*code == 'c' && (evt->xkey.state & ControlMask))
				nk_input_key(ctx, NK_KEY_COPY, down);
			else if(*code == 'v' && (evt->xkey.state & ControlMask))
				nk_input_key(ctx, NK_KEY_PASTE, down);
			else if(*code == 'x' && (evt->xkey.state & ControlMask))
				nk_input_key(ctx, NK_KEY_CUT, down);
			else if(*code == 'z' && (evt->xkey.state & ControlMask))
				nk_input_key(ctx, NK_KEY_TEXT_UNDO, down);
			else if(*code == 'r' && (evt->xkey.state & ControlMask))
				nk_input_key(ctx, NK_KEY_TEXT_REDO, down);
			else if(*code == XK_Left && (evt->xkey.state & ControlMask))
				nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
			else if(*code == XK_Right && (evt->xkey.state & ControlMask))
				nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
			else if(*code == 'b' && (evt->xkey.state & ControlMask))
				nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down);
			else if(*code == 'e' && (evt->xkey.state & ControlMask))
				nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down);
			else
			{
				if(*code == 'i')
					nk_input_key(ctx, NK_KEY_TEXT_INSERT_MODE, down);
				else if(*code == 'r')
					nk_input_key(ctx, NK_KEY_TEXT_REPLACE_MODE, down);
				if(down)
				{
					char buf[32];
					KeySym keysym = 0;
					if(XLookupString((XKeyEvent*)evt, buf, 32, &keysym, NULL) != NoSymbol)
						nk_input_glyph(ctx, buf);
				}
			}
		}
		XFree(code);
		return 1;
	}
	else if( (evt->type == ButtonPress) || (evt->type == ButtonRelease) )
	{
		/* Button handler */
		int down = (evt->type == ButtonPress);
		const int x = evt->xbutton.x;
		const int y = evt->xbutton.y;

		if(evt->xbutton.button == Button1)
			nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
		if(evt->xbutton.button == Button2)
			nk_input_button(ctx, NK_BUTTON_MIDDLE, x, y, down);
		else if(evt->xbutton.button == Button3)
			nk_input_button(ctx, NK_BUTTON_RIGHT, x, y, down);
		else if(evt->xbutton.button == Button4)
			nk_input_scroll(ctx, 1.0f);
		else if(evt->xbutton.button == Button5)
			nk_input_scroll(ctx, -1.0f);
		else return 0;
		return 1;
	}
	else if(evt->type == MotionNotify)
	{
		/* Mouse motion handler */
		const int x = evt->xmotion.x;
		const int y = evt->xmotion.y;
		nk_input_motion(ctx, x, y);
		return 1;
	}
	else if(evt->type == KeymapNotify)
	{
		XRefreshKeyboardMapping(&evt->xmapping);
		return 1;
	}
	return 0;
}

NK_API void
nk_cairo_render(struct nk_context *ctx, cairo_t *cairo)
{
	const struct nk_command *cmd;
	int is_clipped = 0;

	cairo_save(cairo);

	nk_foreach(cmd, ctx)
	{
		switch(cmd->type)
		{
			case NK_COMMAND_NOP:
				break;
			case NK_COMMAND_SCISSOR: {
					const struct nk_command_scissor *s =(const struct nk_command_scissor*)cmd;

					cairo_save(cairo);
					cairo_rectangle(cairo, s->x, s->y, s->w, s->h);
					cairo_clip(cairo);
					is_clipped += 1;
			} break;
			case NK_COMMAND_LINE: {
					const struct nk_command_line *l = (const struct nk_command_line *)cmd;

					cairo_move_to(cairo, l->begin.x, l->begin.y);
					cairo_line_to(cairo, l->end.x, l->end.y);
					nk_cairo_stroke(cairo, l->line_thickness, l->color);
			} break;
			case NK_COMMAND_CURVE: {
					const struct nk_command_curve *q = (const struct nk_command_curve *)cmd;

					cairo_move_to(cairo, q->begin.x, q->begin.y);
					cairo_curve_to(cairo, q->ctrl[0].x, q->ctrl[1].y,
						q->ctrl[1].x, q->ctrl[1].y, q->end.x, q->end.y);
					nk_cairo_stroke(cairo, q->line_thickness, q->color);
			} break;
			case NK_COMMAND_RECT: {
					const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;

					nk_cairo_rectangle(cairo, r->x, r->y, r->w, r->h, r->rounding);
					nk_cairo_stroke(cairo, r->line_thickness, r->color);
			} break;
			case NK_COMMAND_RECT_FILLED: {
					const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled *)cmd;

					nk_cairo_rectangle(cairo, r->x, r->y, r->w, r->h, r->rounding);
					nk_cairo_fill(cairo, r->color);
			} break;
			case NK_COMMAND_RECT_MULTI_COLOR: {
					const struct nk_command_rect_multi_color *r = (const struct nk_command_rect_multi_color *)cmd;

					cairo_pattern_t *pat = cairo_pattern_create_mesh();
					if(pat)
					{
						cairo_mesh_pattern_begin_patch(pat);
						cairo_mesh_pattern_move_to(pat, r->x, r->y);
						cairo_mesh_pattern_line_to(pat, r->x, r->y + r->h);
						cairo_mesh_pattern_line_to(pat, r->x + r->w, r->y + r->h);
						cairo_mesh_pattern_line_to(pat, r->x + r->w, r->y);
						nk_cairo_mesh_pattern_set_corner_color(pat, 0, r->left);
						nk_cairo_mesh_pattern_set_corner_color(pat, 1, r->bottom);
						nk_cairo_mesh_pattern_set_corner_color(pat, 2, r->right);
						nk_cairo_mesh_pattern_set_corner_color(pat, 3, r->top);
						cairo_mesh_pattern_end_patch(pat);

						cairo_rectangle(cairo, r->x, r->y, r->w, r->h);
						cairo_set_source(cairo, pat);
						cairo_fill(cairo);

						cairo_pattern_destroy(pat);
					}
			} break;
			case NK_COMMAND_CIRCLE: {
					const struct nk_command_circle *c = (const struct nk_command_circle *)cmd;

					nk_cairo_circle(cairo, c->x, c->y, c->w, c->h);
					nk_cairo_stroke(cairo, c->line_thickness, c->color);
			} break;
			case NK_COMMAND_CIRCLE_FILLED: {
					const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;

					nk_cairo_circle(cairo, c->x, c->y, c->w, c->h);
					nk_cairo_fill(cairo, c->color);
			} break;
			case NK_COMMAND_ARC: {
					const struct nk_command_arc *a = (const struct nk_command_arc *)cmd;

					cairo_arc(cairo, a->cx, a->cy, a->r, a->a[0], a->a[1]);
					nk_cairo_stroke(cairo, a->line_thickness, a->color);
			} break;
			case NK_COMMAND_ARC_FILLED: {
					const struct nk_command_arc *a = (const struct nk_command_arc *)cmd;

					cairo_arc(cairo, a->cx, a->cy, a->r, a->a[0], a->a[1]);
					nk_cairo_fill(cairo, a->color);
			} break;
			case NK_COMMAND_TRIANGLE: {
					const struct nk_command_triangle *t = (const struct nk_command_triangle*)cmd;

					cairo_move_to(cairo, t->a.x, t->a.y);
					cairo_line_to(cairo, t->b.x, t->b.y);
					cairo_line_to(cairo, t->c.x, t->c.y);
					cairo_close_path(cairo);
					nk_cairo_stroke(cairo, t->line_thickness, t->color);
			} break;
			case NK_COMMAND_TRIANGLE_FILLED: {
					const struct nk_command_triangle_filled *t = (const struct nk_command_triangle_filled *)cmd;

					cairo_move_to(cairo, t->a.x, t->a.y);
					cairo_line_to(cairo, t->b.x, t->b.y);
					cairo_line_to(cairo, t->c.x, t->c.y);
					cairo_close_path(cairo);
					nk_cairo_fill(cairo, t->color);
			} break;
			case NK_COMMAND_POLYGON: {
					const struct nk_command_polygon *p =(const struct nk_command_polygon*)cmd;
					int i;

					cairo_move_to(cairo, p->points[0].x, p->points[0].y);
					for(i=1; i<p->point_count; p++)
						cairo_line_to(cairo, p->points[i].x, p->points[i].y);
					cairo_close_path(cairo);
					nk_cairo_stroke(cairo, p->line_thickness, p->color);
			} break;
			case NK_COMMAND_POLYGON_FILLED: {
					const struct nk_command_polygon_filled *p = (const struct nk_command_polygon_filled *)cmd;
					int i;

					cairo_move_to(cairo, p->points[0].x, p->points[0].y);
					for(i=1; i<p->point_count; p++)
						cairo_line_to(cairo, p->points[i].x, p->points[i].y);
					cairo_close_path(cairo);
					nk_cairo_fill(cairo, p->color);
			} break;
			case NK_COMMAND_POLYLINE: {
					const struct nk_command_polyline *p = (const struct nk_command_polyline *)cmd;
					int i;

					cairo_move_to(cairo, p->points[0].x, p->points[0].y);
					for(i=1; i<p->point_count; p++)
						cairo_line_to(cairo, p->points[i].x, p->points[i].y);
					nk_cairo_stroke(cairo, p->line_thickness, p->color);
			} break;
			case NK_COMMAND_TEXT: {
					const struct nk_command_text *t = (const struct nk_command_text*)cmd;

					cairo_rectangle(cairo, t->x, t->y, t->w, t->height);
					nk_cairo_fill(cairo, t->background);
					/* draw bounding box of text
					cairo_rectangle(cairo, t->x, t->y, t->w, t->height);
					nk_cairo_stroke(cairo, 1, nk_rgb(0xff, 0, 0));
					*/

					cairo_move_to(cairo, t->x, t->y);
					cairo_rel_move_to(cairo, 0, t->height - 2); /* TODO why do we need the -2 here? */
					nk_cairo_set_color(cairo, t->foreground);
					cairo_show_text(cairo, t->string);
			} break;
			case NK_COMMAND_IMAGE: {
					const struct nk_command_image *i = (const struct nk_command_image*)cmd;
					const struct nk_image *img = &i->img;
					cairo_surface_t *surface = img->handle.ptr;

					if(surface)
					{
						const double scale_x = (double)i->w / img->region[2];
						const double scale_y = (double)i->h / img->region[3];
						cairo_save(cairo);
						cairo_translate(cairo, i->x, i->y);
						cairo_scale(cairo, scale_x, scale_y);
						cairo_set_source_surface(cairo, surface, img->region[0], img->region[1]);
						cairo_paint(cairo);
						cairo_restore(cairo);
					}
			} break;
		}

		if( (cmd->type != NK_COMMAND_SCISSOR) && (is_clipped > 0))
		{
			is_clipped -= 1;
			cairo_restore(cairo);
		}
	}

	nk_clear(ctx);
	cairo_restore(cairo);
}

NK_API void
nk_cairo_shutdown(struct nk_context *ctx)
{
	nk_free(ctx);
}

NK_API struct nk_image
nk_cairo_png_load(const char *filename)
{
	cairo_surface_t *surface = cairo_image_surface_create_from_png(filename);

	if(surface)
	{
		struct nk_image img = nk_image_ptr(surface);
		img.w = cairo_image_surface_get_width(surface);
		img.h = cairo_image_surface_get_width(surface);
		img.region[0] = 0;
		img.region[1] = 0;
		img.region[2] = img.w;
		img.region[3] = img.h;

		return img;
	}

	return nk_image_ptr(NULL);
}

NK_API void
nk_cairo_png_unload(struct nk_image image)
{
	cairo_surface_t *surface = image.handle.ptr;

	if(surface)
		cairo_surface_destroy(surface);
}

#endif
