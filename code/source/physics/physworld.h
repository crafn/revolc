#ifndef REVOLC_PHYSICS_PHYS_WORLD_H
#define REVOLC_PHYSICS_PHYS_WORLD_H

#include "build.h"
#include "core/vector.h"
#include "rigidbodydef.h"
#include "shapes.h"

#include <chipmunk/chipmunk.h>

#define HANDLE_NULL ((U8)-1)

typedef struct RigidBody {
	bool allocated;
	V3d pos;
	Qd rot;
	cpShape *cp_shapes[MAX_SHAPES_PER_BODY];
	U8 cp_shape_count;
	cpBody *cp_body;
} RigidBody;

typedef struct PhysWorld {
	bool debug_draw;

	RigidBody bodies[MAX_RIGIDBODY_COUNT];
	U32 next_body, body_count;

	//Circle circles[MAX_CIRCLE_COUNT];
	//U32 next_circle, circle_count;
	//Poly polys[MAX_POLY_COUNT];
	//U32 next_poly, poly_count;

	cpSpace *space;
	cpShape *ground;
} PhysWorld;

/// @note Sets g_env.phys_world
REVOLC_API PhysWorld *create_physworld();
REVOLC_API void destroy_physworld(PhysWorld *w);

REVOLC_API U32 alloc_rigidbody(PhysWorld *w);
REVOLC_API void free_rigidbody(PhysWorld *w, U32 h);
REVOLC_API void set_rigidbody(PhysWorld *w, U32 h, V2d p, F64 r, RigidBodyDef *def);

REVOLC_API void upd_physworld(PhysWorld *w, F32 dt);

#endif // REVOLC_PHYSICS_PHYS_WORLD_H
