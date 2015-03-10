#ifndef REVOLC_CORE_QUATERNION_H
#define REVOLC_CORE_QUATERNION_H

// This file is automatically generated (codegen)

#include "math_constants.h"
#include "vector.h"

#include <stdlib.h> // abs

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
V3f rot_v3f(Qf q, V3f v)
{
	V3f a, b, c= {q.x, q.y, q.z};
	a= cross_v3f(c, v);
	b= cross_v3f(c, a);
	a= scaled_v3f(2.0 * q.w, a);
	b= scaled_v3f(2.0, b);
	return add_v3f(add_v3f(v, a), b);
}

static
F32 rotation_z_qf(Qf q)
{
	/// @note Copy-pasted from old engine
	/// @note	If fixing is needed, make sure that these still work:
	///			 - physics object mirroring (e.g. stoneFlail in hand)
	///			 - full rotation in 2d plane (e.g. grassClump attached to object)
	bool flip= rot_v3f(q, (V3f) {0, 0, 1}).z < 0;
	// Euler angles
	F64 heading= atan2(2.0*q.y*q.w - 2.0*q.x*q.z, 1.0 - 2.0*q.y*q.y - 2.0*q.z*q.z);
	F64 attitude= asin(2.0*q.x*q.y + 2.0*q.z*q.w);
	// Using heading to detect rotations greater than +-90 degrees
	// Flipping was adjusted by trial & error
	if (abs(heading) < PI*0.5 || flip)
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
	F32 a = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;	if (a == 1)		return q;	else if (a == 0)		return identity_qf();	a= sqrt(a);	return (Qf) {q.x/a, q.y/a, q.z/a, q.w/a};
}

static
Qf qf_by_axis(V3f axis, F32 angle)
{
	axis= normalized_v3f(axis);
	F32 half= 0.5*angle;
	F32 s= sin(half);
	return (Qf) {axis.x*s, axis.y*s, axis.z*s, cos(half)};
}

static
Qf qf_by_xy_rot_matrix(F32 cs, F32 sn)
{
	/// @todo There must be a faster way
	F64 rot= atan2(sn, cs);
	return (Qf) {0, 0, sin(rot/2.0), cos(rot/2.0) };
}

static
Qf lerp_qf(Qf a, Qf b, F32 t)
{ return (Qf) {a.x*(1 - t) + b.x*t, a.y*(1 - t) + b.y*t, a.z*(1 - t) + b.z*t, a.w*(1 - t) + b.w*t, }; }

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
V3d rot_v3d(Qd q, V3d v)
{
	V3d a, b, c= {q.x, q.y, q.z};
	a= cross_v3d(c, v);
	b= cross_v3d(c, a);
	a= scaled_v3d(2.0 * q.w, a);
	b= scaled_v3d(2.0, b);
	return add_v3d(add_v3d(v, a), b);
}

static
F64 rotation_z_qd(Qd q)
{
	/// @note Copy-pasted from old engine
	/// @note	If fixing is needed, make sure that these still work:
	///			 - physics object mirroring (e.g. stoneFlail in hand)
	///			 - full rotation in 2d plane (e.g. grassClump attached to object)
	bool flip= rot_v3d(q, (V3d) {0, 0, 1}).z < 0;
	// Euler angles
	F64 heading= atan2(2.0*q.y*q.w - 2.0*q.x*q.z, 1.0 - 2.0*q.y*q.y - 2.0*q.z*q.z);
	F64 attitude= asin(2.0*q.x*q.y + 2.0*q.z*q.w);
	// Using heading to detect rotations greater than +-90 degrees
	// Flipping was adjusted by trial & error
	if (abs(heading) < PI*0.5 || flip)
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
	F64 a = q.x*q.x + q.y*q.y + q.z*q.z + q.w*q.w;	if (a == 1)		return q;	else if (a == 0)		return identity_qd();	a= sqrt(a);	return (Qd) {q.x/a, q.y/a, q.z/a, q.w/a};
}

static
Qd qd_by_axis(V3d axis, F64 angle)
{
	axis= normalized_v3d(axis);
	F64 half= 0.5*angle;
	F64 s= sin(half);
	return (Qd) {axis.x*s, axis.y*s, axis.z*s, cos(half)};
}

static
Qd qd_by_xy_rot_matrix(F64 cs, F64 sn)
{
	/// @todo There must be a faster way
	F64 rot= atan2(sn, cs);
	return (Qd) {0, 0, sin(rot/2.0), cos(rot/2.0) };
}

static
Qd lerp_qd(Qd a, Qd b, F64 t)
{ return (Qd) {a.x*(1 - t) + b.x*t, a.y*(1 - t) + b.y*t, a.z*(1 - t) + b.z*t, a.w*(1 - t) + b.w*t, }; }

static
Qf qd_to_qf(Qd q)
{ return (Qf) {q.x, q.y, q.z, q.w}; }
static
Qd qf_to_qd(Qf q)
{ return (Qd) {q.x, q.y, q.z, q.w}; }
#endif
