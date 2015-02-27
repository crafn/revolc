#ifndef REVOLC_GAME_WORLD_H
#define REVOLC_GAME_WORLD_H

#include "build.h"
#include "core/quaternion.h"
#include "core/vector.h"
#include "global/cfg.h"
#include "nodegroupdef.h"
#include "nodetype.h"
#include "visual/modelentity.h"

typedef struct SlotRouting {
	U8 allocated;
	U8 src_offset;
	U8 dst_offset;
	U8 size;
	U32 dst_node;
} SlotRouting;

typedef struct NodeInfo {
	bool allocated; /// @todo Can be substituted by type (== NULL)
	NodeType *type;
	char type_name[RES_NAME_SIZE];
	U32 impl_handle; // e.g. Handle to ModelEntity
	SlotRouting routing[MAX_NODE_ROUTING_COUNT];
	U64 group_id; // Entity id
} NodeInfo;

typedef struct World {
	NodeInfo nodes[MAX_NODE_COUNT];
	U32 next_node;
	U32 node_count;

	NodeInfo sort_space[MAX_NODE_COUNT];
} World;

REVOLC_API WARN_UNUSED World * create_world();
REVOLC_API void destroy_world(World *w);

REVOLC_API void upd_world(World *w, F64 dt);

REVOLC_API void load_world(World *w, const char *path);
REVOLC_API void save_world(World *w, const char *path);

REVOLC_API void create_nodes(World *w, const NodeGroupDef *def, U64 group_id);
REVOLC_API U32 alloc_node(World *w, NodeType *type, U64 group_id);
REVOLC_API void free_node(World *w, U32 handle);
REVOLC_API void free_node_group(World *w, U64 group_id);
REVOLC_API U32 node_impl_handle(World *w, U32 node_handle);
REVOLC_API void add_routing(World *w,
							U32 src_node_h, U32 src_offset,
							U32 dst_node_h, U32 dst_offset,
							U32 size);

struct ResBlob;
internal
void world_on_res_reload(struct ResBlob* blob);

#endif // REVOLC_GAME_WORLD_H
