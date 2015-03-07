#ifndef REVOLC_VISUAL_COMPOUNDENTITY_H
#define REVOLC_VISUAL_COMPOUNDENTITY_H

#include "animation/armature.h"
#include "animation/joint.h"
#include "build.h"
#include "core/transform.h"

typedef struct CompoundEntity {
	char armature_name[RES_NAME_SIZE];
	T3d tf;
	bool allocated;

	JointPoseArray joint_offsets; // Relative to bind offsets

	// Cached
	const Armature *armature;
} CompoundEntity;

REVOLC_API void init_compoundentity(CompoundEntity *data);

#endif // REVOLC_VISUAL_COMPOUNDENTITY_H
