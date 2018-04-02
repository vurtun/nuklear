#include "nuklear.h"
#include "nuklear_internal.h"

/* ===============================================================
 *
 *                          FILTER
 *
 * ===============================================================*/
NK_API int
nk_filter_default(const struct nk_text_edit *box, nk_rune unicode)
{
    NK_UNUSED(unicode);
    NK_UNUSED(box);
    return nk_true;
}
NK_API int
nk_filter_ascii(const struct nk_text_edit *box, nk_rune unicode)
{
    NK_UNUSED(box);
    if (unicode > 128) return nk_false;
    else return nk_true;
}
NK_API int
nk_filter_float(const struct nk_text_edit *box, nk_rune unicode)
{
    NK_UNUSED(box);
    if ((unicode < '0' || unicode > '9') && unicode != '.' && unicode != '-')
        return nk_false;
    else return nk_true;
}
NK_API int
nk_filter_decimal(const struct nk_text_edit *box, nk_rune unicode)
{
    NK_UNUSED(box);
    if ((unicode < '0' || unicode > '9') && unicode != '-')
        return nk_false;
    else return nk_true;
}
NK_API int
nk_filter_hex(const struct nk_text_edit *box, nk_rune unicode)
{
    NK_UNUSED(box);
    if ((unicode < '0' || unicode > '9') &&
        (unicode < 'a' || unicode > 'f') &&
        (unicode < 'A' || unicode > 'F'))
        return nk_false;
    else return nk_true;
}
NK_API int
nk_filter_oct(const struct nk_text_edit *box, nk_rune unicode)
{
    NK_UNUSED(box);
    if (unicode < '0' || unicode > '7')
        return nk_false;
    else return nk_true;
}
NK_API int
nk_filter_binary(const struct nk_text_edit *box, nk_rune unicode)
{
    NK_UNUSED(box);
    if (unicode != '0' && unicode != '1')
        return nk_false;
    else return nk_true;
}

/* ===============================================================
 *
 *                          EDIT
 *
 * ===============================================================*/
NK_LIB void
nk_edit_draw_text(struct nk_command_buffer *out,
    const struct nk_style_edit *style, float pos_x, float pos_y,
    float x_offset, const char *text, int byte_len, float row_height,
    const struct nk_user_font *font, struct nk_color background,
    struct nk_color foreground, int is_selected)
{
    NK_ASSERT(out);
    NK_ASSERT(font);
    NK_ASSERT(style);
    if (!text || !byte_len || !out || !style) return;

    {int glyph_len = 0;
    nk_rune unicode = 0;
    int text_len = 0;
    float line_width = 0;
    float glyph_width;
    const char *line = text;
    float line_offset = 0;
    int line_count = 0;

    struct nk_text txt;
    txt.padding = nk_vec2(0,0);
    txt.background = background;
    txt.text = foreground;

    glyph_len = nk_utf_decode(text+text_len, &unicode, byte_len-text_len);
    if (!glyph_len) return;
    while ((text_len < byte_len) && glyph_len)
    {
        if (unicode == '\n') {
            /* new line separator so draw previous line */
            struct nk_rect label;
            label.y = pos_y + line_offset;
            label.h = row_height;
            label.w = line_width;
            label.x = pos_x;
            if (!line_count)
                label.x += x_offset;

            if (is_selected) /* selection needs to draw different background color */
                nk_fill_rect(out, label, 0, background);
            nk_widget_text(out, label, line, (int)((text + text_len) - line),
                &txt, NK_TEXT_CENTERED, font);

            text_len++;
            line_count++;
            line_width = 0;
            line = text + text_len;
            line_offset += row_height;
            glyph_len = nk_utf_decode(text + text_len, &unicode, (int)(byte_len-text_len));
            continue;
        }
        if (unicode == '\r') {
            text_len++;
            glyph_len = nk_utf_decode(text + text_len, &unicode, byte_len-text_len);
            continue;
        }
        glyph_width = font->width(font->userdata, font->height, text+text_len, glyph_len);
        line_width += (float)glyph_width;
        text_len += glyph_len;
        glyph_len = nk_utf_decode(text + text_len, &unicode, byte_len-text_len);
        continue;
    }
    if (line_width > 0) {
        /* draw last line */
        struct nk_rect label;
        label.y = pos_y + line_offset;
        label.h = row_height;
        label.w = line_width;
        label.x = pos_x;
        if (!line_count)
            label.x += x_offset;

        if (is_selected)
            nk_fill_rect(out, label, 0, background);
        nk_widget_text(out, label, line, (int)((text + text_len) - line),
            &txt, NK_TEXT_LEFT, font);
    }}
}
NK_LIB nk_flags
nk_do_edit(nk_flags *state, struct nk_command_buffer *out,
    struct nk_rect bounds, nk_flags flags, nk_plugin_filter filter,
    struct nk_text_edit *edit, const struct nk_style_edit *style,
    struct nk_input *in, const struct nk_user_font *font)
{
    struct nk_rect area;
    nk_flags ret = 0;
    float row_height;
    char prev_state = 0;
    char is_hovered = 0;
    char select_all = 0;
    char cursor_follow = 0;
    struct nk_rect old_clip;
    struct nk_rect clip;

    NK_ASSERT(state);
    NK_ASSERT(out);
    NK_ASSERT(style);
    if (!state || !out || !style)
        return ret;

    /* visible text area calculation */
    area.x = bounds.x + style->padding.x + style->border;
    area.y = bounds.y + style->padding.y + style->border;
    area.w = bounds.w - (2.0f * style->padding.x + 2 * style->border);
    area.h = bounds.h - (2.0f * style->padding.y + 2 * style->border);
    if (flags & NK_EDIT_MULTILINE)
        area.w = NK_MAX(0, area.w - style->scrollbar_size.x);
    row_height = (flags & NK_EDIT_MULTILINE)? font->height + style->row_padding: area.h;

    /* calculate clipping rectangle */
    old_clip = out->clip;
    nk_unify(&clip, &old_clip, area.x, area.y, area.x + area.w, area.y + area.h);

    /* update edit state */
    prev_state = (char)edit->active;
    is_hovered = (char)nk_input_is_mouse_hovering_rect(in, bounds);
    if (in && in->mouse.buttons[NK_BUTTON_LEFT].clicked && in->mouse.buttons[NK_BUTTON_LEFT].down) {
        edit->active = NK_INBOX(in->mouse.pos.x, in->mouse.pos.y,
                                bounds.x, bounds.y, bounds.w, bounds.h);
    }

    /* (de)activate text editor */
    if (!prev_state && edit->active) {
        const enum nk_text_edit_type type = (flags & NK_EDIT_MULTILINE) ?
            NK_TEXT_EDIT_MULTI_LINE: NK_TEXT_EDIT_SINGLE_LINE;
        nk_textedit_clear_state(edit, type, filter);
        if (flags & NK_EDIT_AUTO_SELECT)
            select_all = nk_true;
        if (flags & NK_EDIT_GOTO_END_ON_ACTIVATE) {
            edit->cursor = edit->string.len;
            in = 0;
        }
    } else if (!edit->active) edit->mode = NK_TEXT_EDIT_MODE_VIEW;
    if (flags & NK_EDIT_READ_ONLY)
        edit->mode = NK_TEXT_EDIT_MODE_VIEW;
    else if (flags & NK_EDIT_ALWAYS_INSERT_MODE)
        edit->mode = NK_TEXT_EDIT_MODE_INSERT;

    ret = (edit->active) ? NK_EDIT_ACTIVE: NK_EDIT_INACTIVE;
    if (prev_state != edit->active)
        ret |= (edit->active) ? NK_EDIT_ACTIVATED: NK_EDIT_DEACTIVATED;

    /* handle user input */
    if (edit->active && in)
    {
        int shift_mod = in->keyboard.keys[NK_KEY_SHIFT].down;
        const float mouse_x = (in->mouse.pos.x - area.x) + edit->scrollbar.x;
        const float mouse_y = (in->mouse.pos.y - area.y) + edit->scrollbar.y;

        /* mouse click handler */
        is_hovered = (char)nk_input_is_mouse_hovering_rect(in, area);
        if (select_all) {
            nk_textedit_select_all(edit);
        } else if (is_hovered && in->mouse.buttons[NK_BUTTON_LEFT].down &&
            in->mouse.buttons[NK_BUTTON_LEFT].clicked) {
            nk_textedit_click(edit, mouse_x, mouse_y, font, row_height);
        } else if (is_hovered && in->mouse.buttons[NK_BUTTON_LEFT].down &&
            (in->mouse.delta.x != 0.0f || in->mouse.delta.y != 0.0f)) {
            nk_textedit_drag(edit, mouse_x, mouse_y, font, row_height);
            cursor_follow = nk_true;
        } else if (is_hovered && in->mouse.buttons[NK_BUTTON_RIGHT].clicked &&
            in->mouse.buttons[NK_BUTTON_RIGHT].down) {
            nk_textedit_key(edit, NK_KEY_TEXT_WORD_LEFT, nk_false, font, row_height);
            nk_textedit_key(edit, NK_KEY_TEXT_WORD_RIGHT, nk_true, font, row_height);
            cursor_follow = nk_true;
        }

        {int i; /* keyboard input */
        int old_mode = edit->mode;
        for (i = 0; i < NK_KEY_MAX; ++i) {
            if (i == NK_KEY_ENTER || i == NK_KEY_TAB) continue; /* special case */
            if (nk_input_is_key_pressed(in, (enum nk_keys)i)) {
                nk_textedit_key(edit, (enum nk_keys)i, shift_mod, font, row_height);
                cursor_follow = nk_true;
            }
        }
        if (old_mode != edit->mode) {
            in->keyboard.text_len = 0;
        }}

        /* text input */
        edit->filter = filter;
        if (in->keyboard.text_len) {
            nk_textedit_text(edit, in->keyboard.text, in->keyboard.text_len);
            cursor_follow = nk_true;
            in->keyboard.text_len = 0;
        }

        /* enter key handler */
        if (nk_input_is_key_pressed(in, NK_KEY_ENTER)) {
            cursor_follow = nk_true;
            if (flags & NK_EDIT_CTRL_ENTER_NEWLINE && shift_mod)
                nk_textedit_text(edit, "\n", 1);
            else if (flags & NK_EDIT_SIG_ENTER)
                ret |= NK_EDIT_COMMITED;
            else nk_textedit_text(edit, "\n", 1);
        }

        /* cut & copy handler */
        {int copy= nk_input_is_key_pressed(in, NK_KEY_COPY);
        int cut = nk_input_is_key_pressed(in, NK_KEY_CUT);
        if ((copy || cut) && (flags & NK_EDIT_CLIPBOARD))
        {
            int glyph_len;
            nk_rune unicode;
            const char *text;
            int b = edit->select_start;
            int e = edit->select_end;

            int begin = NK_MIN(b, e);
            int end = NK_MAX(b, e);
            text = nk_str_at_const(&edit->string, begin, &unicode, &glyph_len);
            if (edit->clip.copy)
                edit->clip.copy(edit->clip.userdata, text, end - begin);
            if (cut && !(flags & NK_EDIT_READ_ONLY)){
                nk_textedit_cut(edit);
                cursor_follow = nk_true;
            }
        }}

        /* paste handler */
        {int paste = nk_input_is_key_pressed(in, NK_KEY_PASTE);
        if (paste && (flags & NK_EDIT_CLIPBOARD) && edit->clip.paste) {
            edit->clip.paste(edit->clip.userdata, edit);
            cursor_follow = nk_true;
        }}

        /* tab handler */
        {int tab = nk_input_is_key_pressed(in, NK_KEY_TAB);
        if (tab && (flags & NK_EDIT_ALLOW_TAB)) {
            nk_textedit_text(edit, "    ", 4);
            cursor_follow = nk_true;
        }}
    }

    /* set widget state */
    if (edit->active)
        *state = NK_WIDGET_STATE_ACTIVE;
    else nk_widget_state_reset(state);

    if (is_hovered)
        *state |= NK_WIDGET_STATE_HOVERED;

    /* DRAW EDIT */
    {const char *text = nk_str_get_const(&edit->string);
    int len = nk_str_len_char(&edit->string);

    {/* select background colors/images  */
    const struct nk_style_item *background;
    if (*state & NK_WIDGET_STATE_ACTIVED)
        background = &style->active;
    else if (*state & NK_WIDGET_STATE_HOVER)
        background = &style->hover;
    else background = &style->normal;

    /* draw background frame */
    if (background->type == NK_STYLE_ITEM_COLOR) {
        nk_stroke_rect(out, bounds, style->rounding, style->border, style->border_color);
        nk_fill_rect(out, bounds, style->rounding, background->data.color);
    } else nk_draw_image(out, bounds, &background->data.image, nk_white);}

    area.w = NK_MAX(0, area.w - style->cursor_size);
    if (edit->active)
    {
        int total_lines = 1;
        struct nk_vec2 text_size = nk_vec2(0,0);

        /* text pointer positions */
        const char *cursor_ptr = 0;
        const char *select_begin_ptr = 0;
        const char *select_end_ptr = 0;

        /* 2D pixel positions */
        struct nk_vec2 cursor_pos = nk_vec2(0,0);
        struct nk_vec2 selection_offset_start = nk_vec2(0,0);
        struct nk_vec2 selection_offset_end = nk_vec2(0,0);

        int selection_begin = NK_MIN(edit->select_start, edit->select_end);
        int selection_end = NK_MAX(edit->select_start, edit->select_end);

        /* calculate total line count + total space + cursor/selection position */
        float line_width = 0.0f;
        if (text && len)
        {
            /* utf8 encoding */
            float glyph_width;
            int glyph_len = 0;
            nk_rune unicode = 0;
            int text_len = 0;
            int glyphs = 0;
            int row_begin = 0;

            glyph_len = nk_utf_decode(text, &unicode, len);
            glyph_width = font->width(font->userdata, font->height, text, glyph_len);
            line_width = 0;

            /* iterate all lines */
            while ((text_len < len) && glyph_len)
            {
                /* set cursor 2D position and line */
                if (!cursor_ptr && glyphs == edit->cursor)
                {
                    int glyph_offset;
                    struct nk_vec2 out_offset;
                    struct nk_vec2 row_size;
                    const char *remaining;

                    /* calculate 2d position */
                    cursor_pos.y = (float)(total_lines-1) * row_height;
                    row_size = nk_text_calculate_text_bounds(font, text+row_begin,
                                text_len-row_begin, row_height, &remaining,
                                &out_offset, &glyph_offset, NK_STOP_ON_NEW_LINE);
                    cursor_pos.x = row_size.x;
                    cursor_ptr = text + text_len;
                }

                /* set start selection 2D position and line */
                if (!select_begin_ptr && edit->select_start != edit->select_end &&
                    glyphs == selection_begin)
                {
                    int glyph_offset;
                    struct nk_vec2 out_offset;
                    struct nk_vec2 row_size;
                    const char *remaining;

                    /* calculate 2d position */
                    selection_offset_start.y = (float)(NK_MAX(total_lines-1,0)) * row_height;
                    row_size = nk_text_calculate_text_bounds(font, text+row_begin,
                                text_len-row_begin, row_height, &remaining,
                                &out_offset, &glyph_offset, NK_STOP_ON_NEW_LINE);
                    selection_offset_start.x = row_size.x;
                    select_begin_ptr = text + text_len;
                }

                /* set end selection 2D position and line */
                if (!select_end_ptr && edit->select_start != edit->select_end &&
                    glyphs == selection_end)
                {
                    int glyph_offset;
                    struct nk_vec2 out_offset;
                    struct nk_vec2 row_size;
                    const char *remaining;

                    /* calculate 2d position */
                    selection_offset_end.y = (float)(total_lines-1) * row_height;
                    row_size = nk_text_calculate_text_bounds(font, text+row_begin,
                                text_len-row_begin, row_height, &remaining,
                                &out_offset, &glyph_offset, NK_STOP_ON_NEW_LINE);
                    selection_offset_end.x = row_size.x;
                    select_end_ptr = text + text_len;
                }
                if (unicode == '\n') {
                    text_size.x = NK_MAX(text_size.x, line_width);
                    total_lines++;
                    line_width = 0;
                    text_len++;
                    glyphs++;
                    row_begin = text_len;
                    glyph_len = nk_utf_decode(text + text_len, &unicode, len-text_len);
                    glyph_width = font->width(font->userdata, font->height, text+text_len, glyph_len);
                    continue;
                }

                glyphs++;
                text_len += glyph_len;
                line_width += (float)glyph_width;

                glyph_len = nk_utf_decode(text + text_len, &unicode, len-text_len);
                glyph_width = font->width(font->userdata, font->height,
                    text+text_len, glyph_len);
                continue;
            }
            text_size.y = (float)total_lines * row_height;

            /* handle case when cursor is at end of text buffer */
            if (!cursor_ptr && edit->cursor == edit->string.len) {
                cursor_pos.x = line_width;
                cursor_pos.y = text_size.y - row_height;
            }
        }
        {
            /* scrollbar */
            if (cursor_follow)
            {
                /* update scrollbar to follow cursor */
                if (!(flags & NK_EDIT_NO_HORIZONTAL_SCROLL)) {
                    /* horizontal scroll */
                    const float scroll_increment = area.w * 0.25f;
                    if (cursor_pos.x < edit->scrollbar.x)
                        edit->scrollbar.x = (float)(int)NK_MAX(0.0f, cursor_pos.x - scroll_increment);
                    if (cursor_pos.x >= edit->scrollbar.x + area.w)
                        edit->scrollbar.x = (float)(int)NK_MAX(0.0f, edit->scrollbar.x + scroll_increment);
                } else edit->scrollbar.x = 0;

                if (flags & NK_EDIT_MULTILINE) {
                    /* vertical scroll */
                    if (cursor_pos.y < edit->scrollbar.y)
                        edit->scrollbar.y = NK_MAX(0.0f, cursor_pos.y - row_height);
                    if (cursor_pos.y >= edit->scrollbar.y + area.h)
                        edit->scrollbar.y = edit->scrollbar.y + row_height;
                } else edit->scrollbar.y = 0;
            }

            /* scrollbar widget */
            if (flags & NK_EDIT_MULTILINE)
            {
                nk_flags ws;
                struct nk_rect scroll;
                float scroll_target;
                float scroll_offset;
                float scroll_step;
                float scroll_inc;

                scroll = area;
                scroll.x = (bounds.x + bounds.w - style->border) - style->scrollbar_size.x;
                scroll.w = style->scrollbar_size.x;

                scroll_offset = edit->scrollbar.y;
                scroll_step = scroll.h * 0.10f;
                scroll_inc = scroll.h * 0.01f;
                scroll_target = text_size.y;
                edit->scrollbar.y = nk_do_scrollbarv(&ws, out, scroll, 0,
                        scroll_offset, scroll_target, scroll_step, scroll_inc,
                        &style->scrollbar, in, font);
            }
        }

        /* draw text */
        {struct nk_color background_color;
        struct nk_color text_color;
        struct nk_color sel_background_color;
        struct nk_color sel_text_color;
        struct nk_color cursor_color;
        struct nk_color cursor_text_color;
        const struct nk_style_item *background;
        nk_push_scissor(out, clip);

        /* select correct colors to draw */
        if (*state & NK_WIDGET_STATE_ACTIVED) {
            background = &style->active;
            text_color = style->text_active;
            sel_text_color = style->selected_text_hover;
            sel_background_color = style->selected_hover;
            cursor_color = style->cursor_hover;
            cursor_text_color = style->cursor_text_hover;
        } else if (*state & NK_WIDGET_STATE_HOVER) {
            background = &style->hover;
            text_color = style->text_hover;
            sel_text_color = style->selected_text_hover;
            sel_background_color = style->selected_hover;
            cursor_text_color = style->cursor_text_hover;
            cursor_color = style->cursor_hover;
        } else {
            background = &style->normal;
            text_color = style->text_normal;
            sel_text_color = style->selected_text_normal;
            sel_background_color = style->selected_normal;
            cursor_color = style->cursor_normal;
            cursor_text_color = style->cursor_text_normal;
        }
        if (background->type == NK_STYLE_ITEM_IMAGE)
            background_color = nk_rgba(0,0,0,0);
        else background_color = background->data.color;


        if (edit->select_start == edit->select_end) {
            /* no selection so just draw the complete text */
            const char *begin = nk_str_get_const(&edit->string);
            int l = nk_str_len_char(&edit->string);
            nk_edit_draw_text(out, style, area.x - edit->scrollbar.x,
                area.y - edit->scrollbar.y, 0, begin, l, row_height, font,
                background_color, text_color, nk_false);
        } else {
            /* edit has selection so draw 1-3 text chunks */
            if (edit->select_start != edit->select_end && selection_begin > 0){
                /* draw unselected text before selection */
                const char *begin = nk_str_get_const(&edit->string);
                NK_ASSERT(select_begin_ptr);
                nk_edit_draw_text(out, style, area.x - edit->scrollbar.x,
                    area.y - edit->scrollbar.y, 0, begin, (int)(select_begin_ptr - begin),
                    row_height, font, background_color, text_color, nk_false);
            }
            if (edit->select_start != edit->select_end) {
                /* draw selected text */
                NK_ASSERT(select_begin_ptr);
                if (!select_end_ptr) {
                    const char *begin = nk_str_get_const(&edit->string);
                    select_end_ptr = begin + nk_str_len_char(&edit->string);
                }
                nk_edit_draw_text(out, style,
                    area.x - edit->scrollbar.x,
                    area.y + selection_offset_start.y - edit->scrollbar.y,
                    selection_offset_start.x,
                    select_begin_ptr, (int)(select_end_ptr - select_begin_ptr),
                    row_height, font, sel_background_color, sel_text_color, nk_true);
            }
            if ((edit->select_start != edit->select_end &&
                selection_end < edit->string.len))
            {
                /* draw unselected text after selected text */
                const char *begin = select_end_ptr;
                const char *end = nk_str_get_const(&edit->string) +
                                    nk_str_len_char(&edit->string);
                NK_ASSERT(select_end_ptr);
                nk_edit_draw_text(out, style,
                    area.x - edit->scrollbar.x,
                    area.y + selection_offset_end.y - edit->scrollbar.y,
                    selection_offset_end.x,
                    begin, (int)(end - begin), row_height, font,
                    background_color, text_color, nk_true);
            }
        }

        /* cursor */
        if (edit->select_start == edit->select_end)
        {
            if (edit->cursor >= nk_str_len(&edit->string) ||
                (cursor_ptr && *cursor_ptr == '\n')) {
                /* draw cursor at end of line */
                struct nk_rect cursor;
                cursor.w = style->cursor_size;
                cursor.h = font->height;
                cursor.x = area.x + cursor_pos.x - edit->scrollbar.x;
                cursor.y = area.y + cursor_pos.y + row_height/2.0f - cursor.h/2.0f;
                cursor.y -= edit->scrollbar.y;
                nk_fill_rect(out, cursor, 0, cursor_color);
            } else {
                /* draw cursor inside text */
                int glyph_len;
                struct nk_rect label;
                struct nk_text txt;

                nk_rune unicode;
                NK_ASSERT(cursor_ptr);
                glyph_len = nk_utf_decode(cursor_ptr, &unicode, 4);

                label.x = area.x + cursor_pos.x - edit->scrollbar.x;
                label.y = area.y + cursor_pos.y - edit->scrollbar.y;
                label.w = font->width(font->userdata, font->height, cursor_ptr, glyph_len);
                label.h = row_height;

                txt.padding = nk_vec2(0,0);
                txt.background = cursor_color;;
                txt.text = cursor_text_color;
                nk_fill_rect(out, label, 0, cursor_color);
                nk_widget_text(out, label, cursor_ptr, glyph_len, &txt, NK_TEXT_LEFT, font);
            }
        }}
    } else {
        /* not active so just draw text */
        int l = nk_str_len_char(&edit->string);
        const char *begin = nk_str_get_const(&edit->string);

        const struct nk_style_item *background;
        struct nk_color background_color;
        struct nk_color text_color;
        nk_push_scissor(out, clip);
        if (*state & NK_WIDGET_STATE_ACTIVED) {
            background = &style->active;
            text_color = style->text_active;
        } else if (*state & NK_WIDGET_STATE_HOVER) {
            background = &style->hover;
            text_color = style->text_hover;
        } else {
            background = &style->normal;
            text_color = style->text_normal;
        }
        if (background->type == NK_STYLE_ITEM_IMAGE)
            background_color = nk_rgba(0,0,0,0);
        else background_color = background->data.color;
        nk_edit_draw_text(out, style, area.x - edit->scrollbar.x,
            area.y - edit->scrollbar.y, 0, begin, l, row_height, font,
            background_color, text_color, nk_false);
    }
    nk_push_scissor(out, old_clip);}
    return ret;
}
NK_API void
nk_edit_focus(struct nk_context *ctx, nk_flags flags)
{
    nk_hash hash;
    struct nk_window *win;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return;

    win = ctx->current;
    hash = win->edit.seq;
    win->edit.active = nk_true;
    win->edit.name = hash;
    if (flags & NK_EDIT_ALWAYS_INSERT_MODE)
        win->edit.mode = NK_TEXT_EDIT_MODE_INSERT;
}
NK_API void
nk_edit_unfocus(struct nk_context *ctx)
{
    struct nk_window *win;
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return;

    win = ctx->current;
    win->edit.active = nk_false;
    win->edit.name = 0;
}
NK_API nk_flags
nk_edit_string(struct nk_context *ctx, nk_flags flags,
    char *memory, int *len, int max, nk_plugin_filter filter)
{
    nk_hash hash;
    nk_flags state;
    struct nk_text_edit *edit;
    struct nk_window *win;

    NK_ASSERT(ctx);
    NK_ASSERT(memory);
    NK_ASSERT(len);
    if (!ctx || !memory || !len)
        return 0;

    filter = (!filter) ? nk_filter_default: filter;
    win = ctx->current;
    hash = win->edit.seq;
    edit = &ctx->text_edit;
    nk_textedit_clear_state(&ctx->text_edit, (flags & NK_EDIT_MULTILINE)?
        NK_TEXT_EDIT_MULTI_LINE: NK_TEXT_EDIT_SINGLE_LINE, filter);

    if (win->edit.active && hash == win->edit.name) {
        if (flags & NK_EDIT_NO_CURSOR)
            edit->cursor = nk_utf_len(memory, *len);
        else edit->cursor = win->edit.cursor;
        if (!(flags & NK_EDIT_SELECTABLE)) {
            edit->select_start = win->edit.cursor;
            edit->select_end = win->edit.cursor;
        } else {
            edit->select_start = win->edit.sel_start;
            edit->select_end = win->edit.sel_end;
        }
        edit->mode = win->edit.mode;
        edit->scrollbar.x = (float)win->edit.scrollbar.x;
        edit->scrollbar.y = (float)win->edit.scrollbar.y;
        edit->active = nk_true;
    } else edit->active = nk_false;

    max = NK_MAX(1, max);
    *len = NK_MIN(*len, max-1);
    nk_str_init_fixed(&edit->string, memory, (nk_size)max);
    edit->string.buffer.allocated = (nk_size)*len;
    edit->string.len = nk_utf_len(memory, *len);
    state = nk_edit_buffer(ctx, flags, edit, filter);
    *len = (int)edit->string.buffer.allocated;

    if (edit->active) {
        win->edit.cursor = edit->cursor;
        win->edit.sel_start = edit->select_start;
        win->edit.sel_end = edit->select_end;
        win->edit.mode = edit->mode;
        win->edit.scrollbar.x = (nk_uint)edit->scrollbar.x;
        win->edit.scrollbar.y = (nk_uint)edit->scrollbar.y;
    } return state;
}
NK_API nk_flags
nk_edit_buffer(struct nk_context *ctx, nk_flags flags,
    struct nk_text_edit *edit, nk_plugin_filter filter)
{
    struct nk_window *win;
    struct nk_style *style;
    struct nk_input *in;

    enum nk_widget_layout_states state;
    struct nk_rect bounds;

    nk_flags ret_flags = 0;
    unsigned char prev_state;
    nk_hash hash;

    /* make sure correct values */
    NK_ASSERT(ctx);
    NK_ASSERT(edit);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    win = ctx->current;
    style = &ctx->style;
    state = nk_widget(&bounds, ctx);
    if (!state) return state;
    in = (win->layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;

    /* check if edit is currently hot item */
    hash = win->edit.seq++;
    if (win->edit.active && hash == win->edit.name) {
        if (flags & NK_EDIT_NO_CURSOR)
            edit->cursor = edit->string.len;
        if (!(flags & NK_EDIT_SELECTABLE)) {
            edit->select_start = edit->cursor;
            edit->select_end = edit->cursor;
        }
        if (flags & NK_EDIT_CLIPBOARD)
            edit->clip = ctx->clip;
        edit->active = (unsigned char)win->edit.active;
    } else edit->active = nk_false;
    edit->mode = win->edit.mode;

    filter = (!filter) ? nk_filter_default: filter;
    prev_state = (unsigned char)edit->active;
    in = (flags & NK_EDIT_READ_ONLY) ? 0: in;
    ret_flags = nk_do_edit(&ctx->last_widget_state, &win->buffer, bounds, flags,
                    filter, edit, &style->edit, in, style->font);

    if (ctx->last_widget_state & NK_WIDGET_STATE_HOVER)
        ctx->style.cursor_active = ctx->style.cursors[NK_CURSOR_TEXT];
    if (edit->active && prev_state != edit->active) {
        /* current edit is now hot */
        win->edit.active = nk_true;
        win->edit.name = hash;
    } else if (prev_state && !edit->active) {
        /* current edit is now cold */
        win->edit.active = nk_false;
    } return ret_flags;
}
NK_API nk_flags
nk_edit_string_zero_terminated(struct nk_context *ctx, nk_flags flags,
    char *buffer, int max, nk_plugin_filter filter)
{
    nk_flags result;
    int len = nk_strlen(buffer);
    result = nk_edit_string(ctx, flags, buffer, &len, max, filter);
    buffer[NK_MIN(NK_MAX(max-1,0), len)] = '\0';
    return result;
}

