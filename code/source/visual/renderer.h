#ifndef REVOLC_VISUAL_RENDERER_H
#define REVOLC_VISUAL_RENDERER_H

#include "build.h"
#include "entity_model.h"

typedef struct Renderer {
	ModelEntity *entities;
	U32 next_entity;
	U32 entity_count;
	U32 max_entity_count;
	U32 atlas_gl_id;
} Renderer;

/// @note Modifies g_env.renderer
REVOLC_API Renderer* create_renderer();
REVOLC_API void destroy_renderer(Renderer *r);

REVOLC_API U32 create_modelentity(Renderer *r, const Model *model);
REVOLC_API void destroy_modelentity(Renderer *r, U32 h);
REVOLC_API ModelEntity* get_modelentity(Renderer *rend, U32 h);

REVOLC_API void render_frame(Renderer *r, float cam_x, float cam_y);

REVOLC_API void recreate_texture_atlas(Renderer *r, ResBlob *blob);

#define ATLAS_WIDTH 2048

#endif // REVOLC_VISUAL_RENDERER_H
