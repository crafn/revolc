#ifndef REVOLC_VISUAL_RENDERER_H
#define REVOLC_VISUAL_RENDERER_H

#include "build.h"
#include "entity_model.h"
#include "global/cfg.h"
#include "mesh.h"

typedef struct Renderer {
	ModelEntity entities[MAX_MODELENTITY_COUNT];
	U32 next_entity;
	U32 entity_count;

	TriMeshVertex ddraw_v[MAX_DEBUG_DRAW_VERTICES];
	MeshIndexType ddraw_i[MAX_DEBUG_DRAW_INDICES];
	U32 ddraw_v_count;
	U32 ddraw_i_count;

	U32 atlas_gl_id;
} Renderer;

/// @note Modifies g_env.renderer
REVOLC_API Renderer* create_renderer();
REVOLC_API void destroy_renderer(Renderer *r);

REVOLC_API U32 alloc_modelentity(Renderer *r);
REVOLC_API void free_modelentity(Renderer *r, U32 h);
REVOLC_API void set_modelentity(Renderer *r, U32 h, const Model *model);

REVOLC_API void render_frame(Renderer *r, F64 cam_x, F64 cam_y);

REVOLC_API void ddraw_poly(Renderer *r, Color c, V2d *poly, U32 count);

struct ResBlob;
REVOLC_API void on_res_reload(Renderer *r, struct ResBlob *new_blob);

#endif // REVOLC_VISUAL_RENDERER_H
