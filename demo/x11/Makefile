# Install
BIN = zahnrad

# Flags
CFLAGS += -std=c89 -pedantic -O2

SRC = main.c
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
	$(CC) $(SRC) $(CFLAGS) -D_POSIX_C_SOURCE=200809L -o bin/$(BIN) -lX11 -lm
