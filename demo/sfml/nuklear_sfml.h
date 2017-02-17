/*
 * Nuklear - 1.32.0 - public domain
 * no warrenty implied; use at your own risk.
 * authored from 2015-2016 by Micha Mettke
 */
/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */
#ifndef NK_SFML_H_
#define NK_SFML_H_

#include <string>
#include <SFML/Graphics.hpp>

NK_API struct nk_context*	nk_sfml_init(NkSFMLFont* sfmlfont, sf::RenderWindow* window, sf::View view);
NK_API int 					nk_sfml_handle_event(sf::Event* event);
NK_API void 				nk_sfml_render(enum nk_anti_aliasing, int max_vertex_buffer, int max_element_buffer);
NK_API void 				nk_sfml_shutdown(void);

typedef struct NkSFMLFont;
NK_API NkSFMLFont*			nk_sfml_font_create(const std::string& file_name, int font_size, int flags);
NK_API void					nk_sfml_font_delete(NkSFMLFont);
NK_API void					nk_sfml_font_set(NkSFMLFont);

#endif
/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
 #ifdef NK_SFML_IMPLEMENTATION

sf::Shape& RoundedRectangle(float rectWidth, float rectHeight, float radius)
{
	sf::Shape round_rect;
	rect->SetOutlineWidth(Outline);

	float a = 0.0f;
	float b = 0.0f;

	for(int i=0; i<POINTS; i++)
	{
		X+=radius/POINTS;
		Y=sqrt(radius*radius-X*X);
		rrect->AddPoint(X+x+rectWidth-radius,y-Y+radius,Col,OutlineCol);
	}
	Y=0;
	for(int i=0; i<POINTS; i++)
	{
		Y+=radius/POINTS;
		X=sqrt(radius*radius-Y*Y);
		rrect->AddPoint(x+rectWidth+X-radius,y+rectHeight-radius+Y,Col,OutlineCol);
	}
	X=0;
	for(int i=0; i<POINTS; i++)
	{
		X+=radius/POINTS;
		Y=sqrt(radius*radius-X*X);
		rrect->AddPoint(x+radius-X,y+rectHeight-radius+Y,Col,OutlineCol);
	}
	Y=0;
	for(int i=0; i<POINTS; i++)
	{
		Y+=radius/POINTS;
		X=sqrt(radius*radius-Y*Y);
		rrect->AddPoint(x-X+radius,y+radius-Y,Col,OutlineCol);
	}
	return *rrect;
	
}

struct NkSFMLFont
{
	struct nk_user_font nk;
	int height;
	sf::Font font;
};

static struct nk_sfml
{
	sf::RenderWindow* window;
	sf::View view;
	struct nk_context ctx;
	struct nk_buffer cmds;
} sfml;

NK_API NkSFMLFont*
nk_sfml_font_create(const std::string& file_name)
{
	NkSFMLFont* font = (NkSFMLFont*)calloc(1, sizeof(NkSFMLFont));
	
	if(!font->font.loadFromFile(file_name))
	{
		fprintf(stdout, "Unable to load font file: %s\n", file_name);
		return NULL;
	}

	// I really need to think of a better way of doing this
	sf::Text text("abjdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789", font->font);
	font->height = text.getGlobalBounds().height;

	return font;
}

static float
nk_sfml_font_get_text_width(nk_handle handle, float height, const char* string, int len)
{
	NkSFMLFont* font = (NkSFMLFont*)handle.ptr;

	if(!font || !string)
		return 0;

	sf::Text sfmltext(font->font, string);

	//char strcpy[len + 1];
	//strncpy((char*)&strcpy, text, len);
	//strcpy[len] = '\0';

	return sfmltext.getGlobalBounds().width;
}

NK_API void
nk_sfml_font_set(NkSFMLFont* sfmlfont)
{
	struct nk_user_font* font = &sfmlfont->nk;
	font->userdata = nk_handle_ptr(sfmlfont);
	font->height = (float)sfmlfont->height;
	font->width = nk_sfml_font_get_text_width;
	nk_style_set_font(&sfml.ctx, font);
}

NK_API void
nk_sfml_font_del(NkSFMLFont* font)
{
	if(!font)
		return;

	free(font);
}

static sf::Color
nk_color_to_sfml(struct nk_color color)
{
	return sf::Color(color.r, color.g, color.b, color.a);
}

NK_API void
nk_sfml_render(enum nk_anti_aliasing AA, int max_vertex_buffer, int max_element_buffer)
{
    const struct nk_command *cmd;



    nk_foreach(cmd, &sfml.ctx)
    {
        sf::Color color;

        switch (cmd->type)
        {
        	case NK_COMMAND_NOP: break;
        	case NK_COMMAND_SCISSOR:
        	{
            	const struct nk_command_scissor *s = (const struct nk_command_scissor*)cmd;
            	
            	
            	//sf::RenderTexture clip_tex((unsigned int)s->x, (unsigned int)s->y);
            	//sf::Sprite clip;
            	sf::View view;
            	view.setCenter(s->x, s->y);
            	view.setSize(s->w, s->h);
            	sfml.window->setView(view);
            	//al_set_clipping_rectangle((int)s->x, (int)s->y, (int)s->w, (int)s->h);
        	} break;
        	case NK_COMMAND_LINE:
        	{
            	const struct nk_command_line *l = (const struct nk_command_line *)cmd;
            	
            	color = nk_color_to_sfml(l->color);

            	sf::Vertex line[] = 
            	{
            		sf::Vertex(sf::Vector2f((float)l->begin.x - l->line_thickness, (float)l->begin.y + l->line_thickness), color),
            		sf::Vertex(sf::Vector2f((float)l->begin.x + l->line_thickness, (float)l->begin.y - l->line_thickness), color),
            		sf::Vertex(sf::Vector2f((float)l->end.x + l->line_thickness, (float)l->end.y - l->line_thickness), color),
            		sf::Vertex(sf::Vector2f((float)l->end.x - l->line_thickness, (float)l->end.y + l->line_thickness), color)
            	};

            	sfml.window->draw(line, sf::Quads);
        	} break;
        	case NK_COMMAND_RECT:
        	{
            	const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
            	
            	color = nk_color_to_sfml(l->color);

            	sf::RectangleShape rect;
            	rect.setSize(sf::Vector2f(()))

            	sfml.window->draw(line, sf::Quads);


            	color = nk_color_to_sfml(r->color);
            	al_draw_rounded_rectangle(
            		(float)r->x, (float)r->y, (float)(r->x + r->w),
                	(float)(r->y + r->h), (float)r->rounding, (float)r->rounding, color,
                	(float)r->line_thickness);
        	} break;
        	case NK_COMMAND_RECT_FILLED:
        	{
            	const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled *)cmd;
            	
            	color = nk_color_to_sfml(r->color);
            	al_draw_filled_rounded_rectangle(
            		(float)r->x, (float)r->y,
                	(float)(r->x + r->w), (float)(r->y + r->h), (float)r->rounding,
                	(float)r->rounding, color);
        	} break;
        	case NK_COMMAND_CIRCLE:
        	{
            	const struct nk_command_circle *c = (const struct nk_command_circle *)cmd;
            	
            	color = nk_color_to_sfml(c->color);
            	float xr, yr;
            	xr = (float)c->w/2;
            	yr = (float)c->h/2;
            	al_draw_ellipse(
            		((float)(c->x)) + xr, ((float)c->y) + yr,
                	xr, yr, color, (float)c->line_thickness);
        	} break;
        	case NK_COMMAND_CIRCLE_FILLED:
        	{
            	const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;
            	
            	color = nk_color_to_sfml(c->color);
            	float xr, yr;
            	xr = (float)c->w/2;
            	yr = (float)c->h/2;
            	al_draw_filled_ellipse(
            		((float)(c->x)) + xr, ((float)c->y) + yr,
                	xr, yr, color);
        	} break;
        	case NK_COMMAND_TRIANGLE:
        	{
            	const struct nk_command_triangle*t = (const struct nk_command_triangle*)cmd;
            	
            	color = nk_color_to_sfml(t->color);
            	al_draw_triangle(
            		(float)t->a.x, (float)t->a.y, (float)t->b.x, (float)t->b.y,
                	(float)t->c.x, (float)t->c.y, color, (float)t->line_thickness);
        	} break;
        	case NK_COMMAND_TRIANGLE_FILLED:
        	{
            	const struct nk_command_triangle_filled *t = (const struct nk_command_triangle_filled *)cmd;
            	
            	color = nk_color_to_sfml(t->color);
            	al_draw_filled_triangle(
            		(float)t->a.x, (float)t->a.y, (float)t->b.x,
                	(float)t->b.y, (float)t->c.x, (float)t->c.y, color);
        	} break;
        	case NK_COMMAND_POLYGON:
        	{
            	const struct nk_command_polygon *p = (const struct nk_command_polygon*)cmd;
            	
            	color = nk_color_to_sfml(p->color);
            	int i;
            	float vertices[p->point_count * 2];
            	
            	for (i = 0; i < p->point_count; i++)
            	{
                	vertices[i*2] = p->points[i].x;
                	vertices[(i*2) + 1] = p->points[i].y;
            	}
            	
            	al_draw_polyline(
            		(const float*)&vertices, (2 * sizeof(float)),
                	(int)p->point_count, ALLEGRO_LINE_JOIN_ROUND, ALLEGRO_LINE_CAP_CLOSED,
                	color, (float)p->line_thickness, 0.0);
        	} break;
        	case NK_COMMAND_POLYGON_FILLED:
        	{
            	const struct nk_command_polygon_filled *p = (const struct nk_command_polygon_filled *)cmd;
            	
            	color = nk_color_to_sfml(p->color);
            	int i;
            	float vertices[p->point_count * 2];
            	
            	for (i = 0; i < p->point_count; i++)
            	{
                	vertices[i*2] = p->points[i].x;
                	vertices[(i*2) + 1] = p->points[i].y;
            	}
            	
            	al_draw_filled_polygon((const float*)&vertices, (int)p->point_count, color);
        	} break;
        	case NK_COMMAND_POLYLINE:
        	{
            	const struct nk_command_polyline *p = (const struct nk_command_polyline *)cmd;
            
            	color = nk_color_to_sfml(p->color);
            	int i;
            	float vertices[p->point_count * 2];
            
            	for (i = 0; i < p->point_count; i++)
            	{
                	vertices[i*2] = p->points[i].x;
                	vertices[(i*2) + 1] = p->points[i].y;
            	}
            	
            	al_draw_polyline(
            		(const float*)&vertices, (2 * sizeof(float)),
                	(int)p->point_count, ALLEGRO_LINE_JOIN_ROUND, ALLEGRO_LINE_CAP_ROUND,
        	        color, (float)p->line_thickness, 0.0);
        	} break;
        	case NK_COMMAND_TEXT:
        	{
	            const struct nk_command_text *t = (const struct nk_command_text*)cmd;
            	
            	color = nk_color_to_sfml(t->foreground);
            	NkAllegro5Font *font = (NkAllegro5Font*)t->font->userdata.ptr;
            	
            	al_draw_text(
            		font->font,
                	color, (float)t->x, (float)t->y, 0,
                	(const char*)t->string);
        	} break;
        	case NK_COMMAND_CURVE:
        	{
            	const struct nk_command_curve *q = (const struct nk_command_curve *)cmd;
            	
            	color = nk_color_to_sfml(q->color);
            	float points[8];
            	points[0] = (float)q->begin.x;
            	points[1] = (float)q->begin.y;
            	points[2] = (float)q->ctrl[0].x;
            	points[3] = (float)q->ctrl[0].y;
            	points[4] = (float)q->ctrl[1].x;
            	points[5] = (float)q->ctrl[1].y;
            	points[6] = (float)q->end.x;
            	points[7] = (float)q->end.y;
            	al_draw_spline(points, color, (float)q->line_thickness);
        	} break;
        	case NK_COMMAND_ARC:
        	{
            	const struct nk_command_arc *a = (const struct nk_command_arc *)cmd;
            	
            	color = nk_color_to_sfml(a->color);
            	al_draw_arc(
            		(float)a->cx, (float)a->cy, (float)a->r, a->a[0],
                	a->a[1], color, (float)a->line_thickness);
        	} break;
        	case NK_COMMAND_RECT_MULTI_COLOR:
        	case NK_COMMAND_IMAGE:
        	case NK_COMMAND_ARC_FILLED:
        	default: break;
        }
    }
    nk_clear(&allegro5.ctx);
}

static void
nk_sfml_clipboard_paste(nk_handle usr, struct nk_text_edit* edit)
{
	/* Not Implemented in SFML 
	sf::Clipboard clipboard(sfml.window);
	const char* text = clipboard.getText();

	if(text)
		nk_textedit_paste(edit, text, nk_strlen(text));
		(void)usr;
	*/
}

static void
nk_sfml_clipboard_copy(nk_handle usr, const char* text, int len)
{
	char* str = 0;
	(void)usr;
	if(!len)
		return;
	str = (char*)malloc((size_t)len+1);
	if(!str)
		return;
	memcpy(str, text, (size_t)len);
	str[len] = '\0';

	/* Not Implemented in SFML
	sf::Clipboard clipboard(sfml.window);
	clipboard.setText(str);
	*/

	free(str);
}

NK_API struct nk_context*
nk_sfml_init(NkSFMLFont* sfmlfont, sf::RenderWindow* window, sf::View view)
{
	struct nk_user_font* font = &sfmlfont->nk;
	font->userdata = nk_handle_ptr(sfmlfont);
	font->height = (float)sfmlfont->height;
	font->width = nk_sfml_get_text_width;

	sfml.window = window;
	sfml.view = view;

	nk_init_default(&sfml.ctx, font);
	sfml.ctx.clip.copy = nk_sfml_clipboard_copy;
	sfml.ctx.clip.paste = nk_sfml_clipboard_paste;
	sfml.ctx.clip.userdata = nk_handle_ptr(0);
	return &sfml.ctx;
}

NK_API int
nk_sfml_handle_event(sf::Event* event)
{
	struct nk_context* ctx = &sfml.ctx;

	/* optional grabbing behavior */
	if(ctx->input.mouse.grab)
	{
		sfml.window->setMouseCursorGrabbed(true);
		ctx->input.mouse.grab = 0;
	}
	else if(ctx->input.mouse.ungrab)
	{
		int x = (int)ctx->input.mouse.prev.x;
		int y = (int)ctx->input.mouse.prev.y;
		sfml.window->setMouseCursorGrabbed(false);
		sf::Mouse::setPosition(sf::Vector2i(x, y));
		ctx->input.mouse.ungrab = 0;
	}

	if(event->type == sf::Event::KeyReleased || event->type == sf::Event::KeyPressed)
	{
		int down = event->type == sf::Event::KeyPressed;
		sf::Keyboard::Key key = event->key.code;

		if(key == sf::Keyboard::RShift || key == sf::Keyboard::LShift)
			nk_input_key(ctx, NK_KEY_SHIFT, down);
		else if(key == sf::Keyboard::Delete)
			nk_input_key(ctx, NK_KEY_DEL, down);
		else if(key == sf::Keyboard::Return)
			nk_input_key(ctx, NK_KEY_ENTER, down);
		else if(key == sf::Keyboard::Tab)
			nk_input_key(ctx, NK_KEY_TAB, down);
		else if(key == sf::Keyboard::BackSpace)
			nk_input_key(ctx, NK_KEY_BACKSPACE, down);
		else if(key == sf::Keyboard::Home)
		{
			nk_input_key(ctx, NK_KEY_TEXT_START, down);
			nk_input_key(ctx, NK_KEY_SCROLL_START, down);
		}
		else if(key == sf::Keyboard::End)
		{
			nk_input_key(ctx, NK_KEY_TEXT_END, down);
			nk_input_key(ctx, NK_KEY_SCROLL_END, down);
		}
		else if(key == sf::Keyboard::PageDown)
			nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
		else if(key == sf::Keyboard::PageUp)
			nk_input_key(ctx, NK_KEY_SCROLL_DOWN, down);
		else if(key == sf::Keyboard::Z)
			nk_input_key(ctx, NK_KEY_TEXT_UNDO, down && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl));
		else if(key == sf::Keyboard::R)
			nk_input_key(ctx, NK_KEY_TEXT_REDO, down && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl));
		else if(key == sf::Keyboard::C)
			nk_input_key(ctx, NK_KEY_COPY, down && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl));
		else if(key == sf::Keyboard::V)
			nk_input_key(ctx, NK_KEY_PASTE, down && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl));
		else if(key == sf::Keyboard::X)
			nk_input_key(ctx, NK_KEY_CUT, down && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl));
		else if(key == sf::Keyboard::B)
			nk_input_key(ctx, NK_KEY_TEXT_LINE_START, down && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl));
		else if(key == sf::Keyboard::E)
			nk_input_key(ctx, NK_KEY_TEXT_LINE_END, down && sf::Keyboard::isKeyPressed(sf::Keyboard::LControl));
		else if(key == sf::Keyboard::Up)
			nk_input_key(ctx, NK_KEY_UP, down);
		else if(key == sf::Keyboard::Down)
			nk_input_key(ctx, NK_KEY_DOWN, down);
		else if(key == sf::Keyboard::Left)
		{
			if(sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
				nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, down);
			else
				nk_input_key(ctx, NK_KEY_LEFT, down);
		}
		else if(key == sf::Keyboard::Right)
		{
			if(sf::Keyboard::isKeyPressed(sf::Keyboard::LControl))
				nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, down);
			else
				nk_input_key(ctx, NK_KEY_RIGHT, down);
		}
		else return 0;

		return 1;
	}
	else if(event->type == sf::Event::MouseButtonPressed || event->type == sf::Event::MouseButtonReleased)
	{
		int down = event->type == sf::Event::MouseButtonPressed;
		const int x = event->mouseButton.x;
		const int y = event->mouseButton.y;

		if(event->mouseButton.button == sf::Mouse::Left)
			nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);
		if(event->mouseButton.button == sf::Mouse::Middle)
			nk_input_button(ctx, NK_BUTTON_MIDDLE, x, y, down);
		if(event->mouseButton.button == sf::Mouse::Right)
			nk_input_button(ctx, NK_BUTTON_RIGHT, x, y, down);
		else
			return 0;

		return 1;
	}
	else if(event->type == sf::Event::MouseMoved)
	{
		if(ctx->input.mouse.grabbed)
		{
			int x = (int)ctx->input.mouse.prev.x + event->mouseMove.x;
			int y = (int)ctx->input.mouse.prev.y + event->mouseMove.y;

			nk_input_motion(ctx, x, y);
		}
		else
			nk_input_motion(ctx, event->mouseMove.x, event->mouseMove.y);

		return 1;
	}
	/* For Android*/
	else if(event->type == sf::Event::TouchBegan || event->type == sf::Event::TouchEnded)
	{
		int down = event->type == sf::Event::TouchBegan;
		const int x = event->touch.x;
		const int y = event->touch.y;

		nk_input_button(ctx, NK_BUTTON_LEFT, x, y, down);

		return 1;
	}
	else if(event->type == sf::Event::TouchMoved)
	{
		if(ctx->input.mouse.grabbed)
		{
			int x = (int)ctx->input.mouse.prev.x;
			int y = (int)ctx->input.mouse.prev.y;

			nk_input_motion(ctx, x + event->touch.x, y + event->touch.y);
		}
		else
			nk_input_motion(ctx, event->touch.x, event->touch.y);

		return 1;
	}
	else if(event->type == sf::Event::TextEntered)
	{
		nk_input_unicode(ctx, event->text.unicode);

		return 1;
	}
	else if(event->type == sf::Event::MouseWheelScrolled)
	{
		nk_input_scroll(ctx, event->mouseWheelScroll.delta);

		return 1;
	}

	return 0;
}

NK_API
void nk_sfml_shutdown(void)
{
	nk_free(&sfml.ctx);
	memset(&sfml, 0, sizeof(sfml));
}

#endif
