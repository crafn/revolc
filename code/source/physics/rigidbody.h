#ifndef REVOLC_PHYSICS_RIGIDBODY_H
#define REVOLC_PHYSICS_RIGIDBODY_H

#include "build.h"
#include "core/transform.h"
#include "global/cfg.h"
#include "shapes.h"

#include <chipmunk/chipmunk.h>

typedef struct RigidBody {
	/// @todo Mechanism for separating input variables
	V2d input_force; // in
	char def_name[RES_NAME_SIZE]; // in

	T3d tf;
	T3d prev_tf;
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

	cpShape *cp_shapes[MAX_SHAPES_PER_BODY];
	U8 cp_shape_count;
	cpBody *cp_body;
} RigidBody;

REVOLC_API void apply_torque(RigidBody *b, F64 torque);

#endif // REVOLC_PHYSICS_RIGIDBODY_H
