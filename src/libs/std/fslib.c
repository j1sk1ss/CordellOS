#include "../include/fslib.h"


Content* FSLIB_create_content() {
    Content* content = (Content*)clralloc(sizeof(Content));

    content->directory      = NULL;
    content->file           = NULL;
    content->parent_cluster = -1;

    return content;
}

Directory* FSLIB_create_directory() {
    Directory* directory = (Directory*)clralloc(sizeof(Directory));

    directory->name         = NULL;
    directory->files        = NULL;
    directory->subDirectory = NULL;
    directory->next         = NULL;
    directory->data_pointer = NULL;

    return directory;
}

File* FSLIB_create_file() {
    File* file = (File*)clralloc(sizeof(File));

    file->name         = NULL;
    file->extension    = NULL;
    file->next         = NULL;
    file->data         = NULL;
    file->data_pointer = NULL;

    return file;
}

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
        "mov $9, %%rax\n"
        "mov %1, %%rbx\n"
        "mov $0, %%rcx\n"
        "int %2\n"
        "mov %%rax, %0\n"
        : "=r"(pointed_data)
        : "r"((uint64_t)path), "i"(SYSCALL_INTERRUPT)
        : "rax", "rbx", "rcx", "rdx"
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
        "mov $59, %%rax\n"
        "mov %1, %%rbx\n"
        "mov %2, %%rcx\n"
        "int %3\n"
        "mov %%rax, %0\n"
        : "=r"(pointed_data)
        : "r"((uint64_t)path), "r"(stop), "i"(SYSCALL_INTERRUPT)
        : "rax", "rbx", "rcx", "rdx"
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
        "mov $33, %%rax\n"
        "mov %0, %%rbx\n"
        "mov %1, %%rcx\n"
        "mov %2, %%rdx\n"
        "mov %3, %%rsi\n"
        "int $0x80\n"
        :
        : "g"(content), "g"(offset), "g"(buffer), "g"(len)
        : "rax", "rbx", "rcx", "rdx", "rsi"
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
        "mov $58, %%rax\n"
        "mov %0, %%rbx\n"
        "mov %1, %%rcx\n"
        "mov %2, %%rdx\n"
        "mov %3, %%rsi\n"
        "mov %4, %%rdi\n"
        "int $0x80\n"
        :
        : "g"(content), "g"(offset), "g"(buffer), "g"(len), "g"(stop)
        : "rax", "rbx", "rcx", "rdx", "rsi", "rdi"
    );
}

//====================================================================
// Write file to content (if it exists) by path (rewrite all content)
// EBX - path
// ECX - data
void fwrite(const char* path, const char* data) {
    __asm__ volatile(
        "mov $10, %%rax\n"
        "mov %0, %%rbx\n"
        "mov %1, %%rcx\n"
        "int $0x80\n"
        :
        : "r"((uint64_t)path), "r"((uint64_t)data)
        : "rax", "rbx", "rcx"
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
        "mov $50, %%rax\n"
        "mov %0, %%rbx\n"
        "mov %1, %%rcx\n"
        "mov %2, %%rdx\n"
        "mov %3, %%rsi\n"
        "int $0x80\n"
        :
        : "g"(content), "g"(offset), "g"(buffer), "g"(len)
        : "rax", "rbx", "rcx", "rdx", "rsi"
    );
}

//====================================================================
// Returns linked list of dir content by path
// EBX - path
// ECX - pointer to directory
Directory* opendir(const char* path) {
    Directory* directory;
    __asm__ volatile(
        "mov $11, %%rax\n"
        "mov %1, %%rbx\n"
        "int $0x80\n"
        "mov %%rax, %0\n"
        : "=r"(directory)
        : "r"((uint64_t)path)
        : "rax", "rbx", "rcx"
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
        "mov $30, %%rax\n"
        "mov %1, %%rbx\n"
        "mov %2, %%rcx\n"
        "int $0x80\n"
        "mov %%rax, %0\n"
        : "=r"(content)
        : "r"((uint64_t)path), "r"(content)
        : "rax", "rbx", "rcx"
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
        "mov $15, %%rax\n"
        "mov %0, %%rbx\n"
        "mov %1, %%rcx\n"
        "int $0x80\n"
        : 
        : "r"((uint64_t)path), "r"(&result)
        : "rax", "rbx"
    );

    return result;
}

//====================================================================
//  This function creates file
//  EBX - path
//  RCX - name (with extention)
void mkfile(const char* path, const char* name) {
    __asm__ volatile(
        "mov $16, %%rax\n"
        "mov %0, %%rbx\n"
        "mov %1, %%rcx\n"
        "int $0x80\n"
        :
        : "r"((uint64_t)path), "r"((uint64_t)name)
        : "rax", "rbx", "rcx"
    );
}

//====================================================================
//  This function creates directory
//  EBX - path
//  ECX - name
void mkdir(const char* path, const char* name) {
    __asm__ volatile(
        "mov $17, %%rax\n"
        "mov %0, %%rbx\n"
        "mov %1, %%rcx\n"
        "int $0x80\n"
        :
        : "r"((uint64_t)path), "r"((uint64_t)name)
        : "rax", "rbx", "rcx"
    );
}

//====================================================================
//  This function remove content
//  EBX - path
//  ECX - name (if file - with extention)
void rmcontent(const char* path) {
    __asm__ volatile(
        "mov $18, %%rax\n"
        "mov %0, %%rbx\n"
        "int $0x80\n"
        :
        : "r"((uint64_t)path)
        : "rax", "rbx"
    );
}

//====================================================================
// This function change content meta by path
// EBX - path
// ECX - new meta
void chgcontent(const char* path, directory_entry_t* meta) {
    __asm__ volatile(
        "mov $25, %%rax\n"
        "mov %0, %%rbx\n"
        "mov %1, %%rcx\n"
        "int $0x80\n"
        :
        : "r"((uint64_t)path), "r"((uint64_t)meta)
        : "rax", "rbx", "rcx"
    );
}

//====================================================================
//  Function that executes ELF file
//  EAX - result CODE
//  EBX - path
//  ECX - args (count)
//  EDX - argv (array of args)
int fexec(char* path, int argc, char** argv) {
    struct ELF_program* program = get_entry_point(path);
    int result = execute(program, argc, argv);
    free_program(program);

    return result;
}