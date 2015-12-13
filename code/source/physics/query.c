#include "chipmunk_util.h"
#include "global/env.h"
#include "physworld.h"
#include "query.h"

struct SegmentCbWrapData
{
	PhysSegmentCb cb; 
	void *data;
};

internal
void phys_segment_cb_wrap(	cpShape *shape,
							cpVect point, cpVect normal,
							cpFloat fraction, void *void_wrapped_data)
{
	cpBody *cp_body = cpShapeGetBody(shape);
	RigidBodyCpData *body_cp_data = cpBodyGetUserData(cp_body);
	RigidBody *body = body_cp_data->body;
	ensure(body);
	ensure(body->cp_body == cp_body);

	struct SegmentCbWrapData *wrapped_data = void_wrapped_data;
	wrapped_data->cb(	body, from_cpv(point), from_cpv(normal),
						fraction, wrapped_data->data);
}

void phys_segment_query(V2d a, V2d b, F64 rad, 
						PhysSegmentCb cb, void *data)
{
	struct SegmentCbWrapData wrapped_data = {
		.cb = cb,
		.data = data,
	};
	cpSpaceSegmentQuery(g_env.physworld->cp_space,
						to_cpv(a), to_cpv(b), rad,
						CP_SHAPE_FILTER_ALL,
						phys_segment_cb_wrap,
						&wrapped_data);	
}
