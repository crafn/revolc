#ifndef REVOLC_RESOURCES_RESBLOB_H
#define REVOLC_RESOURCES_RESBLOB_H

#include "build.h"
#include "core/ptr.h"
#include "resource.h"

typedef struct ResBlob {
	U32 version;
	U32 res_count;

	char res_file_paths[MAX_RES_FILES][MAX_PATH_SIZE];
	U32 res_file_count;

	RuntimeResource *first_runtime_res;

	BlobOffset res_offsets[];
} PACKED ResBlob;

REVOLC_API void load_blob(ResBlob **blob, const char *path);
REVOLC_API void unload_blob(ResBlob *blob);
REVOLC_API void reload_blob(ResBlob **new, ResBlob *old, const char *path);

// Use RES_BY_NAME instead which will be faster (compile-time hashing of string)
// @todo Remove
REVOLC_API Resource * res_by_name(ResBlob *b, ResType t, const char *n);

REVOLC_API Resource * res_by_id(ResId id);
REVOLC_API ResId res_id(ResType t, const char *n);

#define RES_BY_NAME(T, N) ((T*)res_by_id(RES_ID(T, N)))
// @todo Compile-time
#define RES_ID(T, N) res_id(JOIN2(ResType_, T), N)

REVOLC_API bool res_exists(const ResBlob *blob, ResType t, const char *n);
REVOLC_API Resource * find_res_by_name(const ResBlob *b, ResType t, const char *n);
REVOLC_API Resource ** all_res_by_type(	U32 *count,
										const ResBlob *blob,
										ResType t);

REVOLC_API void * blob_ptr(const Resource *who_asks, BlobOffset offset);
// @todo Don't use this. Use relative ptr from core/ptr.h
REVOLC_API BlobOffset blob_offset(const Resource *who_asks, const void *ptr);

REVOLC_API void print_blob(const ResBlob *blob);

// Gathers human-readable resources from `res_file_paths` and
// makes binary blob out of them to `dst_file
// `res_file_paths` is a null-terminated array of null-terminated strings
REVOLC_API void make_blob(const char *dst_file, char **res_file_paths);

// @todo
//REVOLC_API bool inside_blob(const ResBlob *blob, void *ptr);

// Can be called for any resource
// Dev-only - no need to free :))))
REVOLC_API void realloc_res_member(RelPtr *member, U32 size, U32 old_size);

// Saves changes to original, unpacked resource files
REVOLC_API U32 mirror_blob_modifications(ResBlob *blob);
REVOLC_API bool blob_has_modifications(const ResBlob *blob);

typedef struct BlobBuf {
	// @todo Use WArchive
	void *data;
	U32 offset;
	U32 max_size;

	// Carried along to allow querying for resources during blob making
	BlobOffset *res_offsets;
	U32 res_count;
} BlobBuf;

REVOLC_API
void blob_write(BlobBuf *buf, const void *data, U32 byte_count);

// Patches current offset to "offset_to_ptr" location in buf
REVOLC_API
void blob_patch_rel_ptr(BlobBuf *buf, U32 offset_to_ptr);

// Gives ptr to resource created earlier during blob making
// e.g. a Clip has to get the corresponding Armature by only a name
REVOLC_API
Resource * find_res_by_name_from_blobbuf(	const BlobBuf *buf,
											ResType t,
											const char *n);

#endif // REVOLC_RESOURCES_RESBLOB_H
