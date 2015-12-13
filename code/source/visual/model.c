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
	JsonTok j_mesh = json_value_by_key(j, "mesh");
	JsonTok j_texs = json_value_by_key(j, "textures");
	JsonTok j_color = json_value_by_key(j, "color");
	JsonTok j_emission = json_value_by_key(j, "emission");
	JsonTok j_pattern = json_value_by_key(j, "pattern");

	if (json_is_null(j_mesh))
		RES_ATTRIB_MISSING("mesh");
	if (json_is_null(j_texs))
		RES_ATTRIB_MISSING("textures");
	if (json_is_null(j_color))
		RES_ATTRIB_MISSING("color");

	Model m = {};
	json_strcpy(m.mesh, sizeof(m.mesh), j_mesh);

	for (U32 i = 0; i < json_member_count(j_texs); ++i) {
		JsonTok j_tex = json_member(j_texs, i);
		json_strcpy(m.textures[i], sizeof(m.textures[i]), j_tex);
	}

	m.color = json_color(j_color);
	if (!json_is_null(j_emission))
		m.emission = json_real(j_emission);
	if (!json_is_null(j_pattern))
		m.pattern = json_integer(j_pattern);

	blob_write(buf, (U8*)&m + sizeof(m.res), sizeof(m) - sizeof(m.res));
	return 0;

error:
	return 1;
}
