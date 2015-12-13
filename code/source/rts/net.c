#include "net.h"
#include "rts.h"

void send_rts_msg(RtsMsg type, void *data, U32 data_size)
{
	RtsMsgHeader *header;
	U32 buf_size = sizeof(*header) + data_size;
	U8 *buf = frame_alloc(buf_size);
	header = (void*)buf;
	header->type = type;
	memcpy(buf + sizeof(*header), data, buf_size - sizeof(*header));

	SentMsgInfo info = buffer_udp_msg(rts_env()->peer, buf, buf_size);
	debug_print("rts send %i: %.3fkb", type, info.msg_size/1024.0);
}

void local_brush_action(BrushAction *action)
{
	set_grid_material_in_circle(action->pos, action->size, action->material);
}

void brush_action(BrushAction *action)
{
	if (rts_env()->authority)
		local_brush_action(action);
	else
		send_rts_msg(RtsMsg_brush_action, action, sizeof(*action));
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
	if (rts_env()->authority)
		local_spawn_action(action);
	else
		send_rts_msg(RtsMsg_spawn_action, action, sizeof(*action));
}

void make_and_save_base()
{
	debug_print("make_and_save_base");
	RtsEnv *env = rts_env();
	U32 next_base_ix = (env->cur_base_ix + 1) % RTS_MAX_BASE_HISTORY_COUNT;

	Ator ator = linear_ator(	env->bases[next_base_ix].data,
							RTS_BASE_SIZE,
							"base_buf");
	WArchive ar = create_warchive(ArchiveType_binary, &ator, ator.capacity);

	env->bases[next_base_ix].seq = env->world_seq;
	pack_u32(&ar, &env->bases[next_base_ix].seq);
	save_world(&ar, g_env.world);

	if (ar.data_size > RTS_BASE_SIZE)
		fail("Too large world");

	env->bases[next_base_ix].size = ar.data_size;

	env->cur_base_ix = next_base_ix;

	destroy_warchive(&ar);
}

void resurrect_and_save_base(RArchive *ar)
{
	debug_print("resurrect_base");
	RtsEnv *env = rts_env();

	// Save base
	U32 next_base_ix = (env->cur_base_ix + 1) % RTS_MAX_BASE_HISTORY_COUNT;
	if (ar->data_size > RTS_BASE_SIZE)
		fail("Too large world");
	memcpy(env->bases[next_base_ix].data, ar->data, ar->data_size);
	env->bases[next_base_ix].size = ar->data_size;

	unpack_u32(ar, &env->bases[next_base_ix].seq);
	env->cur_base_ix = next_base_ix;
	env->world_seq = env->bases[env->cur_base_ix].seq;

	// Resurrect world
	clear_world_nodes(g_env.world);
	load_world(ar, g_env.world);
	g_env.physworld->grid.modified = true;
}

void make_world_delta(WArchive *ar)
{
	RtsEnv *env = rts_env();
	debug_print("make_world_delta: world_seq: %i", env->world_seq);

	// @todo Use base which has been received by remote
	RArchive base = create_rarchive(	ArchiveType_binary,
									env->bases[env->cur_base_ix].data,
									env->bases[env->cur_base_ix].size);
	U32 base_seq;
	unpack_u32(&base, &base_seq);

	pack_u32(ar, &base_seq);
	pack_u32(ar, &env->world_seq);

	save_world_delta(ar, g_env.world, &base);
	destroy_rarchive(&base);
}

void resurrect_world_delta(RArchive *ar)
{
	debug_print("resurrect_world_delta");
	RtsEnv *env = rts_env();

	U32 delta_base_seq;
	U32 delta_seq;
	unpack_u32(ar, &delta_base_seq);
	unpack_u32(ar, &delta_seq);

	// Find base with correct sequence number
	U32 base_ix = (U32)-1;
	for (U32 i = 0; i < RTS_MAX_BASE_HISTORY_COUNT; ++i) {
		if (env->bases[i].seq == delta_base_seq) {
			base_ix = i;
			break;
		}
	}
	if (base_ix == (U32)-1) {
		critical_print("Base not present for delta: %i", delta_seq);
		goto cleanup;
	}

	RArchive base = create_rarchive(	ArchiveType_binary,
									env->bases[base_ix].data,
									env->bases[base_ix].size);
	U32 base_seq;
	unpack_u32(&base, &base_seq);

	ensure(base_seq == delta_base_seq);

	load_world_delta(ar, g_env.world, &base);
	g_env.physworld->grid.modified = true;
	destroy_rarchive(&base);

	env->world_seq = delta_seq;
cleanup:
	return;
}

void upd_rts_net()
{
	RtsEnv *env = rts_env();
	UdpPeer *peer = env->peer;

	// World sync
	if (peer->connected) {
		env->stats_timer += g_env.dt;

		if (	env->authority &&
				env->world_upd_time + RTS_DELTA_INTERVAL < env->game_time) {
			// Deliver world state

			++env->world_seq;
			if (env->world_upd_time <= 0.0) {
				// Send a base.
				// This might not include the most recent client commands (latency), but
				// they will be rescheduled after the base instead of dropping.
				make_and_save_base();
				send_rts_msg(	RtsMsg_base,
								env->bases[env->cur_base_ix].data,
								env->bases[env->cur_base_ix].size);
			} else {
				// Send a delta
				WArchive measure = create_warchive(ArchiveType_measure, NULL, 0);
				make_world_delta(&measure);
				U32 delta_size = measure.data_size;
				destroy_warchive(&measure);
		
				WArchive ar = create_warchive(	ArchiveType_binary,
												frame_ator(),
												delta_size);
				make_world_delta(&ar);
				send_rts_msg(RtsMsg_delta, ar.data, ar.data_size);

				// New base using this delta
				make_and_save_base();
			}

			env->world_upd_time = env->game_time;
		}

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
	}

	UdpMsg *msgs;
	U32 msg_count;
	upd_udp_peer(peer, &msgs, &msg_count, NULL, NULL);

	for (U32 i = 0; i < msg_count; ++i) {
		if (msgs[i].data_size < 2) // RtsMsg takes 1 byte
			fail("Corrupted message");

		RtsMsgHeader *header = msgs[i].data;
		void *data = header + 1;
		U32 data_size = msgs[i].data_size - sizeof(*header);

		//debug_print("Recv %i: %i", header->type, msgs[i].data_size);

		switch (header->type) {
			case RtsMsg_chat:
				debug_print("> %.*s", data_size, data);
			break;

			case RtsMsg_base: {
				if (env->authority) {
					critical_print("Ignoring incoming base");
					break;
				}
				RArchive ar = create_rarchive(ArchiveType_binary, data, data_size);
				resurrect_and_save_base(&ar);
				destroy_rarchive(&ar);
			} break;

			case RtsMsg_delta: {
				if (env->authority) {
					critical_print("Ignoring incoming delta");
					break;
				}
				RArchive ar = create_rarchive(ArchiveType_binary, data, data_size);
				debug_print("received delta %.3fkb", 1.0*ar.data_size/1024);
				resurrect_world_delta(&ar);
				destroy_rarchive(&ar);

				make_and_save_base();
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
