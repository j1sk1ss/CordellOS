#include <fslib.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <keyboard.h>
#include <graphics.h>


#define ROWS            14
#define LINE_OFFSEET    4

#define LEFT_OFFSET     20
#define RIGHT_OFFSET    20

#define LINE        "+-------------+--------+----------+----------+-----------+------+-------------+\n"
#define HEADER      "| NAME        | TYPE   | CREATED  | MODIF    | ACCESS    | EXT  | SIZE        |\n"
#define UPPED_DIR   "|     ...     | NONE   | N/A      | N/A      | N/A       | N/A  | N/A         |\n"
#define EMPTY       "|             |        |          |          |           |      |             |\n"

#define EDIT_LINE   "| EDIT                                                                        |\n"
#define VIEW_LINE   "| VIEW                                                                        |\n"
#define RENAME_LINE "| RENAME                                                                      |\n"
#define DELETE_LINE "| DELETE                                                                      |\n"

#define EDIT_POS        0
#define VIEW_POS        1
#define RENAME_POS      2
#define DELETE_POS      3


int main(int argc, char* argv[]);
int loop();
void display_manager();

void open_content();
void file2display(Content* content);