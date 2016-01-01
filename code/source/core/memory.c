#include "memory.h"

internal void *commit_uninit_mem(void *data, U32 size)
{ return memset(data, 0x95, size); }

internal void *check_ptr(void *ptr)
{
	ensure(ptr);
	return ptr;
}

void * alloc_impl(Ator *ator, U32 size, void *realloc_ptr, const char *tag)
{
	if (size == 0)
		return NULL;

	// @note Memory allocated from system heap should be always written over so that
	// - accessing uninitialized memory bugs will happen consistently
	// - memory is committed so that costly page faults during game are avoided
	switch (ator->type) {
		case AtorType_none: {
			fail("None allocator used");
		} break;
		case AtorType_gen: {
			if (g_env.os_allocs_forbidden)
				fail("Illegal alloc");
			++g_env.prod_heap_alloc_count;
			if (realloc_ptr) {
				// @todo Commit
				return check_ptr(realloc(realloc_ptr, size));
			} else {
				return commit_uninit_mem(check_ptr(malloc(size)), size);
			}
		} break;
		case AtorType_linear: {
			if (realloc_ptr && realloc_ptr == ator->last_allocd_ptr) {
				// Resize last allocation
				ator->offset -= ator->last_allocd_size;
				ator->offset += size;
				if (ator->offset > ator->capacity)
					fail("Linear allocator '%s' out of space", ator->tag);

				ator->last_allocd_size = size;
				return realloc_ptr;
			} else {
				void *next_mem = ator->buf + ator->offset;
				ensure(MAX_ALIGNMENT == 16);
				U8 *block = (void*)((U64)(next_mem + 15) & ~0x0F); // 16-aligned
				ator->offset = block + size - ator->buf;
				if (ator->offset > ator->capacity)
					fail("Linear allocator '%s' out of space", ator->tag);

				ator->last_allocd_size = size;
				ator->last_allocd_ptr = block;
				return block;
			}
		} break;
		case AtorType_dev:
			if (realloc_ptr) {
				// @todo Commit
				return dev_realloc(realloc_ptr, size);
			} else {
				return commit_uninit_mem(dev_malloc(size), size);
			}
		break;
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
	local_persist Ator ator = { .type = AtorType_stack, .tag = "stack" };
	return &ator;
}

Ator *gen_ator()
{
	local_persist Ator ator = { .type = AtorType_gen, .tag = "general_heap" };
	return &ator;
}

Ator linear_ator(void *buf, U32 size, const char *tag)
{
	return (Ator) {
		.type = AtorType_linear,
		.buf = buf,
		.capacity = size,
		.tag = tag
	};
}

Ator *dev_ator()
{
	local_persist Ator ator = { .type = AtorType_dev, .tag = "dev_heap" };
	return &ator;
}

Ator *leakable_dev_ator()
{
	local_persist Ator ator = { .type = AtorType_dev, .tag = "leakable_dev_heap" };
	return &ator;
}

Ator *frame_ator()
{ return &g_env.frame_ator; }



void * dev_malloc(U32 size)
{ return check_ptr(malloc(size)); }

void * dev_realloc(void *ptr, U32 size)
{ return check_ptr(realloc(ptr, size)); }

void dev_free(void *mem)
{ free(mem); }

void * frame_alloc(U32 size)
{
	return ALLOC(frame_ator(), size, "frame_alloc");
}

void reset_frame_alloc()
{
	g_env.frame_ator.offset = 0;
}
