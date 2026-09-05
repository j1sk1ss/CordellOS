#ifndef NETUTILS_H_
#define NETUTILS_H_

#include <stdint.h>

uint16_t     flip_endian16(uint16_t short_int);
uint32_t     flip_endian32(uint32_t long_int);
uint8_t      swap_byte_parts(uint8_t byte, int num_bits);
uint8_t      host2net8(uint8_t byte, int num_bits);
uint8_t      net2host8(uint8_t byte, int num_bits);
uint16_t     host2net16(uint16_t hostshort);
uint32_t     host2net32(uint32_t hostlong);
uint16_t     net2host16(uint16_t netshort);
uint32_t     net2host32(uint32_t netlong);
uint16_t     net_checksum(const uint8_t* data, const uint8_t* end);
unsigned int net_checksum_acc(const uint8_t* data, const uint8_t* end, unsigned int sum);
uint16_t     net_checksum_final(unsigned int sum);

#endif