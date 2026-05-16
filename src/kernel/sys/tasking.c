#include <tasking.h>

// TODO: make process that clean another processes

TaskManager taskManager = { // Task manager placed in kernel space
	.currentTask = -1,
	.tasksCount  = 0,
	.tasking     = 0
};

void TASK_start_tasking() {
	if (taskManager.tasksCount <= 0 || taskManager.tasks[0] == NULL || taskManager.tasks[0]->cpuState == NULL) return;
	i386_disableInterrupts();

	taskManager.currentTask = 0;
	taskManager.tasking     = 1;

	// Set task page directory
	VMM_set_directory(taskManager.tasks[0]->page_directory);

	uint32_t task_esp = taskManager.tasks[0]->cpuState->esp;
	asm (
		"mov %0, %%esp\n"
		"pop %%gs\n"
		"pop %%fs\n"
		"pop %%es\n"
		"pop %%ds\n"
		"pop %%ebp\n"
		"pop %%edi\n"
		"pop %%esi\n"
		"pop %%edx\n"
		"pop %%ecx\n"
		"pop %%ebx\n"
		"pop %%eax\n"
		"sti\n"
		"iret\n"
		:
		: "r"(task_esp)
		: "memory"
	);
}

void TASK_stop_tasking() {
	taskManager.tasking = 0;
}

void TASK_continue_tasking() {
	if (taskManager.currentTask >= 0 && taskManager.tasksCount > 0) {
		taskManager.tasking = 1;
	}
}

void i386_task_init() {
	for (int i = 0; i < TASKS_MAX; i++) taskManager.tasks[i] = NULL;
	i386_irq_registerHandler(0, TASK_task_switch);
}

Task* TASK_create_task(char* pname, uint32_t address, int type, int priority) {
	// Allocate memory for new task body
	Task* task     = (Task*)ALC_malloc(sizeof(Task), KERNEL);
	if (task == NULL) return NULL;

	task->cpuState = (struct Registers*)ALC_malloc(sizeof(struct Registers), KERNEL);
	if (task->cpuState == NULL) {
		ALC_free(task, KERNEL);
		return NULL;
	}

	task->space    = type;

	// Find free PID
	task->state = PROCESS_STATE_ALIVE;
	task->pid   = -1;
	task->name  = pname;
	task->page_directory = NULL;

	task->delay     = priority;
	task->exec_time = 0;

	if (taskManager.tasksCount <= 0) task->pid = 0;
	for (int pid = 0; pid < taskManager.tasksCount; pid++) {
		for (int id = 0; id < TASKS_MAX; id++) 
			if (taskManager.tasks[pid]->pid != id) {
				task->pid = id;
				break;
			}

		if (task->pid != -1) break;
	}

	if (task->pid == -1) {
		ALC_free(task->cpuState, KERNEL);
		ALC_free(task, KERNEL);
		return NULL;
	}

	// Create empty pd and fill it by tables from kernel pd
	task->page_directory = VMM_mkpdir();
	if (task->page_directory == NULL) {
		ALC_free(task->cpuState, KERNEL);
		ALC_free(task, KERNEL);
		return NULL;
	}

	VMM_copy_dir2dir(VMM_get_dirs()->kern, task->page_directory);
	VMM_set_directory(task->page_directory);

	// Allocate page in pd, link it to v_addr
	if (ALC_mallocp(TASK_VIRT_ADDRESS, type) != 1) {
		VMM_set_directory(VMM_get_dirs()->kern);
		VMM_free_pdir(task->page_directory);
		ALC_free(task->cpuState, KERNEL);
		ALC_free(task, KERNEL);
		return NULL;
	}
	memset((void*)TASK_VIRT_ADDRESS, 0, PAGE_SIZE);
	
	// Set stack pointer to allocated region
	task->cpuState->esp     = VMM_virtual2physical((void*)TASK_VIRT_ADDRESS);
	task->virtual_address   = task->cpuState->esp;
	uint32_t* stack_pointer = (uint32_t*)(task->cpuState->esp + PAGE_SIZE);

	*--stack_pointer = 0x00000202; // eflags
	*--stack_pointer = 0x8; // cs
	*--stack_pointer = (uint32_t)address; // eip

	*--stack_pointer = 0; // eax
	*--stack_pointer = 0; // ebx
	*--stack_pointer = 0; // ecx
	*--stack_pointer = 0; // edx
	*--stack_pointer = 0; // esi
	*--stack_pointer = 0; // edi

	*--stack_pointer = task->cpuState->esp + PAGE_SIZE; //ebp

	*--stack_pointer = 0x10; // ds
	*--stack_pointer = 0x10; // fs
	*--stack_pointer = 0x10; // es
	*--stack_pointer = 0x10; // gs

	task->cpuState->eflag = 0x00000202;
	task->cpuState->cs    = 0x8;
	task->cpuState->eip   = (uint32_t)address;

	task->cpuState->eax = 0;
	task->cpuState->ebx = 0;
	task->cpuState->ecx = 0;
	task->cpuState->edx = 0;
	task->cpuState->esi = 0;
	task->cpuState->edi = 0;

	task->cpuState->ebp = task->cpuState->esp + PAGE_SIZE;
	task->cpuState->esp = (uint32_t)stack_pointer;

	VMM_set_directory(VMM_get_dirs()->kern);
	return task;
}

void _destroy_task(Task* task) {
	pdir_t* task_pagedir = (pdir_t*)VMM_virtual2physical(task->page_directory);
	VMM_set_directory(VMM_get_dirs()->kern);
	VMM_free_pdir(task_pagedir);
	ALC_free(task->cpuState, KERNEL);
	ALC_free(task, KERNEL);
}

Task* _get_task(int pid) {
	for (int i = 0; i < TASKS_MAX; i++) {
		if (taskManager.tasks[i] != NULL && taskManager.tasks[i]->pid == pid) return taskManager.tasks[i];
	}
	return NULL;
}

void __kill() { // TODO: complete multitask disabling when tasks == 0
	if (taskManager.currentTask < 0 || taskManager.currentTask >= taskManager.tasksCount) return;
	if (taskManager.tasks[taskManager.currentTask] == NULL) return;

	TASK_stop_tasking();
	_kill(taskManager.tasks[taskManager.currentTask]->pid);
	TASK_continue_tasking();
}

void _kill(int pid) {
	if (pid >= 0 && pid < taskManager.tasksCount) {
		for (int task = 0; task < taskManager.tasksCount; task++) {
			if (taskManager.tasks[task]->pid == pid) {
				taskManager.tasks[task]->state = PROCESS_STATE_DEAD;
				break;
			}
		}
	}
}

int _add_task(Task* task) {
	if (task == NULL || taskManager.tasksCount >= TASKS_MAX) return -1;
	taskManager.tasks[taskManager.tasksCount++] = task;
	
	return task->pid;
}

int TASK_add_task(Task* task) {
	bool was_tasking = taskManager.tasking;
	TASK_stop_tasking();
	int pid = _add_task(task);
	taskManager.tasking = was_tasking;

	return pid;
}

void TASK_task_switch(struct Registers* regs) {
	if (!taskManager.tasking || taskManager.currentTask < 0 || taskManager.currentTask >= taskManager.tasksCount) return;
	
	// Get current task
	Task* task = taskManager.tasks[taskManager.currentTask];
	if (task == NULL) return;
	if (task->cpuState == NULL) return;

	i386_disableInterrupts();

	if (task->exec_time > task->delay) task->exec_time = 0;
	else {
		task->exec_time++;
		i386_enableInterrupts();
		return;
	}

	memcpy(task->cpuState, regs, sizeof(struct Registers));
	if (task->page_directory != VMM_get_dirs()->curr) {
		task->page_directory = VMM_get_dirs()->curr;
	}

	if (++taskManager.currentTask >= taskManager.tasksCount) {
		taskManager.currentTask = 0;
	}

	// If next task finished / broken and something like that, find next
	Task* new_task = taskManager.tasks[taskManager.currentTask];
	int scanned_tasks = 0;
	while ((new_task == NULL || new_task->state != PROCESS_STATE_ALIVE) && scanned_tasks < taskManager.tasksCount) {
		if (++taskManager.currentTask >= taskManager.tasksCount) taskManager.currentTask = 0;
		new_task = taskManager.tasks[taskManager.currentTask];
		scanned_tasks++;
	}

	if (new_task == NULL || new_task->state != PROCESS_STATE_ALIVE) {
		taskManager.tasking = 0;
		i386_enableInterrupts();
		return;
	}

	// Load task CPU state and page directory
	if (new_task->cpuState == NULL) {
		taskManager.tasking = 0;
		i386_enableInterrupts();
		return;
	}
	memcpy(regs, new_task->cpuState, sizeof(struct Registers));
	if (new_task->page_directory != NULL)
		if (new_task->page_directory != task->page_directory)
			VMM_set_directory(new_task->page_directory);

	i386_enableInterrupts();
}
