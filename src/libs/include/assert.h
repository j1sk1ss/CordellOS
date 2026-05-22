#ifndef ASSERT_H_
#define ASSERT_H_

#include <stdbool.h>

#include "stdlib.h"
#include "stdio.h"

#define assert(statement)   ((statement) ? ((void)0) : assert_failed(__FILE__, __LINE__))

void assert_failed(char* file, int line);
void panic(char* message);

#endif