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
#include "visual/renderer.h"

typedef enum RtsMsg {
	RtsMsg_chat = 1,
	RtsMsg_snapshot,
	RtsMsg_brush_action,
	RtsMsg_spawn_action,
} RtsMsg;

typedef struct RtsMsgHeader {
	RtsMsg type;
	//F64 time; // Game time
} PACKED RtsMsgHeader;

internal
void send_rts_msg(RtsMsg type, void *data, U32 data_size)
{
	RtsMsgHeader *header;
	U32 buf_size= sizeof(*header) + data_size;
	U8 *buf= frame_alloc(buf_size);
	header= (void*)buf;
	header->type= type;
	memcpy(buf + sizeof(*header), data, buf_size - sizeof(*header));

	debug_print("rts send %i: %.3fkb", type, 1.0*buf_size/1024);
	buffer_udp_msg(rts_env()->peer, buf, buf_size);
}

RtsEnv *rts_env() { return g_env.game_data; }

MOD_API void init_rts()
{
	debug_print("init_rts()");

	RtsEnv *env= ZERO_ALLOC(gen_ator(), sizeof(*env), "rts_env");
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

	g_env.physworld->debug_draw= true;
}

MOD_API void deinit_rts()
{
	debug_print("deinit_rts()");

	destroy_udp_peer(rts_env()->peer);
	FREE(gen_ator(), rts_env());
}

typedef struct BrushAction { // @todo Range and precision "attributes"
	V2d pos;
	F64 size;
	U8 material;
} BrushAction;


internal
void local_brush_action(BrushAction *action)
{
	set_grid_material_in_circle(action->pos, action->size, action->material);
}

internal
void brush_action(BrushAction *action)
{ // @todo Generate this function
	if (rts_env()->authority)
		local_brush_action(action);
	else
		send_rts_msg(RtsMsg_brush_action, action, sizeof(*action));
}

typedef struct SpawnAction {
	char name[RES_NAME_SIZE];
	V2d pos;
} SpawnAction;


internal
void local_spawn_action(SpawnAction *action)
{
	V3d pos= {action->pos.x, action->pos.y, 0};
	SlotVal init_vals[]= {
		{"logic", "pos", WITH_DEREF_SIZEOF(&pos)},
	};
	NodeGroupDef *def=
		(NodeGroupDef*)res_by_name(g_env.resblob, ResType_NodeGroupDef, action->name);
	create_nodes(g_env.world, def, WITH_ARRAY_COUNT(init_vals), 0);
}

internal
void spawn_action(SpawnAction *action)
{ // @todo Generate this function
	if (rts_env()->authority)
		local_spawn_action(action);
	else
		send_rts_msg(RtsMsg_spawn_action, action, sizeof(*action));
}

internal
void pack_world(WArchive *ar)
{
	// @todo Grid(s) should be nodes
	const U32 mat_grid_size= GRID_CELL_COUNT;
	U8 *mat_grid= frame_alloc(mat_grid_size);
	for (U32 i= 0; i < GRID_CELL_COUNT; ++i)
		mat_grid[i]= g_env.physworld->grid[i].material;
	pack_buf(ar, mat_grid, mat_grid_size);

	save_world(g_env.world, ar);
}

internal
void unpack_world(RArchive *ar)
{
	clear_world_nodes(g_env.world);

	// @todo Grid(s) should be nodes
	const U32 mat_grid_size= GRID_CELL_COUNT;
	U8 *mat_grid= frame_alloc(mat_grid_size);
	unpack_buf(ar, mat_grid, mat_grid_size);
	for (U32 i= 0; i < GRID_CELL_COUNT; ++i)
		g_env.physworld->grid[i].material= mat_grid[i];
	g_env.physworld->grid_modified= true;

	load_world(g_env.world, ar);
}

MOD_API void upd_rts()
{
	rts_env()->game_time += g_env.dt;

	{ // UI
		Device *d= g_env.device;
		V2d cursor_on_world= screen_to_world_point(g_env.device->cursor_pos);
		if (d->key_down['t'])
			brush_action(&(BrushAction) {cursor_on_world, 2.0, GRIDCELL_MATERIAL_AIR});
		if (d->key_down['g'])
			brush_action(&(BrushAction) {cursor_on_world, 1.0, GRIDCELL_MATERIAL_GROUND});
		if (d->key_pressed['e'])
			spawn_action(&(SpawnAction) {"minion", cursor_on_world});
	}

	{ // Networking
		UdpPeer *peer= rts_env()->peer;

		// World sync
		if (	peer->connected &&
				rts_env()->game_time - rts_env()->snapshot_time > RTS_SNAPSHOT_INTERVAL) {
			rts_env()->snapshot_time= rts_env()->game_time;
			if (rts_env()->authority) {

				// Send a snapshot.
				// This might not include the most recent client commands (latency), but
				// they will be rescheduled after the snapshot instead dropping.

				WArchive measure= create_warchive(ArchiveType_measure, 0);
				pack_world(&measure);
				U32 world_size= measure.data_size;
				destroy_warchive(&measure);
		
				WArchive ar= create_warchive(ArchiveType_binary, world_size);
				pack_world(&ar);
				send_rts_msg(RtsMsg_snapshot, ar.data, ar.data_size);
				destroy_warchive(&ar);

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

				case RtsMsg_snapshot: {
					RArchive ar= create_rarchive(ArchiveType_binary, data, data_size);
					unpack_world(&ar);
					destroy_rarchive(&ar);
				} break;

				case RtsMsg_brush_action: {
					ensure(data_size == sizeof(BrushAction));
					local_brush_action(data);
				} break;
				case RtsMsg_spawn_action: {
					ensure(data_size == sizeof(SpawnAction));
					local_spawn_action(data);
				} break;
				default: fail("Unknown message type: %i", header->type);
			}
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
