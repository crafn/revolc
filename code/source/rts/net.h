#ifndef RTS_NET_H
#define RTS_NET_H

#include "build.h"

typedef enum RtsMsg {
	RtsMsg_chat = 1,
	RtsMsg_base,
	RtsMsg_delta,
	// Debug
	RtsMsg_brush_action,
	RtsMsg_spawn_action,
} RtsMsg;

typedef struct RtsMsgHeader {
	RtsMsg type;
	//F64 time; // Game time
} PACKED RtsMsgHeader;

typedef struct BrushAction { // @todo Range and precision "attributes"
	V2d pos;
	F64 size;
	U8 material;
} BrushAction;

typedef struct SpawnAction {
	char name[RES_NAME_SIZE];
	V2d pos;
} SpawnAction;


MOD_API void send_rts_msg(RtsMsg type, void *data, U32 data_size);
MOD_API void local_brush_action(BrushAction *action);
MOD_API void brush_action(BrushAction *action);
MOD_API void local_spawn_action(SpawnAction *action);
MOD_API void spawn_action(SpawnAction *action);

MOD_API void upd_rts_net();

// Internal
MOD_API void make_and_save_base();
MOD_API void resurrect_and_save_base(RArchive *ar);
MOD_API void make_world_delta(WArchive *ar);
MOD_API void resurrect_world_delta(RArchive *ar);

#endif // RTS_NET_H
