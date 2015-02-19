#include "global/env.h"
#include "model.h"
#include "resources/resblob.h"

Texture* model_texture(const Model *model, U32 index)
{
	if (model->texture_offsets[index] == 0)
		return NULL;
	return blob_ptr(g_env.res_blob, model->texture_offsets[index]);
}

Mesh* model_mesh(const Model *model)
{ return blob_ptr(g_env.res_blob, model->mesh_offset); }

