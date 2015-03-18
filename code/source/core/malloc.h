#ifndef REVOLC_CORE_MALLOC_H
#define REVOLC_CORE_MALLOC_H

#include <stdlib.h>

REVOLC_API WARN_UNUSED
void * zero_malloc(U32 size);

REVOLC_API WARN_UNUSED
void * dev_malloc(U32 size);
void dev_free(void *mem);

#endif // REVOLC_CORE_MALLOC_H
