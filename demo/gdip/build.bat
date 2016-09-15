@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86

cl /nologo /W3 /O2 /fp:fast /Gm- /Fedemo.exe main.c user32.lib gdiplus.lib /link /incremental:no
