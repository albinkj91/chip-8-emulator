#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "gfx.h" // https://www3.nd.edu/~dthain/courses/cse20211/fall2013/gfx/

#define WIDTH 64
#define HEIGHT 32
#define PIXEL_WIDTH 20
#define MEMORY_SIZE 4096
#define REGISTER_SIZE 16

unsigned char memory[MEMORY_SIZE];
unsigned char v[REGISTER_SIZE];
unsigned short stack[REGISTER_SIZE];
unsigned short i_register = 0;
unsigned short pc = 0x200;
unsigned char sp = 0;
unsigned char sound = 0;
unsigned char delay = 0;

char screen[HEIGHT][WIDTH];
unsigned char keys[] = {'x', '1', '2', '3', 'q', 'w', 'e', 'a', 's', 'd', 'z', 'c', '4', 'r', 'f', 'v'};

unsigned char sprites[] = {
	// 0
	0xf0,
	0x90,
	0x90,
	0x90,
	0xf0,

	// 1
	0x20,
	0x60,
	0x20,
	0x20,
	0x70,

	// 2
	0xf0,
	0x10,
	0xf0,
	0x80,
	0xf0,

	// 3
	0xf0,
	0x10,
	0xf0,
	0x10,
	0xf0,

	// 4
	0x90,
	0x90,
	0xf0,
	0x10,
	0x10,

	// 5
	0xf0,
	0x80,
	0xf0,
	0x10,
	0xf0,

	// 6
	0xf0,
	0x80,
	0xf0,
	0x90,
	0xf0,

	// 7
	0xf0,
	0x10,
	0x20,
	0x40,
	0x40,

	// 8
	0xf0,
	0x90,
	0xf0,
	0x90,
	0xf0,

	// 9
	0xf0,
	0x90,
	0xf0,
	0x10,
	0xf0,

	// A
	0xf0,
	0x90,
	0xf0,
	0x90,
	0x90,

	// B
	0xe0,
	0x90,
	0xe0,
	0x90,
	0xe0,

	// C
	0xf0,
	0x80,
	0x80,
	0x80,
	0xf0,

	// D
	0xe0,
	0x90,
	0x90,
	0x90,
	0xe0,

	// E
	0xf0,
	0x80,
	0xf0,
	0x80,
	0xf0,

	// F
	0xf0,
	0x80,
	0xf0,
	0x80,
	0x80
};
	
void load_sprites();
void load_program(char *path);
void execute_op_code(unsigned short instruction);
void init_graphics();
void render();
void draw_bytes(unsigned char x, unsigned char y, unsigned char byte_count);
void clear_screen();
unsigned char pixels_to_byte(unsigned char x, unsigned char y);


int main(int argc, char *argv[]){
	if(argc < 2){
		printf("ERROR::NEED_TO_INCLUDE_PATH\n\n");
		printf("Type the path to the ROM\n./chip8 [path_to_rom]\n");
		exit(0);
	}

	init_graphics();
	load_sprites();
	load_program(argv[1]);
	srand((unsigned int) time(NULL));
	
	clock_t start = clock();
	double freq = 1.0 / 60.0;

	while(1){
		unsigned short instruction = memory[pc] << 8;
		instruction |= memory[pc+1];
		execute_op_code(instruction);

		clock_t now = clock();
		double diff = (double)(now - start) / CLOCKS_PER_SEC;

		if(diff >= freq){
			if(delay > 0){
				delay--;
			}
			if(sound > 0){
				sound--;
			}
			start = clock();
		}
		//gfx_wait();
	}
	return 0;
}

void load_sprites(){
	for(int i = 0; i < (5 * 16); i++){
		memory[i] = sprites[i];
	}
}

void load_program(char *path){
	int index = 0x200;
	FILE *fd = fopen(path, "r");
	if(fd < 0){
		printf("ERROR::ROM_NOT_FOUND\n");
		exit(0);
	}

	int result = 1;
	while(result > 0){
		result = fread(&memory[index++], sizeof(char), 1, fd);
	}
}

void init_graphics(){
	gfx_open(WIDTH * PIXEL_WIDTH, HEIGHT * PIXEL_WIDTH, "CHIP-8 Emulator");
}

void render(){
	for(int i = 0; i < HEIGHT; i++){
		for(int j = 0; j < WIDTH; j++){
			if(screen[i][j]){
				gfx_color(0xff, 0xff, 0xff);
				gfx_fill_rect(j * PIXEL_WIDTH, i * PIXEL_WIDTH, PIXEL_WIDTH, PIXEL_WIDTH);
			}else{
				gfx_color(0x0, 0x0, 0x0);
				gfx_fill_rect(j * PIXEL_WIDTH, i * PIXEL_WIDTH, PIXEL_WIDTH, PIXEL_WIDTH);
			}
		}
	}
	gfx_flush();
}

void draw_bytes(unsigned char x, unsigned char y, unsigned char byte_count){
	v[0xf] = 0;
	for(int i = 0; i < byte_count; i++){
		screen[y+i][x]	 ^=	 (memory[i+i_register] & 0xf0)		   >> 7;
		screen[y+i][x+1] ^= ((memory[i+i_register] & 0xf0) & 0x40) >> 6;
		screen[y+i][x+2] ^= ((memory[i+i_register] & 0xf0) & 0x20) >> 5;
		screen[y+i][x+3] ^= ((memory[i+i_register] & 0xf0) & 0x10) >> 4;
		screen[y+i][x+4] ^= ((memory[i+i_register] & 0x0f) & 0x8)  >> 3;
		screen[y+i][x+5] ^= ((memory[i+i_register] & 0x0f) & 0x4)  >> 2;
		screen[y+i][x+6] ^= ((memory[i+i_register] & 0x0f) & 0x2)  >> 1;
		screen[y+i][x+7] ^= ((memory[i+i_register] & 0x0f) & 0x1);

		if(memory[i+i_register] ^ pixels_to_byte(x, y+i)){
			v[0xf] = 1;
		}
	}
	render();
}

unsigned char pixels_to_byte(unsigned char x, unsigned char y){
	unsigned char byte = 0x0;
	for(int i = 0; i < 8; i++){
		byte |= (screen[y][x+i] << (7 - i));
	}
	return byte;
}

void clear_screen(){
	for(int i = 0; i < HEIGHT; i++){
		for(int j = 0; j < WIDTH; j++){
			screen[i][j] = 0;
		}
	}
}

void execute_op_code(unsigned short instruction){
	unsigned char higher = instruction >> 8;
	unsigned char lower = instruction & 0x00ff;
	unsigned short opCode = instruction & 0xf000;
	unsigned short nnn = instruction & 0x0fff;
	unsigned char x = higher & 0x0f;
	unsigned char y = lower >> 4;

	/*printf("Instruction: %x\n", instruction);
	printf("Higher: %x\n", higher);
	printf("Lower: %x\n", lower);
	printf("OP Code: %x\n", opCode);
	printf("NNN: %x\n", nnn);
	printf("X: %x\n", x);
	printf("Y: %x\n", y);
	printf("I: %x\n\n", i_register);

	for(int i = 0; i < 0x10; i++){
		printf("v[%d] = %x   ", i, v[i]);
	}
	printf("\n\n\n");*/

	switch(opCode){
		case 0x0000:
			if(lower == 0xe0){
				clear_screen();
				gfx_clear();
				printf("Clearing screen\n");
			}else if(lower == 0xee){
				pc = stack[sp--];
			}else{
				printf("0x0000 OP code not implemented\n");
			}
			pc += 2;
			break;
		case 0x1000:
			pc = nnn;
			break;
		case 0x2000:
			stack[++sp] = pc;
			pc = nnn; 
			break;
		case 0x3000:
			if(v[x] == lower){
				pc += 2;
			}
			pc += 2;
			break;
		case 0x4000:
			if(v[x] != lower){
				pc += 2;
			}
			pc += 2;
			break;
		case 0x5000:
			if(v[x] == v[y]){
				pc += 2;
			}
			pc += 2;
			break;
		case 0x6000:
			v[x] = lower;
			pc += 2;
			break;
		case 0x7000:
			v[x] = v[x] + lower;
			pc += 2;
			break;
		case 0x8000: ; //Semicolon needed since label must be followed by a statement
			unsigned char lowestNibble = lower & 0x0F;
			switch(lowestNibble){
				case 0x0:
					v[x] = v[y];
					break;
				case 0x1:
					v[x] = v[x] | v[y];
					break;
				case 0x2:
					v[x] = v[x] & v[y];
					break;
				case 0x3:
					v[x] = v[x] ^ v[y];
					break;
				case 0x4:
					v[x] = v[x] + v[y];
					unsigned short carry = v[x] + v[y];
					v[0xf] = carry > 0xff ? 1 : 0;
					break;
				case 0x5:
					v[0xf] = v[x] > v[y] ? 1 : 0;
					v[x] = v[x] - v[y];
					break;
				case 0x6:
					v[0xf] = v[x] & 1 ? 1 : 0;
					v[x] >>= 1;
					break;
				case 0x7:
					v[0xf] = v[y] > v[x] ? 1 : 0;
					v[x] = v[y] - v[x];
					break;
				case 0xe:
					v[0xf] = (v[x] >> 7) & 1 ? 1 : 0;
					v[x] <<= 1;
					break;
			}
			pc += 2;
			break;
		case 0x9000:
			if(v[x] != v[y]){
				pc += 2;
			}
			pc += 2;
			break;
		case 0xa000:
			i_register = nnn;
			pc += 2;
			break;
		case 0xb000:
			pc = nnn + v[0x0];
			break;
		case 0xc000:
			v[x] = rand() & lower;
			pc += 2;
			break;
		case 0xd000: ;
			// TODO - Out of bounds
			unsigned char n = lower & 0xf;
			draw_bytes(v[x], v[y], n);
			pc += 2;
			break;
		case 0xe000:
			if(!gfx_event_waiting()){
				pc += 2;
				break;
			}
			char pressed_key = gfx_wait();
			if(lower == 0x9e){
				if(pressed_key == keys[v[x]]){
					pc += 2;
				}
			}else if(lower == 0xa1){
				if(pressed_key != keys[v[x]]){
					pc += 2;
				}
			}
			pc += 2;
			break;
		case 0xf000:
			switch(lower){
				case 0x07:
					v[x] = delay;
					break;
				case 0x0A: ;
					unsigned char key_pressed = gfx_wait();
					for(int i = 0; i <= 0xf; i++){
						if(key_pressed == keys[i]){
							key_pressed = i;
							break;
						}
					}

					if(key_pressed > 0xf){
						printf("ERROR::INVALID_KEY_PRESS\n");
					}else{
						v[x] = key_pressed;
					}
					break;
				case 0x15:
					delay = v[x];
					break;
				case 0x18:
					sound = v[x];
					break;
				case 0x1e:
					i_register += v[x];
					break;
				case 0x29:
					i_register = v[x] * 5;
					break;
				case 0x33:;
					unsigned char *decimal = malloc(2);
					sprintf(decimal, "%03d", v[x]);
					memory[i_register] = (unsigned char) (decimal[0] - '0');
					memory[i_register+1] = (unsigned char) (decimal[1] - '0');
					memory[i_register+2] = (unsigned char) (decimal[2] - '0');
					free(decimal);
					break;
				case 0x55:
					for(int i = 0; i <= x; i++){
						memory[i+i_register] = v[i];
					}
					break;
				case 0x65:
					for(int i = 0; i <= x; i++){
						v[i] = memory[i+i_register];
					}
					break;
			}
			pc += 2;
			break;
	}
}
