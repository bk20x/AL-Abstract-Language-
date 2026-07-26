#ifndef ALD_AL_STRING_H
#define ALD_AL_STRING_H
#include <stddef.h>
#include <stdio.h>
#include "al_cdefs.h"

typedef struct {
    size_t  len;
    size_t  cap;
    char*   chars;
} String;
/* Constructor Division */
String* str_of_cap(size_t);
String* str_of_cstr(const char*);
String* str_byteslice(const char*, size_t low, size_t high);
String* string_copy(const String*);
/* Destructor Division */
void str_free(String*);

/* Operations Division */
// All s64 return -1 on failure, otherwise they return the new length of `str`
s64     str_append_cstr(String* str, const char* cstr);
s64     str_append(String*, String*);
s64     str_appendf(String* str, const char* format, ...);
void    str_ensure_cap(String*, size_t needed_capacity);

/* Operations Division [Writing/Printing] */
void    str_writen(const String*, FILE*, size_t n);
void    str_write(const String*, FILE*);
void    str_print(const String*);
void    str_println(const String* str);

/* Operations Division [UNSAFE] */
s64     str_append_bytes_unsafe(String* str, const char* bytes, size_t n_bytes); // does NOT check the length of argument `bytes`; it's up to you to ensure `n_bytes` is correct






#endif //ALD_AL_STRING_H
