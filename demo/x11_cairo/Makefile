# Install
BIN = demo

# Flags
CFLAGS = -std=c89 -pedantic -O2 -I/usr/include/cairo

SRC = main.c
OBJ = $(SRC:.c=.o)

LIBS = -lcairo -lXext -lXrender -lX11 -lGL -lm

$(BIN):
	@mkdir -p bin
	rm -f bin/$(BIN) $(OBJS)
	$(CC) $(SRC) $(CFLAGS) -o bin/$(BIN) $(LIBS)
