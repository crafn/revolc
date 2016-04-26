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

int main(int argc, const char **argv)
{
	const char *game = NULL;
	if (argc < 2)
		fail("Specify name of the game");
	else
		game = argv[1];

	init_env(argc, argv);
	g_env.game = game;

	Device *d = plat_init(frame_str("Revolc engine - %s", game), (V2i) {1280, 1024});

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
		load_world_from_file(world, SAVEFILE_PATH);
	} else {
		worldgen_for_modules(world);
	}

	plat_update(d); // First dt should not include initialization

	g_env.os_allocs_forbidden = true; // Keep fps steady
	while (1) {
		reset_frame_alloc();

		plat_update(d);

		begin_ui_frame();

		if (!gui_has_input(g_env.uicontext->gui) && d->key_pressed['u']) {
			if (!d->key_down[KEY_LSHIFT] && blob_has_modifications(g_env.resblob))
				critical_print("Current resblob has unsaved modifications -- press shift + u to quit without saving");
			else
				break;
		}

		g_env.time_from_start += d->dt;
		g_env.dt = d->dt;
		F64 world_dt = d->dt;
		upd_editor(&world_dt);

		upd_for_modules(); // This should be in multiple places with different enum params

		if (g_env.debug)
			upd_debug(g_env.debug);

		upd_physworld(world_dt);
		upd_world(world, world_dt);
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
