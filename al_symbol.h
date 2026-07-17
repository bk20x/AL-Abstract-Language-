#ifndef ALD_AL_SYMBOL_H
#define ALD_AL_SYMBOL_H
#include <stddef.h>
#include "al_cdefs.h"

typedef struct Symbol Symbol;
struct Symbol {
    u64    hash;
    size_t len;
    char   name[];
};

u64     symhash(const char*, size_t len);
Symbol* sym_new(const char*, size_t len);
#endif //ALD_AL_SYMBOL_H
