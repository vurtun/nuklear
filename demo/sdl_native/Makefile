BIN = main
CFLAGS += -std=c99 -Wall -pedantic -O0
SRC = main.c
LIBS = -lSDL2 -lSDL2_ttf -lSDL2_gfx -lm

$(BIN): Makefile main.c nuklear_sdl.h
	rm -f $(BIN)
	$(CC) $(SRC) $(CFLAGS) -o $(BIN) $(LIBS)

clean:
	rm -f $(BIN)