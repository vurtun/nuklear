# Install
BIN = demo

# Flags
CFLAGS += -std=c99 -pedantic -O2

SRC = main.c
OBJ = $(SRC:.c=.o)

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	LIBS = -lSDL2 -framework OpenGLES -lm
else
	LIBS = -lSDL2 -lGLESv2 -lm
endif

$(BIN): prepare
	$(CC) $(SRC) $(CFLAGS) -o bin/$(BIN) $(LIBS)

web: prepare
	emcc $(SRC) -Os -s USE_SDL=2 -o bin/index.html

rpi: prepare
	$(CC) $(SRC) $(CFLAGS) -o bin/$(BIN) `PKG_CONFIG_PATH=/opt/vc/lib/pkgconfig/ pkg-config --cflags --libs bcm_host brcmglesv2` `/usr/local/bin/sdl2-config --libs --cflags`

prepare:
	@mkdir -p bin
	rm -f bin/$(BIN) $(OBJS)
