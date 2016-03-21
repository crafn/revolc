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

int json_armature_to_blob(struct BlobBuf *buf, JsonTok j)
{
	JsonTok j_joints = json_value_by_key(j, "joints");
	if (json_is_null(j_joints))
		RES_ATTRIB_MISSING("joints");

	JointDef defs[MAX_ARMATURE_JOINT_COUNT] = {};
	U32 def_count = 0;
	if (json_member_count(j_joints) > MAX_ARMATURE_JOINT_COUNT) {
		critical_print("Too many joints: %i > %i",
				json_member_count(j_joints), MAX_ARMATURE_JOINT_COUNT);
		goto error;
	}
	for (U32 joint_i = 0; joint_i < json_member_count(j_joints); ++joint_i) {
		JsonTok j_joint = json_member(j_joints, joint_i);

		JsonTok j_name = json_value_by_key(j_joint, "name");
		JsonTok j_super = json_value_by_key(j_joint, "super");
		JsonTok j_offset = json_value_by_key(j_joint, "offset");

		if (json_is_null(j_name))
			RES_ATTRIB_MISSING("name");
		if (json_is_null(j_super))
			RES_ATTRIB_MISSING("super");
		if (json_is_null(j_offset))
			RES_ATTRIB_MISSING("offset");

		JointDef *def = &defs[joint_i];
		def->name = json_str(j_name);
		def->super_name = json_str(j_super);
		def->offset = t3d_to_t3f(json_t3(j_offset));
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

	blob_write(buf, &a, sizeof(a));

	return 0;

error:
	return 1;
}

void armature_to_json(WJson *j, const Armature *a)
{
	WJson *j_joints = wjson_named_member(j, JsonType_array, "joints");

	for (U32 i = 0; i < a->joint_count; ++i) {
		/// @todo Name and super joints

		WJson *j_offset = wjson_str("offset");
		wjson_append(	j_offset,
						wjson_t3(t3f_to_t3d(a->joints[i].bind_pose)));

		WJson *j_joint = wjson_object();
		wjson_append(j_joint, j_offset);
		wjson_append(j_joints, j_joint);
	}
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
JointId joint_id_by_name(const Armature *a, const char *name)
{
	for (JointId i = 0; i < a->joint_count; ++i) {
		if (!strcmp(a->joint_names[i], name))
			return i;
	}
	return NULL_JOINT_ID;
}

