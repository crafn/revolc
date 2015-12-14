#ifndef REVOLC_GAME_WORLD_H
#define REVOLC_GAME_WORLD_H

#include "build.h"
#include "core/archive.h"
#include "core/hashtable.h"
#include "core/math.h"
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

// @todo Simplify:
// - conditions -> bool enabled;
typedef struct SlotCmd {
	CmdType type;

	bool has_condition;
	U32 cond_node_h;
	U16 cond_offset;
	U16 cond_size;

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
	SlotCmd cmds[MAX_NODE_CMD_COUNT]; // @todo Cmds out of nodes, to a single array
	char type_name[RES_NAME_SIZE];
	NodeType *type; // @todo Resource id
	Id node_id; // Unique id
	Id group_id; // Entity id
	Handle impl_handle; // e.g. Handle to ModelEntity
	Handle cmd_count;
	bool allocated; /// @todo Can be substituted by type ( == NULL)
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
	Id next_entity_id;

	// @todo Nodes don't need to be limited to game world
	NodeInfo nodes[MAX_NODE_COUNT];
	Handle next_node;
	U32 node_count;
	Id next_node_id;

	Id_Handle_Tbl id_to_handle;

	// Storages for node types specified with auto_impl_mgmt
	AutoNodeImplStorage *auto_storages;
	U32 auto_storage_count;

	NodeInfo sort_space[MAX_NODE_COUNT];
} World;

REVOLC_API WARN_UNUSED World * create_world();
REVOLC_API void destroy_world(World *w);
REVOLC_API void clear_world_nodes(World *w);

REVOLC_API void upd_world(World *w, F64 dt);

REVOLC_API void save_world(WArchive *ar, World *w);
REVOLC_API void load_world(RArchive *ar, World *w);

REVOLC_API void save_world_delta(WArchive *ar, World *w, RArchive *base_ar);
REVOLC_API void load_world_delta(RArchive *ar, World *w, RArchive *base_ar);

REVOLC_API void create_nodes(	World *w,
								const NodeGroupDef *def,
								const SlotVal *init_vals, U32 init_vals_count,
								U64 group_id);
REVOLC_API void free_node(World *w, U32 handle);
REVOLC_API void free_node_group(World *w, U64 group_id);
REVOLC_API void remove_node_group(World *w, void *node_impl_in_group);
REVOLC_API U32 node_impl_handle(World *w, U32 node_handle);

REVOLC_API Id node_handle_to_id(World *w, Handle handle);
REVOLC_API Handle node_id_to_handle(World *w, Id id);

// Not intended to be widely used
REVOLC_API void * node_impl(World *w, U32 *size, NodeInfo *node);
REVOLC_API void resurrect_node_impl(World *w, NodeInfo *n, void *dead_impl_bytes);
REVOLC_API U32 alloc_node_without_impl(World *w, NodeType *n, U64 node_id, U64 group_id);

void world_on_res_reload(struct ResBlob *old);

#endif // REVOLC_GAME_WORLD_H
