#ifndef STDLIB_H_
#define STDLIB_H_

#include <stdint.h>

#include "memory.h"


#define SYSCALL_INTERRUPT   0x80
#define PAGE_SIZE           4096


typedef struct {
    uint8_t second;
    uint8_t minute;
    uint8_t hour;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} DateInfo_t;


void tstart(char* name, uint32_t address, uint32_t delay);
int tpid();
void tkill();

void get_datetime(DateInfo_t* info);

void* malloc(uint32_t size);
void* mallocp(uint32_t v_addr);
void* realloc(void* ptr, size_t size);
void* calloc(size_t nelem, size_t elsize);
void* clralloc(size_t size);
void free(void* ptr);
void freep(void* ptr);

void machine_restart();
void switch_disk(int index);

void switch2user();

#endif