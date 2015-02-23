#ifndef REVOLC_GAME_WORLD_H
#define REVOLC_GAME_WORLD_H

#include "build.h"
#include "core/vector.h"
#include "global/cfg.h"
#include "visual/entity_model.h"

typedef struct {
	V3d pos;
} T3d;

typedef enum {
	NodeType_ModelEntity,
	NodeType_T3d
} NodeType;

typedef struct {
	U8 allocated;
	U8 src_offset;
	U8 dst_offset;
	U8 size;
	U32 dst_node;
} SlotRouting;

typedef struct {
	bool allocated;
	NodeType type;
	U32 impl_handle; /// e.g. Handle to ModelEntity

	SlotRouting routing[MAX_NODE_ROUTING_COUNT];
} NodeInfo;

typedef struct World {
	NodeInfo nodes[MAX_NODE_COUNT];
	U32 next_node;
	U32 node_count;

	F64 time;
} World;


REVOLC_API
void upd_t3d_nodes(	World *w,
					T3d *t,
					U32 count);

REVOLC_API
void upd_modelentity_nodes(	World *w,
							ModelEntity *e,
							U32 count);

REVOLC_API void upd_world(World *w, F64 dt);

REVOLC_API U32 alloc_node(World *w, NodeType type);
REVOLC_API void free_node(World *w, U32 handle);
REVOLC_API U32 node_impl_handle(World *w, U32 node_handle);
REVOLC_API void add_routing(World *w,
							U32 dst_node_h, U32 dst_offset,
							U32 src_node_h, U32 src_offset,
							U32 size);

#endif // REVOLC_GAME_WORLD_H
