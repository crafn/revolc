#ifndef REVOLC_PHYSICS_RIGIDBODY_H
#define REVOLC_PHYSICS_RIGIDBODY_H

#include "build.h"
#include "core/math.h"
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

	// @todo Shapes to own arrays
	Poly polys[MAX_SHAPES_PER_BODY];
	Circle circles[MAX_SHAPES_PER_BODY];
	U8 poly_count;
	U8 circle_count;

	RigidBodyCpData cp_data;

	// @todo Could store these in separate array for perf
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

// @todo Joints to immediate-mode api
REVOLC_API Constraint *add_simplemotor(RigidBody *b);
REVOLC_API void set_simplemotor_rate(Constraint *c, F64 rate);

REVOLC_API void set_constraint_max_force(Constraint *c, F64 force);

// Constraints are removed along bodies
REVOLC_API void remove_constraint(Constraint *c);


// Immediate-mode joints. Anchor points in world coordinates.

// @todo Rename joints attached to static ground to *_single and normal ones *
REVOLC_API void apply_slide_joint(RigidBody *body, V2d body_p, V2d ground_p, F64 min, F64 max);
REVOLC_API void apply_groove_joint(RigidBody *body, V2d ground_p_1, V2d ground_p_2);
REVOLC_API void apply_spring_joint(	RigidBody *a, RigidBody *b, V2d a_p, V2d b_p,
									F64 length, F64 stiffness, F64 damping);
REVOLC_API void apply_spring_joint_single(	RigidBody *body, V2d body_p, V2d ground_p,
											F64 length, F64 stiffness, F64 damping);

// Returns applied force
REVOLC_API V2d apply_velocity_target(RigidBody *b, V2d velocity, F64 max_force);


struct WArchive;
struct RArchive;
REVOLC_API void pack_rigidbody(	struct WArchive *ar,
								const RigidBody *begin,
								const RigidBody *end);
REVOLC_API void unpack_rigidbody(	struct RArchive *ar,
									RigidBody *begin,
									RigidBody *end);

#endif // REVOLC_PHYSICS_RIGIDBODY_H
