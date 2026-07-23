#include "al_elastic_fixed_size_pool.h"

#include <assert.h>
#include <sys/mman.h>
#define standard_assert(cond) assert(cond && "Buy more RAM")
#include <stddef.h>
#include <string.h>

#include "al_cdefs.h"

static Mem_Chunk* init_chunk(const Elastic_Fixed_Size_Pool* pool) {
    const size_t storage_bytes_of_chunk = pool->elements_per_chunk * pool->element_size;
    const size_t size_bytes_of_chunk    = sizeof(Mem_Chunk) + storage_bytes_of_chunk;
    void* block = mmap(
        NULL,
        size_bytes_of_chunk,
        PROT_READ | PROT_WRITE,
        MAP_ANONYMOUS | MAP_PRIVATE,
        -1,
        0
    );
    if (block == MAP_FAILED) return nullptr;
    Mem_Chunk* result = block;
    result->next = nullptr;
    result->occupied_slots = 0;
    return result;
}

Elastic_Fixed_Size_Pool create_elastic_fixed_size_pool(const size_t element_size,
                                                       const size_t elements_per_chunk,
                                                       const int    initial_chunks) {
    Elastic_Fixed_Size_Pool result = {
        .top                = nullptr,
        .size               = 0,
        .element_size       = element_size,
        .elements_per_chunk = elements_per_chunk,
        .chunk_size         = element_size * elements_per_chunk
    };
    for (size_t i = 0; i < initial_chunks; i++) {
        Mem_Chunk* new_chunk = init_chunk(&result);
        standard_assert(new_chunk);

        new_chunk->next = result.top;
        result.top      = new_chunk;
        result.size += elements_per_chunk;
    }

    return result;
}

void* alloc_from_elastic_fixed_size_pool(Elastic_Fixed_Size_Pool* restrict pool) {
    Mem_Chunk* this_chunk = pool->top;

    /* find first chunk with slots */
    while (this_chunk != nullptr && this_chunk->occupied_slots >= pool->elements_per_chunk) {
        this_chunk = this_chunk->next;
    }

    if (!this_chunk) {
        this_chunk = init_chunk(pool);
        standard_assert(this_chunk);
        this_chunk->occupied_slots = 0;

        // push
        this_chunk->next = pool->top;
        pool->top = this_chunk;
        pool->size += pool->elements_per_chunk;
    }

    const size_t offset = this_chunk->occupied_slots * pool->element_size;
    void* new_element   = this_chunk->storage + offset;
    this_chunk->occupied_slots++;

    return new_element;
}


void reset_elastic_fixed_size_pool(const Elastic_Fixed_Size_Pool* restrict const pool) {
    Mem_Chunk* current = pool->top;
    while (current != nullptr) {
        current->occupied_slots = 0;
        memset(current->storage, 0, pool->chunk_size);
        current = current->next;
    }
}

void reset_elastic_fixed_size_pool_with_dtor(const Elastic_Fixed_Size_Pool* restrict const pool, EFSPDestructor const destroy) {
    Mem_Chunk* current = pool->top;
    while (current != nullptr) {
        for (size_t i = 0; i < current->occupied_slots; i++) {
            const size_t offset = i * pool->element_size;
            void* element = current->storage + offset;
            destroy(element);
        }
        current->occupied_slots = 0;
        memset(current->storage, 0, pool->chunk_size);
        current = current->next;
    }
}

void destroy_elastic_fixed_size_pool(Elastic_Fixed_Size_Pool* pool) {
    Mem_Chunk* current = pool->top;
    const size_t total_bytes = sizeof(Mem_Chunk) + pool->chunk_size;

    while (current != nullptr) {
        Mem_Chunk* next_chunk = current->next;
        munmap(current, total_bytes);
        current = next_chunk;
    }
    pool->top  = nullptr;
    pool->size = 0;
}

void destroy_elastic_fixed_size_pool_with_dtor(Elastic_Fixed_Size_Pool* pool, const EFSPDestructor destroy) {
    if (!pool) return;

    Mem_Chunk* current = pool->top;
    const size_t total_bytes = sizeof(Mem_Chunk) + pool->chunk_size;

    while (current != nullptr) {
        if (destroy != nullptr) {
            for (size_t i = 0; i < current->occupied_slots; i++) {
                const size_t offset = i * pool->element_size;
                void* element = (uchar*)current->storage + offset;
                destroy(element);
            }
        }
        Mem_Chunk* next_chunk = current->next;
        munmap(current, total_bytes);
        current = next_chunk;
    }
    pool->top  = nullptr;
    pool->size = 0;
}


void* mem_chunk_elt(const Mem_Chunk* chunk, const Elastic_Fixed_Size_Pool* restrict owning_pool, const size_t elt) {
    const size_t offset = owning_pool->element_size * elt;
    if (offset > owning_pool->chunk_size) return nullptr;
    return (void*)chunk->storage + offset;
}
