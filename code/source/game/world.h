#ifndef REVOLC_GAME_WORLD_H
#define REVOLC_GAME_WORLD_H

#include "build.h"
#include "core/archive.h"
#include "core/quaternion.h"
#include "core/vector.h"
#include "global/cfg.h"
#include "nodegroupdef.h"
#include "nodetype.h"
#include "visual/modelentity.h"

// Used to supply default values to nodes when instantiating
typedef struct SlotVal {
	const char *node_name;
	const char *member_name;
	const void *data;
	U32 size;
} SlotVal;

typedef struct SlotCmd {
	CmdType type;

	bool has_condition;
	U32 cond_node_h;
	U32 cond_offset;
	U32 cond_size;

	union {
		struct { // memcpy
			U16 src_offset;
			U16 dst_offset;
			U16 size;
			U32 src_node;
		};
		struct { // call
			void *fptr;

			U32 p_nodes[MAX_CMD_CALL_PARAMS];
			U16 p_node_count;
		};
	};

} SlotCmd;

typedef struct NodeInfo {
	SlotCmd cmds[MAX_NODE_CMD_COUNT];
	char type_name[RES_NAME_SIZE];
	NodeType *type; // @todo Resource id
	U64 node_id; // Unique id
	U64 group_id; // Entity id
	U32 impl_handle; // e.g. Handle to ModelEntity
	U32 cmd_count;
	bool allocated; /// @todo Can be substituted by type (== NULL)
	bool remove;
} NodeInfo;

typedef struct AutoNodeImplStorage {
	void *storage;
	bool *allocated;
	U32 count;
	U32 next;
	U32 max_count;
	U32 size;
} AutoNodeImplStorage;

typedef struct World {
	F64 time;
	F64 dt;
	U64 next_entity_id;

	// @todo Nodes don't need to be limited to game world
	NodeInfo nodes[MAX_NODE_COUNT];
	U32 next_node;
	U32 node_count;
	U64 next_node_id;

	// Storages for node types specified with auto_impl_mgmt
	AutoNodeImplStorage *auto_storages;
	U32 auto_storage_count;

	NodeInfo sort_space[MAX_NODE_COUNT];
} World;

REVOLC_API WARN_UNUSED World * create_world();
REVOLC_API void destroy_world(World *w);

REVOLC_API void upd_world(World *w, F64 dt);

REVOLC_API void save_world(World *w, WArchive *ar);
REVOLC_API void load_world(World *w, RArchive *ar);

REVOLC_API void create_nodes(	World *w,
								const NodeGroupDef *def,
								const SlotVal *init_vals, U32 init_vals_count,
								U64 group_id);
REVOLC_API void free_node(World *w, U32 handle);
REVOLC_API void free_node_group(World *w, U64 group_id);
REVOLC_API void remove_node_group(World *w, void *node_impl_in_group);
REVOLC_API U32 node_impl_handle(World *w, U32 node_handle);

// Not intended to be widely used
REVOLC_API void * node_impl(World *w, U32 *size, NodeInfo *node);
REVOLC_API void resurrect_node_impl(World *w, NodeInfo *n, void *dead_impl_bytes);
REVOLC_API U32 alloc_node_without_impl(World *w, NodeType *type, U64 node_id, U64 group_id);

void world_on_res_reload(struct ResBlob *old);

#endif // REVOLC_GAME_WORLD_H
