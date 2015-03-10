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

	for (U32 inst_i= 0; inst_i < count; ++inst_i, ++inst) {
		const Clip *c= inst->clip;
		const U32 frame_i=
			(S32)(inst->t/c->duration*c->fps) % c->frame_count;

		for (U32 j_i= 0; j_i < c->joint_count; ++j_i) {
			U32 sample_i= frame_i*c->joint_count + j_i;
			inst->pose.tf[j_i]= c->local_samples[sample_i];
		}

		inst->t += dt;
	}
}
