#ifndef REVOLC_GAME_NODETYPE_H
#define REVOLC_GAME_NODETYPE_H

#include "build.h"
#include "resources/resource.h"
#include "core/json.h"

struct World;
struct Module;
struct WArchive;
struct RArchive;
typedef void (*InitNodeImpl)(void *data);
typedef Handle (*ResurrectNodeImpl)(void *dead);
typedef void (*FreeNodeImpl)(Handle h);
typedef void * (*StorageNodeImpl)();
typedef void (*UpdNodeImpl)(void *begin,
							void *end);
typedef void (*PackNodeImpl)(WArchive *ar, const void *begin, const void *end);
typedef void (*UnpackNodeImpl)(RArchive *ar, void *begin, void *end);

typedef struct NodeType {
	Resource res;

	char init_func_name[MAX_FUNC_NAME_SIZE];
	char resurrect_func_name[MAX_FUNC_NAME_SIZE];
	char upd_func_name[MAX_FUNC_NAME_SIZE];

	char free_func_name[MAX_FUNC_NAME_SIZE];
	char storage_func_name[MAX_FUNC_NAME_SIZE];
	char pack_func_name[MAX_FUNC_NAME_SIZE];
	char unpack_func_name[MAX_FUNC_NAME_SIZE];

	// If true, handles & impls are managed by the node system.
	// Also, free and storage funcs are NULL, and resurrect
	// works in-place, returning NULL_HANDLE.
	bool auto_impl_mgmt;
	U32 max_count;

	bool serialize; // If false, node is not netsynced or saved

	// Cached
	const struct Module *module;
	InitNodeImpl init;
	ResurrectNodeImpl resurrect;
	UpdNodeImpl upd;
	FreeNodeImpl free;
	StorageNodeImpl storage;
	PackNodeImpl pack;
	UnpackNodeImpl unpack;
	U32 size;

	// Set by node system!
	U32 auto_storage_handle; // Handle to AutoNodeImplStorage
} NodeType;

REVOLC_API void init_nodetype(NodeType *node);

REVOLC_API WARN_UNUSED
int json_nodetype_to_blob(struct BlobBuf *buf, JsonTok j);

#endif // REVOLC_GAME_NODETYPE_H
