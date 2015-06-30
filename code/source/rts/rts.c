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

typedef struct RtsEnv {
	UdpPeer *peer;
	bool authority; // Do we have authority over game world
	F64 last_send_time;
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
}

MOD_API void deinit_rts()
{
	debug_print("deinit_rts()");

	destroy_udp_peer(rts_env()->peer);
	free(rts_env());
}

typedef enum RtsMsg {
	RtsMsg_chat = 1,
	RtsMsg_grid,
	RtsMsg_brush_action,
} RtsMsg;

typedef struct BrushAction { // @todo Range and precision "attributes"
	V2d pos;
	F64 size;
	U8 material;
} BrushAction;

internal
void local_brushaction(BrushAction action)
{
	set_grid_material_in_circle(action.pos, action.size, action.material);
}

internal
void brushaction(BrushAction action)
{
	// @todo Generate this
	local_brushaction(action); // Client prediction

	if (!rts_env()->authority) {
		// Send action to server
		const U32 buf_size= 1 + sizeof(action);
		U8 buf[buf_size];
		buf[0]= RtsMsg_brush_action;
		memcpy(buf + 1, &action, buf_size - 1);
		buffer_udp_msg(rts_env()->peer, buf, buf_size);
	}
}

MOD_API void upd_rts()
{
	{ // UI
		Device *d= g_env.device;
		V2d cursor_on_world= screen_to_world_point(g_env.device->cursor_pos);
		if (d->key_down['t']) {
			brushaction((BrushAction) {cursor_on_world, 2.0, GRIDCELL_MATERIAL_AIR});
		}
		if (d->key_down['g']) {
			brushaction((BrushAction) {cursor_on_world, 1.0, GRIDCELL_MATERIAL_GROUND});
		}
	}

	{ // Networking
		UdpPeer *peer= rts_env()->peer;

		if (peer->connected && g_env.time_from_start - rts_env()->last_send_time > 0.5) {
			rts_env()->last_send_time= g_env.time_from_start;
			//const char *msg = frame_str(rts_env()->authority ? "\1hello %i" : "\1world %i", peer->next_msg_id);
			//buffer_udp_msg(peer, msg, strlen(msg) + 1);

			if (rts_env()->authority) {
				// Stress test :::D
				U32 buf_size= 1 + sizeof(g_env.physworld->grid);
				U8 *buf= frame_alloc(buf_size);
				buf[0]= RtsMsg_grid;
				memcpy(buf + 1, g_env.physworld->grid, buf_size - 1);
				buffer_udp_msg(peer, buf, buf_size);
			}

			debug_print("packet loss: %.1f%%", 100.0*peer->drop_count/(peer->acked_packet_count + peer->drop_count));
			debug_print("rtt: %.3f", peer->rtt);
			//debug_print("sent packet count: %i", peer->sent_packet_count);
			//debug_print("acked packet count: %i", peer->acked_packet_count);
			//debug_print("current incomplete recv msgs %i", peer->cur_incomplete_recv_msg_count);
		}

		UdpMsg *msgs;
		U32 msg_count;
		upd_udp_peer(peer, &msgs, &msg_count);

		for (U32 i= 0; i < msg_count; ++i) {
			if (msgs[i].data_size < 2) // RtsMsg takes 1 byte
				fail("Corrupted message");

			U8 msg_type= *(U8*)msgs[i].data;
			void *data= (U8*)msgs[i].data + 1;
			U32 data_size= msgs[i].data_size - 1;
			switch (msg_type) {
				case RtsMsg_chat:
					debug_print("> %.*s", data_size, data);
				break;

				case RtsMsg_grid: {
					debug_print("grid received");
					GridCell *recv_grid= data;
					if (data_size != sizeof(g_env.physworld->grid))
						fail("Corrupted grid message");
					memcpy(g_env.physworld->grid, recv_grid, data_size);
				} break;
				
				case RtsMsg_brush_action: {
					local_brushaction(*(BrushAction*)data);
				} break;
				default: fail("Unknown message type: %i", msg_type);
			}
		}
	}
}

MOD_API void worldgen_rts(World *w)
{
	generate_test_world(w);
}
