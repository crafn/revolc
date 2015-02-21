#ifndef REVOLC_RESOURCES_RESOURCE_H
#define REVOLC_RESOURCES_RESOURCE_H

#include "build.h"

typedef U64 BlobOffset; // Offset from the beginning of a resource blob
#define RES_NAME_LEN 32

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

typedef struct {
	ResType type;
	char name[RES_NAME_LEN];
	struct ResBlob *blob;
} PACKED Resource;

#endif // REVOLC_RESOURCES_RESOURCE_H
