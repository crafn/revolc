#ifndef REVOLC_ANIMATION_ARMATURE_H
#define REVOLC_ANIMATION_ARMATURE_H

#include "build.h"
#include "core/json.h"
#include "global/cfg.h"
#include "joint.h"
#include "resources/resource.h"

struct Armature;
typedef struct ArmaturePose {
	const struct Armature* armature;
	JointPose local_in_bind_pose[MAX_ARMATURE_JOINT_COUNT];
} ArmaturePose;

typedef struct Armature {
	Resource res;
	char joint_names[MAX_ARMATURE_JOINT_COUNT][RES_NAME_SIZE];
	Joint joints[MAX_ARMATURE_JOINT_COUNT];
	U32 joint_count;
	ArmaturePose bind_pose;
} Armature;

REVOLC_API WARN_UNUSED
int json_armature_to_blob(struct BlobBuf *buf, JsonTok j);

#endif // REVOLC_ANIMATION_ARMATURE_H
