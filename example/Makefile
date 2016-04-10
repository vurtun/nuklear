# Compiler
CC = clang

# Flags
CFLAGS = -std=c99 -pedantic -O2

ifeq ($(OS),Windows_NT)
BIN := $(BIN).exe
LIBS = -lglfw3 -lopengl32 -lm -lGLU32 -lGLEW32
else
LIBS = -lglfw -lGL -lm -lGLU -lGLEW
endif

all: node_editor file_browser overview extended
node_editor:
	@mkdir -p bin
	rm -f bin/node_editor $(OBJS)
	$(CC) $(CFLAGS) -o bin/node_editor node_editor.c $(LIBS)

file_browser:
	@mkdir -p bin
	rm -f bin/file_browser $(OBJS)
	$(CC) $(CFLAGS) -o bin/file_browser file_browser.c $(LIBS)

overview:
	@mkdir -p bin
	rm -f bin/overview $(OBJS)
	$(CC) $(CFLAGS) -o bin/overview overview.c $(LIBS)

extended:
	@mkdir -p bin
	rm -f bin/extended $(OBJS)
	$(CC) $(CFLAGS) -o bin/extended extended.c $(LIBS)

