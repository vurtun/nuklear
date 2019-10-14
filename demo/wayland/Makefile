WAYLAND=`pkg-config wayland-client --cflags --libs`
CFLAGS?=-std=c11 -Wall -Werror -O3 -fvisibility=hidden

hello_wayland: main.c 
	$(CC) -o demo *.c $(WAYLAND) -lrt -lm


clean:
	$(RM) demo
