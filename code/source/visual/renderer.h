#ifndef REVOLC_VISUAL_RENDERER_H
#define REVOLC_VISUAL_RENDERER_H

#include "build.h"
#include "modelentity.h"
#include "global/cfg.h"
#include "mesh.h"

typedef struct Renderer {
	V3d cam_pos;
	F32 cam_fov;

	ModelEntity entities[MAX_MODELENTITY_COUNT];
	U32 next_entity;
	U32 entity_count;

	TriMeshVertex ddraw_v[MAX_DEBUG_DRAW_VERTICES];
	MeshIndexType ddraw_i[MAX_DEBUG_DRAW_INDICES];
	U32 ddraw_v_count;
	U32 ddraw_i_count;

	U32 atlas_gl_id;

	ModelEntity sort_space[MAX_MODELENTITY_COUNT];
} Renderer;

// Sets g_env.renderer
REVOLC_API void create_renderer();
REVOLC_API void destroy_renderer();

REVOLC_API U32 resurrect_modelentity(const ModelEntity *dead);
REVOLC_API void free_modelentity(U32 h);
REVOLC_API void * storage_modelentity();

REVOLC_API void render_frame();

REVOLC_API void ddraw_poly(Color c, V2d *poly, U32 count);
REVOLC_API V2d screen_to_world_point(V2d p);

struct ResBlob;
internal void renderer_on_res_reload(struct ResBlob *new_blob);

#endif // REVOLC_VISUAL_RENDERER_H
