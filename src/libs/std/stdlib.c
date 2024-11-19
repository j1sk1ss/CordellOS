#include "../include/stdlib.h"


//====================================================================
// Function start task at address with entered name
// EBX - name
// ECX - address 
void tstart(char* name, uint32_t address, uint32_t delay) {
    __asm__ volatile(
        "movl $26, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "movl %2, %%edx\n"
        "int %3\n"
        :
        : "r"(name), "r"(address), "r"(delay), "i"(SYSCALL_INTERRUPT)
        : "eax", "ebx", "ecx"
    );
}

//====================================================================
// Function return PID of current task
int tpid() {
    int pid;
    __asm__ volatile(
        "movl $52, %%eax\n"
        "int $0x80\n"
        "movl %%eax, %0\n"
        : "=r" (pid)
        : 
        : "%eax"
    );

    return pid;
}

//====================================================================
// Function kill current task
void tkill() {
    __asm__ volatile(
        "movl $27, %%eax\n"
        "int %0\n"
        :
        : "i"(SYSCALL_INTERRUPT)
        : "eax"
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
        "movl $6, %%eax\n"
        "movl %0, %%ecx\n"
        "int %1\n"
        :
        : "r"(data), "i"(SYSCALL_INTERRUPT)
        : "eax", "ecx"
    );
}

//====================================================================
//  Allocate memory and return pointer
//  EBX - size
//  EAX - returned pointer to allocated memory
void* malloc(uint32_t size) {
    void* allocated_memory;
    __asm__ volatile(
        "movl $7, %%eax\n"
        "movl %1, %%ebx\n"
        "int $0x80\n"
        "movl %%eax, %0\n"
        : "=r" (allocated_memory)
        : "r" (size)
        : "%eax", "%ebx"
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
        "movl $35, %%eax\n"
        "movl %1, %%ebx\n"
        "int $0x80\n"
        "movl %%eax, %0\n"
        : "=r" (allocated_v_addr)
        : "r" (v_addr)
        : "%eax", "%ebx"
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
        "movl $8, %%eax\n"
        "movl %0, %%ebx\n"
        "int $0x80\n"
        :
        : "r" (ptr)
        : "%eax", "%ebx"
    );
}

//====================================================================
//  Free alocated memory
//  EBX - pointer to allocated memory
void freep(void* ptr) {
    if (ptr == NULL) return;
    __asm__ volatile(
        "movl $34, %%eax\n"
        "movl %0, %%ebx\n"
        "int $0x80\n"
        :
        : "r" (ptr)
        : "%eax", "%ebx"
    );
}

//====================================================================
// Function restart machine
void machine_restart() {
    __asm__ volatile(
        "movl $44, %%eax\n"
        "int %0\n"
        :
        : "i"(SYSCALL_INTERRUPT)
        : "%eax"
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
        "movl $45, %%eax\n"
        "movl %0, %%ebx\n"
        "int $0x80\n"
        :
        : "r"(buffer)
        : "%eax", "%ebx"
    );
}

//====================================================================
// Function add enviroment variable with value
// EBX - name
// ECX - value
void envar_add(char* name, char* value) {
     __asm__ volatile(
        "movl $53, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "int $0x80\n"
        :
        : "r"(name), "r"(value)
        : "%eax", "%ebx", "%ecx"
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
    int value;
    __asm__ volatile(
        "movl $57, %%eax\n"
        "movl %1, %%ebx\n"
        "int $0x80\n"
        "movl %%eax, %0\n"
        : "=r" (value)
        : "r" (name)
        : "%eax", "%ebx"
    );
    
    return value;
}

//====================================================================
// Function set enviroment variable with value
// EBX - name
// ECX - value
void envar_set(char* name, char* value) {
     __asm__ volatile(
        "movl $54, %%eax\n"
        "movl %0, %%ebx\n"
        "movl %1, %%ecx\n"
        "int $0x80\n"
        :
        : "r"(name), "r"(value)
        : "%eax", "%ebx", "%ecx"
    );
}

//====================================================================
// Function get enviroment variable
// EBX - name
char* envar_get(char* name) {
    char* value;
    __asm__ volatile(
        "movl $55, %%eax\n"
        "movl %1, %%ebx\n"
        "int $0x80\n"
        "movl %%eax, %0\n"
        : "=r" (value)
        : "r" (name)
        : "%eax", "%ebx"
    );
    
    return value;
}

//====================================================================
// Function delete enviroment variable
// EBX - name
void envar_delete(char* name) {
     __asm__ volatile(
        "movl $56, %%eax\n"
        "movl %0, %%ebx\n"
        "int $0x80\n"
        :
        : "r"(name)
        : "%eax", "%ebx"
    );
}

void switch2user() {
    __asm__ volatile(
        "movl $60, %%eax\n"
        "int $0x80\n"
        :
        :
        : "eax"
    );
}