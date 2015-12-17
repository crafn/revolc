#ifndef REVOLC_VISUAL_COMPOUNDENTITY_H
#define REVOLC_VISUAL_COMPOUNDENTITY_H

#include "animation/armature.h"
#include "animation/joint.h"
#include "compdef.h"
#include "build.h"
#include "core/math.h"

typedef enum {
	VEntityType_model,
	VEntityType_comp,
} VEntityType;

typedef struct SubEntity {
	T3f offset; // Relative to joint
	U32 handle; // Owned handle to the entity
	VEntityType type;
	JointId joint_id;
} SubEntity;

// Compound entity
// Contains other entities positioned by armature
typedef struct CompEntity {
	char def_name[RES_NAME_SIZE];
	T3d tf;
	bool allocated;

	JointPoseArray pose; // Relative to bind offsets

	// Local
	T3d smoothing_delta;
	F32 smoothing_phase; // 1 at start of smoothing, 0 when done 

	// Cached
	Armature *armature;
	SubEntity subs[MAX_SUBENTITY_COUNT];
	U8 sub_count;
} CompEntity;

REVOLC_API void init_compentity(CompEntity *data);
struct WArchive;
struct RArchive;
REVOLC_API void pack_compentity(	struct WArchive *ar,
									const CompEntity *begin,
									const CompEntity *end);
REVOLC_API void unpack_compentity(	struct RArchive *ar,
									CompEntity *begin,
									CompEntity *end);

REVOLC_API SubEntity create_subentity(const Armature *a, CompDef_Sub sub);
REVOLC_API void destroy_subentity(SubEntity e);

REVOLC_API void calc_global_pose(T3d *global_pose, const CompEntity *e);

#endif // REVOLC_VISUAL_COMPOUNDENTITY_H
