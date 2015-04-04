#ifndef REVOLC_RESOURCES_RESOURCE_H
#define REVOLC_RESOURCES_RESOURCE_H

#include "build.h"
#include "global/cfg.h"

typedef U64 BlobOffset;

#define RES_ATTRIB_MISSING(name) \
	do { critical_print("Attrib '%s' missing", name); goto error; } while(0)

typedef enum {
	ResType_None,
#define RESOURCE(x, init, deinit, blobify, jsonify) ResType_ ## x,
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

typedef struct Resource {
	ResType type;
	char name[RES_NAME_SIZE];
	struct ResBlob *blob;

	// Dev info
	U32 res_file_index;
	bool is_runtime_res; // true if Resource is owned by RuntimeResource
	bool needs_saving; // Can be true only for RuntimeResources
	struct Resource *substitute; // Runtime res. Used for editing
} PACKED Resource;

// RuntimeResource are created on demand to separately allocated
// chunks, using the conventional blob creation facility.
//
// Can't use just a single Resource represent runtime ones, as then
// pointers can't be resolved when reloading updated blob.
//
// For RuntimeResource, BlobOffsets are calculated from the
// beginning of the Resource, NOT from the beginning of the ResBlob.
/// @todo Change offsets to always refer from beginning of the res
//
// RuntimeResources are used for missing and edited resources
typedef void (*RtResFree)(Resource *res);
typedef struct RuntimeResource {
	Resource *res;
	struct RuntimeResource *next;
	RtResFree rt_free; // Called after ordinary res deinit
} RuntimeResource;

#endif // REVOLC_RESOURCES_RESOURCE_H
