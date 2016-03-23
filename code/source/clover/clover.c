#include "animation/clip.h"
#include "animation/joint.h"
#include "audio/audiosystem.h"
#include "build.h"
#include "core/debug.h"
#include "core/device.h"
#include "core/random.h"
#include "core/math.h"
#include "global/env.h"
#include "game/net.h"
#include "game/world.h"
#include "game/worldgen.h"
#include "physics/chipmunk_util.h"
#include "physics/rigidbody.h"
#include "physics/physworld.h"
#include "physics/query.h"
#include "resources/resblob.h"
#include "visual/renderer.h" // screen_to_world_point, camera

#include <qc/ast.h> // Test output
#include <qc/backend_c.h> // Test output
#include "global/module.h" // Test serialization
#include "visual/shadersource.h" // Test
#include "audio/sound.h" // Test
#include "visual/font.h" // Test

MOD_API void clover_worldgen(World *w)
{
	if (!g_env.netstate->authority)
		return;
	U64 seed = 10;
	generate_test_world(w);
	{
		SlotVal init_vals[] = { };
		NodeGroupDef *def =
			(NodeGroupDef*)res_by_name(g_env.resblob, ResType_NodeGroupDef, "world_env");
		create_nodes(w, def, WITH_ARRAY_COUNT(init_vals), w->next_entity_id++, AUTHORITY_PEER);
	}

	spawn_visual_prop(w, (V3d) {-100, -50, -490}, 0, (V3d) {250, 250, 1}, "bg_mountain");

	spawn_visual_prop(w, (V3d) {20, -120, -100}, 0, (V3d) {90, 90, 1}, "bg_meadow");
	spawn_visual_prop(w, (V3d) {-70, -60, -200}, 0, (V3d) {90, 90, 1}, "bg_meadow");
	spawn_visual_prop(w, (V3d) {60, -80, -160}, 0, (V3d) {90, 90, 1}, "bg_meadow");
	spawn_visual_prop(w, (V3d) {150, -50, -300}, 0, (V3d) {90, 90, 1}, "bg_meadow");
	spawn_visual_prop(w, (V3d) {400, -70, -350}, 0, (V3d) {110, 110, 1}, "bg_meadow");


	for (int i = 0; i < 1; ++i) {
		V2d pos = {
			random_f64(-30.0, 30.0, &seed),
			0
		};
		pos.y = ground_surf_y(pos.x) + 2;

		//spawn_phys_prop(w, pos, "wbarrel", false);
		spawn_phys_prop(w, pos, "rollbot", false);
		spawn_phys_prop(w, pos, "wbox", false);
	}
	for (int i = -GRID_WIDTH + 1; i < GRID_WIDTH - 1; ++i) {
		F64 x = i/2.0;
		V3d p_front = {x, ground_surf_y(x) - 0.1, random_f64(-0.1, 0.1, &seed)};
		//V3d p_back = {x, ground_surf_y(x) + 0.02, -0.1 + random_f64(-0.1, 0.0, &seed)};

		V2d a = {i - 0.2, ground_surf_y(x - 0.2)};
		V2d b = {i + 0.2, ground_surf_y(x + 0.2)};
		V2d tangent = sub_v2d(b, a);
		F64 rot = atan2(tangent.y, tangent.x) + random_f64(-0.3, 0.3, &seed);

		F64 scale = random_f64(0.85, 1.45, &seed);

		T3d tf = {{scale, scale, 1.0}, qd_by_axis((V3d){0, 0, 1}, rot), p_front};
		SlotVal init_vals[] = {
			{"body",	"tf",			WITH_DEREF_SIZEOF(&tf)},
		};
		NodeGroupDef *def =
			(NodeGroupDef*)res_by_name(g_env.resblob, ResType_NodeGroupDef, "grass");
		create_nodes(w, def, WITH_ARRAY_COUNT(init_vals), w->next_entity_id++, AUTHORITY_PEER);
	}

	// Compound test
	for (U32 i = 0; i < 5; ++i) {
		T3d tf = {(V3d) {1, 1, 1}, identity_qd(), {-3 + i, 15, 0}};
		SlotVal init_vals[] = {
			{"body", "tf", WITH_DEREF_SIZEOF(&tf)},
		};
		NodeGroupDef *def =
			(NodeGroupDef*)res_by_name(g_env.resblob, ResType_NodeGroupDef, "test_comp");
		create_nodes(w, def, WITH_ARRAY_COUNT(init_vals), w->next_entity_id++, AUTHORITY_PEER);
	}

	{ // Server character
		T3d tf = {{1, 1, 1}, identity_qd(), {0, 12}};
		SlotVal init_vals[] = {
			{"body", "tf", WITH_DEREF_SIZEOF(&tf)},
		};
		NodeGroupDef *def = (NodeGroupDef*)res_by_name(g_env.resblob, ResType_NodeGroupDef, "playerch");
		create_nodes(w, def, WITH_ARRAY_COUNT(init_vals), w->next_entity_id++, AUTHORITY_PEER);
	}

	{ // Client character
		T3d tf = {{1, 1, 1}, identity_qd(), {3, 12}};
		U8 peer_id = 1;
		SlotVal init_vals[] = {
			{"ch", "peer_id", WITH_DEREF_SIZEOF(&peer_id)},
			{"body", "tf", WITH_DEREF_SIZEOF(&tf)},
		};
		NodeGroupDef *def = (NodeGroupDef*)res_by_name(g_env.resblob, ResType_NodeGroupDef, "playerch");
		create_nodes(w, def, WITH_ARRAY_COUNT(init_vals), w->next_entity_id++, 1);
	}

#if 0
	for (U32 i = 0; i < 1; ++i) { // Dirtbug
		T3d tf = {{1, 1, 1}, identity_qd(), {0, 20}};
		SlotVal init_vals[] = {
			{"body", "tf", WITH_DEREF_SIZEOF(&tf)},
		};
		NodeGroupDef *def =
			(NodeGroupDef*)res_by_name(g_env.resblob, ResType_NodeGroupDef, "dirtbug");
		create_nodes(w, def, WITH_ARRAY_COUNT(init_vals), w->next_entity_id++, AUTHORITY_PEER);
	}
#endif
}

typedef struct WorldEnv {
	F64 time; // Game world time
} WorldEnv;

internal
void adjust_soundtrack(const char *sound_name, F32 vol)
{
	SoundHandle h = sound_handle_by_name(sound_name);
	if (!is_sound_playing(h) && vol > 0.0f)
		play_sound(sound_name, vol, 0);
	else
		set_sound_vol(h, vol);
}

MOD_API void upd_worldenv(WorldEnv *w)
{
	w->time += g_env.world->dt;

	if (g_env.device->key_down[KEY_KP_9])
		w->time += g_env.world->dt*50;
	if (g_env.device->key_down[KEY_KP_6])
		w->time -= g_env.world->dt*50;

	const F32 day_duration = 60.0*4;
	F64 time = w->time + day_duration/3.0; // Start somewhere morning
	F32 dayphase = fmod(time, day_duration)/day_duration;

	Color night = {0.15*0.3, 0.1*0.3, 0.6*0.3}; // Night
	struct TimeOfDay {
		F32 dayphase;
		Color color;
		const char *model;
		F32 ambient_fade; // 0.0 night, 1.0 day
	} times[] = {
		{0.0, night, "sky_night", 0.0},
		{0.1, night, "sky_night", 0.3},
		{0.2, {0.9, 0.2, 0.1}, "sky_evening", 0.5},
		{0.5, {2.3, 1.8, 1.9}, "sky_day", 1.0},
		{0.8, {0.9, 0.1, 0.05}, "sky_evening", 0.5},
		{0.9, night, "sky_night", 0.0},
		{1.0, night, "sky_night", 0.3},
	};
	const int times_count = ARRAY_COUNT(times);
	
	int times_i = 0;
	while (times[times_i + 1].dayphase < dayphase)
		++times_i;
	ensure(times_i + 1 < times_count);

	struct TimeOfDay cur = times[times_i];
	struct TimeOfDay next = times[times_i + 1];

	F32 t = smootherstep_f32(cur.dayphase, next.dayphase, dayphase);

	{ // Graphics
		g_env.renderer->env_light_color = lerp_color(cur.color, next.color, t);

		T3d tf = {{600, 600, 1}, identity_qd(), {0, 0, -500}};
		drawcmd_model(	tf,
						(Model*)res_by_name(g_env.resblob, ResType_Model, cur.model),
						white_color(), white_color(), 0, 0);
		tf.pos.z += 1;
		drawcmd_model(	tf,
						(Model*)res_by_name(g_env.resblob, ResType_Model, next.model),
						(Color) {1, 1, 1, t},
						(Color) {1, 1, 1, t}, 0, 0);
	}

	if (0) { // Sounds
		F32 ambient_fade = lerp_f32(cur.ambient_fade, next.ambient_fade, t);

		adjust_soundtrack("ambient_day", ambient_fade);
		adjust_soundtrack("ambient_night", 1 - ambient_fade);
	}

	{ // Test ground drawing

		const Model *model = (Model*)res_by_name(g_env.resblob, ResType_Model, "dirt");

		V2i px_ll = {0, g_env.device->win_size.y};
		V2i px_tr = {g_env.device->win_size.x, 0};
		V3d w_ll = px_tf(px_ll, (V2i) {0}).pos;
		V3d w_tr = px_tf(px_tr, (V2i) {0}).pos;
		V2i ll = GRID_VEC_W(w_ll.x, w_ll.y);
		V2i tr = GRID_VEC_W(w_tr.x, w_tr.y);

		int draw_count = 0;
		for (int y = ll.y - 3; y < tr.y + 2; ++y) {
		for (int x = ll.x - 3; x < tr.x + 3; ++x) {
			if (x < 0 || y < 0 || x >= GRID_WIDTH_IN_CELLS || y >= GRID_WIDTH_IN_CELLS)
				continue;
			if (g_env.physworld->grid.cells[GRID_INDEX(x, y)].material == GRIDCELL_MATERIAL_AIR)
				continue;

			U64 seed = x + y*10000;
			float z = random_f32(-0.05, 0.05, &seed);
			float scale = 1.3*random_f32(1.5, 1.8, &seed);
			float rot = random_f32(-0.7, 0.7, &seed);
			float brightness = random_f32(0.5, 1.0, &seed);
			float x_dif = random_f32(-0.07, 0.07, &seed);
			float y_dif = random_f32(-0.07, 0.07, &seed);
			/*
			float z = 0;
			float scale = 1;
			float rot = 0;
			float brightness = 1;
			float x_dif = 0;
			float y_dif = 0;
			*/

			V3d size = {scale/GRID_RESO_PER_UNIT, scale/GRID_RESO_PER_UNIT, 1.0};
			V3d pos = {
				(x + 0.5)/GRID_RESO_PER_UNIT - GRID_WIDTH/2 + x_dif,
				(y + 0.5)/GRID_RESO_PER_UNIT - GRID_WIDTH/2 + y_dif,
				z
			};
			// @todo Cache mesh so we don't need to recalculate everything every frame
			Color c = (Color) {brightness, brightness, brightness, 1};
			drawcmd_model((T3d) {size, qd_by_axis((V3d) {0, 0, 1}, rot), pos},
					model,
					c, c,
					0,
					0.0);
			++draw_count;
		}
		}
	}
}

MOD_API void upd_grass(	ModelEntity *front,
						ModelEntity *back,
						RigidBody *body)
{
	T3d tf = body->tf;

	back->tf = tf;
	back->tf.pos.z -= 0.05;

	front->tf = tf;
	front->tf.pos.z += 0.05;

	V2i cell_vec = GRID_VEC_W(body->tf.pos.x, body->tf.pos.y);
	V2i above_cell_vec = {cell_vec.x, cell_vec.y + 1};
	if (grid_cell(cell_vec).material == GRIDCELL_MATERIAL_AIR) {
		remove_node_group(g_env.world, body); // Kill me
	}
	if (grid_cell(above_cell_vec).material != GRIDCELL_MATERIAL_AIR) {
		remove_node_group(g_env.world, body); // Kill me
	}

}

MOD_API void init_clover()
{
	bool authority = false;
	bool connect = false;
	IpAddress remote_addr = {};
	for (U32 i = 0; i < g_env.argc; ++i) {
		if (!strcmp(g_env.argv[i], "-authority")) {
			authority = true;
		} else if (g_env.argv[i][0] == '-') {
			connect = true;
			remote_addr = str_to_ip(g_env.argv[i] + 1);
		}
	}
	const int auth_port = 19995;
	const int client_port = 19996;
	remote_addr.port = authority ? client_port : auth_port;
	create_netstate(authority, 0.1,
					10, 1024*1024*5,
					authority ? auth_port : client_port, connect ? &remote_addr : NULL);
#if 0
	critical_print("Simulated packet loss is ON!");
	g_env.netstate->peer->simulated_packet_loss = 0.05f;
#endif


#if 1 // Test C output and parsing
	QC_Array(char) code = qc_create_array(char)(128);

	debug_print("Written code");
	{
		QC_Write_Context *ctx = qc_create_write_context();
		qc_begin_initializer(ctx);
			qc_add_designated(ctx, "erkki");
			qc_add_integer(ctx, 123);

			qc_add_designated(ctx, "pekka");
			qc_begin_compound(ctx, "Raimo");
				qc_add_designated(ctx, "vuosi");
				qc_add_integer(ctx, 95);
			qc_end_compound(ctx);

			qc_add_designated(ctx, "pätkä");
			qc_begin_compound(ctx, "Reiska");
				qc_add_designated(ctx, "loitsu");
				qc_add_string(ctx, "ös");

				qc_add_designated(ctx, "magia");
				qc_add_integer(ctx, 9000);
			qc_end_compound(ctx);

			qc_add_designated(ctx, "array");
			qc_begin_compound(ctx, NULL);
				qc_add_floating(ctx, 0.0);
				qc_add_floating(ctx, 1.0);
				qc_add_floating(ctx, 2.0);
				qc_add_floating(ctx, 3.0);
				qc_add_floating(ctx, -1235.6);
			qc_end_compound(ctx);

			qc_add_integer(ctx, 1337);
			qc_add_floating(ctx, 1337.95);
		qc_end_initializer(ctx);

		qc_ast_to_c_str(&code, 0, QC_AST_BASE(ctx->root));
		qc_destroy_write_context(ctx);

		debug_print("%s", code.data);
	}

	debug_print("Parsed code");
	{
		Cson expr = cson_create(code.data, "");

		qc_clear_array(char)(&code);
		qc_ast_to_c_str(&code, 0, expr.ast_node);
		qc_print_ast(expr.ast_node, 4);
		debug_print("%s", code.data);

		{ /* Test json-like interface for reading parsed C */
			bool err = false;
			Cson pekka = cson_key(expr, "pekka");
			Cson vuosi = cson_key(pekka, "vuosi");
			debug_print("recovered pekka.vuosi: %i", blobify_integer(vuosi, &err));
			ensure(!err);

			Cson arr = cson_key(expr, "array");
			ensure(!cson_is_null(arr));
			Cson arr_4 = cson_member(arr, 4);
			ensure(!cson_is_null(arr_4));
			debug_print("recovered array[4]: %f", blobify_floating(arr_4, &err));
			ensure(!err);

			debug_print("recovered pätkä.loitsu: %s", blobify_string(cson_key(cson_key(expr, "pätkä"), "loitsu"), &err));

			ensure(!err);
		}

		cson_destroy(expr);
	}


	{
		qc_clear_array(char)(&code);

		Font *res = (Font*)res_by_name(g_env.resblob, ResType_Font, "dev");
		WCson *cson = wcson_create();

		deblobify_res(cson, &res->res);
		debug_print("DEBLOBBED RES!!");

		qc_ast_to_c_str(&code, 0, QC_AST_BASE(cson->root));
		debug_print("%s", code.data);

		wcson_destroy(cson);
	}

	qc_destroy_array(char)(&code);
#endif
}

MOD_API void deinit_clover()
{
	destroy_netstate(g_env.netstate);
}

MOD_API void upd_clover()
{
	upd_netstate(g_env.netstate);
}
