#include "animation/armature.h"
#include "animation/joint.h"
#include "build.h"
#include "core/basic.h"
#include "core/debug.h"
#include "core/device.h"
#include "core/math.h"
#include "core/random.h"
#include "core/socket.h"
#include "editor/editor.h"
#include "game/aitest.h"
#include "game/world.h"
#include "game/worldgen.h"
#include "global/env.h"
#include "physics/physworld.h"
#include "resources/resblob.h"
#include "ui/uicontext.h"
#include "visual/model.h"
#include "visual/renderer.h"

internal
void make_main_blob(const char *blob_path, const char *game)
{
	const char *engine_res_root = frame_str("%srevolc/", DEFAULT_RES_ROOT);
	const char *game_res_root = frame_str("%s%s/", DEFAULT_RES_ROOT, game);

	// @todo Fix crappy api!
	char **game_res_paths = plat_find_paths_with_end(game_res_root, ".res");
	char **engine_res_paths = plat_find_paths_with_end(engine_res_root, ".res");

	U32 res_count = 0;
	char *res_paths[MAX_RES_FILES] = {0};
	for (U32 i = 0; res_count < MAX_RES_FILES && engine_res_paths[i]; ++i)
		res_paths[res_count++] = engine_res_paths[i];
	for (U32 i = 0; res_count < MAX_RES_FILES && game_res_paths[i]; ++i)
		res_paths[res_count++] = game_res_paths[i];

	make_blob(blob_path, res_paths);
	for (U32 i = 0; res_paths[i]; ++i)
		FREE(gen_ator(), res_paths[i]);
	FREE(gen_ator(), engine_res_paths);
	FREE(gen_ator(), game_res_paths);
}

const char *blob_path(const char *game)
{ return frame_str("%s.blob", game); }

internal
void spawn_entity(World *world, ResBlob *blob, V2d pos)
{
	if (g_env.netstate && !g_env.netstate->authority)
		return;

	local_persist U64 group_i = 0;
	group_i = (group_i + 1) % 3;

	const char* prop_name =
		(char*[]) {"wbarrel", "wbox", "rollbot"}[group_i];

	T3d tf = {{1, 1, 1}, identity_qd(), {pos.x, pos.y, 0}};
	SlotVal init_vals[] = { // Override default values from json
		{"body",	"tf",			WITH_DEREF_SIZEOF(&tf)},
		{"body",	"def_name",		WITH_STR_SIZE(prop_name)},
		{"visual",	"model_name",	WITH_STR_SIZE(prop_name)},
	};
	NodeGroupDef *def =
		(NodeGroupDef*)res_by_name(blob, ResType_NodeGroupDef, "phys_prop");
	create_nodes(world, def, WITH_ARRAY_COUNT(init_vals), group_i, AUTHORITY_PEER);
}

#define SAVEFILE_PATH "save.bin"

int main(int argc, const char **argv)
{
	const char *game = NULL;
	if (argc < 2)
		fail("Specify name of the game");
	else
		game = argv[1];

	init_env(argc, argv);

	Device *d = plat_init(frame_str("Revolc engine - %s", game), (V2i) {800, 600});

	if (!file_exists(blob_path(game)))
		make_main_blob(blob_path(game), game);
	load_blob(&g_env.resblob, blob_path(game));
	print_blob(g_env.resblob);

	create_audiosystem();
	create_renderer();
	create_physworld();
	create_uicontext();
	create_editor();

	init_for_modules();

	World *world = g_env.world = create_world();

	// Init/load world
	if (file_exists(SAVEFILE_PATH)) {
		//load_world(world, SAVEFILE_PATH);
	} else {
		worldgen_for_modules(world);
	}

	F64 time_accum = 0.0; // For fps
	U32 frame = 0;
	plat_update(d); // First dt should not include initialization

	g_env.os_allocs_forbidden = true; // Keep fps steady
	while (1) {
		reset_frame_alloc();

		plat_update(d);
		time_accum += d->dt;
		if (frame++== 60 && 0) {
			debug_print("---");
			debug_print("model entities: %i", g_env.renderer->m_entity_count);
			debug_print("comp entities: %i", g_env.renderer->c_entity_count);
			debug_print("bodies: %i", g_env.physworld->body_count);
			debug_print("nodes: %i", g_env.world->node_count);
			debug_print("fps: %f", frame/time_accum);
			frame = 0;
			time_accum = 0;
		}

		begin_ui_frame();

		// User input
		if (!gui_has_input(g_env.uicontext->gui)) {
			V2d cursor_on_world = screen_to_world_point(g_env.device->cursor_pos);
			V2d prev_cursor_on_world = screen_to_world_point(g_env.uicontext->dev.prev_cursor_pos);
			V2d cursor_delta_on_world = sub_v2d(cursor_on_world, prev_cursor_on_world);

			F32 dt = d->dt;
			g_env.time_from_start += dt;
			g_env.dt = dt;
			F32 spd = 25.0;
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

			g_env.renderer->cam_pos.z -= g_env.device->mwheel_delta;
			g_env.renderer->cam_pos.z = MIN(g_env.renderer->cam_pos.z, 30);

			{ // Fov which cuts stuff away with non-square window
				V2i win_size = g_env.device->win_size;
				F64 fov_scale = MAX(win_size.x, win_size.y);
				g_env.renderer->cam_fov = (V2d) {
					2*atan(g_env.device->win_size.x/fov_scale),
					2*atan(g_env.device->win_size.y/fov_scale)
				};
			}

			if (d->key_down['e'])
				spawn_entity(world, g_env.resblob, cursor_on_world);

#			ifdef USE_FLUID
			if (d->key_down['r']) {
				GridCell *grid = g_env.physworld->grid;
				U32 i = GRID_INDEX_W(cursor_on_world.x, cursor_on_world.y);
				U32 width = 20;
				if (d->key_down[KEY_LSHIFT])
					width = 1;
				if (!d->key_down[KEY_LCTRL]) {
					for (U32 x = 0; x < width; ++x) {
					for (U32 y = 0; y < width; ++y) {
						grid[i + x + y*GRID_WIDTH_IN_CELLS].water = 1;
					}
					}
				} else {
					width = 50;
					for (U32 x = 0; x < width; ++x) {
					for (U32 y = 0; y < width; ++y) {
						if (rand() % 300 == 0)
							grid[i + x + y*GRID_WIDTH_IN_CELLS].water = 1;
					}
					}
				}
			}
#			endif


			if (d->key_pressed['q'])
				g_env.physworld->debug_draw = !g_env.physworld->debug_draw;

			if (d->key_pressed['k'])
				play_sound("dev_beep0", 1.0, -1.0);
			if (d->key_pressed['l'])
				play_sound("dev_beep1", 0.5, 1.0);

			if (d->key_pressed[KEY_F5]) {
				U32 count = mirror_blob_modifications(g_env.resblob);
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
				Module **modules = (Module**)all_res_by_type(&count,
															g_env.resblob,
															ResType_Module);
				for (U32 i = 0; i < count; ++i)
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

			/*if (d->key_pressed['p'])
				save_world(world, SAVEFILE_PATH);
			if (d->key_pressed['o']) {
				destroy_world(world);
				world = g_env.world = create_world();
				load_world(world, SAVEFILE_PATH);
			}*/

			if (g_env.editor->state == EditorState_invisible) {
				local_persist cpBody *body = NULL;
				if (d->key_down['f']) {
					cpVect p = {cursor_on_world.x, cursor_on_world.y};
					cpShape *shape =
						cpSpacePointQueryNearest(
								g_env.physworld->cp_space,
								p, 0.1,
								CP_SHAPE_FILTER_ALL, NULL);

					if (!body && shape && body != g_env.physworld->cp_ground_body) {
						body = cpShapeGetBody(shape);
					}

					if (body) {
						cpBodySetPosition(body, p);
						cpBodySetVelocity(body, cpv(0, 0));
					}
				} else if (body) {
					body = NULL;
				}
			}
		}

		upd_editor();

		upd_for_modules(); // This should be in multiple places with different enum params

		if (g_env.debug)
			upd_debug(g_env.debug);

		F64 game_dt = d->dt;
		if (g_env.editor->state != EditorState_invisible)
			game_dt = 0.0;
		upd_physworld(game_dt);
		upd_world(world, game_dt);
		post_upd_physworld();
		upd_phys_rendering();
		end_ui_frame();

		render_frame();
		plat_swap_buffers(d);

		gl_check_errors("loop");
		plat_sleep(1);
	}
	g_env.os_allocs_forbidden = false;


	destroy_world(world);
	g_env.world = NULL;

	deinit_for_modules();

	destroy_editor();
	destroy_uicontext();

	destroy_physworld();
	destroy_renderer();
	destroy_audiosystem();

	unload_blob(g_env.resblob);
	g_env.resblob = NULL;

	debug_print("Heap allocation count: %i", g_env.prod_heap_alloc_count);

	plat_quit(d);

	deinit_env();

	return 0;
}
