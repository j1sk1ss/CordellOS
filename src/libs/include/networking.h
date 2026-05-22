#ifndef NETWORKING_H_
#define NETWORKING_H_

#include <stdint.h>

void set_ip(uint8_t* ip);
void get_ip(uint8_t* buffer);
void get_mac(uint8_t* buffer);
void send_udp_packet(uint8_t* dst_ip, uint16_t src_port, uint16_t dst_port, void* data, int len);
void pop_received_udp_packet(uint8_t* buffer);

#endif