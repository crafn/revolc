#ifndef REVOLC_VISUAL_VAO_H
#define REVOLC_VISUAL_VAO_H

#include "build.h"
#include "mesh.h"

typedef struct Vao {
	U32 vao_id;
	U32 vbo_id;
	U32 ibo_id;

	MeshType mesh_type;
	U32 v_count;
	U32 v_capacity;
	U32 v_size;
	U32 i_count;
	U32 i_capacity;
} Vao;

REVOLC_API Vao create_vao(MeshType m, U32 max_v_count, U32 max_i_count);
REVOLC_API void destroy_vao(Vao *vao);

REVOLC_API void bind_vao(const Vao *vao);
REVOLC_API void unbind_vao();
REVOLC_API void add_mesh_to_vao(Vao *vao, const Mesh *mesh);
REVOLC_API void add_vertices_to_vao(Vao *vao, void *vertices, U32 count);
REVOLC_API void add_indices_to_vao(Vao *vao, MeshIndexType *indices, U32 count);
REVOLC_API void draw_vao(const Vao *vao);

#endif // REVOLC_VISUAL_VAO_H
