#include "core/archive.h"
#include "core/ensure.h"
#include "core/debug_print.h"
#include "core/memory.h"
#include "core/udp.h"
#include "core/string.h"
#include "global/env.h"
#include "game/world.h"
#include "game/worldgen.h"
#include "minion.h"
#include "platform/device.h"
#include "physics/physworld.h"
#include "resources/resblob.h"
#include "rts.h"
#include "rts/net.h"
#include "visual/renderer.h"

DEFINE_SPARSETABLE(Selection);

RtsEnv *rts_env() { return g_env.game_data; }

MOD_API void init_rts()
{
	debug_print("init_rts()");

	RtsEnv *env= ZERO_ALLOC(gen_ator(), sizeof(*env), "rts_env");
	g_env.game_data= env;

	rts_env()->selection_nodes=
		create_stbl(Selection)(
			gen_ator(),
			RES_BY_NAME(NodeType, "Selection")->max_count);

	bool authority= false;
	bool connect= false;
	IpAddress remote_addr= {};
	for (U32 i= 0; i < g_env.argc; ++i) {
		if (!strcmp(g_env.argv[i], "-authority")) {
			authority= true;
		} else if (g_env.argv[i][0] == '-') {
			connect= true;
			remote_addr= str_to_ip(g_env.argv[i] + 1);
		}
	}
	remote_addr.port= authority ? RTS_CLIENT_PORT : RTS_AUTHORITY_PORT;

	rts_env()->authority= authority;
	rts_env()->peer= create_udp_peer(	authority ? RTS_AUTHORITY_PORT : RTS_CLIENT_PORT,
										connect ? &remote_addr : NULL);
	rts_env()->world_upd_time= -10000.0;

	g_env.physworld->debug_draw= true;
}

MOD_API void deinit_rts()
{
	debug_print("deinit_rts()");

	destroy_udp_peer(rts_env()->peer);
	destroy_stbl(Selection)(&rts_env()->selection_nodes);

	FREE(gen_ator(), rts_env());
}

void upd_rts()
{
	RtsEnv *env= rts_env();
	env->game_time += g_env.dt;

	upd_rts_net();

	{ // UI
		Device *d= g_env.device;
		V2d cursor_on_world= screen_to_world_point(g_env.device->cursor_pos);
		if (d->key_down['t'])
			brush_action(&(BrushAction) {cursor_on_world, 2.0, GRIDCELL_MATERIAL_AIR});
		if (d->key_down['g'])
			brush_action(&(BrushAction) {cursor_on_world, 1.0, GRIDCELL_MATERIAL_GROUND});
		if (d->key_pressed['e'])
			spawn_action(&(SpawnAction) {"minion", cursor_on_world});

		SparseTbl(Selection) *sels= &env->selection_nodes;
		bool already_selected= false;
		for (	Selection *it= begin_stbl(Selection)(sels);
				it != end_stbl(Selection)(sels);
				it = next_stbl(Selection)(sels, it)) {
			bool hit=	length_sqr_v2d(sub_v2d(cursor_on_world, v3d_to_v2d(it->pos))) < 4.0 &&
						!already_selected;
			it->selected= hit;
			already_selected= hit || already_selected;
		}
	}
}

MOD_API void worldgen_rts(World *w)
{
	if (!rts_env()->authority)
		return; // Only server generates world

	generate_test_world(w);

	spawn_action(&(SpawnAction) {"minion", {0, 5}});
}
