#ifndef REVOLC_RESOURCES_RESBLOB_HPP
#define REVOLC_RESOURCES_RESBLOB_HPP

#include "build.h"
#include "resource.h"

typedef struct {
	U32 version;
	U32 res_count;
	ResOffset res_offsets[1];
} ResBlob;

ResBlob* load_blob(const char *path);
void unload_blob(ResBlob *blob);
Resource* get_resource(const ResBlob *blob, U32 index);
void print_resources(const ResBlob *blob);

#endif // REVOLC_RESOURCES_RESBLOB_HPP
