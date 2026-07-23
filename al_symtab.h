#ifndef DXM_TABLES_H
#define DXM_TABLES_H
#include <stddef.h>
#include <stdint.h>

#include "al_symbol.h"

#define Get_As(T, Ht, Key) (T)st_get((Ht), (Key))
typedef uint64_t(*Hash_Function)(const char*, size_t len);

typedef struct {
    Symbol* key;
    Symbol* value;
} Symtab_Entry;

typedef struct {
    Symtab_Entry*   entries;
    size_t cap;
    size_t size;
} Symbol_Table;

Symbol_Table* create_symbol_table(size_t cap);
Symbol* st_put(Symbol_Table* st, Symbol* key, void* value);
Symbol* st_get(const Symbol_Table* st, Symbol* key);
Symbol* st_gethash(const Symbol_Table* st, u64 hash);
void    st_destroy(Symbol_Table* st);
bool    st_contains(const Symbol_Table* st, Symbol* key);
bool    st_contains_hash(const Symbol_Table* st, u64);
#endif
