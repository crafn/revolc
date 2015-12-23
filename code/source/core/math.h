#ifndef REVOLC_PLATFORM_MATH_H
#define REVOLC_PLATFORM_MATH_H

#ifndef CODEGEN
#	include <math.h>
#endif

#define PI 3.1415926535
#define TAU (2*PI)

// ???
#define EPSILON 0.00000000001


// Scalars

static
F32 smoothstep_f32(F32 a, F32 b, F32 x)
{
	F32 t = (x - a)/(b - a);
	return t*t*(3 - 2*t);
}

static
F32 smootherstep_f32(F32 a, F32 b, F32 x)
{
	F32 t = (x - a)/(b - a);
	return t*t*t*(t*(6*t - 15) + 10);
}

static
F32 lerp_f32(F32 a, F32 b, F32 t)
{ return a*(1 - t) + b*t; }

// FPS independent exponential decay towards `target`
static
F64 exp_drive(F64 value, F64 target, F64 dt)
{
	return (value - target)*pow(2, -dt) + target;
}


// Vectors

typedef struct V2d {
	F64 x;
	F64 y;
} V2d;

typedef struct V2f {
	F32 x;
	F32 y;
} V2f;

typedef struct V2i {
	S32 x;
	S32 y;
} V2i;

typedef struct V3d {
	F64 x;
	F64 y;
	F64 z;
} V3d;

typedef struct V3f {
	F32 x;
	F32 y;
	F32 z;
} V3f;

static
bool equals_v2d(V2d a, V2d b)
{ return a.x == b.x && a.y == b.y && 1; }

static
V2d add_v2d(V2d a, V2d b)
{ return (V2d) {a.x + b.x, a.y + b.y, }; }

static
V2d sub_v2d(V2d a, V2d b)
{ return (V2d) {a.x - b.x, a.y - b.y, }; }

static
V2d mul_v2d(V2d a, V2d b)
{ return (V2d) {a.x * b.x, a.y * b.y, }; }

static
V2d neg_v2d(V2d v)
{ return (V2d) {-v.x, -v.y, }; }

static
V2d scaled_v2d(F64 s, V2d v)
{ return (V2d) {s*v.x, s*v.y, }; }

static
F64 dot_v2d(V2d a, V2d b)
{ return a.x*b.x + a.y*b.y + 0; }

static
F64 length_sqr_v2d(V2d v)
{ return v.x*v.x + v.y*v.y + 0; }

static
F64 length_v2d(V2d v)
{ return sqrt(length_sqr_v2d(v)); }

static
F64 dist_sqr_v2d(V2d a, V2d b)
{ return (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y) + 0; }

static
F64 dist_v2d(V2d a, V2d b)
{ return sqrt(dist_sqr_v2d(a, b)); }

static
V2d normalized_v2d(V2d v)
{ return scaled_v2d(1.0/length_v2d(v), v); }

static
V2d lerp_v2d(V2d a, V2d b, F64 t)
{ return (V2d) {a.x*(1 - t) + b.x*t, a.y*(1 - t) + b.y*t, }; }

static
V2d rot_v2d(F64 f, V2d v)
{ return (V2d) {v.x*cos(f) - v.y*sin(f), v.x*sin(f) + v.y*cos(f)}; }

static
V2i round_v2d_to_v2i(V2d v)
{ return (V2i) {floor(v.x + 0.5), floor(v.y + 0.5), }; }

static
V2f v2d_to_v2f(V2d v)
{ return (V2f) {v.x, v.y, }; }

static
bool equals_v2f(V2f a, V2f b)
{ return a.x == b.x && a.y == b.y && 1; }

static
V2f add_v2f(V2f a, V2f b)
{ return (V2f) {a.x + b.x, a.y + b.y, }; }

static
V2f sub_v2f(V2f a, V2f b)
{ return (V2f) {a.x - b.x, a.y - b.y, }; }

static
V2f mul_v2f(V2f a, V2f b)
{ return (V2f) {a.x * b.x, a.y * b.y, }; }

static
V2f neg_v2f(V2f v)
{ return (V2f) {-v.x, -v.y, }; }

static
V2f scaled_v2f(F32 s, V2f v)
{ return (V2f) {s*v.x, s*v.y, }; }

static
F32 dot_v2f(V2f a, V2f b)
{ return a.x*b.x + a.y*b.y + 0; }

static
F32 length_sqr_v2f(V2f v)
{ return v.x*v.x + v.y*v.y + 0; }

static
F64 length_v2f(V2f v)
{ return sqrt(length_sqr_v2f(v)); }

static
F32 dist_sqr_v2f(V2f a, V2f b)
{ return (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y) + 0; }

static
F64 dist_v2f(V2f a, V2f b)
{ return sqrt(dist_sqr_v2f(a, b)); }

static
V2f normalized_v2f(V2f v)
{ return scaled_v2f(1.0/length_v2f(v), v); }

static
V2f lerp_v2f(V2f a, V2f b, F32 t)
{ return (V2f) {a.x*(1 - t) + b.x*t, a.y*(1 - t) + b.y*t, }; }

static
V2f rot_v2f(F64 f, V2f v)
{ return (V2f) {v.x*cos(f) - v.y*sin(f), v.x*sin(f) + v.y*cos(f)}; }

static
V2i v2f_to_v2i(V2f v)
{ return (V2i) {(int)v.x, (int)v.y, }; }

static
V2i round_v2f_to_v2i(V2f v)
{ return (V2i) {floor(v.x + 0.5), floor(v.y + 0.5), }; }

static
V2d v2f_to_v2d(V2f v)
{ return (V2d) {v.x, v.y, }; }

static
bool equals_v2i(V2i a, V2i b)
{ return a.x == b.x && a.y == b.y && 1; }

static
V2i add_v2i(V2i a, V2i b)
{ return (V2i) {a.x + b.x, a.y + b.y, }; }

static
V2i sub_v2i(V2i a, V2i b)
{ return (V2i) {a.x - b.x, a.y - b.y, }; }

static
V2i mul_v2i(V2i a, V2i b)
{ return (V2i) {a.x * b.x, a.y * b.y, }; }

static
V2i neg_v2i(V2i v)
{ return (V2i) {-v.x, -v.y, }; }

static
V2i scaled_v2i(S32 s, V2i v)
{ return (V2i) {s*v.x, s*v.y, }; }

static
S32 dot_v2i(V2i a, V2i b)
{ return a.x*b.x + a.y*b.y + 0; }

static
S32 length_sqr_v2i(V2i v)
{ return v.x*v.x + v.y*v.y + 0; }

static
F64 length_v2i(V2i v)
{ return sqrt(length_sqr_v2i(v)); }

static
S32 dist_sqr_v2i(V2i a, V2i b)
{ return (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y) + 0; }

static
F64 dist_v2i(V2i a, V2i b)
{ return sqrt(dist_sqr_v2i(a, b)); }

static
V2i normalized_v2i(V2i v)
{ return scaled_v2i(1.0/length_v2i(v), v); }

static
V2i lerp_v2i(V2i a, V2i b, S32 t)
{ return (V2i) {a.x*(1 - t) + b.x*t, a.y*(1 - t) + b.y*t, }; }

static
V2i rot_v2i(F64 f, V2i v)
{ return (V2i) {v.x*cos(f) - v.y*sin(f), v.x*sin(f) + v.y*cos(f)}; }

static
bool equals_v3d(V3d a, V3d b)
{ return a.x == b.x && a.y == b.y && a.z == b.z && 1; }

static
V3d add_v3d(V3d a, V3d b)
{ return (V3d) {a.x + b.x, a.y + b.y, a.z + b.z, }; }

static
V3d sub_v3d(V3d a, V3d b)
{ return (V3d) {a.x - b.x, a.y - b.y, a.z - b.z, }; }

static
V3d mul_v3d(V3d a, V3d b)
{ return (V3d) {a.x * b.x, a.y * b.y, a.z * b.z, }; }

static
V3d neg_v3d(V3d v)
{ return (V3d) {-v.x, -v.y, -v.z, }; }

static
V3d scaled_v3d(F64 s, V3d v)
{ return (V3d) {s*v.x, s*v.y, s*v.z, }; }

static
F64 dot_v3d(V3d a, V3d b)
{ return a.x*b.x + a.y*b.y + a.z*b.z + 0; }

static
F64 length_sqr_v3d(V3d v)
{ return v.x*v.x + v.y*v.y + v.z*v.z + 0; }

static
F64 length_v3d(V3d v)
{ return sqrt(length_sqr_v3d(v)); }

static
F64 dist_sqr_v3d(V3d a, V3d b)
{ return (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y) + (a.z - b.z)*(a.z - b.z) + 0; }

static
F64 dist_v3d(V3d a, V3d b)
{ return sqrt(dist_sqr_v3d(a, b)); }

static
V3d normalized_v3d(V3d v)
{ return scaled_v3d(1.0/length_v3d(v), v); }

static
V3d lerp_v3d(V3d a, V3d b, F64 t)
{ return (V3d) {a.x*(1 - t) + b.x*t, a.y*(1 - t) + b.y*t, a.z*(1 - t) + b.z*t, }; }

static
V3d cross_v3d(V3d a, V3d b)
{ return (V3d) {a.y*b.z - b.y*a.z, b.x*a.z - a.x*b.z, a.x*b.y - b.x*a.y}; }

static
V3f v3d_to_v3f(V3d v)
{ return (V3f) {v.x, v.y, v.z, }; }

static
bool equals_v3f(V3f a, V3f b)
{ return a.x == b.x && a.y == b.y && a.z == b.z && 1; }

static
V3f add_v3f(V3f a, V3f b)
{ return (V3f) {a.x + b.x, a.y + b.y, a.z + b.z, }; }

static
V3f sub_v3f(V3f a, V3f b)
{ return (V3f) {a.x - b.x, a.y - b.y, a.z - b.z, }; }

static
V3f mul_v3f(V3f a, V3f b)
{ return (V3f) {a.x * b.x, a.y * b.y, a.z * b.z, }; }

static
V3f neg_v3f(V3f v)
{ return (V3f) {-v.x, -v.y, -v.z, }; }

static
V3f scaled_v3f(F32 s, V3f v)
{ return (V3f) {s*v.x, s*v.y, s*v.z, }; }

static
F32 dot_v3f(V3f a, V3f b)
{ return a.x*b.x + a.y*b.y + a.z*b.z + 0; }

static
F32 length_sqr_v3f(V3f v)
{ return v.x*v.x + v.y*v.y + v.z*v.z + 0; }

static
F64 length_v3f(V3f v)
{ return sqrt(length_sqr_v3f(v)); }

static
F32 dist_sqr_v3f(V3f a, V3f b)
{ return (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y) + (a.z - b.z)*(a.z - b.z) + 0; }

static
F64 dist_v3f(V3f a, V3f b)
{ return sqrt(dist_sqr_v3f(a, b)); }

static
V3f normalized_v3f(V3f v)
{ return scaled_v3f(1.0/length_v3f(v), v); }

static
V3f lerp_v3f(V3f a, V3f b, F32 t)
{ return (V3f) {a.x*(1 - t) + b.x*t, a.y*(1 - t) + b.y*t, a.z*(1 - t) + b.z*t, }; }

static
V3f cross_v3f(V3f a, V3f b)
{ return (V3f) {a.y*b.z - b.y*a.z, b.x*a.z - a.x*b.z, a.x*b.y - b.x*a.y}; }

static
V3d v3f_to_v3d(V3f v)
{ return (V3d) {v.x, v.y, v.z, }; }

static
V2d v3d_to_v2d(V3d v)
{ return (V2d) {v.x, v.y}; }

static
V3d v2d_to_v3d(V2d v)
{ return (V3d) {v.x, v.y, 0.0}; }


// Quaternions

typedef struct Qf {
	F32 x;
	F32 y;
	F32 z;
	F32 w;
} Qf;

typedef struct Qd {
	F64 x;
	F64 y;
	F64 z;
	F64 w;
} Qd;

static
bool equals_qf(Qf a, Qf b)
{ return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w && 1; }

static
Qf mul_qf(Qf a, Qf b)
{ return (Qf) {
	a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
	a.w*b.y + a.y*b.w + a.z*b.x - a.x*b.z,
	a.w*b.z + a.z*b.w + a.x*b.y - a.y*b.x,
	a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z };
}

static
F32 dot_qf(Qf a, Qf b)
{ return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}

static
Qf neg_qf(Qf q)
{ return (Qf) {-q.x, -q.y, -q.z, q.w}; }

static
V3f rot_v3f(Qf q, V3f v)
{
	V3f a, b, c = {q.x, q.y, q.z};
	a = cross_v3f(c, v);
	b = cross_v3f(c, a);
	a = scaled_v3f(2.0 * q.w, a);
	b = scaled_v3f(2.0, b);
	return add_v3f(add_v3f(v, a), b);
}

static
F32 rotation_z_qf(Qf q)
{
	/// @note Copy-pasted from old engine
	/// @note	If fixing is needed, make sure that these still work:
	///			 - physics object mirroring (e.g. stoneFlail in hand)
	///			 - full rotation in 2d plane (e.g. grassClump attached to object)
	bool flip = rot_v3f(q, (V3f) {0, 0, 1}).z < 0;
	// Euler angles
	F64 heading = atan2(2.0*q.y*q.w - 2.0*q.x*q.z, 1.0 - 2.0*q.y*q.y - 2.0*q.z*q.z);
	F64 attitude = asin(2.0*q.x*q.y + 2.0*q.z*q.w);
	// Using heading to detect rotations greater than +-90 degrees
	// Flipping was adjusted by trial & error
	if (ABS(heading) < PI*0.5 || flip)
		return flip ? -attitude : attitude;
	else
		return PI - attitude;
}

static
Qf identity_qf()
{ return (Qf) {0, 0, 0, 1}; }

static
Qf normalized_qf(Qf q)
{
	F32 a = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
	if (a == 1)
		return q;
	else if (a == 0)
		return identity_qf();
	a = sqrt(a);
	return (Qf) {q.x/a, q.y/a, q.z/a, q.w/a};
}

static
V3f axis_qf(Qf q)
{
	F32 s = sqrt(q.x*q.x + q.y*q.y + q.z*q.z);
	if (s <= EPSILON || q.w > 1.0 || q.w < -1.0) {
		return (V3f) {0, 1, 0};
	} else {
		F32 inv_s = 1.0/s;
		return (V3f) {q.x*inv_s, q.y*inv_s, q.z*inv_s};	}
}

static
F32 angle_qf(Qf q)
{ return 2.0*acos(q.w); }

static
Qf qf_by_axis(V3f axis, F32 angle)
{
	axis = normalized_v3f(axis);
	F32 half = 0.5*angle;
	F32 s = sin(half);
	return (Qf) {axis.x*s, axis.y*s, axis.z*s, cos(half)};
}

static
Qf qf_by_from_to(V3f v1, V3f v2)
{
	v1 = normalized_v3f(v1); v2 = normalized_v3f(v2);
	F64 dot = dot_v3f(v1, v2);
	if (dot >= 1.0) {
		return identity_qf();
	} else if (dot <= -1.0) {
		V3f axis = {1.0, 0.0, 0.0};
		axis = cross_v3f(axis, v1);
		if (length_sqr_v3f(axis) == 0.0) {
			axis = (V3f) {0.0, 1.0, 0.0};
			axis = cross_v3f(axis, v1);
		}
		return normalized_qf((Qf) {axis.x, axis.y, axis.z, 0});
	}
	F64 mul = sqrt(2 + dot*2);
	V3f v = scaled_v3f(1.0/mul, cross_v3f(v1, v2));
	return (Qf) {v.x, v.y, v.z, 0.5*mul};
}

static
Qf qf_by_xy_rot_matrix(F32 cs, F32 sn)
{
	/// @todo There must be a faster way
	F64 rot = atan2(sn, cs);
	return (Qf) {0, 0, sin(rot/2.0), cos(rot/2.0) };
}

// Not normalized, use only for small angles
static
Qf lerp_qf(Qf a, Qf b, F32 t)
{
	if (a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w < 0.0f) // Pick shortest path
		b = (Qf) {-b.x, -b.y, -b.z, -b.w};
	return (Qf) {a.x*(1 - t) + b.x*t, a.y*(1 - t) + b.y*t, a.z*(1 - t) + b.z*t, a.w*(1 - t) + b.w*t, };
}

static
bool equals_qd(Qd a, Qd b)
{ return a.x == b.x && a.y == b.y && a.z == b.z && a.w == b.w && 1; }

static
Qd mul_qd(Qd a, Qd b)
{ return (Qd) {
	a.w*b.x + a.x*b.w + a.y*b.z - a.z*b.y,
	a.w*b.y + a.y*b.w + a.z*b.x - a.x*b.z,
	a.w*b.z + a.z*b.w + a.x*b.y - a.y*b.x,
	a.w*b.w - a.x*b.x - a.y*b.y - a.z*b.z };
}

static
F64 dot_qd(Qd a, Qd b)
{ return a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w;
}

static
Qd neg_qd(Qd q)
{ return (Qd) {-q.x, -q.y, -q.z, q.w}; }

static
V3d rot_v3d(Qd q, V3d v)
{
	V3d a, b, c = {q.x, q.y, q.z};
	a = cross_v3d(c, v);
	b = cross_v3d(c, a);
	a = scaled_v3d(2.0 * q.w, a);
	b = scaled_v3d(2.0, b);
	return add_v3d(add_v3d(v, a), b);
}

static
F64 rotation_z_qd(Qd q)
{
	/// @note Copy-pasted from old engine
	/// @note	If fixing is needed, make sure that these still work:
	///			 - physics object mirroring (e.g. stoneFlail in hand)
	///			 - full rotation in 2d plane (e.g. grassClump attached to object)
	bool flip = rot_v3d(q, (V3d) {0, 0, 1}).z < 0;
	// Euler angles
	F64 heading = atan2(2.0*q.y*q.w - 2.0*q.x*q.z, 1.0 - 2.0*q.y*q.y - 2.0*q.z*q.z);
	F64 attitude = asin(2.0*q.x*q.y + 2.0*q.z*q.w);
	// Using heading to detect rotations greater than +-90 degrees
	// Flipping was adjusted by trial & error
	if (ABS(heading) < PI*0.5 || flip)
		return flip ? -attitude : attitude;
	else
		return PI - attitude;
}

static
Qd identity_qd()
{ return (Qd) {0, 0, 0, 1}; }

static
Qd normalized_qd(Qd q)
{
	F64 a = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;
	if (a == 1)
		return q;
	else if (a == 0)
		return identity_qd();
	a = sqrt(a);
	return (Qd) {q.x/a, q.y/a, q.z/a, q.w/a};
}

static
V3d axis_qd(Qd q)
{
	F64 s = sqrt(q.x*q.x + q.y*q.y + q.z*q.z);
	if (s <= EPSILON || q.w > 1.0 || q.w < -1.0) {
		return (V3d) {0, 1, 0};
	} else {
		F64 inv_s = 1.0/s;
		return (V3d) {q.x*inv_s, q.y*inv_s, q.z*inv_s};	}
}

static
F64 angle_qd(Qd q)
{ return 2.0*acos(q.w); }

static
Qd qd_by_axis(V3d axis, F64 angle)
{
	axis = normalized_v3d(axis);
	F64 half = 0.5*angle;
	F64 s = sin(half);
	return (Qd) {axis.x*s, axis.y*s, axis.z*s, cos(half)};
}

static
Qd qd_by_from_to(V3d v1, V3d v2)
{
	v1 = normalized_v3d(v1); v2 = normalized_v3d(v2);
	F64 dot = dot_v3d(v1, v2);
	if (dot >= 1.0) {
		return identity_qd();
	} else if (dot <= -1.0) {
		V3d axis = {1.0, 0.0, 0.0};
		axis = cross_v3d(axis, v1);
		if (length_sqr_v3d(axis) == 0.0) {
			axis = (V3d) {0.0, 1.0, 0.0};
			axis = cross_v3d(axis, v1);
		}
		return normalized_qd((Qd) {axis.x, axis.y, axis.z, 0});
	}
	F64 mul = sqrt(2 + dot*2);
	V3d v = scaled_v3d(1.0/mul, cross_v3d(v1, v2));
	return (Qd) {v.x, v.y, v.z, 0.5*mul};
}

static
Qd qd_by_xy_rot_matrix(F64 cs, F64 sn)
{
	/// @todo There must be a faster way
	F64 rot = atan2(sn, cs);
	return (Qd) {0, 0, sin(rot/2.0), cos(rot/2.0) };
}

// Not normalized, use only for small angles
static
Qd lerp_qd(Qd a, Qd b, F64 t)
{
	if (a.x*b.x + a.y*b.y + a.z*b.z + a.w*b.w < 0.0) // Pick shortest path
		b = (Qd) {-b.x, -b.y, -b.z, -b.w};
	return (Qd) {a.x*(1 - t) + b.x*t, a.y*(1 - t) + b.y*t, a.z*(1 - t) + b.z*t, a.w*(1 - t) + b.w*t, };
}

static
Qf qd_to_qf(Qd q)
{ return (Qf) {q.x, q.y, q.z, q.w}; }
static
Qd qf_to_qd(Qf q)
{ return (Qd) {q.x, q.y, q.z, q.w}; }


// Matrices

typedef struct M44f {
	// Column-major
	F32 e[16];
} M44f;

REVOLC_API M44f mul_m44f(M44f a, M44f b);
REVOLC_API M44f inverted_m44f(M44f m);


// Transforms


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
	T3f t = subj;
	t.scale = mul_v3f(op.scale, t.scale);
	t.rot = mul_qf(op.rot, t.rot);
	t.pos = add_v3f(rot_v3f(op.rot, mul_v3f(op.scale, t.pos)), op.pos);
	return t;
}

/// `subj` transformed by `op` in order: scale -> rotate -> translate
static
T3d mul_t3d(T3d op, T3d subj)
{
	T3d t = subj;
	t.scale = mul_v3d(op.scale, t.scale);
	t.rot = mul_qd(op.rot, t.rot);
	t.pos = add_v3d(rot_v3d(op.rot, mul_v3d(op.scale, t.pos)), op.pos);
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
	T3f t = tf;
	t.scale = (V3f) {1/t.scale.x, 1/t.scale.y, 1/t.scale.z};
	t.rot = neg_qf(t.rot);
	t.pos = neg_v3f(rot_v3f(t.rot, mul_v3f(t.pos, t.scale)));
	return t;
}

static
T3d inv_t3d(T3d tf)
{
	T3d t = tf;
	t.scale = (V3d) {1/t.scale.x, 1/t.scale.y, 1/t.scale.z};
	t.rot = neg_qd(t.rot);
	t.pos = neg_v3d(rot_v3d(t.rot, mul_v3d(t.pos, t.scale)));
	return t;
}

// mul(a, delta(b, a)) == b

static
T3f delta_t3f(T3f to, T3f from)
{ return mul_t3f(inv_t3f(from), to); }

static
T3d delta_t3d(T3d to, T3d from)
{ return mul_t3d(inv_t3d(from), to); }

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

static
T3d lerp_t3d(T3d a, T3d b, F64 t)
{
	return (T3d) {
		lerp_v3d(a.scale, b.scale, t),
		lerp_qd(a.rot, b.rot, t),
		lerp_v3d(a.pos, b.pos, t)
	};
}

#endif // REVOLC_PLATFORM_MATH_H
