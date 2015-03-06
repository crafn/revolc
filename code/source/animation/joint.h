#ifndef REVOLC_ANIMATION_JOINT_H
#define REVOLC_ANIMATION_JOINT_H

#include "build.h"
#include "core/transform.h"
#include "resources/resource.h"

typedef U8 JointId;
#define NULL_JOINT_ID ((U8)-1)

typedef struct JointPose {
	/// Joint id in Armature
	JointId joint_id;

	/// Defines joints coordinate space relative to parent (superjoint, bind pose or entity, depends on situation)
	/// e.g. ({1, 0, 0}, Quaternion::constructByRotationAxis({0, 0, 1}, util::pi/2), 2)
	/// implies that this joint is 1 unit away from the super along its x-axis,
	/// subjoints rotated 90 degrees counterclockwise and double sized
	T3f tf;
} JointPose;

typedef struct Joint {
	char name[RES_NAME_SIZE];
	JointId id;
	JointId super_id;
	JointPose bind_pose;
} Joint;

#endif // REVOLC_ANIMATION_JOINT_H
