#ifndef FSLIB_H_
#define FSLIB_H_

#include "stdio.h"
#include "stdlib.h"
#include "ctype.h"
#include "execute.h"


#define STAT_FILE	0x00
#define STAT_DIR	0x01


typedef struct FATDate {
	uint16_t hour;
	uint16_t minute;
	uint16_t second;
	uint16_t year;
	uint16_t mounth;
	uint16_t day;
} Date;

typedef struct {
	uint8_t full_name[11];
	char file_name[8];
	char file_extension[4];
	int type;
	int size;
	uint16_t creation_time;
	uint16_t creation_date;
	uint16_t last_accessed;
	uint16_t last_modification_time;
	uint16_t last_modification_date;
} CInfo_t;


char* FSLIB_change_path(const char* currentPath, const char* content);
Date* FSLIB_get_date(short data, int type);

int cexists(const char* path);
void rmcontent(const char* path);
void chgcontent(const char* path, const char* meta);

void fread(int ci, int offset, uint8_t* buffer, int len);
void fread_stop(int ci, int offset, uint8_t* buffer, int len, char* stop);
void fwrite(int ci, int offset, uint8_t* buffer, int len);
int fexec(char* path, int args, char** argv);

void mkfile(const char* path, const char* name);
void mkdir(const char* path, const char* name);

int lsdir(const char* path, char* name, int step);
int fstat(int ci, CInfo_t* info);
int fopen(const char* path);
int fclose(int ci);

#endif