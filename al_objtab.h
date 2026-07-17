#ifndef ALD_AL_OBJTAB_H
#define ALD_AL_OBJTAB_H
#include "al_object.h"
#include "al_symbol.h"

typedef struct {
    Symbol*    key;
    Al_Object* value;
} Objtab_Entry;

typedef struct Object_Table Object_Table;
struct Object_Table {
    Objtab_Entry* entries;
    size_t        cap;
    size_t        size;
};

Object_Table* create_object_table(size_t);
Al_Object*    ot_put(Object_Table*, Symbol*, Al_Object*);
Al_Object*    ot_get(const Object_Table*, const Symbol*);

void          ot_destroy(Object_Table*);
#endif //ALD_AL_OBJTAB_H
