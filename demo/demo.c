#define MAX_BUFFER  64
#define MAX_MEMORY  (16 * 1024)
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

struct tree_node {
    gui_tree_node_state state;
    const char *name;
    struct tree_node *parent;
    struct tree_node *children[8];
    int count;
};

struct test_tree {
    struct tree_node root;
    struct tree_node *clipboard[16];
    int count;
};

struct state {
    /* input buffer */
    gui_char input_buffer[MAX_BUFFER];
    struct gui_edit_box input;
    gui_char in_buf[MAX_BUFFER];
    gui_size in_len;
    gui_bool in_active;

    /* widgets state */
    gui_size menu_item;
    gui_bool scaleable;
    gui_bool checkbox;
    gui_float slider;
    gui_size progressbar;
    gui_int spinner_int;
    gui_float spinner_float;
    gui_bool spinner_active;
    gui_size item_current;
    gui_size shelf_selection;
    gui_bool toggle;
    gui_int option;

    gui_int op;
    gui_size cur;

    /* tree */
    struct test_tree tree;
    struct tree_node nodes[8];

    /* tabs */
    enum gui_node_state config_tab;
    enum gui_node_state widget_tab;
    enum gui_node_state style_tab;
    enum gui_node_state round_tab;
    enum gui_node_state color_tab;
    enum gui_node_state flag_tab;

    /* scrollbars */
    struct gui_vec2 shelf_scrollbar;
    struct gui_vec2 table_scrollbar;
    struct gui_vec2 tree_scrollbar;

    /* color picker */
    gui_bool picker_active;
    gui_bool spinner_r_active;
    gui_bool spinner_g_active;
    gui_bool spinner_b_active;
    gui_bool spinner_a_active;
    gui_size current_color;
    struct gui_color color;

};

struct demo_gui {
    gui_bool running;
    void *memory;
    struct gui_command_queue queue;
    struct gui_config config;
    struct gui_font font;
    struct gui_panel panel;
    struct gui_panel sub;
    struct state state;
};

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
upload_tree(struct test_tree *base, struct gui_tree *tree, struct tree_node *node)
{
    int i = 0, n = 0;
    enum gui_tree_node_operation op;
    if (node->count) {
        i = 0;
        op = gui_panel_tree_begin_node(tree, node->name, &node->state);
        while (i < node->count)
            i += upload_tree(base, tree, node->children[i]);
        gui_panel_tree_end_node(tree);
    }
    else op = gui_panel_tree_leaf(tree, node->name, &node->state);

    switch (op) {
    case GUI_NODE_NOP: break;
    case GUI_NODE_CUT:
        tree_remove_node(node->parent, node);
        tree_push_node(base, node);
        return 0;
    case GUI_NODE_DELETE:
        tree_remove_node(node->parent, node); break;
        return 0;
    case GUI_NODE_PASTE:
        i = 0; n = base->count;
        while (i < n) {
            tree_add_node(node, tree_pop_node(base));
            i++;
        }
    case GUI_NODE_CLONE:
    default:break;
    }
    return 1;
}

static void
widget_panel(struct gui_panel_layout *panel, struct state *demo)
{
    const char *items[] = {"Fist", "Pistol", "Shotgun", "Railgun", "BFG"};

    /* Labels */
    gui_panel_row_dynamic(panel, 30, 1);
    demo->scaleable = gui_panel_check(panel, "Scaleable Layout", demo->scaleable);

    if (!demo->scaleable)
        gui_panel_row_static(panel, 30, 150, 1);
    gui_panel_label(panel, "text left", GUI_TEXT_LEFT);
    gui_panel_label(panel, "text center", GUI_TEXT_CENTERED);
    gui_panel_label(panel, "text right", GUI_TEXT_RIGHT);

    /* Buttons */
    if (gui_panel_button_text(panel, "button", GUI_BUTTON_DEFAULT))
        fprintf(stdout, "button pressed!\n");
    if (gui_panel_button_text_triangle(panel, GUI_RIGHT, "next", GUI_TEXT_LEFT, GUI_BUTTON_DEFAULT))
        fprintf(stdout, "right triangle button pressed!\n");
    if (gui_panel_button_text_triangle(panel,GUI_LEFT,"previous",GUI_TEXT_RIGHT,GUI_BUTTON_DEFAULT))
        fprintf(stdout, "left triangle button pressed!\n");

    demo->toggle = gui_panel_button_toggle(panel, "toggle", demo->toggle);
    demo->checkbox = gui_panel_check(panel, "checkbox", demo->checkbox);

    if (!demo->scaleable)
        gui_panel_row_static(panel, 30, 75, 1);
    else  gui_panel_row_dynamic(panel, 30, 2);

    if (gui_panel_option(panel, "option 0", demo->option == 0)) demo->option = 0;
    if (gui_panel_option(panel, "option 1", demo->option == 1)) demo->option = 1;

    {
        /* templated row layout */
        char buffer[MAX_BUFFER];
        if (demo->scaleable) {
            const gui_float ratio[] = {0.8f, 0.2f};
            gui_panel_row(panel, GUI_DYNAMIC, 30, 2, ratio);
            demo->slider = gui_panel_slider(panel, 0, demo->slider, 10, 1.0f);
            sprintf(buffer, "%.2f", demo->slider);
            gui_panel_label(panel, buffer, GUI_TEXT_LEFT);
            demo->progressbar = gui_panel_progress(panel, demo->progressbar, 100, gui_true);
            sprintf(buffer, "%lu", demo->progressbar);
            gui_panel_label(panel, buffer, GUI_TEXT_LEFT);
        } else {
            const gui_float ratio[] = {150.0f, 30.0f};
            gui_panel_row(panel, GUI_STATIC, 30, 2, ratio);
            demo->slider = gui_panel_slider(panel, 0, demo->slider, 10, 1.0f);
            sprintf(buffer, "%.2f", demo->slider);
            gui_panel_label(panel, buffer, GUI_TEXT_LEFT);
            demo->progressbar = gui_panel_progress(panel, demo->progressbar, 100, gui_true);
            sprintf(buffer, "%lu", demo->progressbar);
            gui_panel_label(panel, buffer, GUI_TEXT_LEFT);
        }
    }

    if (!demo->scaleable)
        gui_panel_row_static(panel, 30, 150, 1);
    else gui_panel_row_dynamic(panel, 30, 1);

    demo->item_current = gui_panel_selector(panel, items, LEN(items), demo->item_current);
    demo->spinner_int = gui_panel_spinner_int(panel, 0, demo->spinner_int, 250, 10, &demo->spinner_active);
    demo->spinner_float = gui_panel_spinner_float(panel, 0.0f, demo->spinner_float,
                                                    1.0f, 0.1f, &demo->spinner_active);
    demo->in_len = gui_panel_edit(panel, demo->in_buf, demo->in_len, MAX_BUFFER,
                                    &demo->in_active, NULL, GUI_INPUT_DEFAULT);

    if (demo->scaleable) {
        gui_panel_row_begin(panel, GUI_DYNAMIC, 30, 2);
        {
            gui_panel_row_push(panel, 0.7f);
            gui_panel_editbox(panel, &demo->input);
            gui_panel_row_push(panel, 0.3f);
            if (gui_panel_button_text(panel, "submit", GUI_BUTTON_DEFAULT)) {
                gui_edit_box_clear(&demo->input);
                fprintf(stdout, "command executed!\n");
            }
        }
        gui_panel_row_end(panel);
    } else {
        gui_panel_row_begin(panel, GUI_STATIC, 30, 2);
        {
            gui_panel_row_push(panel, 100);
            gui_panel_editbox(panel, &demo->input);
            gui_panel_row_push(panel, 80);
            if (gui_panel_button_text(panel, "submit", GUI_BUTTON_DEFAULT)) {
                gui_edit_box_clear(&demo->input);
                fprintf(stdout, "command executed!\n");
            }
        }
        gui_panel_row_end(panel);
    }
}

static void
graph_panel(struct gui_panel_layout *panel, gui_size current)
{
    enum {COL, PLOT};
    static const gui_float values[]={8.0f,15.0f,20.0f,12.0f,30.0f,12.0f,35.0f,40.0f,20.0f};
    gui_panel_row_dynamic(panel, 100, 1);
    if (current == COL) {
        gui_panel_graph(panel, GUI_GRAPH_COLUMN, values, LEN(values), 0);
    } else {
        gui_panel_graph(panel, GUI_GRAPH_LINES, values, LEN(values), 0);
    }
}

static void
table_panel(struct gui_panel_layout *panel)
{
    gui_size i = 0;
    const char *table[] = {"Move forward", "w", "Move back", "s", "Move left", "a",
        "Move right", "d", "Jump", "SPACE", "Duck", "CTRL"};
    gui_panel_table_begin(panel, GUI_TABLE_HHEADER, 30, 2);
    gui_panel_label_colored(panel, "MOVEMENT", GUI_TEXT_CENTERED, gui_rgba(178, 122, 1, 255));
    gui_panel_label_colored(panel, "KEY/BUTTON", GUI_TEXT_CENTERED, gui_rgba(178, 122, 1, 255));
    for (i = 0; i < LEN(table); i += 2) {
        gui_panel_table_row(panel);
        gui_panel_label(panel, table[i], GUI_TEXT_LEFT);
        gui_panel_label(panel, table[i+1], GUI_TEXT_CENTERED);
    }
    gui_panel_table_end(panel);
}

static void
update_menu(struct gui_panel_layout *layout, struct state *win, struct gui_config *config)
{
    int i = 0;
    enum level_id {LEVEL_MENU,LEVEL_FILE,LEVEL_OPEN,LEVEL_EDIT};
    enum item_id {ITEM_FILE, ITEM_EDIT,
        ITEM_FILE_BACK, ITEM_FILE_OPEN, ITEM_FILE_CLOSE, ITEM_FILE_QUIT,
        ITEM_FILE_OPEN_BACK, ITEM_FILE_OPEN_EXE, ITEM_FILE_OPEN_SRC,
        ITEM_EDIT_BACK, ITEM_EDIT_COPY, ITEM_EDIT_CUT, ITEM_EDIT_PASTE, ITEM_EDIT_DELETE};
    enum combi_id {MENU_FILE, MENU_EDIT,
        FILE_BACK, FILE_OPEN, FILE_CLOSE, FILE_QUIT,
        OPEN_BACK, OPEN_EXE, OPEN_SRC,
        EDIT_BACK, EDIT_COPY, EDIT_CUT, EDIT_PASTE, EDIT_DELETE};

    struct level {const enum level_id id; const int items; enum combi_id list;};
    struct item {const enum item_id id; const char *name; const enum level_id lvl, next;};
    struct combi {const enum combi_id id; const enum level_id level; const enum item_id item;};

    static const struct level levels[] = {
        {LEVEL_MENU, 2, MENU_FILE},
        {LEVEL_FILE, 4, FILE_BACK},
        {LEVEL_OPEN, 3, OPEN_BACK},
        {LEVEL_EDIT, 5, EDIT_BACK},
    };
    static const struct item items[] = {
        {ITEM_FILE, "FILE", LEVEL_MENU, LEVEL_FILE},
        {ITEM_EDIT, "EDIT", LEVEL_MENU, LEVEL_EDIT},
        {ITEM_FILE_BACK, "BACK", LEVEL_FILE, LEVEL_MENU},
        {ITEM_FILE_OPEN, "OPEN", LEVEL_FILE, LEVEL_OPEN},
        {ITEM_FILE_CLOSE, "CLOSE", LEVEL_FILE, LEVEL_MENU},
        {ITEM_FILE_QUIT, "QUIT", LEVEL_FILE, LEVEL_MENU},
        {ITEM_FILE_OPEN_BACK, "BACK", LEVEL_OPEN, LEVEL_FILE},
        {ITEM_FILE_OPEN_EXE, "IMAGE", LEVEL_OPEN, LEVEL_MENU},
        {ITEM_FILE_OPEN_SRC, "TEXT", LEVEL_OPEN, LEVEL_MENU},
        {ITEM_EDIT_BACK, "BACK", LEVEL_EDIT, LEVEL_MENU},
        {ITEM_EDIT_COPY, "COPY", LEVEL_EDIT, LEVEL_MENU},
        {ITEM_EDIT_CUT, "CUT", LEVEL_EDIT, LEVEL_MENU},
        {ITEM_EDIT_PASTE, "PASTE", LEVEL_EDIT, LEVEL_MENU},
        {ITEM_EDIT_DELETE, "DEL", LEVEL_EDIT, LEVEL_MENU}
    };
    static const struct combi combis[] = {
        /* main menu level */
        {MENU_FILE, LEVEL_MENU, ITEM_FILE},
        {MENU_EDIT, LEVEL_MENU, ITEM_EDIT},
        /* file menu level */
        {FILE_BACK, LEVEL_FILE, ITEM_FILE_BACK},
        {FILE_OPEN, LEVEL_FILE, ITEM_FILE_OPEN},
        {FILE_CLOSE, LEVEL_FILE, ITEM_FILE_CLOSE},
        {FILE_QUIT, LEVEL_FILE, ITEM_FILE_QUIT},
        /* open file options menu level */
        {OPEN_BACK, LEVEL_OPEN, ITEM_FILE_OPEN_BACK},
        {OPEN_EXE, LEVEL_OPEN, ITEM_FILE_OPEN_EXE},
        {OPEN_SRC, LEVEL_OPEN, ITEM_FILE_OPEN_SRC},
        /* edit main level*/
        {EDIT_BACK, LEVEL_EDIT, ITEM_EDIT_BACK},
        {EDIT_COPY, LEVEL_EDIT, ITEM_EDIT_COPY},
        {EDIT_CUT, LEVEL_EDIT, ITEM_EDIT_CUT},
        {EDIT_PASTE, LEVEL_EDIT, ITEM_EDIT_PASTE},
        {EDIT_DELETE, LEVEL_EDIT, ITEM_EDIT_DELETE}
    };

    {
        /* calculate column row count to fit largets menu item  */
        gui_int max = 0;
        for (i = 0; i < (int)LEN(levels); ++i) {
            if (levels[0].items > max)
                max = levels[0].items;
        }
        gui_panel_row_dynamic(layout, 18, 5);
    }

    /* output current menu level entries */
    gui_panel_menu_begin(layout);
    {
        const struct level *lvl = &levels[win->menu_item];
        const struct combi *iter = &combis[lvl->list];
        gui_config_push_color(config, GUI_COLOR_BUTTON_BORDER, 45, 45, 45, 250);
        gui_config_push_property(config, GUI_PROPERTY_ITEM_SPACING, 0, 4.0f);
        for (i = 0; i < lvl->items; ++i) {
            const struct item *item = &items[iter->item];
            if (gui_panel_button_text(layout, item->name, GUI_BUTTON_DEFAULT)) {
                if (item->id == ITEM_FILE_OPEN_EXE) {
                    fprintf(stdout, "open program file button pressed!\n");
                } else if (item->id == ITEM_FILE_OPEN_SRC) {
                    fprintf(stdout, "open source file button pressed!\n");
                } else if (item->id == ITEM_FILE_CLOSE) {
                    fprintf(stdout, "close button pressed!\n");
                } else if (item->id == ITEM_FILE_QUIT) {
                    fprintf(stdout, "quit button pressed!\n");
                } else if (item->id == ITEM_EDIT_COPY) {
                    fprintf(stdout, "copy button pressed!\n");
                } else if (item->id == ITEM_EDIT_CUT) {
                    fprintf(stdout, "cut button pressed!\n");
                } else if (item->id == ITEM_EDIT_PASTE) {
                    fprintf(stdout, "paste button pressed!\n");
                } else if (item->id == ITEM_EDIT_DELETE) {
                    fprintf(stdout, "delete button pressed!\n");
                }
                win->menu_item = item->next;
            }
            iter++;
        }
        gui_config_pop_color(config);
        gui_config_pop_property(config);
    }
    gui_panel_menu_end(layout);
}

static void
update_flags(struct gui_panel_layout *panel)
{
    gui_size n = 0;
    gui_flags res = 0;
    gui_flags i = 0x01;
    const char *options[]={"Hidden","Border","Header Border", "Moveable","Scaleable", "Minimized"};
    gui_panel_row_dynamic(panel, 30, 2);
    do {
        if (gui_panel_check(panel,options[n++],(panel->flags & i)?gui_true:gui_false))
            res |= i;
        i = i << 1;
    } while (i <= GUI_PANEL_MINIMIZED);
    panel->flags = res;
}

static void
properties_tab(struct gui_panel_layout *panel, struct gui_config *config)
{
    int i = 0;
    const char *properties[] = {"item spacing:", "item padding:", "panel padding:",
        "scaler size:", "scrollbar:"};

    gui_panel_row_dynamic(panel, 30, 3);
    for (i = 0; i <= GUI_PROPERTY_SCROLLBAR_SIZE; ++i) {
        gui_int tx, ty;
        gui_panel_label(panel, properties[i], GUI_TEXT_LEFT);
        tx = gui_panel_spinner_int(panel,0,(gui_int)config->properties[i].x, 20, 1, NULL);
        ty = gui_panel_spinner_int(panel,0,(gui_int)config->properties[i].y, 20, 1, NULL);
        config->properties[i].x = (float)tx;
        config->properties[i].y = (float)ty;
    }
}

static void
round_tab(struct gui_panel_layout *panel, struct gui_config *config)
{
    int i = 0;
    const char *rounding[] = {"panel:", "button:", "checkbox:", "progress:", "input: ",
        "graph:", "scrollbar:"};

    gui_panel_row_dynamic(panel, 30, 2);
    for (i = 0; i < GUI_ROUNDING_MAX; ++i) {
        gui_int t;
        gui_panel_label(panel, rounding[i], GUI_TEXT_LEFT);
        t = gui_panel_spinner_int(panel,0,(gui_int)config->rounding[i], 20, 1, NULL);
        config->rounding[i] = (float)t;
    }
}

static struct gui_color
color_picker(struct gui_panel_layout *panel, struct state *control,
    const char *name, struct gui_color color)
{
    int i;
    gui_byte *iter;
    gui_bool *active[4];

    active[0] = &control->spinner_r_active;
    active[1] = &control->spinner_g_active;
    active[2] = &control->spinner_b_active;
    active[3] = &control->spinner_a_active;

    gui_panel_row_dynamic(panel, 30, 2);
    gui_panel_label(panel, name, GUI_TEXT_LEFT);
    gui_panel_button_color(panel, color, GUI_BUTTON_DEFAULT);

    iter = &color.r;
    gui_panel_row_dynamic(panel, 30, 2);
    for (i = 0; i < 4; ++i, iter++) {
        gui_float t = *iter;
        t = gui_panel_slider(panel, 0, t, 255, 10);
        *iter = (gui_byte)t;
        *iter = (gui_byte)gui_panel_spinner_int(panel, 0, *iter, 255, 1, active[i]);
    }
    return color;
}

static void
color_tab(struct gui_panel_layout *panel, struct state *control, struct gui_config *config)
{
    gui_size i = 0;
    static const char *labels[] = {"Text:", "Panel:", "Header:", "Border:", "Button:",
        "Button Border:", "Button Hovering:", "Button Toggle:", "Button Hovering Text:",
        "Check:", "Check BG:", "Check Active:", "Option:", "Option BG:", "Option Active:",
        "Slider:", "Slider bar:", "Slider boder:","Slider cursor:", "Progress:", "Progress Cursor:",
        "Editbox:", "Editbox cursor:", "Editbox Border:", "Editbox Text:",
        "Spinner:", "Spinner Border:", "Spinner Triangle:", "Spinner Text:",
        "Selector:", "Selector Border:", "Selector Triangle:", "Selector Text:",
        "Histo:", "Histo Bars:", "Histo Negative:", "Histo Hovering:", "Plot:", "Plot Lines:",
        "Plot Hightlight:", "Scrollbar:", "Scrollbar Cursor:", "Scrollbar Border:",
        "Table lines:", "Tab header", "Tab border",
        "Shelf:", "Shelf Text:", "Shelf Active:", "Shelf Active Text:", "Scaler:",
        "Tiled Scaler"
    };

    if (control->picker_active) {
        control->color = color_picker(panel,control,labels[control->current_color], control->color);
        gui_panel_row_dynamic(panel, 30, 3);
        gui_panel_spacing(panel, 1);
        if (gui_panel_button_text(panel, "ok", GUI_BUTTON_DEFAULT)) {
            config->colors[control->current_color] = control->color;
            control->picker_active = gui_false;
        }
        if (gui_panel_button_text(panel, "cancel", GUI_BUTTON_DEFAULT))
            control->picker_active = gui_false;
    } else {
        gui_panel_row_dynamic(panel, 30, 2);
        for (i = 0; i < GUI_COLOR_COUNT; ++i) {
            struct gui_color c = config->colors[i];
            gui_panel_label(panel, labels[i], GUI_TEXT_LEFT);
            if (gui_panel_button_color(panel, c, GUI_BUTTON_DEFAULT)) {
                if (!control->picker_active) {
                    control->picker_active = gui_true;
                    control->color = config->colors[i];
                    control->current_color = i;
                } else continue;
            }
        }
    }
}

static void
copy(gui_handle handle, const char *text, gui_size size)
{
    gui_char buffer[1024];
    UNUSED(handle);
    if (size >= 1023) return;
    memcpy(buffer, text, size);
    buffer[size] = '\0';
    clipboard_set(buffer);
}

static void
paste(gui_handle handle, struct gui_edit_box *box)
{
    gui_size len;
    const char *text;
    UNUSED(handle);
    if (!clipboard_is_filled()) return;
    text = clipboard_get();
    len = strlen(text);
    gui_edit_box_add(box, text, len);
}

static void
init_demo(struct demo_gui *gui, struct gui_font *font)
{
    struct gui_config *config = &gui->config;
    struct state *win = &gui->state;
    struct gui_clipboard clip;
    gui->font = *font;
    gui->running = gui_true;

    clip.userdata.ptr = NULL,
    clip.copy = copy;
    clip.paste = paste;

    /* panel */
    gui_command_queue_init_fixed(&gui->queue, gui->memory, MAX_MEMORY);
    gui_config_default(config, GUI_DEFAULT_ALL, font);
    gui_panel_init(&gui->panel, 30, 30, 280, 530,
        GUI_PANEL_BORDER|GUI_PANEL_MOVEABLE|GUI_PANEL_SCALEABLE, &gui->queue, config);
    gui_panel_init(&gui->sub, 50, 50, 220, 180,
        GUI_PANEL_BORDER|GUI_PANEL_MOVEABLE|GUI_PANEL_SCALEABLE,
        &gui->queue, config);

    /* widget state */
    gui_edit_box_init_fixed(&win->input, win->input_buffer, MAX_BUFFER, &clip, NULL);
    win->config_tab = GUI_MINIMIZED;
    win->widget_tab = GUI_MINIMIZED;
    win->style_tab = GUI_MINIMIZED;
    win->round_tab = GUI_MINIMIZED;
    win->color_tab = GUI_MINIMIZED;
    win->flag_tab = GUI_MINIMIZED;
    win->scaleable = gui_true;
    win->slider = 2.0f;
    win->progressbar = 50;
    win->spinner_int = 100;
    win->spinner_float = 0.5f;

    {
        /* test tree data */
        struct test_tree *tree = &win->tree;
        tree->root.state = GUI_NODE_ACTIVE;
        tree->root.name = "Primitives";
        tree->root.parent = NULL;
        tree->root.count = 2;
        tree->root.children[0] = &win->nodes[0];
        tree->root.children[1] = &win->nodes[4];

        win->nodes[0].state = 0;
        win->nodes[0].name = "Boxes";
        win->nodes[0].parent = &tree->root;
        win->nodes[0].count = 2;
        win->nodes[0].children[0] = &win->nodes[1];
        win->nodes[0].children[1] = &win->nodes[2];

        win->nodes[1].state = 0;
        win->nodes[1].name = "Box0";
        win->nodes[1].parent = &win->nodes[0];
        win->nodes[1].count = 0;

        win->nodes[2].state = 0;
        win->nodes[2].name = "Box1";
        win->nodes[2].parent = &win->nodes[0];
        win->nodes[2].count = 0;

        win->nodes[4].state = GUI_NODE_ACTIVE;
        win->nodes[4].name = "Cylinders";
        win->nodes[4].parent = &tree->root;
        win->nodes[4].count = 2;
        win->nodes[4].children[0] = &win->nodes[5];
        win->nodes[4].children[1] = &win->nodes[6];

        win->nodes[5].state = 0;
        win->nodes[5].name = "Cylinder0";
        win->nodes[5].parent = &win->nodes[4];
        win->nodes[5].count = 0;

        win->nodes[6].state = 0;
        win->nodes[6].name = "Cylinder1";
        win->nodes[6].parent = &win->nodes[4];
        win->nodes[6].count = 0;
    }
}

static void
run_demo(struct demo_gui *gui, struct gui_input *input)
{
    struct gui_panel_layout layout;
    struct state *state = &gui->state;
    struct gui_panel_layout tab;
    struct gui_config *config = &gui->config;
    static const char *shelfs[] = {"Histogram", "Lines"};
    enum {EASY, HARD};

    gui_panel_begin(&layout, &gui->panel, input);
    {
        /* Header + Menubar  */
        gui->running = !gui_panel_header(&layout, "Demo",
            GUI_CLOSEABLE|GUI_MINIMIZABLE, GUI_CLOSEABLE, GUI_HEADER_RIGHT);
        update_menu(&layout, state, config);

        /* Panel style configuration  */
        if (gui_panel_layout_push(&layout, GUI_LAYOUT_TAB, "Style", &state->config_tab))
        {
            if (gui_panel_layout_push(&layout, GUI_LAYOUT_NODE, "Options", &state->flag_tab)) {
                update_flags(&layout);
                gui_panel_layout_pop(&layout);
            }
            if (gui_panel_layout_push(&layout, GUI_LAYOUT_NODE, "Properties", &state->style_tab)) {
                properties_tab(&layout, config);
                gui_panel_layout_pop(&layout);
            }
            if (gui_panel_layout_push(&layout, GUI_LAYOUT_NODE, "Rounding", &state->round_tab)) {
                round_tab(&layout, config);
                gui_panel_layout_pop(&layout);
            }
            if (gui_panel_layout_push(&layout, GUI_LAYOUT_NODE, "Color", &state->color_tab)) {
                color_tab(&layout, state, config);
                gui_panel_layout_pop(&layout);
            }
            gui_panel_layout_pop(&layout);
        }

        /* Widgets examples */
        if (gui_panel_layout_push(&layout, GUI_LAYOUT_TAB, "Widgets", &state->widget_tab)) {
            widget_panel(&layout, state);
            gui_panel_layout_pop(&layout);
        }

        /* Shelf + Graphes  */
        gui_panel_row_dynamic(&layout, 180, 1);
        state->shelf_selection = gui_panel_shelf_begin(&layout, &tab, shelfs,
            LEN(shelfs), state->shelf_selection, state->shelf_scrollbar);
        graph_panel(&tab, state->shelf_selection);
        state->shelf_scrollbar = gui_panel_shelf_end(&layout, &tab);

        /* Tables */
        gui_panel_row_dynamic(&layout, 180, 1);
        gui_panel_group_begin(&layout, &tab, "Table", state->table_scrollbar);
        table_panel(&tab);
        state->table_scrollbar = gui_panel_group_end(&layout, &tab);

        {
            /* Tree */
            struct gui_tree tree;
            gui_panel_row_dynamic(&layout, 250, 1);
            gui_panel_tree_begin(&layout, &tree, "Tree", 20, state->tree_scrollbar);
            upload_tree(&state->tree, &tree, &state->tree.root);
            state->tree_scrollbar = gui_panel_tree_end(&layout, &tree);
        }
    }
    gui_panel_end(&layout, &gui->panel);

    gui_panel_begin(&layout, &gui->sub, input);
    {
        const char *items[] = {"Fist", "Pistol", "Railgun", "BFG"};
        gui_panel_header(&layout, "Demo", GUI_CLOSEABLE, 0, GUI_HEADER_LEFT);
        gui_panel_row_dynamic(&layout, 30, 1);
        if (gui_panel_button_text(&layout, "button", GUI_BUTTON_DEFAULT)) {
            /* event handling */
        }
        gui_panel_row_dynamic(&layout, 30, 2);
        if (gui_panel_option(&layout, "easy", state->op == EASY)) state->op = EASY;
        if (gui_panel_option(&layout, "hard", state->op == HARD)) state->op = HARD;
        gui_panel_label(&layout, "Weapon:", GUI_TEXT_LEFT);
        state->cur = gui_panel_selector(&layout, items, LEN(items), state->cur);
    }
    gui_panel_end(&layout, &gui->sub);
}

