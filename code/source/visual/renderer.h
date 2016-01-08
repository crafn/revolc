#ifndef REVOLC_VISUAL_RENDERER_H
#define REVOLC_VISUAL_RENDERER_H

#include "build.h"
#include "compentity.h"
#include "modelentity.h"
#include "global/cfg.h"
#include "mesh.h"
#include "vao.h"

#define NULL_PATTERN 0

// Command for the (immediate-mode) renderer to draw a model this frame
typedef struct DrawCmd {
	T3d tf;
	S32 layer; // Provides order for entities at same z (e.g. gui). 0 for world stuff
	Color color, outline_color;
	V3f atlas_uv;
	F32 emission;
	bool has_alpha;
	V2f scale_to_atlas_uv;
	U32 mesh_v_count;
	U32 mesh_i_count;
	TriMeshVertex* vertices;
	MeshIndexType* indices;
} DrawCmd;

typedef struct Renderer {
	// These can be directly written outside rendering system
	V3d cam_pos;
	V2d cam_fov;
	F32 exposure;
	Color env_light_color;
	bool multisample; // Multisampled color buffer
	U32 msaa_samples;

	// Internals

	V3d prev_cam_pos;

	DrawCmd cmds[MAX_DRAW_CMD_COUNT];
	U32 cmd_count;

	// Not sure if these need to be here anymore, as renderer is now immediate-mode.
	// These could just be normal nodes and issue drawing commands.
	ModelEntity m_entities[MAX_MODELENTITY_COUNT];
	U32 next_m_entity;
	U32 m_entity_count; // Statistics

	CompEntity c_entities[MAX_COMPENTITY_COUNT];
	U32 next_c_entity;
	U32 c_entity_count; // Statistics

	// @todo Debug draw can be replaced with the new immediate-mode rendering
	TriMeshVertex ddraw_v[MAX_DEBUG_DRAW_VERTICES];
	MeshIndexType ddraw_i[MAX_DEBUG_DRAW_INDICES];
	U32 ddraw_v_count;
	U32 ddraw_i_count;

	// Directly written
	Texel grid_ddraw_data[GRID_CELL_COUNT];
	bool draw_grid;
	U32 grid_ddraw_tex;

	U8 occlusion_grid[GRID_CELL_COUNT];
	U32 occlusion_grid_tex;

	Texel fluid_grid[GRID_CELL_COUNT];
	U32 fluid_grid_tex;
	bool draw_fluid;

	U32 atlas_tex;
	Vao vao;


	// Rendering pipeline
	U32 scene_ms_fbo; // Used with multisampling
	U32 scene_color_ms_tex;
	U32 scene_depth_tex; // Used with scene_ms_fbo or scene_fbo
	U32 scene_fbo;
	U32 scene_color_tex;
	V2i scene_fbo_reso;
	U32 hl_fbo; // Highlights to be bloomed
	U32 hl_tex;
	V2i hl_fbo_reso;
	U32 blur_tmp_fbo;
	U32 blur_tmp_tex;
	V2i blur_tmp_fbo_reso;
	U32 occlusion_fbo; // Stuff from The Grid
	U32 occlusion_tex;
	V2i occlusion_fbo_reso;
} Renderer;

// Sets g_env.renderer
REVOLC_API void create_renderer();
REVOLC_API void destroy_renderer();

// Main interface to draw something
REVOLC_API void drawcmd(	T3d tf,
							TriMeshVertex *v, U32 v_count,
							MeshIndexType *i, U32 i_count,
							AtlasUv uv,
							Color c,
							Color outline_c,
							S32 layer,
							F32 emission,
							bool has_alpha);
REVOLC_API void drawcmd_model(	T3d tf,
								const Model *model,
								Color c,
								Color outline_c,
								S32 layer,
								F32 emission);

// Draws single-color quad
REVOLC_API void drawcmd_px_quad(V2i px_pos, V2i px_size, F32 rot, Color c, Color outline_c, S32 layer);

// Draws texture of a model
REVOLC_API void drawcmd_px_model_image(	V2i px_pos,
										V2i px_size, ModelEntity *src_model, S32 layer);

// Valid for only a frame (because camera can move)
REVOLC_API T3d px_tf(V2i px_pos, V2i px_size);

REVOLC_API U32 resurrect_modelentity(const ModelEntity *d);
REVOLC_API void overwrite_modelentity(ModelEntity *entity, const ModelEntity *dead);
REVOLC_API void free_modelentity(Handle h);
REVOLC_API void * storage_modelentity();

REVOLC_API ModelEntity * get_modelentity(U32 h);
REVOLC_API CompEntity * get_compentity(U32 h);

REVOLC_API U32 resurrect_compentity(const CompEntity *dead);
REVOLC_API void overwrite_compentity(CompEntity *entity, const CompEntity *dead);
REVOLC_API void free_compentity(Handle h);
REVOLC_API void * storage_compentity();

REVOLC_API void render_frame();

// Pixel coord (upper-left origin) -> world coord
REVOLC_API V2d screen_to_world_point(V2i p);
// @todo Rename to "delta", as Y is inverted
REVOLC_API V2d screen_to_world_size(V2i s);

REVOLC_API V2i world_to_screen_point(V2d p);

REVOLC_API U32 find_modelentity_at_pixel(V2i p);
REVOLC_API U32 find_compentity_at_pixel(V2i p);

REVOLC_API void renderer_on_res_reload();
REVOLC_API void recache_ptrs_to_meshes();
REVOLC_API void recache_ptrs_to_armatures();

#endif // REVOLC_VISUAL_RENDERER_H
