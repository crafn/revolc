#include "build.h"
#include "core/ensure.h"
#include "core/file.h"
#include "core/debug_print.h"
#include "core/vector.h"
#include "game/aitest.h"
#include "game/world.h"
#include "game/worldgen.h"
#include "global/env.h"
#include "physics/physworld.h"
#include "platform/device.h"
#include "resources/resblob.h"
#include "visual/model.h"
#include "visual/renderer.h"

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

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

	SlotVal init_vals[]= { // Override default values from json
		{"body",	"pos",			WITH_DEREF_SIZEOF(&pos)},
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
	ensure(sizeof(bool) == 1 && "Codegen relies on this");

	init_frame_alloc(FRAME_MEM_SIZE);

	Device *d= plat_init("Revolc engine", 800, 600);

	if (!file_exists(DEFAULT_BLOB_PATH))
		make_main_blob();

	ResBlob *blob= g_env.res_blob= load_blob(DEFAULT_BLOB_PATH);
	print_blob(blob);

	create_renderer();
	create_physworld();
	World *world= g_env.world= create_world();

	if (file_exists(SAVEFILE_PATH))
		load_world(world, SAVEFILE_PATH);
	else
		generate_world(world, (U32)time(NULL));

	F64 time_accum= 0.0; // For fps
	U32 frame= 0;
	while (!d->quit_requested) {
		reset_frame_alloc();

		plat_update(d);
		time_accum += d->dt;
		if (frame++ == 60) {
			debug_print("fps: %f", frame/time_accum);
			frame= 0;
			time_accum= 0;
		}

		{ // User input
			V2d cursor= {
				2.0*d->cursor_pos[0]/d->win_size[0] - 1.0,
				-2.0*d->cursor_pos[1]/d->win_size[1] + 1.0
			};

			V2d cursor_on_world= screen_to_world_point(cursor);

			F32 dt= d->dt;
			F32 spd= 25.0;
			if (d->key_down['w'])
				g_env.renderer->cam_pos.y += spd*dt;
			if (d->key_down['a'])
				g_env.renderer->cam_pos.x -= spd*dt;
			if (d->key_down['s'])
				g_env.renderer->cam_pos.y -= spd*dt;
			if (d->key_down['d'])
				g_env.renderer->cam_pos.x += spd*dt;

			if (d->key_down['y'])
				g_env.renderer->cam_pos.z -= spd*dt;
			if (d->key_down['h'])
				g_env.renderer->cam_pos.z += spd*dt;

			if (d->key_down['e'])
				spawn_entity(world, blob, cursor_on_world);

			if (d->key_pressed['r'])
				free_node_group(world, 0);
			if (d->key_pressed['t'])
				free_node_group(world, 1);
			if (d->key_pressed['g'])
				free_node_group(world, 2);
			
			if (d->key_pressed['q'])
				g_env.phys_world->debug_draw= !g_env.phys_world->debug_draw;

			if (d->key_pressed[KEY_F12]) {
				make_main_blob();
				blob= g_env.res_blob= reload_blob(blob, DEFAULT_BLOB_PATH);
			}

			if (d->key_pressed[KEY_F5])
				save_world(world, SAVEFILE_PATH);
			if (d->key_pressed[KEY_F9]) {
				destroy_world(world);
				world= g_env.world= create_world();
				load_world(world, SAVEFILE_PATH);
			}

			local_persist cpBody *body= NULL;
			if (d->lmb_down) {
				cpVect p= {cursor_on_world.x, cursor_on_world.y};
				cpShape *shape=
					cpSpacePointQueryNearest(
							g_env.phys_world->space,
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

		upd_physworld(d->dt);
		upd_world(world, d->dt);
		post_upd_physworld();
		render_frame();

		gl_check_errors("loop");
		plat_sleep(1);
	}

	destroy_world(world);
	g_env.world= NULL;

	destroy_physworld();
	destroy_renderer();

	unload_blob(blob);
	g_env.res_blob= NULL;

	free(g_env.frame_mem_begin);

	plat_quit(d);

	return 0;
}
