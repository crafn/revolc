#include "armature.h"
#include "clip.h"
#include "core/array.h"
#include "core/memory.h"
#include "global/env.h"
#include "resources/resblob.h"

Armature *clip_armature(const Clip *c)
{
	return (Armature*)res_by_name(	c->res.blob,
									ResType_Armature,
									c->armature_name);
}

U32 clip_sample_count(const Clip *c)
{ return c->joint_count*c->frame_count; }

T3f * clip_local_samples(const Clip *c)
{ return rel_ptr(&c->local_samples); }

Clip_Key * clip_keys(const Clip *c)
{ return rel_ptr(&c->keys); }

internal
int clip_key_cmp(const void *a_, const void *b_)
{
	const Clip_Key *a = a_;
	const Clip_Key *b = b_;

	if (strcmp(a->joint_name, b->joint_name))
		return strcmp(a->joint_name, b->joint_name);

	if (a->type != b->type)
		return CMP(a->type, b->type);

	return CMP(a->time, b->time);
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
	const U32 sample_count = joint_count*frame_count;
	// Reset samples to bind pose, so that if (every) key is removed in
	// the editor the samples will reset to identity instead of this function
	// being NOP
	for (U32 i = 0; i < sample_count; ++i)
		samples[i] = identity_t3f();

	// Each joint has three channels: scale, rot and pos which may or may not
	// be specified by the `keys` array
	U32 ch_key_begin_i = 0;
	U32 ch_key_end_i = 0;
	for (;	ch_key_begin_i < key_count;
			ch_key_begin_i = ch_key_end_i) {
		const Clip_Key_Type ch_type = keys[ch_key_begin_i].type;
		const char *joint_name = keys[ch_key_begin_i].joint_name;

		while (	ch_key_end_i < key_count &&
				keys[ch_key_end_i].type == ch_type &&
				!strcmp(keys[ch_key_end_i].joint_name, joint_name))
			++ch_key_end_i;

		// Interpolate samples from sorted keys
		U32 key_i = 0;
		U32 next_key_i = 0;
		F32 key_t = 0.0;
		F32 next_key_t = 0.0;
		T3f key_value, next_key_value;
		bool first = true;
		for (U32 frame_i = 0; frame_i < frame_count; ++frame_i) {
			const F32 frame_t = frame_i*duration/(frame_count - 1);

			// Update cur/next keys
			if (first || frame_t > next_key_t) {
				if (first) {
					first = false;
					key_i = ch_key_begin_i;
					next_key_i = key_i + 1;
				} else {
					++key_i;
					++next_key_i;
				}
				if (key_i >= ch_key_end_i)
					key_i = ch_key_end_i - 1;
				if (next_key_i >= ch_key_end_i)
					next_key_i = ch_key_end_i - 1;

				key_value =		keys[key_i].value;
				next_key_value =	keys[next_key_i].value;
				key_t =			keys[key_i].time;
				next_key_t =		keys[next_key_i].time;
			}

			// Write samples
			const U32 s_i = frame_i*joint_count + keys[ch_key_begin_i].joint_ix;
			ensure(s_i < sample_count);
			const F32 lerp = CLAMP((frame_t - key_t)/(next_key_t - key_t), 0, 1);
			switch (ch_type) {
				case Clip_Key_Type_pos:
					samples[s_i].pos =
						lerp_v3f(key_value.pos, next_key_value.pos, lerp);
				break;
				case Clip_Key_Type_rot:
					/// @todo qlerp?
					samples[s_i].rot =
						normalized_qf(
								lerp_qf(key_value.rot, next_key_value.rot, lerp));
				break;
				case Clip_Key_Type_scale:
					samples[s_i].scale =
						lerp_v3f(key_value.scale, next_key_value.scale, lerp);
				break;
				default: fail("Unhandled Clip_Key_Type: %i", ch_type);
			}
		}
	}
}

void init_clip(Clip *clip)
{
	Armature *armature = clip_armature(clip);

	// Write joint ix -> id map
	U32 key_count = clip->key_count;
	Clip_Key *keys = clip_keys(clip);
	for (U32 i = 0; i < key_count;) {
		const char *joint_name = keys[i].joint_name;
		clip->joint_ix_to_id[keys[i].joint_ix] = joint_id_by_name(armature, joint_name);

		while (	i < key_count &&
				!strcmp(keys[i].joint_name, joint_name))
			++i;
	}
}

int json_clip_to_blob(struct BlobBuf *buf, JsonTok j)
{
	int return_value = 0;
	Clip_Key *keys = NULL;
	T3f *samples = NULL;

	JsonTok j_armature = json_value_by_key(j, "armature");
	JsonTok j_duration = json_value_by_key(j, "duration");
	JsonTok j_channels = json_value_by_key(j, "channels");

	if (json_is_null(j_armature))
		RES_ATTRIB_MISSING("armature");
	if (json_is_null(j_duration))
		RES_ATTRIB_MISSING("duration");
	if (json_is_null(j_channels))
		RES_ATTRIB_MISSING("channels");

	U32 fps = 30.0;
	F32 duration = json_real(j_duration);

	U32 keys_capacity = 0;
	U32 total_key_count = 0;

	for (U32 ch_i = 0; ch_i < json_member_count(j_channels); ++ch_i) {
		JsonTok j_ch = json_member(j_channels, ch_i);

		JsonTok j_joint = json_value_by_key(j_ch, "joint");
		JsonTok j_type = json_value_by_key(j_ch, "type");
		JsonTok j_keys = json_value_by_key(j_ch, "keys");

		if (json_is_null(j_joint))
			RES_ATTRIB_MISSING("joint");
		if (json_is_null(j_type))
			RES_ATTRIB_MISSING("type");
		if (json_is_null(j_keys))
			RES_ATTRIB_MISSING("keys");

		const char *type_str = json_str(j_type);
		//JointId joint_id = joint_id_by_name(armature, json_str(j_joint));

		Clip_Key_Type type;
		if (!strcmp(type_str, "pos"))
			type = Clip_Key_Type_pos;
		else if (!strcmp(type_str, "rot"))
			type = Clip_Key_Type_rot;
		else if (!strcmp(type_str, "scale"))
			type = Clip_Key_Type_scale;
		else
			fail("Unknown Clip channel type: %s", type_str);

		const U32 key_count = json_member_count(j_keys);
		if (key_count == 0)
			continue;

		// Gather keys of the channel
		for (U32 key_i = 0; key_i < key_count; ++key_i) {
			JsonTok j_key = json_member(j_keys, key_i);
			JsonTok j_value = json_value_by_key(j_key, "v");
			Clip_Key key = { .type = type };
			fmt_str(key.joint_name, sizeof(key.joint_name), "%s", json_str(j_joint));
			key.time = json_real(json_value_by_key(j_key, "t"));
			key.time = CLAMP(key.time, 0, duration);

			switch (type) {
				case Clip_Key_Type_pos:
					key.value.pos = v3d_to_v3f(json_v3(j_value));
				break;
				case Clip_Key_Type_rot:
					key.value.rot = qd_to_qf(json_q(j_value));
				break;
				case Clip_Key_Type_scale:
					key.value.scale = v3d_to_v3f(json_v3(j_value));
				break;
				default: fail("Unhandled Clip_Key_Type: %i", type);
			}

			keys = push_dyn_array(	keys, &keys_capacity, &total_key_count,
									sizeof(*keys), &key);
		}
	}

	sort_clip_keys(keys, total_key_count);

	U32 joint_count = 0;
	for (U32 i = 0; i < total_key_count; ++joint_count) {
		const char *joint_name = keys[i].joint_name;

		while (	i < total_key_count &&
				!strcmp(keys[i].joint_name, joint_name)) {
			keys[i].joint_ix = joint_count;
			++i;
		}
	}

	U32 frame_count = floor(duration*fps + 0.5) + 1; // +1 for end lerp target
	const U32 sample_count = joint_count*frame_count;
	samples = malloc(sizeof(*samples)*sample_count);
	for (U32 i = 0; i < sample_count; ++i)
		samples[i] = identity_t3f();

	calc_samples_for_clip(	samples, joint_count, frame_count,
							keys, total_key_count,
							duration);

	Clip clip = {
		.duration = duration,
		.key_count = total_key_count,
		.joint_count = joint_count,
		.frame_count = frame_count,
	};
	fmt_str(clip.armature_name, sizeof(clip.armature_name),
			"%s", json_str(j_armature));

	const U32 samples_offset = buf->offset + offsetof(Clip, local_samples);
	const U32 keys_offset = buf->offset + offsetof(Clip, keys);

	const U32 samples_size = sizeof(*samples)*sample_count;
	const U32 keys_size = sizeof(*keys)*total_key_count;

	blob_write(buf, &clip, sizeof(clip));

	blob_patch_rel_ptr(buf, samples_offset);
	blob_write(buf, samples, samples_size);

	blob_patch_rel_ptr(buf, keys_offset);
	blob_write(buf, keys, keys_size);

cleanup:
	free(samples);
	free(keys);
	return return_value;

error:
	return_value = 1;
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

internal
JointId joint_id_by_ix(const Clip *c, U8 ix)
{
	ensure(ix < c->joint_count);
	return c->joint_ix_to_id[ix];
}

void clip_to_json(WJson *j, const Clip *c)
{
	const Armature *a = clip_armature(c);
	const Clip_Key *keys = clip_keys(c);
	// @todo armature field
	WJson *j_channels = wjson_named_member(j, JsonType_array, "channels");

	U32 ch_key_begin_i = 0;
	U32 ch_key_end_i = 0;
	for (;	ch_key_begin_i < c->key_count;
			ch_key_begin_i = ch_key_end_i) {
		const Clip_Key_Type ch_type = keys[ch_key_begin_i].type;
		U8 joint_ix = keys[ch_key_begin_i].joint_ix;
		const JointId joint_id = joint_id_by_ix(c, joint_ix);

		while (	ch_key_end_i < c->key_count &&
				keys[ch_key_end_i].type == ch_type &&
				keys[ch_key_end_i].joint_ix == joint_ix)
			++ch_key_end_i;

		WJson *j_ch = wjson_object();
		wjson_append(j_channels, j_ch);

		wjson_add_named_member(	j_ch,
								"joint",
								wjson_str(a->joint_names[joint_id]));

		wjson_add_named_member(	j_ch,
								"type",
								wjson_str(clip_key_type_to_str(ch_type)));

		WJson *j_keys = wjson_add_named_member(j_ch, "keys", wjson_array());
		for (U32 i = ch_key_begin_i; i < ch_key_end_i; ++i) {
			WJson *j_key = wjson_object();
			wjson_append(j_keys, j_key);

			wjson_add_named_member(j_key, "t", wjson_number(keys[i].time));

			WJson *j_v = NULL;
			switch (ch_type) {
			case Clip_Key_Type_pos:
				j_v = wjson_v3(v3f_to_v3d(keys[i].value.pos));
			break;
			case Clip_Key_Type_rot:
				j_v = wjson_q(qf_to_qd(keys[i].value.rot));
			break;
			case Clip_Key_Type_scale:
				j_v = wjson_v3(v3f_to_v3d(keys[i].value.scale));
			break;
			default: fail("Unhandled Clip_Key_Type: %i", ch_type);
			}

			wjson_add_named_member(j_key, "v", j_v);
		}
	}
}

Clip *blobify_clip(struct WArchive *ar, Cson c, bool *err)
{
	Clip *ptr = warchive_ptr(ar);
	Clip_Key *keys = NULL;
	T3f *samples = NULL;

	Cson c_armature = cson_key(c, "armature");
	Cson c_duration = cson_key(c, "duration");
	Cson c_channels = cson_key(c, "channels");

	if (cson_is_null(c_armature))
		RES_ATTRIB_MISSING("armature");
	if (cson_is_null(c_duration))
		RES_ATTRIB_MISSING("duration");
	if (cson_is_null(c_channels))
		RES_ATTRIB_MISSING("channels");

	U32 fps = 30.0;
	F32 duration = blobify_floating(c_duration, err);

	U32 keys_capacity = 0;
	U32 total_key_count = 0;

	for (U32 ch_i = 0; ch_i < cson_member_count(c_channels); ++ch_i) {
		Cson c_ch = cson_member(c_channels, ch_i);

		Cson c_joint = cson_key(c_ch, "joint");
		Cson c_type = cson_key(c_ch, "type");
		Cson c_keys = cson_key(c_ch, "keys");

		if (cson_is_null(c_joint))
			RES_ATTRIB_MISSING("joint");
		if (cson_is_null(c_type))
			RES_ATTRIB_MISSING("type");
		if (cson_is_null(c_keys))
			RES_ATTRIB_MISSING("keys");

		const char *type_str = blobify_string(c_type, err);

		Clip_Key_Type type;
		if (!strcmp(type_str, "pos"))
			type = Clip_Key_Type_pos;
		else if (!strcmp(type_str, "rot"))
			type = Clip_Key_Type_rot;
		else if (!strcmp(type_str, "scale"))
			type = Clip_Key_Type_scale;
		else
			fail("Unknown Clip channel type: %s", type_str);

		const U32 key_count = cson_member_count(c_keys);
		if (key_count == 0)
			continue;

		// Gather keys of the channel
		for (U32 key_i = 0; key_i < key_count; ++key_i) {
			Cson c_key = cson_member(c_keys, key_i);
			Cson c_value = cson_key(c_key, "v");
			Clip_Key key = { .value = identity_t3f(), .type = type };
			fmt_str(key.joint_name, sizeof(key.joint_name), "%s", blobify_string(c_joint, err));
			key.time = blobify_floating(cson_key(c_key, "t"), err);
			key.time = CLAMP(key.time, 0, duration);

			switch (type) {
				case Clip_Key_Type_pos:
					key.value.pos = v3d_to_v3f(blobify_v3(c_value, err));
				break;
				case Clip_Key_Type_rot:
					key.value.rot = qd_to_qf(blobify_q(c_value, err));
				break;
				case Clip_Key_Type_scale:
					key.value.scale = v3d_to_v3f(blobify_v3(c_value, err));
				break;
				default: fail("Unhandled Clip_Key_Type: %i", type);
			}

			keys = push_dyn_array(	keys, &keys_capacity, &total_key_count,
									sizeof(*keys), &key);
		}
	}

	sort_clip_keys(keys, total_key_count);

	U32 joint_count = 0;
	for (U32 i = 0; i < total_key_count; ++joint_count) {
		const char *joint_name = keys[i].joint_name;

		while (	i < total_key_count &&
				!strcmp(keys[i].joint_name, joint_name)) {
			keys[i].joint_ix = joint_count;
			++i;
		}
	}

	U32 frame_count = floor(duration*fps + 0.5) + 1; // +1 for end lerp target
	const U32 sample_count = joint_count*frame_count;
	samples = malloc(sizeof(*samples)*sample_count);
	for (U32 i = 0; i < sample_count; ++i)
		samples[i] = identity_t3f();

	calc_samples_for_clip(	samples, joint_count, frame_count,
							keys, total_key_count,
							duration);

	Clip clip = {
		.duration = duration,
		.key_count = total_key_count,
		.joint_count = joint_count,
		.frame_count = frame_count,
	};
	fmt_str(clip.armature_name, sizeof(clip.armature_name),
			"%s", blobify_string(c_armature, err));

	const U32 samples_offset = ar->data_size + offsetof(Clip, local_samples);
	const U32 keys_offset = ar->data_size + offsetof(Clip, keys);

	const U32 samples_size = sizeof(*samples)*sample_count;
	const U32 keys_size = sizeof(*keys)*total_key_count;

	if (err && *err)
		goto error;

	pack_buf(ar, &clip, sizeof(clip));

	pack_patch_rel_ptr(ar, samples_offset);
	pack_buf(ar, samples, samples_size);

	pack_patch_rel_ptr(ar, keys_offset);
	pack_buf(ar, keys, keys_size);

cleanup:
	free(samples);
	free(keys);
	return ptr;

error:
	ptr = NULL;
	SET_ERROR_FLAG(err);
	goto cleanup;
}

void deblobify_clip(WCson *c, struct RArchive *ar)
{
	Clip *clip = rarchive_ptr(ar, sizeof(*clip));
	unpack_advance(ar,	sizeof(*clip) +
						sizeof(*clip_keys(clip))*clip->key_count +
						sizeof(*clip_local_samples(clip))*clip->joint_count*clip->frame_count);

	Armature *a = clip_armature(clip);
	Clip_Key *keys = clip_keys(clip);

	wcson_begin_compound(c, "Clip");

	wcson_designated(c, "name");
	deblobify_string(c, clip->res.name);

	wcson_designated(c, "duration");
	deblobify_floating(c, clip->duration);

	wcson_designated(c, "armature");
	deblobify_string(c, clip->armature_name);

	wcson_designated(c, "channels");
	wcson_begin_initializer(c);

	U32 ch_key_begin_i = 0;
	U32 ch_key_end_i = 0;
	for (;	ch_key_begin_i < clip->key_count;
			ch_key_begin_i = ch_key_end_i) {
		const Clip_Key_Type ch_type = keys[ch_key_begin_i].type;
		U8 joint_ix = keys[ch_key_begin_i].joint_ix;
		const JointId joint_id = joint_id_by_ix(clip, joint_ix);

		while (	ch_key_end_i < clip->key_count &&
				keys[ch_key_end_i].type == ch_type &&
				keys[ch_key_end_i].joint_ix == joint_ix)
			++ch_key_end_i;

		wcson_begin_initializer(c);

		wcson_designated(c, "joint");
		deblobify_string(c, a->joint_names[joint_id]);

		wcson_designated(c, "type");
		deblobify_string(c, clip_key_type_to_str(ch_type));

		wcson_designated(c, "keys");
		wcson_begin_initializer(c);
		for (U32 i = ch_key_begin_i; i < ch_key_end_i; ++i) {
			wcson_begin_initializer(c);

			wcson_designated(c, "t");
			deblobify_floating(c, keys[i].time);

			wcson_designated(c, "v");
			switch (ch_type) {
			case Clip_Key_Type_pos:
				deblobify_v3(c, v3f_to_v3d(keys[i].value.pos));
			break;
			case Clip_Key_Type_rot:
				deblobify_q(c, qf_to_qd(keys[i].value.rot));
			break;
			case Clip_Key_Type_scale:
				deblobify_v3(c, v3f_to_v3d(keys[i].value.scale));
			break;
			default: fail("Unhandled Clip_Key_Type: %i", ch_type);
			}

			wcson_end_initializer(c);
		}
		wcson_end_initializer(c);

		wcson_end_initializer(c);
	}

	wcson_end_compound(c);

	wcson_end_compound(c);
}

internal
F64 wrap_float(F64 t, const F64 max)
{
	return t - max*floor(t/max);
}

JointPoseArray calc_clip_pose(const Clip *c, F64 t)
{
	t = wrap_float(t, c->duration);
	ensure(t >= 0 && t <= c->duration);

	JointPoseArray pose = identity_pose();
	// -1's are in the calculations because last frame
	// is only for interpolation target.
	const U32 frame_i =
		(U32)(t/c->duration*(c->frame_count - 1)) % c->frame_count;
	const U32 next_frame_i = (frame_i + 1) % c->frame_count;

	double int_part;
	const F32 lerp = modf(t/c->duration*(c->frame_count - 1), &int_part);

	for (U32 j_i = 0; j_i < c->joint_count; ++j_i) {
		U32 sample_i = frame_i*c->joint_count + j_i;
		U32 next_sample_i = next_frame_i*c->joint_count + j_i;
		JointId id = c->joint_ix_to_id[j_i];

		pose.tf[id] =
			lerp_t3f(	clip_local_samples(c)[sample_i],
						clip_local_samples(c)[next_sample_i],
						lerp);
	}
	return pose;
}

internal
void add_rt_clip_key(Clip *c, Clip_Key key)
{
	fail("This crash is expected! @todo Update joint_count and realloc samples if joint_ix will be new");

	const U32 old_count = c->key_count;
	const U32 new_count = old_count + 1;

	realloc_res_member(&c->res, &c->keys, sizeof(Clip_Key)*new_count, sizeof(Clip_Key)*old_count);
	Clip_Key *keys = rel_ptr(&c->keys);

	keys[old_count] = key;

	c->key_count = new_count;

	resource_modified(&c->res);
}

void update_rt_clip_key(Clip *c, Clip_Key key)
{
	Clip_Key *edit_key = NULL;
	for (U32 i = 0; i < c->key_count; ++i) {
		Clip_Key *cmp = &clip_keys(c)[i];
		if (	cmp->time == key.time &&
				cmp->joint_ix == key.joint_ix &&
				cmp->type == key.type) {
			edit_key = cmp;
			break;
		}
	}

	if (edit_key)
		*edit_key = key;
	else
		add_rt_clip_key(c, key);

	sort_clip_keys(clip_keys(c), c->key_count);
	calc_samples_for_clip(	clip_local_samples(c),
							c->joint_count, c->frame_count,
							clip_keys(c), c->key_count,
							c->duration);

	resource_modified(&c->res);
}

void delete_rt_clip_key(Clip *c, U32 del_i)
{
	for (U32 i = del_i; i + 1 < c->key_count; ++i)
		clip_keys(c)[i] = clip_keys(c)[i + 1];

	--c->key_count;

	// No need for sorting
	calc_samples_for_clip(	clip_local_samples(c),
							c->joint_count, c->frame_count,
							clip_keys(c), c->key_count,
							c->duration);
	resource_modified(&c->res);
}

void make_rt_clip_looping(Clip *c)
{
	Clip_Key *keys = ALLOC(frame_ator(), sizeof(*keys)*c->key_count, "keys");
	U32 key_count = 0;

	// Find keys to-be-appended
	U8 last_key_joint = (U8)-1;
	Clip_Key_Type last_key_type = Clip_Key_Type_none;
	for (U32 i = 0; i < c->key_count; ++i) {
		Clip_Key key = clip_keys(c)[i];
		if (	key.joint_ix == last_key_joint ||
				key.type != last_key_type) {
			keys[key_count] = key;
			keys[key_count].time = c->duration;
			++key_count;

			last_key_joint = key.joint_ix;
			last_key_type = key.type;
		}
	}

	// Append keys
	for (U32 i = 0; i < key_count; ++i)
		update_rt_clip_key(c, keys[i]);

	resource_modified(&c->res);
}

void move_rt_clip_keys(Clip *c, F64 from, F64 to)
{
	const Armature *a = (Armature *)res_by_name(c->res.blob,
												ResType_Armature,
												c->armature_name);

	for (U32 i = 0; i < c->key_count; ++i) {
		Clip_Key *key = &clip_keys(c)[i];
		JointId id = joint_id_by_ix(c, key->joint_ix);
		if (key->time == from && a->joints[id].selected) {
			key->time = to;
		}
	}

	// @todo These commands are repeated -- maybe some on_clip_edit_end() ?
	sort_clip_keys(clip_keys(c), c->key_count);
	calc_samples_for_clip(	clip_local_samples(c),
							c->joint_count, c->frame_count,
							clip_keys(c), c->key_count,
							c->duration);

	resource_modified(&c->res);
}

void recache_ptrs_to_clips()
{
	// Nobody stores pointers yet
}
