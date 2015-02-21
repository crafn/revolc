#include "malloc.h"

void* zero_malloc(U32 size)
{
	void* ptr= malloc(size);
	memset(ptr, 0, size);
	return ptr;
}
