/*
 * Nuklear - 1.32.0 - public domain
 * no warrenty implied; use at your own risk.
 * authored from 2015-2016 by Micha Mettke
 */
/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */
#ifndef NK_GDI_H_
#define NK_GDI_H_

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef struct GdiFont GdiFont;
NK_API struct nk_context* nk_gdi_init(GdiFont *font, HDC window_dc, unsigned int width, unsigned int height);
NK_API int nk_gdi_handle_event(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);
NK_API void nk_gdi_render(struct nk_color clear);
NK_API void nk_gdi_shutdown(void);

/* font */
NK_API GdiFont* nk_gdifont_create(const char *name, int size);
NK_API void nk_gdifont_del(GdiFont *font);
NK_API void nk_gdi_set_font(GdiFont *font);

#endif

/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#ifdef NK_GDI_IMPLEMENTATION

#include <stdlib.h>
#include <malloc.h>

struct GdiFont {
    struct nk_user_font nk;
    int height;
    HFONT handle;
    HDC dc;
};

static struct {
    HBITMAP bitmap;
    HDC window_dc;
    HDC memory_dc;
    unsigned int width;
    unsigned int height;
    struct nk_context ctx;
} gdi;

static void
nk_create_image(struct nk_image * image, const char * frame_buffer, const int width, const int height)
{
    if (image && frame_buffer && (width > 0) && (height > 0))
    {
        image->w = width;
        image->h = height;
        image->region[0] = 0;
        image->region[1] = 0;
        image->region[2] = width;
        image->region[3] = height;
        
        INT row = ((width * 3 + 3) & ~3);
        BITMAPINFO bi = { 0 };
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = width;
        bi.bmiHeader.biHeight = height;
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 24;
        bi.bmiHeader.biCompression = BI_RGB;
        bi.bmiHeader.biSizeImage = row * height;
        
        LPBYTE lpBuf, pb = NULL;
        HBITMAP hbm = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, (void**)&lpBuf, NULL, 0);
        
        pb = lpBuf + row * height;
        unsigned char * src = (unsigned char *)frame_buffer;
        for (int v = 0; v<height; v++)
        {
            pb -= row;
            for (int i = 0; i < row; i += 3)
            {
                pb[i + 0] = src[0];
                pb[i + 1] = src[1];
                pb[i + 2] = src[2];
                src += 3;
            }
        }        
        SetDIBits(NULL, hbm, 0, height, lpBuf, &bi, DIB_RGB_COLORS);
        image->handle.ptr = hbm;
    }
}

static void
nk_delete_image(struct nk_image * image)
{
    if (image && image->handle.id != 0)
    {
        HBITMAP hbm = (HBITMAP)image->handle.ptr;
        DeleteObject(hbm);
        memset(image, 0, sizeof(struct nk_image));
    }
}

static void
nk_gdi_draw_image(short x, short y, unsigned short w, unsigned short h,
    struct nk_image img, struct nk_color col)
{
    HBITMAP hbm = (HBITMAP)img.handle.ptr;
    HDC     hDCBits;
    BITMAP  bitmap;
    
    if (!gdi.memory_dc || !hbm)
        return;
    
    hDCBits = CreateCompatibleDC(gdi.memory_dc);
    GetObject(hbm, sizeof(BITMAP), (LPSTR)&bitmap);
    SelectObject(hDCBits, hbm);
    StretchBlt(gdi.memory_dc, x, y, w, h, hDCBits, 0, 0, bitmap.bmWidth, bitmap.bmHeight, SRCCOPY);
    DeleteDC(hDCBits);
}

static COLORREF
convert_color(struct nk_color c)
{
    return c.r | (c.g << 8) | (c.b << 16);
}

static void
nk_gdi_scissor(HDC dc, float x, float y, float w, float h)
{
    SelectClipRgn(dc, NULL);
    IntersectClipRect(dc, (int)x, (int)y, (int)(x + w + 1), (int)(y + h + 1));
}

static void
nk_gdi_stroke_line(HDC dc, short x0, short y0, short x1,
    short y1, unsigned int line_thickness, struct nk_color col)
{
    COLORREF color = convert_color(col);

    HPEN pen = NULL;
    if (line_thickness == 1) {
        SetDCPenColor(dc, color);
    } else {
        pen = CreatePen(PS_SOLID, line_thickness, color);
        SelectObject(dc, pen);
    }

    MoveToEx(dc, x0, y0, NULL);
    LineTo(dc, x1, y1);

    if (pen) {
        SelectObject(dc, GetStockObject(DC_PEN));
        DeleteObject(pen);
    }
}

static void
nk_gdi_stroke_rect(HDC dc, short x, short y, unsigned short w,
    unsigned short h, unsigned short r, unsigned short line_thickness, struct nk_color col)
{
    COLORREF color = convert_color(col);

    HPEN pen = NULL;
    if (line_thickness == 1) {
        SetDCPenColor(dc, color);
    } else {
        pen = CreatePen(PS_SOLID, line_thickness, color);
        SelectObject(dc, pen);
    }

    HGDIOBJ br = SelectObject(dc, GetStockObject(NULL_BRUSH));
    if (r == 0) {
        Rectangle(dc, x, y, x + w, y + h);
    } else {
        RoundRect(dc, x, y, x + w, y + h, r, r);
    }
    SelectObject(dc, br);

    if (pen) { 
        SelectObject(dc, GetStockObject(DC_PEN));
        DeleteObject(pen);
    }
}

static void
nk_gdi_fill_rect(HDC dc, short x, short y, unsigned short w,
    unsigned short h, unsigned short r, struct nk_color col)
{
    COLORREF color = convert_color(col);

    if (r == 0) {
        RECT rect = { x, y, x + w, y + h };
        SetBkColor(dc, color);
        ExtTextOutW(dc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
    } else {
        SetDCPenColor(dc, color);
        SetDCBrushColor(dc, color);
        RoundRect(dc, x, y, x + w, y + h, r, r);
    }
}
static void
nk_gdi_set_vertexColor(PTRIVERTEX tri, struct nk_color col)
{
    tri->Red   = col.r << 8;
    tri->Green = col.g << 8;
    tri->Blue  = col.b << 8;
    tri->Alpha = 0xff << 8;
}

static void
nk_gdi_rect_multi_color(HDC dc, short x, short y, unsigned short w,
    unsigned short h, struct nk_color left, struct nk_color top,
    struct nk_color right, struct nk_color bottom)
{
    BLENDFUNCTION alphaFunction;
    GRADIENT_RECT gRect;
    GRADIENT_TRIANGLE gTri[2];
    TRIVERTEX vt[4];
    alphaFunction.BlendOp = AC_SRC_OVER;
    alphaFunction.BlendFlags = 0;
    alphaFunction.SourceConstantAlpha = 0;
    alphaFunction.AlphaFormat = AC_SRC_ALPHA;

    /* TODO: This Case Needs Repair.*/
    /* Top Left Corner */
    vt[0].x     = x;
    vt[0].y     = y;
    nk_gdi_set_vertexColor(&vt[0], left);
    /* Top Right Corner */
    vt[1].x     = x+w;
    vt[1].y     = y;
    nk_gdi_set_vertexColor(&vt[1], top);
    /* Bottom Left Corner */
    vt[2].x     = x;
    vt[2].y     = y+h;
    nk_gdi_set_vertexColor(&vt[2], right);

    /* Bottom Right Corner */
    vt[3].x     = x+w;
    vt[3].y     = y+h;
    nk_gdi_set_vertexColor(&vt[3], bottom);

    gTri[0].Vertex1 = 0;
    gTri[0].Vertex2 = 1;
    gTri[0].Vertex3 = 2;
    gTri[1].Vertex1 = 2;
    gTri[1].Vertex2 = 1;
    gTri[1].Vertex3 = 3;
    GdiGradientFill(dc, vt, 4, gTri, 2 , GRADIENT_FILL_TRIANGLE);
    AlphaBlend(gdi.window_dc,  x, y, x+w, y+h,gdi.memory_dc, x, y, x+w, y+h,alphaFunction);

}

static void
nk_gdi_fill_triangle(HDC dc, short x0, short y0, short x1,
    short y1, short x2, short y2, struct nk_color col)
{
    COLORREF color = convert_color(col);
    POINT points[] = {
        { x0, y0 },
        { x1, y1 },
        { x2, y2 },
    };

    SetDCPenColor(dc, color);
    SetDCBrushColor(dc, color);
    Polygon(dc, points, 3);
}

static void
nk_gdi_stroke_triangle(HDC dc, short x0, short y0, short x1,
    short y1, short x2, short y2, unsigned short line_thickness, struct nk_color col)
{
    COLORREF color = convert_color(col);
    POINT points[] = {
        { x0, y0 },
        { x1, y1 },
        { x2, y2 },
        { x0, y0 },
    };

    HPEN pen = NULL;
    if (line_thickness == 1) {
        SetDCPenColor(dc, color);
    } else {
        pen = CreatePen(PS_SOLID, line_thickness, color);
        SelectObject(dc, pen);
    }

    Polyline(dc, points, 4);

    if (pen) {
        SelectObject(dc, GetStockObject(DC_PEN));
        DeleteObject(pen);
    }
}

static void
nk_gdi_fill_polygon(HDC dc, const struct nk_vec2i *pnts, int count, struct nk_color col)
{
    int i = 0;
    #define MAX_POINTS 64
    POINT points[MAX_POINTS];
    COLORREF color = convert_color(col);
    SetDCBrushColor(dc, color);
    SetDCPenColor(dc, color);
    for (i = 0; i < count && i < MAX_POINTS; ++i) {
        points[i].x = pnts[i].x;
        points[i].y = pnts[i].y;
    }
    Polygon(dc, points, i);
    #undef MAX_POINTS
}

static void
nk_gdi_stroke_polygon(HDC dc, const struct nk_vec2i *pnts, int count,
    unsigned short line_thickness, struct nk_color col)
{
    COLORREF color = convert_color(col);
    HPEN pen = NULL;
    if (line_thickness == 1) {
        SetDCPenColor(dc, color);
    } else {
        pen = CreatePen(PS_SOLID, line_thickness, color);
        SelectObject(dc, pen);
    }

    if (count > 0) {
        int i;
        MoveToEx(dc, pnts[0].x, pnts[0].y, NULL);
        for (i = 1; i < count; ++i)
            LineTo(dc, pnts[i].x, pnts[i].y);
        LineTo(dc, pnts[0].x, pnts[0].y);
    }

    if (pen) {
        SelectObject(dc, GetStockObject(DC_PEN));
        DeleteObject(pen);
    }
}

static void
nk_gdi_stroke_polyline(HDC dc, const struct nk_vec2i *pnts,
    int count, unsigned short line_thickness, struct nk_color col)
{
    COLORREF color = convert_color(col);
    HPEN pen = NULL;
    if (line_thickness == 1) {
        SetDCPenColor(dc, color);
    } else {
        pen = CreatePen(PS_SOLID, line_thickness, color);
        SelectObject(dc, pen);
    }

    if (count > 0) {
        int i;
        MoveToEx(dc, pnts[0].x, pnts[0].y, NULL);
        for (i = 1; i < count; ++i)
            LineTo(dc, pnts[i].x, pnts[i].y);
    }

    if (pen) {
        SelectObject(dc, GetStockObject(DC_PEN));
        DeleteObject(pen);
    }
}

static void
nk_gdi_fill_circle(HDC dc, short x, short y, unsigned short w,
    unsigned short h, struct nk_color col)
{
    COLORREF color = convert_color(col);
    SetDCBrushColor(dc, color);
    SetDCPenColor(dc, color);
    Ellipse(dc, x, y, x + w, y + h);
}

static void
nk_gdi_stroke_circle(HDC dc, short x, short y, unsigned short w,
    unsigned short h, unsigned short line_thickness, struct nk_color col)
{
    COLORREF color = convert_color(col);
    HPEN pen = NULL;
    if (line_thickness == 1) {
        SetDCPenColor(dc, color);
    } else {
        pen = CreatePen(PS_SOLID, line_thickness, color);
        SelectObject(dc, pen);
    }

    SetDCBrushColor(dc, OPAQUE);
    Ellipse(dc, x, y, x + w, y + h);

    if (pen) {
        SelectObject(dc, GetStockObject(DC_PEN));
        DeleteObject(pen);
    }
}

static void
nk_gdi_stroke_curve(HDC dc, struct nk_vec2i p1,
    struct nk_vec2i p2, struct nk_vec2i p3, struct nk_vec2i p4,
    unsigned short line_thickness, struct nk_color col)
{
    COLORREF color = convert_color(col);
    POINT p[] = {
        { p1.x, p1.y },
        { p2.x, p2.y },
        { p3.x, p3.y },
        { p4.x, p4.y },
    };

    HPEN pen = NULL;
    if (line_thickness == 1) {
        SetDCPenColor(dc, color);
    } else {
        pen = CreatePen(PS_SOLID, line_thickness, color);
        SelectObject(dc, pen);
    }

    SetDCBrushColor(dc, OPAQUE);
    PolyBezier(dc, p, 4);

    if (pen) {
        SelectObject(dc, GetStockObject(DC_PEN));
        DeleteObject(pen);
    }
}

static void
nk_gdi_draw_text(HDC dc, short x, short y, unsigned short w, unsigned short h,
    const char *text, int len, GdiFont *font, struct nk_color cbg, struct nk_color cfg)
{
    int wsize;
    WCHAR* wstr;

    if(!text || !font || !len) return;

    wsize = MultiByteToWideChar(CP_UTF8, 0, text, len, NULL, 0);
    wstr = (WCHAR*)_alloca(wsize * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, text, len, wstr, wsize);

    SetBkColor(dc, convert_color(cbg));
    SetTextColor(dc, convert_color(cfg));

    SelectObject(dc, font->handle);
    ExtTextOutW(dc, x, y, ETO_OPAQUE, NULL, wstr, wsize, NULL);
}

static void
nk_gdi_clear(HDC dc, struct nk_color col)
{
    COLORREF color = convert_color(col);
    RECT rect = { 0, 0, gdi.width, gdi.height };
    SetBkColor(dc, color);

    ExtTextOutW(dc, 0, 0, ETO_OPAQUE, &rect, NULL, 0, NULL);
}

static void
nk_gdi_blit(HDC dc)
{
    BitBlt(dc, 0, 0, gdi.width, gdi.height, gdi.memory_dc, 0, 0, SRCCOPY);

}

GdiFont*
nk_gdifont_create(const char *name, int size)
{
    TEXTMETRICW metric;
    GdiFont *font = (GdiFont*)calloc(1, sizeof(GdiFont));
    if (!font)
        return NULL;
    font->dc = CreateCompatibleDC(0);
    font->handle = CreateFontA(size, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, name);
    SelectObject(font->dc, font->handle);
    GetTextMetricsW(font->dc, &metric);
    font->height = metric.tmHeight;
    return font;
}

static float
nk_gdifont_get_text_width(nk_handle handle, float height, const char *text, int len)
{
    GdiFont *font = (GdiFont*)handle.ptr;
    SIZE size;
    int wsize;
    WCHAR* wstr;
    if (!font || !text)
        return 0;

    wsize = MultiByteToWideChar(CP_UTF8, 0, text, len, NULL, 0);
    wstr = (WCHAR*)_alloca(wsize * sizeof(wchar_t));
    MultiByteToWideChar(CP_UTF8, 0, text, len, wstr, wsize);
    if (GetTextExtentPoint32W(font->dc, wstr, wsize, &size))
        return (float)size.cx;
    return -1.0f;
}

void
nk_gdifont_del(GdiFont *font)
{
    if(!font) return;
    DeleteObject(font->handle);
    DeleteDC(font->dc);
    free(font);
}

static void
nk_gdi_clipboard_paste(nk_handle usr, struct nk_text_edit *edit)
{
    (void)usr;
    if (IsClipboardFormatAvailable(CF_UNICODETEXT) && OpenClipboard(NULL))
    {
        HGLOBAL mem = GetClipboardData(CF_UNICODETEXT); 
        if (mem)
        {
            SIZE_T size = GlobalSize(mem) - 1;
            if (size)
            {
                LPCWSTR wstr = (LPCWSTR)GlobalLock(mem);
                if (wstr) 
                {
                    int utf8size = WideCharToMultiByte(CP_UTF8, 0, wstr, (int)(size / sizeof(wchar_t)), NULL, 0, NULL, NULL);
                    if (utf8size)
                    {
                        char* utf8 = (char*)malloc(utf8size);
                        if (utf8)
                        {
                            WideCharToMultiByte(CP_UTF8, 0, wstr, (int)(size / sizeof(wchar_t)), utf8, utf8size, NULL, NULL);
                            nk_textedit_paste(edit, utf8, utf8size);
                            free(utf8);
                        }
                    }
                    GlobalUnlock(mem); 
                }
            }
        }
        CloseClipboard();
    }
}

static void
nk_gdi_clipboard_copy(nk_handle usr, const char *text, int len)
{
    if (OpenClipboard(NULL))
    {
        int wsize = MultiByteToWideChar(CP_UTF8, 0, text, len, NULL, 0);
        if (wsize)
        {
            HGLOBAL mem = (HGLOBAL)GlobalAlloc(GMEM_MOVEABLE, (wsize + 1) * sizeof(wchar_t));
            if (mem)
            {
                wchar_t* wstr = (wchar_t*)GlobalLock(mem);
                if (wstr)
                {
                    MultiByteToWideChar(CP_UTF8, 0, text, len, wstr, wsize);
                    wstr[wsize] = 0;
                    GlobalUnlock(mem); 

                    SetClipboardData(CF_UNICODETEXT, mem); 
                }
            }
        }
        CloseClipboard();
    }
}

NK_API struct nk_context*
nk_gdi_init(GdiFont *gdifont, HDC window_dc, unsigned int width, unsigned int height)
{
    struct nk_user_font *font = &gdifont->nk;
    font->userdata = nk_handle_ptr(gdifont);
    font->height = (float)gdifont->height;
    font->width = nk_gdifont_get_text_width;

    gdi.bitmap = CreateCompatibleBitmap(window_dc, width, height);
    gdi.window_dc = window_dc;
    gdi.memory_dc = CreateCompatibleDC(window_dc);
    gdi.width = width;
    gdi.height = height;
    SelectObject(gdi.memory_dc, gdi.bitmap);

    nk_init_default(&gdi.ctx, font);
    gdi.ctx.clip.copy = nk_gdi_clipboard_copy;
    gdi.ctx.clip.paste = nk_gdi_clipboard_paste;
    return &gdi.ctx;
}

NK_API void
nk_gdi_set_font(GdiFont *gdifont)
{
    struct nk_user_font *font = &gdifont->nk;
    font->userdata = nk_handle_ptr(gdifont);
    font->height = (float)gdifont->height;
    font->width = nk_gdifont_get_text_width;
    nk_style_set_font(&gdi.ctx, font);
}

NK_API int
nk_gdi_handle_event(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
    switch (msg)
    {
    case WM_SIZE:
    {
        unsigned width = LOWORD(lparam);
        unsigned height = HIWORD(lparam);
        if (width != gdi.width || height != gdi.height)
        {
            DeleteObject(gdi.bitmap);
            gdi.bitmap = CreateCompatibleBitmap(gdi.window_dc, width, height);
            gdi.width = width;
            gdi.height = height;
            SelectObject(gdi.memory_dc, gdi.bitmap);
        }
        break;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT paint;
        HDC dc = BeginPaint(wnd, &paint);
        nk_gdi_blit(dc);
        EndPaint(wnd, &paint);
        return 1;
    }

    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
    {
        int down = !((lparam >> 31) & 1);
        int ctrl = GetKeyState(VK_CONTROL) & (1 << 15);

        switch (wparam)
        {
        case VK_SHIFT:
        case VK_LSHIFT:
        case VK_RSHIFT:
            nk_input_key(&gdi.ctx, NK_KEY_SHIFT, down);
            return 1;

        case VK_DELETE:
            nk_input_key(&gdi.ctx, NK_KEY_DEL, down);
            return 1;

        case VK_RETURN:
            nk_input_key(&gdi.ctx, NK_KEY_ENTER, down);
            return 1;

        case VK_TAB:
            nk_input_key(&gdi.ctx, NK_KEY_TAB, down);
            return 1;

        case VK_LEFT:
            if (ctrl)
                nk_input_key(&gdi.ctx, NK_KEY_TEXT_WORD_LEFT, down);
            else
                nk_input_key(&gdi.ctx, NK_KEY_LEFT, down);
            return 1;

        case VK_RIGHT:
            if (ctrl)
                nk_input_key(&gdi.ctx, NK_KEY_TEXT_WORD_RIGHT, down);
            else
                nk_input_key(&gdi.ctx, NK_KEY_RIGHT, down);
            return 1;

        case VK_BACK:
            nk_input_key(&gdi.ctx, NK_KEY_BACKSPACE, down);
            return 1;

        case VK_HOME:
            nk_input_key(&gdi.ctx, NK_KEY_TEXT_START, down);
            nk_input_key(&gdi.ctx, NK_KEY_SCROLL_START, down);
            return 1;

        case VK_END:
            nk_input_key(&gdi.ctx, NK_KEY_TEXT_END, down);
            nk_input_key(&gdi.ctx, NK_KEY_SCROLL_END, down);
            return 1;

        case VK_NEXT:
            nk_input_key(&gdi.ctx, NK_KEY_SCROLL_DOWN, down);
            return 1;

        case VK_PRIOR:
            nk_input_key(&gdi.ctx, NK_KEY_SCROLL_UP, down);
            return 1;

        case 'C':
            if (ctrl) {
                nk_input_key(&gdi.ctx, NK_KEY_COPY, down);
                return 1;
            }
            break;

        case 'V':
            if (ctrl) {
                nk_input_key(&gdi.ctx, NK_KEY_PASTE, down);
                return 1;
            }
            break;

        case 'X':
            if (ctrl) {
                nk_input_key(&gdi.ctx, NK_KEY_CUT, down);
                return 1;
            }
            break;

        case 'Z':
            if (ctrl) {
                nk_input_key(&gdi.ctx, NK_KEY_TEXT_UNDO, down);
                return 1;
            }
            break;

        case 'R':
            if (ctrl) {
                nk_input_key(&gdi.ctx, NK_KEY_TEXT_REDO, down);
                return 1;
            }
            break;
        }
        return 0;
    }

    case WM_CHAR:
        if (wparam >= 32)
        {
            nk_input_unicode(&gdi.ctx, (nk_rune)wparam);
            return 1;
        }
        break;

    case WM_LBUTTONDOWN:
        nk_input_button(&gdi.ctx, NK_BUTTON_LEFT, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
        SetCapture(wnd);
        return 1;

    case WM_LBUTTONUP:
        nk_input_button(&gdi.ctx, NK_BUTTON_DOUBLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
        nk_input_button(&gdi.ctx, NK_BUTTON_LEFT, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
        ReleaseCapture();
        return 1;

    case WM_RBUTTONDOWN:
        nk_input_button(&gdi.ctx, NK_BUTTON_RIGHT, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
        SetCapture(wnd);
        return 1;

    case WM_RBUTTONUP:
        nk_input_button(&gdi.ctx, NK_BUTTON_RIGHT, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
        ReleaseCapture();
        return 1;

    case WM_MBUTTONDOWN:
        nk_input_button(&gdi.ctx, NK_BUTTON_MIDDLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
        SetCapture(wnd);
        return 1;

    case WM_MBUTTONUP:
        nk_input_button(&gdi.ctx, NK_BUTTON_MIDDLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 0);
        ReleaseCapture();
        return 1;

    case WM_MOUSEWHEEL:
        nk_input_scroll(&gdi.ctx, nk_vec2(0,(float)(short)HIWORD(wparam) / WHEEL_DELTA));
        return 1;

    case WM_MOUSEMOVE:
        nk_input_motion(&gdi.ctx, (short)LOWORD(lparam), (short)HIWORD(lparam));
        return 1;

    case WM_LBUTTONDBLCLK:
        nk_input_button(&gdi.ctx, NK_BUTTON_DOUBLE, (short)LOWORD(lparam), (short)HIWORD(lparam), 1);
        return 1;
    }

    return 0;
}

NK_API void
nk_gdi_shutdown(void)
{
    DeleteObject(gdi.memory_dc);
    DeleteObject(gdi.bitmap);
    nk_free(&gdi.ctx);
}

NK_API void
nk_gdi_render(struct nk_color clear)
{
    const struct nk_command *cmd;

    HDC memory_dc = gdi.memory_dc;
    SelectObject(memory_dc, GetStockObject(DC_PEN));
    SelectObject(memory_dc, GetStockObject(DC_BRUSH));
    nk_gdi_clear(memory_dc, clear);

    nk_foreach(cmd, &gdi.ctx)
    {
        switch (cmd->type) {
        case NK_COMMAND_NOP: break;
        case NK_COMMAND_SCISSOR: {
            const struct nk_command_scissor *s =(const struct nk_command_scissor*)cmd;
            nk_gdi_scissor(memory_dc, s->x, s->y, s->w, s->h);
        } break;
        case NK_COMMAND_LINE: {
            const struct nk_command_line *l = (const struct nk_command_line *)cmd;
            nk_gdi_stroke_line(memory_dc, l->begin.x, l->begin.y, l->end.x,
                l->end.y, l->line_thickness, l->color);
        } break;
        case NK_COMMAND_RECT: {
            const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
            nk_gdi_stroke_rect(memory_dc, r->x, r->y, r->w, r->h,
                (unsigned short)r->rounding, r->line_thickness, r->color);
        } break;
        case NK_COMMAND_RECT_FILLED: {
            const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled *)cmd;
            nk_gdi_fill_rect(memory_dc, r->x, r->y, r->w, r->h,
                (unsigned short)r->rounding, r->color);
        } break;
        case NK_COMMAND_CIRCLE: {
            const struct nk_command_circle *c = (const struct nk_command_circle *)cmd;
            nk_gdi_stroke_circle(memory_dc, c->x, c->y, c->w, c->h, c->line_thickness, c->color);
        } break;
        case NK_COMMAND_CIRCLE_FILLED: {
            const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;
            nk_gdi_fill_circle(memory_dc, c->x, c->y, c->w, c->h, c->color);
        } break;
        case NK_COMMAND_TRIANGLE: {
            const struct nk_command_triangle*t = (const struct nk_command_triangle*)cmd;
            nk_gdi_stroke_triangle(memory_dc, t->a.x, t->a.y, t->b.x, t->b.y,
                t->c.x, t->c.y, t->line_thickness, t->color);
        } break;
        case NK_COMMAND_TRIANGLE_FILLED: {
            const struct nk_command_triangle_filled *t = (const struct nk_command_triangle_filled *)cmd;
            nk_gdi_fill_triangle(memory_dc, t->a.x, t->a.y, t->b.x, t->b.y,
                t->c.x, t->c.y, t->color);
        } break;
        case NK_COMMAND_POLYGON: {
            const struct nk_command_polygon *p =(const struct nk_command_polygon*)cmd;
            nk_gdi_stroke_polygon(memory_dc, p->points, p->point_count, p->line_thickness,p->color);
        } break;
        case NK_COMMAND_POLYGON_FILLED: {
            const struct nk_command_polygon_filled *p = (const struct nk_command_polygon_filled *)cmd;
            nk_gdi_fill_polygon(memory_dc, p->points, p->point_count, p->color);
        } break;
        case NK_COMMAND_POLYLINE: {
            const struct nk_command_polyline *p = (const struct nk_command_polyline *)cmd;
            nk_gdi_stroke_polyline(memory_dc, p->points, p->point_count, p->line_thickness, p->color);
        } break;
        case NK_COMMAND_TEXT: {
            const struct nk_command_text *t = (const struct nk_command_text*)cmd;
            nk_gdi_draw_text(memory_dc, t->x, t->y, t->w, t->h,
                (const char*)t->string, t->length,
                (GdiFont*)t->font->userdata.ptr,
                t->background, t->foreground);
        } break;
        case NK_COMMAND_CURVE: {
            const struct nk_command_curve *q = (const struct nk_command_curve *)cmd;
            nk_gdi_stroke_curve(memory_dc, q->begin, q->ctrl[0], q->ctrl[1],
                q->end, q->line_thickness, q->color);
        } break;
        case NK_COMMAND_RECT_MULTI_COLOR: {
            const struct nk_command_rect_multi_color *r = (const struct nk_command_rect_multi_color *)cmd;
            nk_gdi_rect_multi_color(memory_dc, r->x, r->y,r->w, r->h, r->left, r->top, r->right, r->bottom);
        } break;
        case NK_COMMAND_IMAGE: {
            const struct nk_command_image *i = (const struct nk_command_image *)cmd;
            nk_gdi_draw_image(i->x, i->y, i->w, i->h, i->img, i->col);
        } break;
        case NK_COMMAND_ARC:
        case NK_COMMAND_ARC_FILLED:
        default: break;
        }
    }
    nk_gdi_blit(gdi.window_dc);
    nk_clear(&gdi.ctx);
}

#endif

