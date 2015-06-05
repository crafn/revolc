#include "grid.h"

GridDef make_griddef(V2d center, V2d size, U32 reso_per_unit, U32 sizeof_cell)
{
	// @todo floor() is slow
	V2i origo_cell= {
		(S32)floor((center.x - size.x*0.5)*reso_per_unit + 0.5),
		(S32)floor((center.y - size.y*0.5)*reso_per_unit + 0.5),
	};
	V2i size_in_cells= {
		(S32)floor(size.x*reso_per_unit + 0.5),
		(S32)floor(size.y*reso_per_unit + 0.5),
	};

	return (GridDef) {
		.offset= origo_cell,
		.reso= size_in_cells,
		.reso_per_unit= reso_per_unit,
		.cell_count= size_in_cells.x*size_in_cells.y,
		.sizeof_cell= sizeof_cell,
		.sizeof_grid= sizeof_cell*size_in_cells.x*size_in_cells.y,
	};
}

void set_grid_border(GridDef def, void *grid_data, void *value)
{
	for (S32 i = 0; i < def.reso.x; ++i)
		memcpy(grid_data + i*def.sizeof_cell, value, def.sizeof_cell);

	for (S32 i = def.reso.x*(def.reso.y - 1); i < (S32)def.cell_count; ++i)
		memcpy(grid_data + i*def.sizeof_cell, value, def.sizeof_cell);

	for (S32 i = 0; i < (S32)def.cell_count; i += def.reso.x)
		memcpy(grid_data + i*def.sizeof_cell, value, def.sizeof_cell);

	for (S32 i = def.reso.x - 1; i < (S32)def.cell_count; i += def.reso.x)
		memcpy(grid_data + i*def.sizeof_cell, value, def.sizeof_cell);
}

U32 gvec_to_gix(GridDef def, V2i grid_cell)
{ return grid_cell.x - def.offset.x + (grid_cell.y - def.offset.y)*def.reso.x; }

U32 wvec_to_gix(GridDef def, V2d world_vec)
{ return gvec_to_gix(def, wvec_to_gvec(def, world_vec)); }

V2i gix_to_gvec(GridDef def, U32 ix)
{
	return (V2i) {
		ix % def.reso.x + def.offset.x,
		ix/def.reso.x + def.offset.y,
	};
}

V2d gix_to_wvec_center(GridDef def, U32 ix)
{
	ensure(def.reso_per_unit != 0);
	V2i gvec= gix_to_gvec(def, ix);
	return (V2d) {
		(gvec.x + 0.5)/def.reso_per_unit,
		(gvec.y + 0.5)/def.reso_per_unit,
	};
}

U32 gix_to_gix(GridDef dst, GridDef src, U32 src_ix)
{
	ensure(dst.reso_per_unit == src.reso_per_unit);
	return gvec_to_gix(dst, gix_to_gvec(src, src_ix));
}

V2i wvec_to_gvec(GridDef def, V2d world_vec)
{
	// @todo floor() is slow
	return (V2i) {
		(S32)floor(world_vec.x*def.reso_per_unit + 0.5),
		(S32)floor(world_vec.y*def.reso_per_unit + 0.5),
	};
}

bool is_wvec_in_grid(GridDef def, V2d world_vec)
{
	V2i vec= wvec_to_gvec(def, world_vec);
	if (vec.x < def.offset.x || vec.y < def.offset.y)
		return false;
	if (vec.x >= def.offset.x + def.reso.x || vec.y >= def.offset.y + def.reso.y)
		return false;
	return true;
}
