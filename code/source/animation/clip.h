#ifndef REVOLC_ANIMATION_CLIP_H
#define REVOLC_ANIMATION_CLIP_H

#include "build.h"
#include "core/json.h"
#include "resources/resource.h"

typedef struct Clip {
	Resource res;
	F32 duration;
	U32 fps;
	U32 joint_count;
	U32 frame_count;
	T3f local_samples[]; // joint_count*frame_count elements
} PACKED Clip;

REVOLC_API WARN_UNUSED
int json_clip_to_blob(struct BlobBuf *buf, JsonTok j);

#endif // REVOLC_ANIMATION_CLIP_H
