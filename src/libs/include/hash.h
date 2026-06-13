#ifndef HASH_H_
#define HASH_H_

#include "string.h"

#define SALT    "cordellOS_SHA"
#define MAGIC   5

unsigned long str2hash(const char *str);
int compare_hash(char* first, char* second);

#endif