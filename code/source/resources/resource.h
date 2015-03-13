#ifndef REVOLC_RESOURCES_RESOURCE_H
#define REVOLC_RESOURCES_RESOURCE_H

#include "build.h"
#include "global/cfg.h"

typedef U64 BlobOffset; // Offset from the beginning of a resource blob

#define RES_ATTRIB_MISSING(name) \
	do { critical_print("Attrib '%s' missing", name); goto error; } while(0)

typedef enum {
	ResType_None,
#define RESOURCE(x, init, deinit, blobify) ResType_ ## x,
#	include "resources.def"
#undef RESOURCE
	ResType_last
} ResType;

ResType str_to_restype(const char *str);
const char * restype_to_str(ResType type);

struct ResBlob;
struct BlobBuf;

typedef struct Resource {
	ResType type;
	char name[RES_NAME_SIZE];
	struct ResBlob *blob;
	bool is_missing_res; /// true if Resource is owned by MissingResource
} PACKED Resource;

/// MissingResources are created on demand to separately allocated
/// chunks, using the conventional blob creation facility.
///
/// Can't use just a single Resource represent missing ones, as then
/// pointers can't be resolved when reloading updated blob.
///
/// For MissingResources, BlobOffsets are calculated from the
/// beginning of the Resource, NOT from the beginning of the ResBlob.
typedef struct MissingResource {
	Resource *res;
	struct MissingResource *next;
} MissingResource;

#endif // REVOLC_RESOURCES_RESOURCE_H
