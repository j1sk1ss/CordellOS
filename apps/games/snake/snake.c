#include "snake.h"


static snake snk[N];
static fruit frt;

static int hardnes_level = 20000000;
static int snake_size    = 0;
static int max_score     = 0;
static int dead          = 0;


int main(int args, char* argv[]) {
	clrscr();

	text_object_t* logo = create_text(get_resolution_x() / 2, get_resolution_y() / 2, "SNAKE GAME!", WHITE);
	window_t* main_menu = create_window("SnakeGameMenu", -1, 0, 0, get_resolution_y(), get_resolution_x(), WHITE, NULL);
	display_window(main_menu);

	printf("\nSNAKE GAME v. 1.0\nPress ENTER to start.\n");
	while (dead == 0) {
        char key = keyboard_wait();
		switch (key) {
			case ENTER_BUTTON:
				dead = 1;
				break;

			case F3_BUTTON:
			return 1;
		}
    }

	char map[V][H];
	
	snake_size 	= 4;
	dead 		= 0;

	begin(map);
	show(map);
	loop(map);
	
	return snake_size - 4;
}

// Initialized in a initial position
void begin(char map[V][H]) {
	int i;
	// snake head
	snk[0].x = 32;
	snk[0].y = 10; // V

	frt.x = srand_r() % (H - 4) + 1; // automatic position of fruit
	frt.y = srand_r() % (V - 4) + 1;

	for (i = 0; i < snake_size; i++) {
		snk[i].movX = 1;
		snk[i].movY = 0;
	} // snake only move on X

	intro(map);
	intro_data(map);
}

// Show everything
void show(char map[V][H]) {
	int i, j;

	for (i = 0; i < V; i++) {
		for (j = 0; j < H; j++)
			printf("%c", map[i][j]);
		
		printf("\n");
	}

	printf("Score: [%i]", snake_size - 4);
	printf("\nMax score: [%i]", max_score);
}

void intro(char map[V][H]) {
	int i, j;

	for (i = 0; i < V; i++) 
		for (j = 0; j < H; j++) 
			if (i == 0 || i == V - 1) map[i][j] = '-';
			else if (j == 0 || j == H - 1) map[i][j] = '|';
			else map[i][j] = ' ';
}

void intro_data(char map[V][H]) {
	int i;
	for (i = 1; i < snake_size; i++) {
		snk[i].x = snk[i - 1].x - 1;
		snk[i].y = snk[i - 1].y;

		snk[i].imagen = 'X';
	}

	snk[0].imagen = 'O';
	for (i = 0; i < snake_size; i++)
		map[snk[i].y][snk[i].x] = snk[i].imagen;
	
	map[frt.y][frt.x] = 'M';
}

void loop(char map[V][H]) {
	do {
		clrscr();

		show(map);
		input(map);
		update(map);

        int delay = hardnes_level;
        while (--delay > 0)
            continue;
	} while (dead == 0);
}

int input(char map[V][H]) {
	int i;
	char key;

	if (snk[0].x == 0 || snk[0].x == H - 1 || snk[0].y == 0 || snk[0].y == V - 1)  
		dead = 1;

	for (i = 1; i < snake_size && dead == 0; i++) 
		if (snk[0].x == snk[i].x && snk[0].y == snk[i].y) 
			dead = 1;

	if (snk[0].x == frt.x && snk[0].y == frt.y) {
		snake_size++;
		if (hardnes_level < 7000000)
			snake_size++;

		if (hardnes_level < 5000000)
			snake_size++;

		snk[snake_size - 1].imagen = 'X';

		frt.x = srand_r() % (H - 2) + 1;
		frt.y = srand_r() % (V - 2) + 1;
	}

	if (dead == 0) {
		key = get_char();

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

	return 1;
}

void update(char map[V][H]) {
	intro(map);
	intro_data2(map);
}

void intro_data2(char map[V][H]) {
	int i = 0;
	for (i = snake_size - 1; i > 0; i--) {
		snk[i].x = snk[i - 1].x;
		snk[i].y = snk[i - 1].y;
	}

	snk[0].x += snk[0].movX;
	snk[0].y += snk[0].movY;

	for (i = 0; i < snake_size; i++) 
		map[snk[i].y][snk[i].x] = snk[i].imagen;
	
	map[frt.y][frt.x] = 'M';
}