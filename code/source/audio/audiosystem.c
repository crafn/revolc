#include "audiosystem.h"
#include "core/debug_print.h"
#include "core/ensure.h"
#include "core/malloc.h"
#include "global/cfg.h"
#include "global/env.h"
#include "sound.h"

#include "resources/resblob.h" // test

#include <string.h>

internal
int audio_callback(	const void* input_data, void* output_data,
					unsigned long out_frame_count,
					const PaStreamCallbackTimeInfo* time_info,
					PaStreamCallbackFlags status_flags,
					void* user_data){
	F32* out= output_data;
	AudioChannel *chs= user_data;

	for (U32 i= 0; i < out_frame_count; ++i){
		out[2*i]= 0.0;
		out[2*i + 1]= 0.0;
	}

	// TEST
	local_persist int beep_i= 0;
	Sound *s= (Sound*)res_by_name(g_env.resblob, ResType_Sound, "dev_beep0");
	ensure(s->ch_count == 2);
	for (int i= 0; i < out_frame_count; ++i) {
		out[2*i]= s->samples[beep_i*2];
		out[2*i + 1]= s->samples[beep_i*2 + 1];
		beep_i= (beep_i + 1) % s->frame_count;
	}

	for (U32 ch_i= 0; ch_i < MAX_AUDIO_CHANNELS; ++ch_i) {
		AudioChannel *ch= &chs[ch_i];
		if (ch->state != AC_play)
			continue;

		ensure(ch->ch_count == 2 && "@todo mono");

		bool going_to_finish= false;
		U32 frame_count= out_frame_count;
		if (frame_count + ch->cur_frame > ch->frame_count) {
			frame_count= ch->frame_count - ch->cur_frame;
			going_to_finish= true;
		}

		for (U32 f_i= 0; f_i < out_frame_count; ++f_i) {
			U32 l_i= f_i*2;
			U32 r_i= f_i*2 + 1;

			out[l_i] += ch->samples[l_i];
			out[r_i] += ch->samples[r_i];
			++ch->cur_frame;
		}

		if (going_to_finish) {
			/// @todo MEMORY FENCE
			ch->state= AC_play_finished;
		}
	}
	return paContinue;
}

void create_audiosystem()
{
	AudioSystem *a= zero_malloc(sizeof(*a));
	ensure(!g_env.audiosystem);
	g_env.audiosystem= a;

	// Default outputstream params
	PaStreamParameters out_params;
	out_params.channelCount= 2;
	out_params.hostApiSpecificStreamInfo= NULL;
	out_params.sampleFormat= paFloat32;
	out_params.suggestedLatency= 0.05;

	PaError err= Pa_Initialize();

	if(err != paNoError) {
		fail("create_audiosystem(): PortAudio init failed: %s",
				Pa_GetErrorText(err));
	}

	S32 host_api_count= Pa_GetHostApiCount();
	if (host_api_count < 1)
		fail("PortAudio host api count: %i", host_api_count);

	// Look for all audio devices
	S32 num_devices= Pa_GetDeviceCount();
	if(num_devices < 0) {
		fail("create_audiosystem(): Pa_GetDeviceCount failed: %s",
				Pa_GetErrorText(num_devices));
	} else if (num_devices == 0) {
		fail("No audio devices");
	} else {
		S32 picked_device_i= -1;
		debug_print("Available audio devices:");
		for(S32 i= 0; i < num_devices; i++) {
			const PaDeviceInfo *device_info= Pa_GetDeviceInfo(i);
			bool supported= false;
			if (!strcmp(device_info->name, "dmix")) {
				supported= false; // Caused flood of alsa error messages
			} else {
				PaStreamParameters p= out_params;
				p.device= i;
				PaError ret= Pa_IsFormatSupported(0, &p, AUDIO_SAMPLE_RATE);
				if(ret == paFormatIsSupported) {
					picked_device_i= i;
					supported= true;
				}
			}

			debug_print("  %s", device_info->name);
			debug_print("    low: %f", device_info->defaultLowOutputLatency);
			debug_print("    high: %f", device_info->defaultHighOutputLatency);
			debug_print("    supported: %i", supported);
		}

		out_params.device= picked_device_i;

		if (picked_device_i == -1)
			fail("Sufficient audio device not found");

		const PaDeviceInfo *device_info= Pa_GetDeviceInfo(picked_device_i);
		debug_print("-> %s", device_info->name);
	}

	// Create output stream
	PaStream *out_stream;
	err= Pa_OpenStream(
			  &out_stream,
			  NULL,
			  &out_params,
			  AUDIO_SAMPLE_RATE,
			  AUDIO_BUFFER_SIZE,
			  paNoFlag,
			  &audio_callback,
			  a->channels);
	if(err != paNoError) {
		debug_print("create_audiosystem(): Pa_OpenStream failed: %s",
				Pa_GetErrorText(err));
	}

	err= Pa_StartStream(out_stream);
	if(err != paNoError) {
		debug_print("create_audiosystem(): Pa_StartStream failed: %s",
				Pa_GetErrorText(err));
	}

	a->pa_out_stream= out_stream;
}

void destroy_audiosystem()
{
	AudioSystem *a= g_env.audiosystem;

	{ // Shutdown stream
		PaError err= Pa_StopStream(a->pa_out_stream);
		if(err != paNoError) {
			debug_print("destroy_audiosystem(): PortAudio stream stop failed: %s",
					Pa_GetErrorText(err));
		}

		err= Pa_CloseStream(a->pa_out_stream);
		if(err != paNoError) {
			debug_print("destroy_audiosystem(): PortAudio stream close failed: %s",
					Pa_GetErrorText(err));
		}
	}

	{ // Shutdown PortAudio
		PaError err= Pa_Terminate();
		if(err != paNoError) {
			fail("AudioDevice::shutdown(): PortAudio terminate failed: %s",
					Pa_GetErrorText(err));
		}
	}

	free(g_env.audiosystem);
	g_env.audiosystem= NULL;
}
