#ifndef VARS_H_
#define VARS_H_

#include <stddef.h>
#include <string.h>

#include "vfs.h"


#define VARS_COUNT 100


void VARS_init();
int VARS_exist(char* name);
char* VARS_get(char* name);
void VARS_set(char* name, char* value);
void VARS_add(char* name, char* value);
void VARS_delete(char* name);

char** VARS_get_names();
char** VARS_get_vars();

#endif