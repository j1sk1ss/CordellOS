#include <netutils.h>

uint16_t flip_endian16(uint16_t short_int) {
    uint32_t first_byte  = *((uint8_t*)(&short_int));
    uint32_t second_byte = *((uint8_t*)(&short_int) + 1);
    return (first_byte << 8) | (second_byte);
}

uint32_t flip_endian32(uint32_t long_int) {
    uint32_t first_byte  = *((uint8_t*)(&long_int));
    uint32_t second_byte = *((uint8_t*)(&long_int) + 1);
    uint32_t third_byte  = *((uint8_t*)(&long_int)  + 2);
    uint32_t fourth_byte = *((uint8_t*)(&long_int) + 3);
    return (first_byte << 24) | (second_byte << 16) | (third_byte << 8) | (fourth_byte);
}

/*
 * Flip two parts within a byte
 * For example, 0b11110000 will be 0b00001111 instead
 * This is necessary because endiness is also relevant to byte, where there are two fields in one byte.
 * number_bits: number of bits of the less significant field
 * */
uint8_t swap_byte_parts(uint8_t byte, int num_bits) {
    uint8_t t = byte << (8 - num_bits);
    return t | (byte >> num_bits);
}

uint8_t host2net8(uint8_t byte, int num_bits) {
    return swap_byte_parts(byte, num_bits);
}

uint8_t net2host8(uint8_t byte, int num_bits) {
    return swap_byte_parts(byte, 8 - num_bits);
}

uint16_t host2net16(uint16_t hostshort) {
    return flip_endian16(hostshort);
}

uint32_t host2net32(uint32_t hostlong) {
    return flip_endian32(hostlong);
}

uint16_t net2host16(uint16_t netshort) {
    return flip_endian16(netshort);
}

uint32_t net2host32(uint32_t netlong) {
    return flip_endian32(netlong);
}

uint16_t net_checksum(const uint8_t* data, const uint8_t* end) {
    unsigned int sum = net_checksum_acc(data, end, 0);
    return net_checksum_final(sum);
}

unsigned int net_checksum_acc(const uint8_t* data, const uint8_t* end, unsigned int sum) {
    unsigned int len = end - data;
    uint16_t* p = (uint16_t*)data;

    while (len > 1) {
        sum += *p++;
        len -= 2;
    }

    if (len) 
        sum += *(uint8_t*)p;

    return sum;
}

uint16_t net_checksum_final(unsigned int sum) {
    sum = (sum & 0xFFFF) + (sum >> 16);
    sum += (sum >> 16);
    uint16_t temp = ~sum;
    return ((temp & 0x00FF) << 8) | ((temp & 0xFF00) >> 8); // TODO - shouldn't swap this twice
}