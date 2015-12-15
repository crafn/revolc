#ifndef REVOLC_GAME_NODEGROUPDEF_H
#define REVOLC_GAME_NODEGROUPDEF_H

#include "build.h"
#include "resources/resource.h"
#include "core/json.h"
#include "core/basic.h"

typedef enum {
	CmdType_memcpy,
	CmdType_call,
} CmdType;

typedef struct NodeGroupDef_Cmd {
	char str[MAX_CMD_STR_SIZE];

	CmdType type;

	bool has_condition;
	U32 cond_node_i;
	U32 cond_offset;
	U32 cond_size;

	union {
		struct { // memcpy
			U32 src_offset;
			U32 src_node_i; // Index to `nodes in NodeGroupDef`
			U32 dst_offset;
			U32 dst_node_i;
			U32 size;
		};
		struct { // call
			char func_name[MAX_FUNC_NAME_SIZE];
			void *fptr; // Cached

			U32 p_node_i[MAX_CMD_CALL_PARAMS];
			U32 p_count;
		};
	};
} PACKED NodeGroupDef_Cmd;

typedef struct NodeGroupDef_Node_Defaults {
	char dst[RES_NAME_SIZE];
	char src[RES_NAME_SIZE];
} PACKED NodeGroupDef_Node_Defaults;

typedef struct NodeGroupDef_Node {
	char type_name[RES_NAME_SIZE];
	char name[RES_NAME_SIZE];

	NodeGroupDef_Node_Defaults defaults[MAX_NODE_DEFAULTS];
	U32 defaults_count;

	// Would like to write these to blob, but that would require modules
	// to be loaded at make_blob...
	// Bytes of default node struct, which can be resurrected
	U8* default_struct;
	// 1 if corresponding byte is set in default_struct
	U8* default_struct_set_bytes;
	U32 default_struct_size;

} PACKED NodeGroupDef_Node;

typedef struct NodeGroupDef {
	Resource res;
	// @todo Allocate separately if sizes grow much
	NodeGroupDef_Node nodes[MAX_NODES_IN_GROUP_DEF];
	U32 node_count;

	// Cmds of different types are in the same array, as the order of
	// performing cmds matter. Consider e.g. "a = b", "copy(b, a)"
	NodeGroupDef_Cmd cmds[MAX_CMDS_IN_GROUP_DEF];
	U32 cmd_count;
} PACKED NodeGroupDef;

REVOLC_API void init_nodegroupdef(NodeGroupDef *def);
REVOLC_API void deinit_nodegroupdef(NodeGroupDef *def);

REVOLC_API WARN_UNUSED
int json_nodegroupdef_to_blob(struct BlobBuf *buf, JsonTok j);

#endif // REVOLC_GAME_NODEGROUPDEF_H
