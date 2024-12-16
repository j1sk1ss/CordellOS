#include "../include/ip.h"


uint8_t my_ip[]              = { 10, 0, 2, 14 };
uint8_t zero_hardware_addr[] = { 0, 0, 0, 0, 0, 0 };


void IP_get_ip_str(uint8_t* ip) {
    kprintf("IP: %d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

void IP_set(uint8_t* ip) {
    memcpy(my_ip, ip, 4);

    uint8_t mac[6] = { 0x00 };
    get_mac_addr(mac);

    ARP_lookup_add(mac, my_ip); // TODO: delete old ARP data
}

void IP_get(uint8_t* buffer) {
    DHCP_get_host_addr(buffer);
    memcpy(buffer, my_ip, 4);
}

uint16_t IP_calculate_checksum(ip_packet_t* packet) {
    int array_size  = sizeof(ip_packet_t) / 2;
    uint16_t* array = (uint16_t*)(uintptr_t)packet;
    uint32_t sum    = 0;
    for (int i = 0; i < array_size; i++) 
        sum += flip_endian16(array[i]);

    uint32_t carry = sum >> 16;

    sum = sum & 0x0000FFFF;
    sum = sum + carry;

    uint16_t ret = ~sum;
    return ret;
}

void IP_send_packet(uint8_t* dst_ip, void* data, int len, uint8_t protocol) {
    int arp_sent = 3;
    ip_packet_t* packet = (ip_packet_t*)ALC_malloc(sizeof(ip_packet_t) + len, KERNEL);

    packet->version              = IP_IPV4;
    packet->ihl                  = 5; // Internet Header Length
    packet->tos                  = 0; // Type of Service
    packet->length               = sizeof(ip_packet_t) + len;
    packet->id                   = 0;
    packet->flags                = 0;
    packet->fragment_offset_high = 0;
    packet->fragment_offset_low  = 0;
    packet->ttl                  = 64;
    packet->protocol             = protocol;

    DHCP_get_host_addr(my_ip);
    memcpy(packet->src_ip, my_ip, 4);
    memcpy(packet->dst_ip, dst_ip, 4);

    void* packet_data = (void*)packet + packet->ihl * 4;
    memcpy(packet_data, data, len);

    *((uint8_t*)(&packet->version_ihl_ptr))   = host2net8(*((uint8_t*)(&packet->version_ihl_ptr)), 4);
    *((uint8_t*)(packet->flags_fragment_ptr)) = host2net8(*((uint8_t*)(packet->flags_fragment_ptr)), 3);

    packet->length          = host2net16(sizeof(ip_packet_t) + len);
    packet->header_checksum = 0;
    packet->header_checksum = host2net16(IP_calculate_checksum(packet));

    uint8_t dst_hardware_addr[6];
    int delay = 9999999;
    while (!ARP_lookup(dst_hardware_addr, dst_ip)) {
        if (--delay < 0) {
            kprintf("\nARP not found MAC!!\n");
            return;
        }

        if (arp_sent != 0) {
            arp_sent--;
            ARP_send_packet(zero_hardware_addr, dst_ip);
        } else break;
    }

    ETH_send_packet(dst_hardware_addr, (uint8_t*)packet, host2net16(packet->length), ETHERNET_TYPE_IP);
    if (NETWORK_DEBUG) kprintf("\nIP Packet Sent...(checksum: %x)", packet->header_checksum);
    ALC_free(packet, KERNEL);
}


void IP_handle_packet(ip_packet_t* packet) {
    *((uint8_t*)(&packet->version_ihl_ptr))   = net2host8(*((uint8_t*)(&packet->version_ihl_ptr)), 4);
    *((uint8_t*)(packet->flags_fragment_ptr)) = net2host8(*((uint8_t*)(packet->flags_fragment_ptr)), 3);
    
    if (packet->version == IP_IPV4) {
        void* data_ptr = (void*)packet + packet->ihl * 4;
        if (NETWORK_DEBUG) {
            kprintf("\nIP packet src: ");
            IP_get_ip_str(packet->src_ip);
        }
        
        if (packet->protocol == PROTOCOL_UDP) UDP_handle_packet(data_ptr);
    }
}