#ifndef REVOLC_CORE_QUATERNION_H
#define REVOLC_CORE_QUATERNION_H

#include "build.h"
#include "vector.h"

#include <math.h>

typedef struct Qd {
	/// @todo Actual quaternion
	F64 cs, sn;
} Qd;

typedef struct Qf {
	F32 x, y, z, w;
} Qf;

static
Qf mul_qf(Qf a, Qf b)
{ fail("@todo"); return (Qf) { }; }

static
V3f rot_v3f(Qf q, V3f v)
{ fail("@todo"); return (V3f) { }; }

static
F64 rot_z_qd(Qd q)
{ return acos(q.cs); }

static
V2d rot_v2d_by_qd(Qd r, V2d p)
{ return (V2d) {r.cs*p.x - r.sn*p.y, r.sn*p.x + r.cs*p.y}; }

static
bool equals_qd(Qd a, Qd b)
{ return a.cs == b.cs && a.sn == b.sn; }

#endif // REVOLC_CORE_QUATERNION_H
