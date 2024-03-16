#include "../include/fslib.h"


void FSLIB_unload_directories_system(Directory* directory) {
    if (directory == NULL) return;
    if (directory->files != NULL) FSLIB_unload_files_system(directory->files);
    if (directory->subDirectory != NULL) FSLIB_unload_directories_system(directory->subDirectory);
    if (directory->next != NULL) FSLIB_unload_directories_system(directory->next);
    if (directory->data_pointer != NULL) free(directory->data_pointer);

    free(directory->name);
    free(directory);
}

void FSLIB_unload_files_system(File* file) {
    if (file == NULL) return;
    if (file->next != NULL) FSLIB_unload_files_system(file->next);
    if (file->data_pointer != NULL) free(file->data_pointer);
    if (file->data != NULL) free(file->data);

    free(file->name);
    free(file->extension);
    free(file);
}

void FSLIB_unload_content_system(Content* content) {
    if (content == NULL) return;
    if (content->directory != NULL) FSLIB_unload_directories_system(content->directory);
    if (content->file != NULL) FSLIB_unload_files_system(content->file);
    
    free(content);
}

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
        int newPathLen = strlen(currentPath) + strlen(content) + 2;
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

//====================================================================
//  Read file content by path
//  ECX - path
//  EAX - returned data
char* fread(const char* path) {
    void* pointed_data;
    __asm__ volatile(
        "movl $9, %%eax\n"
        "movl %1, %%ebx\n"
        "movl $0, %%ecx\n"
        "int %2\n"
        "movl %%eax, %0\n"
        : "=r"(pointed_data)
        : "r"((uint32_t)path), "i"(SYSCALL_INTERRUPT)
        : "eax", "ebx", "ecx", "edx"
    );

    return pointed_data;
}

//====================================================================
//  Read file content by path
//  ECX - path
//  EAX - returned data
char* fread_stop(const char* path, char* stop) {
    void* pointed_data;
    __asm__ volatile(
        "movl $59, %%eax\n"
        "movl %1, %%ebx\n"
        "movl %2, %%ecx\n"
        "int %3\n"
        "movl %%eax, %0\n"
        : "=r"(pointed_data)
        : "r"((uint32_t)path), "r"(stop), "i"(SYSCALL_INTERRUPT)
        : "eax", "ebx", "ecx", "edx"
    );

    return pointed_data;
}

//====================================================================
// Function read content data to buffer with file seeking
// EBX - content pointer
// ECX - data offset (file seek)
// EDX - buffer pointer
// ESI - buffer len / data len
void fread_off(Content* content, int offset, uint8_t* buffer, int len) {
    __asm__ volatile(
        "movl $33, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "movl %2, %%edx\n"
        "movl %3, %%esi\n"
        "int $0x80\n"
        :
        : "g"(content), "g"(offset), "g"(buffer), "g"(len)
        : "eax", "ebx", "ecx", "edx", "esi"
    );
}

//====================================================================
// Function read content data to buffer with file seeking
// EBX - content pointer
// ECX - data offset (file seek)
// EDX - buffer pointer
// ESI - buffer len / data len
void fread_off_stop(Content* content, int offset, uint8_t* buffer, int len, char* stop) {
    __asm__ volatile(
        "movl $58, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "movl %2, %%edx\n"
        "movl %3, %%esi\n"
        "movl %3, %%edi\n"
        "int $0x80\n"
        :
        : "g"(content), "g"(offset), "g"(buffer), "g"(len), "g"(stop)
        : "eax", "ebx", "ecx", "edx", "esi", "edi"
    );
}

//====================================================================
// Write file to content (if it exists) by path (rewrite all content)
// EBX - path
// ECX - data
void fwrite(const char* path, const char* data) {
    __asm__ volatile(
        "movl $10, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "int $0x80\n"
        :
        : "r"((uint32_t)path), "r"((uint32_t)data)
        : "eax", "ebx", "ecx"
    );
}

//====================================================================
// Write file to content (if it exists) with offset
// EBX - content pointer
// ECX - data offset
// EDX - buffer pointer
// ESI - buffer len / data len
void fwrite_off(Content* content, int offset, uint8_t* buffer, int len) {
    __asm__ volatile(
        "movl $50, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "movl %2, %%edx\n"
        "movl %3, %%esi\n"
        "int $0x80\n"
        :
        : "g"(content), "g"(offset), "g"(buffer), "g"(len)
        : "eax", "ebx", "ecx", "edx", "esi"
    );
}

//====================================================================
// Returns linked list of dir content by path
// EBX - path
// ECX - pointer to directory
Directory* opendir(const char* path) {
    Directory* directory;
    __asm__ volatile(
        "movl $11, %%eax\n"
        "movl %1, %%ebx\n"
        "int $0x80\n"
        "movl %%eax, %0\n"
        : "=r"(directory)
        : "r"((uint32_t)path)
        : "eax", "ebx", "ecx"
    );

    return directory;
}

//====================================================================
//  Returns linked list of dir content by path
//  EBX - path
//  ECX - pointer to directory
Content* get_content(const char* path) {
    Content* content;
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

//====================================================================
//  Function for checking if content exist by this path
//  EBX - path
//  ECX - result (0 - nexists)
int cexists(const char* path) {
    int result;

    __asm__ volatile(
        "movl $15, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "int $0x80\n"
        : 
        : "r"((uint32_t)path), "r"(&result)
        : "eax", "ebx"
    );

    return result;
}

//====================================================================
//  This function creates file
//  EBX - path
//  RCX - name (with extention)
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

//====================================================================
//  This function creates directory
//  EBX - path
//  ECX - name
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

//====================================================================
//  This function remove content
//  EBX - path
//  ECX - name (if file - with extention)
void rmcontent(const char* path, const char* name) {
    __asm__ volatile(
        "movl $18, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "int $0x80\n"
        :
        : "r"((uint32_t)path), "r"((uint32_t)name)
        : "eax", "ebx", "ecx"
    );
}

//====================================================================
// This function change content meta by path
// EBX - path
// ECX - new meta
void chgcontent(const char* path, directory_entry_t* meta) {
    __asm__ volatile(
        "movl $25, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "int $0x80\n"
        :
        : "r"((uint32_t)path), "r"((uint32_t)meta)
        : "eax", "ebx", "ecx"
    );
}

//====================================================================
//  Function that executes ELF file
//  EAX - result CODE
//  EBX - path
//  ECX - args (count)
//  EDX - argv (array of args)
int fexec(char* path, int argc, char** argv) {
    int result = 0;
    __asm__ volatile(
        "movl $12, %%eax\n"
        "movl %1, %%ebx\n"
        "movl %2, %%ecx\n"
        "movl %3, %%edx\n"
        "int $0x80\n"
        "movl %%eax, %0\n"
        : "=r"(result)
        : "m"((uint32_t)path), "r"(argc), "m"(argv)
        : "eax", "ebx", "ecx", "edx"
    );

    return result;
}