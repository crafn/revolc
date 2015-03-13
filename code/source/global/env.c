#include "env.h"
#include "symbol.h"

Env g_env;

void init_env()
{
	{ // Frame allocator
		ensure(g_env.frame_mem_begin == NULL);
		g_env.frame_mem_begin= g_env.frame_mem= malloc(FRAME_MEM_SIZE);
		g_env.frame_mem_end= g_env.frame_mem_begin + FRAME_MEM_SIZE;
		ensure(g_env.frame_mem_begin != NULL);
	}

	g_env.used_rtti_symbols= zero_malloc(sizeof(*g_env.used_rtti_symbols));
}

void deinit_env()
{
	free(g_env.used_rtti_symbols);
	free(g_env.frame_mem_begin);
}

void * frame_alloc(U32 size)
{
	/// @todo ALIGNMENT
	U8 *block= g_env.frame_mem;
	g_env.frame_mem += size;
	ensure(g_env.frame_mem < g_env.frame_mem_end && "Frame allocator out of space");
	return block;
}

void reset_frame_alloc()
{
	g_env.frame_mem= g_env.frame_mem_begin;
}
