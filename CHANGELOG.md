# Changelog
- 2016/12/03 (1.191)- Fixed wrapped text with no seperator and C89 error
- 2016/12/03 (1.19) - Changed text wrapping to process words not characters
- 2016/11/22 (1.184)- Fixed window minimized closing bug
- 2016/11/19 (1.184)- Fixed abstract combo box closing behavior
- 2016/11/19 (1.184)- Fixed tooltip flickering
- 2016/11/19 (1.183)- Fixed memory leak caused by popup repeated closing
- 2016/11/18 (1.182)- Fixed memory leak caused by popup panel allocation
- 2016/11/10 (1.181)- Fixed some warnings and C++ error
- 2016/11/10 (1.180)- Added additional `nk_button` versions which allows to directly
                        pass in a style struct to change buttons visual.
- 2016/11/10 (1.180)- Added additional 'nk_tree' versions to support external state
                        storage. Just like last the `nk_group` commit the main
                        advantage is that you optionally can minimize nuklears runtime
                        memory consumption or handle hash collisions.
- 2016/11/09 (1.180)- Added additional `nk_group` version to support external scrollbar
                        offset storage. Main advantage is that you can externalize
                        the memory management for the offset. It could also be helpful
                        if you have a hash collision in `nk_group_begin` but really
                        want the name. In addition I added `nk_list_view` which allows
                        to draw big lists inside a group without actually having to
                        commit the whole list to nuklear (issue #269).
- 2016/10/30 (1.171)- Fixed clipping rectangle bug inside `nk_draw_list`
- 2016/10/29 (1.170)- Pulled `nk_panel` memory management into nuklear and out of
                        the hands of the user. From now on users don't have to care
                        about panels unless they care about some information. If you
                        still need the panel just call `nk_window_get_panel`.
- 2016/10/21 (1.160)- Changed widget border drawing to stroked rectangle from filled
                        rectangle for less overdraw and widget background transparency.
- 2016/10/18 (1.160)- Added `nk_edit_focus` for manually edit widget focus control
- 2016/09/29 (1.157)- Fixed deduction of basic type in non `<stdint.h>` compilation
- 2016/09/29 (1.156)- Fixed edit widget UTF-8 text cursor drawing bug
- 2016/09/28 (1.156)- Fixed edit widget UTF-8 text appending/inserting/removing
- 2016/09/28 (1.156)- Fixed drawing bug inside edit widgets which offset all text
                        text in every edit widget if one of them is scrolled.
- 2016/09/28 (1.155)- Fixed small bug in edit widgets if not active. The wrong
                        text length is passed. It should have been in bytes but
                        was passed as glyphes.
- 2016/09/20 (1.154)- Fixed color button size calculation
- 2016/09/20 (1.153)- Fixed some `nk_vsnprintf` behavior bugs and removed
                        `<stdio.h>` again from `NK_INCLUDE_STANDARD_VARARGS`.
- 2016/09/18 (1.152)- C89 does not support vsnprintf only C99 and newer as well
                        as C++11 and newer. In addition to use vsnprintf you have
                        to include <stdio.h>. So just defining `NK_INCLUDE_STD_VAR_ARGS`
                        is not enough. That behavior is now fixed. By default if
                        both varargs as well as stdio is selected I try to use
                        vsnprintf if not possible I will revert to vsprintf. If
                        varargs but not stdio was defined I will use my own function.
- 2016/09/15 (1.151)- Fixed panel `close` behavior for deeper panel levels
- 2016/09/15 (1.151)- Fixed C++ errors and wrong argument to `nk_panel_get_xxxx`
- 2016/09/13 (1.15) - !BREAKING! Fixed nonblocking popup behavior in menu, combo,
                        and contextual which prevented closing in y-direction if
                        popup did not reach max height.
                        In addition the height parameter was changed into vec2
                        for width and height to have more control over the popup size.
- 2016/09/13 (1.15) - Cleaned up and extended type selection
- 2016/09/13 (1.141)- Fixed slider behavior hopefully for the last time. This time
                        all calculation are correct so no more hackery.
- 2016/09/13 (1.141)- Internal change to divide window/panel flags into panel flags and types.
                        Suprisinly spend years in C and still happened to confuse types
                        with flags. Probably something to take note.
- 2016/09/08 (1.14)- Added additional helper function to make it easier to just
                        take the produced buffers from `nk_convert` and unplug the
                        iteration process from `nk_context`. So now you can
                        just use the vertex,element and command buffer + two pointer
                        inside the command buffer retrieved by calls `nk__draw_begin`
                        and `nk__draw_end` and macro `nk_draw_foreach_bounded`.
- 2016/09/08 (1.14)- Added additional asserts to make sure every `nk_xxx_begin` call
                    for windows, popups, combobox, menu and contextual is guarded by
                    `if` condition and does not produce false drawing output.
- 2016/09/08 (1.14)- Changed confusing name for `NK_SYMBOL_RECT_FILLED`, `NK_SYMBOL_RECT`
                        to hopefully easier to understand `NK_SYMBOL_RECT_FILLED` and
                        `NK_SYMBOL_RECT_OUTLINE`.
- 2016/09/08 (1.14)- Changed confusing name for `NK_SYMBOL_CIRLCE_FILLED`, `NK_SYMBOL_CIRCLE`
                        to hopefully easier to understand `NK_SYMBOL_CIRCLE_FILLED` and
                        `NK_SYMBOL_CIRCLE_OUTLINE`.
- 2016/09/08 (1.14)- Added additional checks to select correct types if `NK_INCLUDE_FIXED_TYPES`
                        is not defined by supporting the biggest compiler GCC, clang and MSVC.
- 2016/09/07 (1.133)- Fixed `NK_INCLUDE_COMMAND_USERDATA` define to not cause an error
- 2016/09/04 (1.132)- Fixed wrong combobox height calculation
- 2016/09/03 (1.131)- Fixed gaps inside combo boxes in OpenGL
- 2016/09/02 (1.13) - Changed nuklear to not have any default vertex layout and
                        instead made it user provided. The range of types to convert
                        to is quite limited at the moment, but I would be more than
                        happy to accept PRs to add additional.
- 2016/08/30 (1.12) - Removed unused variables
- 2016/08/30 (1.12) - Fixed C++ build errors
- 2016/08/30 (1.12) - Removed mouse dragging from SDL demo since it does not work correctly
- 2016/08/30 (1.12) - Tweaked some default styling variables
- 2016/08/30 (1.12) - Hopefully fixed drawing bug in slider, in general I would
                        refrain from using slider with a big number of steps.
- 2016/08/30 (1.12) - Fixed close and minimize button which would fire even if the
                        window was in Read Only Mode.
- 2016/08/30 (1.12) - Fixed popup panel padding handling which was previously just
                        a hack for combo box and menu.
- 2016/08/30 (1.12) - Removed `NK_WINDOW_DYNAMIC` flag from public API since
                        it is bugged and causes issues in window selection.
- 2016/08/30 (1.12) - Removed scaler size. The size of the scaler is now
                        determined by the scrollbar size
- 2016/08/30 (1.12) - Fixed some drawing bugs caused by changes from 1.11
- 2016/08/30 (1.12) - Fixed overlapping minimized window selection
- 2016/08/30 (1.11) - Removed some internal complexity and overly complex code
                        handling panel padding and panel border.
- 2016/08/29 (1.10) - Added additional height parameter to `nk_combobox_xxx`
- 2016/08/29 (1.10) - Fixed drawing bug in dynamic popups
- 2016/08/29 (1.10) - Added experimental mouse scrolling to popups, menus and comboboxes
- 2016/08/26 (1.10) - Added window name string prepresentation to account for
                        hash collisions. Currently limited to NK_WINDOW_MAX_NAME
                        which in term can be redefined if not big enough.
- 2016/08/26 (1.10) - Added stacks for temporary style/UI changes in code
- 2016/08/25 (1.10) - Changed `nk_input_is_key_pressed` and 'nk_input_is_key_released'
                        to account for key press and release happening in one frame.
- 2016/08/25 (1.10) - Added additional nk_edit flag to directly jump to the end on activate
- 2016/08/17 (1.096)- Removed invalid check for value zero in nk_propertyx
- 2016/08/16 (1.095)- Fixed ROM mode for deeper levels of popup windows parents.
- 2016/08/15 (1.094)- Editbox are now still active if enter was pressed with flag
                        `NK_EDIT_SIG_ENTER`. Main reasoning is to be able to keep
                        typing after commiting.
- 2016/08/15 (1.094)- Removed redundant code
- 2016/08/15 (1.094)- Fixed negative numbers in `nk_strtoi` and remove unused variable
- 2016/08/15 (1.093)- Fixed `NK_WINDOW_BACKGROUND` flag behavior to select a background
                        window only as selected by hovering and not by clicking.
- 2016/08/14 (1.092)- Fixed a bug in font atlas which caused wrong loading
                        of glyphes for font with multiple ranges.
- 2016/08/12 (1.091)- Added additional function to check if window is currently
                        hidden and therefore not visible.
- 2016/08/12 (1.091)- nk_window_is_closed now queries the correct flag `NK_WINDOW_CLOSED`
                        instead of the old flag `NK_WINDOW_HIDDEN`
- 2016/08/09 (1.09) - Added additional double version to nk_property and changed
                        the underlying implementation to not cast to float and instead
                        work directly on the given values.
- 2016/08/09 (1.08) - Added additional define to overwrite library internal
                        floating pointer number to string conversion for additional
                        precision.
- 2016/08/09 (1.08) - Added additional define to overwrite library internal
                        string to floating point number conversion for additional
                        precision.
- 2016/08/08 (1.072)- Fixed compiling error without define NK_INCLUDE_FIXED_TYPE
- 2016/08/08 (1.071)- Fixed possible floating point error inside `nk_widget` leading
                        to wrong wiget width calculation which results in widgets falsly
                        becomming tagged as not inside window and cannot be accessed.
- 2016/08/08 (1.07) - Nuklear now differentiates between hiding a window (NK_WINDOW_HIDDEN) and
                        closing a window (NK_WINDOW_CLOSED). A window can be hidden/shown
                        by using `nk_window_show` and closed by either clicking the close
                        icon in a window or by calling `nk_window_close`. Only closed
                        windows get removed at the end of the frame while hidden windows
                        remain.
- 2016/08/08 (1.06) - Added `nk_edit_string_zero_terminated` as a second option to
                        `nk_edit_string` which takes, edits and outputs a '\0' terminated string.
- 2016/08/08 (1.054)- Fixed scrollbar auto hiding behavior
- 2016/08/08 (1.053)- Fixed wrong panel padding selection in `nk_layout_widget_space`
- 2016/08/07 (1.052)- Fixed old bug in dynamic immediate mode layout API, calculating
                        wrong item spacing and panel width.
- 2016/08/07 (1.051)- Hopefully finally fixed combobox popup drawing bug
- 2016/08/07 (1.05) - Split varargs away from NK_INCLUDE_STANDARD_IO into own
                        define NK_INCLUDE_STANDARD_VARARGS to allow more fine
                        grained controlled over library includes.
- 2016/08/06 (1.045)- Changed memset calls to NK_MEMSET
- 2016/08/04 (1.044)- Fixed fast window scaling behavior
- 2016/08/04 (1.043)- Fixed window scaling, movement bug which appears if you
                        move/scale a window and another window is behind it.
                        If you are fast enough then the window behind gets activated
                        and the operation is blocked. I now require activating
                        by hovering only if mouse is not pressed.
- 2016/08/04 (1.042)- Fixed changing fonts
- 2016/08/03 (1.041)- Fixed `NK_WINDOW_BACKGROUND` behavior
- 2016/08/03 (1.04) - Added color parameter to `nk_draw_image`
- 2016/08/03 (1.04) - Added additional window padding style attributes for
                        sub windows (combo, menu, ...)
- 2016/08/03 (1.04) - Added functions to show/hide software cursor
- 2016/08/03 (1.04) - Added `NK_WINDOW_BACKGROUND` flag to force a window
                        to be always in the background of the screen
- 2016/08/03 (1.032)- Removed invalid assert macro for NK_RGB color picker
- 2016/08/01 (1.031)- Added helper macros into header include guard
- 2016/07/29 (1.03) - Moved the window/table pool into the header part to
                        simplify memory management by removing the need to
                        allocate the pool.
- 2016/07/29 (1.03) - Added auto scrollbar hiding window flag which if enabled
                        will hide the window scrollbar after NK_SCROLLBAR_HIDING_TIMEOUT
                        seconds without window interaction. To make it work
                        you have to also set a delta time inside the `nk_context`.
- 2016/07/25 (1.02) - Fixed small panel and panel border drawing bugs
- 2016/07/15 (1.01) - Added software cursor to `nk_style` and `nk_context`
- 2016/07/15 (1.01) - Added const correctness to `nk_buffer_push' data argument
- 2016/07/15 (1.01) - Removed internal font baking API and simplified
                        font atlas memory management by converting pointer
                        arrays for fonts and font configurations to lists.
- 2016/07/15 (1.01) - Changed button API to use context dependend button
                        behavior instead of passing it for every function call.

