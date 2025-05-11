#include "shell.h"


static vars_entry_t vars[50];
static char* current_path = "HOME";
static int exit = 1;


void main(int argc, char* argv[]) {
    load_font("home\\shell.psf");
    clrscr();
    shell_start_screen();

#ifdef ENVARS

    //====================
    //  SET INIT ENVARS

        envar_init_stack(vars, 50);
        if (envar_exist("clc", vars, 50) == -1) envar_add("clc", "\\HOME\\APPS\\STD\\CALC\\CALC.ELF", vars, 50);

    //  SET INIT ENVARS
     //====================

#endif

    //====================
    //  PREPARE SCREEN & INPUT

        while (exit) {
            printf("\n$%s> ", current_path);

            char input[COMMAND_LENGHT] = { '\0' };
            char input_data = ' ';
            int pos = 0;

            while (input_data != ENTER_BUTTON) {
                input_data = wait_char();
                if (input_data != BACKSPACE_BUTTON && input_data != LSHIFT_BUTTON && input_data != RSHIFT_BUTTON) {
                    input[pos++] = input_data;
                    putc(input_data, WHITE, BLACK);
                }
                else if (input_data == BACKSPACE_BUTTON) {
                    if (pos <= 0) continue;
                    input[pos--] = '\0';
                    cursor_set32(cursor_get_x32() - _psf_get_width(get_font()), cursor_get_y32());
                    display_char(cursor_get_x32(), cursor_get_y32(), ' ', WHITE, BLACK);
                }
            }

            int last_char = max(0, strlen(input) - 1);
            input[last_char] = '\0';
            
            execute_command(input);
        }

    //  PREPARE SCREEN & INPUT
    //====================

    free(current_path);
    tkill();
}

void shell_start_screen() {
    printf("\n");
    printf("Cordell Shell [ver. 0.6f | 11.05.2025]\n");
    printf("             NOW ON ENGLISH!          \n");
    printf("\n\n");
}

//====================
//  KSHELL COMMANDS
//====================

    void execute_command(char* command) {
        if (command == NULL) return;
        if (strlen(command) <= 0) return;
  
        //====================
        //  SPLIT COMMAND LINE TO ARGS
        //====================

            char* command_line[100] = { NULL };
            int tokenCount = 0;
            char* splitted = strtok(command, " ");

            while (splitted && tokenCount < 100) {
                char* token = (char*)clralloc(strlen(splitted) + 1);
                strncpy(token, splitted, strlen(splitted));

                if (token[0] != '$') command_line[tokenCount++] = splitted;
                else {
                    splitted++;
                    char envar_buffer[128] = { 0 };
                    if (envar_exist(splitted, vars, 50) != -1) {
                        command_line[tokenCount++] = envar_get(splitted, envar_buffer, vars, 50);
                    }
                    else {
                        command_line[tokenCount++] = splitted;
                    }
                }

                splitted = strtok(NULL, " ");
                free(token);
            }

        //====================
        //  SPLIT COMMAND LINE TO ARGS
        //====================
        //  DEFAULT SHELL COMMANDS CLEAR, ECHO AND HELP
        //====================

            if (strcmp(command_line[0], COMMAND_HELP) == 0) {
                printf("\n +========================================================");
                printf("\n | Common:");
                printf("\n | [%s] - show this message.", COMMAND_HELP);
                printf("\n | [%s] - screen cleanup.", COMMAND_CLEAR);
                printf("\n | [%s] - print message on screen.", COMMAND_ECHO);
                printf("\n | [%s] - current date.", COMMAND_TIME);
                printf("\n +--------------------------------------------------------");
                printf("\n | Network:\n");
                printf("\n | [%s] - show ip configuration.", COMMAND_IPCONFIG);
                printf("\n | [%s] - send UDP packet.", COMMAND_SEND_UDP_PACKET);
                printf("\n | [%s] - get last received UDP packet.", COMMAND_POP_UDP_PACKET);
                printf("\n +--------------------------------------------------------");
                printf("\n | Statistics:\n");
                printf("\n | [%s] - get summary info about content.", COMMAND_CINFO);
                printf("\n | [%s] - show current kernel and shell version.", COMMAND_VERSION);
                printf("\n | [%s] - show summary disk information.", COMMAND_DISK_DATA);
                printf("\n | [%s] - show heap usage.", COMMAND_MEM_DATA);
                printf("\n | [%s] <option> - show page usage", COMMAND_PAGE_DATA);
                printf("\n +--------------------------------------------------------");
                printf("\n | FileSystem:\n");
                printf("\n | [%s] <path> - go to directory.", COMMAND_IN_DIR);
                printf("\n | [%s] - show directory content.", COMMAND_LIST_DIR);
                printf("\n | [%s] - print file content to console.", COMMAND_FILE_VIEW);
                printf("\n | [%s] <path> <x> <y> - draw .bmp image", COMMAND_BMP_SHOW);
                printf("\n | [%s] <path> - launch .elf executable.\n", COMMAND_FILE_RUN);
                printf("\n | [%s] - reboot.", COMMAND_REBOOT);
                printf("\n | [%s] - exit from kernel shell. (Unrecomended option).", COMMAND_EXIT);
                printf("\n +========================================================\n");
            }
            else if (strcmp(command_line[0], COMMAND_EXIT)      == 0) exit = 1;
            else if (strcmp(command_line[0], COMMAND_REBOOT)    == 0) machine_restart();
            else if (strcmp(command_line[0], COMMAND_VERSION)   == 0) shell_start_screen();
            else if (strcmp(command_line[0], COMMAND_ECHO)      == 0) printf("\n%s", command_line[1]);
            else if (strcmp(command_line[0], COMMAND_CLEAR)     == 0) clrscr();
            else if (strcmp(command_line[0], COMMAND_DISK_DATA) == 0) {
                FSInfo_t info;
                get_fs_info(&info);

                printf("\nKernel disc-stat ver 0.3b\n");
                printf("dev:                             [%s]\n", info.mount);
                printf("fs type:                         [%s]\n", info.name);
                printf("type:                            [%i]\n", info.type);
                printf("total clusters x32:              [%i]\n", info.clusters);
                printf("sectors per cluster:             [%i]\n", info.spc);
                printf("fat size:                        [%i]\n", info.size);
            }
            else if (strcmp(command_line[0], COMMAND_TICKS) == 0) {
                printf("\nCurrent tick: %i\n", get_tick());
            }
            else if (strcmp(command_line[0], COMMAND_TIME) == 0) {
                DateInfo_t info;
                get_datetime(&info);
                printf(
                    "\nday: %i/%i/%i\ttime: %i:%i:%i", 
                    info.day, info.month, info.year, info.hour, info.minute, info.second 
                );
            }
            else if (strcmp(command_line[0], COMMAND_SET_ENVAR) == 0) {
                if (envar_exist(command_line[1], vars, 50) == -1) envar_add(command_line[1], command_line[2], vars, 50);
                else envar_set(command_line[1], command_line[2], vars, 50);
            }
            else if (strcmp(command_line[0], COMMAND_DEL_ENVAR) == 0) {
                envar_delete(command_line[1], vars, 50);
            }

        //====================
        //  DEFAULT SHELL COMMANDS CLEAR, ECHO AND HELP
        //====================
        //  FILE SYSTEM COMMANDS
        //====================

            else if (strcmp(command_line[0], COMMAND_IN_DIR) == 0) {
                str2uppercase(command_line[1]);
                if (strcmp(command_line[1], COMMAND_OUT_DIR) == 0) {
                    char* up_path = FSLIB_change_path(current_path, NULL);
                    if (up_path == NULL) {
                        up_path = malloc(5);
                        strcpy(up_path, "HOME");
                    }

                    free(current_path);
                    current_path = up_path;         
                }
                else {
                    char* path = command_line[1];
                    char* dir_path = NULL;

                    if (path[0] == '\\') {
                        memmove(path, path + 1, strlen(path));
                        dir_path = path;
                    } 
                    else {
                        dir_path = FSLIB_change_path(current_path, path);
                    }

                    if (!cexists(dir_path)) {
                        free(dir_path);
                        printf("\nDirectory not exists.");
                        return;
                    }

                    int ci = copen(dir_path);
                    if (ci >= 0) {
                        CInfo_t content_info;
                        cstat(ci, &content_info);

                        if (content_info.type == STAT_FILE) {
                            cclose(ci);
                            free(dir_path);
                            printf("\nNot a directory.");
                            return;
                        }

                        cclose(ci);
                        free(current_path);
                        current_path = dir_path;
                    }
                }
            }
            else if (strcmp(command_line[0], COMMAND_MAKE_FILE) == 0) mkfile(current_path, command_line[1], command_line[2]);
            else if (strcmp(command_line[0], COMMAND_MAKE_DIR) == 0) mkdir(current_path, command_line[1]);
            else if (strcmp(command_line[0], COMMAND_DELETE_CONTENT) == 0) {
                char* path = get_path(command_line[1]); 
                if (cexists(path) == 0) {
                    printf("\nContent not found.");
                    free(path);
                    return;
                }

                rmcontent(path);
                free(path);       
            }
            else if (strcmp(command_line[0], COMMAND_LIST_DIR) == 0) {
                int step = 0;
                int dir_ci = copen(current_path);
                if (dir_ci >= 0) {
                    int root_ci = opendir(dir_ci);
                    if (root_ci < 0) {
                        cclose(dir_ci);
                    }
                    else {    
                        while (step != -1) {
                            char name[11] = { 0 };
                            step = lsdir(root_ci, name, step);
                            printf("%s\t", name);
                        }

                        cclose(root_ci);
                        cclose(dir_ci);
                    }
                }
            }
            else if (strcmp(command_line[0], COMMAND_FILE_VIEW) == 0) {
                char* file_path = get_path(command_line[1]);
                if (cexists(file_path) == 0) {
                    printf("\nFile not found.");
                    free(file_path);
                    return;
                }
                
                printf("\n");

                int ci = copen(file_path);
                if (ci >= 0) {
                    int data_size = 0;
                    CInfo_t content_info;
                    cstat(ci, &content_info);

                    while (data_size < content_info.size) {
                        int copy_size = min(content_info.size - data_size, 128);
                        char* data = (char*)clralloc(copy_size);

                        fread(ci, data_size, (uint8_t*)data, copy_size);
                        printf("%s", data);

                        free(data);
                        data_size += copy_size;
                    }
                    
                    cclose(ci);
                    free(file_path);
                }
            }
            else if (strcmp(command_line[0], COMMAND_BMP_SHOW) == 0) {
                char* file_path = get_path(command_line[1]);
                if (cexists(file_path) == 0) {
                    printf("\nFile not found.");
                    free(file_path);
                    return;
                }
                
                bitmap_t* bitmap = BMP_create(file_path, atoi(command_line[2]), atoi(command_line[3]));
                BMP_display(bitmap);
                BMP_unload(bitmap);
                free(file_path);
            }
            else if (strcmp(command_line[0], COMMAND_FILE_RUN) == 0) {
                int pos = 2;
                char* exe_argv[COMMAND_BUFFER];
                while (command_line[pos] != NULL && pos < COMMAND_BUFFER) {
                    exe_argv[pos - 2] = command_line[pos];
                    pos++;
                }

                char* file_path = get_path(command_line[1]);
                if (cexists(file_path) == 0) {
                    printf("\nFile [%s] not found.", file_path);
                    return;
                }

                printf("\nExit code: [%i]\n", fexec(file_path, pos - 2, exe_argv));
                free(file_path);
            }
            else if (strcmp(command_line[0], COMMAND_CINFO) == 0) {
                char* info_file = get_path(command_line[1]);
                if (cexists(info_file) == 0) {
                    printf("\nContent not found.");
                    free(info_file);
                    return;
                }

                printf("\n");
                int ci = copen(info_file);
                if (ci >= 0) {
                    CInfo_t content_info;
                    cstat(ci, &content_info);

                    if (content_info.type == STAT_DIR) {
                        Date* creation_date  = FSLIB_get_date(content_info.creation_date, 1);
                        Date* accesed_date   = FSLIB_get_date(content_info.last_modification_date, 1);

                        printf("Directory\n");
                        printf("name:          [%s]\n", content_info.full_name);
                        printf("size:          [NaN]\n");
                        printf("creation date: [%i/%i/%i]\n", creation_date->day, creation_date->mounth, creation_date->year);
                        printf("accesed date:  [%i/%i/%i]\n", accesed_date->day, accesed_date->mounth, accesed_date->year);

                        free(creation_date);
                        free(accesed_date);
                    }
                    else if (content_info.type == STAT_FILE) {
                        Date* creation_date = FSLIB_get_date(content_info.creation_date, 1);
                        Date* accesed_date  = FSLIB_get_date(content_info.last_modification_date, 1);

                        printf("File\n");
                        printf("name:          [%s.%s]\n", content_info.file_name, content_info.file_extension);
                        printf("size:          [%iB]\n", content_info.size);
                        printf("creation date: [%i/%i/%i]\n", creation_date->day, creation_date->mounth, creation_date->year);
                        printf("accesed date:  [%i/%i/%i]\n", accesed_date->day, accesed_date->mounth, accesed_date->year);

                        free(creation_date);
                        free(accesed_date);
                    }

                    cclose(ci);
                }

                free(info_file);
            }

        //====================
        //  FILE SYSTEM COMMANDS
        //====================
        //  NETWORKING COMMANDS
        //====================

#ifdef NETWORK

            else if (strcmp(command_line[0], COMMAND_IPCONFIG) == 0) {
                uint8_t ip[4]  = { 0x00 };
                uint8_t mac[6] = { 0xFF };

                get_ip(ip);
                get_mac(mac);

                printf("\nKernel network-stat 0.2c\n");
                printf("\nCurrent IP:  [%i.%i.%i.%i]", ip[0], ip[1], ip[2], ip[3]);
                printf("\nCurrent MAC: [%p:%p:%p:%p:%p:%p]", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            }
            else if (strcmp(command_line[0], COMMAND_SEND_UDP_PACKET) == 0) {
                uint8_t ip[4] = { 0x00, 0x00, 0x00, 0x00 };
                get_ip(ip);

                uint8_t dst_ip[4] = { atoi(command_line[1]), atoi(command_line[2]), atoi(command_line[3]), atoi(command_line[4]) };
                uint16_t dst_port = atoi(command_line[5]);
                uint16_t src_port = atoi(command_line[6]);

                printf("\nTransfered packet [%s] | [%i.%i.%i.%i:%i] => [%i.%i.%i.%i:%i]",
                                                command_line[7], ip[0], ip[1], ip[2], ip[3], src_port,
                                                dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3], dst_port);
                send_udp_packet(dst_ip, src_port, dst_port, command_line[7], strlen(command_line[7]));
            }
            else if (strcmp(command_line[0], COMMAND_POP_UDP_PACKET) == 0) {
                uint8_t buffer[512];
                pop_received_udp_packet(buffer);

                printf("\n");
                printf("UDP str:     [%s]\n", (char*)buffer);
                printf("UDP uint:    [%u]\n", buffer);
                printf("UPD int:     [%i]\n", buffer);
                printf("UDP hex:     [%x]\n", buffer);
                printf("UDP ptr:     [%p]\n", buffer);
            }

#endif

        //====================
        //  NETWORKING COMMANDS
        //====================

            else printf("\nUnknown command [%s ...]", command_line[0]);

        printf("\n");
    }

//====================
//  KSHELL COMMANDS
//====================

// 0 - nlogin
// 1 - login success
int ulogin(char* login, char* password) {
    char hashed_login[100] = { '\0' };
    char hashed_passw[100] = { '\0' };

    sprintf(hashed_login, 100, "%lu", str2hash(login));
    sprintf(hashed_passw, 100, "%lu", str2hash(password));

    char* lines[40] = { NULL };
    int pos = 0;
    
    char content_text[512] = { 0 };
    int ci = copen("boot\\users.txt");
    if (ci >= 0) {
        fread(ci, 0, (uint8_t*)content_text, 512);
        cclose(ci);
    }

    char* token = strtok(content_text, "\n");
    while (token) {
        lines[pos++] = token;
        token = strtok(NULL, "\n");
    }

    for (int i = 0; i < pos - 1; i++) {
        if (compare_hash(hashed_login, lines[i]) == 0 && compare_hash(hashed_passw, lines[i + 1]) == 0) {
            free(content_text);
            return 1;
        }
    }

    free(content_text);
    return 0;
}

char* get_path(char* path) {
    if (path[0] == '\\') {
        char* new_path = (char*)clralloc(strlen(path) + 1);
        strncpy(new_path, path + 1, strlen(path));
        return new_path;
    } 

    return FSLIB_change_path(current_path, path);
}