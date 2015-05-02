/*
    Copyright (c) 2015
    vurtun <polygone@gmx.net>
    MIT licence
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <windows.h>
#include <windowsx.h>

#include "gui.h"

/* macros */
#define MAX_BUFFER  64
#define MAX_MEMORY  (8 * 1024)
#define MAX_PANELS  4
#define WIN_WIDTH   800
#define WIN_HEIGHT  600
#define DTIME       33

#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#define MAX(a,b)    ((a) < (b) ? (b) : (a))
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#define LEN(a)      (sizeof(a)/sizeof(a)[0])
#define UNUSED(a)   ((void)(a))

/* Types */
typedef struct XFont {
    HFONT handle;
    int height;
    int ascent;
    int descent;
} XFont;

typedef struct XSurface {
    int save;
    HDC hdc;
    HBITMAP bitmap;
    HRGN region;
    unsigned int width;
    unsigned int height;
} XSurface;

typedef struct XWindow {
    HINSTANCE hInstance;
    WNDCLASS wc;
    HWND hWnd;
    HDC hdc;
    RECT rect;
    XSurface *backbuffer;
    XFont *font;
    unsigned int width;
    unsigned int height;
} XWindow;

struct demo {
    gui_char in_buf[MAX_BUFFER];
    gui_size in_len;
    gui_bool in_act;
    gui_bool check;
    gui_int option;
    gui_float slider;
    gui_size prog;
    gui_int spinner;
    gui_bool spin_act;
    gui_size item_cur;
    gui_size cur;
    gui_bool tab_min;
    gui_float group_off;
    gui_float shelf_off;
    gui_bool toggle;
};

static int running = 1;
static int quit = 0;
static XWindow xw;

static void
die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    exit(EXIT_FAILURE);
}

static long long
timestamp(LARGE_INTEGER freq)
{
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    return (1000LL * now.QuadPart) / freq.QuadPart;
}


static XFont*
font_new(HDC hdc, const char *name, int height)
{
    HFONT old;
    TEXTMETRIC metrics;
    XFont *font = malloc(sizeof(XFont));
    font->height = height;
    font->handle = CreateFont(height, 0, 0, 0, 0, FALSE, FALSE, FALSE,
        ANSI_CHARSET, OUT_CHARACTER_PRECIS, CLIP_CHARACTER_PRECIS,
        ANTIALIASED_QUALITY, DEFAULT_PITCH, name);

    old = SelectObject(hdc, font->handle);
    GetTextMetrics(hdc, &metrics);
    font->ascent = metrics.tmAscent;
    font->descent = metrics.tmDescent;
    SelectObject(hdc, old);
    return font;
}

gui_size
font_get_text_width(void *handle, const gui_char *text, gui_size len)
{
    SIZE size;
    HFONT old;
    XWindow *win = handle;
    XFont *font = win->font;
    XSurface *surf = win->backbuffer;
    if (!font ||!text || !len) return 0;

    old = SelectObject(surf->hdc, font->handle);
    GetTextExtentPoint32(surf->hdc, (LPCTSTR)text, len, &size);
    SelectObject(surf->hdc, old);
    return size.cx;
}

static void
font_del(XFont *font)
{
    if (font->handle)
	DeleteObject(font->handle);
    free(font);
}

static XSurface*
surface_new(HDC hdc, unsigned int width, unsigned int height)
{
    XSurface *surf = malloc(sizeof(XSurface));
    surf->hdc = CreateCompatibleDC(hdc);
    surf->bitmap = CreateCompatibleBitmap(hdc, width, height);
    surf->width = width;
    surf->height = height;
    surf->region = NULL;
    return surf;
}

static void
surface_resize(XSurface *surf, HDC hdc, unsigned int width, unsigned int height)
{
    DeleteObject(surf->bitmap);
    surf->bitmap = CreateCompatibleBitmap(hdc, width, height);
    surf->width = width;
    surf->height = height;
}

static void
surface_del(XSurface *surf)
{
    if (surf->region) DeleteObject(surf->region);
    DeleteDC(surf->hdc);
    DeleteObject(surf->bitmap);
    free(surf);
}

static void
surface_scissor(XSurface *surf, short x, short y, unsigned short w, unsigned short h)
{
    if (surf->region) DeleteObject(surf->region);
    surf->region = CreateRectRgn(x, y, x + w, y + h);
    SelectClipRgn(surf->hdc, surf->region);
}

static void
surface_draw_line(XSurface *surf, short x0, short y0, short x1, short y1,
    unsigned char r,  unsigned char g, unsigned char b)
{
    HPEN hPen;
    HPEN old;
    hPen = CreatePen(PS_SOLID, 1, RGB(r,g,b));
    old = SelectObject(surf->hdc, hPen);
    MoveToEx(surf->hdc, x0, y0, NULL);
    LineTo(surf->hdc, x1, y1);
    SelectObject(surf->hdc, old);
    DeleteObject(hPen);
}

static void
surface_draw_rect(XSurface *surf, short x, short y, unsigned short w, unsigned short h,
    unsigned char r,  unsigned char g, unsigned char b)
{
    HBRUSH hBrush;
    HBRUSH old;
    RECT rect;
    rect.left = (LONG)x;
    rect.top = (LONG)y;
    rect.right = (LONG)(x + w);
    rect.bottom = (LONG)(y + h);
    hBrush = CreateSolidBrush(RGB(r,g,b));
    old = SelectObject(surf->hdc, hBrush);
    FillRect(surf->hdc, &rect, hBrush);
    SelectObject(surf->hdc, old);
    DeleteObject(hBrush);
}

static void
surface_draw_circle(XSurface *surf, short x, short y, unsigned short w, unsigned short h,
    unsigned char r,  unsigned char g, unsigned char b)
{
    HBRUSH hBrush;
    HBRUSH old;
    hBrush = CreateSolidBrush(RGB(r,g,b));
    old = SelectObject(surf->hdc, hBrush);
    Ellipse(surf->hdc, x, y, (x + w), (y + h));
    SelectObject(surf->hdc, old);
    DeleteObject(hBrush);
}

static void
surface_draw_triangle(XSurface *surf, short x0, short y0, short x1, short y1,
    short x2, short y2, unsigned char r,  unsigned char g, unsigned char b)
{
    HBRUSH old;
    HBRUSH hBrush;
    POINT pnt[3];
    pnt[0].x = (LONG)x0;
    pnt[0].y = (LONG)y0;
    pnt[1].x = (LONG)x1;
    pnt[1].y = (LONG)y1;
    pnt[2].x = (LONG)x2;
    pnt[2].y = (LONG)y2;

    hBrush = CreateSolidBrush(RGB(r,g,b));
    old = SelectObject(surf->hdc, hBrush);
    Polygon(surf->hdc, pnt, 3);
    SelectObject(surf->hdc, old);
    DeleteObject(hBrush);
}

static void
surface_draw_text(XSurface *surf, XFont *font, short x, short y, unsigned short w, unsigned short h,
    const char *text, unsigned int len,
    unsigned char bg_r, unsigned char bg_g, unsigned char bg_b,
    unsigned char fg_r, unsigned char fg_g, unsigned char fg_b)
{
    RECT format;
    UINT bg = RGB(bg_r, bg_g, bg_b);
    UINT fg = RGB(fg_r, fg_g, fg_b);
    HFONT old = SelectObject(surf->hdc, font->handle);

    format.left = x;
    format.top = y;
    format.right = x + w;
    format.bottom = y + h;

    SetBkColor(surf->hdc, bg);
    SetTextColor(surf->hdc, fg);
    SetBkMode(surf->hdc, OPAQUE);
    DrawText(surf->hdc, text, len, &format, DT_LEFT);
    SelectObject(surf->hdc, old);
}

static void
surface_clear(XSurface *surf, unsigned char r,  unsigned char g, unsigned char b)
{surface_draw_rect(surf, 0, 0, (unsigned short)surf->width, (unsigned short)surf->height, r, g, b);}

static void
surface_begin(XSurface *surf)
{
    surf->save = SaveDC(surf->hdc);
    SelectObject(surf->hdc, surf->bitmap);
}

static void
surface_end(XSurface *surf, HDC hdc)
{
    BitBlt(hdc, 0, 0, surf->width, surf->height, surf->hdc, 0, 0, SRCCOPY);
    RestoreDC(hdc, surf->save);
}

static void
draw(XSurface *surf, struct gui_command_list *list)
{
    struct gui_command *cmd;
    if (!list->count) return;
    cmd = list->begin;
    while (cmd != list->end) {
        switch (cmd->type) {
        case GUI_COMMAND_NOP: break;
        case GUI_COMMAND_SCISSOR: {
            struct gui_command_scissor *s = (void*)cmd;
            surface_scissor(surf, s->x, s->y, s->w, s->h);
        } break;
        case GUI_COMMAND_LINE: {
            struct gui_command_line *l = (void*)cmd;
            surface_draw_line(surf, l->begin[0], l->begin[1], l->end[0],
                l->end[1], l->color.r, l->color.g, l->color.b);
        } break;
        case GUI_COMMAND_RECT: {
            struct gui_command_rect *r = (void*)cmd;
            surface_draw_rect(surf, r->x, r->y, r->w, r->h,
                r->color.r, r->color.g, r->color.b);
        } break;
        case GUI_COMMAND_CIRCLE: {
            struct gui_command_circle *c = (void*)cmd;
            surface_draw_circle(surf, c->x, c->y, c->w, c->h,
                c->color.r, c->color.g, c->color.b);
        } break;
        case GUI_COMMAND_TRIANGLE: {
            struct gui_command_triangle *t = (void*)cmd;
            surface_draw_triangle(surf, t->a[0], t->a[1], t->b[0], t->b[1],
                t->c[0], t->c[1], t->color.r, t->color.g, t->color.b);
        } break;
        case GUI_COMMAND_TEXT: {
            struct gui_command_text *t = (void*)cmd;
            XWindow *xw = t->font;
            surface_draw_text(surf, xw->font, t->x, t->y, t->w, t->h, (const char*)t->string,
                    t->length, t->bg.r, t->bg.g, t->bg.b, t->fg.r, t->fg.g, t->fg.b);
        } break;
        default: break;
        }
        cmd = cmd->next;
    }
}

static void
demo_panel(struct gui_panel_layout *panel, struct demo *demo)
{
    gui_int i = 0;
    enum {HISTO, PLOT};
    const char *shelfs[] = {"Histogram", "Lines"};
    const gui_float values[] = {8.0f, 15.0f, 20.0f, 12.0f, 30.0f};
    const char *items[] = {"Fist", "Pistol", "Shotgun", "Railgun", "BFG"};
    const char *options[] = {"easy", "normal", "hard", "hell", "doom", "godlike"};
    struct gui_panel_layout tab;

    /* Tabs */
    demo->tab_min = gui_panel_tab_begin(panel, &tab, "Difficulty", demo->tab_min);
    gui_panel_row(&tab, 30, 3);
    for (i = 0; i < (gui_int)LEN(options); i++) {
        if (gui_panel_option(&tab, options[i], demo->option == i))
            demo->option = i;
    }
    gui_panel_tab_end(panel, &tab);

    /* Shelf */
    gui_panel_row(panel, 200, 2);
    demo->cur = gui_panel_shelf_begin(panel,&tab,shelfs,LEN(shelfs),demo->cur,demo->shelf_off);
    gui_panel_row(&tab, 100, 1);
    if (demo->cur == HISTO) {
        gui_panel_graph(&tab, GUI_GRAPH_HISTO, values, LEN(values));
    } else {
        gui_panel_graph(&tab, GUI_GRAPH_LINES, values, LEN(values));
    }
    demo->shelf_off = gui_panel_shelf_end(panel, &tab);

    /* Group */
    gui_panel_group_begin(panel, &tab, "Options", demo->group_off);
    gui_panel_row(&tab, 30, 1);
    if (gui_panel_button_text(&tab, "button", GUI_BUTTON_DEFAULT))
        fprintf(stdout, "button pressed!\n");
    demo->toggle = gui_panel_button_toggle(&tab, "toggle", demo->toggle);
    demo->check = gui_panel_check(&tab, "advanced", demo->check);
    demo->slider = gui_panel_slider(&tab, 0, demo->slider, 10, 1.0f);
    demo->prog = gui_panel_progress(&tab, demo->prog, 100, gui_true);
    demo->item_cur = gui_panel_selector(&tab, items, LEN(items), demo->item_cur);
    demo->spinner = gui_panel_spinner(&tab, 0, demo->spinner, 250, 10, &demo->spin_act);
    demo->in_len = gui_panel_input(&tab,demo->in_buf,demo->in_len,
                        MAX_BUFFER,&demo->in_act,GUI_INPUT_DEFAULT);
    demo->group_off = gui_panel_group_end(panel, &tab);
}

static void
key(struct gui_input *in, MSG *msg, gui_bool down)
{
    if (msg->lParam == VK_CONTROL)
        gui_input_key(in, GUI_KEY_CTRL, down);
    else if (msg->lParam == VK_SHIFT)
        gui_input_key(in, GUI_KEY_SHIFT, down);
    else if (msg->lParam == VK_DELETE)
        gui_input_key(in, GUI_KEY_DEL, down);
    else if (msg->lParam == VK_RETURN)
        gui_input_key(in, GUI_KEY_ENTER, down);
    else if (msg->lParam == VK_SPACE)
        gui_input_key(in, GUI_KEY_SPACE, down);
    else if (msg->lParam == VK_BACK)
        gui_input_key(in, GUI_KEY_BACKSPACE, down);
}

static void
text(struct gui_input *in, MSG *msg)
{
    gui_glyph glyph;
    if (msg->wParam < 32 && msg->wParam >= 128) return;
    glyph[0] = (gui_char)msg->wParam;
    gui_input_char(in, glyph);
}

static void
motion(struct gui_input *in, MSG *msg)
{
    const gui_int x = GET_X_LPARAM(msg->lParam);
    const gui_int y = GET_Y_LPARAM(msg->lParam);
    gui_input_motion(in, x, y);
}

static void
btn(struct gui_input *in, MSG *msg, gui_bool down)
{
    const gui_int x = GET_X_LPARAM(msg->lParam);
    const gui_int y = GET_Y_LPARAM(msg->lParam);
    gui_input_button(in, x, y, down);
}

LRESULT CALLBACK
wnd_proc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_DESTROY:
        PostQuitMessage(WM_QUIT);
        running = 0;
        quit = 1;
        break;
    case WM_SIZE:
        if (xw.backbuffer) {
        xw.width = LOWORD(lParam);
        xw.height = HIWORD(lParam);
        surface_resize(xw.backbuffer, xw.hdc, xw.width, xw.height);
        } break;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

INT WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE prev, LPSTR lpCmdLine, int show)
{
    LARGE_INTEGER freq;
    long long start;
    long long dt;
    struct demo demo;

    /* GUI */
    struct gui_input in;
    struct gui_font font;
    struct gui_memory memory;
    struct gui_memory_status status;
    struct gui_config config;
    struct gui_canvas canvas;
    struct gui_command_buffer buffer;
    struct gui_command_list list;
    struct gui_panel_layout layout;
    struct gui_panel panel;

    /* Window */
    QueryPerformanceFrequency(&freq);
    xw.wc.style = CS_HREDRAW|CS_VREDRAW;
    xw.wc.lpfnWndProc = wnd_proc;
    xw.wc.hInstance = hInstance;
    xw.wc.lpszClassName = "GUI";
    RegisterClass(&xw.wc);
    xw.hWnd = CreateWindowEx(
        0, xw.wc.lpszClassName, "Demo",
        WS_OVERLAPPEDWINDOW|WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        CW_USEDEFAULT, CW_USEDEFAULT,
        0, 0, hInstance, 0);

    xw.hdc = GetDC(xw.hWnd);
    GetClientRect(xw.hWnd, &xw.rect);
    xw.backbuffer = surface_new(xw.hdc, xw.rect.right, xw.rect.bottom);
    xw.font = font_new(xw.hdc, "Times New Roman", 14);
    xw.width = xw.rect.right;
    xw.height = xw.rect.bottom;

    /* GUI */
    memset(&in, 0, sizeof in);
    memory.memory = calloc(MAX_MEMORY, 1);
    memory.size = MAX_MEMORY;
    gui_buffer_init_fixed(&buffer, &memory, GUI_BUFFER_CLIPPING);

    font.userdata = &xw;
    font.height = (gui_float)xw.font->height;
    font.width = font_get_text_width;
    gui_default_config(&config);
    gui_panel_init(&panel, 50, 50, 420, 300,
        GUI_PANEL_BORDER|GUI_PANEL_MOVEABLE|
        GUI_PANEL_CLOSEABLE|GUI_PANEL_SCALEABLE|
        GUI_PANEL_MINIMIZABLE, &config, &font);

    /* Demo */
    memset(&demo, 0, sizeof(demo));
    demo.tab_min = gui_true;
    demo.spinner = 100;
    demo.slider = 2.0f;
    demo.prog = 60;

    while (running && !quit) {
        /* Input */
        MSG msg;
        start = timestamp(freq);
        gui_input_begin(&in);
        while (PeekMessage(&msg, xw.hWnd, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_KEYDOWN) key(&in, &msg, gui_true);
            else if (msg.message == WM_KEYUP) key(&in, &msg, gui_false);
            else if (msg.message == WM_LBUTTONDOWN) btn(&in, &msg, gui_true);
            else if (msg.message == WM_LBUTTONUP) btn(&in, &msg, gui_false);
            else if (msg.message == WM_MOUSEMOVE) motion(&in, &msg);
            else if (msg.message == WM_CHAR) text(&in, &msg);
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        gui_input_end(&in);

        /* GUI */
        gui_buffer_begin(&canvas, &buffer, xw.width, xw.height);
        running = gui_panel_begin(&layout, &panel, "Demo", &canvas, &in);
        demo_panel(&layout, &demo);
        gui_panel_end(&layout, &panel);
        gui_buffer_end(&list, &buffer, &canvas, &status);

        /* Draw */
        surface_begin(xw.backbuffer);
        surface_clear(xw.backbuffer, 255, 255, 255);
        draw(xw.backbuffer, &list);
        surface_end(xw.backbuffer, xw.hdc);

        /* Timing */
        dt = timestamp(freq) - start;
        if (dt < DTIME) Sleep(DTIME - dt);
    }

    font_del(xw.font);
    surface_del(xw.backbuffer);
    ReleaseDC(xw.hWnd, xw.hdc);
    return 0;
}

