#!/bin/sh
set -e
cd opengl
make -f Makefile
cd ../x11/
make -f Makefile
cd ../nanovg/
make -f Makefile
cd ..
