#include "audiosystem.h"
#include "core/debug_print.h"
#include "core/ensure.h"
#include "core/malloc.h"
#include "global/cfg.h"
#include "global/env.h"

#include <string.h>

internal
int audio_callback(	const void* input_data, void* output_data,
					unsigned long out_frame_count,
					const PaStreamCallbackTimeInfo* time_info,
					PaStreamCallbackFlags status_flags,
					void* user_data){
	F32* out= output_data;

	for (U32 i= 0; i < out_frame_count; ++i){
		out[2*i]= 0.0;
		out[2*i + 1]= 0.0f;
	}
	return 0;
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

	// PortAudio initialization
	{
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
			debug_print("Available audio devices:");
			for(S32 i= 0; i < num_devices; i++) {
				const PaDeviceInfo *device_info= Pa_GetDeviceInfo(i);
				debug_print("  %s", device_info->name);
				debug_print("    Low latency: %f", device_info->defaultLowOutputLatency);
				debug_print("    High latency: %f", device_info->defaultHighOutputLatency);
			}

			// Find supported devices
			S32 picked_device_i= -1;
			for (S32 i= 0; i < num_devices; i++) {
				const PaDeviceInfo *device_info= Pa_GetDeviceInfo(i);
				out_params.device= i;

				if (!strcmp(device_info->name, "dmix"))
					continue; // Caused flood of alsa error messages

				err= Pa_IsFormatSupported(0, &out_params, AUDIO_SAMPLE_RATE);
				if(err == paFormatIsSupported) {
					debug_print("Supported device: %s", device_info->name);
					picked_device_i= i;
				}
			}

			if (picked_device_i == -1)
				fail("Sufficient audio device not found");

			const PaDeviceInfo *device_info= Pa_GetDeviceInfo(picked_device_i);
			debug_print("-> %s", device_info->name);
		}
	}

	{ // Output stream
		PaStream *out_stream;
		PaError err = Pa_OpenStream(
				  &out_stream,
				  NULL,
				  &out_params,
				  AUDIO_SAMPLE_RATE,
				  AUDIO_BUFFER_SIZE,
				  paNoFlag,
				  &audio_callback,
				  NULL);
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
