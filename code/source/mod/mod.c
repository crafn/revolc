#include "animation/clip.h"
#include "animation/joint.h"
#include "build.h"
#include "core/debug_print.h"
#include "core/ensure.h"
#include "core/vector.h"
#include "global/env.h"
#include "game/world.h"
#include "physics/rigidbody.h"
#include "physics/physworld.h"
#include "platform/device.h"
#include "resources/resblob.h"

// Character
typedef struct PlayerCh {
	V3d pos;
	JointPoseArray pose;

	// Cached
	ResId run_clip_id;
	ResId idle_clip_id;
	F64 idle_run_lerp;
	F64 clip_time;

	RigidBody *body;
	Constraint *motor;
} PlayerCh;

MOD_API U32 resurrect_playerch(PlayerCh *p)
{
	RigidBody dead_body= {
		.def_name= "playerch_legs",
		.tf.pos= p->pos,
	};
	p->body= get_rigidbody(resurrect_rigidbody(&dead_body));

	p->motor= add_simplemotor(p->body);
	set_constraint_max_force(p->motor, 200.0);

	p->run_clip_id= res_id(ResType_Clip, "playerch_run");
	p->idle_clip_id= res_id(ResType_Clip, "playerch_idle");

	return NULL_HANDLE;
}

MOD_API void upd_playerch(PlayerCh *p, PlayerCh *e)
{
	int dir= 0;
	if (g_env.device->key_down['a'])
		dir -= 1;
	if (g_env.device->key_down['d'])
		dir += 1;
	bool jump= g_env.device->key_pressed[KEY_SPACE];
	F64 dt= g_env.world->dt;

	for (; p != e; ++p) {
		set_simplemotor_rate(p->motor, -dir*15);

		if (jump) {
			apply_impulse_world(	p->body,
									(V2d) {0, 15},
									(V2d) {p->body->tf.pos.x, p->body->tf.pos.y});
		}

		if (dir) {
			p->idle_run_lerp= (p->idle_run_lerp - 1)*pow(2, -dt*15.0) + 1;
			p->clip_time += dt*dir;
		} else {
			p->clip_time += dt;
			p->idle_run_lerp= p->idle_run_lerp*pow(2, -dt*20.0);
		}

		const Clip *idle_clip= (Clip *)res_by_id(p->idle_clip_id);
		const Clip *run_clip= (Clip *)res_by_id(p->run_clip_id);
		p->pose= lerp_pose(	calc_clip_pose(idle_clip, p->clip_time),
							calc_clip_pose(run_clip, p->clip_time),
							p->idle_run_lerp);
		p->pos= add_v3d(p->body->tf.pos, (V3d) {0, 0.25, 0});
	}
}

MOD_API void free_playerch(PlayerCh *p)
{
	free_rigidbody(p->body);
	// No need to free p->motor -- destroyed along body
}
