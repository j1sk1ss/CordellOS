// Some data from: https://en.wikipedia.org/wiki/Dynamic_Host_Configuration_Protocol
// Also thanks to: https://github.com/szhou42/osdev/blob/master/src/kernel/network/dhcp.c#L9
#include "../include/dhcp.h"


uint8_t ip_addr[4] = { 0x00 };
int is_ip_allocated = 0;


// Get IP of This machine 
int DHCP_get_host_addr(uint8_t* addr) {
    memcpy(addr, ip_addr, 4);
    if (is_ip_allocated == 0) return 0;
    else return 1;
}

// Find DHCP server in LNET
void DHCP_discover() {
    uint8_t request_ip[4];
    uint8_t dst_ip[4];

    memset(request_ip, 0x0, 4); // 0.0.0.0:68
    memset(dst_ip, 0xFF, 4);    // 255.255.255.255:67

    dhcp_packet_t* packet = (dhcp_packet_t*)kmalloc(sizeof(dhcp_packet_t));
    DHCP_make_packet(packet, DHCPDISCOVER, request_ip);
    UDP_send_packet(dst_ip, 68, 67, packet, sizeof(dhcp_packet_t));
    kfree(packet);
}

// Send DHCP request for IP
void DHCP_request(uint8_t* request_ip) {
    uint8_t dst_ip[4];
    memset(dst_ip, 0xFF, 4);

    dhcp_packet_t* packet = (dhcp_packet_t*)kmalloc(sizeof(dhcp_packet_t));
    DHCP_make_packet(packet, DHCPREQUEST, request_ip);
    UDP_send_packet(dst_ip, 68, 67, packet, sizeof(dhcp_packet_t));
    kfree(packet);
}

void DHCP_handle_packet(dhcp_packet_t* packet) {
    uint8_t* options = (uint8_t*)(packet->options + 4);
    if (packet->op == DHCP_REPLY) {
        uint8_t* type    = DHCP_options(packet, 53);
        uint8_t* pointer = type;

        if (*type == 2) DHCP_request((uint8_t*)&packet->your_ip);
        else if (*type == 5) {
            memcpy(ip_addr, &packet->your_ip, 4);
            is_ip_allocated = 1;

            uint8_t mac[6];
            get_mac_addr(mac);
            ARP_lookup_add(mac, ip_addr);
        }

        kfree(pointer);
    }
}

// Handle packet and work with him
void* DHCP_options(dhcp_packet_t* packet, uint8_t type) {
    uint8_t* options  = packet->options + 4;
    uint8_t curr_type = *options;
    while(curr_type != 0xFF) {
        uint8_t len = *(options + 1);
        if(curr_type == type) {
            void* ret = kmalloc(len);
            memcpy(ret, options + 2, len);
            return ret;
        }

        options += (2 + len);
    }

    return NULL;
}

void DHCP_make_packet(dhcp_packet_t* packet, uint8_t msg_type, uint8_t* request_ip) {
    packet->op                = DHCP_REQUEST;
    packet->hardware_type     = HARDWARE_TYPE_ETHERNET;
    packet->hardware_addr_len = 6;
    packet->hops              = 0;
    packet->xid               = host2net32(DHCP_TRANSACTION_IDENTIFIER);
    packet->flags             = host2net16(0x8000);

    get_mac_addr(packet->client_hardware_addr);

    uint8_t dst_ip[4];
    memset(dst_ip, 0xff, 4);

    uint8_t* options = packet->options;
    *((uint32_t*)(options)) = host2net32(0x63825363); // Magic cookie 
    options += 4;

    // First option, message type = DHCP_DISCOVER/DHCP_REQUEST
    *(options++) = OPT_DHCP_MESSAGE_TYPE;
    *(options++) = 1;
    *(options++) = msg_type;

    //===================
    // Client identifier
    //===================

        *(options++) = 61;
        *(options++) = 0x07;
        *(options++) = 0x01;
        get_mac_addr(options);
        options += 6;

    //===================
    // Client identifier
    //===================
    // Requested IP address
    //===================

        *(options++) = 50;
        *(options++) = 0x04;
        *((uint32_t*)(options)) = host2net32(0x0a00020e);
        memcpy((uint32_t*)(options), request_ip, 4);
        options += 4;

    //===================
    // Requested IP address
    //===================
    // Host Name
    //===================
    
        *(options++) = 12;
        *(options++) = 0x09;
        memcpy(options, "cordelos", strlen("cordelos"));
        options += 8;
        *(options++) = 0x00;

    //===================
    // Host Name
    //===================

    *(options++) = 55;  // Parameter request list
    *(options++) = 8;   // Length of the option

    // List of requested parameters
    *(options++) = 0x1;    // Request Subnet Mask (Code 1)
    *(options++) = 0x3;    // Request Router (Code 3)
    *(options++) = 0x6;    // Request Domain Name Server (DNS) (Code 6)
    *(options++) = 0xf;    // Request Domain Name (Code 15)
    *(options++) = 0x2c;   // Request Time Offset (Code 44)
    *(options++) = 0x2e;   // Request NTP Servers (Code 46)
    *(options++) = 0x2f;   // Request Vendor-Specific Information (Code 47)
    *(options++) = 0x39;   // Request NetBIOS over TCP/IP Name Server (Code 57)
    *(options++) = 0xff;   // End marker for options
}