#include "physmat.h"
#include "resources/resblob.h"

int json_physmat_to_blob(struct BlobBuf *buf, JsonTok j)
{
	JsonTok j_density = json_value_by_key(j, "density");
	JsonTok j_friction = json_value_by_key(j, "friction");
	JsonTok j_restitution = json_value_by_key(j, "restitution");

	if (json_is_null(j_density))
		RES_ATTRIB_MISSING("density");
	if (json_is_null(j_friction))
		RES_ATTRIB_MISSING("friction");
	if (json_is_null(j_restitution))
		RES_ATTRIB_MISSING("restitution");

	PhysMat mat = {
		.density = json_real(j_density),
		.friction = json_real(j_friction),
		.restitution = json_real(j_restitution),
	};
	blob_write(buf, (U8*)&mat + sizeof(Resource), sizeof(mat) - sizeof(Resource));

	return 0;
error:
	return 1;
}
