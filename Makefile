CFLAGS= -Wall -Wextra -std=c11  -Wpedantic -ggdb `pkg-config --cflags sdl2`
LIBS= `pkg-config --libs sdl2` -lm -lSDL2_image -lSDL2_ttf


ion: main.c
	$(CC) $(CFLAGS) -o ion main.c la.c $(LIBS)
