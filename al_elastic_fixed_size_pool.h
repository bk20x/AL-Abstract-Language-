#ifndef ELASTIC_FIXED_SIZE_POOL_LIBRARY_H
#define ELASTIC_FIXED_SIZE_POOL_LIBRARY_H
#include <stddef.h>
#include "al_cdefs.h"

typedef void (*EFSPDestructor)(void* element);
typedef struct Mem_Chunk Mem_Chunk;
typedef struct Elastic_Fixed_Size_Pool Elastic_Fixed_Size_Pool;
struct Mem_Chunk {
    Mem_Chunk* next;
    size_t     occupied_slots;
    uchar      storage[];
};
void* mem_chunk_elt(const Mem_Chunk*               restrict chunk,
                    const Elastic_Fixed_Size_Pool* restrict owning_pool,
                    size_t elt);


// THIS IS A FIXED SIZE POOL IN THAT THE SIZE OF EACH ELEMENT AND THE AMOUNT OF ELEMENTS WHO LIVE IN A CHUNK ARE FIXED
//... otherwise, it can grow indefinitely
struct Elastic_Fixed_Size_Pool {
    Mem_Chunk*   top;
    size_t       size;
    size_t element_size; // do not change these three fields.
    size_t elements_per_chunk;
    size_t chunk_size;
};
Elastic_Fixed_Size_Pool create_elastic_fixed_size_pool(size_t element_size,
                                                       size_t elements_per_chunk,
                                                       int    initial_chunks);

void* alloc_from_elastic_fixed_size_pool(Elastic_Fixed_Size_Pool* pool);

/* this basically zeroes everything out, it leaves the pool in a usable state though. */
void reset_elastic_fixed_size_pool(const Elastic_Fixed_Size_Pool* pool);
/* same as the above reset except it calls a destructor on each element in each chunk before resetting. */
void reset_elastic_fixed_size_pool_with_dtor(const Elastic_Fixed_Size_Pool*  pool, EFSPDestructor destroy);

/*  completely destroys the fuck out of the pool. don't try to use it after calling these functions.
 *  also, They do not check if the `destroy` parameter is NULL. because who is going to do that shit???just use the regular destroy or reset
 */
void destroy_elastic_fixed_size_pool(Elastic_Fixed_Size_Pool* pool);
void destroy_elastic_fixed_size_pool_with_dtor(Elastic_Fixed_Size_Pool* pool, EFSPDestructor destroy);
#endif // ELASTIC_FIXED_SIZE_POOL_LIBRARY_H
