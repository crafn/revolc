#include "core/array.h"
#include "core/debug_print.h"
#include "core/ensure.h"
#include "core/file.h"
#include "core/json.h"
#include "resblob.h"

#define HEADERS
#	include "resources/resources.def"
#undef HEADERS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MISSING_RES_FILE "../../resources/gamedata/basic/missing"

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

typedef struct ParsedJsonFile {
	jsmntok_t *tokens;
	char *json;
	JsonTok root;
} ParsedJsonFile;

internal
void free_parsed_json_file(ParsedJsonFile json)
{
	free(json.json);
	free(json.tokens);
	free((char*)json.root.json_dir);
}

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
		ret.root.json_dir= malloc_path_to_dir(file);
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
	free_parsed_json_file(ret);

	ParsedJsonFile null= {};
	return null;
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

void load_blob(ResBlob **blob, const char *path)
{
	*blob= NULL;
	{ // Load from file
		U32 blob_size;
		*blob= (ResBlob*)malloc_file(path, &blob_size);
		debug_print("load_blob: %s, %iM", path, (int)blob_size/(1024*1024));
	}

	{ // Initialize resources
		for (ResType t= 0; t < ResType_last; ++t) {
			for (U32 i= 0; i < (*blob)->res_count; ++i) {
				Resource *res= res_by_index(*blob, i);
				res->blob= *blob;
				if (res->type != t)
					continue;
				init_res(res);
			}
		}
	}
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

// Part of unloading (1/2)
internal
void deinit_blob_res(ResBlob *blob)
{
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

// Part of unloading (2/2)
internal
void free_blob(ResBlob *blob)
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
	free(blob);
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
		const U32 missing_res_max_size= 1024*4;
		new_res->res= zero_malloc(missing_res_max_size);

		BlobBuf buf= {
			.data= new_res->res,
			.max_size= missing_res_max_size,
		};

		blob_write(&buf, &res_header, sizeof(res_header));

		ParsedJsonFile parsed_json= malloc_parsed_json_file(MISSING_RES_FILE);
		if (parsed_json.tokens == NULL)
			fail("Failed parsing %s", MISSING_RES_FILE);
		ensure(parsed_json.root.tok);

		JsonTok j_res= json_value_by_key(
							parsed_json.root, 
							restype_to_str(type));
		if (json_is_null(j_res))
			fail("Config for missing resources invalid, not found: %s", restype_to_str(type));

		int err= json_res_to_blob(&buf, j_res, type);
		if (err)
			fail("Creating MissingResource failed (wtf, where's your resources?)");

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

bool res_exists(const ResBlob *blob, ResType t, const char *n)
{ return find_res_by_name(blob, t, n) != NULL; }

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

Resource * find_res_by_name_from_blobbuf(	const BlobBuf *buf,
											ResType t,
											const char *n)
{
	/// @todo Binary search from organized blob
	for (U32 i= 0; i < buf->res_count; ++i) {
		Resource* res= (Resource*)((U8*)buf->data + buf->res_offsets[i]);
		if (res->type == t && !strcmp(n, res->name))
			return res;
	}
	return NULL;
}

void all_res_by_type(	U32 *start_index, U32 *count,
						const ResBlob *blob, ResType t)
{
	U32 placeholder_start_index;
	if (!start_index)
		start_index= &placeholder_start_index;

	*start_index= 0;
	*count= 0;
	while (	*start_index < blob->res_count &&
			res_by_index(blob, *start_index)->type != t)
		++*start_index;
	while (	*start_index + *count < blob->res_count &&
			res_by_index(blob, *start_index + *count)->type == t)
		++*count;
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

		switch (res->type) {
			case ResType_Texture: {
				Texture* tex= (Texture*)res;
				debug_print(
					"  reso: %i, %i",
					tex->reso.x, tex->reso.y);
			} break;
			/*case ResType_Mesh: {
				Mesh* mesh= (Mesh*)res;
				debug_print(
					"  vcount: %i\n  icount: %i",
					mesh->v_count,
					mesh->i_count);
			} break;*/
			default: break;
		}
	}
}

//
// JSON to blob
//

void blob_write(BlobBuf *buf, const void *data, U32 byte_count)
{
	ensure(buf->offset + byte_count <= buf->max_size);
	memcpy(buf->data + buf->offset, data, byte_count);
	buf->offset += byte_count;
}

/// Used only in blob making
/// Information gathered at first scan
/// Used to write header and perform second scan
typedef struct ResInfo {
	Resource header;
	JsonTok tok;
} ResInfo;

internal
int resinfo_cmp(const void *a_, const void *b_)
{
	const ResInfo *a= (ResInfo*)a_;
	const ResInfo *b= (ResInfo*)b_;
	int type_dif= a->header.type - b->header.type;
	// Resources of the same type should be contiguous.
	// all_res_by_type relies on this order.
	if (type_dif != 0)
		return type_dif;
	else
		return strcmp(a->header.name, b->header.name);
}

void make_blob(const char *dst_file_path, char **res_file_paths)
{
	// Resources-to-be-allocated
	char *data= NULL;
	BlobBuf buf= {};
	FILE *blob_file= NULL;
	ResInfo *res_infos= NULL;
	BlobOffset *res_offsets= NULL;

	U32 res_file_count= 0;
	for (; res_file_paths[res_file_count]; ++res_file_count)
		;

	// Parse all resource files
	ParsedJsonFile *parsed_jsons=
		zero_malloc(sizeof(*parsed_jsons)*res_file_count);
	for (U32 i= 0; i < res_file_count; ++i) {
		parsed_jsons[i]= malloc_parsed_json_file(res_file_paths[i]);
		if (parsed_jsons[i].tokens == NULL)
			goto error;
	}

	U32 res_info_count= 0;
	U32 res_info_capacity= 1024;
	res_infos= malloc(sizeof(*res_infos)*res_info_capacity);
	{ // Scan throught JSON and gather ResInfos
		for (U32 json_i= 0; json_i < res_file_count; ++json_i) {
			debug_print(
					"make_blob: gathering res file: %s",
					res_file_paths[json_i]);

			JsonTok j_root= parsed_jsons[json_i].root;
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
		}

		qsort(res_infos, res_info_count, sizeof(*res_infos), resinfo_cmp);
	}

	{ // Output file
		buf= (BlobBuf) {
			.data= malloc(MAX_BLOB_SIZE),
			.max_size= MAX_BLOB_SIZE,
		};

		ResBlob header= {
			.version= 1,
			.res_count= res_info_count
		};
		blob_write(&buf, &header, sizeof(header));

		BlobOffset offset_of_offset_table= buf.offset;

		// Write zeros as offsets and fix them afterwards, as they aren't yet known
		res_offsets= zero_malloc(sizeof(*res_offsets)*header.res_count);
		blob_write(&buf, &res_offsets[0], sizeof(*res_offsets)*header.res_count);

		buf.res_offsets= res_offsets;
		buf.res_count= header.res_count;

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
		const U32 blob_size= buf.offset;

		// Write offsets to the header
		buf.offset= offset_of_offset_table;
		blob_write(&buf, &res_offsets[0], sizeof(*res_offsets)*header.res_count);

		{ // Write blob from memory to file
			blob_file= fopen(dst_file_path, "wb"); 
			if (!blob_file) {
				critical_print("Opening for write failed: %s", dst_file_path);
				goto error;
			}
			U32 written= fwrite(buf.data, 1, blob_size, blob_file);
			if (written != blob_size) {
				critical_print("Writing blob failed: %i != %i", written, blob_size);
				goto error;
			}
		}
	}

exit:
	{
		for (U32 i= 0; i < res_file_count; ++i)
			free_parsed_json_file(parsed_jsons[i]);
		free(parsed_jsons);
	}
	free(res_offsets);
	if (blob_file)
		fclose(blob_file);
	free(buf.data);
	free(res_infos);
	free(data);

	return;

error:
	critical_print("make_blob failed");
	goto exit;
}
