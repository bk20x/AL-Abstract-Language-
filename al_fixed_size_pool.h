#ifndef ALD_FIXED_SIZE_POOL_H
#define ALD_FIXED_SIZE_POOL_H
#include <stddef.h>
#include "al_alloc.h"

typedef struct {
    const size_t   element_size;
    const size_t   max_size; // element_size * max_elements
    size_t         size;
    unsigned char* storage;
} Fixed_Size_Pool;

Fixed_Size_Pool create_fixed_size_pool(size_t element_size, size_t max_elements);
void*           alloc_from_fixed_size_pool(Fixed_Size_Pool*);

int             empty_fixed_size_pool(const Fixed_Size_Pool*);
int             empty_fixed_size_pool_with_dtor(const Fixed_Size_Pool*, Free_Function);
#endif //ALD_FIXED_SIZE_POOL_H
