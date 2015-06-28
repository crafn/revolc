#ifndef REVOLC_CORE_UDP_H
#define REVOLC_CORE_UDP_H

#include "build.h"
#include "core/socket.h"
#include "global/cfg.h"

// A lightweight low-level protocol on top of UDP for message passing

// @todo Message division to packets
// @todo Priority settings (latency, throughput, reliability)
// @todo Packet timeout
// @todo Heartbeat (essential for reliable rtt measurement through acks)
//
// Things I dont care about:
//  - endianness -- only supporting little endian platforms
//  - different floating point binary formats -- only supporting x64 platforms
//
// This I care about after working LAN networking:
// @todo Greeting packet
// @todo Congestion avoidance
// @todo Compression (arithmetic coding)
// @todo Redundant acking

// If an important packet gets lost, it will be buffered with a new
// packet_id, but possibly with the same msg and frag id.
typedef struct UdpPacketHeader {
	U8 packet_id;
	U32 msg_id; // Could use smaller type and wrapping
	U16 msg_frag_count; // Single msg can be divided to multiple packets
	U16 msg_frag_i; 
	U16 data_size;
	bool acked;
	U8 ack_id;
	U32 previous_acks;
} PACKED UdpPacketHeader;

typedef struct UdpPacket {
	UdpPacketHeader header;
	U8 data[UDP_MAX_PACKET_DATA_SIZE];
} PACKED UdpPacket;

// Local peer possibly connected to somebody else
typedef struct UdpPeer {
	Socket socket;
	U16 send_port;

	F64 last_send_time;
	F64 last_recv_time;
	bool connected;
	IpAddress connected_ip;
	F64 rtt; // Round-trip time

	U32 next_msg_id; // Non-wrapping multi-packet message
	U8 next_packet_id; // Wrapping packet id

	U32 recv_packet_count;
	U8 remote_packet_id; // Largest received remote packet id (wrapping)
	U32 prev_out_acks; // Bitfield relative to remote_packet_id

	bool send_buffer_filled[UDP_MAX_BUFFERED_PACKET_COUNT];
	UdpPacket send_buffer[UDP_MAX_BUFFERED_PACKET_COUNT];
	F64 send_times[UDP_PACKET_ID_COUNT];

} UdpPeer;


REVOLC_API UdpPeer create_udp_peer(U16 local_port, U16 remote_port);
REVOLC_API void destroy_udp_peer(UdpPeer *peer);

REVOLC_API void buffer_udp_msg(UdpPeer *peer, const void *data, U32 size);
REVOLC_API void upd_udp_peer(UdpPeer *peer);

#endif // REVOLC_CORE_UDP
