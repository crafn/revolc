#ifndef REVOLC_GAME_NODETYPE_H
#define REVOLC_GAME_NODETYPE_H

#include "build.h"
#include "resources/resource.h"

struct World;
typedef void (*InitNodeImpl)(void *data);
typedef U32 (*ResurrectNodeImpl)(void *dead);
typedef void (*FreeNodeImpl)(U32 handle);
typedef void * (*StorageNodeImpl)();
typedef void (*UpdNodeImpl)(void *,
							U32 count);

typedef struct NodeType {
	Resource res;

	char init_func_name[MAX_FUNC_NAME_SIZE];
	char resurrect_func_name[MAX_FUNC_NAME_SIZE];
	char upd_func_name[MAX_FUNC_NAME_SIZE];

	char free_func_name[MAX_FUNC_NAME_SIZE];
	char storage_func_name[MAX_FUNC_NAME_SIZE];

	// If true, handles & instances are managed by the node system.
	// Also, free and storage funcs are NULL, and resurrect
	// works in-place, returning NULL_HANDLE.
	bool auto_inst_mgmt;

	// Cached
	InitNodeImpl init;
	ResurrectNodeImpl resurrect;
	UpdNodeImpl upd;
	FreeNodeImpl free;
	StorageNodeImpl storage;
	U32 size;
} NodeType;

REVOLC_API void init_nodetype(NodeType *node);

REVOLC_API WARN_UNUSED
int json_nodetype_to_blob(struct BlobBuf *buf, JsonTok j);

#endif // REVOLC_GAME_NODETYPE_H
