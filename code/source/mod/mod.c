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
#include "visual/renderer.h" // screen_to_world_point, camera

// Character
typedef struct PlayerCh {
	T3d tf;
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
		.tf= p->tf,
	};
	p->body= get_rigidbody(resurrect_rigidbody(&dead_body));

	p->motor= add_simplemotor(p->body);
	set_constraint_max_force(p->motor, 100.0);

	p->run_clip_id= res_id(ResType_Clip, "playerch_run");
	p->idle_clip_id= res_id(ResType_Clip, "playerch_idle");

	return NULL_HANDLE;
}

internal
F64 smooth_drive(F64 value, F64 target, F64 dt)
{
	return (value - target)*pow(2, -dt) + target;
}

MOD_API void upd_playerch(PlayerCh *p, PlayerCh *e)
{
	// @todo Input from configurable input layer
	int dir= 0;
	if (g_env.device->key_down['a'])
		dir -= 1;
	if (g_env.device->key_down['d'])
		dir += 1;
	bool jump= g_env.device->key_pressed[KEY_SPACE];
	F64 dt= g_env.world->dt;
	V2d cursor_p= screen_to_world_point(g_env.device->cursor_pos);

	for (; p != e; ++p) {
		set_simplemotor_rate(p->motor, -dir*13);

		if (jump) {
			apply_impulse_world(	p->body,
									(V2d) {0, 15},
									(V2d) {p->body->tf.pos.x, p->body->tf.pos.y});
		}


		p->tf.pos= add_v3d(p->body->tf.pos, (V3d) {0, 0.25, 0});
		
		{ // Camera
			V2d target_pos= {
				p->tf.pos.x*0.5 + cursor_p.x*0.5,
				p->tf.pos.y*0.5 + cursor_p.y*0.5,
			};

			V3d cam_pos= g_env.renderer->cam_pos;
			cam_pos.x= smooth_drive(cam_pos.x, target_pos.x, dt*30);
			cam_pos.y= smooth_drive(cam_pos.y, target_pos.y, dt*30);
			g_env.renderer->cam_pos= cam_pos;
		}

		int facing_dir= 0;
		{
			F64 rot= angle_qd(p->tf.rot);
			if (cursor_p.x < p->tf.pos.x) {
				facing_dir= -1;
				rot= smooth_drive(rot, TAU/2.0, dt*15);
			} else {
				facing_dir= 1;
				rot= smooth_drive(rot, 0, dt*25);
			}
			p->tf.rot= qd_by_axis((V3d) {0, 1, 0}, rot);
		}

		if (dir) {
			// If moving
			p->idle_run_lerp= smooth_drive(p->idle_run_lerp, 1, dt*15.0);
			p->clip_time += dt*dir*facing_dir;
		} else {
			p->clip_time += dt;
			p->idle_run_lerp= smooth_drive(p->idle_run_lerp, 0, dt*15.0);
		}

		const Clip *idle_clip= (Clip *)res_by_id(p->idle_clip_id);
		const Clip *run_clip= (Clip *)res_by_id(p->run_clip_id);
		p->pose= lerp_pose(	calc_clip_pose(idle_clip, p->clip_time),
							calc_clip_pose(run_clip, p->clip_time),
							p->idle_run_lerp);
	}
}

MOD_API void free_playerch(PlayerCh *p)
{
	free_rigidbody(p->body);
	// No need to free p->motor -- destroyed along body
}
