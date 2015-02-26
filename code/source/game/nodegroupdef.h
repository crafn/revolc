#ifndef REVOLC_GAME_NODEGROUPDEF_H
#define REVOLC_GAME_NODEGROUPDEF_H

#include "build.h"

#define MAX_NODES_IN_GROUP_DEF 16
#define MAX_DEFAULT_STRUCT_SIZE 64
#define MAX_OUTPUTS_IN_GROUP_DEF 8

typedef struct NodeGroupDef {
	Resource res;
	// Allocate separately if sizes grow much
	struct {
		char type_name[RES_NAME_SIZE];
		char name[RES_NAME_SIZE];

		// Bytes of default node struct which can be resurrected
		char default_struct[MAX_DEFAULT_STRUCT_SIZE];
		U32 default_struct_size;
		struct {
			U32 src_offset;
			U32 src_size;

			U32 dst_node_i; // Index in `nodes`
			U32 dst_offset;
			U32 dst_size;
		} outputs[MAX_OUTPUTS_IN_GROUP_DEF];
		U32 output_count;

	} nodes[MAX_NODES_IN_GROUP_DEF];
	U32 node_count;
} NodeGroupDef;

REVOLC_API WARN_UNUSED
int json_nodegroupdef_to_blob(struct BlobBuf *buf, JsonTok j);

#endif // REVOLC_GAME_NODEGROUPDEF_H
