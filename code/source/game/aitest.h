#ifndef REVOLC_GAME_AITEST_H
#define REVOLC_GAME_AITEST_H

// Testing node system here

#include "build.h"
#include "core/math.h"
#include "physics/physworld.h"
#include "visual/modelentity.h"

typedef struct AiTest {
	V3d input_pos;
	V2d force;
	bool allocated;
} AiTest;

REVOLC_API U32 resurrect_aitest(const AiTest *dead);
REVOLC_API void free_aitest(U32 handle);
REVOLC_API void * storage_aitest();
REVOLC_API void upd_aitest(AiTest *t);

REVOLC_API
void rotate_modelentity(ModelEntity *e, U32 count);

REVOLC_API
void poly_to_modelentity(	ModelEntity *e, ModelEntity *e_end,
							RigidBody *b, RigidBody *b_end);

#endif // REVOLC_GAME_AITEST_H
