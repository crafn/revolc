#ifndef REVOLC_RESOURCES_RESBLOB_H
#define REVOLC_RESOURCES_RESBLOB_H

#include "build.h"
#include "resource.h"

typedef struct ResBlob {
	U32 version;
	U32 res_count;
	MissingResource *first_missing_res;
	BlobOffset res_offsets[];
} ResBlob;

REVOLC_API ResBlob * load_blob(const char *path);
REVOLC_API void unload_blob(ResBlob *blob);
REVOLC_API WARN_UNUSED ResBlob * reload_blob(ResBlob *blob, const char *path);

REVOLC_API Resource * res_by_index(const ResBlob *blob, U32 index);
REVOLC_API Resource * res_by_name(ResBlob *b, ResType t, const char *n);
REVOLC_API Resource * find_res_by_name(const ResBlob *b, ResType t, const char *n);
REVOLC_API void all_res_by_type(U32 *start_index, U32 *count,
								const ResBlob *blob, ResType t);

REVOLC_API void * blob_ptr(const Resource *who_asks, BlobOffset offset);

REVOLC_API void print_blob(const ResBlob *blob);

/// Gathers human-readable resources from `res_file_paths` and
/// makes binary blob out of them to `dst_file
/// @param res_file_paths Null-terminated array of null-terminated strings
REVOLC_API void make_blob(const char *dst_file, char **res_file_paths);

typedef struct BlobBuf {
	void *data;
	U32 offset;
	bool is_file;
} BlobBuf;

REVOLC_API
void blob_write(BlobBuf *buf, const void *data, U32 byte_count);

#endif // REVOLC_RESOURCES_RESBLOB_H
