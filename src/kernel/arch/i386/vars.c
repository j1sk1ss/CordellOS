#include "../../include/vars.h"


char* names_buffer[VARS_COUNT] = { NULL };
char* vars_buffer[VARS_COUNT]  = { NULL };


void VARS_init() {

#if defined(ENVARS_SAVE)

    if (current_vfs->objexist("boot\\vars.txt") == 1) {
        int current_position = 0;
        Content* content = current_vfs->getobj("boot\\vars.txt");

        while (current_position < content->file->file_meta.file_size) {
            uint8_t buffer[SECTOR_SIZE] = { '\0' };
            char stop[1] = { '\n' };
            fread_off_stop(content, current_position, buffer, SECTOR_SIZE, stop);

            current_position += min(SECTOR_SIZE, strlen(buffer));
        }
    }
    else {
        Content* content = FAT_create_content("vars", 0, "txt");
        current_vfs->putobj("boot", content);
        FSLIB_unload_content_system(content);
    }

#endif

}

// -1 - nexists
// != -1 - exists
int VARS_exist(char* name) {
    for (int i = 0; i < VARS_COUNT; i++) 
        if (strstr(name, names_buffer[i]) == 0) return i;

    return -1;
}

char* VARS_get(char* name) {
    int position = VARS_exist(name);
    if (position == -1) return NULL;

    return vars_buffer[position];
}

void VARS_set(char* name, char* value) {
    int position = VARS_exist(name);
    if (position == -1) return;

    vars_buffer[position] = value;
}

void VARS_add(char* name, char* value) {
    int position = 0;
    while (position < VARS_COUNT) {
        if (names_buffer[position] == NULL && vars_buffer[position] == NULL) {
            vars_buffer[position]  = value;
            names_buffer[position] = name;
            return;
        }

        position++;
    }
}

void VARS_delete(char* name) {
    int position = VARS_exist(name);
    if (position == -1) return;

    free(names_buffer[position]);
    free(vars_buffer[position]);

    names_buffer[position] = NULL;
    vars_buffer[position]  = NULL;
}

char** VARS_get_names() {
    return names_buffer;
}

char** VARS_get_vars() {
    return vars_buffer;
}

void VARS_save() {

#if defined(ENVARS_SAVE)
#endif

}