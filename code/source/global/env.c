#include "env.h"

Env g_env;

void * frame_alloc(U32 size)
{
	/// @todo ALIGNMENT
	U8 *block= g_env.frame_mem;
	g_env.frame_mem += size;
	ensure(g_env.frame_mem < g_env.frame_mem_end && "Frame allocator out of space");
	return block;
}

void init_frame_alloc(U32 size)
{
	ensure(g_env.frame_mem_begin == NULL);
	g_env.frame_mem_begin= g_env.frame_mem=
			malloc(size);
	g_env.frame_mem_end=
		g_env.frame_mem_begin + size;
	ensure(g_env.frame_mem_begin != NULL);
}

void reset_frame_alloc()
{
	g_env.frame_mem= g_env.frame_mem_begin;
	memset(	g_env.frame_mem_begin,
			0,
			g_env.frame_mem_end - g_env.frame_mem_begin);
}
