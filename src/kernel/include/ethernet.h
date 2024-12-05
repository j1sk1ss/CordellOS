#ifndef ETHERNET_H_
#define ETHERNET_H_

#include <stdint.h>
#include <memory.h>
#include <netutils.h>

#include "ip.h"
#include "arp.h"
#include "rtl8139.h"
#include "allocator.h"


#define ETHERNET_TYPE_ARP 0x0806
#define ETHERNET_TYPE_IP  0x0800

#define HARDWARE_TYPE_ETHERNET 0x01


typedef struct ethernet_frame {
  uint8_t dst_mac_addr[6];
  uint8_t src_mac_addr[6];
  uint16_t type;
  uint8_t data[];
} __attribute__((packed)) ethernet_frame_t;


int ETH_send_packet(uint8_t* dst_mac_addr, uint8_t* data, int len, uint16_t protocol);
void ETH_handle_packet(ethernet_frame_t* packet, int len);

#endif