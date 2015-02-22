#include "core/array.h"
#include "core/debug_print.h"
#include "core/ensure.h"
#include "core/json.h"
#include "resblob.h"

#define HEADERS
#	include "resources/resources.def"
#undef HEADERS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MISSING_RES_FILE "../../resources/gamedata/missing"

internal
int json_res_to_blob(BlobBuf *buf, JsonTok j, ResType res_t)
{
#define RESOURCE(name, init, deinit, blobify) \
	if (res_t == ResType_ ## name) \
		return blobify(buf, j);
#	include "resources.def"
#undef RESOURCE
	return 1;
}

internal
U8* malloc_file(const char* path, U32 *file_size)
{
	FILE *file= fopen(path, "rb");
	if (!file)
		fail("Couldn't open file: %s", path);

	fseek(file, 0, SEEK_END);
	U32 size= ftell(file);
	fseek(file, 0, SEEK_SET);

	U8 *buf= malloc(size);
	U64 len= fread(buf, size, 1, file);
	if (len != 1)
		fail("Couldn't fully read file: %s", path);

	fclose(file);

	if (file_size)
		*file_size= size;

	return buf;
}

typedef struct {
	jsmntok_t *tokens;
	char *json;
	JsonTok root;
} ParsedJsonFile;

internal
ParsedJsonFile malloc_parsed_json_file(const char *file)
{
	ParsedJsonFile ret= {};
	U32 file_size;
	ret.json= (char*)malloc_file(file, &file_size);

	{ // Parse json
		U32 token_count= file_size/4 + 64; // Intuition
		ret.tokens= malloc(sizeof(jsmntok_t)*token_count);
		jsmn_parser parser;
		jsmn_init(&parser);
		int r= jsmn_parse(&parser, ret.json, file_size,
				ret.tokens, token_count);
		switch (r) {
			case JSMN_ERROR_NOMEM:
				critical_print("Too large JSON file (engine problem): %s",
						file);
				goto error;
			break;
			case JSMN_ERROR_INVAL:
				critical_print("JSON syntax error: %s",
						file);
				goto error;
			break;
			case JSMN_ERROR_PART:
				critical_print("Unexpected JSON end: %s",
						file);
				goto error;
			break;
			case 0:
				critical_print("Empty JSON file: %s",
						file);
				goto error;
			break;
			default: ensure(r > 0);
		}

		ret.root.json_path= file;
		ret.root.json= ret.json;
		ret.root.tok= ret.tokens;

		{ // Terminate strings and primitives so that they're easier to handle
			for (U32 i= 1; i < r; ++i) {
				if (	ret.tokens[i].type == JSMN_STRING ||
						ret.tokens[i].type == JSMN_PRIMITIVE)
					ret.json[ret.tokens[i].end]= '\0';
			}
		}
	}

	return ret;

error:
	free(ret.json);
	free(ret.tokens);

	ParsedJsonFile null= {};
	return null;
}

internal
void free_parsed_json_file(ParsedJsonFile json)
{
	free(json.json);
	free(json.tokens);
}

internal
void init_res(Resource *res)
{
#define RESOURCE(rtype, init, deinit, blobify) \
	{ \
		void* fptr= (void*)init; \
		if (fptr && ResType_ ## rtype == res->type) \
			((void (*)(rtype *))fptr)((rtype*)res); \
	}
#	include "resources/resources.def"
#undef RESOURCE
}

ResBlob * load_blob(const char *path)
{
	ResBlob *blob= NULL;
	{ // Load from file
		U32 blob_size;
		blob= (ResBlob*)malloc_file(path, &blob_size);
		debug_print("load_blob: %s, %i", path, (int)blob_size);
	}

	{ // Initialize resources
		for (ResType t= 0; t < ResType_last; ++t) {
			for (U32 i= 0; i < blob->res_count; ++i) {
				Resource *res= res_by_index(blob, i);
				res->blob= blob;
				if (res->type != t)
					continue;
				init_res(res);
			}
		}
	}

	return blob;
}

internal
void deinit_res(Resource *res)
{
#define RESOURCE(rtype, init, deinit, blobify) \
	{ \
		void* fptr= (void*)deinit; \
		if (fptr && ResType_ ## rtype == res->type) \
			((void (*)(rtype *))fptr)((rtype*)res); \
	}
#	include "resources/resources.def"
#undef RESOURCE
}

void unload_blob(ResBlob *blob)
{
	{ // Deinitialize resources
		for (ResType t_= ResType_last; t_ > 0; --t_) {
			for (U32 i_= blob->res_count; i_ > 0; --i_) {
				ResType t= t_ - 1;
				U32 i= i_ - 1;
				Resource* res= res_by_index(blob, i);
				if (res->type != t)
	 				continue;
				deinit_res(res);
			}
		}
	}

	{ // Free MissingResources
		MissingResource *res= blob->first_missing_res;
		/// @note Proper order would be reverse
		while (res) {
			MissingResource *next= res->next;
			deinit_res(res->res);
			free(res->res);
			free(res);
			res= next;
		}
	}

	free(blob);
}

ResBlob* reload_blob(ResBlob *old_blob, const char *path)
{
	debug_print("reload_blob: %s", path);

	ResBlob *new_blob= load_blob(path);

	{ // Check that all resources can be found
		/// @todo Can be done in O(n)
		for (U32 i= 0; i < old_blob->res_count; ++i) {
			Resource *old= res_by_index(old_blob, i);

			if (!find_res_by_name(new_blob, old->type, old->name)) {
				critical_print(
						"Can't reload blob: new blob missing resource: %s, %s",
						old->name, restype_to_str(old->type));

				unload_blob(new_blob);
				return old_blob;
			}
		}
	}

	// Update old res pointers to new blob
	if (g_env.renderer) {
		// Model
		for (U32 e_i= 0; e_i < g_env.renderer->entity_count; ++e_i) {
			ModelEntity *e= &g_env.renderer->entities[e_i];

			const Model *m= e->model=
				(Model*)res_by_name(
						new_blob,
						e->model->res.type,
						e->model->res.name);

			// Update cached values in EntityModels
			for (int t_i= 0; t_i < MODEL_TEX_COUNT; ++t_i) {
				Texture *tex= model_texture(m, t_i);
				if (tex)
					e->tex_gl_ids[t_i]= tex->gl_id;
			}
			e->vertices= (TriMeshVertex*)mesh_vertices(model_mesh(m));
			e->indices= (MeshIndexType*)mesh_indices(model_mesh(m));
			e->mesh_v_count= model_mesh(m)->v_count;
			e->mesh_i_count= model_mesh(m)->i_count;
		}
	}

	unload_blob(old_blob);
	return new_blob;
}

Resource * res_by_index(const ResBlob *blob, U32 index)
{
	ensure(index < blob->res_count);
	return (Resource*)((U8*)blob + blob->res_offsets[index]);
}

Resource * res_by_name(ResBlob *blob, ResType type, const char *name)
{
	Resource *res= find_res_by_name(blob, type, name);
	if (res)
		return res;

	critical_print("Resource not found: %s, %s", name, restype_to_str(type));
	// Search already created missing resources
	MissingResource *missing= NULL;
	if (blob->first_missing_res) {
		missing= blob->first_missing_res;
		while (missing) {
			if (	missing->res->type == type &&
					!strcmp(missing->res->name, name))
				break; // Found
			missing= missing->next;
		}
	}

	if (missing) {
		critical_print("Using MissingResource");
		return missing->res;
	} else {
		critical_print("Creating MissingResource");

		Resource res_header= {};
		snprintf(res_header.name, RES_NAME_SIZE, "%s", name);
		res_header.type= type;
		res_header.blob= blob;
		res_header.is_missing_res= true;

		MissingResource *new_res= zero_malloc(sizeof(*new_res));
		const U32 missing_res_max_size= 1024;
		new_res->res= zero_malloc(missing_res_max_size);

		BlobBuf buf= {
			.data= new_res->res,
			.is_file= false
		};

		blob_write(&buf, &res_header, sizeof(res_header));

		ParsedJsonFile parsed_json= malloc_parsed_json_file(MISSING_RES_FILE);
		if (parsed_json.tokens == NULL)
			fail("Failed parsing %s", MISSING_RES_FILE);

		int err=
			json_res_to_blob(
					&buf,
					json_value_by_key(
						parsed_json.root, 
						restype_to_str(type)),
					type);
		if (err)
			fail("Creating MissingResource failed (wtf, where's your resources?)");
		ensure(buf.offset <= missing_res_max_size);

		free_parsed_json_file(parsed_json);
		init_res(new_res->res);

		// Insert to missing resources of the blob
		if (!blob->first_missing_res) {
			blob->first_missing_res= new_res;
		} else {
			MissingResource *last= blob->first_missing_res;
			while (last->next)
				last= last->next;
			last->next= new_res;
		}

		return new_res->res;
	}
}

Resource * find_res_by_name(const ResBlob *b, ResType t, const char *n)
{
	/// @todo Binary search from organized blob
	for (U32 i= 0; i < b->res_count; ++i) {
		Resource* res= res_by_index(b, i);
		if (res->type == t && !strcmp(n, res->name))
			return res;
	}
	return NULL;	
}

void* blob_ptr(const Resource *who_asks, BlobOffset offset)
{
	if (!who_asks->is_missing_res)
		return (void*)((U8*)who_asks->blob + offset);
	else /// @see MissingResource definition
		return (void*)((U8*)who_asks + offset);
}

void print_blob(const ResBlob *blob)
{
	debug_print("print_blob - res count: %i", (int)blob->res_count);
	for (int res_i= 0; res_i < blob->res_count; ++res_i) {
		Resource* res= res_by_index(blob, res_i);
		debug_print(
				"  resource: %s, %s",
				res->name,
				restype_to_str(res->type));

		/*switch (res->type) {
			case ResType_Texture: {
				Texture* tex= (Texture*)res;
				debug_print(
					"  reso: %i, %i",
					tex->reso[0], tex->reso[1]);
			} break;
			case ResType_Mesh: {
				Mesh* mesh= (Mesh*)res;
				debug_print(
					"  vcount: %i\n  icount: %i",
					mesh->v_count,
					mesh->i_count);
			} break;
			default: break;
		}*/
	}
}

//
// JSON to blob
//

void blob_write(BlobBuf *buf, const void *data, U32 byte_count)
{
	if (buf->is_file) {
		U32 ret= fwrite(data, 1, byte_count, buf->data);
		if (ret != byte_count)
			fail("blob_write failed: %i != %i", ret, byte_count);
	} else {
		memcpy(buf->data + buf->offset, data, byte_count);
	}
	buf->offset += byte_count;
}

/// Used only in blob making
/// Information gathered at first scan
/// Used to write header and perform second scan
typedef struct {
	Resource header;
	JsonTok tok;
} ResInfo;

internal
int resinfo_cmp(const void *a_, const void *b_)
{
	const ResInfo *a= (ResInfo*)a_;
	const ResInfo *b= (ResInfo*)b_;
	int str_cmp= strcmp(a->header.name, b->header.name);
	if (str_cmp != 0)
		return str_cmp;
	else
		return a->header.type - b->header.type;
}

void make_blob(const char *dst_file_path, const char *src_file_path)
{
	// Resources-to-be-allocated
	char *data= NULL;
	FILE *blob_file= NULL;
	ResInfo *res_infos= NULL;
	BlobOffset *res_offsets= NULL;

	ParsedJsonFile parsed_json= malloc_parsed_json_file(src_file_path);
	if (parsed_json.tokens == NULL)
		goto error;
	JsonTok j_root= parsed_json.root;

	U32 res_info_count= 0;
	U32 res_info_capacity= 1024;
	res_infos= malloc(sizeof(*res_infos)*res_info_capacity);
	{ // Scan throught JSON and gather ResInfos
		for (U32 res_i= 0; res_i < json_member_count(j_root); ++res_i) {
			JsonTok j_res= json_member(j_root, res_i);
			ResInfo res_info= {};
			res_info.tok= j_res;

			ensure(json_is_object(j_res));

			// Read type
			JsonTok j_type= json_value_by_key(j_res, "type");
			if (!json_is_null(j_type)) {
				const char *type_str= json_str(j_type);
				res_info.header.type= str_to_restype(type_str);
				if (res_info.header.type == ResType_None) {
					critical_print("Invalid resource type: %s", type_str);
					goto error;
				}
			} else {
				critical_print("JSON resource missing 'type'");
				goto error;
			}

			// Read name
			JsonTok j_name= json_value_by_key(j_res, "name");
			if (!json_is_null(j_name)) {
				json_strcpy(
						res_info.header.name,
						sizeof(res_info.header.name),
						j_name);
			} else {
				critical_print("JSON resource missing 'name'");
				goto error;
			}

			res_infos= push_dyn_array(
					res_infos,
					&res_info_capacity, &res_info_count, sizeof(*res_infos),
					&res_info);
		}

		qsort(res_infos, res_info_count, sizeof(*res_infos), resinfo_cmp);
	}

	{ // Output file
		blob_file= fopen(dst_file_path, "wb"); 
		if (!blob_file) {
			critical_print("Opening for write failed: %s", dst_file_path);
			goto error;
		}
		BlobBuf buf= {};
		buf.data= blob_file;
		buf.is_file= true;

		ResBlob header= {
			.version= 1,
			.res_count= res_info_count
		};
		blob_write(&buf, &header, sizeof(header));

		BlobOffset offset_of_offset_table= buf.offset;

		// Write zeros as offsets and fix them afterwards, as they aren't yet known
		res_offsets= zero_malloc(sizeof(*res_offsets)*header.res_count);
		blob_write(&buf, &res_offsets[0], sizeof(*res_offsets)*header.res_count);

		for (U32 res_i= 0; res_i < res_info_count; ++res_i) {
			ResInfo *res= &res_infos[res_i];
			debug_print("blobbing: %s, %s", res->header.name, restype_to_str(res->header.type));

			res_offsets[res_i]= buf.offset;
			blob_write(&buf, &res->header, sizeof(res->header));
			int err= json_res_to_blob(&buf, res->tok, res->header.type);
			if (err) {
				critical_print("Failed to process resource: %s, %s",
					res->header.name, restype_to_str(res->header.type));
				goto error;
			}
		}

		// Write offsets to the header
		fseek(buf.data, offset_of_offset_table, SEEK_SET);
		blob_write(&buf, &res_offsets[0], sizeof(*res_offsets)*header.res_count);
	}

exit:
	free_parsed_json_file(parsed_json);
	free(res_offsets);
	if (blob_file)
		fclose(blob_file);
	free(res_infos);
	free(data);

	return;

error:
	critical_print("make_blob failed");
	goto exit;
}
