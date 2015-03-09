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

		x += tex->reso.x + 1; // Prevent bleeding due to floating point errors
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
	const Model *model=
		(Model*)res_by_name(
					g_env.resblob,
					ResType_Model,
					e->model_name);
	Texture *tex= model_texture(model, 0);
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

	if (r->m_entity_count >= MAX_MODELENTITY_COUNT)
		fail("Too many ModelEntities");

	while (r->m_entities[r->next_m_entity].allocated)
		r->next_m_entity= (r->next_m_entity + 1) % MAX_MODELENTITY_COUNT;

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

void free_modelentity(U32 h)
{
	Renderer *r= g_env.renderer;

	ensure(h < MAX_MODELENTITY_COUNT);
	ModelEntity *e= &r->m_entities[h];
	if (e->has_own_mesh) {
		free(e->vertices);
		free(e->indices);
	}

	*e= (ModelEntity) { .allocated= false };
	--r->m_entity_count;
}

void * storage_modelentity()
{ return g_env.renderer->m_entities; }

internal
void recache_compentity(CompEntity *e)
{
	for (U32 i= 0; i < e->sub_count; ++i)
		destroy_subentity(e->subs[i]);
	e->sub_count= 0;

	const CompDef *def= (CompDef*)res_by_name(	g_env.resblob,
												ResType_CompDef,
												e->def_name);
	e->armature= (Armature*)res_by_name(g_env.resblob,
										ResType_Armature,
										def->armature_name);

	for (U32 i= 0; i < def->sub_count; ++i) {
		e->subs[i]= create_subentity(e->armature, def->subs[i]);
		++e->sub_count;
	}
}

U32 resurrect_compentity(const CompEntity *dead)
{
	Renderer *r= g_env.renderer;

	if (r->c_entity_count >= MAX_COMPENTITY_COUNT)
		fail("Too many CompEntities");

	while (r->c_entities[r->next_c_entity].allocated)
		r->next_c_entity= (r->next_c_entity + 1) % MAX_COMPENTITY_COUNT;
	++r->c_entity_count;

	const U32 h= r->next_c_entity;
	CompEntity *e= &r->c_entities[h];
	*e= *dead;
	e->allocated= true;
	e->sub_count= 0;
	recache_compentity(e);
	return h;
}

void free_compentity(U32 h)
{
	Renderer *r= g_env.renderer;

	ensure(h < MAX_COMPENTITY_COUNT);
	CompEntity *e= &r->c_entities[h];
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
	return	( ((ModelEntity*)e1)->tf.pos.z > ((ModelEntity*)e2)->tf.pos.z ) -
			( ((ModelEntity*)e1)->tf.pos.z < ((ModelEntity*)e2)->tf.pos.z );
}

void render_frame()
{
	Renderer *r= g_env.renderer;

	{ // Update CompEntities
		for (U32 e_i= 0; e_i < MAX_COMPENTITY_COUNT; ++e_i) {
			CompEntity *e= &r->c_entities[e_i];
			if (!e->allocated)
				continue;
			ensure(e->armature);

			// Calculate global armature pose
			T3d global_pose[MAX_ARMATURE_JOINT_COUNT];
			const Joint *joints= e->armature->joints;
			for (U32 j_i= 0; j_i < e->armature->joint_count; ++j_i) {
				T3f joint_pose=
					mul_t3f(e->joint_offsets.tf[j_i],
							joints[j_i].bind_pose);

				JointId super_id= joints[j_i].super_id;
				if (super_id != NULL_JOINT_ID) {
					joint_pose=
						mul_t3f(joints[super_id].bind_pose, joint_pose);
				}

				global_pose[j_i]= mul_t3d(e->tf, t3f_to_t3d(joint_pose));
			}

			// Position subentities by global_pose
			for (U32 s_i= 0; s_i < e->sub_count; ++s_i) {
				const SubEntity *sub= &e->subs[s_i];
				T3d tf= mul_t3d(global_pose[sub->joint_id],
								t3f_to_t3d(sub->offset));
				set_ventity_tf(sub->handle, sub->type, tf);
			}
		}
	}

	U32 total_v_count= 0;
	U32 total_i_count= 0;
	for (U32 i= 0; i < MAX_MODELENTITY_COUNT; ++i) {
		ModelEntity *e= &r->m_entities[i];
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
			if (!e->model_name[0])
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

				v.color= (Color) {1, 1, 1, 1};

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
				g_env.device->win_size[0],
				g_env.device->win_size[1]);
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

			// Grid
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
			g_env.resblob,
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

void renderer_on_res_reload()
{
	Renderer *r= g_env.renderer;

	recreate_texture_atlas(r, g_env.resblob);

	for (U32 e_i= 0; e_i < MAX_MODELENTITY_COUNT; ++e_i) {
		ModelEntity *e= &r->m_entities[e_i];
		if (!e->allocated)
			continue;
		if (e->has_own_mesh)
			continue;

		recache_modelentity(e);
	}

	for (U32 e_i= 0; e_i < MAX_COMPENTITY_COUNT; ++e_i) {
		CompEntity *e= &r->c_entities[e_i];
		if (!e->allocated)
			continue;

		recache_compentity(e);
	}
}
