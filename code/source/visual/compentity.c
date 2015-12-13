#include "compentity.h"
#include "global/env.h"

void init_compentity(CompEntity *data)
{
	*data = (CompEntity) {
		.tf = identity_t3d(),
		.pose = identity_pose(),
	};
}

SubEntity create_subentity(const Armature *a, CompDef_Sub sub)
{
	// Visual entity type of subentity must be deduced
	bool model_exists = res_exists(g_env.resblob, ResType_Model, sub.entity_name);
	bool compdef_exists = res_exists(g_env.resblob, ResType_CompDef, sub.entity_name);
	if (!model_exists && !compdef_exists) {
		critical_print("Entity res for SubEntity not found: %s", sub.entity_name);
		// Default to missing model
		model_exists = true;
	}

	VEntityType type;
	U32 h = NULL_HANDLE;
	if (model_exists) {
		type = VEntityType_model;
		ModelEntity init;
		init_modelentity(&init);

		fmt_str(init.model_name, sizeof(init.model_name), "%s", sub.entity_name);
		h = resurrect_modelentity(&init);
	} else if (compdef_exists) {
		type = VEntityType_comp;
		CompEntity init;
		init_compentity(&init);

		fmt_str(init.def_name, sizeof(init.def_name), "%s", sub.entity_name);
		h = resurrect_compentity(&init);
	}
	ensure(h != NULL_HANDLE);

	return (SubEntity) {
		.type = type,
		.handle = h,
		.joint_id = joint_id_by_name(a, sub.joint_name),
		.offset = sub.offset,
	};
}

void destroy_subentity(SubEntity e)
{
	switch (e.type) {
		case VEntityType_model:
			free_modelentity(e.handle);
		break;
		case VEntityType_comp:
			free_compentity(e.handle);
		break;
		default: fail("Unhandled VEntityType: %i", e.type);
	}
}

void calc_global_pose(T3d *global_pose, const CompEntity *e)
{
	// In armature coordinates
	T3f joint_poses[MAX_ARMATURE_JOINT_COUNT];
	const Joint *joints = e->armature->joints;
	for (U32 j_i = 0; j_i < e->armature->joint_count; ++j_i) {
		T3f joint_pose =
			mul_t3f(joints[j_i].bind_pose,
					e->pose.tf[j_i]);
		JointId super_id = joints[j_i].super_id;
		if (super_id != NULL_JOINT_ID) {
			joint_pose =
				mul_t3f(joint_poses[super_id], joint_pose);
		}

		joint_poses[j_i] = joint_pose;
		global_pose[j_i] = mul_t3d(e->tf, t3f_to_t3d(joint_pose));
	}
}

