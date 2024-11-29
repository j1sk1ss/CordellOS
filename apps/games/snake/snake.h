#include <stdio.h>
#include <rand.h>
#include <fslib.h>
#include <string.h>
#include <stdlib.h>
#include <keyboard.h>
#include <graphics.h>
#include <window.h>


#define V 21
#define H 75
#define N 100

#define V 21
#define H 75
#define N 100


typedef struct {
	int x, y;
	int movX, movY;
	char imagen;
} snake;

typedef struct {
	int x, y;
} fruit;


int main(int args, char* argv[]);
void begin(char map[V][H]);
void show(char map[V][H]);
void intro(char map[V][H]);
void intro_data(char map[V][H]);
void loop(char map[V][H]);
int input(char map[V][H]);
void update(char map[V][H]);
void intro_data2(char map[V][H]);
