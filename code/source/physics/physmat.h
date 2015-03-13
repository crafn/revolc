#ifndef REVOLC_PHYSICS_PHYSMAT_HPP
#define REVOLC_PHYSICS_PHYSMAT_HPP

#include "build.h"
#include "core/json.h"
#include "resources/resource.h"

typedef struct PhysMat {
	Resource res;
	F64 density;
	F64 friction;
	F64 restitution;
} PACKED PhysMat;


REVOLC_API WARN_UNUSED
int json_physmat_to_blob(struct BlobBuf *buf, JsonTok j);

#endif // REVOLC_PHYSICS_PHYSMAT_HPP
