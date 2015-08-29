#define MAX_BUFFER  64
#define MAX_MEMORY  (16 * 1024)
#define WINDOW_WIDTH 1200
#define WINDOW_HEIGHT 800

#include <stdio.h>

#define WEAPON_MAP(WEAPON)\
    WEAPON(FIST, "fist")\
    WEAPON(PISTOL, "pistol")\
    WEAPON(SHOTGUN, "shotgun")\
    WEAPON(RAILGUN, "railgun")\
    WEAPON(BFG, "bfg")

#define MENU_FILE_ITEMS(ITEM)\
    ITEM(OPEN, "open")\
    ITEM(CLOSE, "close")\
    ITEM(QUIT, "quit")

#define MENU_EDIT_ITEMS(ITEM)\
    ITEM(COPY, "copy")\
    ITEM(CUT, "cut")\
    ITEM(DELETE, "delete")\
    ITEM(PASTE, "paste")

#define COLOR_MAP(COLOR)\
    COLOR(text)\
    COLOR(text_hovering)\
    COLOR(text_active)\
    COLOR(window)\
    COLOR(header)\
    COLOR(border)\
    COLOR(button)\
    COLOR(button_hover)\
    COLOR(button_active)\
    COLOR(toggle)\
    COLOR(toggle_hover)\
    COLOR(toggle_cursor)\
    COLOR(slider)\
    COLOR(slider_cursor)\
    COLOR(slider_cursor_hover)\
    COLOR(slider_cursor_active)\
    COLOR(progress)\
    COLOR(progress_cursor)\
    COLOR(progress_cursor_hover)\
    COLOR(progress_cursor_active)\
    COLOR(input)\
    COLOR(input_cursor)\
    COLOR(input_text)\
    COLOR(spinner)\
    COLOR(spinner_triangle)\
    COLOR(histo)\
    COLOR(histo_bars)\
    COLOR(histo_negative)\
    COLOR(histo_highlight)\
    COLOR(plot)\
    COLOR(plot_lines)\
    COLOR(plot_highlight)\
    COLOR(scrollbar)\
    COLOR(scrollbar_cursor)\
    COLOR(scrollbar_cursor_hover)\
    COLOR(scrollbar_cursor_active)\
    COLOR(table_lines)\
    COLOR(tab_header)\
    COLOR(shelf)\
    COLOR(shelf_text)\
    COLOR(shelf_active)\
    COLOR(shelf_active_text)\
    COLOR(scaler)

enum weapon_types {
#define WEAPON(id, name) WEAPON_##id,
    WEAPON_MAP(WEAPON)
#undef WEAPON
    WEAPON_MAX
};
enum menu_file_items {
#define ITEM(id, name) MENU_FILE_##id,
    MENU_FILE_ITEMS(ITEM)
#undef ITEM
    MENU_FILE_MAX
};
enum menu_edit_items {
#define ITEM(id, name) MENU_EDIT_##id,
    MENU_EDIT_ITEMS(ITEM)
#undef ITEM
    MENU_EDIT_MAX
};

static const char *weapons[] = {
#define WEAPON(id,name) name,
    WEAPON_MAP(WEAPON)
#undef WEAPON
};
static const char *file_items[] = {
#define ITEM(id,name) name,
    MENU_FILE_ITEMS(ITEM)
#undef ITEM
};
static const char *edit_items[] = {
#define ITEM(id,name) name,
    MENU_EDIT_ITEMS(ITEM)
#undef ITEM
};
static const char *colors[] = {
#define COLOR(name) #name,
    COLOR_MAP(COLOR)
#undef COLOR
};

/* =================================================================
 *
 *                      CUSTOM WIDGET
 *
 * =================================================================
 */
/* -----------------------------------------------------------------
 *  TREE WIDGET
 * ----------------------------------------------------------------- */
struct tree_node {
    gui_state state;
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

static void
tree_init(struct test_tree *tree)
{
    /* this is just test data */
    tree->root.state = GUI_NODE_ACTIVE;
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

    tree->nodes[4].state = GUI_NODE_ACTIVE;
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
upload_tree(struct test_tree *base, struct gui_tree *tree, struct tree_node *node)
{
    int i = 0, n = 0;
    enum gui_tree_node_operation op;
    if (node->count) {
        i = 0;
        op = gui_tree_begin_node(tree, node->name, &node->state);
        while (i < node->count)
            i += upload_tree(base, tree, node->children[i]);
        gui_tree_end_node(tree);
    }
    else op = gui_tree_leaf(tree, node->name, &node->state);

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
        while (i++ < n)
            tree_add_node(node, tree_pop_node(base));
    case GUI_NODE_CLONE:
    default:break;
    }
    return 1;
}

/* -----------------------------------------------------------------
 *  COLOR PICKER POPUP
 * ----------------------------------------------------------------- */
struct color_picker {
    gui_state active;
    struct gui_color color;
    gui_state r, g, b, a;
    gui_size index;
};

static gui_bool
color_picker(struct gui_context *panel, struct color_picker* control,
    const char *name, struct gui_color *color)
{
    int i;
    gui_byte *iter;
    gui_bool ret = gui_true;
    struct gui_context popup;
    gui_popup_begin(panel, &popup, GUI_POPUP_STATIC,0, gui_rect(10, 100, 280, 280), gui_vec2(0,0));
    {
        if (gui_header(&popup, "Color", GUI_CLOSEABLE, GUI_CLOSEABLE, GUI_HEADER_LEFT)) {
            gui_popup_close(&popup);
            return gui_false;
        }
        gui_layout_row_dynamic(&popup, 30, 2);
        gui_label(&popup, name, GUI_TEXT_LEFT);
        gui_button_color(&popup, control->color, GUI_BUTTON_DEFAULT);

        iter = &control->color.r;
        gui_layout_row_dynamic(&popup, 30, 2);
        for (i = 0; i < 4; ++i, iter++) {
            gui_float t = *iter;
            t = gui_slider(&popup, 0, t, 255, 10);
            *iter = (gui_byte)t;
            *iter = (gui_byte)gui_spinner(&popup, 0, *iter, 255, 1, NULL);
        }

        gui_layout_row_dynamic(&popup, 30, 3);
        gui_spacing(&popup, 1);
        if (gui_button_text(&popup, "ok", GUI_BUTTON_DEFAULT)) {
            gui_popup_close(&popup);
            *color = control->color;
            ret = gui_false;
        }
        if (gui_button_text(&popup, "cancel", GUI_BUTTON_DEFAULT)) {
            gui_popup_close(&popup);
            ret = gui_false;
        }
    }
    gui_popup_end(panel, &popup);
    control->active = (gui_state)ret;
    return ret;
}

/* -----------------------------------------------------------------
 *  LABEL
 * ----------------------------------------------------------------- */
static void
gui_labelf(struct gui_context *panel, enum gui_text_align align, const gui_char *fmt, ...)
{
    gui_char buffer[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    buffer[1023] = 0;
    gui_label(panel, buffer, align);
    va_end(args);
}

/* -----------------------------------------------------------------
 *  COMBOBOXES
 * ----------------------------------------------------------------- */
struct combobox {
    gui_size selected;
    gui_state active;
};

struct check_combo_box {
    gui_bool values[4];
    gui_state active;
};

struct prog_combo_box {
    gui_state active;
};

struct color_combo_box {
    gui_state active;
    struct gui_color color;
};

static void
combo_box(struct gui_context *panel, struct combobox *combo,
    const char**values, gui_size count)
{
    gui_combo(panel, values, count, &combo->selected, 20, &combo->active);
}

static void
prog_combo_box(struct gui_context *panel, gui_size *values, gui_size count,
    struct prog_combo_box *demo)
{
    gui_size i = 0;
    gui_int sum = 0;
    gui_char buffer[64];
    struct gui_context combo;
    memset(&combo, 0, sizeof(combo));
    for (i = 0; i < count; ++i)
        sum += (gui_int)values[i];

    sprintf(buffer, "%d", sum);
    gui_combo_begin(panel, &combo, buffer, &demo->active);
    {
        gui_layout_row_dynamic(&combo, 30, 1);
        for (i = 0; i < count; ++i)
            values[i] = gui_progress(&combo, values[i], 100, gui_true);
    }
    gui_combo_end(panel, &combo);
}

static void
color_combo_box(struct gui_context *panel, struct color_combo_box *demo)
{
    /* color slider progressbar */
    gui_char buffer[32];
    struct gui_context combo;
    memset(&combo, 0, sizeof(combo));
    sprintf(buffer, "#%02x%02x%02x%02x", demo->color.r, demo->color.g,
            demo->color.b, demo->color.a);
    gui_combo_begin(panel, &combo, buffer,  &demo->active);
    {
        int i;
        const char *color_names[] = {"R:", "G:", "B:", "A:"};
        gui_float ratios[] = {0.15f, 0.85f};
        gui_byte *iter = &demo->color.r;
        gui_layout_row(&combo, GUI_DYNAMIC, 30, 2, ratios);
        for (i = 0; i < 4; ++i, iter++) {
            gui_float t = *iter;
            gui_label(&combo, color_names[i], GUI_TEXT_LEFT);
            t = gui_slider(&combo, 0, t, 255, 5);
            *iter = (gui_byte)t;
        }
    }
    gui_combo_end(panel, &combo);
}

static void
check_combo_box(struct gui_context *panel, gui_bool *values, gui_size count,
    struct check_combo_box *demo)
{
    /* checkbox combobox  */
    gui_int sum = 0;
    gui_size i = 0;
    gui_char buffer[64];
    struct gui_context combo;
    memset(&combo, 0, sizeof(combo));
    for (i = 0; i < count; ++i)
        sum += (gui_int)values[i];

    sprintf(buffer, "%d", sum);
    gui_combo_begin(panel, &combo, buffer,  &demo->active);
    {
        gui_layout_row_dynamic(&combo, 30, 1);
        for (i = 0; i < count; ++i)
            values[i] = gui_check(&combo, weapons[i], values[i]);
    }
    gui_combo_end(panel, &combo);
}

/* =================================================================
 *
 *                          DEMO
 *
 * =================================================================
 */
struct state {
    gui_char edit_buffer[MAX_BUFFER];
    struct gui_edit_box edit;
    struct color_picker picker;
    struct check_combo_box checkcom;
    struct prog_combo_box progcom;
    struct color_combo_box colcom;
    struct combobox combo;
    struct test_tree test;

    /* widgets state */
    gui_size prog_values[4];
    gui_bool check_values[WEAPON_MAX];
    gui_bool scaleable;
    gui_bool checkbox;
    gui_float slider;
    gui_size progressbar;
    gui_int spinner;
    gui_state spinner_active;
    gui_size item_current;
    gui_size shelf_selection;
    gui_bool toggle;
    gui_int option;
    gui_state popup;
    gui_size cur;
    gui_size op;

    /* subpanels */
    struct gui_vec2 shelf;
    struct gui_vec2 table;
    struct gui_vec2 tree;
    struct gui_vec2 menu;

    /* open/close state */
    gui_state file_open;
    gui_state edit_open;
    gui_state config_tab;
    gui_state widget_tab;
    gui_state combo_tab;
    gui_state style_tab;
    gui_state round_tab;
    gui_state color_tab;
    gui_state flag_tab;
};

struct demo_gui {
    gui_bool running;
    void *memory;
    const struct gui_input *input;
    struct gui_command_queue queue;
    struct gui_style config;
    struct gui_font font;
    struct gui_window panel;
    struct gui_window sub;
    struct state state;
    gui_size w, h;
};

/* -----------------------------------------------------------------
 *  WIDGETS
 * ----------------------------------------------------------------- */
static void
widget_panel(struct gui_context *panel, struct state *demo)
{
    /* Labels */
    gui_layout_row_dynamic(panel, 30, 1);
    demo->scaleable = gui_check(panel, "Scaleable Layout", demo->scaleable);
    if (!demo->scaleable)
        gui_layout_row_static(panel, 30, 150, 1);
    gui_label(panel, "text left", GUI_TEXT_LEFT);
    gui_label(panel, "text center", GUI_TEXT_CENTERED);
    gui_label(panel, "text right", GUI_TEXT_RIGHT);

    /* Buttons */
    if (gui_button_text(panel, "button", GUI_BUTTON_DEFAULT))
        demo->popup = gui_true;
    if (gui_button_text_symbol(panel, GUI_SYMBOL_TRIANGLE_RIGHT, "next", GUI_TEXT_LEFT, GUI_BUTTON_DEFAULT))
        fprintf(stdout, "right triangle button pressed!\n");
    if (gui_button_text_symbol(panel,GUI_SYMBOL_TRIANGLE_LEFT,"previous",GUI_TEXT_RIGHT,GUI_BUTTON_DEFAULT))
        fprintf(stdout, "left triangle button pressed!\n");
    demo->toggle = gui_button_toggle(panel, "toggle", demo->toggle);

    /* Checkbox + Radio buttons */
    demo->checkbox = gui_check(panel, "checkbox", demo->checkbox);
    if (!demo->scaleable)
        gui_layout_row_static(panel, 30, 75, 2);
    else gui_layout_row_dynamic(panel, 30, 2);
    if (gui_option(panel, "option 0", demo->option == 0))
        demo->option = 0;
    if (gui_option(panel, "option 1", demo->option == 1))
        demo->option = 1;

    {
        /* custom row layout by array */
        const gui_float ratio[] = {0.8f, 0.2f};
        const gui_float pixel[] = {150.0f, 30.0f};
        enum gui_row_layout_format fmt = (demo->scaleable) ? GUI_DYNAMIC : GUI_STATIC;
        gui_layout_row(panel, fmt, 30, 2, (fmt == GUI_DYNAMIC) ? ratio: pixel);
        demo->slider = gui_slider(panel, 0, demo->slider, 10, 1.0f);
        gui_labelf(panel, GUI_TEXT_LEFT, "%.2f", demo->slider);
        demo->progressbar = gui_progress(panel, demo->progressbar, 100, gui_true);
        gui_labelf(panel, GUI_TEXT_LEFT, "%lu", demo->progressbar);
    }

    /* item selection  */
    if (!demo->scaleable) gui_layout_row_static(panel, 30, 150, 1);
    else gui_layout_row_dynamic(panel, 30, 1);
    demo->spinner = gui_spinner(panel, 0, demo->spinner, 250, 10, &demo->spinner_active);

    /* combo boxes  */
    if (!demo->scaleable) gui_layout_row_static(panel, 30, 150, 1);
    else gui_layout_row_dynamic(panel, 25, 1);
    combo_box(panel, &demo->combo, weapons, LEN(weapons));
    prog_combo_box(panel, demo->prog_values, LEN(demo->prog_values), &demo->progcom);
    color_combo_box(panel, &demo->colcom);
    check_combo_box(panel, demo->check_values, LEN(demo->check_values), &demo->checkcom);

    {
        /* immediate mode custom row layout */
        enum gui_row_layout_format fmt = (demo->scaleable) ? GUI_DYNAMIC : GUI_STATIC;
        gui_layout_row_begin(panel, fmt, 30, 2);
        {
            gui_layout_row_push(panel,(fmt == GUI_DYNAMIC) ? 0.7f : 100);
            gui_editbox(panel, &demo->edit);
            gui_layout_row_push(panel, (fmt == GUI_DYNAMIC) ? 0.3f : 80);
            if (gui_button_text(panel, "submit", GUI_BUTTON_DEFAULT)) {
                gui_edit_box_clear(&demo->edit);
                fprintf(stdout, "command executed!\n");
            }
        }
        gui_layout_row_end(panel);
    }
}

static void
table_panel(struct gui_context *panel)
{
    gui_size i = 0;
    const char *table[] = {"Move forward", "w", "Move back", "s", "Move left", "a",
        "Move right", "d", "Jump", "SPACE", "Duck", "CTRL"};
    gui_table_begin(panel, GUI_TABLE_HHEADER, 30, 2);
    gui_label_colored(panel, "MOVEMENT", GUI_TEXT_CENTERED, gui_rgba(178, 122, 1, 255));
    gui_label_colored(panel, "KEY/BUTTON", GUI_TEXT_CENTERED, gui_rgba(178, 122, 1, 255));
    for (i = 0; i < LEN(table); i += 2) {
        gui_table_row(panel);
        gui_label(panel, table[i], GUI_TEXT_LEFT);
        gui_label(panel, table[i+1], GUI_TEXT_CENTERED);
    }
    gui_table_end(panel);
}

/* -----------------------------------------------------------------
 *  STYLE
 * ----------------------------------------------------------------- */
static void
update_flags(struct gui_context *panel)
{
    gui_size n = 0;
    gui_flags res = 0;
    gui_flags i = 0x01;
    const char *options[]={"Hidden","Border","Header Border", "Moveable","Scaleable", "Minimized", "ROM"};
    gui_layout_row_dynamic(panel, 30, 2);
    do {
        if (gui_check(panel,options[n++],(panel->flags & i)?gui_true:gui_false))
            res |= i;
        i = i << 1;
    } while (i <= GUI_WINDOW_ROM);
    panel->flags = res;
}

static void
properties_tab(struct gui_context *panel, struct gui_style *config)
{
    int i = 0;
    const char *properties[] = {"item spacing:", "item padding:", "panel padding:",
        "scaler size:", "scrollbar:"};

    gui_layout_row_dynamic(panel, 30, 3);
    for (i = 0; i <= GUI_PROPERTY_SCROLLBAR_SIZE; ++i) {
        gui_int tx, ty;
        gui_label(panel, properties[i], GUI_TEXT_LEFT);
        tx = gui_spinner(panel,0,(gui_int)config->properties[i].x, 20, 1, NULL);
        ty = gui_spinner(panel,0,(gui_int)config->properties[i].y, 20, 1, NULL);
        config->properties[i].x = (float)tx;
        config->properties[i].y = (float)ty;
    }
}

static void
round_tab(struct gui_context *panel, struct gui_style *config)
{
    int i = 0;
    const char *rounding[] = {"panel:", "button:", "checkbox:", "progress:", "input: ",
        "graph:", "scrollbar:"};

    gui_layout_row_dynamic(panel, 30, 2);
    for (i = 0; i < GUI_ROUNDING_MAX; ++i) {
        gui_int t;
        gui_label(panel, rounding[i], GUI_TEXT_LEFT);
        t = gui_spinner(panel,0,(gui_int)config->rounding[i], 20, 1, NULL);
        config->rounding[i] = (float)t;
    }
}

static void
color_tab(struct gui_context *panel, struct state *control, struct gui_style *config)
{
    gui_size i = 0;
    gui_layout_row_dynamic(panel, 30, 2);
    for (i = 0; i < GUI_COLOR_COUNT; ++i) {
        struct gui_color c = config->colors[i];
        gui_label(panel, colors[i], GUI_TEXT_LEFT);
        if (gui_button_color(panel, c, GUI_BUTTON_DEFAULT)) {
            control->picker.active = gui_true;
            control->picker.color = config->colors[i];
            control->picker.index = i;
        }
    }
    if (control->picker.active) {
        color_picker(panel, &control->picker, colors[control->picker.index],
            &config->colors[control->picker.index]);
    }
}

/* -----------------------------------------------------------------
 *  COPY & PASTE
 * ----------------------------------------------------------------- */
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

/* -----------------------------------------------------------------
 *  INIT
 * ----------------------------------------------------------------- */
static void
init_demo(struct demo_gui *gui, struct gui_font *font)
{
    struct gui_style *config = &gui->config;
    struct state *win = &gui->state;
    struct gui_clipboard clip;
    gui->font = *font;
    gui->running = gui_true;

    gui_command_queue_init_fixed(&gui->queue, gui->memory, MAX_MEMORY);
    gui_style_default(config, GUI_DEFAULT_ALL, font);

    /* panel */
    gui_window_init(&gui->panel, 30, 30, 280, 530,
        GUI_WINDOW_BORDER|GUI_WINDOW_MOVEABLE|GUI_WINDOW_SCALEABLE,
        &gui->queue, config, gui->input);
    gui_window_init(&gui->sub, 400, 50, 220, 180,
        GUI_WINDOW_BORDER|GUI_WINDOW_MOVEABLE|GUI_WINDOW_SCALEABLE,
        &gui->queue, config, gui->input);

    /* widget state */
    tree_init(&win->test);
    clip.userdata.ptr = NULL,
    clip.copy = copy;
    clip.paste = paste;
    gui_edit_box_init_fixed(&win->edit, win->edit_buffer, MAX_BUFFER, &clip, NULL);

    win->prog_values[0] = 30;
    win->prog_values[1] = 80;
    win->prog_values[2] = 70;
    win->prog_values[3] = 50;

    win->scaleable = gui_true;
    win->slider = 2.0f;
    win->progressbar = 50;
    win->spinner = 100;
}

/* -----------------------------------------------------------------
 *  RUN
 * ----------------------------------------------------------------- */
static void
run_demo(struct demo_gui *gui)
{
    struct gui_context layout;
    struct state *state = &gui->state;
    struct gui_context tab;
    struct gui_style *config = &gui->config;

    /* first window */
    gui_begin(&layout, &gui->panel);
    {
        /* header */
        gui->running = !gui_header(&layout, "Demo",
            GUI_CLOSEABLE|GUI_MINIMIZABLE, GUI_CLOSEABLE, GUI_HEADER_RIGHT);

        /* menubar */
        gui_menubar_begin(&layout);
        {
            gui_layout_row_begin(&layout, GUI_STATIC, 18, 2);
            {
                gui_int sel;
                gui_layout_row_push(&layout, config->font.width(config->font.userdata, "__FILE__", 8));
                sel = gui_menu(&layout, "FILE", file_items, LEN(file_items), 25, 100,
                    &state->file_open);
                switch (sel) {
                case MENU_FILE_OPEN:
                    fprintf(stdout, "[Menu:File] open clicked!\n"); break;
                case MENU_FILE_CLOSE:
                    fprintf(stdout, "[Menu:File] close clicked!\n"); break;
                case MENU_FILE_QUIT:
                    fprintf(stdout, "[Menu:File] quit clicked!\n"); break;
                case GUI_NONE:
                default: break;
                }

                gui_layout_row_push(&layout, config->font.width(config->font.userdata, "__EDIT__", 8));
                sel = gui_menu(&layout, "EDIT", edit_items, LEN(edit_items), 25, 100,
                    &state->edit_open);
                switch (sel) {
                case MENU_EDIT_COPY:
                    fprintf(stdout, "[Menu:Edit] copy clicked!\n"); break;
                case MENU_EDIT_CUT:
                    fprintf(stdout, "[Menu:Edit] cut clicked!\n"); break;
                case MENU_EDIT_DELETE:
                    fprintf(stdout, "[Menu:Edit] delete clicked!\n"); break;
                case MENU_EDIT_PASTE:
                    fprintf(stdout, "[Menu:Edit] paste clicked!\n"); break;
                case GUI_NONE:
                default: break;
                }
            }
            gui_layout_row_end(&layout);
        }
        gui_menubar_end(&layout);

        /* panel style configuration  */
        if (gui_layout_push(&layout, GUI_LAYOUT_TAB, "Style", &state->config_tab))
        {
            if (gui_layout_push(&layout, GUI_LAYOUT_NODE, "Options", &state->flag_tab)) {
                update_flags(&layout);
                gui_layout_pop(&layout);
            }
            if (gui_layout_push(&layout, GUI_LAYOUT_NODE, "Properties", &state->style_tab)) {
                properties_tab(&layout, config);
                gui_layout_pop(&layout);
            }
            if (gui_layout_push(&layout, GUI_LAYOUT_NODE, "Rounding", &state->round_tab)) {
                round_tab(&layout, config);
                gui_layout_pop(&layout);
            }
            if (gui_layout_push(&layout, GUI_LAYOUT_NODE, "Color", &state->color_tab)) {
                color_tab(&layout, state, config);
                gui_layout_pop(&layout);
            }
            gui_layout_pop(&layout);
        }

        /* widgets examples */
        if (gui_layout_push(&layout, GUI_LAYOUT_TAB, "Widgets", &state->widget_tab)) {
            widget_panel(&layout, state);
            gui_layout_pop(&layout);
        }

        /* popup panel */
        if (state->popup) {
            gui_popup_begin(&layout, &tab, GUI_POPUP_STATIC, 0, gui_rect(20, 100, 220, 150), gui_vec2(0,0));
            {
                if (gui_header(&tab, "Popup", GUI_CLOSEABLE, GUI_CLOSEABLE, GUI_HEADER_LEFT)) {
                    gui_popup_close(&tab);
                    state->popup = gui_false;
                }
                gui_layout_row_dynamic(&tab, 30, 1);
                gui_label(&tab, "Are you sure you want to exit?", GUI_TEXT_LEFT);
                gui_layout_row_dynamic(&tab, 30, 4);
                gui_spacing(&tab, 1);
                if (gui_button_text(&tab, "Yes", GUI_BUTTON_DEFAULT)) {
                    gui_popup_close(&tab);
                    state->popup = gui_false;
                }
                if (gui_button_text(&tab, "No", GUI_BUTTON_DEFAULT)) {
                    gui_popup_close(&tab);
                    state->popup = gui_false;
                }
            }
            gui_popup_end(&layout, &tab);
        }

        {
            /* shelf + graphes  */
            static const char *shelfs[] = {"Histogram", "Lines"};
            gui_layout_row_dynamic(&layout, 180, 1);
            state->shelf_selection = gui_shelf_begin(&layout, &tab, shelfs,
                LEN(shelfs), state->shelf_selection, state->shelf);
            {
                enum {COLUMNS, LINES};
                static const gui_float values[]={8.0f,15.0f,20.0f,12.0f,30.0f,12.0f,35.0f,40.0f,20.0f};
                gui_layout_row_dynamic(&tab, 100, 1);
                switch (state->shelf_selection) {
                case COLUMNS:
                    gui_graph(&tab, GUI_GRAPH_COLUMN, values, LEN(values), 0); break;
                case LINES:
                    gui_graph(&tab, GUI_GRAPH_LINES, values, LEN(values), 0); break;
                default: break;
                }
            }
            state->shelf = gui_shelf_end(&layout, &tab);
        }

        /* table */
        gui_layout_row_dynamic(&layout, 180, 1);
        gui_group_begin(&layout, &tab, "Table", 0, state->table);
        table_panel(&tab);
        state->table = gui_group_end(&layout, &tab);

        {
            /* tree */
            struct gui_tree tree;
            gui_layout_row_dynamic(&layout, 250, 1);
            gui_tree_begin(&layout, &tree, "Tree", 20, state->tree);
            upload_tree(&state->test, &tree, &state->test.root);
            state->tree = gui_tree_end(&layout, &tree);
        }
    }
    gui_end(&layout, &gui->panel);

    /* second panel */
    gui_begin(&layout, &gui->sub);
    {
        enum {EASY, HARD};
        gui_header(&layout, "Show", GUI_CLOSEABLE, 0, GUI_HEADER_LEFT);
        gui_layout_row_static(&layout, 30, 80, 1);
        if (gui_button_text(&layout, "button", GUI_BUTTON_DEFAULT)) {
            /* event handling */
        }
        gui_layout_row_dynamic(&layout, 30, 2);
        if (gui_option(&layout, "easy", state->op == EASY)) state->op = EASY;
        if (gui_option(&layout, "hard", state->op == HARD)) state->op = HARD;
    }
    gui_end(&layout, &gui->sub);
}

