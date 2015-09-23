# Install
BIN = zahnrad

# Compiler
CC = clang
DCC = gcc

CFLAGS = -std=c99

SRC = nanovg.c ../../zahnrad.c
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
