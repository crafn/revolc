#include "core/ensure.h"
#include "core/debug_print.h"
#include "core/malloc.h"
#include "core/udp.h"
#include "core/string.h"
#include "global/env.h"
#include "game/world.h"
#include "game/worldgen.h"
#include "platform/device.h"
#include "physics/physworld.h"
#include "visual/renderer.h"

#define RTS_AUTHORITY_PORT 19995
#define RTS_CLIENT_PORT 19996
#define RTS_SNAPSHOT_INTERVAL 10.0

typedef enum RtsMsg {
	RtsMsg_chat = 1,
	RtsMsg_grid,
	RtsMsg_brushaction,
} RtsMsg;

typedef struct RtsMsgHeader {
	RtsMsg type;
	//F64 time; // Game time
} PACKED RtsMsgHeader;

typedef struct RtsEnv {
	UdpPeer *peer;
	bool authority; // Do we have authority over game world
	F64 game_time; // Same at client and server

	F64 snapshot_time;
} RtsEnv;

internal
RtsEnv *rts_env() { return g_env.game_data; }

MOD_API void init_rts()
{
	debug_print("init_rts()");

	RtsEnv *env= zero_malloc(sizeof(*env));
	g_env.game_data= env;

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
	rts_env()->snapshot_time= -10000.0;
}

MOD_API void deinit_rts()
{
	debug_print("deinit_rts()");

	destroy_udp_peer(rts_env()->peer);
	free(rts_env());
}

typedef struct BrushAction { // @todo Range and precision "attributes"
	V2d pos;
	F64 size;
	U8 material;
} BrushAction;

internal
void local_brushaction(BrushAction *action)
{
	set_grid_material_in_circle(action->pos, action->size, action->material);
}

internal
void send_rts_msg(RtsMsg type, void *data, U32 data_size)
{
	RtsMsgHeader *header;
	U32 buf_size= sizeof(*header) + data_size;
	U8 *buf= frame_alloc(buf_size);
	header= (void*)buf;
	header->type= type;
	memcpy(buf + sizeof(*header), data, buf_size - sizeof(*header));
	buffer_udp_msg(rts_env()->peer, buf, buf_size);
	debug_print("rts send %i: %ikb", type, buf_size/1024);
}

internal
void brushaction(BrushAction *action)
{ // @todo Generate this function
	if (rts_env()->authority) {
		local_brushaction(action);
	} else {
		send_rts_msg(RtsMsg_brushaction, action, sizeof(*action));
	}
}

internal
void apply_world_state(void *data, U32 data_size)
{
	debug_print("Grid received");
	U8 *mat_grid= data;
	if (data_size != GRID_CELL_COUNT)
		fail("Corrupted world state");

	for (U32 i= 0; i < GRID_CELL_COUNT; ++i)
		g_env.physworld->grid[i].material= mat_grid[i];
}

MOD_API void upd_rts()
{
	rts_env()->game_time += g_env.dt;

	{ // UI
		Device *d= g_env.device;
		V2d cursor_on_world= screen_to_world_point(g_env.device->cursor_pos);
		if (d->key_down['t']) {
			brushaction(&(BrushAction) {cursor_on_world, 2.0, GRIDCELL_MATERIAL_AIR});
		}
		if (d->key_down['g']) {
			brushaction(&(BrushAction) {cursor_on_world, 1.0, GRIDCELL_MATERIAL_GROUND});
		}
	}

	{ // Networking
		UdpPeer *peer= rts_env()->peer;

		// World sync
		if (	peer->connected &&
				rts_env()->game_time - rts_env()->snapshot_time > RTS_SNAPSHOT_INTERVAL) {
			if (rts_env()->authority) {
				rts_env()->snapshot_time= rts_env()->game_time;

				// Send a snapshot.
				// This might not include the most recent client commands (latency), but
				// they will be rescheduled after the snapshot instead dropping.
		
				const U32 mat_grid_size= GRID_CELL_COUNT;
				U8 *mat_grid= frame_alloc(mat_grid_size);
				for (U32 i= 0; i < GRID_CELL_COUNT; ++i)
					mat_grid[i]= g_env.physworld->grid[i].material;
				send_rts_msg(RtsMsg_grid, mat_grid, mat_grid_size);
				rts_env()->snapshot_time= rts_env()->game_time;
			}

			//const char *msg = frame_str(rts_env()->authority ? "\1hello %i" : "\1world %i", peer->next_msg_id);
			//buffer_udp_msg(peer, msg, strlen(msg) + 1);

			debug_print("packet loss: %.1f%%", 100.0*peer->drop_count/(peer->acked_packet_count + peer->drop_count));
			debug_print("rtt: %.3f", peer->rtt);
			debug_print("sent packet count: %i", peer->sent_packet_count);
			debug_print("recv packet count: %i", peer->recv_packet_count);
			debug_print("recv msg count: %i", peer->recv_msg_count);
			debug_print("recv incomplete msg count: %i", peer->cur_incomplete_recv_msg_count);
			debug_print("waiting sending count: %i", peer->packets_waiting_send_count);

			//debug_print("acked packet count: %i", peer->acked_packet_count);
			//debug_print("current incomplete recv msgs %i", peer->cur_incomplete_recv_msg_count);
		}

		UdpMsg *msgs;
		U32 msg_count;
		upd_udp_peer(peer, &msgs, &msg_count);

		for (U32 i= 0; i < msg_count; ++i) {
			if (msgs[i].data_size < 2) // RtsMsg takes 1 byte
				fail("Corrupted message");

			RtsMsgHeader *header= msgs[i].data;
			void *data= header + 1;
			U32 data_size= msgs[i].data_size - sizeof(*header);

			//debug_print("Recv %i: %i", header->type, msgs[i].data_size);

			switch (header->type) {
				case RtsMsg_chat:
					debug_print("> %.*s", data_size, data);
				break;

				case RtsMsg_grid: {
					apply_world_state(data, data_size);
				} break;

				case RtsMsg_brushaction: {
					ensure(data_size == sizeof(BrushAction));
					local_brushaction(data);
				} break;
				default: fail("Unknown message type: %i", header->type);
			}
		}
	}
}

MOD_API void worldgen_rts(World *w)
{
	if (rts_env()->authority)
		generate_test_world(w);
}
