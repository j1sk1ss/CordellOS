[bits 32]

extern i386_isr_handler


%macro ISR_NOERRORCODE 1

global i386_ISR%1:
 i386_ISR%1:
    push 0                  ; pusg dummy error code
    push %1                 ; push interrupt number
    jmp isr_common

%endmacro

%macro ISR_ERRORCODE 1

global i386_ISR%1:
 i386_ISR%1:
    push %1                 ; push interrupt number
    jmp isr_common

%endmacro

%include "arch/drivers/isrs_gen.inc"

isr_common:
    pusha

    xor eax, eax
    mov ax, ds
    push eax

    mov ax, 0x10             ; uses kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp                 ; pass pointer stack to C
    call i386_isr_handler
    add esp, 4

    pop eax                  ; restore old segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    add esp, 8               ; remove error code & interrupt number

    iret                     ; will pop : cs, eip, eflags, ss, esp
