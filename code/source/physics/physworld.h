#ifndef REVOLC_PHYSICS_PHYS_WORLD_H
#define REVOLC_PHYSICS_PHYS_WORLD_H

#include "build.h"
#include "core/grid.h"
#include "rigidbody.h"
#include "rigidbodydef.h"

#include <chipmunk/chipmunk.h>

// @todo Replace these with core/grid functions
#define GRID_INDEX_W(x, y) \
		((U32)((floor((x + 0.0)*GRID_RESO_PER_UNIT) + GRID_WIDTH_IN_CELLS/2)) + \
	   	(U32)((floor((y + 0.0)*GRID_RESO_PER_UNIT) + GRID_WIDTH_IN_CELLS/2)*GRID_WIDTH_IN_CELLS))

#define GRID_VEC_W(x, y) \
		(V2i) { \
			(U32)(floor((x + 0.0)*GRID_RESO_PER_UNIT) + GRID_WIDTH_IN_CELLS/2), \
			(U32)(floor((y + 0.0)*GRID_RESO_PER_UNIT) + GRID_WIDTH_IN_CELLS/2) \
		}

#define GRID_INDEX(x, y) ((x) + (y)*GRID_WIDTH_IN_CELLS)

#define GRIDCELL_MATERIAL_AIR 0
#define GRIDCELL_MATERIAL_GROUND 1

//#define USE_FLUIDS
#define SIMPLE_FLUID_SWAP_DYNAMICS

typedef struct GridCell {
	// @todo Rethink division between static/dynamic when fluids are ready
	U8 body_portion;
	bool is_static_edge;

	U8 material; // Ground or what

	U8 draw_something; // For debugging
#ifdef USE_FLUID_PROTO
	// @todo Pack to bitfields when design is stabilized
	U8 water;
	bool already_swapped; // To prevent double swap in sim, could be temp
	U16 pressure;
#ifndef SIMPLE_FLUID_SWAP_DYNAMICS
	U16 fluid_area_id; // Used in sim, could be temp
	U16 potential; // Used in flow analysis, could be temp
	U32 edge_begin_index; // Could be temp
	U8 fluid_path_count; // Could be temp
#endif
#endif
} GridCell;

typedef struct PhysGrid {
	GridDef def;
	GridCell cells[GRID_CELL_COUNT];
	bool modified; // Set to true after making changes to grid
} PhysGrid;

typedef struct PhysWorld {
	bool debug_draw;

	RigidBody bodies[MAX_RIGIDBODY_COUNT];
	U32 next_body, body_count;

	PhysGrid grid;

	cpSpace *cp_space;
	cpBody *cp_ground_body; // Recreated when 
	RigidBody ground_body; // So that every cpShape has a RigidBody
} PhysWorld;

/// @note Sets g_env.physworld
REVOLC_API void create_physworld();
REVOLC_API void destroy_physworld();

REVOLC_API U32 resurrect_rigidbody(const RigidBody *dead);
REVOLC_API void free_rigidbody(RigidBody *b);
REVOLC_API void * storage_rigidbody();
REVOLC_API RigidBody * get_rigidbody(U32 h);

REVOLC_API U32 resurrect_physgrid(const PhysGrid *dead);
REVOLC_API void *storage_physgrid();

REVOLC_API void upd_physworld(F64 dt);
REVOLC_API void post_upd_physworld();
REVOLC_API void upd_phys_rendering();

// 0 = empty, 1 = partial, 2 = full
REVOLC_API U32 grid_material_fullness_in_circle(V2d center, F64 rad, U8 material);
// Returns number of changed cells
REVOLC_API U32 set_grid_material_in_circle(V2d center, F64 rad, U8 material);
REVOLC_API GridCell grid_cell(V2i vec);

#endif // REVOLC_PHYSICS_PHYS_WORLD_H
