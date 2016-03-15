#include "rigidbodydef.h"

int json_rigidbodydef_to_blob(struct BlobBuf *buf, JsonTok j)
{
	JsonTok j_mat = json_value_by_key(j, "mat");
	JsonTok j_disable_rot = json_value_by_key(j, "disable_rot");
	JsonTok j_is_static = json_value_by_key(j, "is_static");
	JsonTok j_shapes = json_value_by_key(j, "shapes");

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

	RigidBodyDef def = {};
	fmt_str(def.mat_name, sizeof(def.mat_name), "%s",
			json_str(j_mat));


	if (!json_is_null(j_disable_rot))
		def.disable_rot = json_bool(j_disable_rot);

	if (!json_is_null(j_is_static))
		def.is_static = json_bool(j_is_static);

	/// @todo Take density into account

	for (U32 i = 0; i < json_member_count(j_shapes); ++i) {
		JsonTok j_shp = json_member(j_shapes, i);

		if (!json_is_object(j_shp)) {
			critical_print("Attrib 'shapes' should contain objects");
			return 1;
		}

		JsonTok j_shp_type = json_value_by_key(j_shp, "type");
		if (json_is_same(j_shp_type, "Circle")) {
			JsonTok j_pos = json_value_by_key(j_shp, "pos");
			JsonTok j_rad = json_value_by_key(j_shp, "rad");
			
			if (json_is_null(j_pos) || json_is_null(j_rad)) {
				critical_print("Circle shape must have 'pos' and 'rad'");
				return 1;
			}

			def.circles[def.circle_count].pos = json_v2(j_pos);
			def.circles[def.circle_count].rad = json_real(j_rad);
			++def.circle_count;
		} else if (json_is_same(j_shp_type, "Poly")) {
			JsonTok j_v = json_value_by_key(j_shp, "v");
			
			if (json_is_null(j_v)) {
				critical_print("Poly shape must have 'v'");
				return 1;
			}

			for (U32 v_i = 0; v_i < json_member_count(j_v); ++v_i) {
				def.polys[def.poly_count].v[v_i] = json_v2(json_member(j_v, v_i));
				++def.polys[def.poly_count].v_count;
			}
			++def.poly_count;
		} else {
			critical_print("Unknown shape type: %s",
					json_str(j_shp_type));
			return 1;
		}
	}

	blob_write(buf, &def, sizeof(def));
	return 0;

error:
	return 1;
}

int blobify_rigidbodydef(struct WArchive *ar, Cson c, const char *base_path)
{
	Cson c_mat = cson_key(c, "mat");
	Cson c_disable_rot = cson_key(c, "disable_rot");
	Cson c_is_static = cson_key(c, "is_static");
	Cson c_shapes = cson_key(c, "shapes");

	if (cson_is_null(c_mat))
		RES_ATTRIB_MISSING("mat");

	if (cson_is_null(c_shapes)) {
		RES_ATTRIB_MISSING("shapes");
	} else if (cson_member_count(c_shapes) > MAX_SHAPES_PER_BODY) {
		critical_print("Too many shapes in RigidBodyDef: %i > %i",
				cson_member_count(c_shapes),
				MAX_SHAPES_PER_BODY);
		goto error;
	}

	RigidBodyDef def = {};
	fmt_str(def.mat_name, sizeof(def.mat_name), "%s",
			cson_string(c_mat, NULL));


	if (!cson_is_null(c_disable_rot))
		def.disable_rot = cson_boolean(c_disable_rot, NULL);

	if (!cson_is_null(c_is_static))
		def.is_static = cson_boolean(c_is_static, NULL);

	/// @todo Take density into account

	for (U32 i = 0; i < cson_member_count(c_shapes); ++i) {
		Cson c_shp = cson_member(c_shapes, i);

		if (!cson_is_compound(c_shp)) {
			critical_print("Attrib 'shapes' should contain objects");
			return 1;
		}

		const char *shp_type = cson_string(cson_key(c_shp, "type"), NULL);
		if (!strcmp(shp_type, "Circle")) {
			Cson c_pos = cson_key(c_shp, "pos");
			Cson c_rad = cson_key(c_shp, "rad");
			
			if (cson_is_null(c_pos) || cson_is_null(c_rad)) {
				critical_print("Circle shape must have 'pos' and 'rad'");
				return 1;
			}

			def.circles[def.circle_count].pos = cson_v2(c_pos, NULL);
			def.circles[def.circle_count].rad = cson_floating(c_rad, NULL);
			++def.circle_count;
		} else if (!strcmp(shp_type, "Poly")) {
			Cson c_v = cson_key(c_shp, "v");
			
			if (cson_is_null(c_v)) {
				critical_print("Poly shape must have 'v'");
				return 1;
			}

			for (U32 v_i = 0; v_i < cson_member_count(c_v); ++v_i) {
				def.polys[def.poly_count].v[v_i] = cson_v2(cson_member(c_v, v_i), NULL);
				++def.polys[def.poly_count].v_count;
			}
			++def.poly_count;
		} else {
			critical_print("Unknown shape type: %s", shp_type);
			return 1;
		}
	}

	pack_buf(ar, &def, sizeof(def));
	return 0;

error:
	return 1;
}

