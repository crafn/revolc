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

ResBlob* load_blob(const char *path)
{
	ResBlob* blob= NULL;
	{ // Load from file
		FILE *file= fopen(path, "rb");
		if (!file)
			fail("Couldn't open blob");

		fseek(file, 0, SEEK_END);
		U64 size= ftell(file);
		fseek(file, 0, SEEK_SET);

		U8 *buf= malloc(size);
		U64 len= fread(buf, size, 1, file);
		if (len != 1)
			fail("Couldn't fully read blob");

		fclose(file);

		blob= (ResBlob*)buf;
		if (g_env.res_blob == NULL)
			g_env.res_blob= blob;

		debug_print("ResBlob loaded: %s, %i", path, (int)size);
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
{
	return (void*)((U8*)blob + offset);
}

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
					mesh->vertex_count,
					mesh->index_count);
			} break;
			default: break;
		}
	}
}
