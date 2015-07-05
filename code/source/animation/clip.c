#include "armature.h"
#include "clip.h"
#include "core/array.h"
#include "core/memory.h"
#include "resources/resblob.h"

U32 clip_sample_count(const Clip *c)
{ return c->joint_count*c->frame_count; }

T3f * clip_local_samples(const Clip *c)
{ return blob_ptr(&c->res, c->local_samples_offset); }

Clip_Key * clip_keys(const Clip *c)
{ return blob_ptr(&c->res, c->keys_offset); }


#define QSORT_CMP_INC(a, b) (((a) > (b)) - ((a) < (b)))

int clip_key_cmp(const void *a_, const void *b_)
{
	const Clip_Key *a= a_;
	const Clip_Key *b= b_;

	if (a->joint_id != b->joint_id)
		return QSORT_CMP_INC(a->joint_id, b->joint_id);

	if (a->type != b->type)
		return QSORT_CMP_INC(a->type, b->type);

	return QSORT_CMP_INC(a->time, b->time);
}

internal
void sort_clip_keys(Clip_Key *keys, U32 key_count)
{
	qsort(keys, key_count, sizeof(*keys), clip_key_cmp);
}

// `keys` should be sorted with sort_clip_keys
internal
void calc_samples_for_clip(	T3f *samples, U32 joint_count, U32 frame_count,
							const Clip_Key *keys, U32 key_count,
							F64 duration)
{
	const U32 sample_count= joint_count*frame_count;
	// Reset samples to bind pose, so that if (every) key is removed in
	// the editor the samples will reset to identity instead of this function
	// being NOP
	for (U32 i= 0; i < sample_count; ++i)
		samples[i]= identity_t3f();

	// Each joint has three channels: scale, rot and pos which may or may not
	// be specified by the `keys` array
	U32 ch_key_begin_i= 0;
	U32 ch_key_end_i= 0;
	for (;	ch_key_begin_i < key_count;
			ch_key_begin_i= ch_key_end_i) {
		const Clip_Key_Type ch_type= keys[ch_key_begin_i].type;
		const JointId joint_id= keys[ch_key_begin_i].joint_id;

		while (	ch_key_end_i < key_count &&
				keys[ch_key_end_i].type == ch_type &&
				keys[ch_key_end_i].joint_id == joint_id)
			++ch_key_end_i;

		// Interpolate samples from sorted keys
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
					key_i= ch_key_begin_i;
					next_key_i= key_i + 1;
				} else {
					++key_i;
					++next_key_i;
				}
				if (key_i >= ch_key_end_i)
					key_i= ch_key_end_i - 1;
				if (next_key_i >= ch_key_end_i)
					next_key_i= ch_key_end_i - 1;

				key_value=		keys[key_i].value;
				next_key_value=	keys[next_key_i].value;
				key_t=			keys[key_i].time;
				next_key_t=		keys[next_key_i].time;
			}

			// Write samples
			const U32 s_i= frame_i*joint_count + joint_id;
			ensure(s_i < sample_count);
			const F32 lerp= CLAMP((frame_t - key_t)/(next_key_t - key_t), 0, 1);
			switch (ch_type) {
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
				default: fail("Unhandled Clip_Key_Type: %i", ch_type);
			}
		}
	}
}

int json_clip_to_blob(struct BlobBuf *buf, JsonTok j)
{
	int return_value= 0;
	T3f *samples= NULL;
	const Armature *armature= NULL;
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

	armature= (Armature*)find_res_by_name_from_blobbuf(	buf,
														ResType_Armature,
														json_str(j_armature));
	if (!armature) {
		critical_print("Couldn't find armature %s", json_str(j_armature));
		goto error;
	}

	const U32 fps= 30.0;
	const U32 joint_count= armature->joint_count;
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
		JointId joint_id= joint_id_by_name(armature, json_str(j_joint));

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
		for (U32 key_i= 0; key_i < key_count; ++key_i) {
			JsonTok j_key= json_member(j_keys, key_i);
			JsonTok j_value= json_value_by_key(j_key, "v");
			Clip_Key key= {
				.joint_id= joint_id,
				.type= type,
			};
			key.time= json_real(json_value_by_key(j_key, "t"));
			key.time= CLAMP(key.time, 0, duration);

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
				default: fail("Unhandled Clip_Key_Type: %i", type);
			}

			keys= push_dyn_array(	keys, &keys_capacity, &total_key_count,
									sizeof(*keys), &key);
		}
	}

	sort_clip_keys(keys, total_key_count);
	calc_samples_for_clip(	samples, joint_count, frame_count,
							keys, total_key_count,
							duration);

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
	fmt_str(clip.armature_name, sizeof(clip.armature_name),
			"%s", armature->res.name);

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

internal
const char * clip_key_type_to_str(Clip_Key_Type t)
{
	switch (t) {
	case Clip_Key_Type_scale: return "scale";
	case Clip_Key_Type_rot: return "rot";
	case Clip_Key_Type_pos: return "pos";
	default: fail("Unhandled Clip_Key_Type: %i", t);
	}
}

void clip_to_json(WJson *j, const Clip *c)
{
	const Armature *a=
		(Armature*)res_by_name(	c->res.blob,
								ResType_Armature, 
								c->armature_name);
	const Clip_Key *keys= clip_keys(c);
	// @todo armature field
	WJson *j_channels= wjson_named_member(j, JsonType_array, "channels");

	U32 ch_key_begin_i= 0;
	U32 ch_key_end_i= 0;
	for (;	ch_key_begin_i < c->key_count;
			ch_key_begin_i= ch_key_end_i) {
		const Clip_Key_Type ch_type= keys[ch_key_begin_i].type;
		const JointId joint_id= keys[ch_key_begin_i].joint_id;

		while (	ch_key_end_i < c->key_count &&
				keys[ch_key_end_i].type == ch_type &&
				keys[ch_key_end_i].joint_id == joint_id)
			++ch_key_end_i;

		WJson *j_ch= wjson_object();
		wjson_append(j_channels, j_ch);

		wjson_add_named_member(	j_ch,
								"joint",
								wjson_str(a->joint_names[joint_id]));

		wjson_add_named_member(	j_ch,
								"type",
								wjson_str(clip_key_type_to_str(ch_type)));

		WJson *j_keys= wjson_add_named_member(j_ch, "keys", wjson_array());
		for (U32 i= ch_key_begin_i; i < ch_key_end_i; ++i) {
			WJson *j_key= wjson_object();
			wjson_append(j_keys, j_key);

			wjson_add_named_member(j_key, "t", wjson_number(keys[i].time));

			WJson *j_v= NULL;
			switch (ch_type) {
			case Clip_Key_Type_pos:
				j_v= wjson_v3(v3f_to_v3d(keys[i].value.pos));
			break;
			case Clip_Key_Type_rot:
				j_v= wjson_q(qf_to_qd(keys[i].value.rot));
			break;
			case Clip_Key_Type_scale:
				j_v= wjson_v3(v3f_to_v3d(keys[i].value.scale));
			break;
			default: fail("Unhandled Clip_Key_Type: %i", ch_type);
			}

			wjson_add_named_member(j_key, "v", j_v);
		}
	}
}

internal
F64 wrap_float(F64 t, const F64 max)
{
	return fmod(t, max);
}

JointPoseArray calc_clip_pose(const Clip *c, F64 t)
{
	t= wrap_float(t, c->duration);
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

	recache_ptrs_to_clips();
	return rt_clip;
}

internal
void add_rt_clip_key(Clip *c, Clip_Key key)
{
	ensure(c->res.is_runtime_res);
	const U32 old_count= c->key_count;
	const U32 new_count= old_count + 1;

	Clip_Key *keys= blob_ptr(&c->res, c->keys_offset);
	keys= dev_realloc(keys, sizeof(*keys)*new_count);
	
	keys[old_count]= key;

	c->keys_offset= blob_offset(&c->res, keys);
	c->key_count= new_count;


	c->res.needs_saving= true;
}

void update_rt_clip_key(Clip *c, Clip_Key key)
{
	ensure(c->res.is_runtime_res);

	Clip_Key *edit_key= NULL;
	for (U32 i= 0; i < c->key_count; ++i) {
		Clip_Key *cmp= &clip_keys(c)[i];
		if (	cmp->time == key.time &&
				cmp->joint_id == key.joint_id &&
				cmp->type == key.type) {
			edit_key= cmp;
			break;
		}
	}

	if (edit_key)
		*edit_key= key;
	else
		add_rt_clip_key(c, key);

	sort_clip_keys(clip_keys(c), c->key_count);
	calc_samples_for_clip(	clip_local_samples(c),
							c->joint_count, c->frame_count,
							clip_keys(c), c->key_count,
							c->duration);

	c->res.needs_saving= true;
}

void delete_rt_clip_key(Clip *c, U32 del_i)
{
	for (U32 i= del_i; i + 1 < c->key_count; ++i)
		clip_keys(c)[i]= clip_keys(c)[i + 1];

	--c->key_count;

	// No need for sorting
	calc_samples_for_clip(	clip_local_samples(c),
							c->joint_count, c->frame_count,
							clip_keys(c), c->key_count,
							c->duration);
	c->res.needs_saving= true;
}

void make_rt_clip_looping(Clip *c)
{

	Clip_Key *keys= dev_malloc(sizeof(*keys)*c->key_count);
	U32 key_count= 0;

	// Find keys to-be-appended
	JointId last_key_joint= NULL_JOINT_ID;
	Clip_Key_Type last_key_type= Clip_Key_Type_none;
	for (U32 i= 0; i < c->key_count; ++i) {
		Clip_Key key= clip_keys(c)[i];
		if (	key.joint_id != last_key_joint ||
				key.type != last_key_type) {
			keys[key_count]= key;
			keys[key_count].time= c->duration;
			++key_count;

			last_key_joint= key.joint_id;
			last_key_type= key.type;
		}
	}

	// Append keys
	for (U32 i= 0; i < key_count; ++i)
		update_rt_clip_key(c, keys[i]);

	c->res.needs_saving= true;

	dev_free(keys);
}

void move_rt_clip_keys(Clip *c, F64 from, F64 to)
{
	const Armature *a= (Armature *)res_by_name(	c->res.blob,
												ResType_Armature,
												c->armature_name);

	for (U32 i= 0; i < c->key_count; ++i) {
		Clip_Key *key= &clip_keys(c)[i];
		if (key->time == from && a->joints[key->joint_id].selected) {
			key->time= to;
		}
	}

	// @todo These commands are repeated -- maybe some on_clip_edit_end() ?
	sort_clip_keys(clip_keys(c), c->key_count);
	calc_samples_for_clip(	clip_local_samples(c),
							c->joint_count, c->frame_count,
							clip_keys(c), c->key_count,
							c->duration);

	c->res.needs_saving= true;
}

void recache_ptrs_to_clips()
{
	// Nobody stores pointers yet
}
