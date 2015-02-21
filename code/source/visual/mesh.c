#include "core/ensure.h"
#include "global/env.h"
#include "mesh.h"
#include "resources/resblob.h"

void vertex_attributes(MeshType type, const VertexAttrib **attribs, U32 *count)
{
	if (type == MeshType_tri) {
		local_persist VertexAttrib tri_attribs[]= {
			{ "a_pos", 3, GL_FLOAT, false, offsetof(TriMeshVertex, pos) },
			{ "a_uv", 2, GL_FLOAT, false, offsetof(TriMeshVertex, uv) },
			{ "a_color", 4, GL_FLOAT, false, offsetof(TriMeshVertex, color) },
			{ "a_normal", 3, GL_FLOAT, false, offsetof(TriMeshVertex, normal) },
			{ "a_tangent", 3, GL_FLOAT, false, offsetof(TriMeshVertex, tangent) }
		};
		if (attribs)
			*attribs= tri_attribs;
		*count= sizeof(tri_attribs)/sizeof(*tri_attribs);
	} else {
		fail("Unimplemented mesh type");
	}
}

bool is_indexed_mesh(MeshType type)
{ return type == MeshType_tri; }

U32 vertex_size(MeshType type)
{
	switch (type) {
		case MeshType_tri: return sizeof(TriMeshVertex);
		default: fail("Unhandled mesh type");
	}
	return 0;
}

void* mesh_vertices(const Mesh *m)
{ return blob_ptr(g_env.res_blob, m->v_offset); }

MeshIndexType* mesh_indices(const Mesh *m)
{ return (MeshIndexType*)blob_ptr(g_env.res_blob, m->i_offset); }

int json_mesh_to_blob(BlobBuf blob, BlobOffset *offset, JsonTok j)
{
	MeshType type= MeshType_tri;
	BlobOffset v_offset= 0;
	BlobOffset i_offset= 0;

	JsonTok j_pos= json_value_by_key(j, "pos");
	JsonTok j_uv= json_value_by_key(j, "uv");
	JsonTok j_ind= json_value_by_key(j, "ind");

	if (json_is_null(j_pos)) {
		critical_print("Attrib 'pos' missing for Mesh: %s",
				json_str(json_value_by_key(j, "name")));
		return 1;
	}

	if (json_is_null(j_ind)) {
		critical_print("Attrib 'ind' missing for Mesh: %s",
				json_str(json_value_by_key(j, "name")));
		return 1;
	}

	if (	!json_is_null(j_uv) &&
			json_member_count(j_uv) != json_member_count(j_pos)) {
		critical_print("Attrib 'uv' size doesn't match with 'pos' for Mesh: %s",
				json_str(json_value_by_key(j, "name")));
		return 1;
	}

	{
		const U32 v_count= json_member_count(j_pos);
		const U32 i_count= json_member_count(j_ind);
		TriMeshVertex *vertices= malloc(sizeof(*vertices)*v_count);
		MeshIndexType *indices= malloc(sizeof(*indices)*i_count);
		for (U32 i= 0; i < v_count; ++i) {
			JsonTok j_p= json_member(j_pos, i);
			V3f p= {};
			for (U32 c= 0; c < json_member_count(j_p); ++c)
				(&p.x)[c]= json_real(json_member(j_p, c));
			vertices[i].pos= p;

			if (json_is_null(j_uv))
				continue;
			JsonTok j_u= json_member(j_uv, i);
			V2f u= {};
			for (U32 c= 0; c < json_member_count(j_u); ++c)
				(&u.x)[c]= json_real(json_member(j_u, c));
			vertices[i].uv= u;
		}
		for (U32 i= 0; i < i_count; ++i)
			indices[i]= json_integer(json_member(j_ind, i));

		blob_write(blob, offset, &type, sizeof(type));
		blob_write(blob, offset, &v_count, sizeof(v_count));
		blob_write(blob, offset, &i_count, sizeof(i_count));

		v_offset= *offset + sizeof(v_offset) + sizeof(i_offset);
		blob_write(blob, offset, &v_offset, sizeof(v_offset));

		i_offset= *offset + sizeof(i_offset) + sizeof(*vertices)*v_count;
		blob_write(blob, offset, &i_offset, sizeof(i_offset));

		blob_write(blob, offset, &vertices[0], sizeof(*vertices)*v_count);
		blob_write(blob, offset, &indices[0], sizeof(*indices)*i_count);

		free(vertices);
		free(indices);
	}

	return 0;
}
