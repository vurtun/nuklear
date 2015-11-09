#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#define MAX(a,b)    ((a) < (b) ? (b) : (a))
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#define LEN(a)      (sizeof(a)/sizeof(a)[0])
#define UNUSED(a)   ((void)(a))

#define MAX_BUFFER  64
#define MAX_MEMORY  (32 * 1024)
#define MAX_COMMAND_MEMORY  (16 * 1024)
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

enum theme {THEME_BLACK, THEME_WHITE};

struct tree_node {
    zr_state state;
    const char *name;
    struct tree_node *parent;
    struct tree_node *children[8];
    int count;
};

struct test_tree {
    struct tree_node root;
    struct tree_node *clipboard[16];
    struct tree_node nodes[8];
    int count;
};

struct demo {
    zr_bool running;
    struct zr_input input;
    struct zr_command_queue queue;
    struct zr_style config_black;
    struct zr_style config_white;
    struct zr_user_font font;
    struct zr_window panel;
    struct zr_window sub;
    struct zr_window metrics;
    zr_size w, h;
    enum theme theme;

    struct test_tree tree;
    struct zr_edit_box text;
};

static void
zr_labelf(struct zr_context *panel, enum zr_text_align align, const zr_char *fmt, ...)
{
    zr_char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    buffer[1023] = 0;
    zr_label(panel, buffer, align);
    va_end(args);
}

static void
tree_init(struct test_tree *tree)
{
    /* this is just test data */
    tree->root.state = ZR_NODE_ACTIVE;
    tree->root.name = "Primitives";
    tree->root.parent = NULL;
    tree->root.count = 2;
    tree->root.children[0] = &tree->nodes[0];
    tree->root.children[1] = &tree->nodes[4];

    tree->nodes[0].state = 0;
    tree->nodes[0].name = "Boxes";
    tree->nodes[0].parent = &tree->root;
    tree->nodes[0].count = 2;
    tree->nodes[0].children[0] = &tree->nodes[1];
    tree->nodes[0].children[1] = &tree->nodes[2];

    tree->nodes[1].state = 0;
    tree->nodes[1].name = "Box0";
    tree->nodes[1].parent = &tree->nodes[0];
    tree->nodes[1].count = 0;

    tree->nodes[2].state = 0;
    tree->nodes[2].name = "Box1";
    tree->nodes[2].parent = &tree->nodes[0];
    tree->nodes[2].count = 0;

    tree->nodes[4].state = ZR_NODE_ACTIVE;
    tree->nodes[4].name = "Cylinders";
    tree->nodes[4].parent = &tree->root;
    tree->nodes[4].count = 2;
    tree->nodes[4].children[0] = &tree->nodes[5];
    tree->nodes[4].children[1] = &tree->nodes[6];

    tree->nodes[5].state = 0;
    tree->nodes[5].name = "Cylinder0";
    tree->nodes[5].parent = &tree->nodes[4];
    tree->nodes[5].count = 0;

    tree->nodes[6].state = 0;
    tree->nodes[6].name = "Cylinder1";
    tree->nodes[6].parent = &tree->nodes[4];
    tree->nodes[6].count = 0;
}

static void
tree_remove_node(struct tree_node *parent, struct tree_node *child)
{
    int i = 0;
    child->parent = NULL;
    if (!parent->count) return;
    if (parent->count == 1) {
        parent->count = 0;
        return;
    }
    for (i = 0; i < parent->count; ++i) {
        if (parent->children[i] == child)
            break;
    }
    if (i == parent->count) return;
    if (i == parent->count - 1) {
        parent->count--;
        return;
    } else{
        parent->children[i] = parent->children[parent->count-1];
        parent->count--;
    }
}

static void
tree_add_node(struct tree_node *parent, struct tree_node *child)
{
    assert(parent->count < 8);
    child->parent = parent;
    parent->children[parent->count++] = child;
}

static void
tree_push_node(struct test_tree *tree, struct tree_node *node)
{
    assert(tree->count < 16);
    tree->clipboard[tree->count++] = node;
}

static struct tree_node*
tree_pop_node(struct test_tree *tree)
{
    assert(tree->count > 0);
    return tree->clipboard[--tree->count];
}

static int
upload_tree(struct test_tree *base, struct zr_tree *tree, struct tree_node *node)
{
    int i = 0, n = 0;
    enum zr_tree_node_operation op;
    if (node->count) {
        i = 0;
        op = zr_tree_begin_node(tree, node->name, &node->state);
        while (i < node->count)
            i += upload_tree(base, tree, node->children[i]);
        zr_tree_end_node(tree);
    }
    else op = zr_tree_leaf(tree, node->name, &node->state);

    switch (op) {
    case ZR_NODE_NOP: break;
    case ZR_NODE_CUT:
        tree_remove_node(node->parent, node);
        tree_push_node(base, node);
        return 0;
    case ZR_NODE_DELETE:
        tree_remove_node(node->parent, node); break;
        return 0;
    case ZR_NODE_PASTE:
        i = 0; n = base->count;
        while (i++ < n)
            tree_add_node(node, tree_pop_node(base));
    case ZR_NODE_CLONE:
    default:break;
    }
    return 1;
}

static int
show_test_window(struct zr_window *window, struct zr_style *config, enum theme *theme,
    struct test_tree *test_tree, struct zr_edit_box *edit_box)
{
    zr_flags ret;
    struct zr_context layout;

    /* window flags */
    static zr_bool show_menu = zr_true;
    static zr_bool titlebar = zr_true;
    static zr_bool border = zr_true;
    static zr_bool resize = zr_true;
    static zr_bool moveable = zr_true;
    static zr_bool no_scrollbar = zr_false;
    static zr_flags window_flags = 0;

    /* header flags */
    static zr_bool minimizable = zr_true;
    static zr_bool close = zr_true;
    static zr_bool scale = zr_false;
    static zr_bool move = zr_false;
    static zr_flags header_flags = 0;

    /* collapsable headers */
    static zr_state window_option_state = ZR_MINIMIZED;
    static zr_state header_option_state = ZR_MINIMIZED;
    static zr_state widget_state = ZR_MINIMIZED;
    static zr_state graph_state = ZR_MINIMIZED;
    static zr_state style_state = ZR_MINIMIZED;
    static zr_state group_state = ZR_MINIMIZED;
    static zr_state shelf_state = ZR_MINIMIZED;
    static zr_state splitter_state = ZR_MINIMIZED;

    /* popups */
    static enum zr_header_align header_align = ZR_HEADER_RIGHT;
    static zr_bool show_app_about = zr_false;
    static zr_state show_contextual = zr_false;
    static struct zr_rect contextual_bounds;
    static zr_bool show_close_popup = zr_false;
    static zr_bool show_color_picker_popup = zr_false;
    static zr_int color_picker_index;
    static struct zr_color color_picker_color;

    /* window flags */
    window->style = config;
    window_flags = 0;
    if (border) window_flags |= ZR_WINDOW_BORDER;
    if (resize) window_flags |= ZR_WINDOW_SCALEABLE;
    if (moveable) window_flags |= ZR_WINDOW_MOVEABLE;
    if (no_scrollbar) window_flags |= ZR_WINDOW_NO_SCROLLBAR;
    if (window->flags & ZR_WINDOW_ACTIVE) window_flags |= ZR_WINDOW_ACTIVE;
    if (window->flags & ZR_WINDOW_ROM) window_flags |= ZR_WINDOW_ROM;
    if (window->flags & ZR_WINDOW_MINIMIZED) window_flags |= ZR_WINDOW_MINIMIZED;

    /* header flags */
    header_flags = 0;
    if (minimizable) header_flags |= ZR_MINIMIZABLE;
    if (close) header_flags |= ZR_CLOSEABLE;
    if (scale) header_flags |= ZR_SCALEABLE;
    if (move) header_flags |= ZR_MOVEABLE;

    /* main window */
    window->flags = window_flags;
    zr_begin(&layout, window);
    ret = zr_header(&layout, (titlebar)? "Zahnrad":"", header_flags, header_flags, header_align);
    if (ret & ZR_CLOSEABLE) {
        layout.flags &= (zr_flags)~ZR_WINDOW_HIDDEN;
        layout.valid = zr_true;
        show_close_popup = zr_true;
    }

    if (show_menu)
    {
        /* menubar */
        struct zr_context menu;
        static zr_state file_state = ZR_MINIMIZED;
        static zr_size mprog = 60;
        static zr_int mslider = 10;
        static zr_bool mcheck = zr_true;

        zr_menubar_begin(&layout);

        zr_layout_row_begin(&layout, ZR_STATIC, 25, 2);
        zr_layout_row_push(&layout, 45);
        zr_menu_begin(&layout, &menu, "MENU", 100, &file_state);
        {
            static zr_size prog = 40;
            static zr_int slider = 10;
            static zr_bool check = zr_true;
            zr_layout_row_dynamic(&menu, 25, 1);
            zr_progress(&menu, &prog, 100, ZR_MODIFYABLE);
            zr_slider_int(&menu, 0, &slider, 16, 1);
            zr_checkbox(&menu, "check", &check);
            if (zr_menu_item(&menu, ZR_TEXT_CENTERED, "Hide")) {
                show_menu = zr_false;
                zr_menu_close(&menu, &file_state);
            }
            if (zr_menu_item(&menu, ZR_TEXT_CENTERED, "About")) {
                show_app_about = zr_true;
                zr_menu_close(&menu, &file_state);
            }
            if (zr_menu_item(&menu, ZR_TEXT_CENTERED, "Quit")) {
                show_close_popup = zr_true;
                zr_menu_close(&menu, &file_state);
            }
        }
        zr_menu_end(&layout, &menu);

        zr_layout_row_push(&layout, 60);
        zr_progress(&layout, &mprog, 100, ZR_MODIFYABLE);
        zr_slider_int(&layout, 0, &mslider, 16, 1);
        zr_checkbox(&layout, "check", &mcheck);

        zr_menubar_end(&layout);
    }

    if (show_app_about)
    {
        /* about popup */
        struct zr_context popup;
        static struct zr_vec2 about_scrollbar;
        static struct zr_rect s = {20, 100, 300, 190};
        zr_popup_begin(&layout, &popup, ZR_POPUP_STATIC, 0, s, about_scrollbar);
        if (zr_header(&popup, "About", ZR_CLOSEABLE, ZR_CLOSEABLE, ZR_HEADER_LEFT)){
            show_app_about = zr_false;
            zr_popup_close(&popup);
        }
        zr_layout_row_dynamic(&popup, 20, 1);
        zr_label(&popup, "Zahnrad", ZR_TEXT_LEFT);
        zr_label(&popup, "By Micha Mettke", ZR_TEXT_LEFT);
        zr_label(&popup, "Zahnrad is licensed under the MIT License.",  ZR_TEXT_LEFT);
        zr_label(&popup, "See LICENSE for more information", ZR_TEXT_LEFT);
        zr_popup_end(&layout, &popup, &about_scrollbar);
    }

    if (show_close_popup)
    {
        /* close popup */
        struct zr_context popup;
        static struct zr_vec2 close_scrollbar = {0,0};
        static const struct zr_rect s = {20, 150, 230, 150};
        zr_popup_begin(&layout, &popup, ZR_POPUP_STATIC, 0, s, close_scrollbar);
        if (zr_header(&popup, "Quit", ZR_CLOSEABLE, ZR_CLOSEABLE, ZR_HEADER_LEFT)) {
            show_close_popup = zr_false;
            zr_popup_close(&popup);
        }
        zr_layout_row_dynamic(&popup, 30, 1);
        zr_label(&popup, "Are you sure you want to exit?", ZR_TEXT_LEFT);
        zr_layout_row_dynamic(&popup, 30, 4);
        zr_spacing(&popup, 1);
        if (zr_button_text(&popup, "Yes", ZR_BUTTON_DEFAULT)) {
            show_close_popup = zr_false;
            zr_popup_close(&popup);
            return 0;
        }
        if (zr_button_text(&popup, "No", ZR_BUTTON_DEFAULT)) {
            show_close_popup = zr_false;
            zr_popup_close(&popup);
        }
        zr_popup_end(&layout, &popup, NULL);
    }

    if (show_color_picker_popup)
    {
        /* color picker popup */
        static zr_state active[4];
        struct zr_context popup;
        zr_int r,g,b,a;
        zr_popup_begin(&layout, &popup, ZR_POPUP_STATIC,0, zr_rect(10, 100, 280, 280), zr_vec2(0,0));
        if (zr_header(&popup, "Color", ZR_CLOSEABLE, ZR_CLOSEABLE, ZR_HEADER_LEFT))
        {
            zr_popup_close(&popup);
            show_color_picker_popup = zr_false;
            memset(active, 0, sizeof(active));
        }

        zr_layout_row_dynamic(&popup, 30, 2);
        zr_label(&popup, zr_style_color_name((enum zr_style_colors)color_picker_index), ZR_TEXT_LEFT);
        zr_button_color(&popup, color_picker_color, ZR_BUTTON_DEFAULT);

        zr_layout_row_dynamic(&popup, 30, 2);
        r = color_picker_color.r, g = color_picker_color.g, b = color_picker_color.b, a = color_picker_color.a;
        zr_spinner_int(&popup, 0, &r, 255, 1, &active[0]);
        zr_slider_int(&popup, 0, &r,  255, 10);
        zr_spinner_int(&popup, 0, &g, 255, 1, &active[1]);
        zr_slider_int(&popup, 0, &g,  255, 10);
        zr_spinner_int(&popup, 0, &b, 255, 1, &active[2]);
        zr_slider_int(&popup, 0, &b,  255, 10);
        zr_spinner_int(&popup, 0, &a, 255, 1, &active[3]);
        zr_slider_int(&popup, 0, &a,  255, 10);
        color_picker_color = zr_rgba((zr_byte)r,(zr_byte)g,(zr_byte)b,(zr_byte)a);

        zr_layout_row_dynamic(&popup, 30, 4);
        zr_spacing(&popup, 1);
        if (zr_button_text(&popup, "ok", ZR_BUTTON_DEFAULT))
        {
            zr_popup_close(&popup);
            show_color_picker_popup = zr_false;
            config->colors[color_picker_index] = color_picker_color;
            memset(active, 0, sizeof(active));
        }
        if (zr_button_text(&popup, "cancel", ZR_BUTTON_DEFAULT))
        {
            zr_popup_close(&popup);
            show_color_picker_popup = zr_false;
            memset(active, 0, sizeof(active));
        }
        zr_popup_end(&layout, &popup, NULL);
    }

    /* contextual menu */
    if (zr_input_mouse_clicked(layout.input, ZR_BUTTON_RIGHT, layout.bounds)) {
        contextual_bounds = zr_rect(layout.input->mouse.pos.x, layout.input->mouse.pos.y, 100, 200);
        show_contextual = zr_true;
    }
    if (show_contextual) {
        struct zr_context menu;
        static zr_size prog = 40;
        static zr_int slider = 10;

        zr_contextual_begin(&layout, &menu, ZR_WINDOW_NO_SCROLLBAR, &show_contextual, contextual_bounds);
        zr_layout_row_dynamic(&menu, 25, 1);
        zr_checkbox(&menu, "Menu", &show_menu);
        zr_progress(&menu, &prog, 100, ZR_MODIFYABLE);
        zr_slider_int(&menu, 0, &slider, 16, 1);
        if (zr_contextual_item(&menu, "About", ZR_TEXT_CENTERED))
            show_app_about = zr_true;
        if (zr_contextual_item(&menu, "Quit", ZR_TEXT_CENTERED))
            show_close_popup = zr_true;
        zr_contextual_end(&layout, &menu, &show_contextual);
    }

    if (zr_layout_push(&layout, ZR_LAYOUT_TAB, "Window", &window_option_state))
    {
        /* window options */
        zr_layout_row_dynamic(&layout, 30, 2);
        zr_checkbox(&layout, "Titlebar", &titlebar);
        zr_checkbox(&layout, "Menu", &show_menu);
        zr_checkbox(&layout, "Border", &border);
        zr_checkbox(&layout, "Resizable", &resize);
        zr_checkbox(&layout, "Moveable", &moveable);
        zr_checkbox(&layout, "No Scrollbar", &no_scrollbar);
        zr_layout_pop(&layout);
    }

    if (zr_layout_push(&layout, ZR_LAYOUT_TAB, "Header", &header_option_state))
    {
        /* header options */
        zr_layout_row_dynamic(&layout, 30, 2);
        zr_checkbox(&layout, "Minimizable", &minimizable);
        zr_checkbox(&layout, "Closeable", &close);
        zr_checkbox(&layout, "Scaleable", &scale);
        zr_checkbox(&layout, "Moveable", &move);
        zr_layout_pop(&layout);
    }

    if (zr_layout_push(&layout, ZR_LAYOUT_TAB, "Style", &style_state))
    {
        /* style editor */
        static zr_state property_state = ZR_MINIMIZED;
        static zr_state rounding_state = ZR_MINIMIZED;
        static zr_state color_state = ZR_MINIMIZED;
        struct zr_context combo;
        static const char *themes[] = {"Black", "White"};
        static zr_state theme_active = zr_false;

        /* theme */
        zr_layout_row_static(&layout, 30, 80, 2);
        zr_label(&layout, "Theme:", ZR_TEXT_LEFT);
        zr_combo_begin(&layout, &combo, themes[*theme], &theme_active);
        zr_layout_row_dynamic(&combo, 25, 1);
        *theme = zr_combo_item(&combo, themes[THEME_BLACK], ZR_TEXT_CENTERED) ? THEME_BLACK : *theme;
        *theme = zr_combo_item(&combo, themes[THEME_WHITE], ZR_TEXT_CENTERED) ? THEME_WHITE : *theme;
        zr_combo_end(&layout, &combo, &theme_active);

        if (zr_layout_push(&layout, ZR_LAYOUT_NODE, "Properties", &property_state))
        {
            /* properties */
            zr_size i = 0;
            zr_layout_row_dynamic(&layout, 30, 3);
            for (i = 0; i <= ZR_PROPERTY_SCROLLBAR_SIZE; ++i) {
                zr_label(&layout, zr_style_property_name((enum zr_style_properties)i), ZR_TEXT_LEFT);
                zr_spinner_float(&layout, 0, &config->properties[i].x, 20, 1, NULL);
                zr_spinner_float(&layout, 0, &config->properties[i].y, 20, 1, NULL);
            }
            zr_layout_pop(&layout);
        }

        if (zr_layout_push(&layout, ZR_LAYOUT_NODE, "Rounding", &rounding_state))
        {
            /* rounding */
            zr_size i = 0;
            zr_layout_row_dynamic(&layout, 30, 2);
            for (i = 0; i < ZR_ROUNDING_MAX; ++i) {
                zr_label(&layout, zr_style_rounding_name((enum zr_style_rounding)i), ZR_TEXT_LEFT);
                zr_spinner_float(&layout, 0, &config->rounding[i], 20, 1, NULL);
            }
            zr_layout_pop(&layout);
        }

        if (zr_layout_push(&layout, ZR_LAYOUT_NODE, "Color", &color_state))
        {
            /* color */
            zr_size i = 0;
            struct zr_context tab;
            static struct zr_vec2 scrollbar;
            zr_layout_row_dynamic(&layout, 400, 1);
            zr_group_begin(&layout, &tab, NULL, 0, scrollbar);
            for (i = 0; i < ZR_COLOR_COUNT; ++i) {
                zr_layout_row_dynamic(&tab, 30, 2);
                zr_label(&tab, zr_style_color_name((enum zr_style_colors)i), ZR_TEXT_LEFT);
                if (zr_button_color(&tab, config->colors[i], ZR_BUTTON_DEFAULT)) {
                    show_color_picker_popup = zr_true;
                    color_picker_index = (zr_int)i;
                    color_picker_color = config->colors[i];
                }
            }
            zr_group_end(&layout, &tab, &scrollbar);
            zr_layout_pop(&layout);
        }
        zr_layout_pop(&layout);
    }

    if (zr_layout_push(&layout, ZR_LAYOUT_TAB, "Widgets", &widget_state))
    {
        static zr_state text_state = zr_false;
        static zr_state main_state = zr_false;
        static zr_state button_state = zr_false;
        static zr_state combo_state = zr_false;
        static zr_state input_state = zr_false;
        static zr_state sel_state = zr_false;

        enum options {A,B,C};
        static zr_bool checkbox;
        static zr_bool option;

        if (zr_layout_push(&layout, ZR_LAYOUT_NODE, "Text", &text_state))
        {
            /* Text Widgets */
            zr_layout_row_dynamic(&layout, 20, 1);
            zr_label(&layout, "Label aligned left", ZR_TEXT_LEFT);
            zr_label(&layout, "Label aligned centered", ZR_TEXT_CENTERED);
            zr_label(&layout, "Label aligned right", ZR_TEXT_RIGHT);
            zr_label_colored(&layout, "Blue text", ZR_TEXT_LEFT, zr_rgb(0,0,255));
            zr_label_colored(&layout, "Yellow text", ZR_TEXT_LEFT, zr_rgb(255,255,0));
            zr_text(&layout, "Text without /0", 15, ZR_TEXT_RIGHT);

            zr_layout_row_static(&layout, 100, 200, 1);
            zr_label_wrap(&layout, "This is a very long line to hopefully get this text to be wrapped into multiple lines to show line wrapping");
            zr_layout_row_dynamic(&layout, 100, 1);
            zr_label_wrap(&layout, "This is another long text to show dynamic window changes on multiline text");
            zr_layout_pop(&layout);
        }

        if (zr_layout_push(&layout, ZR_LAYOUT_NODE, "Button", &button_state))
        {
            /* Buttons Widgets */
            zr_layout_row_static(&layout, 30, 100, 3);
            if (zr_button_text(&layout, "Button", ZR_BUTTON_DEFAULT))
                fprintf(stdout, "Button pressed!\n");
            if (zr_button_text(&layout, "Repeater", ZR_BUTTON_REPEATER))
                fprintf(stdout, "Repeater is being pressed!\n");
            zr_button_color(&layout, zr_rgb(0,0,255), ZR_BUTTON_DEFAULT);

            zr_layout_row_static(&layout, 20, 20, 8);
            zr_button_symbol(&layout, ZR_SYMBOL_CIRCLE, ZR_BUTTON_DEFAULT);
            zr_button_symbol(&layout, ZR_SYMBOL_CIRCLE_FILLED, ZR_BUTTON_DEFAULT);
            zr_button_symbol(&layout, ZR_SYMBOL_RECT, ZR_BUTTON_DEFAULT);
            zr_button_symbol(&layout, ZR_SYMBOL_RECT_FILLED, ZR_BUTTON_DEFAULT);
            zr_button_symbol(&layout, ZR_SYMBOL_TRIANGLE_UP, ZR_BUTTON_DEFAULT);
            zr_button_symbol(&layout, ZR_SYMBOL_TRIANGLE_DOWN, ZR_BUTTON_DEFAULT);
            zr_button_symbol(&layout, ZR_SYMBOL_TRIANGLE_LEFT, ZR_BUTTON_DEFAULT);
            zr_button_symbol(&layout, ZR_SYMBOL_TRIANGLE_RIGHT, ZR_BUTTON_DEFAULT);

            zr_layout_row_static(&layout, 30, 100, 2);
            zr_button_text_symbol(&layout, ZR_SYMBOL_TRIANGLE_LEFT, "prev", ZR_TEXT_RIGHT, ZR_BUTTON_DEFAULT);
            zr_button_text_symbol(&layout, ZR_SYMBOL_TRIANGLE_RIGHT, "next", ZR_TEXT_LEFT, ZR_BUTTON_DEFAULT);
            zr_layout_pop(&layout);
        }

        if (zr_layout_push(&layout, ZR_LAYOUT_NODE, "Basic", &main_state))
        {
            /* Basic widgets */
            static zr_int int_slider = 5;
            static zr_float float_slider = 2.5f;
            static zr_size prog_value = 40;
            static zr_float float_spinner = 2.5f;
            static zr_int int_spinner = 20;
            static zr_state spinneri_active, spinnerf_active;
            static const zr_float ratio[] = {100, 150};
            const struct zr_input *in = zr_input(&layout);
            struct zr_rect bounds;

            zr_layout_row_static(&layout, 30, 100, 1);
            zr_checkbox(&layout, "Checkbox", &checkbox);

            zr_layout_row_static(&layout, 30, 80, 3);
            option = zr_option(&layout, "optionA", option == A) ? A : option;
            option = zr_option(&layout, "optionB", option == B) ? B : option;
            option = zr_option(&layout, "optionC", option == C) ? C : option;

            zr_layout_row_static(&layout, 30, 150, 1);
            zr_layout_peek(&bounds, &layout);
            zr_label(&layout, "Hover me for tooltip", ZR_TEXT_LEFT);
            if (zr_input_is_mouse_hovering_rect(in, bounds))
                zr_tooltip(&layout, "This is a tooltip");

            zr_layout_row(&layout, ZR_STATIC, 30, 2, ratio);
            zr_labelf(&layout, ZR_TEXT_LEFT, "Slider(%d):", int_slider);
            zr_slider_int(&layout, 0, &int_slider, 10, 1);
            zr_labelf(&layout, ZR_TEXT_LEFT, "Slider(%.2f):", float_slider);
            zr_slider_float(&layout, 0, &float_slider, 5.0, 0.5f);
            zr_labelf(&layout, ZR_TEXT_LEFT, "Progressbar: %lu:" , prog_value);
            zr_progress(&layout, &prog_value, 100, ZR_MODIFYABLE);
            zr_label(&layout, "Spinner int:", ZR_TEXT_LEFT);
            zr_spinner_int(&layout, 0, &int_spinner, 50.0, 1, &spinneri_active);
            zr_label(&layout, "Spinner float:", ZR_TEXT_LEFT);
            zr_spinner_float(&layout, 0, &float_spinner, 5.0, 0.5f, &spinnerf_active);
            zr_layout_pop(&layout);
        }

        if (zr_layout_push(&layout, ZR_LAYOUT_NODE, "Selectable", &sel_state))
        {
            static zr_state basic_state = ZR_MINIMIZED;
            static zr_state list_state = ZR_MINIMIZED;
            static zr_state grid_state = ZR_MINIMIZED;
            if (zr_layout_push(&layout, ZR_LAYOUT_NODE, "Basic", &basic_state))
            {
                zr_layout_row_static(&layout, 18, 100, 2);
                zr_select(&layout, "Selected", ZR_TEXT_LEFT, zr_true);
                zr_select(&layout, "Not Selected", ZR_TEXT_LEFT, zr_false);
                zr_layout_pop(&layout);
            }
            if (zr_layout_push(&layout, ZR_LAYOUT_NODE, "List", &list_state))
            {
                static zr_bool selected[4] = {zr_false, zr_false, zr_true, zr_false};
                zr_layout_row_static(&layout, 18, 100, 1);
                zr_selectable(&layout, "Selectable", ZR_TEXT_LEFT, &selected[0]);
                zr_selectable(&layout, "Selectable", ZR_TEXT_LEFT, &selected[1]);
                zr_label(&layout, "Not Selectable", ZR_TEXT_LEFT);
                zr_selectable(&layout, "Selectable", ZR_TEXT_LEFT, &selected[2]);
                zr_selectable(&layout, "Selectable", ZR_TEXT_LEFT, &selected[3]);
                zr_layout_pop(&layout);
            }
            if (zr_layout_push(&layout, ZR_LAYOUT_NODE, "Grid", &grid_state))
            {
                int i;
                static zr_bool selected[16];
                zr_layout_row_static(&layout, 50, 50, 4);
                for (i = 0; i < 16; ++i) {
                    if (zr_selectable(&layout, "Z", ZR_TEXT_CENTERED, &selected[i])) {
                        zr_int x = (i % 4), y = i / 4;
                        if (x > 0) selected[i - 1] ^= 1;
                        if (x < 3) selected[i + 1] ^= 1;
                        if (y > 0) selected[i - 4] ^= 1;
                        if (y < 3) selected[i + 4] ^= 1;
                    }
                }
                zr_layout_pop(&layout);
            }
            zr_layout_pop(&layout);
        }

        if (zr_layout_push(&layout, ZR_LAYOUT_NODE, "Combo", &combo_state))
        {
            /* Combobox Widgets */
            static zr_state weapon_active = zr_false;
            static zr_state com_color_active = zr_false;
            static zr_state prog_active = zr_false;
            static zr_state check_active = zr_false;

            static const char *weapons[] = {"Fist","Pistol","Shotgun","Plasma","BFG"};
            static zr_size current_weapon = 0;
            static zr_bool check_values[5];
            static zr_int r = 130, g = 50, b = 50, a = 255;
            static zr_size x =  20, y = 40, z = 10, w = 90;

            struct zr_context combo;
            zr_char buffer[32];
            zr_size sum = 0;

            /* default combobox */
            zr_layout_row_static(&layout, 30, 200, 1);
            zr_combo_begin(&layout, &combo, weapons[current_weapon], &weapon_active);
            {
                zr_size i = 0;
                zr_layout_row_dynamic(&combo, 25, 1);
                for (i = 0; i < LEN(weapons); ++i) {
                    if (zr_combo_item(&combo, weapons[i], ZR_TEXT_LEFT))
                        current_weapon = i;
                }
            }
            zr_combo_end(&layout, &combo, &weapon_active);

            /* slider color combobox */
            sprintf(buffer, "#%02x%02x%02x%02x", r, g, b, a);
            zr_style_push_color(config, ZR_COLOR_SPINNER, zr_rgba((zr_byte)r,(zr_byte)g,(zr_byte)b,(zr_byte)a));
            zr_combo_begin(&layout, &combo, buffer, &com_color_active);
            {
                zr_float ratios[] = {0.15f, 0.85f};
                zr_layout_row(&combo, ZR_DYNAMIC, 30, 2, ratios);
                zr_label(&combo, "R", ZR_TEXT_LEFT);
                zr_slider_int(&combo, 0, &r, 255, 5);
                zr_label(&combo, "G", ZR_TEXT_LEFT);
                zr_slider_int(&combo, 0, &g, 255, 5);
                zr_label(&combo, "B", ZR_TEXT_LEFT);
                zr_slider_int(&combo, 0, &b, 255, 5);
                zr_label(&combo, "A", ZR_TEXT_LEFT);
                zr_slider_int(&combo, 0, &a, 255, 5);
            }
            zr_combo_end(&layout, &combo, NULL);
            zr_style_pop_color(config);

            /* progressbar combobox */
            sum = x + y + z + w;
            sprintf(buffer, "%lu", sum);
            zr_combo_begin(&layout, &combo, buffer, &prog_active);
            {
                zr_layout_row_dynamic(&combo, 30, 1);
                zr_progress(&combo, &x, 100, ZR_MODIFYABLE);
                zr_progress(&combo, &y, 100, ZR_MODIFYABLE);
                zr_progress(&combo, &z, 100, ZR_MODIFYABLE);
                zr_progress(&combo, &w, 100, ZR_MODIFYABLE);
            }
            zr_combo_end(&layout, &combo, NULL);

            /* checkbox combobox */
            sum = (zr_size)(check_values[0] + check_values[1] + check_values[2] + check_values[3] + check_values[4]);
            sprintf(buffer, "%lu", sum);
            zr_combo_begin(&layout, &combo, buffer, &check_active);
            {
                zr_layout_row_dynamic(&combo, 30, 1);
                zr_checkbox(&combo, weapons[0], &check_values[0]);
                zr_checkbox(&combo, weapons[1], &check_values[1]);
                zr_checkbox(&combo, weapons[2], &check_values[2]);
                zr_checkbox(&combo, weapons[3], &check_values[3]);
            }
            zr_combo_end(&layout, &combo, NULL);

            zr_layout_pop(&layout);
        }
        if (zr_layout_push(&layout, ZR_LAYOUT_NODE, "Input", &input_state))
        {
            static char text[7][64];
            static zr_size text_len[7];
            static zr_state text_active[7];
            static zr_size text_cursor[7];
            static const zr_float ratio[] = {120, 100};

            zr_layout_row(&layout, ZR_STATIC, 25, 2, ratio);
            zr_label(&layout, "Default:", ZR_TEXT_LEFT);
            zr_edit(&layout, text[0], &text_len[0], 64, &text_active[0], &text_cursor[0], ZR_INPUT_DEFAULT);
            zr_label(&layout, "Int:", ZR_TEXT_LEFT);
            zr_edit(&layout, text[1], &text_len[1], 64, &text_active[1], &text_cursor[1], ZR_INPUT_DEC);
            zr_label(&layout, "Float:", ZR_TEXT_LEFT);
            zr_edit(&layout, text[2], &text_len[2], 64, &text_active[2], &text_cursor[2], ZR_INPUT_FLOAT);
            zr_label(&layout, "Hex:", ZR_TEXT_LEFT);
            zr_edit(&layout, text[4], &text_len[4], 64, &text_active[4], &text_cursor[4], ZR_INPUT_HEX);
            zr_label(&layout, "Octal:", ZR_TEXT_LEFT);
            zr_edit(&layout, text[5], &text_len[5], 64, &text_active[5], &text_cursor[5], ZR_INPUT_OCT);
            zr_label(&layout, "Binary:", ZR_TEXT_LEFT);
            zr_edit(&layout, text[6], &text_len[6], 64, &text_active[6], &text_cursor[6], ZR_INPUT_BIN);
            zr_label(&layout, "Editbox:", ZR_TEXT_LEFT);
            zr_editbox(&layout, edit_box);

            zr_layout_pop(&layout);
        }
        zr_layout_pop(&layout);
    }

    if (zr_layout_push(&layout, ZR_LAYOUT_TAB, "Graph", &graph_state))
    {
        static const zr_float values[]={8.0f,15.0f,20.0f,12.0f,30.0f,12.0f,35.0f,40.0f,20.0f};
        static zr_int col_index = -1;
        static zr_int line_index = -1;
        static struct zr_vec2 scrollbar;

        zr_size i;
        zr_float min_value;
        zr_float max_value;
        zr_int index = -1;
        struct zr_rect bounds;
        struct zr_graph graph;
        struct zr_tree tree;
        char buffer[64];

        /* find min and max graph value */
        max_value = values[0];
        min_value = values[0];
        for (i = 0; i < LEN(values); ++i) {
            if (values[i] > max_value)
                max_value = values[i];
            if (values[i] < min_value)
                min_value = values[i];
        }

        /* column graph */
        zr_layout_row_dynamic(&layout, 100, 1);
        zr_layout_peek(&bounds, &layout);
        zr_graph_begin(&layout, &graph, ZR_GRAPH_COLUMN, LEN(values), min_value, max_value);
        for (i = 0; i < LEN(values); ++i) {
            zr_flags res = zr_graph_push(&layout, &graph, values[i]);
            if (res & ZR_GRAPH_HOVERING)
                index = (zr_int)i;
            if (res & ZR_GRAPH_CLICKED)
                col_index = (zr_int)i;
        }
        zr_graph_end(&layout, &graph);

        if (index != -1) {
            sprintf(buffer, "Value: %.2f", values[index]);
            zr_tooltip(&layout, buffer);
        }
        if (col_index != -1) {
            zr_layout_row_dynamic(&layout, 20, 1);
            zr_labelf(&layout, ZR_TEXT_LEFT, "Selected value: %.2f", values[col_index]);
        }

        /* line graph */
        index = -1;
        zr_layout_row_dynamic(&layout, 100, 1);
        zr_layout_peek(&bounds, &layout);
        zr_graph_begin(&layout, &graph, ZR_GRAPH_LINES, LEN(values), min_value, max_value);
        for (i = 0; i < LEN(values); ++i) {
            zr_flags res = zr_graph_push(&layout, &graph, values[i]);
            if (res & ZR_GRAPH_HOVERING)
                index = (zr_int)i;
            if (res & ZR_GRAPH_CLICKED)
                line_index = (zr_int)i;
        }
        zr_graph_end(&layout, &graph);

        if (index != -1) {
            sprintf(buffer, "Value: %.2f", values[index]);
            zr_tooltip(&layout, buffer);
        }
        if (line_index != -1) {
            zr_layout_row_dynamic(&layout, 20, 1);
            zr_labelf(&layout, ZR_TEXT_LEFT, "Selected value: %.2f", values[line_index]);
        }

        /* tree */
        zr_layout_row_dynamic(&layout, 220, 1);
        zr_tree_begin(&layout, &tree, NULL, 0, 20, scrollbar);
        upload_tree(test_tree, &tree, &test_tree->root);
        zr_tree_end(&layout, &tree, &scrollbar);

        zr_layout_pop(&layout);
    }

    if (zr_layout_push(&layout, ZR_LAYOUT_TAB, "Group", &group_state))
    {
        static zr_bool group_titlebar = zr_false;
        static zr_bool group_border = zr_true;
        static zr_bool group_no_scrollbar = zr_false;
        static zr_int group_width = 320;
        static zr_int group_height = 200;
        static struct zr_vec2 scrollbar;
        static zr_state width_active, height_active;
        struct zr_context tab;

        zr_flags group_flags = 0;
        if (group_border) group_flags |= ZR_WINDOW_BORDER;
        if (group_no_scrollbar) group_flags |= ZR_WINDOW_NO_SCROLLBAR;

        zr_layout_row_dynamic(&layout, 30, 3);
        zr_checkbox(&layout, "Titlebar", &group_titlebar);
        zr_checkbox(&layout, "Border", &group_border);
        zr_checkbox(&layout, "No Scrollbar", &group_no_scrollbar);

        zr_layout_row_begin(&layout, ZR_STATIC, 30, 2);
        zr_layout_row_push(&layout, 50);
        zr_label(&layout, "size:", ZR_TEXT_LEFT);
        zr_layout_row_push(&layout, 130);
        zr_spinner_int(&layout, 100, &group_width, 500, 10, &width_active);
        zr_layout_row_push(&layout, 130);
        zr_spinner_int(&layout, 100, &group_height, 500, 10, &height_active);
        zr_layout_row_end(&layout);

        zr_layout_row_static(&layout, (zr_float)group_height, (zr_size)group_width, 2);
        zr_group_begin(&layout, &tab, group_titlebar ? "Group" : NULL, group_flags, scrollbar);
        {
            int i = 0;
            static zr_bool selected[16];
            zr_layout_row_static(&tab, 18, 100, 1);
            for (i = 0; i < 16; ++i)
                zr_selectable(&tab, (selected[i]) ? "Selected": "Unselected", ZR_TEXT_CENTERED, &selected[i]);
        }
        zr_group_end(&layout, &tab, &scrollbar);
        zr_layout_pop(&layout);
    }

    if (zr_layout_push(&layout, ZR_LAYOUT_TAB, "Shelf", &shelf_state))
    {
        static const char *tabs[] = {"Up","Down","Left","Right"};
        static zr_size active = 0;
        static struct zr_vec2 scrollbar;
        struct zr_context tab;

        zr_layout_row_dynamic(&layout, 140, 1);
        zr_shelf_begin(&layout, &tab, tabs, LEN(tabs), &active, scrollbar);
        zr_layout_row_static(&tab, 50, 50, 4);
        if (active == 0) {
            zr_button_symbol(&tab, ZR_SYMBOL_TRIANGLE_UP, ZR_BUTTON_DEFAULT);
            zr_button_symbol(&tab, ZR_SYMBOL_TRIANGLE_DOWN, ZR_BUTTON_DEFAULT);
            zr_button_symbol(&tab, ZR_SYMBOL_TRIANGLE_LEFT, ZR_BUTTON_DEFAULT);
            zr_button_symbol(&tab, ZR_SYMBOL_TRIANGLE_RIGHT, ZR_BUTTON_DEFAULT);
        } else if (active == 1) {
            zr_button_symbol(&tab, ZR_SYMBOL_TRIANGLE_DOWN, ZR_BUTTON_DEFAULT);
            zr_button_symbol(&tab, ZR_SYMBOL_TRIANGLE_LEFT, ZR_BUTTON_DEFAULT);
            zr_button_symbol(&tab, ZR_SYMBOL_TRIANGLE_RIGHT, ZR_BUTTON_DEFAULT);
            zr_button_symbol(&tab, ZR_SYMBOL_TRIANGLE_UP, ZR_BUTTON_DEFAULT);
        } else if (active == 2) {
            zr_button_symbol(&tab, ZR_SYMBOL_TRIANGLE_LEFT, ZR_BUTTON_DEFAULT);
            zr_button_symbol(&tab, ZR_SYMBOL_TRIANGLE_RIGHT, ZR_BUTTON_DEFAULT);
            zr_button_symbol(&tab, ZR_SYMBOL_TRIANGLE_UP, ZR_BUTTON_DEFAULT);
            zr_button_symbol(&tab, ZR_SYMBOL_TRIANGLE_DOWN, ZR_BUTTON_DEFAULT);
        } else if (active == 3) {
            zr_button_symbol(&tab, ZR_SYMBOL_TRIANGLE_RIGHT, ZR_BUTTON_DEFAULT);
            zr_button_symbol(&tab, ZR_SYMBOL_TRIANGLE_UP, ZR_BUTTON_DEFAULT);
            zr_button_symbol(&tab, ZR_SYMBOL_TRIANGLE_DOWN, ZR_BUTTON_DEFAULT);
            zr_button_symbol(&tab, ZR_SYMBOL_TRIANGLE_LEFT, ZR_BUTTON_DEFAULT);
        }
        zr_shelf_end(&layout, &tab, &scrollbar);
        zr_layout_pop(&layout);
    }

    if (zr_layout_push(&layout, ZR_LAYOUT_TAB, "Splitter", &splitter_state))
    {
        static zr_state vertical_state = ZR_MINIMIZED;
        static zr_state horizontal_state = ZR_MINIMIZED;
        const struct zr_input *in = window->input;

        zr_layout_row_static(&layout, 20, 320, 1);
        zr_label(&layout, "Use slider and spinner to change tile size", ZR_TEXT_LEFT);
        zr_label(&layout, "Drag the space between tiles to change tile ratio", ZR_TEXT_LEFT);

        if (zr_layout_push(&layout, ZR_LAYOUT_NODE, "Vertical", &vertical_state))
        {
            static zr_float a = 100, b = 100, c = 100;
            static zr_state a_active, b_active, c_active;
            struct zr_rect bounds, total_space;
            struct zr_context sub;

            zr_float row_layout[5];
            total_space = zr_space(&layout);
            row_layout[0] = a;
            row_layout[1] = 8;
            row_layout[2] = b;
            row_layout[3] = 8;
            row_layout[4] = c;

            /* header */
            zr_layout_row_static(&layout, 30, 100, 3);
            zr_label(&layout, "left:", ZR_TEXT_LEFT);
            zr_slider_float(&layout, 10.0f, &a, 200.0f, 10.0f);
            zr_spinner_float(&layout, 10.0f, &a, 200.0f, 1.0f, &a_active);

            zr_label(&layout, "middle:", ZR_TEXT_LEFT);
            zr_slider_float(&layout, 10.0f, &b, 200.0f, 10.0f);
            zr_spinner_float(&layout, 10.0f, &b, 200.0f, 1.0f, &b_active);

            zr_label(&layout, "right:", ZR_TEXT_LEFT);
            zr_slider_float(&layout, 10.0f, &c, 200.0f, 10.0f);
            zr_spinner_float(&layout, 10.0f, &c, 200.0f, 1.0f, &c_active);

            /* tiles */
            zr_layout_row(&layout, ZR_STATIC, 200, 5, row_layout);
            zr_style_push_property(config, ZR_PROPERTY_ITEM_SPACING, zr_vec2(0, 4));

            /* left space */
            zr_group_begin(&layout, &sub, NULL, ZR_WINDOW_NO_SCROLLBAR|ZR_WINDOW_BORDER, zr_vec2(0,0));
            zr_group_end(&layout, &sub, NULL);

            /* scaler */
            zr_layout_peek(&bounds, &layout);
            zr_spacing(&layout, 1);
            if ((zr_input_is_mouse_hovering_rect(in, bounds) ||
                zr_input_is_mouse_prev_hovering_rect(in, bounds)) &&
                zr_input_is_mouse_down(in, ZR_BUTTON_LEFT))
            {
                a = row_layout[0] + in->mouse.delta.x;
                b = row_layout[2] - in->mouse.delta.x;
            }

            /* middle space */
            zr_group_begin(&layout, &sub, NULL, ZR_WINDOW_BORDER, zr_vec2(0,0));
            zr_group_end(&layout, &sub, NULL);

            /* scaler */
            zr_layout_peek(&bounds, &layout);
            zr_spacing(&layout, 1);
            if ((zr_input_is_mouse_hovering_rect(in, bounds) ||
                zr_input_is_mouse_prev_hovering_rect(in, bounds)) &&
                zr_input_is_mouse_down(in, ZR_BUTTON_LEFT))
            {
                b = (row_layout[2] + in->mouse.delta.x);
                c = (row_layout[4] - in->mouse.delta.x);
            }

            /* right space */
            zr_group_begin(&layout, &sub, NULL, ZR_WINDOW_BORDER, zr_vec2(0,0));
            zr_group_end(&layout, &sub, NULL);

            zr_style_pop_property(config);
            zr_layout_pop(&layout);
        }

        if (zr_layout_push(&layout, ZR_LAYOUT_NODE, "Horizontal", &horizontal_state))
        {
            static zr_float a = 100, b = 100, c = 100;
            static zr_state a_active, b_active, c_active;
            struct zr_context sub;
            struct zr_rect bounds;

            /* header */
            zr_layout_row_static(&layout, 30, 100, 3);
            zr_label(&layout, "top:", ZR_TEXT_LEFT);
            zr_slider_float(&layout, 10.0f, &a, 200.0f, 10.0f);
            zr_spinner_float(&layout, 10.0f, &a, 200.0f, 1.0f, &a_active);

            zr_label(&layout, "middle:", ZR_TEXT_LEFT);
            zr_slider_float(&layout, 10.0f, &b, 200.0f, 10.0f);
            zr_spinner_float(&layout, 10.0f, &b, 200.0f, 1.0f, &b_active);

            zr_label(&layout, "bottom:", ZR_TEXT_LEFT);
            zr_slider_float(&layout, 10.0f, &c, 200.0f, 10.0f);
            zr_spinner_float(&layout, 10.0f, &c, 200.0f, 1.0f, &c_active);

            zr_style_push_property(config, ZR_PROPERTY_ITEM_SPACING, zr_vec2(4, 0));

            /* top space */
            zr_layout_row_dynamic(&layout, a, 1);
            zr_group_begin(&layout, &sub, NULL, ZR_WINDOW_NO_SCROLLBAR|ZR_WINDOW_BORDER, zr_vec2(0,0));
            zr_group_end(&layout, &sub, NULL);

            /* scaler */
            zr_layout_row_dynamic(&layout, 8, 1);
            zr_layout_peek(&bounds, &layout);
            zr_spacing(&layout, 1);
            if ((zr_input_is_mouse_hovering_rect(in, bounds) ||
                zr_input_is_mouse_prev_hovering_rect(in, bounds)) &&
                zr_input_is_mouse_down(in, ZR_BUTTON_LEFT))
            {
                a = a + in->mouse.delta.y;
                b = b - in->mouse.delta.y;
            }

            /* middle space */
            zr_layout_row_dynamic(&layout, b, 1);
            zr_group_begin(&layout, &sub, NULL, ZR_WINDOW_NO_SCROLLBAR|ZR_WINDOW_BORDER, zr_vec2(0,0));
            zr_group_end(&layout, &sub, NULL);

            /* scaler */
            zr_layout_row_dynamic(&layout, 8, 1);
            zr_layout_peek(&bounds, &layout);
            zr_spacing(&layout, 1);
            if ((zr_input_is_mouse_hovering_rect(in, bounds) ||
                zr_input_is_mouse_prev_hovering_rect(in, bounds)) &&
                zr_input_is_mouse_down(in, ZR_BUTTON_LEFT))
            {
                b = b + in->mouse.delta.y;
                c = c - in->mouse.delta.y;
            }

            /* bottom space */
            zr_layout_row_dynamic(&layout, c, 1);
            zr_group_begin(&layout, &sub, NULL, ZR_WINDOW_NO_SCROLLBAR|ZR_WINDOW_BORDER, zr_vec2(0,0));
            zr_group_end(&layout, &sub, NULL);

            zr_style_pop_property(config);
            zr_layout_pop(&layout);
        }
        zr_layout_pop(&layout);
    }
    zr_end(&layout, window);
    return 1;
}

static void
copy_callback(zr_handle handle, const char *text, zr_size size)
{
    zr_char buffer[1024];
    UNUSED(handle);
    if (size >= 1023) return;
    memcpy(buffer, text, size);
    buffer[size] = '\0';
    clipboard_set(buffer);
}

static void
paste_callback(zr_handle handle, struct zr_edit_box *box)
{
    zr_size len;
    const char *text;
    UNUSED(handle);
    if (!clipboard_is_filled())return;
    text = clipboard_get();
    len = strlen(text);
    zr_edit_box_add(box, text, len);
}

static void
init_demo(struct demo *gui)
{
    gui->running = zr_true;
    gui->theme = THEME_WHITE;
    tree_init(&gui->tree);

    /* themes */
    zr_style_default(&gui->config_black, ZR_DEFAULT_ALL, &gui->font);
    zr_style_default(&gui->config_white, ZR_DEFAULT_ALL, &gui->font);

    gui->config_white.colors[ZR_COLOR_TEXT] = zr_rgba(70, 70, 70, 255);
    gui->config_white.colors[ZR_COLOR_TEXT_HOVERING] = zr_rgba(10, 10, 10, 255);
    gui->config_white.colors[ZR_COLOR_TEXT_ACTIVE] = zr_rgba(20, 20, 20, 255);
    gui->config_white.colors[ZR_COLOR_WINDOW] = zr_rgba(175, 175, 175, 255);
    gui->config_white.colors[ZR_COLOR_HEADER] = zr_rgba(175, 175, 175, 255);
    gui->config_white.colors[ZR_COLOR_BORDER] = zr_rgba(0, 0, 0, 255);
    gui->config_white.colors[ZR_COLOR_BUTTON] = zr_rgba(185, 185, 185, 255);
    gui->config_white.colors[ZR_COLOR_BUTTON_HOVER] = zr_rgba(170, 170, 170, 255);
    gui->config_white.colors[ZR_COLOR_BUTTON_ACTIVE] = zr_rgba(160, 160, 160, 255);
    gui->config_white.colors[ZR_COLOR_TOGGLE] = zr_rgba(150, 150, 150, 255);
    gui->config_white.colors[ZR_COLOR_TOGGLE_HOVER] = zr_rgba(120, 120, 120, 255);
    gui->config_white.colors[ZR_COLOR_TOGGLE_CURSOR] = zr_rgba(175, 175, 175, 255);
    gui->config_white.colors[ZR_COLOR_SELECTABLE] = zr_rgba(190, 190, 190, 255);
    gui->config_white.colors[ZR_COLOR_SELECTABLE_HOVER] = zr_rgba(150, 150, 150, 255);
    gui->config_white.colors[ZR_COLOR_SELECTABLE_TEXT] = zr_rgba(70, 70, 70, 255);
    gui->config_white.colors[ZR_COLOR_SLIDER] = zr_rgba(190, 190, 190, 255);
    gui->config_white.colors[ZR_COLOR_SLIDER_CURSOR] = zr_rgba(80, 80, 80, 255);
    gui->config_white.colors[ZR_COLOR_SLIDER_CURSOR_HOVER] = zr_rgba(70, 70, 70, 255);
    gui->config_white.colors[ZR_COLOR_SLIDER_CURSOR_ACTIVE] = zr_rgba(60, 60, 60, 255);
    gui->config_white.colors[ZR_COLOR_PROGRESS] = zr_rgba(190, 190, 190, 255);
    gui->config_white.colors[ZR_COLOR_PROGRESS_CURSOR] = zr_rgba(80, 80, 80, 255);
    gui->config_white.colors[ZR_COLOR_PROGRESS_CURSOR_HOVER] = zr_rgba(70, 70, 70, 255);
    gui->config_white.colors[ZR_COLOR_PROGRESS_CURSOR_ACTIVE] = zr_rgba(60, 60, 60, 255);
    gui->config_white.colors[ZR_COLOR_INPUT] = zr_rgba(150, 150, 150, 255);
    gui->config_white.colors[ZR_COLOR_INPUT_CURSOR] = zr_rgba(0, 0, 0, 255);
    gui->config_white.colors[ZR_COLOR_INPUT_TEXT] = zr_rgba(0, 0, 0, 255);
    gui->config_white.colors[ZR_COLOR_SPINNER] = zr_rgba(175, 175, 175, 255);
    gui->config_white.colors[ZR_COLOR_SPINNER_TRIANGLE] = zr_rgba(0, 0, 0, 255);
    gui->config_white.colors[ZR_COLOR_HISTO] = zr_rgba(160, 160, 160, 255);
    gui->config_white.colors[ZR_COLOR_HISTO_BARS] = zr_rgba(45, 45, 45, 255);
    gui->config_white.colors[ZR_COLOR_HISTO_NEGATIVE] = zr_rgba(255, 255, 255, 255);
    gui->config_white.colors[ZR_COLOR_HISTO_HIGHLIGHT] = zr_rgba( 255, 0, 0, 255);
    gui->config_white.colors[ZR_COLOR_PLOT] = zr_rgba(160, 160, 160, 255);
    gui->config_white.colors[ZR_COLOR_PLOT_LINES] = zr_rgba(45, 45, 45, 255);
    gui->config_white.colors[ZR_COLOR_PLOT_HIGHLIGHT] = zr_rgba(255, 0, 0, 255);
    gui->config_white.colors[ZR_COLOR_SCROLLBAR] = zr_rgba(180, 180, 180, 255);
    gui->config_white.colors[ZR_COLOR_SCROLLBAR_CURSOR] = zr_rgba(140, 140, 140, 255);
    gui->config_white.colors[ZR_COLOR_SCROLLBAR_CURSOR_HOVER] = zr_rgba(150, 150, 150, 255);
    gui->config_white.colors[ZR_COLOR_SCROLLBAR_CURSOR_ACTIVE] = zr_rgba(160, 160, 160, 255);
    gui->config_white.colors[ZR_COLOR_TABLE_LINES] = zr_rgba(100, 100, 100, 255);
    gui->config_white.colors[ZR_COLOR_TAB_HEADER] = zr_rgba(180, 180, 180, 255);
    gui->config_white.colors[ZR_COLOR_SHELF] = zr_rgba(150, 150, 150, 255);
    gui->config_white.colors[ZR_COLOR_SHELF_TEXT] = zr_rgba(0, 0, 0, 255);
    gui->config_white.colors[ZR_COLOR_SHELF_ACTIVE] = zr_rgba(180, 180, 180, 255);
    gui->config_white.colors[ZR_COLOR_SHELF_ACTIVE_TEXT] = zr_rgba(0, 0, 0, 255);
    gui->config_white.colors[ZR_COLOR_SCALER] = zr_rgba(100, 100, 100, 255);

    /* windows */
    zr_window_init(&gui->panel, zr_rect(30, 30, 400, 600),
        ZR_WINDOW_BORDER|ZR_WINDOW_MOVEABLE|ZR_WINDOW_SCALEABLE,
        &gui->queue, &gui->config_black, &gui->input);
    zr_window_init(&gui->sub, zr_rect(400, 50, 220, 180),
        ZR_WINDOW_BORDER|ZR_WINDOW_MOVEABLE|ZR_WINDOW_SCALEABLE,
        &gui->queue, &gui->config_black, &gui->input);
    zr_window_init(&gui->metrics, zr_rect(200, 400, 250, 300),
        ZR_WINDOW_BORDER|ZR_WINDOW_MOVEABLE|ZR_WINDOW_SCALEABLE,
        &gui->queue, &gui->config_black, &gui->input);

    {
        /* edit box */
        static char text[64];
        struct zr_clipboard clip;
        clip.userdata.ptr = 0;
        clip.paste = paste_callback;
        clip.copy = copy_callback;
        zr_edit_box_init_fixed(&gui->text, text, 64, &clip, 0);
    }
}

static void
run_demo(struct demo *gui)
{
    struct zr_context layout;
    struct zr_style *current = (gui->theme == THEME_BLACK) ? &gui->config_black : &gui->config_white;
    gui->running = show_test_window(&gui->panel, current, &gui->theme, &gui->tree, &gui->text);

    /* ussage example  */
    gui->sub.style = current;
    zr_begin(&layout, &gui->sub);
    {
        enum {EASY, HARD};
        static zr_int op = EASY;
        static zr_float value = 0.5f;
        zr_header(&layout, "Show", ZR_CLOSEABLE, 0, ZR_HEADER_LEFT);
        zr_layout_row_static(&layout, 30, 80, 1);
        if (zr_button_text(&layout, "button", ZR_BUTTON_DEFAULT)) {
            /* event handling */
        }
        zr_layout_row_dynamic(&layout, 30, 2);
        if (zr_option(&layout, "easy", op == EASY)) op = EASY;
        if (zr_option(&layout, "hard", op == HARD)) op = HARD;
        zr_layout_row_begin(&layout, ZR_STATIC, 30, 2);
        {
            zr_layout_row_push(&layout, 50);
            zr_label(&layout, "Volume:", ZR_TEXT_LEFT);
            zr_layout_row_push(&layout, 110);
            zr_slider_float(&layout, 0, &value, 1.0f, 0.1f);
        }
        zr_layout_row_end(&layout);
    }
    zr_end(&layout, &gui->sub);

    /* metrics window */
    gui->metrics.style = current;
    zr_begin(&layout, &gui->metrics);
    {
        static zr_state prim_state = ZR_MINIMIZED;
        static zr_state mem_state = ZR_MINIMIZED;
        struct zr_memory_status status;
        struct zr_command_stats *stats = &gui->panel.buffer.stats;
        zr_buffer_info(&status, &gui->queue.buffer);

        zr_header(&layout, "Metrics", ZR_CLOSEABLE, 0, ZR_HEADER_LEFT);
        if (zr_layout_push(&layout, ZR_LAYOUT_NODE, "Memory", &mem_state))
        {
            zr_layout_row_dynamic(&layout, 20, 2);
            zr_label(&layout,"Total:", ZR_TEXT_LEFT);
            zr_labelf(&layout, ZR_TEXT_LEFT, "%lu", status.size);
            zr_label(&layout,"Used:", ZR_TEXT_LEFT);
            zr_labelf(&layout, ZR_TEXT_LEFT, "%lu", status.allocated);
            zr_label(&layout,"Required:", ZR_TEXT_LEFT);
            zr_labelf(&layout, ZR_TEXT_LEFT, "%lu", status.needed);
            zr_label(&layout,"Calls:", ZR_TEXT_LEFT);
            zr_labelf(&layout, ZR_TEXT_LEFT, "%lu", status.calls);
            zr_layout_pop(&layout);
        }

        if (zr_layout_push(&layout, ZR_LAYOUT_NODE, "Primitives", &prim_state))
        {
            zr_layout_row_dynamic(&layout, 20, 2);
            zr_label(&layout,"Scissor:", ZR_TEXT_LEFT);
            zr_labelf(&layout, ZR_TEXT_LEFT, "%u", stats->scissors);
            zr_label(&layout,"Lines:", ZR_TEXT_LEFT);
            zr_labelf(&layout, ZR_TEXT_LEFT, "%u", stats->lines);
            zr_label(&layout,"Curves:", ZR_TEXT_LEFT);
            zr_labelf(&layout, ZR_TEXT_LEFT, "%u", stats->curves);
            zr_label(&layout,"Rectangles:", ZR_TEXT_LEFT);
            zr_labelf(&layout, ZR_TEXT_LEFT, "%u", stats->rectangles);
            zr_label(&layout,"Circles:", ZR_TEXT_LEFT);
            zr_labelf(&layout, ZR_TEXT_LEFT, "%u", stats->circles);
            zr_label(&layout,"Triangles:", ZR_TEXT_LEFT);
            zr_labelf(&layout, ZR_TEXT_LEFT, "%u", stats->triangles);
            zr_label(&layout,"Images:", ZR_TEXT_LEFT);
            zr_labelf(&layout, ZR_TEXT_LEFT, "%u", stats->images);
            zr_label(&layout,"Text:", ZR_TEXT_LEFT);
            zr_labelf(&layout, ZR_TEXT_LEFT, "%u", stats->text);
            zr_label(&layout,"Glyphes:", ZR_TEXT_LEFT);
            zr_labelf(&layout, ZR_TEXT_LEFT, "%u", stats->glyphes);
            zr_layout_pop(&layout);
        }
    }
    zr_end(&layout, &gui->metrics);
}

