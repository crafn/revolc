#ifndef REVOLC_ANIMATION_CLIP_H
#define REVOLC_ANIMATION_CLIP_H

#include "build.h"
#include "core/json.h"
#include "joint.h"
#include "resources/resource.h"

// For editor
typedef enum {
	Clip_Key_Ch_scale,
	Clip_Key_Ch_rot,
	Clip_Key_Ch_pos,
} Clip_Key_Ch;

// For editor
struct Clip_Key {
	JointId joint_id;
	F64 time;

	Clip_Key_Ch ch;
	union {
		V3f scale;
		Qf rot;
		V3f pos;
	} value;
};

typedef struct Clip {
	Resource res;
	F32 duration; // A the beginning of the last frame

	// @todo #if away from release build
	BlobOffset keys_offset; // Clip_Key[]
	U32 key_count;

	U32 joint_count;
	U32 frame_count; // Last frame is only interpolation target
	BlobOffset local_samples_offset; // joint_count*frame_count elements, T3f
} PACKED Clip;

REVOLC_API T3f * local_samples(const Clip *c);

REVOLC_API WARN_UNUSED
int json_clip_to_blob(struct BlobBuf *buf, JsonTok j);

REVOLC_API JointPoseArray calc_clip_pose(const Clip *c, F64 t);

#endif // REVOLC_ANIMATION_CLIP_H
