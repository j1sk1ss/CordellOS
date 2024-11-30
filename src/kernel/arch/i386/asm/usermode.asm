; from https://github.com/szhou42/osdev/blob/master

global i386_switch2user
i386_switch2user:
    cli

    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push 0x23
    push esp
    pushfd
    
    push 0x1b
    lea eax, [user_start]
    push eax
    
    iretd

; Create page fault
user_start:
    add esp, 4
    ret