#include "animation/clip.h"
#include "animation/joint.h"
#include "audio/audiosystem.h"
#include "core/archive.h"
#include "core/debug_print.h"
#include "core/ensure.h"
#include "core/grid.h"
#include "core/memory.h"
#include "core/random.h"
#include "core/scalar.h"
#include "global/env.h"
#include "game/world.h"
#include "minion.h"
#include "physics/physworld.h"
#include "physics/query.h"
#include "platform/device.h"
#include "resources/resblob.h"
#include "visual/renderer.h" // screen_to_world_point, camera


void upd_minion(Minion *minion_begin, Minion *minion_end)
{
	if (!rts_env()->authority)
		return;

	F64 dt= g_env.world->dt;
	for (Minion *minion= minion_begin; minion != minion_end; ++minion) {
		minion->pos.x += 0.5*dt;
	}
}

/*
void pack_minion(WArchive *ar, Minion *minion)
{ // @todo Generate this function. Use arrays.
	pack_f64(ar, &minion->pos.x);
	pack_f64(ar, &minion->pos.y);
}

void unpack_minion(RArchive *ar, Minion *minion)
{ // @todo Generate this function. Use arrays.
	unpack_f64(ar, &minion->pos.x);
	unpack_f64(ar, &minion->pos.y);
}
*/
