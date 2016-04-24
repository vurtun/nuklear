/* nuklear - v1.00 - public domain */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

#define NK_INCLUDE_FIXED_TYPES
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#include "nuklear_gdip.h"
#include "nuklear_gdip.c"

static LRESULT CALLBACK
WindowProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    if (nk_gdip_handle_event(wnd, msg, wparam, lparam))
        return 0;

    return DefWindowProcW(wnd, msg, wparam, lparam);
}

int main(void)
{
    GdipFont* font;
    struct nk_context *ctx;

    WNDCLASSW wc;
    RECT rect = { 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT };
    DWORD style = WS_OVERLAPPEDWINDOW;
    DWORD exstyle = WS_EX_APPWINDOW;
    HWND wnd;
    int running = 1;
    int needs_refresh = 1;

    /* Win32 */
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = GetModuleHandleW(0);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"NuklearWindowClass";
    RegisterClassW(&wc);

    AdjustWindowRectEx(&rect, style, FALSE, exstyle);

    wnd = CreateWindowExW(exstyle, wc.lpszClassName, L"Nuklear Demo",
        style | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT,
        rect.right - rect.left, rect.bottom - rect.top,
        NULL, NULL, wc.hInstance, NULL);

    /* GUI */
    ctx = nk_gdip_init(wnd, WINDOW_WIDTH, WINDOW_HEIGHT);
    font = nk_gdipfont_create("Arial", 12);
    nk_gdip_set_font(font);
    while (running)
    {
        MSG msg;

        /* Input */
        nk_input_begin(ctx);
        if (needs_refresh == 0)
        {
            if (GetMessageW(&msg, NULL, 0, 0) <= 0)
            {
                running = 0;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            needs_refresh = 1;
        }
        else
        {
            needs_refresh = 0;
        }
        while (PeekMessageW(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                running = 0;
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            needs_refresh = 1;
        }
        nk_input_end(ctx);

        /* GUI */
        {struct nk_panel layout;
        if (nk_begin(ctx, &layout, "Demo", nk_rect(50, 50, 200, 200),
            NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
            NK_WINDOW_CLOSABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
        {
            enum {EASY, HARD};
            static int op = EASY;
            static int property = 20;

            nk_layout_row_static(ctx, 30, 80, 1);
            if (nk_button_label(ctx, "button", NK_BUTTON_DEFAULT))
                fprintf(stdout, "button pressed\n");
            nk_layout_row_dynamic(ctx, 30, 2);
            if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
            if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;
            nk_layout_row_dynamic(ctx, 22, 1);
            nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);
        }
        nk_end(ctx);}
        if (nk_window_is_closed(ctx, "Demo")) break;

        /* Draw */
        nk_gdip_render(NK_ANTI_ALIASING_ON, nk_rgb(30,30,30));
    }

    nk_gdipfont_del(font);
    nk_gdip_shutdown();
    UnregisterClassW(wc.lpszClassName, wc.hInstance);
    return 0;
}
