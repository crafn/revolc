#ifndef REVOLC_CORE_GRID_H
#define REVOLC_CORE_GRID_H

#include "build.h"
#include "vector.h"

// Utility for grid calculations
typedef struct GridDef {
	V2i offset;
	V2i reso;
	U32 reso_per_unit;
	U32 cell_count;
	U32 sizeof_cell;
	U32 sizeof_grid;
} GridDef;

// grid_vec is measured from world origo, not grid origo (makes using patches of grid with different offsets easy as they have the same grid vectors)
REVOLC_API
GridDef make_griddef(V2d center, V2d size, U32 reso_per_unit, U32 sizeof_cell);
REVOLC_API  void set_grid_border(GridDef def, void *grid_data, void *value);

REVOLC_API U32 gvec_to_gix(GridDef def, V2i grid_vec);
REVOLC_API U32 wvec_to_gix(GridDef def, V2d world_vec);
REVOLC_API V2i gix_to_gvec(GridDef def, U32 ix);
REVOLC_API V2d gix_to_wvec_center(GridDef def, U32 ix);
REVOLC_API V2i wvec_to_gvec(GridDef def, V2d world_vec);
REVOLC_API U32 gix_to_gix(GridDef dst, GridDef src, U32 src_ix);
REVOLC_API bool is_wvec_in_grid(GridDef def, V2d world_vec);

#endif // REVOLC_CORE_GRID_H
