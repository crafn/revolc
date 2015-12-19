#ifndef REVOLC_ANIMATION_CLIP_H
#define REVOLC_ANIMATION_CLIP_H

#include "build.h"
#include "core/json.h"
#include "core/basic.h"
#include "joint.h"
#include "resources/resource.h"

typedef enum {
	Clip_Key_Type_none,
	Clip_Key_Type_scale,
	Clip_Key_Type_rot,
	Clip_Key_Type_pos,
} Clip_Key_Type;

typedef union Clip_Key_Value {
	V3f scale;
	Qf rot;
	V3f pos;
} Clip_Key_Value;

typedef struct Clip_Key {
	JointId joint_id;
	F64 time;

	Clip_Key_Type type;
	Clip_Key_Value value;
} Clip_Key;

typedef struct Clip {
	Resource res;

	F32 duration; // A the beginning of the last frame

	// @todo #if away from release build -- only for editor/saving
	char armature_name[RES_NAME_SIZE];
	REL_PTR(Clip_Key) keys;
	U32 key_count;

	U32 joint_count;
	U32 frame_count; // Last frame is only interpolation target
	REL_PTR(T3f) local_samples; // joint_count*frame_count elements
} PACKED Clip;

REVOLC_API U32 clip_sample_count(const Clip *c);
REVOLC_API T3f * clip_local_samples(const Clip *c);
REVOLC_API Clip_Key * clip_keys(const Clip *c);

REVOLC_API WARN_UNUSED
int json_clip_to_blob(struct BlobBuf *buf, JsonTok j);
REVOLC_API
void clip_to_json(WJson *j, const Clip *c);

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
