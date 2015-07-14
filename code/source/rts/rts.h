#ifndef REVOLC_RTS_RTS_H
#define REVOLC_RTS_RTS_H

#include "build.h"

#define RTS_AUTHORITY_PORT 19995
#define RTS_CLIENT_PORT 19996
#define RTS_SNAPSHOT_INTERVAL 5.0
#define RTS_SNAPSHOT_SIZE (1024*1024*10)
#define RTS_DELTA_INTERVAL 0.3

typedef struct RtsClient {
} RtsClient;

typedef struct RtsServer {
} RtsServer;

typedef struct RtsEnv {
	UdpPeer *peer;
	bool authority; // Do we have authority over the game world
	F64 game_time; // Same at client and server

	F64 snapshot_time;
	U8 snapshot[RTS_SNAPSHOT_SIZE];
	U32 snapshot_id;
	U32 snapshot_size;
} RtsEnv;

MOD_API RtsEnv *rts_env();
MOD_API void upd_rts();

#endif // REVOLC_RTS_RTS_H
