#include "../../../libs/include/syscall.h"
#include "../../../libs/include/rand.h"

#define F4_BUTTON               '\254'
#define F3_BUTTON               '\255'
#define F2_BUTTON               '\7'
#define F1_BUTTON               '\6'

#define UP_ARROW_BUTTON         '\4'
#define DOWN_ARROW_BUTTON       '\3'
#define LEFT_ARROW_BUTTON       '\1'
#define RIGHT_ARROW_BUTTON      '\2'

#define ENTER_BUTTON            '\n'
#define BACKSPACE_BUTTON        '\b'

#define V 21
#define H 75
#define N 100

#define V 21
#define H 75
#define N 100

int main(void) {
    const char data[50] = "SNAKE GAME ver 0.1\nPress ENTER to start game\n\0";
    SYS_print(data);
    
    while (1) {
        char key = SYS_keyboard_wait_key();
		switch (key) {
			case ENTER_BUTTON:
				snake_start();
				return -1;

			case F3_BUTTON:
			return -1;
		}
    }

	return -1;
}

typedef struct {
	int x, y;
	int movX, movY;

	char imagen;
} snake;

typedef struct {
	int x, y;
} fruit;

int snake_start() {
    int snake_size = 4;
    int dead       = 0;

    char map[V][H];
    snake snk[N];
    fruit frt;

    begin(map, snk, &frt, snake_size);
	show(map, snake_size);
	loop(map, dead, snake_size, snk, &frt);

	return snake_size - 4;
}

void begin(char map[V][H], snake* snk, fruit* frt, int size) {
	int i;

	snk[0].x = 32;
	snk[0].y = 10;

	frt->x = srand_r() % (H - 4) + 1;
	frt->y = srand_r() % (V - 4) + 1;

	for (i = 0; i < size; i++) {
		snk[i].movX = 1;
		snk[i].movY = 0;
	}

	intro(map);
	intro_data(map, snk, size, &frt);
}

void show(char map[V][H], int size) {
	int i, j;

	for (i = 0; i < V; i++) {
		for (j = 0; j < H; j++) 
			SYS_putc(map[i][j]);
		
		SYS_putc('\n');
	}

	char message[25] = "Score: ";
	char score = (size - 4) + '0';
	SYS_print(message);
	SYS_putc(score);
}

void intro(char map[V][H]) {
	int i, j;

	for (i = 0; i < V; i++) 
		for (j = 0; j < H; j++) 
			if (i == 0 || i == V - 1) map[i][j] = '-';
			else if (j == 0 || j == H - 1) map[i][j] = '|';
			else map[i][j] = ' ';
}

void intro_data(char map[V][H], snake* snk, int size, fruit* frt) {
	int i;
	for (i = 1; i < size; i++) {
		snk[i].x = snk[i - 1].x - 1;
		snk[i].y = snk[i - 1].y;

		snk[i].imagen = 'X'; // body
	}

	snk[0].imagen = 'O'; // head
	for (i = 0; i < size; i++)
		map[snk[i].y][snk[i].x] = snk[i].imagen;
	
	map[frt->y][frt->x] = 'M'; // fruit
}

int loop(char map[V][H], int dead, int size, snake* snk, fruit* frt) {
	do {
		SYS_clrs();

		show(map, size);
		if (input(map, dead, snk, size, &frt) == -1) return -1;
		update(map, snk, size, &frt);

        SYS_sleep(100);
	} while (dead == 0);
}

int input(char map[V][H], int dead, snake* snk, int size, fruit* frt) {
	int i;
	char key;

	if (snk[0].x == 0 || snk[0].x == H - 1 || snk[0].y == 0 || snk[0].y == V - 1) {
		dead = 1;
		return -1;
	}

	for (i = 1; i < size && dead == 0; i++) 
		if (snk[0].x == snk[i].x && snk[0].y == snk[i].y) {
			dead = 1;
			return -1;
		}

	if (snk[0].x == frt->x && snk[0].y == frt->y) {
		size++;

		snk[size - 1].imagen = 'X';

		frt->x = srand_r() % (H - 2) + 1;
		frt->y = srand_r() % (V - 2) + 1;
	}

	if (dead == 0) {
        key = SYS_keyboard_get_key();
		if (key == 's' && snk[0].movY != -1) {
			snk[0].movX = 0;
			snk[0].movY = 1;
		}

		if (key == 'w' && snk[0].movY != 1) {
			snk[0].movX = 0;
			snk[0].movY = -1;
		}

		if (key == 'a' && snk[0].movX != 1) {
			snk[0].movX = -1;
			snk[0].movY = 0;
		}

		if (key == 'd' && snk[0].movX != -1) {
			snk[0].movX = 1;
			snk[0].movY = 0;
		}

		if (key == F3_BUTTON) {
			dead = 1;
			return -1;
		}
	}
}

void update(char map[V][H], snake* snk, int size, fruit* frt) {
	intro(map);
	intro_data2(map, snk, size, &frt);
}

void intro_data2(char map[V][H], snake* snk, int size, fruit* frt) {
	int i;

	for (i = size - 1; i > 0; i--) { // 0 is the head. so we going decresing until extremities
		snk[i].x = snk[i - 1].x;
		snk[i].y = snk[i - 1].y;
	}

	snk[0].x += snk[0].movX; // plus movemenrts
	snk[0].y += snk[0].movY;

	// now introduce the values in the camp

	for (i = 0; i < size; i++) 
		map[snk[i].y][snk[i].x] = snk[i].imagen;
	
	map[frt->y][frt->x] = 'M';
}