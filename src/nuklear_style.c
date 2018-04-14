#include "nuklear.h"
#include "nuklear_internal.h"

/* ===============================================================
 *
 *                              STYLE
 *
 * ===============================================================*/
NK_API void nk_style_default(struct nk_context *ctx){nk_style_from_table(ctx, 0);}
#define NK_COLOR_MAP(NK_COLOR)\
    NK_COLOR(NK_COLOR_TEXT,                     175,175,175,255) \
    NK_COLOR(NK_COLOR_WINDOW,                   45, 45, 45, 255) \
    NK_COLOR(NK_COLOR_HEADER,                   40, 40, 40, 255) \
    NK_COLOR(NK_COLOR_BORDER,                   65, 65, 65, 255) \
    NK_COLOR(NK_COLOR_BUTTON,                   50, 50, 50, 255) \
    NK_COLOR(NK_COLOR_BUTTON_HOVER,             40, 40, 40, 255) \
    NK_COLOR(NK_COLOR_BUTTON_ACTIVE,            35, 35, 35, 255) \
    NK_COLOR(NK_COLOR_TOGGLE,                   100,100,100,255) \
    NK_COLOR(NK_COLOR_TOGGLE_HOVER,             120,120,120,255) \
    NK_COLOR(NK_COLOR_TOGGLE_CURSOR,            45, 45, 45, 255) \
    NK_COLOR(NK_COLOR_SELECT,                   45, 45, 45, 255) \
    NK_COLOR(NK_COLOR_SELECT_ACTIVE,            35, 35, 35,255) \
    NK_COLOR(NK_COLOR_SLIDER,                   38, 38, 38, 255) \
    NK_COLOR(NK_COLOR_SLIDER_CURSOR,            100,100,100,255) \
    NK_COLOR(NK_COLOR_SLIDER_CURSOR_HOVER,      120,120,120,255) \
    NK_COLOR(NK_COLOR_SLIDER_CURSOR_ACTIVE,     150,150,150,255) \
    NK_COLOR(NK_COLOR_PROPERTY,                 38, 38, 38, 255) \
    NK_COLOR(NK_COLOR_EDIT,                     38, 38, 38, 255)  \
    NK_COLOR(NK_COLOR_EDIT_CURSOR,              175,175,175,255) \
    NK_COLOR(NK_COLOR_COMBO,                    45, 45, 45, 255) \
    NK_COLOR(NK_COLOR_CHART,                    120,120,120,255) \
    NK_COLOR(NK_COLOR_CHART_COLOR,              45, 45, 45, 255) \
    NK_COLOR(NK_COLOR_CHART_COLOR_HIGHLIGHT,    255, 0,  0, 255) \
    NK_COLOR(NK_COLOR_SCROLLBAR,                40, 40, 40, 255) \
    NK_COLOR(NK_COLOR_SCROLLBAR_CURSOR,         100,100,100,255) \
    NK_COLOR(NK_COLOR_SCROLLBAR_CURSOR_HOVER,   120,120,120,255) \
    NK_COLOR(NK_COLOR_SCROLLBAR_CURSOR_ACTIVE,  150,150,150,255) \
    NK_COLOR(NK_COLOR_TAB_HEADER,               40, 40, 40,255)

NK_GLOBAL const struct nk_color
nk_default_color_style[NK_COLOR_COUNT] = {
#define NK_COLOR(a,b,c,d,e) {b,c,d,e},
    NK_COLOR_MAP(NK_COLOR)
#undef NK_COLOR
};
NK_GLOBAL const char *nk_color_names[NK_COLOR_COUNT] = {
#define NK_COLOR(a,b,c,d,e) #a,
    NK_COLOR_MAP(NK_COLOR)
#undef NK_COLOR
};

NK_API const char*
nk_style_get_color_by_name(enum nk_style_colors c)
{
    return nk_color_names[c];
}
NK_API struct nk_style_item
nk_style_item_image(struct nk_image img)
{
    struct nk_style_item i;
    i.type = NK_STYLE_ITEM_IMAGE;
    i.data.image = img;
    return i;
}
NK_API struct nk_style_item
nk_style_item_color(struct nk_color col)
{
    struct nk_style_item i;
    i.type = NK_STYLE_ITEM_COLOR;
    i.data.color = col;
    return i;
}
NK_API struct nk_style_item
nk_style_item_hide(void)
{
    struct nk_style_item i;
    i.type = NK_STYLE_ITEM_COLOR;
    i.data.color = nk_rgba(0,0,0,0);
    return i;
}
NK_API void
nk_style_from_table(struct nk_context *ctx, const struct nk_color *table)
{
    struct nk_style *style;
    struct nk_style_text *text;
    struct nk_style_button *button;
    struct nk_style_toggle *toggle;
    struct nk_style_selectable *select;
    struct nk_style_slider *slider;
    struct nk_style_progress *prog;
    struct nk_style_scrollbar *scroll;
    struct nk_style_edit *edit;
    struct nk_style_property *property;
    struct nk_style_combo *combo;
    struct nk_style_chart *chart;
    struct nk_style_tab *tab;
    struct nk_style_window *win;

    NK_ASSERT(ctx);
    if (!ctx) return;
    style = &ctx->style;
    table = (!table) ? nk_default_color_style: table;

    /* default text */
    text = &style->text;
    text->color = table[NK_COLOR_TEXT];
    text->padding = nk_vec2(0,0);

    /* default button */
    button = &style->button;
    nk_zero_struct(*button);
    button->normal          = nk_style_item_color(table[NK_COLOR_BUTTON]);
    button->hover           = nk_style_item_color(table[NK_COLOR_BUTTON_HOVER]);
    button->active          = nk_style_item_color(table[NK_COLOR_BUTTON_ACTIVE]);
    button->border_color    = table[NK_COLOR_BORDER];
    button->text_background = table[NK_COLOR_BUTTON];
    button->text_normal     = table[NK_COLOR_TEXT];
    button->text_hover      = table[NK_COLOR_TEXT];
    button->text_active     = table[NK_COLOR_TEXT];
    button->padding         = nk_vec2(2.0f,2.0f);
    button->image_padding   = nk_vec2(0.0f,0.0f);
    button->touch_padding   = nk_vec2(0.0f, 0.0f);
    button->userdata        = nk_handle_ptr(0);
    button->text_alignment  = NK_TEXT_CENTERED;
    button->border          = 1.0f;
    button->rounding        = 4.0f;
    button->draw_begin      = 0;
    button->draw_end        = 0;

    /* contextual button */
    button = &style->contextual_button;
    nk_zero_struct(*button);
    button->normal          = nk_style_item_color(table[NK_COLOR_WINDOW]);
    button->hover           = nk_style_item_color(table[NK_COLOR_BUTTON_HOVER]);
    button->active          = nk_style_item_color(table[NK_COLOR_BUTTON_ACTIVE]);
    button->border_color    = table[NK_COLOR_WINDOW];
    button->text_background = table[NK_COLOR_WINDOW];
    button->text_normal     = table[NK_COLOR_TEXT];
    button->text_hover      = table[NK_COLOR_TEXT];
    button->text_active     = table[NK_COLOR_TEXT];
    button->padding         = nk_vec2(2.0f,2.0f);
    button->touch_padding   = nk_vec2(0.0f,0.0f);
    button->userdata        = nk_handle_ptr(0);
    button->text_alignment  = NK_TEXT_CENTERED;
    button->border          = 0.0f;
    button->rounding        = 0.0f;
    button->draw_begin      = 0;
    button->draw_end        = 0;

    /* menu button */
    button = &style->menu_button;
    nk_zero_struct(*button);
    button->normal          = nk_style_item_color(table[NK_COLOR_WINDOW]);
    button->hover           = nk_style_item_color(table[NK_COLOR_WINDOW]);
    button->active          = nk_style_item_color(table[NK_COLOR_WINDOW]);
    button->border_color    = table[NK_COLOR_WINDOW];
    button->text_background = table[NK_COLOR_WINDOW];
    button->text_normal     = table[NK_COLOR_TEXT];
    button->text_hover      = table[NK_COLOR_TEXT];
    button->text_active     = table[NK_COLOR_TEXT];
    button->padding         = nk_vec2(2.0f,2.0f);
    button->touch_padding   = nk_vec2(0.0f,0.0f);
    button->userdata        = nk_handle_ptr(0);
    button->text_alignment  = NK_TEXT_CENTERED;
    button->border          = 0.0f;
    button->rounding        = 1.0f;
    button->draw_begin      = 0;
    button->draw_end        = 0;

    /* checkbox toggle */
    toggle = &style->checkbox;
    nk_zero_struct(*toggle);
    toggle->normal          = nk_style_item_color(table[NK_COLOR_TOGGLE]);
    toggle->hover           = nk_style_item_color(table[NK_COLOR_TOGGLE_HOVER]);
    toggle->active          = nk_style_item_color(table[NK_COLOR_TOGGLE_HOVER]);
    toggle->cursor_normal   = nk_style_item_color(table[NK_COLOR_TOGGLE_CURSOR]);
    toggle->cursor_hover    = nk_style_item_color(table[NK_COLOR_TOGGLE_CURSOR]);
    toggle->userdata        = nk_handle_ptr(0);
    toggle->text_background = table[NK_COLOR_WINDOW];
    toggle->text_normal     = table[NK_COLOR_TEXT];
    toggle->text_hover      = table[NK_COLOR_TEXT];
    toggle->text_active     = table[NK_COLOR_TEXT];
    toggle->padding         = nk_vec2(2.0f, 2.0f);
    toggle->touch_padding   = nk_vec2(0,0);
    toggle->border_color    = nk_rgba(0,0,0,0);
    toggle->border          = 0.0f;
    toggle->spacing         = 4;

    /* option toggle */
    toggle = &style->option;
    nk_zero_struct(*toggle);
    toggle->normal          = nk_style_item_color(table[NK_COLOR_TOGGLE]);
    toggle->hover           = nk_style_item_color(table[NK_COLOR_TOGGLE_HOVER]);
    toggle->active          = nk_style_item_color(table[NK_COLOR_TOGGLE_HOVER]);
    toggle->cursor_normal   = nk_style_item_color(table[NK_COLOR_TOGGLE_CURSOR]);
    toggle->cursor_hover    = nk_style_item_color(table[NK_COLOR_TOGGLE_CURSOR]);
    toggle->userdata        = nk_handle_ptr(0);
    toggle->text_background = table[NK_COLOR_WINDOW];
    toggle->text_normal     = table[NK_COLOR_TEXT];
    toggle->text_hover      = table[NK_COLOR_TEXT];
    toggle->text_active     = table[NK_COLOR_TEXT];
    toggle->padding         = nk_vec2(3.0f, 3.0f);
    toggle->touch_padding   = nk_vec2(0,0);
    toggle->border_color    = nk_rgba(0,0,0,0);
    toggle->border          = 0.0f;
    toggle->spacing         = 4;

    /* selectable */
    select = &style->selectable;
    nk_zero_struct(*select);
    select->normal          = nk_style_item_color(table[NK_COLOR_SELECT]);
    select->hover           = nk_style_item_color(table[NK_COLOR_SELECT]);
    select->pressed         = nk_style_item_color(table[NK_COLOR_SELECT]);
    select->normal_active   = nk_style_item_color(table[NK_COLOR_SELECT_ACTIVE]);
    select->hover_active    = nk_style_item_color(table[NK_COLOR_SELECT_ACTIVE]);
    select->pressed_active  = nk_style_item_color(table[NK_COLOR_SELECT_ACTIVE]);
    select->text_normal     = table[NK_COLOR_TEXT];
    select->text_hover      = table[NK_COLOR_TEXT];
    select->text_pressed    = table[NK_COLOR_TEXT];
    select->text_normal_active  = table[NK_COLOR_TEXT];
    select->text_hover_active   = table[NK_COLOR_TEXT];
    select->text_pressed_active = table[NK_COLOR_TEXT];
    select->padding         = nk_vec2(2.0f,2.0f);
    select->image_padding   = nk_vec2(2.0f,2.0f);
    select->touch_padding   = nk_vec2(0,0);
    select->userdata        = nk_handle_ptr(0);
    select->rounding        = 0.0f;
    select->draw_begin      = 0;
    select->draw_end        = 0;

    /* slider */
    slider = &style->slider;
    nk_zero_struct(*slider);
    slider->normal          = nk_style_item_hide();
    slider->hover           = nk_style_item_hide();
    slider->active          = nk_style_item_hide();
    slider->bar_normal      = table[NK_COLOR_SLIDER];
    slider->bar_hover       = table[NK_COLOR_SLIDER];
    slider->bar_active      = table[NK_COLOR_SLIDER];
    slider->bar_filled      = table[NK_COLOR_SLIDER_CURSOR];
    slider->cursor_normal   = nk_style_item_color(table[NK_COLOR_SLIDER_CURSOR]);
    slider->cursor_hover    = nk_style_item_color(table[NK_COLOR_SLIDER_CURSOR_HOVER]);
    slider->cursor_active   = nk_style_item_color(table[NK_COLOR_SLIDER_CURSOR_ACTIVE]);
    slider->inc_symbol      = NK_SYMBOL_TRIANGLE_RIGHT;
    slider->dec_symbol      = NK_SYMBOL_TRIANGLE_LEFT;
    slider->cursor_size     = nk_vec2(16,16);
    slider->padding         = nk_vec2(2,2);
    slider->spacing         = nk_vec2(2,2);
    slider->userdata        = nk_handle_ptr(0);
    slider->show_buttons    = nk_false;
    slider->bar_height      = 8;
    slider->rounding        = 0;
    slider->draw_begin      = 0;
    slider->draw_end        = 0;

    /* slider buttons */
    button = &style->slider.inc_button;
    button->normal          = nk_style_item_color(nk_rgb(40,40,40));
    button->hover           = nk_style_item_color(nk_rgb(42,42,42));
    button->active          = nk_style_item_color(nk_rgb(44,44,44));
    button->border_color    = nk_rgb(65,65,65);
    button->text_background = nk_rgb(40,40,40);
    button->text_normal     = nk_rgb(175,175,175);
    button->text_hover      = nk_rgb(175,175,175);
    button->text_active     = nk_rgb(175,175,175);
    button->padding         = nk_vec2(8.0f,8.0f);
    button->touch_padding   = nk_vec2(0.0f,0.0f);
    button->userdata        = nk_handle_ptr(0);
    button->text_alignment  = NK_TEXT_CENTERED;
    button->border          = 1.0f;
    button->rounding        = 0.0f;
    button->draw_begin      = 0;
    button->draw_end        = 0;
    style->slider.dec_button = style->slider.inc_button;

    /* progressbar */
    prog = &style->progress;
    nk_zero_struct(*prog);
    prog->normal            = nk_style_item_color(table[NK_COLOR_SLIDER]);
    prog->hover             = nk_style_item_color(table[NK_COLOR_SLIDER]);
    prog->active            = nk_style_item_color(table[NK_COLOR_SLIDER]);
    prog->cursor_normal     = nk_style_item_color(table[NK_COLOR_SLIDER_CURSOR]);
    prog->cursor_hover      = nk_style_item_color(table[NK_COLOR_SLIDER_CURSOR_HOVER]);
    prog->cursor_active     = nk_style_item_color(table[NK_COLOR_SLIDER_CURSOR_ACTIVE]);
    prog->border_color      = nk_rgba(0,0,0,0);
    prog->cursor_border_color = nk_rgba(0,0,0,0);
    prog->userdata          = nk_handle_ptr(0);
    prog->padding           = nk_vec2(4,4);
    prog->rounding          = 0;
    prog->border            = 0;
    prog->cursor_rounding   = 0;
    prog->cursor_border     = 0;
    prog->draw_begin        = 0;
    prog->draw_end          = 0;

    /* scrollbars */
    scroll = &style->scrollh;
    nk_zero_struct(*scroll);
    scroll->normal          = nk_style_item_color(table[NK_COLOR_SCROLLBAR]);
    scroll->hover           = nk_style_item_color(table[NK_COLOR_SCROLLBAR]);
    scroll->active          = nk_style_item_color(table[NK_COLOR_SCROLLBAR]);
    scroll->cursor_normal   = nk_style_item_color(table[NK_COLOR_SCROLLBAR_CURSOR]);
    scroll->cursor_hover    = nk_style_item_color(table[NK_COLOR_SCROLLBAR_CURSOR_HOVER]);
    scroll->cursor_active   = nk_style_item_color(table[NK_COLOR_SCROLLBAR_CURSOR_ACTIVE]);
    scroll->dec_symbol      = NK_SYMBOL_CIRCLE_SOLID;
    scroll->inc_symbol      = NK_SYMBOL_CIRCLE_SOLID;
    scroll->userdata        = nk_handle_ptr(0);
    scroll->border_color    = table[NK_COLOR_SCROLLBAR];
    scroll->cursor_border_color = table[NK_COLOR_SCROLLBAR];
    scroll->padding         = nk_vec2(0,0);
    scroll->show_buttons    = nk_false;
    scroll->border          = 0;
    scroll->rounding        = 0;
    scroll->border_cursor   = 0;
    scroll->rounding_cursor = 0;
    scroll->draw_begin      = 0;
    scroll->draw_end        = 0;
    style->scrollv = style->scrollh;

    /* scrollbars buttons */
    button = &style->scrollh.inc_button;
    button->normal          = nk_style_item_color(nk_rgb(40,40,40));
    button->hover           = nk_style_item_color(nk_rgb(42,42,42));
    button->active          = nk_style_item_color(nk_rgb(44,44,44));
    button->border_color    = nk_rgb(65,65,65);
    button->text_background = nk_rgb(40,40,40);
    button->text_normal     = nk_rgb(175,175,175);
    button->text_hover      = nk_rgb(175,175,175);
    button->text_active     = nk_rgb(175,175,175);
    button->padding         = nk_vec2(4.0f,4.0f);
    button->touch_padding   = nk_vec2(0.0f,0.0f);
    button->userdata        = nk_handle_ptr(0);
    button->text_alignment  = NK_TEXT_CENTERED;
    button->border          = 1.0f;
    button->rounding        = 0.0f;
    button->draw_begin      = 0;
    button->draw_end        = 0;
    style->scrollh.dec_button = style->scrollh.inc_button;
    style->scrollv.inc_button = style->scrollh.inc_button;
    style->scrollv.dec_button = style->scrollh.inc_button;

    /* edit */
    edit = &style->edit;
    nk_zero_struct(*edit);
    edit->normal            = nk_style_item_color(table[NK_COLOR_EDIT]);
    edit->hover             = nk_style_item_color(table[NK_COLOR_EDIT]);
    edit->active            = nk_style_item_color(table[NK_COLOR_EDIT]);
    edit->cursor_normal     = table[NK_COLOR_TEXT];
    edit->cursor_hover      = table[NK_COLOR_TEXT];
    edit->cursor_text_normal= table[NK_COLOR_EDIT];
    edit->cursor_text_hover = table[NK_COLOR_EDIT];
    edit->border_color      = table[NK_COLOR_BORDER];
    edit->text_normal       = table[NK_COLOR_TEXT];
    edit->text_hover        = table[NK_COLOR_TEXT];
    edit->text_active       = table[NK_COLOR_TEXT];
    edit->selected_normal   = table[NK_COLOR_TEXT];
    edit->selected_hover    = table[NK_COLOR_TEXT];
    edit->selected_text_normal  = table[NK_COLOR_EDIT];
    edit->selected_text_hover   = table[NK_COLOR_EDIT];
    edit->scrollbar_size    = nk_vec2(10,10);
    edit->scrollbar         = style->scrollv;
    edit->padding           = nk_vec2(4,4);
    edit->row_padding       = 2;
    edit->cursor_size       = 4;
    edit->border            = 1;
    edit->rounding          = 0;

    /* property */
    property = &style->property;
    nk_zero_struct(*property);
    property->normal        = nk_style_item_color(table[NK_COLOR_PROPERTY]);
    property->hover         = nk_style_item_color(table[NK_COLOR_PROPERTY]);
    property->active        = nk_style_item_color(table[NK_COLOR_PROPERTY]);
    property->border_color  = table[NK_COLOR_BORDER];
    property->label_normal  = table[NK_COLOR_TEXT];
    property->label_hover   = table[NK_COLOR_TEXT];
    property->label_active  = table[NK_COLOR_TEXT];
    property->sym_left      = NK_SYMBOL_TRIANGLE_LEFT;
    property->sym_right     = NK_SYMBOL_TRIANGLE_RIGHT;
    property->userdata      = nk_handle_ptr(0);
    property->padding       = nk_vec2(4,4);
    property->border        = 1;
    property->rounding      = 10;
    property->draw_begin    = 0;
    property->draw_end      = 0;

    /* property buttons */
    button = &style->property.dec_button;
    nk_zero_struct(*button);
    button->normal          = nk_style_item_color(table[NK_COLOR_PROPERTY]);
    button->hover           = nk_style_item_color(table[NK_COLOR_PROPERTY]);
    button->active          = nk_style_item_color(table[NK_COLOR_PROPERTY]);
    button->border_color    = nk_rgba(0,0,0,0);
    button->text_background = table[NK_COLOR_PROPERTY];
    button->text_normal     = table[NK_COLOR_TEXT];
    button->text_hover      = table[NK_COLOR_TEXT];
    button->text_active     = table[NK_COLOR_TEXT];
    button->padding         = nk_vec2(0.0f,0.0f);
    button->touch_padding   = nk_vec2(0.0f,0.0f);
    button->userdata        = nk_handle_ptr(0);
    button->text_alignment  = NK_TEXT_CENTERED;
    button->border          = 0.0f;
    button->rounding        = 0.0f;
    button->draw_begin      = 0;
    button->draw_end        = 0;
    style->property.inc_button = style->property.dec_button;

    /* property edit */
    edit = &style->property.edit;
    nk_zero_struct(*edit);
    edit->normal            = nk_style_item_color(table[NK_COLOR_PROPERTY]);
    edit->hover             = nk_style_item_color(table[NK_COLOR_PROPERTY]);
    edit->active            = nk_style_item_color(table[NK_COLOR_PROPERTY]);
    edit->border_color      = nk_rgba(0,0,0,0);
    edit->cursor_normal     = table[NK_COLOR_TEXT];
    edit->cursor_hover      = table[NK_COLOR_TEXT];
    edit->cursor_text_normal= table[NK_COLOR_EDIT];
    edit->cursor_text_hover = table[NK_COLOR_EDIT];
    edit->text_normal       = table[NK_COLOR_TEXT];
    edit->text_hover        = table[NK_COLOR_TEXT];
    edit->text_active       = table[NK_COLOR_TEXT];
    edit->selected_normal   = table[NK_COLOR_TEXT];
    edit->selected_hover    = table[NK_COLOR_TEXT];
    edit->selected_text_normal  = table[NK_COLOR_EDIT];
    edit->selected_text_hover   = table[NK_COLOR_EDIT];
    edit->padding           = nk_vec2(0,0);
    edit->cursor_size       = 8;
    edit->border            = 0;
    edit->rounding          = 0;

    /* chart */
    chart = &style->chart;
    nk_zero_struct(*chart);
    chart->background       = nk_style_item_color(table[NK_COLOR_CHART]);
    chart->border_color     = table[NK_COLOR_BORDER];
    chart->selected_color   = table[NK_COLOR_CHART_COLOR_HIGHLIGHT];
    chart->color            = table[NK_COLOR_CHART_COLOR];
    chart->padding          = nk_vec2(4,4);
    chart->border           = 0;
    chart->rounding         = 0;

    /* combo */
    combo = &style->combo;
    combo->normal           = nk_style_item_color(table[NK_COLOR_COMBO]);
    combo->hover            = nk_style_item_color(table[NK_COLOR_COMBO]);
    combo->active           = nk_style_item_color(table[NK_COLOR_COMBO]);
    combo->border_color     = table[NK_COLOR_BORDER];
    combo->label_normal     = table[NK_COLOR_TEXT];
    combo->label_hover      = table[NK_COLOR_TEXT];
    combo->label_active     = table[NK_COLOR_TEXT];
    combo->sym_normal       = NK_SYMBOL_TRIANGLE_DOWN;
    combo->sym_hover        = NK_SYMBOL_TRIANGLE_DOWN;
    combo->sym_active       = NK_SYMBOL_TRIANGLE_DOWN;
    combo->content_padding  = nk_vec2(4,4);
    combo->button_padding   = nk_vec2(0,4);
    combo->spacing          = nk_vec2(4,0);
    combo->border           = 1;
    combo->rounding         = 0;

    /* combo button */
    button = &style->combo.button;
    nk_zero_struct(*button);
    button->normal          = nk_style_item_color(table[NK_COLOR_COMBO]);
    button->hover           = nk_style_item_color(table[NK_COLOR_COMBO]);
    button->active          = nk_style_item_color(table[NK_COLOR_COMBO]);
    button->border_color    = nk_rgba(0,0,0,0);
    button->text_background = table[NK_COLOR_COMBO];
    button->text_normal     = table[NK_COLOR_TEXT];
    button->text_hover      = table[NK_COLOR_TEXT];
    button->text_active     = table[NK_COLOR_TEXT];
    button->padding         = nk_vec2(2.0f,2.0f);
    button->touch_padding   = nk_vec2(0.0f,0.0f);
    button->userdata        = nk_handle_ptr(0);
    button->text_alignment  = NK_TEXT_CENTERED;
    button->border          = 0.0f;
    button->rounding        = 0.0f;
    button->draw_begin      = 0;
    button->draw_end        = 0;

    /* tab */
    tab = &style->tab;
    tab->background         = nk_style_item_color(table[NK_COLOR_TAB_HEADER]);
    tab->border_color       = table[NK_COLOR_BORDER];
    tab->text               = table[NK_COLOR_TEXT];
    tab->sym_minimize       = NK_SYMBOL_TRIANGLE_RIGHT;
    tab->sym_maximize       = NK_SYMBOL_TRIANGLE_DOWN;
    tab->padding            = nk_vec2(4,4);
    tab->spacing            = nk_vec2(4,4);
    tab->indent             = 10.0f;
    tab->border             = 1;
    tab->rounding           = 0;

    /* tab button */
    button = &style->tab.tab_minimize_button;
    nk_zero_struct(*button);
    button->normal          = nk_style_item_color(table[NK_COLOR_TAB_HEADER]);
    button->hover           = nk_style_item_color(table[NK_COLOR_TAB_HEADER]);
    button->active          = nk_style_item_color(table[NK_COLOR_TAB_HEADER]);
    button->border_color    = nk_rgba(0,0,0,0);
    button->text_background = table[NK_COLOR_TAB_HEADER];
    button->text_normal     = table[NK_COLOR_TEXT];
    button->text_hover      = table[NK_COLOR_TEXT];
    button->text_active     = table[NK_COLOR_TEXT];
    button->padding         = nk_vec2(2.0f,2.0f);
    button->touch_padding   = nk_vec2(0.0f,0.0f);
    button->userdata        = nk_handle_ptr(0);
    button->text_alignment  = NK_TEXT_CENTERED;
    button->border          = 0.0f;
    button->rounding        = 0.0f;
    button->draw_begin      = 0;
    button->draw_end        = 0;
    style->tab.tab_maximize_button =*button;

    /* node button */
    button = &style->tab.node_minimize_button;
    nk_zero_struct(*button);
    button->normal          = nk_style_item_color(table[NK_COLOR_WINDOW]);
    button->hover           = nk_style_item_color(table[NK_COLOR_WINDOW]);
    button->active          = nk_style_item_color(table[NK_COLOR_WINDOW]);
    button->border_color    = nk_rgba(0,0,0,0);
    button->text_background = table[NK_COLOR_TAB_HEADER];
    button->text_normal     = table[NK_COLOR_TEXT];
    button->text_hover      = table[NK_COLOR_TEXT];
    button->text_active     = table[NK_COLOR_TEXT];
    button->padding         = nk_vec2(2.0f,2.0f);
    button->touch_padding   = nk_vec2(0.0f,0.0f);
    button->userdata        = nk_handle_ptr(0);
    button->text_alignment  = NK_TEXT_CENTERED;
    button->border          = 0.0f;
    button->rounding        = 0.0f;
    button->draw_begin      = 0;
    button->draw_end        = 0;
    style->tab.node_maximize_button =*button;

    /* window header */
    win = &style->window;
    win->header.align = NK_HEADER_RIGHT;
    win->header.close_symbol = NK_SYMBOL_X;
    win->header.minimize_symbol = NK_SYMBOL_MINUS;
    win->header.maximize_symbol = NK_SYMBOL_PLUS;
    win->header.normal = nk_style_item_color(table[NK_COLOR_HEADER]);
    win->header.hover = nk_style_item_color(table[NK_COLOR_HEADER]);
    win->header.active = nk_style_item_color(table[NK_COLOR_HEADER]);
    win->header.label_normal = table[NK_COLOR_TEXT];
    win->header.label_hover = table[NK_COLOR_TEXT];
    win->header.label_active = table[NK_COLOR_TEXT];
    win->header.label_padding = nk_vec2(4,4);
    win->header.padding = nk_vec2(4,4);
    win->header.spacing = nk_vec2(0,0);

    /* window header close button */
    button = &style->window.header.close_button;
    nk_zero_struct(*button);
    button->normal          = nk_style_item_color(table[NK_COLOR_HEADER]);
    button->hover           = nk_style_item_color(table[NK_COLOR_HEADER]);
    button->active          = nk_style_item_color(table[NK_COLOR_HEADER]);
    button->border_color    = nk_rgba(0,0,0,0);
    button->text_background = table[NK_COLOR_HEADER];
    button->text_normal     = table[NK_COLOR_TEXT];
    button->text_hover      = table[NK_COLOR_TEXT];
    button->text_active     = table[NK_COLOR_TEXT];
    button->padding         = nk_vec2(0.0f,0.0f);
    button->touch_padding   = nk_vec2(0.0f,0.0f);
    button->userdata        = nk_handle_ptr(0);
    button->text_alignment  = NK_TEXT_CENTERED;
    button->border          = 0.0f;
    button->rounding        = 0.0f;
    button->draw_begin      = 0;
    button->draw_end        = 0;

    /* window header minimize button */
    button = &style->window.header.minimize_button;
    nk_zero_struct(*button);
    button->normal          = nk_style_item_color(table[NK_COLOR_HEADER]);
    button->hover           = nk_style_item_color(table[NK_COLOR_HEADER]);
    button->active          = nk_style_item_color(table[NK_COLOR_HEADER]);
    button->border_color    = nk_rgba(0,0,0,0);
    button->text_background = table[NK_COLOR_HEADER];
    button->text_normal     = table[NK_COLOR_TEXT];
    button->text_hover      = table[NK_COLOR_TEXT];
    button->text_active     = table[NK_COLOR_TEXT];
    button->padding         = nk_vec2(0.0f,0.0f);
    button->touch_padding   = nk_vec2(0.0f,0.0f);
    button->userdata        = nk_handle_ptr(0);
    button->text_alignment  = NK_TEXT_CENTERED;
    button->border          = 0.0f;
    button->rounding        = 0.0f;
    button->draw_begin      = 0;
    button->draw_end        = 0;

    /* window */
    win->background = table[NK_COLOR_WINDOW];
    win->fixed_background = nk_style_item_color(table[NK_COLOR_WINDOW]);
    win->border_color = table[NK_COLOR_BORDER];
    win->popup_border_color = table[NK_COLOR_BORDER];
    win->combo_border_color = table[NK_COLOR_BORDER];
    win->contextual_border_color = table[NK_COLOR_BORDER];
    win->menu_border_color = table[NK_COLOR_BORDER];
    win->group_border_color = table[NK_COLOR_BORDER];
    win->tooltip_border_color = table[NK_COLOR_BORDER];
    win->scaler = nk_style_item_color(table[NK_COLOR_TEXT]);

    win->rounding = 0.0f;
    win->spacing = nk_vec2(4,4);
    win->scrollbar_size = nk_vec2(10,10);
    win->min_size = nk_vec2(64,64);

    win->combo_border = 1.0f;
    win->contextual_border = 1.0f;
    win->menu_border = 1.0f;
    win->group_border = 1.0f;
    win->tooltip_border = 1.0f;
    win->popup_border = 1.0f;
    win->border = 2.0f;
    win->min_row_height_padding = 8;

    win->padding = nk_vec2(4,4);
    win->group_padding = nk_vec2(4,4);
    win->popup_padding = nk_vec2(4,4);
    win->combo_padding = nk_vec2(4,4);
    win->contextual_padding = nk_vec2(4,4);
    win->menu_padding = nk_vec2(4,4);
    win->tooltip_padding = nk_vec2(4,4);
}
NK_API void
nk_style_set_font(struct nk_context *ctx, const struct nk_user_font *font)
{
    struct nk_style *style;
    NK_ASSERT(ctx);

    if (!ctx) return;
    style = &ctx->style;
    style->font = font;
    ctx->stacks.fonts.head = 0;
    if (ctx->current)
        nk_layout_reset_min_row_height(ctx);
}
NK_API int
nk_style_push_font(struct nk_context *ctx, const struct nk_user_font *font)
{
    struct nk_config_stack_user_font *font_stack;
    struct nk_config_stack_user_font_element *element;

    NK_ASSERT(ctx);
    if (!ctx) return 0;

    font_stack = &ctx->stacks.fonts;
    NK_ASSERT(font_stack->head < (int)NK_LEN(font_stack->elements));
    if (font_stack->head >= (int)NK_LEN(font_stack->elements))
        return 0;

    element = &font_stack->elements[font_stack->head++];
    element->address = &ctx->style.font;
    element->old_value = ctx->style.font;
    ctx->style.font = font;
    return 1;
}
NK_API int
nk_style_pop_font(struct nk_context *ctx)
{
    struct nk_config_stack_user_font *font_stack;
    struct nk_config_stack_user_font_element *element;

    NK_ASSERT(ctx);
    if (!ctx) return 0;

    font_stack = &ctx->stacks.fonts;
    NK_ASSERT(font_stack->head > 0);
    if (font_stack->head < 1)
        return 0;

    element = &font_stack->elements[--font_stack->head];
    *element->address = element->old_value;
    return 1;
}
#define NK_STYLE_PUSH_IMPLEMENATION(prefix, type, stack) \
nk_style_push_##type(struct nk_context *ctx, prefix##_##type *address, prefix##_##type value)\
{\
    struct nk_config_stack_##type * type_stack;\
    struct nk_config_stack_##type##_element *element;\
    NK_ASSERT(ctx);\
    if (!ctx) return 0;\
    type_stack = &ctx->stacks.stack;\
    NK_ASSERT(type_stack->head < (int)NK_LEN(type_stack->elements));\
    if (type_stack->head >= (int)NK_LEN(type_stack->elements))\
        return 0;\
    element = &type_stack->elements[type_stack->head++];\
    element->address = address;\
    element->old_value = *address;\
    *address = value;\
    return 1;\
}
#define NK_STYLE_POP_IMPLEMENATION(type, stack) \
nk_style_pop_##type(struct nk_context *ctx)\
{\
    struct nk_config_stack_##type *type_stack;\
    struct nk_config_stack_##type##_element *element;\
    NK_ASSERT(ctx);\
    if (!ctx) return 0;\
    type_stack = &ctx->stacks.stack;\
    NK_ASSERT(type_stack->head > 0);\
    if (type_stack->head < 1)\
        return 0;\
    element = &type_stack->elements[--type_stack->head];\
    *element->address = element->old_value;\
    return 1;\
}
NK_API int NK_STYLE_PUSH_IMPLEMENATION(struct nk, style_item, style_items)
NK_API int NK_STYLE_PUSH_IMPLEMENATION(nk,float, floats)
NK_API int NK_STYLE_PUSH_IMPLEMENATION(struct nk, vec2, vectors)
NK_API int NK_STYLE_PUSH_IMPLEMENATION(nk,flags, flags)
NK_API int NK_STYLE_PUSH_IMPLEMENATION(struct nk,color, colors)

NK_API int NK_STYLE_POP_IMPLEMENATION(style_item, style_items)
NK_API int NK_STYLE_POP_IMPLEMENATION(float,floats)
NK_API int NK_STYLE_POP_IMPLEMENATION(vec2, vectors)
NK_API int NK_STYLE_POP_IMPLEMENATION(flags,flags)
NK_API int NK_STYLE_POP_IMPLEMENATION(color,colors)

NK_API int
nk_style_set_cursor(struct nk_context *ctx, enum nk_style_cursor c)
{
    struct nk_style *style;
    NK_ASSERT(ctx);
    if (!ctx) return 0;
    style = &ctx->style;
    if (style->cursors[c]) {
        style->cursor_active = style->cursors[c];
        return 1;
    }
    return 0;
}
NK_API void
nk_style_show_cursor(struct nk_context *ctx)
{
    ctx->style.cursor_visible = nk_true;
}
NK_API void
nk_style_hide_cursor(struct nk_context *ctx)
{
    ctx->style.cursor_visible = nk_false;
}
NK_API void
nk_style_load_cursor(struct nk_context *ctx, enum nk_style_cursor cursor,
    const struct nk_cursor *c)
{
    struct nk_style *style;
    NK_ASSERT(ctx);
    if (!ctx) return;
    style = &ctx->style;
    style->cursors[cursor] = c;
}
NK_API void
nk_style_load_all_cursors(struct nk_context *ctx, struct nk_cursor *cursors)
{
    int i = 0;
    struct nk_style *style;
    NK_ASSERT(ctx);
    if (!ctx) return;
    style = &ctx->style;
    for (i = 0; i < NK_CURSOR_COUNT; ++i)
        style->cursors[i] = &cursors[i];
    style->cursor_visible = nk_true;
}

