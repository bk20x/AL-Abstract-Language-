#include "al_symtab.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "al_alloc.h"

Symbol_Table* create_symbol_table(const size_t cap, const Hash_Function hash_func) {
    Symbol_Table* result = new(Symbol_Table);
    if (!result) return nullptr;

    result->cap       = cap;
    result->size      = 0;
    result->entries   = calloc(cap, sizeof(Symtab_Entry));
    result->hash_func = hash_func;

    if (!result->entries) {
        dealloc(result);
        return nullptr;
    }

    return result;
}

static Symbol* set_table_entry(const Symbol_Table* tab, Symtab_Entry* entries,
                               const size_t cap, Symbol* key, void* value,
                               size_t* pointer_to_size)
{
    if (!key) return nullptr;

    const u64 hash = tab->hash_func(key->name, key->len); // only time we need to hash it is in here. see `st_gethash`
    size_t idx = hash & (cap - 1);

    while (entries[idx].key != nullptr) {
        if (entries[idx].key->len == key->len
            && memcmp(key->name, entries[idx].key->name, key->len) == 0) {
            entries[idx].key->hash = hash;
            entries[idx].value = value;
            return entries[idx].key;
        }
        idx++;
        if (idx >= cap) idx = 0;
    }

    if (pointer_to_size != nullptr) {
        (*pointer_to_size)++;
    }

    key->hash = hash;
    entries[idx].key = key;
    entries[idx].value = value;
    return key;
}

static bool st_expand(Symbol_Table* ht) {
    const size_t new_cap = ht->cap * 2;
    if (new_cap < ht->cap) return false;

    Symtab_Entry* new_entries = calloc(new_cap, sizeof(Symtab_Entry));
    if (new_entries == nullptr) return false;

    const size_t old_size = ht->size;
    ht->size = 0;

    for (size_t i = 0; i < ht->cap; ++i) {
        const Symtab_Entry entry = ht->entries[i];
        if (entry.key) {
            if (!set_table_entry(ht, new_entries, new_cap, entry.key, entry.value, &ht->size)) {
                dealloc(new_entries);
                ht->size = old_size;
                return false;
            }
        }
    }

    dealloc(ht->entries);
    ht->cap = new_cap;
    ht->entries = new_entries;
    return true;
}

Symbol* st_put(Symbol_Table* st, Symbol* key, void* value) {
    assert(st != nullptr && key != nullptr);
    if (st->size >= st->cap / 2) {
        if (!st_expand(st)) return nullptr;
    }
    return set_table_entry(st, st->entries, st->cap, key, value, &st->size);
}

Symbol* st_get(const Symbol_Table* st, Symbol* key) {
    if (!st || !key) return nullptr;

    const u64 hash = key->hash;
    size_t idx = hash & (st->cap - 1);

    while (st->entries[idx].key != nullptr) {
        const Symbol* const stored = st->entries[idx].key;
        if (stored->len == key->len &&
            memcmp(key->name, stored->name, key->len) == 0) {
            return st->entries[idx].value;
        }
        idx++;
        if (idx >= st->cap) idx = 0;
    }

    return nullptr;
}

Symbol* st_gethash(const Symbol_Table* st, const u64 hash) {
    if (!st) return nullptr;
    size_t idx = hash & st->cap - 1;
    while (st->entries[idx].key != nullptr) {
        const Symbol* const stored = st->entries[idx].key;
        if (stored->hash == hash) {
            return st->entries[idx].value;
        }
        idx++;
        if (idx >= st->cap) idx = 0;
    }
    return nullptr;
}

void st_destroy(Symbol_Table* restrict st) {
    if (!st) return;

    for (size_t i = 0; i < st->cap; i++) {
        void* key = st->entries[i].key;

        if (key != nullptr) {
            dealloc(key);
            for (size_t j = i; j < st->cap; j++) {
                if (st->entries[j].key == key) {
                    st->entries[j].key   = nullptr;
                    st->entries[j].value = nullptr;
                }
            }
        }
    }
    dealloc(st->entries);
    dealloc(st);
}

bool st_contains(const Symbol_Table* st, Symbol* key) {
    return st_get(st, key) ? true : false;
}

bool st_contains_hash(const Symbol_Table* st, const u64 hash) {
    return st_gethash(st, hash) ? true :false;
}
