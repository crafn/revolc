#include "animation/clip.h"
#include "animation/joint.h"
#include "audio/audiosystem.h"
#include "build.h"
#include "core/debug_print.h"
#include "core/ensure.h"
#include "core/random.h"
#include "core/vector.h"
#include "core/scalar.h"
#include "global/env.h"
#include "game/world.h"
#include "physics/chipmunk_util.h"
#include "physics/rigidbody.h"
#include "physics/physworld.h"
#include "physics/query.h"
#include "platform/device.h"
#include "resources/resblob.h"
#include "visual/renderer.h" // screen_to_world_point, camera

typedef struct WorldEnv {
	U8 placeholder;
} WorldEnv;

internal
void adjust_soundtrack(const char *sound_name, F32 vol)
{
	SoundHandle h= sound_handle_by_name(sound_name);
	if (!is_sound_playing(h) && vol > 0.0f)
		play_sound(sound_name, vol, 0);
	else
		set_sound_vol(h, vol);
}

MOD_API void upd_worldenv(WorldEnv *w, WorldEnv *e)
{
	ensure(w + 1 == e);

	if (g_env.device->key_down[KEY_KP_9])
		g_env.world->time += g_env.world->dt*50;
	if (g_env.device->key_down[KEY_KP_6])
		g_env.world->time -= g_env.world->dt*50;

	const F32 day_duration= 100.0;
	F64 time= g_env.world->time + day_duration/3.0; // Start somewhere morning
	F32 dayphase= fmod(time, day_duration)/day_duration;

	Color night= {0.15*0.3, 0.1*0.3, 0.6*0.3}; // Night
	struct TimeOfDay {
		F32 dayphase;
		Color color;
		const char *model;
		F32 ambient_fade; // 0.0 night, 1.0 day
	} times[] = {
		{0.0, night, "sky_night", 0.0},
		{0.1, night, "sky_night", 0.3},
		{0.2, {0.9, 0.2, 0.1}, "sky_evening", 0.5},
		{0.5, {2.3, 1.8, 1.9}, "sky_day", 1.0},
		{0.8, {0.9, 0.1, 0.05}, "sky_evening", 0.5},
		{0.9, night, "sky_night", 0.0},
		{1.0, night, "sky_night", 0.3},
	};
	const int times_count = ARRAY_COUNT(times);
	
	int times_i= 0;
	while (times[times_i + 1].dayphase < dayphase)
		++times_i;
	ensure(times_i + 1 < times_count);

	struct TimeOfDay cur= times[times_i];
	struct TimeOfDay next= times[times_i + 1];

	F32 t= smootherstep_f32(cur.dayphase, next.dayphase, dayphase);

	{ // Graphics
		g_env.renderer->env_light_color= lerp_color(cur.color, next.color, t);

		T3d tf= {{600, 1200, 1}, identity_qd(), {0, 0, -500}};
		drawcmd_model(	tf,
						(Model*)res_by_name(g_env.resblob, ResType_Model, cur.model),
						(Color) {1, 1, 1, 1}, 0, 0);
		tf.pos.z += 0.01;
		drawcmd_model(	tf,
						(Model*)res_by_name(g_env.resblob, ResType_Model, next.model),
						(Color) {1, 1, 1, t} , 0, 0);
	}

	{ // Sounds
		F32 ambient_fade= lerp_f32(cur.ambient_fade, next.ambient_fade, t);

		adjust_soundtrack("ambient_day", ambient_fade);
		adjust_soundtrack("ambient_night", 1 - ambient_fade);
	}
}

// Character
typedef struct PlayerCh {
	T3d tf;
	JointPoseArray pose;

	F64 idle_run_lerp;
	F64 clip_time;
	F64 fake_dif;
	F64 on_ground_timer;
	V2d last_ground_velocity;
	V2d last_ground_contact_point;
	F64 time_from_jump;
	F64 dig_timer;

	// Cached
	ResId run_clip_id;
	ResId idle_clip_id;

	RigidBody *body;
} PlayerCh;

MOD_API U32 resurrect_playerch(PlayerCh *p)
{
	RigidBody dead_body= {
		.def_name= "playerch_body",
		.tf= p->tf,
	};
	p->body= get_rigidbody(resurrect_rigidbody(&dead_body));

	p->run_clip_id= res_id(ResType_Clip, "playerch_run");
	p->idle_clip_id= res_id(ResType_Clip, "playerch_idle");

	return NULL_HANDLE;
}

// FPS independent exponential decay towards `target`
internal
F64 exp_drive(F64 value, F64 target, F64 dt)
{
	return (value - target)*pow(2, -dt) + target;
}

typedef struct PlayerChLegRayResult {
	F64 ray_fraction;
	RigidBody *ground_body;
	V2d ground_velocity;
	V2d ground_contact_point;
} PlayerChLegRayResult;

internal
void playerch_ray_callback(RigidBody *body, V2d point, V2d normal, F64 fraction, void *data)
{
	PlayerChLegRayResult *result= data;
	if (!result->ground_body || fraction < result->ray_fraction) {
		result->ray_fraction= fraction;
		result->ground_velocity= rigidbody_velocity_at(body, point);
		result->ground_body= body;
		result->ground_contact_point= point;
	}
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
	bool dig= g_env.device->key_down[KEY_LMB];
	V2d cursor_on_world= screen_to_world_point(g_env.device->cursor_pos);

	F64 dt= g_env.world->dt;
	V2d cursor_p= screen_to_world_point(g_env.device->cursor_pos);

	for (; p != e; ++p) {

		const F64 box_lower_height= 0.5; // @todo Should be in data
		const F64 leg_height= 0.65;
		const F64 on_ground_timer_start= 0.2; // Time to jump after ground has disappeared
		//const F64 leg_width= 0.2;
		F64 leg_space= leg_height;
		RigidBody *ground_body= NULL;

		{ // Levitate box
			V2d a= {
				p->body->tf.pos.x,
				p->body->tf.pos.y - box_lower_height - 0.02,
			};
			V2d b= {
				p->body->tf.pos.x,
				p->body->tf.pos.y - box_lower_height - leg_height,
			};

			PlayerChLegRayResult result= {};
			// @todo Multiple segments (or radius for the segment)
			phys_segment_query(
				a, b, 0.0,
				playerch_ray_callback, &result);

			F64 leg_space= leg_height*result.ray_fraction;

			if (result.ground_body && p->time_from_jump > 0.2) {
				p->on_ground_timer= on_ground_timer_start;
				p->last_ground_velocity= result.ground_velocity;
				p->last_ground_contact_point= result.ground_contact_point;
				ground_body= result.ground_body;

				const F64 max_force= 200;
				F64 yforce= (leg_height - leg_space)*300;
				if (ABS(yforce) > max_force)
					yforce = SIGN(yforce)*max_force;

				// Damping
				V2d vel= p->body->velocity;
				yforce -= vel.y*50.0;

				V2d force= (V2d) {0, yforce};
				apply_force(p->body, force);

				if (ground_body)
					apply_force_at(	ground_body,
									scaled_v2d(-1, force),
									p->last_ground_contact_point);
			}
		}

		const F64 walking_speed= 6;
		if (p->on_ground_timer > 0.0) { // Walking
			V2d ground_vel= p->last_ground_velocity;
			V2d target_vel= {
				walking_speed*dir + ground_vel.x,
				p->body->velocity.y + ground_vel.y
			};
			V2d force= apply_velocity_target(	p->body,
												target_vel,
												100.0);
			if (ground_body)
				apply_force_at(	ground_body,
								scaled_v2d(-1, force),
								p->last_ground_contact_point);
		}

		if (jump && p->on_ground_timer > 0.0) {
			p->on_ground_timer= 0.0; // Prevent double jumps
			p->time_from_jump= 0.0;

			V2d vel_after_jump= {
				p->body->velocity.x,
				p->last_ground_velocity.y + 11,
			};
			V2d vel_dif= sub_v2d(p->body->velocity, vel_after_jump);
			rigidbody_set_velocity(p->body, vel_after_jump);

			if (ground_body) // @todo fix ground_body is NULL if we have already detached
				apply_impulse_at(	ground_body,
									scaled_v2d(rigidbody_mass(p->body), vel_dif),
									p->last_ground_contact_point);
		}

		if (p->on_ground_timer < 0.0) { // Air control
			V2d target_vel= {
				p->last_ground_velocity.x + dir*walking_speed*1.1,
				p->body->velocity.y,
			};
			apply_velocity_target(p->body, target_vel, dir ? 40 : 10);
		}

		p->tf.pos= add_v3d(p->body->tf.pos, (V3d) {0, 0.25, 0});
		p->tf.pos.y -= leg_space;

		F64 dif_to_target_v= (dir*walking_speed - p->body->velocity.x)*0.2;
		p->fake_dif= exp_drive(p->fake_dif, dif_to_target_v, dt*8);
		p->fake_dif= CLAMP(p->fake_dif, -0.1, 0.1);
		//p->tf.pos.x += p->fake_dif;

		{ // Camera
			V2d target_pos= {
				p->tf.pos.x*0.5 + cursor_p.x*0.5,
				p->tf.pos.y*0.5 + cursor_p.y*0.5,
			};

			V3d cam_pos= g_env.renderer->cam_pos;
			cam_pos.x= exp_drive(cam_pos.x, target_pos.x, dt*30);
			cam_pos.y= exp_drive(cam_pos.y, target_pos.y, dt*30);
			g_env.renderer->cam_pos= cam_pos;
		}

		p->dig_timer -= dt;
		if (dig && p->dig_timer <= 0.0f) {
			const V2d dist= {0.5, 0.7};
			local_persist U64 seed;
			const F64 rad= random_f32(0.9, 1.1, &seed);

			V2d dif= sub_v2d(cursor_on_world, v3d_to_v2d(p->tf.pos));
			dif= mul_v2d(dist, normalized_v2d(dif));

			V2d center= add_v2d(v3d_to_v2d(p->tf.pos), dif);
			set_grid_material_in_circle(center, rad, GRIDCELL_MATERIAL_AIR);
			p->dig_timer= 0.25;
		}

		int facing_dir= 0;
		{
			F64 rot= angle_qd(p->tf.rot);
			if (cursor_p.x < p->tf.pos.x) {
				facing_dir= -1;
				rot= exp_drive(rot, TAU/2.0, dt*15);
			} else {
				facing_dir= 1;
				rot= exp_drive(rot, 0, dt*25);
			}
			p->tf.rot= qd_by_axis((V3d) {0, 1, 0}, rot);
		}

		{ // Animations
			const F64 run_anim_mul= 1.1;
			if (dir) {
				// If moving
				p->idle_run_lerp= exp_drive(p->idle_run_lerp, 1, dt*15.0);
				p->clip_time += dt*dir*facing_dir*run_anim_mul;
			} else {
				p->clip_time += dt*run_anim_mul;
				p->idle_run_lerp= exp_drive(p->idle_run_lerp, 0, dt*15.0);
			}

			const Clip *idle_clip= (Clip *)res_by_id(p->idle_clip_id);
			const Clip *run_clip= (Clip *)res_by_id(p->run_clip_id);
			p->pose= lerp_pose(	calc_clip_pose(idle_clip, p->clip_time),
								calc_clip_pose(run_clip, p->clip_time),
								p->idle_run_lerp);
		}

		p->on_ground_timer -= dt;
		p->time_from_jump += dt;
	}
}

MOD_API void free_playerch(PlayerCh *p)
{
	free_rigidbody(p->body);
}
