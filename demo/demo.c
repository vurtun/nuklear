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

    /* menubar state */
    gui_bool file_open;
    gui_bool edit_open;

    /* widgets state */
    gui_size menu_item;
    gui_bool scaleable;
    gui_bool checkbox;
    gui_float slider;
    gui_size progressbar;
    gui_int spinner;
    gui_bool spinner_active;
    gui_size item_current;
    gui_size shelf_selection;
    gui_bool toggle;
    gui_int option;

    gui_bool popup;
    gui_size cur;
    gui_size op;

    /* combo */
    struct gui_color combo_color;
    gui_size combo_prog[4];
    gui_bool combo_sel[4];
    gui_size sel_item;
    gui_bool col_act;
    gui_bool sel_act;
    gui_bool box_act;
    gui_bool prog_act;

    /* tree */
    struct test_tree tree;
    struct tree_node nodes[8];

    /* tabs */
    enum gui_node_state config_tab;
    enum gui_node_state widget_tab;
    enum gui_node_state combo_tab;
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
    const struct gui_input *input;
    struct gui_command_queue queue;
    struct gui_config config;
    struct gui_font font;
    struct gui_panel panel;
    struct gui_panel sub;
    struct state state;
    gui_size w, h;
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
        demo->popup = gui_true;
    if (gui_panel_button_text_triangle(panel, GUI_RIGHT, "next", GUI_TEXT_LEFT, GUI_BUTTON_DEFAULT))
        fprintf(stdout, "right triangle button pressed!\n");
    if (gui_panel_button_text_triangle(panel,GUI_LEFT,"previous",GUI_TEXT_RIGHT,GUI_BUTTON_DEFAULT))
        fprintf(stdout, "left triangle button pressed!\n");

    demo->toggle = gui_panel_button_toggle(panel, "toggle", demo->toggle);
    demo->checkbox = gui_panel_check(panel, "checkbox", demo->checkbox);

    if (!demo->scaleable)
        gui_panel_row_static(panel, 30, 75, 2);
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
    gui_panel_combo(panel, items, LEN(items), &demo->sel_item, 30, &demo->sel_act, gui_vec2(0,0));
    {
        /* progressbar combobox  */
        gui_int sum;
        gui_char buffer[64];
        struct gui_panel_layout combo;
        memset(&combo, 0, sizeof(combo));

        sum = (gui_int)(demo->combo_prog[0] + demo->combo_prog[1]);
        sum += (gui_int)(demo->combo_prog[2] + demo->combo_prog[3]);
        sprintf(buffer, "%d", sum);
        gui_panel_combo_begin(panel, &combo, buffer, &demo->prog_act, gui_vec2(0,0));
        {
            gui_panel_row_dynamic(&combo, 30, 1);
            demo->combo_prog[0] = gui_panel_progress(&combo, demo->combo_prog[0], 100, gui_true);
            demo->combo_prog[1] = gui_panel_progress(&combo, demo->combo_prog[1], 100, gui_true);
            demo->combo_prog[2] = gui_panel_progress(&combo, demo->combo_prog[2], 100, gui_true);
            demo->combo_prog[3] = gui_panel_progress(&combo, demo->combo_prog[3], 100, gui_true);
        }
        gui_panel_combo_end(panel, &combo);
    }
    {
        /* color slider progressbar */
        gui_char buffer[32];
        struct gui_panel_layout combo;
        memset(&combo, 0, sizeof(combo));
        sprintf(buffer, "#%02x%02x%02x%02x", demo->combo_color.r, demo->combo_color.g,
                demo->combo_color.b, demo->combo_color.a);
        gui_panel_combo_begin(panel, &combo, buffer,  &demo->col_act, gui_vec2(0,0));
        {
            int i;
            const char *color_names[] = {"R:", "G:", "B:", "A:"};
            gui_float ratios[] = {0.15f, 0.85f};
            gui_byte *iter = &demo->combo_color.r;
            gui_panel_row(&combo, GUI_DYNAMIC, 30, 2, ratios);
            for (i = 0; i < 4; ++i, iter++) {
                gui_float t = *iter;
                gui_panel_label(&combo, color_names[i], GUI_TEXT_LEFT);
                t = gui_panel_slider(&combo, 0, t, 255, 5);
                *iter = (gui_byte)t;
            }
        }
        gui_panel_combo_end(panel, &combo);
    }
    {
        /* checkbox combobox  */
        gui_int sum;
        gui_char buffer[64];
        struct gui_panel_layout combo;
        memset(&combo, 0, sizeof(combo));
        sum = demo->combo_sel[0] + demo->combo_sel[1];
        sum += demo->combo_sel[2] + demo->combo_sel[3];
        sprintf(buffer, "%d", sum);
        gui_panel_combo_begin(panel, &combo, buffer,  &demo->box_act, gui_vec2(0,0));
        {
            gui_panel_row_dynamic(&combo, 30, 1);
         demo->combo_sel[0] = gui_panel_check(&combo, items[0], demo->combo_sel[0]);
         demo->combo_sel[1] = gui_panel_check(&combo, items[1], demo->combo_sel[1]);
         demo->combo_sel[2] = gui_panel_check(&combo, items[2], demo->combo_sel[2]);
         demo->combo_sel[3] = gui_panel_check(&combo, items[3], demo->combo_sel[3]);
        }
        gui_panel_combo_end(panel, &combo);
    }
    demo->spinner= gui_panel_spinner(panel, 0, demo->spinner, 250, 10, &demo->spinner_active);
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
update_flags(struct gui_panel_layout *panel)
{
    gui_size n = 0;
    gui_flags res = 0;
    gui_flags i = 0x01;
    const char *options[]={"Hidden","Border","Header Border", "Moveable","Scaleable", "Minimized", "ROM"};
    gui_panel_row_dynamic(panel, 30, 2);
    do {
        if (gui_panel_check(panel,options[n++],(panel->flags & i)?gui_true:gui_false))
            res |= i;
        i = i << 1;
    } while (i <= GUI_PANEL_ROM);
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
        tx = gui_panel_spinner(panel,0,(gui_int)config->properties[i].x, 20, 1, NULL);
        ty = gui_panel_spinner(panel,0,(gui_int)config->properties[i].y, 20, 1, NULL);
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
        t = gui_panel_spinner(panel,0,(gui_int)config->rounding[i], 20, 1, NULL);
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
        *iter = (gui_byte)gui_panel_spinner(panel, 0, *iter, 255, 1, active[i]);
    }
    return color;
}

static void
color_tab(struct gui_panel_layout *panel, struct state *control, struct gui_config *config)
{
    gui_size i = 0;
    static const char *labels[] = {"Text:", "Panel:", "Header:", "Border:", "Button:",
        "Button Hovering:", "Button Toggle:", "Button Hovering Text:",
        "Check:", "Check BG:", "Check Active:", "Option:", "Option BG:", "Option Active:",
        "Slider:", "Slider bar:", "Slider boder:","Slider cursor:", "Progress:", "Progress Cursor:",
        "Editbox:", "Editbox cursor:", "Editbox Text:",
        "Spinner:", "Spinner Triangle:", "Spinner Text:",
        "Selector:", "Selector Triangle:", "Selector Text:", "Selector Button:",
        "Histo:", "Histo Bars:", "Histo Negative:", "Histo Hovering:", "Plot:", "Plot Lines:",
        "Plot Hightlight:", "Scrollbar:", "Scrollbar Cursor:",
        "Table lines:", "Tab header",
        "Shelf:", "Shelf Text:", "Shelf Active:", "Shelf Active Text:",
        "Scaler:", "Layout Scaler"
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

    gui_command_queue_init_fixed(&gui->queue, gui->memory, MAX_MEMORY);
    gui_config_default(config, GUI_DEFAULT_ALL, font);

    /* panel */
    gui_panel_init(&gui->panel, 30, 30, 280, 530,
        GUI_PANEL_BORDER|GUI_PANEL_MOVEABLE|GUI_PANEL_SCALEABLE,
        &gui->queue, config, gui->input);
    gui_panel_init(&gui->sub, 400, 50, 220, 180,
        GUI_PANEL_BORDER|GUI_PANEL_MOVEABLE|GUI_PANEL_SCALEABLE,
        &gui->queue, config, gui->input);

    /* widget state */
    clip.userdata.ptr = NULL,
    clip.copy = copy;
    clip.paste = paste;
    gui_edit_box_init_fixed(&win->input, win->input_buffer, MAX_BUFFER, &clip, NULL);

    win->config_tab = GUI_MINIMIZED;
    win->widget_tab = GUI_MINIMIZED;
    win->combo_tab = GUI_MINIMIZED;
    win->style_tab = GUI_MINIMIZED;
    win->round_tab = GUI_MINIMIZED;
    win->color_tab = GUI_MINIMIZED;
    win->flag_tab = GUI_MINIMIZED;

    win->combo_prog[0] = 30;
    win->combo_prog[1] = 80;
    win->combo_prog[2] = 70;
    win->combo_prog[3] = 50;

    win->scaleable = gui_true;
    win->slider = 2.0f;
    win->progressbar = 50;
    win->spinner = 100;

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
run_demo(struct demo_gui *gui)
{
    struct gui_panel_layout layout;
    struct state *state = &gui->state;
    struct gui_panel_layout tab;
    struct gui_config *config = &gui->config;
    static const char *shelfs[] = {"Histogram", "Lines"};
    enum {EASY, HARD};

    /* first panel */
    gui_panel_begin(&layout, &gui->panel);
    {
        /* header + menubar */
        gui->running = !gui_panel_header(&layout, "Demo",
            GUI_CLOSEABLE|GUI_MINIMIZABLE, GUI_CLOSEABLE, GUI_HEADER_RIGHT);
        gui_panel_menubar_begin(&layout);
        {
            const gui_char *file_items[] = {"Open", "Close", "Quit"};
            const gui_char *edit_items[] = {"Copy", "Cut", "Delete", "Paste"};
            gui_panel_row_begin(&layout, GUI_STATIC, 25, 2);
            gui_panel_row_push(&layout, config->font.width(config->font.userdata, "_FILE_", 6));
            gui_panel_menu(&layout, "FILE", file_items, LEN(file_items), 25, 100,
                &state->file_open, gui_vec2(0,0));
            gui_panel_row_push(&layout, config->font.width(config->font.userdata, "_EDIT_", 6));
            gui_panel_menu(&layout, "EDIT", edit_items, LEN(edit_items), 25, 100,
                &state->edit_open, gui_vec2(0,0));
            gui_panel_row_end(&layout);
        }
        gui_panel_menubar_end(&layout);

        /* panel style configuration  */
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

        /* widgets examples */
        if (gui_panel_layout_push(&layout, GUI_LAYOUT_TAB, "Widgets", &state->widget_tab)) {
            widget_panel(&layout, state);
            gui_panel_layout_pop(&layout);
        }

        /* popup panel */
        if (state->popup) {
            gui_panel_popup_begin(&layout, &tab, GUI_POPUP_STATIC, gui_rect(20, 100, 220, 150), gui_vec2(0,0));
            {
                if (gui_panel_header(&tab, "Popup", GUI_CLOSEABLE, GUI_CLOSEABLE, GUI_HEADER_LEFT)) {
                    gui_panel_popup_close(&tab);
                    state->popup = gui_false;
                }
                gui_panel_row_dynamic(&tab, 30, 1);
                gui_panel_label(&tab, "Are you sure you want to exit?", GUI_TEXT_LEFT);
                gui_panel_row_dynamic(&tab, 30, 4);
                gui_panel_spacing(&tab, 1);
                if (gui_panel_button_text(&tab, "Yes", GUI_BUTTON_DEFAULT)) {
                    gui_panel_popup_close(&tab);
                    state->popup = gui_false;
                }
                if (gui_panel_button_text(&tab, "No", GUI_BUTTON_DEFAULT)) {
                    gui_panel_popup_close(&tab);
                    state->popup = gui_false;
                }
            }
            gui_panel_popup_end(&layout, &tab);
        }

        /* shelf + graphes  */
        gui_panel_row_dynamic(&layout, 180, 1);
        state->shelf_selection = gui_panel_shelf_begin(&layout, &tab, shelfs,
            LEN(shelfs), state->shelf_selection, state->shelf_scrollbar);
        graph_panel(&tab, state->shelf_selection);
        state->shelf_scrollbar = gui_panel_shelf_end(&layout, &tab);

        /* tables */
        gui_panel_row_dynamic(&layout, 180, 1);
        gui_panel_group_begin(&layout, &tab, "Table", state->table_scrollbar);
        table_panel(&tab);
        state->table_scrollbar = gui_panel_group_end(&layout, &tab);

        {
            /* tree */
            struct gui_tree tree;
            gui_panel_row_dynamic(&layout, 250, 1);
            gui_panel_tree_begin(&layout, &tree, "Tree", 20, state->tree_scrollbar);
            upload_tree(&state->tree, &tree, &state->tree.root);
            state->tree_scrollbar = gui_panel_tree_end(&layout, &tree);
        }
    }
    gui_panel_end(&layout, &gui->panel);

    /* second panel */
    gui_panel_begin(&layout, &gui->sub);
    {
        gui_panel_header(&layout, "Show", GUI_CLOSEABLE, 0, GUI_HEADER_LEFT);
        gui_panel_row_static(&layout, 30, 80, 1);
        if (gui_panel_button_text(&layout, "button", GUI_BUTTON_DEFAULT)) {
            /* event handling */
        }
        gui_panel_row_dynamic(&layout, 30, 2);
        if (gui_panel_option(&layout, "easy", state->op == EASY)) state->op = EASY;
        if (gui_panel_option(&layout, "hard", state->op == HARD)) state->op = HARD;
    }
    gui_panel_end(&layout, &gui->sub);
}

