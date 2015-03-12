#include "build.h"
#include "core/debug_print.h"
#include "core/vector.h"

// Character
typedef struct TestNode {
	V2d in_dir;
	V3d pos;
} TestNode;

MOD_API void upd_testnode(TestNode *t, TestNode *e)
{
	for (; t != e; ++t) {
		t->pos= (V3d) {-3.0, 10.0, 0.0};
	}
}
