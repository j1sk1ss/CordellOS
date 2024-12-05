#ifndef UDP_H
#define UDP_H

#include <memory.h>
#include <stdint.h>
#include <netutils.h>

#include "ip.h"
#include "allocator.h"


#define PACKET_BUFFER_SIZE  100


typedef struct UDPpacket {
    uint8_t* data;
    int16_t data_size;
} udp_packet_body;

typedef struct udp_packet {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum;
    uint8_t data[];
} __attribute__((packed)) udp_packet_t;


void UDP_init();
struct UDPpacket* UDP_pop_packet();

uint16_t UDP_calculate_checksum(udp_packet_t* packet);
void UDP_send_packet(uint8_t* dst_ip, uint16_t src_port, uint16_t dst_port, void* data, int len);
void UDP_handle_packet(udp_packet_t* packet);

#endif