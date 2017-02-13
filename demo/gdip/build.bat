@echo off

call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86

cl /nologo /W3 /O2 /fp:fast /Gm- /Fedemo.exe /D_CRT_SECURE_NO_DEPRECATE main.c user32.lib gdiplus.lib shlwapi.lib /link /incremental:no
