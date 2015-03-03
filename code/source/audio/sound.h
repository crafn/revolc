#ifndef REVOLC_SOUND_H
#define REVOLC_SOUND_H

#include "build.h"
#include "core/json.h"
#include "resources/resource.h"

typedef struct Sound {
	Resource res;

	U32 ch_count;
	U32 frame_count; // sample_count/ch_count
	F32 samples[]; // Interleaved samples
} PACKED Sound;

REVOLC_API WARN_UNUSED
int json_sound_to_blob(struct BlobBuf *buf, JsonTok j);

#endif // REVOLC_SOUND_H
