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

static
V2d rot_v2d_qd(V2d p, Qd r)
{ return (V2d) {r.cs*p.x - r.sn*p.y, r.sn*p.x + r.cs*p.y}; }

#endif // REVOLC_CORE_QUATERNION_H
