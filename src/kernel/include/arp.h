#ifndef ARP_H_
#define ARP_H_

#include <memory.h>
#include <netutils.h>

#include "stdio.h"
#include "rtl8139.h"
#include "ethernet.h"
#include "allocator.h"


#define ARP_REQUEST     1
#define ARP_REPLY       2

#define ARP_TABLE_SIZE  512


typedef struct arp_packet {
    uint16_t hardware_type;
    uint16_t protocol;
    uint8_t hardware_addr_len;
    uint8_t protocol_addr_len;
    uint16_t opcode;
    uint8_t src_hardware_addr[6];
    uint8_t src_protocol_addr[4];
    uint8_t dst_hardware_addr[6];
    uint8_t dst_protocol_addr[4];
} __attribute__((packed)) arp_packet_t;

typedef struct arp_table_entry {
    uint32_t ip_addr;
    uint64_t mac_addr;
} arp_table_entry_t;

typedef struct arp_data {
    arp_table_entry_t arp_table[ARP_TABLE_SIZE];
    int arp_table_size;
    int arp_table_curr;
    uint8_t broadcast_mac_address[6];
} arp_data_t;


void ARP_handle_packet(arp_packet_t* arp_packet, int len);
void ARP_send_packet(uint8_t* dst_hardware_addr, uint8_t* dst_protocol_addr);
int arp_lookup(uint8_t* ret_hardware_addr, uint8_t* ip_addr);
void ARP_lookup_add(uint8_t* ret_hardware_addr, uint8_t* ip_addr);
void ARP_init();

#endif