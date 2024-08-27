#include "../include/stdlib.h"


//====================================================================
// Function start task at address with entered name
// EBX - name
// ECX - address 
void tstart(char* name, uint32_t address, uint32_t delay) {
    __asm__ volatile(
        "mov $26, %%rax\n"
        "mov %0, %%rbx\n"
        "mov %1, %%rcx\n"
        "mov %2, %%rdx\n"
        "int %3\n"
        :
        : "r"(name), "r"((uint64_t)address), "r"((uint64_t)delay), "i"(SYSCALL_INTERRUPT)
        : "rax", "rbx", "rcx"
    );
}

//====================================================================
// Function return PID of current task
int tpid() {
    uint64_t pid;
    __asm__ volatile(
        "mov $52, %%rax\n"
        "int $0x80\n"
        "mov %%rax, %0\n"
        : "=r" (pid)
        : 
        : "%rax"
    );

    return pid;
}

//====================================================================
// Function kill current task
void tkill() {
    __asm__ volatile(
        "mov $27, %%rax\n"
        "int %0\n"
        :
        : "i"(SYSCALL_INTERRUPT)
        : "rax"
    );
}

//====================================================================
//  Return date time from cmos in short*
//  ECX - pointer to array
//
//  data[0] - seconds
//  data[1] - minute
//  data[2] - hour
//  data[3] - day
//  data[4] - month
//  data[5] - year
void get_datetime(short* data) {
    __asm__ volatile(
        "mov $6, %%rax\n"
        "mov %0, %%rcx\n"
        "int %1\n"
        :
        : "r"(data), "i"(SYSCALL_INTERRUPT)
        : "rax", "rcx"
    );
}

//====================================================================
//  Allocate memory and return pointer
//  EBX - size
//  EAX - returned pointer to allocated memory
void* malloc(uint32_t size) {
    void* allocated_memory;
    __asm__ volatile(
        "mov $7, %%rax\n"
        "mov %1, %%rbx\n"
        "int $0x80\n"
        "mov %%rax, %0\n"
        : "=r" (allocated_memory)
        : "r" ((uint64_t)size)
        : "%rax", "%rbx"
    );
    
    return allocated_memory;
}

//====================================================================
//  Allocate memory and return pointer
//  EBX - v_addr
//  EAX - returned v_addr of page
void* mallocp(uint32_t v_addr) {
    void* allocated_v_addr;
    __asm__ volatile(
        "mov $35, %%rax\n"
        "mov %1, %%rbx\n"
        "int $0x80\n"
        "mov %%rax, %0\n"
        : "=r" (allocated_v_addr)
        : "r" ((uint64_t)v_addr)
        : "%rax", "%rbx"
    );
    
    return allocated_v_addr;
}

void* realloc(void* ptr, size_t size) {
    void* new_data = NULL;
    if (size) {
        if(!ptr) return malloc(size);

        new_data = malloc(size);
        if(new_data) {
            memcpy(new_data, ptr, size);
            free(ptr);
        }
    }

    return new_data;
}

void* calloc(size_t nelem, size_t elsize) {
    void* tgt = malloc(nelem * elsize);
    if (tgt != NULL) 
        memset(tgt, 0, nelem * elsize);

    return tgt;
}

void* clralloc(size_t size) {
    void* tgt = malloc(size);
    if (tgt != NULL) 
        memset(tgt, 0, size);

    return tgt;
}

//====================================================================
//  Free alocated memory
//  EBX - pointer to allocated memory
void free(void* ptr) {
    if (ptr == NULL) return;
    __asm__ volatile(
        "mov $8, %%rax\n"
        "mov %0, %%rbx\n"
        "int $0x80\n"
        :
        : "r" (ptr)
        : "%rax", "%rbx"
    );
}

//====================================================================
//  Free alocated memory
//  EBX - pointer to allocated memory
void freep(void* ptr) {
    if (ptr == NULL) return;
    __asm__ volatile(
        "mov $34, %%rax\n"
        "mov %0, %%rbx\n"
        "int $0x80\n"
        :
        : "r" (ptr)
        : "%rax", "%rbx"
    );
}

//====================================================================
// Function restart machine
void machine_restart() {
    __asm__ volatile(
        "mov $44, %%rax\n"
        "int %0\n"
        :
        : "i"(SYSCALL_INTERRUPT)
        : "%rax"
    );
}

void switch_disk(int index) {

}

//====================================================================
// Function that get FS info
//
// buffer[0] - mountpoint
// buffer[1] - name
// buffer[2] - fat type
// buffer[3] - total clusters
// buffer[4] - total sectors
// buffer[5] - bytes per_sector
// buffer[6] - sectors per cluster
// buffer[7] - fat size
void get_fs_info(uint32_t* buffer) {
     __asm__ volatile(
        "mov $45, %%rax\n"
        "mov %0, %%rbx\n"
        "int $0x80\n"
        :
        : "r"(buffer)
        : "%rax", "%rbx"
    );
}

//====================================================================
// Function add enviroment variable with value
// EBX - name
// ECX - value
void envar_add(char* name, char* value) {
     __asm__ volatile(
        "mov $53, %%rax\n"
        "mov %0, %%rbx\n"
        "mov %1, %%rcx\n"
        "int $0x80\n"
        :
        : "r"(name), "r"(value)
        : "%rax", "%rbx", "%rcx"
    );
}

//====================================================================
// Function check enviroment variable
// EBX - name
//
// values:
// -1 - nexists
// != -1 - exists
int envar_exists(char* name) {
    uint64_t value;
    __asm__ volatile(
        "mov $57, %%rax\n"
        "mov %1, %%rbx\n"
        "int $0x80\n"
        "mov %%rax, %0\n"
        : "=r" (value)
        : "r" (name)
        : "%rax", "%rbx"
    );
    
    return value;
}

//====================================================================
// Function set enviroment variable with value
// EBX - name
// ECX - value
void envar_set(char* name, char* value) {
     __asm__ volatile(
        "mov $54, %%rax\n"
        "mov %0, %%rbx\n"
        "mov %1, %%rcx\n"
        "int $0x80\n"
        :
        : "r"(name), "r"(value)
        : "%rax", "%rbx", "%rcx"
    );
}

//====================================================================
// Function get enviroment variable
// EBX - name
char* envar_get(char* name) {
    char* value;
    __asm__ volatile(
        "mov $55, %%rax\n"
        "mov %1, %%rbx\n"
        "int $0x80\n"
        "mov %%rax, %0\n"
        : "=r" (value)
        : "r" (name)
        : "%rax", "%rbx"
    );
    
    return value;
}

//====================================================================
// Function delete enviroment variable
// EBX - name
void envar_delete(char* name) {
     __asm__ volatile(
        "mov $56, %%rax\n"
        "mov %0, %%rbx\n"
        "int $0x80\n"
        :
        : "r"(name)
        : "%rax", "%rbx"
    );
}