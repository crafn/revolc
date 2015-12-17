#ifndef REVOLC_GAME_NET_H
#define REVOLC_GAME_NET_H

#include "build.h"
#include "core/basic.h"
#include "core/array.h"
#include "core/udp.h"

// World snapshot to which deltas are relative
typedef struct WorldBaseState {
	U32 seq;
	U8 *data ALIGNED(MAX_ALIGNMENT);
	U32 size;
	U32 capacity;

	bool peer_has_this;
} WorldBaseState;

DECLARE_ARRAY(WorldBaseState)

#define NULL_PEER ((U8)-1)
#define AUTHORITY_PEER 0

// Client/server state
typedef struct NetState {
	UdpPeer *peer;
	bool authority; // Do we have authority over the game world
	U8 peer_id; // server = 0, clients = 1, ..

	F64 game_time; // Increasing monotonically in real time
	F64 delta_interval;

	bool peer_has_received_base;

	F64 world_upd_time; // Send/recv time
	F64 stats_timer;
	U32 world_seq; // Incremented at every delta
	Array(WorldBaseState) bases;
	U32 cur_base_ix;
} NetState;

REVOLC_API NetState *create_netstate(	bool authority, F64 delta_interval,
										U32 state_history_count, U32 state_max_size,
										U16 local_port, IpAddress *remote_addr);
REVOLC_API void destroy_netstate(NetState *net);
REVOLC_API void upd_netstate(NetState *net);

// Messaging

typedef enum NetMsg {
	NetMsg_chat = 1,
	NetMsg_client_init,
	NetMsg_base,
	NetMsg_delta,
	NetMsg_world_seq_confirm, // Client sends to server, so server knows which base it can use
	// Debug
	NetMsg_brush_action,
	NetMsg_spawn_action,
} NetMsg;

typedef struct NetMsgHeader {
	NetMsg type;
	//F64 time; // Game time
} PACKED NetMsgHeader;

typedef struct ClientInit {
	U8 peer_id;
} ClientInit;

typedef struct BrushAction { // @todo Range and precision "attributes"
	V2d pos;
	F64 size;
	U8 material;
} BrushAction;

typedef struct SpawnAction {
	char name[RES_NAME_SIZE];
	V2d pos;
} SpawnAction;


REVOLC_API U32 send_net_msg(NetMsg type, void *data, U32 data_size);
REVOLC_API void local_brush_action(BrushAction *action);
REVOLC_API void brush_action(BrushAction *action);
REVOLC_API void local_spawn_action(SpawnAction *action);
REVOLC_API void spawn_action(SpawnAction *action);

#endif
