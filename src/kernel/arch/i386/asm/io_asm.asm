;
;   Cordell OS base code
;   Cordell OS is simple example of Operation system from scratch
;

; Output byte to port
;
; void  i386_outb(uint16_t port, uint8_t data);
;
global i386_panic
i386_panic:
    cli
    hlt

global i386_enableInterrupts
i386_enableInterrupts:
    sti
    ret

global i386_disableInterrupts
i386_disableInterrupts:
    cli
    ret
