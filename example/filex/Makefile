BIN = filex

# Compiler
CC = clang
DCC = gcc

CFLAGS = -std=c99 -pedantic -Wdeprecated-declarations
CFLAGS += -g -Wall -Wextra -Wformat-security -Wunreachable-code
CFLAGS += -fstack-protector-strong -Winline -Wshadow -Wwrite-strings -fstrict-aliasing
CFLAGS += -Wstrict-prototypes -Wold-style-definition -Wconversion -Wfloat-equal
CFLAGS += -Wredundant-decls -Wnested-externs -Wmissing-include-dirs
CFLAGS += -Wshadow  -Wcast-qual -Wcast-align -Wmissing-prototypes -Wconversion
CFLAGS += -Wswitch-default -Wundef -Wno-unused -Wstrict-overflow=5 -Wsign-conversion
CFLAGS += -Winit-self -Wstrict-aliasing -ftrapv -Wpointer-arith
CFLAGS += -Wswitch-enum -Winvalid-pch -Wbad-function-cast -fno-omit-frame-pointer

SRC = filex.c ../../zahnrad.c
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
	$(CC) $(SRC) $(CFLAGS) -D_POSIX_C_SOURCE=200809L -o bin/$(BIN) -lSDL2 -lGL -lm -lGLU -lGLEW
