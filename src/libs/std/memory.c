#include "../include/memory.h"

// https://forum.osdev.org/viewtopic.php?t=18119
void* memcpy(void* destination, const void* source, size_t num) {
    uint32_t num_dwords = num / 4;
    uint32_t num_bytes = num % 4;
    uint32_t* dest32 = (uint32_t*)destination;
    uint32_t* src32 = (uint32_t*)source;
    uint8_t* dest8 = ((uint8_t*)destination) + num_dwords * 4;
    uint8_t* src8 = ((uint8_t*)source) + num_dwords * 4;
    uint32_t i = 0;

    for (i = 0; i < num_dwords; i++) dest32[i] = src32[i];
    for (i = 0; i < num_bytes; i++) dest8[i] = src8[i];

    return destination;
}

void* memset(void* destination, int value, size_t num) {
    uint32_t num_dwords = num / 4;
    uint32_t num_bytes = num % 4;
    uint32_t *dest32 = (uint32_t*)destination;
    uint8_t *dest8 = ((uint8_t*)destination)+num_dwords*4;
    uint8_t val8 = (uint8_t)value;
    uint32_t val32 = value | (value << 8) | (value << 16) | (value << 24);
    uint32_t i = 0;

    for (i = 0; i < num_dwords; i++) dest32[i] = val32;
    for (i = 0; i < num_bytes; i++) dest8[i] = val8;
    
    return destination;
}

int memcmp(const void* firstPointer, const void* secondPointer, size_t num) {
    const uint8_t* u8Ptr1 = (const uint8_t *)firstPointer;
    const uint8_t* u8Ptr2 = (const uint8_t *)secondPointer;
    for (uint16_t i = 0; i < num; i++)
        if (u8Ptr1[i] != u8Ptr2[i])
            return 1;

    return 0;
}

void* seg_offset_to_linear(void* address) {
    uint32_t offset = (uint32_t)(address) & 0xFFFF;
    uint32_t segment = (uint32_t)(address) >> 16;
    return (void*)(segment * 16 + offset);
}

void* memmove(void* dest, const void* src, size_t len) {
    char* d = dest;
    const char* s = src;
    if (d < s)
        while (len--)
            *d++ = *s++;
    else {
      const char* lasts = s + (len - 1);
      char* lastd = d + (len - 1);
        while (len--)
            *lastd-- = *lasts--;
    }

    return dest;
}

void* memmove32(void* dest, const void* src, size_t len) {
    uint32_t* d = (uint32_t*)dest;
    const uint32_t* s = (const uint32_t*)src;

    if (d < s) {
        while (len >= 4) {
            *d++ = *s++;
            len -= 4;
        }
    } else {
        d += len / 4;
        s += len / 4;
        while (len >= 4) {
            *--d = *--s;
            len -= 4;
        }
    }

    uint8_t* d_byte = (uint8_t*)d;
    const uint8_t* s_byte = (const uint8_t*)s;
    while (len--) *d_byte++ = *s_byte++;
    
    return dest;
}