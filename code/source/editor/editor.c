#include "armature_editor.h"
#include "core/basic.h"
#include "core/device.h"
#include "core/memory.h"
#include "editor.h"
#include "editor_util.h"
#include "global/env.h"
#include "mesh_editor.h"
#include "ui/gui.h"
#include "ui/uicontext.h"
#include "visual/renderer.h"
#include "global/rtti.h"

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

internal const char *gui_value_str(GuiContext *ctx, const char *type_name, void *deref_ptr, int ptr_depth, void *ptr)
{
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
	} else if (!strcmp(type_name, "Color") && deref_ptr) {
		Color v = *(Color*)deref_ptr;
		return gui_str(ctx, "{%f, %f, %f, %f}", v.r, v.g, v.b, v.a);
	} else if (!strcmp(type_name, "char") && deref_ptr && ptr_depth > 0)
		return gui_str(ctx, "%s", deref_ptr);
	else if (ptr)
		return gui_str(ctx, "%p", ptr);

	return "??";
}

internal void gui_data_tree(GuiContext *ctx, const char *struct_name, void *struct_ptr)
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

		const char *value_str = gui_value_str(ctx, m.base_type_name, deref_ptr, m.ptr_depth, member_ptr);
		const char stars[] = "**********";
		ensure(sizeof(stars) > m.ptr_depth);
		const char *label = gui_str(ctx, "datatree+%s.%s|%s %s%s = %s",
			m.base_type_name, m.name, m.base_type_name, &stars[sizeof(stars) - m.ptr_depth - 1], m.name, value_str);

		if (gui_begin_tree(ctx, label)) {
			gui_data_tree(ctx, m.base_type_name, deref_ptr);
			gui_end_tree(ctx);
		}
	}
}

void upd_editor()
{
	Editor *e = g_env.editor;
	GuiContext *ctx = g_env.uicontext->gui;

	if (g_env.device->key_pressed[KEY_F1])
		e->state = EditorState_mesh;
	if (g_env.device->key_pressed[KEY_F2])
		e->state = EditorState_armature;
	if (g_env.device->key_pressed[KEY_F3])
		e->state = EditorState_world;
	if (g_env.device->key_pressed[KEY_F4])
		e->state = EditorState_gui_test;

	if (g_env.device->key_pressed[KEY_ESC])
		e->state = EditorState_invisible;

	if (e->state != EditorState_invisible) {
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
	}

	if (e->state == EditorState_world) {
		gui_begin_panel(ctx, "world_tools|World tools");
			gui_checkbox(ctx, "show_prog_state|Show program state", &e->show_prog_state);
			gui_checkbox(ctx, "show_node_list|Show nodes", &e->show_node_list);
			gui_checkbox(ctx, "show_cmd_list|Show cmds", &e->show_cmd_list);
		gui_end_panel(ctx);

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

	if (e->show_prog_state) {
		gui_begin_window(ctx, "program_state|Program state");
		gui_data_tree(ctx, "Env", &g_env);
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
				gui_data_tree(ctx, info->type_name, node_impl(g_env.world, NULL, info));
				gui_end_tree(ctx);
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
				gui_data_tree(ctx, "NodeCmd", cmd);
				gui_end_tree(ctx);
			}
		}
		gui_end_window(ctx);
	}

	if (e->edit_layout)
		gui_layout_settings(g_env.uicontext->gui, "../../code/source/ui/gen_layout.c");
}
