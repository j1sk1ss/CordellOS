global i386_switch2user
i386_switch2user:
    cli

    ; Set user data (0x23)
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; setup stack: SS, ESP, EFLAGS, CS, EIP
    mov eax, [esp]
    push 0x23        ; SS (user data selector)
    push user_stack_top
    pushfd           ; EFLAGS
    push 0x1B        ; CS (user code selector)
    push eax         ; EIP

    iretd            ; Entry to userspace

user_stack:
    resb 4096
user_stack_top:
