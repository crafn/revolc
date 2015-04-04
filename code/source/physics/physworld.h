#ifndef REVOLC_PHYSICS_PHYS_WORLD_H
#define REVOLC_PHYSICS_PHYS_WORLD_H

#include "build.h"
#include "rigidbody.h"
#include "rigidbodydef.h"

#include <chipmunk/chipmunk.h>

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

	cpSpace *cp_space;
} PhysWorld;

/// @note Sets g_env.physworld
REVOLC_API void create_physworld();
REVOLC_API void destroy_physworld();

REVOLC_API U32 resurrect_rigidbody(const RigidBody *dead);
REVOLC_API void free_rigidbody(RigidBody *b);
REVOLC_API void * storage_rigidbody();
REVOLC_API RigidBody * get_rigidbody(U32 h);

REVOLC_API void upd_physworld(F64 dt);
REVOLC_API void post_upd_physworld();
REVOLC_API void upd_phys_debugdraw();

#endif // REVOLC_PHYSICS_PHYS_WORLD_H
