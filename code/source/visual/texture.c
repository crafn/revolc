#include "core/debug_print.h"
#include "core/ensure.h"
#include "texture.h"

void init_Texture(Texture *tex)
{
	debug_print("Texture init: %s", tex->res.name);
}

void deinit_Texture(Texture *tex)
{
	debug_print("Texture deinit: %s", tex->res.name);
}

