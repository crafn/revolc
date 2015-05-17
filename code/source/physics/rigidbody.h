#ifndef REVOLC_PHYSICS_RIGIDBODY_H
#define REVOLC_PHYSICS_RIGIDBODY_H

#include "build.h"
#include "core/transform.h"
#include "global/cfg.h"
#include "physmat.h"
#include "shapes.h"

#include <chipmunk/chipmunk.h>

typedef struct RigidBody {
	/// @todo Mechanism for separating input variables
	V2d input_force; // in
	char def_name[RES_NAME_SIZE]; // in

	T3d tf;
	T3d prev_tf;
	V2d velocity;
	/// @todo Bit fields
	bool allocated;
	bool is_in_grid;
	bool is_static;
	bool shape_changed;
	bool tf_changed;
	bool has_own_shape; // Ignores shape of def_name

	Poly polys[MAX_SHAPES_PER_BODY];
	Circle circles[MAX_SHAPES_PER_BODY];
	U8 poly_count;
	U8 circle_count;

	// Cached
	cpShape *cp_shapes[MAX_SHAPES_PER_BODY];
	cpBody *cp_body;
	U8 cp_shape_count;
} RigidBody;

#define Constraint cpConstraint

REVOLC_API void apply_force(RigidBody *b, V2d force);
REVOLC_API void apply_torque(RigidBody *b, F64 torque);
REVOLC_API void apply_impulse_local(RigidBody *b, V2d i, V2d p);
REVOLC_API void apply_impulse_world(RigidBody *b, V2d i, V2d p);

REVOLC_API Constraint * add_simplemotor(RigidBody *b);
REVOLC_API void set_simplemotor_rate(Constraint *c, F64 rate);

REVOLC_API void set_constraint_max_force(Constraint *c, F64 force);

// Constraints are removed along bodies
REVOLC_API void remove_constraint(Constraint *c);

REVOLC_API void apply_velocity_target(RigidBody *b, V2d velocity, F64 max_force);

REVOLC_API F64 rigidbody_mass(const RigidBody *b);

#endif // REVOLC_PHYSICS_RIGIDBODY_H
