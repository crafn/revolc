#include "audiosystem.h"
#include "core/debug_print.h"
#include "core/ensure.h"
#include "core/malloc.h"
#include "global/cfg.h"
#include "global/env.h"
#include "platform/memory.h"
#include "platform/stdlib.h"
#include "resources/resblob.h"
#include "sound.h"

#ifndef CODEGEN
#	include <portaudio.h>
#endif

internal inline
void upd_smoothed(F32 *var, F32 target)
{
	const F32 delta= 0.005;
	if (abs(*var - target) <= delta)
		*var= target;
	else if (*var < target)
		*var += delta;
	else if (*var > target)
		*var -= delta;
}

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

	for (U32 ch_i= 0; ch_i < MAX_AUDIO_CHANNELS; ++ch_i) {
		AudioChannel *ch= &chs[ch_i];
		if (ch->state != AC_play)
			continue;

		ensure(ch->ch_count == 2 && "@todo mono sounds");

		bool going_to_finish= false;
		U32 frame_count= out_frame_count;
		if (frame_count + ch->cur_frame > ch->frame_count) {
			frame_count= ch->frame_count - ch->cur_frame;
			going_to_finish= true;
		}

		U32 frame_offset= ch->cur_frame;
		for (U32 f_i= 0; f_i < frame_count; ++f_i) {
			upd_smoothed(&ch->out_pan, ch->in_pan);
			upd_smoothed(&ch->out_vol, ch->in_vol);

			U32 l_i= (frame_offset + f_i)*2;
			U32 r_i= (frame_offset + f_i)*2 + 1;

			F32 l_mul= ch->out_vol*MIN(1.0 - ch->out_pan, 1.0);
			F32 r_mul= ch->out_vol*MIN(1.0 + ch->out_pan, 1.0);

			out[f_i*2] += ch->samples[l_i]*l_mul;
			out[f_i*2 + 1] += ch->samples[r_i]*r_mul;

			++ch->cur_frame;
		}

		if (going_to_finish) {
			ch->samples= NULL;

			/// @todo Full barrier not necessary 
			PLAT_FULL_MEMORY_BARRIER();
			ch->state= AC_free;
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
		for(S32 i= 0; i < num_devices; ++i) {
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

internal
SoundHandle sound_handle(U32 sound_id, U32 channel)
{ return sound_id*MAX_AUDIO_CHANNELS + channel; }

SoundHandle play_sound(const char *name, F32 vol, F32 pan)
{
	AudioSystem *a= g_env.audiosystem;
	Sound *s= (Sound*)res_by_name(g_env.resblob, ResType_Sound, name);

	U32 channel_i= 0;
	while (	channel_i < MAX_AUDIO_CHANNELS &&
			a->channels[channel_i].state != AC_free)
		++channel_i;

	if (channel_i == MAX_AUDIO_CHANNELS) {
		debug_print("play_sounds: too many sounds");
		return NULL_SOUND_HANDLE;
	}

	U32 sound_id= ++a->next_sound_id;
	{ // Start playing
		AudioChannel *ch= &a->channels[channel_i];
		*ch= (AudioChannel) {
			.state= AC_free,
			.samples= s->samples,
			.ch_count= s->ch_count,
			.frame_count= s->frame_count,
			.last_id= sound_id,
			.last_sound_name= name,
			.in_vol= vol,
			.out_vol= vol,
			.in_pan= pan,
			.out_pan= pan,
		};

		/// @todo Full barrier not necessary 
		PLAT_FULL_MEMORY_BARRIER();
		ch->state= AC_play;
	}

	return sound_handle(sound_id, channel_i);
}

SoundHandle sound_handle_by_name(const char *name)
{
	AudioSystem *a= g_env.audiosystem;
	for (U32 i= 0; i < MAX_AUDIO_CHANNELS; ++i) {
		if (a->channels[i].state != AC_play)
			continue;
		if (!strcmp(a->channels[i].last_sound_name, name))
			return sound_handle(a->channels[i].last_id, i);
	}
	return NULL_SOUND_HANDLE;
}

bool is_sound_playing(SoundHandle h)
{
	AudioSystem *a= g_env.audiosystem;
	U32 channel_i= h % MAX_AUDIO_CHANNELS;
	U32 sound_id= h/MAX_AUDIO_CHANNELS;
	return	a->channels[channel_i].state == AC_play &&
			a->channels[channel_i].last_id == sound_id;
}

void set_sound_vol(SoundHandle h, F32 vol)
{
	AudioSystem *a= g_env.audiosystem;
	U32 channel_i= h % MAX_AUDIO_CHANNELS;
	U32 sound_id= h/MAX_AUDIO_CHANNELS;
	if (	a->channels[channel_i].state != AC_play ||
			a->channels[channel_i].last_id != sound_id)
		return;
	a->channels[channel_i].in_vol= vol;
}
