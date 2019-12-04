# Install
BIN = demo

# Flags
CFLAGS += -std=c99 -pedantic -O2

SRC = main.c
OBJ = $(SRC:.c=.o)

# TODO: Handle Windows build
#ifeq ($(OS),Windows_NT)
#BIN := $(BIN).exe
#LIBS = -lglfw3 -lopengl32 -lm -lGLU32 -lGLEW32
#PRE := del bin\$(BIN) || rmdir /s /q bin || mkdir bin || rem
#else
LIBS = -lallegro -lallegro_main -lallegro_image -lallegro_font \
	-lallegro_ttf -lallegro_primitives -lm
PRE := rm -f bin/$(BIN) $(OBJS) && mkdir -p bin
#endif

$(BIN):
	$(PRE)
	$(CC) $(SRC) $(CFLAGS) -o bin/$(BIN) $(LIBS)
