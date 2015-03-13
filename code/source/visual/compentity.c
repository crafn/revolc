#include "compentity.h"
#include "global/env.h"

void init_compentity(CompEntity *data)
{
	*data= (CompEntity) {
		.tf= identity_t3d(),
	};
	for (U32 i= 0; i < MAX_ARMATURE_JOINT_COUNT; ++i)
		data->pose.tf[i]= identity_t3f();
}

SubEntity create_subentity(const Armature *a, CompDef_Sub sub)
{
	// Visual entity type of subentity must be deduced
	bool model_exists= res_exists(g_env.resblob, ResType_Model, sub.entity_name);
	bool compdef_exists= res_exists(g_env.resblob, ResType_CompDef, sub.entity_name);
	if (!model_exists && !compdef_exists) {
		critical_print("Entity res for SubEntity not found: %s", sub.entity_name);
		// Default to missing model
		model_exists= true;
	}

	VEntityType type;
	U32 h= NULL_HANDLE;
	if (model_exists) {
		type= VEntityType_model;
		ModelEntity init;
		init_modelentity(&init);

		snprintf(init.model_name, sizeof(init.model_name), "%s", sub.entity_name);
		h= resurrect_modelentity(&init);
	} else if (compdef_exists) {
		type= VEntityType_comp;
		CompEntity init;
		init_compentity(&init);

		snprintf(init.def_name, sizeof(init.def_name), "%s", sub.entity_name);
		h= resurrect_compentity(&init);
	}
	ensure(h != NULL_HANDLE);

	return (SubEntity) {
		.type= type,
		.handle= h,
		.joint_id= joint_id_by_name(a, sub.joint_name),
		.offset= sub.offset,
	};
}

void destroy_subentity(SubEntity e)
{
	switch (e.type) {
		case VEntityType_model:
			free_modelentity(&g_env.renderer->m_entities[e.handle]);
		break;
		case VEntityType_comp:
			free_compentity(&g_env.renderer->c_entities[e.handle]);
		break;
		default: fail("Unhandled VEntityType: %i", e.type);
	}
}
