#include "armature.h"
#include "core/debug_print.h"

int json_armature_to_blob(struct BlobBuf *buf, JsonTok j)
{
	typedef struct JointDef {
		const char *name;
		const char *super_name;
		T3f offset;
	} JointDef;

	JsonTok j_joints= json_value_by_key(j, "joints");
	if (json_is_null(j_joints))
		RES_ATTRIB_MISSING("joints");

	JointDef defs[MAX_ARMATURE_JOINT_COUNT]= {};
	if (json_member_count(j_joints) > MAX_ARMATURE_JOINT_COUNT) {
		critical_print("Too many joints: %i > %i",
				json_member_count(j_joints), MAX_ARMATURE_JOINT_COUNT);
		goto error;
	}

	for (U32 joint_i= 0; joint_i < json_member_count(j_joints); ++joint_i) {
		JsonTok j_joint= json_member(j_joints, joint_i);

		JointDef *def= &defs[joint_i];
		JsonTok j_name= json_value_by_key(j_joint, "name");
		JsonTok j_super= json_value_by_key(j_joint, "super");
		JsonTok j_offset= json_value_by_key(j_joint, "offset");

		if (json_is_null(j_name))
			RES_ATTRIB_MISSING("name");
		if (json_is_null(j_super))
			RES_ATTRIB_MISSING("super");
		if (json_is_null(j_offset))
			RES_ATTRIB_MISSING("offset");

		def->name= json_str(j_name);
		def->super_name= json_str(j_super);
		def->offset= t3d_to_t3f(json_t3(j_offset));
	}

	return 0;
error:
	return 1;
}
