#include "memory.h"

internal
U32 malloc_size(void *ptr)
{
	return _msize(ptr);
}

void * alloc_impl(Ator ator, U32 size, void *realloc_ptr, const char *tag)
{
	switch (ator.type) {
		case AtorType_none:
			fail("None allocator used");
		case AtorType_gen:
			if (g_env.os_allocs_forbidden)
				fail("Illegal alloc");
			++g_env.prod_heap_alloc_count;
			if (realloc_ptr)
				return realloc(realloc_ptr, size);
			else
				return malloc(size);
		case AtorType_frame:
			return frame_alloc(size);
		case AtorType_dev:
			if (realloc_ptr)
				dev_free(realloc_ptr);
			return dev_malloc(size);
		default: fail("Unknown allocator type");
	}
}

void * zero_alloc_impl(Ator ator, U32 size, void *realloc_ptr, const char *tag)
{
	switch (ator.type) {
		case AtorType_none:
			fail("None allocator used");
		case AtorType_gen:
			if (g_env.os_allocs_forbidden)
				fail("Illegal alloc: %s", tag);
			++g_env.prod_heap_alloc_count;
			if (realloc_ptr) {
				U32 old_size= malloc_size(realloc_ptr);
				void *ptr= realloc(realloc_ptr, size);
				if (old_size < size)
					memset(ptr + old_size, 0, size - old_size);
				return ptr;
			} else {
				return calloc(1, size);
			}
		case AtorType_frame:
			return memset(frame_alloc(size), 0, size);
		case AtorType_dev:
			if (realloc_ptr)
				dev_free(realloc_ptr);
			 return memset(dev_malloc(size), 0, size);
		default: fail("Unknown allocator type");
	}
}

void free_impl(Ator ator, void *ptr)
{
	switch (ator.type) {
		case AtorType_none:
			return;
		case AtorType_gen:
			free(ptr);
			return;
		case AtorType_frame:
			return;
		case AtorType_dev:
			dev_free(ptr);
			return;
		default: fail("Unknown allocator type");
	}
}

Ator stack_ator()
{ return (Ator) { .type= AtorType_stack }; }

Ator gen_ator()
{ return (Ator) { .type= AtorType_gen }; }

Ator dev_ator()
{ return (Ator) { .type= AtorType_dev }; }

Ator frame_ator()
{ return (Ator) { .type= AtorType_frame }; }



void * dev_malloc(U32 size)
{ return malloc(size); }

void * dev_realloc(void *ptr, U32 size)
{ return realloc(ptr, size); }

void dev_free(void *mem)
{ free(mem); }

void * frame_alloc(U32 size)
{
	/// @todo ALIGNMENT
	U8 *block= (void*)((U64)(g_env.frame_mem + 15) & ~0x0F); // 16-aligned
	g_env.frame_mem= block + size;
	ensure(g_env.frame_mem < g_env.frame_mem_end && "Frame allocator out of space");
	return block;
}

void reset_frame_alloc()
{
	g_env.frame_mem= g_env.frame_mem_begin;
}
