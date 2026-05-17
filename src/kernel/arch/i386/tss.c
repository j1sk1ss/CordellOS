#include <arch/i386/tss.h>

static tss_entry_t _kernel_tss = { 0 };

void TSS_init(uint32_t idx, uint32_t kss, uint32_t kesp) {
    uint32_t base = (uint32_t)&_kernel_tss;
    GDT_set_entry(idx, base, sizeof(tss_entry_t) - 1, GDT_ACCESS_DISCRIPTOR_TSS, 0x00);
    memset((void*)&_kernel_tss, 0, sizeof(tss_entry_t));
    _kernel_tss.ss0  = kss;
    _kernel_tss.esp0 = kesp;
    _kernel_tss.cs   = 0x0b;
    _kernel_tss.ds   = 0x13;
    _kernel_tss.es   = 0x13;
    _kernel_tss.fs   = 0x13;
    _kernel_tss.gs   = 0x13;
    _kernel_tss.ss   = 0x13;
    TSS_flush();
}

// kss - kernel stack segment
// kesp - kernel stack pointer
void TSS_set_stack(uint32_t kss, uint32_t kesp) {
    _kernel_tss.ss0  = kss;
    _kernel_tss.esp0 = kesp;
}