CONTRIBUTING
============
## Submitting changes
Please send a GitHub Pull Request with a clear list of what you've done (read more about [pull requests](http://help.github.com/pull-requests/)). 

## Features
If you have an idea for new features just [open an issue](https://github.com/vurtun/zahnrad/issues) with your suggestion.
  * Find and correct spelling mistakes
  * Add (insert your favorite platform or render backend here) demo implementation
  * Add clipboard user callbacks back into all demos
  * Add additional widgets [some possible widgets](http://doc.qt.io/qt-5/widget-classes.html#the-widget-classes)
  * Extend xlib demo to support image drawing with arbitrary image width and height
  * Change cursor in `zr_widget_edit_box` and `zr_widget_edit_field` to thin standard cursor version used in editors
  * Rewrite the chart API to support a better range of charts (maybe take notes from Javascript chart frameworks) 
  * Create an API to allow scaling between groups (maybe extend and convert the demo example)
  * Add Tab support (maybe use `zr_group` and add a header)  
  * Come up with a better way to provide and create widget and window styles
  * Add support for multiple pointers for touch input devices (probably requires to rewrite mouse handling in `struct zr_input`)
  * Extend context to not only support overlapping windows but tiled windows as well

## Bugs
  * Text pixel width is bugged at the moment if you edit text with enabled font baking and vertex buffer API.
  * Text handling is still a little bit chanky and probably needs to be further tested and polished

## Coding conventions
  * Only use a C89 (ANSI C) compiler or rather style
  * For indent use four spaces
  * Do not typedef structs, unions and enums
  * Variable, object and function names should always be lowercase and use underscores instead of camel case
  * Whitespace after for, while, if, do, switch
  * Always use parentheses if using the sizeof operator (e.g: sizeof(struct zr_context) and not sizeof struct zr_context)
  * Beginning braces on the new line for functions and on the same line otherwise.
  * Only use fixed size types (zr_uint, zr_size, ...) if you really need to and use basic types otherwise
  * Do not include any header files in either zahnrad.h or zahnrad.c
  * Do not add any dependencies
  * Do not use any compiler specific extensions

