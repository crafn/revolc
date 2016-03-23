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

REVOLC_API WARN_UNUSED
PhysMat *blobify_physmat(struct WArchive *ar, Cson c, bool *err);
REVOLC_API void deblobify_physmat(WCson *c, struct RArchive *ar);

#endif // REVOLC_PHYSICS_PHYSMAT_HPP
