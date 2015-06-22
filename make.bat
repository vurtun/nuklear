@echo off
IF NOT EXIST bin mkdir bin
pushd bin
CL ..\demo\win32.c ..\gui.c /Fegui /GS /analyze- /W4 /ZI /Gm /Od /Gd /MDd /EHa- /GA /FC /Oi /GR- /Gm- /EHsc /nologo /wd4127 /wd4201 /wd4189 /wd4100 /wd4505 /D_CRT_SECURE_NO_WARNINGS /link /DEBUG /MACHINE:X86 /INCREMENTAL /SUBSYSTEM:WINDOWS /DYNAMICBASE "kernel32.lib" "user32.lib" "gdi32.lib" 
popd
