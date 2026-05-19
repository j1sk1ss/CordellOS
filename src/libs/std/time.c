#include <time.h>

void sleep_s(const uint16_t seconds)  {
    for (int i = 0; i < seconds * 10000; i++);
}

void sleep_ms(const uint32_t milliseconds) {
   for (int i = 0; i < milliseconds * 10000; i++);
}

int get_tick() {
    int ticks = 0;
    __asm__ volatile(
        "movl $3, %%eax\n"
        "int $0x80\n"
        "movl %%eax, %0\n"
        : "=r" (ticks)
        : 
        : "%eax"
    );

    return ticks;
}