#ifndef REVOLC_CORE_UDP_H
#define REVOLC_CORE_UDP_H

#include "build.h"
#include "core/hashtable.h"
#include "core/socket.h"
#include "global/cfg.h"

// A lightweight low-level protocol on top of UDP for message passing
// @todo Metrics (rtt, drop rate, throughput)
// @todo Maximum bandwidth usage limit + packet count
// @todo Unreliable messages
//
// Things I dont care about:
//  - endianness -- only supporting little endian platforms
//  - different floating point binary formats -- only supporting x64 platforms
//
// Things I care about after working LAN networking:
// @todo Drop duplicate messages (but acknowledge the packets)
// @todo Drop too old incoming messages (to prevent id mixup)
// @todo Greeting packet
// @todo Congestion avoidance
// @todo Priority settings (latency, throughput, reliability)
// @todo Compression (arithmetic coding)
// @todo Warnings when buffers are too full (packet ids might get wrongly interpreted)
// @todo Threading (fps dependency is not nice)
// @todo Random corruption test -> proper validating

typedef enum UdpPacketState {
	UdpPacketState_free,
	UdpPacketState_buffered,
	UdpPacketState_waiting_ack,
} UdpPacketState;

// If an important packet gets lost, it will be buffered with a new
// packet_id, but possibly with the same msg and frag id.
typedef struct UdpPacketHeader {
	U8 packet_id;
	U32 msg_id; // Could use smaller type and wrapping
	U16 msg_frag_count; // Single msg can be divided to multiple packets
	U16 msg_frag_ix; 
	U16 data_size;
	bool acked;
	U8 ack_id;
	U64 previous_acks;
} PACKED UdpPacketHeader;

typedef struct UdpPacket {
	UdpPacketHeader header;
	U8 data[UDP_MAX_PACKET_DATA_SIZE];
} PACKED UdpPacket;

// Local peer possibly connected to somebody else
typedef struct UdpPeer {
	Socket socket;

	F64 last_send_time;
	F64 last_recv_time;
	bool connected;
	IpAddress remote_addr;
	U16 local_port;
	F64 rtt; // Round-trip time

	U32 next_msg_id; // Non-wrapping multi-packet message
	U8 next_packet_id; // Wrapping packet id

	UdpPacketState send_buffer_state[UDP_MAX_BUFFERED_PACKET_COUNT];
	UdpPacket send_buffer[UDP_MAX_BUFFERED_PACKET_COUNT];
	F64 send_times[UDP_PACKET_ID_COUNT];
	U32 packet_id_to_send_buffer_ix[UDP_PACKET_ID_COUNT];
	U32 packets_waiting_send_count;
	U32 sent_packet_count;
	U32 acked_packet_count;
	U32 drop_count;
	HashTbl(U32, U32) sent_msg_acks; // For determining if sent msg was received

	U32 recv_packet_count;
	U32 recv_msg_count;
	U8 remote_packet_id; // Largest received remote packet id (wrapping)
	U32 prev_out_acks; // Bitfield relative to remote_packet_id
	U8 last_sent_ack_id; // Most recent remote_packet_id sent
	U32 cur_incomplete_recv_msg_count;

	// Contains packets of incomplete messages
	UdpPacket recv_buffer[UDP_MAX_BUFFERED_PACKET_COUNT];

	// For incoming packets
	F32 simulated_packet_loss;
} UdpPeer;

// Read-only convenience struct for receiving messages.
// Don't store permanently, will be freed automatically
typedef struct UdpMsg {
	U32 data_size;
	void *data;
} UdpMsg;

// If remote_addr == NULL we just wait for incoming connections.
REVOLC_API UdpPeer *create_udp_peer(U16 local_port, IpAddress *remote_addr);
REVOLC_API void destroy_udp_peer(UdpPeer *peer);

typedef struct SentMsgInfo {
	U32 msg_id;
	U32 msg_size;
} SentMsgInfo;

// Returns message id
REVOLC_API SentMsgInfo buffer_udp_msg(UdpPeer *peer, const void *data, U32 size);
REVOLC_API void upd_udp_peer(	UdpPeer *peer,
								UdpMsg **msgs, U32 *msg_count,
								U32 **acked_msgs, U32 *acked_msg_count);

#endif // REVOLC_CORE_UDP
