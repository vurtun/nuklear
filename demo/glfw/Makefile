# Install
BIN = demo

# Compiler
CC = clang
DCC = gcc

# Flags
CFLAGS = -std=c89 -pedantic

SRC = ../../zahnrad.c glfw.c
OBJ = $(SRC:.c=.o)

ifeq ($(OS),Windows_NT)
BIN := $(BIN).exe
LIBS = -lglfw3 -lopengl32 -lm -lGLU32 -lGLEW32
else
LIBS = -lglfw -lGL -lm -lGLU -lGLEW
endif

# Modes
.PHONY: clang
clang: CC = clang
clang: $(BIN)

.PHONY: gcc
gcc: CC = gcc
gcc: $(BIN)

$(BIN):
	@mkdir -p bin
	rm -f bin/$(BIN) $(OBJS)
	$(CC) $(SRC) $(CFLAGS) -o bin/$(BIN) $(LIBS)
