#ifndef TSS_H
#define TSS_H

#include <stdint.h>

extern void TSS_init(uint32_t idx, uint32_t kss, uint32_t kesp);
extern void TSS_set_stack(uint32_t kss, uint32_t kesp);

#endif