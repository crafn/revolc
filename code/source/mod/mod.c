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
} PlayerChar;

MOD_API U32 resurrect_playerchar(PlayerChar *data)
{
	data->body= NULL;
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

MOD_API void upd_playerchar(PlayerChar *t, PlayerChar *e)
{
	int dir= 0;
	if (g_env.device->key_down['c'])
		dir -= 1;
	if (g_env.device->key_down['b'])
		dir += 1;

	for (; t != e; ++t) {
		if (!t->body)
			continue;

		apply_torque(t->body, -dir*60);
		t->pos= t->body->tf.pos;
	}
}
