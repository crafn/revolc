#ifndef REVOLC_PHYSICS_PHYS_WORLD_H
#define REVOLC_PHYSICS_PHYS_WORLD_H

#include "build.h"

typedef struct RigidBody {

} RigidBody;

typedef struct PhysWorld {
	RigidBody bodies[MAX_RIGID_BODY_COUNT];
} PhysWorld;

#endif // REVOLC_PHYSICS_PHYS_WORLD_H
