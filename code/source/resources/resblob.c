#include "core/array.h"
#include "core/basic.h"
#include "core/debug.h"
#include "core/cson.h"
#include "resblob.h"

#define HEADERS
#	include "resources/resources.def"
#undef HEADERS

#ifndef CODEGEN
#	include <qc/backend_c.h>
#endif

internal
void init_res(Resource *res)
{
#define RESOURCE(rtype, init, deinit, blobify, deblobify, recache) \
	{ \
		void *fptr = (void*)init; \
		if (fptr && ResType_ ## rtype == res->type) \
			((void (*)(rtype *))fptr)((rtype*)res); \
	}
#	include "resources/resources.def"
#undef RESOURCE
}

Resource * res_by_index(const ResBlob *blob, U32 index)
{
	ensure(index < blob->res_count);
	return (Resource*)((U8*)blob + blob->res_offsets[index]);
}

void load_blob(ResBlob **blob, const char *path)
{
	*blob = NULL;
	{ // Load from file
		U32 blob_size;
		*blob = (ResBlob*)read_file(gen_ator(), path, &blob_size);
		(*blob)->size = blob_size;
		debug_print("load_blob: %s, %iM", path, (int)blob_size/(1024*1024));

		for (U32 i = 0; i < (*blob)->res_file_count; ++i)
			debug_print("  %s", (*blob)->res_file_paths[i]);
	}

	{ // Initialize resources
		for (ResType t = 0; t < ResType_last; ++t) {
			for (U32 i = 0; i < (*blob)->res_count; ++i) {
				Resource *res = res_by_index(*blob, i);
				res->blob = *blob;
				if (res->type != t)
					continue;
				init_res(res);
			}
			// Update symbol table after loading all dll's
			// This allows calling rtti_relocate_sym once per invalidated symbol
			if (t == ResType_Module) {
				rtti_requery_syms();
			}
		}
	}
}

internal
void deinit_res(Resource *res)
{
#define RESOURCE(rtype, init, deinit, blobify, deblobify, recache) \
	{ \
		void* fptr = (void*)deinit; \
		if (fptr && ResType_ ## rtype == res->type) \
			((void (*)(rtype *))fptr)((rtype*)res); \
	}
#	include "resources/resources.def"
#undef RESOURCE
}

// Part of unloading (1/2)
internal
void deinit_blob_res(ResBlob *blob)
{
	for (ResType t_ = ResType_last; t_ > 0; --t_) {
		for (U32 i_ = blob->res_count; i_ > 0; --i_) {
			ResType t = t_ - 1;
			U32 i = i_ - 1;
			Resource* res = res_by_index(blob, i);
			if (res->type != t)
				continue;
			deinit_res(res);
		}
	}
}

// Part of unloading (2/2)
internal
void free_blob(ResBlob *blob)
{ // Free RuntimeResource
	RuntimeResource *res = blob->first_runtime_res;
	// RuntimeResources are in reverse, so destruction is properly ordered
	while (res) {
		RuntimeResource *next = res->next;
		deinit_res(res->res);
		for (U32 i = 0; i < MAX_RESOURCE_REL_PTR_COUNT; ++i) {
			if (res->allocated_ptrs[i])
				FREE(dev_ator(), rel_ptr(res->allocated_ptrs[i]));
		}
		FREE(dev_ator(), res->res);
		FREE(dev_ator(), res);
		res = next;
	}
	FREE(gen_ator(), blob);
}

void unload_blob(ResBlob *blob)
{
	deinit_blob_res(blob);
	free_blob(blob);
}

void reload_blob(ResBlob **new_blob, ResBlob *old_blob, const char *path)
{
	debug_print("reload_blob: %s", path);
	deinit_blob_res(old_blob);

	load_blob(new_blob, path);

	{ // Update old res pointers to new blob
		renderer_on_res_reload();
		world_on_res_reload(old_blob);
	}

	free_blob(old_blob);
}

internal void add_rt_res(ResBlob *blob, RuntimeResource *res)
{
	// Insert to runtime resources of the blob in reverse order
	// this way destruction happens naturally in reverse
	res->next = blob->first_runtime_res;
	blob->first_runtime_res = res;
}

Resource * res_by_name(ResBlob *blob, ResType type, const char *name)
{
	Resource *res = find_res_by_name(blob, type, name);
	if (res)
		return res->substitute ? res->substitute : res;

	// Search already created runtime resources
	RuntimeResource *runtime_res = NULL;
	if (blob->first_runtime_res) {
		runtime_res = blob->first_runtime_res;
		while (runtime_res) {
			if (	runtime_res->res->type == type &&
					!strcmp(runtime_res->res->name, name))
				break; // Found
			runtime_res = runtime_res->next;
		}
	}

	if (runtime_res) {
		return runtime_res->res;
	} else {
		critical_print("Resource not found: %s, %s", name, restype_to_str(type));
		critical_print("Creating MissingResource");

		RuntimeResource *new_res = ALLOC(dev_ator(), sizeof(*new_res), "new_res");
		*new_res = (RuntimeResource) {};

		const U32 missing_res_max_size = 1024*4;
		new_res->res = ZERO_ALLOC(dev_ator(), missing_res_max_size, "new_res->res");

		Ator new_res_ator = linear_ator(new_res->res, missing_res_max_size, "new_res");
		WArchive ar = create_warchive(ArchiveType_binary, &new_res_ator, missing_res_max_size);

		char *file_data = read_file_as_str(dev_ator(), MISSING_RES_FILE);

		Cson c_file = cson_create(file_data, MISSING_RES_BASE);

		Cson c_res = cson_key(c_file, restype_to_str(type));
		if (cson_is_null(c_res))
			fail("Config for missing resources invalid, not found: %s", restype_to_str(type));
		bool err = false;
		Resource *res = blobify_res(&ar, c_res, &err);
		if (err)
			fail("Creating MissingResource failed (wtf, where's your resources?)");

		// Patch header
		fmt_str(res->name, sizeof(res->name), "%s", name);
		res->type = type;
		res->blob = blob;
		res->runtime_owner = new_res;
		res->size = ar.data_size;

		cson_destroy(c_file);

		FREE(dev_ator(), file_data);
		destroy_warchive(&ar);

		init_res(res);

		add_rt_res(blob, new_res);
		return new_res->res;
	}
}

Resource * res_by_id(ResId id)
{
	/// @todo O(1) hash-lookup
	return res_by_name(g_env.resblob, id.type, id.name);
}

ResId res_id(ResType t, const char *n)
{
	ResId id = {
		.type = t
	};
	fmt_str(id.name, sizeof(id.name), "%s", n);
	return id;
}

bool res_exists(const ResBlob *blob, ResType t, const char *n)
{ return find_res_by_name(blob, t, n) != NULL; }

Resource * find_res_by_name(const ResBlob *b, ResType t, const char *n)
{
	/// @todo Binary search from organized blob
	for (U32 i = 0; i < b->res_count; ++i) {
		Resource* res = res_by_index(b, i);
		if (res->type == t && !strcmp(n, res->name)) {
			return res;
		}
	}
	return NULL;
}

Resource ** all_res_by_type(	U32 *count,
								const ResBlob *blob,
								ResType t)
{
	*count = 0;
	// @todo Use Array with frame_ator() (reallocs internally)
	{ // Find count
		for (U32 i = 0; i < blob->res_count; ++i) {
			Resource *res = res_by_index(blob, i);
			if (res->type == t && !res->substitute)
				++*count;
		}
		RuntimeResource *res = blob->first_runtime_res;
		while (res) {
			if (res->res->type == t)
				++*count;
			res = res->next;
		}
	}

	Resource **res_list = frame_alloc(sizeof(*res_list)*(*count));
	Resource **res_it = res_list;
	{
		for (U32 i = 0; i < blob->res_count; ++i) {
			Resource *res = res_by_index(blob, i);
			if (res->type == t && !res->substitute)
				*(res_it++) = res;
		}
		RuntimeResource *res = blob->first_runtime_res;
		while (res) {
			if (res->res->type == t)
				*(res_it++) = res->res;
			res = res->next;
		}
	}
	return res_list;
}

void print_blob(const ResBlob *blob)
{
	debug_print("print_blob - res count: %i", (int)blob->res_count);
	for (U32 res_i = 0; res_i < blob->res_count; ++res_i) {
		Resource* res = res_by_index(blob, res_i);
		debug_print(
				"  %s, %s",
				res->name,
				restype_to_str(res->type));

		switch (res->type) {
			case ResType_Texture: {
				Texture* tex = (Texture*)res;
				debug_print(
					"    reso: %i, %i",
					tex->reso.x, tex->reso.y);
			} break;
			/*case ResType_Mesh: {
				Mesh* mesh = (Mesh*)res;
				debug_print(
					"  vcount: %i\n  icount: %i",
					mesh->v_count,
					mesh->i_count);
			} break;*/
			default: break;
		}
	}
}

/// Used only in blob making
/// Information gathered at first scan
/// Used to write header and perform second scan
typedef struct ResInfo {
	Resource header;
	Cson cson;
} ResInfo;

internal
int resinfo_cmp(const void *a_, const void *b_)
{
	const ResInfo *a = (ResInfo*)a_;
	const ResInfo *b = (ResInfo*)b_;
	int type_dif = a->header.type - b->header.type;
	// Resources of the same type should be contiguous.
	// all_res_by_type relies on this order.
	if (type_dif != 0)
		return type_dif;
	else
		return strcmp(a->header.name, b->header.name);
}

void make_blob(const char *dst_file_path, char **res_file_paths)
{
	char *data = NULL;
	WArchive ar = {};
	FILE *blob_file = NULL;
	ResInfo *res_infos = NULL;
	BlobOffset *res_offsets = NULL;

	U32 res_file_count = 0;
	while (res_file_paths[res_file_count])
		++res_file_count;

	// Parse all resource files
	Cson *csons = ZERO_ALLOC(dev_ator(), sizeof(*csons)*res_file_count, "csons");
	char **file_contents = 
		ZERO_ALLOC(dev_ator(), sizeof(*file_contents)*res_file_count, "file_contents");
	for (U32 i = 0; i < res_file_count; ++i) {
		char *dir = ALLOC(frame_ator(), MAX_PATH_SIZE, "res_dir");
		path_to_dir(dir, res_file_paths[i]);
		file_contents[i] = read_file_as_str(dev_ator(), res_file_paths[i]);
		csons[i] = cson_create(file_contents[i], dir);
		if (cson_is_null(csons[i]))
			goto error;
	}

	U32 res_info_count = 0;
	U32 res_info_capacity = 1024;
	res_infos = ALLOC(dev_ator(), sizeof(*res_infos)*res_info_capacity, "res_infos");
	{ // Scan throught JSON and gather ResInfos
		for (U32 cson_i = 0; cson_i < res_file_count; ++cson_i) {
			debug_print(
					"make_blob: gathering res file: %s",
					res_file_paths[cson_i]);

			Cson cson = csons[cson_i];
			for (U32 res_i = 0; res_i < cson_member_count(cson); ++res_i) {
				Cson c_res = cson_member(cson, res_i);
				ResInfo res_info = {};
				res_info.cson = c_res;
				res_info.header.res_file_index = cson_i;

				ensure(cson_is_compound(c_res));

				// Read type
				const char *type_str = cson_compound_type(c_res);
				if (strlen(type_str) > 0) {
					res_info.header.type = str_to_restype(type_str);
					if (res_info.header.type == ResType_None) {
						critical_print("Invalid resource type: %s", type_str);
						goto error;
					}
				} else {
					critical_print("CSON resource missing 'type'");
					goto error;
				}

				// Read name
				// @todo This should be done by blobify functions
				Cson c_name = cson_key(c_res, "name");
				if (!cson_is_null(c_name)) {
					fmt_str(res_info.header.name, sizeof(res_info.header.name), "%s", blobify_string(c_name, NULL));
				} else {
					critical_print("JSON resource missing 'name'");
					goto error;
				}

				res_infos = push_dyn_array(
						res_infos,
						&res_info_capacity, &res_info_count, sizeof(*res_infos),
						&res_info);
			}
		}

		qsort(res_infos, res_info_count, sizeof(*res_infos), resinfo_cmp);
	}

	{ // Output file
		ar = create_warchive(ArchiveType_binary, dev_ator(), MAX_BLOB_SIZE);

		ResBlob header = {
			.version = 1,
			.res_count = res_info_count,
			.res_file_count = res_file_count,
		};
		for (U32 i = 0; i < res_file_count; ++i) {
			fmt_str(	header.res_file_paths[i], sizeof(header.res_file_paths[i]),
						"%s", res_file_paths[i]);
		}
		pack_buf(&ar, &header, sizeof(header));

		BlobOffset offset_of_offset_table = ar.data_size;

		// Write zeros as offsets and fix them afterwards, as they aren't yet known
		res_offsets = ZERO_ALLOC(dev_ator(), sizeof(*res_offsets)*header.res_count, "res_offsets");
		pack_buf(&ar, &res_offsets[0], sizeof(*res_offsets)*header.res_count);

		for (U32 res_i = 0; res_i < res_info_count; ++res_i) {
			ResInfo *res = &res_infos[res_i];
			debug_print("blobbing: %s, %s", res->header.name, restype_to_str(res->header.type));

			U64 offset_before_res = ar.data_size;
			res_offsets[res_i] = ar.data_size;
			bool err = false;
			blobify_res(&ar, res->cson, &err);

			// Patch res header
			res->header.size = ar.data_size - offset_before_res;
			pack_buf_patch(&ar, res_offsets[res_i], &res->header, sizeof(res->header));

			if (err) {
				critical_print("Failed to process resource: %s, %s",
					res->header.name, restype_to_str(res->header.type));
				goto error;
			}
		}

		// Write offsets to the header
		pack_buf_patch(&ar, offset_of_offset_table, &res_offsets[0], sizeof(*res_offsets)*header.res_count);

		{ // Write blob from memory to file
			blob_file = fopen(dst_file_path, "wb"); 
			if (!blob_file) {
				critical_print("Opening for write failed: %s", dst_file_path);
				goto error;
			}
			U32 written = fwrite(ar.data, 1, ar.data_size, blob_file);
			if (written != ar.data_size) {
				critical_print("Writing blob failed: %i != %i", written, ar.data_size);
				goto error;
			}
		}
	}

exit:
	{
		for (U32 i = 0; i < res_file_count; ++i) {
			FREE(dev_ator(), file_contents[i]);
			cson_destroy(csons[i]);
		}
		FREE(dev_ator(), file_contents);
	}
	FREE(dev_ator(), res_offsets);
	if (blob_file)
		fclose(blob_file);
	destroy_warchive(&ar);
	FREE(dev_ator(), res_infos);
	FREE(dev_ator(), data);

	return;

error:
	critical_print("make_blob failed");
	goto exit;
}

bool inside_blob(const ResBlob *blob, void *ptr)
{
	if ((U8*)ptr >= (U8*)blob && (U8*)ptr < (U8*)blob + blob->size)
		return true;
	return false;
}

internal void recache_ptrs_to(Resource *res)
{
#define RESOURCE(rtype, init, deinit, blobify, deblobify, recache) \
	{ \
		void (*fptr)() = recache; \
		if (fptr && ResType_ ## rtype == res->type) \
			fptr();\
	}
#	include "resources/resources.def"
#undef RESOURCE
}

Resource *substitute_res(Resource *res)
{
	ensure(!res->substitute || !res->runtime_owner); // Can't be both
	ensure(res->size > sizeof(Resource));

	if (res->substitute)
		return res->substitute; // This has already a substitute
	if (res->runtime_owner)
		return res->runtime_owner->res; // Runtime resources can't be substituted (no need)

	RuntimeResource *new_rt = ALLOC(dev_ator(), sizeof(*new_rt), "new_rt");
	*new_rt = (RuntimeResource) {};

	// Copy node from blob to separately allocated
	Resource *new_res = ALLOC(dev_ator(), res->size, "new_res");
	memcpy(new_res, res, res->size);

	res->substitute = new_res;
	new_res->runtime_owner = new_rt;
	new_rt->res = new_res;

	add_rt_res(g_env.resblob, new_rt);

	// Update pointers to 'res' to point 'new_res', and same with the members
	debug_print("recaching subs %s", res->name);
	recache_ptrs_to(res);

	return new_res;
}

void realloc_res_member(Resource *res, RelPtr *member, U32 size, U32 old_size)
{
	if (!res->runtime_owner)
		fail("realloc_res_member: resource is not runtime resource");

	RuntimeResource *rt = res->runtime_owner;

	Handle free_ix = NULL_HANDLE;
	Handle old_ix = NULL_HANDLE;
	for (U32 i = 0; i < MAX_RESOURCE_REL_PTR_COUNT; ++i) {
		if (rt->allocated_ptrs[i] == NULL)
			free_ix = i;
		if (rt->allocated_ptrs[i] == member)
			old_ix = i;
	}

	if (old_ix != NULL_HANDLE) {
		void *ptr = REALLOC(dev_ator(), rel_ptr(member), size, "realloc_res_member");
		set_rel_ptr(member, ptr);
		rt->allocated_ptrs[old_ix] = member;
		rt->allocated_sizes[old_ix] = size;
	} else if (free_ix != NULL_HANDLE) {
		void *ptr = ALLOC(dev_ator(), size, "alloc_res_member");
		memcpy(ptr, rel_ptr(member), MIN(size, old_size));
		set_rel_ptr(member, ptr);
		rt->allocated_ptrs[free_ix] = member;
		rt->allocated_sizes[free_ix] = size;
	} else {
		fail("Too many simultaneous allocations for a resource");
	}

	res->size += (int)size - (int)old_size;

	debug_print("recaching realloc %s", res->name);

	// Update pointers pointing to the old/freed member
	recache_ptrs_to(res);
}

// Mirror possibly modified resource to file
internal
bool mirror_res(Resource *res)
{
	bool err = false;
	ResBlob *blob = res->blob;
	const char *res_file_path = res->blob->res_file_paths[res->res_file_index];
	WCson *cson = wcson_create();

	wcson_begin_initializer(cson);
	for (U32 i = 0; i < blob->res_count; ++i) {
		Resource *blob_res = res_by_index(blob, i);
		if (blob_res->res_file_index != res->res_file_index)
			continue;

		if (blob_res->substitute)
			blob_res = blob_res->substitute;

		deblobify_res(cson, blob_res);
	}
	wcson_end_initializer(cson);

	{
		FILE *file = fopen(res_file_path, "wb");
		if (!file) {
			critical_print("Writing resource file failed: '%s'", res_file_path);
			goto error;
		}
		
		debug_print("Writing resource file '%s'", res_file_path);

		QC_Array(char) code = qc_create_array(char)(0);
		qc_ast_to_c_str(&code, 0, QC_AST_BASE(cson->root));
		fprintf(file, "%s", code.data);
		qc_destroy_array(char)(&code);

		fclose(file);
	}

cleanup:
	wcson_destroy(cson);
	return err;

error:
	critical_print("Error at mirroring resources");
	err = true;
	goto cleanup;
}

void resource_modified(Resource *res)
{
	if (!res->runtime_owner)
		fail("resource_modified: only runtime resources can be modified");
	else
		res->runtime_owner->needs_saving = true;
}

U32 mirror_blob_modifications(ResBlob *blob)
{
	U32 count = 0;
	RuntimeResource *rt_res = blob->first_runtime_res;
	while (rt_res) {
		Resource *res = rt_res->res;
		if (rt_res->needs_saving) {
			// Reloading & writing json for every modified resource
			// shouldn't be a problem, because typically few resources
			// are modified at once
			debug_print("mirroring %s %s", restype_to_str(res->type), res->name);
			bool err = mirror_res(res);
			if (!err)
				rt_res->needs_saving = false;
			++count;
		}
		rt_res = rt_res->next;
	}
	debug_print("mirror_blob_modifications: %i", count);
	return count;
}

bool blob_has_modifications(const ResBlob *blob)
{
	RuntimeResource *rt_res = blob->first_runtime_res;
	while (rt_res) {
		if (rt_res->needs_saving)
			return true;
		rt_res = rt_res->next;
	}
	return false;
}

void *save_res_state(const Resource *res)
{
	if (!res->runtime_owner)
		fail("Resource packing is only for runtime resources");
	RuntimeResource *rt = res->runtime_owner;

	// Calc archived size
	U32 size = res->size;
	for (U32 i = 0; i < MAX_RESOURCE_REL_PTR_COUNT; ++i) {
		if (rt->allocated_ptrs[i])
			size += rt->allocated_sizes[i];
	}
	debug_print("save_res_state: name %s size %i", res->name, size);

	WArchive ar = create_warchive(ArchiveType_binary, dev_ator(), size);

	pack_buf(&ar, res, res->size); // Pack res struct and possibly appended data
	Resource header = *res;
	header.size = size; // This gets larger because of below
	pack_buf_patch(&ar, 0, &header, sizeof(header));

	// Make separately allocated data to appended data
	for (U32 i = 0; i < MAX_RESOURCE_REL_PTR_COUNT; ++i) {
		if (rt->allocated_ptrs[i]) {
			U32 offset_to_rel_ptr = (U8*)rt->allocated_ptrs[i] - (U8*)res;
			U32 offset_to_data = ar.data_size;
			// Patch the relative pointer in res struct
			RelPtr rel = { .value = offset_to_data - offset_to_rel_ptr };
			pack_buf_patch(&ar, offset_to_rel_ptr, &rel, sizeof(rel));
			// Write the external array
			pack_buf(&ar, rel_ptr(rt->allocated_ptrs[i]), rt->allocated_sizes[i]);
		}
	}

	void *ret;
	release_warchive(&ret, NULL, &ar);
	return ret;
}

Resource *blobify_res(WArchive *ar, Cson c, bool *err)
{
	const char *type_name = cson_compound_type(c);
	ResType res_t = str_to_restype(type_name);
#define RESOURCE(name, init, deinit, blobify, deblobify, recache) \
	if (res_t == ResType_ ## name) \
		return (Resource*)((name *(*)(WArchive *, Cson, bool *))blobify)(ar, c, err);
#	include "resources.def"
#undef RESOURCE
	if (err)
		*err = true;
	return NULL;
}

void deblobify_res(WCson *c, Resource *res)
{
	RArchive ar = create_rarchive(ArchiveType_binary, res, res->size);
#define RESOURCE(name, init, deinit, blobify, deblobify, recache) \
	if (res->type == ResType_ ## name) \
		((void (*)(WCson *, RArchive *))deblobify)(c, &ar);
#	include "resources.def"
#undef RESOURCE
	destroy_rarchive(&ar);
}


void load_res_state(void *data)
{
	Resource header = *(Resource*)data;
	if (!header.runtime_owner)
		fail("Resource unpacking is only for runtime resources");
	RuntimeResource *rt = header.runtime_owner; // Assuming all ptrs are valid in the saved res state
	Resource *old_res = rt->res;

	debug_print("load_res_state: name %s size %i", header.name, header.size);

	Resource *new_res = ALLOC(dev_ator(), header.size, "new_res");
	memcpy(new_res, data, header.size);

	rt->res = new_res;
	// Expecting that all ptrs packed to archive are part of the resource, not separately allocated.
	// Therefore free the old allocated members.
	for (U32 i = 0; i < MAX_RESOURCE_REL_PTR_COUNT; ++i) {
		if (rt->allocated_ptrs[i]) {
			FREE(dev_ator(), rel_ptr(rt->allocated_ptrs[i]));
			rt->allocated_ptrs[i] = NULL;
			rt->allocated_sizes[i] = 0;
		}
	}

	// Original blob resource .substitute points to the old runtime res. Fix that.
	for (U32 i = 0; i < header.blob->res_count; ++i) {
		Resource *orig_res = (Resource*)((U8*)header.blob + header.blob->res_offsets[i]);
		if (orig_res->substitute == old_res)
			orig_res->substitute = new_res;
	}

	// Need not to uninit + init anything, as new resource has the same device handles etc.
	// Only relative member arrays have changed place in memory, recaching fixes that
	FREE(dev_ator(), old_res);
	recache_ptrs_to(new_res);
}

