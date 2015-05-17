#ifndef REVOLC_PHYSICS_QUERY_H
#define REVOLC_PHYSICS_QUERY_H

#include "build.h"

// @todo Wrap
typedef void (*PhysSegmentCb)(cpShape *shape, cpVect v1, cpVect v2, cpFloat t, void *data);

REVOLC_API void phys_segment_query(	V2d a, V2d b, F64 rad, 
									PhysSegmentCb cb, void *data);	

#endif // REVOLC_PHYSICS_QUERY_H
