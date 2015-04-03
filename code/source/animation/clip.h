#ifndef REVOLC_ANIMATION_CLIP_H
#define REVOLC_ANIMATION_CLIP_H

#include "build.h"
#include "core/json.h"
#include "joint.h"
#include "resources/resource.h"

typedef enum {
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

	// @todo #if away from release build -- only for editor
	BlobOffset keys_offset; // Clip_Key[]
	U32 key_count;

	U32 joint_count;
	U32 frame_count; // Last frame is only interpolation target
	BlobOffset local_samples_offset; // joint_count*frame_count elements, T3f
} PACKED Clip;

REVOLC_API U32 clip_sample_count(const Clip *c);
REVOLC_API T3f * clip_local_samples(const Clip *c);
REVOLC_API Clip_Key * clip_keys(const Clip *c);

REVOLC_API WARN_UNUSED
int json_clip_to_blob(struct BlobBuf *buf, JsonTok j);

REVOLC_API JointPoseArray calc_clip_pose(const Clip *c, F64 t);

// Creates modifiable substitute for Clip resource
Clip *create_rt_clip(Clip *src);
// Add or update
void update_rt_clip_key(Clip *c, Clip_Key key);

#endif // REVOLC_ANIMATION_CLIP_H
