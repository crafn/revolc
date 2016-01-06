#include "core/debug.h"
#include "mesh.h"
#include "resources/resblob.h"

void vertex_attributes(MeshType type, const VertexAttrib **attribs, U32 *count)
{
	if (type == MeshType_tri) {
		local_persist VertexAttrib tri_attribs[] = {
			{ "a_pos", 3, GL_FLOAT, true, false, offsetof(TriMeshVertex, pos) },
			{ "a_uv", 3, GL_FLOAT, true, false, offsetof(TriMeshVertex, uv) },
			{ "a_outline_uv", 2, GL_FLOAT, true, false, offsetof(TriMeshVertex, outline_uv) },
			{ "a_color", 4, GL_FLOAT, true, false, offsetof(TriMeshVertex, color) },
			{ "a_outline_color", 4, GL_FLOAT, true, false, offsetof(TriMeshVertex, outline_color) },
			{ "a_outline_width", 1, GL_FLOAT, true, false, offsetof(TriMeshVertex, outline_width) },
			{ "a_outline_exp", 1, GL_FLOAT, true, false, offsetof(TriMeshVertex, outline_exp) },
			{ "a_emission", 1, GL_FLOAT, true, false, offsetof(TriMeshVertex, emission) },
		};
		if (attribs)
			*attribs = tri_attribs;
		*count = sizeof(tri_attribs)/sizeof(*tri_attribs);
	} else {
		fail("Unimplemented mesh type");
	}
}

bool is_indexed_mesh(MeshType type)
{
	switch (type) {
		case MeshType_tri: return true;
		default: fail("Unhandled mesh type");
	}
}

U32 vertex_size(MeshType type)
{
	switch (type) {
		case MeshType_tri: return sizeof(TriMeshVertex);
		default: fail("Unhandled mesh type");
	}
}

TriMeshVertex default_vertex()
{
	return (TriMeshVertex) {
		.color = white_color(),
		.outline_color = white_color(),
		.outline_width = 1,
		.outline_exp = 1,
	};
}

TriMeshVertex * mesh_vertices(const Mesh *m)
{ return rel_ptr(&m->vertices); }

MeshIndexType * mesh_indices(const Mesh *m)
{ return rel_ptr(&m->indices); }

int json_mesh_to_blob(struct BlobBuf *buf, JsonTok j)
{
	MeshType type = MeshType_tri;

	JsonTok j_pos = json_value_by_key(j, "pos");
	JsonTok j_uv = json_value_by_key(j, "uv");
	JsonTok j_outline_uv = json_value_by_key(j, "outline_uv");
	JsonTok j_ind = json_value_by_key(j, "ind");

	if (json_is_null(j_pos))
		RES_ATTRIB_MISSING("pos");
	if (json_is_null(j_ind))
		RES_ATTRIB_MISSING("ind");

	if (	!json_is_null(j_uv) &&
			json_member_count(j_uv) != json_member_count(j_pos)) {
		critical_print("Attrib 'uv' size doesn't match with 'pos' for Mesh: %s",
				json_str(json_value_by_key(j, "name")));
		goto error;
	}

	if (	!json_is_null(j_outline_uv) &&
			json_member_count(j_outline_uv) != json_member_count(j_pos)) {
		critical_print("Attrib 'outline_uv' size doesn't match with 'pos' for Mesh: %s",
				json_str(json_value_by_key(j, "name")));
		goto error;
	}

	{
		const U32 v_count = json_member_count(j_pos);
		const U32 i_count = json_member_count(j_ind);
		TriMeshVertex *vertices = ZERO_ALLOC(dev_ator(), sizeof(*vertices)*v_count, "verts");
		MeshIndexType *indices = ZERO_ALLOC(dev_ator(), sizeof(*indices)*i_count, "inds");
		for (U32 i = 0; i < v_count; ++i) {
			JsonTok j_p = json_member(j_pos, i);
			V3f p = {};
			for (U32 c = 0; c < json_member_count(j_p); ++c)
				(&p.x)[c] = json_real(json_member(j_p, c));
			vertices[i] = default_vertex();
			vertices[i].pos = p;

			if (!json_is_null(j_uv)) {
				JsonTok j_u = json_member(j_uv, i);
				V3f u = {};
				u.x = json_real(json_member(j_u, 0));
				u.y = json_real(json_member(j_u, 1));
				vertices[i].uv = u;
			}

			if (!json_is_null(j_outline_uv)) {
				JsonTok j_u = json_member(j_outline_uv, i);
				V2f u = {};
				u.x = json_real(json_member(j_u, 0));
				u.y = json_real(json_member(j_u, 1));
				vertices[i].outline_uv = u;
			}
		}
		for (U32 i = 0; i < i_count; ++i)
			indices[i] = json_integer(json_member(j_ind, i));

		// @todo Fill Mesh and write it instead of individual members
		Resource res;

		blob_write(buf, &res, sizeof(res));
		blob_write(buf, &type, sizeof(type));
		blob_write(buf, &v_count, sizeof(v_count));
		blob_write(buf, &i_count, sizeof(i_count));

		U32 v_offset = buf->offset;
		RelPtr rel_ptr = {};
		blob_write(buf, &rel_ptr, sizeof(rel_ptr));

		U32 i_offset = buf->offset;
		blob_write(buf, &rel_ptr, sizeof(rel_ptr));

		blob_patch_rel_ptr(buf, v_offset);
		blob_write(buf, &vertices[0], sizeof(*vertices)*v_count);

		blob_patch_rel_ptr(buf, i_offset);
		blob_write(buf, &indices[0], sizeof(*indices)*i_count);

		FREE(dev_ator(), vertices);
		FREE(dev_ator(), indices);
	}

	return 0;

error:
	return 1;
}

void mesh_to_json(WJson *j, const Mesh *m)
{
	WJson *j_pos = wjson_named_member(j, JsonType_array, "pos");
	WJson *j_uv = wjson_named_member(j, JsonType_array, "uv");
	WJson *j_ind = wjson_named_member(j, JsonType_array, "ind");

	for (U32 i = 0; i < m->v_count; ++i) {
		wjson_append(	j_pos,
						wjson_v3(v3f_to_v3d(mesh_vertices(m)[i].pos)));
		wjson_append(	j_uv,
						wjson_v3(v3f_to_v3d(mesh_vertices(m)[i].uv)));
	}

	for (U32 i = 0; i < m->i_count; ++i) {
		wjson_append(	j_ind,
						wjson_number(mesh_indices(m)[i]));
	}
}

