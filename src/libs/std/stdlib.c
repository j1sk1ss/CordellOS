#include "../include/stdlib.h"


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

int tpid() {
    int pid = -1;
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

void tkill() {
    __asm__ volatile(
        "movl $27, %%eax\n"
        "int %0\n"
        :
        : "i"(SYSCALL_INTERRUPT)
        : "eax"
    );
}

void get_datetime(DateInfo_t* info) {
    __asm__ volatile(
        "movl $6, %%eax\n"
        "movl %0, %%ecx\n"
        "int %1\n"
        :
        : "r"(info), "i"(SYSCALL_INTERRUPT)
        : "eax", "ecx"
    );
}

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

void* mallocp(uint32_t v_addr) {
    void* allocated_v_addr = NULL;
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

void switch2user() {
    __asm__ volatile(
        "movl $60, %%eax\n"
        "int $0x80\n"
        :
        :
        : "eax"
    );
}