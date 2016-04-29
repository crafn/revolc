#ifndef REVOLC_VISUAL_DDRAW_H
#define REVOLC_VISUAL_DDRAW_H

#include "build.h"

REVOLC_API void ddraw_poly(Color c, V3d *poly, U32 count, S32 layer);
REVOLC_API void ddraw_circle(Color c, V3d p, F64 rad, S32 layer);

// Sizes in pixels
REVOLC_API void ddraw_line(Color c, V3d a, V3d b, F64 w, S32 layer);
REVOLC_API void ddraw_dot(Color c, V3d p, F64 w, S32 layer);

#endif // REVOLC_VISUAL_DDRAW_H
