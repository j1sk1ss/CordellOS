#include "../../include/isr.h"

static ISRHandler _isr_handlers[256];
static struct ELF32_symbols_desctiptor* _current_descriptor = NULL;

static const char* const _exceptions[] = {
    "DIVIDE BY ZERO",                 "DEBUG",
    "NON-MASKABLE INTERRUPT",         "BREAKPOINT",
    "OVERFLOW",                       "BOUND RANGE EXCEEDED",
    "INVALID OPCODE",                 "DEVICE NOT AVALIABLE",
    "DOUBLE FAULT",                   "COPROCESSOR SEGMENT OVERRUN",
    "INVALID TSS",                    "SEGMENT NOT PRESENT",
    "SS FAULT",                       "GENERAL PROTECTION FAULT",
    "PAGE FAULT", "",                 "X87 FLOATING-POINT EXCEPTION",
    "ALOGNMENT CHECK",                "MACHINE CHECK",
    "SIMD FLOACTING-POINT EXCEPTION", "VIRTUALIZATION EXCEPTION",
    "CONTROL PROTECTION EXCEPTION",   "", "", "", "", "", "",
    "HYPERVISOR INJECTION EXCEPTION", "VMM COMMUNICATION EXCEPTION",
    "SECURITY EXCEPTION", ""
};

void i386_ISR_initialize_gates();

void i386_isr_initialize() {
    i386_ISR_initialize_gates();
    for (int i = 0; i < 256; i++)
        i386_idt_enableGate(i);
}

void __attribute__((cdecl)) i386_isr_handler(struct Registers* regs) {
    if (regs->interrupt < 256) {
        if (_isr_handlers[regs->interrupt] != NULL) {
            _isr_handlers[regs->interrupt](regs);
            return;
        }

        kclrscr();

        if (regs->interrupt < SIZE(_exceptions) && _exceptions[regs->interrupt] != NULL) 
            kprintf("UNHANDLED EXCEPTION [%d] ['%s']\n", regs->interrupt, _exceptions[regs->interrupt]);
        else kprintf("UNHANDLED INTERRUPT! INTERRUPT: %d\n", regs->interrupt);
        
        kprintf("  eax=%u ebx=%u ecx=%u edx=%u esi=%u edi=%u\n",
                regs->eax, regs->ebx, regs->ecx, regs->edx, regs->esi, regs->edi);
        kprintf("  esp=%p ebp=%u eip=0x%p eflags=%u cs=%u ds=%u ss=%u\n",
                regs->esp, regs->ebp, regs->eip, regs->eflag, regs->cs, regs->ds, regs->ss);
        kprintf("  INTERRUPT=%u ERRORCODE=%u\n", regs->interrupt, regs->error);

        kprintf("\nSTACK TRACE:\n");
        i386_isr_interrupt_details(regs->eip, regs->ebp, regs->esp);

        kernel_panic("\nKERNEL PANIC");
    }
}

void i386_isr_interrupt_details(uint32_t eip, uint32_t ebp, uint32_t esp) {
    i386_isr_stack_trace_line(eip);
    uint32_t stack_highest_address = ((uint32_t)&esp + PAGE_SIZE - 4);
    while (ebp <= stack_highest_address && ebp >= ((uint32_t) &esp)) {
        eip = ((uint32_t*) ebp)[1];
        i386_isr_stack_trace_line(eip);
        ebp = *((uint32_t*)ebp);
    }
}

void i386_isr_stack_trace_line(uint32_t eip) {
  kprintf("[0x%x] : %s\n", eip, ELF_lookup_function((uint32_t)((uint32_t*)eip)));
}

void i386_isr_register_handler(int interrupt, ISRHandler handler) {
    _isr_handlers[interrupt] = handler;
    i386_idt_enableGate(interrupt);
}

void i386_isr_set_symdes(struct ELF32_symbols_desctiptor* desciptor) {
    _current_descriptor = desciptor;
}