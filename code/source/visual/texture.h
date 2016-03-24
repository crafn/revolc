#ifndef REVOLC_VISUAL_TEXTURE_H
#define REVOLC_VISUAL_TEXTURE_H

#include "atlas.h"
#include "build.h"
#include "core/cson.h"
#include "core/math.h"
#include "global/cfg.h"
#include "resources/resource.h"

typedef struct Texel {
	U8 r, g, b, a;
} Texel;

typedef struct Texture {
	Resource res;
	V2i reso;
	char rel_file[MAX_PATH_SIZE];

	// Renderer sets
	AtlasUv atlas_uv;

	REL_PTR(Texel) texel_offsets[MAX_TEXTURE_LOD_COUNT];
	U32 lod_count;
	U32 texel_data_size;
} PACKED Texture;

Texel * texture_texels(const Texture *tex, U32 lod);
V2i lod_reso(V2i base, U32 lod);

REVOLC_API WARN_UNUSED
Texture *blobify_texture(struct WArchive *ar, Cson c, bool *err);
REVOLC_API void deblobify_texture(WCson *c, struct RArchive *ar);

#endif // REVOLC_VISUAL_TEXTURE_H
