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

	blob_write(buf, &m, sizeof(m));
	return 0;

error:
	return 1;
}

void model_to_json(WJson *j, const Model *m)
{
	wjson_add_named_member(j, "mesh", wjson_str(m->mesh));
	WJson *j_texs = wjson_add_named_member(j, "textures", wjson_array());
	wjson_add_named_member(j, "color", wjson_color(m->color));
	wjson_add_named_member(j, "emission", wjson_number(m->emission));

	for (U32 i = 0; i < MODEL_TEX_COUNT; ++i) {
		wjson_append(j_texs, wjson_str(m->textures[i]));
	}
}

Model *blobify_model(struct WArchive *ar, Cson c, bool *err)
{
	Cson c_mesh = cson_key(c, "mesh");
	Cson c_texs = cson_key(c, "textures");
	Cson c_color = cson_key(c, "color");
	Cson c_emission = cson_key(c, "emission");

	if (cson_is_null(c_mesh))
		RES_ATTRIB_MISSING("mesh");
	if (cson_is_null(c_texs))
		RES_ATTRIB_MISSING("textures");
	if (cson_is_null(c_color))
		RES_ATTRIB_MISSING("color");

	Model m = {};
	fmt_str(m.mesh, sizeof(m.mesh), "%s", blobify_string(c_mesh, err));

	for (U32 i = 0; i < cson_member_count(c_texs); ++i) {
		Cson c_tex = cson_member(c_texs, i);
		fmt_str(m.textures[i], sizeof(m.textures[i]), "%s", blobify_string(c_tex, err));
	}

	m.color = blobify_color(c_color, err);
	if (!cson_is_null(c_emission))
		m.emission = blobify_floating(c_emission, err);

	if (err && *err)
		goto error;

	Model *ptr = warchive_ptr(ar);
	pack_buf(ar, &m, sizeof(m));
	return ptr;

error:
	SET_ERROR_FLAG(err);
	return NULL;
}
