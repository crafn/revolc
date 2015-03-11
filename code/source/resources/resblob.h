#ifndef REVOLC_RESOURCES_RESBLOB_H
#define REVOLC_RESOURCES_RESBLOB_H

#include "build.h"
#include "resource.h"

typedef struct ResBlob {
	U32 version;
	U32 res_count;
	MissingResource *first_missing_res;
	BlobOffset res_offsets[];
} PACKED ResBlob;

REVOLC_API void load_blob(ResBlob **blob, const char *path);
REVOLC_API void unload_blob(ResBlob *blob);
REVOLC_API void reload_blob(ResBlob **new, ResBlob *old, const char *path);

REVOLC_API Resource * res_by_index(const ResBlob *blob, U32 index);
REVOLC_API Resource * res_by_name(ResBlob *b, ResType t, const char *n);
REVOLC_API bool res_exists(const ResBlob *blob, ResType t, const char *n);
REVOLC_API Resource * find_res_by_name(const ResBlob *b, ResType t, const char *n);
REVOLC_API void all_res_by_type(U32 *start_index, U32 *count,
								const ResBlob *blob, ResType t);

REVOLC_API void * blob_ptr(const Resource *who_asks, BlobOffset offset);

REVOLC_API void print_blob(const ResBlob *blob);

// Gathers human-readable resources from `res_file_paths` and
// makes binary blob out of them to `dst_file
// `res_file_paths` is a null-terminated array of null-terminated strings
REVOLC_API void make_blob(const char *dst_file, char **res_file_paths);

typedef struct BlobBuf {
	void *data;
	U32 offset;
	U32 max_size;

	// Carried along to allow querying for resources during blob making
	BlobOffset *res_offsets;
	U32 res_count;
} BlobBuf;

REVOLC_API
void blob_write(BlobBuf *buf, const void *data, U32 byte_count);

// Gives ptr to resource created earlier during blob making
// e.g. a Clip has to get the corresponding Armature by only a name
REVOLC_API
Resource * find_res_by_name_from_blobbuf(	const BlobBuf *buf,
											ResType t,
											const char *n);

#endif // REVOLC_RESOURCES_RESBLOB_H
