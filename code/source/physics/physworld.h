#ifndef REVOLC_PHYSICS_PHYS_WORLD_H
#define REVOLC_PHYSICS_PHYS_WORLD_H

#include "build.h"
#include "core/quaternion.h"
#include "core/vector.h"
#include "rigidbodydef.h"
#include "shapes.h"

#include <chipmunk/chipmunk.h>

#define HANDLE_NULL ((U8)-1)

typedef struct RigidBody {
	/// @todo Mechanism for separating input variables
	V2d input_force; // in
	char def_name[RES_NAME_SIZE]; // in

	V3d pos;
	V3d prev_pos;
	Qd rot;
	Qd prev_rot;
	/// @todo Group booleans to bit fields
	bool allocated;
	bool is_in_grid;
	bool is_static;
	bool shape_changed;
	bool has_own_shape; // Ignores shape of def_name

	Poly polys[MAX_SHAPES_PER_BODY];
	Circle circles[MAX_SHAPES_PER_BODY];
	U8 poly_count;
	U8 circle_count;

	cpShape *cp_shapes[MAX_SHAPES_PER_BODY];
	U8 cp_shape_count;
	cpBody *cp_body;
} RigidBody;

#define GRID_INDEX(x, y) \
		((((U32)((floor(x + 0.5))*GRID_RESO_PER_UNIT + GRID_WIDTH_IN_CELLS/2)) + \
	   	(U32)((floor(y + 0.5))*GRID_RESO_PER_UNIT + GRID_WIDTH_IN_CELLS/2)*GRID_WIDTH_IN_CELLS))

typedef struct GridCell {
	U8 static_portion; // Max == 64
	U8 dynamic_portion;
	bool is_static_edge;
} GridCell;

typedef struct PhysWorld {
	bool debug_draw;

	RigidBody bodies[MAX_RIGIDBODY_COUNT];
	U32 next_body, body_count;

	GridCell grid[GRID_CELL_COUNT];

	cpSpace *space;
} PhysWorld;

/// @note Sets g_env.phys_world
REVOLC_API void create_physworld();
REVOLC_API void destroy_physworld();

REVOLC_API U32 resurrect_rigidbody(const RigidBody *dead);
REVOLC_API void free_rigidbody(U32 h);
REVOLC_API void * storage_rigidbody();

REVOLC_API void upd_physworld(F64 dt);
REVOLC_API void post_upd_physworld();

#endif // REVOLC_PHYSICS_PHYS_WORLD_H
