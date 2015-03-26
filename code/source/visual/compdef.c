#include "compdef.h"

int json_compdef_to_blob(struct BlobBuf *buf, JsonTok j)
{
	JsonTok j_armature= json_value_by_key(j, "armature");
	JsonTok j_subs= json_value_by_key(j, "subs");

	if (json_is_null(j_armature))
		RES_ATTRIB_MISSING("armature");
	if (json_is_null(j_subs))
		RES_ATTRIB_MISSING("subs");

	CompDef def= {};
	fmt_str(def.armature_name, sizeof(def.armature_name), "%s",
			json_str(j_armature));
	if (json_member_count(j_subs) > MAX_SUBENTITY_COUNT) {
		critical_print("Too many subs: %i > %i",
				json_member_count(j_subs), MAX_SUBENTITY_COUNT);
		goto error;
	}
	for (U32 sub_i= 0; sub_i < json_member_count(j_subs); ++sub_i) {
		JsonTok j_sub= json_member(j_subs, sub_i);

		JsonTok j_entity= json_value_by_key(j_sub, "entity");
		JsonTok j_joint= json_value_by_key(j_sub, "joint");
		JsonTok j_offset= json_value_by_key(j_sub, "offset");

		if (json_is_null(j_entity))
			RES_ATTRIB_MISSING("entity");
		if (json_is_null(j_joint))
			RES_ATTRIB_MISSING("joint");
		if (json_is_null(j_offset))
			RES_ATTRIB_MISSING("offset");

		CompDef_Sub *sub= &def.subs[sub_i];
		fmt_str(sub->entity_name, sizeof(sub->entity_name), "%s",
				json_str(j_entity));
		fmt_str(sub->joint_name, sizeof(sub->joint_name), "%s",
				json_str(j_joint));
		sub->offset= t3d_to_t3f(json_t3(j_offset));
		++def.sub_count;
	}

	blob_write(	buf,
				(U8*)&def + sizeof(Resource),
				sizeof(def) - sizeof(Resource));

	return 0;

error:
	return 1;
}
