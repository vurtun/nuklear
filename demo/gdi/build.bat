@echo off

rem This will use VS2015 for compiler
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86

cl /nologo /W3 /O2 /fp:fast /Gm- /Fedemo.exe main.c user32.lib gdi32.lib Msimg32.lib /link /incremental:no
