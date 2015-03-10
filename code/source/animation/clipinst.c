#include "clipinst.h"
#include "global/env.h"
#include "resources/resblob.h"
#include "game/world.h"

void init_clipinst(ClipInst *data)
{
	*data= (ClipInst) {};
	for (U32 i= 0; i < MAX_ARMATURE_JOINT_COUNT; ++i)
		data->pose.tf[i]= identity_t3f();
}

U32 resurrect_clipinst(ClipInst *dead)
{
	dead->clip= (Clip*)res_by_name(g_env.resblob, ResType_Clip, dead->clip_name);
	return NULL_HANDLE;
}

void upd_clipinst(ClipInst *inst, U32 count)
{
	F32 dt= g_env.world->dt;

	// -1's are in the calculations because last frame
	// is only for interpolation target.
	for (U32 inst_i= 0; inst_i < count; ++inst_i, ++inst) {
		const Clip *c= inst->clip;

		inst->t += dt;
		while(inst->t > c->duration)
			inst->t -= c->duration;

		const U32 frame_i=
			(U32)(inst->t/c->duration*(c->frame_count - 1)) % c->frame_count;
		const U32 next_frame_i= (frame_i + 1) % c->frame_count;

		double int_part;
		const F32 lerp= modf(inst->t/c->duration*(c->frame_count - 1), &int_part);

		for (U32 j_i= 0; j_i < c->joint_count; ++j_i) {
			U32 sample_i= frame_i*c->joint_count + j_i;
			U32 next_sample_i= next_frame_i*c->joint_count + j_i;

			inst->pose.tf[j_i]=
				lerp_t3f(	c->local_samples[sample_i],
							c->local_samples[next_sample_i],
							lerp);
		}
	}
}
