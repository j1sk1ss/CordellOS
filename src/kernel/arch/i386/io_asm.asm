;
;   Cordell OS base code
;   Cordell OS is simple example of Operation system from scratch
;

; Output byte to port
;
; void __attribute__((cdecl)) i386_outb(uint16_t port, uint8_t data);
;
global i386_panic
i386_panic:
    cli
    hlt

global i386_enable_interrupts
i386_enable_interrupts:
    sti
    ret

global i386_disable_interrupts
i386_disable_interrupts:
    cli
    ret
