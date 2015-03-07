#ifndef REVOLC_ANIMATION_CLIP_H
#define REVOLC_ANIMATION_CLIP_H

#include "build.h"
#include "core/json.h"
#include "resources/resource.h"

/*
typedef enum {
	pos,
	rot,
	scale,
} ClipChannelType;

struct ClipChannel {
	ClipChannelType type;
	char joint[RES_NAME_SIZE];
	struct {
		F32 time;
		union {
			V3f pos;
			Qf rot;
			V3f scale;
		};
	} key;
	util::DynArray<Key> keys;
};
*/

typedef struct Clip {
	Resource res;
	F32 fps;
	U32 joint_count;
	U32 frame_count;
	T3f local_samples[]; // joint_count*frame_count elements
} Clip;

REVOLC_API WARN_UNUSED
int json_clip_to_blob(struct BlobBuf *buf, JsonTok j);

#endif // REVOLC_ANIMATION_CLIP_H
