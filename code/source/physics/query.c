#include "chipmunk_util.h"
#include "global/env.h"
#include "physworld.h"
#include "query.h"

void phys_segment_query(V2d a, V2d b, F64 rad, 
						PhysSegmentCb cb, void *data)
{
	cpSpaceSegmentQuery(g_env.physworld->cp_space,
						to_cpv(a), to_cpv(b), rad,
						CP_SHAPE_FILTER_ALL,
						cb,
						data);	
}
