#include <assert.h>
#include "al_symbol.h"
#include <string.h>
#include "al_alloc.h"
#include "al_assert.h"


// https://en.wikipedia.org/wiki/Fowler–Noll–Vo_hash_function
u64 symhash(const char* key, const size_t len) {
    #define FNV_OFFSET 14695981039346656037UL
    #define FNV_PRIME  1099511628211UL
    u64 hash = FNV_OFFSET;
    const char* p = key;
    for (size_t i = 0; i < len; i++) {
        hash ^= (u64)(char)*p;
        hash *= FNV_PRIME;
        p++;
    }
    return hash;
}

Symbol* sym_new(const char* name, const size_t len) {
    Symbol* result = alloc(sizeof(Symbol) + sizeof(char) * len);
    if (!result) die("cant allocate symbol for some reason");
    result->len = len; 
    memcpy(result->name, name, len);
    return result;
}