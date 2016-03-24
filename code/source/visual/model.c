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

void deblobify_model(WCson *c, struct RArchive *ar)
{
	Model *m = rarchive_ptr(ar, sizeof(*m));
	unpack_advance(ar, sizeof(*m));

	wcson_begin_compound(c, "Model");

	wcson_designated(c, "name");
	deblobify_string(c, m->res.name);

	wcson_designated(c, "mesh");
	deblobify_string(c, m->mesh);

	wcson_designated(c, "color");
	deblobify_color(c, m->color);

	wcson_designated(c, "emission");
	deblobify_floating(c, m->emission);

	wcson_designated(c, "textures");
	wcson_begin_initializer(c);
	for (U32 i = 0; i < MODEL_TEX_COUNT; ++i) {
		deblobify_string(c, m->textures[i]);
	}
	wcson_end_initializer(c);

	wcson_end_compound(c);
}

