#include "build.h"
#include "core/debug_print.h"
#include "core/vector.h"

typedef struct TestNode {
	V3d pos;
} TestNode;

// In-place
MOD_API U32 resurrect_testnode(TestNode *dead)
{
	return NULL_HANDLE;
}

MOD_API void upd_testnode(TestNode *t, TestNode *e)
{
	for (; t != e; ++t) {
		t->pos= (V3d) {-3.0, 10.0, 0.0};
	}
}
