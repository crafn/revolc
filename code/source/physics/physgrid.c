#include "core/archive.h"
#include "physgrid.h"

void pack_physgrid(WArchive *ar, const PhysGrid *begin, const PhysGrid *end)
{
	for (const PhysGrid *grid = begin; grid != end; ++grid) {
		pack_buf(ar, WITH_DEREF_SIZEOF(&grid->def));

		const U32 mat_grid_size = grid->def.cell_count;
		U8 *mat_grid = ALLOC(frame_ator(), mat_grid_size, "mat_grid");
		for (U32 i = 0; i < grid->def.cell_count; ++i) {
			mat_grid[i] = grid->cells[i].material;
			ensure(mat_grid[i] < 2);
		}
		pack_buf(ar, mat_grid, mat_grid_size);
	}
}

void unpack_physgrid(RArchive *ar, PhysGrid *begin, PhysGrid *end)
{
	for (PhysGrid *grid = begin; grid != end; ++grid) {
		unpack_buf(ar, WITH_DEREF_SIZEOF(&grid->def));

		const U32 mat_grid_size = grid->def.cell_count;
		U8 *mat_grid = ALLOC(frame_ator(), mat_grid_size, "mat_grid");
		unpack_buf(ar, mat_grid, mat_grid_size); 
		for (U32 i = 0; i < grid->def.cell_count; ++i) {
			ensure(mat_grid[i] < 2);
			grid->cells[i] = (GridCell) {
				.material = mat_grid[i],
			};
		}
	}
}

