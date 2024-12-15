#include "../include/vars.h"


void envar_init_stack(vars_entry_t vars[], size_t count) {
    for (int i = 0; i < count; i++) {
        vars[i].name[0] = '\0';
        vars[i].value[0] = '\0';
    }

#ifdef ENVARS_SAVE
#endif
}

// -1 - nexists
// != -1 - exists
int envar_exist(char* name, vars_entry_t vars[], size_t count) {
    for (int i = 0; i < count; i++) {
        if (!strcmp(name, vars[i].name)) return i;
    }

    return -1;
}

char* envar_get(char* name, char* buffer, vars_entry_t vars[], size_t count) {
    strcpy(buffer, vars[envar_exist(name, vars, count)].value);
    return buffer;
}

int envar_set(char* name, char* value, vars_entry_t vars[], size_t count) {
    int position = envar_exist(name, vars, count);
    if (position == -1) return 0;
    strcpy(vars[position].value, value);
    return 1;
}

int envar_add(char* name, char* value, vars_entry_t vars[], size_t count) {
    int position = envar_exist(name, vars, count);
    if (position != -1) return 0;
    
    for (int i = 0; i < count; i++) {
        if (vars[i].name[0] == '\0' && vars[i].value[0] == '\0') {
            strcpy(vars[i].name, name);
            strcpy(vars[i].value, value);
            return 1;
        }
    }

    return 0;
}

int envar_delete(char* name, vars_entry_t vars[], size_t count) {
    int position = envar_exist(name, vars, count);
    if (position == -1) return 0;
    vars[position].name[0] = '\0';
    vars[position].value[0] = '\0';
    return 1;
}

void envar_save(vars_entry_t vars[], size_t count) {
#ifdef ENVARS_SAVE
#endif
}