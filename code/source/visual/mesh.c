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
	const U32 v_count= 4;
	const U32 i_count= 6;
	BlobOffset v_offset= 0;
	BlobOffset i_offset= 0;

	TriMeshVertex vertices[4]= {};
	vertices[1].pos.x= 0.7;
	vertices[1].uv.x= 1.0;

	vertices[2].pos.x= 1.0;
	vertices[2].pos.y= 0.7;
	vertices[2].uv.x= 1.0;
	vertices[2].uv.y= 1.0;

	vertices[3].pos.y= 1.0;
	vertices[3].uv.y= 1.0;

	MeshIndexType indices[6]= {
		0, 1, 2, 0, 2, 3
	};

	blob_write(blob, offset, &type, sizeof(type));
	blob_write(blob, offset, &v_count, sizeof(v_count));
	blob_write(blob, offset, &i_count, sizeof(i_count));

	v_offset= *offset + sizeof(v_offset) + sizeof(i_offset);
	blob_write(blob, offset, &v_offset, sizeof(v_offset));

	i_offset= *offset + sizeof(i_offset) + sizeof(vertices);
	blob_write(blob, offset, &i_offset, sizeof(i_offset));

	blob_write(blob, offset, &vertices[0], sizeof(vertices));
	blob_write(blob, offset, &indices[0], sizeof(indices));

	return 0;
}
