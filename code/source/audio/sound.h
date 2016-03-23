#ifndef REVOLC_SOUND_H
#define REVOLC_SOUND_H

#include "build.h"
#include "core/json.h"
#include "resources/resource.h"

typedef struct Sound {
	Resource res;
	char rel_file[MAX_PATH_SIZE];

	U32 ch_count;
	U32 frame_count; // sample_count/ch_count
	F32 samples[]; // Interleaved samples
} PACKED Sound;

REVOLC_API WARN_UNUSED
int json_sound_to_blob(struct BlobBuf *buf, JsonTok j);

REVOLC_API WARN_UNUSED
Sound *blobify_sound(struct WArchive *ar, Cson c, bool *err);
REVOLC_API void deblobify_sound(WCson *c, struct RArchive *ar);

#endif // REVOLC_SOUND_H
