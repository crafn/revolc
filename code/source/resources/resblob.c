#include "core/array.h"
#include "core/debug_print.h"
#include "core/ensure.h"
#include "core/json.h"
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
	
	fail("Resource not found: %s, %s", name, restype_to_str(t));
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
				"Resource: %s, %s",
				res->name,
				restype_to_str(res->type));

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


typedef FILE* BlobBuf;

internal
void blob_write(BlobBuf blob, BlobOffset *offset, const void *data, U32 byte_count)
{
	U32 ret= fwrite(data, byte_count, 1, blob);
	if (ret != 1)
		fail("blob_write failed");
	*offset += byte_count;
}

internal
WARN_UNUSED
int json_model_to_blob(BlobBuf blob, BlobOffset *offset, JsonTok j)
{
	char textures[3][RES_NAME_LEN]= {};
	char mesh[RES_NAME_LEN]= {};

	JsonTok j_mesh= json_value_by_key(j, "mesh");
	JsonTok j_texs= json_value_by_key(j, "textures");

	if (json_is_null(j_mesh)) {
		critical_print("Attrib 'mesh' missing for Model: %s",
				json_str(json_value_by_key(j, "name")));
		return 1;
	}

	if (json_is_null(j_texs)) {
		critical_print("Attrib 'textures' missing for Model: %s",
				json_str(json_value_by_key(j, "name")));
		return 1;
	}

	json_strcpy(mesh, sizeof(mesh), j_mesh);

	for (U32 i= 0; i < json_member_count(j_texs); ++i) {
		JsonTok m= json_member(j_texs, i);
		if (!json_is_string(m)) {
			fail("@todo ERR MSG");
			return 1;
		}
		json_strcpy(textures[i], sizeof(textures[i]), m);
	}

	blob_write(blob, offset, textures, sizeof(textures));
	blob_write(blob, offset, mesh, sizeof(mesh));
	return 0;
}

internal
WARN_UNUSED
int json_texture_to_blob(BlobBuf blob, BlobOffset *offset, JsonTok j)
{
	U16 reso[2]= {8, 8};
	U32 gl_id= 0; // Cached

	Texel edge= {100, 200, 255, 255};
	Texel data[reso[0]*reso[1]];
	for (U32 y= 0; y < reso[1]; ++y) {
		for (U32 x= 0; x < reso[0]; ++x) {
			Texel t= {250, 200, 150, 150};
			if (	x == 0 || x == reso[0] - 1 ||
					y == 0 || y == reso[1] - 1)
				t= edge;
			data[x + reso[0]*y]= t;
		}
	}

	blob_write(blob, offset, reso, sizeof(reso));
	blob_write(blob, offset, &gl_id, sizeof(gl_id));
	blob_write(blob, offset, &data, sizeof(data));
	return 0;
}

internal
WARN_UNUSED
int json_mesh_to_blob(BlobBuf blob, BlobOffset *offset, JsonTok j)
{
	MeshType type= MeshType_tri;
	const U32 v_count= 4;
	const U32 i_count= 6;
	BlobOffset v_offset= 0;
	BlobOffset i_offset= 0;

	TriMeshVertex vertices[4]= {};
	vertices[1].pos.x= 0.7;
	vertices[1].uv.x= 1.0;

	vertices[2].pos.x= 1.0;
	vertices[2].pos.y= 0.7;
	vertices[2].uv.x= 1.0;
	vertices[2].uv.y= 1.0;

	vertices[3].pos.y= 1.0;
	vertices[3].uv.y= 1.0;

	MeshIndexType indices[6]= {
		0, 1, 2, 0, 2, 3
	};

	blob_write(blob, offset, &type, sizeof(type));
	blob_write(blob, offset, &v_count, sizeof(v_count));
	blob_write(blob, offset, &i_count, sizeof(i_count));

	v_offset= *offset + sizeof(v_offset) + sizeof(i_offset);
	blob_write(blob, offset, &v_offset, sizeof(v_offset));

	i_offset= *offset + sizeof(i_offset) + sizeof(vertices);
	blob_write(blob, offset, &i_offset, sizeof(i_offset));

	blob_write(blob, offset, &vertices[0], sizeof(vertices));
	blob_write(blob, offset, &indices[0], sizeof(indices));

	return 0;
}

internal
WARN_UNUSED
int json_shader_to_blob(BlobBuf blob, BlobOffset *offset, JsonTok j)
{
	const char* vs_src=
		"#version 150 core\n"
		"in vec3 a_pos;"
		"in vec2 a_uv;"
		"uniform vec2 u_cursor;"
		"out vec2 v_uv;"
		"void main() {"
		"	v_uv= a_uv;"
		"	gl_Position= vec4((a_pos.xy + u_cursor)/(1.0 + a_pos.z), 0.0, 1.0);"
		"}\n";
	const char* fs_src=
		"#version 150 core\n"
		"uniform sampler2D u_tex_color;"
		"in vec2 v_uv;"
		"void main() { gl_FragColor= texture2D(u_tex_color, v_uv); }\n";

	BlobOffset vs_src_offset= *offset + sizeof(Shader) - sizeof(Resource);
	BlobOffset gs_src_offset= 0;
	BlobOffset fs_src_offset= vs_src_offset + strlen(vs_src) + 1;
	MeshType mesh_type= MeshType_tri;
	U32 cached= 0;

	blob_write(blob, offset, &vs_src_offset, sizeof(vs_src_offset));
	blob_write(blob, offset, &gs_src_offset, sizeof(gs_src_offset));
	blob_write(blob, offset, &fs_src_offset, sizeof(fs_src_offset));
	blob_write(blob, offset, &mesh_type, sizeof(mesh_type));
	blob_write(blob, offset, &cached, sizeof(cached));
	blob_write(blob, offset, &cached, sizeof(cached));
	blob_write(blob, offset, &cached, sizeof(cached));
	blob_write(blob, offset, &cached, sizeof(cached));
	blob_write(blob, offset, vs_src, strlen(vs_src) + 1);
	blob_write(blob, offset, fs_src, strlen(fs_src) + 1);

	return 0;
}
/// Used only in blob making
/// Information gathered at first scan
/// Used to write header and perform second scan
typedef struct {
	Resource header;
	JsonTok tok;
} ResInfo;

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

void make_blob(const char *dst_file, const char *src_file)
{
	// Resources-to-be-allocated
	char *data= NULL;
	jsmntok_t *t= NULL;
	FILE *blob= NULL;
	ResInfo *res_infos= NULL;
	BlobOffset *res_offsets= NULL;

	// Input file
	U32 file_size;
	data= (char*)malloc_file(src_file, &file_size);

	JsonTok j_root= {};
	{ // Parse json
		U32 token_count= file_size/4 + 64; // Intuition
		t= malloc(sizeof(jsmntok_t)*token_count);
		jsmn_parser parser;
		jsmn_init(&parser);
		int r= jsmn_parse(&parser, (char*)data, file_size,
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

		j_root.json= data;
		j_root.tok= t;

		{ // " to '\0' so that json strings are null-terminated
			for (U32 i= 1; i < r; ++i) {
				if (t[i].type == JSMN_STRING)
					data[t[i].end]= '\0';
			}
		}
	}

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

		BlobOffset offset_of_offset_table= offset;

		// Write zeros as offsets and fix them afterwards, as they aren't yet known
		res_offsets= zero_malloc(sizeof(*res_offsets)*res_count);
		blob_write(blob, &offset, &res_offsets[0], sizeof(*res_offsets)*res_count);

		for (U32 res_i= 0; res_i < res_info_count; ++res_i) {
			ResInfo *res= &res_infos[res_i];
			debug_print("blobbing: %s, %s", res->header.name, restype_to_str(res->header.type));

			res_offsets[res_i]= offset;
			blob_write(blob, &offset, &res->header, sizeof(res->header));
			int err= 0;
			if (res->header.type == ResType_Model) {
				err= json_model_to_blob(blob, &offset, res->tok);
			} else if (res->header.type == ResType_Texture) {
				err= json_texture_to_blob(blob, &offset, res->tok);
			} else if (res->header.type == ResType_Mesh) {
				err= json_mesh_to_blob(blob, &offset, res->tok);
			} else if (res->header.type == ResType_Shader) {
				err= json_shader_to_blob(blob, &offset, res->tok);
			} else {
				for (U32 i= 0; i < json_member_count(res->tok); ++i) {
					debug_print("  attrib: %s",
							json_str(json_member(res->tok, i)));
				}
			}

			if (err)
				goto error;
		}

		// Write offsets to the header
		fseek(blob, offset_of_offset_table, SEEK_SET);
		blob_write(blob, &offset, &res_offsets[0], sizeof(*res_offsets)*res_count);
	}

exit:
	free(res_offsets);
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
