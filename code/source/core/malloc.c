#include "malloc.h"

void* zero_malloc(U32 size)
{
	void* ptr= malloc(size);
	memset(ptr, 0, size);
	return ptr;
}

void * dev_malloc(U32 size)
{ return malloc(size); }

void * dev_realloc(void *ptr, U32 size)
{ return realloc(ptr, size); }

void dev_free(void *mem)
{ free(mem); }

