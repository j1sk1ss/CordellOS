// Thanks to https://wiki.osdev.org/ATA_PIO_Mode and https://github.com/szhou42/osdev/blob/master/src/kernel/drivers/ata.c#L267
#include "../../include/ata.h"


pci_dev_t ata_device = { 0 };
struct ata_dev* current_ata_device = NULL;

ata_dev_t primary_master   = {.slave = 0};
ata_dev_t primary_slave    = {.slave = 1};
ata_dev_t secondary_master = {.slave = 0};
ata_dev_t secondary_slave  = {.slave = 1};


//====================
//  ATA init (found devices)
//  - Find primary drive
//  - Find slave drive
//  - Find Secondary drive
//  - Find secondary slave drive
//====================

#pragma region [initialization]

    void ATA_initialize() {
        ata_device = pci_get_device(ATA_VENDOR_ID, ATA_DEVICE_ID, -1);

        i386_irq_registerHandler(14, ATA_handler);

        ATA_device_detect(&primary_master,   1);
        ATA_device_detect(&primary_slave,    1);
        ATA_device_detect(&secondary_master, 0);
        ATA_device_detect(&secondary_slave,  0);

        current_ata_device = &primary_master;
        VFS_initialize(&primary_master, FAT_FS);
    }

    void ATA_handler(struct Registers* reg) {
        i386_inb(primary_master.status);
        i386_inb(primary_master.BMR_STATUS);
        i386_outb(primary_master.BMR_COMMAND, BMR_COMMAND_DMA_STOP);
    }

    void ATA_software_reset(ata_dev_t* dev) {
        i386_outb(dev->control, CONTROL_SOFTWARE_RESET);
        i386_io_wait(dev);
        i386_outb(dev->control, CONTROL_ZERO);
    }

    void ATA_device_detect(ata_dev_t* dev, int primary) {
        ATA_device_init(dev, primary);
        ATA_software_reset(dev);
        i386_io_wait(dev);

        i386_outb(dev->drive, (0xA + dev->slave) << 4);
        i386_outb(dev->sector_count, 0);
        i386_outb(dev->lba_lo, 0);
        i386_outb(dev->lba_mid, 0);
        i386_outb(dev->lba_high, 0);

        i386_outb(dev->command, COMMAND_IDENTIFY);
        if (!i386_inb(dev->status)) {
            kprintf("ATA_device_detect: device does not exist\n");
            _kfree(dev->prdt);
            _kfree(dev->mem_buffer);
            return;
        }

        uint8_t lba_lo = i386_inb(dev->lba_lo);
        uint8_t lba_hi = i386_inb(dev->lba_high);
        if (lba_lo != 0 || lba_hi != 0) {
            kprintf("ATA_device_detect: not ata device\n");
            _kfree(dev->prdt);
            _kfree(dev->mem_buffer);
            return;
        }

        uint8_t drq = 0, err = 0;
        int delay = 9999999;
        while (!drq && !err) {
            drq = i386_inb(dev->status) & ATA_STATUS_DRQ;
            err = i386_inb(dev->status) & ATA_STATUS_ERR;
            if (--delay < 0) {
                kprintf("DRIVE [%i] NOT FOUND / ATTACHED\n");
                _kfree(dev->prdt);
                _kfree(dev->mem_buffer);
                return;
            }
        }

        if (err) {
            kprintf("ATA_device_detect: err when polling\n");
            _kfree(dev->prdt);
            _kfree(dev->mem_buffer);
            return;
        }

        for (int i = 0; i < 256; i++) i386_inw(dev->data);

        uint32_t pci_command_reg = pci_read(ata_device, PCI_COMMAND);
        if (!(pci_command_reg & (1 << 2))) {
            pci_command_reg |= (1 << 2);
            pci_write(ata_device, PCI_COMMAND, pci_command_reg);
        }

        kprintf("DRIVE [%i] FOUND\n", dev->drive);
    }

    void ATA_device_init(ata_dev_t* dev, int primary) {
        dev->prdt      = (prdt_t*)_kmalloc(sizeof(prdt_t));
        dev->prdt_phys = (uint8_t*)VMM_virtual2physical(dev->prdt);

        uint8_t* mem_buffer = dev->mem_buffer;
        dev->prdt[0].buffer_phys   = (uint32_t)mem_buffer;
        dev->prdt[0].transfer_size = SECTOR_SIZE;
        dev->prdt[0].mark_end      = MARK_END;

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

        dev->bar4 = pci_read(ata_device, PCI_BAR4);
        if(dev->bar4 & 0x1) dev->bar4 = dev->bar4 & 0xFFFFFFFC;

        dev->BMR_COMMAND = dev->bar4;
        dev->BMR_STATUS  = dev->bar4 + 2;
        dev->BMR_prdt    = dev->bar4 + 4;

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
        if (device == 1) current_ata_device = &primary_master;
        else if (device == 2) current_ata_device = &primary_slave;
        else if (device == 3) current_ata_device = &secondary_master;
        else if (device == 4) current_ata_device = &secondary_slave;
    }

//====================
//  ATA device switch
//====================
//  READ SECTOR FROM DISK BY LBA ADRESS
//====================

#pragma region [Read operations]

    void ATA_prepare4reading(uint32_t lba) {
        i386_outb(current_ata_device->drive, 0xE0 | (current_ata_device->slave << 4) | ((lba >> 24) & 0x0F));
        i386_outb(FEATURES_REGISTER, 0x00);
        i386_outb(current_ata_device->sector_count, 1);
        i386_outb(current_ata_device->lba_lo, (uint8_t)(lba & 0xFF));
        i386_outb(current_ata_device->lba_mid, (uint8_t)((lba >> 8) & 0xFF));
        i386_outb(current_ata_device->lba_high, (uint8_t)((lba >> 16) & 0xFF));
        i386_outb(STATUS_REGISTER, ATA_CMD_READ_PIO);
    }

    uint8_t* ATA_read_sector(uint32_t lba) {
        ATA_ata_wait();
        uint8_t* buffer = (uint8_t*)_kmalloc(SECTOR_SIZE);
        if (buffer == NULL) return NULL;

        ATA_prepare4reading(lba);
        if (ATA_ata_ready() == -1) {
            _kfree(buffer);
            return NULL;
        }

        for (int n = 0; n < SECTOR_SIZE / 2; n++) {
            uint16_t value = i386_inw(DATA_REGISTER);
            buffer[n * 2] = value & 0xFF;
            buffer[n * 2 + 1] = value >> 8;
        }

        return buffer;
    }

    // Return two values
    // stop == ERROR_SYMBOL (error), stop == STOP_SYMBOL (found)
    uint8_t* ATA_read_sector_stop(uint32_t lba, uint8_t* stop) {
        ATA_ata_wait();

        uint8_t* buffer = (uint8_t*)_kmalloc(SECTOR_SIZE);
        uint8_t dummy_buffer[SECTOR_SIZE] = { 0 };

        uint8_t* buffer_pointer = buffer;
        memset(buffer, 0, SECTOR_SIZE);
        
        ATA_prepare4reading(lba);
        if (ATA_ata_ready() == -1) {
            _kfree(buffer);
            return NULL;
        }

        for (int n = 0; n < SECTOR_SIZE / 2; n++) {
            uint16_t value = i386_inw(DATA_REGISTER);
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
        ATA_ata_wait();
        uint8_t* buffer = (uint8_t*)_kmalloc(SECTOR_SIZE);
        uint8_t dummy_buffer[SECTOR_SIZE] = { 0 };

        uint8_t* buffer_pointer = buffer;
        memset(buffer, 0, SECTOR_SIZE);
        
        ATA_prepare4reading(lba);
        if (ATA_ata_ready() == -1) {
            _kfree(buffer);
            return NULL;
        }
        
        for (int n = 0; n < SECTOR_SIZE / 2; n++) {
            uint16_t value = i386_inw(DATA_REGISTER);
            uint8_t first = (uint8_t)(value & 0xFF);
            buffer_pointer[n * 2] = first;
            if (first == stop[0] && n * 2 >= offset) {
                stop[0] = STOP_SYMBOL;
                buffer_pointer = dummy_buffer;
            }

            uint8_t second = (uint8_t)(value >> 8);
            buffer_pointer[n * 2 + 1] = second;
            if (second == stop[0] && n * 2 + 1 >= offset) {
                stop[0] = STOP_SYMBOL;
                buffer_pointer = dummy_buffer;
            }
        }
        
        return buffer;
    }

    // Function to read a sectors from the disk.
    uint8_t* ATA_read_sectors(uint32_t lba, uint32_t sector_count) {
        ATA_ata_wait();
        uint8_t* buffer = (uint8_t*)_kmalloc(SECTOR_SIZE * sector_count);
        if (buffer == NULL) return NULL;

        memset(buffer, 0, SECTOR_SIZE * sector_count);
        for (uint32_t i = 0; i < sector_count; i++) {
            uint8_t* sector_data = ATA_read_sector(lba + i);
            if (sector_data == NULL) return NULL;
            
            memcpy(buffer + i * SECTOR_SIZE, sector_data, SECTOR_SIZE);
            _kfree(sector_data);
        }

        return buffer;
    }

    uint8_t* ATA_readoff_sectors(uint32_t lba, uint32_t offset, uint32_t sector_count) {
        ATA_ata_wait();

        uint32_t sectors_seek = offset / SECTOR_SIZE;
        uint32_t data_seek    = offset % SECTOR_SIZE;
        uint32_t size         = (SECTOR_SIZE * (sector_count - 1)) + (SECTOR_SIZE - data_seek);

        uint8_t* buffer = (uint8_t*)_kmalloc(size);
        if (buffer == NULL) return NULL;

        memset(buffer, 0, size);
        for (uint32_t i = sectors_seek; i < sector_count; i++) {
            uint8_t* sector_data = ATA_read_sector(lba + i);
            if (sector_data == NULL) return NULL;
            
            memcpy(buffer + i * (SECTOR_SIZE - data_seek), sector_data + data_seek, SECTOR_SIZE - data_seek);
            _kfree(sector_data);

            data_seek = 0;
        }

        return buffer;
    }

    // Return two values
    // data[0] - Find (1) or not found (0) stop data in data
    // data[1] - Loaded data from disk
    uint8_t* ATA_read_sectors_stop(uint32_t lba, uint32_t sector_count, uint8_t* stop) {
        ATA_ata_wait();

        uint8_t* buffer = (uint8_t*)_kmalloc(SECTOR_SIZE * sector_count);
        if (buffer == NULL) return NULL;

        memset(buffer, 0, SECTOR_SIZE * sector_count);
        for (uint32_t i = 0; i < sector_count; i++) {
            uint8_t* sector_data = ATA_read_sector_stop(lba + i, stop);
            if (sector_data == NULL) return NULL;
            
            memcpy(buffer + i * SECTOR_SIZE, sector_data, SECTOR_SIZE);
            _kfree(sector_data);

            if (stop[0] == STOP_SYMBOL) break;
        }

        return buffer;
    }

    // Read sectors with start seek
    // Stop reading when meet stop value
    uint8_t* ATA_readoff_sectors_stop(uint32_t lba, uint32_t offset, uint32_t sector_count, uint8_t* stop) {
        ATA_ata_wait();

        uint32_t sectors_seek = offset / SECTOR_SIZE;
        uint32_t data_seek    = offset % SECTOR_SIZE;
        uint32_t size         = (SECTOR_SIZE * (sector_count - 1)) + (SECTOR_SIZE - data_seek);

        uint8_t* buffer = (uint8_t*)_kmalloc(size);
        if (buffer == NULL) return NULL;

        memset(buffer, 0, size);
        for (uint32_t i = sectors_seek; i < sector_count; i++) {
            uint8_t* sector_data = ATA_read_sector_stopoff(lba + i, data_seek, stop);
            if (sector_data == NULL) return NULL;
            
            memcpy(buffer + i * (SECTOR_SIZE - data_seek), sector_data + data_seek, SECTOR_SIZE - data_seek);
            _kfree(sector_data);

            data_seek = 0;

            if (stop[0] == STOP_SYMBOL) break;
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

    void ATA_prepare4writing(uint32_t lba) {
        i386_outb(current_ata_device->drive, 0xE0 | (current_ata_device->slave << 4) | ((lba >> 24) & 0x0F));
        i386_outb(FEATURES_REGISTER, 0x00);
        i386_outb(current_ata_device->sector_count, 1);
        i386_outb(current_ata_device->lba_lo, (uint8_t)lba);
        i386_outb(current_ata_device->lba_mid, (uint8_t)(lba >> 8));
        i386_outb(current_ata_device->lba_high, (uint8_t)(lba >> 16));
        i386_outb(STATUS_REGISTER, ATA_CMD_WRITE_PIO);
    }

    int ATA_write_sector(uint32_t lba, const uint8_t* buffer) {
        if (lba == BOOT_SECTOR) return -1;

        ATA_ata_wait();
        ATA_prepare4writing(lba);

        int timeout = 9000000;
        while ((i386_inb(STATUS_REGISTER) & ATA_SR_BSY) == 0) 
            if (--timeout < 0) return -1;
            else continue;
        
        for (int i = 0; i < SECTOR_SIZE / 2; i++) {
            uint16_t data = *((uint16_t*)(buffer + i * 2));
            i386_outw(DATA_REGISTER, data);
        }

        return 1;
    }

    int ATA_writeoff_sector(uint32_t lba, const uint8_t* buffer, uint32_t offset, uint32_t size) {
        if (lba == BOOT_SECTOR) return -1;

        ATA_ata_wait();
        ATA_prepare4writing(lba);

        int timeout = 9000000;
        while ((i386_inb(STATUS_REGISTER) & ATA_SR_BSY) == 0) 
            if (--timeout < 0) return -1;
            else continue;
        
        for (int i = offset / 2; i < min(size / 2, (SECTOR_SIZE / 2) - offset); i++) {
            uint16_t data = *((uint16_t*)(buffer + i * 2));
            i386_outw(DATA_REGISTER, data);
        }

        return 1;
    }

    // Function to write a sector on the disk.
    int ATA_write_sectors(uint32_t lba, const uint8_t* buffer, uint32_t sector_count) {
        ATA_ata_wait();
        for(uint32_t i = 0; i < sector_count; i++) {
            if (ATA_write_sector(lba + i, buffer) == -1) 
                return -1;
            
            buffer += SECTOR_SIZE;
        }

        return 1;
    }

    int ATA_writeoff_sectors(uint32_t lba, const uint8_t* buffer, uint32_t sector_count, uint32_t offset, uint32_t size) {
        ATA_ata_wait();

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
        ATA_ata_wait();
        uint8_t* source = ATA_read_sectors(source_lba, sector_count);
        int result = ATA_write_sectors(distenation_lba, source, sector_count);
        _kfree(source);
        return result;
    }

#pragma endregion

//====================
//  WRITE DATA TO SECTOR ON DISK BY LBA ADRESS
//====================
//  OTHER FUNCTIONS
//====================

#pragma region [Other]

    // Function that add data to sector
    void ATA_append_sector(uint32_t lba, uint8_t* data, uint32_t len, uint32_t offset) {
        uint8_t buffer[SECTOR_SIZE] = { 0 };
        uint8_t* sector_data = ATA_read_sector(lba);
        memcpy(buffer, sector_data, SECTOR_SIZE);
        _kfree(sector_data);
        
        memcpy(buffer + offset, data, len);
        ATA_write_sector(lba, buffer);
    }

    // Function that clear sector
    int ATA_clear_sector(uint32_t lba) {
        uint8_t buffer[SECTOR_SIZE] = { 0 };
        if (ATA_write_sector(lba, buffer) == -1) {
            kprintf("\nPulizia del settore non completata!");
            return -1;
        }

        return 1;
    }

    // Function to check if a sector (by LBA) is empty (all bytes are zero)
    bool ATA_is_current_sector_empty(uint32_t lba) {
        uint8_t* sector_data = ATA_read_sector(lba);
        if (ATA_is_sector_empty(sector_data)) {
            _kfree(sector_data);
            return true;
        }

        _kfree(sector_data);
        return false;
    }

    // Function to check if a sector is empty (contains all zeros).
    bool ATA_is_sector_empty(const uint8_t* sector_data) {
        for (int i = 0; i < SECTOR_SIZE; i++) 
            if (sector_data[i] != 0x00) return false;

        return true;
    }

    // Function that find first empty sector
    uint32_t ATA_find_empty_sector(uint32_t offset) {
        for (uint32_t lba = offset; lba <= SECTOR_COUNT; lba++) {
            uint8_t* sector_data = ATA_read_sector(lba);
            if (sector_data == NULL) continue;
            if (ATA_is_sector_empty((const uint8_t*)sector_data)) {
                _kfree(sector_data);
                return lba;
            }
                
            _kfree(sector_data);
        }

        return -1;
    }

    // Delay for working with ATA
    void ATA_ata_wait() {
        int delay = 150000;
        while (--delay > 0)
            continue;
    }

    int ATA_ata_ready() {
        int timeout = 9000000;
        while ((i386_inb(STATUS_REGISTER) & ATA_SR_BSY) == 0) 
            if (--timeout < 0) return -1;

        return 1;
    }

    // Function for getting avaliable sector count in disk
    int ATA_global_sector_count() {
        int sectors = 0;
        for (uint32_t lba = 0; lba < SECTOR_COUNT; lba++) {
            uint8_t* sector_data = ATA_read_sector(lba);
            if (sector_data != NULL) {
                sectors++;
                _kfree(sector_data);
            }
        }

        return sectors;
    }

    // Function for getting empty sector count
    int ATA_global_sector_empty() {
        int sectors = 0;
        for (uint32_t lba = 0; lba < SECTOR_COUNT; lba++) {
            uint8_t* sector_data = ATA_read_sector(lba);
            if (sector_data != NULL) {
                if (ATA_is_sector_empty((const uint8_t*)sector_data)) sectors++;
                _kfree(sector_data);
            }
        }

        return sectors;
    }

#pragma endregion

//====================
//  OTHER FUNCTIONS
//====================