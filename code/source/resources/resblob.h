#ifndef REVOLC_RESOURCES_RESBLOB_HPP
#define REVOLC_RESOURCES_RESBLOB_HPP

#include "build.h"
#include "resource.h"

typedef struct ResBlob {
	U32 version;
	U32 res_count;
	BlobOffset res_offsets[];
} ResBlob;

REVOLC_API ResBlob * load_blob(const char *path);
REVOLC_API void unload_blob(ResBlob *blob);
REVOLC_API WARN_UNUSED ResBlob * reload_blob(ResBlob *blob, const char *path);

REVOLC_API Resource * res_by_index(const ResBlob *blob, U32 index);
REVOLC_API Resource * res_by_name(const ResBlob *b, ResType t, const char *n);
REVOLC_API Resource * find_res_by_name(const ResBlob *b, ResType t, const char *n);

REVOLC_API void * blob_ptr(ResBlob *blob, BlobOffset offset);

REVOLC_API void print_blob(const ResBlob *blob);

/// Gathers human-readable resources by `src_file` and
/// makes binary blob out of them to `dst_file
REVOLC_API void make_blob(const char *dst_file, const char *src_file);

typedef FILE* BlobBuf;
REVOLC_API
void blob_write(BlobBuf blob, BlobOffset *offset, const void *data, U32 byte_count);

#endif // REVOLC_RESOURCES_RESBLOB_HPP
