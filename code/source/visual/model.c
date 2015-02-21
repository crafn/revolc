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

int json_model_to_blob(BlobBuf blob, BlobOffset *offset, JsonTok j)
{
	char textures[3][RES_NAME_LEN]= {};
	char mesh[RES_NAME_LEN]= {};

	JsonTok j_mesh= json_value_by_key(j, "mesh");
	JsonTok j_texs= json_value_by_key(j, "textures");

	if (json_is_null(j_mesh)) {
		critical_print("Attrib 'mesh' missing for Model: %s",
				json_str(json_value_by_key(j, "name")));
		return 1;
	}

	if (json_is_null(j_texs)) {
		critical_print("Attrib 'textures' missing for Model: %s",
				json_str(json_value_by_key(j, "name")));
		return 1;
	}

	json_strcpy(mesh, sizeof(mesh), j_mesh);

	for (U32 i= 0; i < json_member_count(j_texs); ++i) {
		JsonTok m= json_member(j_texs, i);
		if (!json_is_string(m)) {
			fail("@todo ERR MSG");
			return 1;
		}
		json_strcpy(textures[i], sizeof(textures[i]), m);
	}

	blob_write(blob, offset, textures, sizeof(textures));
	blob_write(blob, offset, mesh, sizeof(mesh));
	return 0;
}
