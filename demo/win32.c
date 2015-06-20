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
#include <assert.h>
#include <string.h>

/* macros */
#define DTIME       16
#define MIN(a,b)    ((a) < (b) ? (a) : (b))
#define MAX(a,b)    ((a) < (b) ? (b) : (a))
#define CLAMP(i,v,x) (MAX(MIN(v,x), i))
#define LEN(a)      (sizeof(a)/sizeof(a)[0])
#define UNUSED(a)   ((void)(a))

#define GUI_IMPLEMENTATION
#include "gui.h"
#include "demo.c"

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
    fputs("\n", stderr);
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
    if (!font) return NULL;
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
font_get_text_width(gui_handle handle, const gui_char *text, gui_size len)
{
    SIZE size;
    HFONT old;
    XWindow *win = handle.ptr;
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
    if (!surf) return NULL;
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
surface_draw_rounded_rect(XSurface *surf, short x, short y, unsigned short w, unsigned short h,
    unsigned short rounding,  unsigned char r,  unsigned char g, unsigned char b)
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
    RoundRect(surf->hdc, rect.left, rect.top, rect.right, rect.bottom,2*rounding, 2*rounding);
    SelectObject(surf->hdc, old);
    DeleteObject(hBrush);
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
    DrawText(surf->hdc, text, len, &format, DT_LEFT|DT_SINGLELINE|DT_VCENTER);
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
execute(XSurface *surf, struct gui_command_buffer *buffer)
{
    const struct gui_command *cmd;
    gui_foreach_command(cmd, buffer) {
        switch (cmd->type) {
        case GUI_COMMAND_NOP: break;
        case GUI_COMMAND_SCISSOR: {
            const struct gui_command_scissor *s = gui_command(scissor, cmd);
            surface_scissor(surf, s->x, s->y, s->w, s->h);
        } break;
        case GUI_COMMAND_LINE: {
            const struct gui_command_line *l = gui_command(line, cmd);
            surface_draw_line(surf, l->begin[0], l->begin[1], l->end[0],
                l->end[1], l->color[0], l->color[1], l->color[2]);
        } break;
        case GUI_COMMAND_RECT: {
            const struct gui_command_rect *r = gui_command(rect, cmd);
	    if (!r->r) {
		surface_draw_rect(surf, r->x, r->y, r->w, r->h,
		    r->color[0], r->color[1], r->color[2]);
	    } else {
		surface_draw_rounded_rect(surf, r->x, r->y, r->w, r->h, r->r,
		    r->color[0], r->color[1], r->color[2]);
	    }
        } break;
        case GUI_COMMAND_CIRCLE: {
            const struct gui_command_circle *c = gui_command(circle, cmd);
            surface_draw_circle(surf, c->x, c->y, c->w, c->h,
                c->color[0], c->color[1], c->color[2]);
        } break;
        case GUI_COMMAND_TRIANGLE: {
            const struct gui_command_triangle *t = gui_command(triangle, cmd);
            surface_draw_triangle(surf, t->a[0], t->a[1], t->b[0], t->b[1],
                t->c[0], t->c[1], t->color[0], t->color[1], t->color[2]);
        } break;
        case GUI_COMMAND_TEXT: {
            const struct gui_command_text *t = gui_command(text, cmd);
            XWindow *win = t->font.ptr;
            surface_draw_text(surf, win->font, t->x, t->y, t->w, t->h, (const char*)t->string,
                    t->length, t->bg[0], t->bg[1], t->bg[2], t->fg[0], t->fg[1], t->fg[2]);
        } break;
        case GUI_COMMAND_IMAGE:
        default: break;
        }
    }
}

static void
draw(XSurface *surf, struct gui_stack *stack)
{
    struct gui_panel *iter;
    gui_foreach_panel(iter, stack)
        execute(surf, iter->buffer);
}

static void
key(struct gui_input *in, MSG *msg, gui_bool down)
{
    //if (msg->lParam == VK_CONTROL)
    //    gui_input_key(in, GUI_KEY_CTRL, down);
    //else 
    if (msg->lParam == VK_SHIFT)
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
WinMain(HINSTANCE hInstance, HINSTANCE prev, LPSTR lpCmdLine, int shown)
{
    LARGE_INTEGER freq;
    long long start;
    long long dt;

    /* GUI */
    struct gui_input in;
    struct gui_font font;
    struct demo_gui gui;

    /* Window */
    QueryPerformanceFrequency(&freq);
    xw.wc.style = CS_HREDRAW|CS_VREDRAW;
    xw.wc.lpfnWndProc = wnd_proc;
    xw.wc.hInstance = hInstance;
    xw.wc.lpszClassName = "GUI";
    RegisterClass(&xw.wc);
    xw.hWnd = CreateWindowEx(
        0, xw.wc.lpszClassName, "Demo",
        WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_MAXIMIZEBOX|WS_VISIBLE,
        CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        0, 0, hInstance, 0);

    xw.hdc = GetDC(xw.hWnd);
    GetClientRect(xw.hWnd, &xw.rect);
    xw.backbuffer = surface_new(xw.hdc, xw.rect.right, xw.rect.bottom);
    xw.font = font_new(xw.hdc, "Times New Roman", 14);
    xw.width = xw.rect.right;
    xw.height = xw.rect.bottom;

    /* GUI */
    memset(&in, 0, sizeof in);
    memset(&gui, 0, sizeof gui);
    font.userdata.ptr = &xw;
    font.height = (gui_float)xw.font->height;
    font.width = font_get_text_width;
    gui.running = gui_true;
    gui.memory = malloc(MAX_MEMORY);
    init_demo(&gui, &font);

    while (gui.running && !quit) {
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
        run_demo(&gui, &in);

        /* Draw */
        surface_begin(xw.backbuffer);
        surface_clear(xw.backbuffer, 100, 100, 100);
        draw(xw.backbuffer, &gui.stack);
        surface_end(xw.backbuffer, xw.hdc);

        /* Timing */
        dt = timestamp(freq) - start;
        if (dt < DTIME) Sleep(DTIME - dt);
    }

    free(gui.memory);
    font_del(xw.font);
    surface_del(xw.backbuffer);
    ReleaseDC(xw.hWnd, xw.hdc);
    return 0;
}

