#include "net.h"
#include "core/memory.h"

DEFINE_ARRAY(WorldBaseState)

NetState *create_netstate(	bool authority, F64 delta_interval,
							U32 state_history_count, U32 state_max_size,
							U16 local_port, IpAddress *remote_addr)
{
	NetState *net = ZERO_ALLOC(gen_ator(), sizeof(*net), "net");
	ensure(!g_env.netstate);
	g_env.netstate = net;

	net->authority = authority;
	net->peer = create_udp_peer(local_port, remote_addr);
	net->delta_interval = delta_interval;
	net->world_upd_time = -10000.0;
	net->bases = create_array(WorldBaseState)(state_history_count);
	for (U32 i = 0; i < state_history_count; ++i) {
		WorldBaseState base = {0};
		base.data = ZERO_ALLOC(gen_ator(), state_max_size, "WorldBaseState data");
		base.capacity = state_max_size;
		push_array(WorldBaseState)(&net->bases, base);
	}
	return net;
}

void destroy_netstate(NetState *net)
{
	ensure(g_env.netstate == net);
	g_env.netstate = NULL;

	for (U32 i = 0; i < net->bases.size; ++i)
		FREE(gen_ator(), net->bases.data[i].data);
	destroy_array(WorldBaseState)(&net->bases);
	destroy_udp_peer(net->peer);
	FREE(gen_ator(), net);
}

internal void make_and_save_base(NetState *net)
{
	debug_print("make_and_save_base");
	U32 next_base_ix = (net->cur_base_ix + 1) % net->bases.size;

	Ator ator = linear_ator(net->bases.data[next_base_ix].data,
							net->bases.data[next_base_ix].capacity,
							"base_buf");
	WArchive ar = create_warchive(ArchiveType_binary, &ator, ator.capacity);

	net->bases.data[next_base_ix].seq = net->world_seq;
	pack_u32(&ar, &net->bases.data[next_base_ix].seq);
	save_world(&ar, g_env.world);

	if (ar.data_size > net->bases.data[next_base_ix].capacity)
		fail("Too large world");

	net->bases.data[next_base_ix].size = ar.data_size;

	net->cur_base_ix = next_base_ix;

	destroy_warchive(&ar);
}

internal void resurrect_and_save_base(NetState *net, RArchive *ar)
{
	debug_print("resurrect_base");

	// Save base
	U32 next_base_ix = (net->cur_base_ix + 1) % net->bases.size;
	if (ar->data_size > net->bases.data[next_base_ix].capacity)
		fail("Too large world");
	memcpy(net->bases.data[next_base_ix].data, ar->data, ar->data_size);
	net->bases.data[next_base_ix].size = ar->data_size;

	unpack_u32(ar, &net->bases.data[next_base_ix].seq);
	net->cur_base_ix = next_base_ix;
	net->world_seq = net->bases.data[net->cur_base_ix].seq;

	// Resurrect world
	clear_world_nodes(g_env.world);
	load_world(ar, g_env.world);
	g_env.physworld->grid.modified = true;
}

internal void make_world_delta(NetState *net, WArchive *ar)
{
	debug_print("make_world_delta: world_seq: %i", net->world_seq);

	// @todo Use base which has been received by remote
	RArchive base = create_rarchive(	ArchiveType_binary,
									net->bases.data[net->cur_base_ix].data,
									net->bases.data[net->cur_base_ix].size);
	U32 base_seq;
	unpack_u32(&base, &base_seq);

	pack_u32(ar, &base_seq);
	pack_u32(ar, &net->world_seq);

	save_world_delta(ar, g_env.world, &base);
	destroy_rarchive(&base);
}

internal void resurrect_world_delta(NetState *net, RArchive *ar)
{
	debug_print("resurrect_world_delta");

	U32 delta_base_seq;
	U32 delta_seq;
	unpack_u32(ar, &delta_base_seq);
	unpack_u32(ar, &delta_seq);

	// Find base with correct sequence number
	U32 base_ix = (U32)-1;
	for (U32 i = 0; i < net->bases.size; ++i) {
		if (net->bases.data[i].seq == delta_base_seq) {
			base_ix = i;
			break;
		}
	}
	if (base_ix == (U32)-1) {
		critical_print("Base not present for delta: %i", delta_seq);
		goto cleanup;
	}

	RArchive base = create_rarchive(	ArchiveType_binary,
									net->bases.data[base_ix].data,
									net->bases.data[base_ix].size);
	U32 base_seq;
	unpack_u32(&base, &base_seq);

	ensure(base_seq == delta_base_seq);

	load_world_delta(ar, g_env.world, &base);
	g_env.physworld->grid.modified = true;
	destroy_rarchive(&base);

	net->world_seq = delta_seq;
cleanup:
	return;
}

void upd_netstate(NetState *net)
{
	net->game_time += g_env.dt;

	UdpPeer *peer = net->peer;

	// World sync
	if (peer->connected) {
		net->stats_timer += g_env.dt;

		if (	net->authority &&
				net->world_upd_time + net->delta_interval < net->game_time) {
			// Deliver world state

			++net->world_seq;
			if (net->world_upd_time <= 0.0) {
				// Send a base.
				// This might not include the most recent client commands (latency), but
				// they will be rescheduled after the base instead of dropping.
				make_and_save_base(net);
				send_net_msg(	NetMsg_base,
								net->bases.data[net->cur_base_ix].data,
								net->bases.data[net->cur_base_ix].size);
			} else {
				// Send a delta
				WArchive measure = create_warchive(ArchiveType_measure, NULL, 0);
				make_world_delta(net, &measure);
				U32 delta_size = measure.data_size;
				destroy_warchive(&measure);
		
				WArchive ar = create_warchive(	ArchiveType_binary,
												frame_ator(),
												delta_size);
				make_world_delta(net, &ar);
				send_net_msg(NetMsg_delta, ar.data, ar.data_size);

				// New base using this delta
				make_and_save_base(net);
			}

			net->world_upd_time = net->game_time;
		}

#if 0
		//const char *msg = frame_str(env->authority ? "\1hello %i" : "\1world %i", peer->next_msg_id);
		//buffer_udp_msg(peer, msg, strlen(msg) + 1);
		if (env->stats_timer > 2.0) {
			F64 packet_loss = peer->drop_count/(peer->acked_packet_count + peer->drop_count);

			debug_print("--- net stats ---");
			debug_print("  packet loss: %.1f%%", 100.0*packet_loss);
			debug_print("  rtt: %.3f", peer->rtt);
			debug_print("  sent packet count: %i", peer->sent_packet_count);
			debug_print("  recv packet count: %i", peer->recv_packet_count);
			debug_print("  recv msg count: %i", peer->recv_msg_count);
			debug_print("  recv incomplete msg count: %i", peer->cur_incomplete_recv_msg_count);
			debug_print("  waiting sending count: %i", peer->packets_waiting_send_count);

			//debug_print("acked packet count: %i", peer->acked_packet_count);
			//debug_print("current incomplete recv msgs %i", peer->cur_incomplete_recv_msg_count);
			env->stats_timer = 0.0;
		}
#endif
	}

	UdpMsg *msgs;
	U32 msg_count;
	upd_udp_peer(peer, &msgs, &msg_count, NULL, NULL);

	for (U32 i = 0; i < msg_count; ++i) {
		if (msgs[i].data_size < 2) // NetMsg takes 1 byte
			fail("Corrupted message");

		NetMsgHeader *header = msgs[i].data;
		void *data = header + 1;
		U32 data_size = msgs[i].data_size - sizeof(*header);

		//debug_print("Recv %i: %i", header->type, msgs[i].data_size);

		switch (header->type) {
			case NetMsg_chat:
				debug_print("> %.*s", data_size, data);
			break;

			case NetMsg_base: {
				if (net->authority) {
					critical_print("Ignoring incoming base");
					break;
				}
				RArchive ar = create_rarchive(ArchiveType_binary, data, data_size);
				debug_print("received base %.fkb", 1.0*ar.data_size/1024);
				resurrect_and_save_base(net, &ar);
				destroy_rarchive(&ar);
			} break;

			case NetMsg_delta: {
				if (net->authority) {
					critical_print("Ignoring incoming delta");
					break;
				}
				RArchive ar = create_rarchive(ArchiveType_binary, data, data_size);
				debug_print("received delta %.3fkb", 1.0*ar.data_size/1024);
				static int asd = 0;
				debug_print("DELTA %i", asd++);
				resurrect_world_delta(net, &ar);
				destroy_rarchive(&ar);

				make_and_save_base(net);
			} break;

			case NetMsg_brush_action: {
				ensure(data_size == sizeof(BrushAction));
				//local_brush_action(data);
			} break;
			case NetMsg_spawn_action: {
				ensure(data_size == sizeof(SpawnAction));
				//local_spawn_action(data);
			} break;
			default: fail("Unknown message type: %i", header->type);
		}
	}

	{ // UI
		Device *d = g_env.device;
		V2d cursor_on_world = screen_to_world_point(g_env.device->cursor_pos);
		if (d->key_down['t'])
			brush_action(&(BrushAction) {cursor_on_world, 2.0, GRIDCELL_MATERIAL_AIR});
		if (d->key_down['g'])
			brush_action(&(BrushAction) {cursor_on_world, 1.0, GRIDCELL_MATERIAL_GROUND});
		if (d->key_pressed['e'])
			spawn_action(&(SpawnAction) {"playerch", cursor_on_world});
	}
}

// Messaging

void send_net_msg(NetMsg type, void *data, U32 data_size)
{
	NetMsgHeader *header;
	U32 buf_size = sizeof(*header) + data_size;
	U8 *buf = frame_alloc(buf_size);
	header = (void*)buf;
	header->type = type;
	memcpy(buf + sizeof(*header), data, buf_size - sizeof(*header));

	SentMsgInfo info = buffer_udp_msg(g_env.netstate->peer, buf, buf_size);
	debug_print("net send %i: %.3fkb", type, info.msg_size/1024.0);
}

void local_brush_action(BrushAction *action)
{
	set_grid_material_in_circle(action->pos, action->size, action->material);
}

void brush_action(BrushAction *action)
{
	if (g_env.netstate->authority)
		local_brush_action(action);
	else
		send_net_msg(NetMsg_brush_action, action, sizeof(*action));
}

void local_spawn_action(SpawnAction *action)
{
	V3d pos = {action->pos.x, action->pos.y, 0};
	F32 health = 1.0;
	SlotVal init_vals[] = {
		{"logic", "pos", WITH_DEREF_SIZEOF(&pos)},
		{"logic", "health", WITH_DEREF_SIZEOF(&health)},
	};
	NodeGroupDef *def =
		(NodeGroupDef*)res_by_name(g_env.resblob, ResType_NodeGroupDef, action->name);
	create_nodes(g_env.world, def, WITH_ARRAY_COUNT(init_vals), 0);
}

void spawn_action(SpawnAction *action)
{
	if (g_env.netstate->authority)
		local_spawn_action(action);
	else
		send_net_msg(NetMsg_spawn_action, action, sizeof(*action));
}

