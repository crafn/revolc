#include "core/array.h"
#include "core/debug_print.h"
#include "core/ensure.h"
#include "core/malloc.h"
#include "core/vector.h"
#include "renderer.h"
#include "resources/resblob.h"
#include "vao.h"

#include <stdlib.h>
#include <string.h>

#define ATLAS_WIDTH 4096

/// Helper in `recreate_texture_atlas`
typedef struct {
	Texture *tex;
	V2i reso;
	V3f *atlas_uv;
	Texel *texels;
} TexInfo;

int texinfo_cmp(const void *a_, const void *b_)
{
	const TexInfo *a= a_, *b= b_;
	if (a->reso.y == b->reso.y)
		return b->reso.x - a->reso.x;
	return b->reso.y - a->reso.y;
}

internal
void recreate_texture_atlas(Renderer *r, ResBlob *blob)
{
	gl_check_errors("recreate_texture_atlas: begin");
	if (r->atlas_gl_id)
		glDeleteTextures(1, &r->atlas_gl_id);

	glGenTextures(1, &r->atlas_gl_id);
	ensure(r->atlas_gl_id);
	glBindTexture(GL_TEXTURE_2D_ARRAY, r->atlas_gl_id);

	const U32 mip_levels= 1;
	const U32 layers= 4;

	glTexStorage3D(GL_TEXTURE_2D_ARRAY, mip_levels, GL_RGBA8,
			ATLAS_WIDTH, ATLAS_WIDTH, layers);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, 0); /// @todo Mipmaps
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 1000);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, -1000);

	// Gather TexInfos
	/// @todo MissingResource
	U32 tex_count= 0;
	TexInfo *texs= NULL;
	{
		U32 first_res;
		all_res_by_type(&first_res, &tex_count, blob, ResType_Texture);
		texs= malloc(sizeof(*texs)*tex_count);
		for (	U32 cur_res= first_res, i= 0;
				i < tex_count;
				++cur_res, ++i) {
			Texture *tex= (Texture*)res_by_index(blob, cur_res);
			texs[i]= (TexInfo) {
				.tex= tex,
				.reso= tex->reso,
				.atlas_uv= &tex->atlas_uv,
				.texels= tex->texels,
			};
		}
		qsort(texs, tex_count, sizeof(*texs), texinfo_cmp);
	}

	// Blit to atlas
	int x= 0, y= 0, z= 0;
	int last_row_height= 0;
	for (U32 i= 0; i < tex_count; ++i) {
		TexInfo *tex= &texs[i];

		if (	tex->reso.x > ATLAS_WIDTH ||
				tex->reso.y > ATLAS_WIDTH)
			fail("Too large texture (max %i): %s",
					ATLAS_WIDTH, tex->tex->res.name);

		if (x + tex->reso.x > ATLAS_WIDTH) {
			y += last_row_height;
			x= 0;
			last_row_height= 0;
		}

		if (y + tex->reso.y > ATLAS_WIDTH) {
			x= 0;
			y= 0;
			++z;
		}

		if (z > layers)
			critical_print("Texture atlas full!");

		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
				x, y, z,
				tex->reso.x, tex->reso.y, 1,
				GL_RGBA, GL_UNSIGNED_BYTE, tex->texels);
		*tex->atlas_uv= (V3f) {
			(F32)x/ATLAS_WIDTH,
			(F32)y/ATLAS_WIDTH,
			z
		};

		x += tex->reso.x;
		if (tex->reso.y > last_row_height)
			last_row_height= tex->reso.y;
	}

	free(texs);
	gl_check_errors("recreate_texture_atlas: end");
}


Renderer* create_renderer()
{
	const U32 default_count= 1024;

	Renderer *rend= zero_malloc(sizeof(*rend));
	rend->entities= zero_malloc(sizeof(*rend->entities)*default_count);
	rend->next_entity= 0;
	rend->max_entity_count= default_count;

	recreate_texture_atlas(rend, g_env.res_blob);

	if (!g_env.renderer)
		g_env.renderer= rend;

	return rend;
}

void destroy_renderer(Renderer *r)
{
	if (g_env.renderer == r)
		g_env.renderer= NULL;
	glDeleteTextures(1, &r->atlas_gl_id);
	free(r->entities);
	free(r);
}

internal
void init_modelentity(ModelEntity *e, const Model *model)
{
	Texture *tex= model_texture(model, 0);
	e->model= model;
	e->pos.x= 0; e->pos.y= 0; e->pos.z= 0;
	e->atlas_uv= tex->atlas_uv;
	e->scale_to_atlas_uv= (V2f) {
		(F32)tex->reso.x/ATLAS_WIDTH,
		(F32)tex->reso.y/ATLAS_WIDTH,
	};
	e->vertices= (TriMeshVertex*)mesh_vertices(model_mesh(model));
	e->indices= (MeshIndexType*)mesh_indices(model_mesh(model));
	e->mesh_v_count= model_mesh(model)->v_count;
	e->mesh_i_count= model_mesh(model)->i_count;
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

	ModelEntity *e= &r->entities[r->next_entity];
	init_modelentity(e, model);

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
	for (U32 i= 0; i < r->max_entity_count; ++i) {
		ModelEntity *e= &r->entities[i];
		if (!e->model)
			continue;

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

				v.uv.x *= e->scale_to_atlas_uv.x;
				v.uv.y *= e->scale_to_atlas_uv.y;

				v.uv.x += e->atlas_uv.x;
				v.uv.y += e->atlas_uv.y;
				v.uv.z += e->atlas_uv.z;

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
		glBindTexture(GL_TEXTURE_2D_ARRAY, r->atlas_gl_id);

		ShaderSource* shd=
			(ShaderSource*)res_by_name(
					g_env.res_blob,
					ResType_ShaderSource,
					"gen");
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

void on_res_reload(Renderer *r, ResBlob *new_blob)
{
	recreate_texture_atlas(r, new_blob);

	for (U32 e_i= 0; e_i < r->entity_count; ++e_i) {
		ModelEntity *e= &r->entities[e_i];
		const Model *m= e->model=
			(Model*)res_by_name(
					new_blob,
					e->model->res.type,
					e->model->res.name);
		init_modelentity(e, m);
	}
}
