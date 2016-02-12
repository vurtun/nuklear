struct node {
    int ID;
    char name[32];
    struct zr_rect bounds;
    float value;
    struct zr_color color;
    int input_count;
    int output_count;
    struct node *next;
    struct node *prev;
};

struct node_link {
    int input_id;
    int input_slot;
    int output_id;
    int output_slot;
    struct zr_vec2 in;
    struct zr_vec2 out;
};

struct node_linking {
    int active;
    struct node *node;
    int input_id;
    int input_slot;
};

struct node_editor {
    struct node node_buf[32];
    struct node_link links[64];
    struct node *begin;
    struct node *end;
    int node_count;
    int link_count;
    struct zr_rect bounds;
    struct node *selected;
    int show_grid;
    struct zr_vec2 scrolling;
    struct node_linking linking;
};

static void
node_editor_push(struct node_editor *editor, struct node *node)
{
    if (!editor->begin) {
        node->next = NULL;
        node->prev = NULL;
        editor->begin = node;
        editor->end = node;
    } else {
        node->prev = editor->end;
        if (editor->end)
            editor->end->next = node;
        node->next = NULL;
        editor->end = node;
    }
}

static void
node_editor_pop(struct node_editor *editor, struct node *node)
{
    if (node->next)
        node->next->prev = node->prev;
    if (node->prev)
        node->prev->next = node->next;
    if (editor->end == node)
        editor->end = node->prev;
    if (editor->begin == node)
        editor->begin = node->next;
    node->next = NULL;
    node->prev = NULL;
}

static struct node*
node_editor_find(struct node_editor *editor, int ID)
{
    struct node *iter = editor->begin;
    while (iter) {
        if (iter->ID == ID)
            return iter;
        iter = iter->next;
    }
    return NULL;
}

static void
node_editor_add(struct node_editor *editor, const char *name, struct zr_rect bounds,
    struct zr_color col, int in_count, int out_count)
{
    static int IDs = 0;
    struct node *node;
    assert((zr_size)editor->node_count < LEN(editor->node_buf));
    node = &editor->node_buf[editor->node_count++];
    node->ID = IDs++;
    node->value = 0;
    node->color = zr_rgb(255, 0, 0);
    node->input_count = in_count;
    node->output_count = out_count;
    node->color = col;
    strcpy(node->name, name);
    node->bounds = bounds;
    node_editor_push(editor, node);
}

static void
node_editor_link(struct node_editor *editor, int in_id, int in_slot,
    int out_id, int out_slot)
{
    struct node_link *link;
    assert((zr_size)editor->link_count < LEN(editor->links));
    link = &editor->links[editor->link_count++];
    link->input_id = in_id;
    link->input_slot = in_slot;
    link->output_id = out_id;
    link->output_slot = out_slot;
}

static void
node_editor_demo(struct zr_context *ctx, struct node_editor *nodedit)
{
    int n = 0;
    struct zr_rect total_space;
    const struct zr_input *in = &ctx->input;
    struct zr_command_buffer *canvas;
    struct node *updated = 0;
    struct zr_panel layout;

    if (zr_begin(ctx, &layout, "Node Editor", zr_rect(50, 50, 650, 650),
        ZR_WINDOW_BORDER|ZR_WINDOW_NO_SCROLLBAR|ZR_WINDOW_CLOSABLE|ZR_WINDOW_MOVABLE))
    {
        /* allocate complete window space */
        canvas = zr_window_get_canvas(ctx);
        total_space = zr_window_get_content_region(ctx);
        zr_layout_space_begin(ctx, ZR_STATIC, total_space.h, (zr_size)nodedit->node_count);
        {
            struct zr_panel node, menu;
            struct node *it = nodedit->begin;
            struct zr_rect size = zr_layout_space_bounds(ctx);

            if (nodedit->show_grid) {
                /* display grid */
                float x, y;
                const float grid_size = 32.0f;
                const struct zr_color grid_color = zr_rgb(50, 50, 50);
                for (x = (float)fmod(size.x - nodedit->scrolling.x, grid_size); x < size.w; x += grid_size)
                    zr_draw_line(canvas, x+size.x, size.y, x+size.x, size.y+size.h, grid_color);
                for (y = (float)fmod(size.y - nodedit->scrolling.y, grid_size); y < size.h; y += grid_size)
                    zr_draw_line(canvas, size.x, y+size.y, size.x+size.w, y+size.y, grid_color);
            }

            /* execute each node as a moveable group */
            while (it) {
                /* calculate scrolled node window position and size */
                zr_layout_space_push(ctx, zr_rect(it->bounds.x - nodedit->scrolling.x,
                    it->bounds.y - nodedit->scrolling.y, it->bounds.w, it->bounds.h));

                /* execute node window */
                if (zr_group_begin(ctx, &node, it->name, ZR_WINDOW_MOVABLE|ZR_WINDOW_NO_SCROLLBAR|ZR_WINDOW_BORDER|ZR_WINDOW_TITLE))
                {
                    /* always have last selected node on top */
                    if (zr_input_mouse_clicked(in, ZR_BUTTON_LEFT, node.bounds) &&
                        (!(it->prev && zr_input_mouse_clicked(in, ZR_BUTTON_LEFT,
                        zr_layout_space_rect_to_screen(ctx, node.bounds)))) &&
                        nodedit->end != it)
                    {
                        updated = it;
                    }

                    /* ================= NODE CONTENT =====================*/
                    zr_layout_row_dynamic(ctx, 25, 1);
                    zr_button_color(ctx, it->color, ZR_BUTTON_DEFAULT);
                    it->color.r = (zr_byte)zr_propertyi(ctx, "#R:", 0, it->color.r, 255, 1,1);
                    it->color.g = (zr_byte)zr_propertyi(ctx, "#G:", 0, it->color.g, 255, 1,1);
                    it->color.b = (zr_byte)zr_propertyi(ctx, "#B:", 0, it->color.b, 255, 1,1);
                    it->color.a = (zr_byte)zr_propertyi(ctx, "#A:", 0, it->color.a, 255, 1,1);
                    /* ====================================================*/
                    zr_group_end(ctx);
                }

                {
                    /* node connector and linking */
                    float space;
                    struct zr_rect bounds;
                    bounds = zr_layout_space_rect_to_local(ctx, node.bounds);
                    bounds.x += nodedit->scrolling.x;
                    bounds.y += nodedit->scrolling.y;
                    it->bounds = bounds;

                    /* output connector */
                    space = node.bounds.h / ((it->output_count) + 1);
                    for (n = 0; n < it->output_count; ++n) {
                        struct zr_rect circle;
                        circle.x = node.bounds.x + node.bounds.w-4;
                        circle.y = node.bounds.y + space * (n+1);
                        circle.w = 8; circle.h = 8;
                        zr_draw_circle(canvas, circle, zr_rgb(100, 100, 100));

                        /* start linking process */
                        if (zr_input_has_mouse_click_down_in_rect(in, ZR_BUTTON_LEFT, circle, zr_true)) {
                            nodedit->linking.active = zr_true;
                            nodedit->linking.node = it;
                            nodedit->linking.input_id = it->ID;
                            nodedit->linking.input_slot = n;
                        }

                        /* draw curve from linked node slot to mouse position */
                        if (nodedit->linking.active && nodedit->linking.node == it &&
                            nodedit->linking.input_slot == n) {
                            struct zr_vec2 l0 = zr_vec2(circle.x + 3, circle.y + 3);
                            struct zr_vec2 l1 = in->mouse.pos;
                            zr_draw_curve(canvas, l0.x, l0.y, l0.x + 50.0f, l0.y,
                                l1.x - 50.0f, l1.y, l1.x, l1.y, zr_rgb(100, 100, 100));
                        }
                    }

                    /* input connector */
                    space = node.bounds.h / ((it->input_count) + 1);
                    for (n = 0; n < it->input_count; ++n) {
                        struct zr_rect circle;
                        circle.x = node.bounds.x-4;
                        circle.y = node.bounds.y + space * (n+1);
                        circle.w = 8; circle.h = 8;
                        zr_draw_circle(canvas, circle, zr_rgb(100, 100, 100));
                        if (zr_input_is_mouse_released(in, ZR_BUTTON_LEFT) &&
                            zr_input_is_mouse_hovering_rect(in, circle) &&
                            nodedit->linking.active && nodedit->linking.node != it) {
                            nodedit->linking.active = zr_false;
                            node_editor_link(nodedit, nodedit->linking.input_id,
                                nodedit->linking.input_slot, it->ID, n);
                        }
                    }
                }
                it = it->next;
            }

            /* reset linking connection */
            if (nodedit->linking.active && zr_input_is_mouse_released(in, ZR_BUTTON_LEFT)) {
                nodedit->linking.active = zr_false;
                nodedit->linking.node = NULL;
                fprintf(stdout, "linking failed\n");
            }

            /* draw each link */
            for (n = 0; n < nodedit->link_count; ++n) {
                struct node_link *link = &nodedit->links[n];
                struct node *ni = node_editor_find(nodedit, link->input_id);
                struct node *no = node_editor_find(nodedit, link->output_id);
                float spacei = node.bounds.h / ((ni->output_count) + 1);
                float spaceo = node.bounds.h / ((no->input_count) + 1);
                struct zr_vec2 l0 = zr_layout_space_to_screen(ctx,
                    zr_vec2(ni->bounds.x + ni->bounds.w, 3+ni->bounds.y + spacei * (link->input_slot+1)));
                struct zr_vec2 l1 = zr_layout_space_to_screen(ctx,
                    zr_vec2(no->bounds.x, 3 + no->bounds.y + spaceo * (link->output_slot+1)));

                l0.x -= nodedit->scrolling.x;
                l0.y -= nodedit->scrolling.y;
                l1.x -= nodedit->scrolling.x;
                l1.y -= nodedit->scrolling.y;
                zr_draw_curve(canvas, l0.x, l0.y, l0.x + 50.0f, l0.y,
                    l1.x - 50.0f, l1.y, l1.x, l1.y, zr_rgb(100, 100, 100));
            }

            if (updated) {
                /* reshuffle nodes to have last recently selected node on top */
                node_editor_pop(nodedit, updated);
                node_editor_push(nodedit, updated);
            }

            /* node selection */
            if (zr_input_mouse_clicked(in, ZR_BUTTON_LEFT, zr_layout_space_bounds(ctx))) {
                it = nodedit->begin;
                nodedit->selected = NULL;
                nodedit->bounds = zr_rect(in->mouse.pos.x, in->mouse.pos.y, 100, 200);
                while (it) {
                    struct zr_rect b = zr_layout_space_rect_to_screen(ctx, it->bounds);
                    b.x -= nodedit->scrolling.x;
                    b.y -= nodedit->scrolling.y;
                    if (zr_input_is_mouse_hovering_rect(in, b))
                        nodedit->selected = it;
                    it = it->next;
                }
            }

            /* contextual menu */
            if (zr_contextual_begin(ctx, &menu, 0, zr_vec2(100, 220), zr_window_get_bounds(ctx))) {
                const char *grid_option[] = {"Show Grid", "Hide Grid"};
                zr_layout_row_dynamic(ctx, 25, 1);
                if (zr_contextual_item(ctx, "New", ZR_TEXT_CENTERED))
                    node_editor_add(nodedit, "New", zr_rect(400, 260, 180, 220),
                            zr_rgb(255, 255, 255), 1, 2);
                if (zr_contextual_item(ctx, grid_option[nodedit->show_grid],ZR_TEXT_CENTERED))
                    nodedit->show_grid = !nodedit->show_grid;
                zr_contextual_end(ctx);
            }
        }
        zr_layout_space_end(ctx);

        /* window content scrolling */
        if (zr_input_is_mouse_hovering_rect(in, zr_window_get_bounds(ctx)) &&
            zr_input_is_mouse_down(in, ZR_BUTTON_MIDDLE)) {
            nodedit->scrolling.x += in->mouse.delta.x;
            nodedit->scrolling.y += in->mouse.delta.y;
        }
    }
    zr_end(ctx);
}

static void
node_editor_init(struct node_editor *editor)
{
    memset(editor, 0, sizeof(*editor));
    editor->begin = NULL;
    editor->end = NULL;
    node_editor_add(editor, "Source", zr_rect(40, 10, 180, 220), zr_rgb(255, 0, 0), 0, 1);
    node_editor_add(editor, "Source", zr_rect(40, 260, 180, 220), zr_rgb(0, 255, 0), 0, 1);
    node_editor_add(editor, "Combine", zr_rect(400, 100, 180, 220), zr_rgb(0,0,255), 2, 2);
    node_editor_link(editor, 0, 0, 2, 0);
    node_editor_link(editor, 1, 0, 2, 1);
    editor->show_grid = zr_true;
}

