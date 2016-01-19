#include "grid.h"
#include "visual/renderer.h"


void upd_rtsgrid(RtsGrid *grid)
{
	Texel *rgrid = g_env.renderer->grid_ddraw_data;
	for (U32 i = 0; i < GRID_CELL_COUNT; ++i) {
		if (grid->cells[i].assigned) {
			rgrid[i].r = 255;
			rgrid[i].g = 255;
			rgrid[i].b = 255;
			rgrid[i].a = 255;
		}
	}
}
