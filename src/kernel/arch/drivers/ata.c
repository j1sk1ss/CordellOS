// Thanks to https://wiki.osdev.org/ATA_PIO_Mode and https://github.com/szhou42/osdev/blob/master/src/kernel/drivers/ata.c#L267
#include <arch/drivers/ata.h>

static pci_dev_t _ata_device               = { 0 };
static struct ata_dev* _current_ata_device = NULL;
static ata_dev_t _primary_master           = {.slave = 0};
static ata_dev_t _primary_slave            = {.slave = 1};
static ata_dev_t _secondary_master         = {.slave = 0};
static ata_dev_t _secondary_slave          = {.slave = 1};

static void _ata_wait() {
    int delay = 150000;
    while (--delay > 0) continue;
}

static int _is_ata_ready() {
    if (_current_ata_device == NULL || !_current_ata_device->present) return 0;

    int timeout = 9000000;
    uint8_t status = 0;

    do {
        status = i386_inb(_current_ata_device->status);
        if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) return 0;
        if (--timeout < 0) return 0;
    } while (status & ATA_SR_BSY);

    timeout = 9000000;
    while (!(status & ATA_STATUS_DRQ)) {
        status = i386_inb(_current_ata_device->status);
        if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) return 0;
        if (--timeout < 0) return 0;
    }

    return 1;
}

int ATA_initialize() {
    _ata_device.bits = 0;

    kprintf("ATA PROBE PM...\n");
    if (ATA_device_detect(&_primary_master, 1)) {
        _current_ata_device = &_primary_master;
    }

    if (_current_ata_device == NULL) {
        kprintf("ATA PROBE PS...\n");
        if (ATA_device_detect(&_primary_slave, 1)) {
            _current_ata_device = &_primary_slave;
        }
    }

    if (_current_ata_device == NULL) {
        kprintf("ATA PROBE SM...\n");
        if (ATA_device_detect(&_secondary_master, 0)) {
            _current_ata_device = &_secondary_master;
        }
    }

    if (_current_ata_device == NULL) {
        kprintf("ATA PROBE SS...\n");
        if (ATA_device_detect(&_secondary_slave, 0)) {
            _current_ata_device = &_secondary_slave;
        }
    }

    if (_current_ata_device == NULL) {
        LOG("ATA_initialize: no ATA disk found\n");
        return 0;
    }

    kprintf("ATA VFS INIT...\n");
    if (!VFS_initialize(_current_ata_device, FAT_FS)) {
        LOG("ATA_initialize: VFS init failed\n");
        return 0;
    }

    kprintf("ATA READY\n");
    return 1;
}

void ATA_handler(struct Registers* reg) {
    if (_current_ata_device == NULL) return;

    i386_inb(_current_ata_device->status);
    if (_current_ata_device->BMR_STATUS) i386_inb(_current_ata_device->BMR_STATUS);
    if (_current_ata_device->BMR_COMMAND) i386_outb(_current_ata_device->BMR_COMMAND, BMR_COMMAND_DMA_STOP);
}

void ATA_software_reset(ata_dev_t* dev) {
    i386_outb(dev->control, CONTROL_SOFTWARE_RESET);
    i386_io_wait(dev);
    i386_outb(dev->control, CONTROL_ZERO);
}

int ATA_device_detect(ata_dev_t* dev, int primary) {
    ATA_device_init(dev, primary);
    i386_io_wait(dev);

    i386_outb(dev->drive, (0xA + dev->slave) << 4);
    _ata_wait();
    i386_outb(dev->sector_count, 0);
    i386_outb(dev->lba_lo, 0);
    i386_outb(dev->lba_mid, 0);
    i386_outb(dev->lba_high, 0);

    i386_outb(dev->command, COMMAND_IDENTIFY);
    _ata_wait();

    uint8_t status = i386_inb(dev->status);
    if (status == 0 || status == 0xFF) {
        LOG("ATA_device_detect: device does not exist\n");
        dev->prdt = NULL;
        return 0;
    }

    uint8_t lba_lo = i386_inb(dev->lba_lo);
    uint8_t lba_mid = i386_inb(dev->lba_mid);
    uint8_t lba_high = i386_inb(dev->lba_high);
    if (lba_lo != 0 || lba_mid != 0 || lba_high != 0) {
        LOG("ATA_device_detect: not ata device\n");
        dev->prdt = NULL;
        return 0;
    }

    int timeout = 9999999;
    while (status & ATA_SR_BSY) {
        status = i386_inb(dev->status);
        if (--timeout < 0) {
            LOG("DRIVE [%i] NOT FOUND / ATTACHED\n");
            dev->prdt = NULL;
            return 0;
        }
    }

    if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) {
        LOG("ATA_device_detect: err when polling\n");
        dev->prdt = NULL;
        return 0;
    }

    if (!(status & ATA_STATUS_DRQ)) {
        LOG("ATA_device_detect: DRQ not set\n");
        dev->prdt = NULL;
        return 0;
    }

    for (int i = 0; i < 256; i++) i386_inw(dev->data);

    if (_ata_device.bits) {
        uint32_t pci_command_reg = pci_read(_ata_device, PCI_COMMAND);
        if (!(pci_command_reg & (1 << 2))) {
            pci_command_reg |= (1 << 2);
            pci_write(_ata_device, PCI_COMMAND, pci_command_reg);
        }
    }

    dev->present = 1;
    LOG("DRIVE [%s] FOUND (DATA PORT: %x)\n", dev->mountpoint, dev->data);
    return 1;
}

void ATA_device_init(ata_dev_t* dev, int primary) {
    memset(dev, 0, sizeof(ata_dev_t));
    dev->slave = dev == &_primary_slave || dev == &_secondary_slave;

    uint16_t base_addr  = primary ? (0x1F0) : (0x170);
    uint16_t alt_status = primary ? (0x3F6) : (0x376);

    dev->data         = base_addr;
    dev->error        = base_addr + 1;
    dev->sector_count = base_addr + 2;
    dev->lba_lo       = base_addr + 3;
    dev->lba_mid      = base_addr + 4;
    dev->lba_high     = base_addr + 5;
    dev->drive        = base_addr + 6;
    dev->command      = base_addr + 7;
    dev->alt_status   = alt_status;

    dev->bar4 = _ata_device.bits != 0 ? pci_read(_ata_device, PCI_BAR4) : 0;
    if (dev->bar4 & 0x1) dev->bar4 = dev->bar4 & 0xFFFFFFFC;

    if (dev->bar4 != 0) {
        dev->BMR_COMMAND = dev->bar4;
        dev->BMR_STATUS  = dev->bar4 + 2;
        dev->BMR_prdt    = dev->bar4 + 4;
    }

    memset(dev->mountpoint, 0, 32);
    strncpy(dev->mountpoint, "/DEV/HD", 7);
    
    dev->mountpoint[strlen(dev->mountpoint)] = 'a' + (((!primary) << 1) | dev->slave);
}

void ATA_device_switch(int device) {
    switch (device) {
        case 1: _current_ata_device = &_primary_master; break;
        case 2: _current_ata_device = &_primary_slave; break;
        case 3: _current_ata_device = &_secondary_master; break;
        case 4: _current_ata_device = &_secondary_slave; break;
    }
}

static void _prepare_for_reading(uint32_t lba) {
    if (_current_ata_device == NULL) return;
    i386_outb(_current_ata_device->drive, 0xE0 | (_current_ata_device->slave << 4) | ((lba >> 24) & 0x0F));
    i386_outb(_current_ata_device->error, 0x00);
    i386_outb(_current_ata_device->sector_count, 1);
    i386_outb(_current_ata_device->lba_lo, (uint8_t)(lba & 0xFF));
    i386_outb(_current_ata_device->lba_mid, (uint8_t)((lba >> 8) & 0xFF));
    i386_outb(_current_ata_device->lba_high, (uint8_t)((lba >> 16) & 0xFF));
    i386_outb(_current_ata_device->command, ATA_CMD_READ_PIO);
}

int ATA_read_sector(uint32_t lba, uint8_t* buffer) {
    _ata_wait();
    if (!buffer) return -1;

    _prepare_for_reading(lba);
    if (!_is_ata_ready()) return -1;

    for (int n = 0; n < SECTOR_SIZE / 2; n++) {
        uint16_t value = i386_inw(_current_ata_device->data);
        buffer[n * 2] = value & 0xFF;
        buffer[n * 2 + 1] = value >> 8;
    }

    return 1;
}

// Return two values
// stop == ERROR_SYMBOL (error), stop == STOP_SYMBOL (found)
int ATA_read_sector_stop(uint32_t lba, uint8_t* buffer, uint8_t* stop) {
    _ata_wait();
    if (!buffer) return -1;

    uint8_t dummy_buffer[SECTOR_SIZE] = { 0 };
    uint8_t* buffer_pointer = buffer;
    memset(buffer, 0, SECTOR_SIZE);
    
    _prepare_for_reading(lba);
    if (!_is_ata_ready()) return -1;

    for (int n = 0; n < SECTOR_SIZE / 2; n++) {
        uint16_t value = i386_inw(_current_ata_device->data);
        uint8_t first = (uint8_t)(value & 0xFF);
        buffer_pointer[n * 2] = first;
        if (first == stop[0]) {
            stop[0] = STOP_SYMBOL;
            buffer_pointer = dummy_buffer;
        }

        uint8_t second = (uint8_t)(value >> 8);
        buffer_pointer[n * 2 + 1] = second;
        if (second == stop[0]) {
            stop[0] = STOP_SYMBOL;
            buffer_pointer = dummy_buffer;
        }
    }

    return 1;
}

// TODO: issue with pointer moving 
int ATA_read_sector_stopoff(uint32_t lba, uint32_t offset, uint8_t* buffer, uint8_t* stop) {
    _ata_wait();
    if (!buffer) return -1;

    uint8_t dummy_buffer[SECTOR_SIZE] = { 0 };
    uint8_t* buffer_pointer = buffer;
    memset(buffer, 0, SECTOR_SIZE);
    
    _prepare_for_reading(lba);
    if (!_is_ata_ready()) return -1;
    
    for (int n = 0; n < SECTOR_SIZE / 2; n++) {
        uint16_t value = i386_inw(_current_ata_device->data);
        uint8_t first = (uint8_t)(value & 0xFF);
        buffer_pointer[n * 2] = first;
        if (first == *stop && n * 2 >= offset) {
            *stop = STOP_SYMBOL;
            buffer_pointer = dummy_buffer;
        }

        uint8_t second = (uint8_t)(value >> 8);
        buffer_pointer[n * 2 + 1] = second;
        if (second == *stop && n * 2 + 1 >= offset) {
            *stop = STOP_SYMBOL;
            buffer_pointer = dummy_buffer;
        }
    }
    
    return 1;
}

// Function to read a sectors from the disk.
int ATA_read_sectors(uint32_t lba, uint8_t* buffer, uint32_t sector_count) {
    _ata_wait();
    if (!buffer) return -1;
    memset(buffer, 0, SECTOR_SIZE * sector_count);
    for (uint32_t i = 0; i < sector_count; i++) {
        if (ATA_read_sector(lba + i, buffer + i * SECTOR_SIZE) != 1) {
            return -1;
        }
    }

    return 1;
}

int ATA_readoff_sectors(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t sector_count) {
    uint32_t sectors_seek = offset / SECTOR_SIZE;
    uint32_t data_seek = offset % SECTOR_SIZE;
    if (!buffer || sector_count == 0 || sectors_seek >= sector_count) return -1;

    uint32_t size = (SECTOR_SIZE * (sector_count - 1)) + (SECTOR_SIZE - data_seek);
    uint8_t sector_data[SECTOR_SIZE] = { 0 };

    memset(buffer, 0, size);
    uint32_t data_position = 0;
    for (uint32_t i = sectors_seek; i < sector_count; i++) {
        if (ATA_read_sector(lba + i, sector_data) != 1) return -1;
        
        uint32_t copy_size = SECTOR_SIZE - data_seek;
        memcpy(buffer + data_position, sector_data + data_seek, copy_size);
        data_position += copy_size;

        data_seek = 0;
    }

    return 1;
}

// Return two values
// data[0] - Find (1) or not found (0) stop data in data
// data[1] - Loaded data from disk
int ATA_read_sectors_stop(uint32_t lba, uint8_t* buffer, uint32_t sector_count, uint8_t* stop) {
    _ata_wait();
    if (buffer == NULL) return -1;

    memset(buffer, 0, SECTOR_SIZE * sector_count);
    for (uint32_t i = 0; i < sector_count; i++) {
        if (ATA_read_sector_stop(lba + i, buffer + i * SECTOR_SIZE, stop) != 1) return -1;

        if (*stop == STOP_SYMBOL) break;
    }

    return 1;
}

// Read sectors with start seek
// Stop reading when meet stop value
int ATA_readoff_sectors_stop(uint32_t lba, uint32_t offset, uint8_t* buffer, uint32_t sector_count, uint8_t* stop) {
    _ata_wait();

    uint32_t sectors_seek = offset / SECTOR_SIZE;
    uint32_t data_seek    = offset % SECTOR_SIZE;
    if (buffer == NULL || sector_count == 0 || sectors_seek >= sector_count) return -1;

    uint32_t size         = (SECTOR_SIZE * (sector_count - 1)) + (SECTOR_SIZE - data_seek);
    uint8_t sector_data[SECTOR_SIZE] = { 0 };

    memset(buffer, 0, size);
    uint32_t data_position = 0;
    for (uint32_t i = sectors_seek; i < sector_count; i++) {
        if (ATA_read_sector_stopoff(lba + i, data_seek, sector_data, stop) != 1) return -1;
        
        uint32_t copy_size = SECTOR_SIZE - data_seek;
        memcpy(buffer + data_position, sector_data + data_seek, copy_size);
        data_position += copy_size;

        data_seek = 0;
        if (*stop == STOP_SYMBOL) break;
    }

    return 1;
}

static void _prepare_for_writing(uint32_t lba) {
    if (_current_ata_device == NULL) return;
    i386_outb(_current_ata_device->drive, 0xE0 | (_current_ata_device->slave << 4) | ((lba >> 24) & 0x0F));
    i386_outb(_current_ata_device->error, 0x00);
    i386_outb(_current_ata_device->sector_count, 1);
    i386_outb(_current_ata_device->lba_lo, (uint8_t)lba);
    i386_outb(_current_ata_device->lba_mid, (uint8_t)(lba >> 8));
    i386_outb(_current_ata_device->lba_high, (uint8_t)(lba >> 16));
    i386_outb(_current_ata_device->command, ATA_CMD_WRITE_PIO);
}

int ATA_write_sector(uint32_t lba, const uint8_t* buffer) {
    if (lba == BOOT_SECTOR) return -1;

    _ata_wait();
    _prepare_for_writing(lba);

    int timeout = 9000000;
    while ((i386_inb(_current_ata_device->status) & ATA_SR_BSY) != 0) {
        if (--timeout < 0) return -1;
        else continue;
    }
    
    for (int i = 0; i < SECTOR_SIZE / 2; i++) {
        uint16_t data = *((uint16_t*)(buffer + i * 2));
        i386_outw(_current_ata_device->data, data);
    }

    return 1;
}

int ATA_writeoff_sector(uint32_t lba, const uint8_t* buffer, uint32_t offset, uint32_t size) {
    if (lba == BOOT_SECTOR) return -1;

    _ata_wait();
    _prepare_for_writing(lba);

    int timeout = 9000000;
    while ((i386_inb(_current_ata_device->status) & ATA_SR_BSY) != 0) {
        if (--timeout < 0) return -1;
        else continue;
    }
    
    for (int i = offset / 2; i < min(size / 2, (SECTOR_SIZE / 2) - offset); i++) {
        uint16_t data = *((uint16_t*)(buffer + i * 2));
        i386_outw(_current_ata_device->data, data);
    }

    return 1;
}

// Function to write a sector on the disk.
int ATA_write_sectors(uint32_t lba, const uint8_t* buffer, uint32_t sector_count) {
    _ata_wait();
    for(uint32_t i = 0; i < sector_count; i++) {
        if (ATA_write_sector(lba + i, buffer) == -1) 
            return -1;
        
        buffer += SECTOR_SIZE;
    }

    return 1;
}

int ATA_writeoff_sectors(uint32_t lba, const uint8_t* buffer, uint32_t sector_count, uint32_t offset, uint32_t size) {
    _ata_wait();

    uint32_t sectors_seek  = offset / SECTOR_SIZE;
    uint32_t sector_seek   = offset % SECTOR_SIZE;
    uint32_t data_position = 0;

    for (uint32_t i = sectors_seek; i < sector_count && data_position < size; i++) {
        uint32_t write_size = min(size - data_position, SECTOR_SIZE - sector_seek);
        if (ATA_writeoff_sector(lba + i, buffer, sector_seek, write_size) == -1) {
            return -1;
        }
        
        buffer += write_size;
        data_position += write_size;
        sector_seek = 0;
    }

    return 1;
}

int ATA_copy_sectors2sectors(uint32_t source_lba, uint32_t sector_count, uint32_t distenation_lba) {
    _ata_wait();
    uint8_t source[SECTOR_SIZE] = { 0 };

    for (uint32_t i = 0; i < sector_count; i++) {
        if (ATA_read_sector(source_lba + i, source) != 1) return -1;
        if (ATA_write_sector(distenation_lba + i, source) == -1) return -1;
    }

    return 1;
}
