#ifndef STDLIB_H_
#define STDLIB_H_

#include <stdint.h>

#include "memory.h"


#define SYSCALL_INTERRUPT   0x80
#define PAGE_SIZE           4096


void tstart(char* name, uint32_t address);
int tpid();
void tkill();

void get_datetime(short* data);

void* malloc(uint32_t size);
void* mallocp(uint32_t v_addr);
void* realloc(void* ptr, size_t size);
void* calloc(size_t nelem, size_t elsize);
void* clralloc(size_t size);
void free(void* ptr);
void freep(void* ptr);

void machine_restart();
void switch_disk(int index);

void get_fs_info(uint32_t* buffer);

void envar_add(char* name, char* value);
int envar_exists(char* name);
void envar_set(char* name, char* value);
char* envar_get(char* name);
void envar_delete(char* name);

#endif