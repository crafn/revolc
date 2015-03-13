#include "build.h"
#include "core/debug_print.h"
#include "core/ensure.h"
#include "core/vector.h"
#include "global/env.h"
#include "physics/rigidbody.h"
#include "platform/device.h"

// Character
typedef struct PlayerChar {
	V3d pos;

	// Cached
	RigidBody *body;
	Constraint *motor;
} PlayerChar;

MOD_API U32 resurrect_playerchar(PlayerChar *data)
{
	data->body= NULL;
	data->motor= NULL;
	return NULL_HANDLE;
}

/// @todo Replace with owned RigidBody 
MOD_API void supply_playerchar(	PlayerChar *p, PlayerChar *p_end,
								RigidBody *b, RigidBody *b_end)
{
	ensure(p_end - p == b_end - b);
	for (; p != p_end; ++p, ++b)
		p->body= b;
}

MOD_API void upd_playerchar(PlayerChar *p, PlayerChar *e)
{
	int dir= 0;
	if (g_env.device->key_down['c'])
		dir -= 1;
	if (g_env.device->key_down['b'])
		dir += 1;
	bool jump= g_env.device->key_pressed['v'];

	for (; p != e; ++p) {
		if (!p->body)
			continue;

		if (!p->motor) {
			p->motor= add_simplemotor(p->body);
			set_constraint_max_force(p->motor, 100.0);
		}
		set_simplemotor_rate(p->motor, -dir*20);

		if (jump) {
			apply_impulse_world(	p->body,
									(V2d) {0, 15},
									(V2d) {p->body->tf.pos.x, p->body->tf.pos.y});
		}

		p->pos= p->body->tf.pos;
	}
}

MOD_API void free_playerchar(PlayerChar *p)
{
	// No need to free p->motor -- destroyed along body
}
