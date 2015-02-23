#ifndef REVOLC_GAME_WORLD_H
#define REVOLC_GAME_WORLD_H

#include "build.h"
#include "core/vector.h"
#include "global/cfg.h"
#include "visual/entity_model.h"

typedef struct {
	V2d pos;
	F64 rot;
} Transform2Node;

typedef enum {
	NodeType_ModelEntity,
	NodeType_Transform2
} NodeType;

typedef struct {
	bool allocated;
	NodeType type;
	U32 impl_handle; /// e.g. Handle to ModelEntity
} NodeInfo;

typedef struct World {
	NodeInfo nodes[MAX_NODE_COUNT];
	U32 next_node;
	U32 node_count;

	F64 time;
} World;

void upd_modelentity_nodes(	World *w,
							ModelEntity *node,
							U32 count);

void upd_world(World *w, F64 dt);

REVOLC_API U32 alloc_node(World *w, NodeType type);
REVOLC_API void free_node(World *w, U32 handle);
REVOLC_API U32 node_impl_handle(World *w, U32 node_handle);

#endif // REVOLC_GAME_WORLD_H
