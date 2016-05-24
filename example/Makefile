# Flags
CFLAGS = -std=c99 -pedantic -O2

ifeq ($(OS),Windows_NT)
BIN := $(BIN).exe
LIBS = -lglfw3 -lopengl32 -lm -lGLU32 -lGLEW32
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		LIBS = -lglfw3 -framework OpenGL -lm -lGLEW -L/usr/local/lib
		CFLAGS += -I/usr/local/include
	else
		LIBS = -lglfw -lGL -lm -lGLU -lGLEW
	endif
endif

all: file_browser extended
file_browser:
	@mkdir -p bin
	rm -f bin/file_browser $(OBJS)
	$(CC) $(CFLAGS) -o bin/file_browser file_browser.c $(LIBS)

extended:
	@mkdir -p bin
	rm -f bin/extended $(OBJS)
	$(CC) $(CFLAGS) -o bin/extended extended.c $(LIBS)

