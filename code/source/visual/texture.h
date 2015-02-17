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

	Texel texels[1];
} PACKED Texture;

REVOLC_API void init_Texture(Texture *tex);
REVOLC_API void deinit_Texture(Texture *tex);

#endif // REVOLC_VISUAL_TEXTURE_H
