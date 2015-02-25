#include "rigidbodydef.h"

int json_rigidbodydef_to_blob(struct BlobBuf *buf, JsonTok j)
{
	RigidBodyDef def= {};

	JsonTok j_shapes= json_value_by_key(j, "shapes");

	if (json_is_null(j_shapes)) {
		critical_print("Attrib 'shapes' missing for RigidBodyDef");
		return 1;
	}

	for (U32 i= 0; i < json_member_count(j_shapes); ++i) {
		JsonTok j_shp= json_member(j_shapes, i);

		if (!json_is_object(j_shp)) {
			critical_print("Attrib 'shapes' should contain objects");
			return 1;
		}

		if (json_is_same(json_value_by_key(j_shp, "type"), "Circle")) {
			JsonTok j_pos= json_value_by_key(j_shp, "pos");
			JsonTok j_rad= json_value_by_key(j_shp, "rad");
			
			if (json_is_null(j_pos) || json_is_null(j_rad)) {
				critical_print("Circle shape must contain 'pos' and 'rad'");
				return 1;
			}

			def.circles[def.circle_count].pos= json_v2(j_pos);
			def.circles[def.circle_count].rad= json_real(j_rad);
			++def.circle_count;
		} else if (json_is_same(json_value_by_key(j_shp, "type"), "Circle")) {
			fail("@todo Polys");
		} else {
			critical_print("Unknown shape type");
			return 1;
		}
	}

	blob_write(buf, (U8*)&def + sizeof(Resource), sizeof(def) - sizeof(Resource));
	return 0;
}
