#include "core/debug_print.h"
#include "core/ensure.h"
#include "core/memory.h"
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

internal
void draw_screen_quad()
{
	// @todo Don't recreate vao
	const Mesh *quad= (Mesh*)res_by_name(g_env.resblob, ResType_Mesh, "unitquad");
	Vao quad_vao= create_vao(MeshType_tri, 4, 6);
	bind_vao(&quad_vao);
	add_mesh_to_vao(&quad_vao, quad);
	draw_vao(&quad_vao);
	destroy_vao(&quad_vao);
}

internal
void draw_grid_quad()
{
	// @todo Don't recreate vao
	const Color white= {1, 1, 1, 1};
	const F32 rad= GRID_WIDTH/2.0;
	Vao grid_vao= create_vao(MeshType_tri, 4, 6);
	bind_vao(&grid_vao);
	add_vertices_to_vao(&grid_vao, (TriMeshVertex[]) {
		{ .pos= {-rad, -rad}, .uv= {0, 0}, .color= white, },
		{ .pos= {+rad, -rad}, .uv= {1, 0}, .color= white, },
		{ .pos= {+rad, +rad}, .uv= {1, 1}, .color= white, },
		{ .pos= {-rad, +rad}, .uv= {0, 1}, .color= white },
	}, 4);
	add_indices_to_vao(&grid_vao, (MeshIndexType[]) {
		0, 1, 2,
		0, 2, 3,
	}, 6);
	draw_vao(&grid_vao);
	destroy_vao(&grid_vao);
}

internal
GLint uniform_loc(U32 shd, const char *name)
{
	GLint loc= glGetUniformLocation(shd, name);
	if (loc == -1) {
		critical_print("Uniform missing: %s", name);
	}
	return loc;
}

internal
U32 gen_tex(GLenum mag_filter, GLenum min_filter)
{
	U32 tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	return tex;
}

// Apply blur to fbo
internal
void blur_fbo(Renderer *r, V2f screenspace_radius, U32 fbo, U32 fbo_tex, V2i reso)
{
	glDisable(GL_BLEND);

	{ // Vertical blur to tmp fbo
		glBindFramebuffer(GL_FRAMEBUFFER, r->blur_tmp_fbo);
		glViewport(0, 0, r->blur_tmp_fbo_reso.x, r->blur_tmp_fbo_reso.y);

		ShaderSource* shd=
			(ShaderSource*)res_by_name(g_env.resblob, ResType_ShaderSource, "blur_v");
		glUseProgram(shd->prog_gl_id);

		// @todo To shader res
		glBindFragDataLocation(shd->prog_gl_id, 0, "f_color");

		glUniform1f(uniform_loc(shd->prog_gl_id, "u_radius"), screenspace_radius.x);

		glUniform1i(uniform_loc(shd->prog_gl_id, "u_tex"), 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, fbo_tex);

		draw_screen_quad();
	}
	{ // Horizontal blur back to hl fbo
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, reso.x, reso.y);

		ShaderSource* shd=
			(ShaderSource*)res_by_name(g_env.resblob, ResType_ShaderSource, "blur_h");
		glUseProgram(shd->prog_gl_id);

		// @todo To shader res
		glBindFragDataLocation(shd->prog_gl_id, 0, "f_color");

		glUniform1f(uniform_loc(shd->prog_gl_id, "u_radius"), screenspace_radius.y);

		glUniform1i(uniform_loc(shd->prog_gl_id, "u_tex"), 0);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, r->blur_tmp_tex);

		draw_screen_quad();
	}
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
M44f view_matrix(V3d cam_pos)
{
	F32 cx= cam_pos.x;
	F32 cy= cam_pos.y;
	F32 cz= cam_pos.z;
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
M44f cam_matrix(V3d cam_pos, V2d cam_fov)
{
	F32 n= 0.1;
	F32 f= 1.0;
	V2d fov= cam_fov;
	F32 h= tan(fov.y/2)*n;
	F32 w= tan(fov.x/2)*n;

	M44f p_matrix= {{
		n/w, 0, 0, 0,
		0, n/h, 0, 0,
		0, 0, (f + n)/(n - f), -1,
		0, 0, 2*f*n/(n - f), 0,
	}};

	return mul_m44f(view_matrix(cam_pos), p_matrix);
}

/// Helper in `recreate_gl_textures`
typedef struct TexInfo {
	const char *name;
	V2i reso;

	AtlasUv *atlas_uv;

	Texel *texels[MAX_TEXTURE_LOD_COUNT];
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
void recreate_gl_textures(Renderer *r, ResBlob *blob)
{
	gl_check_errors("recreate_gl_textures: begin");
	{ // Dither patterns
		if (r->brush_tex)
			glDeleteTextures(1, &r->brush_tex);

		Texture *tex= (Texture*)res_by_name(g_env.resblob,
											ResType_Texture,
											"brush");
		gl_check_errors("recreate_gl_textures brush: begin");
		glGenTextures(1, &r->brush_tex);
		ensure(r->brush_tex);
		glBindTexture(GL_TEXTURE_2D, r->brush_tex);
		// @note No sRGB because shaders depend on exact numerical values
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
				tex->reso.x, tex->reso.y, 0,
				GL_RGBA, GL_UNSIGNED_BYTE,
				texture_texels(tex, 0));

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 1);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 0);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, 0);

		gl_check_errors("recreate_gl_textures brush: end");
	}

	if (r->atlas_tex)
		glDeleteTextures(1, &r->atlas_tex);

	glGenTextures(1, &r->atlas_tex);
	ensure(r->atlas_tex);
	glBindTexture(GL_TEXTURE_2D_ARRAY, r->atlas_tex);

	const int atlas_lod_count= MAX_TEXTURE_LOD_COUNT;
	const int layers= TEXTURE_ATLAS_LAYER_COUNT;

	glTexStorage3D(GL_TEXTURE_2D_ARRAY, atlas_lod_count, GL_SRGB8_ALPHA8,
			TEXTURE_ATLAS_WIDTH, TEXTURE_ATLAS_WIDTH, layers);

	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAX_LEVEL, atlas_lod_count);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Can't be mipmap
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LOD, 1000);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_LOD, -1000);
	gl_check_errors("recreate_gl_textures: atlas alloc");

	// Gather TexInfos
	/// @todo MissingResource
	U32 tex_info_count= 0;
	TexInfo *tex_infos= NULL;
	{
		U32 tex_count;
		U32 font_count;
		Texture **textures=
			(Texture **)all_res_by_type(	&tex_count,
											blob,
											ResType_Texture);
		Font **fonts=
			(Font **)all_res_by_type(	&font_count,
										blob,
										ResType_Font);

		tex_info_count= tex_count + font_count;
		tex_infos=
			malloc(sizeof(*tex_infos)*(tex_info_count));
		U32 tex_info_count= 0;
		for (U32 tex_i= 0; tex_i < tex_count; ++tex_i) {
			Texture *tex= textures[tex_i];
			tex_infos[tex_info_count]= (TexInfo) {
				.name= tex->res.name,
				.reso= tex->reso,
				.atlas_uv= &tex->atlas_uv,
			};
			for (U32 lod_i= 0; lod_i < tex->lod_count; ++lod_i)
				tex_infos[tex_info_count].texels[lod_i]= texture_texels(tex, lod_i);
			++tex_info_count;
		}
		for (U32 font_i= 0; font_i < font_count; ++font_i) {
			Font *font= fonts[font_i];
			tex_infos[tex_info_count++]= (TexInfo) {
				.name= font->res.name,
				.reso= font->bitmap_reso,
				.atlas_uv= &font->atlas_uv,
				.texels= {malloc_rgba_font_bitmap(font)},
				.free_texels= true,
			};
		}

		qsort(	tex_infos, tex_info_count,
				sizeof(*tex_infos), texinfo_cmp);
	}

	// Blit to atlas
	int x= 0, y= 0, z= 0;
	int last_row_height= 0;
	const int margin= 1*atlas_lod_count;
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

		// Submit texture data
		for (int lod_i= 0; lod_i < atlas_lod_count; ++lod_i) {
			if (!tex->texels[lod_i])
				continue;
			const V2i reso= lod_reso(tex->reso, lod_i);
			const V2i pos= lod_reso((V2i) {x, y}, lod_i); // @todo x, y should be properly aligned
			glTexSubImage3D(GL_TEXTURE_2D_ARRAY, lod_i,
					pos.x, pos.y, z,
					reso.x, reso.y, 1,
					GL_RGBA, GL_UNSIGNED_BYTE, tex->texels[lod_i]);
		}


		if (tex->free_texels) {
			for (U32 i= 0; i < MAX_TEXTURE_LOD_COUNT; ++i) {
				free(tex->texels[i]);
				tex->texels[i]= NULL;
			}
		}

		*tex->atlas_uv= (AtlasUv) {
			.uv= {
				(F32)x/TEXTURE_ATLAS_WIDTH,
				(F32)y/TEXTURE_ATLAS_WIDTH,
				z,
			},
			.scale= scale_to_atlas_uv(tex->reso),
		};

		x += tex->reso.x + margin;
		if (tex->reso.y > last_row_height)
			last_row_height= tex->reso.y;
	}

	free(tex_infos);
	gl_check_errors("recreate_gl_textures: end");
}

internal
void destroy_rendering_pipeline(Renderer *r)
{
	if (!r->scene_fbo)
		return;

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDeleteFramebuffers(1, &r->scene_fbo);
	glDeleteTextures(1, &r->scene_color_tex);
	glDeleteTextures(1, &r->scene_detail_tex);
	glDeleteTextures(1, &r->scene_move_tex);
	glDeleteTextures(1, &r->scene_src_tex);
	glDeleteTextures(1, &r->scene_dst_tex);

	glDeleteFramebuffers(1, &r->paint_fbo);
	glDeleteTextures(1, &r->paint_fbo_tex);

	glDeleteFramebuffers(1, &r->hl_fbo);
	glDeleteTextures(1, &r->hl_tex);

	glDeleteFramebuffers(1, &r->blur_tmp_fbo);
	glDeleteTextures(1, &r->blur_tmp_tex);

	glDeleteFramebuffers(1, &r->occlusion_fbo);
	glDeleteTextures(1, &r->occlusion_tex);
}

internal
bool rendering_pipeline_obsolete(Renderer *r)
{
	return !equals_v2i(r->scene_fbo_reso, g_env.device->win_size);
}

internal
void recreate_rendering_pipeline(Renderer *r)
{
	gl_check_errors("recreate_rendering_pipeline: begin");
	destroy_rendering_pipeline(r);

	r->scene_fbo_reso= g_env.device->win_size;
	r->paint_fbo_reso= g_env.device->win_size;
	r->hl_fbo_reso= (V2i) {512, 512};
	r->blur_tmp_fbo_reso= r->hl_fbo_reso;
	r->occlusion_fbo_reso= (V2i) {128, 128};

	{ // Setup framebuffers

		// Fbo & tex to store HDR render of the scene and "requested brush detail" -map
		glGenFramebuffers(1, &r->scene_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, r->scene_fbo);

		r->scene_color_tex= gen_tex(GL_LINEAR, GL_LINEAR);
		glTexImage2D(	GL_TEXTURE_2D, 0, GL_RGB16F,
						r->scene_fbo_reso.x, r->scene_fbo_reso.y,
						0, GL_RGB, GL_FLOAT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, r->scene_color_tex, 0);
		r->scene_detail_tex= gen_tex(GL_NEAREST, GL_NEAREST);
		glTexImage2D(	GL_TEXTURE_2D, 0, GL_RED,
						r->scene_fbo_reso.x, r->scene_fbo_reso.y,
						0, GL_RED, GL_FLOAT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, r->scene_detail_tex, 0);
		r->scene_move_tex= gen_tex(GL_NEAREST, GL_NEAREST);
		glTexImage2D(	GL_TEXTURE_2D, 0, GL_RG16F,
						r->scene_fbo_reso.x, r->scene_fbo_reso.y,
						0, GL_RGB, GL_FLOAT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, r->scene_move_tex, 0);
		r->scene_src_tex= gen_tex(GL_NEAREST, GL_NEAREST);
		glTexImage2D(	GL_TEXTURE_2D, 0, GL_R16UI,
						r->scene_fbo_reso.x, r->scene_fbo_reso.y,
						0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, r->scene_src_tex, 0);
		r->scene_dst_tex= gen_tex(GL_NEAREST, GL_NEAREST);
		glTexImage2D(	GL_TEXTURE_2D, 0, GL_R16UI,
						r->scene_fbo_reso.x, r->scene_fbo_reso.y,
						0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, NULL);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, r->scene_dst_tex, 0);
		{
			GLenum ret= glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (ret != GL_FRAMEBUFFER_COMPLETE)
				fail("Incomplete framebuffer (scene): %i", ret);
		}
		gl_check_errors("recreate_rendering_pipeline: scene fbo");

		// Fbo & tex to store HDR painting of the scene (could try without HDR if slow)
		r->paint_fbo_tex= gen_tex(GL_LINEAR, GL_LINEAR);
		glTexImage2D(	GL_TEXTURE_2D, 0, GL_RGB16F,
						r->paint_fbo_reso.x, r->paint_fbo_reso.y,
						0, GL_RGB, GL_FLOAT, NULL);
		glGenFramebuffers(1, &r->paint_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, r->paint_fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, r->paint_fbo_tex, 0);
		{
			GLenum ret= glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (ret != GL_FRAMEBUFFER_COMPLETE)
				fail("Incomplete framebuffer (paint): %i", ret);
		}

		// Fbo & tex to store highlights of the scene
		r->hl_tex= gen_tex(GL_LINEAR, GL_LINEAR);
		glTexImage2D(	GL_TEXTURE_2D, 0, GL_RGB16F,
						r->hl_fbo_reso.x, r->hl_fbo_reso.y,
						0, GL_RGB, GL_FLOAT, NULL);
		glGenFramebuffers(1, &r->hl_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, r->hl_fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, r->hl_tex, 0);
		{
			GLenum ret= glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (ret != GL_FRAMEBUFFER_COMPLETE)
				fail("Incomplete framebuffer (hl): %i", ret);
		}

		// Fbo & tex to store halfway blurred highlight
		r->blur_tmp_tex= gen_tex(GL_LINEAR, GL_LINEAR);
		glTexImage2D(	GL_TEXTURE_2D, 0, GL_RGB16F,
						r->blur_tmp_fbo_reso.x, r->blur_tmp_fbo_reso.y,
						0, GL_RGB, GL_FLOAT, NULL);
		glGenFramebuffers(1, &r->blur_tmp_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, r->blur_tmp_fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, r->blur_tmp_tex, 0);
		{
			GLenum ret= glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (ret != GL_FRAMEBUFFER_COMPLETE)
				fail("Incomplete framebuffer (blur): %i", ret);
		}

		// Fbo & tex to store occlusion map (shadow)
		r->occlusion_tex= gen_tex(GL_LINEAR, GL_LINEAR);
		glTexImage2D(	GL_TEXTURE_2D, 0, GL_RED,
						r->occlusion_fbo_reso.x, r->occlusion_fbo_reso.y,
						0, GL_RED, GL_UNSIGNED_BYTE, NULL);
		glGenFramebuffers(1, &r->occlusion_fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, r->occlusion_fbo);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, r->occlusion_tex, 0);
		{
			GLenum ret= glCheckFramebufferStatus(GL_FRAMEBUFFER);
			if (ret != GL_FRAMEBUFFER_COMPLETE)
				fail("Incomplete framebuffer (occlusion): %i", ret);
		}
	}

	gl_check_errors("recreate_rendering_pipeline: end");
}

void create_renderer()
{
	gl_check_errors("create_renderer: begin");
	Renderer *r= ZERO_ALLOC(gen_ator(), sizeof(*r), "renderer");

	r->cam_pos.y= 5.0;
	r->cam_pos.z= 7.0;
	r->prev_cam_pos= r->cam_pos;
	r->cam_fov= (V2d) {3.141/2.0, 3.0141/2.0};
	r->env_light_color= (Color) {1, 1, 1, 1};

	r->vao= create_vao(MeshType_tri, MAX_DRAW_VERTEX_COUNT, MAX_DRAW_INDEX_COUNT);

	{ // Create brush vao
		struct Layer {
			V2i brush_count;
			F32 brush_size;
		} layers[] = {
			{{2, 2}, 5.12},
			{{4, 4}, 2.56},
			{{7, 7}, 1.28},
			{{13, 13}, 0.64},
			{{25, 25}, 0.32},
			{{50, 50}, 0.16},
			{{100, 100}, 0.08},
			{{200, 200}, 0.04},
			{{400, 400}, 0.02},
			{{800, 800}, 0.01},
		};
		const int layer_count= ARRAY_COUNT(layers);

		int total_brush_count= 0;
		for (int i= 0; i < layer_count; ++i)
			total_brush_count += layers[i].brush_count.x*layers[i].brush_count.y;
		BrushMeshVertex *brushes= frame_alloc(sizeof(*brushes)*total_brush_count);

		int brush_i= 0;
		U64 seed= 42;
		for (int i= 0; i < layer_count; ++i) {
			struct Layer *layer= &layers[i];
			V2i c= layer->brush_count;
			for (int y= 0; y < c.y; ++y)
			for (int x= 0; x < c.x; ++x) {
				brushes[brush_i++]= (BrushMeshVertex) {
					.pos= {
						(2.0f*x/c.x - 1.0f)*1.1f,
						(2.0f*y/c.y - 1.0f)*1.1f,
					},
					.size= layer->brush_size*random_f32(0.7f, 1.3f, &seed),
				};
			}
		}

		r->brush_vaos[0]= create_vao(MeshType_brush, total_brush_count, 0);
		r->brush_vaos[1]= create_vao(MeshType_brush, total_brush_count, 0);
		r->src_brush_vao= &r->brush_vaos[0];
		r->dst_brush_vao= &r->brush_vaos[1];

		bind_vao(r->src_brush_vao);
		add_vertices_to_vao(r->src_brush_vao, brushes, total_brush_count);

		r->dst_brush_vao->v_count= r->dst_brush_vao->v_capacity; // Filled by transform feedback

		gl_check_errors("create_renderer: brush vao");
	}

	recreate_rendering_pipeline(r);
	recreate_gl_textures(r, g_env.resblob);

	{
		glGenTextures(1, &r->grid_ddraw_tex);
		glBindTexture(GL_TEXTURE_2D, r->grid_ddraw_tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	{
		glGenTextures(1, &r->occlusion_grid_tex);
		glBindTexture(GL_TEXTURE_2D, r->occlusion_grid_tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}

	{
		glGenTextures(1, &r->fluid_grid_tex);
		glBindTexture(GL_TEXTURE_2D, r->fluid_grid_tex);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	}

	ensure(!g_env.renderer);
	g_env.renderer= r;
	gl_check_errors("create_renderer: end");
}

void destroy_renderer()
{
	Renderer *r= g_env.renderer;
	g_env.renderer= NULL;

	destroy_vao(&r->vao);
	destroy_vao(&r->brush_vaos[0]);
	destroy_vao(&r->brush_vaos[1]);

	destroy_rendering_pipeline(r);
	glDeleteTextures(1, &r->brush_tex);
	glDeleteTextures(1, &r->atlas_tex);
	glDeleteTextures(1, &r->fluid_grid_tex);
	glDeleteTextures(1, &r->occlusion_grid_tex);
	glDeleteTextures(1, &r->grid_ddraw_tex);

	FREE(gen_ator(), r);
}

void drawcmd(	T3d tf,
				TriMeshVertex *v, U32 v_count,
				MeshIndexType *i, U32 i_count,
				AtlasUv uv,
				Color c,
				S32 layer,
				F32 emission,
				U8 pattern)
{
	DrawCmd cmd= {
		.tf= tf,
		.layer= layer,
		.color= c,
		.atlas_uv= uv.uv,
		.scale_to_atlas_uv= uv.scale,
		.mesh_v_count= v_count,
		.mesh_i_count= i_count,
		.vertices= v,
		.indices= i,
		.emission= emission,
		.pattern= pattern,
	};
	Renderer *r= g_env.renderer;
	if (r->cmd_count >= MAX_DRAW_CMD_COUNT) {
		fail("Too many draw commands");
	}
	r->cmds[r->cmd_count++]= cmd;
}

void drawcmd_model(	T3d tf,
					const Model *model,
					Color c,
					S32 layer,
					F32 emission)
{
	const Texture *tex= model_texture(model, 0);
	const Mesh *mesh= model_mesh(model);
	drawcmd(tf,
			mesh_vertices(mesh), mesh->v_count,
			mesh_indices(mesh), mesh->i_count,
			tex->atlas_uv,
			mul_color(model->color, c),
			layer,
			model->emission + emission,
			model->pattern);
}

T3d px_tf(V2i px_pos, V2i px_size)
{
	T3d tf= identity_t3d();
	tf.pos= v2d_to_v3d(screen_to_world_point(px_pos)); 
	tf.scale= v2d_to_v3d(screen_to_world_size(px_size));
	return tf;
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
	e->emission= model->emission;
	e->pattern= model->pattern;
	e->atlas_uv= tex->atlas_uv.uv;
	e->scale_to_atlas_uv= tex->atlas_uv.scale;
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
int drawcmd_z_cmp(const void *e1, const void *e2)
{
#define E1 ((DrawCmd*)e1)
#define E2 ((DrawCmd*)e2)
	if (E1->layer != E2->layer)
		return (E1->layer > E2->layer) - (E1->layer < E2->layer);

	// Smallest Z first (furthest away from camera)
	// Uglier but faster than using temp vars with -O0
	return	(E1->tf.pos.z > E2->tf.pos.z) - (E1->tf.pos.z < E2->tf.pos.z);
#undef E1
#undef E2
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

	for (U32 i= 0; i < MAX_MODELENTITY_COUNT; ++i) {
		ModelEntity *e= &r->m_entities[i];
		if (!e->allocated)
			continue;
		drawcmd(e->tf,
				e->vertices, e->mesh_v_count,
				e->indices, e->mesh_i_count,
				(AtlasUv) {e->atlas_uv, e->scale_to_atlas_uv},
				e->color,
				e->layer,
				e->emission,
				e->pattern);
	}

	// Calculate total vertex and index count for frame
	U32 total_v_count= 0;
	U32 total_i_count= 0;
	for (U32 i= 0; i < r->cmd_count; ++i) {
		DrawCmd *cmd= &r->cmds[i];
		total_v_count += cmd->mesh_v_count;
		total_i_count += cmd->mesh_i_count;
	}

	if (total_v_count > MAX_DRAW_VERTEX_COUNT)
		fail(	"Too many vertices to draw: %i > %i",
				total_i_count, MAX_DRAW_VERTEX_COUNT);

	if (total_i_count > MAX_DRAW_INDEX_COUNT) 
		fail(	"Too many indices to draw: %i > %i",
				total_i_count, MAX_DRAW_INDEX_COUNT);

	bind_vao(&r->vao);
	reset_vao_mesh(&r->vao);

	{ // Issue drawing commands (= meshes to Vao)
		// Z-sort
		qsort(r->cmds, r->cmd_count, sizeof(*r->cmds), drawcmd_z_cmp);

		TriMeshVertex *total_verts= frame_alloc(sizeof(*total_verts)*total_v_count);
		MeshIndexType *total_inds= frame_alloc(sizeof(*total_inds)*total_i_count);
		U32 cur_v= 0;
		U32 cur_i= 0;
		for (U32 i= 0; i < r->cmd_count; ++i) {
			DrawCmd *cmd= &r->cmds[i];

			for (U32 k= 0; k < cmd->mesh_i_count; ++k) {
				total_inds[cur_i]= cmd->indices[k] + cur_v;
				++cur_i;
			}

			for (U32 k= 0; k < cmd->mesh_v_count; ++k) {
				TriMeshVertex v= cmd->vertices[k];
				V3d p= {v.pos.x, v.pos.y, v.pos.z};
				p= mul_v3d(cmd->tf.scale, p);
				p= rot_v3d(cmd->tf.rot, p);
				p= add_v3d(cmd->tf.pos, p);

				v.pos= (V3f) {p.x, p.y, p.z};

				v.uv.x *= cmd->scale_to_atlas_uv.x;
				v.uv.y *= cmd->scale_to_atlas_uv.y;

				v.uv.x += cmd->atlas_uv.x;
				v.uv.y += cmd->atlas_uv.y;
				v.uv.z += cmd->atlas_uv.z;

				v.color= cmd->color;
				v.emission= cmd->emission;

				v.draw_id= i;

				total_verts[cur_v]= v;
				++cur_v;
			}
		}
		add_vertices_to_vao(&r->vao, total_verts, total_v_count);
		add_indices_to_vao(&r->vao, total_inds, total_i_count);

		r->cmd_count= 0; // Clear commands
	}

	{ // Actual rendering
		if (rendering_pipeline_obsolete(r))
			recreate_rendering_pipeline(r);

		V2i reso= g_env.device->win_size;
		V2d scrn_in_world= screen_to_world_size(reso);
		scrn_in_world.x= ABS(scrn_in_world.x);
		scrn_in_world.y= ABS(scrn_in_world.y);

		// Controls how much further outside the screen shadows are calculated (= blurred)
		const F32 occlusion_safe_dist= 10.0;
		const V2f occlusion_scale= {1.0/(1.0 + occlusion_safe_dist/scrn_in_world.x),
									1.0/(1.0 + occlusion_safe_dist/scrn_in_world.y)};
		{ // Render occlusion grid to fbo
			glDisable(GL_BLEND);

			glBindTexture(GL_TEXTURE_2D, r->occlusion_grid_tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
				GRID_WIDTH_IN_CELLS, GRID_WIDTH_IN_CELLS,
				0, GL_RED, GL_UNSIGNED_BYTE,
				r->occlusion_grid);

			// @todo Not sure if drawing right after glTexImage stalls

			glBindFramebuffer(GL_FRAMEBUFFER, r->occlusion_fbo);
			glViewport(0, 0, r->occlusion_fbo_reso.x, r->occlusion_fbo_reso.y);

			ShaderSource* shd=
				(ShaderSource*)res_by_name( g_env.resblob,
											ResType_ShaderSource,
											"grid_blit");
			glUseProgram(shd->prog_gl_id);

			glUniform1i(uniform_loc(shd->prog_gl_id, "u_tex_color"), 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, r->occlusion_grid_tex);

			glUniform2f(uniform_loc(shd->prog_gl_id, "u_screenspace_scale"),
				occlusion_scale.x, occlusion_scale.y);
			glUniformMatrix4fv( uniform_loc(shd->prog_gl_id, "u_cam"),
								1, GL_FALSE, cam_matrix(r->cam_pos, r->cam_fov).e);
			draw_grid_quad();

			// Reset this as no other needs the uniform
			glUniform2f(uniform_loc(shd->prog_gl_id, "u_screenspace_scale"), 1.0, 1.0);

			for (U32 blur_i= 0; blur_i < 2; ++blur_i) { // Blur shadows
				F32 rad= (5.0 + blur_i*3)*15;
				// @todo Maybe we should take aspect ratio into account
				V2f rad_scrn= {rad/scrn_in_world.x, rad/scrn_in_world.y};
				blur_fbo(r, rad_scrn, r->occlusion_fbo, r->occlusion_tex, r->occlusion_fbo_reso);
			}
		}

		{ // Render scene to fbo (color + detail + move)
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glClearColor(0.0, 0.0, 0.0, 0.0);

			glBindFramebuffer(GL_FRAMEBUFFER, r->scene_fbo);
			glViewport(0, 0, r->scene_fbo_reso.x, r->scene_fbo_reso.y);
			glClear(GL_COLOR_BUFFER_BIT);

			ShaderSource* shd=
				(ShaderSource*)res_by_name(
						g_env.resblob,
						ResType_ShaderSource,
						"gen");
			glUseProgram(shd->prog_gl_id);

			glUniform1f(uniform_loc(shd->prog_gl_id, "u_exposure"), r->exposure);
			glUniform3f(uniform_loc(shd->prog_gl_id, "u_env_light_color"),
						r->env_light_color.r,
						r->env_light_color.g,
						r->env_light_color.b);
			glUniformMatrix4fv(	uniform_loc(shd->prog_gl_id, "u_cam"),
								1, GL_FALSE, cam_matrix(r->cam_pos, r->cam_fov).e);
			glUniformMatrix4fv(	uniform_loc(shd->prog_gl_id, "u_prev_cam"),
								1, GL_FALSE, cam_matrix(r->prev_cam_pos, r->cam_fov).e);

			glUniform1i(uniform_loc(shd->prog_gl_id, "u_tex_color"), 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D_ARRAY, r->atlas_tex);

			GLenum buffers[]= {
				GL_COLOR_ATTACHMENT0,
				GL_COLOR_ATTACHMENT1,
				GL_COLOR_ATTACHMENT2,
				GL_COLOR_ATTACHMENT3,
				GL_COLOR_ATTACHMENT4,
			};
			// Do buffers need to be disabled afterwards?
			glDrawBuffers(ARRAY_COUNT(buffers), buffers);
			glBindFragDataLocation(shd->prog_gl_id, 0, "f_color"); // to scene_color_tex
			glBindFragDataLocation(shd->prog_gl_id, 1, "f_detail"); // to scene_detail_tex
			glBindFragDataLocation(shd->prog_gl_id, 2, "f_move"); // to scene_move_tex
			glBindFragDataLocation(shd->prog_gl_id, 3, "f_src"); // to scene_src_tex
			glBindFragDataLocation(shd->prog_gl_id, 4, "f_dst"); // to scene_dst_tex

			bind_vao(&r->vao);
			draw_vao(&r->vao);

			if (r->draw_fluid) {
				// Proto fluid proto render
				ShaderSource* grid_shd=
					(ShaderSource*)res_by_name(
							g_env.resblob,
							ResType_ShaderSource,
							"grid_blit");
				glUseProgram(grid_shd->prog_gl_id);
				glUniform1i(uniform_loc(grid_shd->prog_gl_id, "u_tex_color"), 0);
				glUniformMatrix4fv(
						uniform_loc(grid_shd->prog_gl_id, "u_cam"),
						1,
						GL_FALSE,
						cam_matrix(r->cam_pos, r->cam_fov).e);

				glBindTexture(GL_TEXTURE_2D, r->fluid_grid_tex);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
					GRID_WIDTH_IN_CELLS, GRID_WIDTH_IN_CELLS,
					0, GL_RGBA, GL_UNSIGNED_BYTE,
					r->fluid_grid);
				draw_grid_quad();
			}
		}

		if (r->brush_rendering)
		{ // Move brushes according to scene
			ShaderSource* shd=
				(ShaderSource*)res_by_name(g_env.resblob, ResType_ShaderSource, "upd_brushes");
			glUseProgram(shd->prog_gl_id);

			glUniform1i(uniform_loc(shd->prog_gl_id, "u_scene_move"), 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, r->scene_move_tex);

			glUniform1i(uniform_loc(shd->prog_gl_id, "u_scene_src"), 1);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, r->scene_src_tex);

			glUniform1i(uniform_loc(shd->prog_gl_id, "u_scene_dst"), 2);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, r->scene_dst_tex);

			glEnable(GL_RASTERIZER_DISCARD);
			glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, r->dst_brush_vao->vbo_id);
		 
			glBeginTransformFeedback(GL_POINTS);
			bind_vao(r->src_brush_vao);
			draw_vao(r->src_brush_vao);
			glEndTransformFeedback();

			glDisable(GL_RASTERIZER_DISCARD);

			SWAP(Vao*, r->src_brush_vao, r->dst_brush_vao);
		}

		if (r->brush_rendering)
		{ // Paintify rendered scene
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

			glBindFramebuffer(GL_FRAMEBUFFER, r->paint_fbo);
			glViewport(0, 0, r->paint_fbo_reso.x, r->paint_fbo_reso.y);
			// @todo Remove (just to clean up persistent pixel mess on linux)
			glClear(GL_COLOR_BUFFER_BIT);

			ShaderSource* shd=
				(ShaderSource*)res_by_name(g_env.resblob, ResType_ShaderSource, "paint");
			glUseProgram(shd->prog_gl_id);

			glUniform1i(uniform_loc(shd->prog_gl_id, "u_scene_color"), 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, r->scene_color_tex);

			glUniform1i(uniform_loc(shd->prog_gl_id, "u_scene_detail"), 1);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, r->scene_detail_tex);

			glUniform1i(uniform_loc(shd->prog_gl_id, "u_brush"), 2);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, r->brush_tex);


			bind_vao(r->src_brush_vao);
			draw_vao(r->src_brush_vao);
		}

		{ // Overexposed parts to small "highlight" texture
			glDisable(GL_BLEND);

			glBindFramebuffer(GL_FRAMEBUFFER, r->hl_fbo);
			glViewport(0, 0, r->hl_fbo_reso.x, r->hl_fbo_reso.y);

			ShaderSource* shd=
				(ShaderSource*)res_by_name(g_env.resblob, ResType_ShaderSource, "highlight");
			glUseProgram(shd->prog_gl_id);

			// Should we use scene_color or paint?
			glUniform1i(uniform_loc(shd->prog_gl_id, "u_scene_color"), 0);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, r->scene_color_tex);

			draw_screen_quad();
		}

		for (U32 blur_i= 0; blur_i < 3; ++blur_i) { // Blur highlights (bloom)
			// @todo Aspect ratio
			F32 rad= 0.1 + blur_i*0.2;
			blur_fbo(r, (V2f) {rad, rad}, r->hl_fbo, r->hl_tex, r->hl_fbo_reso);
		}

		{ // Post process and show scene
			glDisable(GL_BLEND);

			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			glViewport(0, 0, reso.x, reso.y);
			ShaderSource* shd=
				(ShaderSource*)res_by_name(g_env.resblob, ResType_ShaderSource, "post");
			glUseProgram(shd->prog_gl_id);

			if (r->brush_rendering) {
				glUniform1i(uniform_loc(shd->prog_gl_id, "u_color"), 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, r->paint_fbo_tex);
			} else {
				glUniform1i(uniform_loc(shd->prog_gl_id, "u_color"), 0);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, r->scene_color_tex);
			}

			glUniform1i(uniform_loc(shd->prog_gl_id, "u_highlight"), 1);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, r->hl_tex);

			glUniform1i(uniform_loc(shd->prog_gl_id, "u_occlusion"), 2);
			glActiveTexture(GL_TEXTURE2);
			glBindTexture(GL_TEXTURE_2D, r->occlusion_tex);

			glUniform2f(uniform_loc(shd->prog_gl_id, "u_occlusion_scale"),
				occlusion_scale.x, occlusion_scale.y);

			draw_screen_quad();
		}

		// Debug draw
		{
			glEnable(GL_BLEND);
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			ShaderSource* shd=
				(ShaderSource*)res_by_name(g_env.resblob, ResType_ShaderSource, "gen");
			glUseProgram(shd->prog_gl_id);

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
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, r->grid_ddraw_tex);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
					GRID_WIDTH_IN_CELLS, GRID_WIDTH_IN_CELLS,
					0, GL_RGBA, GL_UNSIGNED_BYTE,
					r->grid_ddraw_data);

				ShaderSource* grid_shd=
					(ShaderSource*)res_by_name(
							g_env.resblob,
							ResType_ShaderSource,
							"grid_blit");
				glUseProgram(grid_shd->prog_gl_id);
				glUniform1i(uniform_loc(grid_shd->prog_gl_id, "u_tex_color"), 0);
				glUniformMatrix4fv(
						uniform_loc(grid_shd->prog_gl_id, "u_cam"),
						1,
						GL_FALSE,
						cam_matrix(r->cam_pos, r->cam_fov).e);
				draw_grid_quad();
			}
		}
	}

	r->prev_cam_pos= r->cam_pos;
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

	M44f m= inverted_m44f(view_matrix(r->cam_pos));
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

	recreate_gl_textures(r, g_env.resblob);
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
