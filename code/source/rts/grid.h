#ifndef REVOLC_RTS_GRID_H
#define REVOLC_RTS_GRID_H

#include "build.h"
#include "global/cfg.h"

typedef struct RtsGridCell {
	bool assigned;
} RtsGridCell;

typedef struct RtsGrid {
	RtsGridCell cells[GRID_CELL_COUNT];
} RtsGrid;

MOD_API void upd_rtsgrid(RtsGrid *grid_begin, RtsGrid *grid_end);

#endif // REVOLC_RTS_GRID_H
