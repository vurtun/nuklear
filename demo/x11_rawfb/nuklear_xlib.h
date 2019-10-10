/*
 * MIT License
 *
 * Copyright (c) 2016-2017 Patrick Rudolph <siro@das-labor.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Based on x11/nuklear_xlib.h.
*/
/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */
#ifndef NK_XLIBSHM_H_
#define NK_XLIBSHM_H_

#include <X11/Xlib.h>

NK_API int  nk_xlib_init(Display *dpy, Visual *vis, int screen, Window root, unsigned int w, unsigned int h, void **fb, rawfb_pl *pl);
NK_API int  nk_xlib_handle_event(Display *dpy, int screen, Window win, XEvent *evt, struct rawfb_context *rawfb);
NK_API void nk_xlib_render(Drawable screen);
NK_API void nk_xlib_shutdown(void);

#endif
/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#ifdef NK_XLIBSHM_IMPLEMENTATION
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Xlocale.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>

static struct  {
    struct nk_context ctx;
    struct XSurface *surf;
    Cursor cursor;
    Display *dpy;
    Window root;
    XImage *ximg;
    XShmSegmentInfo xsi;
    char fallback;
    GC gc;
} xlib;

NK_API int
nk_xlib_init(Display *dpy, Visual *vis, int screen, Window root,
    unsigned int w, unsigned int h, void **fb, rawfb_pl *pl)
{
    unsigned int depth = XDefaultDepth(dpy, screen);
    xlib.dpy = dpy;
    xlib.root = root;

    if (!setlocale(LC_ALL,"")) return 0;
    if (!XSupportsLocale()) return 0;
    if (!XSetLocaleModifiers("@im=none")) return 0;

    /* create invisible cursor */
    {static XColor dummy; char data[1] = {0};
    Pixmap blank = XCreateBitmapFromData(dpy, root, data, 1, 1);
    if (blank == None) return 0;
    xlib.cursor = XCreatePixmapCursor(dpy, blank, blank, &dummy, &dummy, 0, 0);
    XFreePixmap(dpy, blank);}

    xlib.fallback = False;
    do {/* Initialize shared memory according to:
         * https://www.x.org/archive/X11R7.5/doc/Xext/mit-shm.html */
        int status;
        if (!XShmQueryExtension(dpy)) {
            printf("No XShm Extension available.\n");
            xlib.fallback = True;
            break;
        }
        xlib.ximg = XShmCreateImage(dpy, vis, depth, ZPixmap, NULL, &xlib.xsi, w, h);
        if (!xlib.ximg) {
            xlib.fallback = True;
            break;
        }
        xlib.xsi.shmid = shmget(IPC_PRIVATE, xlib.ximg->bytes_per_line * xlib.ximg->height, IPC_CREAT | 0777);
        if (xlib.xsi.shmid < 0) {
            XDestroyImage(xlib.ximg);
            xlib.fallback = True;
            break;
        }
        xlib.xsi.shmaddr = xlib.ximg->data = shmat(xlib.xsi.shmid, NULL, 0);
        if ((size_t)xlib.xsi.shmaddr < 0) {
            XDestroyImage(xlib.ximg);
            xlib.fallback = True;
            break;
        }
        xlib.xsi.readOnly = False;
        status = XShmAttach(dpy, &xlib.xsi);
        if (!status) {
            shmdt(xlib.xsi.shmaddr);
            XDestroyImage(xlib.ximg);
            xlib.fallback = True;
            break;
        } XSync(dpy, False);
        shmctl(xlib.xsi.shmid, IPC_RMID, NULL);
    } while(0);

    if (xlib.fallback) {
        xlib.ximg = XCreateImage(dpy, vis, depth, ZPixmap, 0, NULL, w, h, 32, 0);
        if (!xlib.ximg) return 0;
        xlib.ximg->data = malloc(h * xlib.ximg->bytes_per_line);
        if (!xlib.ximg->data)
            return 0;
    }
    xlib.gc = XDefaultGC(dpy, screen);
    *fb = xlib.ximg->data;

    if (xlib.ximg->red_mask == 0xff0000 &&
	xlib.ximg->green_mask == 0xff00 &&
	xlib.ximg->blue_mask == 0xff &&
	xlib.ximg->bits_per_pixel == 32) {
	*pl = PIXEL_LAYOUT_XRGB_8888;
    }
    else if (xlib.ximg->red_mask == 0xff000000 &&
	     xlib.ximg->green_mask == 0xff0000 &&
	     xlib.ximg->blue_mask == 0xff00 &&
	     xlib.ximg->bits_per_pixel == 32) {
	*pl = PIXEL_LAYOUT_RGBX_8888;
    }
    else {
	perror("nk_xlib_init(): Unrecognized pixel layout.\n");
	return 0;
    }

    return 1;
}

NK_API int
nk_xlib_handle_event(Display *dpy, int screen, Window win, XEvent *evt, struct rawfb_context *rawfb)
{
    /* optional grabbing behavior */
    if (rawfb->ctx.input.mouse.grab) {
        /* XDefineCursor(xlib.dpy, xlib.root, xlib.cursor); */
        rawfb->ctx.input.mouse.grab = 0;
    } else if (rawfb->ctx.input.mouse.ungrab) {
        XWarpPointer(xlib.dpy, None, xlib.root, 0, 0, 0, 0,
            (int)rawfb->ctx.input.mouse.prev.x, (int)rawfb->ctx.input.mouse.prev.y);
        /* XUndefineCursor(xlib.dpy, xlib.root); */
        rawfb->ctx.input.mouse.ungrab = 0;
    }

    if (evt->type == KeyPress || evt->type == KeyRelease)
    {
        /* Key handler */
        int ret, down = (evt->type == KeyPress);
        KeySym *code = XGetKeyboardMapping(xlib.dpy, (KeyCode)evt->xkey.keycode, 1, &ret);
        if (*code == XK_Shift_L || *code == XK_Shift_R) nk_input_key(&rawfb->ctx, NK_KEY_SHIFT, down);
        else if (*code == XK_Control_L || *code == XK_Control_R) nk_input_key(&rawfb->ctx, NK_KEY_CTRL, down);
        else if (*code == XK_Delete)    nk_input_key(&rawfb->ctx, NK_KEY_DEL, down);
        else if (*code == XK_Return)    nk_input_key(&rawfb->ctx, NK_KEY_ENTER, down);
        else if (*code == XK_Tab)       nk_input_key(&rawfb->ctx, NK_KEY_TAB, down);
        else if (*code == XK_Left)      nk_input_key(&rawfb->ctx, NK_KEY_LEFT, down);
        else if (*code == XK_Right)     nk_input_key(&rawfb->ctx, NK_KEY_RIGHT, down);
        else if (*code == XK_Up)        nk_input_key(&rawfb->ctx, NK_KEY_UP, down);
        else if (*code == XK_Down)      nk_input_key(&rawfb->ctx, NK_KEY_DOWN, down);
        else if (*code == XK_BackSpace) nk_input_key(&rawfb->ctx, NK_KEY_BACKSPACE, down);
        else if (*code == XK_Escape)    nk_input_key(&rawfb->ctx, NK_KEY_TEXT_RESET_MODE, down);
        else if (*code == XK_Page_Up)   nk_input_key(&rawfb->ctx, NK_KEY_SCROLL_UP, down);
        else if (*code == XK_Page_Down) nk_input_key(&rawfb->ctx, NK_KEY_SCROLL_DOWN, down);
        else if (*code == XK_Home) {
            nk_input_key(&rawfb->ctx, NK_KEY_TEXT_START, down);
            nk_input_key(&rawfb->ctx, NK_KEY_SCROLL_START, down);
        } else if (*code == XK_End) {
            nk_input_key(&rawfb->ctx, NK_KEY_TEXT_END, down);
            nk_input_key(&rawfb->ctx, NK_KEY_SCROLL_END, down);
        } else {
            if (*code == 'c' && (evt->xkey.state & ControlMask))
                nk_input_key(&rawfb->ctx, NK_KEY_COPY, down);
            else if (*code == 'v' && (evt->xkey.state & ControlMask))
                nk_input_key(&rawfb->ctx, NK_KEY_PASTE, down);
            else if (*code == 'x' && (evt->xkey.state & ControlMask))
                nk_input_key(&rawfb->ctx, NK_KEY_CUT, down);
            else if (*code == 'z' && (evt->xkey.state & ControlMask))
                nk_input_key(&rawfb->ctx, NK_KEY_TEXT_UNDO, down);
            else if (*code == 'r' && (evt->xkey.state & ControlMask))
                nk_input_key(&rawfb->ctx, NK_KEY_TEXT_REDO, down);
            else if (*code == XK_Left && (evt->xkey.state & ControlMask))
                nk_input_key(&rawfb->ctx, NK_KEY_TEXT_WORD_LEFT, down);
            else if (*code == XK_Right && (evt->xkey.state & ControlMask))
                nk_input_key(&rawfb->ctx, NK_KEY_TEXT_WORD_RIGHT, down);
            else if (*code == 'b' && (evt->xkey.state & ControlMask))
                nk_input_key(&rawfb->ctx, NK_KEY_TEXT_LINE_START, down);
            else if (*code == 'e' && (evt->xkey.state & ControlMask))
                nk_input_key(&rawfb->ctx, NK_KEY_TEXT_LINE_END, down);
            else {
                if (*code == 'i')
                    nk_input_key(&rawfb->ctx, NK_KEY_TEXT_INSERT_MODE, down);
                else if (*code == 'r')
                    nk_input_key(&rawfb->ctx, NK_KEY_TEXT_REPLACE_MODE, down);
                if (down) {
                    char buf[32];
                    KeySym keysym = 0;
                    if (XLookupString((XKeyEvent*)evt, buf, 32, &keysym, NULL) != NoSymbol)
                        nk_input_glyph(&rawfb->ctx, buf);
                }
            }
        } XFree(code);
        return 1;
    } else if (evt->type == ButtonPress || evt->type == ButtonRelease) {
        /* Button handler */
        int down = (evt->type == ButtonPress);
        const int x = evt->xbutton.x, y = evt->xbutton.y;
        if (evt->xbutton.button == Button1)
            nk_input_button(&rawfb->ctx, NK_BUTTON_LEFT, x, y, down);
        if (evt->xbutton.button == Button2)
            nk_input_button(&rawfb->ctx, NK_BUTTON_MIDDLE, x, y, down);
        else if (evt->xbutton.button == Button3)
            nk_input_button(&rawfb->ctx, NK_BUTTON_RIGHT, x, y, down);
        else if (evt->xbutton.button == Button4)
            nk_input_scroll(&rawfb->ctx, nk_vec2(0, 1.0f));
        else if (evt->xbutton.button == Button5)
            nk_input_scroll(&rawfb->ctx, nk_vec2(0, -1.0f));
        else return 0;
        return 1;
    } else if (evt->type == MotionNotify) {
        /* Mouse motion handler */
        const int x = evt->xmotion.x, y = evt->xmotion.y;
        nk_input_motion(&rawfb->ctx, x, y);
        if (rawfb->ctx.input.mouse.grabbed) {
            rawfb->ctx.input.mouse.pos.x = rawfb->ctx.input.mouse.prev.x;
            rawfb->ctx.input.mouse.pos.y = rawfb->ctx.input.mouse.prev.y;
            XWarpPointer(xlib.dpy, None, xlib.root, 0, 0, 0, 0, (int)rawfb->ctx.input.mouse.pos.x, (int)rawfb->ctx.input.mouse.pos.y);
        } return 1;
    } else if (evt->type == Expose || evt->type == ConfigureNotify) {
        /* Window resize handler */
        void *fb;
        unsigned int width, height;
        XWindowAttributes attr;
        XGetWindowAttributes(dpy, win, &attr);
	rawfb_pl pl;

        width = (unsigned int)attr.width;
        height = (unsigned int)attr.height;

        nk_xlib_shutdown();
        nk_xlib_init(dpy, XDefaultVisual(dpy, screen), screen, win, width, height, &fb, &pl);
        nk_rawfb_resize_fb(rawfb, fb, width, height, width * 4, pl);
    } else if (evt->type == KeymapNotify) {
        XRefreshKeyboardMapping(&evt->xmapping);
        return 1;
    } else if (evt->type == LeaveNotify) {
        XUndefineCursor(xlib.dpy, xlib.root);
    } else if (evt->type == EnterNotify) {
        XDefineCursor(xlib.dpy, xlib.root, xlib.cursor);
    } return 0;
}

NK_API void
nk_xlib_shutdown(void)
{
    XFreeCursor(xlib.dpy, xlib.cursor);
    if (xlib.fallback) {
        free(xlib.ximg->data);
        XDestroyImage(xlib.ximg);
    } else {
        XShmDetach(xlib.dpy, &xlib.xsi);
        XDestroyImage(xlib.ximg);
        shmdt(xlib.xsi.shmaddr);
        shmctl(xlib.xsi.shmid, IPC_RMID, NULL);
    } NK_MEMSET(&xlib, 0, sizeof(xlib));
}

NK_API void
nk_xlib_render(Drawable screen)
{
    if (xlib.fallback)
        XPutImage(xlib.dpy, screen, xlib.gc, xlib.ximg,
            0, 0, 0, 0, xlib.ximg->width, xlib.ximg->height);
    else XShmPutImage(xlib.dpy, screen, xlib.gc, xlib.ximg,
            0, 0, 0, 0, xlib.ximg->width, xlib.ximg->height, False);
}
#endif

