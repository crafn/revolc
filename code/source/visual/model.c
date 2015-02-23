#include "model.h"
#include "resources/resblob.h"

Texture* model_texture(const Model *model, U32 index)
{
	if (model->textures[index][0] == 0)
		return NULL;
	return (Texture*)res_by_name(
			model->res.blob, ResType_Texture, model->textures[index]);
}

Mesh* model_mesh(const Model *model)
{ return (Mesh*)res_by_name(
		model->res.blob, ResType_Mesh, model->mesh); }

int json_model_to_blob(struct BlobBuf *buf, JsonTok j)
{
	char textures[MODEL_TEX_COUNT][RES_NAME_SIZE]= {};
	char mesh[RES_NAME_SIZE]= {};

	JsonTok j_mesh= json_value_by_key(j, "mesh");
	JsonTok j_texs= json_value_by_key(j, "textures");

	if (json_is_null(j_mesh)) {
		critical_print("Attrib 'mesh' missing for Model");
		return 1;
	}

	if (json_is_null(j_texs)) {
		critical_print("Attrib 'textures' missing for Model");
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

	blob_write(buf, textures, sizeof(textures));
	blob_write(buf, mesh, sizeof(mesh));
	return 0;
}
