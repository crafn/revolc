#ifndef REVOLC_PHYSICS_PHYSGRID_H
#define REVOLC_PHYSICS_PHYSGRID_H

#include "build.h"

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

struct WArchive;
struct RArchive;
void pack_physgrid(struct WArchive *ar, const PhysGrid *begin, const PhysGrid *end);
void unpack_physgrid(struct RArchive *ar, PhysGrid *begin, PhysGrid *end);

#endif // REVOLC_PHYSICS_PHYSGRID_H
