#include "core/file.h"
#include "core/string.h"
#include "resources/resblob.h"
#include "sound.h"

#ifndef CODEGEN
	// Ugggghh
#	undef internal
#		include <vorbis/codec.h>
#	define internal static
#endif

F32 *malloc_decoded_ogg_vorbis(U32 *frame_count, U32 *ch_count, U8 *ogg_data, U32 ogg_data_size)
{
	vorbis_info info;

	{ // Make sure that data is ogg vorbis
		ogg_stream_state streamstate;
		ogg_sync_state syncstate;

		vorbis_comment comment;

		ogg_packet packet;
		ogg_page page;

		// Ogg checks

		const U32 buffer_size= 4096;
		ogg_sync_init(&syncstate);
		char* buffer= ogg_sync_buffer(&syncstate, buffer_size);
		U32 data_i= 0;
		for (U32 i= 0; i < buffer_size; ++i){
			buffer[i]= ogg_data[data_i];
			++data_i;
		}
		ogg_sync_wrote(&syncstate, buffer_size);

		if (ogg_sync_pageout(&syncstate, &page) != 1){
			fail("Data not ogg");
		}

		ogg_stream_init(&streamstate, ogg_page_serialno(&page));

		// Vorbis-specific

		vorbis_info_init(&info);
		vorbis_comment_init(&comment);

		if (ogg_stream_pagein(&streamstate, &page) < 0){
			fail("Error reading first page");
		}

		if (ogg_stream_packetout(&streamstate, &packet) != 1){
			fail("Error reading header packet");
		}

		if (vorbis_synthesis_headerin(&info, &comment, &packet) < 0){
			fail("Ogg doesn't contain Vorbis");
		}

		// It's vorbis!

		U32 audio_data_begin_i= 0;

		// Process vorbis header
		U32 i= 0;
		while (i < 3) {
			while (i < 3) {
				S32 ret= ogg_sync_pageout(&syncstate, &page);
				if (ret == 0)
					break; // Moar data needed

				if (ret == 1) {
					ogg_stream_pagein(&streamstate, &page);

					while (i < 3) {
						// Submit packets of vorbis header
						// 0: Info, 1: Comments, 2: Codebooks
						ret= ogg_stream_packetout(&streamstate, &packet);

						if (ret == 0)
							break;
						if (ret < 0)
							fail("Vorbis header is corrupt");

						audio_data_begin_i += packet.bytes;
						vorbis_synthesis_headerin(&info, &comment, &packet);
						++i;
					}
				}
			}
			buffer= ogg_sync_buffer(&syncstate, buffer_size);

			for (U32 a= 0; a < buffer_size && data_i < ogg_data_size; ++a){
				buffer[a]= ogg_data[data_i];
				++data_i;
			}

			ogg_sync_wrote(&syncstate, buffer_size);
		}

		// Comments from header
		char **ptr= comment.user_comments;
		while (*ptr){
			debug_print("Vorbis comment: %s", *ptr);
			++ptr;
		}
		debug_print("Channels: %d, Rate: %ld", info.channels, info.rate);
		debug_print("Encoded by: %s", comment.vendor);

		ogg_stream_clear(&streamstate);
		vorbis_comment_clear(&comment);
		ogg_sync_clear(&syncstate);
	}


	U32 samples_capacity= 1024*16;
	F32 *samples= malloc(sizeof(*samples)*samples_capacity);
	U32 total_sample_count= 0;

	{ // Decode sample data
		// Copy-pasted from old engine
		U32 dataSize= ogg_data_size;
		U32 dataIndex= 0;
		bool needsNewPage= true;
		bool needsNewPacket= true;
		bool needsNewBlock= true;
		bool eos= false;

		/// @todo Fix: changing this to bigger will make short sounds not to play
		const U32 pageSize= 1024;
			
		ogg_packet	oggPacket; // Raw data
		ogg_page	oggPage; // Contains packets
		ogg_sync_state oggSyncState;
		ogg_stream_state oggStreamState;
		
		vorbis_dsp_state vorbisDspState; // Decoder state
		vorbis_block vorbisBlock;

		ogg_sync_init(&oggSyncState);

		const U32 request_count= U32_MAX; // Every sample
		U32 frames_decoded= 0;
		while (frames_decoded < request_count && !eos){
			bool first_time= dataIndex == 0;

			U32 data_submitted= 0;
			if (needsNewPage){
				// Submit more data
				while (ogg_sync_pageout(&oggSyncState, &oggPage) != 1){
					ensure(dataIndex <= dataSize);

					if (dataIndex == dataSize){
						//print(debug::Ch::Audio, debug::Vb::Trivial, "All data written to ogg stream");
						break;
					}

					char* buf= ogg_sync_buffer(&oggSyncState, pageSize);
					ensure(buf);

					U32 written=0;
					for (U32 i=0; i<pageSize && dataIndex < dataSize; ++i){
						buf[i]= ogg_data[dataIndex];
						++dataIndex;
						++written;
					}

					U32 ret= ogg_sync_wrote(&oggSyncState, written);
					ensure(ret == 0);

					data_submitted += written;
				}

				if (data_submitted != 0)
					needsNewPage= false;

				if (first_time){
					U32 ret=0;
					ret= ogg_stream_init(&oggStreamState, ogg_page_serialno(&oggPage));
					ensure(!ret);
					ret= vorbis_synthesis_init(&vorbisDspState, &info);
					ensure(!ret);
					ret= vorbis_block_init(&vorbisDspState, &vorbisBlock);
					ensure(!ret);
				}
				ogg_stream_pagein(&oggStreamState, &oggPage);
			}

			ensure(oggPage.body);
			if (	needsNewPage &&
					ogg_stream_eos(&oggStreamState) &&
					ogg_page_eos(&oggPage) &&
					dataIndex == dataSize){
				eos= true;
				break;
			}

			while(frames_decoded < request_count){
				if (needsNewPacket){
					S32 result= ogg_stream_packetout(&oggStreamState, &oggPacket);

					if (result == 0){
						needsNewPage= true; // Next page
						break; 
					}
					else if (result < 0){
						fail("Invalid/corrupt ogg packet");
					}
					else {
						needsNewPacket= false;
						needsNewBlock= true;
					}
				}

				if (!needsNewPacket) {
					// Process the packet
					if (needsNewBlock){
						S32 result=0;
						// Packet to block
						if ((result= vorbis_synthesis(&vorbisBlock, &oggPacket)) == 0){
							vorbis_synthesis_blockin(&vorbisDspState, &vorbisBlock);
							needsNewBlock= false;
						}
						else {
							if (result == OV_ENOTAUDIO){
								// Skip the header
							}
							else {
								fail("Error in synthesis: %i", result);
							}
						}
					}

					F32 **rawsamples;
					while (frames_decoded < request_count){
						U32 frame_count=0; // Sample count of each channel
						frame_count= vorbis_synthesis_pcmout(&vorbisDspState, &rawsamples);

						if (frame_count <= 0){
							// New packet needed, not block, because packet is converted to a block at once
							needsNewPacket= true;
							break;
						}

						U32 frames_to_read= frame_count; // Sample count of one channel
						ensure(frames_decoded < request_count);
						if (frames_to_read + frames_decoded >= request_count){
							frames_to_read = request_count - frames_decoded;
						}

						if (samples_capacity < total_sample_count + frames_to_read*info.channels) {
							samples_capacity *= 2;
							samples= realloc(samples, sizeof(*samples)*samples_capacity);
						}

						U32 begin_frame= total_sample_count/info.channels;
						for (U32 ch_i= 0; ch_i < info.channels; ++ch_i) {
							F32* mono= rawsamples[ch_i];
							for (U32 i= 0; i < frames_to_read; ++i){
								U32 frame= begin_frame + i;
								samples[frame*info.channels + ch_i]= mono[i];
								++total_sample_count;
							}
						}

						frames_decoded += frames_to_read;
						vorbis_synthesis_read(&vorbisDspState, frames_to_read);
					}
				}
			}
			ensure(dataIndex <= dataSize);
		}

		ogg_stream_clear(&oggStreamState);
		vorbis_block_clear(&vorbisBlock);
		vorbis_dsp_clear(&vorbisDspState);
		ogg_sync_clear(&oggSyncState);
	}

	*ch_count= info.channels;
	*frame_count= total_sample_count/info.channels;

	vorbis_info_clear(&info);
	return samples;
}

int json_sound_to_blob(struct BlobBuf *buf, JsonTok j)
{
	U8 *file_contents= NULL;
	F32 *samples= NULL;
	int return_value= 0;

	JsonTok j_file= json_value_by_key(j, "file");
	if (json_is_null(j_file)) {
		RES_ATTRIB_MISSING("file");
		goto error;
	}

	char total_path[MAX_PATH_SIZE];
	joined_path(total_path, j.json_path, json_str(j_file));

	U32 file_size;
	file_contents= malloc_file(total_path, &file_size);

	U32 ch_count;
	U32 frame_count;
	samples= malloc_decoded_ogg_vorbis(	&frame_count, &ch_count,
										file_contents, file_size);

	blob_write(buf, &ch_count, sizeof(ch_count));
	blob_write(buf, &frame_count, sizeof(frame_count));
	blob_write(buf, samples, sizeof(*samples)*ch_count*frame_count);

cleanup:
	free(samples);
	free(file_contents);
	return return_value;

error:
	return_value= 1;
	goto cleanup;
}
