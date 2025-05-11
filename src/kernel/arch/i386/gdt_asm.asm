[bits 32]

global i386_gdt_load
i386_gdt_load:
    push ebp                    ; save old frame
    mov ebp, esp                ; init new call frame

    ; load gdt
    mov eax, [ebp + 8]
    lgdt [eax]                  ; Loads the values in the source operand into the global descriptor
                                ; table register (GDTR) or the interrupt descriptor table register (IDTR).
                                ; The source operand specifies a 6-byte memory location that contains the
                                ; base address (a linear address) and the limit (size of table in bytes) of
                                ; the global descriptor table (GDT) or the interrupt descriptor table (IDT).
                                ; If operand-size attribute is 32 bits, a 16-bit limit (lower 2 bytes of the
                                ; 6-byte data operand) and a 32-bit base address (upper 4 bytes of the data operand)
                                ; are loaded into the register. If the operand-size attribute is 16 bits, a 16-bit
                                ; limit (lower 2 bytes) and a 24-bit base address (third, fourth, and fifth byte)
                                ; are loaded. Here, the high-order byte of the operand is not used and the high-order
                                ; byte of the base address in the GDTR or IDTR is filled with zeros.

    ; reload code segment
    mov eax, [ebp + 12]
    push eax
    push .reload_cs
    retf

.reload_cs:
    ; reload data segments
    mov ax, [ebp + 16]

    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, ebp                ; restore data
    pop ebp
    ret