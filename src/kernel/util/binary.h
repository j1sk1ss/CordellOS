#ifndef BINARY_H_
#define BINARY_H_
#define FLAG_SET(x, flag) x |= (flag)
#define FLAG_UNSET(x, flag) x &= ~(flag)
#define PUSH(stack, type, item) stack -= sizeof(type); *((type *) stack) = item
#endif