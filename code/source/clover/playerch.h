#ifndef CLOVER_PLAYERCH_H
#define CLOVER_PLAYERCH_H

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
	S8 walk_dir;
	S8 facing_dir;

	U8 active_slot; // 0-9
	S32 dirt_amount;

	// Cached
	ResId run_clip_id;
	ResId idle_clip_id;
	ResId dig_clip_id;
} PlayerCh;

MOD_API U32 resurrect_playerch(PlayerCh *p);
MOD_API void upd_playerch(PlayerCh *p, PlayerCh *p_e, RigidBody *body, RigidBody *body_e);
MOD_API void free_playerch(Handle h, PlayerCh *p);

#endif
