#include "core/debug_print.h"
#include "core/ensure.h"
#include "core/malloc.h"
#include "core/vector.h"
#include "renderer.h"
#include "resources/resblob.h"
#include "vao.h"

#include <stdlib.h>
#include <string.h>

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
	const U32 layers= TEXTURE_ATLAS_LAYER_COUNT;

	glTexStorage3D(GL_TEXTURE_2D_ARRAY, mip_levels, GL_RGBA8,
			TEXTURE_ATLAS_WIDTH, TEXTURE_ATLAS_WIDTH, layers);

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

		if (	tex->reso.x > TEXTURE_ATLAS_WIDTH ||
				tex->reso.y > TEXTURE_ATLAS_WIDTH)
			fail("Too large texture (max %i): %s",
					TEXTURE_ATLAS_WIDTH, tex->tex->res.name);

		if (x + tex->reso.x > TEXTURE_ATLAS_WIDTH) {
			y += last_row_height;
			x= 0;
			last_row_height= 0;
		}

		if (y + tex->reso.y > TEXTURE_ATLAS_WIDTH) {
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
			(F32)x/TEXTURE_ATLAS_WIDTH,
			(F32)y/TEXTURE_ATLAS_WIDTH,
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
	Renderer *rend= zero_malloc(sizeof(*rend));
	rend->cam_pos.z= 5.0;

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
	free(r);
}

U32 alloc_modelentity(Renderer *r)
{
	if (r->entity_count >= MAX_MODELENTITY_COUNT)
		fail("Too many modelentities");

	while (r->entities[r->next_entity].allocated)
		r->next_entity= (r->next_entity + 1) % MAX_MODELENTITY_COUNT;

	ModelEntity *e= &r->entities[r->next_entity];
	*e= (ModelEntity) { .allocated= true };

	++r->entity_count;
	return r->next_entity;
}

void free_modelentity(Renderer *r, U32 h)
{
	ensure(h < MAX_MODELENTITY_COUNT);
	r->entities[h]= (ModelEntity) { .allocated= false };
	--r->entity_count;
}

void set_modelentity(Renderer *r, U32 h, const Model *model)
{
	ModelEntity *e= &r->entities[h];
	Texture *tex= model_texture(model, 0);
	e->pos.x= 0; e->pos.y= 0; e->pos.z= 0;
	strncpy(e->model_name, model->res.name, sizeof(e->model_name));
	e->atlas_uv= tex->atlas_uv;
	e->scale_to_atlas_uv= (V2f) {
		(F32)tex->reso.x/TEXTURE_ATLAS_WIDTH,
		(F32)tex->reso.y/TEXTURE_ATLAS_WIDTH,
	};
	e->vertices= (TriMeshVertex*)mesh_vertices(model_mesh(model));
	e->indices= (MeshIndexType*)mesh_indices(model_mesh(model));
	e->mesh_v_count= model_mesh(model)->v_count;
	e->mesh_i_count= model_mesh(model)->i_count;
}

internal
int entity_cmp(const void *e1, const void *e2)
{
	F32 a= ((ModelEntity*)e1)->pos.z;
	F32 b= ((ModelEntity*)e2)->pos.z;
	// Largest Z first (nearest camera)
	return (a < b) - (a > b);
}

typedef struct {
	F32 e[16];
} M44f;

internal
M44f mul_m44f(const M44f a, const M44f b)
{
	M44f result;
	for (U32 r= 0; r < 4; ++r)
	for (U32 c= 0; c < 4; ++c)
		result.e[c + 4*r]=
			a.e[0 + 4*r]*b.e[c + 4*0] +
			a.e[1 + 4*r]*b.e[c + 4*1] +
			a.e[2 + 4*r]*b.e[c + 4*2] +
			a.e[3 + 4*r]*b.e[c + 4*3];
	return result;
}

void render_frame(Renderer *r)
{
	U32 total_v_count= 0;
	U32 total_i_count= 0;
	for (U32 i= 0; i < MAX_MODELENTITY_COUNT; ++i) {
		ModelEntity *e= &r->entities[i];
		if (!e->model_name[0])
			continue;

		total_v_count += e->mesh_v_count;
		total_i_count += e->mesh_i_count;
	}

	Vao vao= create_vao(MeshType_tri, total_v_count, total_i_count);
	bind_vao(&vao);

	{ // Meshes to Vao
		ModelEntity *entities= malloc(sizeof(*r->entities)*MAX_MODELENTITY_COUNT);	
		memcpy(entities, r->entities, sizeof(*r->entities)*MAX_MODELENTITY_COUNT);

		// Z-sort
		qsort(entities, MAX_MODELENTITY_COUNT, sizeof(*entities), entity_cmp);

		TriMeshVertex *total_verts= malloc(sizeof(*total_verts)*total_v_count);
		MeshIndexType *total_inds= malloc(sizeof(*total_inds)*total_i_count);
		U32 cur_v= 0;
		U32 cur_i= 0;
		for (U32 i= 0; i < MAX_MODELENTITY_COUNT; ++i) {
			ModelEntity *e= &entities[i];
			if (!e->model_name[0])
				continue;

			for (U32 k= 0; k < e->mesh_i_count; ++k) {
				total_inds[cur_i]= e->indices[k] + cur_v;
				++cur_i;
			}

			F64 cs= e->rot.cs;
			F64 sn= e->rot.sn;

			for (U32 k= 0; k < e->mesh_v_count; ++k) {
				TriMeshVertex v= e->vertices[k];

				// Rotate
				V2d p= {v.pos.x, v.pos.y};
				v.pos.x = cs*p.x - sn*p.y;
				v.pos.y = sn*p.x + cs*p.y;

				// Translate
				v.pos.x += e->pos.x;
				v.pos.y += e->pos.y;
				v.pos.z += e->pos.z;

				v.uv.x *= e->scale_to_atlas_uv.x;
				v.uv.y *= e->scale_to_atlas_uv.y;

				v.uv.x += e->atlas_uv.x;
				v.uv.y += e->atlas_uv.y;
				v.uv.z += e->atlas_uv.z;

				v.color= (Color) {1, 1, 1, 1};

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
		F32 cx= r->cam_pos.x;
		F32 cy= r->cam_pos.y;
		F32 cz= r->cam_pos.z;
		F32 near= 0.1;
		F32 far= 1.0;

		F32 fov= 3.141/2.0; // 45
    	F32 h= tan(fov/2)*near;
    	F32 w= h; // 1:1
 
		M44f p_matrix= {{
			near/w, 0, 0, 0,
			0, near/h, 0, 0,
			0, 0, (far + near)/(near - far), -1,
			0, 0, 2*far*near/(near - far), 0,
		}};
		M44f t_matrix= {{
			1, 0, 0, 0,
			0, 1, 0, 0,
			0, 0, 1, 0,
			-cx, -cy, -cz, 0,
		}};

		M44f cam_matrix= mul_m44f(t_matrix, p_matrix);

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glClearColor(0.0, 0.0, 0.0, 0.0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D_ARRAY, r->atlas_gl_id);

		ShaderSource* shd=
			(ShaderSource*)res_by_name(
					g_env.res_blob,
					ResType_ShaderSource,
					"gen");
		glUseProgram(shd->prog_gl_id);
		glUniform1i(glGetUniformLocation(shd->prog_gl_id, "u_tex_color"), 0);
		glUniformMatrix4fv(
				glGetUniformLocation(shd->prog_gl_id, "u_cam"),
				1,
				GL_FALSE,
				cam_matrix.e);

		glViewport(0, 0,
				g_env.device->win_size[0],
				g_env.device->win_size[1]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		draw_vao(&vao);

		// Debug draw
		if (r->ddraw_v_count > 0) {
			Vao ddraw_vao=
				create_vao(MeshType_tri, r->ddraw_v_count, r->ddraw_i_count);
			bind_vao(&ddraw_vao);
			add_vertices_to_vao(&ddraw_vao, r->ddraw_v, r->ddraw_v_count);
			add_indices_to_vao(&ddraw_vao, r->ddraw_i, r->ddraw_i_count);
			draw_vao(&ddraw_vao);
			destroy_vao(&ddraw_vao);

			r->ddraw_v_count= 0;
			r->ddraw_i_count= 0;
		}
	}


	destroy_vao(&vao);
}

void ddraw_poly(Renderer *r, Color c, V2d *poly, U32 count)
{
	if (r->ddraw_v_count + count > MAX_DEBUG_DRAW_VERTICES)
		critical_print("ddraw_poly: Too many vertices");
	if (r->ddraw_i_count + count > MAX_DEBUG_DRAW_INDICES)
		critical_print("ddraw_poly: Too many indices");

	Texture *white=
		(Texture*)res_by_name(
			g_env.res_blob,
			ResType_Texture,
			"white");
	V3f atlas_uv= white->atlas_uv;
	atlas_uv.x += 0.5*white->reso.x/TEXTURE_ATLAS_WIDTH;
	atlas_uv.y += 0.5*white->reso.y/TEXTURE_ATLAS_WIDTH;

	U32 start_index= r->ddraw_v_count;
	for (U32 i= 0; i < count; ++i) {
		TriMeshVertex v= {
			.pos= { .x= poly[i].x, .y= poly[i].y },
			.uv= atlas_uv,
			.color= c,
		};
		r->ddraw_v[r->ddraw_v_count++]= v;
	}

	for (U32 i= 0; i < count - 2; ++i) {
		r->ddraw_i[r->ddraw_i_count++]= start_index;
		r->ddraw_i[r->ddraw_i_count++]= start_index + i + 1;
		r->ddraw_i[r->ddraw_i_count++]= start_index + i + 2;
	}
}

void on_res_reload(Renderer *r, ResBlob *new_blob)
{
	recreate_texture_atlas(r, new_blob);

	for (U32 e_i= 0; e_i < r->entity_count; ++e_i) {
		ModelEntity *e= &r->entities[e_i];
		const Model *m=
			(Model*)res_by_name(
					new_blob,
					ResType_Model,
					e->model_name);
		set_modelentity(r, e_i, m);
	}
}
