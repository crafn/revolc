#ifndef REVOLC_ANIMATION_ARMATURE_H
#define REVOLC_ANIMATION_ARMATURE_H

#include "build.h"
#include "core/json.h"
#include "global/cfg.h"
#include "joint.h"
#include "resources/resource.h"

typedef struct Armature {
	Resource res;
	Joint joints[MAX_ARMATURE_JOINT_COUNT];
	U32 joint_count;
} Armature;

REVOLC_API WARN_UNUSED
int json_armature_to_blob(struct BlobBuf *buf, JsonTok j);

#endif // REVOLC_ANIMATION_ARMATURE_H
