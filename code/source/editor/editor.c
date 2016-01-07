#include "armature_editor.h"
#include "core/basic.h"
#include "core/device.h"
#include "core/memory.h"
#include "editor.h"
#include "editor_util.h"
#include "game/game.h"
#include "game/net.h"
#include "global/env.h"
#include "global/module.h"
#include "global/rtti.h"
#include "mesh_editor.h"
#include "physics/physworld.h"
#include "physics/chipmunk_util.h"
#include "ui/gui.h"
#include "ui/uicontext.h"
#include "visual/renderer.h"

internal
void editor_free_res_state()
{
	Editor *e = g_env.editor;

	FREE(dev_ator(), e->mesh_state);
	FREE(dev_ator(), e->armature_state);
	FREE(dev_ator(), e->clip_state);

	e->mesh_state = NULL;
	e->armature_state = NULL;
	e->clip_state = NULL;
}

// Store currently selected resources for cancellation of edit
internal
void editor_store_res_state()
{
	Editor *e = g_env.editor;
	editor_free_res_state();
	
	if (e->cur_model_h != NULL_HANDLE) {
		ModelEntity *m = get_modelentity(e->cur_model_h);
		Mesh *mesh = model_mesh((Model*)res_by_name(	g_env.resblob,
													ResType_Model,
													m->model_name));
		e->mesh_state = save_res_state(&mesh->res);
	}

	if (e->ae_state.comp_h != NULL_HANDLE) {
		CompEntity *c = get_compentity(e->ae_state.comp_h);
		Armature *a = c->armature;

		e->armature_state = save_res_state(&a->res);
	}

	if (	!e->ae_state.clip_is_bind_pose &&
			e->ae_state.clip_name[0] != '\0') {
		Clip *clip =
				(Clip*)res_by_name(	g_env.resblob,
									ResType_Clip,
									e->ae_state.clip_name);
		e->clip_state = save_res_state(&clip->res);
	}
}

// Revert currenty select resources to state before edit action
internal
void editor_revert_res_state()
{
	debug_print("Trying to revert res state");
	Editor *e = g_env.editor;
	if (e->mesh_state)
		load_res_state(e->mesh_state);

	if (e->armature_state)
		load_res_state(e->armature_state);

	if (e->clip_state)
		load_res_state(e->clip_state);
}


void create_editor()
{
	Editor* e = ZERO_ALLOC(dev_ator(), sizeof(*e), "editor");
	e->cur_model_h = NULL_HANDLE;
	e->ae_state.comp_h = NULL_HANDLE;

	g_env.editor = e;
}

void destroy_editor()
{
	editor_free_res_state();
	FREE(dev_ator(), g_env.editor);
	g_env.editor = NULL;
}

internal const char *gui_value_str(GuiContext *ctx, const char *type_name, void *deref_ptr, int ptr_depth, int array_depth, void *ptr)
{
	// @todo Rest
	if (!strcmp(type_name, "int") && deref_ptr)
		return gui_str(ctx, "%i", *(int*)deref_ptr);
	else if (!strcmp(type_name, "F64") && deref_ptr)
		return gui_str(ctx, "%f", *(F64*)deref_ptr);
	else if (!strcmp(type_name, "F32") && deref_ptr)
		return gui_str(ctx, "%f", *(F32*)deref_ptr);
	else if (!strcmp(type_name, "U32") && deref_ptr)
		return gui_str(ctx, "%u", *(U32*)deref_ptr);
	else if (!strcmp(type_name, "bool") && deref_ptr)
		return gui_str(ctx, "%s", *(bool*)deref_ptr ? "true" : "false");
	else if (!strcmp(type_name, "V2i") && deref_ptr) {
		V2i v = *(V2i*)deref_ptr;
		return gui_str(ctx, "{%i, %i}", v.x, v.y);
	} else if (!strcmp(type_name, "V2f") && deref_ptr) {
		V2f v = *(V2f*)deref_ptr;
		return gui_str(ctx, "{%f, %f}", v.x, v.y);
	} else if (!strcmp(type_name, "V2d") && deref_ptr) {
		V2d v = *(V2d*)deref_ptr;
		return gui_str(ctx, "{%f, %f}", v.x, v.y);
	} else if (!strcmp(type_name, "V3d") && deref_ptr) {
		V3d v = *(V3d*)deref_ptr;
		return gui_str(ctx, "{%f, %f, %f}", v.x, v.y, v.z);
	} else if (!strcmp(type_name, "T3d") && deref_ptr) {
		T3d v = *(T3d*)deref_ptr;
		return gui_str(ctx, "{ {%f, %f, %f}, {todo}, {%f, %f, %f}}", v.scale.x, v.scale.y, v.scale.z, v.pos.x, v.pos.y, v.pos.z);
	} else if (!strcmp(type_name, "Color") && deref_ptr) {
		Color v = *(Color*)deref_ptr;
		return gui_str(ctx, "{%f, %f, %f, %f}", v.r, v.g, v.b, v.a);
	} else if (!strcmp(type_name, "char") && deref_ptr && (ptr_depth == 1 || array_depth == 1))
		return gui_str(ctx, "%s", deref_ptr);
	else if (ptr)
		return gui_str(ctx, "%p", ptr);

	return "??";
}

typedef struct DataTreeSelected {
	void *ptr;
	U32 size;
} DataTreeSelected;

internal void gui_data_tree(GuiContext *ctx, const char *struct_name, void *struct_ptr, const char *tag, DataTreeSelected *sel)
{
	StructRtti *s = rtti_struct(struct_name);
	if (!s)
		return;
	for (U32 i = 0; i < s->member_count; ++i) {
		MemberRtti m = s->members[i];

		void *member_ptr = NULL;
		void *deref_ptr = NULL;
		if (struct_ptr && m.size > 0) {
			member_ptr = (U8*)struct_ptr + m.offset;
			deref_ptr = member_ptr;

			// Dereference in case of pointers as members
			for (U32 k = 0; k < m.ptr_depth; ++k) {
				if (deref_ptr)
					deref_ptr = *(U8**)deref_ptr;
			}
		}

		const char *value_str = gui_value_str(ctx, m.base_type_name, deref_ptr, m.ptr_depth, m.array_depth, member_ptr);
		const char stars[] = "**********";
		ensure(sizeof(stars) > m.ptr_depth);
		const char *label = gui_str(ctx, "datatree+%s_%s_%s|%s %s%s%s = %s",
			tag, m.base_type_name, m.name,
			m.base_type_name,
			&stars[sizeof(stars) - m.ptr_depth - 1],
			m.name,
			m.array_depth > 0 ? "[]" : "",
			value_str);

		// @todo Rest
		// @todo Unify somehow. Maybe have an array of values describing formatting, gui controls etc.
		if (!strcmp(m.base_type_name, "bool")) {
			if (!sel) {
				gui_checkbox(ctx, label, deref_ptr);
			} else {
				if (gui_button(ctx, label)) {
					ensure(member_ptr == deref_ptr); // Can't refer outside struct
					sel->ptr = member_ptr; 
					sel->size = m.size;
				}
			}
		} else if (!strcmp(m.base_type_name, "F64")) {
			if (!sel) {
				gui_doublefield(ctx, label, deref_ptr);
			} else {
				if (gui_button(ctx, label)) {
					ensure(member_ptr == deref_ptr); // Can't refer outside struct
					sel->ptr = member_ptr; 
					sel->size = m.size;
				}
			}
		} else if (!strcmp(m.base_type_name, "char") && m.array_depth == 1 && m.ptr_depth == 0) {
			if (!sel) {
				gui_textfield(ctx, label, deref_ptr, m.size);
			} else {
				if (gui_button(ctx, label)) {
					ensure(member_ptr == deref_ptr); // Can't refer outside struct
					sel->ptr = member_ptr; 
					sel->size = m.size;
				}
			}
		} else if (gui_begin_tree(ctx, label)) {
			gui_data_tree(ctx, m.base_type_name, deref_ptr, tag, sel);
			gui_end_tree(ctx);
		}
	}
}

internal void spawn_entity(World *world, ResBlob *blob, V2d pos)
{
	if (g_env.netstate && !g_env.netstate->authority)
		return;

	local_persist int type = 0;
	type = (type + 1) % 3;
	const char* prop_name =
		(char*[]) {"wbarrel", "wbox", "rollbot"}[type];

	T3d tf = {{1, 1, 1}, identity_qd(), {pos.x, pos.y, 0}};
	SlotVal init_vals[] = { // Override default values from json
		{"body",	"tf",			WITH_DEREF_SIZEOF(&tf)},
		{"body",	"def_name",		WITH_STR_SIZE(prop_name)},
		{"visual",	"model_name",	WITH_STR_SIZE(prop_name)},
	};
	NodeGroupDef *def =
		(NodeGroupDef*)res_by_name(blob, ResType_NodeGroupDef, "phys_prop");
	create_nodes(world, def, WITH_ARRAY_COUNT(init_vals), g_env.world->next_entity_id++, AUTHORITY_PEER);
}

void upd_editor(F64 *world_dt)
{
	Editor *e = g_env.editor;
	GuiContext *ctx = g_env.uicontext->gui;

	{ // F-keys
		if (g_env.device->key_pressed[KEY_F1])
			e->state = EditorState_mesh;
		if (g_env.device->key_pressed[KEY_F2])
			e->state = EditorState_armature;
		if (g_env.device->key_pressed[KEY_F3])
			e->state = EditorState_world;
		if (g_env.device->key_pressed[KEY_F4])
			e->state = EditorState_gui_test;

		if (g_env.device->key_pressed[KEY_F5]) {
			U32 count = mirror_blob_modifications(g_env.resblob);
			if (count > 0)
				delete_file(blob_path(g_env.game)); // Force make_blob at restart
		}

		if (g_env.device->key_pressed[KEY_F9]) {
			system("cd ../../code && clbs debug mod");
			make_main_blob(blob_path(g_env.game), g_env.game);

			if (!g_env.device->key_down[KEY_LSHIFT] && blob_has_modifications(g_env.resblob))
				critical_print("Current resblob has unsaved modifications -- not reloading");
			else
				reload_blob(&g_env.resblob, g_env.resblob, blob_path(g_env.game));
		}

		if (g_env.device->key_pressed[KEY_F12]) {
			U32 count;
			Module **modules = (Module**)all_res_by_type(&count,
														g_env.resblob,
														ResType_Module);
			for (U32 i = 0; i < count; ++i)
				system(frame_str("cd ../../code && clbs debug %s", count[modules]->res.name));

			if (!file_exists(blob_path(g_env.game)))
				make_main_blob(blob_path(g_env.game), g_env.game);

			/// @todo Reload only modules
			if (!g_env.device->key_down[KEY_LSHIFT] && blob_has_modifications(g_env.resblob))
				critical_print("Current resblob has unsaved modifications -- not reloading");
			else
				reload_blob(&g_env.resblob, g_env.resblob, blob_path(g_env.game));
		}
	}

	if (g_env.device->key_pressed[KEY_ESC])
		e->state = EditorState_invisible;
	
	// Prevent overwriting animation clip pose by other nodes
	g_env.world->editor_disable_memcpy_cmds = e->state == EditorState_armature;

	if (e->state != EditorState_invisible) {
		// In editor

		*world_dt *= e->world_time_mul;

		if (world_has_input(g_env.uicontext->gui)) {
			V2d cursor_on_world = screen_to_world_point(g_env.device->cursor_pos);
			V2d prev_cursor_on_world = screen_to_world_point(g_env.uicontext->dev.prev_cursor_pos);
			V2d cursor_delta_on_world = sub_v2d(cursor_on_world, prev_cursor_on_world);

			F32 dt = g_env.device->dt;
			F32 spd = 25.0;
			if (g_env.device->key_down[KEY_UP])
				g_env.renderer->cam_pos.y += spd*dt;
			if (g_env.device->key_down[KEY_LEFT])
				g_env.renderer->cam_pos.x -= spd*dt;
			if (g_env.device->key_down[KEY_DOWN])
				g_env.renderer->cam_pos.y -= spd*dt;
			if (g_env.device->key_down[KEY_RIGHT])
				g_env.renderer->cam_pos.x += spd*dt;

			if (g_env.device->key_down[KEY_MMB]) {
				g_env.renderer->cam_pos.x -= cursor_delta_on_world.x;
				g_env.renderer->cam_pos.y -= cursor_delta_on_world.y;
			}

			if (g_env.device->key_down['y'])
				g_env.renderer->cam_pos.z -= spd*dt;
			if (g_env.device->key_down['h'])
				g_env.renderer->cam_pos.z += spd*dt;

			F32 zoom = g_env.device->mwheel_delta;
			if (g_env.device->key_down[KEY_LSHIFT])
				zoom *= 0.1;
			g_env.renderer->cam_pos.z -= zoom;
			g_env.renderer->cam_pos.z = MIN(g_env.renderer->cam_pos.z, 30);

			if (g_env.device->key_pressed['k'])
				play_sound("dev_beep0", 1.0, -1.0);
			if (g_env.device->key_pressed['l'])
				play_sound("dev_beep1", 0.5, 1.0);

#			ifdef USE_FLUID
			if (g_env.device->key_down['r']) {
				GridCell *grid = g_env.physworld->grid;
				U32 i = GRID_INDEX_W(cursor_on_world.x, cursor_on_world.y);
				U32 width = 20;
				if (g_env.device->key_down[KEY_LSHIFT])
					width = 1;
				if (!g_env.device->key_down[KEY_LCTRL]) {
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
		}

		bool tab_pressed = g_env.device->key_pressed[KEY_TAB];
		if (tab_pressed)
			toggle_bool(&e->is_edit_mode);
		if (e->state == EditorState_mesh && e->cur_model_h == NULL_HANDLE)
			e->is_edit_mode = false;
		if (	e->state == EditorState_armature &&
				e->ae_state.comp_h == NULL_HANDLE)
			e->is_edit_mode = false;

		bool mesh_editor_active = e->state == EditorState_mesh;
		bool armature_editor_active = e->state == EditorState_armature;

		do_mesh_editor(&e->cur_model_h, &e->is_edit_mode, mesh_editor_active);
		do_armature_editor(	&e->ae_state,
							e->is_edit_mode,
							armature_editor_active);

		if (e->state == EditorState_world) {
			gui_begin_panel(ctx, "world_tools|World tools");
				gui_checkbox(ctx, "world_tool_elem+prog|Show program state", &e->show_prog_state);
				gui_checkbox(ctx, "world_tool_elem+nodes|Show nodes", &e->show_node_list);
				gui_checkbox(ctx, "world_tool_elem+cmds|Show commands", &e->show_cmd_list);
				gui_checkbox(ctx, "world_tool_elem+create_cmd|Create command", &e->show_create_cmd);
				gui_checkbox(ctx, "world_tool_elem+nodegroupdefs|Show NodeGroupDefs", &e->show_nodegroupdef_list);
				gui_slider(ctx, "world_tool_elem+dt_mul|Time mul", &e->world_time_mul, 0.0f, 10.0f);
				if (gui_button(ctx, "world_tool_elem+pause_game|Toggle pause")) {
					if (e->world_time_mul > 0.0)
						e->world_time_mul = 0.0;
					else
						e->world_time_mul = 1.0;
				}
				gui_slider(ctx, "world_tool_elem+exp|Exposure", &g_env.renderer->exposure, -5.0f, 5.0f);
				gui_checkbox(ctx, "world_tool_elem+physdraw|Physics debug draw", &g_env.physworld->debug_draw);

				if (gui_button(ctx, "world_tool_elem+save|Save world")) {
					save_world_to_file(g_env.world, SAVEFILE_PATH);
				}
				if (gui_button(ctx, "world_tool_elem+load|Load world")) {
					load_world_from_file(g_env.world, SAVEFILE_PATH);
				}

			gui_end_panel(ctx);

			if (world_has_input()) {
				Device *d = g_env.device;
				V2d cursor_on_world = screen_to_world_point(g_env.device->cursor_pos);
				if (d->key_down['t'])
					set_grid_material_in_circle(cursor_on_world, 2, GRIDCELL_MATERIAL_AIR);
				if (d->key_down['g'])
					set_grid_material_in_circle(cursor_on_world, 1, GRIDCELL_MATERIAL_GROUND);
				if (g_env.device->key_down['e'])
					spawn_entity(g_env.world, g_env.resblob, cursor_on_world);
				local_persist RigidBody *body = NULL;
				if (g_env.device->key_down['f']) {
					if (!body) {
						U32 count;
						QueryInfo *infos = query_bodies(&count, cursor_on_world, 0.1);
						for (U32 i = 0; i < count; ++i) {
							// @todo Closest one
							if (infos[i].body->cp_body != g_env.physworld->cp_ground_body) {
								body = infos[i].body;
								break;
							}
						}
					}

					if (body) {
						cpBodySetPosition(body->cp_body, to_cpv(cursor_on_world));
						cpBodySetVelocity(body->cp_body, cpv(0, 0));
					}
				} else if (body) {
					body = NULL;
				}
			}

			if (e->show_prog_state) {
				gui_begin_window(ctx, "program_state|Program state");
				gui_data_tree(ctx, "Env", &g_env, "prog", NULL);
				gui_end_window(ctx);
			}

			if (e->show_node_list) {
				gui_begin_window(ctx, "node_list|Node list");
				for (U32 i = 0; i < MAX_NODE_COUNT; ++i) {
					NodeInfo *info = &g_env.world->nodes[i];
					if (!info->allocated)
						continue;

					if (gui_begin_tree(	ctx, gui_str(ctx, "node_list_item+%i|%s id %i group %i",
										info->node_id, info->type_name, info->node_id, info->group_id))) {
						info->selected = true;
						gui_data_tree(ctx, info->type_name, node_impl(g_env.world, NULL, info), gui_str(ctx, "node_%i", info->node_id), NULL);
						gui_end_tree(ctx);
					} else {
						info->selected = false;
					}
				}
				gui_end_window(ctx);
			}

			if (e->show_cmd_list) {
				gui_begin_window(ctx, "cmd_list|Node command list");
				for (U32 i = 0; i < MAX_NODE_CMD_COUNT; ++i) {
					NodeCmd *cmd = &g_env.world->cmds[i];
					if (!cmd->allocated)
						continue;

					if (gui_begin_tree(ctx, gui_str(ctx, "cmd_list_item+%i|%i", cmd->cmd_id, cmd->cmd_id))) {
						gui_data_tree(ctx, "NodeCmd", cmd, gui_str(ctx, "cmd_%i", i), NULL);
						gui_end_tree(ctx);
					}
				}
				gui_end_window(ctx);
			}

			if (e->show_nodegroupdef_list) {
				U32 count;
				Resource **defs = all_res_by_type(	&count,
													g_env.resblob,
													ResType_NodeGroupDef);
				gui_begin_window(ctx, "nodegroupdef_list|NodeGroupDef list");
				for (U32 i = 0; i < count; ++i) {
					NodeGroupDef *def = (NodeGroupDef*)defs[i];

					if (gui_selectable(ctx, gui_str(ctx, "nodegroupdef_list_item+%i|%s", i, def->res.name), e->selected_nodegroupdef == i)) {
						e->selected_nodegroupdef = i;
					}
				}
				gui_end_window(ctx);

				if (world_has_input(ctx) && g_env.device->key_pressed[KEY_LMB] && e->selected_nodegroupdef < count) {
					V2d cursor_on_world = screen_to_world_point(g_env.device->cursor_pos);
					T3d tf = {{1, 1, 1}, identity_qd(), {cursor_on_world.x, cursor_on_world.y, 0}};
					SlotVal init_vals[] = { // Override default values from json
						{"body",	"tf",			WITH_DEREF_SIZEOF(&tf)},
						{"door",	"pos",			WITH_DEREF_SIZEOF(&cursor_on_world)},
					};
					NodeGroupDef *def = (NodeGroupDef*)defs[e->selected_nodegroupdef];
					create_nodes(g_env.world, def, WITH_ARRAY_COUNT(init_vals), g_env.world->next_entity_id++, AUTHORITY_PEER);
				}
			}

			if (e->show_create_cmd) {
				gui_begin_window(ctx, "create_cmd|Create node command");

				gui_begin(ctx, "create_cmd_list_1");
					if (gui_selectable(ctx, "create_cmd_list_item+sel_src_btn|Select source", e->create_cmd.select_src))
						e->create_cmd.select_src = true;
					if (gui_selectable(ctx, "create_cmd_list_item+sel_dst_btn|Select destination", e->create_cmd.select_dst))
						e->create_cmd.select_dst = true;
					const char *desc = gui_str(ctx, "src: %ld, %i, %i   dst: %ld, %i, %i%s",
						(long)e->create_cmd.src_node, e->create_cmd.src_offset, e->create_cmd.src_size,
						(long)e->create_cmd.dst_node, e->create_cmd.dst_offset, e->create_cmd.dst_size,
						e->create_cmd.dst_size != e->create_cmd.src_size ? " <- Size mismatch!" : "");
					gui_label(ctx, gui_str(ctx, "create_cmd_list_item+desc|%s", desc));
					if (gui_button(ctx, "create_cmd_list_item+create|Create command")) {
						ensure(e->create_cmd.dst_size == e->create_cmd.src_size);
						NodeCmd cmd = {};
						cmd.type = CmdType_memcpy;
						cmd.cmd_id = g_env.world->next_cmd_id++;
						cmd.src_offset = e->create_cmd.src_offset;
						cmd.dst_offset = e->create_cmd.dst_offset;
						cmd.size = e->create_cmd.src_size;
						cmd.src_node = node_id_to_handle(g_env.world, e->create_cmd.src_node);
						cmd.dst_node = node_id_to_handle(g_env.world, e->create_cmd.dst_node);
						resurrect_cmd(g_env.world, cmd);

						memset(&e->create_cmd, 0, sizeof(e->create_cmd));
					}
				gui_end(ctx);

				gui_begin(ctx, "create_cmd_list_2");
					gui_label(ctx, "create_cmd_list_item+label|Selected nodes");

					for (U32 i = 0; i < MAX_NODE_COUNT; ++i) {
						NodeInfo *info = &g_env.world->nodes[i];
						if (!info->selected)
							continue;

						if (gui_begin_tree(	ctx, gui_str(ctx, "create_cmd_list_item+%i|%s id %i group %i",
											info->node_id, info->type_name, info->node_id, info->group_id))) {
							DataTreeSelected selected = {};
							gui_data_tree(	ctx, info->type_name, node_impl(g_env.world, NULL, info),
											gui_str(ctx, "create_cmd_node_%i", info->node_id), &selected);
							if (selected.ptr) {
								U32 offset = (U8*)selected.ptr - (U8*)node_impl(g_env.world, NULL, info);
								U32 size = selected.size;
								if (e->create_cmd.select_src) {
									e->create_cmd.select_src = false;
									e->create_cmd.src_node = info->node_id;
									e->create_cmd.src_offset = offset;
									e->create_cmd.src_size = size;
								}
								if (e->create_cmd.select_dst) {
									e->create_cmd.select_dst = false;
									e->create_cmd.dst_node = info->node_id;
									e->create_cmd.dst_offset = offset;
									e->create_cmd.dst_size = size;
								}
							}

							gui_end_tree(ctx);
						}
					}

				gui_end(ctx);
				gui_end_window(ctx);
			}

		} else if (e->state == EditorState_gui_test) {
			GuiContext *ctx = g_env.uicontext->gui;

			gui_begin_window(ctx, "win");
				for (U32 i = 0; i < 10; ++i) {
					gui_button(ctx, gui_str(ctx, "btn_in_list+%i|button_%i", i, i));
				}
			gui_end_window(ctx);

			gui_begin_window(ctx, "Gui components");
				local_persist int btn = 0;
				local_persist F32 slider = 0;
				local_persist char buf[128];
				local_persist const char *combo = "combo+0|none";

				const char *combos[3] = {"combo+1|combo1", "combo+2|combo2", "combo+3|combo3"};

				gui_checkbox(ctx, "Show layout editor", &e->edit_layout);
				if (gui_radiobutton(ctx, "Radio 1", btn == 0)) btn = 0;
				if (gui_radiobutton(ctx, "Radio 2", btn == 1)) btn = 1;
				if (gui_radiobutton(ctx, "Radio 3", btn == 2)) btn = 2;
				gui_slider(ctx, "Slider", &slider, 0.0f, 1.0f);
				gui_textfield(ctx, "Textfield", buf, sizeof(buf));

				if (gui_begin_combo(ctx, combo)) {
					for (U32 i = 0; i < sizeof(combos)/sizeof(*combos); ++i) {
						if (gui_combo_item(ctx, combos[i]))
							combo = combos[i];
					}
					gui_end_combo(ctx);
				}

				if (gui_begin_tree(ctx, "Tree component")) {
					gui_button(ctx, "ASDFG1");
					gui_button(ctx, "ASDFG2");
					if (gui_begin_tree(ctx, "ASDFG3")) {
						gui_button(ctx, "sub menu thing");
						gui_button(ctx, "sub menu thing 2");
						gui_end_tree(ctx);
					}
					gui_end_tree(ctx);
				}

			gui_end_window(ctx);

			gui_begin_panel(ctx, "panel");
				gui_button(ctx, "button");
			gui_end_panel(ctx);
		}
	}

	if (e->edit_layout && e->state != EditorState_invisible)
		gui_layout_editor(g_env.uicontext->gui, "../../code/source/ui/gen_layout.c");
}
