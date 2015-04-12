# GUI
Work in progress

## Features
- Immediate mode graphical user interface
- Written in C89 (ANSI C)
- Small (~2.5kLOC)
- Focus on portability and minimal internal state
- Suited for embedding into graphical applications
- No global hidden state
- No direct dependencies (not even libc!)
- Renderer and platform independent
- Complete memory management control
- Configurable
- UTF-8 supported

## Layer
The gui toolkit consists of three level of abstraction. First the basic widget layer
for a as pure functional as possible set of widgets functions without
any kind of internal state, with the tradeoff off of a lot of boilerplate code.
Second the panel layer for a static grouping of widgets into a panel with a reduced need for
a lot of the boilerplate code but takes away some freedome of widget placing and
introduces the state of the pannel.
Finally there is the context layer which represent the complete window and
enables moveable, scaleable and overlapping panels, but needs complete control
over the panel management and therefore needs the most amount of internal state.
Each higher level level of astraction uses the lower level(s) internally to build
on but offers mostly a different API.

### Widgets

### Panels

### Context

## Memory

## Input

## Output

## Font

## Configuration
While the widgets layer offers the most configuration and even expects you to
configure each and every widget, the upper levels provide you with a basic set of
configurable attributes like color, padding and spacing.

## FAQ

### Is this project stable?
The basic API is stable while the needed backend may change. For example the draw
command backend will probably be changed to support primitives instead of
just using triangles with textures. Which would simplify the library a whole lot
while providing more freedom and control to the user.

### Where is widget X?
A number of basic widgets are provided but some of the more complex widgets like
comboboxes, tables and tree views are not yet implemented. Maybe if I have more
time I will look into adding them, except for comboboxes which are just
discusting to implement.

### Why should I use this library over ImGui
You probably shouldn't since ImGui provides a lot more features, better
support and more polish. That being said ImGui is C++ and not C and
therefore for people who exclusivly only use C not interesting. Furthermore
while ImGui provides a way more bloat free expierence than most classic
non immediate mode GUIs, it still focuses on unneeded global hidden state.
And finally the biggest difference between this project and ImGui is the
flexible selection of abstraction provided by this project. That beeing said I
still think the ImGui team did a great job of providing a good
immedate mode GUI that I don't activily try compete against since the
contributors of ImGUI are a.) alot more contributers and b.) probably have more
programming expierence than I do.

### Why did you use ANSI C and not C99 or C++?
Personally I stay out of all "discussions" about C vs C++ since they are totally
worthless and never brought anything good with it. The simple answer is I
personally love C have nothing against people using C++ exspecially the new
iterations with C++11 and C++14 as long as they stay away from my code.
While this hopefully settles my view on C vs C++ there is still ANSI C vs C99.
While for personal projects I only use C99 with all its niceties, libraries are a
whole different animal. Since libraries are designed to reach the highest number of
users possible. Which brings me to ANSI C which is probably the most portable
programming langauge out there. In addition not all C compiler like the MSVC
compiler fully support C99, which finalized my decision to use
ANSI C.

### Why do you typedef our own types instead of using the standard types
Because this project is in ANSI C which does not have the header file `<stdint.h>`
and therefore does not provide me with fixed size types that I need . Therefore
I defined my own types which need to be set to the correct size for each
plaform. But if your development environment provides the header you can define
`GUI_USE_FIXED_SIZE_TYPES` to directly use the correct types.

### Why do you use Freetype? I thought there are no direct dependencies?
Freefont is no direct dependency and is only used if you specifically import
the library by defining `GUI_USE_FRETYPE_FONT`. The main reason I even added
freetype was to have a basic easy way to test out the library without having to
manage font handling.

## References
- [Tutorial from Jari Komppa about imgui libraries](http://www.johno.se/book/imgui.html)
- [Johannes 'johno' Norneby's article](http://iki.fi/sol/imgui/)
- [Casey Muratori's original introduction to imgui's](http:://mollyrocket.com/861?node=861)
- [Casey Muratori's imgui Panel design 1/2](http://mollyrocket.com/casey/stream_0019.html)
- [Casey Muratori's imgui Panel design 2/2](http://mollyrocket.com/casey/stream_0020.html)
- [ImGui: The inspiration for this project](https://github.com/ocornut/imgui)

# License
    (The MIT License)
