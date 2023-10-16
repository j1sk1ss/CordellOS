#include "../include/string.h"

#include <stdint.h>
#include <stddef.h>

const char* strchr(const char* str, char chr) {
    if (str == NULL)
        return NULL;

    while (*str) {
        if (*str == chr)
            return str;

        ++str;
    }

    return NULL;
}

char* strcat(char* dest, const char* src) {
    strcpy(dest + strlen(dest), src);
    return dest;
}

int strstr(const char* haystack, const char* needle) {
    if (*needle == '\0')    // If the needle is an empty string, return 0 (position 0).
        return 0;
    
    int position = 0;       // Initialize the position to 0.

    while (*haystack) {
        const char* hay_ptr     = haystack;
        const char* needle_ptr  = needle;

        // Compare characters in the haystack and needle.
        while (*hay_ptr == *needle_ptr && *needle_ptr) {
            hay_ptr++;
            needle_ptr++;
        }

        // If we reached the end of the needle, we found a match.
        if (*needle_ptr == '\0') 
            return position;

        // Move to the next character in the haystack.
        haystack++;
        position++;
    }

    return -1;  // Needle not found, return -1 to indicate that.
}

char* strcpy(char* dst, const char* src) {
    char* origDst = dst;

    if (dst == NULL)
        return NULL;

    if (src == NULL) {
        *dst = '\0';
        return dst;
    }

    while (*src) {
        *dst = *src;

        ++src;
        ++dst;
    }
    
    *dst = '\0';
    return origDst;
}

unsigned strlen(const char* str) {
    unsigned len = 0;
    while (*str) {
        ++len;
        ++str;
    }

    return len;
}

int strcmp(const char* firstStr, const char* secondStr) {
    if (firstStr == NULL && secondStr == NULL)
        return 0;

    if (firstStr == NULL || secondStr == NULL)
        return -10;

    while (*firstStr && *secondStr && *firstStr == *secondStr) {
        ++firstStr;
        ++secondStr;
    }

    return (*firstStr) - (*secondStr);
}

wchar_t* utf16_to_codepoint(wchar_t* string, int* codePoint) {
    int first = *string;
    ++string;

    if (first >= 0xd800 && first < 0xdc00) {
        int second = *string;
        ++string;

        *codePoint = ((first & 0x3ff) << 10) + (second & 0x3ff) + 0x10000;
    }

    *codePoint = first;

    return string;
}

char* codepoint_to_utf8(int codePoint, char* stringOutput) {
    if (codePoint <= 0x7F) 
        *stringOutput = (char)codePoint;
    else if (codePoint <= 0x7FF) {
        *stringOutput++ = 0xC0 | ((codePoint >> 6) & 0x1F);
        *stringOutput++ = 0xC0 | (codePoint & 0x3F);
    }
    else if (codePoint <= 0xFFFF) {
        *stringOutput++ = 0xE0 | ((codePoint >> 12) & 0xF);
        *stringOutput++ = 0x80 | ((codePoint >> 6) & 0x3F);
        *stringOutput++ = 0x80 | (codePoint & 0x3F);
    }
    else if (codePoint <= 0x1FFFFF) {
        *stringOutput++ = 0xF0 | ((codePoint >> 18) & 0x7);
        *stringOutput++ = 0x80 | ((codePoint >> 12) & 0x3F);
        *stringOutput++ = 0x80 | ((codePoint >> 6) & 0x3F);
        *stringOutput++ = 0x80 | (codePoint & 0x3F);
    }

    return stringOutput;
}