#ifndef REVOLC_CORE_VECTOR_H
#define REVOLC_CORE_VECTOR_H

// This is a generated file (codegen)

#include <math.h>

typedef struct V2d {
	F64 x;
	F64 y;
} V2d;

typedef struct V2f {
	F32 x;
	F32 y;
} V2f;

typedef struct V2i {
	S32 x;
	S32 y;
} V2i;

typedef struct V3d {
	F64 x;
	F64 y;
	F64 z;
} V3d;

typedef struct V3f {
	F32 x;
	F32 y;
	F32 z;
} V3f;

static
bool equals_v2d(V2d a, V2d b)
{ return a.x == b.x && a.y == b.y && 1; }
static
V2d add_v2d(V2d a, V2d b)
{ return (V2d) {a.x + b.x, a.y + b.y, }; }
static
V2d sub_v2d(V2d a, V2d b)
{ return (V2d) {a.x - b.x, a.y - b.y, }; }
static
V2d mul_v2d(V2d a, V2d b)
{ return (V2d) {a.x * b.x, a.y * b.y, }; }
static
V2d scaled_v2d(F64 s, V2d v)
{ return (V2d) {s*v.x, s*v.y, }; }
static
F64 length_sqr_v2d(V2d v)
{ return v.x*v.x + v.y*v.y + 0; }
static
F64 length_v2d(V2d v)
{ return sqrt(length_sqr_v2d(v)); }
static
F64 dist_sqr_v2d(V2d a, V2d b)
{ return (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y) + 0; }
static
F64 dist_v2d(V2d a, V2d b)
{ return sqrt(dist_sqr_v2d(a, b)); }
static
V2d normalized_v2d(V2d v)
{ return scaled_v2d(1.0/length_v2d(v), v); }
static
V2d rot_v2d(F64 f, V2d v)
{ return (V2d) {v.x*cos(f) - v.y*sin(f), v.x*sin(f) + v.y*cos(f)}; }
static
V2i round_v2d_to_v2i(V2d v)
{ return (V2i) {floor(v.x + 0.5), floor(v.y + 0.5), }; }
static
bool equals_v2f(V2f a, V2f b)
{ return a.x == b.x && a.y == b.y && 1; }
static
V2f add_v2f(V2f a, V2f b)
{ return (V2f) {a.x + b.x, a.y + b.y, }; }
static
V2f sub_v2f(V2f a, V2f b)
{ return (V2f) {a.x - b.x, a.y - b.y, }; }
static
V2f mul_v2f(V2f a, V2f b)
{ return (V2f) {a.x * b.x, a.y * b.y, }; }
static
V2f scaled_v2f(F32 s, V2f v)
{ return (V2f) {s*v.x, s*v.y, }; }
static
F32 length_sqr_v2f(V2f v)
{ return v.x*v.x + v.y*v.y + 0; }
static
F64 length_v2f(V2f v)
{ return sqrt(length_sqr_v2f(v)); }
static
F32 dist_sqr_v2f(V2f a, V2f b)
{ return (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y) + 0; }
static
F64 dist_v2f(V2f a, V2f b)
{ return sqrt(dist_sqr_v2f(a, b)); }
static
V2f normalized_v2f(V2f v)
{ return scaled_v2f(1.0/length_v2f(v), v); }
static
V2f rot_v2f(F64 f, V2f v)
{ return (V2f) {v.x*cos(f) - v.y*sin(f), v.x*sin(f) + v.y*cos(f)}; }
static
V2i round_v2f_to_v2i(V2f v)
{ return (V2i) {floor(v.x + 0.5), floor(v.y + 0.5), }; }
static
bool equals_v2i(V2i a, V2i b)
{ return a.x == b.x && a.y == b.y && 1; }
static
V2i add_v2i(V2i a, V2i b)
{ return (V2i) {a.x + b.x, a.y + b.y, }; }
static
V2i sub_v2i(V2i a, V2i b)
{ return (V2i) {a.x - b.x, a.y - b.y, }; }
static
V2i mul_v2i(V2i a, V2i b)
{ return (V2i) {a.x * b.x, a.y * b.y, }; }
static
V2i scaled_v2i(S32 s, V2i v)
{ return (V2i) {s*v.x, s*v.y, }; }
static
S32 length_sqr_v2i(V2i v)
{ return v.x*v.x + v.y*v.y + 0; }
static
F64 length_v2i(V2i v)
{ return sqrt(length_sqr_v2i(v)); }
static
S32 dist_sqr_v2i(V2i a, V2i b)
{ return (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y) + 0; }
static
F64 dist_v2i(V2i a, V2i b)
{ return sqrt(dist_sqr_v2i(a, b)); }
static
V2i normalized_v2i(V2i v)
{ return scaled_v2i(1.0/length_v2i(v), v); }
static
V2i rot_v2i(F64 f, V2i v)
{ return (V2i) {v.x*cos(f) - v.y*sin(f), v.x*sin(f) + v.y*cos(f)}; }
static
bool equals_v3d(V3d a, V3d b)
{ return a.x == b.x && a.y == b.y && a.z == b.z && 1; }
static
V3d add_v3d(V3d a, V3d b)
{ return (V3d) {a.x + b.x, a.y + b.y, a.z + b.z, }; }
static
V3d sub_v3d(V3d a, V3d b)
{ return (V3d) {a.x - b.x, a.y - b.y, a.z - b.z, }; }
static
V3d mul_v3d(V3d a, V3d b)
{ return (V3d) {a.x * b.x, a.y * b.y, a.z * b.z, }; }
static
V3d scaled_v3d(F64 s, V3d v)
{ return (V3d) {s*v.x, s*v.y, s*v.z, }; }
static
F64 length_sqr_v3d(V3d v)
{ return v.x*v.x + v.y*v.y + v.z*v.z + 0; }
static
F64 length_v3d(V3d v)
{ return sqrt(length_sqr_v3d(v)); }
static
F64 dist_sqr_v3d(V3d a, V3d b)
{ return (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y) + (a.z - b.z)*(a.z - b.z) + 0; }
static
F64 dist_v3d(V3d a, V3d b)
{ return sqrt(dist_sqr_v3d(a, b)); }
static
V3d normalized_v3d(V3d v)
{ return scaled_v3d(1.0/length_v3d(v), v); }
static
bool equals_v3f(V3f a, V3f b)
{ return a.x == b.x && a.y == b.y && a.z == b.z && 1; }
static
V3f add_v3f(V3f a, V3f b)
{ return (V3f) {a.x + b.x, a.y + b.y, a.z + b.z, }; }
static
V3f sub_v3f(V3f a, V3f b)
{ return (V3f) {a.x - b.x, a.y - b.y, a.z - b.z, }; }
static
V3f mul_v3f(V3f a, V3f b)
{ return (V3f) {a.x * b.x, a.y * b.y, a.z * b.z, }; }
static
V3f scaled_v3f(F32 s, V3f v)
{ return (V3f) {s*v.x, s*v.y, s*v.z, }; }
static
F32 length_sqr_v3f(V3f v)
{ return v.x*v.x + v.y*v.y + v.z*v.z + 0; }
static
F64 length_v3f(V3f v)
{ return sqrt(length_sqr_v3f(v)); }
static
F32 dist_sqr_v3f(V3f a, V3f b)
{ return (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y) + (a.z - b.z)*(a.z - b.z) + 0; }
static
F64 dist_v3f(V3f a, V3f b)
{ return sqrt(dist_sqr_v3f(a, b)); }
static
V3f normalized_v3f(V3f v)
{ return scaled_v3f(1.0/length_v3f(v), v); }
static
V2d v3d_to_v2d(V3d v)
{ return (V2d) {v.x, v.y}; }

#endif
