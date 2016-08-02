@echo off

rem This will use VS2015 for compiler
call "C:\Program Files (x86)\Microsoft Visual Studio 14.0\VC\vcvarsall.bat" x86

fxc.exe /nologo /T vs_4_0_level_9_0 /E vs /O3 /Zpc /Ges /Fh nuklear_d3d11_vertex_shader.h /Vn nk_d3d11_vertex_shader /Qstrip_reflect /Qstrip_debug /Qstrip_priv nuklear_d3d11.hlsl
fxc.exe /nologo /T ps_4_0_level_9_0 /E ps /O3 /Zpc /Ges /Fh nuklear_d3d11_pixel_shader.h /Vn nk_d3d11_pixel_shader /Qstrip_reflect /Qstrip_debug /Qstrip_priv nuklear_d3d11.hlsl

cl /D_CRT_SECURE_NO_DEPRECATE /nologo /W3 /O2 /fp:fast /Gm- /Fedemo.exe main.c user32.lib dxguid.lib d3d11.lib /link /incremental:no
