# Install
CC = g++
BIN = demo

# Flags
CFLAGS += -s -O2

SRC = main.cpp
OBJ = $(SRC:.cpp=.o)

ifeq ($(OS),Windows_NT)
	# Edit the line below to point to your SFML folder on Windows
	SFML_DIR = C:/Users/Ricky/MinGW-Libs/SFML

	BIN := $(BIN).exe
	LIBS = -lmingw32 -DSFML_STATIC -lsfml-window-s -lsfml-system-s -lopengl32 -lwinmm -lgdi32
else
	# Edit the line below to point to your SFML folder on Linux/MacOS
	SFML_DIR = /home/ricky/Libraries/SFML

	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		LIBS = -lsfml-window -lsfml-system -pthread -framework OpenGL
	else
		LIBS = -DSFML_STATIC -lsfml-window-s -lsfml-system-s -pthread -ludev -lGL -lX11 -lXrandr
	endif
endif

SFML_INC = -I $(SFML_DIR)/include
SFML_LIB = -L $(SFML_DIR)/lib

$(BIN):
	$(CC) $(SRC) $(CFLAGS) -o $(BIN) $(SFML_INC) $(SFML_LIB) $(LIBS)
