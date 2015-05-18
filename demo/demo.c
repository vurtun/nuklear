#define MAX_BUFFER  64
#define MAX_MEMORY  (64 * 1024)

struct settings_window {
    struct gui_panel_hook hook;

    /* brush tab */
    gui_bool brush_tab;
    gui_bool radius_u_active;
    gui_bool radius_l_active;
    gui_float radiusu;
    gui_float radiusl;
    gui_bool rotate_to_stroke;

    /* color tab */
    gui_bool color_tab;
    struct gui_color color;
    gui_float opacity;
    gui_bool opacity_active;

    /* floot tab */
    gui_bool flood_tab;
    struct gui_color flood_color;
    gui_float flood_opacity;
    gui_size flood_type;

    /* paint tabs */
    gui_bool paint_tab;
    gui_size artisan_type;
    gui_size effect_type;
    gui_size brush_mode;

    /* texture tab */
    gui_bool texture_tab;
    gui_size attribute;
    gui_bool update_on_stroke;
    gui_bool save_on_stroke;
    gui_bool extend_seam_color;
};

struct show_window {
    struct gui_panel_hook hook;
    /* input buffer */
    gui_char input_buffer[MAX_BUFFER];
    gui_size input_length;
    gui_bool input_active;
    /* command buffer */
    gui_char command_buffer[MAX_BUFFER];
    gui_size command_length;
    gui_bool command_active;
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
    /* tabs */
    gui_bool combobox_tab;
    gui_bool widget_tab;
    gui_bool table_tab;
    /* scrollbars */
    gui_float shelf_scrollbar;
    gui_float table_scrollbar;
};

struct control_window {
    struct gui_panel_hook hook;
    gui_flags show_flags;
    /* tabs */
    gui_bool flag_tab;
    gui_bool style_tab;
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
    struct show_window show;
    struct control_window control;
    struct settings_window settings;

    struct gui_memory memory;
    struct gui_command_buffer buffer;

    struct gui_layout_config conf;
    struct gui_panel_stack background;
    struct gui_layout layout;

    struct gui_panel_stack floating;
    struct gui_panel_layout tab;
    struct gui_config config;
    struct gui_font font;
    gui_size width, height;
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
    char buffer[MAX_BUFFER];
    const char *items[] = {"Fist", "Pistol", "Shotgun", "Railgun", "BFG"};

    gui_panel_row(panel, 30, 1);
    gui_panel_label(panel, "text left", GUI_TEXT_LEFT);
    gui_panel_label(panel, "text center", GUI_TEXT_CENTERED);
    gui_panel_label(panel, "text right", GUI_TEXT_RIGHT);
    if (gui_panel_button_text(panel, "button", GUI_BUTTON_DEFAULT))
        fprintf(stdout, "button pressed!\n");
    demo->toggle = gui_panel_button_toggle(panel, "toggle", demo->toggle);
    demo->checkbox = gui_panel_check(panel, "checkbox", demo->checkbox);

    gui_panel_row(panel, 30, 2);
    if (gui_panel_option(panel, "option 0", demo->option == 0)) demo->option = 0;
    if (gui_panel_option(panel, "option 1", demo->option == 1)) demo->option = 1;
    demo->slider = gui_panel_slider(panel, 0, demo->slider, 10, 1.0f);
    sprintf(buffer, "%.2f", demo->slider);
    gui_panel_label(panel, buffer, GUI_TEXT_LEFT);
    demo->progressbar = gui_panel_progress(panel, demo->progressbar, 100, gui_true);
    sprintf(buffer, "%lu", demo->progressbar);
    gui_panel_label(panel, buffer, GUI_TEXT_LEFT);

    gui_panel_row(panel, 30, 1);
    demo->item_current = gui_panel_selector(panel, items, LEN(items), demo->item_current);
    demo->spinner = gui_panel_spinner(panel, 0, demo->spinner, 250, 10, &demo->spinner_active);
    if (gui_panel_shell(panel, demo->command_buffer, &demo->command_length, MAX_BUFFER, &demo->command_active))
        demo->command_length = 0;
    demo->input_length = gui_panel_edit(panel, demo->input_buffer, demo->input_length,
        MAX_BUFFER, &demo->input_active, GUI_INPUT_DEFAULT);
}

static void
graph_panel(struct gui_panel_layout *panel, gui_size current)
{
    enum {HISTO, PLOT};
    static const gui_float values[] = {8.0f,15.0f,20.0f,12.0f,30.0f,12.0f,35.0f,40.0f,20.0f};
    gui_panel_row(panel, 100, 1);
    if (current == HISTO) {
        gui_panel_graph(panel, GUI_GRAPH_HISTO, values, LEN(values));
    } else {
        gui_panel_graph(panel, GUI_GRAPH_LINES, values, LEN(values));
    }
}

static void
table_panel(struct gui_panel_layout *panel)
{
    static const struct gui_color header = {178, 122, 1, 255};
    gui_panel_table_begin(panel, GUI_TABLE_HHEADER, 30, 2);
    gui_panel_label_colored(panel, "MOVEMENT", GUI_TEXT_CENTERED, header);
    gui_panel_label_colored(panel, "KEY/BUTTON", GUI_TEXT_CENTERED, header);
    gui_panel_table_row(panel);
    gui_panel_label(panel, "Move foward", GUI_TEXT_LEFT);
    gui_panel_label(panel, "w", GUI_TEXT_CENTERED);
    gui_panel_table_row(panel);
    gui_panel_label(panel, "Move back", GUI_TEXT_LEFT);
    gui_panel_label(panel, "s", GUI_TEXT_CENTERED);
    gui_panel_table_row(panel);
    gui_panel_label(panel, "Move left", GUI_TEXT_LEFT);
    gui_panel_label(panel, "a", GUI_TEXT_CENTERED);
    gui_panel_table_row(panel);
    gui_panel_label(panel, "Move right", GUI_TEXT_LEFT);
    gui_panel_label(panel, "d", GUI_TEXT_CENTERED);
    gui_panel_table_row(panel);
    gui_panel_label(panel, "Jump", GUI_TEXT_LEFT);
    gui_panel_label(panel, "SPACE", GUI_TEXT_CENTERED);
    gui_panel_table_row(panel);
    gui_panel_label(panel, "Duck", GUI_TEXT_LEFT);
    gui_panel_label(panel, "CTRL", GUI_TEXT_CENTERED);
    gui_panel_table_end(panel);
}

static void
init_show(struct show_window *win, struct gui_config *config, struct gui_font *font,
        struct gui_panel_stack *stack)
{
    gui_panel_hook_init(&win->hook, 50, 50, 300, 500,
        GUI_PANEL_BORDER|GUI_PANEL_MOVEABLE|
        GUI_PANEL_CLOSEABLE|GUI_PANEL_SCALEABLE|
        GUI_PANEL_MINIMIZABLE, config, font);
    gui_stack_push(stack, &win->hook);

    win->widget_tab = GUI_MINIMIZED;
    win->combobox_tab = GUI_MINIMIZED;
    win->slider = 5.0f;
    win->progressbar = 50;
    win->spinner = 100;
}

static void
update_show(struct show_window *show, struct gui_panel_stack *stack,
    struct gui_input *in, struct gui_canvas *canvas)
{
    struct gui_panel_layout layout;
    struct gui_panel_layout tab;
    static const char *shelfs[] = {"Histogram", "Lines"};
    gui_panel_hook_begin_stacked(&layout, &show->hook, stack, "Show", canvas, in);

    show->combobox_tab = gui_panel_tab_begin(&layout, &tab, "Combobox", show->combobox_tab);
    combobox_panel(&tab, show);
    gui_panel_tab_end(&layout, &tab);

    show->widget_tab = gui_panel_tab_begin(&layout, &tab, "Widgets", show->widget_tab);
    widget_panel(&tab, show);
    gui_panel_tab_end(&layout, &tab);

    gui_panel_row(&layout, 180, 1);
    show->shelf_selection = gui_panel_shelf_begin(&layout, &tab, shelfs,
        LEN(shelfs), show->shelf_selection, show->shelf_scrollbar);
    graph_panel(&tab, show->shelf_selection);
    show->shelf_scrollbar = gui_panel_shelf_end(&layout, &tab);

    gui_panel_row(&layout, 180, 1);
    gui_panel_group_begin(&layout, &tab, "Table", show->table_scrollbar);
    table_panel(&tab);
    show->table_scrollbar = gui_panel_group_end(&layout, &tab);

    gui_panel_hook_end(&layout, &show->hook);
}

static void
update_flags(struct gui_panel_layout *panel, struct control_window *control)
{
    gui_flags i = 0x01;
    gui_size n = 0;
    gui_flags res = 0;
    const char *options[] = {"Hidden", "Border", "Minimizable", "Closeable", "Moveable", "Scaleable"};
    gui_panel_row(panel, 30, 2);
    do {
        if (gui_panel_check(panel, options[n++], (control->show_flags & i) ? gui_true : gui_false))
            res |= i;
        i = i << 1;
    } while (i <= GUI_PANEL_SCALEABLE);
    control->show_flags = res;
}

static void
style_tab(struct gui_panel_layout *panel, struct gui_config *config)
{
    gui_int tx, ty;

    gui_panel_label(panel, "scrollbar width:", GUI_TEXT_LEFT);
    tx = gui_panel_spinner(panel, 0, (gui_int)config->scrollbar_width, 20, 1, NULL);
    config->scrollbar_width = (float)tx;

    gui_panel_row(panel, 30, 3);
    gui_panel_label(panel, "padding:", GUI_TEXT_LEFT);
    tx = gui_panel_spinner(panel, 0, (gui_int)config->panel_padding.x, 20, 1, NULL);
    ty = gui_panel_spinner(panel, 0, (gui_int)config->panel_padding.y, 20, 1, NULL);
    config->panel_padding.x = (float)tx;
    config->panel_padding.y = (float)ty;

    gui_panel_label(panel, "item spacing:", GUI_TEXT_LEFT);
    tx = gui_panel_spinner(panel, 0, (gui_int)config->item_spacing.x, 20, 1, NULL);
    ty = gui_panel_spinner(panel, 0, (gui_int)config->item_spacing.y, 20, 1, NULL);
    config->item_spacing.x = (float)tx;
    config->item_spacing.y = (float)ty;

    gui_panel_label(panel, "item padding:", GUI_TEXT_LEFT);
    tx = gui_panel_spinner(panel, 0, (gui_int)config->item_padding.x, 20, 1, NULL);
    ty = gui_panel_spinner(panel, 0, (gui_int)config->item_padding.y, 20, 1, NULL);
    config->item_padding.x = (float)tx;
    config->item_padding.y = (float)ty;

    gui_panel_label(panel, "scaler size:", GUI_TEXT_LEFT);
    tx = gui_panel_spinner(panel, 0, (gui_int)config->scaler_size.x, 20, 1, NULL);
    ty = gui_panel_spinner(panel, 0, (gui_int)config->scaler_size.y, 20, 1, NULL);
    config->scaler_size.x = (float)tx;
    config->scaler_size.y = (float)ty;
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
    color.r = (gui_byte)gui_panel_spinner(panel, 0, color.r, 255, 1, &control->spinner_r_active);

    g = gui_panel_slider(panel, 0, g, 255, 10);
    color.g = (gui_byte)g;
    color.g = (gui_byte)gui_panel_spinner(panel, 0, color.g, 255, 1, &control->spinner_g_active);

    b = gui_panel_slider(panel, 0, b, 255, 10);
    color.b = (gui_byte)b;
    color.b = (gui_byte)gui_panel_spinner(panel, 0,(gui_int)color.b, 255, 1, &control->spinner_b_active);

    a = gui_panel_slider(panel, 0, a, 255, 10);
    color.a = (gui_byte)a;
    color.a = (gui_byte)gui_panel_spinner(panel, 0, (gui_int)color.a, 255, 1, &control->spinner_a_active);
    return color;
}

static void
color_tab(struct gui_panel_layout *panel, struct control_window *control, struct gui_config *config)
{
    gui_size i = 0;
    static const char *labels[] = {"Text:", "Panel:", "Header:", "Border:", "Button:",
        "Button Border:", "Button Hovering:", "Button Toggle:", "Button Hovering Text:",
        "Check:", "Check BG:", "Check Active:", "Option:", "Option BG:", "Option Active:",
        "Slider:", "Slider cursor:", "Progress:", "Progress Cursor:", "Editbox:", "Editbox cursor:",
        "Editbox Border:", "Spinner:", "Spinner Border:", "Selector:", "Selector Border:",
        "Histo:", "Histo Bars:", "Histo Negative:", "Histo Hovering:", "Plot:", "Plot Lines:",
        "Plot Hightlight:", "Scrollbar:", "Scrollbar Cursor:", "Scrollbar Border:",
        "Table lines:", "Shelf:", "Shelf Text:", "Shelf Active:", "Shelf Active Text:", "Scaler:"
    };

    if (control->picker_active) {
        control->color = color_picker(panel,control,labels[control->current_color], control->color);
        gui_panel_row(panel, 30, 3);
        gui_panel_seperator(panel, 1);
        if (gui_panel_button_text(panel, "ok", GUI_BUTTON_DEFAULT)) {
            config->colors[control->current_color] = control->color;
            control->picker_active = gui_false;
        }
        if (gui_panel_button_text(panel, "cancel", GUI_BUTTON_DEFAULT))
            control->picker_active = gui_false;
    } else {
        gui_panel_row(panel, 30, 2);
        for (i = 0; i < GUI_COLOR_COUNT; ++i) {
            struct gui_panel_layout layout;
            gui_panel_label(panel, labels[i], GUI_TEXT_LEFT);
            if (gui_panel_button_color(panel, config->colors[i], GUI_BUTTON_DEFAULT)) {
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
init_control(struct control_window *win, struct gui_config *config, struct gui_font *font,
        struct gui_panel_stack *stack)
{
    gui_panel_hook_init(&win->hook, 380, 50, 400, 350,
        GUI_PANEL_BORDER|GUI_PANEL_MOVEABLE|GUI_PANEL_CLOSEABLE|GUI_PANEL_SCALEABLE, config, font);
    gui_stack_push(stack, &win->hook);
    win->show_flags = gui_hook_panel(&win->hook)->flags;
    win->style_tab = GUI_MINIMIZED;
    win->color_tab = GUI_MINIMIZED;
}

static gui_bool
update_control(struct control_window *control, struct gui_panel_stack *stack,
    struct gui_input *in, struct gui_canvas *canvas, struct gui_config *config)
{
    gui_bool running;
    struct gui_panel_layout layout;
    struct gui_panel_layout tab;

    running = gui_panel_hook_begin_stacked(&layout, &control->hook, stack, "Control", canvas, in);
    control->flag_tab = gui_panel_tab_begin(&layout, &tab, "Options", control->flag_tab);
    update_flags(&tab, control);
    gui_panel_tab_end(&layout, &tab);

    control->style_tab = gui_panel_tab_begin(&layout, &tab, "Style", control->style_tab);
    style_tab(&tab, config);
    gui_panel_tab_end(&layout, &tab);

    control->color_tab = gui_panel_tab_begin(&layout, &tab, "Color", control->color_tab);
    color_tab(&tab, control, config);
    gui_panel_tab_end(&layout, &tab);

    gui_panel_hook_end(&layout, &control->hook);
    return running;
}


static void
brush_tab(struct gui_panel_layout *panel, struct settings_window *win)
{
    gui_panel_row(panel, 20, 2);
    gui_panel_label(panel, "Radius (U):", GUI_TEXT_RIGHT);
    win->radiusu = gui_panel_slider(panel, 0, win->radiusu, 20, 0.5);
    gui_panel_label(panel, "Radius (L):", GUI_TEXT_RIGHT);
    win->radiusl = gui_panel_slider(panel, 0, win->radiusl, 20, 0.5);

    gui_panel_row(panel, 25, 3);
    gui_panel_seperator(panel, 1);
    win->rotate_to_stroke = gui_panel_check(panel, "Rotate to stroke", win->rotate_to_stroke);
}

static struct gui_color
scolor_tab(struct gui_panel_layout *panel, struct settings_window *win)
{
    char buffer[256];
    struct gui_color color = win->color;
    gui_float c = color.r;

    gui_panel_row(panel, 20, 3);
    gui_panel_label(panel, "Color:", GUI_TEXT_RIGHT);
    c = gui_panel_slider(panel, 0, c, 250, 10);
    color.r = (gui_byte)c;
    color.g = (gui_byte)c;
    color.b = (gui_byte)c;
    color.a = 255;
    gui_panel_button_color(panel, color, GUI_BUTTON_DEFAULT);

    gui_panel_label(panel, "Opacity:", GUI_TEXT_RIGHT);
    win->flood_opacity = gui_panel_slider(panel, 0, win->flood_opacity, 250, 10);
    sprintf(buffer, "%.2f", win->flood_opacity);
    gui_panel_label(panel, buffer, GUI_TEXT_LEFT);
    return color;
}

static struct gui_color
flood_tab(struct gui_panel_layout *panel, struct settings_window *win)
{
    char buffer[256];
    const char *flood_types[] = {"All", "Selected"};
    struct gui_color color = win->flood_color;
    gui_float c = color.r;

    gui_panel_row(panel, 25, 3);
    gui_panel_label(panel, "Color:", GUI_TEXT_RIGHT);
    c = gui_panel_slider(panel, 0, c, 250, 10);
    color.r = (gui_byte)c;
    color.g = (gui_byte)c;
    color.b = (gui_byte)c;
    color.a = 255;
    gui_panel_button_color(panel, color, GUI_BUTTON_DEFAULT);

    gui_panel_label(panel, "Opacity:", GUI_TEXT_RIGHT);
    win->opacity = gui_panel_slider(panel, 0, win->opacity, 250, 10);
    sprintf(buffer, "%.2f", win->opacity);
    gui_panel_label(panel, buffer, GUI_TEXT_LEFT);

    gui_panel_row(panel, 25, 2);
    if (gui_panel_button_text(panel, "Flood Paint", GUI_BUTTON_DEFAULT))
        fprintf(stdout, "flood paint pressed!\n");
    if (gui_panel_button_text(panel, "Flood Erease", GUI_BUTTON_DEFAULT))
        fprintf(stdout, "flood erase pressed!\n");

    gui_panel_row(panel, 25, LEN(flood_types) + 1);
    gui_panel_label(panel, "Flood:", GUI_TEXT_RIGHT);
    win->flood_type = gui_panel_option_group(panel, flood_types, LEN(flood_types), win->flood_type);
    return color;
}


static void
paint_tab(struct gui_panel_layout *panel, struct settings_window *win)
{
    const char *brush_mode[] = {"Dynamic", "Static"};
    const char *artisan_types[] = {"Paint", "Erase", "Clone"};
    const char *paint_effects[] = {"Paint", "Smear", "Blur"};
    gui_panel_row(panel, 25, LEN(artisan_types) + 1);
    gui_panel_label(panel, "Artisan:", GUI_TEXT_RIGHT);
    win->artisan_type = gui_panel_option_group(panel, artisan_types, LEN(artisan_types), win->artisan_type);
    gui_panel_row(panel, 25, LEN(paint_effects) + 1);
    gui_panel_label(panel, "Effects:", GUI_TEXT_RIGHT);
    win->effect_type = gui_panel_option_group(panel, paint_effects, LEN(paint_effects), win->effect_type);

    gui_panel_row(panel, 25, 3);
    gui_panel_seperator(panel, 1);
    if (gui_panel_button_text(panel, "Erase", GUI_BUTTON_DEFAULT))
        fprintf(stdout, "set erase image button pressed!\n");

    gui_panel_row(panel, 25, 3);
    gui_panel_seperator(panel, 1);
    if (gui_panel_button_text(panel, "Reset", GUI_BUTTON_DEFAULT))
        fprintf(stdout, "reset brushes button pressed!\n");

    gui_panel_row(panel, 25, LEN(brush_mode) + 1);
    gui_panel_label(panel, "Brush mode:", GUI_TEXT_RIGHT);
    win->brush_mode = gui_panel_option_group(panel, brush_mode, LEN(brush_mode), win->brush_mode);
}

static void
texture_tab(struct gui_panel_layout *panel, struct settings_window *win)
{
    const char *attributes[] = {"Color", "Normals", "Depth"};
    gui_panel_row(panel, 25, 3);
    gui_panel_label(panel, "Attribute:", GUI_TEXT_RIGHT);
    win->attribute = gui_panel_selector(panel, attributes, LEN(attributes), win->attribute);
    gui_panel_seperator(panel, 2);
    if (gui_panel_button_text(panel, "Assign", GUI_BUTTON_DEFAULT))
        fprintf(stdout, "assign/edit textures button pressed!\n");
    gui_panel_seperator(panel, 2);
    if (gui_panel_button_text(panel, "Save", GUI_BUTTON_DEFAULT))
        fprintf(stdout, "save textures button pressed!\n");
    gui_panel_seperator(panel, 2);
    if (gui_panel_button_text(panel, "Reload", GUI_BUTTON_DEFAULT))
        fprintf(stdout, "reload textures button pressed!\n");

    gui_panel_seperator(panel, 2);
    win->update_on_stroke = gui_panel_check(panel, "Update on stroke", win->update_on_stroke);
    gui_panel_seperator(panel, 2);
    win->save_on_stroke = gui_panel_check(panel, "Save texture on stroke", win->save_on_stroke);
    gui_panel_seperator(panel, 2);
    win->extend_seam_color = gui_panel_check(panel, "Extend seam color", win->extend_seam_color);
}

static void
update_settings(struct settings_window *win, struct gui_layout *layout,
    struct gui_input *in, struct gui_canvas *canvas)
{
    gui_bool running;
    struct gui_panel_layout panel;
    struct gui_panel_layout tab;
    gui_panel_hook_begin_tiled(&panel, &win->hook, layout, GUI_SLOT_RIGHT, 0, "Tool Settings", canvas, in);

    win->brush_tab = gui_panel_tab_begin(&panel, &tab, "Brush", win->brush_tab);
    brush_tab(&tab, win);
    gui_panel_tab_end(&panel, &tab);

    win->color_tab = gui_panel_tab_begin(&panel, &tab, "Color", win->color_tab);
    win->color = scolor_tab(&tab, win);
    gui_panel_tab_end(&panel, &tab);

    win->flood_tab = gui_panel_tab_begin(&panel, &tab, "Flood", win->flood_tab);
    win->flood_color = flood_tab(&tab, win);
    gui_panel_tab_end(&panel, &tab);

    win->paint_tab = gui_panel_tab_begin(&panel, &tab, "Paint", win->paint_tab);
    paint_tab(&tab, win);
    gui_panel_tab_end(&panel, &tab);

    win->texture_tab = gui_panel_tab_begin(&panel, &tab, "Textures", win->texture_tab);
    texture_tab(&tab, win);
    gui_panel_tab_end(&panel, &tab);

    gui_panel_hook_end(&panel, &win->hook);
}

static void
init_demo(struct demo_gui *gui, struct gui_font *font)
{
    struct gui_layout_config ratio;
    struct gui_command_buffer *buffer = &gui->buffer;
    struct gui_memory *memory = &gui->memory;
    struct gui_config *config = &gui->config;

    gui->font = *font;
    memory->memory = calloc(MAX_MEMORY, 1);
    memory->size = MAX_MEMORY;
    gui_buffer_init_fixed(buffer, memory, GUI_BUFFER_CLIPPING);
    gui_default_config(config);

    ratio.left = 0.10f;
    ratio.right = 0.40f;
    ratio.centerv = 0.9f;
    ratio.centerh = 0.5f;
    ratio.bottom = 0.05f;
    ratio.top = 0.05f;
    gui_layout_init(&gui->layout, &ratio);
    gui_panel_hook_init(&gui->settings.hook, 0, 0, 0, 0, GUI_PANEL_BORDER, config, font);

    gui_stack_clear(&gui->floating);
    init_show(&gui->show, config, font, &gui->floating);
    init_control(&gui->control, config, font, &gui->floating);
}

static void
background_demo(struct demo_gui *gui, struct gui_input *input, struct gui_command_buffer *buffer,
    gui_bool active)
{
    struct gui_command_buffer sub;
    struct gui_canvas canvas;
    struct settings_window *settings = &gui->settings;

    gui_layout_begin(&gui->layout, gui->width, gui->height, active);
    gui_layout_slot(&gui->layout, GUI_SLOT_RIGHT, GUI_LAYOUT_VERTICAL, 1);

    gui_buffer_lock(&canvas, buffer, &sub, 0, gui->width, gui->height);
    update_settings(&gui->settings, &gui->layout, input, &canvas);
    gui_buffer_unlock(gui_hook_output(&settings->hook), buffer, &sub, &canvas, NULL);
    gui_layout_end(&gui->background, &gui->layout);
}

static gui_bool
floating_demo(struct demo_gui *gui, struct gui_input *input, struct gui_command_buffer *buffer)
{
    gui_bool running;
    struct show_window *show = &gui->show;
    struct control_window *control = &gui->control;
    struct gui_command_buffer sub;
    struct gui_canvas canvas;

    /* Show window */
    gui_buffer_lock(&canvas, buffer, &sub, 0, gui->width, gui->height);
    running = update_control(control, &gui->floating, input, &canvas, &gui->config);
    gui_buffer_unlock(gui_hook_output(&control->hook), buffer, &sub, &canvas, NULL);

    /* control window */
    gui_hook_panel(&show->hook)->flags = control->show_flags;
    gui_buffer_lock(&canvas, buffer, &sub, 0, gui->width, gui->height);
    update_show(show, &gui->floating, input, &canvas);
    if (gui_hook_panel(&show->hook)->flags & GUI_PANEL_HIDDEN)
        control->show_flags |= GUI_PANEL_HIDDEN;
    gui_buffer_unlock(gui_hook_output(&show->hook), buffer, &sub, &canvas, NULL);
    return running;
}

static gui_bool
run_demo(struct demo_gui *gui, struct gui_input *input)
{
    gui_bool running;
    struct gui_command_buffer *buffer = &gui->buffer;
    gui_buffer_begin(NULL, buffer, gui->width, gui->height);
    background_demo(gui, input, buffer, gui->control.show_flags & GUI_PANEL_HIDDEN);
    running = floating_demo(gui, input, buffer);
    gui_buffer_end(NULL, buffer, NULL, NULL);
    return running;
}

