#include "global/env.h"
#include "model.h"
#include "resources/resblob.h"

Texture* model_texture(const Model *model, U32 index)
{
	if (model->textures[index][0] == 0)
		return NULL;
	return (Texture*)resource_by_name(
			g_env.res_blob, ResType_Texture, model->textures[index]);
}

Mesh* model_mesh(const Model *model)
{ return (Mesh*)resource_by_name(
		g_env.res_blob, ResType_Mesh, model->mesh); }

