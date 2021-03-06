#include "udp.h"

#define MINIZ_HEADER_FILE_ONLY
#include <miniz/miniz.c>

// Allocation functions used by miniz
void *mz_ext_alloc(size_t size)
{ return ALLOC(frame_ator(), size, "mz_alloc"); }
void *mz_ext_realloc(void *ptr, size_t size)
{ return REALLOC(frame_ator(), ptr, size, "mz_realloc"); }
void mz_ext_free(void *ptr)
{ FREE(frame_ator(), ptr); }

#define UDP_HEARTBEAT_MSG_ID 0

internal
bool wrapped_gr(U32 s1, U32 s2, U32 max)
{ return	((s1 > s2) && (s1 - s2 <= max/2)) ||
			((s2 > s1) && (s2 - s1 > max/2)); }

// (re)init
internal void init_udp_peer(UdpPeer *peer, U16 local_port, IpAddress *remote_addr)
{
	peer->socket = open_udp_socket(local_port);
	peer->local_port = local_port;
	peer->remote_addr = remote_addr ? *remote_addr : (IpAddress) {};
	peer->rtt = 0.5; // Better to be too large than too small at the startup
	peer->next_msg_id = 1; // 0 is heartbeat

	for (U32 i = 0; i < UDP_PACKET_ID_COUNT; ++i) {
		peer->packet_id_to_send_buffer_ix[i] = NULL_HANDLE;
	}
	if (remote_addr)
		debug_print("Trying to connect: %s", ip_to_str(peer->remote_addr));
	else
		debug_print("Waiting for connection");

	if (peer->socket == invalid_socket())
		fail("Socket creation failed");

}

internal void deinit_udp_peer(UdpPeer *peer)
{
	close_socket(&peer->socket);
	clear_tbl(U32, U32)(&peer->sent_msg_acks);
}

UdpPeer *create_udp_peer(U16 local_port, IpAddress *remote_addr)
{
	UdpPeer *peer = ZERO_ALLOC(gen_ator(), sizeof(*peer), "udp_peer");
	init_udp_peer(peer, local_port, remote_addr);
	peer->sent_msg_acks = create_tbl(U32, U32)(	UDP_HEARTBEAT_MSG_ID, 0,
												gen_ator(), UDP_MAX_BUFFERED_PACKET_COUNT);
	return peer;
}

void destroy_udp_peer(UdpPeer *peer)
{
	deinit_udp_peer(peer);
	destroy_tbl(U32, U32)(&peer->sent_msg_acks);
	FREE(gen_ator(), peer);
}

internal
void buffer_udp_packet(	UdpPeer *peer, const void *data, U16 size,
						U32 msg_id, U16 frag_ix, U16 frag_count)
{
	// Not nice
	U32 free_i = 0;
	while (	peer->send_buffer_state[free_i] != UdpPacketState_free &&
			free_i < UDP_MAX_BUFFERED_PACKET_COUNT)
		++free_i;

	if (free_i >= UDP_MAX_BUFFERED_PACKET_COUNT)
		fail("Too many packets in send buffer, max %i", UDP_MAX_BUFFERED_PACKET_COUNT);

	UdpPacket packet = {};
	memcpy(packet.data, data, size);

	packet.header = (UdpPacketHeader) {
		.packet_id = 42, // id set at packet send time
		.msg_id = msg_id,
		.msg_frag_count = frag_count,
		.msg_frag_ix = frag_ix,
		.data_size = size,
		.acked = false, // acks set at packet send time
	};

	peer->send_buffer[free_i] = packet;
	peer->send_buffer_state[free_i] = UdpPacketState_buffered;
	++peer->packets_waiting_send_count;
}

SentMsgInfo buffer_udp_msg(UdpPeer *peer, const void *data, U32 size)
{
	ensure(!data || size > 0);
	U32 frag_count = 1;
	unsigned char *comp_data = NULL;
	mz_ulong comp_size = size*2; // Maybe compressed data won't grow 2x
	if (size > 0) {
		comp_size += 64; // Add a little more, tiny messages might get longer when compressed
		comp_data = ALLOC(frame_ator(), comp_size, "comp_data");
		int ret =
			mz_compress2(comp_data, &comp_size, data, size, UDP_COMPRESSION_LEVEL);
		if (ret != MZ_OK)
			fail("mz_compress2 failed: %i, %s", ret, mz_error(ret));
		frag_count = (comp_size - 1)/UDP_MAX_PACKET_DATA_SIZE + 1;
	}
	U32 left_size = comp_size;
	U32 msg_id = comp_data ? peer->next_msg_id++ : UDP_HEARTBEAT_MSG_ID;
	for (U32 i = 0; i < frag_count; ++i) {
		buffer_udp_packet(	peer,
							(const U8*)comp_data + i*UDP_MAX_PACKET_DATA_SIZE,
							MIN(left_size, UDP_MAX_PACKET_DATA_SIZE),
							msg_id,
							i,
							frag_count);
		left_size -= UDP_MAX_PACKET_DATA_SIZE;
	}

	//if (msg_id != UDP_HEARTBEAT_MSG_ID)
	//	debug_print("SENDING MSG %i WITH %i PACKETS", msg_id, frag_count);
	
	SentMsgInfo info = {
		.msg_id = msg_id,
		.msg_size = comp_size,
	};
	return info;
}

struct SendPriority {
	U32 buf_ix; // send_buffer
	U32 msg_id;
	U16 msg_frag_ix;
	UdpPacketState state;
};

internal
int send_priority_cmp(const void *a_, const void *b_)
{
	const struct SendPriority *a = a_, *b = b_;
	if (	a->state != UdpPacketState_buffered &&
			b->state == UdpPacketState_buffered) {
		return 1;
	} else if (	a->state == UdpPacketState_buffered &&
				b->state != UdpPacketState_buffered) {
		return -1;
	} else {
		if (a->msg_id != b->msg_id)
			return a->msg_id - b->msg_id;
		else
			return a->msg_frag_ix - b->msg_frag_ix;
	}
}

internal
int recv_packet_cmp(const void *a_, const void *b_)
{
	const UdpPacketHeader *a = a_, *b = b_;
	if (a->data_size == 0 && b->data_size > 0) {
		return 1;
	} else if (a->data_size > 0 && b->data_size == 0) {
		return -1;
	} else {
		if (a->msg_id != b->msg_id)
			return a->msg_id - b->msg_id;
		else
			return a->msg_frag_ix - b->msg_frag_ix;
	}
}

void upd_udp_peer(	UdpPeer *peer,
					UdpMsg **msgs, U32 *msg_count,
					U32 **acked_msgs, U32 *acked_msg_count)
{
	ensure(!acked_msgs == !acked_msg_count);
	if (acked_msgs) {
		*acked_msgs = ALLOC(	frame_ator(),
							sizeof(**acked_msgs)*UDP_ACK_COUNT,
							"acked_msgs");
		*acked_msg_count = 0;
	}

	bool has_target = peer->remote_addr.port != 0;

	if (	has_target &&
			g_env.time_from_start - peer->last_send_time > UDP_HEARTBEAT_INTERVAL)
		buffer_udp_msg(peer, NULL, 0); // Heartbeat

	if (has_target) { // Send buffered packets
		IpAddress peer_addr = peer->remote_addr;

		// Form a priority order
		struct SendPriority *priority_buf =
			frame_alloc(sizeof(*priority_buf)*UDP_MAX_BUFFERED_PACKET_COUNT);
		for (U32 i = 0; i < UDP_MAX_BUFFERED_PACKET_COUNT; ++i) {
			priority_buf[i] = (struct SendPriority) {
				.buf_ix = i,
				.msg_id = peer->send_buffer[i].header.msg_id,
				.msg_frag_ix = peer->send_buffer[i].header.msg_frag_ix,
				.state = peer->send_buffer_state[i],
			};
		}
		qsort(priority_buf, UDP_MAX_BUFFERED_PACKET_COUNT,
				sizeof(*priority_buf), send_priority_cmp);

		// Send packets with highest priority first
		bool first_packet = true;
		U8 first_packet_id = 0;
		U32 frame_sent_bytes = 0;
		U32 frame_sent_count = 0;
		for (U32 priority_i = 0; priority_i < UDP_MAX_BUFFERED_PACKET_COUNT; ++priority_i) {
			U32 i = priority_buf[priority_i].buf_ix;
			if (peer->send_buffer_state[i] != UdpPacketState_buffered)
				continue;
			UdpPacket *packet = &peer->send_buffer[i];
			UdpPacketHeader *header = &packet->header;
			header->packet_id = peer->next_packet_id++;
			header->acked = peer->recv_packet_count > 0;
			header->ack_id = peer->remote_packet_id;
			header->previous_acks = peer->prev_out_acks;
			U32 packet_size = sizeof(*header) + header->data_size;
			if (peer->next_packet_id == 0)
				debug_print("PACKET_ID WRAPPED");

			if (first_packet) {
				first_packet = false;
				first_packet_id = header->packet_id;
			}

			if (wrapped_gr(first_packet_id, header->packet_id, UDP_PACKET_ID_COUNT)) {
				fail("Sending too many packets at once");
			}

			if (frame_sent_bytes + packet_size > UDP_OUTGOING_LIMIT_FRAME) {
#if 0
				debug_print("Limiting sending (bandwidth)");
#endif
				break; // Limit sending to prevent packet loss in kernel
			}

			if (frame_sent_count > UDP_ACK_COUNT/2) {
				debug_print("Limiting sending (packet count)");
				break; // Don't overflow ack id's. Could be done without limiting by gradual ack_id change
			}

			ensure(header->msg_id == UDP_HEARTBEAT_MSG_ID || header->data_size > 0);
			U32 bytes = send_packet(	peer->socket, peer_addr,
									packet, packet_size);
			if (bytes == 0)
				critical_print("Packet lost at send: %i", packet->header.packet_id);
			else if (bytes != packet_size)
				fail("Udp packet with incorrect size sent");

			--peer->packets_waiting_send_count;
			frame_sent_bytes += packet_size;
			++frame_sent_count;

			peer->last_sent_ack_id = header->ack_id;
			peer->last_send_time = g_env.time_from_start;
			peer->send_times[packet->header.packet_id] = g_env.time_from_start;
			// Even unreliable packets wait for ack to measure drop rate
			peer->send_buffer_state[i] = UdpPacketState_waiting_ack;
			peer->packet_id_to_send_buffer_ix[header->packet_id] = i;

			++peer->sent_packet_count;
		}
	}

	// Recv packets
	{
		IpAddress addr;
		UdpPacket packet;
		bool packet_received = false;

		ensure(msgs && msg_count);
		U32 max_msg_count = UDP_MAX_BUFFERED_PACKET_COUNT;
		*msgs = frame_alloc(sizeof(*msgs)*max_msg_count);
		*msg_count = 0;

		U32 bytes = 0;
		while (	(bytes = recv_packet(peer->socket, &addr, &packet, UDP_MAX_PACKET_SIZE))
				> 0) {
			if (peer->simulated_packet_loss > 0.0f && (rand() % 1000)/1000.0f < peer->simulated_packet_loss)
				continue;

			if (peer->connected && !ip_equals(peer->remote_addr, addr))
				continue; // Discard packets from others when connected

			if (bytes < sizeof(UdpPacketHeader))
				fail("Corrupted UDP packet (header too small)");

			if (	(	packet.header.data_size == 0 &&
						packet.header.msg_id != UDP_HEARTBEAT_MSG_ID) ||
					packet.header.data_size > UDP_MAX_PACKET_DATA_SIZE ||
					packet.header.data_size != bytes - sizeof(UdpPacketHeader))
				fail("Corrupted UDP packet (wrong size)");

			//if (packet.header.msg_id == UDP_HEARTBEAT_MSG_ID)
			//	debug_print("Heartbeat: %i", packet.header.packet_id);
			packet_received = true;
			if (!peer->connected) {
				peer->connected = true;
				peer->remote_addr = addr;
				debug_print("Connected to %s", ip_to_str(peer->remote_addr));
			}

			U32 packet_id = packet.header.packet_id;
			if (wrapped_gr(	peer->last_sent_ack_id,
							packet_id,
							UDP_PACKET_ID_COUNT)) {
				debug_print("Too many recvs, not enough sends (to ack), dropping");
				continue;
			}

			peer->last_recv_time = g_env.time_from_start;
			++peer->recv_packet_count;

			// Update next acks to remote
			if (wrapped_gr(packet_id, peer->remote_packet_id, UDP_PACKET_ID_COUNT)) {
				// Received more recent remote packet
				U32 shift = packet_id - peer->remote_packet_id;
				peer->prev_out_acks = peer->prev_out_acks << shift;
				peer->prev_out_acks |= 1 << (shift - 1);

				peer->remote_packet_id = packet_id;
			} else if (packet_id != peer->remote_packet_id) {
				// Received older than recent remote packet
				U32 shift = peer->remote_packet_id - packet_id;
				peer->prev_out_acks |= 1 << (shift - 1);
			}

			// Check which our packets were acked
			if (packet.header.acked) {
				for (U32 ack_i = 0; ack_i < UDP_ACK_COUNT; ++ack_i) {
					U8 ack = 0;
					if (ack_i == 0) {
						ack = packet.header.ack_id;
					} else {
						bool bit = (packet.header.previous_acks >> (ack_i - 1)) & 1;
						if (!bit)
							continue;
						ack = packet.header.ack_id - ack_i;
					}

					U32 acked_ix = peer->packet_id_to_send_buffer_ix[ack];
					if (acked_ix == NULL_HANDLE)
						continue; // Packet already acknowledged
					ensure(acked_ix < UDP_MAX_BUFFERED_PACKET_COUNT);
					if (peer->send_buffer_state[acked_ix] != UdpPacketState_waiting_ack)
						continue; // New packet has been written to this place, or 

					F64 rtt_sample = g_env.time_from_start - peer->send_times[ack];
					//debug_print("ack %i", ack);
					//debug_print("rtt_sample: %f, %f - %f",
					//		rtt_sample, g_env.time_from_start, peer->send_times[ack]);
					const F64 change_p = 0.1;
					peer->rtt = peer->rtt*(1 - change_p) + rtt_sample*change_p;
					++peer->acked_packet_count;

					// Update msg progression
					const UdpPacket* acked_packet = &peer->send_buffer[acked_ix];

					if (acked_packet->header.msg_id != UDP_HEARTBEAT_MSG_ID) {
						U32 packet_sent_count = get_tbl(U32, U32)(&peer->sent_msg_acks, acked_packet->header.msg_id);
						ensure(packet_sent_count < acked_packet->header.msg_frag_count);
						if (ack != acked_packet->header.packet_id) {
							fail("ack mismatch %i != %i", ack, acked_packet->header.packet_id);
						}
						++packet_sent_count;
						if (packet_sent_count == acked_packet->header.msg_frag_count) {
							//debug_print("MSG %i SUCCESFULLY SENT, %i packets", acked_packet->header.msg_id, acked_packet->header.msg_frag_count);
							// Msg succesfully sent, notify caller
							if (acked_msgs && acked_msg_count) {
								ensure(*acked_msg_count < UDP_ACK_COUNT);
								(*acked_msgs)[(*acked_msg_count)++] = acked_packet->header.msg_id;
							}
							set_tbl(U32, U32)(	&peer->sent_msg_acks,
												acked_packet->header.msg_id,
												0);
						} else {
							//debug_print("MSG %i PIECE %i/%i (%i) ACKNOWLEDGED", acked_packet->header.msg_id, acked_packet->header.msg_frag_ix, acked_packet->header.msg_frag_count - 1, acked_packet->header.packet_id);
							// Update succesfully sent packet count of the msg
							set_tbl(U32, U32)(	&peer->sent_msg_acks,
												acked_packet->header.msg_id,
												packet_sent_count);
						}
					}

					// Remove succesfully transferred packet from send-buffer
					peer->send_buffer_state[acked_ix] = UdpPacketState_free;
					peer->packet_id_to_send_buffer_ix[ack] = NULL_HANDLE;
				}
			}

			// Write received packet to buffer
			if (packet.header.msg_id != UDP_HEARTBEAT_MSG_ID) {
				U32 free_i = 0;
				while (	peer->recv_buffer[free_i].header.data_size > 0 &&
						free_i < UDP_MAX_BUFFERED_PACKET_COUNT)
					++free_i;

				// @todo This shouldn't terminate
				if (free_i >= UDP_MAX_BUFFERED_PACKET_COUNT)
					fail(	"Too many packets in recv buffer, max %i",
							UDP_MAX_BUFFERED_PACKET_COUNT);

				//debug_print("MSG %i PIECE %i/%i (%i) RECV", packet.header.msg_id, packet.header.msg_frag_ix, packet.header.msg_frag_count - 1, packet.header.packet_id);
				ensure(packet.header.data_size > 0);
				peer->recv_buffer[free_i] = packet;
			}

			//debug_print("data: %.*s", packet.header.data_size, packet.data);
			//debug_print("rtt: %f", peer->rtt);
		}

		if (	!packet_received && peer->connected &&
				peer->last_recv_time + UDP_CONNECTION_TIMEOUT < g_env.time_from_start) {
			U16 local_port = peer->local_port;
			IpAddress remote_addr = peer->remote_addr;
			deinit_udp_peer(peer);
			init_udp_peer(peer, local_port, remote_addr.port == 0 ? NULL : &remote_addr);
			debug_print("Disconnected from %s", ip_to_str(peer->remote_addr));
		}

		// Write complete messages to caller
		if (packet_received) {
			// Sort recv buffer so that messages are contiguous,
			// makes finding complete messages easy.
			// Can sort in-place because no indices are taken to the buf.
			qsort(peer->recv_buffer, UDP_MAX_BUFFERED_PACKET_COUNT,
					sizeof(*peer->recv_buffer), recv_packet_cmp);

#if 0
			for (U32 i = 0; i < UDP_MAX_BUFFERED_PACKET_COUNT; ++i) {
				debug_print(	"%i: data_size: %i, msg_id: %i, frag_ix: %i",
								i,
								peer->recv_buffer[i].header.data_size,
								peer->recv_buffer[i].header.msg_id,
								peer->recv_buffer[i].header.msg_frag_ix);
			}
#endif

			peer->cur_incomplete_recv_msg_count = 0;
			U32 msg_begin_i = 0;
			while (msg_begin_i < UDP_MAX_BUFFERED_PACKET_COUNT) {
				// Check if message is complete
				// @note UDP packets can be duplicated!
				UdpPacketHeader first_header = peer->recv_buffer[msg_begin_i].header;
				if (first_header.data_size == 0) {
					++msg_begin_i;
					continue;
				}
				U32 cur_frag_i = 0;
				U32 msg_end_i = msg_begin_i;
				U32 msg_data_size = 0;
				for (; msg_end_i < UDP_MAX_BUFFERED_PACKET_COUNT; ++msg_end_i) {
					U32 i = msg_end_i;
					UdpPacketHeader *h = &peer->recv_buffer[i].header;
					if (	h->msg_id != first_header.msg_id ||
							h->msg_frag_ix >= first_header.msg_frag_count)
						break;
					if (h->msg_frag_ix == cur_frag_i) {
						++cur_frag_i;
						msg_data_size += h->data_size;
					}
				}

				bool complete = cur_frag_i == first_header.msg_frag_count;
				if (complete) {
					// Join packet datas and write message to the caller

					void *copied_data_begin = frame_alloc(msg_data_size);
					U8 *copied_data = copied_data_begin;
					U32 cur_frag_i = 0;
					for (U32 i = msg_begin_i; i < msg_end_i; ++i) {
						UdpPacket *packet = &peer->recv_buffer[i];
						if (packet->header.msg_frag_ix != cur_frag_i)
							continue;
						memcpy(copied_data, packet->data, packet->header.data_size);
						copied_data += packet->header.data_size;
						++cur_frag_i;
					}

					mz_ulong uncomp_size = UDP_MAX_MSG_SIZE;
					unsigned char *uncomp_data = ALLOC(frame_ator(), uncomp_size, "uncomp_data");
					int ret =
						mz_uncompress(uncomp_data, &uncomp_size, copied_data_begin, msg_data_size);
					if (ret != MZ_OK)
						fail("mz_uncompress failed: %i, %s", ret, mz_error(ret));
					ensure(*msg_count < max_msg_count);
					(*msgs)[(*msg_count)++] = (UdpMsg) {
						.data = uncomp_data,
						.data_size = uncomp_size,
					};

					// Erase processed packets from buffer
					for (U32 i = msg_begin_i; i < msg_end_i; ++i)
						peer->recv_buffer[i].header.data_size = 0;
					++peer->recv_msg_count;
				} else {
					++peer->cur_incomplete_recv_msg_count;
				}

				msg_begin_i = msg_end_i;
			}
		}
	}

	{ // Detect dropped packets
		for (U32 i = 0; i < UDP_MAX_BUFFERED_PACKET_COUNT; ++i) {
			if (peer->send_buffer_state[i] != UdpPacketState_waiting_ack)
				continue;

			UdpPacket *packet = &peer->send_buffer[i];

			// If we have waited too long, consider packet dropped
			if (	g_env.time_from_start - peer->send_times[packet->header.packet_id]
					< peer->rtt*UDP_DROP_RTT_MUL)
				continue;

#if 0
			debug_print(	"Packet id %i, %i (%i/%i) dropped (rtt %f)",
							packet->header.packet_id,
							packet->header.msg_id,
							packet->header.msg_frag_ix,
							packet->header.msg_frag_count,
							peer->rtt);
#endif
			++peer->drop_count;

			// @todo Only for reliable messages
			if (packet->header.msg_id != UDP_HEARTBEAT_MSG_ID) {
				//debug_print("RESENDING MSG %i PIECE %i/%i (%i)", packet->header.msg_id, packet->header.msg_frag_ix, packet->header.msg_frag_count - 1, packet->header.packet_id);
				// Note that rebuffering is a bit unnecessary.
				// UdpPacket would just need a new packet id and state set to buffered for resending.
				buffer_udp_packet(	peer, packet->data, packet->header.data_size,
									packet->header.msg_id,
									packet->header.msg_frag_ix, packet->header.msg_frag_count);
			}

			// Remove dropped packet
			peer->send_buffer_state[i] = UdpPacketState_free;
			peer->packet_id_to_send_buffer_ix[packet->header.packet_id] = NULL_HANDLE;
		}
	}
}
