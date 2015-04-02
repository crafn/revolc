#include "armature.h"
#include "clip.h"
#include "core/array.h"
#include "core/malloc.h"
#include "resources/resblob.h"

U32 clip_sample_count(const Clip *c)
{ return c->joint_count*c->frame_count; }

T3f * clip_local_samples(const Clip *c)
{ return blob_ptr(&c->res, c->local_samples_offset); }

Clip_Key * clip_keys(const Clip *c)
{ return blob_ptr(&c->res, c->keys_offset); }

int json_clip_to_blob(struct BlobBuf *buf, JsonTok j)
{
	int return_value= 0;
	T3f *samples= NULL;
	const Armature *a= NULL;
	Clip_Key *keys= NULL;

	JsonTok j_armature= json_value_by_key(j, "armature");
	JsonTok j_duration= json_value_by_key(j, "duration");
	JsonTok j_channels= json_value_by_key(j, "channels");

	if (json_is_null(j_armature))
		RES_ATTRIB_MISSING("armature");
	if (json_is_null(j_duration))
		RES_ATTRIB_MISSING("duration");
	if (json_is_null(j_channels))
		RES_ATTRIB_MISSING("channels");

	a= (Armature*)find_res_by_name_from_blobbuf(	buf,
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

	U32 keys_capacity= 0;
	U32 total_key_count= 0;

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

		Clip_Key_Type type;
		if (!strcmp(type_str, "pos"))
			type= Clip_Key_Type_pos;
		else if (!strcmp(type_str, "rot"))
			type= Clip_Key_Type_rot;
		else if (!strcmp(type_str, "scale"))
			type= Clip_Key_Type_scale;
		else
			fail("Unknown Clip channel type: %s", type_str);

		const U32 key_count= json_member_count(j_keys);
		if (key_count == 0)
			continue;

		// Gather keys of the channel
		const U32 ch_begin_key_i= total_key_count;
		for (U32 key_i= 0; key_i < key_count; ++key_i) {
			JsonTok j_key= json_member(j_keys, key_i);
			JsonTok j_value= json_value_by_key(j_key, "v");
			Clip_Key key= {
				.joint_id= joint_id,
				.type= type,
			};
			key.time= json_real(json_value_by_key(j_key, "t"));

			switch (type) {
				case Clip_Key_Type_pos:
					key.value.pos= v3d_to_v3f(json_v3(j_value));
				break;
				case Clip_Key_Type_rot:
					key.value.rot= qd_to_qf(json_q(j_value));
				break;
				case Clip_Key_Type_scale:
					key.value.scale= v3d_to_v3f(json_v3(j_value));
				break;
				default: fail("Unhandled ChType: %i", type);
			}

			keys= push_dyn_array(	keys, &keys_capacity, &total_key_count,
									sizeof(*keys), &key);
		}

		// Interpolate samples from keys
		U32 key_i= 0;
		U32 next_key_i= 0;
		F32 key_t= 0.0;
		F32 next_key_t= 0.0;
		Clip_Key_Value key_value, next_key_value;
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

				key_value=		keys[ch_begin_key_i + key_i].value;
				next_key_value=	keys[ch_begin_key_i + next_key_i].value;
				key_t=			keys[ch_begin_key_i + key_i].time;
				next_key_t=		keys[ch_begin_key_i + next_key_i].time;
			}

			// Write samples
			const U32 s_i= frame_i*joint_count + joint_id;
			ensure(s_i < sample_count);
			const F32 lerp= (frame_t - key_t)/(next_key_t - key_t);
			switch (type) {
				case Clip_Key_Type_pos:
					samples[s_i].pos=
						lerp_v3f(key_value.pos, next_key_value.pos, lerp);
				break;
				case Clip_Key_Type_rot:
					/// @todo qlerp?
					samples[s_i].rot=
						normalized_qf(
								lerp_qf(key_value.rot, next_key_value.rot, lerp));
				break;
				case Clip_Key_Type_scale:
					samples[s_i].scale=
						lerp_v3f(key_value.scale, next_key_value.scale, lerp);
				break;
				default: fail("Unhandled ChType: %i", type);
			}
		}
	}

	const U32 samples_offset= buf->offset + sizeof(Clip) - sizeof(Resource);
	const U32 samples_size= sizeof(*samples)*sample_count;

	const U32 keys_offset= samples_offset + samples_size;
	const U32 keys_size= sizeof(*keys)*total_key_count;

	Clip clip= {
		.duration= duration,
		.keys_offset= keys_offset,
		.key_count= total_key_count,
		.joint_count= joint_count,
		.frame_count= frame_count,
		.local_samples_offset= samples_offset,
	};

	blob_write(buf, (U8*)&clip + sizeof(clip.res),
					sizeof(clip) - sizeof(clip.res));
	blob_write(buf, samples, samples_size);
	blob_write(buf, keys, keys_size);

cleanup:
	free(samples);
	free(keys);
	return return_value;

error:
	return_value= 1;
	goto cleanup;
}

JointPoseArray calc_clip_pose(const Clip *c, F64 t)
{
	ensure(t >= 0 && t <= c->duration);

	JointPoseArray pose;
	// -1's are in the calculations because last frame
	// is only for interpolation target.
	const U32 frame_i=
		(U32)(t/c->duration*(c->frame_count - 1)) % c->frame_count;
	const U32 next_frame_i= (frame_i + 1) % c->frame_count;

	double int_part;
	const F32 lerp= modf(t/c->duration*(c->frame_count - 1), &int_part);

	for (U32 j_i= 0; j_i < c->joint_count; ++j_i) {
		U32 sample_i= frame_i*c->joint_count + j_i;
		U32 next_sample_i= next_frame_i*c->joint_count + j_i;

		pose.tf[j_i]=
			lerp_t3f(	clip_local_samples(c)[sample_i],
						clip_local_samples(c)[next_sample_i],
						lerp);
	}
	return pose;
}

internal
void destroy_rt_clip(Resource *res)
{
	Clip *c= (Clip*)res;
	dev_free(blob_ptr(res, c->keys_offset));
	dev_free(blob_ptr(res, c->local_samples_offset));
	dev_free(c);
}

Clip *create_rt_clip(Clip *src)
{
	Clip *rt_clip= dev_malloc(sizeof(*rt_clip));
	*rt_clip= *src;

	substitute_res(&src->res, &rt_clip->res, destroy_rt_clip);

	rt_clip->keys_offset=
		alloc_substitute_res_member(	&rt_clip->res, &src->res,
										src->keys_offset,
										sizeof(Clip_Key)*src->key_count);

	rt_clip->local_samples_offset=
		alloc_substitute_res_member(	&rt_clip->res, &src->res,
										src->local_samples_offset,
										sizeof(T3f)*clip_sample_count(src));

	// Nobody stores pointers to Clips (yet) so no need for recaching any ptrs
	return rt_clip;
}

void rt_clip_add_key(Clip *c, F64 time)
{
	ensure(c->res.is_runtime_res);
	const U32 old_count= c->key_count;
	const U32 new_count= old_count + 1;

	Clip_Key *keys= blob_ptr(&c->res, c->keys_offset);
	keys= dev_realloc(keys, sizeof(*keys)*new_count);
	
	keys[old_count]= (Clip_Key) {
		.time= time,
	};

	c->keys_offset= blob_offset(&c->res, keys);
	c->key_count= new_count;

	// @todo Update samples
}
