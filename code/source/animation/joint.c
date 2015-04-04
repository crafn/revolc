#include "joint.h"

JointPoseArray identity_pose()
{
	JointPoseArray pose;
	for (U32 i= 0; i < MAX_ARMATURE_JOINT_COUNT; ++i)
		pose.tf[i]= identity_t3f();
	return pose;
}

JointPoseArray lerp_pose(	JointPoseArray p1,
							JointPoseArray p2,
							F64 t)
{
	JointPoseArray p;
	for (U32 i= 0; i < MAX_ARMATURE_JOINT_COUNT; ++i)
		p.tf[i]= lerp_t3f(p1.tf[i], p2.tf[i], t);
	return p;
}
