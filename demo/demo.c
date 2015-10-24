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

/* -----------------------------------------------------------------
 *  COLOR PICKER POPUP
 * ----------------------------------------------------------------- */
struct color_picker {
    zr_state active;
    struct zr_color color;
    zr_state r, g, b, a;
    zr_size index;
};

static zr_bool
color_picker(struct zr_context *panel, struct color_picker* control,
    const char *name, struct zr_color *color)
{
    int i;
    zr_byte *iter;
    zr_bool ret = zr_true;
    struct zr_context popup;
    zr_popup_begin(panel, &popup, ZR_POPUP_STATIC,0, zr_rect(10, 100, 280, 280), zr_vec2(0,0));
    {
        if (zr_header(&popup, "Color", ZR_CLOSEABLE, ZR_CLOSEABLE, ZR_HEADER_LEFT)) {
            zr_popup_close(&popup);
            return zr_false;
        }
        zr_layout_row_dynamic(&popup, 30, 2);
        zr_label(&popup, name, ZR_TEXT_LEFT);
        zr_button_color(&popup, control->color, ZR_BUTTON_DEFAULT);

        iter = &control->color.r;
        zr_layout_row_dynamic(&popup, 30, 2);
        for (i = 0; i < 4; ++i, iter++) {
            zr_float t;
            *iter = (zr_byte)zr_spinner(&popup, 0, *iter, 255, 1, NULL);
            t = *iter;
            t = zr_slider(&popup, 0, t, 255, 10);
            *iter = (zr_byte)t;
        }

        zr_layout_row_dynamic(&popup, 30, 4);
        zr_spacing(&popup, 1);
        if (zr_button_text(&popup, "ok", ZR_BUTTON_DEFAULT)) {
            zr_popup_close(&popup);
            *color = control->color;
            ret = zr_false;
        }
        if (zr_button_text(&popup, "cancel", ZR_BUTTON_DEFAULT)) {
            zr_popup_close(&popup);
            ret = zr_false;
        }
    }
    zr_popup_end(panel, &popup, NULL);
    control->active = (zr_state)ret;
    return ret;
}

/* -----------------------------------------------------------------
 *  LABEL
 * ----------------------------------------------------------------- */
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

/* -----------------------------------------------------------------
 *  COMBOBOXES
 * ----------------------------------------------------------------- */
struct combobox {
    zr_size selected;
    zr_state active;
};

struct check_combo_box {
    zr_bool values[4];
    zr_state active;
};

struct prog_combo_box {
    zr_state active;
};

struct color_combo_box {
    zr_state active;
    struct zr_color color;
};

static void
combo_box(struct zr_context *panel, struct combobox *combo,
    const char**values, zr_size count)
{
    zr_size i = 0;
    struct zr_context layout;
    zr_combo_begin(panel, &layout, values[combo->selected], &combo->active);
    {
        zr_layout_row_dynamic(&layout, 25, 1);
        for (i = 0; i < count; ++i) {
            if (combo->selected == i) continue;
            if (zr_combo_item(&layout, ZR_TEXT_LEFT, values[i])) {
                combo->selected = i;
            }
        }
    }
    zr_combo_end(panel, &layout, &combo->active);
}

static void
prog_combo_box(struct zr_context *panel, zr_size *values, zr_size count,
    struct prog_combo_box *demo)
{
    zr_size i = 0;
    zr_int sum = 0;
    zr_char buffer[64];
    struct zr_context combo;
    memset(&combo, 0, sizeof(combo));
    for (i = 0; i < count; ++i)
        sum += (zr_int)values[i];

    sprintf(buffer, "%d", sum);
    zr_combo_begin(panel, &combo, buffer, &demo->active);
    {
        zr_layout_row_dynamic(&combo, 30, 1);
        for (i = 0; i < count; ++i)
            values[i] = zr_progress(&combo, values[i], 100, zr_true);
    }
    zr_combo_end(panel, &combo, NULL);
}

static void
color_combo_box(struct zr_context *panel, struct color_combo_box *demo)
{
    /* color slider progressbar */
    zr_char buffer[32];
    struct zr_context combo;
    memset(&combo, 0, sizeof(combo));
    sprintf(buffer, "#%02x%02x%02x%02x", demo->color.r, demo->color.g,
            demo->color.b, demo->color.a);
    zr_combo_begin(panel, &combo, buffer,  &demo->active);
    {
        int i;
        const char *color_names[] = {"R:", "G:", "B:", "A:"};
        zr_float ratios[] = {0.15f, 0.85f};
        zr_byte *iter = &demo->color.r;
        zr_layout_row(&combo, ZR_DYNAMIC, 30, 2, ratios);
        for (i = 0; i < 4; ++i, iter++) {
            zr_float t = *iter;
            zr_label(&combo, color_names[i], ZR_TEXT_LEFT);
            t = zr_slider(&combo, 0, t, 255, 5);
            *iter = (zr_byte)t;
        }
    }
    zr_combo_end(panel, &combo, NULL);
}

static void
check_combo_box(struct zr_context *panel, zr_bool *values, zr_size count,
    struct check_combo_box *demo)
{
    /* checkbox combobox  */
    zr_int sum = 0;
    zr_size i = 0;
    zr_char buffer[64];
    struct zr_context combo;
    memset(&combo, 0, sizeof(combo));
    for (i = 0; i < count; ++i)
        sum += (zr_int)values[i];

    sprintf(buffer, "%d", sum);
    zr_combo_begin(panel, &combo, buffer,  &demo->active);
    {
        zr_layout_row_dynamic(&combo, 30, 1);
        for (i = 0; i < count; ++i)
            values[i] = zr_check(&combo, weapons[i], values[i]);
    }
    zr_combo_end(panel, &combo, NULL);
}

/* -----------------------------------------------------------------
 *  GRAPH
 * ----------------------------------------------------------------- */
static int
graph(struct zr_context *layout, enum zr_graph_type type,
    const zr_float *values, zr_size count, zr_size offset)
{
    zr_size i;
    zr_int index = -1;
    zr_float min_value;
    zr_float max_value;
    struct zr_graph graph;

    ZR_ASSERT(layout);
    ZR_ASSERT(values);
    ZR_ASSERT(count);
    if (!layout || !layout->valid || !values || !count)
        return -1;

    /* find min and max graph value */
    max_value = values[0];
    min_value = values[0];
    for (i = offset; i < count; ++i) {
        if (values[i] > max_value)
            max_value = values[i];
        if (values[i] < min_value)
            min_value = values[i];
    }

    /* execute graph */
    zr_graph_begin(layout, &graph, type, count, min_value, max_value);
    for (i = offset; i < count; ++i) {
        if (zr_graph_push(layout, &graph, values[i]))
            index = (zr_int)i;
    }
    zr_graph_end(layout, &graph);
    return index;
}

/* =================================================================
 *
 *                          DEMO
 *
 * =================================================================
 */
struct state {
    zr_char edit_buffer[MAX_BUFFER];
    struct zr_edit_box edit;
    struct color_picker picker;
    struct check_combo_box checkcom;
    struct prog_combo_box progcom;
    struct color_combo_box colcom;
    struct combobox combo;
    struct test_tree test;

    /* widgets state */
    zr_bool list[4];
    zr_size prog_values[4];
    zr_bool check_values[WEAPON_MAX];
    zr_bool scaleable;
    zr_bool checkbox;
    zr_float slider;
    zr_size progressbar;
    zr_int spinner;
    zr_state spinner_active;
    zr_size item_current;
    zr_size shelf_selection;
    zr_bool toggle;
    zr_int option;
    zr_state popup;
    zr_size cur;
    zr_size op;
    zr_float value;

    /* subpanels */
    struct zr_vec2 shelf;
    struct zr_vec2 table;
    struct zr_vec2 tree;
    struct zr_vec2 menu;

    /* open/close state */
    zr_state file_open;
    zr_state edit_open;
    zr_state next_open;
    zr_state config_tab;
    zr_state widget_tab;
    zr_state combo_tab;
    zr_state style_tab;
    zr_state round_tab;
    zr_state color_tab;
    zr_state flag_tab;
};

struct demo_gui {
    zr_bool running;
    struct zr_input input;
    struct zr_command_queue queue;
    struct zr_style config;
    struct zr_user_font font;
    struct zr_window panel;
    struct zr_window sub;
    struct state state;
    zr_size w, h;
};


/* -----------------------------------------------------------------
 *  WIDGETS
 * ----------------------------------------------------------------- */
static void
widget_panel(struct zr_context *panel, struct state *demo)
{
    /* Labels */
    zr_layout_row_dynamic(panel, 30, 1);
    demo->scaleable = zr_check(panel, "Scaleable Layout", demo->scaleable);
    if (!demo->scaleable)
        zr_layout_row_static(panel, 30, 150, 1);
    zr_label(panel, "text left", ZR_TEXT_LEFT);
    zr_label(panel, "text center", ZR_TEXT_CENTERED);
    zr_label(panel, "text right", ZR_TEXT_RIGHT);

    {
        /* Tooltip */
        struct zr_rect bounds;
        const struct zr_input *in = zr_input(panel);
        zr_layout_peek(&bounds, panel);
        zr_label(panel, "Hover me for tooltip", ZR_TEXT_CENTERED);
        if (zr_input_is_mouse_hovering_rect(in, bounds))
            zr_tooltip(panel, "This is a tooltip");
    }

    /* Buttons */
    if (zr_button_text(panel, "button", ZR_BUTTON_DEFAULT))
        demo->popup = zr_true;
    if (zr_button_text_symbol(panel, ZR_SYMBOL_TRIANGLE_RIGHT, "next", ZR_TEXT_LEFT, ZR_BUTTON_DEFAULT))
        fprintf(stdout, "right triangle button pressed!\n");
    if (zr_button_text_symbol(panel,ZR_SYMBOL_TRIANGLE_LEFT,"previous",ZR_TEXT_RIGHT,ZR_BUTTON_DEFAULT))
        fprintf(stdout, "left triangle button pressed!\n");

    /* Checkbox + Radio buttons */
    demo->checkbox = zr_check(panel, "checkbox", demo->checkbox);
    if (!demo->scaleable)
        zr_layout_row_static(panel, 30, 75, 2);
    else zr_layout_row_dynamic(panel, 30, 2);
    if (zr_option(panel, "option 0", demo->option == 0))
        demo->option = 0;
    if (zr_option(panel, "option 1", demo->option == 1))
        demo->option = 1;

    {
        /* custom row layout by array */
        const zr_float ratio[] = {0.8f, 0.2f};
        const zr_float pixel[] = {150.0f, 30.0f};
        enum zr_layout_format fmt = (demo->scaleable) ? ZR_DYNAMIC : ZR_STATIC;
        zr_layout_row(panel, fmt, 30, 2, (fmt == ZR_DYNAMIC) ? ratio: pixel);
        demo->slider = zr_slider(panel, 0, demo->slider, 10, 1.0f);
        zr_labelf(panel, ZR_TEXT_LEFT, "%.2f", demo->slider);
        demo->progressbar = zr_progress(panel, demo->progressbar, 100, zr_true);
        zr_labelf(panel, ZR_TEXT_LEFT, "%lu", demo->progressbar);
    }

    /* item selection  */
    if (!demo->scaleable) zr_layout_row_static(panel, 30, 150, 1);
    else zr_layout_row_dynamic(panel, 30, 1);
    demo->spinner = zr_spinner(panel, 0, demo->spinner, 250, 10, &demo->spinner_active);

    /* combo boxes  */
    if (!demo->scaleable) zr_layout_row_static(panel, 30, 150, 1);
    else zr_layout_row_dynamic(panel, 30, 1);
    combo_box(panel, &demo->combo, weapons, LEN(weapons));
    prog_combo_box(panel, demo->prog_values, LEN(demo->prog_values), &demo->progcom);
    color_combo_box(panel, &demo->colcom);
    check_combo_box(panel, demo->check_values, LEN(demo->check_values), &demo->checkcom);

    {
        /* custom row layout by im */
        enum zr_layout_format fmt = (demo->scaleable) ? ZR_DYNAMIC : ZR_STATIC;
        zr_layout_row_begin(panel, fmt, 30, 2);
        {
            zr_layout_row_push(panel,(fmt == ZR_DYNAMIC) ? 0.7f : 100);
            zr_editbox(panel, &demo->edit);
            zr_layout_row_push(panel, (fmt == ZR_DYNAMIC) ? 0.3f : 80);
            if (zr_button_text(panel, "submit", ZR_BUTTON_DEFAULT)) {
                zr_edit_box_clear(&demo->edit);
                fprintf(stdout, "command executed!\n");
            }
        }
        zr_layout_row_end(panel);
    }
}

/* -----------------------------------------------------------------
 *  STYLE
 * ----------------------------------------------------------------- */
static void
update_flags(struct zr_context *panel)
{
    zr_size n = 0;
    zr_flags res = 0;
    zr_flags i = 0x01;
    const char *options[]={"Hidden","Border","Header Border", "Moveable","Scaleable", "Minimized", "ROM"};
    zr_layout_row_dynamic(panel, 30, 2);
    do {
        if (zr_check(panel,options[n++],(panel->flags & i)?zr_true:zr_false))
            res |= i;
        i = i << 1;
    } while (i <= ZR_WINDOW_ROM);
    panel->flags = res;
}

static void
properties_tab(struct zr_context *panel, struct zr_style *config)
{
    int i = 0;
    const char *properties[] = {"item spacing:", "item padding:", "panel padding:",
        "scaler size:", "scrollbar:"};

    zr_layout_row_dynamic(panel, 30, 3);
    for (i = 0; i <= ZR_PROPERTY_SCROLLBAR_SIZE; ++i) {
        zr_int tx, ty;
        zr_label(panel, properties[i], ZR_TEXT_LEFT);
        tx = zr_spinner(panel,0,(zr_int)config->properties[i].x, 20, 1, NULL);
        ty = zr_spinner(panel,0,(zr_int)config->properties[i].y, 20, 1, NULL);
        config->properties[i].x = (float)tx;
        config->properties[i].y = (float)ty;
    }
}

static void
round_tab(struct zr_context *panel, struct zr_style *config)
{
    int i = 0;
    const char *rounding[] = {"panel:", "button:", "checkbox:", "progress:", "input: ",
        "graph:", "scrollbar:"};

    zr_layout_row_dynamic(panel, 30, 2);
    for (i = 0; i < ZR_ROUNDING_MAX; ++i) {
        zr_int t;
        zr_label(panel, rounding[i], ZR_TEXT_LEFT);
        t = zr_spinner(panel,0,(zr_int)config->rounding[i], 20, 1, NULL);
        config->rounding[i] = (float)t;
    }
}

static void
color_tab(struct zr_context *panel, struct state *control, struct zr_style *config)
{
    zr_size i = 0;
    zr_layout_row_dynamic(panel, 30, 2);
    for (i = 0; i < ZR_COLOR_COUNT; ++i) {
        struct zr_color c = config->colors[i];
        zr_label(panel, colors[i], ZR_TEXT_LEFT);
        if (zr_button_color(panel, c, ZR_BUTTON_DEFAULT)) {
            control->picker.active = zr_true;
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
copy(zr_handle handle, const char *text, zr_size size)
{
    zr_char buffer[1024];
    UNUSED(handle);
    if (size >= 1023) return;
    memcpy(buffer, text, size);
    buffer[size] = '\0';
    clipboard_set(buffer);
}

static void
paste(zr_handle handle, struct zr_edit_box *box)
{
    zr_size len;
    const char *text;
    UNUSED(handle);
    if (!clipboard_is_filled()) return;
    text = clipboard_get();
    len = strlen(text);
    zr_edit_box_add(box, text, len);
}

/* -----------------------------------------------------------------
 *  INIT
 * ----------------------------------------------------------------- */
static void
init_demo(struct demo_gui *gui)
{
    struct zr_style *config = &gui->config;
    struct state *win = &gui->state;
    struct zr_clipboard clip;
    gui->running = zr_true;

    /* panel */
    zr_style_default(config, ZR_DEFAULT_ALL, &gui->font);
    zr_window_init(&gui->panel, zr_rect(30, 30, 280, 600),
        ZR_WINDOW_BORDER|ZR_WINDOW_MOVEABLE|ZR_WINDOW_SCALEABLE,
        &gui->queue, config, &gui->input);
    zr_window_init(&gui->sub, zr_rect(400, 50, 220, 180),
        ZR_WINDOW_BORDER|ZR_WINDOW_MOVEABLE|ZR_WINDOW_SCALEABLE,
        &gui->queue, config, &gui->input);

    /* widget state */
    tree_init(&win->test);
    clip.userdata.ptr = NULL,
    clip.copy = copy;
    clip.paste = paste;
    zr_edit_box_init_fixed(&win->edit, win->edit_buffer, MAX_BUFFER, &clip, NULL);

    win->prog_values[0] = 30;
    win->prog_values[1] = 80;
    win->prog_values[2] = 70;
    win->prog_values[3] = 50;

    win->scaleable = zr_true;
    win->slider = 2.0f;
    win->progressbar = 50;
    win->spinner = 100;
    win->value = 0.6f;
}

/* -----------------------------------------------------------------
 *  RUN
 * ----------------------------------------------------------------- */
static void
run_demo(struct demo_gui *gui)
{
    struct zr_context layout;
    struct state *state = &gui->state;
    struct zr_context tab;
    struct zr_style *config = &gui->config;

    /* first window */
    zr_begin(&layout, &gui->panel);
    {
        /* header */
        gui->running = !zr_header(&layout, "Demo",
            ZR_CLOSEABLE|ZR_MINIMIZABLE, ZR_CLOSEABLE, ZR_HEADER_RIGHT);

        /* menubar */
        zr_menubar_begin(&layout);
        {
            zr_size i = 0;
            zr_layout_row_begin(&layout, ZR_STATIC, 18, 2);
            zr_layout_row_push(&layout, 45);

            zr_menu_begin(&layout, &tab, "FILE", 100, &state->file_open);
            zr_layout_row_dynamic(&tab, 25, 1);
            for (i = 0; i < LEN(file_items); ++i) {
                if (zr_menu_item(&tab, ZR_TEXT_LEFT, file_items[i])) {
                    fprintf(stdout, "[Menu:File] %s clicked!\n", file_items[i]);
                    state->file_open = zr_menu_close(&tab);
                }
            }
            zr_menu_end(&layout, &tab);

            zr_layout_row_push(&layout, 45);
            zr_menu_begin(&layout, &tab, "EDIT", 100, &state->edit_open);
            zr_layout_row_dynamic(&tab, 25, 1);
            for (i = 0; i < LEN(edit_items); ++i) {
                if (zr_menu_item(&tab, ZR_TEXT_LEFT, edit_items[i])) {
                    fprintf(stdout, "[Menu:Edit] %s clicked!\n", edit_items[i]);
                    state->edit_open = zr_menu_close(&tab);
                }
            }
            zr_menu_end(&layout, &tab);
        }
        zr_menubar_end(&layout);

        /* panel style configuration  */
        if (zr_layout_push(&layout, ZR_LAYOUT_TAB, "Style", &state->config_tab))
        {
            if (zr_layout_push(&layout, ZR_LAYOUT_NODE, "Options", &state->flag_tab)) {
                update_flags(&layout);
                zr_layout_pop(&layout);
            }
            if (zr_layout_push(&layout, ZR_LAYOUT_NODE, "Properties", &state->style_tab)) {
                properties_tab(&layout, config);
                zr_layout_pop(&layout);
            }
            if (zr_layout_push(&layout, ZR_LAYOUT_NODE, "Rounding", &state->round_tab)) {
                round_tab(&layout, config);
                zr_layout_pop(&layout);
            }
            if (zr_layout_push(&layout, ZR_LAYOUT_NODE, "Color", &state->color_tab)) {
                color_tab(&layout, state, config);
                zr_layout_pop(&layout);
            }
            zr_layout_pop(&layout);
        }

        /* widgets examples */
        if (zr_layout_push(&layout, ZR_LAYOUT_TAB, "Widgets", &state->widget_tab)) {
            widget_panel(&layout, state);
            zr_layout_pop(&layout);
        }

        /* popup panel */
        if (state->popup)
        {
            zr_popup_begin(&layout, &tab, ZR_POPUP_STATIC, 0, zr_rect(20, 100, 220, 180), zr_vec2(0,0));
            {
                if (zr_header(&tab, "Popup", ZR_CLOSEABLE, ZR_CLOSEABLE, ZR_HEADER_LEFT)) {
                    zr_popup_close(&tab);
                    state->popup = zr_false;
                }
                zr_layout_row_dynamic(&tab, 30, 1);
                zr_label(&tab, "Are you sure you want to exit?", ZR_TEXT_LEFT);
                zr_layout_row_dynamic(&tab, 30, 4);
                zr_spacing(&tab, 1);
                if (zr_button_text(&tab, "Yes", ZR_BUTTON_DEFAULT)) {
                    zr_popup_close(&tab);
                    state->popup = zr_false;
                }
                if (zr_button_text(&tab, "No", ZR_BUTTON_DEFAULT)) {
                    zr_popup_close(&tab);
                    state->popup = zr_false;
                }
            }
            zr_popup_end(&layout, &tab, NULL);
        }

        {
            /* shelf + graphes  */
            static const char *shelfs[] = {"Histogram", "Lines"};
            zr_layout_row_dynamic(&layout, 190, 1);
            state->shelf_selection = zr_shelf_begin(&layout, &tab, shelfs,
                LEN(shelfs), state->shelf_selection, state->shelf);
            {
                enum {COLUMNS, LINES};
                static const zr_float values[]={8.0f,15.0f,20.0f,12.0f,30.0f,12.0f,35.0f,40.0f,20.0f};
                zr_layout_row_dynamic(&tab, 100, 1);
                switch (state->shelf_selection) {
                case COLUMNS:
                    graph(&tab, ZR_GRAPH_COLUMN, values, LEN(values), 0); break;
                case LINES:
                    graph(&tab, ZR_GRAPH_LINES, values, LEN(values), 0); break;
                default: break;
                }
            }
            zr_shelf_end(&layout, &tab, &state->shelf);
        }

        {
            /* tree */
            struct zr_tree tree;
            zr_layout_row_dynamic(&layout, 250, 1);
            zr_tree_begin(&layout, &tree, "Tree", 20, state->tree);
            upload_tree(&state->test, &tree, &state->test.root);
            zr_tree_end(&layout, &tree, &state->tree);
        }
    }
    zr_end(&layout, &gui->panel);

    /* second panel */
    zr_begin(&layout, &gui->sub);
    {
        enum {EASY, HARD};
        zr_header(&layout, "Show", ZR_CLOSEABLE, 0, ZR_HEADER_LEFT);
        zr_layout_row_static(&layout, 30, 80, 1);
        if (zr_button_text(&layout, "button", ZR_BUTTON_DEFAULT)) {
            /* event handling */
        }
        zr_layout_row_dynamic(&layout, 30, 2);
        if (zr_option(&layout, "easy", state->op == EASY)) state->op = EASY;
        if (zr_option(&layout, "hard", state->op == HARD)) state->op = HARD;
        zr_layout_row_begin(&layout, ZR_STATIC, 30, 2);
        {
            zr_layout_row_push(&layout, 50);
            zr_label(&layout, "Volume:", ZR_TEXT_LEFT);
            zr_layout_row_push(&layout, 110);
            state->value = zr_slider(&layout, 0, state->value, 1.0f, 0.1f);
        }
        zr_layout_row_end(&layout);
    }
    zr_end(&layout, &gui->sub);
}

