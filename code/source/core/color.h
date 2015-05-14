#ifndef REVOLC_CORE_COLOR_H
#define REVOLC_CORE_COLOR_H

#include "build.h"

typedef struct Color {
	F32 r, g, b, a;
} Color;

static
Color mul_color(Color c1, Color c2)
{ return (Color) {c1.r*c2.r, c1.g*c2.g, c1.b*c2.b, c1.a*c2.a}; }

#endif // REVOLC_CORE_COLOR_H
