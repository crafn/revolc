#include "core/ensure.h"
#include "core/debug_print.h"
#include "core/malloc.h"
#include "core/udp.h"
#include "core/string.h"
#include "global/env.h"
#include "game/world.h"
#include "game/worldgen.h"
#include "physics/physworld.h"

#define RTS_AUTHORITY_PORT 19995
#define RTS_CLIENT_PORT 19996

typedef struct RtsEnv {
	UdpPeer peer;
	bool authority; // Do we have authority over game world
} RtsEnv;

internal
RtsEnv *rts_env() { return g_env.game_data; }


MOD_API void init_rts()
{
	debug_print("init_rts()");

	RtsEnv *env= zero_malloc(sizeof(*env));
	g_env.game_data= env;

	bool authority= false;
	for (U32 i= 0; i < g_env.argc; ++i) {
		if (!strcmp(g_env.argv[i], "-authority"))
			authority= true;
	}
	rts_env()->peer= create_udp_peer(	authority ? RTS_AUTHORITY_PORT : RTS_CLIENT_PORT,
										authority ? RTS_CLIENT_PORT : RTS_AUTHORITY_PORT);
}

MOD_API void deinit_rts()
{
	debug_print("deinit_rts()");

	destroy_udp_peer(&rts_env()->peer);
	free(rts_env());
}

MOD_API void upd_rts()
{
	UdpPeer *peer= &rts_env()->peer;

	if (peer->last_send_time + 0.5 < g_env.time_from_start) {
		// Maintain connection
		const char *msg = frame_str(rts_env()->authority ? "hello %i" : "world %i", peer->next_msg_id);
		buffer_udp_msg(peer, msg, strlen(msg) + 1);
	}

	if (peer->connected && rts_env()->authority) {
		// Stress test :::D
		//buffer_udp_msg(peer, g_env.physworld->grid, sizeof(*g_env.physworld->grid)*GRID_CELL_COUNT);
	}

	UdpMsg *msgs;
	U32 msg_count;
	upd_udp_peer(peer, &msgs, &msg_count);

	for (U32 i= 0; i < msg_count; ++i) {
		debug_print("message received: %.*s", msgs[i].data_size, msgs[i].data);
	}
}

MOD_API void worldgen_rts(World *w)
{
	generate_test_world(w);
}
