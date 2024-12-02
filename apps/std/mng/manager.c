#include "manager.h"


int current_position = 0;
int max_rows = 0;

Content* current_directory;
Content choosed_content;

char* current_path = "HOME";


int main(int argc, char* argv[]) {
    clrscr();

    choosed_content.directory = NULL;
    choosed_content.file      = NULL;

    if (argc != 0) current_path = argv[0];
    current_directory = opendir(current_path);

    return loop();
}

int loop() {
    while (1) {
        char user_action = keyboard_wait();
        if (user_action == UP_ARROW_BUTTON) current_position = max(0, current_position - 1);
        else if (user_action == DOWN_ARROW_BUTTON) current_position = min(max_rows - 1, current_position + 1);
        
        else if (user_action == ENTER_BUTTON) open_content();
        
        else if (user_action == F1_BUTTON) return 1;
        else if (user_action == F2_BUTTON) {
            // mkdir
        }
        else if (user_action == F3_BUTTON) {
            // mkfile
        }
        else if (user_action == F4_BUTTON) {
            // content edit
            // - rename
            // - delete
            // - copy / move
        }

        clrscr();
        display_manager();
    }
}

void open_content() {
    if (choosed_content.directory != NULL) {
        FSLIB_unload_content_system(current_directory);
        char* path = FSLIB_change_path(current_path, choosed_content.directory->name);
        current_directory = opendir(path);

        free(current_path);
        current_path = path;
    }

    else if (choosed_content.file != NULL) {
        char* argv[1] = { current_path };

        if (strstr(choosed_content.file->extension, "txt") == 0) fexec(envar_get("edt"), 1, argv);
        else fexec(envar_get("edt"), 1, argv);
    }
}

void display_manager() {
    Directory* subdir = current_directory->directory->subDirectory;
    File* subfile = current_directory->directory->files;
    int line = 0;
    max_rows = 0;

    printf("| NAME        | EXT  | TYPE | SIZE  |\n");

    while (subdir != NULL) {
        char dir_name[12] = { ' ' };
        str2len(dir_name, subdir->name, 11);

        cprintf(line == current_position ? RED : BLACK, "| %s | NONE | DIR  | N/D   |\n",
            dir_name, subdir->directory_meta.file_size
        );

        if (line++ == current_position) {
            choosed_content.file = NULL;
            choosed_content.directory = subdir;
        }

        subdir = subdir->next;
        max_rows++;
    }

    while (subfile != NULL) {
        char file_name[9] = { ' ' };
        str2len(file_name, subfile->name, 8);

        char file_extension[5] = { ' ' };
        str2len(file_extension, subfile->extension, 4);

        char* file_size_tmp = itoa(subfile->file_meta.file_size);
        char file_size[5] = { ' ' };
        str2len(file_size, file_size_tmp, 4);
        free(file_size_tmp);

        cprintf(line == current_position ? RED : BLACK, "| %s | %s | FILE | %s |\n",
            file_name, file_extension, file_size
        );

        if (line++ == current_position) {
            choosed_content.file = subfile;
            choosed_content.directory = NULL;
        }

        subfile = subfile->next;
        max_rows++;
    }
}