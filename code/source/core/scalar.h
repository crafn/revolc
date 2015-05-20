#ifndef REVOLC_CORE_SCALAR_H
#define REVOLC_CORE_SCALAR_H

#include "build.h"

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

#endif // REVOLC_CORE_SCALAR_H
