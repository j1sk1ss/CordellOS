#include <hash.h>

unsigned long str2hash(const char* str) {
    unsigned long hashedValue = 0;
    for (int i = 0; str[i]; i++) hashedValue ^= (hashedValue << MAGIC) + (hashedValue >> 2) + str[i];
    for (int i = 0; i < MAGIC; i++) hashedValue ^= (hashedValue << MAGIC) + (hashedValue >> 2) + SALT[i];
    return hashedValue;
}

// 0 - equals
// != 0 - nequals
int compare_hash(char* first, char* second) {
    return strstr(first, second) == NULL;
}
