#ifndef REVOLC_VISUAL_TEXTURE_H
#define REVOLC_VISUAL_TEXTURE_H

#include "build.h"
#include "core/json.h"
#include "core/vector.h"
#include "resources/resource.h"

typedef struct {
	U8 r, g, b, a;
} Texel;

typedef struct {
	Resource res;
	V2i reso;

	// Renderer sets
	V3f atlas_uv;

	/// @todo Mipmaps
	Texel texels[];
} PACKED Texture;

REVOLC_API WARN_UNUSED
int json_texture_to_blob(struct BlobBuf *buf, JsonTok j);

#endif // REVOLC_VISUAL_TEXTURE_H
