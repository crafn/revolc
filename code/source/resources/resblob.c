#include "core/debug_print.h"
#include "core/ensure.h"
#include "resblob.h"

#define HEADERS
#	include "resources/resources.def" // for print_resources
#undef HEADERS

#include <stdlib.h>
#include <stdio.h>

ResBlob* load_blob(const char *path)
{
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

	debug_print("ResBlob loaded: %s, %i", path, (int)size);
	return (ResBlob*)buf;
}

void unload_blob(ResBlob *blob)
{
	free(blob);
}

Resource* get_resource(const ResBlob *blob, U32 index)
{
	ensure(index < blob->res_count);
	return (Resource*)((U8*)blob + blob->res_offsets[index]);
}

void print_resources(const ResBlob *blob)
{
	debug_print("Res count: %i", (int)blob->res_count);
	for (int res_i= 0; res_i < blob->res_count; ++res_i) {
		Resource* res= get_resource(blob, res_i);
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
			case ResType_Model: {
			} break;
			default: fail("Unknown resource type");
		}
	}
}
