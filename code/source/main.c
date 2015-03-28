#include "animation/armature.h"
#include "animation/joint.h"
#include "build.h"
#include "core/ensure.h"
#include "core/file.h"
#include "core/debug_print.h"
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

#define DEFAULT_RES_ROOT "../../resources/gamedata/"

internal
void make_main_blob()
{
	char **res_paths= plat_find_paths_with_end(DEFAULT_RES_ROOT, ".res");
	make_blob("main.blob", res_paths);
	for (U32 i= 0; res_paths[i]; ++i)
		free(res_paths[i]);
	free(res_paths);
}

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
#define DEFAULT_BLOB_PATH "main.blob"


int main(int argc, const char **argv)
{
	char test[4];
	fmt_str(test, 4, "test");
	debug_print("%s", test);


	init_env();

	Device *d= plat_init("Revolc engine", (V2i) {1024, 768});

	if (!file_exists(DEFAULT_BLOB_PATH))
		make_main_blob();

	load_blob(&g_env.resblob, DEFAULT_BLOB_PATH);
	print_blob(g_env.resblob);

	create_audiosystem();
	create_renderer();
	create_physworld();
	World *world= g_env.world= create_world();

	if (file_exists(SAVEFILE_PATH))
		load_world(world, SAVEFILE_PATH);
	else
		generate_world(world, (U32)time(NULL));
	create_uicontext();
	create_editor();

	F64 time_accum= 0.0; // For fps
	U32 frame= 0;
	while (1) {
		reset_frame_alloc();

		plat_update(d);
		upd_uicontext();
		time_accum += d->dt;
		if (frame++ == 60) {
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

			g_env.renderer->cam_pos.z -= g_env.device->mwheel_delta;

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

			if (d->key_pressed['t'])
				free_node_group(world, 1);

			if (d->key_pressed['q'])
				g_env.physworld->debug_draw= !g_env.physworld->debug_draw;

			if (d->key_pressed['k'])
				play_sound("dev_beep0", 1.0, -1.0);
			if (d->key_pressed['l'])
				play_sound("dev_beep1", 0.5, 1.0);

			if (d->key_pressed[KEY_F5]) {
				U32 count= mirror_blob_modifications(g_env.resblob);
				if (count > 0)
					delete_file(DEFAULT_BLOB_PATH); // Force make_blob at restart
			}

			if (d->key_pressed[KEY_F9]) {
				system("cd ../../code && clbs debug mod");
				make_main_blob();

				if (!d->key_down[KEY_LSHIFT] && blob_has_modifications(g_env.resblob))
					critical_print("Current resblob has unsaved modifications -- not reloading");
				else
					reload_blob(&g_env.resblob, g_env.resblob, DEFAULT_BLOB_PATH);
			}

			if (d->key_pressed[KEY_F12]) {
				/// @todo Reload only modules
				system("cd ../../code && clbs debug mod");
				if (!file_exists(DEFAULT_BLOB_PATH))
					make_main_blob();

				if (!d->key_down[KEY_LSHIFT] && blob_has_modifications(g_env.resblob))
					critical_print("Current resblob has unsaved modifications -- not reloading");
				else
					reload_blob(&g_env.resblob, g_env.resblob, DEFAULT_BLOB_PATH);
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
				if (d->key_down[KEY_LMB]) {
					cpVect p= {cursor_on_world.x, cursor_on_world.y};
					cpShape *shape=
						cpSpacePointQueryNearest(
								g_env.physworld->cp_space,
								p, 0.1,
								CP_SHAPE_FILTER_ALL, NULL);

					if (!body && shape) {
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

		if (g_env.editor->state == EditorState_invisible) {
			upd_physworld(d->dt);
			upd_world(world, d->dt);
			post_upd_physworld();
		}
		upd_phys_debugdraw();

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
