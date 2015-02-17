#ifndef REVOLC_RESOURCES_RESBLOB_HPP
#define REVOLC_RESOURCES_RESBLOB_HPP

#include "build.h"
#include "resource.h"

typedef struct {
	U32 version;
	U32 res_count;
	BlobOffset res_offsets[1];
} ResBlob;

REVOLC_API ResBlob* load_blob(const char *path);
REVOLC_API void unload_blob(ResBlob *blob);
REVOLC_API Resource* get_resource(const ResBlob *blob, U32 index);
REVOLC_API void print_resources(const ResBlob *blob);

#endif // REVOLC_RESOURCES_RESBLOB_HPP
