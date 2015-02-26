#ifndef REVOLC_CORE_VECTOR_H
#define REVOLC_CORE_VECTOR_H

#include "build.h"

#include <math.h>

/// @todo Generate this file

typedef struct V3d {
	F64 x, y, z;
} V3d;

typedef struct V2d {
	F64 x, y;
} V2d;

typedef struct Color {
	F32 r, g, b, a;
} Color;

typedef struct V3f {
	F32 x, y, z;
} V3f;

typedef struct V2f {
	F32 x, y;
} V2f;

typedef struct V2i {
	S32 x, y;
} V2i;

static
V2d sub_v2d(V2d a, V2d b)
{ return (V2d) {a.x - b.x, a.y - b.y}; }

static
V2d scaled_v2d(V2d v, F64 s)
{ return (V2d) {v.x*s, v.y*s}; }

static
F64 length_sqr_v2d(V2d v)
{ return v.x*v.x + v.y*v.y; }

static
V2d normalized_v2d(V2d v)
{
	F64 len= sqrt(length_sqr_v2d(v));
	return (V2d) {v.x/len, v.y/len};
}

static
V3f zero_v3f()
{
	V3f result= {};
	return result;
}

static
V3f add_v3f(V3f a, V3f b)
{
	V3f result;
	result.x= a.x + b.x;
	result.y= a.y + b.y;
	result.z= a.z + b.z;
	return result;
}

static
V3f sub_v3f(V3f a, V3f b)
{
	V3f result;
	result.x= a.x - b.x;
	result.y= a.y - b.y;
	result.z= a.z - b.z;
	return result;
}

#endif // REVOLC_CORE_VECTOR_H
