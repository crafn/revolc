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
		minion->pos.x += 0.5*dt*(ABS(sin(minion->pos.x)) + 0.1);
	}
}

void pack_minion(WArchive *ar, const Minion *begin, const Minion *end)
{ // @todo Generate this function
	for (const Minion *minion= begin; minion != end; ++minion) {
		pack_f64(ar, &minion->pos.x);
		pack_f64(ar, &minion->pos.y);
		pack_f32(ar, &minion->health);
	}
}

void unpack_minion(RArchive *ar, Minion *begin, Minion *end)
{ // @todo Generate this function
	for (Minion *minion= begin; minion != end; ++minion) {
		unpack_f64(ar, &minion->pos.x);
		unpack_f64(ar, &minion->pos.y);
		unpack_f32(ar, &minion->health);
	}
}
