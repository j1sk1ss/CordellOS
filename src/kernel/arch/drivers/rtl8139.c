#include "../../include/rtl8139.h"


static int packet_meta[100] = { 0 };
static ethernet_frame_t* ethernet_packets[100] = { NULL };
static int current_ethernet_packet = 0;

static pci_dev_t pci_rtl8139_device;
static rtl8139_dev_t rtl8139_device;

static uint32_t current_packet_ptr = 0;

static uint8_t TSAD_array[4] = { 0x20, 0x24, 0x28, 0x2C };
static uint8_t TSD_array[4]  = { 0x10, 0x14, 0x18, 0x1C };


struct ethernet_packet* pop_packet() {
    struct ethernet_packet* packet = _kmalloc(sizeof(ethernet_frame_t) + sizeof(int));
    packet->packet = ethernet_packets[current_ethernet_packet];
    packet->len = packet_meta[current_ethernet_packet--];

    return packet;
}

void __rtl_receive_packet() {
    uint8_t* rx_buffer     = rtl8139_device.rx_buffer;
    uint16_t* t            = (uint16_t*)(rx_buffer + current_packet_ptr);
    uint16_t packet_length = *(t + 1);

    t = t + 2;
    void* packet = _kmalloc(packet_length);
    memcpy(packet, t, packet_length);

    ETH_handle_packet((ethernet_frame_t*)packet, packet_length);
    ethernet_packets[current_ethernet_packet] = (ethernet_frame_t*)packet;
    packet_meta[current_ethernet_packet++] = packet_length;

    current_packet_ptr = (current_packet_ptr + packet_length + 4 + 3) & RX_READ_POINTER_MASK;
    if (current_packet_ptr > RX_BUFFER_SIZE) current_packet_ptr -= RX_BUFFER_SIZE;

    i386_outw(rtl8139_device.io_base + CAPR, current_packet_ptr - 0x10);
    _kfree(packet);
}

void rtl8139_handler(struct Registers* reg) {
    uint16_t status = i386_inw(rtl8139_device.io_base + IntrStatus);
    i386_outw(rtl8139_device.io_base + IntrStatus, 0x05);

    if (status & TOK) if (NETWORK_DEBUG) kprintf("\nRLT8139 packet sent");
    if (status & ROK) __rtl_receive_packet();
}

void get_mac_addr(uint8_t* src_mac_addr) {
    memcpy(src_mac_addr, rtl8139_device.mac_addr, 6);
}

void rtl8139_send_packet(void* data, uint32_t len) {
    void* transfer_data = _kmalloc(len);
    void* phys_addr     = (void*)VMM_virtual2physical(transfer_data);
    memcpy(transfer_data, data, len);

    i386_outl(rtl8139_device.io_base + TSAD_array[rtl8139_device.tx_cur], (uint32_t)phys_addr);
    i386_outl(rtl8139_device.io_base + TSD_array[rtl8139_device.tx_cur++], len);
    if (rtl8139_device.tx_cur > 3) rtl8139_device.tx_cur = 0;

    _kfree(transfer_data);
}

void i386_init_rtl8139() {
    pci_rtl8139_device = pci_get_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID, -1);
    uint32_t ret = pci_read(pci_rtl8139_device, PCI_BAR0);
    rtl8139_device.bar_type = ret & 0x1;
    rtl8139_device.io_base  = ret & (~0x3);
    rtl8139_device.mem_base = ret & (~0xf);
    kprintf("\nRTL_8139 USE %s ACCESS (BASE: %x)", 
                (rtl8139_device.bar_type == 0) ? "MEM BASED" : "PORT BASED", 
                (rtl8139_device.bar_type != 0) ? rtl8139_device.io_base : rtl8139_device.mem_base
    );
    
    rtl8139_device.tx_cur = 0;
    uint32_t pci_command_reg = pci_read(pci_rtl8139_device, PCI_COMMAND);
    if (!(pci_command_reg & (1 << 2))) {
        pci_command_reg |= (1 << 2);
        pci_write(pci_rtl8139_device, PCI_COMMAND, pci_command_reg);
    }

    kprintf("\nBASE PORT: %i", rtl8139_device.io_base);
    kprintf("\nMEM BASE:  %i", rtl8139_device.mem_base);

    i386_outb(rtl8139_device.io_base + Config1, 0x0);
    i386_outb(rtl8139_device.io_base + ChipCmd, 0x10);
    while((i386_inb(rtl8139_device.io_base + ChipCmd) & 0x10) != 0) continue;

    uint8_t* rx_buffer = rtl8139_device.rx_buffer;
    i386_outl(rtl8139_device.io_base + RxBuf, (uint32_t)rx_buffer);

    i386_outw(rtl8139_device.io_base + IntrMask, 0x0005);
    i386_outb(rtl8139_device.io_base + ChipCmd, 0x0C);
    i386_outl(rtl8139_device.io_base + RxConfig, 0xF);

    uint32_t irq_num = pci_read(pci_rtl8139_device, PCI_INTERRUPT_LINE);
    kprintf("\nRTL8139 IRQ REGISTERED [%i]", irq_num);
    i386_irq_registerHandler(irq_num, rtl8139_handler);

    uint32_t mac_part1 = i386_inl(rtl8139_device.io_base + 0x00);
    uint16_t mac_part2 = i386_inw(rtl8139_device.io_base + 0x04);
    
    rtl8139_device.mac_addr[0] = mac_part1 >> 0;
    rtl8139_device.mac_addr[1] = mac_part1 >> 8;
    rtl8139_device.mac_addr[2] = mac_part1 >> 16;
    rtl8139_device.mac_addr[3] = mac_part1 >> 24;
    rtl8139_device.mac_addr[4] = mac_part2 >> 0;
    rtl8139_device.mac_addr[5] = mac_part2 >> 8;

    kprintf("\nMAC Address: %x:%x:%x:%x:%x:%x\n", 
            rtl8139_device.mac_addr[0], 
            rtl8139_device.mac_addr[1], 
            rtl8139_device.mac_addr[2], 
            rtl8139_device.mac_addr[3], 
            rtl8139_device.mac_addr[4], 
            rtl8139_device.mac_addr[5]
    );
}