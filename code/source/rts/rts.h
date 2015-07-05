#ifndef REVOLC_RTS_RTS_H
#define REVOLC_RTS_RTS_H

#include "build.h"

#define RTS_AUTHORITY_PORT 19995
#define RTS_CLIENT_PORT 19996
#define RTS_SNAPSHOT_INTERVAL 5.0
#define RTS_WORLD_STATE_SIZE (1024*1024*10)

typedef struct RtsEnv {
	UdpPeer *peer;
	bool authority; // Do we have authority over the game world
	F64 game_time; // Same at client and server

	F64 snapshot_time;
	U8 world_state[RTS_WORLD_STATE_SIZE];
} RtsEnv;

MOD_API RtsEnv *rts_env();


#endif // REVOLC_RTS_RTS_H
