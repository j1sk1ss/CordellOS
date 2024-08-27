global i386_switch2user
i386_switch2user:
    ; Disable interrupts
    cli

    ; Set up stack frame for user mode
    mov rax, 0x23               ; User data segment selector (assuming 0x23 is correct)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    mov rax, rsp                ; Move current stack pointer to rax
    push rax                    ; Push current stack pointer (for iretq)

    push 0x202                  ; Push RFLAGS with interrupts enabled (0x202 is IF flag set)

    mov rax, 0x1B               ; User code segment selector (assuming 0x1B is correct)
    push rax                    ; Push user code segment

    lea rax, [user_start]       ; Load the address of user_start into rax
    push rax                    ; Push the address of user_start

    ; IRETQ to user mode
    iretq

user_start:
    ; Code to execute in user mode
    nop                         ; No operation (placeholder for user mode code)
    ret                         ; Return (this will return to kernel mode, depending on setup)
