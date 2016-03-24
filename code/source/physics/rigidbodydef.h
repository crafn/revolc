#ifndef REVOLC_PHYSICS_RIGIDBODYDEF_H
#define REVOLC_PHYSICS_RIGIDBODYDEF_H

#include "build.h"
#include "global/cfg.h"
#include "resources/resource.h"
#include "shapes.h"

typedef struct RigidBodyDef {
	Resource res;

	char mat_name[RES_NAME_SIZE];
	bool disable_rot;
	bool is_static;

	Circle circles[MAX_SHAPES_PER_BODY];
	U32 circle_count;
	Poly polys[MAX_SHAPES_PER_BODY];
	U32 poly_count;
} PACKED RigidBodyDef;

REVOLC_API WARN_UNUSED
RigidBodyDef *blobify_rigidbodydef(struct WArchive *ar, Cson c, bool *err);
REVOLC_API void deblobify_rigidbodydef(WCson *c, struct RArchive *ar);

#endif // REVOLC_PHYSICS_RIGIDBODYDEF_H
