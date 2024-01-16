#include "../include/rand.h"
#include "../include/syscall.h"


int rand_r(int seed) {
	seed = seed * 1103515245 + 12345;
	return ((srand_r(123) * seed) & 2147483647);
}

int srand_r(int fseed) {
	short date_time[7];
    SYS_get_datetime(&date_time);

	int seed = date_time[0] * date_time[2] * 11015245 + 12345;
	return ((fseed * seed) & 2147483647);
}

static int rand(unsigned long *ctx) {
	long hi, lo, x;

	hi = *ctx / 127773;
	lo = *ctx % 127773;
	x = 16807 * lo - 2836 * hi;

	if (x < 0)
		x += 0x7fffffff;

	*ctx = x;
    
	return (x - 1);
}