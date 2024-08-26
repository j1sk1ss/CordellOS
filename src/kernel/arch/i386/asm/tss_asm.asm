global TSS_flush
TSS_flush:
    mov ax, 0x28
    ltr ax
    ret