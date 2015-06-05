#include "malloc.h"

void* zero_malloc(U32 size)
{
	void* mem= malloc(size);
	memset(mem, 0, size);
	return mem;
}

void * dev_malloc(U32 size)
{ return malloc(size); }

void * dev_realloc(void *ptr, U32 size)
{ return realloc(ptr, size); }

void dev_free(void *mem)
{ free(mem); }

