#ifndef REVOLC_GAME_NODEGROUPDEF_H
#define REVOLC_GAME_NODEGROUPDEF_H

#include "build.h"
#include "resources/resource.h"

#define MAX_NODES_IN_GROUP_DEF 16
#define MAX_DEFAULT_STRUCT_SIZE 512
#define MAX_OUTPUTS_IN_GROUP_DEF 8

typedef struct NodeGroupDef_Node_Output {
	U32 src_offset;
	U32 dst_offset;
	U32 dst_node_i; // Index to `nodes in NodeGroupDef`
	U32 size;
} NodeGroupDef_Node_Output;

typedef struct NodeGroupDef_Node {
	char type_name[RES_NAME_SIZE];
	char name[RES_NAME_SIZE];

	// Bytes of default node struct, which can be resurrected
	U8 default_struct[MAX_DEFAULT_STRUCT_SIZE];
	// 1 if corresponding byte is set in default_struct
	U8 default_struct_set_bytes[MAX_DEFAULT_STRUCT_SIZE];
	U32 default_struct_size;

	NodeGroupDef_Node_Output outputs[MAX_OUTPUTS_IN_GROUP_DEF];
	U32 output_count;

} NodeGroupDef_Node;

typedef struct NodeGroupDef {
	Resource res;
	// Allocate separately if sizes grow much
	NodeGroupDef_Node nodes[MAX_NODES_IN_GROUP_DEF];
	U32 node_count;
} NodeGroupDef;

REVOLC_API WARN_UNUSED
int json_nodegroupdef_to_blob(struct BlobBuf *buf, JsonTok j);

#endif // REVOLC_GAME_NODEGROUPDEF_H
