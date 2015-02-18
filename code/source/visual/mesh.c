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

Bool is_indexed_mesh(MeshType type)
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
