#ifndef REVOLC_CORE_TRANSFORM_H
#define REVOLC_CORE_TRANSFORM_H

/// @todo Generate

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

/// `subj` transformed by `op` in order: scale -> rotate -> translate
static
T3d mul_t3d(T3d op, T3d subj)
{
	T3d t= subj;
	t.scale= mul_v3d(op.scale, t.scale);
	t.rot= mul_qd(op.rot, t.rot);
	t.pos= add_v3d(rot_v3d(op.rot, mul_v3d(op.scale, t.pos)), op.pos);
	return t;
}

static
V3f transform_v3f(T3f op, V3f subj)
{ return mul_t3f(op, (T3f) {{1, 1, 1}, identity_qf(), subj}).pos; }

static
V3d transform_v3d(T3d op, V3d subj)
{ return mul_t3d(op, (T3d) {{1, 1, 1}, identity_qd(), subj}).pos; }

static
T3f inv_t3f(T3f tf)
{
	T3f t= tf;
	t.scale= (V3f) {1/t.scale.x, 1/t.scale.y, 1/t.scale.z};
	t.rot= neg_qf(t.rot);
	t.pos= neg_v3f(rot_v3f(t.rot, mul_v3f(t.pos, t.scale)));
	return t;
}

static
T3d inv_t3d(T3d tf)
{
	T3d t= tf;
	t.scale= (V3d) {1/t.scale.x, 1/t.scale.y, 1/t.scale.z};
	t.rot= neg_qd(t.rot);
	t.pos= neg_v3d(rot_v3d(t.rot, mul_v3d(t.pos, t.scale)));
	return t;
}

static
T3f t3d_to_t3f(T3d tf)
{ return (T3f) {v3d_to_v3f(tf.scale), qd_to_qf(tf.rot), v3d_to_v3f(tf.pos)}; }

static
T3d t3f_to_t3d(T3f tf)
{ return (T3d) {v3f_to_v3d(tf.scale), qf_to_qd(tf.rot), v3f_to_v3d(tf.pos)}; }

static
T3f identity_t3f()
{ return (T3f) {{1, 1, 1}, identity_qf(), {0, 0, 0}}; }

static
T3d identity_t3d()
{ return (T3d) {{1, 1, 1}, identity_qd(), {0, 0, 0}}; }

static
T3f lerp_t3f(T3f a, T3f b, F32 t)
{
	return (T3f) {
		lerp_v3f(a.scale, b.scale, t),
		lerp_qf(a.rot, b.rot, t),
		lerp_v3f(a.pos, b.pos, t)
	};
}

#endif // REVOLC_CORE_TRANSFORM_H
