#include "core/archive.h"
#include "core/ensure.h"
#include "core/debug_print.h"
#include "core/malloc.h"
#include "core/udp.h"
#include "core/string.h"
#include "global/env.h"
#include "game/world.h"
#include "game/worldgen.h"
#include "minion.h"
#include "platform/device.h"
#include "physics/physworld.h"
#include "resources/resblob.h"
#include "visual/renderer.h"

#define RTS_AUTHORITY_PORT 19995
#define RTS_CLIENT_PORT 19996
#define RTS_SNAPSHOT_INTERVAL 5.0

typedef enum RtsMsg {
	RtsMsg_chat = 1,
	RtsMsg_snapshot,
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

	g_env.physworld->debug_draw= true;
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
	debug_print("rts send %i: %.3fkb", type, 1.0*buf_size/1024);
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

// @todo Proof of concept, bad performance. Nodes should be processed by type.
// (which shouldn't be too hard as they're in buffers by types already)
internal
void pack_nodeinfo(WArchive *ar, NodeInfo *node)
{
	pack_u64(ar, &node->node_id);
	pack_u64(ar, &node->group_id);
	pack_strbuf(ar, node->type_name, sizeof(node->type_name));
}
internal
void unpack_nodeinfo(RArchive *ar, NodeInfo *node)
{
	unpack_u64(ar, &node->node_id);
	unpack_u64(ar, &node->group_id);
	unpack_strbuf(ar, node->type_name, sizeof(node->type_name));
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

	U32 packed_count= 0;
	const U32 node_count= g_env.world->node_count; 
	pack_u32(ar, &node_count);
	for (U32 i= 0; i < MAX_NODE_COUNT; ++i) {
		NodeInfo *node= &g_env.world->nodes[i];
		if (!node->allocated)
			continue;

		ensure(packed_count < node_count);
		pack_nodeinfo(ar, node);
		if (!strcmp(node->type_name, "Minion")) {
			Minion *minion= node_impl(g_env.world, NULL, node);
			pack_minion(ar, minion);
		}
		++packed_count;
	}
}

internal
void unpack_world(RArchive *ar)
{
	destroy_world(g_env.world);
	g_env.world= create_world();

	// @todo Grid(s) should be nodes
	const U32 mat_grid_size= GRID_CELL_COUNT;
	U8 *mat_grid= frame_alloc(mat_grid_size);
	unpack_buf(ar, mat_grid, mat_grid_size);
	for (U32 i= 0; i < GRID_CELL_COUNT; ++i)
		g_env.physworld->grid[i].material= mat_grid[i];
	g_env.physworld->grid_modified= true;

	U32 node_count;
	unpack_u32(ar, &node_count);
	debug_print("node count: %i", node_count);
	for (U32 i= 0; i < node_count; ++i) {
		NodeInfo node= {};
		unpack_nodeinfo(ar, &node);
		debug_print("node: %s", node.type_name);
		if (!strcmp(node.type_name, "Minion")) {
			Minion minion;
			unpack_minion(ar, &minion);

			debug_print("@todo create minion");
		}
	}
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
	if (!rts_env()->authority)
		return; // Only server generates world

	generate_test_world(w);

	V3d pos= {0, 5, 0};
	SlotVal init_vals[]= {
		{"minion", "pos", WITH_DEREF_SIZEOF(&pos)},
	};
	NodeGroupDef *def=
		(NodeGroupDef*)res_by_name(g_env.resblob, ResType_NodeGroupDef, "minion");
	create_nodes(g_env.world, def, WITH_ARRAY_COUNT(init_vals), 0);
}
