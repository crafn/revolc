#include "aitest.h"
#include "world.h"

internal
AiTest temptest_aitest_storage[MAX_NODE_COUNT];
internal
U32 next_aitest= 0;

U32 alloc_aitest()
{
	while (temptest_aitest_storage[next_aitest].allocated)
		next_aitest= (next_aitest + 1) % MAX_NODE_COUNT;
	temptest_aitest_storage[next_aitest].allocated= true;
	return next_aitest;
}

void free_aitest(U32 handle)
{
	temptest_aitest_storage[handle].allocated= false;
}

void * storage_aitest()
{ return temptest_aitest_storage; }

void upd_aitest(	World *w,
					AiTest *t,
					U32 count)
{
	V2d target= {0.0, 30.0};
	for (U32 i= 0; i < count; ++i, ++t) {
		V2d dif= sub_v2d(target, t->input_pos);
		F64 r2= length_sqr_v2d(dif);
		t->force= scaled_v2d(dif, 1000.0/(r2 + 10.0));
	}
}

