#include "../include/rand.h"


int rand_r(int seed) {
	seed = seed * 1103515245 + 12345;
	return ((srand_r(123) * seed) & 2147483647);
}

int srand_r(int fseed) {
	DateInfo_t info;
    get_datetime(&info);

	int seed = info.second * info.hour * 11015245 + 12345;
	return ((fseed * seed) & 2147483647);
}

int rand(unsigned long *ctx) {
	long hi, lo, x;

	hi = *ctx / 127773;
	lo = *ctx % 127773;
	x = 16807 * lo - 2836 * hi;

	if (x < 0)
		x += 0x7fffffff;

	*ctx = x;
    
	return (x - 1);
}