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
// for mem i already own
void str_init_of_cap(String*,  size_t cap);
void str_init_of_cstr(String*, const char* cstr);

/* Operations Division */
/* All s64 return -1 on failure, otherwise they return the new length of `str`
 * `str_append_bytes` does NOT check the length of argument `bytes` and is unsafe. it's up to you to ensure `n_bytes` is correct */
s64  str_append_cstr(String* str, const char* cstr);
s64  str_append_bytes(String* str, const char* bytes, size_t n_bytes);
s64  str_appendf(String* str, const char* format, ...);
void str_ensure_cap(String*, size_t needed_capacity);
/* [Writing] Operations Division */
void    str_writen(const String*, FILE*, size_t n);
void    str_write(const String*, FILE*);
void    str_print(const String*);
void    str_println(const String* str);
void    str_free(String*);

// Extra stuff
String* str_byteslice(const char*, size_t low, size_t high);

#endif //ALD_AL_STRING_H
