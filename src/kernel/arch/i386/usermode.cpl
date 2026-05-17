@[nosection] 
@[naked]
glob function i386_switch2user(u32 eip, u32 esp) -> i0 {
    asm(eip, esp) {
        "cli",
        "mov ax, 0x23",
        "mov ds, ax",
        "mov es, ax",
        "mov fs, ax",
        "mov gs, ax",
        "push 0x23",
        "push %1",
        "push 0x202",
        "push 0x1b",
        "push %0",
        "iretd"
    }
}