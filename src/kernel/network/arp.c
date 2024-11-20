#include "../include/arp.h"


arp_table_entry_t arp_table[ARP_TABLE_SIZE];
int arp_table_size;
int arp_table_curr;

uint8_t broadcast_mac_address[6] = { 0xFF };


void ARP_handle_packet(arp_packet_t* arp_packet, int len) {
    char dst_hardware_addr[6] = { 0 };
    char dst_protocol_addr[4] = { 0 };

    // Save some packet field
    memcpy(dst_hardware_addr, arp_packet->src_hardware_addr, 6);
    memcpy(dst_protocol_addr, arp_packet->src_protocol_addr, 4);

    if (net2host16(arp_packet->opcode) == ARP_REQUEST) {
        uint32_t my_ip = 0x0e02000a;
        if (memcmp(arp_packet->dst_protocol_addr, &my_ip, 4)) {
            
            if (is_ip_allocated == 1) {
                DHCP_get_host_addr(arp_packet->src_protocol_addr);
                arp_lookup(arp_packet->src_hardware_addr, arp_packet->src_protocol_addr);
            }
            else {
                get_mac_addr(arp_packet->src_hardware_addr);
                arp_packet->src_protocol_addr[0] = 10;
                arp_packet->src_protocol_addr[1] = 0;
                arp_packet->src_protocol_addr[2] = 2;
                arp_packet->src_protocol_addr[3] = 14;
            }

            // Set destination MAC address, IP address
            memcpy(arp_packet->dst_hardware_addr, dst_hardware_addr, 6);
            memcpy(arp_packet->dst_protocol_addr, dst_protocol_addr, 4);

            // Set opcode
            arp_packet->opcode            = host2net16(ARP_REPLY);
            arp_packet->hardware_addr_len = 6;
            arp_packet->protocol_addr_len = 4;
            arp_packet->hardware_type     = host2net16(HARDWARE_TYPE_ETHERNET);
            arp_packet->protocol          = host2net16(ETHERNET_TYPE_IP);

            // Now send it with ethernet
            ETH_send_packet((uint8_t*)dst_hardware_addr, (uint8_t*)arp_packet, sizeof(arp_packet_t), ETHERNET_TYPE_ARP);
        }
    }

    else if (net2host16(arp_packet->opcode) == ARP_REPLY) { /* TODO: ARP reply */ }
    else kprintf("\n[%s %i] Got unknown ARP, opcode = %d\n", __FILE__, __LINE__, arp_packet->opcode);
    
    // Now, store the ip-mac address mapping relation
    memcpy(&arp_table[arp_table_curr].ip_addr, dst_protocol_addr, 4);
    memcpy(&arp_table[arp_table_curr].mac_addr, dst_hardware_addr, 6);

    if(arp_table_size < ARP_TABLE_SIZE) arp_table_size++;
    if(arp_table_curr >= ARP_TABLE_SIZE) arp_table_curr = 0;
}

void ARP_send_packet(uint8_t* dst_hardware_addr, uint8_t* dst_protocol_addr) {
    arp_packet_t* arp_packet = (arp_packet_t*)kmalloc(sizeof(arp_packet_t));

    get_mac_addr(arp_packet->src_hardware_addr);
    if (is_ip_allocated == 1) DHCP_get_host_addr(arp_packet->src_protocol_addr);
    else {
        arp_packet->src_protocol_addr[0] = 10;
        arp_packet->src_protocol_addr[1] = 0;
        arp_packet->src_protocol_addr[2] = 2;
        arp_packet->src_protocol_addr[3] = 14;
    }

    // Set destination MAC address, IP address
    memcpy(arp_packet->dst_hardware_addr, dst_hardware_addr, 6);
    memcpy(arp_packet->dst_protocol_addr, dst_protocol_addr, 4);

    // Set opcode
    arp_packet->opcode            = host2net16(ARP_REQUEST);
    arp_packet->hardware_addr_len = 6;
    arp_packet->protocol_addr_len = 4;
    arp_packet->hardware_type     = host2net16(HARDWARE_TYPE_ETHERNET);
    arp_packet->protocol          = host2net16(ETHERNET_TYPE_IP);

    // Now send it with ethernet
    ETH_send_packet(broadcast_mac_address, (uint8_t*)arp_packet, sizeof(arp_packet_t), ETHERNET_TYPE_ARP);
    kfree(arp_packet);
}

void ARP_lookup_add(uint8_t* ret_hardware_addr, uint8_t* ip_addr) {
    memcpy(&arp_table[arp_table_curr].ip_addr, ip_addr, 4);
    memcpy(&arp_table[arp_table_curr].mac_addr, ret_hardware_addr, 6);
    
    if (arp_table_size < ARP_TABLE_SIZE) arp_table_size++;
    if (arp_table_curr >= ARP_TABLE_SIZE) arp_table_curr = 0;
}

int arp_lookup(uint8_t* ret_hardware_addr, uint8_t* ip_addr) {
    uint32_t ip_entry = *((uint32_t*)(ip_addr));
    for (int i = 0; i < ARP_TABLE_SIZE; i++) 
        if (arp_table[i].ip_addr == ip_entry) {
            memcpy(ret_hardware_addr, &arp_table[i].mac_addr, 6);
            return 1;
        }

    return 0;
}

void ARP_init() {
    uint8_t broadcast_ip[4];
    uint8_t broadcast_mac[6];

    memset(broadcast_ip, 0xff, 4);
    memset(broadcast_mac, 0xff, 6);
    
    ARP_lookup_add(broadcast_mac, broadcast_ip);
}