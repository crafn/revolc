#ifndef REVOLC_AUDIO_AUDIOSYSTEM_H
#define REVOLC_AUDIO_AUDIOSYSTEM_H

#include "build.h"
#include "global/cfg.h"

typedef U64 SoundHandle;
#define NULL_SOUND_HANDLE 0

typedef enum {
	AC_free,  // No audio is assigned for this channel
	AC_play,  // Audio callback is pushing samples to be played
} AudioChannel_State;

typedef struct AudioChannel {
	AudioChannel_State state;
	F32 *samples; // Interleaved samples
	U32 ch_count;
	U32 frame_count; // sample_count/ch_count
	U32 last_id;
	const char *last_sound_name;

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
	U32 next_sound_id;

	void *pa_out_stream;
} AudioSystem;

/// @note Sets g_env.audiosystem
REVOLC_API void create_audiosystem();
REVOLC_API void destroy_audiosystem();

REVOLC_API SoundHandle play_sound(const char *name, F32 vol, F32 pan);
REVOLC_API SoundHandle sound_handle_by_name(const char *name);
REVOLC_API bool is_sound_playing(SoundHandle h);
REVOLC_API void set_sound_vol(SoundHandle h, F32 vol);

#endif // REVOLC_AUDIO_AUDIOSYSTEM_H
