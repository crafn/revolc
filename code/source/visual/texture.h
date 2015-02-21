#ifndef REVOLC_VISUAL_TEXTURE_H
#define REVOLC_VISUAL_TEXTURE_H

#include "build.h"
#include "core/json.h"
#include "resources/resource.h"

typedef struct {
	U8 r, g, b, a;
} Texel;

typedef struct {
	Resource res;
	U16 reso[2];

	// On init
	U32 gl_id;

	/// @todo Mipmaps
	Texel texels[];
} PACKED Texture;

REVOLC_API void init_texture(Texture *tex, ResBlob *blob);
REVOLC_API void deinit_texture(Texture *tex);

REVOLC_API WARN_UNUSED
int json_texture_to_blob(BlobBuf blob, BlobOffset *offset, JsonTok j);

#endif // REVOLC_VISUAL_TEXTURE_H
