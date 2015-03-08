#ifndef REVOLC_ANIMATION_ARMATURE_H
#define REVOLC_ANIMATION_ARMATURE_H

#include "build.h"
#include "core/json.h"
#include "global/cfg.h"
#include "joint.h"
#include "resources/resource.h"

typedef struct Armature {
	Resource res;
	char joint_names[MAX_ARMATURE_JOINT_COUNT][RES_NAME_SIZE];
	Joint joints[MAX_ARMATURE_JOINT_COUNT];
	U32 joint_count;
} PACKED Armature;

REVOLC_API WARN_UNUSED
int json_armature_to_blob(struct BlobBuf *buf, JsonTok j);

REVOLC_API
JointId joint_id_by_name(const Armature *a, const char *name);

#endif // REVOLC_ANIMATION_ARMATURE_H
