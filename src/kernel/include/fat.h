#ifndef FAT_H_
#define FAT_H_

#include <stddef.h>
#include <memory.h>
#include <stdlib.h>  // Allocators (basic malloc required)
#include <string.h>
#include <fslib.h>
#include <ctype.h>
#include <assert.h>
#include <stdint.h>

#include "ata.h"      // Lib for reading and writing ATA PIO sectors
#include "elf.h"      // Not important for base implementation. ELF executer
#include "kstdio.h"
#include "datetime.h" // Not important for base implementation. Date time getter from CMOS


#define SECTOR_OFFSET			23000

#define END_CLUSTER_32      	0x0FFFFFF8
#define BAD_CLUSTER_32      	0x0FFFFFF7
#define FREE_CLUSTER_32     	0x00000000
#define END_CLUSTER_16      	0xFFF8
#define BAD_CLUSTER_16      	0xFFF7
#define FREE_CLUSTER_16     	0x0000
#define END_CLUSTER_12      	0xFF8
#define BAD_CLUSTER_12      	0xFF7
#define FREE_CLUSTER_12     	0x000

#define CLEAN_EXIT_BMASK_16 	0x8000
#define HARD_ERR_BMASK_16   	0x4000
#define CLEAN_EXIT_BMASK_32 	0x08000000
#define HARD_ERR_BMASK_32   	0x04000000

#define FILE_LONG_NAME 			(FILE_READ_ONLY|FILE_HIDDEN|FILE_SYSTEM|FILE_VOLUME_ID)
#define FILE_LONG_NAME_MASK 	(FILE_READ_ONLY|FILE_HIDDEN|FILE_SYSTEM|FILE_VOLUME_ID|FILE_DIRECTORY|FILE_ARCHIVE)

#define FILE_LAST_LONG_ENTRY    0x40
#define ENTRY_FREE              0xE5
#define ENTRY_END               0x00
#define ENTRY_JAPAN             0x05
#define LAST_LONG_ENTRY         0x40

#define LOWERCASE_ISSUE	        0x01
#define BAD_CHARACTER	        0x02
#define BAD_TERMINATION         0x04
#define NOT_CONVERTED_YET       0x08
#define TOO_MANY_DOTS           0x10

#define FILE_READ_ONLY 			0x01
#define FILE_HIDDEN    			0x02
#define FILE_SYSTEM    			0x04
#define FILE_VOLUME_ID 			0x08
#define FILE_DIRECTORY 			0x10
#define FILE_ARCHIVE   			0x20

#define FILE_LAST_LONG_ENTRY 	0x40
#define ENTRY_FREE           	0xE5
#define ENTRY_END            	0x00
#define ENTRY_JAPAN          	0x05
#define LAST_LONG_ENTRY      	0x40

#define LOWERCASE_ISSUE	  		0x01
#define BAD_CHARACTER	  		0x02
#define BAD_TERMINATION   		0x04
#define NOT_CONVERTED_YET 		0x08
#define TOO_MANY_DOTS     		0x10

#define GET_CLUSTER_FROM_ENTRY(x, fat_type)       (x.low_bits | (x.high_bits << (fat_type / 2)))
#define GET_CLUSTER_FROM_PENTRY(x, fat_type)      (x->low_bits | (x->high_bits << (fat_type / 2)))

#define GET_ENTRY_LOW_BITS(x, fat_type)           ((x) & ((1 << (fat_type / 2)) - 1))
#define GET_ENTRY_HIGH_BITS(x, fat_type)          ((x) >> (fat_type / 2))
#define CONCAT_ENTRY_HL_BITS(high, low, fat_type) ((high << (fat_type / 2)) | low)

#define CONTENT_TABLE_SIZE	50

/* Bpb taken from http://wiki.osdev.org/FAT */

//FAT directory and bootsector structures
typedef struct fat_extBS_32 {
	uint32_t table_size_32;
	uint16_t extended_flags;
	uint16_t fat_version;
	uint32_t root_cluster;
	uint16_t fat_info;
	uint16_t backup_BS_sector;
	uint8_t  reserved_0[12];
	uint8_t	 drive_number;
	uint8_t  reserved_1;
	uint8_t	 boot_signature;
	uint32_t volume_id;
	uint8_t	 volume_label[11];
	uint8_t	 fat_type_label[8];
} __attribute__((packed)) fat_extBS_32_t;

typedef struct fat_BS {
	uint8_t  bootjmp[3];
	uint8_t  oem_name[8];
	uint16_t bytes_per_sector;
	uint8_t	 sectors_per_cluster;
	uint16_t reserved_sector_count;
	uint8_t	 table_count;
	uint16_t root_entry_count;
	uint16_t total_sectors_16;
	uint8_t	 media_type;
	uint16_t table_size_16;
	uint16_t sectors_per_track;
	uint16_t head_side_count;
	uint32_t hidden_sector_count;
	uint32_t total_sectors_32;
	uint8_t	 extended_section[54];
} __attribute__((packed)) fat_BS_t;

/* from http://wiki.osdev.org/FAT */
/* From file_system.h (CordellOS brunch FS_based_on_FAL) */

typedef struct fat_data {
	uint32_t fat_size;
	uint32_t fat_type;
	uint32_t first_data_sector;
	uint32_t total_sectors;
	uint32_t total_clusters;
	uint32_t bytes_per_sector;
	uint32_t cluster_size;
	uint32_t sectors_per_cluster;
	uint32_t ext_root_cluster;
	uint32_t first_fat_sector;
} fat_data_t;

typedef struct directory_entry {
	uint8_t file_name[11];
	uint8_t attributes;
	uint8_t reserved0;
	uint8_t creation_time_tenths;
	uint16_t creation_time;
	uint16_t creation_date;
	uint16_t last_accessed;
	uint16_t high_bits;
	uint16_t last_modification_time;
	uint16_t last_modification_date;
	uint16_t low_bits;
	uint32_t file_size;
} __attribute__((packed)) directory_entry_t;

typedef struct FATFile {
	char name[8];
	char extension[4];
	int data_size;
	uint32_t* data;
    struct FATFile* next;
} File;

typedef struct FATDirectory {
	char name[11];
	struct FATDirectory* next;
    struct FATFile* files;
    struct FATDirectory* subDirectory;
} Directory;

typedef struct FATContent {
	Directory* directory;
	File* file;
	uint32_t parent_cluster;
	directory_entry_t meta;
} Content;


//Global variables
extern fat_data_t FAT_data;

//===================================
//   _____  _    ____  _     _____ 
//  |_   _|/ \  | __ )| |   | ____|
//    | | / _ \ |  _ \| |   |  _|  
//    | |/ ___ \| |_) | |___| |___ 
//    |_/_/   \_\____/|_____|_____|
//===================================

	int FAT_initialize(); 
	int FAT_directory_list(int ci, uint8_t attrs, int exclusive);

//===================================
//    ____ ___  _   _ _____ _____ _   _ _____ 
//   / ___/ _ \| \ | |_   _| ____| \ | |_   _|
//  | |  | | | |  \| | | | |  _| |  \| | | |  
//  | |__| |_| | |\  | | | | |___| |\  | | |  
//   \____\___/|_| \_| |_| |_____|_| \_| |_|  
//===================================

	int FAT_content_exists(const char* path);
	int FAT_open_content(const char* path);
	int FAT_close_content(int ci);
	int FAT_read_content2buffer(int ci, uint8_t* buffer, uint32_t offset, uint32_t size);
	int FAT_read_content2buffer_stop(int ci, uint8_t* buffer, uint32_t offset, uint32_t size, uint8_t* stop);
	int FAT_put_content(const char* path, Content* content);
	int FAT_delete_content(const char* path);
	int FAT_write_buffer2content(int ci, uint8_t* buffer, uint32_t offset, uint32_t size);
	int FAT_ELF_execute_content(int ci, int argc, char* argv[], int type);
	int FAT_change_meta(const char* path, const char* new_name);
	int FAT_stat_content(int ci, CInfo_t* info);

//===================================
//    ___ _____ _   _ _____ ____  
//   / _ \_   _| | | | ____|  _ \ 
//  | | | || | | |_| |  _| | |_) |
//  | |_| || | |  _  | |___|  _ < 
//   \___/ |_| |_| |_|_____|_| \_\
//=================================== 

	uint16_t _current_time();
	uint16_t _current_date();
	void _fatname2name(char* input, char* output);
	char* _name2fatname(char* input);
	int _name_check(const char* input);

	int _add_content2table(Content* content);
	Content* FAT_get_content_from_table(int ci) ;
	int _remove_content_from_table(int index);

	Content* FAT_create_object(char* name, int is_directory, char* extension);
	Content* FAT_create_content();
	int FAT_unload_content_system(Content* content);
	Directory* _create_directory();
	File* _create_file();

//===================================

#endif