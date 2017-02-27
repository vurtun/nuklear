# Install
BIN = demo

# Flags
CFLAGS = -std=c99 -pedantic -O2

SRC = main.c
OBJ = $(SRC:.c=.o)

ifeq ($(OS),Windows_NT)
BIN := $(BIN).exe
LIBS = -lmingw32 -lSDL2main -lSDL2 -lopengl32 -lm -lGLU32 -lGLEW32
PRE := del bin\$(BIN) || rmdir /s /q bin || mkdir bin || rem
else
	PRE := rm -f bin/$(BIN) $(OBJS) && mkdir -p bin
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		LIBS = -lSDL2 -framework OpenGL -lm -lGLEW
	else
		LIBS = -lSDL2 -lGL -lm -lGLU -lGLEW
	endif
endif

$(BIN):
	$(PRE)
	$(CC) $(SRC) $(CFLAGS) -o bin/$(BIN) $(LIBS)
