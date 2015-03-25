# Install
BIN = opengl

# Compiler
CC = gcc
DCC = clang

# Flags
CFLAGS = -std=c89 -pedantic -Wdeprecated-declarations
CFLAGS += -g -Wall -Wextra -Wformat-security -Wunreachable-code
CFLAGS += -fstack-protector-strong -Winline -Wshadow -Wwrite-strings -fstrict-aliasing
CFLAGS += -Wstrict-prototypes -Wold-style-definition -Wconversion -Wfloat-equal
CFLAGS += -Wredundant-decls -Wnested-externs -Wmissing-include-dirs
CFLAGS += -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wmissing-prototypes -Wconversion
CFLAGS += -Wswitch-default -Wundef -Wno-unused -Wstrict-overflow=5 -Wsign-conversion
CFLAGS += -Winit-self -Wstrict-aliasing -fsanitize=address -fsanitize=undefined -ftrapv
CFLAGS += -Wswitch-enum -Winvalid-pch -Wbad-function-cast

SRC = gui.c opengl.c
OBJ = $(SRC:.c=.o)

# Modes
.PHONY: gcc
gcc: CC = gcc
gcc: $(BIN)

.PHONY: clang
clang: CC = clang
clang: $(BIN)

$(BIN):
	@mkdir -p bin
	rm -f bin/$(BIN) $(OBJS)
	$(CC) $(SRC) $(CFLAGS) -o bin/$(BIN) -lSDL2 -lGL -lGLU

