#include "armature.h"
#include "core/archive.h"
#include "core/debug.h"
#include "core/memory.h"
#include "core/basic.h"
#include "resources/resblob.h"

typedef struct JointDef {
	const char *name;
	const char *super_name;
	T3f offset;
} JointDef;

internal
JointId super_id_from_defs(	const JointDef *defs,
							U32 def_count,
							const char *super_name)
{
	for (U32 i = 0; i < def_count; ++i) {
		if (!strcmp(defs[i].name, super_name))
			return i;
	}
	return NULL_JOINT_ID;
}

Armature *blobify_armature(struct WArchive *ar, Cson c, bool *err)
{
	Cson c_joints = cson_key(c, "joints");
	if (cson_is_null(c_joints))
		RES_ATTRIB_MISSING("joints");

	JointDef defs[MAX_ARMATURE_JOINT_COUNT] = {};
	U32 def_count = 0;
	if (cson_member_count(c_joints) > MAX_ARMATURE_JOINT_COUNT) {
		critical_print("Too many joints: %i > %i",
				cson_member_count(c_joints), MAX_ARMATURE_JOINT_COUNT);
		goto error;
	}
	for (U32 joint_i = 0; joint_i < cson_member_count(c_joints); ++joint_i) {
		Cson c_joint = cson_member(c_joints, joint_i);

		Cson c_name = cson_key(c_joint, "name");
		Cson c_super = cson_key(c_joint, "super");
		Cson c_offset = cson_key(c_joint, "offset");

		if (cson_is_null(c_name))
			RES_ATTRIB_MISSING("name");
		if (cson_is_null(c_super))
			RES_ATTRIB_MISSING("super");
		if (cson_is_null(c_offset))
			RES_ATTRIB_MISSING("offset");

		JointDef *def = &defs[joint_i];
		def->name = blobify_string(c_name, err);
		def->super_name = blobify_string(c_super, err);
		def->offset = t3d_to_t3f(blobify_t3(c_offset, err));

		++def_count;
	}

	Armature a = {};	
	for (U32 i = 0; i < def_count; ++i) {
		Joint *joint = &a.joints[i];
		JointDef *def = &defs[i];

		fmt_str(	a.joint_names[i], sizeof(a.joint_names[i]), "%s",
					def->name);
		joint->id = i;
		joint->super_id = NULL_JOINT_ID;
		joint->bind_pose = def->offset;

		if (strlen(def->super_name) > 0) {
			joint->super_id =
				super_id_from_defs(defs, def_count, def->super_name);
			if (joint->super_id == NULL_JOINT_ID) {
				critical_print("Unknown super joint: %s", def->super_name);
				goto error;
			}
			ensure(	joint->super_id < joint->id &&
					"@todo Allow free joint ordering");
		}

		++a.joint_count;
	}

	if (err && *err)
		goto error;

	Armature *ptr = warchive_ptr(ar);
	pack_buf(ar, &a, sizeof(a));

	return ptr;

error:
	return NULL;
}

void deblobify_armature(WCson *c, struct RArchive *ar)
{
	Armature *a = rarchive_ptr(ar, sizeof(*a));
	unpack_advance(ar, sizeof(*a));

	wcson_begin_compound(c, "Armature");

	wcson_designated(c, "name");
	deblobify_string(c, a->res.name);

	wcson_designated(c, "joints");
	wcson_begin_initializer(c);
	for (U32 i = 0; i < a->joint_count; ++i) {
		Joint joint = a->joints[i];

		wcson_begin_initializer(c);

		wcson_designated(c, "name");
		deblobify_string(c, a->joint_names[i]);

		wcson_designated(c, "super");
		JointId super_id = joint.super_id;
		deblobify_string(c, super_id != NULL_JOINT_ID ? a->joint_names[super_id] : "");

		wcson_designated(c, "offset");
		deblobify_t3(c, t3f_to_t3d(joint.bind_pose));

		wcson_end_initializer(c);
	}
	wcson_end_initializer(c);

	wcson_end_compound(c);
}

JointId joint_id_by_name(const Armature *a, const char *name)
{
	for (JointId i = 0; i < a->joint_count; ++i) {
		if (!strcmp(a->joint_names[i], name))
			return i;
	}
	return NULL_JOINT_ID;
}

