#ifndef DOOM_CORDELLOS_COMPAT_H_
#define DOOM_CORDELLOS_COMPAT_H_

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct FILE FILE;

extern FILE* stdin;
extern FILE* stdout;
extern FILE* stderr;

FILE* dg_fopen(const char* path, const char* mode);
int dg_fclose(FILE* stream);
size_t dg_fread(void* ptr, size_t size, size_t nmemb, FILE* stream);
size_t dg_fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream);
int dg_fseek(FILE* stream, long offset, int whence);
long dg_ftell(FILE* stream);
int dg_feof(FILE* stream);
int dg_ferror(FILE* stream);
int dg_fflush(FILE* stream);
int dg_fprintf(FILE* stream, const char* fmt, ...);
int dg_vfprintf(FILE* stream, const char* fmt, va_list args);
int dg_fscanf(FILE* stream, const char* fmt, ...);
char* dg_fgets(char* str, int n, FILE* stream);
int dg_remove(const char* path);
int dg_rename(const char* oldpath, const char* newpath);
int dg_mkdir(const char* path, int mode);

#define fopen dg_fopen
#define fclose dg_fclose
#define fread dg_fread
#define fwrite dg_fwrite
#define fseek dg_fseek
#define ftell dg_ftell
#define feof dg_feof
#define ferror dg_ferror
#define fflush dg_fflush
#define fprintf dg_fprintf
#define vfprintf dg_vfprintf
#define fscanf dg_fscanf
#define fgets dg_fgets
#define remove dg_remove
#define rename dg_rename
#define mkdir dg_mkdir

#endif
