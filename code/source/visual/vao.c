#include "core/basic.h"
#include "core/debug.h"
#include "core/gl.h"
#include "vao.h"

Vao create_vao(MeshType m, U32 max_v_count, U32 max_i_count)
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

	bind_vao(&vao);
	
	for (U32 i= 0; i < attrib_count; ++i) {
		glEnableVertexAttribArray(i);
		if (attribs[i].floating) {
			glVertexAttribPointer(
					i,
					attribs[i].size,
					attribs[i].type,
					attribs[i].normalized,
					vao.v_size,
					(const GLvoid*)(PtrInt)attribs[i].offset);
		} else {
			glVertexAttribIPointer(
					i,
					attribs[i].size,
					attribs[i].type,
					vao.v_size,
					(const GLvoid*)(PtrInt)attribs[i].offset);
		}
	}

	glBufferData(GL_ARRAY_BUFFER, vao.v_size*max_v_count, NULL, GL_STATIC_DRAW);
	if (vao.ibo_id)
		glBufferData(GL_ELEMENT_ARRAY_BUFFER,
				sizeof(MeshIndexType)*max_i_count, NULL, GL_STATIC_DRAW);

	return vao;
}

void destroy_vao(Vao *vao)
{
	ensure(vao->vao_id);

	bind_vao(vao);

	U32 attrib_count;
	vertex_attributes(vao->mesh_type, NULL, &attrib_count);
	for (U32 i= 0; i < attrib_count; ++i)
		glDisableVertexAttribArray(i);

	unbind_vao();

	glDeleteVertexArrays(1, &vao->vao_id);
	glDeleteBuffers(1, &vao->vbo_id);
	if (vao->ibo_id)
		glDeleteBuffers(1, &vao->ibo_id);
	vao->vao_id= vao->vbo_id= vao->ibo_id= 0;
}

void bind_vao(const Vao *vao)
{
	ensure(vao && vao->vao_id);
	glBindVertexArray(vao->vao_id);
	glBindBuffer(GL_ARRAY_BUFFER, vao->vbo_id);
	if (vao->ibo_id)
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vao->ibo_id);
}

void unbind_vao()
{
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void add_mesh_to_vao(Vao *vao, const Mesh* mesh)
{
	ensure(mesh->mesh_type == vao->mesh_type);

	add_vertices_to_vao(vao, mesh_vertices(mesh), mesh->v_count);
	if (vao->ibo_id)
		add_indices_to_vao(vao, mesh_indices(mesh), mesh->i_count);
}

void add_vertices_to_vao(Vao *vao, void *vertices, U32 count)
{
	ensure(vao->v_count + count <= vao->v_capacity);
	glBufferSubData(GL_ARRAY_BUFFER,
			vao->v_size*vao->v_count,
			vao->v_size*count,
			vertices);
	vao->v_count += count;
}

void add_indices_to_vao(Vao *vao, MeshIndexType *indices, U32 count)
{
	ensure(vao->ibo_id);
	ensure(vao->i_count + count <= vao->i_capacity);

	/*const U32 buf_size= 1024;
	MeshIndexType buf[buf_size];
	for (U32 i= 0; i < count; i += buf_size) {
		U32 count_in_buf= buf_size;
		if (i + count_in_buf > count)
			count_in_buf= count - i; 

		for (U32 k= 0; k < count_in_buf; ++k)
			buf[k]= indices[i + k] + vao->v_count;

		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
				sizeof(MeshIndexType)*vao->i_count,
				sizeof(MeshIndexType)*count_in_buf,
				buf);
		vao->i_count += count_in_buf;
	}
	*/

	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER,
			sizeof(MeshIndexType)*vao->i_count,
			sizeof(MeshIndexType)*count,
			indices);
	vao->i_count += count;
}

void reset_vao_mesh(Vao *vao)
{ vao->v_count= vao->i_count= 0; }

void draw_vao(const Vao *vao)
{
	if (vao->ibo_id) {
		glDrawRangeElements(GL_TRIANGLES,
				0, vao->i_count, vao->i_count, MESH_INDEX_GL_TYPE, 0);
	} else {
		glDrawArrays(GL_POINTS, 0, vao->v_count);
	}
}
