#ifndef REVOLC_CORE_MALLOC_H
#define REVOLC_CORE_MALLOC_H

#include "build.h"

REVOLC_API WARN_UNUSED
void * zero_malloc(U32 size);

#define STACK_ALLOC(size) alloca(size)
#define ZERO_STACK_ALLOC(size) memset(alloca(size), 0, size)

REVOLC_API WARN_UNUSED
void * dev_malloc(U32 size);
REVOLC_API WARN_UNUSED
void * dev_realloc(void *ptr, U32 size);
REVOLC_API
void dev_free(void *mem);

#endif // REVOLC_CORE_MALLOC_H
