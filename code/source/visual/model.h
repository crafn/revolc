#ifndef REVOLC_VISUAL_MODEL_H
#define REVOLC_VISUAL_MODEL_H

#include "build.h"
#include "core/json.h"
#include "resources/resource.h"
#include "texture.h"
#include "mesh.h"

typedef struct {
	/// @todo
} BlendFunc;

#define MODEL_TEX_COUNT 3
typedef struct {
	Resource res;
	//F32 color[4];
	//BlendFunc blend_func;
	//bool dynamic_lighting;
	//bool casts_shadow;
	//bool billboard;
	//bool snap_to_pixels;
	char textures[MODEL_TEX_COUNT][RES_NAME_LEN];
	char mesh[RES_NAME_LEN];
} PACKED Model;

REVOLC_API Texture* model_texture(const Model *model, U32 index);
REVOLC_API Mesh* model_mesh(const Model *model);

REVOLC_API
WARN_UNUSED
int json_model_to_blob(BlobBuf blob, BlobOffset *offset, JsonTok j);

#endif // REVOLC_VISUAL_MODEL_H
