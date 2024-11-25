#ifndef RAND_H_
#define RAND_H_

#include "stdlib.h"

#define	RAND_MAX	2147483647

int rand_r(int seed);
int rand(unsigned long* ctx);
int srand_r();

#endif