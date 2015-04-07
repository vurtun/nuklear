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

static GLuint
ldbmp(gui_byte *data, uint32_t *width, uint32_t *height)
{
    /* texture */
    GLuint texture;
    gui_byte *header;
    gui_byte *target;
    gui_byte *writer;
    gui_byte *reader;
    uint32_t ioff;
    uint32_t j;
    int32_t i;

    header = data;
    memcpy(width, &header[0x12], sizeof(uint32_t));
    memcpy(height, &header[0x16], sizeof(uint32_t));
    memcpy(&ioff, &header[0x0A], sizeof(uint32_t));

    data = data + ioff;
    reader = data;
    target = calloc(*width * *height * 4, 1);
    for (i = (int32_t)*height-1; i >= 0; i--) {
        writer = target + (i * (int32_t)*width * 4);
        for (j = 0; j < *width; j++) {
            *writer++ = *(reader + (j * 4) + 1);
            *writer++ = *(reader + (j * 4) + 2);
            *writer++ = *(reader + (j * 4) + 3);
            *writer++ = *(reader + (j * 4) + 0);
        }
        reader += *width * 4;
    }

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)*width, (GLsizei)*height, 0,
                GL_RGBA, GL_UNSIGNED_BYTE, target);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    free(target);
    return texture;
}

static struct gui_font*
ldfont(const char *name, unsigned char height)
{
    size_t mem;
    size_t size;
    size_t i = 0;
    gui_byte *iter;
    gui_texture tex;
    short max_height = 0;
    struct gui_font *font;
    uint32_t img_width, img_height;
    struct gui_font_glyph *glyphes;

    uint16_t num;
    uint16_t indexes;
    uint16_t tex_width;
    uint16_t tex_height;

    gui_byte *buffer = (gui_byte*)ldfile(name, &size);
    memcpy(&num, buffer, sizeof(uint16_t));
    memcpy(&indexes, &buffer[0x02], sizeof(uint16_t));
    memcpy(&tex_width, &buffer[0x04], sizeof(uint16_t));
    memcpy(&tex_height, &buffer[0x06], sizeof(uint16_t));

    iter = &buffer[0x08];
    mem = sizeof(struct gui_font_glyph) * ((size_t)indexes + 1);
    glyphes = calloc(mem, 1);
    for(i = 0; i < num; ++i) {
        uint16_t id, x, y, w, h;
        float xoff, yoff, xadv;

        memcpy(&id, iter, sizeof(uint16_t));
        memcpy(&x, &iter[0x02], sizeof(uint16_t));
        memcpy(&y, &iter[0x04], sizeof(uint16_t));
        memcpy(&w, &iter[0x06], sizeof(uint16_t));
        memcpy(&h, &iter[0x08], sizeof(uint16_t));
        memcpy(&xoff, &iter[10], sizeof(float));
        memcpy(&yoff, &iter[14], sizeof(float));
        memcpy(&xadv, &iter[18], sizeof(float));

        glyphes[id].code = id;
        glyphes[id].width = (short)w;
        glyphes[id].height = (short)h;
        glyphes[id].xoff  = xoff;
        glyphes[id].yoff = yoff;
        glyphes[id].xadvance = xadv;
        glyphes[id].uv[0].u = (float)x/(float)tex_width;
        glyphes[id].uv[0].v = (float)y/(float)tex_height;
        glyphes[id].uv[1].u = (float)(x+w)/(float)tex_width;
        glyphes[id].uv[1].v = (float)(y+h)/(float)tex_height;
        if (glyphes[id].height > max_height) max_height = glyphes[id].height;
        iter += 22;
    }

    font = calloc(sizeof(struct gui_font), 1);
    font->height = height;
    font->scale = (float)height/(float)max_height;
    font->texture.gl = ldbmp(iter, &img_width, &img_height);
    font->tex_size.x = tex_width;
    font->tex_size.y = tex_height;
    font->fallback = &glyphes['?'];
    font->glyphes = glyphes;
    font->glyph_count = indexes + 1;
    free(buffer);
    return font;
}

static void
delfont(struct gui_font *font)
{
    if (!font) return;
    if (font->glyphes)
        free(font->glyphes);
    free(font);
}

static void
message_panel(struct gui_context *ctx, struct gui_panel *panel)
{
    gui_begin_panel(ctx, panel, "Error", GUI_PANEL_HEADER|
        GUI_PANEL_MOVEABLE|GUI_PANEL_BORDER);
    gui_panel_layout(panel, 30, 2);
    if (gui_panel_button_text(panel, "ok", GUI_BUTTON_SWITCH))
        fprintf(stdout, "ok pressed!\n");
    if (gui_panel_button_text(panel, "cancel", GUI_BUTTON_SWITCH))
        fprintf(stdout, "cancel pressed!\n");
    gui_end_panel(ctx, panel, NULL);
}

static gui_int
color_picker_panel(struct gui_context *ctx, struct gui_panel *panel, struct color_picker *picker)
{
    gui_size i;
    gui_int ret = -1;
    gui_byte *ptr = &picker->color.r;
    gui_begin_panel(ctx, panel, "Color Picker",
        GUI_PANEL_HEADER|GUI_PANEL_MOVEABLE|GUI_PANEL_BORDER);
    gui_panel_layout(panel, 30, 2);
    for (i = 0; i < 4; ++i) {
        gui_int ivalue;
        gui_float fvalue = (gui_float)*ptr;
        fvalue = gui_panel_slider(panel, 0, fvalue, 255.0f, 10.0f, GUI_HORIZONTAL);
        ivalue = (gui_int)fvalue;
        picker->active[i] = gui_panel_spinner(panel, 0, &ivalue, 255, 1, picker->active[i]);
        *ptr = (gui_byte)ivalue;
        ptr++;
    }

    gui_panel_layout(panel, 30, 4);
    gui_panel_seperator(panel, 1);
    if (gui_panel_button_text(panel, "ok", GUI_BUTTON_SWITCH)) ret = 1;
    if (gui_panel_button_text(panel, "cancel", GUI_BUTTON_SWITCH)) ret = 0;
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
        GUI_PANEL_HEADER|GUI_PANEL_CLOSEABLE|GUI_PANEL_MINIMIZABLE|
        GUI_PANEL_SCALEABLE|GUI_PANEL_MOVEABLE|GUI_PANEL_BORDER);

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
    if (gui_panel_button_text(&demo->group, "button", GUI_BUTTON_SWITCH))
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

