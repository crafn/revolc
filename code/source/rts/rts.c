#include "core/ensure.h"
#include "core/debug_print.h"
#include "core/malloc.h"
#include "core/socket.h"
#include "core/string.h"
#include "global/env.h"
#include "game/world.h"
#include "game/worldgen.h"

#define RTS_AUTHORITY_PORT 19995
#define RTS_CLIENT_PORT 19996
#define RTS_MAX_PACKET_SIZE 256

typedef struct RtsPeer {
	Socket socket;
	U16 send_port;
	bool authority;
	F64 last_send_time;
	U8 msg_id;
} RtsPeer;

internal
RtsPeer create_rts_peer(bool authority)
{
	RtsPeer peer = {
		.socket= open_udp_socket(authority ? RTS_AUTHORITY_PORT : RTS_CLIENT_PORT),
		.authority= authority,
		.send_port= authority ? RTS_CLIENT_PORT : RTS_AUTHORITY_PORT,
	};
	if (peer.socket == invalid_socket())
		fail("Socket creation failed");
	return peer;
}

void destroy_rts_peer(RtsPeer *peer)
{
	close_socket(&peer->socket);
}

typedef struct RtsEnv {
	RtsPeer peer;
} RtsEnv;

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
	rts_env()->peer= create_rts_peer(authority);
}

MOD_API void deinit_rts()
{
	debug_print("deinit_rts()");

	destroy_rts_peer(&rts_env()->peer);
	free(rts_env());
}

MOD_API void upd_rts()
{
	RtsPeer *peer= &rts_env()->peer;

	// Send packets
	if (peer->last_send_time + 0.5 < g_env.time_from_start) {
		IpAddress addr= {
			127, 0, 0, 1,
			peer->send_port
		};
		const char *msg = frame_str(peer->authority ? "hello %i" : "world %i", peer->msg_id++);
		send_packet(peer->socket, addr, msg, strlen(msg) + 1);
		peer->last_send_time= g_env.time_from_start;

		debug_print("sending");
	}

	// Recv packets
	{
		IpAddress addr;
		U8 packet[RTS_MAX_PACKET_SIZE];
		U32 bytes= recv_packet(peer->socket, &addr, packet, RTS_MAX_PACKET_SIZE);
		if (bytes > 0) {
			debug_print("received: %i, %s", bytes, packet);
		}
	}
}

MOD_API void worldgen_rts(World *w)
{
	generate_test_world(w);
}
