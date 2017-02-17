GLAD_INC = -I C:/Users/Ricky/MinGW-Libs/GLAD/include

SFML_INC = -I C:/Users/Ricky/MinGW-Libs/SFML/include
SFML_LIB = -L C:/Users/Ricky/MinGW-Libs/SFML/lib

LIBS = -DGLAD_HEADER_ONLY -DSFML_STATIC -lmingw32 -lsfml-window-s -lsfml-system-s -lopengl32 -lwinmm -lgdi32


build:
	g++ main.cpp $(GLAD_INC) $(SFML_INC) $(SFML_LIB) $(LIBS) -o demo.exe
