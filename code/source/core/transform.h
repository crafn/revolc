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

typedef struct T3d {
	V3d scale;
	Qd rot;
	V3d pos;
} T3d;

/// `subj` transformed by `op` in order: scale -> rotate -> translate
static
T3f mul_t3f(T3f op, T3f subj)
{
	T3f t= subj;
	t.scale= mul_v3f(op.scale, t.scale);
	t.rot= mul_qf(op.rot, t.rot);
	t.pos= add_v3f(rot_v3f(op.rot, mul_v3f(op.scale, t.pos)), op.pos);
	return t;
}

static
T3f t3d_to_t3f(T3d tf)
{ return (T3f) {v3d_to_v3f(tf.scale), qd_to_qf(tf.rot), v3d_to_v3f(tf.pos)}; }

#endif // REVOLC_CORE_TRANSFORM_H
