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
	net->peer_id = (authority ? 0 : NULL_PEER);
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
	//debug_print("make_and_save_base");
	U32 next_base_ix = (net->cur_base_ix + 1) % net->bases.size;
	WorldBaseState *base = &net->bases.data[next_base_ix];

	Ator ator = linear_ator(base->data,
							base->capacity,
							"base_buf");
	WArchive ar = create_warchive(ArchiveType_binary, &ator, ator.capacity);

	base->seq = net->world_seq;
	pack_u32(&ar, &base->seq);
	save_world(&ar, g_env.world);

	if (ar.data_size > base->capacity)
		fail("Too large world");

	base->size = ar.data_size;
	base->peer_has_this = false; // Reset

	net->cur_base_ix = next_base_ix;

	destroy_warchive(&ar);
}

// Send confirmation of receiving and applying world state succesfully
internal void send_confirmation(U32 seq)
{
	//debug_print("SENDING CONFIRMATION seq %i", seq);
	WArchive ar = create_warchive(	ArchiveType_binary,
									frame_ator(),
									32);
	pack_u32(&ar, &seq);
	send_net_msg(NetMsg_world_seq_confirm, ar.data, ar.data_size);
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

internal void make_world_delta(NetState *net, WArchive *ar, int base_ix)
{
	debug_print("make_world_delta: world_seq: %i", net->world_seq);

	RArchive base = create_rarchive(	ArchiveType_binary,
										net->bases.data[base_ix].data,
										net->bases.data[base_ix].size);
	U32 base_seq;
	unpack_u32(&base, &base_seq);

	pack_u32(ar, &base_seq);
	pack_u32(ar, &net->world_seq);

	save_world_delta(ar, g_env.world, &base);
	destroy_rarchive(&base);
}

internal bool resurrect_world_delta(NetState *net, RArchive *ar)
{
	//debug_print("resurrect_world_delta");

	U32 delta_base_seq;
	U32 delta_seq;
	unpack_u32(ar, &delta_base_seq);
	unpack_u32(ar, &delta_seq);

	if (delta_seq < net->world_seq) {
		debug_print("Dismissing world update from the past");
		goto error;
	}

	// Find base with correct sequence number
	U32 base_ix = (U32)-1;
	for (U32 i = 0; i < net->bases.size; ++i) {
		if (net->bases.data[i].seq == delta_base_seq) {
			base_ix = i;
			break;
		}
	}
	if (base_ix == (U32)-1) {
		critical_print("Base %i not present for delta %i", delta_base_seq, delta_seq);
		goto error;
	}

	RArchive base = create_rarchive(	ArchiveType_binary,
									net->bases.data[base_ix].data,
									net->bases.data[base_ix].size);
	U32 base_seq;
	unpack_u32(&base, &base_seq);

	ensure(base_seq == delta_base_seq);

	load_world_delta(ar, g_env.world, &base, net->peer_id); // Don't resurrect nodes which we control
	g_env.physworld->grid.modified = true;
	destroy_rarchive(&base);

	net->world_seq = delta_seq;

	return true;
error:
	return false;
}

internal void deliver_world_authority(NetState *net)
{
	++net->world_seq;
	if (net->world_upd_time <= 0.0) {
		// Send init message
		ClientInit init = {
			.peer_id = 1,
		};
		send_net_msg(NetMsg_client_init, &init, sizeof(init));

		// Send the world.
		// This might not include the most recent client commands (latency), but
		// they will be rescheduled after the base instead of dropping.
		make_and_save_base(net);
		send_net_msg(	NetMsg_base,
						net->bases.data[net->cur_base_ix].data,
						net->bases.data[net->cur_base_ix].size);
	} else if (net->peer_has_received_base) {
		// Send a delta

		// Find most recent which the remote has
		U32 base_ix = NULL_HANDLE;
		U32 max_seq = 0;
		for (U32 i = 0; i < net->bases.size; ++i) {
			WorldBaseState *base = &net->bases.data[i];
			if (!base->peer_has_this)
				continue;
			if (base_ix == NULL_HANDLE || max_seq < base->seq) {
				max_seq = base->seq;
				base_ix = i;
			}
		}
		if (base_ix == NULL_HANDLE) {
			critical_print("Client map is outdated. @todo resend whole map to client");
		} else {
			WArchive measure = create_warchive(ArchiveType_measure, NULL, 0);
			make_world_delta(net, &measure, base_ix);
			U32 delta_size = measure.data_size;
			destroy_warchive(&measure);

			WArchive ar = create_warchive(	ArchiveType_binary,
											frame_ator(),
											delta_size);
			make_world_delta(net, &ar, base_ix);

			// New base using this delta
			make_and_save_base(net);

			debug_print("Sending world update %i with base %i",
						net->bases.data[net->cur_base_ix].seq, net->bases.data[base_ix].seq);
			send_net_msg(NetMsg_delta, ar.data, ar.data_size);

			// @todo Destroy archive?
		}
	}
}

internal void deliver_world_client(NetState *net)
{
	// Send updates on nodes which we control.
	// This should only be a small amount (character nodes), so we don't need delta machinery.
	for (U32 i = 0; i < MAX_NODE_COUNT; ++i) {
		if (!g_env.world->nodes[i].allocated)
			continue;
		if (g_env.world->nodes[i].peer_id != net->peer_id)
			continue; // Send only nodes which we have authority on

		WArchive measure = create_warchive(ArchiveType_measure, NULL, 0);
		save_single_node(&measure, g_env.world, i);
		U32 size = measure.data_size;
		destroy_warchive(&measure);

		WArchive ar = create_warchive(ArchiveType_binary, frame_ator(), size);
		save_single_node(&ar, g_env.world, i);
		send_net_msg(NetMsg_single_node, ar.data, ar.data_size);
		destroy_warchive(&ar);
	}
}

void upd_netstate(NetState *net)
{
	net->game_time += g_env.dt;

	UdpPeer *peer = net->peer;

	// World sync
	if (peer->connected) {
		net->stats_timer += g_env.dt;

		if (net->world_upd_time + net->delta_interval < net->game_time) {
			if (net->authority)
				deliver_world_authority(net);
			else
				deliver_world_client(net);

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
	} else {
		// Reset net state for new client
		net->world_upd_time = -10000;
		net->peer_has_received_base = false;
		for (U32 i = 0; i < net->bases.size; ++i)
			net->bases.data[i].peer_has_this = false;
	}

	UdpMsg *msgs;
	U32 msg_count;
	U32 sent_msg_count, *sent_msgs;
	upd_udp_peer(peer, &msgs, &msg_count, &sent_msgs, &sent_msg_count);

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

			case NetMsg_client_init: {
				if (net->authority) {
					critical_print("Ignoring incoming client init");
					break;
				}

				ClientInit init;
				RArchive ar = create_rarchive(ArchiveType_binary, data, data_size);
				unpack_buf(&ar, &init, sizeof(init));
				destroy_rarchive(&ar);

				net->peer_id = init.peer_id;
			} break;

			case NetMsg_base: {
				if (net->authority) {
					critical_print("Ignoring incoming base");
					break;
				}
				RArchive ar = create_rarchive(ArchiveType_binary, data, data_size);
				debug_print("received base %.fkb", 1.0*ar.data_size/1024);
				resurrect_and_save_base(net, &ar);
				destroy_rarchive(&ar);

				send_confirmation(net->world_seq);
			} break;

			case NetMsg_delta: {
				if (net->authority) {
					critical_print("Ignoring incoming delta");
					break;
				}
				RArchive ar = create_rarchive(ArchiveType_binary, data, data_size);
				debug_print("received delta %.3fkb", 1.0*ar.data_size/1024);
				bool success = resurrect_world_delta(net, &ar);
				destroy_rarchive(&ar);

				if (success) {
					make_and_save_base(net);

					send_confirmation(net->world_seq);
				}
			} break;

			case NetMsg_world_seq_confirm: {
				if (!net->authority) {
					critical_print("Ignoring incoming seq confirm");
					break;
				}
				U32 seq;
				RArchive ar = create_rarchive(ArchiveType_binary, data, data_size);
				unpack_u32(&ar, &seq);
				destroy_rarchive(&ar);

				net->peer_has_received_base = true;

				for (U32 k = 0; k < net->bases.size; ++k) {
					if (net->bases.data[k].seq == seq) {
						// @todo Cancel previous deltas which have not yet reached
						//debug_print("World update %i went through", net->bases.data[k].seq);
						net->bases.data[k].peer_has_this = true;
					}
				}
			} break;

			case NetMsg_single_node: {
				if (!net->authority) {
					critical_print("Ignoring incoming single node");
					break;
				}

				RArchive ar = create_rarchive(ArchiveType_binary, data, data_size);
				// @todo Make sure that peer had authority over this node
				// @todo Don't load updates which are older than most recent (reordered messages)
				load_single_node(&ar, g_env.world);
				destroy_rarchive(&ar);
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

	if (g_env.editor->state == EditorState_invisible) { // Debug UI
		Device *d = g_env.device;
		V2d cursor_on_world = screen_to_world_point(g_env.device->cursor_pos);
		if (d->key_down['t'])
			brush_action(&(BrushAction) {cursor_on_world, 2.0, GRIDCELL_MATERIAL_AIR});
		if (d->key_down['g'])
			brush_action(&(BrushAction) {cursor_on_world, 1.0, GRIDCELL_MATERIAL_GROUND});
	}
}

// Messaging

U32 send_net_msg(NetMsg type, void *data, U32 data_size)
{
	NetMsgHeader *header;
	U32 buf_size = sizeof(*header) + data_size;
	U8 *buf = frame_alloc(buf_size);
	header = (void*)buf;
	header->type = type;
	memcpy(buf + sizeof(*header), data, buf_size - sizeof(*header));

	SentMsgInfo info = buffer_udp_msg(g_env.netstate->peer, buf, buf_size);
	debug_print("net send type %i id %i size %.3fkb", type, info.msg_id, info.msg_size/1024.0);
	return info.msg_id;
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
	create_nodes(g_env.world, def, WITH_ARRAY_COUNT(init_vals), 0, AUTHORITY_PEER);
}

void spawn_action(SpawnAction *action)
{
	if (g_env.netstate->authority)
		local_spawn_action(action);
	else
		send_net_msg(NetMsg_spawn_action, action, sizeof(*action));
}

