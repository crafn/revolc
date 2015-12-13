#ifndef REVOLC_CHIPMUNK_UTIL_H
#define REVOLC_CHIPMUNK_UTIL_H

#include "build.h"
#include "core/math.h"

#include <chipmunk/chipmunk.h>

static
V2d from_cpv(cpVect v)
{ return (V2d) {v.x, v.y}; }

static
cpVect to_cpv(V2d v)
{ return (cpVect) {v.x, v.y}; }

#endif // REVOLC_CHIPMUNK_UTIL_H
