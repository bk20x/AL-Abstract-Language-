#ifndef ALD_AL_ALLOCATOR_H
#define ALD_AL_ALLOCATOR_H

typedef void(*Free_Function)(void*);

#define ALLOC_USE_LIBC
#ifdef ALLOC_USE_LIBC
    #include <stdlib.h>
    #define alloc   malloc
    #define dealloc free
    #define realloc realloc
    #define alloc0  calloc
#endif

#define new(T) (T*)alloc(sizeof(T))

#endif //ALD_AL_ALLOCATOR_H
