# Install
BIN = demo

# Flags
CFLAGS = -std=c99 -pedantic -O2

SRC = main.c
OBJ = $(SRC:.c=.o)

ifeq ($(OS),Windows_NT)
BIN := $(BIN).exe
LIBS = -lglfw3 -lopengl32 -lm -lGLU32
PRE := del bin\$(BIN) || rmdir /s /q bin || mkdir bin || rem
else
	PRE := rm -f bin/$(BIN) $(OBJS) && mkdir -p bin
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		LIBS := -lglfw3 -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -lm -lGLEW -L/usr/local/lib
	else
		LIBS = -lglfw -lGL -lm -lGLU
	endif
endif

$(BIN):
	$(PRE)
	$(CC) $(SRC) $(CFLAGS) -o bin/$(BIN) $(LIBS)
