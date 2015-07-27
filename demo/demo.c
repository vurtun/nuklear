#define MAX_BUFFER  64
#define MAX_MEMORY  (32 * 1024)
#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

struct show_window {
    struct gui_panel hook;
    gui_flags header_flags;
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
    /* menu */
    gui_size menu_item;
};

struct control_window {
    struct gui_panel hook;
    gui_flags show_flags;
    gui_flags header_flags;
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
widget_panel(struct gui_panel_layout *panel, struct show_window *demo)
{
    const char *items[] = {"Fist", "Pistol", "Shotgun", "Railgun", "BFG"};

    /* Labels */
    gui_panel_layout_fixed_ratio(panel, 30, 1);
    gui_panel_label(panel, "text left", GUI_TEXT_LEFT);
    gui_panel_label(panel, "text center", GUI_TEXT_CENTERED);
    gui_panel_label(panel, "text right", GUI_TEXT_RIGHT);

    /* Buttons */
    gui_panel_layout_fixed_ratio(panel, 30, 1);
    if (gui_panel_button_text(panel, "button", GUI_BUTTON_DEFAULT))
        fprintf(stdout, "button pressed!\n");
    if (gui_panel_button_text_triangle(panel, GUI_RIGHT, "next", GUI_TEXT_LEFT, GUI_BUTTON_DEFAULT))
        fprintf(stdout, "right triangle button pressed!\n");
    if (gui_panel_button_text_triangle(panel,GUI_LEFT,"previous",GUI_TEXT_RIGHT,GUI_BUTTON_DEFAULT))
        fprintf(stdout, "left triangle button pressed!\n");

    demo->toggle = gui_panel_button_toggle(panel, "toggle", demo->toggle);
    demo->checkbox = gui_panel_check(panel, "checkbox", demo->checkbox);
    gui_panel_layout_fixed_ratio(panel, 30, 2);
    if (gui_panel_option(panel, "option 0", demo->option == 0)) demo->option = 0;
    if (gui_panel_option(panel, "option 1", demo->option == 1)) demo->option = 1;

    {
        /* templated row layout */
        char buffer[MAX_BUFFER];
        const gui_float ratio[] = {0.8f, 0.2f};
        gui_panel_layout_def_ratio(panel, 30, 2, ratio);
        demo->slider = gui_panel_slider(panel, 0, demo->slider, 10, 1.0f);
        sprintf(buffer, "%.2f", demo->slider);
        gui_panel_label(panel, buffer, GUI_TEXT_LEFT);
        demo->progressbar = gui_panel_progress(panel, demo->progressbar, 100, gui_true);
        sprintf(buffer, "%lu", demo->progressbar);
        gui_panel_label(panel, buffer, GUI_TEXT_LEFT);
    }

    gui_panel_layout_fixed_ratio(panel, 30, 1);
    demo->item_current = gui_panel_selector(panel, items, LEN(items), demo->item_current);
    demo->spinner = gui_panel_spinner(panel, 0, demo->spinner, 250, 10, &demo->spinner_active);

    gui_panel_layout_row_ratio_begin(panel, 30, 2);
    {
        gui_panel_layout_row_ratio_push(panel, 0.7f);
        gui_panel_editbox(panel, &demo->input);
        gui_panel_layout_row_ratio_push(panel, 0.3f);
        if (gui_panel_button_text(panel, "submit", GUI_BUTTON_DEFAULT)) {
            gui_edit_box_reset(&demo->input);
            fprintf(stdout, "command executed!\n");
        }
    }
    gui_panel_layout_row_ratio_end(panel);
}

static void
graph_panel(struct gui_panel_layout *panel, gui_size current)
{
    enum {COL, PLOT};
    static const gui_float values[]={8.0f,15.0f,20.0f,12.0f,30.0f,12.0f,35.0f,40.0f,20.0f};
    gui_panel_layout_fixed_ratio(panel, 100, 1);
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
init_show(struct show_window *win, struct gui_config *config,
    struct gui_command_buffer *buffer, struct gui_stack *stack)
{
    memset(win, 0, sizeof(*win));
    gui_panel_init(&win->hook, 20, 20, 310, 550,
        GUI_PANEL_BORDER|GUI_PANEL_MOVEABLE|
        GUI_PANEL_SCALEABLE, buffer, config);
    gui_stack_push(stack, &win->hook);
    gui_edit_box_init_fixed(&win->input, win->input_buffer, MAX_BUFFER, NULL, NULL);

    win->header_flags = GUI_CLOSEABLE|GUI_MINIMIZABLE;
    win->widget_tab = GUI_MINIMIZED;
    win->combobox_tab = GUI_MINIMIZED;
    win->slider = 10.0f;
    win->progressbar = 50;
    win->spinner = 100;
}

static void
update_menu(struct gui_panel_layout *layout, struct show_window *win, struct gui_config *config)
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

    const struct level *lvl = &levels[win->menu_item];
    const struct combi *iter = &combis[lvl->list];
    {
        /* calculate column row count to fit largets menu item  */
        gui_size cols, max = 0;
        for (i = 0; i < lvl->items; ++i) {
            gui_size text_w, w;
            const struct item *item = &items[iter->item];
            text_w = config->font.width(config->font.userdata,item->name,strlen(item->name));
            w = text_w + (gui_size)config->properties[GUI_PROPERTY_ITEM_PADDING].x * 2;
            if (w > max) max = w;
            iter++;
        }
        cols = gui_panel_table_columns(layout, max);
        gui_panel_layout_fixed_ratio(layout, 18, cols);
    }

    /* output current menu level entries */
    gui_panel_menu_begin(layout);
    {
        gui_config_push_color(config, GUI_COLOR_BUTTON_BORDER, 45, 45, 45, 250);
        gui_config_push_property(config, GUI_PROPERTY_ITEM_SPACING, 0, 4.0f);
        iter = &combis[lvl->list];
        for (i = 0; i < lvl->items; ++i) {
            const struct item *item = &items[iter->item];
            if (gui_panel_menu_item(layout, item->name)) {
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
update_show(struct show_window *show, struct gui_stack *stack, struct gui_input *in, struct gui_config *config)
{
    struct gui_panel_layout tab;
    struct gui_panel_layout layout;
    static const char *shelfs[] = {"Histogram", "Lines"};
    gui_panel_begin_stacked(&layout, &show->hook, stack, in);
    gui_panel_header(&layout, "Show", show->header_flags, 0, GUI_HEADER_RIGHT);
    update_menu(&layout, show, config);
    {
        /* Widgets */
        show->widget_tab = gui_panel_tab_begin(&layout, &tab, "Widgets",GUI_BORDER, show->widget_tab);
        widget_panel(&tab, show);
        gui_panel_tab_end(&layout, &tab);

        /* Graph */
        gui_panel_layout_fixed_ratio(&layout, 180, 1);
        show->shelf_selection = gui_panel_shelf_begin(&layout, &tab, shelfs,
            LEN(shelfs), show->shelf_selection, show->shelf_scrollbar);
        graph_panel(&tab, show->shelf_selection);
        show->shelf_scrollbar = gui_panel_shelf_end(&layout, &tab);

        /* Table */
        gui_panel_layout_fixed_ratio(&layout, 180, 1);
        gui_panel_group_begin(&layout, &tab, "Table", show->table_scrollbar);
        table_panel(&tab);
        show->table_scrollbar = gui_panel_group_end(&layout, &tab);
    }
    gui_panel_end(&layout, &show->hook);
}

static void
update_flags(struct gui_panel_layout *panel, struct control_window *control)
{
    gui_size n = 0;
    gui_flags res = 0;
    gui_flags i = 0x01;
    const char *options[]={"Hidden","Border","Header Border", "Moveable","Scaleable", "Minimized"};
    gui_panel_layout_fixed_ratio(panel, 30, 2);
    do {
        if (gui_panel_check(panel,options[n++],(control->show_flags & i)?gui_true:gui_false))
            res |= i;
        i = i << 1;
    } while (i <= GUI_PANEL_MINIMIZED);
    control->show_flags = res;
}

static void
properties_tab(struct gui_panel_layout *panel, struct gui_config *config)
{
    int i = 0;
    const char *properties[] = {"item spacing:", "item padding:", "panel padding:",
        "scaler size:", "scrollbar:"};

    gui_panel_layout_fixed_ratio(panel, 30, 3);
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

    gui_panel_layout_fixed_ratio(panel, 30, 2);
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

    gui_panel_layout_fixed_ratio(panel, 30, 2);
    gui_panel_label(panel, name, GUI_TEXT_LEFT);
    gui_panel_button_color(panel, color, GUI_BUTTON_DEFAULT);

    iter = &color.r;
    gui_panel_layout_fixed_ratio(panel, 30, 2);
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
        gui_panel_layout_fixed_ratio(panel, 30, 3);
        gui_panel_spacing(panel, 1);
        if (gui_panel_button_text(panel, "ok", GUI_BUTTON_DEFAULT)) {
            config->colors[control->current_color] = control->color;
            control->picker_active = gui_false;
        }
        if (gui_panel_button_text(panel, "cancel", GUI_BUTTON_DEFAULT))
            control->picker_active = gui_false;
    } else {
        gui_panel_layout_fixed_ratio(panel, 30, 2);
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
        GUI_PANEL_BORDER|GUI_PANEL_MOVEABLE|GUI_PANEL_SCALEABLE, buffer, config);
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

    gui_panel_begin_stacked(&layout, &control->hook, stack, in);
    running = !gui_panel_header(&layout, "Control", GUI_CLOSEABLE|GUI_MINIMIZABLE,
                                GUI_CLOSEABLE, GUI_HEADER_LEFT);
    {
        control->flag_tab = gui_panel_tab_begin(&layout, &tab, "Options", GUI_BORDER, control->flag_tab);
        update_flags(&tab, control);
        gui_panel_tab_end(&layout, &tab);

        control->style_tab = gui_panel_tab_begin(&layout, &tab, "Properties", GUI_BORDER, control->style_tab);
        properties_tab(&tab, config);
        gui_panel_tab_end(&layout, &tab);

        control->round_tab = gui_panel_tab_begin(&layout, &tab, "Rounding", GUI_BORDER, control->round_tab);
        round_tab(&tab, config);
        gui_panel_tab_end(&layout, &tab);

        control->color_tab = gui_panel_tab_begin(&layout, &tab, "Color", GUI_BORDER, control->color_tab);
        color_tab(&tab, control, config);
        gui_panel_tab_end(&layout, &tab);
    }
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
    gui->control.header_flags = gui->show.header_flags;
}

static void
run_demo(struct demo_gui *gui, struct gui_input *input)
{
    gui_flags prev;
    struct control_window *control = &gui->control;
    struct show_window *show = &gui->show;

    gui->running = update_control(control, &gui->stack, input, &gui->config);

    if (show->hook.flags & GUI_PANEL_ACTIVE)
        show->hook.flags = control->show_flags|GUI_PANEL_ACTIVE;
    else show->hook.flags = control->show_flags;
    gui->show.header_flags = gui->control.header_flags;

    prev = show->hook.flags;
    update_show(show, &gui->stack, input, &gui->config);
    if (show->hook.flags & GUI_PANEL_HIDDEN)
        control->show_flags |= GUI_PANEL_HIDDEN;
    if (show->hook.flags & GUI_PANEL_MINIMIZED && !(prev & GUI_PANEL_MINIMIZED))
        control->show_flags |= GUI_PANEL_MINIMIZED;
    else if (prev & GUI_PANEL_MINIMIZED && !(show->hook.flags & GUI_PANEL_MINIMIZED))
        control->show_flags &= ~(gui_flags)GUI_PANEL_MINIMIZED;
}

