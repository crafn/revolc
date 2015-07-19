#include "memory.h"

internal
U32 malloc_size(void *ptr)
{
	return _msize(ptr);
}

internal
void *conditional_memset(void *data, U32 size, bool zero)
{
	if (zero)
		memset(data, 0, size);
	return data;
}

void * alloc_impl(Ator *ator, U32 size, void *realloc_ptr, const char *tag, bool zero)
{
	switch (ator->type) {
		case AtorType_none:
			fail("None allocator used");
		case AtorType_gen:
			if (g_env.os_allocs_forbidden)
				fail("Illegal alloc");
			++g_env.prod_heap_alloc_count;
			if (!zero) {
				if (realloc_ptr)
					return realloc(realloc_ptr, size);
				else
					return malloc(size);
			} else {
				if (realloc_ptr) {
					U32 old_size= malloc_size(realloc_ptr);
					void *ptr= realloc(realloc_ptr, size);
					if (old_size < size)
						memset(ptr + old_size, 0, size - old_size);
					return ptr;
				} else {
					return calloc(1, size);
				}
			}
		case AtorType_linear: {
			void *next_mem= ator->buf + ator->offset;
			ensure(MAX_ALIGNMENT == 16);
			U8 *block= (void*)((U64)(next_mem + 15) & ~0x0F); // 16-aligned
			ator->offset= block + size - ator->buf;
			if (ator->offset > ator->capacity)
				fail("Linear allocator '%s' out of space", ator->tag);
			return conditional_memset(block, size, zero);
		} case AtorType_dev:
			if (realloc_ptr)
				dev_free(realloc_ptr);
			return conditional_memset(dev_malloc(size), size, zero);
		default: fail("Unknown allocator type");
	}
}

void free_impl(Ator *ator, void *ptr)
{
	switch (ator->type) {
		case AtorType_none:
			return;
		case AtorType_gen:
			free(ptr);
			return;
		case AtorType_linear:
			return;
		case AtorType_dev:
			dev_free(ptr);
			return;
		default: fail("Unknown allocator type");
	}
}

Ator *stack_ator()
{
	local_persist Ator ator= { .type= AtorType_stack, .tag= "stack" };
	return &ator;
}

Ator *gen_ator()
{
	local_persist Ator ator= { .type= AtorType_gen, .tag= "general_heap" };
	return &ator;
}

Ator linear_ator(void *buf, U32 size, const char *tag)
{
	return (Ator) {
		.type= AtorType_linear,
		.buf= buf,
		.capacity= size,
		.tag= tag
	};
}

Ator *dev_ator()
{
	local_persist Ator ator= { .type= AtorType_dev, .tag= "dev_heap" };
	return &ator;
}

Ator *leakable_dev_ator()
{
	local_persist Ator ator= { .type= AtorType_dev, .tag= "leakable_dev_heap" };
	return &ator;
}

Ator *frame_ator()
{ return &g_env.frame_ator; }



void * dev_malloc(U32 size)
{ return malloc(size); }

void * dev_realloc(void *ptr, U32 size)
{ return realloc(ptr, size); }

void dev_free(void *mem)
{ free(mem); }

void * frame_alloc(U32 size)
{
	return ALLOC(frame_ator(), size, "frame_alloc");
}

void reset_frame_alloc()
{
	g_env.frame_ator.offset= 0;
}
