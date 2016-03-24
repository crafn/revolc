#include "physmat.h"
#include "resources/resblob.h"

PhysMat *blobify_physmat(struct WArchive *ar, Cson c, bool *err)
{
	Cson c_density = cson_key(c, "density");
	Cson c_friction = cson_key(c, "friction");
	Cson c_restitution = cson_key(c, "restitution");

	if (cson_is_null(c_density))
		RES_ATTRIB_MISSING("density");
	if (cson_is_null(c_friction))
		RES_ATTRIB_MISSING("friction");
	if (cson_is_null(c_restitution))
		RES_ATTRIB_MISSING("restitution");

	PhysMat mat = {
		.density = blobify_floating(c_density, err),
		.friction = blobify_floating(c_friction, err),
		.restitution = blobify_floating(c_restitution, err),
	};

	if (err && *err)
		goto error;

	PhysMat *ptr = warchive_ptr(ar);
	pack_buf(ar, &mat, sizeof(mat));
	return ptr;

error:
	SET_ERROR_FLAG(err);
	return NULL;
}

void deblobify_physmat(WCson *c, struct RArchive *ar)
{
	PhysMat *mat = rarchive_ptr(ar, sizeof(*mat));
	unpack_advance(ar, sizeof(*mat));

	wcson_begin_compound(c, "PhysMat");

	wcson_designated(c, "name");
	deblobify_string(c, mat->res.name);

	wcson_designated(c, "density");
	deblobify_floating(c, mat->density);

	wcson_designated(c, "friction");
	deblobify_floating(c, mat->friction);

	wcson_designated(c, "restitution");
	deblobify_floating(c, mat->restitution);

	wcson_end_compound(c);
}

