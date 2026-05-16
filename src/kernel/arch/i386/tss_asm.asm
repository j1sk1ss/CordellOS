global TSS_flush ; TODO: Replace with tss.cpl
TSS_flush:
    mov ax, 0x28
    ltr ax
    ret