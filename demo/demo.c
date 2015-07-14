#define MAX_BUFFER  64
#define MAX_MEMORY  (32 * 1024)
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

struct show_window {
    struct gui_panel hook;
    /* input buffer */
    gui_char input_buffer[MAX_BUFFER];
    struct gui_edit_box input;

    /* widgets state */
    gui_bool checkbox;
    gui_float slider;
    gui_size progressbar;
    gui_int spinner;
    gui_bool spinner_active;
    gui_size item_current;
    gui_size shelf_selection;
    gui_int combo_selection;
    gui_bool toggle;
    gui_int option;
    gui_size text_box;
    /* tabs */
    gui_bool combobox_tab;
    gui_bool widget_tab;
    gui_bool table_tab;
    /* scrollbars */
    gui_float shelf_scrollbar;
    gui_float table_scrollbar;
    gui_float time_scrollbar;
};

struct control_window {
    struct gui_panel hook;
    gui_flags show_flags;
    /* tabs */
    gui_bool flag_tab;
    gui_bool style_tab;
    gui_bool round_tab;
    gui_bool color_tab;
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
    unsigned int ms;
    void *memory;
    struct gui_command_buffer show_buffer;
    struct gui_command_buffer control_buffer;
    struct gui_config config;
    struct gui_font font;
    struct control_window control;
    struct show_window show;
    struct gui_stack stack;
};

static void
combobox_panel(struct gui_panel_layout *panel, struct show_window *demo)
{
    gui_int i = 0;
    static const char *options[] = {"easy", "normal", "hard", "hell", "doom", "godlike"};
    gui_panel_row(panel, 30, 3);
    for (i = 0; i < (gui_int)LEN(options); i++) {
        if (gui_panel_option(panel, options[i], demo->combo_selection == i))
            demo->combo_selection = i;
    }
}

static void
widget_panel(struct gui_panel_layout *panel, struct show_window *demo)
{
    const char *items[] = {"Fist", "Pistol", "Shotgun", "Railgun", "BFG"};
    gui_panel_row(panel, 30, 1);
    gui_panel_label(panel, "text left", GUI_TEXT_LEFT);
    gui_panel_label(panel, "text center", GUI_TEXT_CENTERED);
    gui_panel_label(panel, "text right", GUI_TEXT_RIGHT);
    if (gui_panel_button_text(panel, "button", GUI_BUTTON_DEFAULT))
        fprintf(stdout, "button pressed!\n");
    if (gui_panel_button_text_triangle(panel, GUI_RIGHT, "next", GUI_TEXT_LEFT, GUI_BUTTON_DEFAULT))
        fprintf(stdout, "right triangle button pressed!\n");
    if (gui_panel_button_text_triangle(panel,GUI_LEFT,"previous",GUI_TEXT_RIGHT,GUI_BUTTON_DEFAULT))
        fprintf(stdout, "left triangle button pressed!\n");
    demo->toggle = gui_panel_button_toggle(panel, "toggle", demo->toggle);
    demo->checkbox = gui_panel_check(panel, "checkbox", demo->checkbox);

    gui_panel_row(panel, 30, 2);
    if (gui_panel_option(panel, "option 0", demo->option == 0)) demo->option = 0;
    if (gui_panel_option(panel, "option 1", demo->option == 1)) demo->option = 1;

    {
        char buffer[MAX_BUFFER];
        const gui_float ratio[] = {0.8f, 0.2f};
        gui_panel_row_templated(panel, 30, 2, ratio);
        demo->slider = gui_panel_slider(panel, 0, demo->slider, 10, 1.0f);
        sprintf(buffer, "%.2f", demo->slider);
        gui_panel_label(panel, buffer, GUI_TEXT_LEFT);
        demo->progressbar = gui_panel_progress(panel, demo->progressbar, 100, gui_true);
        sprintf(buffer, "%lu", demo->progressbar);
        gui_panel_label(panel, buffer, GUI_TEXT_LEFT);
    }

    gui_panel_row(panel, 30, 1);
    demo->item_current = gui_panel_selector(panel, items, LEN(items), demo->item_current);
    demo->spinner = gui_panel_spinner(panel, 0, demo->spinner, 250, 10, &demo->spinner_active);

    gui_panel_row_begin(panel, 30);
    gui_panel_row_push_widget(panel, 0.7f);
    gui_panel_editbox(panel, &demo->input);
    gui_panel_row_push_widget(panel, 0.3f);
    if (gui_panel_button_text(panel, "submit", GUI_BUTTON_DEFAULT)) {
        gui_edit_box_reset(&demo->input);
        fprintf(stdout, "command executed!\n");
    }
    gui_panel_row_end(panel);
}

static void
graph_panel(struct gui_panel_layout *panel, gui_size current)
{
    enum {COL, PLOT};
    static const gui_float values[]={8.0f,15.0f,20.0f,12.0f,30.0f,12.0f,35.0f,40.0f,20.0f};
    gui_panel_row(panel, 100, 1);
    if (current == COL) {
        gui_panel_graph(panel, GUI_GRAPH_COLUMN, values, LEN(values), 0);
    } else {
        gui_panel_graph(panel, GUI_GRAPH_LINES, values, LEN(values), 0);
    }
}

static void
time_panel(struct gui_panel_layout *panel, unsigned int ms)
{
    char buffer[MAX_BUFFER];
    ms = MAX(1, ms);
    gui_panel_row(panel, 20, 2);
    gui_panel_label(panel, "FPS:", GUI_TEXT_LEFT);
    sprintf(buffer, "%.2f", 1.0f/((float)ms/1000.0f));
    gui_panel_label(panel, buffer, GUI_TEXT_CENTERED);
    gui_panel_label(panel, "MS:", GUI_TEXT_LEFT);
    sprintf(buffer, "%d", ms);
    gui_panel_label(panel, buffer, GUI_TEXT_CENTERED);
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
init_show(struct show_window *win, struct gui_config *config,
    struct gui_command_buffer *buffer, struct gui_stack *stack)
{
    memset(win, 0, sizeof(*win));
    gui_panel_init(&win->hook, 20, 20, 300, 550,
        GUI_PANEL_BORDER|GUI_PANEL_MOVEABLE|
        GUI_PANEL_CLOSEABLE|GUI_PANEL_SCALEABLE|
        GUI_PANEL_MINIMIZABLE|GUI_PANEL_HIDDEN, buffer, config);
    gui_stack_push(stack, &win->hook);
    gui_edit_box_init_fixed(&win->input, win->input_buffer, MAX_BUFFER, NULL, NULL);

    win->widget_tab = GUI_MAXIMIZED;
    win->combobox_tab = GUI_MINIMIZED;
    win->slider = 10.0f;
    win->progressbar = 50;
    win->spinner = 100;
}

static void
update_show(struct show_window *show, struct gui_stack *stack, struct gui_input *in,
    unsigned int ms)
{
    struct gui_panel_layout tab;
    struct gui_panel_layout layout;
    static const char *shelfs[] = {"Histogram", "Lines"};
    gui_panel_begin_stacked(&layout, &show->hook, stack, "Show", in);

    show->combobox_tab = gui_panel_tab_begin(&layout, &tab, "Combobox", show->combobox_tab);
    combobox_panel(&tab, show);
    gui_panel_tab_end(&layout, &tab);

    show->widget_tab = gui_panel_tab_begin(&layout, &tab, "Widgets", show->widget_tab);
    widget_panel(&tab, show);
    gui_panel_tab_end(&layout, &tab);

    gui_panel_row(&layout, 110, 1);
    gui_panel_group_begin(&layout, &tab, "Time", show->time_scrollbar);
    time_panel(&tab, ms);
    show->time_scrollbar = gui_panel_group_end(&layout, &tab);

    gui_panel_row(&layout, 180, 1);
    show->shelf_selection = gui_panel_shelf_begin(&layout, &tab, shelfs,
        LEN(shelfs), show->shelf_selection, show->shelf_scrollbar);
    graph_panel(&tab, show->shelf_selection);
    show->shelf_scrollbar = gui_panel_shelf_end(&layout, &tab);

    gui_panel_row(&layout, 180, 1);
    gui_panel_group_begin(&layout, &tab, "Table", show->table_scrollbar);
    table_panel(&tab);
    show->table_scrollbar = gui_panel_group_end(&layout, &tab);
    gui_panel_end(&layout, &show->hook);
}

static void
update_flags(struct gui_panel_layout *panel, struct control_window *control)
{
    gui_size n = 0;
    gui_flags res = 0;
    gui_flags i = 0x01;
    const char *options[]={"Hidden","Border","Minimizable","Closeable","Moveable","Scaleable"};
    gui_panel_row(panel, 30, 2);
    do {
        if (gui_panel_check(panel,options[n++],(control->show_flags & i)?gui_true:gui_false))
            res |= i;
        i = i << 1;
    } while (i <= GUI_PANEL_SCALEABLE);
    control->show_flags = res;
}

static void
properties_tab(struct gui_panel_layout *panel, struct gui_config *config)
{
    int i = 0;
    const char *properties[] = {"item spacing:", "item padding:", "panel padding:",
        "scaler size:", "scrollbar:"};

    gui_panel_row(panel, 30, 3);
    for (i = 0; i <= GUI_PROPERTY_SCROLLBAR_WIDTH; ++i) {
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

    gui_panel_row(panel, 30, 2);
    for (i = 0; i < GUI_ROUNDING_MAX; ++i) {
        gui_int t;
        gui_panel_label(panel, rounding[i], GUI_TEXT_LEFT);
        t = gui_panel_spinner(panel,0,(gui_int)config->rounding[i], 20, 1, NULL);
        config->rounding[i] = (float)t;
    }
}

static struct gui_color
color_picker(struct gui_panel_layout *panel, struct control_window *control,
    const char *name, struct gui_color color)
{
    int i;
    gui_byte *iter;
    gui_bool *active[4];

    active[0] = &control->spinner_r_active;
    active[1] = &control->spinner_g_active;
    active[2] = &control->spinner_b_active;
    active[3] = &control->spinner_a_active;

    gui_panel_row(panel, 30, 2);
    gui_panel_label(panel, name, GUI_TEXT_LEFT);
    gui_panel_button_color(panel, color, GUI_BUTTON_DEFAULT);

    iter = &color.r;
    gui_panel_row(panel, 30, 2);
    for (i = 0; i < 4; ++i, iter++) {
        gui_float t = *iter;
        t = gui_panel_slider(panel, 0, t, 255, 10);
        *iter = (gui_byte)t;
        *iter = (gui_byte)gui_panel_spinner(panel, 0, *iter, 255, 1, active[i]);
    }
    return color;
}

static void
color_tab(struct gui_panel_layout *panel, struct control_window *control, struct gui_config *config)
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
        "Table lines:", "Shelf:", "Shelf Text:", "Shelf Active:", "Shelf Active Text:", "Scaler:",
        "Tiled Scaler"
    };

    if (control->picker_active) {
        control->color = color_picker(panel,control,labels[control->current_color], control->color);
        gui_panel_row(panel, 30, 3);
        gui_panel_spacing(panel, 1);
        if (gui_panel_button_text(panel, "ok", GUI_BUTTON_DEFAULT)) {
            config->colors[control->current_color] = control->color;
            control->picker_active = gui_false;
        }
        if (gui_panel_button_text(panel, "cancel", GUI_BUTTON_DEFAULT))
            control->picker_active = gui_false;
    } else {
        gui_panel_row(panel, 30, 2);
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
init_control(struct control_window *win, struct gui_config *config,
    struct gui_command_buffer *buffer, struct gui_stack *stack)
{
    memset(win, 0, sizeof(*win));
    gui_panel_init(&win->hook, 380, 20, 350, 500,
        GUI_PANEL_BORDER|GUI_PANEL_MOVEABLE|GUI_PANEL_CLOSEABLE|GUI_PANEL_SCALEABLE,
        buffer, config);
    gui_stack_push(stack, &win->hook);
    win->show_flags = win->hook.flags;
    win->color_tab = GUI_MINIMIZED;
}

static gui_bool
update_control(struct control_window *control, struct gui_stack *stack,
    struct gui_input *in, struct gui_config *config)
{
    gui_bool running;
    struct gui_panel_layout layout;
    struct gui_panel_layout tab;

    running = gui_panel_begin_stacked(&layout, &control->hook, stack, "Control", in);
    control->flag_tab = gui_panel_tab_begin(&layout, &tab, "Options", control->flag_tab);
    update_flags(&tab, control);
    gui_panel_tab_end(&layout, &tab);

    control->style_tab = gui_panel_tab_begin(&layout, &tab, "Properties", control->style_tab);
    properties_tab(&tab, config);
    gui_panel_tab_end(&layout, &tab);

    control->round_tab = gui_panel_tab_begin(&layout, &tab, "Rounding", control->round_tab);
    round_tab(&tab, config);
    gui_panel_tab_end(&layout, &tab);

    control->color_tab = gui_panel_tab_begin(&layout, &tab, "Color", control->color_tab);
    color_tab(&tab, control, config);
    gui_panel_tab_end(&layout, &tab);

    gui_panel_end(&layout, &control->hook);
    return running;
}

static void
init_demo(struct demo_gui *gui, struct gui_font *font)
{
    struct gui_config *config = &gui->config;
    gui->font = *font;
    gui->running = gui_true;

    gui_command_buffer_init_fixed(&gui->show_buffer, gui->memory, MAX_MEMORY/2, GUI_CLIP);
    gui_command_buffer_init_fixed(&gui->control_buffer,
        gui_ptr_add(void*, gui->memory, (MAX_MEMORY/2)), MAX_MEMORY/2, GUI_CLIP);
    gui_config_default(config, GUI_DEFAULT_ALL, font);

    gui_stack_clear(&gui->stack);
    init_show(&gui->show, config, &gui->show_buffer, &gui->stack);
    init_control(&gui->control, config, &gui->control_buffer, &gui->stack);
    gui->show.hook.flags |= GUI_PANEL_HIDDEN;
}

static void
run_demo(struct demo_gui *gui, struct gui_input *input)
{
    struct control_window *control = &gui->control;
    struct show_window *show = &gui->show;

    gui->running = update_control(control, &gui->stack, input, &gui->config);
    if (show->hook.flags & GUI_PANEL_ACTIVE)
        show->hook.flags = control->show_flags|GUI_PANEL_ACTIVE;
    else show->hook.flags = control->show_flags;
    update_show(show, &gui->stack, input, gui->ms);
    if (show->hook.flags & GUI_PANEL_HIDDEN)
        control->show_flags |= GUI_PANEL_HIDDEN;
}

