#ifndef REVOLC_RESOURCES_RESBLOB_H
#define REVOLC_RESOURCES_RESBLOB_H

#include "build.h"
#include "core/basic.h"
#include "resource.h"

typedef struct ResBlob {
	U32 version;
	U32 res_count;
	U32 size; // Size of whole blob. Set at load time.

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

REVOLC_API void print_blob(const ResBlob *blob);

// Gathers human-readable resources from `res_file_paths` and
// makes binary blob out of them to `dst_file
// `res_file_paths` is a null-terminated array of null-terminated strings
REVOLC_API void make_blob(const char *dst_file, char **res_file_paths);

REVOLC_API bool inside_blob(const ResBlob *blob, void *ptr);

// Create modifiable resource. After calling this the original resource can't be queried anymore.
REVOLC_API WARN_UNUSED Resource *substitute_res(Resource *res);

// Resize a RelPtr in a resource. Freeing happens automatically.
// Only for runtime resources.
REVOLC_API void realloc_res_member(Resource *res, RelPtr *member, U32 size, U32 old_size);

// Notify that resource data was changed.
// 'res' better be a runtime resource.
REVOLC_API void resource_modified(Resource *res);

// Saves changes to original, unpacked resource files
REVOLC_API U32 mirror_blob_modifications(ResBlob *blob);
REVOLC_API bool blob_has_modifications(const ResBlob *blob);

// Saving and restoring of resource state. For editor undo.
// Data contains pointers so this is only short-term storage.
// @todo Make this safer:
//       - no stored pointers
//       - not just overwrite of data but proper init and deinit
//       - no reallocation of the resources, can be done in-place
REVOLC_API void *save_res_state(const Resource *res);
REVOLC_API void load_res_state(void *data);

// Convenience functions forwarding to corresponding blobify_* and deblobify_*
REVOLC_API int blobify_res(WArchive *ar, Cson c, ResType res_t, const char *base_path);
REVOLC_API void deblobify_res(WCson *c, Resource *res);

// @todo Remove BlobBuf when substituted by WArchive

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
