#include "al_vector.h"
#include <assert.h>
#include "al_alloc.h"

#define AL_VEC_INITIAL_CAP 16
/* Constructor Division */
Vector* vec_new() {
    Vector* result = new(Vector);
    assert(result);
    *result = (Vector) {
        .cap  = 0,
        .len  = 0,
        .data = nullptr
    };
    return result;
}


Vector* vec_new_of_cap(const size_t cap) {
    Vector* result = new(Vector);
    assert(result);
    *result = (Vector){
        .cap  = cap,
        .len  = 0,
        .data = alloc(sizeof(void*) * cap)
    };
    assert(result->data);
    return result;
}

Vector* vec_new_of_cap0(const size_t cap) {
    Vector* result = new(Vector);
    assert(result);
    *result = (Vector){
        .cap  = cap,
        .len  = 0,
        .data = alloc0(cap, sizeof(void*))
    };
    assert(result->data);
    return result;
}
/* Destructor Division */
void vec_destroy(Vector* vec) {
    for (size_t i = 0; i < vec->len; i++) {
        dealloc(vec->data[i]);
    }
    dealloc(vec->data);
    dealloc(vec);
    vec = nullptr;
}

void vec_destroy_with_dtor(Vector* vec, const Free_Function dtor) {
    for (size_t i = 0; i < vec->len; i++) {
        dtor(vec->data[i]);
    }
    dealloc(vec->data);
    dealloc(vec);
    vec = nullptr;
}

void vec_destroy_if_elements_are_pooled(Vector* vec) {
    dealloc(vec->data);
    dealloc(vec);
    vec = nullptr;
}
/* Operations Division */
s64 vec_append(Vector* restrict vec, void* element) {
    if (!vec || !element) return AL_VEC_OP_FAILURE;

    if (vec->len >= vec->cap) {
        const size_t new_cap = vec->cap == 0 ? AL_VEC_INITIAL_CAP : vec->cap * 2;
        void** new_data = realloc(vec->data, new_cap * sizeof(void*));
        if (!new_data) {
            return AL_VEC_OP_FAILURE;
        }

        vec->data = new_data;
        vec->cap  = new_cap;
    }

    vec->data[vec->len] = element;
    vec->len++;
    return (s64)vec->len;
}

/* [Operations] Unsafe Division */
void vec_append_unsafe(Vector* restrict vec, void* element) {
    if (vec->len >= vec->cap) {
        const size_t new_cap = vec->cap == 0 ? AL_VEC_INITIAL_CAP : vec->cap * 2;
        void** new_data = realloc(vec->data, new_cap * sizeof(void*));

        vec->data = new_data;
        vec->cap  = new_cap;
    }
    vec->data[vec->len] = element;
    vec->len++;
}