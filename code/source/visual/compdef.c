#include "compdef.h"

int json_compdef_to_blob(struct BlobBuf *buf, JsonTok j)
{
	JsonTok j_armature = json_value_by_key(j, "armature");
	JsonTok j_subs = json_value_by_key(j, "subs");

	if (json_is_null(j_armature))
		RES_ATTRIB_MISSING("armature");
	if (json_is_null(j_subs))
		RES_ATTRIB_MISSING("subs");

	CompDef def = {};
	fmt_str(def.armature_name, sizeof(def.armature_name), "%s",
			json_str(j_armature));
	if (json_member_count(j_subs) > MAX_SUBENTITY_COUNT) {
		critical_print("Too many subs: %i > %i",
				json_member_count(j_subs), MAX_SUBENTITY_COUNT);
		goto error;
	}
	for (U32 sub_i = 0; sub_i < json_member_count(j_subs); ++sub_i) {
		JsonTok j_sub = json_member(j_subs, sub_i);

		JsonTok j_entity = json_value_by_key(j_sub, "entity");
		JsonTok j_joint = json_value_by_key(j_sub, "joint");
		JsonTok j_offset = json_value_by_key(j_sub, "offset");

		if (json_is_null(j_entity))
			RES_ATTRIB_MISSING("entity");
		if (json_is_null(j_joint))
			RES_ATTRIB_MISSING("joint");
		if (json_is_null(j_offset))
			RES_ATTRIB_MISSING("offset");

		CompDef_Sub *sub = &def.subs[sub_i];
		fmt_str(sub->entity_name, sizeof(sub->entity_name), "%s",
				json_str(j_entity));
		fmt_str(sub->joint_name, sizeof(sub->joint_name), "%s",
				json_str(j_joint));
		sub->offset = t3d_to_t3f(json_t3(j_offset));
		++def.sub_count;
	}

	blob_write(buf, &def, sizeof(def));

	return 0;

error:
	return 1;
}

CompDef *blobify_compdef(struct WArchive *ar, Cson c, bool *err)
{
	Cson c_armature = cson_key(c, "armature");
	Cson c_subs = cson_key(c, "subs");

	if (cson_is_null(c_armature))
		RES_ATTRIB_MISSING("armature");
	if (cson_is_null(c_subs))
		RES_ATTRIB_MISSING("subs");

	CompDef def = {};
	fmt_str(def.armature_name, sizeof(def.armature_name), "%s",
			blobify_string(c_armature, err));
	if (cson_member_count(c_subs) > MAX_SUBENTITY_COUNT) {
		critical_print("Too many subs: %i > %i",
				cson_member_count(c_subs), MAX_SUBENTITY_COUNT);
		goto error;
	}
	for (U32 sub_i = 0; sub_i < cson_member_count(c_subs); ++sub_i) {
		Cson c_sub = cson_member(c_subs, sub_i);

		Cson c_entity = cson_key(c_sub, "entity");
		Cson c_joint = cson_key(c_sub, "joint");
		Cson c_offset = cson_key(c_sub, "offset");

		if (cson_is_null(c_entity))
			RES_ATTRIB_MISSING("entity");
		if (cson_is_null(c_joint))
			RES_ATTRIB_MISSING("joint");
		if (cson_is_null(c_offset))
			RES_ATTRIB_MISSING("offset");

		CompDef_Sub *sub = &def.subs[sub_i];
		fmt_str(sub->entity_name, sizeof(sub->entity_name), "%s",
				blobify_string(c_entity, err));
		fmt_str(sub->joint_name, sizeof(sub->joint_name), "%s",
				blobify_string(c_joint, err));
		sub->offset = t3d_to_t3f(blobify_t3(c_offset, err));
		++def.sub_count;
	}

	if (err && *err)
		goto error;

	CompDef *ptr = warchive_ptr(ar);
	pack_buf(ar, &def, sizeof(def));
	return ptr;

error:
	SET_ERROR_FLAG(err);
	return NULL;
}
