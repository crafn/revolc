#include "core/ensure.h"
#include "core/debug_print.h"
#include "platform/gl.h"
#include "vao.h"

internal
void unbind_Vao()
{
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

Vao create_Vao(MeshType m, U32 max_v_count, U32 max_i_count)
{
	U32 attrib_count;
	const VertexAttrib *attribs;
	vertex_attributes(
			m,
			&attribs,
			&attrib_count);

	Vao vao= {};
	vao.v_size= vertex_size(m);
	vao.v_capacity= max_v_count;
	vao.i_capacity= max_i_count;

	glGenVertexArrays(1, &vao.vao_id);
	glGenBuffers(1, &vao.vbo_id);
	if (is_indexed_mesh(m))
		glGenBuffers(1, &vao.ibo_id);

	bind_Vao(&vao);
	
	for (U32 i= 0; i < attrib_count; ++i) {
		glEnableVertexAttribArray(i);
		glVertexAttribPointer(
				i,
				attribs[i].size,
				attribs[i].type,
				attribs[i].normalized,
				vao.v_size,
				(const GLvoid*)attribs[i].offset);
	}

	glBufferData(GL_ARRAY_BUFFER, vao.v_size*max_v_count, NULL, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
			sizeof(MeshIndexType)*max_v_count, NULL, GL_STATIC_DRAW);

	return vao;
}

void destroy_Vao(Vao *vao)
{
	ensure(vao->vao_id);

	bind_Vao(vao);

	U32 attrib_count;
	vertex_attributes(vao->mesh_type, NULL, &attrib_count);
	for (U32 i= 0; i < attrib_count; ++i)
		glDisableVertexAttribArray(i);

	unbind_Vao();

	glDeleteVertexArrays(1, &vao->vao_id);
	glDeleteBuffers(1, &vao->vbo_id);
	if (vao->ibo_id)
		glDeleteBuffers(1, &vao->ibo_id);
	vao->vao_id= vao->vbo_id= vao->ibo_id= 0;
}

void bind_Vao(const Vao *vao)
{
	ensure(vao && vao->vao_id);
	glBindVertexArray(vao->vao_id);
	glBindBuffer(GL_ARRAY_BUFFER, vao->vbo_id);
	if (vao->ibo_id)
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vao->ibo_id);
}

void add_mesh_to_Vao(Vao *vao, const Mesh* mesh)
{
	ensure(mesh->mesh_type == vao->mesh_type);

	bind_Vao(vao);

	ensure(vao->v_count + mesh->v_count <= vao->v_capacity);
	ensure(!vao->ibo_id ||
			vao->i_count + mesh->i_count <= vao->i_capacity);

	glBufferSubData(GL_ARRAY_BUFFER,
			vao->v_size*vao->v_count,
			vao->v_size*mesh->v_count,
			mesh_vertices(mesh));
	vao->v_count += mesh->v_count;
	if (vao->ibo_id) {
		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
				sizeof(MeshIndexType)*vao->i_count,
				sizeof(MeshIndexType)*mesh->i_count,
				mesh_indices(mesh));
		vao->i_count += mesh->i_count;
	}
}

void draw_Vao(const Vao *vao)
{
	if (vao->ibo_id) {
		glDrawRangeElements(GL_TRIANGLES,
				0, vao->i_count, vao->i_count, MESH_INDEX_GL_TYPE, 0);
	} else {
		glDrawArrays(GL_POINTS, 0, vao->v_count);
	}
}
