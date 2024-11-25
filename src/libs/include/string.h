#ifndef STRING_H_
#define STRING_H_


#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include "ctype.h"

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>


#define DOUBLE_STR_BUFFER_SIZE 64


const char* strchr(const char* str, char chr);
char* strrchr(const char *s, int c);
int strstr(const char* haystack, const char* needle);
char* strcpy(char* dst, const char* src);
unsigned strlen(const char* str);
int strcmp(const char* firstStr, const char* secondStr);
int strncmp(const char* str1, const char* str2, size_t n);
int strcasecmp(const char *s1, const char *s2);
char* strcat(char* dest, const char* src);

char* strtok(char* string, const char* delim);
char* strtok_r(char* s, const char* delim, char** last);
char place_char_in_text(char* text, char character, int x_position, int y_position);

void reverse(char* str, int len);
char* ftoa(double value);
double atof(const char *str);

char* backspace_string(char* str);
char* add_char2string(char* str, char character);
void add_string2string(char** str, char* string);

wchar_t* utf16_to_codepoint(wchar_t*  strin, int* codePointg);
char* codepoint_to_utf8(int codePoint, char* stringOutput);

int	atoi(char *str);
char* itoa(int n);

char* strncpy(char *dst, const char *src, int n);
char* strdup(const char *src);

void str2uppercase(char* str);
int chars_in_string(char* string, char letter);

void str2len(char* output, const char* input, int len);

#endif