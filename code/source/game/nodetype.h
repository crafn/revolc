#ifndef REVOLC_GAME_NODETYPE_H
#define REVOLC_GAME_NODETYPE_H

#include "build.h"
#include "resources/resource.h"
#include "core/cson.h"

struct World;
struct Module;
struct WArchive;
struct RArchive;
typedef void (*InitNodeImpl)(void *data);
typedef Handle (*ResurrectNodeImpl)(void *dead);
typedef void (*OverwriteNodeImpl)(void *node, const void *dead);
typedef void (*FreeNodeImpl)(Handle h, void *data);
typedef void * (*StorageNodeImpl)();
typedef void (*UpdNodeImpl)(void *node);
typedef void (*PackNodeImpl)(WArchive *ar, const void *begin, const void *end);
typedef void (*UnpackNodeImpl)(RArchive *ar, void *begin, void *end);

typedef enum PackSync {
	PackSync_presence, // Only creation, destroying, and cmds
	PackSync_full, // Full state
} PackSync;

typedef struct NodeType {
	Resource res;

	char init_func_name[MAX_FUNC_NAME_SIZE];
	char resurrect_func_name[MAX_FUNC_NAME_SIZE];
	char overwrite_func_name[MAX_FUNC_NAME_SIZE];
	char free_func_name[MAX_FUNC_NAME_SIZE];
	char upd_func_name[MAX_FUNC_NAME_SIZE];
	char storage_func_name[MAX_FUNC_NAME_SIZE];
	char pack_func_name[MAX_FUNC_NAME_SIZE];
	char unpack_func_name[MAX_FUNC_NAME_SIZE];

	// If true, handles & impls are managed by the node system.
	// Also, free and storage funcs are NULL, and resurrect
	// works in-place, returning NULL_HANDLE.
	bool auto_impl_mgmt;
	U32 max_count;

	PackSync packsync;

	// Cached
	const struct Module *module;
	InitNodeImpl init; // Provides default values as dead data
	ResurrectNodeImpl resurrect; // Creates living node from dead data
	OverwriteNodeImpl overwrite; // Overwrites data of living node by dead data. Used in net game updates.
	UpdNodeImpl upd; // Updates living node
	FreeNodeImpl free; // Frees living node
	StorageNodeImpl storage; // Pointer to storage of nodes
	PackNodeImpl pack; // Picks necessary info from dead node for reconstructing node
	UnpackNodeImpl unpack; // Puts back necessary info to dead node, which can then be resurrected
	U32 size;

	// Set by node system!
	U32 auto_storage_handle; // Handle to AutoNodeImplStorage
} NodeType;

REVOLC_API void init_nodetype(NodeType *node);

REVOLC_API WARN_UNUSED
NodeType *blobify_nodetype(struct WArchive *ar, Cson c, bool *err);
REVOLC_API void deblobify_nodetype(WCson *c, struct RArchive *ar);

#endif // REVOLC_GAME_NODETYPE_H
