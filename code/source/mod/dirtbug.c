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

typedef struct DirtBug {
	RigidBody *body;
} DirtBug;


MOD_API void upd_dirtbug(DirtBug *bug, DirtBug *bug_end)
{
	//F64 dt= g_env.world->dt;

	for (; bug != bug_end; ++bug) {
		//debug_print("DirtBug %f", bug->body->tf.pos.x);
	}
}
