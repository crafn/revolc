#ifndef REVOLC_VISUAL_COMPOUNDENTITY_H
#define REVOLC_VISUAL_COMPOUNDENTITY_H

#include "animation/armature.h"
#include "animation/joint.h"
#include "build.h"
#include "core/transform.h"

typedef enum {
	VEntityType_model,
	VEntityType_compound,
} VEntityType;

typedef struct AttachedEntity {
	char res_name[RES_NAME_SIZE];
	T3f offset;
	U32 handle;
	VEntityType type;
	JointId joint_id;
} AttachedEntity;

typedef struct CompoundEntity {
	char armature_name[RES_NAME_SIZE];
	T3d tf;
	bool allocated;

	JointPoseArray joint_offsets; // Relative to bind offsets
	AttachedEntity attached[MAX_ATTACHED_ENTITY_COUNT];

	// Cached
	const Armature *armature;
} CompoundEntity;

REVOLC_API void init_compoundentity(CompoundEntity *data);

#endif // REVOLC_VISUAL_COMPOUNDENTITY_H
