#include "core/array.h"
#include "core/debug_print.h"
#include "core/ensure.h"
#include "global/env.h"
#include "resblob.h"

#define HEADERS
#	include "resources/resources.def"
#undef HEADERS

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <jsmn.h>

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

ResBlob* load_blob(const char *path)
{
	ResBlob* blob= NULL;
	{ // Load from file

		U32 blob_size;
		blob= (ResBlob*)malloc_file(path, &blob_size);
		if (g_env.res_blob == NULL)
			g_env.res_blob= blob;

		debug_print("ResBlob loaded: %s, %i", path, (int)blob_size);
	}

	{ // Initialize resources
		for (ResType t= 0; t < ResType_last; ++t) {
			for (U32 i= 0; i < blob->res_count; ++i) {
				Resource* res= resource_by_index(blob, i);
				if (res->type != t)
					continue;
#define RESOURCE(type, init, deinit) \
				{ \
					void* fptr= (void*)init; \
					if (fptr && ResType_ ## type == t) \
						((void (*)(type *))fptr)((type*)res); \
				}
#	include "resources/resources.def"
#undef RESOURCE
			}
		}
	}

	return blob;
}

void unload_blob(ResBlob *blob)
{
	{ // Deinitialize resources
		for (ResType t_= ResType_last; t_ > 0; --t_) {
			for (U32 i_= blob->res_count; i_ > 0; --i_) {
				ResType t= t_ - 1;
				U32 i= i_ - 1;
				Resource* res= resource_by_index(blob, i);
				if (res->type != t)
					continue;
#define RESOURCE(type, init, deinit) \
				{ \
					void* fptr= (void*)deinit; \
					if (fptr && ResType_ ## type == t) \
						((void (*)(type *))fptr)((type*)res); \
				}
#	include "resources/resources.def"
#undef RESOURCE
			}
		}
	}

	if (g_env.res_blob == blob)
		g_env.res_blob= NULL;

	free(blob);
}

Resource* resource_by_index(const ResBlob *blob, U32 index)
{
	ensure(index < blob->res_count);
	return (Resource*)((U8*)blob + blob->res_offsets[index]);
}

Resource* resource_by_name(const ResBlob *blob, ResType t, const char *name)
{
	/// @todo Binary search from organized blob
	for (U32 i= 0; i < blob->res_count; ++i) {
		Resource* res= resource_by_index(blob, i);
		if (res->type == t && !strcmp(name, res->name))
			return res;
	}
	
	fail("Resource not found");
	return NULL;
}

void* blob_ptr(ResBlob *blob, BlobOffset offset)
{ return (void*)((U8*)blob + offset); }

void print_blob(const ResBlob *blob)
{
	debug_print("Res count: %i", (int)blob->res_count);
	for (int res_i= 0; res_i < blob->res_count; ++res_i) {
		Resource* res= resource_by_index(blob, res_i);
		debug_print(
				"Resource: %s, %i",
				res->name,
				res->type);

		switch (res->type) {
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
		}
	}
}

//
// JSON to blob
//

typedef jsmntok_t JsonTok;
internal
bool is_json_tok(const char *json, JsonTok t, const char *str)
{
	U32 i= 0;
	while (str[i] != 0 && i < t.end - t.start) {
		if (str[i] != *(json + t.start + i))
			return false;
		++i;
	}
	return true;
}

internal
void json_strcpy(char *dst, const char *json_src, U32 len)
{
	for (U32 i= 0; i < len; ++i)
		dst[i]= json_src[i];
	dst[len]= '\0';
}

internal
U32 json_tok_len(JsonTok t)
{ return t.end - t.start; }

typedef FILE* BlobBuf;

internal
void blob_write(BlobBuf blob, BlobOffset *offset, const void *data, U32 byte_count)
{
	U32 ret= fwrite(data, byte_count, 1, blob);
	if (ret != 1)
		fail("blob_write failed");
	*offset += byte_count;
}

void make_blob(const char *dst_file, const char *src_file)
{
	// Information gathered at first scan
	// Used to write header and perform second scan
	typedef struct {
		ResType type;
		char name[RES_NAME_LEN];
		jsmntok_t* tok;
	} ResInfo;

	// Resources-to-be-allocated
	char *data= NULL;
	jsmntok_t *t= NULL;
	FILE *blob= NULL;
	ResInfo *res_infos= NULL;

	// Input file
	U32 file_size;
	data= (char*)malloc_file(src_file, &file_size);

	int r;
	{ // Parse json
		U32 token_count= file_size/4 + 64; // Intuition
		t= malloc(sizeof(jsmntok_t)*token_count);
		jsmn_parser parser;
		jsmn_init(&parser);
		r= jsmn_parse(&parser, (char*)data, file_size,
				t, token_count);
		switch (r) {
			case JSMN_ERROR_NOMEM:
				critical_print("Too large JSON file (engine problem): %s", src_file);
				goto error;
			break;
			case JSMN_ERROR_INVAL:
				critical_print("JSON syntax error: %s", src_file);
				goto error;
			break;
			case JSMN_ERROR_PART:
				critical_print("Unexpected JSON end: %s", src_file);
				goto error;
			break;
			case 0:
				critical_print("Empty JSON file: %s", src_file);
				goto error;
			break;
			default: ensure(r > 0);
		}

		/*{ // " to '\0' so that json strings are null-terminated
			for (U32 i= 1; i < r; ++i) {
				if (t[i].type == JSMN_STRING)
					data[t[i].end]= '\0';
			}
		}*/
	}

	U32 res_info_count= 0;
	U32 res_info_capacity= 1024;
	res_infos= malloc(sizeof(*res_infos)*res_info_capacity);
	{ // Scan throught JSON and gather ResInfos
		U32 i= 1;
		while (i < r) {
			if (t[i].type != JSMN_OBJECT) {
				i += t[i].deep_size + 1;
				continue;
			}

			// Scan single resource
			ResInfo res_info= {};
			res_info.tok= &t[i];

			ensure(t[i].type == JSMN_OBJECT);
			++i;
			for (	U32 field_i= 0; field_i < res_info.tok->size;
					++field_i, i += t[i].deep_size + 1) {
				if (t[i].type != JSMN_STRING)
					continue;

				// Create null-terminated string for value
				U32 next_len= json_tok_len(t[i + 1]);
				char value_str[next_len + 1];
				json_strcpy(value_str, data + t[i + 1].start, next_len);

				if (is_json_tok(data, t[i], "type")) {
					res_info.type= str_to_restype(value_str);
					if (res_info.type == ResType_None) {
						critical_print("Invalid resource type: %s", value_str);
						goto error;
					}
				} else if (is_json_tok(data, t[i], "name")) {
					strcpy(res_info.name, value_str);
				}
			}

			if (res_info.name[0] == 0) {
				critical_print("JSON resource missing 'name'");
				goto error;
			}
			if (res_info.type == ResType_None) {
				critical_print("JSON resource missing 'type'");
				goto error;
			}

			res_infos= push_dyn_array(
					res_infos,
					&res_info_capacity, &res_info_count, sizeof(*res_infos),
					&res_info);
		}
	}

	{ // Output file
		blob= fopen(dst_file, "wb");
		if (!blob) {
			critical_print("Opening for write failed: %s", dst_file);
			goto error;
		}
		BlobOffset offset= 0;
		U32 blob_version= 1;
		U32 res_count= res_info_count;
		blob_write(blob, &offset, &blob_version, sizeof(blob_version));
		blob_write(blob, &offset, &res_count, sizeof(res_count));

		for (U32 res_i= 0; res_i < res_info_count; ++res_i) {
			ResInfo *res= &res_infos[res_i];
			debug_print("blobbing: %s, %s", res->name, restype_to_str(res->type));

			JsonTok* t= res->tok;
			U32 i= 1; // First one is the object
			for (U32 field_i= 0; field_i < res->tok->size;
					++field_i, i += t[i].deep_size + 1) {
				if (	t[i].type != JSMN_STRING || // Comment, possibly
						is_json_tok(data, t[i], "type") ||
						is_json_tok(data, t[i], "name")) {
					continue;
				}

				/// @todo write to blob

				debug_print("  attrib: %.*s -- %.*s",
						json_tok_len(t[i]),
						data + t[i].start,
						json_tok_len(t[i + 1]),
						data + t[i + 1].start);
			}
		}
	}

exit:
	if (blob)
		fclose(blob);
	free(res_infos);
	free(t);
	free(data);

	return;

error:
	critical_print("make_blob failed");
	goto exit;
}
