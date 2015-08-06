all:
	gcc -Wall -std=c99 -g -o chip8 chip8.c -I /usr/local/include/SDL/ `sdl2-config --cflags --libs`
