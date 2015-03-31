#include "joint.h"

JointPoseArray identity_pose()
{
	JointPoseArray pose;
	for (U32 i= 0; i < MAX_ARMATURE_JOINT_COUNT; ++i)
		pose.tf[i]= identity_t3f();
	return pose;
}
