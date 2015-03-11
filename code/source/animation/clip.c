#include "armature.h"
#include "clip.h"
#include "core/malloc.h"
#include "resources/resblob.h"

int json_clip_to_blob(struct BlobBuf *buf, JsonTok j)
{
	int return_value= 0;
	T3f *samples= NULL;

	JsonTok j_armature= json_value_by_key(j, "armature");
	JsonTok j_duration= json_value_by_key(j, "duration");
	JsonTok j_channels= json_value_by_key(j, "channels");

	if (json_is_null(j_armature))
		RES_ATTRIB_MISSING("armature");
	if (json_is_null(j_duration))
		RES_ATTRIB_MISSING("duration");
	if (json_is_null(j_channels))
		RES_ATTRIB_MISSING("channels");

	const Armature *a=
		(Armature*)find_res_by_name_from_blobbuf(	buf,
													ResType_Armature,
													json_str(j_armature));
	if (!a) {
		critical_print("Couldn't find armature %s", json_str(j_armature));
		goto error;
	}

	const U32 fps= 30.0;
	const U32 joint_count= a->joint_count;
	const F32 duration= json_real(j_duration);
	const U32 frame_count= floor(duration*fps + 0.5) + 1; // +1 for end lerp target

	const U32 sample_count= joint_count*frame_count;
	samples= malloc(sizeof(*samples)*sample_count);
	for (U32 i= 0; i < sample_count; ++i)
		samples[i]= identity_t3f();

	for (U32 ch_i= 0; ch_i < json_member_count(j_channels); ++ch_i) {
		JsonTok j_ch= json_member(j_channels, ch_i);

		JsonTok j_joint= json_value_by_key(j_ch, "joint");
		JsonTok j_type= json_value_by_key(j_ch, "type");
		JsonTok j_keys= json_value_by_key(j_ch, "keys");

		if (json_is_null(j_joint))
			RES_ATTRIB_MISSING("joint");
		if (json_is_null(j_type))
			RES_ATTRIB_MISSING("type");
		if (json_is_null(j_keys))
			RES_ATTRIB_MISSING("keys");

		const char *type_str= json_str(j_type);
		JointId joint_id= joint_id_by_name(a, json_str(j_joint));

		enum {
			ChType_pos,
			ChType_rot,
			ChType_scale,
		} type;
		if (!strcmp(type_str, "pos"))
			type= ChType_pos;
		else if (!strcmp(type_str, "rot"))
			type= ChType_rot;
		else if (!strcmp(type_str, "scale"))
			type= ChType_scale;
		else
			fail("Unknown Clip channel type: %s", type_str);

		const U32 key_count= json_member_count(j_keys);
		if (key_count == 0)
			continue;

		U32 key_i= 0;
		U32 next_key_i= 0;
		F32 key_t= 0.0;
		F32 next_key_t= 0.0;
		union {
			V3f pos;
			Qf rot;
			V3f scale;
		} key_value, next_key_value;
		bool first= true;
		for (U32 frame_i= 0; frame_i < frame_count; ++frame_i) {
			const F32 frame_t= frame_i*duration/(frame_count - 1);

			// Update cur/next keys
			if (first || frame_t > next_key_t) {
				if (first) {
					first= false;
					key_i= 0;
					next_key_i= 1;
				} else {
					++key_i;
					++next_key_i;
				}
				if (key_i >= key_count)
					key_i= key_count - 1;
				if (next_key_i >= key_count)
					next_key_i= key_count - 1;

				JsonTok j_key= json_member(j_keys, key_i);
				JsonTok j_next_key= json_member(j_keys, next_key_i);

				JsonTok j_value= json_value_by_key(j_key, "v");
				JsonTok j_next_value= json_value_by_key(j_next_key, "v");

				key_t= json_real(json_value_by_key(j_key, "t"));
				next_key_t= json_real(json_value_by_key(j_next_key, "t"));

				switch (type) {
					case ChType_pos:
						key_value.pos= v3d_to_v3f(json_v3(j_value));
						next_key_value.pos= v3d_to_v3f(json_v3(j_next_value));
					break;
					case ChType_rot:
						key_value.rot= qd_to_qf(json_q(j_value));
						next_key_value.rot= qd_to_qf(json_q(j_next_value));
					break;
					case ChType_scale:
						key_value.scale= v3d_to_v3f(json_v3(j_value));
						next_key_value.scale= v3d_to_v3f(json_v3(j_next_value));
					break;
					default: fail("Unhandled ChType: %i", type);
				}
			}

			// Write samples
			const U32 s_i= frame_i*joint_count + joint_id;
			ensure(s_i < sample_count);
			const F32 lerp= (frame_t - key_t)/(next_key_t - key_t);
			switch (type) {
				case ChType_pos:
					samples[s_i].pos=
						lerp_v3f(key_value.pos, next_key_value.pos, lerp);
				break;
				case ChType_rot:
					/// @todo qlerp?
					samples[s_i].rot=
						normalized_qf(
								lerp_qf(key_value.rot, next_key_value.rot, lerp));
				break;
				case ChType_scale:
					samples[s_i].scale=
						lerp_v3f(key_value.scale, next_key_value.scale, lerp);
				break;
				default: fail("Unhandled ChType: %i", type);
			}
		}
	}

	blob_write(buf, &duration, sizeof(duration));
	blob_write(buf, &joint_count, sizeof(joint_count));
	blob_write(buf, &frame_count, sizeof(frame_count));
	blob_write(buf, samples, sizeof(*samples)*sample_count);

cleanup:
	free(samples);
	return return_value;

error:
	return_value= 1;
	goto cleanup;
}
