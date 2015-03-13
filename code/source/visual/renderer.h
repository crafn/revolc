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
	F32 cam_fov; // Directly written

	ModelEntity m_entities[MAX_MODELENTITY_COUNT];
	ModelEntity m_entities_sort_space[MAX_MODELENTITY_COUNT];
	U32 next_m_entity;
	U32 m_entity_count;

	CompEntity c_entities[MAX_COMPENTITY_COUNT];
	U32 next_c_entity;
	U32 c_entity_count;

	TriMeshVertex ddraw_v[MAX_DEBUG_DRAW_VERTICES];
	MeshIndexType ddraw_i[MAX_DEBUG_DRAW_INDICES];
	U32 ddraw_v_count;
	U32 ddraw_i_count;

	Texel grid_ddraw_data[GRID_CELL_COUNT]; // Directly written

	U32 atlas_gl_id;
	Vao vao;
} Renderer;

// Sets g_env.renderer
REVOLC_API void create_renderer();
REVOLC_API void destroy_renderer();

REVOLC_API U32 resurrect_modelentity(const ModelEntity *dead);
REVOLC_API void free_modelentity(ModelEntity *m);
REVOLC_API void * storage_modelentity();

REVOLC_API U32 resurrect_compentity(const CompEntity *dead);
REVOLC_API void free_compentity(CompEntity *e);
REVOLC_API void * storage_compentity();

REVOLC_API void render_frame();

REVOLC_API void ddraw_poly(Color c, V2d *poly, U32 count);

REVOLC_API V2d screen_to_world_point(V2d p);

internal void renderer_on_res_reload();

#endif // REVOLC_VISUAL_RENDERER_H
