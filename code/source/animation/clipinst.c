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

void upd_clipinst(ClipInst *inst, ClipInst *e)
{
	F32 dt= g_env.world->dt;
	for (; inst != e; ++inst) {
		const Clip *c= inst->clip;

		inst->t += dt;
		while(inst->t > c->duration)
			inst->t -= c->duration;

		inst->pose= calc_clip_pose(c, inst->t);
	}
}
