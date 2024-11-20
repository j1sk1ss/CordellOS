#ifndef FAT_H_
#define FAT_H_

#include "ata.h"       // Lib for reading and writing ATA PIO sectors
#include "elf.h"       // Not important for base implementation. ELF executer
#include "date_time.h" // Not important for base implementation. Date time getter from CMOS

#include <memory.h>
#include <stdlib.h>  // Allocators (basic malloc required)
#include <string.h>
#include <fslib.h>
#include <ctype.h>
#include <assert.h>


#define SECTOR_OFFSET		23000

#define END_CLUSTER_32      0x0FFFFFF8
#define BAD_CLUSTER_32      0x0FFFFFF7
#define FREE_CLUSTER_32     0x00000000
#define END_CLUSTER_16      0xFFF8
#define BAD_CLUSTER_16      0xFFF7
#define FREE_CLUSTER_16     0x0000
#define END_CLUSTER_12      0xFF8
#define BAD_CLUSTER_12      0xFF7
#define FREE_CLUSTER_12     0x000

#define CLEAN_EXIT_BMASK_16 0x8000
#define HARD_ERR_BMASK_16   0x4000
#define CLEAN_EXIT_BMASK_32 0x08000000
#define HARD_ERR_BMASK_32   0x04000000

#define FILE_READ_ONLY      0x01
#define FILE_HIDDEN         0x02
#define FILE_SYSTEM         0x04
#define FILE_VOLUME_ID      0x08
#define FILE_DIRECTORY      0x10
#define FILE_ARCHIVE        0x20

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

#define GET_CLUSTER_FROM_ENTRY(x, fat_type)       (x.low_bits | (x.high_bits << (fat_type / 2)))
#define GET_CLUSTER_FROM_PENTRY(x, fat_type)      (x->low_bits | (x->high_bits << (fat_type / 2)))

#define GET_ENTRY_LOW_BITS(x, fat_type)           ((x) & ((1 << (fat_type / 2)) - 1))
#define GET_ENTRY_HIGH_BITS(x, fat_type)          ((x) >> (fat_type / 2))
#define CONCAT_ENTRY_HL_BITS(high, low, fat_type) ((high << (fat_type / 2)) | low)

#ifndef NULL
#define NULL 0
#endif

#ifndef BOOL
#define BOOL short
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif


/* Bpb taken from http://wiki.osdev.org/FAT */

//FAT directory and bootsector structures
typedef struct fat_extBS_32 {
	unsigned int		table_size_32;
	unsigned short		extended_flags;
	unsigned short		fat_version;
	unsigned int		root_cluster;
	unsigned short		fat_info;
	unsigned short		backup_BS_sector;
	unsigned char 		reserved_0[12];
	unsigned char		drive_number;
	unsigned char 		reserved_1;
	unsigned char		boot_signature;
	unsigned int 		volume_id;
	unsigned char		volume_label[11];
	unsigned char		fat_type_label[8];

} __attribute__((packed)) fat_extBS_32_t;

typedef struct fat_extBS_16 {
	unsigned char		bios_drive_num;
	unsigned char		reserved1;
	unsigned char		boot_signature;
	unsigned int		volume_id;
	unsigned char		volume_label[11];
	unsigned char		fat_type_label[8];

} __attribute__((packed)) fat_extBS_16_t;

typedef struct fat_BS {
	unsigned char 		bootjmp[3];
	unsigned char 		oem_name[8];
	unsigned short 	    bytes_per_sector;
	unsigned char		sectors_per_cluster;
	unsigned short		reserved_sector_count;
	unsigned char		table_count;
	unsigned short		root_entry_count;
	unsigned short		total_sectors_16;
	unsigned char		media_type;
	unsigned short		table_size_16;
	unsigned short		sectors_per_track;
	unsigned short		head_side_count;
	unsigned int 		hidden_sector_count;
	unsigned int 		total_sectors_32;

	unsigned char		extended_section[54];

} __attribute__((packed)) fat_BS_t;

/* from http://wiki.osdev.org/FAT */

typedef struct fsInfo {
	unsigned int  lead_signature;      //should contain 0x41615252
	unsigned char reserved1[480];
	
	unsigned int  structure_signature; //should contain 0x61417272
	unsigned int  free_space;          //contains last known free cluster count. 0xFFFFFFFF indicates count is unknown.
	unsigned int  last_written;        //contains last-written cluster number to help FAT drivers find a free cluster. 0xFFFFFFFF indicates that cluster number is unknown.
	
	unsigned char reserved2[12];
	unsigned int  trail_signature;     //should contain 0xAA550000

} __attribute__((packed)) FSInfo_t;

typedef struct long_entry {
	unsigned char order;
	unsigned char first_five[10];      //first 5, 2-byte characters
	unsigned char attributes;          //MUST BE FILE_LONG_NAME
	unsigned char type;                //indicates a sub-component of a long name (leave as 0)
	unsigned char checksum;
	unsigned char next_six[12];        //next 6, 2-byte characters

	unsigned short zero;               //must be zero - otherwise meaningless
	unsigned char last_two[4];         //last 2, 2-byte characters

} __attribute__((packed)) long_entry_t;

/* From file_system.h (CordellOS brunch FS_based_on_FAL) */

typedef struct fat_data {
	unsigned int fat_size;
	unsigned int fat_type;
	unsigned int first_fat_sector;
	unsigned int first_data_sector;
	unsigned int total_sectors;
	unsigned int total_clusters;
	unsigned int bytes_per_sector;
	unsigned int sectors_per_cluster;
	unsigned int ext_root_cluster;
} fat_data_t;

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
	int FAT_read(unsigned int clusterNum);
	int FAT_write(unsigned int clusterNum, unsigned int clusterVal);

//===================================


//===================================
//    ____ _    _   _ ____ _____ _____ ____  
//   / ___| |  | | | / ___|_   _| ____|  _ \ 
//  | |   | |  | | | \___ \ | | |  _| | |_) |
//  | |___| |__| |_| |___) || | | |___|  _ < 
//   \____|_____\___/|____/ |_| |_____|_| \_\
//===================================

	unsigned int FAT_cluster_allocate();
	int FAT_cluster_deallocate(const unsigned int cluster);
	uint8_t* FAT_cluster_read(unsigned int clusterNum);
	uint8_t* FAT_cluster_read_stop(unsigned int clusterNum, uint8_t* stop);
	uint8_t* FAT_cluster_readoff(unsigned int clusterNum, uint32_t offset);
	uint8_t* FAT_cluster_readoff_stop(unsigned int clusterNum, uint32_t offset, uint8_t* stop);
	int FAT_cluster_write(void* contentsToWrite, unsigned int clusterNum);
	int FAT_cluster_writeoff(void* contentsToWrite, unsigned int clusterNum, uint32_t offset, uint32_t size);
	int FAT_cluster_clear(unsigned int clusterNum);
	void FAT_add_cluster2content(Content* content);
	int FAT_cpy_cluster2cluster(unsigned int firstCluster, unsigned int secondCluster);

//===================================


//===================================
//   _____ _   _ _____ ______   __
//  | ____| \ | |_   _|  _ \ \ / /
//  |  _| |  \| | | | | |_) \ V / 
//  | |___| |\  | | | |  _ < | |  
//  |_____|_| \_| |_| |_| \_\|_| 
//===================================

	Directory* FAT_directory_list(const unsigned int cluster, unsigned char attributesToAdd, short exclusive);
	int FAT_directory_search(const char* filepart, const unsigned int cluster, directory_entry_t* file, unsigned int* entryOffset);
	int FAT_directory_add(const unsigned int cluster, directory_entry_t* file_to_add);
	int FAT_directory_remove(const unsigned int cluster, const char* fileName);
	int FAT_directory_edit(const unsigned int cluster, directory_entry_t* oldMeta, directory_entry_t* newMeta);

//===================================


//===================================
//    ____ ___  _   _ _____ _____ _   _ _____ 
//   / ___/ _ \| \ | |_   _| ____| \ | |_   _|
//  | |  | | | |  \| | | | |  _| |  \| | | |  
//  | |__| |_| | |\  | | | | |___| |\  | | |  
//   \____\___/|_| \_| |_| |_____|_| \_| |_|  
//===================================

	int FAT_content_exists(const char* filePath);
	Content* FAT_get_content(const char* filePath);
	char* FAT_read_content(Content* data);
	char* FAT_read_content_stop(Content* data, uint8_t* stop);
	void FAT_read_content2buffer(Content* data, uint8_t* buffer, uint32_t offset, uint32_t size);
	void FAT_read_content2buffer_stop(Content* data, uint8_t* buffer, uint32_t offset, uint32_t size, uint8_t* stop);
	int FAT_put_content(const char* filePath, Content* content);
	int FAT_delete_content(const char* path);
	int FAT_write_content(Content* content, char* content_data);
	void FAT_write_buffer2content(Content* data, uint8_t* buffer, uint32_t offset, uint32_t size);
	int FAT_ELF_execute_content(char* path, int argc, char* argv[], int type);
	int FAT_change_meta(const char* filePath, directory_entry_t* newMeta);

//===================================

//===================================
//    ___ _____ _   _ _____ ____  
//   / _ \_   _| | | | ____|  _ \ 
//  | | | || | | |_| |  _| | |_) |
//  | |_| || | |  _  | |___|  _ < 
//   \___/ |_| |_| |_|_____|_| \_\
//=================================== 

	unsigned short FAT_current_time();
	unsigned short FAT_current_date();
	unsigned char FAT_current_time_temths();
	void FAT_fatname2name(char* input, char* output);
	char* FAT_name2fatname(char* input);
	int FAT_name_check(const char* input);
	unsigned char FAT_check_sum(unsigned char *pFcbName);

	directory_entry_t* FAT_create_entry(const char* name, const char* ext, BOOL isDir, uint32_t firstCluster, uint32_t filesize);
	Content* FAT_create_content(char* name, BOOL directory, char* extension);

//===================================

#endif