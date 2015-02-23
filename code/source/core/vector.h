#ifndef REVOLC_CORE_VECTOR_H
#define REVOLC_CORE_VECTOR_H

#include "build.h"

/// @todo Generate this file

typedef struct {
	F32 x, y, z;
} V3f;

typedef struct {
	F32 x, y;
} V2f;

typedef struct {
	S32 x, y;
} V2i;

inline
V3f zero_v3f()
{
	V3f result= {};
	return result;
}

inline
V3f add_v3f(V3f a, V3f b)
{
	V3f result;
	result.x= a.x + b.x;
	result.y= a.y + b.y;
	result.z= a.z + b.z;
	return result;
}

inline
V3f sub_v3f(V3f a, V3f b)
{
	V3f result;
	result.x= a.x - b.x;
	result.y= a.y - b.y;
	result.z= a.z - b.z;
	return result;
}

#endif // REVOLC_CORE_VECTOR_H
