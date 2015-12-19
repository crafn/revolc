#ifndef REVOLC_RESOURCES_RESOURCE_H
#define REVOLC_RESOURCES_RESOURCE_H

#include "build.h"
#include "global/cfg.h"

// Don't use this for members. Use RelPtr in core/ptr.h
typedef U64 BlobOffset;

#define RES_ATTRIB_MISSING(name) \
	do { critical_print("Attrib '%s' missing", name); goto error; } while(0)

typedef enum {
	ResType_None,
#define RESOURCE(x, init, deinit, blobify, jsonify, recache) ResType_ ## x,
#	include "resources.def"
#undef RESOURCE
	ResType_last
} ResType;

ResType str_to_restype(const char *str);
const char * restype_to_str(ResType type);

// @todo Should be hash of resource name and type for extremely fast lookup &
//       small memory footprint
typedef struct ResId {
	char name[RES_NAME_SIZE];
	ResType type;
} ResId;


struct ResBlob;
struct BlobBuf;
struct RuntimeResource;

typedef struct Resource {
	ResType type;
	char name[RES_NAME_SIZE];
	struct ResBlob *blob;
	U32 size; // Size of actual resource struct + appended rel_ptr data

	// Dev info
	U32 res_file_index;
	struct Resource *substitute; // If set, this resource is substituted by another (modified) resource
	struct RuntimeResource *runtime_owner; // If set, this is a missing/created/modified resource
} PACKED Resource;

// RuntimeResource are created on demand to separately allocated
// chunks, using the conventional blob creation facility.
//
// Can't use just a single Resource represent runtime ones, as then
// pointers can't be resolved when reloading updated blob.
//
// RuntimeResources are used for missing and edited resources
typedef void (*RtResFree)(Resource *res);
typedef struct RuntimeResource {
	Resource *res;
	struct RuntimeResource *next;

	bool needs_saving;
	// Allocated RelPtrs in the resource
#	define MAX_RESOURCE_REL_PTR_COUNT 4
	RelPtr *allocated_ptrs[MAX_RESOURCE_REL_PTR_COUNT];
	U32 allocated_sizes[MAX_RESOURCE_REL_PTR_COUNT];
} RuntimeResource;

#endif // REVOLC_RESOURCES_RESOURCE_H
