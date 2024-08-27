global i386_gdt_load
i386_gdt_load:
    ; Save registers
    push rbx
    push rbp
    mov rbp, rsp                ; Set up stack frame

    ; Load GDT
    mov rax, [rbp + 16]         ; Load GDTR address from the stack
    lgdt [rax]                  ; Load the GDT

    ; Reload code segment
    mov rax, [rbp + 24]         ; Load code segment selector from the stack
    push rax
    push rax                    ; Push the code segment selector
    push rax                    ; Push return address (just for the example)
    
    ; Use a dummy return to simulate a segment switch (in real scenarios, you might want to use `jmp` instead)
    jmp short .reload_cs

.reload_cs:
    ; Reload data segments
    mov rax, [rbp + 32]         ; Load data segment selector from the stack
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Restore stack and registers
    mov rsp, rbp
    pop rbp
    pop rbx
    ret