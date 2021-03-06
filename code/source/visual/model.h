#ifndef REVOLC_VISUAL_MODEL_H
#define REVOLC_VISUAL_MODEL_H

#include "build.h"
#include "core/cson.h"
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
} PACKED Model;

REVOLC_API Texture* model_texture(const Model *model, U32 index);
REVOLC_API Mesh* model_mesh(const Model *model);

REVOLC_API WARN_UNUSED Model *blobify_model(struct WArchive *ar, Cson c, bool *err);
REVOLC_API void deblobify_model(WCson *c, struct RArchive *ar);

#endif // REVOLC_VISUAL_MODEL_H
