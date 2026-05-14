// Thanks to https://wiki.osdev.org/ATA_PIO_Mode and https://github.com/szhou42/osdev/blob/master/src/kernel/drivers/ata.c#L267
#include "../../include/ata.h"


static pci_dev_t ata_device = { 0 };
static struct ata_dev* current_ata_device = NULL;
static ata_dev_t primary_master   = {.slave = 0};
static ata_dev_t primary_slave    = {.slave = 1};
static ata_dev_t secondary_master = {.slave = 0};
static ata_dev_t secondary_slave  = {.slave = 1};


#pragma region [Other]

    // Delay for working with ATA
    void _ata_wait() {
        int delay = 150000;
        while (--delay > 0)
            continue;
    }

    int _is_ata_ready() {
        if (current_ata_device == NULL || !current_ata_device->present) return 0;

        int timeout = 9000000;
        uint8_t status = 0;

        do {
            status = i386_inb(current_ata_device->status);
            if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) return 0;
            if (--timeout < 0) return 0;
        } while (status & ATA_SR_BSY);

        timeout = 9000000;
        while (!(status & ATA_STATUS_DRQ)) {
            status = i386_inb(current_ata_device->status);
            if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) return 0;
            if (--timeout < 0) return 0;
        }

        return 1;
    }

#pragma endregion


//====================
//  ATA init (found devices)
//  - Find primary drive
//  - Find slave drive
//  - Find Secondary drive
//  - Find secondary slave drive
//====================

#pragma region [initialization]

    int ATA_initialize() {
        ata_device.bits = 0;

        kprintf("ATA PROBE PM...\n");
        if (ATA_device_detect(&primary_master, 1)) {
            current_ata_device = &primary_master;
        }

        if (current_ata_device == NULL) {
            kprintf("ATA PROBE PS...\n");
            if (ATA_device_detect(&primary_slave, 1)) {
                current_ata_device = &primary_slave;
            }
        }

        if (current_ata_device == NULL) {
            kprintf("ATA PROBE SM...\n");
            if (ATA_device_detect(&secondary_master, 0)) {
                current_ata_device = &secondary_master;
            }
        }

        if (current_ata_device == NULL) {
            kprintf("ATA PROBE SS...\n");
            if (ATA_device_detect(&secondary_slave, 0)) {
                current_ata_device = &secondary_slave;
            }
        }

        if (current_ata_device == NULL) {
            kprintf("ATA_initialize: no ATA disk found\n");
            return 0;
        }

        kprintf("ATA VFS INIT...\n");
        if (!VFS_initialize(current_ata_device, FAT_FS)) {
            kprintf("ATA_initialize: VFS init failed\n");
            return 0;
        }

        kprintf("ATA READY\n");
        return 1;
    }

    void ATA_handler(struct Registers* reg) {
        if (current_ata_device == NULL) return;

        i386_inb(current_ata_device->status);
        if (current_ata_device->BMR_STATUS) i386_inb(current_ata_device->BMR_STATUS);
        if (current_ata_device->BMR_COMMAND) i386_outb(current_ata_device->BMR_COMMAND, BMR_COMMAND_DMA_STOP);
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
            kprintf("ATA_device_detect: device does not exist\n");
            dev->prdt = NULL;
            return 0;
        }

        uint8_t lba_lo = i386_inb(dev->lba_lo);
        uint8_t lba_mid = i386_inb(dev->lba_mid);
        uint8_t lba_high = i386_inb(dev->lba_high);
        if (lba_lo != 0 || lba_mid != 0 || lba_high != 0) {
            kprintf("ATA_device_detect: not ata device\n");
            dev->prdt = NULL;
            return 0;
        }

        int timeout = 9999999;
        while (status & ATA_SR_BSY) {
            status = i386_inb(dev->status);
            if (--timeout < 0) {
                kprintf("DRIVE [%i] NOT FOUND / ATTACHED\n");
                dev->prdt = NULL;
                return 0;
            }
        }

        if (status & (ATA_STATUS_ERR | ATA_STATUS_DF)) {
            kprintf("ATA_device_detect: err when polling\n");
            dev->prdt = NULL;
            return 0;
        }

        if (!(status & ATA_STATUS_DRQ)) {
            kprintf("ATA_device_detect: DRQ not set\n");
            dev->prdt = NULL;
            return 0;
        }

        for (int i = 0; i < 256; i++) i386_inw(dev->data);

        if (ata_device.bits != 0) {
            uint32_t pci_command_reg = pci_read(ata_device, PCI_COMMAND);
            if (!(pci_command_reg & (1 << 2))) {
                pci_command_reg |= (1 << 2);
                pci_write(ata_device, PCI_COMMAND, pci_command_reg);
            }
        }

        dev->present = 1;
        kprintf("DRIVE [%s] FOUND (DATA PORT: %x)\n", dev->mountpoint, dev->data);
        return 1;
    }

    void ATA_device_init(ata_dev_t* dev, int primary) {
        memset(dev, 0, sizeof(ata_dev_t));
        dev->slave = dev == &primary_slave || dev == &secondary_slave;

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

        dev->bar4 = ata_device.bits != 0 ? pci_read(ata_device, PCI_BAR4) : 0;
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

#pragma endregion

//====================
//  ATA init (found devices)
//====================
//  ATA device switch
//  - Note: Don`t forgot to perform all OS preparations before this action
//====================

    void ATA_device_switch(int device) {
        switch (device) {
            case 1: current_ata_device = &primary_master; break;
            case 2: current_ata_device = &primary_slave; break;
            case 3: current_ata_device = &secondary_master; break;
            case 4: current_ata_device = &secondary_slave; break;
        }
    }

//====================
//  ATA device switch
//====================
//  READ SECTOR FROM DISK BY LBA ADRESS
//====================

#pragma region [Read operations]

    void _prepare_for_reading(uint32_t lba) {
        if (current_ata_device == NULL) return;
        i386_outb(current_ata_device->drive, 0xE0 | (current_ata_device->slave << 4) | ((lba >> 24) & 0x0F));
        i386_outb(current_ata_device->error, 0x00);
        i386_outb(current_ata_device->sector_count, 1);
        i386_outb(current_ata_device->lba_lo, (uint8_t)(lba & 0xFF));
        i386_outb(current_ata_device->lba_mid, (uint8_t)((lba >> 8) & 0xFF));
        i386_outb(current_ata_device->lba_high, (uint8_t)((lba >> 16) & 0xFF));
        i386_outb(current_ata_device->command, ATA_CMD_READ_PIO);
    }

    uint8_t* ATA_read_sector(uint32_t lba) {
        _ata_wait();
        uint8_t* buffer = (uint8_t*)ALC_malloc(SECTOR_SIZE, KERNEL);
        if (!buffer) return NULL;

        _prepare_for_reading(lba);
        if (!_is_ata_ready()) {
            ALC_free(buffer, KERNEL);
            return NULL;
        }

        for (int n = 0; n < SECTOR_SIZE / 2; n++) {
            uint16_t value = i386_inw(current_ata_device->data);
            buffer[n * 2] = value & 0xFF;
            buffer[n * 2 + 1] = value >> 8;
        }

        return buffer;
    }

    // Return two values
    // stop == ERROR_SYMBOL (error), stop == STOP_SYMBOL (found)
    uint8_t* ATA_read_sector_stop(uint32_t lba, uint8_t* stop) {
        _ata_wait();
        uint8_t* buffer = (uint8_t*)ALC_malloc(SECTOR_SIZE, KERNEL);
        if (!buffer) return NULL;

        uint8_t dummy_buffer[SECTOR_SIZE] = { 0 };
        uint8_t* buffer_pointer = buffer;
        memset(buffer, 0, SECTOR_SIZE);
        
        _prepare_for_reading(lba);
        if (!_is_ata_ready()) {
            ALC_free(buffer, KERNEL);
            return NULL;
        }

        for (int n = 0; n < SECTOR_SIZE / 2; n++) {
            uint16_t value = i386_inw(current_ata_device->data);
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

        return buffer;
    }

    // TODO: issue with pointer moving 
    uint8_t* ATA_read_sector_stopoff(uint32_t lba, uint32_t offset, uint8_t* stop) {
        _ata_wait();
        uint8_t* buffer = (uint8_t*)ALC_malloc(SECTOR_SIZE, KERNEL);
        if (!buffer) return NULL;

        uint8_t dummy_buffer[SECTOR_SIZE] = { 0 };
        uint8_t* buffer_pointer = buffer;
        memset(buffer, 0, SECTOR_SIZE);
        
        _prepare_for_reading(lba);
        if (!_is_ata_ready()) {
            ALC_free(buffer, KERNEL);
            return NULL;
        }
        
        for (int n = 0; n < SECTOR_SIZE / 2; n++) {
            uint16_t value = i386_inw(current_ata_device->data);
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
        
        return buffer;
    }

    // Function to read a sectors from the disk.
    uint8_t* ATA_read_sectors(uint32_t lba, uint32_t sector_count) {
        _ata_wait();
        uint8_t* buffer = (uint8_t*)ALC_malloc(SECTOR_SIZE * sector_count, KERNEL);
        if (!buffer) return NULL;

        memset(buffer, 0, SECTOR_SIZE * sector_count);
        for (uint32_t i = 0; i < sector_count; i++) {
            uint8_t* sector_data = ATA_read_sector(lba + i);
            if (sector_data == NULL) return NULL;
            
            memcpy(buffer + i * SECTOR_SIZE, sector_data, SECTOR_SIZE);
            ALC_free(sector_data, KERNEL);
        }

        return buffer;
    }

    uint8_t* ATA_readoff_sectors(uint32_t lba, uint32_t offset, uint32_t sector_count) {
        uint32_t sectors_seek = offset / SECTOR_SIZE;
        uint32_t data_seek = offset % SECTOR_SIZE;
        uint32_t size = (SECTOR_SIZE * (sector_count - 1)) + (SECTOR_SIZE - data_seek);
        uint8_t* buffer = (uint8_t*)ALC_malloc(size, KERNEL);
        if (!buffer) return NULL;

        memset(buffer, 0, size);
        for (uint32_t i = sectors_seek; i < sector_count; i++) {
            uint8_t* sector_data = ATA_read_sector(lba + i);
            if (!sector_data) return NULL;
            
            memcpy(buffer + i * (SECTOR_SIZE - data_seek), sector_data + data_seek, SECTOR_SIZE - data_seek);
            ALC_free(sector_data, KERNEL);

            data_seek = 0;
        }

        return buffer;
    }

    // Return two values
    // data[0] - Find (1) or not found (0) stop data in data
    // data[1] - Loaded data from disk
    uint8_t* ATA_read_sectors_stop(uint32_t lba, uint32_t sector_count, uint8_t* stop) {
        _ata_wait();
        uint8_t* buffer = (uint8_t*)ALC_malloc(SECTOR_SIZE * sector_count, KERNEL);
        if (buffer == NULL) return NULL;

        memset(buffer, 0, SECTOR_SIZE * sector_count);
        for (uint32_t i = 0; i < sector_count; i++) {
            uint8_t* sector_data = ATA_read_sector_stop(lba + i, stop);
            if (sector_data == NULL) return NULL;
            
            memcpy(buffer + i * SECTOR_SIZE, sector_data, SECTOR_SIZE);
            ALC_free(sector_data, KERNEL);

            if (*stop == STOP_SYMBOL) break;
        }

        return buffer;
    }

    // Read sectors with start seek
    // Stop reading when meet stop value
    uint8_t* ATA_readoff_sectors_stop(uint32_t lba, uint32_t offset, uint32_t sector_count, uint8_t* stop) {
        _ata_wait();

        uint32_t sectors_seek = offset / SECTOR_SIZE;
        uint32_t data_seek    = offset % SECTOR_SIZE;
        uint32_t size         = (SECTOR_SIZE * (sector_count - 1)) + (SECTOR_SIZE - data_seek);
        uint8_t* buffer = (uint8_t*)ALC_malloc(size, KERNEL);
        if (buffer == NULL) return NULL;

        memset(buffer, 0, size);
        for (uint32_t i = sectors_seek; i < sector_count; i++) {
            uint8_t* sector_data = ATA_read_sector_stopoff(lba + i, data_seek, stop);
            if (sector_data == NULL) return NULL;
            
            memcpy(buffer + i * (SECTOR_SIZE - data_seek), sector_data + data_seek, SECTOR_SIZE - data_seek);
            ALC_free(sector_data, KERNEL);

            data_seek = 0;

            if (*stop == STOP_SYMBOL) break;
        }

        return buffer;
    }

#pragma endregion

//====================
//  READ SECTOR FROM DISK BY LBA ADRESS
//====================
//  WRITE DATA TO SECTOR ON DISK BY LBA ADDRESS
//====================

#pragma region [Write operations]

    void _prepare_for_writing(uint32_t lba) {
        if (current_ata_device == NULL) return;
        i386_outb(current_ata_device->drive, 0xE0 | (current_ata_device->slave << 4) | ((lba >> 24) & 0x0F));
        i386_outb(current_ata_device->error, 0x00);
        i386_outb(current_ata_device->sector_count, 1);
        i386_outb(current_ata_device->lba_lo, (uint8_t)lba);
        i386_outb(current_ata_device->lba_mid, (uint8_t)(lba >> 8));
        i386_outb(current_ata_device->lba_high, (uint8_t)(lba >> 16));
        i386_outb(current_ata_device->command, ATA_CMD_WRITE_PIO);
    }

    int ATA_write_sector(uint32_t lba, const uint8_t* buffer) {
        if (lba == BOOT_SECTOR) return -1;

        _ata_wait();
        _prepare_for_writing(lba);

        int timeout = 9000000;
        while ((i386_inb(current_ata_device->status) & ATA_SR_BSY) != 0)
            if (--timeout < 0) return -1;
            else continue;
        
        for (int i = 0; i < SECTOR_SIZE / 2; i++) {
            uint16_t data = *((uint16_t*)(buffer + i * 2));
            i386_outw(current_ata_device->data, data);
        }

        return 1;
    }

    int ATA_writeoff_sector(uint32_t lba, const uint8_t* buffer, uint32_t offset, uint32_t size) {
        if (lba == BOOT_SECTOR) return -1;

        _ata_wait();
        _prepare_for_writing(lba);

        int timeout = 9000000;
        while ((i386_inb(current_ata_device->status) & ATA_SR_BSY) != 0)
            if (--timeout < 0) return -1;
            else continue;
        
        for (int i = offset / 2; i < min(size / 2, (SECTOR_SIZE / 2) - offset); i++) {
            uint16_t data = *((uint16_t*)(buffer + i * 2));
            i386_outw(current_ata_device->data, data);
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
            if (ATA_writeoff_sector(lba + i, buffer, sector_seek, write_size) == -1) 
                return -1;
            
            buffer += write_size;
            data_position += write_size;
            sector_seek = 0;
        }

        return 1;
    }

    int ATA_copy_sectors2sectors(uint32_t source_lba, uint32_t sector_count, uint32_t distenation_lba) {
        _ata_wait();
        uint8_t* source = ATA_read_sectors(source_lba, sector_count);
        int result = ATA_write_sectors(distenation_lba, source, sector_count);
        ALC_free(source, KERNEL);
        return result;
    }

#pragma endregion

//====================
//  WRITE DATA TO SECTOR ON DISK BY LBA ADRESS
//====================
