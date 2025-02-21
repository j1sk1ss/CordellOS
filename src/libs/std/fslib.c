#include "../include/fslib.h"


// Get date from content meta data (work with FAT32)
// 1 - date
// 2 - time
Date* FSLIB_get_date(short data, int type) {
    Date* date = malloc(sizeof(Date));
    switch (type) {
        case 1: // date
            date->year   = ((data >> 9) & 0x7F) + 1980;
            date->mounth = (data >> 5) & 0xF;
            date->day    = data & 0x1F;
            return date;

        break;

        case 2: // time
            date->hour   = (data >> 11) & 0x1F;
            date->minute = (data >> 5) & 0x3F;
            date->second = (data & 0x1F) * 2;
            return date;

        break;
    }

    return date;
}

// Return NULL if can`t make updir command
char* FSLIB_change_path(const char* currentPath, const char* content) {
    if (content == NULL || content[0] == '\0') {
        const char* lastSeparator = strrchr(currentPath, '\\');
        if (lastSeparator == NULL) return NULL;
        else {
            size_t parentPathLen = lastSeparator - currentPath;
            char* parentPath = malloc(parentPathLen + 1);
            if (parentPath == NULL) {
                printf("Memory allocation failed\n");
                return NULL;
            }

            strncpy(parentPath, currentPath, parentPathLen);
            parentPath[parentPathLen] = '\0';

            return parentPath;
        }
    }
    
    else {
        size_t newPathLen = strlen(currentPath) + strlen(content) + 2;
        char* newPath  = malloc(newPathLen);
        if (newPath == NULL) return NULL;

        strcpy(newPath, currentPath);
        if (newPath[strlen(newPath) - 1] != '\\') 
            strcat(newPath, "\\");

        strcat(newPath, content);
        newPath[newPathLen - 1] = '\0';

        return newPath;
    }
}

void get_fs_info(FSInfo_t* info) {
     __asm__ volatile(
        "movl $45, %%eax\n"
        "movl %0, %%ebx\n"
        "int $0x80\n"
        :
        : "r"(info)
        : "%eax", "%ebx"
    );
}

void fread(int ci, int offset, uint8_t* buffer, int len) {
    __asm__ volatile(
        "movl $33, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "movl %2, %%edx\n"
        "movl %3, %%esi\n"
        "int $0x80\n"
        :
        : "g"(ci), "g"(offset), "g"(buffer), "g"(len)
        : "eax", "ebx", "ecx", "edx", "esi"
    );
}

void fread_stop(int ci, int offset, uint8_t* buffer, int len, char* stop) {
    __asm__ volatile(
        "movl $58, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "movl %2, %%edx\n"
        "movl %3, %%esi\n"
        "movl %4, %%edi\n"
        "int $0x80\n"
        :
        : "g"(ci), "g"(offset), "g"(buffer), "g"(len), "g"(stop)
        : "eax", "ebx", "ecx", "edx", "esi", "edi"
    );
}

void fwrite(int ci, int offset, uint8_t* buffer, int len) {
    __asm__ volatile(
        "movl $50, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "movl %2, %%edx\n"
        "movl %3, %%esi\n"
        "int $0x80\n"
        :
        : "g"(ci), "g"(offset), "g"(buffer), "g"(len)
        : "eax", "ebx", "ecx", "edx", "esi"
    );
}

int opendir(int ci) {
    int root_ci = -1;
    __asm__ volatile(
        "movl $67, %%eax\n"
        "movl %1, %%ebx\n"
        "int $0x80\n"
        "movl %%eax, %0\n"
        : "=r"(root_ci)
        : "r"(ci)
        : "eax", "ebx"
    );

    return root_ci;
}

int lsdir(int ci, char* name, int step) {
    int lstep = -1;
    __asm__ volatile(
        "movl $11, %%eax\n"
        "movl %1, %%ebx\n"
        "movl %2, %%ecx\n"
        "movl %3, %%edx\n"
        "int $0x80\n"
        "movl %%eax, %0\n"
        : "=r"(lstep)
        : "r"(ci), "r"((uint32_t)name), "r"(step)
        : "eax", "ebx", "ecx"
    );

    return lstep;
}

int copen(const char* path) {
    int content = -1;
    __asm__ volatile(
        "movl $30, %%eax\n"
        "movl %1, %%ebx\n"
        "movl %2, %%ecx\n"
        "int $0x80\n"
        "movl %%eax, %0\n"
        : "=r"(content)
        : "r"((uint32_t)path), "r"(content)
        : "eax", "ebx", "ecx"
    );

    return content;
}

int cexists(const char* path) {
    int result = 0;
    __asm__ volatile(
        "movl $15, %%eax\n"
        "movl %1, %%ebx\n"
        "int $0x80\n"
        "movl %%eax, %0\n"
        : "=r"(result)
        : "r"((uint32_t)path)
        : "eax", "ebx"
    );

    return result;
}

void mkfile(const char* path, const char* name) {
    __asm__ volatile(
        "movl $16, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "int $0x80\n"
        :
        : "r"((uint32_t)path), "r"((uint32_t)name)
        : "eax", "ebx", "ecx"
    );
}

void mkdir(const char* path, const char* name) {
    __asm__ volatile(
        "movl $17, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "int $0x80\n"
        :
        : "r"((uint32_t)path), "r"((uint32_t)name)
        : "eax", "ebx", "ecx"
    );
}

void rmcontent(const char* path) {
    __asm__ volatile(
        "movl $18, %%eax\n"
        "movl %0, %%ebx\n"
        "int $0x80\n"
        :
        : "r"((uint32_t)path)
        : "eax", "ebx"
    );
}

void chgcontent(const char* path, const char* new_name) {
    __asm__ volatile(
        "movl $25, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "int $0x80\n"
        :
        : "r"((uint32_t)path), "r"((uint32_t)new_name)
        : "eax", "ebx", "ecx"
    );
}

int cstat(int ci, CInfo_t* info) {
    __asm__ volatile(
        "movl $65, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "int $0x80\n"
        :
        : "r"(ci), "r"((uint32_t)info)
        : "eax", "ebx", "ecx"
    );

    return 1;
}

int cclose(int ci) {
    __asm__ volatile(
        "movl $66, %%eax\n"
        "movl %0, %%ebx\n"
        "int $0x80\n"
        :
        : "r"(ci)
        : "eax", "ebx", "ecx"
    );

    return 1;
}

int fexec(char* path, int argc, char** argv) {
    struct ELF_program* program = get_entry_point(path);
    int result = execute(program, argc, argv);
    free_program(program);

    return result;
}