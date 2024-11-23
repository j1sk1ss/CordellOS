#include "../include/ethernet.h"


int ETH_send_packet(uint8_t* dst_mac_addr, uint8_t* data, int len, uint16_t protocol) {
    uint8_t src_mac_addr[6] = { 0x00 };
    ethernet_frame_t* frame = (ethernet_frame_t*)ALC_malloc(sizeof(ethernet_frame_t) + len, KERNEL);
    void* frame_data = (void*)frame + sizeof(ethernet_frame_t);

    get_mac_addr(src_mac_addr);
    memcpy(frame->src_mac_addr, src_mac_addr, 6);
    memcpy(frame->dst_mac_addr, dst_mac_addr, 6);
    memcpy(frame_data, data, len);

    frame->type = host2net16(protocol);

    rtl8139_send_packet(frame, sizeof(ethernet_frame_t) + len);
    ALC_free(frame, KERNEL);

    return len;
}

void ETH_handle_packet(ethernet_frame_t* packet, int len) {
    void* data   = (void*)packet + sizeof(ethernet_frame_t);
    int data_len = len - sizeof(ethernet_frame_t);
    if (net2host16(packet->type) == ETHERNET_TYPE_ARP) {
        if (NETWORK_DEBUG) kprintf("\nReceived ARP packet\n");
        ARP_handle_packet(data, data_len);
    }
    
    if (net2host16(packet->type) == ETHERNET_TYPE_IP) {
        if (NETWORK_DEBUG) kprintf("\nReceived IP packet\n");
        IP_handle_packet(data);
    }
}