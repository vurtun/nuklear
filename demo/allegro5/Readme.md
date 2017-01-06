# Allegro v5 nuklear backend

This backend provides support for [Allegro version 5](http://liballeg.org/). It works on on all supported platforms with an OpenGL backend, including iOS and Android.

Touch support is provided by handling the first touch (ignoring any extra simultaneous touches) and emitting nuklear mouse events. nuklear will handle only the first touch like a single left-mouse click. Dragging the touch screen emits mouse-move events.

## Compiling
You must link with image, font, ttf, and primitives Allegro addons. See the `Makefile`.

## Resolutions

Like every nuklear backend, handling many different resolutions and resolution densities can be tricky. 14px font on a desktop may be perfect, but extremely small on a retina iPad. I recommend writing a middleware that will detect what kind of screen is being used, and modify the sizes of widgets accordingly.

## Soft Keyboard for Touch Screen Devices

Information on how to implement soft keyboard callbacks for Android can be on the Allegro community wiki: https://wiki.allegro.cc/index.php?title=Running_Allegro_applications_on_Android#Displaying_the_Android_keyboard

To display a soft keyboard on iOS, you must create a `UIView` subclass that implements the `UIKeyInput` interface. See `KeyboardHandleriOS.h` and `KeyboardHandleriOS.m` Objective-C source code files for an example on how to do this. As the Allegro keyboard driver does not currently listen for iOS events, we use a custom event emitter to emit keyboard events, which is passed in after initialization with `(void)setCustomKeyboardEventSource:(ALLEGRO_EVENT_SOURCE *)ev_src`. This causes normal keyboard events to be emitted and properly caught by the nuklear backend. The provided `main.c` demo file does not implement this, but with the provided source code files it is not difficult to do. See this Allegro community forum thread for more information: https://www.allegro.cc/forums/thread/616672

To know when nuklear wants to open and close the keyboard, you can check edit widget flags:

```
nk_flags ed_flags = nk_edit_string(ctx, NK_EDIT_FIELD, field_buffer, &field_len, 64, nk_filter_default);
if (ed_flags & NK_EDIT_ACTIVATED)
    open_ios_soft_keyboard();
if (ed_flags &  NK_EDIT_DEACTIVATED)
    close_ios_soft_keyboard();
```

### Manual Soft Keyboard Dismissal
As the user can dismiss a keyboard manually, nuklear will not be aware when this occurs, and the text edit cursor will think the entry field is still active. I recommend catching the dismiss event, then emitting `ALLEGRO_EVENT_TOUCH_BEGIN` and `ALLEGRO_EVENT_TOUCH_END` events in an unused portion of the screen (like the bottom-right corner). This will simulate the user touching outside of the text entry widget, which will make the edit field inactive.

### The Keyboard Covers Widgets

If you have a widget near the bottom of the screen, the keyboard opening woll cover it, and the user won't see what they are entering. One way to handle this is to make all text edit widgets view-only, and when tapped you dynamically create a new widget above the keyboard that receives all the key strokes. When the user dismisses the keyboard, copy the result from the new widget into the existing read-only text view and destroy the dynamic one.