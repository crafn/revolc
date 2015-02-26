#ifndef REVOLC_CORE_MATRIX_H
#define REVOLC_CORE_MATRIX_H

#include "build.h"

typedef struct M44f {
	// Column-major
	F32 e[16];
} M44f;

REVOLC_API M44f mul_m44f(M44f a, M44f b);
REVOLC_API M44f inverted_m44f(M44f m);

#endif // REVOLC_CORE_MATRIX_H
