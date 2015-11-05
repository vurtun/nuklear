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
#include "../../zahnrad.h"

#define UNUSED(a)   ((void)(a))
static void clipboard_set(const char *text){UNUSED(text);}
static zr_bool clipboard_is_filled(void){return zr_false;}
static const char* clipboard_get(void) {return NULL;}

#include "../demo.c"

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

zr_size
font_get_text_width(zr_handle handle, const zr_char *text, zr_size len)
{
    SIZE size;
    HFONT old;
    XWindow *win = handle.ptr;
    XFont *font = win->font;
    XSurface *surf = win->backbuffer;
    if (!font ||!text || !len) return 0;

    old = SelectObject(surf->hdc, font->handle);
    GetTextExtentPoint32(surf->hdc, (LPCTSTR)text, (int)len, &size);
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
surface_draw_text(XSurface *surf, XFont *font, short x, short y, unsigned short w,
    unsigned short h, const char *text, unsigned int len,
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
{surface_draw_rect(surf,0,0,(unsigned short)surf->width,(unsigned short)surf->height, r, g, b);}

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
draw(XSurface *surf, struct zr_command_queue *queue)
{
    const struct zr_command *cmd;
    zr_foreach_command(cmd, queue) {
        switch (cmd->type) {
        case ZR_COMMAND_NOP: break;
        case ZR_COMMAND_SCISSOR: {
            const struct zr_command_scissor *s = zr_command(scissor, cmd);
            surface_scissor(surf, s->x, s->y, s->w, s->h);
        } break;
        case ZR_COMMAND_LINE: {
            const struct zr_command_line *l = zr_command(line, cmd);
            surface_draw_line(surf, l->begin.x, l->begin.y, l->end.x,
                l->end.y, l->color.r, l->color.g, l->color.b);
        } break;
        case ZR_COMMAND_RECT: {
            const struct zr_command_rect *r = zr_command(rect, cmd);
            surface_draw_rect(surf, r->x, r->y, r->w, r->h,
                r->color.r, r->color.g, r->color.b);
        } break;
        case ZR_COMMAND_CIRCLE: {
            const struct zr_command_circle *c = zr_command(circle, cmd);
            surface_draw_circle(surf, c->x, c->y, c->w, c->h,
                c->color.r, c->color.g, c->color.b);
        } break;
        case ZR_COMMAND_TRIANGLE: {
            const struct zr_command_triangle *t = zr_command(triangle, cmd);
            surface_draw_triangle(surf, t->a.x, t->a.y, t->b.x, t->b.y,
                t->c.x, t->c.y, t->color.r, t->color.g, t->color.b);
        } break;
        case ZR_COMMAND_TEXT: {
            const struct zr_command_text *t = zr_command(text, cmd);
            XWindow *win = t->font->userdata.ptr;
            surface_draw_text(surf, win->font, t->x, t->y, t->w, t->h, (const char*)t->string,
                    (unsigned int)t->length, t->background.r, t->background.g, t->background.b,
                    t->foreground.r, t->foreground.g, t->foreground.b);
        } break;
        case ZR_COMMAND_IMAGE:
        case ZR_COMMAND_CURVE:
        case ZR_COMMAND_ARC:
        default: break;
        }
    }
    zr_command_queue_clear(queue);
}

static void
input_key(struct zr_input *in, MSG *msg, zr_bool down)
{
    switch (msg->wParam) {
    case VK_SHIFT: zr_input_key(in, ZR_KEY_SHIFT, down); break;
    case VK_DELETE: zr_input_key(in, ZR_KEY_DEL, down); break;
    case VK_RETURN: zr_input_key(in, ZR_KEY_ENTER, down); break;
    case VK_BACK: zr_input_key(in, ZR_KEY_BACKSPACE, down); break;
    case VK_LEFT: zr_input_key(in, ZR_KEY_LEFT, down); break;
    case VK_RIGHT: zr_input_key(in, ZR_KEY_RIGHT, down); break;
    default: break;
    }
}

static void
input_text(struct zr_input *in, MSG *msg)
{
    char glyph;
    if (msg->wParam < 32 || msg->wParam >= 128) return;
    glyph = (zr_char)msg->wParam;
    zr_input_char(in, glyph);
}

static void
input_motion(struct zr_input *in, MSG *msg)
{
    const zr_int x = GET_X_LPARAM(msg->lParam);
    const zr_int y = GET_Y_LPARAM(msg->lParam);
    zr_input_motion(in, x, y);
}

static void
input_btn(struct zr_input *in, MSG *msg, zr_bool down)
{
    const zr_int x = GET_X_LPARAM(msg->lParam);
    const zr_int y = GET_Y_LPARAM(msg->lParam);
    zr_input_button(in, ZR_BUTTON_LEFT, x, y, down);
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
    /* GUI */
    struct demo gui;

    /* Window */
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
    memset(&gui, 0, sizeof gui);
    zr_command_queue_init_fixed(&gui.queue, calloc(MAX_MEMORY, 1), MAX_MEMORY);
    gui.font.userdata = zr_handle_ptr(&xw);
    gui.font.height = (zr_float)xw.font->height;
    gui.font.width =  font_get_text_width;
    init_demo(&gui);

    while (gui.running) {
        /* Input */
        MSG msg;
        zr_input_begin(&gui.input);
        while (PeekMessage(&msg, xw.hWnd, 0, 0, PM_REMOVE)) {
            if (msg.message == WM_KEYDOWN)
                input_key(&gui.input, &msg, zr_true);
            else if (msg.message == WM_KEYUP)
                input_key(&gui.input, &msg, zr_false);
            else if (msg.message == WM_LBUTTONDOWN)
                input_btn(&gui.input, &msg, zr_true);
            else if (msg.message == WM_LBUTTONUP)
                input_btn(&gui.input, &msg, zr_false);
            else if (msg.message == WM_MOUSEMOVE)
                input_motion(&gui.input, &msg);
            else if (msg.message == WM_CHAR)
                input_text(&gui.input, &msg);
            else if (msg.message == WM_MOUSEWHEEL)
                zr_input_scroll(&gui.input, GET_WHEEL_DELTA_WPARAM(msg.wParam)/120);
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        zr_input_end(&gui.input);

        /* GUI */
        run_demo(&gui);

        /* Draw */
        surface_begin(xw.backbuffer);
        surface_clear(xw.backbuffer, 100, 100, 100);
        draw(xw.backbuffer, &gui.queue);
        surface_end(xw.backbuffer, xw.hdc);
    }

    free(zr_buffer_memory(&gui.queue.buffer));
    font_del(xw.font);
    surface_del(xw.backbuffer);
    ReleaseDC(xw.hWnd, xw.hdc);
    return 0;
}

