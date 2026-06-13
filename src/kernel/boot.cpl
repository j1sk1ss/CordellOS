@[align(1)]
container framebuffer {
    u32 mode; 
    u32 x;
    u32 y;
    u32 bitness;

}

@[align(1)]
container mb_header {
    u32         magic;
    u32         flags;
    u32         check;
    arr         padding[5, u32];
    framebuffer fb;
}

@[section(".multiboot", 4)]
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

@[section(".text")] function kernel_main(u32 mb_info, u32 magic, u32 esp) -> i0;

@[section(".text")]
@[entry("_start")]
@[naked]
function main() -> i0 {
    @[register(4)] u32 magic;
    @[register(5)] u32 mb_info;
    asm(magic, mb_info) {
        "mov %0, eax", :/ save magic /:
        "mov %1, ebx"  :/ save multiboot info /:
    }

    asm(ref _stack + sizeof(stack)) {
        "mov esp, %0",
        "cli",
        "xor ebp, ebp"
    }

    u32 stack_top = 0;
    asm(stack_top) {
        "mov %0, esp"
    }

    kernel_main(mb_info, magic, stack_top);

    asm() {
        ".hang: hlt",
        "jmp .hang"
    }
}