# Install
BIN = doc

# Flags
CFLAGS += -std=c99 -pedantic -O2

SRC = stddoc.c
OBJ = $(SRC:.c=.o)

ifeq ($(OS),Windows_NT)
BIN := $(BIN).exe
LIBS =
else
	UNAME_S := $(shell uname -s)
	ifeq ($(UNAME_S),Darwin)
		LIBS = 
	else
		LIBS =
	endif
endif

$(BIN):
	rm -f $(BIN) $(OBJS)
	$(CC) $(SRC) $(CFLAGS) -o $(BIN)
