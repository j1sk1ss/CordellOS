ENTRY(_start)
OUTPUT_FORMAT("elf32-i386")

SECTIONS {
    . = 1M;

    .text BLOCK(4K) : ALIGN(4K) {
        *(.multiboot)
        *(.text)
        __symbol_start__ = .;
        *(.dynsm)
    }

    .rodata BLOCK(4K) : ALIGN(4K) {
        *(.rodata)
        *(.shrstrtab)
    }

    .data BLOCK(4K) : ALIGN(4K) {
        *(.data)
    }

    .bss BLOCK(4K) : ALIGN(4K) {
        *(COMMON)
        *(.bss)
        *(.bootstrap_stack)
    }
}