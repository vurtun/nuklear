# Install
BIN = opengl

# Compiler
CC = gcc
DCC = clang

# Flags
CFLAGS = -std=c89 -pedantic-errors -Wno-deprecated-declarations -D_POSIX_C_SOURCE=200809L
CFLAGS += -g -Wall -Wextra -Werror -Wformat -Wunreachable-code
CFLAGS += -Winline -Wshadow -Wwrite-strings -Wno-unused-variable -Wno-unused-parameter -Wno-unused-function
CFLAGS += -Wstrict-prototypes -Wold-style-definition
CFLAGS += -Wredundant-decls -Wnested-externs -Wmissing-include-dirs
CFLAGS += -Wshadow -Wpointer-arith -Wcast-qual -Wmissing-prototypes
CFLAGS += -Wswitch-default -Wundef -Wstrict-overflow=5
CFLAGS += -Winit-self -Wstrict-aliasing -Wunused

SRC = gui.c opengl.c
OBJ = $(SRC:.c=.o)

# Modes
.PHONY: gcc
gcc: CC = gcc
gcc:  CFLAGS += -Wno-unused-local-typedefs
gcc: $(BIN)

.PHONY: clang
clang: CC = clang
clang: $(BIN)

$(BIN):
	@mkdir -p bin
	rm -f bin/$(BIN) $(OBJS)
	$(CC) $(SRC) $(CFLAGS) -o bin/$(BIN) -lX11 -lGL -lGLU

