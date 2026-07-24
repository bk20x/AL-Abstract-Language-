#ifndef ALD_AL_VECTOR_H
#define ALD_AL_VECTOR_H
#include <stddef.h>
#include "al_cdefs.h"
#include "al_alloc.h"

typedef struct {
    size_t cap;
    size_t len;
    void** data;
} Vector;
/* Constructor Division */
Vector* vec_new();
Vector* vec_new_of_cap(size_t);
Vector* vec_new_of_cap0(size_t); // all initialized to 0
/* Destructor Division */
void vec_destroy(Vector*);
void vec_destroy_with_dtor(Vector*, Free_Function);
void vec_destroy_if_elements_are_pooled(Vector*);
/* Operations Division */
#define AL_VEC_OP_FAILURE -1;
s64     vec_append(Vector*, void*); // returns -1 on failure otherwise it returns the new length of the vector
/* [Operations] Unsafe Division */
void    vec_append_unsafe(Vector*, void*);


#define vforeach(T, vec, name) \
    for (size_t _i_ = 0, _keep_ = 1; _keep_ && _i_ < (vec)->len; _keep_ = !_keep_, _i_++) \
        for (T* name = (vec)->data[_i_]; _keep_; _keep_ = !_keep_)

#endif //ALD_AL_VECTOR_H
