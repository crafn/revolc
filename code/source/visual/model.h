#ifndef REVOLC_VISUAL_MODEL_H
#define REVOLC_VISUAL_MODEL_H

#include "build.h"
#include "resources/resource.h"

typedef struct {
	/// @todo
} BlendFunc;

typedef struct {
	Resource res;
	BlobOffset texture_offsets[3]; // Offsets to `Texture`
	F32 color[4];
	BlendFunc blend_func;
	Bool dynamic_lighting;
	Bool casts_shadow;
	Bool billboard;
	Bool snap_to_pixels;
	BlobOffset mesh_offset; // Offset to `Mesh`

	// Cached
	U32 tex_gl_ids[3];
	U32 mesh_gl_id;
} Model;

#endif // REVOLC_VISUAL_MODEL_H
