extern function i386_inb(u16 port) -> i8;
extern function i386_outb(u16 port, u8 data) -> i0;
extern function kprintf(ptr i8 fmt, ...) -> i0;

#define PIT_DATA_PORT0     0x40
#define PIT_COMMAND_PORT   0x43
#define TIMER_FREQUENCY_HZ 100

glob function i386_pit_init() -> i0 {
    u16 divisor = (1193180 / TIMER_FREQUENCY_HZ) as u16;
    i386_outb(PIT_COMMAND_PORT, 0x36 as u8);
    i386_outb(PIT_DATA_PORT0, (divisor & 0xFF) as u8);
    i386_outb(PIT_DATA_PORT0, ((divisor >> 8) & 0xFF) as u8);
    kprintf(ref "PIT is ready and set to %i!\n", TIMER_FREQUENCY_HZ);
}