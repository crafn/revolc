#include "core/debug_print.h"
#include "core/ensure.h"
#include "core/malloc.h"
#include "core/matrix.h"
#include "core/vector.h"
#include "model.h"
#include "platform/stdlib.h"
#include "renderer.h"
#include "resources/resblob.h"

internal
V2f scale_to_atlas_uv(V2i reso)
{
	return (V2f) {
		(F32)reso.x/TEXTURE_ATLAS_WIDTH,
		(F32)reso.y/TEXTURE_ATLAS_WIDTH,
	};
}


// Separated to function, because this switch in a loop
// caused odd regression with -O0; fps was halved for some reason
// even though the code was run only once per frame
internal
void set_ventity_tf(U32 h, VEntityType t, T3d tf)
{
	Renderer *r= g_env.renderer;
	switch (t) {
		case VEntityType_model:
			r->m_entities[h].tf= tf;
		break;
		case VEntityType_comp:
			r->c_entities[h].tf= tf;
		break;
		default: fail("Unhandled VEntityType: %s", t);
	}
}

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
	F32 n= 0.1;
	F32 f= 1.0;
	V2d fov= r->cam_fov;
	F32 h= tan(fov.y/2)*n;
	F32 w= tan(fov.x/2)*n;

	M44f p_matrix= {{
		n/w, 0, 0, 0,
		0, n/h, 0, 0,
		0, 0, (f + n)/(n - f), -1,
		0, 0, 2*f*n/(n - f), 0,
	}};

	return mul_m44f(view_matrix(r), p_matrix);
}

/// Helper in `recreate_texture_atlas`
typedef struct TexInfo {
	const char *name;
	V2i reso;

	V3f *atlas_uv;
	V2f *scale_to_atlas_uv;

	Texel *texels;
	bool free_texels;
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
	U32 tex_info_count= 0;
	TexInfo *tex_infos= NULL;
	{
		U32 first_tex_res, tex_count;
		U32 first_font_res, font_count;
		all_res_by_type(&first_tex_res,
						&tex_count,
						blob,
						ResType_Texture);
		all_res_by_type(&first_font_res,
						&font_count,
						blob,
						ResType_Font);

		tex_info_count= tex_count + font_count;
		tex_infos=
			malloc(sizeof(*tex_infos)*(tex_info_count));
		U32 i= 0;
		for (	U32 cur_res= first_tex_res;
				cur_res < first_tex_res + tex_count;
				++cur_res) {
			Texture *tex=
				(Texture*)res_by_index(blob, cur_res);
			tex_infos[i++]= (TexInfo) {
				.name= tex->res.name,
				.reso= tex->reso,
				.atlas_uv= &tex->atlas_uv,
				.texels= tex->texels,
			};
		}
		for (	U32 cur_res= first_font_res;
				cur_res < first_font_res + font_count;
				++cur_res) {
			Font *font=
				(Font*)res_by_index(blob, cur_res);
			tex_infos[i++]= (TexInfo) {
				.name= font->res.name,
				.reso= font->bitmap_reso,
				.atlas_uv= &font->atlas_uv,
				.scale_to_atlas_uv= &font->scale_to_atlas_uv,
				.texels= malloc_rgba_font_bitmap(font),
			};
		}

		qsort(	tex_infos, tex_info_count,
				sizeof(*tex_infos), texinfo_cmp);
	}

	// Blit to atlas
	int x= 0, y= 0, z= 0;
	int last_row_height= 0;
	const int margin= 1;
	for (U32 i= 0; i < tex_info_count; ++i) {
		TexInfo *tex= &tex_infos[i];

		if (	tex->reso.x > TEXTURE_ATLAS_WIDTH ||
				tex->reso.y > TEXTURE_ATLAS_WIDTH)
			fail("Too large texture (max %i): %s",
					TEXTURE_ATLAS_WIDTH, tex->name);

		if (x + tex->reso.x > TEXTURE_ATLAS_WIDTH) {
			y += last_row_height + margin;
			x= 0;
			last_row_height= 0;
		}

		if (y + tex->reso.y > TEXTURE_ATLAS_WIDTH) {
			x= 0;
			y= 0;
			++z;
		}

		if (z > layers)
			fail("Texture atlas full!");

		glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0,
				x, y, z,
				tex->reso.x, tex->reso.y, 1,
				GL_RGBA, GL_UNSIGNED_BYTE, tex->texels);
		if (tex->free_texels) {
			free(tex->texels);
			tex->texels= NULL;
		}

		*tex->atlas_uv= (V3f) {
			(F32)x/TEXTURE_ATLAS_WIDTH,
			(F32)y/TEXTURE_ATLAS_WIDTH,
			z
		};
		if (tex->scale_to_atlas_uv) {
			*tex->scale_to_atlas_uv=
				scale_to_atlas_uv(tex->reso);
		}

		x += tex->reso.x + margin;
		if (tex->reso.y > last_row_height)
			last_row_height= tex->reso.y;
	}

	free(tex_infos);
	gl_check_errors("recreate_texture_atlas: end");
}

void create_renderer()
{
	Renderer *r= zero_malloc(sizeof(*r));

	r->cam_pos.y= 5.0;
	r->cam_pos.z= 10.0;
	r->cam_fov= (V2d) {3.141/2.0, 3.0141/2.0};

	r->vao= create_vao(MeshType_tri, MAX_DRAW_VERTEX_COUNT, MAX_DRAW_INDEX_COUNT);

	recreate_texture_atlas(r, g_env.resblob);

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
void recache_modelentity(ModelEntity *e)
{
	if (e->model_name[0] == '\0')
		return;

	const Model *model=
		(Model*)res_by_name(
					g_env.resblob,
					ResType_Model,
					e->model_name);
	Texture *tex= model_texture(model, 0);
	e->color= model->color;
	e->atlas_uv= tex->atlas_uv;
	e->scale_to_atlas_uv= scale_to_atlas_uv(tex->reso);
	e->vertices=
		(TriMeshVertex*)mesh_vertices(model_mesh(model));
	e->indices=
		(MeshIndexType*)mesh_indices(model_mesh(model));
	e->mesh_v_count= model_mesh(model)->v_count;
	e->mesh_i_count= model_mesh(model)->i_count;
}

U32 resurrect_modelentity(const ModelEntity *dead)
{
	Renderer *r= g_env.renderer;

	if (r->m_entity_count >= MAX_MODELENTITY_COUNT)
		fail("Too many ModelEntities");

	while (r->m_entities[r->next_m_entity].allocated)
		r->next_m_entity=
			(r->next_m_entity + 1) % MAX_MODELENTITY_COUNT;

	ModelEntity *e= &r->m_entities[r->next_m_entity];
	*e= (ModelEntity) { .allocated= true };

	++r->m_entity_count;
	const U32 h= r->next_m_entity;

	r->m_entities[h]= *dead;
	r->m_entities[h].allocated= true;
	r->m_entities[h].vertices= NULL;
	r->m_entities[h].indices= NULL;
	r->m_entities[h].has_own_mesh= false;

	recache_modelentity(&r->m_entities[h]);
	return h;
}

void free_modelentity(ModelEntity *e)
{
	Renderer *r= g_env.renderer;

	const U32 h= e - r->m_entities;
	ensure(h < MAX_MODELENTITY_COUNT);
	if (e->has_own_mesh) {
		free(e->vertices);
		free(e->indices);
	}

	*e= (ModelEntity) { .allocated= false };
	--r->m_entity_count;
}

void * storage_modelentity()
{ return g_env.renderer->m_entities; }


ModelEntity * get_modelentity(U32 h)
{ return &g_env.renderer->m_entities[h]; }

CompEntity * get_compentity(U32 h)
{ return &g_env.renderer->c_entities[h]; }

internal
void recache_compentity(CompEntity *e)
{
	for (U32 i= 0; i < e->sub_count; ++i)
		destroy_subentity(e->subs[i]);
	e->sub_count= 0;

	const CompDef *def=
		(CompDef*)res_by_name(	g_env.resblob,
								ResType_CompDef,
								e->def_name);
	e->armature= (Armature*)res_by_name(g_env.resblob,
										ResType_Armature,
										def->armature_name);

	for (U32 i= 0; i < def->sub_count; ++i) {
		e->subs[i]=
			create_subentity(e->armature, def->subs[i]);
		++e->sub_count;
	}
}

U32 resurrect_compentity(const CompEntity *dead)
{
	Renderer *r= g_env.renderer;

	if (r->c_entity_count >= MAX_COMPENTITY_COUNT)
		fail("Too many CompEntities");

	while (r->c_entities[r->next_c_entity].allocated)
		r->next_c_entity=
			(r->next_c_entity + 1) % MAX_COMPENTITY_COUNT;
	++r->c_entity_count;

	const U32 h= r->next_c_entity;
	CompEntity *e= &r->c_entities[h];
	*e= *dead;
	e->allocated= true;
	e->sub_count= 0;
	recache_compentity(e);
	return h;
}

void free_compentity(CompEntity *e)
{
	Renderer *r= g_env.renderer;

	const U32 h= e - r->c_entities;
	ensure(h < MAX_COMPENTITY_COUNT);
	for (U32 i= 0; i < e->sub_count; ++i)
		destroy_subentity(e->subs[i]);
	*e= (CompEntity) { .allocated= false };
	--r->c_entity_count;
}

void * storage_compentity()
{ return g_env.renderer->c_entities; }

internal
inline
int entity_cmp(const void *e1, const void *e2)
{
	// Smallest Z first (furthest away from camera)
	// Uglier but faster than using temp vars with -O0
	return	(	((ModelEntity*)e1)->tf.pos.z >
				((ModelEntity*)e2)->tf.pos.z )
			-
			( ((ModelEntity*)e1)->tf.pos.z <
				((ModelEntity*)e2)->tf.pos.z );
}

void render_frame()
{
	Renderer *r= g_env.renderer;

	// Update CompEntities
	for (U32 e_i= 0; e_i < MAX_COMPENTITY_COUNT; ++e_i) {
		CompEntity *e= &r->c_entities[e_i];
		if (!e->allocated)
			continue;
		ensure(e->armature);

		T3d global_pose[MAX_ARMATURE_JOINT_COUNT];
		calc_global_pose(global_pose, e);

		// Position subentities by global_pose
		for (U32 s_i= 0; s_i < e->sub_count; ++s_i) {
			const SubEntity *sub= &e->subs[s_i];
			T3d tf= mul_t3d(global_pose[sub->joint_id],
							t3f_to_t3d(sub->offset));
			set_ventity_tf(sub->handle, sub->type, tf);
		}
	}

	// Calculate total vertex and index count for frame
	U32 total_v_count= 0;
	U32 total_i_count= 0;
	for (U32 i= 0; i < MAX_MODELENTITY_COUNT; ++i) {
		ModelEntity *e= &r->m_entities[i];
		if (!e->allocated)
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
		memcpy(	r->m_entities_sort_space,
				r->m_entities,
				sizeof(*r->m_entities)*MAX_MODELENTITY_COUNT);

		// Z-sort
		qsort(	r->m_entities_sort_space, MAX_MODELENTITY_COUNT,
				sizeof(*r->m_entities_sort_space), entity_cmp);
		ModelEntity *entities= r->m_entities_sort_space;

		TriMeshVertex *total_verts= malloc(sizeof(*total_verts)*total_v_count);
		MeshIndexType *total_inds= malloc(sizeof(*total_inds)*total_i_count);
		U32 cur_v= 0;
		U32 cur_i= 0;
		for (U32 i= 0; i < MAX_MODELENTITY_COUNT; ++i) {
			ModelEntity *e= &entities[i];
			if (!e->allocated)
				continue;

			for (U32 k= 0; k < e->mesh_i_count; ++k) {
				total_inds[cur_i]= e->indices[k] + cur_v;
				++cur_i;
			}

			for (U32 k= 0; k < e->mesh_v_count; ++k) {
				TriMeshVertex v= e->vertices[k];
				V3d p= {v.pos.x, v.pos.y, v.pos.z};
				p= mul_v3d(e->tf.scale, p);
				p= rot_v3d(e->tf.rot, p);
				p= add_v3d(e->tf.pos, p);

				v.pos= (V3f) {p.x, p.y, p.z};

				v.uv.x *= e->scale_to_atlas_uv.x;
				v.uv.y *= e->scale_to_atlas_uv.y;

				v.uv.x += e->atlas_uv.x;
				v.uv.y += e->atlas_uv.y;
				v.uv.z += e->atlas_uv.z;

				v.color= e->color;

				total_verts[cur_v]= v;
				++cur_v;
			}
		}
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
					g_env.resblob,
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
				g_env.device->win_size.x,
				g_env.device->win_size.y);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		draw_vao(&r->vao);


		// Debug draw
		if (r->ddraw_v_count > 0) {
			// Shapes
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

		if (r->draw_grid) {
			U32 grid_id;
			glGenTextures(1, &grid_id);
			glBindTexture(GL_TEXTURE_2D, grid_id);

			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
				GRID_WIDTH_IN_CELLS, GRID_WIDTH_IN_CELLS,
				0, GL_RGBA, GL_UNSIGNED_BYTE,
				r->grid_ddraw_data);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

			Vao grid_vao= create_vao(MeshType_tri, 4, 6);
			bind_vao(&grid_vao);
			add_vertices_to_vao(&grid_vao, (TriMeshVertex[]) {
				{ .pos= {-GRID_WIDTH/2, -GRID_WIDTH/2}, .uv= {0, 0} },
				{ .pos= {+GRID_WIDTH/2, -GRID_WIDTH/2}, .uv= {1, 0} },
				{ .pos= {+GRID_WIDTH/2, +GRID_WIDTH/2}, .uv= {1, 1} },
				{ .pos= {-GRID_WIDTH/2, +GRID_WIDTH/2}, .uv= {0, 1} },
			}, 4);
			add_indices_to_vao(&grid_vao, (MeshIndexType[]) {
				0, 1, 2,
				0, 2, 3,
			}, 6);

			ShaderSource* grid_shd=
				(ShaderSource*)res_by_name(
						g_env.resblob,
						ResType_ShaderSource,
						"grid_ddraw");
			glUseProgram(grid_shd->prog_gl_id);
			glUniform1i(glGetUniformLocation(grid_shd->prog_gl_id, "u_tex_color"), 0);
			glUniformMatrix4fv(
					glGetUniformLocation(grid_shd->prog_gl_id, "u_cam"),
					1,
					GL_FALSE,
					cam_matrix(r).e);

			draw_vao(&grid_vao);
			destroy_vao(&grid_vao);

			glDeleteTextures(1, &grid_id);
		}
	}

	for (U32 i= 0; i < MAX_MODELENTITY_COUNT; ++i) {
		ModelEntity *e= &r->m_entities[i];
		if (e->free_after_draw)
			free_modelentity(e);
	}
}

V2d screen_to_world_point(V2i p)
{
	Renderer *r= g_env.renderer;

	V2d gl_p= {
		2.0*p.x/g_env.device->win_size.x - 1.0,
		-2.0*p.y/g_env.device->win_size.y + 1.0
	};

	V2d view_p= {
		gl_p.x*tan(r->cam_fov.x*0.5)*r->cam_pos.z,
		gl_p.y*tan(r->cam_fov.y*0.5)*r->cam_pos.z,
	};

	M44f m= inverted_m44f(view_matrix(r));
	V2d result= {
		.x= view_p.x*m.e[0] + view_p.y*m.e[4] + m.e[12],
		.y= view_p.x*m.e[1] + view_p.y*m.e[5] + m.e[13],
	};
	return result;
}

V2d screen_to_world_size(V2i s)
{
	V2d a= screen_to_world_point((V2i){0, 0});
	V2d b= screen_to_world_point(s);
	return sub_v2d(b, a);
}

U32 find_modelentity_at_pixel(V2i p)
{
	V3d wp= v2d_to_v3d(screen_to_world_point(p));

	/// @todo Bounds
	F64 closest_dist= 0;
	U32 closest_h= NULL_HANDLE;
	for (U32 i= 0; i < MAX_MODELENTITY_COUNT; ++i) {
		if (!g_env.renderer->m_entities[i].allocated)
			continue;

		V3d entity_pos=
			g_env.renderer->m_entities[i].tf.pos;
		F64 dist= dist_sqr_v3d(entity_pos, wp);
		if (	closest_h == NULL_HANDLE ||
				dist < closest_dist) {
			closest_h= i;
			closest_dist= dist;
		}
	}
	return closest_h;
}

U32 find_compentity_at_pixel(V2i p)
{
	V3d wp= v2d_to_v3d(screen_to_world_point(p));

	/// @todo Bounds
	F64 closest_dist= 0;
	U32 closest_h= NULL_HANDLE;
	for (U32 i= 0; i < MAX_COMPENTITY_COUNT; ++i) {
		if (!g_env.renderer->c_entities[i].allocated)
			continue;

		V3d entity_pos= g_env.renderer->c_entities[i].tf.pos;
		F64 dist= dist_sqr_v3d(entity_pos, wp);
		if (	closest_h == NULL_HANDLE ||
				dist < closest_dist) {
			closest_h= i;
			closest_dist= dist;
		}
	}
	return closest_h;
}

internal
void recache_modelentities()
{
	Renderer *r= g_env.renderer;
	for (U32 e_i= 0; e_i < MAX_MODELENTITY_COUNT; ++e_i) {
		ModelEntity *e= &r->m_entities[e_i];
		if (!e->allocated)
			continue;
		if (e->has_own_mesh)
			continue;

		recache_modelentity(e);
	}
}

void renderer_on_res_reload()
{
	Renderer *r= g_env.renderer;

	recreate_texture_atlas(r, g_env.resblob);
	recache_modelentities();

	for (U32 e_i= 0; e_i < MAX_COMPENTITY_COUNT; ++e_i) {
		CompEntity *e= &r->c_entities[e_i];
		if (!e->allocated)
			continue;

		recache_compentity(e);
	}
}

void recache_ptrs_to_meshes()
{
	recache_modelentities();
}

void recache_ptrs_to_armatures()
{
	// Can't just use recache_compentity as it does too much:
	// invalidates subentity handles

	Renderer *r= g_env.renderer;
	for (U32 e_i= 0; e_i < MAX_COMPENTITY_COUNT; ++e_i) {
		CompEntity *e= &r->c_entities[e_i];
		if (!e->allocated)
			continue;

		const CompDef *def=
			(CompDef*)res_by_name(	g_env.resblob,
									ResType_CompDef,
									e->def_name);
		e->armature=
			(Armature*)res_by_name(	g_env.resblob,
									ResType_Armature,
									def->armature_name);
	}
}
