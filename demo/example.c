#define MAX_BUFFER 64

struct demo {
    gui_char in_buf[MAX_BUFFER];
    gui_size in_len;
    gui_bool in_act;
    gui_char cmd_buf[MAX_BUFFER];
    gui_size cmd_len;
    gui_bool cmd_act;
    gui_bool check;
    gui_int option;
    gui_float slider;
    gui_size prog;
    gui_int spinner;
    gui_bool spin_act;
    gui_size item_cur;
    gui_tab tab;
    gui_group group;
    gui_shelf shelf;
    gui_size current;
};

struct color_picker {
    gui_bool active[4];
    struct gui_color color;
};

static char*
ldfile(const char* path, size_t* siz)
{
    char *buf;
    FILE *fd = fopen(path, "rb");
    if (!fd) exit(EXIT_FAILURE);
    fseek(fd, 0, SEEK_END);
    *siz = (size_t)ftell(fd);
    fseek(fd, 0, SEEK_SET);
    buf = calloc(*siz, 1);
    fread(buf, *siz, 1, fd);
    fclose(fd);
    return buf;
}

static gui_int
message_panel(struct gui_context *ctx, struct gui_panel *panel)
{
    gui_int ret = -1;
    gui_begin_panel(ctx, panel, "Error", GUI_PANEL_MOVEABLE|GUI_PANEL_BORDER);
    gui_panel_layout(panel, 30, 2);
    if (gui_panel_button_text(panel, "ok", GUI_BUTTON_DEFAULT)) ret = 1;
    if (gui_panel_button_text(panel, "cancel", GUI_BUTTON_DEFAULT)) ret = 0;
    if (ret != -1) gui_panel_hide(panel);
    gui_end_panel(ctx, panel, NULL);
    return ret;
}

static gui_int
color_picker_panel(struct gui_context *ctx, struct gui_panel *panel, struct color_picker *picker)
{
    gui_size i;
    gui_int ret = -1;
    gui_byte *ptr = &picker->color.r;
    const char str[] = "R:G:B:A:";
    gui_begin_panel(ctx, panel, "Color Picker", GUI_PANEL_MOVEABLE|GUI_PANEL_BORDER);
    gui_panel_layout(panel, 30, 3);
    for (i = 0; i < 4; ++i) {
        gui_int ivalue;
        gui_float fvalue = (gui_float)*ptr;
        gui_panel_text(panel, &str[i*2], 2, GUI_TEXT_CENTERED);
        fvalue = gui_panel_slider(panel, 0, fvalue, 255.0f, 10.0f, GUI_HORIZONTAL);
        ivalue = (gui_int)fvalue;
        picker->active[i] = gui_panel_spinner(panel, 0, &ivalue, 255, 1, picker->active[i]);
        *ptr = (gui_byte)ivalue;
        ptr++;
    }

    gui_panel_layout(panel, 30, 4);
    gui_panel_seperator(panel, 1);
    if (gui_panel_button_text(panel, "ok", GUI_BUTTON_DEFAULT)) ret = 1;
    if (gui_panel_button_text(panel, "cancel", GUI_BUTTON_DEFAULT)) ret = 0;
    if (ret != -1) gui_panel_hide(panel);
    gui_end_panel(ctx, panel, NULL);
    return ret;
}

static gui_bool
demo_panel(struct gui_context *ctx, struct gui_panel *panel, struct demo *demo)
{
    enum {PLOT, HISTO};
    const char *shelfs[] = {"Histogram", "Lines"};
    const gui_float values[] = {8.0f, 15.0f, 20.0f, 12.0f, 30.0f};
    const char *items[] = {"Fist", "Pistol", "Shotgun", "Railgun", "BFG"};
    gui_bool running;

    running = gui_begin_panel(ctx, panel, "Demo",
        GUI_PANEL_CLOSEABLE|GUI_PANEL_MINIMIZABLE|GUI_PANEL_SCALEABLE|
        GUI_PANEL_MOVEABLE|GUI_PANEL_BORDER);

    /* Tabs */
    gui_panel_layout(panel, 100, 1);
    gui_panel_tab_begin(panel, &demo->tab, "Difficulty");
    gui_panel_layout(&demo->tab, 30, 2);
    if (gui_panel_option(&demo->tab, "easy", demo->option == 0)) demo->option = 0;
    if (gui_panel_option(&demo->tab, "hard", demo->option == 1)) demo->option = 1;
    if (gui_panel_option(&demo->tab, "normal", demo->option == 2)) demo->option = 2;
    if (gui_panel_option(&demo->tab, "godlike", demo->option == 3)) demo->option = 3;
    gui_panel_tab_end(panel, &demo->tab);

    /* Shelf */
    gui_panel_layout(panel, 200, 2);
    demo->current = gui_panel_shelf_begin(panel, &demo->shelf, shelfs, LEN(shelfs), demo->current);
    gui_panel_layout(&demo->shelf, 100, 1);
    if (demo->current == PLOT) {
        gui_panel_histo(&demo->shelf, values, LEN(values));
    } else {
        gui_panel_plot(&demo->shelf, values, LEN(values));
    }
    gui_panel_shelf_end(panel, &demo->shelf);

    /* Group */
    gui_panel_group_begin(panel, &demo->group, "Options");
    gui_panel_layout(&demo->group, 30, 1);
    if (gui_panel_button_text(&demo->group, "button", GUI_BUTTON_DEFAULT))
        fprintf(stdout, "button pressed!\n");
    demo->check = gui_panel_check(&demo->group, "advanced", demo->check);
    demo->slider = gui_panel_slider(&demo->group, 0, demo->slider, 10, 1.0f, GUI_HORIZONTAL);
    demo->prog = gui_panel_progress(&demo->group, demo->prog, 100, gui_true, GUI_HORIZONTAL);
    if (gui_panel_shell(&demo->group, demo->cmd_buf, &demo->cmd_len, MAX_BUFFER, &demo->cmd_act))
        fprintf(stdout, "shell executed!\n");
    demo->spin_act = gui_panel_spinner(&demo->group, 0, &demo->spinner, 1024, 10, demo->spin_act);
    demo->in_act = gui_panel_input(&demo->group, demo->in_buf, &demo->in_len, MAX_BUFFER,
                                    GUI_INPUT_DEFAULT, demo->in_act);
    demo->item_cur = gui_panel_selector(&demo->group, items, LEN(items), demo->item_cur);
    gui_panel_group_end(panel, &demo->group);

    gui_end_panel(ctx, panel, NULL);
    return running;
}

