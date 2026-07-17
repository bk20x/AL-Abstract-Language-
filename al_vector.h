#ifndef ALD_AL_VECTOR_H
#define ALD_AL_VECTOR_H
#include <stddef.h>
#include "al_cdefs.h"


typedef struct {
    size_t cap;
    size_t len;
    void** data;
} Vector;
/* Constructor Division */
Vector* vec_new();
Vector* vec_new_of_cap(size_t);
/* Operations Division */
#define AL_VEC_OP_FAILURE -1;
s64     vec_append(Vector*, void*); // returns -1 on failure otherwise it returns the new length of the vector
/* [Operations] Unsafe Division */
void    vec_append_unsafe(Vector*, void*);
#endif //ALD_AL_VECTOR_H
