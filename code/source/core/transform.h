#ifndef REVOLC_CORE_TRANSFORM_H
#define REVOLC_CORE_TRANSFORM_H

#include "build.h"
#include "quaternion.h"
#include "vector.h"

typedef struct T3f {
	V3f scale;
	Qf rot;
	V3f pos;
} T3f;

/// `subj` transformed by `op` in order: scale -> rotate -> translate
T3f mul_t3f(T3f op, T3f subj)
{
	T3f t= subj;
	t.scale= mul_v3f(op.scale, t.scale);
	t.rot= mul_qf(op.rot, t.rot);
	t.pos= add_v3f(rot_v3f(op.rot, mul_v3f(op.scale, t.pos)), op.pos);
	return t;
}


#endif // REVOLC_CORE_TRANSFORM_H
