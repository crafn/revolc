#ifndef REVOLC_ANIMATION_CLIP_H
#define REVOLC_ANIMATION_CLIP_H

#include "build.h"
#include "core/cson.h"
#include "core/basic.h"
#include "joint.h"
#include "resources/resource.h"

typedef enum {
	Clip_Key_Type_none,
	Clip_Key_Type_scale,
	Clip_Key_Type_rot,
	Clip_Key_Type_pos,
} Clip_Key_Type;

typedef struct Clip_Key {
	char joint_name[RES_NAME_SIZE];
	U8 joint_ix; // Index in clip, not in armature
	F64 time;

	Clip_Key_Type type;
	T3f value;
} Clip_Key;

typedef struct Clip {
	Resource res;

	F32 duration; // At the beginning of the last frame

	char armature_name[RES_NAME_SIZE];
	REL_PTR(Clip_Key) keys;
	U32 key_count;

	// Clip joint index to armature joint id
	JointId joint_ix_to_id[MAX_ARMATURE_JOINT_COUNT];

	U32 joint_count; // Can be less than armature->joint_count
	U32 frame_count; // Last frame is only interpolation target
	REL_PTR(T3f) local_samples; // joint_count*frame_count elements
} PACKED Clip;

struct Armature;
REVOLC_API struct Armature *clip_armature(const Clip *c);
REVOLC_API U32 clip_sample_count(const Clip *c);
REVOLC_API T3f * clip_local_samples(const Clip *c);
REVOLC_API Clip_Key * clip_keys(const Clip *c);

REVOLC_API void init_clip(Clip *clip);

struct WArchive;
struct RArchive;

REVOLC_API WARN_UNUSED Clip *blobify_clip(struct WArchive *ar, Cson c, bool *err);
REVOLC_API void deblobify_clip(WCson *c, struct RArchive *ar);

REVOLC_API JointPoseArray calc_clip_pose(const Clip *c, F64 t);

//
// These all are kind of editor functions, but there's all kinds of caching
// (sorting keys, recalculating samples from keys) to be caren of so it's
//  maybe better that they're near Clip
//

// Add or update key
REVOLC_API void update_rt_clip_key(Clip *c, Clip_Key key);
REVOLC_API void delete_rt_clip_key(Clip *c, U32 del_i);
// Copy first keys of every channel to end
REVOLC_API void make_rt_clip_looping(Clip *c);
// Moves keys __OF_SELECTED_JOINTS__ (<- ugly) at `from` -> `to`
REVOLC_API void move_rt_clip_keys(Clip *c, F64 from, F64 to);

REVOLC_API void recache_ptrs_to_clips();

#endif // REVOLC_ANIMATION_CLIP_H
