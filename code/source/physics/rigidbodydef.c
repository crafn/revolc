#include "rigidbodydef.h"

int json_rigidbodydef_to_blob(struct BlobBuf *buf, JsonTok j)
{
	JsonTok j_mat= json_value_by_key(j, "mat");
	JsonTok j_disable_rot= json_value_by_key(j, "disable_rot");
	JsonTok j_is_static= json_value_by_key(j, "is_static");
	JsonTok j_shapes= json_value_by_key(j, "shapes");

	if (json_is_null(j_mat))
		RES_ATTRIB_MISSING("mat");

	if (json_is_null(j_shapes)) {
		RES_ATTRIB_MISSING("shapes");
	} else if (json_member_count(j_shapes) > MAX_SHAPES_PER_BODY) {
		critical_print("Too many shapes in RigidBodyDef: %i > %i",
				json_member_count(j_shapes),
				MAX_SHAPES_PER_BODY);
		goto error;
	}

	RigidBodyDef def= {};
	fmt_str(def.mat_name, sizeof(def.mat_name), "%s",
			json_str(j_mat));


	if (!json_is_null(j_disable_rot))
		def.disable_rot= json_bool(j_disable_rot);

	if (!json_is_null(j_is_static))
		def.is_static= json_bool(j_is_static);

	/// @todo Take density into account

	for (U32 i= 0; i < json_member_count(j_shapes); ++i) {
		JsonTok j_shp= json_member(j_shapes, i);

		if (!json_is_object(j_shp)) {
			critical_print("Attrib 'shapes' should contain objects");
			return 1;
		}

		JsonTok j_shp_type= json_value_by_key(j_shp, "type");
		if (json_is_same(j_shp_type, "Circle")) {
			JsonTok j_pos= json_value_by_key(j_shp, "pos");
			JsonTok j_rad= json_value_by_key(j_shp, "rad");
			
			if (json_is_null(j_pos) || json_is_null(j_rad)) {
				critical_print("Circle shape must have 'pos' and 'rad'");
				return 1;
			}

			def.circles[def.circle_count].pos= json_v2(j_pos);
			def.circles[def.circle_count].rad= json_real(j_rad);
			++def.circle_count;
		} else if (json_is_same(j_shp_type, "Poly")) {
			JsonTok j_v= json_value_by_key(j_shp, "v");
			
			if (json_is_null(j_v)) {
				critical_print("Poly shape must have 'v'");
				return 1;
			}

			for (U32 v_i= 0; v_i < json_member_count(j_v); ++v_i) {
				def.polys[def.poly_count].v[v_i]= json_v2(json_member(j_v, v_i));
				++def.polys[def.poly_count].v_count;
			}
			++def.poly_count;
		} else {
			critical_print("Unknown shape type: %s",
					json_str(j_shp_type));
			return 1;
		}
	}

	blob_write(buf, (U8*)&def + sizeof(Resource), sizeof(def) - sizeof(Resource));
	return 0;

error:
	return 1;
}
