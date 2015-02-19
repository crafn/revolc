#ifndef REVOLC_VISUAL_TEXTURE_H
#define REVOLC_VISUAL_TEXTURE_H

#include "build.h"
#include "resources/resource.h"

typedef struct {
	U8 r, g, b, a;
} Texel;

typedef struct {
	Resource res;
	U16 reso[2];

	// Cached
	U32 gl_id;

	/// @todo Mipmaps
	Texel texels[1];
} PACKED Texture;

REVOLC_API void init_texture(Texture *tex);
REVOLC_API void deinit_texture(Texture *tex);

#endif // REVOLC_VISUAL_TEXTURE_H
