# Flags
CFLAGS += -std=c99 -pedantic -O2
LIBS :=

ifeq ($(OS),Windows_NT)
BIN := $(BIN).exe
	LIBS := -lglfw3 -lopengl32 -lm -lGLU32 -lGLEW32
else
	UNAME_S := $(shell uname -s)
	GLFW3 := $(shell pkg-config --libs glfw3)
	ifeq ($(UNAME_S),Darwin)
		LIBS := $(GLFW3) -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -lm -lGLEW -L/usr/local/lib
		CFLAGS += -I/usr/local/include
	else
		LIBS := $(GLFW3) -lGL -lm -lGLU -lGLEW
	endif
endif

all: generate file_browser extended canvas skinning

generate: clean
ifeq ($(OS),Windows_NT)
	@mkdir bin 2> nul || exit 0
else
	@mkdir -p bin	
endif

clean:
	@rm -rf bin

file_browser: generate
	$(CC) $(CFLAGS) -o bin/file_browser file_browser.c $(LIBS)

extended: generate
	$(CC) $(CFLAGS) -o bin/extended extended.c $(LIBS)

canvas: generate
	$(CC) $(CFLAGS) -o bin/canvas canvas.c $(LIBS)

skinning: generate
	$(CC) $(CFLAGS) -o bin/skinning skinning.c $(LIBS)

