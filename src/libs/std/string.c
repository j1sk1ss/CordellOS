#include "../include/string.h"

//   ____ _____ ____  ___ _   _  ____ 
//  / ___|_   _|  _ \|_ _| \ | |/ ___|
//  \___ \ | | | |_) || ||  \| | |  _ 
//   ___) || | |  _ < | || |\  | |_| |
//  |____/ |_| |_| \_\___|_| \_|\____|


const char* strchr(const char* str, char chr) {
    if (str == NULL) return NULL;
    while (*str) {
        if (*str == chr)
            return str;

        ++str;
    }

    return NULL;
}

char* strrchr(const char *s, int c) {
    int i = strlen(s);
    while (i >= 0 && s[i] != c) 
        i--;

    if (i >= 0) return ((char *)&s[i]);
    return NULL;
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
    if (strlen(src) <= 0) return NULL;

	int	i = 0;
	while (src[i]) {
		dst[i] = src[i];
		i++;
	}

	dst[i] = '\0';
	return (dst);
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
    if (firstStr == NULL || secondStr == NULL) return -1;
    while (*firstStr && *secondStr && *firstStr == *secondStr) {
        ++firstStr;
        ++secondStr;
    }

    return (unsigned char)(*firstStr) - (unsigned char)(*secondStr);
}

int strcasecmp(const char *s1, const char *s2) {
    unsigned char us1;
    unsigned char us2;
    size_t index;

    us1 = tolower(*s1);
    us2 = tolower(*s2);

    index = 0;
    while (us1 != '\0' && us1 == us2) {
        index++;
        us1 = tolower(s1[index]);
        us2 = tolower(s2[index]);
    }

    return us1 - us2;
}

int strncmp(const char* str1, const char* str2, size_t n) {
    for (size_t i = 0; i < n; ++i) 
        if (str1[i] != str2[i] || str1[i] == '\0' || str2[i] == '\0') 
            return (unsigned char)str1[i] - (unsigned char)str2[i];
        
    return 0;
}

void reverse(char* str, int len) {    
    int start = 0;
    int end = len - 1;
    char temp;

    while (start < end) {
        temp        = str[start];
        str[start]  = str[end];
        str[end]    = temp;

        start++;
        end--;
    }
}

double atof(const char *str) {
    double result       = 0.0;
    int sign            = 1;
    double fraction     = 0.0;
    bool is_decimal     = false;
    int decimal_places  = 0;

    // Skip whitespace characters
    while (isspace(*str)) 
        str++;

    // Determine sign
    if (*str == '-') {
        sign = -1;
        str++;
    } else if (*str == '+') 
        str++;
    
    // Process the string
    while (*str != '\0') {
        if (*str == '.' && !is_decimal) {
            is_decimal = true;
            str++;
            continue;
        }

        if (isdigit(*str)) {
            if (is_decimal) {
                fraction = fraction * 10.0 + (*str - '0');
                decimal_places++;
            } else 
                result = result * 10.0 + (*str - '0');
        } else 
            break;

        str++;
    }

    // Combine integer and fractional parts
    result += fraction / pow(10.0, decimal_places);
    result *= sign;

    return result;
}

char* ftoa(double value) {
    static char buffer[DOUBLE_STR_BUFFER_SIZE];

    int int_part            = (int)value;
    double fractional_part  = value - int_part;
    int i                   = 0;
    bool is_negative        = false;

    if (value < 0) {
        is_negative     = true;
        int_part        = -int_part;
        fractional_part = -fractional_part;
    }

    do {
        buffer[i++] = int_part % 10 + '0';
        int_part /= 10;
    } while (int_part != 0);

    if (is_negative) 
        buffer[i++] = '-';

    reverse(buffer, i);

    buffer[i++] = '.';

    int num_decimal_digits = 6;
    for (int j = 0; j < num_decimal_digits; j++) {
        fractional_part *= 10;
        int digit = (int)fractional_part;
        buffer[i++] = digit + '0';
        fractional_part -= digit;
    }

    buffer[i] = '\0';
    return buffer;
}

char* strcat(char* dest, const char* src) {
    strcpy(dest + strlen(dest), src);
    return dest;
}

void* __rawmemchr (const void* s, int c_in) {
  const unsigned char *char_ptr;
  const unsigned long int *longword_ptr;
  unsigned long int longword, magic_bits, charmask;
  unsigned char c;

  c = (unsigned char) c_in;

  /* Handle the first few characters by reading one character at a time.
     Do this until CHAR_PTR is aligned on a longword boundary.  */
  for (char_ptr = (const unsigned char *) s;
       ((unsigned long int) char_ptr & (sizeof (longword) - 1)) != 0;
       ++char_ptr)
    if (*char_ptr == c)
      return (void*) char_ptr;

  /* All these elucidatory comments refer to 4-byte longwords,
     but the theory applies equally well to 8-byte longwords.  */

  longword_ptr = (unsigned long int *) char_ptr;

#if LONG_MAX <= LONG_MAX_32_BITS
  magic_bits = 0x7efefeff;
#else
  magic_bits = ((unsigned long int) 0x7efefefe << 32) | 0xfefefeff;
#endif

  /* Set up a longword, each of whose bytes is C.  */
  charmask = c | (c << 8);
  charmask |= charmask << 16;
#if LONG_MAX > LONG_MAX_32_BITS
  charmask |= charmask << 32;
#endif

    while (1) {
        longword = *longword_ptr++ ^ charmask;

        /* Add MAGIC_BITS to LONGWORD.  */
        if ((((longword + magic_bits) ^ ~longword) & ~magic_bits) != 0) {
                const unsigned char *cp = (const unsigned char *) (longword_ptr - 1);

                if (cp[0] == c) return (void*)cp;
                if (cp[1] == c) return (void*)&cp[1];
                if (cp[2] == c) return (void*)&cp[2];
                if (cp[3] == c) return (void*)&cp[3];

                #if LONG_MAX > 2147483647
                    if (cp[4] == c) return (void*) &cp[4];
                    if (cp[5] == c) return (void*) &cp[5];
                    if (cp[6] == c) return (void*) &cp[6];
                    if (cp[7] == c) return (void*) &cp[7];
                #endif
            }
    }
}

char* strpbrk (const char* s, const char* accept) {
    while (*s != '\0') {
        const char *a = accept;
        while (*a != '\0')
	        if (*a++ == *s)
	            return (char *) s;

            ++s;
    }

  return NULL;
}

size_t strspn(const char* s, const char* accept) {
    const char *p;
    const char *a;
    size_t count = 0;

    for (p = s; *p != '\0'; ++p) {
        for (a = accept; *a != '\0'; ++a)
            if (*p == *a)
                break;

        if (*a == '\0') return count;
        else ++count;
    }

    return count;
}

static char* olds;
char* strtok (char* string, const char* delim){
    char* token;
    if (string == NULL) string = olds;

    string += strspn(string, delim);
    if (*string == '\0') {
        olds = string;
        return NULL;
    }

    token = string;
    string = strpbrk(token, delim);
    if (string == NULL) olds = __rawmemchr(token, '\0');
    else {
        *string = '\0';
        olds = string + 1;
    }

    return token;
}

char* strtok_r(char* s, const char* delim, char** last) {
	char* spanp;
	int c, sc;
	char* tok;

	if (s == NULL && (s = *last) == NULL) return (NULL);
cont:
	c = *s++;
	for (spanp = (char *)delim; (sc = *spanp++) != 0;) 
        if (c == sc) goto cont;

	if (c == 0) {
	    *last = NULL;
		return (NULL);
	}

	tok = s - 1;
	for (;;) {
		c = *s++;
		spanp = (char *)delim;
		do {
			if ((sc = *spanp++) == c) {
				if (c == 0) s = NULL;
				else s[-1] = 0;

				*last = s;

				return (tok);
			}

		} while (sc != 0);
	}
}

char* add_char2string(char* str, char character) {
    int len = strlen(str);
    char* buffer = malloc(len + 2);
    if (buffer == NULL || str == NULL) return str;

    strcpy(buffer, str);

    buffer[len] = character;
    buffer[len + 1] = '\0';

    free(str);
    return buffer;
}

char* backspace_string(char* str) {
    int len = strlen(str) - 1;
    if (len < 0) return str;

    str[len] = '\0';
    return str;  
}

char place_char_in_text(char* text, char character, int x_position, int y_position) {
    int current_y_position = 0;
    int current_x_position = 0;
    int pos = -1;

    while (current_y_position <= y_position) {
        if (current_x_position == x_position && current_y_position == y_position) break;
        if (text[++pos] == '\n') {
            current_x_position = 0;
            current_y_position++;
        } else current_x_position++;
    }

    char replaced_character = text[pos];
    text[pos] = character;

    return replaced_character;
}

void add_string2string(char** str, char* string) {
    if (str == NULL || string == NULL) 
        return;

    size_t new_size = strlen(*str) + strlen(string) + 1;

    char* buffer = (char*)malloc(new_size);
    if (buffer == NULL) 
        return;

    strcpy(buffer, *str);
    strcat(buffer, string);

    free(*str);
    free(string);

    *str = buffer;
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

int	atoi(char *str) {
	int neg = 1;
	int num = 0;
	int i   = 0;

	while (str[i] <= ' ')
		i++;

	if (str[i] == '-' || str[i] == '+') {
		if (str[i] == '-') 
			neg *= -1;

		i++;
	}

	while (str[i] >= '0' && str[i] <= '9') {
		num = num * 10 + (str[i] - 48);
		i++;
	}
    
	return (num * neg);
}

char* itoa(int n) {
    char* str = (char*)malloc(10);
    int i, sign;

    if ((sign = n) < 0)        /* record sign */
        n = -n;                /* make n positive */
    i = 0;

    do {                         /* generate digits in reverse order */
        str[i++] = n % 10 + '0'; /* get next digit */
    } while ((n /= 10) > 0);     /* delete it */

    if (sign < 0)
        str[i++] = '-';

    reverse(str, strlen(str));
    str[i] = '\0';

    return str;
}

char* strncpy(char* dst, const char* src, int n) {
	int	i = 0;
	while (i < n && src[i]) {
		dst[i] = src[i];
		i++;
	}

	while (i < n) {
		dst[i] = '\0';
		i++;
	}

	return (dst);
}

char* strdup(const char* src) {
	char* new;
	int	i;
	int	size;

	size = 0;
	while (src[size])
		size++;

	if (!(new = malloc(sizeof(char) * (size + 1))))
		return NULL;

	i = 0;
	while (src[i]) {
		new[i] = src[i];
		i++;
	}

	new[i] = '\0';

	return new;
}

void str2uppercase(char* str) {
    if (str == NULL) 
        return;

    for (int i = 0; str[i] != '\0'; i++) {
        str[i] = toupper((unsigned char)str[i]);
    }
}

int chars_in_string(char* string, char letter) {
    int count = 0;
    for (count = 0; string[count]; string[count]==letter ? count++ : *string++);
    return count;
}

// Fit or pad string to len
// output - buffer for new string
// input - source string
// len - final string len
void str2len(char* output, const char* input, int len) {
    int input_len = strlen(input);
    strncpy(output, input, min(input_len, len));

    if (len > input_len)
        for (int i = input_len; i < len; ++i) 
            output[i] = ' '; 

    output[len] = '\0';
}