#ifndef REVOLC_RTS_RTS_H
#define REVOLC_RTS_RTS_H

#include "build.h"

#define RTS_AUTHORITY_PORT 19995
#define RTS_CLIENT_PORT 19996
#define RTS_BASE_SIZE (1024*1024*5)
#define RTS_MAX_BASE_HISTORY_COUNT 10 
#define RTS_DELTA_INTERVAL 0.1

typedef struct WorldBaseState {
	U32 seq;
	U8 data[RTS_BASE_SIZE] ALIGNED(MAX_ALIGNMENT);
	U32 size;
} WorldBaseState;

typedef struct RtsEnv {
	UdpPeer *peer;
	bool authority; // Do we have authority over the game world
	F64 game_time; // Same at client and server

	F64 world_upd_time; // Send/recv time
	F64 stats_timer;

	U32 world_seq; // Incremented at every delta
	WorldBaseState bases[RTS_MAX_BASE_HISTORY_COUNT];
	U32 cur_base_ix;
} RtsEnv;

MOD_API RtsEnv *rts_env();
MOD_API void upd_rts();

#endif // REVOLC_RTS_RTS_H
