#ifndef REVOLC_PHYSICS_RIGIDBODY_H
#define REVOLC_PHYSICS_RIGIDBODY_H

#include "build.h"
#include "core/transform.h"
#include "global/cfg.h"
#include "physmat.h"
#include "shapes.h"

#include <chipmunk/chipmunk.h>

struct RigidBody;

// Pointer to be retrieved from chipmunk bodies
typedef struct RigidBodyCpData {
	struct RigidBody *body;
} RigidBodyCpData;

typedef struct RigidBody {
	/// @todo Mechanism for separating input variables
	char def_name[RES_NAME_SIZE]; // in
	V2d input_force; // in, set every frame
	V2d target_velocity; // in
	F64 max_target_force; // in, set every frame

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

	RigidBodyCpData cp_data;

	// Cached
	cpShape *cp_shapes[MAX_SHAPES_PER_BODY];
	cpBody *cp_body;
	U8 cp_shape_count;
} RigidBody;


#define Constraint cpConstraint

REVOLC_API void apply_force(RigidBody *b, V2d force);
REVOLC_API void apply_force_at(RigidBody *b, V2d force, V2d world_point);
REVOLC_API void apply_torque(RigidBody *b, F64 torque);
REVOLC_API void apply_impulse_local(RigidBody *b, V2d i, V2d p);
REVOLC_API void apply_impulse_at(RigidBody *b, V2d i, V2d p);
REVOLC_API void rigidbody_set_velocity(RigidBody *b, V2d v);

REVOLC_API F64 rigidbody_mass(const RigidBody *b);
REVOLC_API V2d rigidbody_velocity_at(const RigidBody *b, V2d world_point);

REVOLC_API Constraint * add_simplemotor(RigidBody *b);
REVOLC_API void set_simplemotor_rate(Constraint *c, F64 rate);

REVOLC_API void set_constraint_max_force(Constraint *c, F64 force);

// Constraints are removed along bodies
REVOLC_API void remove_constraint(Constraint *c);

// Returns applied force
REVOLC_API V2d apply_velocity_target(RigidBody *b, V2d velocity, F64 max_force);

#endif // REVOLC_PHYSICS_RIGIDBODY_H
