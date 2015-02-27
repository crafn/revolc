#ifndef REVOLC_GAME_AITEST_H
#define REVOLC_GAME_AITEST_H

#include "build.h"
#include "core/vector.h"

typedef struct AiTest {
	V3d input_pos;
	V2d force;
	bool allocated;
} AiTest;

struct World;

REVOLC_API U32 resurrect_aitest(const AiTest *dead);
REVOLC_API void free_aitest(U32 handle);
REVOLC_API void * storage_aitest();
REVOLC_API void upd_aitest(	struct World *w,
							AiTest *t,
							U32 count);

#endif // REVOLC_GAME_AITEST_H