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

#define DIRTBUG_MAX_WAYPOINT_COUNT 64

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

	U32 candidates[MAX_CANDIDATES]= {};
	U32 candidate_count= 0;

	{ // Search through space
		U32 heads[MAX_HEADS][2];
		U32 head_count= 0;
		U32 head_pool= 0;
		U32 cur_dist= 1;
		heads[head_count++][head_pool]= wvec_to_gix(grid, center);
		while (head_count > 0) {
			U32 next_head_pool= (head_pool + 1) % 2;
			U32 next_head_count= 0;

			for (U32 i= 0; i < head_count; ++i) {
				const U32 cur= heads[i][head_pool];

				U32 phys_cur= gix_to_gix(g_env.physworld->griddef, grid, cur);
				bool ground_at_cur = g_env.physworld->grid[phys_cur].material != GRIDCELL_MATERIAL_AIR; // @todo Crashes near edges :::D
				if (ground_at_cur)
					continue;

				// @todo Compress
				const U32 sides[4]= {
					cur - 1,
					cur + grid.reso.x,
					cur + 1,
					cur - grid.reso.x,
				};
				for (U32 s= 0; s < 4; ++s) {
					const U32 side= sides[s];
					ensure(side < grid.cell_count);

					if (distance[side] != 0)
						continue; // Skip visited cells

					U32 phys_side= gix_to_gix(g_env.physworld->griddef, grid, side);
					bool ground_at_side = g_env.physworld->grid[phys_side].material != GRIDCELL_MATERIAL_AIR; // @todo Crashes near edges :::D
					if (ground_at_side && s == 3) {
						ensure(candidate_count < MAX_CANDIDATES);
						candidates[candidate_count++]= cur;
					}

					if (!ground_at_side) {
						// Enlarge to all sides with air & unprocessed area
						distance[side]= cur_dist; 
						ensure(next_head_count < MAX_HEADS);
						heads[next_head_count++][next_head_pool]= side;
					}
				}
			}

			head_pool= next_head_pool;
			head_count= next_head_count;
			++cur_dist;
		}
	}
#	undef MAX_HEADS
#	undef MAX_CANDIDATES
#	undef WIDTH

	U32 chosen_cell= 0;
	{ // Choose target
		debug_print("candidates: %i", (int)candidate_count, candidates[0]);
		if (candidate_count == 0)
			return;
		for (U32 i= 0; i < candidate_count; ++i) {
			//U32 phys_side= gix_to_gix(g_env.physworld->griddef, grid, candidates[i]);
			//g_env.physworld->grid[phys_side].draw_something= 1;
		}
		chosen_cell= candidates[rand() % candidate_count];
	}

	{ // Backtrack from chosen cell
		S32 cur_dist= distance[chosen_cell];
		U32 cur_cell= chosen_cell;
		if (cur_dist >= DIRTBUG_MAX_WAYPOINT_COUNT)
		{
			// @todo Try again
			debug_print("Too long path");
			return;
		}
		bug->waypoint_count= cur_dist + 1;
		bug->next_waypoint_ix= 0;
		while (cur_dist >= 0) {
			bug->waypoints[cur_dist]= gix_to_wvec_center(grid, cur_cell);
			bug->waypoints[cur_dist].y += 1.5;

			U32 phys_side= gix_to_gix(g_env.physworld->griddef, grid, cur_cell);
			g_env.physworld->grid[phys_side].draw_something= 1;

			// @todo Compress
			const U32 sides[4]= {
				cur_cell - 1,
				cur_cell + grid.reso.x,
				cur_cell + 1,
				cur_cell - grid.reso.x,
			};

			--cur_dist;
			for (U32 i= 0; i < 4; ++i) {
				if (distance[sides[i]] == cur_dist)
				{
					cur_cell= sides[i];
					break;
				}
			}
		}
		ensure(bug->waypoint_count <= DIRTBUG_MAX_WAYPOINT_COUNT);
	}
}

MOD_API void upd_dirtbug(DirtBug *bug, DirtBug *bug_end)
{
	//F64 dt= g_env.world->dt;

	for (; bug != bug_end; ++bug) {
		bug->velocity_out= (V2d) {bug->velocity_in.x, bug->velocity_in.y};
		bug->max_force_out= 100.0;
		//debug_print("DirtBug target %f, %f", bug->waypoints[0].x, bug->waypoints[1].y);
		if (bug->next_waypoint_ix >= bug->waypoint_count) {
			choose_safe_route(bug);
		} else {
			ensure(bug->next_waypoint_ix < DIRTBUG_MAX_WAYPOINT_COUNT);
			V2d dif= sub_v2d(bug->waypoints[bug->next_waypoint_ix], v3d_to_v2d(bug->pos));
			//debug_print("dif %f, %f", dif.x, dif.y);
			if (length_sqr_v2d(dif) < 1.0f*1.0f) {
				++bug->next_waypoint_ix;
			} else {
				dif= normalized_v2d(dif);
				bug->velocity_out= scaled_v2d(2.0, dif);
			}
		}
	}
}
