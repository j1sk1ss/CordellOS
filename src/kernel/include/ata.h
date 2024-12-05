#ifndef ATA_H_
#define ATA_H_

#include <stdint.h>
#include <memory.h>
#include <stdlib.h>
#include <string.h>

#include "x86.h"
#include "kstdio.h"
#include "irq.h"
#include "pci.h"
#include "vmm.h"
#include "vfs.h"


// ATA PCI info
#define ATA_VENDOR_ID               0x8086
#define ATA_DEVICE_ID               0x7010
#define MARK_END                    0x8000

// Control reg
#define CONTROL_STOP_INTERRUPT      0x2
#define CONTROL_SOFTWARE_RESET      0x4
#define CONTROL_HIGH_ORDER_BYTE     0x80
#define CONTROL_ZERO                0x00

// Command reg
#define COMMAND_IDENTIFY            0xEC
#define COMMAND_DMA_READ            0xC8
#define ATA_CMD_READ_PIO            0x20

#define BOOT_SECTOR                 0
#define SECTOR_SIZE                 512
#define SECTOR_COUNT                40000

#define DATA_REGISTER               0x1F0
#define FEATURES_REGISTER           0x1F1
#define SECTOR_COUNT_REGISTER       0x1F2
#define LBA_ADRESS_REGISTER         0x1F3
#define CYLINDER_LOW_REGISTER       0x1F4
#define CYLINDER_HIGH_REGISTER      0x1F5
#define DRIVE_REGISTER              0x1F6
#define STATUS_REGISTER             0x1F7

// Status reg
#define ATA_STATUS_ERR              0x0
#define ATA_STATUS_DRQ              0x8
#define ATA_STATUS_SRV              0x10
#define ATA_STATUS_DF               0x20
#define ATA_STATUS_RDY              0x40
#define ATA_SR_BSY                  0x8    // Busy

#define ATA_CMD_READ_PIO            0x20
#define ATA_CMD_WRITE_PIO           0x30

#define ATAPI_CMD_READ              0xA8
#define ATAPI_CMD_EJECT             0x1B

// Bus Master Reg Command
#define BMR_COMMAND_DMA_START 	0x1
#define BMR_COMMAND_DMA_STOP 	0x0
#define BMR_COMMAND_READ 		0x8
#define BMR_STATUS_INT 			0x4
#define BMR_STATUS_ERR 			0x2

#define STOP_SYMBOL		'\20'
#define ERROR_SYMBOL	'\21'


typedef struct prdt {
	uint32_t buffer_phys;
	uint16_t transfer_size;
	uint16_t mark_end;
}__attribute__((packed)) prdt_t;

typedef struct ata_dev {
	uint16_t data;
	uint16_t error;
	uint16_t sector_count;

	union {
		uint16_t sector_num;
		uint16_t lba_lo ;
	};

	union {
		uint16_t cylinder_low;
		uint16_t lba_mid ;
	};

	union {
		uint16_t cylinder_high;
		uint16_t lba_high;
	};

	union {
		uint16_t drive;
		uint16_t head;
	};

	union {
		uint16_t command;
		uint16_t status;
	};

	union {
		uint16_t control;
		uint16_t alt_status;
	};

	int slave;
	uint32_t bar4;
	uint32_t BMR_COMMAND;
	uint32_t BMR_prdt;
	uint32_t BMR_STATUS;


	prdt_t* prdt;
	uint8_t* prdt_phys;

	uint8_t mem_buffer[PAGE_SIZE];
	uint8_t mem_buffer_phys[PAGE_SIZE];

	char mountpoint[32];
}__attribute__((packed)) ata_dev_t;


void ATA_initialize();

struct Registers;
void ATA_handler(struct Registers* reg);
void ATA_device_switch(int device);

void ATA_software_reset(ata_dev_t* dev);
void ATA_device_detect(ata_dev_t* dev, int primary);
void ATA_device_init(ata_dev_t* dev, int primary);

bool ATA_is_sector_empty(const uint8_t* sector_data);
bool ATA_is_current_sector_empty(uint32_t LBA);

void ATA_ata_wait();
int ATA_ata_ready();

void ATA_prepare4reading(uint32_t lba);
uint8_t* ATA_read_sector(uint32_t lba);
uint8_t* ATA_read_sector_stop(uint32_t lba, uint8_t* stop);
uint8_t* ATA_read_sector_stopoff(uint32_t lba, uint32_t offset, uint8_t* stop);
uint8_t* ATA_read_sectors(uint32_t lba, uint32_t sector_count);
uint8_t* ATA_readoff_sectors(uint32_t lba, uint32_t offset, uint32_t sector_count);
uint8_t* ATA_read_sectors_stop(uint32_t lba, uint32_t sector_count, uint8_t* stop);
uint8_t* ATA_readoff_sectors_stop(uint32_t lba, uint32_t offset, uint32_t sector_count, uint8_t* stop);

void ATA_prepare4writing(uint32_t lba);
int ATA_write_sector(uint32_t lba, const uint8_t* buffer);
int ATA_writeoff_sector(uint32_t lba, const uint8_t* buffer, uint32_t offset, uint32_t size);
int ATA_write_sectors(uint32_t lba, const uint8_t* buffer, uint32_t sector_count);
int ATA_writeoff_sectors(uint32_t lba, const uint8_t* buffer, uint32_t sector_count, uint32_t offset, uint32_t size);
int ATA_cpy_sectors2sectors(uint32_t source_lba, uint32_t sector_count, uint32_t distenation_lba);

void ATA_append_sector(uint32_t lba, uint8_t* data, uint32_t len, uint32_t offset);
int ATA_clear_sector(uint32_t LBA);

uint32_t ATA_find_empty_sector(uint32_t offset);

int ATA_global_sector_count();
int ATA_global_sector_empty();


#endif