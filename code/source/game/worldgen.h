#ifndef REVOLC_GAME_WORLDGEN_H
#define REVOLC_GAME_WORLDGEN_H

#include "build.h"
#include "world.h"

// This is just a test world generation

REVOLC_API
void generate_test_world(World *w);

REVOLC_API
F64 ground_surf_y(F64 x);

REVOLC_API
void spawn_visual_prop(World *world, V3d pos, F64 rot, V3d scale, const char *name);

REVOLC_API
void spawn_phys_prop(World *world, V2d pos, const char *name, bool is_static);


#endif // REVOLC_GAME_WORLDGEN_H
