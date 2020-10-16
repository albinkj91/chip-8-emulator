CC=gcc

chip8: chip8.c
	${CC} chip8.c gfx.c -o chip8 -lX11 -lm
