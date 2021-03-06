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

typedef struct NodeCmd_Memcpy {
	U16 src_offset;
	U16 dst_offset;
	U16 size;
	Handle src_node;
	Handle dst_node;
} NodeCmd_Memcpy;

typedef struct NodeCmd_Call {
	void *fptr;

	Handle p_nodes[MAX_CMD_CALL_PARAMS];
	U16 p_node_count;
} NodeCmd_Call;

// @todo Simplify:
// - conditions -> bool enabled;
// - memcpy -> call
// - calls to be non-batched (flexibility)
// @todo Value params to calls
typedef struct NodeCmd {
	bool allocated;
	CmdType type;
	Id cmd_id;

	bool has_condition;
	U32 cond_node_h;
	U16 cond_offset;
	U16 cond_size;

	NodeCmd_Memcpy memcpy;
	NodeCmd_Call call;

	bool selected; // Editor
} NodeCmd;

typedef struct NodeInfo {
	char type_name[RES_NAME_SIZE];
	NodeType *type; // @todo Resource id
	Id node_id; // Unique id
	Id group_id; // Entity id
	Handle impl_handle; // e.g. Handle to ModelEntity
	Handle assoc_cmds[MAX_NODE_ASSOC_CMD_COUNT];
	U8 peer_id;
	bool allocated; /// @todo Can be substituted by type ( == NULL)
	bool remove;

	// Editor stuff
	char group_def_name[RES_NAME_SIZE];
	U8 node_ix_in_group;
	bool selected;
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
	F64 dt;
	Id next_entity_id; // Increase when calling create_nodes (if you want unique group ids)

	NodeInfo nodes[MAX_NODE_COUNT];
	Handle next_node;
	U32 node_count;
	Id next_node_id;

	NodeCmd cmds[MAX_NODE_CMD_COUNT];
	Handle next_cmd;
	U32 cmd_count;
	Id next_cmd_id;

	HashTbl(Id, Handle) node_id_to_handle;
	HashTbl(Id, Handle) cmd_id_to_handle;

	// Storages for node types specified with auto_impl_mgmt
	AutoNodeImplStorage *auto_storages;
	U32 auto_storage_count;

	NodeInfo sort_space[MAX_NODE_COUNT];

	bool editor_disable_memcpy_cmds;
} World;

REVOLC_API WARN_UNUSED World * create_world();
REVOLC_API void destroy_world(World *w);
REVOLC_API void clear_world_nodes(World *w);

REVOLC_API void upd_world(World *w, F64 dt);

REVOLC_API void save_world(WArchive *ar, World *w);
REVOLC_API void load_world(RArchive *ar, World *w);

REVOLC_API void save_world_to_file(World *w, const char *path);
REVOLC_API void load_world_from_file(World *w, const char *path);

REVOLC_API void save_world_delta(WArchive *ar, World *w, RArchive *base_ar);
REVOLC_API void load_world_delta(RArchive *ar, World *w, RArchive *base_ar, U8 ignore_peer_id);

REVOLC_API void save_single_node(WArchive *ar, World *w, Handle handle);
REVOLC_API void load_single_node(RArchive *ar, World *w);

REVOLC_API void create_nodes(	World *w,
								const NodeGroupDef *def,
								const SlotVal *init_vals, U32 init_vals_count,
								U64 group_id, U8 peer_id);
REVOLC_API void free_node(World *w, U32 handle);
REVOLC_API void free_node_group(World *w, U64 group_id);
REVOLC_API void remove_node_group(World *w, void *node_impl_in_group);
REVOLC_API U32 node_impl_handle(World *w, U32 node_handle);

REVOLC_API Id node_handle_to_id(World *w, Handle handle);
REVOLC_API Handle node_id_to_handle(World *w, Id id);

REVOLC_API Id cmd_handle_to_id(World *w, Handle handle);
REVOLC_API Handle cmd_id_to_handle(World *w, Id id);

REVOLC_API U32 resurrect_cmd(World *w, NodeCmd cmd);
REVOLC_API void free_cmd(World *w, U32 handle);

// Not intended to be widely used
REVOLC_API void * node_impl(World *w, U32 *size, NodeInfo *node);
REVOLC_API void resurrect_node_impl(World *w, NodeInfo *n, void *dead_impl_bytes);
REVOLC_API U32 alloc_node_without_impl(World *w, NodeType *n, U64 node_id, U64 group_id, U8 peer_id,
										const char *group_def_name, U8 node_ix_in_group);

void world_on_res_reload(struct ResBlob *old);

#endif // REVOLC_GAME_WORLD_H
