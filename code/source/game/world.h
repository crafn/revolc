#ifndef REVOLC_GAME_WORLD_H
#define REVOLC_GAME_WORLD_H

#include "build.h"
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
	bool allocated; /// @todo Can be substituted by type (== NULL)
	NodeType *type;
	char type_name[RES_NAME_SIZE];
	U32 impl_handle; // e.g. Handle to ModelEntity
	SlotCmd cmds[MAX_NODE_CMD_COUNT];
	U32 cmd_count;
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

REVOLC_API void create_nodes(	World *w,
								const NodeGroupDef *def,
								const SlotVal *init_vals, U32 init_vals_count,
								U64 group_id);
REVOLC_API void free_node(World *w, U32 handle);
REVOLC_API void free_node_group(World *w, U64 group_id);
REVOLC_API U32 node_impl_handle(World *w, U32 node_handle);

struct ResBlob;
internal
void world_on_res_reload(struct ResBlob* blob);

#endif // REVOLC_GAME_WORLD_H
