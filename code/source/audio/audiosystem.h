#ifndef REVOLC_AUDIO_AUDIOSYSTEM_H
#define REVOLC_AUDIO_AUDIOSYSTEM_H

#include "build.h"
#include "global/cfg.h"

#include <portaudio.h>

typedef enum {
	AC_free,  // No audio is assigned for this channel
	AC_play,  // Audio callback is pushing samples to be played
} AudioChannel_State;

typedef struct AudioChannel {
	AudioChannel_State state;
	F32 *samples; // Interleaved samples
	U32 ch_count;
	U32 frame_count; // sample_count/ch_count

	// Modified by non-critical code
	F32 in_pan;
	F32 in_vol;

	// These belong to the audio callback
	F32 out_pan;
	F32 out_vol;
	U32 cur_frame;
} AudioChannel;

typedef struct AudioSystem {
	AudioChannel channels[MAX_AUDIO_CHANNELS];

	PaStream *pa_out_stream;
} AudioSystem;

/// @note Sets g_env.audiosystem
REVOLC_API void create_audiosystem();
REVOLC_API void destroy_audiosystem();

REVOLC_API void play_sound(const char *name);

#endif // REVOLC_AUDIO_AUDIOSYSTEM_H
