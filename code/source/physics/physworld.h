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

	/// @todo Mechanism for separating input variables
	V2d input_force;
	char def_name[RES_NAME_SIZE];

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
REVOLC_API void create_physworld();
REVOLC_API void destroy_physworld();

REVOLC_API U32 alloc_rigidbody();
REVOLC_API void free_rigidbody(U32 h);
REVOLC_API void set_rigidbody(U32 h, V2d p, F64 r, RigidBodyDef *def);
REVOLC_API void * storage_rigidbody();
REVOLC_API void resurrect_rigidbody(U32 h, RigidBody *dead);

REVOLC_API void upd_physworld(F32 dt);

#endif // REVOLC_PHYSICS_PHYS_WORLD_H
