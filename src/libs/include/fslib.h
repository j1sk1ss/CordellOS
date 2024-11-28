#ifndef FSLIB_H_
#define FSLIB_H_

#include "stdio.h"
#include "stdlib.h"
#include "ctype.h"
#include "execute.h"


#define FILE_READ_ONLY 0x01
#define FILE_HIDDEN    0x02
#define FILE_SYSTEM    0x04
#define FILE_VOLUME_ID 0x08
#define FILE_DIRECTORY 0x10
#define FILE_ARCHIVE   0x20

#define FILE_LAST_LONG_ENTRY 0x40
#define ENTRY_FREE           0xE5
#define ENTRY_END            0x00
#define ENTRY_JAPAN          0x05
#define LAST_LONG_ENTRY      0x40

#define LOWERCASE_ISSUE	  0x01
#define BAD_CHARACTER	  0x02
#define BAD_TERMINATION   0x04
#define NOT_CONVERTED_YET 0x08
#define TOO_MANY_DOTS     0x10


typedef struct directory_entry {
	uint8_t file_name[11];
	uint8_t attributes;
	uint8_t reserved0; // Here place access
	uint8_t creation_time_tenths;
	unsigned short creation_time;
	unsigned short creation_date;
	unsigned short last_accessed;
	unsigned short high_bits;
	unsigned short last_modification_time;
	unsigned short last_modification_date;
	unsigned short low_bits;
	unsigned int file_size;
} __attribute__((packed)) directory_entry_t;

typedef struct FATFile {
	char name[8];
	char extension[4];
	directory_entry_t file_meta;
	int data_size;
	void* data_pointer;
	uint32_t* data;
    struct FATFile* next;
} File;

typedef struct FATDirectory {
	char name[12];
	directory_entry_t directory_meta;
	void* data_pointer;
	struct FATDirectory* next;
    struct FATFile* files;
    struct FATDirectory* subDirectory;
} Directory;

typedef struct FATDate {
	uint16_t hour;
	uint16_t minute;
	uint16_t second;
	uint16_t year;
	uint16_t mounth;
	uint16_t day;
} Date;

typedef struct FATContent {
	Directory* directory;
	File* file;

	uint32_t parent_cluster;
} Content;


Content* FSLIB_create_content();
Directory* FSLIB_create_directory();
File* FSLIB_create_file();

void FSLIB_unload_directories_system(Directory* directory);
void FSLIB_unload_files_system(File* file);
void FSLIB_unload_content_system(Content* content);

char* FSLIB_change_path(const char* currentPath, const char* content);

Date* FSLIB_get_date(short data, int type);

int cexists(const char* path);
void rmcontent(const char* path);
void chgcontent(const char* path, directory_entry_t* meta);

char* fread(const char* path);
char* fread_stop(const char* path, char* stop);
void fread_off(Content* content, int offset, uint8_t* buffer, int len);
void fread_off_stop(Content* content, int offset, uint8_t* buffer, int len, char* stop);

void fwrite(const char* path, const char* data);
void fwrite_off(Content* content, int offset, uint8_t* buffer, int len);

void mkfile(const char* path, const char* name);
int fexec(char* path, int args, char** argv);

Directory* opendir(const char* path);
Content* get_content(const char* path);
void mkdir(const char* path, const char* name);

#endif