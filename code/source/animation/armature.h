#ifndef REVOLC_ANIMATION_ARMATURE_H
#define REVOLC_ANIMATION_ARMATURE_H

#include "build.h"
#include "core/cson.h"
#include "global/cfg.h"
#include "joint.h"
#include "resources/resource.h"

typedef struct Armature {
	Resource res;
	char joint_names[MAX_ARMATURE_JOINT_COUNT][RES_NAME_SIZE];
	Joint joints[MAX_ARMATURE_JOINT_COUNT];
	U32 joint_count;
} PACKED Armature;

struct WArchive;
struct RArchive;

REVOLC_API WARN_UNUSED
Armature *blobify_armature(struct WArchive *ar, Cson c, bool *err);
REVOLC_API void deblobify_armature(WCson *c, struct RArchive *ar);

REVOLC_API
JointId joint_id_by_name(const Armature *a, const char *name);

#endif // REVOLC_ANIMATION_ARMATURE_H
