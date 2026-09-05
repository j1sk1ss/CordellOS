@[align(1)]
container framebuffer {
    usize mode; 
    usize x;
    usize y;
    usize bitness;
}

@[align(1)]
container mb_header {
    usize       magic;
    usize       flags;
    usize       check;
    arr         padding[5, usize];
    framebuffer fb;
}

@[section(".multiboot", 4)]
@[volatile]
glob mb_header _header = { 
    :/ magic   /: 0x1BADB002, 
    :/ flags   /: 7, 
    :/ check   /: 3830599671, 
    :/ padding /: 0, 0, 0, 0, 0, 
    :/ mode    /: 0, 
    :/ x       /: 640, 
    :/ y       /: 480, 
    :/ bitness /: 32 
};

container stack {
    arr padding[16384, u8];
}

@[section(".bss", 16)] 
glob stack _stack;

@[section(".text")] @[abi] 
function kernel_main(usize mb_info, usize magic, usize esp) -> i0;

@[section(".text")]
@[entry("_start")]
@[naked]
function main() -> i0 {
#define EAX 16
#define EBX 17
#define ESI 20
#define EDI 21
    @[popreg(EAX)] @[register(EDI)] usize magic;
    @[popreg(EBX)] @[register(ESI)] usize mb_info;

    asm(ref _stack + sizeof(stack)) {
        "mov esp, %0",
        "cli",
        "xor ebp, ebp"
    }

    usize stack_top = 0;
    asm(stack_top) {
        "mov %0, esp"
    }

    kernel_main(mb_info, magic, stack_top);

    asm() {
        ".hang: hlt",
        "jmp .hang"
    }
}