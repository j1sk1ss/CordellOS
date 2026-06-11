#ifndef TSS_H_
#define TSS_H_

#include <stdint.h>

extern void TSS_set_stack(uint32_t kss, uint32_t kesp);

#endif