# Install
BIN = demo

# Flags
CFLAGS += -std=c99 -pedantic -O2

SRC = main.c
OBJ = $(SRC:.c=.o)

ifeq ($(OS),Windows_NT)
BIN := $(BIN).exe
LIBS = -lglfw3 -lopengl32 -lm -lGLU32 -lGLEW32
PRE := del bin\$(BIN) || rmdir /s /q bin || mkdir bin || rem
else
	PRE := rm -f bin/$(BIN) $(OBJS) && mkdir -p bin
	UNAME_S := $(shell uname -s)
	GLFW3 := $(shell pkg-config --libs glfw3)
	ifeq ($(UNAME_S),Darwin)
		LIBS := $(GLFW3) -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -lm -lGLEW -L/usr/local/lib
	else
		LIBS = $(GLFW3) -lGL -lm -lGLU -lGLEW
	endif
endif

$(BIN):
	$(PRE)
	$(CC) $(SRC) $(CFLAGS) -o bin/$(BIN) $(LIBS)
