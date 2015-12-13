#ifndef REVOLC_ANIMATION_JOINT_H
#define REVOLC_ANIMATION_JOINT_H

#include "build.h"
#include "global/cfg.h"
#include "core/math.h"
#include "resources/resource.h"

typedef U8 JointId;
#define NULL_JOINT_ID ((U8)-1)

// Wrapper for an array of joint poses to allow assigning
typedef struct JointPoseArray {
	T3f tf[MAX_ARMATURE_JOINT_COUNT];
} JointPoseArray;

REVOLC_API JointPoseArray identity_pose();

REVOLC_API JointPoseArray lerp_pose(	JointPoseArray p1,
										JointPoseArray p2,
										F64 t);

typedef struct Joint {
	JointId id;
	JointId super_id;
	T3f bind_pose; // Offsets relative to super joint in bind pose
	bool selected; // Editor
} Joint;

#endif // REVOLC_ANIMATION_JOINT_H
