#ifndef REVOLC_CORE_QUATERNION_H
#define REVOLC_CORE_QUATERNION_H

#include "build.h"

#include <math.h>

typedef struct Qd {
	/// @todo Actual quaternion
	F64 cs, sn;
} Qd;

static
F64 rot_z_qd(Qd q)
{ return acos(q.cs); }

#endif // REVOLC_CORE_QUATERNION_H
