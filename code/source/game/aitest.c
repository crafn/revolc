#include "aitest.h"
#include "world.h"

internal
AiTest temptest_aitest_storage[MAX_NODE_COUNT];
internal
U32 next_aitest= 0;

U32 resurrect_aitest(const AiTest *dead)
{
	while (temptest_aitest_storage[next_aitest].allocated)
		next_aitest= (next_aitest + 1) % MAX_NODE_COUNT;
	temptest_aitest_storage[next_aitest].allocated= true;

	temptest_aitest_storage[next_aitest]= *dead;
	return next_aitest;
}

void free_aitest(U32 handle)
{
	temptest_aitest_storage[handle].allocated= false;
}

void * storage_aitest()
{ return temptest_aitest_storage; }

void upd_aitest(	AiTest *t,
					U32 count)
{
	V2d target= {0.0, 30.0};
	for (U32 i= 0; i < count; ++i, ++t) {
		V2d p= {t->input_pos.x, t->input_pos.y};
		V2d dif= sub_v2d(target, p);
		F64 r2= length_sqr_v2d(dif);
		t->force= scaled_v2d(dif, 1000.0/(r2 + 10.0));
	}
}

void rotate_modelentity(ModelEntity *e, U32 count)
{
	for (U32 i= 0; i < count; ++i, e++) {
		e->scale.y= 2.0;
		//e->rot.cs= cos(acos(e->rot.cs) + 0.01);
		//e->rot.sn= sin(asin(e->rot.sn) + 0.01);
	}
}

void poly_to_modelentity(	ModelEntity *e, U32 e_count,
							RigidBody *b, U32 b_count)
{
	ensure(e_count == b_count);
	for (U32 i= 0; i < e_count; ++i, ++e, ++b) {
		e->pos= b->pos;
	}
}
