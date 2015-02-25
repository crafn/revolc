#ifndef REVOLC_PHYSICS_SHAPES_H
#define REVOLC_PHYSICS_SHAPES_H

#include "build.h"
#include "core/vector.h"

typedef struct Circle {
	V2d pos;
	F64 rad;
} Circle;

/// @todo Concave support.
typedef struct Poly {
	V2d v[MAX_POLY_VERTEX_COUNT];
	U32 v_count;
} Poly;

#endif // REVOLC_PHYSICS_SHAPES_H
