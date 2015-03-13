#ifndef REVOLC_GAME_NODETYPE_H
#define REVOLC_GAME_NODETYPE_H

#include "build.h"
#include "resources/resource.h"

struct World;
struct Module;
typedef void (*InitNodeImpl)(void *data);
typedef U32 (*ResurrectNodeImpl)(void *dead);
typedef void (*FreeNodeImpl)(void *data);
typedef void * (*StorageNodeImpl)();
typedef void (*UpdNodeImpl)(void *begin,
							void *end);

typedef struct NodeType {
	Resource res;

	char init_func_name[MAX_FUNC_NAME_SIZE];
	char resurrect_func_name[MAX_FUNC_NAME_SIZE];
	char upd_func_name[MAX_FUNC_NAME_SIZE];

	char free_func_name[MAX_FUNC_NAME_SIZE];
	char storage_func_name[MAX_FUNC_NAME_SIZE];

	// If true, handles & impls are managed by the node system.
	// Also, free and storage funcs are NULL, and resurrect
	// works in-place, returning NULL_HANDLE.
	bool auto_impl_mgmt;
	U32 auto_impl_max_count;

	// Cached
	const struct Module *module;
	InitNodeImpl init;
	ResurrectNodeImpl resurrect;
	UpdNodeImpl upd;
	FreeNodeImpl free;
	StorageNodeImpl storage;
	U32 size;

	// Set by node system!
	U32 auto_storage_handle; // Handle to AutoNodeImplStorage
} NodeType;

REVOLC_API void init_nodetype(NodeType *node);

REVOLC_API WARN_UNUSED
int json_nodetype_to_blob(struct BlobBuf *buf, JsonTok j);

#endif // REVOLC_GAME_NODETYPE_H
