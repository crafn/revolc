#include "core/array.h"
#include "core/debug_print.h"
#include "core/ensure.h"
#include "core/malloc.h"
#include "core/vector.h"
#include "renderer.h"
#include "vao.h"

#include <stdlib.h>
#include <string.h>

Renderer* create_renderer()
{
	const U32 default_count= 1024;

	Renderer *rend= zero_malloc(sizeof(*rend));
	rend->entities= zero_malloc(sizeof(*rend->entities)*default_count);
	rend->next_entity= 0;
	rend->max_entity_count= default_count;

	if (!g_env.renderer)
		g_env.renderer= rend;

	return rend;
}

void destroy_renderer(Renderer *rend)
{
	if (g_env.renderer == rend)
		g_env.renderer= NULL;
	free(rend->entities);
	free(rend);
}

U32 create_modelentity(Renderer *r, const Model *model)
{
	if (r->entity_count == r->max_entity_count) {
		debug_print("Enlargening entity array: %i", (int)r->max_entity_count);
		r->entities= enlarge_array(
			r->entities, &r->max_entity_count, sizeof(*r->entities));
	}

	while (r->entities[r->next_entity].model)
		r->next_entity= (r->next_entity + 1) % r->max_entity_count;

	{ // Init entity
		ModelEntity *e= &r->entities[r->next_entity];
		e->model= model;
		e->pos.x= 0; e->pos.y= 0; e->pos.z= 0;
		for (int i= 0; i < MODEL_TEX_COUNT; ++i) {
			Texture* tex= model_texture(model, 0);
			if (tex)
				e->tex_gl_ids[i]= tex->gl_id;
		}
		e->vertices= (TriMeshVertex*)mesh_vertices(model_mesh(model));
		e->indices= (MeshIndexType*)mesh_indices(model_mesh(model));
		e->mesh_v_count= model_mesh(model)->v_count;
		e->mesh_i_count= model_mesh(model)->i_count;
	}

	++r->entity_count;
	return r->next_entity++;
}

void destroy_modelentity(Renderer *r, U32 h)
{
	ensure(h < r->max_entity_count);
	r->entities[h].model= NULL;
	--r->entity_count;
}

ModelEntity* get_modelentity(Renderer *r, U32 h)
{
	ensure(h < r->max_entity_count);
	ensure(r->entities[h].model != NULL);
	return &r->entities[h];
}

internal
int entity_cmp(const void *e1, const void *e2)
{
	F32 a= ((ModelEntity*)e1)->pos.z;
	F32 b= ((ModelEntity*)e2)->pos.z;
	return (a < b) - (a > b);
}

void render_frame(Renderer *r, float cam_x, float cam_y)
{
	U32 total_v_count= 0;
	U32 total_i_count= 0;
	U32 tex_gl_id= 0; /// @todo TEMP, need texture atlas
	for (U32 i= 0; i < r->max_entity_count; ++i) {
		ModelEntity *e= &r->entities[i];
		if (!e->model)
			continue;

		if (e->tex_gl_ids[0])
			tex_gl_id= e->tex_gl_ids[0];
		total_v_count += e->mesh_v_count;
		total_i_count += e->mesh_i_count;
	}

	Vao vao= create_vao(MeshType_tri, total_v_count, total_i_count);
	bind_vao(&vao);

	{ // Meshes to Vao
		ModelEntity *entities= malloc(sizeof(*r->entities)*r->max_entity_count);	
		memcpy(entities, r->entities, sizeof(*r->entities)*r->max_entity_count);

		// Z-sort
		qsort(entities, r->max_entity_count, sizeof(*entities), entity_cmp);

		TriMeshVertex *total_verts= malloc(sizeof(*total_verts)*total_v_count);
		MeshIndexType *total_inds= malloc(sizeof(*total_inds)*total_i_count);
		U32 cur_v= 0;
		U32 cur_i= 0;
		for (U32 i= 0; i < r->max_entity_count; ++i) {
			ModelEntity *e= &entities[i];
			if (!e->model)
				continue;

			for (U32 k= 0; k < e->mesh_i_count; ++k) {
				total_inds[cur_i]= e->indices[k] + cur_v;
				++cur_i;
			}
			for (U32 k= 0; k < e->mesh_v_count; ++k) {
				TriMeshVertex v= e->vertices[k];
				v.pos.x += e->pos.x;
				v.pos.y += e->pos.y;
				v.pos.z += e->pos.z;
				total_verts[cur_v]= v;
				++cur_v;
			}
		}
		free(entities);
		add_vertices_to_vao(&vao, total_verts, total_v_count);
		add_indices_to_vao(&vao, total_inds, total_i_count);
		free(total_verts);
		free(total_inds);
	}

	{ // Actual rendering
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, tex_gl_id);

		Shader* shd=
			(Shader*)res_by_name(
					g_env.res_blob,
					ResType_Shader,
					"gen_shader");
		glUseProgram(shd->prog_gl_id);
		glUniform1i(glGetUniformLocation(shd->prog_gl_id, "u_tex_color"), 0);
		glUniform2f(glGetUniformLocation(shd->prog_gl_id, "u_cursor"), cam_x, cam_y);

		glViewport(0, 0,
				g_env.device->win_size[0],
				g_env.device->win_size[1]);
		glClear(GL_COLOR_BUFFER_BIT);
		draw_vao(&vao);
	}

	destroy_vao(&vao);
}
