#ifndef REVOLC_VISUAL_VAO_H
#define REVOLC_VISUAL_VAO_H

#include "build.h"
#include "mesh.h"

typedef struct {
	U32 vao_id;
	U32 vbo_id;
	U32 ibo_id;

	MeshType mesh_type;
	U32 v_count;
	U32 v_capacity;
	U32 v_size;
	U32 i_count; // U32 indices
	U32 i_capacity;
} Vao;

REVOLC_API Vao create_Vao(MeshType m, U32 max_v_count, U32 max_i_count);
REVOLC_API void destroy_Vao(Vao *vao);

REVOLC_API void bind_Vao(const Vao *vao);
REVOLC_API void add_mesh_to_Vao(Vao *vao, const Mesh* mesh);
REVOLC_API void draw_Vao(const Vao *vao);

#endif // REVOLC_VISUAL_VAO_H
