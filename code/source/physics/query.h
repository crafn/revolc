#ifndef REVOLC_PHYSICS_QUERY_H
#define REVOLC_PHYSICS_QUERY_H

#include "build.h"

struct RigidBody;

// @todo Wrap
typedef void (*PhysSegmentCb)(RigidBody *body, V2d point, V2d normal, F64 fraction, void *data);

REVOLC_API void phys_segment_query(	V2d a, V2d b, F64 rad, 
									PhysSegmentCb cb, void *data);	

#endif // REVOLC_PHYSICS_QUERY_H
