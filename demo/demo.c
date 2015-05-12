#define MAX_BUFFER  64
#define MAX_MEMORY  (32 * 1024)

struct show_window {
    struct gui_panel_hook hook;

    /* widget data */
    gui_char in_buf[MAX_BUFFER];
    gui_size in_len;
    gui_bool in_act;
    gui_char cmd_buf[MAX_BUFFER];
    gui_size cmd_len;
    gui_bool cmd_act;
    gui_bool check;
    gui_float slider;
    gui_size prog;
    gui_int spinner;
    gui_bool spin_act;
    gui_size item_cur;
    gui_size cur;
    gui_int combo_sel;
    gui_bool toggle;
    gui_int option;

    /* tabs */
    gui_bool diff_min;
    gui_bool wid_min;
    gui_bool tbl_min;

    /* scrollbars */
    gui_float shelf_off;
    gui_float tbl_off;
};

struct control_window {
    struct gui_panel_hook hook;
    gui_flags show_flags;

    /* tabs */
    gui_bool flag_min;
    gui_bool style_min;
    gui_bool color_min;

    /* color picker */
    gui_bool picker_act;
    gui_bool col_r_act;
    gui_bool col_g_act;
    gui_bool col_b_act;
    gui_bool col_a_act;
    gui_size current;
    struct gui_color color;
};

static void
table_panel(struct gui_panel_layout *panel, struct show_window *demo)
{
    struct gui_panel_layout tab;
    static const struct gui_color header = {178, 122, 1, 255};
    gui_panel_row(panel, 250, 1);
    gui_panel_group_begin(panel, &tab, "Table", demo->tbl_off);
    gui_panel_table_begin(&tab, GUI_TABLE_HHEADER, 30, 2);
    gui_panel_label_colored(&tab, "MOVEMENT", GUI_TEXT_CENTERED, header);
    gui_panel_label_colored(&tab, "KEY/BUTTON", GUI_TEXT_CENTERED, header);
    gui_panel_table_row(&tab);
    gui_panel_label(&tab, "Move foward", GUI_TEXT_LEFT);
    gui_panel_label(&tab, "w", GUI_TEXT_CENTERED);
    gui_panel_table_row(&tab);
    gui_panel_label(&tab, "Move back", GUI_TEXT_LEFT);
    gui_panel_label(&tab, "s", GUI_TEXT_CENTERED);
    gui_panel_table_row(&tab);
    gui_panel_label(&tab, "Move left", GUI_TEXT_LEFT);
    gui_panel_label(&tab, "a", GUI_TEXT_CENTERED);
    gui_panel_table_row(&tab);
    gui_panel_label(&tab, "Move right", GUI_TEXT_LEFT);
    gui_panel_label(&tab, "d", GUI_TEXT_CENTERED);
    gui_panel_table_row(&tab);
    gui_panel_label(&tab, "Jump", GUI_TEXT_LEFT);
    gui_panel_label(&tab, "SPACE", GUI_TEXT_CENTERED);
    gui_panel_table_row(&tab);
    gui_panel_label(&tab, "Duck", GUI_TEXT_LEFT);
    gui_panel_label(&tab, "CTRL", GUI_TEXT_CENTERED);
    gui_panel_table_end(&tab);
    demo->tbl_off = gui_panel_group_end(panel, &tab);
}

static void
graph_panel(struct gui_panel_layout *panel, struct show_window *demo)
{
    enum {HISTO, PLOT};
    struct gui_panel_layout tab;
    static const char *shelfs[] = {"Histogram", "Lines"};
    static const gui_float values[] = {8.0f,15.0f,20.0f,12.0f,30.0f,12.0f,35.0f,40.0f,20.0f};
    gui_panel_row(panel, 180, 1);
    demo->cur = gui_panel_shelf_begin(panel,&tab,shelfs,LEN(shelfs),demo->cur,demo->shelf_off);
    gui_panel_row(&tab, 100, 1);
    if (demo->cur == HISTO) {
        gui_panel_graph(&tab, GUI_GRAPH_HISTO, values, LEN(values));
    } else {
        gui_panel_graph(&tab, GUI_GRAPH_LINES, values, LEN(values));
    }
    demo->shelf_off = gui_panel_shelf_end(panel, &tab);
}

static void
combobox_panel(struct gui_panel_layout *panel, struct show_window *demo)
{
    gui_int i = 0;
    static const char *options[] = {"easy", "normal", "hard", "hell", "doom", "godlike"};
    struct gui_panel_layout tab;
    demo->diff_min = gui_panel_tab_begin(panel, &tab, "Comobox", demo->diff_min);
    gui_panel_row(&tab, 30, 3);
    for (i = 0; i < (gui_int)LEN(options); i++) {
        if (gui_panel_option(&tab, options[i], demo->combo_sel == i))
            demo->combo_sel = i;
    }
    gui_panel_tab_end(panel, &tab);
}

static void
widget_panel(struct gui_panel_layout *panel, struct show_window *demo)
{
    struct gui_panel_layout tab;
    char buffer[MAX_BUFFER];
    const char *items[] = {"Fist", "Pistol", "Shotgun", "Railgun", "BFG"};

    demo->wid_min = gui_panel_tab_begin(panel, &tab, "Widgets", demo->wid_min);
    gui_panel_row(&tab, 30, 1);
    gui_panel_label(&tab, "text left", GUI_TEXT_LEFT);
    gui_panel_label(&tab, "text center", GUI_TEXT_CENTERED);
    gui_panel_label(&tab, "text right", GUI_TEXT_RIGHT);
    if (gui_panel_button_text(&tab, "button", GUI_BUTTON_DEFAULT))
        fprintf(stdout, "button pressed!\n");
    demo->toggle = gui_panel_button_toggle(&tab, "toggle", demo->toggle);
    demo->check = gui_panel_check(&tab, "checkbox", demo->check);

    gui_panel_row(&tab, 30, 2);
    if (gui_panel_option(&tab, "option 0", demo->option == 0)) demo->option = 0;
    if (gui_panel_option(&tab, "option 1", demo->option == 1)) demo->option = 1;
    demo->slider = gui_panel_slider(&tab, 0, demo->slider, 10, 1.0f);
    sprintf(buffer, "%.2f", demo->slider);
    gui_panel_label(&tab, buffer, GUI_TEXT_LEFT);
    demo->prog = gui_panel_progress(&tab, demo->prog, 100, gui_true);
    sprintf(buffer, "%lu", demo->prog);
    gui_panel_label(&tab, buffer, GUI_TEXT_LEFT);

    gui_panel_row(&tab, 30, 1);
    demo->item_cur = gui_panel_selector(&tab, items, LEN(items), demo->item_cur);
    demo->spinner = gui_panel_spinner(&tab, 0, demo->spinner, 250, 10, &demo->spin_act);
    if (gui_panel_shell(&tab, demo->cmd_buf, &demo->cmd_len, MAX_BUFFER, &demo->cmd_act))
        demo->cmd_len = 0;
    demo->in_len = gui_panel_edit(&tab, demo->in_buf, demo->in_len,
                        MAX_BUFFER, &demo->in_act, GUI_INPUT_DEFAULT);
    gui_panel_tab_end(panel, &tab);
}

static void
show_panel(struct show_window *show, struct gui_panel_stack *stack,
    struct gui_input *in, struct gui_canvas *canvas)
{
    struct gui_panel_layout layout;
    gui_panel_hook_begin(&layout, &show->hook, stack, "Show", canvas, in);
    combobox_panel(&layout, show);
    widget_panel(&layout, show);
    graph_panel(&layout, show);
    table_panel(&layout, show);
    gui_panel_hook_end(&layout, &show->hook);
}

static void
flags_tab(struct gui_panel_layout *panel, struct control_window *control)
{
    gui_flags i = 0x01;
    gui_size n = 0;
    gui_flags res = 0;
    struct gui_panel_layout tab;

    const char *options[] = {"Hidden", "Border", "Minimizable",
        "Closeable", "Moveable", "Scaleable"};
    control->flag_min = gui_panel_tab_begin(panel, &tab, "Options", control->flag_min);
    gui_panel_row(&tab, 30, 2);
    do {
        if (gui_panel_check(&tab, options[n++], (control->show_flags & i) ? gui_true : gui_false))
            res |= i;
        i = i << 1;
    } while (i <= GUI_PANEL_SCALEABLE);
    control->show_flags = res;
    gui_panel_tab_end(panel, &tab);
}

static void
style_tab(struct gui_panel_layout *panel, struct control_window *control, struct gui_config *config)
{
    gui_int tx, ty;
    struct gui_panel_layout tab;
    control->style_min = gui_panel_tab_begin(panel, &tab, "Style", control->style_min);

    gui_panel_row(&tab, 30, 2);
    gui_panel_label(&tab, "scrollbar width:", GUI_TEXT_LEFT);
    tx = gui_panel_spinner(&tab, 0, (gui_int)config->scrollbar_width, 20, 1, NULL);
    config->scrollbar_width = (float)tx;

    gui_panel_row(&tab, 30, 3);
    gui_panel_label(&tab, "padding:", GUI_TEXT_LEFT);
    tx = gui_panel_spinner(&tab, 0, (gui_int)config->panel_padding.x, 20, 1, NULL);
    ty = gui_panel_spinner(&tab, 0, (gui_int)config->panel_padding.y, 20, 1, NULL);
    config->panel_padding.x = (float)tx;
    config->panel_padding.y = (float)ty;

    gui_panel_label(&tab, "item spacing:", GUI_TEXT_LEFT);
    tx = gui_panel_spinner(&tab, 0, (gui_int)config->item_spacing.x, 20, 1, NULL);
    ty = gui_panel_spinner(&tab, 0, (gui_int)config->item_spacing.y, 20, 1, NULL);
    config->item_spacing.x = (float)tx;
    config->item_spacing.y = (float)ty;

    gui_panel_label(&tab, "item padding:", GUI_TEXT_LEFT);
    tx = gui_panel_spinner(&tab, 0, (gui_int)config->item_padding.x, 20, 1, NULL);
    ty = gui_panel_spinner(&tab, 0, (gui_int)config->item_padding.y, 20, 1, NULL);
    config->item_padding.x = (float)tx;
    config->item_padding.y = (float)ty;

    gui_panel_label(&tab, "scaler size:", GUI_TEXT_LEFT);
    tx = gui_panel_spinner(&tab, 0, (gui_int)config->scaler_size.x, 20, 1, NULL);
    ty = gui_panel_spinner(&tab, 0, (gui_int)config->scaler_size.y, 20, 1, NULL);
    config->scaler_size.x = (float)tx;
    config->scaler_size.y = (float)ty;
    gui_panel_tab_end(panel, &tab);
}

static struct gui_color
color_picker(struct gui_panel_layout *panel, struct control_window *control,
    const char *name, struct gui_color color)
{
    gui_float r, g, b, a;
    gui_panel_row(panel, 30, 2);
    gui_panel_label(panel, name, GUI_TEXT_LEFT);
    gui_panel_button_color(panel, color, GUI_BUTTON_DEFAULT);
    gui_panel_row(panel, 30, 2);
    r = color.r; g = color.g; b = color.b; a = color.a;
    r = gui_panel_slider(panel, 0, r, 255, 10);
    color.r = (gui_byte)r;
    color.r = (gui_byte)gui_panel_spinner(panel, 0, color.r, 255, 1, &control->col_r_act);
    g = gui_panel_slider(panel, 0, g, 255, 10);
    color.g = (gui_byte)g;
    color.g = (gui_byte)gui_panel_spinner(panel, 0, color.g, 255, 1, &control->col_g_act);
    b = gui_panel_slider(panel, 0, b, 255, 10);
    color.b = (gui_byte)b;
    color.b = (gui_byte)gui_panel_spinner(panel, 0, (gui_int)color.b, 255, 1, &control->col_b_act);
    a = gui_panel_slider(panel, 0, a, 255, 10);
    color.a = (gui_byte)a;
    color.a = (gui_byte)gui_panel_spinner(panel, 0, (gui_int)color.a, 255, 1, &control->col_a_act);
    return color;
}

static void
color_tab(struct gui_panel_layout *panel, struct control_window *control, struct gui_config *config)
{
    gui_size i = 0;
    struct gui_panel_layout tab;
    static const char *labels[] = {"Text:", "Panel:", "Header:", "Border:", "Button:",
        "Button Border:", "Button Hovering:", "Button Toggle:", "Button Hovering Text:",
        "Check:", "Check BG:", "Check Active:", "Option:", "Option BG:", "Option Active:",
        "Slider:", "Slider cursor:", "Progress:", "Progress Cursor:", "Editbox:", "Editbox cursor:",
        "Editbox Border:", "Spinner:", "Spinner Border:", "Selector:", "Selector Border:",
        "Histo:", "Histo Bars:", "Histo Negative:", "Histo Hovering:", "Plot:", "Plot Lines:",
        "Plot Hightlight:", "Scrollbar:", "Scrollbar Cursor:", "Scrollbar Border:",
        "Table lines:", "Scaler:"
    };
    control->color_min = gui_panel_tab_begin(panel, &tab, "Color", control->color_min);
    if (control->picker_act) {
        control->color = color_picker(&tab, control, labels[control->current], control->color);
        gui_panel_row(&tab, 30, 3);
        gui_panel_seperator(&tab, 1);
        if (gui_panel_button_text(&tab, "ok", GUI_BUTTON_DEFAULT)) {
            config->colors[control->current] = control->color;
            control->picker_act = gui_false;
        }
        if (gui_panel_button_text(&tab, "cancel", GUI_BUTTON_DEFAULT))
            control->picker_act = gui_false;
    } else {
        gui_panel_row(&tab, 30, 2);
        for (i = 0; i < GUI_COLOR_COUNT; ++i) {
            struct gui_panel_layout layout;
            gui_panel_label(&tab, labels[i], GUI_TEXT_LEFT);
            if (gui_panel_button_color(&tab, config->colors[i], GUI_BUTTON_DEFAULT)) {
                if (!control->picker_act) {
                    control->picker_act = gui_true;
                    control->color = config->colors[i];
                    control->current = i;
                } else continue;
            }
        }
    }
    gui_panel_tab_end(panel, &tab);
}

static gui_bool
control_panel(struct control_window *control, struct gui_panel_stack *stack,
    struct gui_input *in, struct gui_canvas *canvas, struct gui_config *config)
{
    gui_bool running;
    struct gui_panel_layout layout;
    running = gui_panel_hook_begin(&layout, &control->hook, stack, "Control", canvas, in);
    flags_tab(&layout, control);
    style_tab(&layout, control, config);
    color_tab(&layout, control, config);
    gui_panel_hook_end(&layout, &control->hook);
    return running;
}

static void
init_demo(struct show_window *show, struct control_window *control,
    struct gui_panel_stack *stack, struct gui_config *config, struct gui_font *font)
{
    memset(show, 0, sizeof(*show));
    gui_panel_hook_init(&show->hook, 50, 50, 300, 500,
        GUI_PANEL_BORDER|GUI_PANEL_MOVEABLE|
        GUI_PANEL_CLOSEABLE|GUI_PANEL_SCALEABLE|
        GUI_PANEL_MINIMIZABLE, config, font);
    gui_stack_push(stack, &show->hook);

    show->wid_min = gui_true;
    show->diff_min = gui_true;
    show->slider = 5.0f;
    show->prog = 50;
    show->spinner = 100;

    memset(control, 0, sizeof(*control));
    gui_panel_hook_init(&control->hook, 380, 50, 400, 350,
        GUI_PANEL_BORDER|GUI_PANEL_MOVEABLE|GUI_PANEL_CLOSEABLE|GUI_PANEL_SCALEABLE, config, font);
    gui_stack_push(stack, &control->hook);
    control->show_flags = gui_hook_panel(&show->hook)->flags;
    control->style_min = gui_true;
    control->color_min = gui_true;
}

static gui_bool
run_demo(struct show_window *show, struct control_window *control, struct gui_panel_stack *stack,
    struct gui_config *config, struct gui_input *in, struct gui_command_buffer *buffer,
    gui_size width, gui_size height)
{
    gui_bool running;
    struct gui_command_buffer sub;
    struct gui_canvas canvas;

    gui_buffer_begin(NULL, buffer, width, height);
    gui_buffer_lock(&canvas, buffer, &sub, 0, width, height);
    running = control_panel(control, stack, in, &canvas, config);
    gui_buffer_unlock(gui_hook_output(&control->hook), buffer, &sub, &canvas, NULL);

    gui_hook_panel(&show->hook)->flags = control->show_flags;
    gui_buffer_lock(&canvas, buffer, &sub, 0, width, height);
    show_panel(show, stack, in, &canvas);
    if (gui_hook_panel(&show->hook)->flags & GUI_PANEL_HIDDEN)
        control->show_flags |= GUI_PANEL_HIDDEN;
    gui_buffer_unlock(gui_hook_output(&show->hook), buffer, &sub, &canvas, NULL);
    gui_buffer_end(NULL, buffer, NULL, NULL);
    return running;
}

