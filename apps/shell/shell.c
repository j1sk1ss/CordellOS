#include "shell.h"


static vars_entry_t vars[50];
static char* current_path = "HOME";
static int exit = 1;


void main(int argc, char* argv[]) {
    load_font("home\\shell.psf");
    // switch2user();

    clrscr();
    shell_start_screen();

#ifdef ENVARS

    //====================
    //  SET INIT ENVARS

        envar_init_stack(vars, 50);
        if (envar_exist("clc", vars, 50) == -1) envar_add("clc", "\\HOME\\APPS\\STD\\CALC\\CALC.ELF", vars, 50);
        if (envar_exist("asm", vars, 50) == -1) envar_add("asm", "\\HOME\\APPS\\STD\\ASM\\ASM.ELF", vars, 50);

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
    printf("Cordell Shell [ver. 0.6e | 14.12.2024]\n");
    printf("Stai entrando nella shell del kernel leggero. Usa [aiuto] per ottenere aiuto.\n\n");
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
                printf("\n> Usa [%s] per ottenere aiuto",                                     COMMAND_HELP);
                printf("\n> Utilizzare [%s] per la pulizia dello schermo",                    COMMAND_CLEAR);
                printf("\n> Usa [%s] per l'eco",                                              COMMAND_ECHO);
                printf("\n> Usa [%s] per vista questo giorno\n",                              COMMAND_TIME);
                printf("\n> Usa [%s] per ottenere informazioni sulla configurazione di rete", COMMAND_IPCONFIG);
                printf("\n> Usa [%s] per iniziare a inviare pacchetti UDP",                   COMMAND_SEND_UDP_PACKET);
                printf("\n> Usa [%s] per ottenere l'ultimo pacchetto UDP ricevuto\n",         COMMAND_POP_UDP_PACKET);
                printf("\n> Usa [%s] per ottenere informazioni sul contenuto di fs",          COMMAND_CINFO);
                printf("\n> Utilizza la [%s] per vista versione",                             COMMAND_VERSION);
                printf("\n> Utilizza la [%s] per vista disk-data informazione",               COMMAND_DISK_DATA);
                printf("\n> Utilizza la [%s] per vista mem-data informazione",                COMMAND_MEM_DATA);
                printf("\n> Utilizza la [%s] <option> per vista page-data informazione\n",    COMMAND_PAGE_DATA);
                printf("\n> Usa [%s] <nome> per entranto dir",                                COMMAND_IN_DIR);
                printf("\n> Usa [%s] per guardare tutto cosa in dir\n",                       COMMAND_LIST_DIR);
                printf("\n> Usa [%s] per guardare tutto data in file",                        COMMAND_FILE_VIEW);
                printf("\n> Usa [%s] per guardare bmp",                                       COMMAND_BMP_SHOW);
                printf("\n> Usa [%s] per run file\n",                                         COMMAND_FILE_RUN);
                printf("\n> Utilizzare [%s] per riavviare il sistema operativo",              COMMAND_REBOOT);
                printf("\n> Utilizzare [%s] per uscire dal kernel\n",                         COMMAND_EXIT);
            }

            else if (strcmp(command_line[0], COMMAND_EXIT)      == 0) exit = 1;
            else if (strcmp(command_line[0], COMMAND_REBOOT)    == 0) machine_restart();
            else if (strcmp(command_line[0], COMMAND_VERSION)   == 0) shell_start_screen();
            else if (strcmp(command_line[0], COMMAND_ECHO)      == 0) printf("\n%s", command_line[1]);
            else if (strcmp(command_line[0], COMMAND_CLEAR)     == 0) clrscr();
                
            else if (strcmp(command_line[0], COMMAND_DISK_DATA) == 0) {
                FSInfo_t info;
                get_fs_info(&info);

                printf("\nUTILITY KERNEL DISCO-DATI VER 0.3a\n");
                printf("DEV:                             [%s]\n", info.mount);
                printf("FS TYPE:                         [%s]\n", info.name);
                printf("TYPE:                            [%i]\n", info.type);
                printf("CLUSTER TOTALI x32:              [%i]\n", info.clusters);
                printf("SETTORI PER CLUSTER:             [%i]\n", info.spc);
                printf("DIMENSIONE DELLA TABELLA GRASSO: [%i]\n", info.size);
            }
            
            else if (strcmp(command_line[0], COMMAND_TICKS) == 0) {
                printf("\nCurrent tick: %i\n", get_tick());
            }

            else if (strcmp(command_line[0], COMMAND_TIME) == 0) {
                DateInfo_t info;
                get_datetime(&info);
                printf(
                    "\nGIORNO: %i/%i/%i\tTEMPO: %i:%i:%i", 
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
                    else dir_path = FSLIB_change_path(current_path, path);

                    if (!cexists(dir_path)) {
                        free(dir_path);
                        printf("\nLA DIRECTORY NON ESISTE");
                        return;
                    }

                    int content = copen(dir_path);
                    CInfo_t content_info;
                    cstat(content, &content_info);

                    if (content_info.type == STAT_FILE) {
                        cclose(content);
                        free(dir_path);
                        printf("\nQUESTA NON E` UNA DIRECTORY");
                        return;
                    }

                    cclose(content);
                    free(current_path);
                    current_path = dir_path;
                }
            }

            else if (strcmp(command_line[0], COMMAND_MAKE_FILE) == 0) mkfile(current_path, command_line[1]);
            else if (strcmp(command_line[0], COMMAND_MAKE_DIR) == 0) mkdir(current_path, command_line[1]);

            else if (strcmp(command_line[0], COMMAND_DELETE_CONTENT) == 0) {
                char* path = get_path(command_line[1]); 
                if (cexists(path) == 0) {
                    printf("\nLA CONTENT NON ESISTE");
                    free(path);
                    return;
                }

                rmcontent(path);
                free(path);       
            }

            else if (strcmp(command_line[0], COMMAND_LIST_DIR) == 0) {
                int step = 0;
                int dir_ci = copen(current_path);
                int root_ci = opendir(dir_ci);
                
                while (step != -1) {
                    char name[11] = { 0 };
                    step = lsdir(root_ci, name, step);
                    printf("%s\t", name);
                }

                cclose(root_ci);
                cclose(dir_ci);
            }

            else if (strcmp(command_line[0], COMMAND_FILE_VIEW) == 0) {
                char* file_path = get_path(command_line[1]);
                if (cexists(file_path) == 0) {
                    printf("\nLA FILE NON ESISTE");
                    free(file_path);
                    return;
                }
                
                printf("\n");

                int content = copen(file_path);
                int data_size = 0;
                CInfo_t content_info;
                cstat(content, &content_info);

                while (data_size < content_info.size) {
                    int copy_size = min(content_info.size - data_size, 128);
                    char* data = (char*)clralloc(copy_size);

                    fread(content, data_size, (uint8_t*)data, copy_size);
                    printf("%s", data);

                    free(data);
                    data_size += copy_size;
                }
                
                cclose(content);
                free(file_path);
            }

            else if (strcmp(command_line[0], COMMAND_BMP_SHOW) == 0) {
                char* file_path = get_path(command_line[1]);
                if (cexists(file_path) == 0) {
                    printf("\nLA FILE NON ESISTE");
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
                    printf("\nLA FILE [%s] NON ESISTE", file_path);
                    return;
                }

                printf("\nCODE: [%i]\n", fexec(file_path, pos - 2, exe_argv));
                free(file_path);
            }

            else if (strcmp(command_line[0], COMMAND_CINFO) == 0) {
                char* info_file = get_path(command_line[1]);
                if (cexists(info_file) == 0) {
                    printf("\nLA CONTENT NON ESISTE");
                    free(info_file);
                    return;
                }

                printf("\n");
                int content = copen(info_file);
                CInfo_t content_info;
                cstat(content, &content_info);

                if (content_info.type == STAT_DIR) {
                    Date* creation_date  = FSLIB_get_date(content_info.creation_date, 1);
                    Date* accesed_date   = FSLIB_get_date(content_info.last_modification_date, 1);

                    printf("DIRECTORY\n");
                    printf("NAME:          [%s]\n", content_info.full_name);
                    printf("SIZE:          [NaN]\n");
                    printf("CREATION DATE: [%i/%i/%i]\n", creation_date->day, creation_date->mounth, creation_date->year);
                    printf("ACCESED DATE:  [%i/%i/%i]\n", accesed_date->day, accesed_date->mounth, accesed_date->year);

                    free(creation_date);
                    free(accesed_date);
                }
                else if (content_info.type == STAT_FILE) {
                    Date* creation_date = FSLIB_get_date(content_info.creation_date, 1);
                    Date* accesed_date  = FSLIB_get_date(content_info.last_modification_date, 1);

                    printf("FILE\n");
                    printf("NAME:          [%s.%s]\n", content_info.file_name, content_info.file_extension);
                    printf("SIZE:          [%iB]\n", content_info.size);
                    printf("CREATION DATE: [%i/%i/%i]\n", creation_date->day, creation_date->mounth, creation_date->year);
                    printf("ACCESED DATE:  [%i/%i/%i]\n", accesed_date->day, accesed_date->mounth, accesed_date->year);

                    free(creation_date);
                    free(accesed_date);
                }

                cclose(content);
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

                printf("\nUTILITA` KERNEL IPCONF VERSIONE 0.2b\n");
                printf("\nIP ATTUALE:  [%i.%i.%i.%i]", ip[0], ip[1], ip[2], ip[3]);
                printf("\nMAC ATTUALE: [%p:%p:%p:%p:%p:%p]", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            }

            else if (strcmp(command_line[0], COMMAND_SEND_UDP_PACKET) == 0) {
                uint8_t ip[4] = { 0x00, 0x00, 0x00, 0x00 };
                get_ip(ip);

                uint8_t dst_ip[4] = { atoi(command_line[1]), atoi(command_line[2]), atoi(command_line[3]), atoi(command_line[4]) };
                uint16_t dst_port = atoi(command_line[5]);
                uint16_t src_port = atoi(command_line[6]);

                printf("\nTRASFERIMENTO [%s] DA [%i.%i.%i.%i:%i] A [%i.%i.%i.%i:%i]",
                                                command_line[7], ip[0], ip[1], ip[2], ip[3], src_port,
                                                dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3], dst_port);
                send_udp_packet(dst_ip, src_port, dst_port, command_line[7], strlen(command_line[7]));
            }

            else if (strcmp(command_line[0], COMMAND_POP_UDP_PACKET) == 0) {
                uint8_t buffer[512];
                pop_received_udp_packet(buffer);

                printf("\n");
                printf("RICEVUTO PACCHETTO UDP STR:     [%s]\n", (char*)buffer);
                printf("RICEVUTO PACCHETTO UDP UINT:    [%u]\n", buffer);
                printf("RICEVUTO PACCHETTO UDP INT:     [%i]\n", buffer);
                printf("RICEVUTO PACCHETTO UDP HEX:     [%x]\n", buffer);
                printf("RICEVUTO PACCHETTO UDP POINTER: [%p]\n", buffer);
            }

#endif

        //====================
        //  NETWORKING COMMANDS
        //====================

            else printf("\nCOMANDO [%s ...] SCONOSCUITO", command_line[0]);

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
    int users = copen("boot\\users.txt");
    fread(users, 0, (uint8_t*)content_text, 512);
    char* token = strtok(content_text, "\n");
    while (token) {
        lines[pos++] = token;
        token = strtok(NULL, "\n");
    }

    for (int i = 0; i < pos - 1; i++) 
        if (compare_hash(hashed_login, lines[i]) == 0 && compare_hash(hashed_passw, lines[i + 1]) == 0) {
            free(content_text);
            return 1;
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