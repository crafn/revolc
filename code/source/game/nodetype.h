#ifndef REVOLC_GAME_NODETYPE_H
#define REVOLC_GAME_NODETYPE_H

#include "build.h"
#include "resources/resource.h"

struct World;
typedef U32 (*AllocNodeImpl)();
typedef void (*FreeNodeImpl)(U32 handle);
typedef void * (*StorageNodeImpl)();
typedef void (*UpdNodeImpl)(struct World *,
							void *,
							U32 count);
typedef void (*ResurrectNodeImpl)(U32 h, void *dead);

#define MAX_FUNC_NAME_SIZE 64

typedef struct NodeType {
	Resource res;

	char alloc_func_name[MAX_FUNC_NAME_SIZE];
	char free_func_name[MAX_FUNC_NAME_SIZE];
	char upd_func_name[MAX_FUNC_NAME_SIZE];
	char storage_func_name[MAX_FUNC_NAME_SIZE];
	char resurrect_func_name[MAX_FUNC_NAME_SIZE];

	// Cached
	AllocNodeImpl alloc;
	FreeNodeImpl free;
	StorageNodeImpl storage;
	UpdNodeImpl upd;
	ResurrectNodeImpl resurrect;
	U32 size;
} NodeType;

REVOLC_API void init_nodetype(NodeType *node);

REVOLC_API WARN_UNUSED
int json_nodetype_to_blob(struct BlobBuf *buf, JsonTok j);

#endif // REVOLC_GAME_NODETYPE_H
