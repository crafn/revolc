#ifndef REVOLC_ANIMATION_JOINT_H
#define REVOLC_ANIMATION_JOINT_H

#include "build.h"
#include "global/cfg.h"
#include "core/transform.h"
#include "resources/resource.h"

typedef U8 JointId;
#define NULL_JOINT_ID ((U8)-1)

// Wrapper for an array of joint poses to allow assigning
typedef struct JointPoseArray {
	T3f tf[MAX_ARMATURE_JOINT_COUNT];
} JointPoseArray;

typedef struct Joint {
	JointId id;
	JointId super_id;
	T3f bind_pose; // Offsets relative to super joint in bind pose
	bool selected; // Editor
} Joint;

#endif // REVOLC_ANIMATION_JOINT_H
