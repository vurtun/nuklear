OpenGL ES
=========

OpenGL ES (OpenGL for Embedded Systems) it is a subset of OpenGL aimed to be simple and fast. It is designed for embedded systems, so for example, this demo could be a good point to start an Android application.


Linux, Mac OS X
---------------
You can develop under your favorite modern Linux distro. Most of them support Open GL ES out of the box. But remember that implementation could be different and you have to test your application on the end device too.

Just use `make` to build normal Linux / Mac OS X version.


Emscripten
----------
Some other demos could be compiled into WebGL too. But the point of this demo is to be compiled without overhead and compatibility mode.

`make web` to build a web-version using Emscripten.


Raspberry Pi
------------

Accelerated Open GL ES2 output supported for Raspberry Pi too. But SDL2 in default Raspbian repositories is not supporting `rpi` video driver by default, so you have to compile SDL2 yourself.

It is better to compile SDL2 without X11 support (*--disable-video-x11* configure option). Or use `export SDL_VIDEODRIVER=rpi` before each run of the application.

More info can be found here: https://github.com/vurtun/nuklear/issues/748

`make rpi` to build the demo on your Raspberry Pi board.
