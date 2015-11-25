# Install
BIN = demo

# Compiler
CC = clang
DCC = gcc

# Flags
CFLAGS = -std=c89 -pedantic

SRC = ../../zahnrad.c sdl.c
OBJ = $(SRC:.c=.o)

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
	$(CC) $(SRC) $(CFLAGS) -o bin/$(BIN) -lSDL2 -lGL -lm -lGLU -lGLEW
