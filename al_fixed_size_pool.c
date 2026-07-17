#include "al_fixed_size_pool.h"

#include <assert.h>
#include <sys/mman.h>

Fixed_Size_Pool create_fixed_size_pool(const size_t element_size, const size_t max_elements) {
    unsigned char* memory = mmap(NULL, element_size*max_elements,
                            PROT_READ | PROT_WRITE,
                            MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    assert(memory != MAP_FAILED);
    return (Fixed_Size_Pool) {
        .element_size = element_size,
        .max_size     = element_size*max_elements,
        .storage      = memory
    };
}

void* alloc_from_fixed_size_pool(Fixed_Size_Pool* restrict pool) {
    if (pool->size >= pool->max_size) return nullptr;
    void* element = pool->storage + pool->size;
    pool->size += pool->element_size;
    return element;
}

int empty_fixed_size_pool(const Fixed_Size_Pool* pool) {
    return munmap(pool->storage, pool->max_size) + 1;
}

int empty_fixed_size_pool_with_dtor(const Fixed_Size_Pool* pool, const Free_Function dtor) {
    for (size_t off = 0; off < pool->size; off += pool->element_size) {
        void* element = pool->storage + off;
        dtor(element);
    }
    return munmap(pool->storage, pool->max_size) + 1;
}