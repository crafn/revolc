#ifndef REVOLC_PHYSICS_RIGIDBODYDEF_H
#define REVOLC_PHYSICS_RIGIDBODYDEF_H

#include "build.h"
#include "global/cfg.h"
#include "resources/resource.h"
#include "shapes.h"

typedef struct RigidBodyDef {
	Resource res;

	Circle circles[MAX_SHAPES_PER_BODY];
	U32 circle_count;

	Poly polys[MAX_SHAPES_PER_BODY];
	U32 poly_count;
} PACKED RigidBodyDef;

REVOLC_API WARN_UNUSED
int json_rigidbodydef_to_blob(struct BlobBuf *buf, JsonTok j);

#endif // REVOLC_PHYSICS_RIGIDBODYDEF_H
