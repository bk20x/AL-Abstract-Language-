#include "al_objtab.h"

#include <assert.h>

#include "al_alloc.h"

Object_Table* create_object_table(const size_t cap) {
    Object_Table* result = new(Object_Table);
    *result = (Object_Table) {
        .cap     = cap,
        .size    = 0,
        .entries = calloc(cap, sizeof(Objtab_Entry))
    };
    assert(result->entries);
    return result;
}


static Al_Object* set_table_entry(Objtab_Entry* entries,
                                  const size_t cap,
                                  Symbol* key, void* value,
                                  size_t* pointer_to_size)
{
    const u64 hash = key->hash;
    size_t idx = hash & (cap - 1);

    while (entries[idx].key != nullptr) {
        if (entries[idx].key->hash == key->hash) {
            entries[idx].value = value;
            return entries[idx].value;
        }
        idx++;
        if (idx >= cap) idx = 0;
    }

    if (pointer_to_size != nullptr) {
        (*pointer_to_size)++;
    }

    entries[idx].key = key;
    entries[idx].value = value;
    return value;
}

static bool ot_expand(Object_Table* ot) {
    const size_t new_cap = ot->cap * 2;
    if (new_cap < ot->cap) return false;

    Objtab_Entry* new_entries = calloc(new_cap, sizeof(Objtab_Entry));
    if (new_entries == nullptr) return false;

    const size_t old_size = ot->size;
    ot->size = 0;

    for (size_t i = 0; i < ot->cap; ++i) {
        const Objtab_Entry entry = ot->entries[i];
        if (entry.key) {
            if (!set_table_entry(new_entries, new_cap, entry.key, entry.value, &ot->size)) {
                dealloc(new_entries);
                ot->size = old_size;
                return false;
            }
        }
    }

    dealloc(ot->entries);
    ot->cap = new_cap;
    ot->entries = new_entries;
    return true;
}


Al_Object* ot_put(Object_Table* ot, Symbol* key, Al_Object* obj) {
    assert(ot && key);
    if (ot->size >= ot->cap / 2) {
        if (!ot_expand(ot)) return nullptr;
    }
    return set_table_entry(ot->entries, ot->cap, key, obj, &ot->size);
}

Al_Object* ot_get(const Object_Table* restrict ot, const Symbol* restrict key) {
    assert(ot && key);

    const u64 hash = key->hash;
    size_t idx = hash & (ot->cap - 1);

    while (ot->entries[idx].key != nullptr) {
        const Symbol* const stored = ot->entries[idx].key;
        if (stored->hash == key->hash) {
            return ot->entries[idx].value;
        }
        idx++;
        if (idx >= ot->cap) idx = 0;
    }
    return nullptr;
}

void ot_destroy(Object_Table* restrict ot) {
    assert(ot);
    for (size_t i = 0; i < ot->cap; i++) {
        // the reader holds the symbol keys so we don't gotta free them here
        if (ot->entries[i].value) destroy_object(ot->entries[i].value);
    }
    dealloc(ot->entries);
    dealloc(ot);
}