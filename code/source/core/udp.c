#include "udp.h"

UdpPeer create_udp_peer(U16 local_port, U16 remote_port)
{
	UdpPeer peer = {
		.socket= open_udp_socket(local_port),
		.send_port= remote_port,
		.rtt= 1.0, // Better to be too large than too small at the startup
	};
	if (peer.socket == invalid_socket())
		fail("Socket creation failed");
	return peer;
}

void destroy_udp_peer(UdpPeer *peer)
{
	close_socket(&peer->socket);
}

void buffer_udp_msg(UdpPeer *peer, const void *data, U32 size)
{
	ensure(size < UDP_MAX_PACKET_DATA_SIZE && "Too large message");

	U32 free_i= 0;
	while (peer->send_buffer_filled[free_i] && free_i < UDP_MAX_BUFFERED_PACKET_COUNT)
		++free_i;

	if (free_i >= UDP_MAX_BUFFERED_PACKET_COUNT)
		fail("Too many packets queued to send buffer, max %i", UDP_MAX_BUFFERED_PACKET_COUNT);

	UdpPacket packet= {};
	memcpy(packet.data, data, size);

	packet.header= (UdpPacketHeader) {
		.packet_id= peer->next_packet_id++,
		.msg_id= peer->next_msg_id++,
		.msg_frag_count= 1,
		.msg_frag_i= 0,
		.data_size= size,
		.acked= peer->recv_packet_count > 0,
		.ack_id= peer->remote_packet_id,
		.previous_acks= peer->prev_out_acks,
	};

	peer->send_buffer[free_i]= packet;
	peer->send_buffer_filled[free_i]= true;
}

REVOLC_API void upd_udp_peer(UdpPeer *peer)
{
	{ // Send buffered packets
		IpAddress addr= {
			127, 0, 0, 1,
			peer->send_port
		};
		for (U32 i= 0; i < UDP_MAX_BUFFERED_PACKET_COUNT; ++i) {
			if (!peer->send_buffer_filled[i])
				continue;
			UdpPacket *packet= &peer->send_buffer[i];
			UdpPacketHeader *header= &packet->header;
			U32 bytes= send_packet(	peer->socket, addr,
									packet, sizeof(*header) + header->data_size);
			if (bytes == 0)
				critical_print("Packet lost at send: %i", packet->header.packet_id);
			else if (bytes != header->data_size + sizeof(*header))
				fail("Udp packet with incorrect size sent");
			peer->send_buffer_filled[i]= false;
			peer->last_send_time= g_env.time_from_start;
			peer->send_times[packet->header.packet_id]= g_env.time_from_start;
		}
	}

	// Recv packets
	{
		IpAddress addr;
		UdpPacket packet;
		ensure(sizeof(packet) == UDP_MAX_PACKET_SIZE);
		bool packet_received= false;
		U32 bytes= 0;
		while (	(bytes= recv_packet(peer->socket, &addr, &packet, UDP_MAX_PACKET_SIZE))
				> 0) {
			if (peer->connected && !ip_equals(peer->connected_ip, addr))
				continue; // Discard packets from others when connected

			if (	packet.header.data_size >= UDP_MAX_PACKET_DATA_SIZE ||
					packet.header.data_size != bytes - sizeof(UdpPacketHeader))
				fail("Corrupted UDP packet");

			packet_received= true;

			if (!peer->connected) {
				peer->connected= true;
				peer->connected_ip= addr;
				debug_print("Connected to %s", ip_str(peer->connected_ip));
			}

			peer->last_recv_time= g_env.time_from_start;
			++peer->recv_packet_count;
		
			U32 packet_id= packet.header.packet_id;
			// Update next acks
			if (packet_id > peer->remote_packet_id) {
				U32 shift= packet_id - peer->remote_packet_id;
				peer->prev_out_acks= peer->prev_out_acks << shift;
				peer->prev_out_acks |= 1 << (shift - 1);

				peer->remote_packet_id= packet_id;
			} else if (packet_id < peer->remote_packet_id) {
				U32 shift= peer->remote_packet_id - packet_id;
				peer->prev_out_acks |= 1 << (shift - 1);
			}

			// Check which sent packets were acked
			if (packet.header.acked) {
				U8 ack= packet.header.ack_id;

				F64 rtt_sample= g_env.time_from_start - peer->send_times[ack];
				const F64 change_p= 0.1;
				peer->rtt = peer->rtt*(1 - change_p) + rtt_sample*change_p;

				// @todo Read previous_acks for redundant acking
			}

			//debug_print("data: %.*s", packet.header.data_size, packet.data);
			//debug_print("rtt: %f", peer->rtt);
		}

		if (	!packet_received && peer->connected &&
				peer->last_recv_time + UDP_CONNECTION_TIMEOUT < g_env.time_from_start) {
			peer->connected= false;
			debug_print("Disconnected from %s", ip_str(peer->connected_ip));
		}
	}
}
