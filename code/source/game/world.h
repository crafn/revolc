#ifndef REVOLC_GAME_WORLD_H
#define REVOLC_GAME_WORLD_H

#include "build.h"
#include "core/quaternion.h"
#include "core/vector.h"
#include "global/cfg.h"
#include "visual/modelentity.h"

typedef enum {
	NodeTypeId_ModelEntity,
	NodeTypeId_AiTest,
	NodeTypeId_RigidBody,
} NodeTypeId;

typedef struct SlotRouting {
	U8 allocated;
	U8 src_offset;
	U8 dst_offset;
	U8 size;
	U32 dst_node;
} SlotRouting;

typedef struct NodeInfo {
	bool allocated;
	NodeTypeId type;
	U32 impl_handle; /// e.g. Handle to ModelEntity

	SlotRouting routing[MAX_NODE_ROUTING_COUNT];
} NodeInfo;

typedef struct World {
	NodeInfo nodes[MAX_NODE_COUNT];
	U32 next_node;
	U32 node_count;

	F64 time;
} World;

REVOLC_API WARN_UNUSED World * create_world();
REVOLC_API void destroy_world(World *w);

REVOLC_API void upd_world(World *w, F64 dt);

REVOLC_API void load_world(World *w, const char *path);
REVOLC_API void save_world(World *w, const char *path);

REVOLC_API U32 alloc_node(World *w, NodeTypeId type);
REVOLC_API void free_node(World *w, U32 handle);
REVOLC_API U32 node_impl_handle(World *w, U32 node_handle);
REVOLC_API void add_routing(World *w,
							U32 src_node_h, U32 src_offset,
							U32 dst_node_h, U32 dst_offset,
							U32 size);

#endif // REVOLC_GAME_WORLD_H
