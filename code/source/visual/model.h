#ifndef REVOLC_VISUAL_MODEL_H
#define REVOLC_VISUAL_MODEL_H

#include "build.h"
#include "resources/resource.h"
#include "texture.h"
#include "mesh.h"

typedef struct {
	/// @todo
} BlendFunc;

typedef struct {
	Resource res;
	//F32 color[4];
	//BlendFunc blend_func;
	//bool dynamic_lighting;
	//bool casts_shadow;
	//bool billboard;
	//bool snap_to_pixels;
	BlobOffset texture_offsets[3]; // Offsets to `Texture`
	BlobOffset mesh_offset; // Offset to `Mesh`
} PACKED Model;

Texture* model_texture(const Model *model, U32 index);
Mesh* model_mesh(const Model *model);

#endif // REVOLC_VISUAL_MODEL_H
