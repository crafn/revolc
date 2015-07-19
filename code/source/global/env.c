#include "env.h"
#include "symbol.h"
#include "memory.h"

Env g_env;

void init_env(U32 argc, const char **argv)
{
	g_env.argc= argc;
	g_env.argv= argv;

	{ // Frame allocator
		ensure(g_env.frame_ator.buf == NULL);
		g_env.frame_ator=
			linear_ator(	ZERO_ALLOC(gen_ator(), FRAME_MEM_SIZE, "frame"),
							FRAME_MEM_SIZE,
							"frame_ator");
	}

	g_env.used_rtti_symbols=
		ZERO_ALLOC(gen_ator(), sizeof(*g_env.used_rtti_symbols), "rtti_symbols");
}

void deinit_env()
{
	FREE(gen_ator(), g_env.used_rtti_symbols);
	FREE(gen_ator(), g_env.frame_ator.buf);
}

