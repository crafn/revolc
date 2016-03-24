#include "compdef.h"

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

void deblobify_compdef(WCson *c, struct RArchive *ar)
{
	CompDef *def = rarchive_ptr(ar, sizeof(*def));
	unpack_advance(ar, sizeof(*def));

	wcson_begin_compound(c, "CompDef");

	wcson_designated(c, "name");
	deblobify_string(c, def->res.name);

	wcson_designated(c, "armature");
	deblobify_string(c, def->armature_name);

	wcson_designated(c, "subs");
	wcson_begin_initializer(c);
	for (U32 i = 0; i < def->sub_count; ++i) {
		CompDef_Sub sub = def->subs[i];

		wcson_begin_initializer(c);

		wcson_designated(c, "entity");
		deblobify_string(c, sub.entity_name);

		wcson_designated(c, "joint");
		deblobify_string(c, sub.joint_name);

		wcson_designated(c, "offset");
		deblobify_t3(c, t3f_to_t3d(sub.offset));

		wcson_end_initializer(c);
	}
	wcson_end_initializer(c);

	wcson_end_compound(c);
}

