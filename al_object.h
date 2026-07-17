#ifndef ALD_AL_OBJECT_H
#define ALD_AL_OBJECT_H
#include "al_string.h"

typedef enum  {
    okString,
    okInt,
    okFloat,
    okBool,
} Al_Object_Kind;
typedef struct {
    Al_Object_Kind kind;
    union {
        String* as_string;
        s64     as_int;
        float   as_float;
        bool    as_bool;
    };
} Al_Object;

extern const Al_Object FALSE;
extern const Al_Object TRUE;

void destroy_object(Al_Object*);

void print_object(const Al_Object*);

#endif //ALD_AL_OBJECT_H
