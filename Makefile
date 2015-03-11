# Install
BIN = opengl

# Compiler
CC = gcc
DCC = clang

# Flags
CFLAGS = -std=c89 -pedantic-errors -Wdeprecated-declarations -D_POSIX_C_SOURCE=200809L
CFLAGS = -g -Wextra -Werror -Wformat -Wunreachable-code
CFLAGS += -fstack-protector-strong -Winline -Wshadow -Wwrite-strings -fstrict-aliasing
CFLAGS += -Wstrict-prototypes -Wold-style-definition -Wconversion
CFLAGS += -Wredundant-decls -Wnested-externs -Wmissing-include-dirs
CFLAGS += -Wshadow -Wpointer-arith -Wcast-qual -Wcast-align -Wmissing-prototypes -Wconversion
CFLAGS += -Wswitch-default -Wundef -Wno-unused -Wstrict-overflow=5 -Wsign-conversion
CFLAGS += -Winit-self -Wstrict-aliasing -fsanitize=address -fsanitize=undefined

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
	$(CC) $(SRC) $(CFLAGS) -o bin/$(BIN) -lX11 -lGL -lGLU

