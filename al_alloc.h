#ifndef ALD_AL_ALLOCATOR_H
#define ALD_AL_ALLOCATOR_H

typedef void(*Free_Function)(void*);

#define ALLOC_USE_LIBC_MALLOC
#ifdef ALLOC_USE_LIBC_MALLOC
    #include <stdlib.h>
    #define alloc   malloc
    #define dealloc free
    #define realloc realloc
    #define calloc  calloc
#endif

#define new(T) alloc(sizeof(T))

#endif //ALD_AL_ALLOCATOR_H
