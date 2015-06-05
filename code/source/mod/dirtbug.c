#include "animation/clip.h"
#include "animation/joint.h"
#include "audio/audiosystem.h"
#include "build.h"
#include "core/debug_print.h"
#include "core/ensure.h"
#include "core/grid.h"
#include "core/malloc.h"
#include "core/random.h"
#include "core/vector.h"
#include "core/scalar.h"
#include "global/env.h"
#include "game/world.h"
#include "physics/chipmunk_util.h"
#include "physics/rigidbody.h"
#include "physics/physworld.h"
#include "physics/query.h"
#include "platform/device.h"
#include "resources/resblob.h"
#include "visual/renderer.h" // screen_to_world_point, camera

#define DIRTBUG_MAX_WAYPOINT_COUNT 1

typedef struct DirtBug {
	V3d pos;
	V2d velocity_in;
	V2d velocity_out;
	F64 max_force_out;

	F64 hunger;
	F64 fear;

	V2d waypoints[DIRTBUG_MAX_WAYPOINT_COUNT];
	U8 waypoint_count;
	U8 next_waypoint_ix;
} DirtBug;

internal
void choose_safe_route(DirtBug *bug)
{

#	define WIDTH 16
#	define MAX_HEADS (512*GRID_RESO_PER_UNIT)
#	define MAX_CANDIDATES (WIDTH*WIDTH*GRID_RESO_PER_UNIT*GRID_RESO_PER_UNIT)
	V2d center= v3d_to_v2d(bug->pos);
	V2d occupied_area_size= {WIDTH, WIDTH};
	U8 *distance= NULL;
	GridDef grid= make_griddef(	center,
								occupied_area_size,
								GRID_RESO_PER_UNIT,
								sizeof(*distance));
	distance= ZERO_STACK_ALLOC(grid.sizeof_grid); // Careful with this
	set_grid_border(grid, distance, &(U8) {-1});

	// Search through space
	U32 heads[MAX_HEADS]= {wvec_to_gix(grid, center)};
	U32 head_count= 1;
	U32 candidates[MAX_CANDIDATES]= {};
	U32 candidate_count= 0;
	U32 cur_dist= 0;
	while (head_count > 0) {
		U32 cur= heads[--head_count];
		++cur_dist;

		const U32 sides[4]= {
			cur - 1,
			cur + grid.reso.x,
			cur + 1,
			cur - grid.reso.x,
		};
		for (U32 i= 0; i < 4; ++i) {
			const U32 side= sides[i];
			ensure(side < grid.cell_count);
			if (distance[side] != 0)
				continue; // Skip visited cells

			U32 phys_side= gix_to_gix(g_env.physworld->griddef, grid, side);
			bool ground= g_env.physworld->grid[phys_side].material != GRIDCELL_MATERIAL_AIR; // @todo Crashes near edges :::D
			if (ground && i == 3) { // There's ground below
				ensure(candidate_count < MAX_CANDIDATES);
				candidates[candidate_count++]= cur;
			}

			if (ground) {
				// Enlarge to all sides with air & unprocessed area
				distance[side]= cur_dist; 
				ensure(head_count < MAX_HEADS);
				heads[head_count++]= side;
			}
		}
	}
#	undef MAX_HEADS
#	undef MAX_CANDIDATES
#	undef WIDTH

	debug_print("candidates: %i", (int)candidate_count, candidates[0]);
	U32 chosen_cell= candidates[rand() % candidate_count];
	debug_print("chosen_cell: %i: %i, %i", (int)chosen_cell, gix_to_gvec(grid, chosen_cell).x, gix_to_gvec(grid, chosen_cell).y);
	bug->waypoints[0]= gix_to_wvec_center(grid, chosen_cell);
	debug_print("DirtBug target %f, %f", bug->waypoints[0].x, bug->waypoints[1].y);

	bug->next_waypoint_ix= 0;
	bug->waypoint_count= 1;

	ensure(bug->waypoint_count <= DIRTBUG_MAX_WAYPOINT_COUNT);
}

MOD_API void upd_dirtbug(DirtBug *bug, DirtBug *bug_end)
{
	//F64 dt= g_env.world->dt;

	for (; bug != bug_end; ++bug) {
		bug->velocity_out= (V2d) {bug->velocity_in.x, 0.0f};
		bug->max_force_out= 100.0;
		//debug_print("DirtBug target %f, %f", bug->waypoints[0].x, bug->waypoints[1].y);
		if (1 || bug->next_waypoint_ix >= bug->waypoint_count) {
			choose_safe_route(bug);
		} else {
			ensure(bug->next_waypoint_ix < DIRTBUG_MAX_WAYPOINT_COUNT);
			V2d dif= sub_v2d(bug->waypoints[bug->next_waypoint_ix], v3d_to_v2d(bug->pos));
			if (length_sqr_v2d(dif) < 1.0f*1.0f) {
				++bug->next_waypoint_ix;
			} else {
				dif= normalized_v2d(dif);
				bug->velocity_out= scaled_v2d(2.0, dif);
			}
		}
	}
}
