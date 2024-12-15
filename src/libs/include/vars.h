#ifndef VARS_H_
#define VARS_H_

#include <stddef.h>
#include "string.h"
#include "stdio.h"


#define VARS_MAX_NAME   64
#define VARS_MAX_SIZE   128


typedef struct {
    char name[VARS_MAX_NAME];
    char value[VARS_MAX_SIZE];
} vars_entry_t;


void envar_init_stack(vars_entry_t vars[], size_t count);
int envar_exist(char* name, vars_entry_t vars[], size_t count);
char* envar_get(char* name, char* buffer, vars_entry_t vars[], size_t count);
int envar_set(char* name, char* value, vars_entry_t vars[], size_t count);
int envar_add(char* name, char* value, vars_entry_t vars[], size_t count);
int envar_delete(char* name, vars_entry_t vars[], size_t count);
void envar_save(vars_entry_t vars[], size_t count);

#endif