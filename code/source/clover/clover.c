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
#include "game/worldgen.h"
#include "physics/chipmunk_util.h"
#include "physics/rigidbody.h"
#include "physics/physworld.h"
#include "physics/query.h"
#include "platform/device.h"
#include "resources/resblob.h"
#include "visual/renderer.h" // screen_to_world_point, camera

MOD_API void clover_worldgen(World *w)
{
	U64 seed = 10;
	generate_test_world(w);
	{
		SlotVal init_vals[]= { };
		NodeGroupDef *def=
			(NodeGroupDef*)res_by_name(g_env.resblob, ResType_NodeGroupDef, "world_env");
		create_nodes(w, def, WITH_ARRAY_COUNT(init_vals), w->next_entity_id++);
	}

	spawn_visual_prop(w, (V3d) {-100, -50, -490}, 0, (V3d) {700, 250, 1}, "bg_mountain");

	spawn_visual_prop(w, (V3d) {20, -120, -100}, 0, (V3d) {220, 90, 1}, "bg_meadow");
	spawn_visual_prop(w, (V3d) {-70, -60, -200}, 0, (V3d) {220, 90, 1}, "bg_meadow");
	spawn_visual_prop(w, (V3d) {60, -80, -160}, 0, (V3d) {200, 90, 1}, "bg_meadow");
	spawn_visual_prop(w, (V3d) {150, -50, -300}, 0, (V3d) {200, 90, 1}, "bg_meadow");
	spawn_visual_prop(w, (V3d) {400, -70, -350}, 0, (V3d) {200, 110, 1}, "bg_meadow");


	for (int i= 0; i < 5; ++i) {
		V2d pos= {
			random_f64(-30.0, 30.0, &seed),
			0
		};
		pos.y= ground_surf_y(pos.x) + 2;

		//spawn_phys_prop(w, pos, "wbarrel", false);
		spawn_phys_prop(w, pos, "rollbot", false);
		spawn_phys_prop(w, pos, "wbox", false);
	}
	for (int i= -GRID_WIDTH + 1; i < GRID_WIDTH - 1; ++i) {
		F64 x= i/2.0;
		V3d p_front= {x, ground_surf_y(x) - 0.1, 0.01 + random_f64(0.0, 0.1, &seed)};
		//V3d p_back= {x, ground_surf_y(x) + 0.02, -0.1 + random_f64(-0.1, 0.0, &seed)};

		V2d a= {i - 0.2, ground_surf_y(x - 0.2)};
		V2d b= {i + 0.2, ground_surf_y(x + 0.2)};
		V2d tangent= sub_v2d(b, a);
		F64 rot= atan2(tangent.y, tangent.x) + random_f64(-0.3, 0.3, &seed);

		F64 scale= random_f64(0.85, 1.45, &seed);

		T3d tf= {{scale, scale, scale}, qd_by_axis((V3d){0, 0, 1}, rot), p_front};
		SlotVal init_vals[]= {
			{"body",	"tf",			WITH_DEREF_SIZEOF(&tf)},
		};
		NodeGroupDef *def=
			(NodeGroupDef*)res_by_name(g_env.resblob, ResType_NodeGroupDef, "grass");
		create_nodes(w, def, WITH_ARRAY_COUNT(init_vals), w->next_entity_id++);
	}

	// Compound test
	for (U32 i= 0; i < 5; ++i) {
		T3d tf= {(V3d) {1, 1, 1}, identity_qd(), {-3, 15, 0}};
		SlotVal init_vals[]= {
			{"body", "tf", WITH_DEREF_SIZEOF(&tf)},
		};
		NodeGroupDef *def=
			(NodeGroupDef*)res_by_name(g_env.resblob, ResType_NodeGroupDef, "test_comp");
		create_nodes(w, def, WITH_ARRAY_COUNT(init_vals), w->next_entity_id++);
	}

	{ // Player test
		T3d tf= {{1, 1, 1}, identity_qd(), {0, 12}};
		SlotVal init_vals[]= {
			{"char", "tf", WITH_DEREF_SIZEOF(&tf)},
		};
		NodeGroupDef *def=
			(NodeGroupDef*)res_by_name(g_env.resblob, ResType_NodeGroupDef, "playerch");
		create_nodes(w, def, WITH_ARRAY_COUNT(init_vals), w->next_entity_id++);
	}

	for (U32 i= 0; i < 1; ++i) { // Dirtbug
		T3d tf= {{1, 1, 1}, identity_qd(), {0, 20}};
		SlotVal init_vals[]= {
			{"body", "tf", WITH_DEREF_SIZEOF(&tf)},
		};
		NodeGroupDef *def=
			(NodeGroupDef*)res_by_name(g_env.resblob, ResType_NodeGroupDef, "dirtbug");
		create_nodes(w, def, WITH_ARRAY_COUNT(init_vals), w->next_entity_id++);
	}
}

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

	const F32 day_duration= 60.0*4;
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

	{ // Test ground drawing

		const Model *model= (Model*)res_by_name(g_env.resblob, ResType_Model, "dirt");
		const Mesh *mesh= model_mesh(model);

		V2i px_ll= {0, g_env.device->win_size.y};
		V2i px_tr= {g_env.device->win_size.x, 0};
		V3d w_ll= px_tf(px_ll, (V2i) {0}).pos;
		V3d w_tr= px_tf(px_tr, (V2i) {0}).pos;
		V2i ll= GRID_VEC_W(w_ll.x, w_ll.y);
		V2i tr= GRID_VEC_W(w_tr.x, w_tr.y);

		int draw_count= 0;
		for (int y= ll.y - 3; y < tr.y + 1; ++y) {
		for (int x= ll.x - 3; x < tr.x + 1; ++x) {
			if (x < 0 || y < 0 || x >= GRID_WIDTH_IN_CELLS || y >= GRID_WIDTH_IN_CELLS)
				continue;
			if (g_env.physworld->grid.cells[GRID_INDEX(x, y)].material == GRIDCELL_MATERIAL_AIR)
				continue;

			U64 seed= x + y*10000;
			float z= random_f32(-0.1, 0.05, &seed);
			float scale= 1.3*random_f32(1.5, 1.8, &seed);
			float rot= random_f32(-0.7, 0.7, &seed);
			float brightness= random_f32(0.7, 1.0, &seed);
			float x_dif= random_f32(-0.07, 0.07, &seed);
			float y_dif= random_f32(-0.07, 0.07, &seed);
			/*
			float z= 0;
			float scale= 1;
			float rot= 0;
			float brightness= 1;
			float x_dif= 0;
			float y_dif= 0;
			*/

			V3d size= {scale/GRID_RESO_PER_UNIT, scale/GRID_RESO_PER_UNIT, 1.0};
			V3d pos= {
				(x + 0.5)/GRID_RESO_PER_UNIT - GRID_WIDTH/2 + x_dif,
				(y + 0.5)/GRID_RESO_PER_UNIT - GRID_WIDTH/2 + y_dif,
				z
			};
			// @todo Cache mesh so we don't need to recalculate everything every frame
			drawcmd((T3d) {size, qd_by_axis((V3d) {0, 0, 1}, rot), pos},
					mesh_vertices(mesh), mesh->v_count,
					mesh_indices(mesh), mesh->i_count,
					model_texture(model, 0)->atlas_uv,
					(Color) {brightness, brightness, brightness, 1},
					0,
					0.0,
					2);
			++draw_count;
		}
		}
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
	F64 build_timer;

	U8 active_slot; // 0-9
	S32 dirt_amount;

	// Cached
	ResId run_clip_id;
	ResId idle_clip_id;
	ResId dig_clip_id;

	HANDLE(RigidBody) body;
} PlayerCh;

MOD_API U32 resurrect_playerch(PlayerCh *p)
{
	RigidBody dead_body= {
		.def_name= "playerch_body",
		.tf= p->tf,
	};
	p->body= resurrect_rigidbody(&dead_body);

	p->run_clip_id= res_id(ResType_Clip, "playerch_run");
	p->idle_clip_id= res_id(ResType_Clip, "playerch_idle");
	p->dig_clip_id= res_id(ResType_Clip, "playerch_dig");

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
	PlayerChFirstHitResult *result= data;
	if (result->ignore_body && result->ignore_body == body)
		return;
	result->did_hit= true;
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
	bool build_immediately= g_env.device->key_pressed[KEY_RMB];
	bool build= g_env.device->key_down[KEY_RMB];
	int select_slot= -1;
	for (int i= 0; i <= 9; ++i) {
		if (g_env.device->key_pressed[KEY_0 + i]) {
			select_slot= i - 1;
			if (select_slot == -1)
				select_slot= 9;
		}
	}
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
		RigidBody *body= get_rigidbody(p->body);

		{ // Levitate box
			V2d a= {
				body->tf.pos.x,
				body->tf.pos.y - box_lower_height - 0.02,
			};
			V2d b= {
				body->tf.pos.x,
				body->tf.pos.y - box_lower_height - leg_height,
			};

			PlayerChFirstHitResult result= {};
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
				V2d vel= body->velocity;
				yforce -= vel.y*50.0;

				V2d force= (V2d) {0, yforce};
				apply_force(body, force);

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
				body->velocity.y + ground_vel.y
			};
			V2d force= apply_velocity_target(	body,
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
				body->velocity.x,
				p->last_ground_velocity.y + 11,
			};
			V2d vel_dif= sub_v2d(body->velocity, vel_after_jump);
			rigidbody_set_velocity(body, vel_after_jump);

			if (ground_body) // @todo fix ground_body is NULL if we have already detached
				apply_impulse_at(	ground_body,
									scaled_v2d(rigidbody_mass(body), vel_dif),
									p->last_ground_contact_point);
		}

		if (p->on_ground_timer < 0.0) { // Air control
			V2d target_vel= {
				p->last_ground_velocity.x + dir*walking_speed*1.1,
				body->velocity.y,
			};
			apply_velocity_target(body, target_vel, dir ? 40 : 10);
		}

		p->tf.pos= add_v3d(body->tf.pos, (V3d) {0, 0.25, 0});
		p->tf.pos.y -= leg_space;

		F64 dif_to_target_v= (dir*walking_speed - body->velocity.x)*0.2;
		p->fake_dif= exp_drive(p->fake_dif, dif_to_target_v, dt*8);
		p->fake_dif= CLAMP(p->fake_dif, -0.1, 0.1);
		//p->tf.pos.x += p->fake_dif;

		{ // Camera
			V2d target_pos= {
				p->tf.pos.x, // + cursor_p.x*0.5,
				p->tf.pos.y, // + cursor_p.y*0.5,
			};

			V3d cam_pos= g_env.renderer->cam_pos;
			cam_pos.x= exp_drive(cam_pos.x, target_pos.x, dt*30);
			cam_pos.y= exp_drive(cam_pos.y, target_pos.y, dt*30);
			g_env.renderer->cam_pos= cam_pos;
		}

		if (select_slot >= 0) // Select active item slot
			p->active_slot= select_slot;

		const F64 dig_interval= 0.15;
		const F64 build_interval= 0.001;
		{ // Digging and building
			// @todo Separate digging and building as they seem to work quite differently
			p->dig_timer -= dt;
			p->build_timer -= dt;

			const F64 max_dig_reach= 1.5;
			const F64 max_build_reach= 3.0;

			PlayerChFirstHitResult result= {
				.ignore_body= body,
			};
			const V2d start= v3d_to_v2d(p->tf.pos);
			V2d end= cursor_on_world;
			V2d dig_reach_end= cursor_on_world;
			bool build_out_of_reach= false;
			V2d dif= sub_v2d(end, start);
			if (length_sqr_v2d(dif) > max_dig_reach*max_dig_reach) {
				dig_reach_end= add_v2d(	start,
										scaled_v2d(	max_dig_reach,
													normalized_v2d(dif)));
			}
			if (ABS(dif.x) > max_build_reach || ABS(dif.y) > max_build_reach) {
				build_out_of_reach= true;
			}
			phys_segment_query(
				start, dig_reach_end, 0.0,
				playerch_ray_callback, &result);

			V2d dig_center= result.ground_contact_point;
			if (!result.did_hit)
				dig_center= dig_reach_end;

			if (0) { // Max dig
				T3d tf= {{0.5, 0.5, 1}, identity_qd(), v2d_to_v3d(dig_reach_end)};
				drawcmd_model(	tf,
								(Model*)res_by_name(g_env.resblob, ResType_Model, "playerch_target"),
								(Color) {1, 1, 1, 1}, 1, 0);
			}

			local_persist U64 seed;
			if (grid_material_fullness_in_circle(dig_center, 0.5, GRIDCELL_MATERIAL_GROUND) > 0) {
				{ // Dig target
					T3d tf= {{0.4, 0.4, 1}, identity_qd(), v2d_to_v3d(dig_center)};
					drawcmd_model(	tf,
									(Model*)res_by_name(g_env.resblob, ResType_Model, "playerch_target"),
									(Color) {1, 1, 1, 1}, 1, 0);
				}
				if (dig && p->dig_timer <= 0.0f) { // Dig
					const F64 rad= random_f32(0.5, 0.6, &seed);
					p->dirt_amount += set_grid_material_in_circle(dig_center, rad, GRIDCELL_MATERIAL_AIR);
					p->dig_timer= dig_interval;
				}
			}

			if (	p->dirt_amount > 0 &&
					!build_out_of_reach &&
					grid_material_fullness_in_circle(end, 0.5, GRIDCELL_MATERIAL_GROUND) == 1) {
				{ // Build cursor
					T3d tf= {{1.3, 1.3, 1}, identity_qd(), v2d_to_v3d(end)};
					//F32 alpha= MIN(1 - ABS(dif.x)/max_build_reach, 1 - ABS(dif.y)/max_build_reach);
					//alpha= CLAMP(alpha, 0, 1);
					drawcmd_model(	tf,
									(Model*)res_by_name(g_env.resblob, ResType_Model, "playerch_target"),
									(Color) {0.7, 0.2, 0.1, 1.0}, 1, 0);
				}
				if ((build && p->build_timer <= 0.0f) || build_immediately) { // Build
					const F64 rad= random_f32(0.5, 0.6, &seed);
					p->dirt_amount -= set_grid_material_in_circle(end, rad, GRIDCELL_MATERIAL_GROUND);
					if (p->dirt_amount < 0)
						p->dirt_amount= 0;
					p->build_timer= build_interval;
				}
			}
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

			const Clip *idle_clip= (Clip*)res_by_id(p->idle_clip_id);
			const Clip *run_clip= (Clip*)res_by_id(p->run_clip_id);
			p->pose= lerp_pose(	calc_clip_pose(idle_clip, p->clip_time),
								calc_clip_pose(run_clip, p->clip_time),
								p->idle_run_lerp);

			if (p->dig_timer > 0.0) {
				const Clip *dig_clip= (Clip*)res_by_id(p->dig_clip_id);
				p->pose= calc_clip_pose(dig_clip, (dig_interval - p->dig_timer)/dig_interval);
			}
		}

		{ // Draw bag
			F64 scale_x= MIN(sqrt(p->dirt_amount*0.001)*1.1 + 0.3, 1.3);
			F64 scale_y= MIN(sqrt(p->dirt_amount*0.001) + 0.6, 1.3);
			T3d tf= {{scale_x, scale_y, 1}, identity_qd(), {p->tf.pos.x, p->tf.pos.y + 1}};
			drawcmd_model(	tf,
							(Model*)res_by_name(g_env.resblob, ResType_Model, "dirtbag"),
							(Color) {1, 1, 1, 1}, 0, 0);
		}

		p->on_ground_timer -= dt;
		p->time_from_jump += dt;
	}
}

MOD_API void free_playerch(Handle h, PlayerCh *p)
{
	// @todo Physics things to be "immediate mode" - then freeing is not needed
	free_rigidbody(p->body);
}

MOD_API void upd_grass(	ModelEntity *front,	ModelEntity *front_e,
						ModelEntity *back,	ModelEntity *back_e,
						RigidBody *body,	RigidBody *body_e)
{
	for (; front != front_e; ++front, ++back, ++body) {
		T3d tf= body->tf;

		back->tf= tf;
		back->tf.pos.z -= 0.05;

		front->tf= tf;
		front->tf.pos.z += 0.05;

		V2i cell_vec= GRID_VEC_W(body->tf.pos.x, body->tf.pos.y);
		V2i above_cell_vec= {cell_vec.x, cell_vec.y + 1};
		if (grid_cell(cell_vec).material == GRIDCELL_MATERIAL_AIR) {
			remove_node_group(g_env.world, body); // Kill me
		}
		if (grid_cell(above_cell_vec).material != GRIDCELL_MATERIAL_AIR) {
			remove_node_group(g_env.world, body); // Kill me
		}
	}
}
