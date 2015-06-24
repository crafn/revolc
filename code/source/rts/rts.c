#include "global/env.h"
#include "game/world.h"
#include "game/worldgen.h"

MOD_API void rts_worldgen(World *w)
{
	generate_test_world(w);
}
