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
    mov eax, [esp + 4]
    mov edx, [esp + 8]
    push 0x23        ; SS (user data selector)
    push edx         ; ESP
    push 0x202       ; EFLAGS, IF=1
    push 0x1B        ; CS (user code selector)
    push eax         ; EIP

    iretd            ; Entry to userspace
