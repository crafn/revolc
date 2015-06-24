#include "animation/armature.h"
#include "animation/joint.h"
#include "build.h"
#include "core/debug_print.h"
#include "core/ensure.h"
#include "core/file.h"
#include "core/random.h"
#include "core/vector.h"
#include "editor/editor.h"
#include "game/aitest.h"
#include "game/world.h"
#include "game/worldgen.h"
#include "global/env.h"
#include "physics/physworld.h"
#include "platform/device.h"
#include "platform/time.h"
#include "resources/resblob.h"
#include "ui/uicontext.h"
#include "visual/model.h"
#include "visual/renderer.h"

internal
void make_main_blob(const char *blob_path, const char *game)
{
	const char *engine_res_root= frame_str("%srevolc/", DEFAULT_RES_ROOT);
	const char *game_res_root= frame_str("%s%s/", DEFAULT_RES_ROOT, game);

	// @todo Fix crappy api!
	char **game_res_paths= plat_find_paths_with_end(game_res_root, ".res");
	char **engine_res_paths= plat_find_paths_with_end(engine_res_root, ".res");

	U32 res_count= 0;
	char *res_paths[MAX_RES_FILES]= {0};
	for (U32 i= 0; res_count < MAX_RES_FILES && engine_res_paths[i]; ++i)
		res_paths[res_count++]= engine_res_paths[i];
	for (U32 i= 0; res_count < MAX_RES_FILES && game_res_paths[i]; ++i)
		res_paths[res_count++]= game_res_paths[i];

	make_blob(blob_path, res_paths);
	for (U32 i= 0; res_paths[i]; ++i)
		free(res_paths[i]);
	free(engine_res_paths);
	free(game_res_paths);
}

const char *blob_path(const char *game)
{ return frame_str("%s.blob", game); }

internal
void spawn_entity(World *world, ResBlob *blob, V2d pos)
{
	local_persist U64 group_i= 0;
	group_i= (group_i + 1) % 3;

	const char* prop_name=
		(char*[]) {"wbarrel", "wbox", "rollbot"}[group_i];

	T3d tf= {{1, 1, 1}, identity_qd(), {pos.x, pos.y, 0}};
	SlotVal init_vals[]= { // Override default values from json
		{"body",	"tf",			WITH_DEREF_SIZEOF(&tf)},
		{"body",	"def_name",		WITH_STR_SIZE(prop_name)},
		{"visual",	"model_name",	WITH_STR_SIZE(prop_name)},
	};
	NodeGroupDef *def=
		(NodeGroupDef*)res_by_name(blob, ResType_NodeGroupDef, "phys_prop");
	create_nodes(world, def, WITH_ARRAY_COUNT(init_vals), group_i);
}

#define SAVEFILE_PATH "save.bin"

int main(int argc, const char **argv)
{
	const char *game= NULL;
	if (argc <= 1) {
		fail("Not enough arguments: specify name of the game");
	} else {
		game= argv[1];
	}

	init_env();

	Device *d= plat_init(frame_str("Revolc engine - %s", game), (V2i) {1024, 768});

	if (!file_exists(blob_path(game)))
		make_main_blob(blob_path(game), game);
	load_blob(&g_env.resblob, blob_path(game));
	print_blob(g_env.resblob);

	create_audiosystem();
	create_renderer();
	create_physworld();
	create_uicontext();
	create_editor();

	World *world= g_env.world= create_world();

	if (file_exists(SAVEFILE_PATH)) {
		load_world(world, SAVEFILE_PATH);
	} else {
		U32 count;
		Module **modules= (Module**)all_res_by_type(&count,
													g_env.resblob,
													ResType_Module);
		for (U32 i= 0; i < count; ++i) {
			if (modules[i]->worldgen)
				modules[i]->worldgen(world);
		}
	}

	F64 time_accum= 0.0; // For fps
	U32 frame= 0;
	while (1) {
		reset_frame_alloc();

		plat_update(d);
		upd_uicontext();
		time_accum += d->dt;
		if (frame++ == 60 && 0) {
			debug_print("---");
			debug_print("model entities: %i", g_env.renderer->m_entity_count);
			debug_print("comp entities: %i", g_env.renderer->c_entity_count);
			debug_print("bodies: %i", g_env.physworld->body_count);
			debug_print("nodes: %i", g_env.world->node_count);
			debug_print("fps: %f", frame/time_accum);
			frame= 0;
			time_accum= 0;
		}

		{ // User input
			V2d cursor_on_world= screen_to_world_point(g_env.device->cursor_pos);
			V2d prev_cursor_on_world= screen_to_world_point(g_env.uicontext->dev.prev_cursor_pos);
			V2d cursor_delta_on_world= sub_v2d(cursor_on_world, prev_cursor_on_world);

			F32 dt= d->dt;
			F32 spd= 25.0;
			if (d->key_down[KEY_UP])
				g_env.renderer->cam_pos.y += spd*dt;
			if (d->key_down[KEY_LEFT])
				g_env.renderer->cam_pos.x -= spd*dt;
			if (d->key_down[KEY_DOWN])
				g_env.renderer->cam_pos.y -= spd*dt;
			if (d->key_down[KEY_RIGHT])
				g_env.renderer->cam_pos.x += spd*dt;

			if (d->key_down[KEY_MMB]) {
				g_env.renderer->cam_pos.x -= cursor_delta_on_world.x;
				g_env.renderer->cam_pos.y -= cursor_delta_on_world.y;
			}

			if (d->key_down['y'])
				g_env.renderer->cam_pos.z -= spd*dt;
			if (d->key_down['h'])
				g_env.renderer->cam_pos.z += spd*dt;

			if (d->key_down['c'])
				g_env.renderer->exposure -= dt*2.0;
			if (d->key_down['v'])
				g_env.renderer->exposure += dt*2.0;
			if (d->key_pressed['m'])
				toggle_bool(&g_env.renderer->brush_rendering);

			g_env.renderer->cam_pos.z -= g_env.device->mwheel_delta;
			g_env.renderer->cam_pos.z = MIN(g_env.renderer->cam_pos.z, 30);

			{ // Fov which cuts stuff away with non-square window
				V2i win_size= g_env.device->win_size;
				F64 fov_scale= MAX(win_size.x, win_size.y);
				g_env.renderer->cam_fov= (V2d) {
					2*atan(g_env.device->win_size.x/fov_scale),
					2*atan(g_env.device->win_size.y/fov_scale)
				};
			}

			if (d->key_down['e'])
				spawn_entity(world, g_env.resblob, cursor_on_world);

#			ifdef USE_FLUID
			if (d->key_down['r']) {
				GridCell *grid= g_env.physworld->grid;
				U32 i= GRID_INDEX_W(cursor_on_world.x, cursor_on_world.y);
				U32 width= 20;
				if (d->key_down[KEY_LSHIFT])
					width= 1;
				if (!d->key_down[KEY_LCTRL]) {
					for (U32 x= 0; x < width; ++x) {
					for (U32 y= 0; y < width; ++y) {
						grid[i + x + y*GRID_WIDTH_IN_CELLS].water= 1;
					}
					}
				} else {
					width= 50;
					for (U32 x= 0; x < width; ++x) {
					for (U32 y= 0; y < width; ++y) {
						if (rand() % 300 == 0)
							grid[i + x + y*GRID_WIDTH_IN_CELLS].water= 1;
					}
					}
				}
			}
#			endif

			if (d->key_down['t'])
				set_grid_material_in_circle(cursor_on_world, 2.0, GRIDCELL_MATERIAL_AIR);
			if (d->key_down['g'])
				set_grid_material_in_circle(cursor_on_world, 1.0, GRIDCELL_MATERIAL_GROUND);

			if (d->key_pressed['q'])
				g_env.physworld->debug_draw= !g_env.physworld->debug_draw;

			if (d->key_pressed['k'])
				play_sound("dev_beep0", 1.0, -1.0);
			if (d->key_pressed['l'])
				play_sound("dev_beep1", 0.5, 1.0);

			if (d->key_pressed[KEY_F5]) {
				U32 count= mirror_blob_modifications(g_env.resblob);
				if (count > 0)
					delete_file(blob_path(game)); // Force make_blob at restart
			}

			if (d->key_pressed[KEY_F9]) {
				system("cd ../../code && clbs debug mod");
				make_main_blob(blob_path(game), game);

				if (!d->key_down[KEY_LSHIFT] && blob_has_modifications(g_env.resblob))
					critical_print("Current resblob has unsaved modifications -- not reloading");
				else
					reload_blob(&g_env.resblob, g_env.resblob, blob_path(game));
			}

			if (d->key_pressed[KEY_F12]) {
				U32 count;
				Module **modules= (Module**)all_res_by_type(&count,
															g_env.resblob,
															ResType_Module);
				for (U32 i= 0; i < count; ++i)
					system(frame_str("cd ../../code && clbs debug %s", count[modules]->res.name));

				if (!file_exists(blob_path(game)))
					make_main_blob(blob_path(game), game);

				/// @todo Reload only modules
				if (!d->key_down[KEY_LSHIFT] && blob_has_modifications(g_env.resblob))
					critical_print("Current resblob has unsaved modifications -- not reloading");
				else
					reload_blob(&g_env.resblob, g_env.resblob, blob_path(game));
			}

			if (d->key_pressed['u']) {
				if (!d->key_down[KEY_LSHIFT] && blob_has_modifications(g_env.resblob))
					critical_print("Current resblob has unsaved modifications -- press shift + u to quit without saving");
				else
					break;
			}

			if (d->key_pressed['p'])
				save_world(world, SAVEFILE_PATH);
			if (d->key_pressed['o']) {
				destroy_world(world);
				world= g_env.world= create_world();
				load_world(world, SAVEFILE_PATH);
			}

			if (g_env.editor->state == EditorState_invisible) {
				local_persist cpBody *body= NULL;
				if (d->key_down['f']) {
					cpVect p= {cursor_on_world.x, cursor_on_world.y};
					cpShape *shape=
						cpSpacePointQueryNearest(
								g_env.physworld->cp_space,
								p, 0.1,
								CP_SHAPE_FILTER_ALL, NULL);

					if (!body && shape && body != g_env.physworld->cp_ground_body) {
						body= cpShapeGetBody(shape);
					}

					if (body) {
						cpBodySetPosition(body, p);
						cpBodySetVelocity(body, cpv(0, 0));
					}
				} else if (body) {
					body= NULL;
				}
			}
		}

		upd_editor();

		F64 game_dt= d->dt;
		if (g_env.editor->state != EditorState_invisible)
			game_dt= 0.0;
		upd_physworld(game_dt);
		upd_world(world, game_dt);
		post_upd_physworld();
		upd_phys_rendering();

		{ // Test ground drawing

			const Model *model= (Model*)res_by_name(g_env.resblob, ResType_Model, "dirt");
			const Mesh *mesh= model_mesh(model);

			V2i px_ll= {0, g_env.device->win_size.y};
			V2i px_tr= {g_env.device->win_size.x, 0};
			V3d w_ll= px_tf(px_ll, (V2i) {0}).pos;
			V3d w_tr= px_tf(px_tr, (V2i) {0}).pos;
			V2i ll= GRID_VEC_W(w_ll.x, w_ll.y);
			V2i tr= GRID_VEC_W(w_tr.x, w_tr.y);

			int draw_count= 0;
			for (int y= ll.y - 3; y < tr.y + 1; ++y) {
			for (int x= ll.x - 3; x < tr.x + 1; ++x) {
				if (x < 0 || y < 0 || x >= GRID_WIDTH_IN_CELLS || y >= GRID_WIDTH_IN_CELLS)
					continue;
				if (g_env.physworld->grid[GRID_INDEX(x, y)].material == GRIDCELL_MATERIAL_AIR)
					continue;

				U64 seed= x + y*10000;
				float z= random_f32(-0.1, 0.05, &seed);
				float scale= 1.3*random_f32(1.5, 1.8, &seed);
				float rot= random_f32(-0.7, 0.7, &seed);
				float brightness= random_f32(0.7, 1.0, &seed);
				float x_dif= random_f32(-0.07, 0.07, &seed);
				float y_dif= random_f32(-0.07, 0.07, &seed);
				/*
				float z= 0;
				float scale= 1;
				float rot= 0;
				float brightness= 1;
				float x_dif= 0;
				float y_dif= 0;
				*/

				V3d size= {scale/GRID_RESO_PER_UNIT, scale/GRID_RESO_PER_UNIT, 1.0};
				V3d pos= {
					(x + 0.5)/GRID_RESO_PER_UNIT - GRID_WIDTH/2 + x_dif,
					(y + 0.5)/GRID_RESO_PER_UNIT - GRID_WIDTH/2 + y_dif,
					z
				};
				// @todo Cache mesh so we don't need to recalculate everything every frame
				drawcmd((T3d) {size, qd_by_axis((V3d) {0, 0, 1}, rot), pos},
						mesh_vertices(mesh), mesh->v_count,
						mesh_indices(mesh), mesh->i_count,
						model_texture(model, 0)->atlas_uv,
						(Color) {brightness, brightness, brightness, 1},
						0,
						0.0,
						2);
				++draw_count;
			}
			}
		}

		render_frame();

		gl_check_errors("loop");
		plat_sleep(1);
	}

	destroy_editor();
	destroy_uicontext();
	destroy_world(world);
	g_env.world= NULL;

	destroy_physworld();
	destroy_renderer();
	destroy_audiosystem();

	unload_blob(g_env.resblob);
	g_env.resblob= NULL;

	plat_quit(d);

	deinit_env();

	return 0;
}
