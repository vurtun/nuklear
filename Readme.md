# GUI

WORK IN PROGRESS: I do not garantee that everything works right now

## Features
- Immediate mode graphical user interface toolkit
- Written in C89 (ANSI C)
- Small (~2.5kLOC)
- Focus on portability and minimal internal state
- Suited for embedding into graphical applications
- No global hidden state
- No direct dependencies (not even libc!)
- Renderer and platform independent
- Configurable
- UTF-8 supported

## Limitations
- Does NOT provide platform independent window management
- Does NOT provide platform independent input handling
- Does NOT provide a renderer backend
- Does NOT implement a font library  
Summary: It is only responsible for the actual user interface

## Layer
The gui toolkit consists of three level of abstraction. First the basic widget layer
for a as pure functional as possible set of widgets functions without
any kind of internal state, with the tradeoff off of a lot of boilerplate code.
Second the panel layer for a static grouping of widgets into a panel with a reduced need for
a lot of the boilerplate code but takes away some freedom of widget placing and
introduces the state of the panel.
Finally there is the context layer which represent the complete window and
enables moveable, scaleable and overlapping panels, but needs complete control
over the panel management and therefore needs the most amount of internal state.
Each higher level of abstraction uses the lower level(s) internally to build
on but offers a little bit different API.

## Configuration
The gui toolkit provides a number of different attributes that can be
configured, like spacing, padding, size and color.
While the widget API even expects you to provide the configuration
for each and every widget the higher layers provide you with a set of
attributes in the `gui_config` structure. The structure either needs to be
filled by the user or can be setup with some default values by the function
`gui_default_config`. Modification on the fly to the `gui_config` is in
true immedate mode fashion possible and supported.

## FAQ
#### Where is Widget X?
A number of basic widgets are provided but some of the more complex widgets like
comboboxes, tables and trees are not yet implemented. Maybe if I have more
time I will look into adding them, except for comboboxes which are just
really hard to implement.

#### Why did you use ANSI C and not C99 or C++?
Personally I stay out of all "discussions" about C vs C++ since they are totally
worthless and never brought anything good with it. The simple answer is I
personally love C and have nothing against people using C++ exspecially the new
iterations with C++11 and C++14.
While this hopefully settles my view on C vs C++ there is still ANSI C vs C99.
While for personal projects I only use C99 with all its niceties, libraries are
a little bit different. Libraries are designed to reach the highest number of
users possible which brings me to ANSI C which is the most portable version.
In addition not all C compiler like the MSVC
compiler fully support C99, which finalized my decision to use ANSI C.

#### Why do you typedef our own types instead of using the standard types
This Project uses ANSI C which does not have the header file `<stdint.h>`
and therefore does not provide the fixed sized types that I need. Therefore
I defined my own types which need to be set to the correct size for each
plaform. But if your development environment provides the header file you can define
`GUI_USE_FIXED_SIZE_TYPES` to directly use the correct types.

## References
- [Tutorial from Jari Komppa about imgui libraries](http://www.johno.se/book/imgui.html)
- [Johannes 'johno' Norneby's article](http://iki.fi/sol/imgui/)
- [Casey Muratori's original introduction to imgui's](http:://mollyrocket.com/861?node=861)
- [Casey Muratori's imgui panel design 1/2](http://mollyrocket.com/casey/stream_0019.html)
- [Casey Muratori's imgui panel design 2/2](http://mollyrocket.com/casey/stream_0020.html)
- [ImGui: The inspiration for this project](https://github.com/ocornut/imgui)

# License
    (The MIT License)
