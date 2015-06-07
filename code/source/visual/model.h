#ifndef REVOLC_VISUAL_MODEL_H
#define REVOLC_VISUAL_MODEL_H

#include "build.h"
#include "core/json.h"
#include "resources/resource.h"
#include "texture.h"
#include "mesh.h"

#define MODEL_TEX_COUNT 1
typedef struct Model {
	Resource res;
	char textures[MODEL_TEX_COUNT][RES_NAME_SIZE];
	char mesh[RES_NAME_SIZE];
	Color color;
	F64 emission;
	U8 pattern;
} PACKED Model;

REVOLC_API Texture* model_texture(const Model *model, U32 index);
REVOLC_API Mesh* model_mesh(const Model *model);

REVOLC_API
WARN_UNUSED
int json_model_to_blob(struct BlobBuf *buf, JsonTok j);

#endif // REVOLC_VISUAL_MODEL_H
