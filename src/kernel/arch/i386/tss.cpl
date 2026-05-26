extern function memset(ptr i0 d, u8 v, u32 n) -> ptr i0;
extern function GDT_set_entry(i32 index, i32 base, i32 limit, u8 access, u8 flags) -> i0;

@[align(1)]
container tss_entry {
    u32 prevTss;
    u32 esp0;
    u32 ss0;
    u32 esp1;
    u32 ss1;
    u32 esp2;
    u32 ss2;
    u32 cr3;
    u32 eip;
    u32 eflags;
    u32 eax;
    u32 ecx;
    u32 edx;
    u32 ebx;
    u32 esp;
    u32 ebp;
    u32 esi;
    u32 edi;
    u32 es;
    u32 cs;
    u32 ss;
    u32 ds;
    u32 fs;
    u32 gs;
    u32 ldt;
    u16 trap;
    u16 iomap;

    @[self]
    function init(ptr tss_entry self) -> i0 {
        memset(self, 0, sizeof(tss_entry));
    }
}

@[section(".bss")]
glob tss_entry _kernel_tss;

@[nosection]
function _tss_flush() -> i0 {
    asm() {
        "mov ax, 0x28",
        "ltr ax"
    }
}

glob function TSS_init(u32 idx, u32 kss, u32 kesp) -> i0 {
    u32 base = (ref _kernel_tss) as u32;
    GDT_set_entry(idx, base, sizeof(tss_entry) as i32 - 1, 137 as i32, 0);
    _kernel_tss.init();

    _kernel_tss.ss0  = kss;
    _kernel_tss.esp0 = kesp;
    _kernel_tss.cs   = 0x0b;
    _kernel_tss.ds   = 0x13;
    _kernel_tss.es   = 0x13;
    _kernel_tss.fs   = 0x13;
    _kernel_tss.gs   = 0x13;
    _kernel_tss.ss   = 0x13;

    _tss_flush();
}

glob function TSS_set_stack(u32 kss, u32 kesp) -> i0 {
    _kernel_tss.ss0  = kss;
    _kernel_tss.esp0 = kesp;
}

