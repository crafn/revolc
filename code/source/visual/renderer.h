#ifndef REVOLC_VISUAL_RENDERER_H
#define REVOLC_VISUAL_RENDERER_H

#include "build.h"
#include "compentity.h"
#include "modelentity.h"
#include "global/cfg.h"
#include "mesh.h"
#include "vao.h"

typedef struct Renderer {
	V3d cam_pos; // Directly written
	V2d cam_fov;
	F32 exposure;

	ModelEntity m_entities[MAX_MODELENTITY_COUNT];
	ModelEntity m_entities_sort_space[MAX_MODELENTITY_COUNT];
	U32 next_m_entity;
	U32 m_entity_count; // Statistics

	CompEntity c_entities[MAX_COMPENTITY_COUNT];
	U32 next_c_entity;
	U32 c_entity_count; // Statistics

	TriMeshVertex ddraw_v[MAX_DEBUG_DRAW_VERTICES];
	MeshIndexType ddraw_i[MAX_DEBUG_DRAW_INDICES];
	U32 ddraw_v_count;
	U32 ddraw_i_count;

	// Directly written
	Texel grid_ddraw_data[GRID_CELL_COUNT];
	bool draw_grid;

	U32 atlas_gl_id;
	Vao vao;

	// Rendering pipeline
	U32 scene_fbo;
	U32 scene_fbo_tex;
	V2i scene_fbo_reso;
	U32 hl_fbo; // Highlights to be bloomed
	U32 hl_tex;
	V2i hl_fbo_reso;
	U32 blur_tmp_fbo;
	U32 blur_tmp_tex;
	V2i blur_tmp_fbo_reso;
} Renderer;

// Sets g_env.renderer
REVOLC_API void create_renderer();
REVOLC_API void destroy_renderer();

// @todo This should be The interface to renderer
//       So remove ModelEntities from renderer and have them call this
REVOLC_API void push_model(	T3d tf,
							TriMeshVertex *v, U32 v_count,
							MeshIndexType *i, U32 i_count,
							Color c,
							AtlasUv uv,
							S32 layer);

// Valid for only a frame (because camera can move)
REVOLC_API T3d px_tf(V2i px_pos, V2i px_size);

REVOLC_API U32 resurrect_modelentity(const ModelEntity *d);
REVOLC_API void free_modelentity(ModelEntity *m);
REVOLC_API void * storage_modelentity();

REVOLC_API ModelEntity * get_modelentity(U32 h);
REVOLC_API CompEntity * get_compentity(U32 h);

REVOLC_API U32 resurrect_compentity(const CompEntity *dead);
REVOLC_API void free_compentity(CompEntity *e);
REVOLC_API void * storage_compentity();

REVOLC_API void render_frame();

// Pixel coord (upper-left origin) -> world coord
REVOLC_API V2d screen_to_world_point(V2i p);
REVOLC_API V2d screen_to_world_size(V2i s);

REVOLC_API U32 find_modelentity_at_pixel(V2i p);
REVOLC_API U32 find_compentity_at_pixel(V2i p);

REVOLC_API void renderer_on_res_reload();
REVOLC_API void recache_ptrs_to_meshes();
REVOLC_API void recache_ptrs_to_armatures();

#endif // REVOLC_VISUAL_RENDERER_H
