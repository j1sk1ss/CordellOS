#include "../include/tasking.h"

// TODO: make process that clean another processes

TaskManager taskManager = { // Task manager placed in kernel space
	.currentTask = -1,
	.tasksCount = 0,
	.tasking = false
};

//==================
// Task interaction
//==================

	void TASK_start_tasking() {
		if (taskManager.tasksCount <= 0) return;
		i386_disableInterrupts();

		// Set task page directory
		VMM_set_directory(taskManager.tasks[0]->page_directory);

		// Load stack to esp
		asm ("mov %%eax, %%esp": :"a"(taskManager.tasks[0]->cpuState->esp));

		// Set values from stack
		asm ("pop %gs");
		asm ("pop %fs");
		asm ("pop %es");
		asm ("pop %ds");
		asm ("pop %ebp");
		asm ("pop %edi");
		asm ("pop %esi");
		asm ("pop %edx");
		asm ("pop %ecx");
		asm ("pop %ebx");
		asm ("pop %eax");
		
		// Set multitasking on
		taskManager.currentTask = 0;
		taskManager.tasking = true;

		i386_enableInterrupts();

		// Return from interrupt and execute process
		asm ("iret");		
	}

	void TASK_stop_tasking() {
		taskManager.tasking = false;
	}

	void TASK_continue_tasking() {
		taskManager.tasking = true;
	}

//==================
// Task interaction
//==================
// Task manager
//==================

	void i386_task_init() {
		for (int i = 0; i < TASKS_MAX; i++) taskManager.tasks[i] = NULL;
		i386_irq_registerHandler(0, TASK_task_switch);
	}

	Task* TASK_create_task(char* pname, uint32_t address, int type, int priority) {

		//=============================
		// Allocate memory for new task
		//=============================

			// Allocate memory for new task body
			Task* task     = (Task*)_kmalloc(sizeof(Task));
			task->cpuState = (struct Registers*)_kmalloc(sizeof(struct Registers));

			//=============================
			// Find new pid
			//=============================

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
					_kfree(task->cpuState);
					_kfree(task);
				}

			//=============================
			// Find new pid
			//=============================
			// Allocate data for stack
			//=============================

				// Create empty pd and fill it by tables from kernel pd
				if (type == USER) task->page_directory = _mkupdir();
				else if (type == KERNEL) task->page_directory = _mkpdir();

				_copy_dir2dir(kernel_page_directory, task->page_directory);
                VMM_set_directory(task->page_directory);
				
				// Allocate page in pd, link it to v_addr
				type == USER ? _umallocp(0x00C00000) : _kmallocp(0x00C00000);
                memset((void*)0x00C00000, 0, PAGE_SIZE);
                
				// Set stack pointer to allocated region
				task->cpuState->esp     = VMM_virtual2physical((void*)0x00C00000);
				task->virtual_address   = task->cpuState->esp;
				uint32_t* stack_pointer = (uint32_t*)(task->cpuState->esp + PAGE_SIZE);

			//=============================
			// Allocate data for stack
			//=============================

		//=============================
		// Allocate memory for new task
		//=============================
		// Fill EBP stack
		//=============================

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

		//=============================
		// Fill EBP stack
		//=============================
		// Fill registers
		//=============================

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

		//=============================
		// Fill registers
		//=============================

		VMM_set_directory(kernel_page_directory);

        // v_addr += PAGE_SIZE * 2;
		return task;
	}

	void destroy_task(Task* task) {
		page_directory* task_pagedir = (page_directory*)VMM_virtual2physical(task->page_directory);
		VMM_set_directory(kernel_page_directory);
		_free_pdir(task_pagedir);
		_kfree(task->cpuState);
		_kfree(task);
	}

	Task* get_task(int pid) {
		for (int i = 0; i < TASKS_MAX; i++) 
			if (taskManager.tasks[i]->pid == pid) return taskManager.tasks[i];

		return NULL;
	}

	void __kill() { // TODO: complete multitask disabling when tasks == 0
		TASK_stop_tasking();
		_kill(taskManager.tasks[taskManager.currentTask]->pid);
		TASK_continue_tasking();
	}

	void _kill(int pid) {
		if (pid >= 0 && pid < taskManager.tasksCount) 
			for (int task = 0; task < taskManager.tasksCount; task++) 
				if (taskManager.tasks[task]->pid == pid) {
					taskManager.tasks[task]->state = PROCESS_STATE_DEAD;
					break;
				}
	}

	int _TASK_add_task(Task* task) {
		if (taskManager.tasksCount >= 256) return -1;
		taskManager.tasks[taskManager.tasksCount++] = task;
		
		return task->pid;
	}

	int TASK_add_task(Task* task) {
		TASK_stop_tasking();
		_TASK_add_task(task);
		TASK_continue_tasking();

		return task->pid;
	}

	void TASK_task_switch(struct Registers* regs) {
		if (!taskManager.tasking) return;
		
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

        // Save current state and current page directory
        memcpy(task->cpuState, regs, sizeof(struct Registers));
        if (task->page_directory != current_page_directory)
            task->page_directory = current_page_directory;

        // Select next task
		if (++taskManager.currentTask >= taskManager.tasksCount)
				taskManager.currentTask = 0;

        // If next task finished / broken and something like that, find next
		Task* new_task = taskManager.tasks[taskManager.currentTask];
		while (new_task->state != PROCESS_STATE_ALIVE || new_task == NULL) {
			if (++taskManager.currentTask >= taskManager.tasksCount) taskManager.currentTask = 0;
			new_task = taskManager.tasks[taskManager.currentTask];
		}

        // Load task CPU state and page directory
        if (new_task->cpuState == NULL) return;
        memcpy(regs, new_task->cpuState, sizeof(struct Registers));
        if (new_task->page_directory != NULL)
            if (new_task->page_directory != task->page_directory)
		        VMM_set_directory(new_task->page_directory);

        i386_enableInterrupts();
	}

//==================
// Task manager
//==================