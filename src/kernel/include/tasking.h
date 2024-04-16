#ifndef TASKING_H
#define TASKING_H


#include <stdint.h>
#include <stdbool.h>
#include <memory.h>
#include <stdlib.h>

#include "x86.h"
#include "stdio.h"
#include "irq.h"
#include "virt_manager.h"
#include "allocator.h"

#include "../util/binary.h"


#define TASKS_MAX             10
#define TASK_SWITCH_DELAY     10

#define PROCESS_STATE_ALIVE   0 
#define PROCESS_STATE_STRANGE 1
#define PROCESS_STATE_DEAD    2

#define START_PROCESS(name, address, type, delay)    TASK_add_task(TASK_create_task(name, address, type, delay))


typedef struct {
    struct Registers* cpuState;

    char* name;
    int state;
    int pid;

    uint8_t delay;
    uint32_t exec_time;

    uint32_t virtual_address;
    page_directory* page_directory;

} Task;

typedef struct {
    Task* tasks[TASKS_MAX];
    int tasksCount;
    int currentTask;
    
} TaskManager;


extern TaskManager taskManager;
extern bool tasking;


void i386_task_init();

void TASK_start_tasking();
void TASK_stop_tasking();
void TASK_continue_tasking();

Task* TASK_create_task(char* pname, uint32_t address, int type, int priority);
int TASK_add_task(Task* task);

void TASK_task_switch(struct Registers* state);

void __kill();
void _kill(int pid);


#endif // TASKING_H