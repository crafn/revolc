#include "playerch.h"

void init_playerch(PlayerCh *p)
{
	*p = (PlayerCh) {
		.tf = identity_t3d(),
		.facing_dir = 1,
	};
}

U32 resurrect_playerch(PlayerCh *p)
{
	p->run_clip_id = res_id(ResType_Clip, "playerch_run");
	p->idle_clip_id = res_id(ResType_Clip, "playerch_idle");
	p->dig_clip_id = res_id(ResType_Clip, "playerch_dig");

	return NULL_HANDLE;
}

// FPS independent exponential decay towards `target`
internal
F64 exp_drive(F64 value, F64 target, F64 dt)
{
	return (value - target)*pow(2, -dt) + target;
}

typedef struct PlayerChFirstHitResult {
	const RigidBody *ignore_body;

	bool did_hit;
	F64 ray_fraction;
	RigidBody *ground_body;
	V2d ground_velocity;
	V2d ground_contact_point;
} PlayerChFirstHitResult;

internal
void playerch_ray_callback(RigidBody *body, V2d point, V2d normal, F64 fraction, void *data)
{
	PlayerChFirstHitResult *result = data;
	if (result->ignore_body && result->ignore_body == body)
		return;
	result->did_hit = true;
	if (!result->ground_body || fraction < result->ray_fraction) {
		result->ray_fraction = fraction;
		result->ground_velocity = rigidbody_velocity_at(body, point);
		result->ground_body = body;
		result->ground_contact_point = point;
	}
}

void upd_playerch(PlayerCh *p, PlayerCh *p_e, RigidBody *body, RigidBody *body_e)
{
	bool in_control = g_env.netstate->authority;
	bool jump = g_env.device->key_pressed[KEY_SPACE];
	bool dig = g_env.device->key_down[KEY_LMB];
	bool build_immediately = g_env.device->key_pressed[KEY_RMB];
	bool build = g_env.device->key_down[KEY_RMB];
	int select_slot = -1;
	for (int i = 0; i <= 9; ++i) {
		if (g_env.device->key_pressed[KEY_0 + i]) {
			select_slot = i - 1;
			if (select_slot == -1)
				select_slot = 9;
		}
	}
	V2d cursor_on_world = screen_to_world_point(g_env.device->cursor_pos);

	F64 dt = g_env.world->dt;
	V2d cursor_p = screen_to_world_point(g_env.device->cursor_pos);

	for (; p != p_e; ++p, ++body) {
		const F64 box_lower_height = 0.5; // @todo Should be in data
		const F64 leg_height = 0.65;
		const F64 on_ground_timer_start = 0.2; // Time to jump after ground has disappeared
		//const F64 leg_width = 0.2;
		F64 leg_space = leg_height;
		RigidBody *ground_body = NULL;

		if (in_control) {
			// @todo Input from configurable input layer
			p->walk_dir = 0;
			if (g_env.device->key_down['a'])
				p->walk_dir -= 1;
			if (g_env.device->key_down['d'])
				p->walk_dir += 1;
		}

		{ // Levitate box
			V2d a = {
				body->tf.pos.x,
				body->tf.pos.y - box_lower_height - 0.02,
			};
			V2d b = {
				body->tf.pos.x,
				body->tf.pos.y - box_lower_height - leg_height,
			};

			PlayerChFirstHitResult result = {};
			// @todo Multiple segments (or radius for the segment)
			phys_segment_query(
				a, b, 0.0,
				playerch_ray_callback, &result);

			F64 leg_space = leg_height*result.ray_fraction;

			if (result.ground_body && p->time_from_jump > 0.2) {
				p->on_ground_timer = on_ground_timer_start;
				p->last_ground_velocity = result.ground_velocity;
				p->last_ground_contact_point = result.ground_contact_point;
				ground_body = result.ground_body;

				const F64 max_force = 200;
				F64 yforce = (leg_height - leg_space)*300;
				if (ABS(yforce) > max_force)
					yforce = SIGN(yforce)*max_force;

				// Damping
				V2d vel = body->velocity;
				yforce -= vel.y*50.0;

				V2d force = (V2d) {0, yforce};
				apply_force(body, force);

				if (ground_body)
					apply_force_at(	ground_body,
									scaled_v2d(-1, force),
									p->last_ground_contact_point);
			}
		}

		const F64 walking_speed = 6;
		if (p->on_ground_timer > 0.0) { // Walking
			V2d ground_vel = p->last_ground_velocity;
			V2d target_vel = {
				walking_speed*p->walk_dir + ground_vel.x,
				body->velocity.y + ground_vel.y
			};
			V2d force = apply_velocity_target(	body,
												target_vel,
												100.0);
			if (ground_body)
				apply_force_at(	ground_body,
								scaled_v2d(-1, force),
								p->last_ground_contact_point);
		}

		if (jump && p->on_ground_timer > 0.0) {
			p->on_ground_timer = 0.0; // Prevent double jumps
			p->time_from_jump = 0.0;

			V2d vel_after_jump = {
				body->velocity.x,
				p->last_ground_velocity.y + 11,
			};
			V2d vel_dif = sub_v2d(body->velocity, vel_after_jump);
			rigidbody_set_velocity(body, vel_after_jump);

			if (ground_body) // @todo fix ground_body is NULL if we have already detached
				apply_impulse_at(	ground_body,
									scaled_v2d(rigidbody_mass(body), vel_dif),
									p->last_ground_contact_point);
		}

		if (p->on_ground_timer < 0.0) { // Air control
			V2d target_vel = {
				p->last_ground_velocity.x + p->walk_dir*walking_speed*1.1,
				body->velocity.y,
			};
			apply_velocity_target(body, target_vel, p->walk_dir ? 40 : 10);
		}

		p->tf.pos = add_v3d(body->tf.pos, (V3d) {0, 0.25, 0});
		p->tf.pos.y -= leg_space;

		F64 dif_to_target_v = (p->walk_dir*walking_speed - body->velocity.x)*0.2;
		p->fake_dif = exp_drive(p->fake_dif, dif_to_target_v, dt*8);
		p->fake_dif = CLAMP(p->fake_dif, -0.1, 0.1);
		//p->tf.pos.x += p->fake_dif;

		if (in_control) { // Camera
			V2d target_pos = {
				p->tf.pos.x, // + cursor_p.x*0.5,
				p->tf.pos.y, // + cursor_p.y*0.5,
			};

			V3d cam_pos = g_env.renderer->cam_pos;
			cam_pos.x = exp_drive(cam_pos.x, target_pos.x, dt*30);
			cam_pos.y = exp_drive(cam_pos.y, target_pos.y, dt*30);
			g_env.renderer->cam_pos = cam_pos;
		}

		if (select_slot >= 0) // Select active item slot
			p->active_slot = select_slot;

		const F64 dig_interval = 0.15;
		const F64 build_interval = 0.001;
		{ // Digging and building
			// @todo Separate digging and building as they seem to work quite differently
			p->dig_timer -= dt;
			p->build_timer -= dt;

			const F64 max_dig_reach = 1.5;
			const F64 max_build_reach = 3.0;

			PlayerChFirstHitResult result = {
				.ignore_body = body,
			};
			const V2d start = v3d_to_v2d(p->tf.pos);
			V2d end = cursor_on_world;
			V2d dig_reach_end = cursor_on_world;
			bool build_out_of_reach = false;
			V2d dif = sub_v2d(end, start);
			if (length_sqr_v2d(dif) > max_dig_reach*max_dig_reach) {
				dig_reach_end = add_v2d(	start,
										scaled_v2d(	max_dig_reach,
													normalized_v2d(dif)));
			}
			if (ABS(dif.x) > max_build_reach || ABS(dif.y) > max_build_reach) {
				build_out_of_reach = true;
			}
			phys_segment_query(
				start, dig_reach_end, 0.0,
				playerch_ray_callback, &result);

			V2d dig_center = result.ground_contact_point;
			if (!result.did_hit)
				dig_center = dig_reach_end;

			if (0) { // Max dig
				T3d tf = {{0.5, 0.5, 1}, identity_qd(), v2d_to_v3d(dig_reach_end)};
				drawcmd_model(	tf,
								(Model*)res_by_name(g_env.resblob, ResType_Model, "playerch_target"),
								(Color) {1, 1, 1, 1}, 1, 0);
			}

			local_persist U64 seed;
			if (grid_material_fullness_in_circle(dig_center, 0.5, GRIDCELL_MATERIAL_GROUND) > 0) {
				{ // Dig target
					T3d tf = {{0.4, 0.4, 1}, identity_qd(), v2d_to_v3d(dig_center)};
					drawcmd_model(	tf,
									(Model*)res_by_name(g_env.resblob, ResType_Model, "playerch_target"),
									(Color) {1, 1, 1, 1}, 1, 0);
				}
				if (dig && p->dig_timer <= 0.0f) { // Dig
					const F64 rad = random_f32(0.5, 0.6, &seed);
					p->dirt_amount += set_grid_material_in_circle(dig_center, rad, GRIDCELL_MATERIAL_AIR);
					p->dig_timer = dig_interval;
				}
			}

			if (	p->dirt_amount > 0 &&
					!build_out_of_reach &&
					grid_material_fullness_in_circle(end, 0.5, GRIDCELL_MATERIAL_GROUND) == 1) {
				{ // Build cursor
					T3d tf = {{1.3, 1.3, 1}, identity_qd(), v2d_to_v3d(end)};
					//F32 alpha = MIN(1 - ABS(dif.x)/max_build_reach, 1 - ABS(dif.y)/max_build_reach);
					//alpha = CLAMP(alpha, 0, 1);
					drawcmd_model(	tf,
									(Model*)res_by_name(g_env.resblob, ResType_Model, "playerch_target"),
									(Color) {0.7, 0.2, 0.1, 1.0}, 1, 0);
				}
				if ((build && p->build_timer <= 0.0f) || build_immediately) { // Build
					const F64 rad = random_f32(0.5, 0.6, &seed);
					p->dirt_amount -= set_grid_material_in_circle(end, rad, GRIDCELL_MATERIAL_GROUND);
					if (p->dirt_amount < 0)
						p->dirt_amount = 0;
					p->build_timer = build_interval;
				}
			}
		}

		{
			F64 rot = angle_qd(p->tf.rot);
			if (in_control) {
				if (cursor_p.x < p->tf.pos.x)
					p->facing_dir = -1;
				else
					p->facing_dir = 1;
			}

			if (p->facing_dir < 0)
				rot = exp_drive(rot, TAU/2.0, dt*15);
			else
				rot = exp_drive(rot, 0, dt*25);
			p->tf.rot = qd_by_axis((V3d) {0, 1, 0}, rot);
		}

		{ // Animations
			const F64 run_anim_mul = 1.1;
			if (p->walk_dir) {
				// If moving
				p->idle_run_lerp = exp_drive(p->idle_run_lerp, 1, dt*15.0);
				p->clip_time += dt*p->walk_dir*p->facing_dir*run_anim_mul;
			} else {
				p->clip_time += dt*run_anim_mul;
				p->idle_run_lerp = exp_drive(p->idle_run_lerp, 0, dt*15.0);
			}

			const Clip *idle_clip = (Clip*)res_by_id(p->idle_clip_id);
			const Clip *run_clip = (Clip*)res_by_id(p->run_clip_id);
			p->pose = lerp_pose(	calc_clip_pose(idle_clip, p->clip_time),
								calc_clip_pose(run_clip, p->clip_time),
								p->idle_run_lerp);

			if (p->dig_timer > 0.0) {
				const Clip *dig_clip = (Clip*)res_by_id(p->dig_clip_id);
				p->pose = calc_clip_pose(dig_clip, (dig_interval - p->dig_timer)/dig_interval);
			}
		}

		{ // Draw bag
			F64 scale_x = MIN(sqrt(p->dirt_amount*0.001)*1.1 + 0.3, 1.3);
			F64 scale_y = MIN(sqrt(p->dirt_amount*0.001) + 0.6, 1.3);
			T3d tf = {{scale_x, scale_y, 1}, identity_qd(), {p->tf.pos.x, p->tf.pos.y + 1}};
			drawcmd_model(	tf,
							(Model*)res_by_name(g_env.resblob, ResType_Model, "dirtbag"),
							(Color) {1, 1, 1, 1}, 0, 0);
		}

		p->on_ground_timer -= dt;
		p->time_from_jump += dt;
	}
}

void free_playerch(Handle h, PlayerCh *p)
{
}
