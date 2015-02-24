#ifndef REVOLC_PHYSICS_PHYS_WORLD_H
#define REVOLC_PHYSICS_PHYS_WORLD_H

#include "build.h"
#include "core/vector.h"

#include <chipmunk/chipmunk.h>

typedef struct RigidBody {
	bool allocated;
	V3d pos;
	Qd rot;
	cpShape *shape;
	cpBody *body;
} RigidBody;

typedef struct PhysWorld {
	bool debug_draw;

	RigidBody bodies[MAX_RIGIDBODY_COUNT];
	U32 next_body;
	U32 body_count;
	cpSpace *space;
	cpShape *ground;
} PhysWorld;

/// @note Sets g_env.phys_world
REVOLC_API PhysWorld *create_physworld();
REVOLC_API void destroy_physworld(PhysWorld *w);

REVOLC_API U32 alloc_rigidbody(PhysWorld *w);
REVOLC_API void free_rigidbody(PhysWorld *w, U32 h);

REVOLC_API void upd_physworld(PhysWorld *w, F32 dt);

#endif // REVOLC_PHYSICS_PHYS_WORLD_H
