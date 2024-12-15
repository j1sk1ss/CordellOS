#include "../../include/tss.h"


static tss_entry_t kernel_tss;


void TSS_init(uint32_t idx, uint32_t kss, uint32_t kesp) {
    uint32_t base = (uint32_t)&kernel_tss;
    GDT_set_entry(idx, base, base + sizeof(tss_entry_t), 0xE9, 0);
    memset((void*)&kernel_tss, 0, sizeof(tss_entry_t));

    kernel_tss.ss0  = kss;
    kernel_tss.esp0 = kesp;
    kernel_tss.cs   = 0x0b;
    kernel_tss.ds   = 0x13;
    kernel_tss.es   = 0x13;
    kernel_tss.fs   = 0x13;
    kernel_tss.gs   = 0x13;
    kernel_tss.ss   = 0x13;

    TSS_flush();
}

// kss - kernel stack segment
// kesp - kernel stack pointer
void TSS_set_stack(uint32_t kss, uint32_t kesp) {
    kernel_tss.ss0  = kss;
    kernel_tss.esp0 = kesp;
}