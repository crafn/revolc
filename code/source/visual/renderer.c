#include "core/debug_print.h"
#include "core/ensure.h"
#include "core/malloc.h"
#include "core/matrix.h"
#include "core/vector.h"
#include "model.h"
#include "renderer.h"
#include "resources/resblob.h"

#include <stdlib.h>
#include <string.h>

internal
M44f view_matrix(const Renderer *r)
{
	F32 cx= r->cam_pos.x;
	F32 cy= r->cam_pos.y;
	F32 cz= r->cam_pos.z;
	M44f m= {{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		-cx, -cy, -cz, 1,
	}};
	return m;
}

// World point to screen
internal
M44f cam_matrix(const Renderer *r)
{
	F32 near= 0.1;
	F32 far= 1.0;
	F32 fov= r->cam_fov;
	F32 h= tan(fov/2)*near;
	/// @todo Aspect ratio
	F32 w= h; // 1:1

	M44f p_matrix= {{
		near/w, 0, 0, 0,
		0, near/h, 0, 0,
		0, 0, (far + near)/(near - far), -1,
		0, 0, 2*far*near/(near - far), 0,
	}};

	return mul_m44f(view_matrix(r), p_matrix);
}

/// Helper in `recreate_texture_atlas`
typedef struct TexInfo {
	Texture *tex;
	V2i reso;
	V3f *atlas_uv;
	Texel *texels;
} TexInfo;

internal
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

void create_renderer()
{
	Renderer *r= zero_malloc(sizeof(*r));

	r->cam_pos.y= 5.0;
	r->cam_pos.z= 10.0;
	r->cam_fov= 3.141/2.0;

	r->vao= create_vao(MeshType_tri, MAX_DRAW_VERTEX_COUNT, MAX_DRAW_INDEX_COUNT);

	recreate_texture_atlas(r, g_env.res_blob);

	ensure(!g_env.renderer);
	g_env.renderer= r;
}

void destroy_renderer()
{
	Renderer *r= g_env.renderer;
	g_env.renderer= NULL;

	destroy_vao(&r->vao);
	glDeleteTextures(1, &r->atlas_gl_id);
	free(r);
}

internal
U32 alloc_modelentity_noinit()
{
	Renderer *r= g_env.renderer;

	if (r->entity_count >= MAX_MODELENTITY_COUNT)
		fail("Too many modelentities");

	while (r->entities[r->next_entity].allocated)
		r->next_entity= (r->next_entity + 1) % MAX_MODELENTITY_COUNT;

	ModelEntity *e= &r->entities[r->next_entity];
	*e= (ModelEntity) { .allocated= true };

	++r->entity_count;
	return r->next_entity;
}

internal
void set_modelentity(U32 h, const Model *model)
{
	Renderer *r= g_env.renderer;

	ModelEntity *e= &r->entities[h];
	Texture *tex= model_texture(model, 0);
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

U32 resurrect_modelentity(const ModelEntity *dead)
{
	Renderer *r= g_env.renderer;
	U32 h= alloc_modelentity_noinit();
	r->entities[h]= *dead;
	r->entities[h].allocated= true;
	r->entities[h].vertices= NULL;
	r->entities[h].indices= NULL;
	r->entities[h].has_own_mesh= false;

	set_modelentity(
			h,
			(Model*)res_by_name(
				g_env.res_blob,
				ResType_Model,
				dead->model_name));
	return h;
}


void free_modelentity(U32 h)
{
	Renderer *r= g_env.renderer;

	ensure(h < MAX_MODELENTITY_COUNT);
	ModelEntity *e= &r->entities[h];
	if (e->has_own_mesh) {
		free(e->vertices);
		free(e->indices);
	}

	*e= (ModelEntity) { .allocated= false };
	--r->entity_count;
}

void * storage_modelentity()
{ return g_env.renderer->entities; }

internal
inline
int entity_cmp(const void *e1, const void *e2)
{
	// Smallest Z first (furthest away from camera)
	// Uglier but faster than using temp vars with -O0
	return	( ((ModelEntity*)e1)->pos.z > ((ModelEntity*)e2)->pos.z ) -
			( ((ModelEntity*)e1)->pos.z < ((ModelEntity*)e2)->pos.z );
}

void render_frame()
{
	Renderer *r= g_env.renderer;

	U32 total_v_count= 0;
	U32 total_i_count= 0;
	for (U32 i= 0; i < MAX_MODELENTITY_COUNT; ++i) {
		ModelEntity *e= &r->entities[i];
		if (!e->model_name[0])
			continue;

		total_v_count += e->mesh_v_count;
		total_i_count += e->mesh_i_count;
	}

	if (total_v_count > MAX_DRAW_VERTEX_COUNT)
		fail(	"Too many vertices to draw: %i > %i",
				total_i_count, MAX_DRAW_VERTEX_COUNT);

	if (total_i_count > MAX_DRAW_INDEX_COUNT) 
		fail(	"Too many indices to draw: %i > %i",
				total_i_count, MAX_DRAW_INDEX_COUNT);

	bind_vao(&r->vao);
	reset_vao_mesh(&r->vao);

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

				// Scale
				v.pos.x *= e->scale.x;
				v.pos.y *= e->scale.y;
				v.pos.z *= e->scale.z;

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
		add_vertices_to_vao(&r->vao, total_verts, total_v_count);
		add_indices_to_vao(&r->vao, total_inds, total_i_count);
		free(total_verts);
		free(total_inds);
	}

	{ // Actual rendering

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
				cam_matrix(r).e);

		glViewport(0, 0,
				g_env.device->win_size[0],
				g_env.device->win_size[1]);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		draw_vao(&r->vao);


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
}

void ddraw_poly(Color c, V2d *poly, U32 count)
{
	Renderer *r= g_env.renderer;

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

V2d screen_to_world_point(V2d p)
{
	Renderer *r= g_env.renderer;

	/// @todo Aspect ratio
	V2d view_p= {
		p.x*tan(r->cam_fov*0.5)*r->cam_pos.z,
		p.y*tan(r->cam_fov*0.5)*r->cam_pos.z,
	};

	M44f m= inverted_m44f(view_matrix(r));
	V2d result= {
		.x= view_p.x*m.e[0] + view_p.y*m.e[4] + m.e[12],
		.y= view_p.x*m.e[1] + view_p.y*m.e[5] + m.e[13],
	};
	return result;
}

void renderer_on_res_reload(ResBlob *new_blob)
{
	Renderer *r= g_env.renderer;

	recreate_texture_atlas(r, new_blob);

	for (U32 e_i= 0; e_i < MAX_MODELENTITY_COUNT; ++e_i) {
		ModelEntity *e= &r->entities[e_i];
		if (!e->allocated)
			continue;

		const Model *m=
			(Model*)res_by_name(
					new_blob,
					ResType_Model,
					e->model_name);
		set_modelentity(e_i, m);
	}
}
