#ifndef ALD_AL_CHARUTILS_H
#define ALD_AL_CHARUTILS_H
#include <stdbool.h>

#define CHAR_IN_RANGE(it, a, b) (it >= a && it <= b)
#define IS_ASCII_DIGIT(c) (c >= 48 && c <= 57)
#define IS_IDENT_START(c) (CHAR_IN_RANGE(c, 'a', 'z') || CHAR_IN_RANGE(c, 'A', 'Z') || c == '_')

#endif //ALD_AL_CHARUTILS_H
