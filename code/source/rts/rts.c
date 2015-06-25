#include "core/debug_print.h"
#include "core/socket.h"
#include "global/env.h"
#include "game/world.h"
#include "game/worldgen.h"

#define NET_ROLE_SERVER 1
#define NET_ROLE_CLIENT 0
U32 net_role = NET_ROLE_CLIENT;

typedef struct RtsPeer {
	Socket socket;
} RtsPeer;

typedef struct RtsEnv {
	RtsPeer peer;
} RtsEnv;

RtsEnv *rts_env() { return g_env.game_data; }

MOD_API void init_rts()
{
	debug_print("init_rts()");

	RtsEnv *env= malloc(sizeof(*env));
	g_env.game_data= env;

/*	if (argc > 0) {
		if (!strcmp(argv[0], "-connect"))
			net_role= NET_ROLE_SERVER;
		else if (!strcmp(argv[0], "-client"))
			net_role= NET_ROLE_CLIENT;
		else
			fail("Unknown net role: %s", argv[arg_i]);
		arg_i++;
	}
	debug_print("net role %i", net_role);
	*/
}

MOD_API void deinit_rts()
{
	free(rts_env());
	debug_print("deinit_rts()");
}

MOD_API void upd_rts()
{
	debug_print("upd_rts()");
}

MOD_API void worldgen_rts(World *w)
{
	generate_test_world(w);
}
