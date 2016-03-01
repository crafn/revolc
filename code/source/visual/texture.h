#ifndef REVOLC_VISUAL_TEXTURE_H
#define REVOLC_VISUAL_TEXTURE_H

#include "atlas.h"
#include "build.h"
#include "core/json.h"
#include "core/math.h"
#include "global/cfg.h"
#include "resources/resource.h"

typedef struct Texel {
	U8 r, g, b, a;
} Texel;

typedef struct Texture {
	Resource res;
	V2i reso;

	// Renderer sets
	AtlasUv atlas_uv;

	REL_PTR(Texel) texel_offsets[MAX_TEXTURE_LOD_COUNT];
	U32 lod_count;
} PACKED Texture;

Texel * texture_texels(const Texture *tex, U32 lod);
V2i lod_reso(V2i base, U32 lod);

REVOLC_API WARN_UNUSED
int json_texture_to_blob(struct BlobBuf *buf, JsonTok j);

REVOLC_API WARN_UNUSED
int blobify_texture(struct WArchive *ar, Cson c, const char *base_path);

#endif // REVOLC_VISUAL_TEXTURE_H
