#ifndef REVOLC_CORE_COLOR_H
#define REVOLC_CORE_COLOR_H

#include "build.h"

typedef struct Color {
	F32 r, g, b, a;
} Color;

static
Color mul_color(Color c1, Color c2)
{ return (Color) {c1.r*c2.r, c1.g*c2.g, c1.b*c2.b, c1.a*c2.a}; }

static
Color lerp_color(Color c1, Color c2, F32 t)
{ return (Color) {	c1.r*(1 - t) + c2.r*t,
					c1.g*(1 - t) + c2.g*t,
					c1.b*(1 - t) + c2.b*t,
					c1.a*(1 - t) + c2.a*t}; }

static
Color white_color()
{ return (Color) {1, 1, 1, 1}; }

#endif // REVOLC_CORE_COLOR_H
