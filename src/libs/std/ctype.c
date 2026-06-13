#include <ctype.h>

int isdigit(int c) {
    return (c >= '0' && c <= '9');
}

int isspace(int c) {
    return (c == ' ' || c == '\n' || c == '\r' || c == '\t' || c == '\v' || c == '\b');
}

int islower(int c) {
    return c >= 'a' && c <= 'z';
}

int tolower(int c) {
	if (c >= 'A' && c <= 'Z') return c - 'A' + 'a';
	return c;
}

int toupper(int c) {
    if (islower(c)) return c - 'a' + 'A';
    else return c;
}
