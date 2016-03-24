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
			{ "a_color_exp", 1, GL_FLOAT, true, false, offsetof(TriMeshVertex, color_exp) },
			{ "a_outline_exp", 1, GL_FLOAT, true, false, offsetof(TriMeshVertex, outline_exp) },
			{ "a_outline_width", 1, GL_FLOAT, true, false, offsetof(TriMeshVertex, outline_width) },
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
		.color_exp = 1,
		.outline_exp = 1,
		.outline_width = 1,
	};
}

TriMeshVertex * mesh_vertices(const Mesh *m)
{ return rel_ptr(&m->vertices); }

MeshIndexType * mesh_indices(const Mesh *m)
{ return rel_ptr(&m->indices); }

Mesh *blobify_mesh(struct WArchive *ar, Cson c, bool *err)
{
	MeshType type = MeshType_tri;

	Cson c_pos = cson_key(c, "pos");
	Cson c_uv = cson_key(c, "uv");
	Cson c_outline_uv = cson_key(c, "outline_uv");
	Cson c_color = cson_key(c, "color");
	Cson c_color_exp = cson_key(c, "color_exp");
	Cson c_outline_color = cson_key(c, "outline_color");
	Cson c_outline_exp = cson_key(c, "outline_exp");
	Cson c_outline_width = cson_key(c, "outline_width");
	Cson c_ind = cson_key(c, "ind");

	if (cson_is_null(c_pos))
		RES_ATTRIB_MISSING("pos");
	if (cson_is_null(c_ind))
		RES_ATTRIB_MISSING("ind");

	if (	!cson_is_null(c_uv) &&
			cson_member_count(c_uv) != cson_member_count(c_pos)) {
		critical_print("Attrib 'uv' size doesn't match with 'pos' for Mesh: %s",
				blobify_string(cson_key(c, "name"), err));
		goto error;
	}

	if (	!cson_is_null(c_outline_uv) &&
			cson_member_count(c_outline_uv) != cson_member_count(c_pos)) {
		critical_print("Attrib 'outline_uv' size doesn't match with 'pos' for Mesh: %s",
				blobify_string(cson_key(c, "name"), err));
		goto error;
	}

	{
		const U32 v_count = cson_member_count(c_pos);
		const U32 i_count = cson_member_count(c_ind);
		TriMeshVertex *vertices = ZERO_ALLOC(dev_ator(), sizeof(*vertices)*v_count, "verts");
		MeshIndexType *indices = ZERO_ALLOC(dev_ator(), sizeof(*indices)*i_count, "inds");
		for (U32 i = 0; i < v_count; ++i) {
			Cson c_p = cson_member(c_pos, i);
			V3f p = {};
			for (U32 c = 0; c < cson_member_count(c_p); ++c)
				(&p.x)[c] = blobify_floating(cson_member(c_p, c), err);
			vertices[i] = default_vertex();
			vertices[i].pos = p;

			if (!cson_is_null(c_uv)) {
				Cson c_u = cson_member(c_uv, i);
				V3f u = {};
				u.x = blobify_floating(cson_member(c_u, 0), err);
				u.y = blobify_floating(cson_member(c_u, 1), err);
				vertices[i].uv = u;
			}

			if (!cson_is_null(c_outline_uv)) {
				Cson c_u = cson_member(c_outline_uv, i);
				vertices[i].outline_uv = v2d_to_v2f(blobify_v2(c_u, err));
			}

			if (!cson_is_null(c_color)) {
				Cson c = cson_member(c_color, i);
				vertices[i].color = blobify_color(c, err);
			}

			if (!cson_is_null(c_outline_color)) {
				Cson c = cson_member(c_outline_color, i);
				vertices[i].outline_color = blobify_color(c, err);
			}

			if (!cson_is_null(c_color_exp)) {
				Cson c = cson_member(c_color_exp, i);
				vertices[i].color_exp = blobify_floating(c, err);
			}

			if (!cson_is_null(c_outline_exp)) {
				Cson c = cson_member(c_outline_exp, i);
				vertices[i].outline_exp = blobify_floating(c, err);
			}

			if (!cson_is_null(c_outline_width)) {
				Cson c = cson_member(c_outline_width, i);
				vertices[i].outline_width = blobify_floating(c, err);
			}

		}
		for (U32 i = 0; i < i_count; ++i)
			indices[i] = blobify_integer(cson_member(c_ind, i), err);

		if (err && *err)
			goto error;

		Mesh *ptr = warchive_ptr(ar);

		// @todo Fill Mesh and write it instead of individual members
		Resource res;

		pack_buf(ar, &res, sizeof(res));
		pack_buf(ar, &type, sizeof(type));
		pack_buf(ar, &v_count, sizeof(v_count));
		pack_buf(ar, &i_count, sizeof(i_count));

		U32 v_offset = ar->data_size;
		RelPtr rel_ptr = {};
		pack_buf(ar, &rel_ptr, sizeof(rel_ptr));

		U32 i_offset = ar->data_size;
		pack_buf(ar, &rel_ptr, sizeof(rel_ptr));

		pack_patch_rel_ptr(ar, v_offset);
		pack_buf(ar, &vertices[0], sizeof(*vertices)*v_count);

		pack_patch_rel_ptr(ar, i_offset);
		pack_buf(ar, &indices[0], sizeof(*indices)*i_count);

		FREE(dev_ator(), vertices);
		FREE(dev_ator(), indices);

		return ptr;
	}

error:
	SET_ERROR_FLAG(err);
	return NULL;
}

void deblobify_mesh(WCson *c, struct RArchive *ar)
{
	Mesh *m = rarchive_ptr(ar, sizeof(*m));
	unpack_advance(ar,	sizeof(*m) +
						sizeof(*mesh_vertices(m))*m->v_count +
						sizeof(*mesh_indices(m))*m->i_count);

	TriMeshVertex *v = mesh_vertices(m);

	wcson_begin_compound(c, "Mesh");

	wcson_designated(c, "name");
	deblobify_string(c, m->res.name);

	wcson_designated(c, "pos");
	wcson_begin_initializer(c);
	for (U32 i = 0; i < m->v_count; ++i)
		deblobify_v3(c, v3f_to_v3d(v[i].pos));
	wcson_end_initializer(c);
	
	wcson_designated(c, "uv");
	wcson_begin_initializer(c);
	for (U32 i = 0; i < m->v_count; ++i)
		deblobify_v2(c, v2f_to_v2d(v3f_to_v2f(v[i].uv)));
	wcson_end_initializer(c);

	wcson_designated(c, "outline_uv");
	wcson_begin_initializer(c);
	for (U32 i = 0; i < m->v_count; ++i)
		deblobify_v2(c, v2f_to_v2d(v[i].outline_uv));
	wcson_end_initializer(c);

	wcson_designated(c, "color");
	wcson_begin_initializer(c);
	for (U32 i = 0; i < m->v_count; ++i)
		deblobify_color(c, v[i].color);
	wcson_end_initializer(c);

	wcson_designated(c, "outline_color");
	wcson_begin_initializer(c);
	for (U32 i = 0; i < m->v_count; ++i)
		deblobify_color(c, v[i].outline_color);
	wcson_end_initializer(c);

	wcson_designated(c, "color_exp");
	wcson_begin_initializer(c);
	for (U32 i = 0; i < m->v_count; ++i)
		deblobify_floating(c, v[i].color_exp);
	wcson_end_initializer(c);

	wcson_designated(c, "outline_exp");
	wcson_begin_initializer(c);
	for (U32 i = 0; i < m->v_count; ++i)
		deblobify_floating(c, v[i].outline_exp);
	wcson_end_initializer(c);

	wcson_designated(c, "outline_width");
	wcson_begin_initializer(c);
	for (U32 i = 0; i < m->v_count; ++i)
		deblobify_floating(c, v[i].outline_width);
	wcson_end_initializer(c);

	wcson_designated(c, "ind");
	wcson_begin_initializer(c);
	for (U32 i = 0; i < m->i_count; ++i)
		deblobify_integer(c, mesh_indices(m)[i]);
	wcson_end_initializer(c);

	wcson_end_compound(c);
}

void add_rt_mesh_vertex(Mesh *mesh, TriMeshVertex vertex)
{
	U32 old_size = sizeof(TriMeshVertex)*mesh->v_count;
	U32 new_size = sizeof(TriMeshVertex)*(mesh->v_count + 1);
	realloc_res_member(&mesh->res, &mesh->vertices, new_size, old_size);
	mesh_vertices(mesh)[mesh->v_count++] = vertex;

	resource_modified(&mesh->res);
}

void add_rt_mesh_index(Mesh *mesh, MeshIndexType index)
{
	U32 old_size = sizeof(MeshIndexType)*mesh->i_count;
	U32 new_size = sizeof(MeshIndexType)*(mesh->i_count + 1);
	realloc_res_member(&mesh->res, &mesh->indices, new_size, old_size);
	mesh_indices(mesh)[mesh->i_count++] = index;

	resource_modified(&mesh->res);
}

void remove_rt_mesh_vertex(Mesh *mesh, MeshIndexType index)
{
	ensure(index < mesh->v_count);

	// Remove faces of which the vertex is part of
	for (U32 i = 0; i + 2 < mesh->i_count;) {
		MeshIndexType *inds = mesh_indices(mesh);
		if (	inds[i + 0] == index ||
				inds[i + 1] == index ||
				inds[i + 2] == index) {
			// Remove face
			memmove(&inds[i], &inds[i + 3], sizeof(*inds)*(mesh->i_count - i - 3));
			realloc_res_member(	&mesh->res, &mesh->indices,
								sizeof(*inds)*(mesh->i_count - 3),
								sizeof(*inds)*(mesh->i_count));
			mesh->i_count -= 3;
		} else {
			i += 3;
		}
	}

	{ // Remove vertex
		TriMeshVertex *verts = mesh_vertices(mesh);
		memmove(&verts[index], &verts[index + 1], sizeof(*verts)*(mesh->v_count - index - 1));
		realloc_res_member(	&mesh->res, &mesh->indices,
							sizeof(*verts)*(mesh->v_count - 1),
							sizeof(*verts)*(mesh->v_count));
		--mesh->v_count;
	}

	// Remap indices
	MeshIndexType *inds = mesh_indices(mesh);
	for (U32 i = 0; i < mesh->i_count; ++i) {
		if (inds[i] > index)
			--inds[i];
	}

	resource_modified(&mesh->res);
}

