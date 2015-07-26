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

void upd_selection(Selection *begin, Selection *end)
{
	for (Selection *sel= begin; sel != end; ++sel) {
		if (!sel->selected)
			continue;
		T3d tf= {{1, 1, 1}, identity_qd(), sel->pos};
		drawcmd_model(	tf,
						(Model*)res_by_name(g_env.resblob, ResType_Model, "unitquad"),
						(Color){1, 1, 1, 0.2}, 0, 0.0);
	}
}

Handle resurrect_selection(const Selection *dead)
{ return insert_stbl(Selection)(&rts_env()->selection_nodes, *dead); }

void free_selection(Handle h)
{ remove_stbl(Selection)(&rts_env()->selection_nodes, h); }

void *storage_selection()
{ return rts_env()->selection_nodes.array; }
