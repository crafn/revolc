#ifndef REVOLC_AUDIO_AUDIOSYSTEM_H
#define REVOLC_AUDIO_AUDIOSYSTEM_H

#include "build.h"
#include <portaudio.h>

typedef struct OutChannel {

} OutChannel;

typedef struct AudioSystem {

	PaStream *pa_out_stream;
} AudioSystem;

/// @note Sets g_env.audiosystem
REVOLC_API void create_audiosystem();
REVOLC_API void destroy_audiosystem();

#endif // REVOLC_AUDIO_AUDIOSYSTEM_H
