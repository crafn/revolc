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
#include "physics/physworld.h"
#include "physics/chipmunk_util.h"
#include "ui/gui.h"
#include "ui/uicontext.h"
#include "visual/renderer.h"
#include "visual/ddraw.h"
#include "visual/renderer.h"

internal
void editor_free_res_state()
{
	Editor *e = g_env.editor;

	FREE(dev_ator(), e->mesh_state);
	FREE(dev_ator(), e->armature_state);
	FREE(dev_ator(), e->clip_state);
	FREE(dev_ator(), e->bodydef_state);

	e->mesh_state = NULL;
	e->armature_state = NULL;
	e->clip_state = NULL;
	e->bodydef_state = NULL;
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

	if (e->armature_editor.comp_h != NULL_HANDLE) {
		CompEntity *c = get_compentity(e->armature_editor.comp_h);
		Armature *a = c->armature;

		e->armature_state = save_res_state(&a->res);
	}

	if (	!e->armature_editor.clip_is_bind_pose &&
			e->armature_editor.clip_name[0] != '\0') {
		Clip *clip =
				(Clip*)res_by_name(	g_env.resblob,
									ResType_Clip,
									e->armature_editor.clip_name);
		e->clip_state = save_res_state(&clip->res);
	}

	if (e->body_editor.body_h != NULL_HANDLE) {
		RigidBody *b = get_rigidbody(e->body_editor.body_h);
		RigidBodyDef *def = (RigidBodyDef*)substitute_res(res_by_name(
			g_env.resblob, ResType_RigidBodyDef, b->def_name));

		e->bodydef_state = save_res_state(&def->res);
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

	if (e->bodydef_state)
		load_res_state(e->bodydef_state);
}

internal void init_createcmdeditor(CreateCmdEditor *e)
{
	memset(e, 0, sizeof(*e));
	e->src_node = NULL_ID;
	e->dst_node = NULL_ID;
}

void create_editor()
{
	Editor* e = ZERO_ALLOC(dev_ator(), sizeof(*e), "editor");
	e->cur_model_h = NULL_HANDLE;
	e->armature_editor.comp_h = NULL_HANDLE;
	e->body_editor.body_h = NULL_HANDLE;
	e->world_node_editor.selected_groups = create_array(U64)(dev_ator(), 32);
	init_createcmdeditor(&e->create_cmd);

	g_env.editor = e;
}

void destroy_editor()
{
	destroy_array(U64)(&g_env.editor->world_node_editor.selected_groups);
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
	else if (!strcmp(type_name, "F32") && deref_ptr)
		return gui_str(ctx, "%f", *(F32*)deref_ptr);
	else if ((!strcmp(type_name, "U32") || !strcmp(type_name, "Handle")) && deref_ptr)
		return gui_str(ctx, "%u", *(U32*)deref_ptr);
	else if (!strcmp(type_name, "U16") && deref_ptr)
		return gui_str(ctx, "%u", *(U16*)deref_ptr);
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

typedef struct DataTreeInfo {
	GuiId element_id;

	const char *struct_name;
	const char *struct_ptr;

	U32 member_offset;
	U32 member_size;
} DataTreeInfo;

DECLARE_ARRAY(DataTreeInfo)
DEFINE_ARRAY(DataTreeInfo)

internal void gui_datatree(GuiContext *ctx, Array(DataTreeInfo) *infos, const char *struct_name, void *struct_ptr, const char *tag, bool editable)
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

		// Record member info for further use
		if (infos) {
			DataTreeInfo info = {
				.element_id = gui_id(label),
				.struct_name = struct_name,
				.struct_ptr = struct_ptr,
				.member_offset = m.offset,
				.member_size = m.size,
			};
			push_array(DataTreeInfo)(infos, info);
		}

		// @todo Rest
		// @todo Unify somehow. Maybe have an array of values describing formatting, gui controls etc.
		if (!strcmp(m.base_type_name, "bool")) {
			if (editable) {
				gui_checkbox(ctx, label, deref_ptr);
			} else {
				gui_button(ctx, label);
			}
		} else if (!strcmp(m.base_type_name, "F64")) {
			if (editable) {
				gui_doublefield(ctx, label, deref_ptr);
			} else {
				gui_button(ctx, label);
			}
		} else if (!strcmp(m.base_type_name, "F32")) {
			if (editable) {
				gui_floatfield(ctx, label, deref_ptr);
			} else {
				gui_button(ctx, label);
			}
		} else if (!strcmp(m.base_type_name, "char") && m.array_depth == 1 && m.ptr_depth == 0) {
			if (editable) {
				gui_textfield(ctx, label, deref_ptr, m.size);
			} else {
				gui_button(ctx, label);
			}
		} else if (gui_begin_tree(ctx, label)) {
			gui_datatree(ctx, infos, m.base_type_name, deref_ptr, tag, editable);
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

internal bool node_world_pos(World *w, V3d *pos, NodeInfo *node)
{
	*pos = (V3d) {};

	StructRtti *type = rtti_struct(node->type_name);
	for (U32 i = 0; i < type->member_count; ++i) {
		MemberRtti member = type->members[i];
		if (member.ptr_depth != 0 || member.array_depth != 0)
			continue;

		if (!strcmp(member.name, "tf")) {
			if (!strcmp(member.base_type_name, "T3d")) {
				T3d val;
				memcpy(&val, (U8*)node_impl(w, NULL, node) + member.offset, sizeof(val));
				*pos = val.pos;
				return true;
			}
		} else if (!strcmp(member.name, "pos")) {
			if (!strcmp(member.base_type_name, "V2d")) {
				memcpy(pos, (U8*)node_impl(w, NULL, node) + member.offset, sizeof(V2d));
				return true;
			} else if (!strcmp(member.base_type_name, "V3d")) {
				memcpy(pos, (U8*)node_impl(w, NULL, node) + member.offset, sizeof(V3d));
				return true;
			}
		}
	}
	return false;
}

typedef struct GroupNode {
	Id group_id;
	NodeInfo *nodeinfo;
} GroupNode;

int group_node_cmp(const void *a, const void *b)
{
	return CMP(	((GroupNode*)a)->group_id,
				((GroupNode*)b)->group_id);
}

internal void try_create_cmd(CreateCmdEditor *e)
{
	if (e->dst_size != e->src_size) {
		critical_print("try_create_cmd: incompatible src and dst sizes");
		return;
	}

	NodeCmd cmd = {};
	cmd.type = CmdType_memcpy;
	cmd.cmd_id = g_env.world->next_cmd_id++;
	cmd.memcpy.src_offset = e->src_offset;
	cmd.memcpy.dst_offset = e->dst_offset;
	cmd.memcpy.size = e->src_size;
	cmd.memcpy.src_node = node_id_to_handle(g_env.world, e->src_node);
	cmd.memcpy.dst_node = node_id_to_handle(g_env.world, e->dst_node);
	resurrect_cmd(g_env.world, cmd);

	init_createcmdeditor(e);
}

internal const char *node_label(NodeInfo *node, Id local_group_id)
{
	NodeGroupDef *def = (NodeGroupDef*)res_by_name(g_env.resblob, ResType_NodeGroupDef, node->group_def_name);
	if (node->group_id == local_group_id) {
		return def->nodes[node->node_ix_in_group].name;
	} else {
		return gui_str(g_env.uicontext->gui,	"%s.%s",
												node->group_def_name, def->nodes[node->node_ix_in_group].name);
	}
}

internal void do_world_node_editor(WorldNodeEditor *e, CreateCmdEditor *cmd_editor)
{
	// Find groups
	GroupNode *nodes = ALLOC(frame_ator(), sizeof(*nodes)*MAX_NODE_COUNT, "nodes");
	U32 node_count = 0;
	for (U32 i = 0; i < MAX_NODE_COUNT; ++i) {
		NodeInfo *node = &g_env.world->nodes[i];
		if (!node->allocated)
			continue;

		nodes[node_count++] = (GroupNode) {
			.group_id = node->group_id,
			.nodeinfo = node,
		};
	}

	qsort(nodes, node_count, sizeof(*nodes), group_node_cmp);

	// Gui for nodegroups
	Id cur_group_id = NULL_ID;
	GroupNode *cur_group_begin = NULL;
	GroupNode *cur_group_end = NULL;
	for (U32 i = 0; i < node_count; ++i) {
		NodeInfo *main_node = nodes[i].nodeinfo;

		// Run rest of loop only once per group
		if (nodes[i].group_id == cur_group_id)
			continue;

		if (cur_group_id != nodes[i].group_id) {
			cur_group_id = nodes[i].group_id;

			cur_group_begin = &nodes[i];
			cur_group_end = &nodes[i];
			while (cur_group_end < nodes + node_count && cur_group_end->group_id == cur_group_id)
				++cur_group_end;
		}

		V3d pos;
		if (!node_world_pos(g_env.world, &pos, main_node))
			continue;

		V2i screen_pos = world_to_screen_point(v3d_to_v2d(pos));
		if (	screen_pos.x < -500 || screen_pos.x > g_env.device->win_size.x + 500 ||
				screen_pos.y < -500 || screen_pos.y > g_env.device->win_size.y + 500)
			continue;

		/*bool selected = false;
		Handle selected_ix = NULL_HANDLE;
		for (U32 k = 0; k < e->selected_groups.size; ++k) {
			if (e->selected_groups.data[k] == cur_group_id) {
				selected = true;
				selected_ix = k;
				break;
			}
		}*/

		GuiContext *ctx = g_env.uicontext->gui;
		ctx->allow_next_window_outside = true;
		ctx->create_next_window_minimized = true;
		ctx->dont_save_next_window_layout = true;
		gui_set_turtle_pos(ctx, screen_pos.x, screen_pos.y);
		gui_begin_window(ctx, gui_str(ctx, "world_node_group_win_%i|%s %i", cur_group_id, main_node->group_def_name, cur_group_id), NULL);

		V2i win_pos;
		gui_window_pos(ctx, &win_pos.x, &win_pos.y);
		V3d win_pos_in_world = v2d_to_v3d(screen_to_world_point(win_pos));
		ddraw_line(black_color(), pos, win_pos_in_world, 1, WORLD_DEBUG_VISUAL_LAYER);

/*
		if (gui_checkbox(ctx, 	gui_str(ctx, "world_node_group+%i|Selected",
										cur_group_id, node->group_def_name),
								&selected)) {
			if (selected_ix != NULL_HANDLE)
				fast_erase_array(U64)(&e->selected_groups, selected_ix);
			else
				push_array(U64)(&e->selected_groups, cur_group_id);
		}
*/

		// Show nodes of the group
		Array(DataTreeInfo) tree_infos = create_array(DataTreeInfo)(dev_ator(), 64);
		{
			gui_label(ctx, gui_str(ctx, "world_node_list_item+%i_label_nodes|Nodes", i));
			for (GroupNode *g = cur_group_begin; g < cur_group_end; ++g) {
				NodeInfo *info = g->nodeinfo;

				NodeGroupDef *def = (NodeGroupDef*)res_by_name(g_env.resblob, ResType_NodeGroupDef, info->group_def_name);
				if (gui_begin_tree(	ctx, gui_str(ctx, "world_node_list_item+node_%i|(%s) %s",
									info->node_id, info->type_name, def->nodes[info->node_ix_in_group].name))) {
					info->selected = true;
					gui_datatree(	ctx, &tree_infos, info->type_name, node_impl(g_env.world, NULL, info),
									gui_str(ctx, "node_%i", info->node_id), true);
					gui_end_tree(ctx);
				} else {
					info->selected = false;
				}
			}
		}

		Array(U32) shown_cmds = create_array(U32)(dev_ator(), 8);
		{ // Show commands associated with the group nodes
			gui_label(ctx, gui_str(ctx, "world_node_list_item+%i_label_cmds|Commands", i));
			for (GroupNode *g = cur_group_begin; g < cur_group_end; ++g) {
				NodeInfo *info = g->nodeinfo;

				for (U32 k = 0; k < MAX_NODE_ASSOC_CMD_COUNT; ++k) {
					Handle cmd_handle = info->assoc_cmds[k];
					if (cmd_handle == NULL_HANDLE)
						continue;

					bool already_shown = false;
					for (U32 m = 0; m < shown_cmds.size; ++m) {
						if (cmd_handle == shown_cmds.data[m]) {
							already_shown = true;
							break;
						}
					}
					if (already_shown)
						continue;
					push_array(U32)(&shown_cmds, cmd_handle);

					NodeCmd *cmd = &g_env.world->cmds[cmd_handle];

					const char *label = "none";
					if (cmd->type == CmdType_memcpy) {
						NodeCmd_Memcpy cpy = cmd->memcpy;
						NodeInfo *src = &g_env.world->nodes[cpy.src_node];
						NodeInfo *dst = &g_env.world->nodes[cpy.dst_node];
						label = gui_str(ctx, "memcpy(%s, %s)",
										node_label(dst, cur_group_id), node_label(src, cur_group_id));
					} else if (cmd->type == CmdType_call) {
						NodeCmd_Call call = cmd->call;
						label = gui_str(ctx, "%s(", rtti_sym_name(call.fptr));
						for (U32 m = 0; m < call.p_node_count; ++m) {
							NodeInfo *param = &g_env.world->nodes[call.p_nodes[m]];
							label = gui_str(ctx, "%s%s%s", label, node_label(param, cur_group_id), 
								m + 1!= call.p_node_count ? ", " : "");
						}
						label = gui_str(ctx, "%s)", label);
					}

					if (gui_begin_tree(ctx, gui_str(ctx,	"world_node_list_item+cmd_%i|%s",
															cmd->cmd_id, label))) {
						cmd->selected = true;
						gui_datatree(	ctx, NULL, "NodeCmd", cmd,
										gui_str(ctx, "world_node_list_item+cmd_data_%i", cmd->cmd_id), true);
						gui_end_tree(ctx);
					} else {
						cmd->selected = false;
					}
				}
			}
		}

		gui_end_window(ctx);
		gui_set_turtle_pos(ctx, 0, 0);

		// Context menu for nodes
		for (GroupNode *g = cur_group_begin; g < cur_group_end; ++g) {
			NodeInfo *info = g->nodeinfo;
			if (gui_begin_contextmenu(ctx, gui_str(ctx, "node_contextmenu+%i", info->node_id),
									gui_id(gui_str(ctx, "world_node_list_item+node_%i", info->node_id)))) {
				if (gui_contextmenu_item(ctx, "node_contextmenu_item+delete|Delete node")) {
					free_node(g_env.world, node_id_to_handle(g_env.world, info->node_id));
					gui_close_contextmenu(ctx);
				}
				gui_end_contextmenu(ctx);
			}
		}

		// Context menu for cmds
		for (U32 k = 0; k < shown_cmds.size; ++k) {
			Handle cmd_handle = shown_cmds.data[k];
			NodeCmd *cmd = &g_env.world->cmds[cmd_handle];
			if (gui_begin_contextmenu(ctx, gui_str(ctx, "cmd_contextmenu+%i", cmd->cmd_id),
									gui_id(gui_str(ctx, "world_node_list_item+cmd_%i", cmd->cmd_id)))) {
				if (gui_contextmenu_item(ctx, "cmd_contextmenu_item+delete|Delete command")) {
					free_cmd(g_env.world, cmd_handle);
					gui_close_contextmenu(ctx);
				}
				gui_end_contextmenu(ctx);
			}
		}

		// Context menu for node members
		for (U32 k = 0; k < tree_infos.size; ++k) {
			DataTreeInfo tree_info = tree_infos.data[k];
			if (gui_begin_contextmenu(ctx, gui_str(ctx, "node_member_contextmenu+%i_%i", i, k), tree_info.element_id)) {
				gui_label(ctx, gui_str(ctx, "node_member_contextmenu_item+%i|Size: %i", 1, tree_info.member_size));

				if (gui_contextmenu_item(ctx, "node_member_contextmenu_item+src|Select as source")) {
					for (GroupNode *n = cur_group_begin; n != cur_group_end; ++n) {
						if (node_impl(g_env.world, NULL, n->nodeinfo) == tree_info.struct_ptr) {
							cmd_editor->src_node = n->nodeinfo->node_id;
							break;
						}
					}
					cmd_editor->src_offset = tree_info.member_offset;
					cmd_editor->src_size = tree_info.member_size;
					gui_close_contextmenu(ctx);
				}

				if (cmd_editor->src_node != NULL_ID) {
					if (gui_contextmenu_item(ctx, "node_member_contextmenu_item+dst|Create memcpy to")) {
						for (GroupNode *n = cur_group_begin; n != cur_group_end; ++n) {
							if (node_impl(g_env.world, NULL, n->nodeinfo) == tree_info.struct_ptr) {
								cmd_editor->dst_node = n->nodeinfo->node_id;
								break;
							}
						}
						cmd_editor->dst_offset = tree_info.member_offset;
						cmd_editor->dst_size = tree_info.member_size;

						try_create_cmd(cmd_editor);

						gui_close_contextmenu(ctx);
					}
				}

				gui_end_contextmenu(ctx);
				break;
			}
		}

		destroy_array(U32)(&shown_cmds);
		destroy_array(DataTreeInfo)(&tree_infos);
	}
}

void upd_editor(F64 *world_dt)
{
	Editor *e = g_env.editor;
	GuiContext *ctx = g_env.uicontext->gui;

	bool request_reblob = false;
	bool request_recompilation = false;

	{ // F-keys
		if (g_env.device->key_pressed[KEY_F1])
			e->state = EditorState_res;
		if (g_env.device->key_pressed[KEY_F3])
			e->state = EditorState_world;
		if (g_env.device->key_pressed[KEY_F4])
			e->state = EditorState_gui_test;

		if (g_env.device->key_pressed[KEY_F5]) {
			U32 count = mirror_blob_modifications(g_env.resblob);
			if (count > 0)
				delete_file(blob_path(g_env.game)); // Force make_blob at restart
		}

		if (g_env.device->key_pressed[KEY_F9])
			request_reblob = true;

		if (g_env.device->key_pressed[KEY_F12])
			request_recompilation = true;
	}

	if (g_env.device->key_pressed[KEY_ESC])
		e->state = EditorState_invisible;

	// Prevent overwriting animation clip pose by other nodes
	g_env.world->editor_disable_memcpy_cmds =
		(e->state == EditorState_res && 
		e->res_state == EditorState_Res_armature);

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
		if (e->state == EditorState_res) {
			if (	e->res_state == EditorState_Res_mesh &&
					e->cur_model_h == NULL_HANDLE)
				e->is_edit_mode = false;
			if (	e->res_state == EditorState_Res_armature &&
					e->armature_editor.comp_h == NULL_HANDLE)
				e->is_edit_mode = false;
		}

		bool mesh_editor_active = e->state == EditorState_res && e->res_state == EditorState_Res_mesh;
		bool armature_editor_active = e->state == EditorState_res && e->res_state == EditorState_Res_armature;
		bool body_editor_active = e->state == EditorState_res && e->res_state == EditorState_Res_body;

		do_mesh_editor(&e->cur_model_h, &e->is_edit_mode, mesh_editor_active);
		do_armature_editor(	&e->armature_editor,
							e->is_edit_mode,
							armature_editor_active);
		do_body_editor(&e->body_editor, e->is_edit_mode, body_editor_active);

		if (e->state == EditorState_res) {

			gui_begin_panel(ctx, "res_tools|Resource tools", "res_tools_content");
				// @todo Selected res names
				/*const char *str = gui_str(	ctx, "Selected: %s: %s",
											restype_to_str(t),
											res ? res->name : "<none>");
				gui_label(ctx, str);
				*/

				if (gui_radiobutton(ctx, "res_tool_elem+mesh|Mesh", e->res_state == EditorState_Res_mesh))
					e->res_state = EditorState_Res_mesh;
				if (gui_radiobutton(ctx, "res_tool_elem+armature|Armature", e->res_state == EditorState_Res_armature))
					e->res_state = EditorState_Res_armature;
				if (gui_radiobutton(ctx, "res_tool_elem+body|Body", e->res_state == EditorState_Res_body))
					e->res_state = EditorState_Res_body;
			gui_end_panel(ctx);
		} else if (e->state == EditorState_world) {

			gui_begin_panel(ctx, "world_tools|World tools", "world_tools_content");
				gui_checkbox(ctx, "world_tool_elem+prog|Show program state", &e->show_prog_state);
				gui_checkbox(ctx, "world_tool_elem+nodes|Show nodes", &e->show_node_list);
				gui_checkbox(ctx, "world_tool_elem+nodegroupdefs|Create NodeGroup", &e->show_nodegroupdef_list);
				if (gui_button(ctx, "world_tool_elem+delete_nodes|Delete selected nodes")) {
					for (U32 i = 0; i < MAX_NODE_COUNT; ++i) {
						NodeInfo *info = &g_env.world->nodes[i];
						if (!info->allocated || !info->selected)
							continue;

						free_node(g_env.world, i);
					}
				}
				gui_checkbox(ctx, "world_tool_elem+cmds|Show commands", &e->show_cmd_list);
				gui_checkbox(ctx, "world_tool_elem+create_cmd|Create command", &e->show_create_cmd);
				if (gui_button(ctx, "world_tool_elem+delete_cmds|Delete selected commands")) {
					for (U32 i = 0; i < MAX_NODE_CMD_COUNT; ++i) {
						NodeCmd *cmd = &g_env.world->cmds[i];
						if (!cmd->allocated || !cmd->selected)
							continue;
						free_cmd(g_env.world, i);
					}
				}
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

				gui_slider_double(ctx, "world_tool_elem+net_delta|Net delta interval", &g_env.netstate->delta_interval, 0.1, 5.0);
				if (gui_button(ctx, "world_tool_elem+reblob|Reload resources"))
					request_reblob = true;

				if (gui_button(ctx, "world_tool_elem+recompile|Recompile code"))
					request_recompilation = true;

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
				gui_begin_window(ctx, "program_state|Program state", "program_state_content");
				gui_datatree(ctx, NULL, "Env", &g_env, "prog", true);
				gui_end_window(ctx);
			}

			if (e->show_node_list) {
				gui_begin_window(ctx, "node_list|Node list", "node_list_content");
				for (U32 i = 0; i < MAX_NODE_COUNT; ++i) {
					NodeInfo *info = &g_env.world->nodes[i];
					if (!info->allocated)
						continue;

					if (gui_begin_tree(	ctx, gui_str(ctx, "node_list_item+%i|%s id %i group %i",
										info->node_id, info->type_name, info->node_id, info->group_id))) {
						info->selected = true;
						gui_datatree(	ctx, NULL, info->type_name, node_impl(g_env.world, NULL, info),
										gui_str(ctx, "node_%i", info->node_id), true);
						gui_end_tree(ctx);
					} else {
						info->selected = false;
					}
				}
				gui_end_window(ctx);
			}

			if (e->show_cmd_list) {
				gui_begin_window(ctx, "cmd_list|Node command list", "cmd_list_content");
				for (U32 i = 0; i < MAX_NODE_CMD_COUNT; ++i) {
					NodeCmd *cmd = &g_env.world->cmds[i];
					if (!cmd->allocated)
						continue;

					if (gui_begin_tree(ctx, gui_str(ctx, "cmd_list_item+%i|%i", cmd->cmd_id, cmd->cmd_id))) {
						cmd->selected = true;
						gui_datatree(ctx, NULL, "NodeCmd", cmd, gui_str(ctx, "cmd_%i", i), true);
						gui_end_tree(ctx);
					} else {
						cmd->selected = false;
					}
				}
				gui_end_window(ctx);
			}

			if (e->show_nodegroupdef_list) {
				U32 count;
				Resource **defs = all_res_by_type(	&count,
													g_env.resblob,
													ResType_NodeGroupDef);
				gui_begin_window(ctx, "nodegroupdef_list|NodeGroupDef list", "nodegroupdef_list_content");
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
				gui_begin_window(ctx, "create_cmd|Create node command", "create_cmd_content");

				gui_begin(ctx, "create_cmd_list_1");
					if (gui_selectable(ctx, "create_cmd_list_item+sel_src_btn|Select source", e->create_cmd.select_src))
						e->create_cmd.select_src = true;
					if (gui_selectable(ctx, "create_cmd_list_item+sel_dst_btn|Select destination", e->create_cmd.select_dst))
						e->create_cmd.select_dst = true;
					const char *src_id = e->create_cmd.src_node == NULL_ID ?
						"none" : gui_str(ctx, "%ld", (long)e->create_cmd.src_node);
					const char *dst_id = e->create_cmd.dst_node == NULL_ID ?
						"none" : gui_str(ctx, "%ld", (long)e->create_cmd.dst_node);
					const char *desc = gui_str(ctx, "src: %s, %i, %i   dst: %s, %i, %i%s",
						src_id, e->create_cmd.src_offset, e->create_cmd.src_size,
						dst_id, e->create_cmd.dst_offset, e->create_cmd.dst_size,
						e->create_cmd.dst_size != e->create_cmd.src_size ? " <- Size mismatch!" : "");
					gui_label(ctx, gui_str(ctx, "create_cmd_list_item+desc|%s", desc));
					if (gui_button(ctx, "create_cmd_list_item+create|Create command")) {
						try_create_cmd(&e->create_cmd);
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
							Array(DataTreeInfo) tree_infos = create_array(DataTreeInfo)(dev_ator(), 64);
							gui_datatree(	ctx, &tree_infos, info->type_name, node_impl(g_env.world, NULL, info),
											gui_str(ctx, "create_cmd_node_%i", info->node_id), false);
							for (U32 k = 0; k < tree_infos.size; ++k) {
								DataTreeInfo tree_info = tree_infos.data[k];
								if (gui_interacted(ctx, tree_info.element_id)) {
									U32 offset = tree_info.member_offset;
									U32 size = tree_info.member_size;
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
							}

							destroy_array(DataTreeInfo)(&tree_infos);

							gui_end_tree(ctx);
						}
					}

				gui_end(ctx);
				gui_end_window(ctx);
			}

			do_world_node_editor(&e->world_node_editor, &e->create_cmd);
		} else if (e->state == EditorState_gui_test) {
			GuiContext *ctx = g_env.uicontext->gui;

			gui_begin_window(ctx, "win", NULL);
				for (U32 i = 0; i < 10; ++i) {
					gui_button(ctx, gui_str(ctx, "btn_in_list+%i|button_%i", i, i));
				}
			gui_end_window(ctx);

			gui_begin_window(ctx, "Gui components", NULL);
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

			if (gui_begin_contextmenu(ctx, "contextmenu1", gui_id("sub menu thing"))) {
				bool clicked = false;
				clicked |= gui_contextmenu_item(ctx, "Foo");
				clicked |= gui_contextmenu_item(ctx, "Bar");
				gui_end_contextmenu(ctx);

				if (clicked)
					gui_close_contextmenu(ctx);
			}

			if (gui_begin_contextmenu(ctx, "contextmenu2", gui_id("sub menu thing 2"))) {
				bool clicked = false;
				clicked |= gui_contextmenu_item(ctx, "Foo");
				clicked |= gui_contextmenu_item(ctx, "Bar");
				clicked |= gui_contextmenu_item(ctx, "Jorma");
				static float f;
				gui_slider(ctx, "gui_contextmenu_item:asd", &f, 0, 1);
				gui_end_contextmenu(ctx);

				if (clicked)
					gui_close_contextmenu(ctx);
			}

			gui_begin_panel(ctx, "panel", "panel_content");
				gui_button(ctx, "button");
			gui_end_panel(ctx);
		}
	}

	if (request_reblob) {
		bool tmp = g_env.os_allocs_forbidden;
		g_env.os_allocs_forbidden = false;

		make_main_blob(blob_path(g_env.game), g_env.game);

		if (!g_env.device->key_down[KEY_LSHIFT] && blob_has_modifications(g_env.resblob))
			critical_print("Current resblob has unsaved modifications -- not reloading");
		else
			reload_blob(&g_env.resblob, g_env.resblob, blob_path(g_env.game));

		g_env.os_allocs_forbidden = tmp;
	}

	if (request_recompilation) {
		bool tmp = g_env.os_allocs_forbidden;
		g_env.os_allocs_forbidden = false;

		system(frame_str("cd ../../code && clbs debug %s", g_env.game));
		make_main_blob(blob_path(g_env.game), g_env.game);

		if (!g_env.device->key_down[KEY_LSHIFT] && blob_has_modifications(g_env.resblob))
			critical_print("Current resblob has unsaved modifications -- not reloading");
		else
			reload_blob(&g_env.resblob, g_env.resblob, blob_path(g_env.game));

		g_env.os_allocs_forbidden = tmp;
	}

	if (e->edit_layout && e->state != EditorState_invisible)
		gui_layout_editor(g_env.uicontext->gui, "../../code/source/ui/gen_layout.c");
}


//
// Individual editor views
//

// Mesh editor

internal
V3d vertex_world_pos(ModelEntity *m, U32 i)
{
	TriMeshVertex *v = &m->vertices[i];
	T3d v_t = identity_t3d();
	v_t.pos = v3f_to_v3d(v->pos);

	T3d t = mul_t3d(m->tf, v_t);
	return t.pos;
}

internal
V2i uv_to_pix(V2f uv, V2i pix_pos, V2i pix_size)
{ return (V2i) {uv.x*pix_size.x + pix_pos.x, (1 - uv.y)*pix_size.y + pix_pos.y}; }

internal
V3d pix_to_uv(V2i p, V2i pix_pos, V2i pix_size)
{ return (V3d) {(F64)(p.x - pix_pos.x)/pix_size.x,
				1 - (F64)(p.y - pix_pos.y)/pix_size.y,
				0.0};
}

internal
void draw_mesh_vertex(V3d p, bool selected, S32 layer)
{
	const F64 v_size = editor_vertex_size();
	V3d poly[4] = {
		{-v_size + p.x, -v_size + p.y, p.z},
		{-v_size + p.x, +v_size + p.y, p.z},
		{+v_size + p.x, +v_size + p.y, p.z},
		{+v_size + p.x, -v_size + p.y, p.z},
	};

	Color c = selected ?	(Color) {1.0, 0.7, 0.2, 0.9} :
							(Color) {0.0, 0.0, 0.0, 0.9};
	ddraw_poly(c, poly, 4, layer);
}

internal
Mesh *editable_model_mesh(const char *name)
{
	Mesh *mesh = model_mesh((Model*)res_by_name(	g_env.resblob,
													ResType_Model,
													name));
	mesh = (Mesh*)substitute_res(&mesh->res);
	return mesh;
}

typedef enum {
	MeshTransformType_pos,
	MeshTransformType_uv,
	MeshTransformType_outline_uv,
} MeshTransformType;

internal
void transform_mesh(ModelEntity *m, T3f tf, MeshTransformType ttype)
{
	if (m->has_own_mesh) {
		debug_print("@todo Modify unique mesh");
		return;
	}

	Mesh *mesh = editable_model_mesh(m->model_name);

	for (U32 i = 0; i < mesh->v_count; ++i) {
		TriMeshVertex *v = &mesh_vertices(mesh)[i];
		if (!v->selected)
			continue;

		if (ttype == MeshTransformType_pos) {
			v->pos = transform_v3f(tf, v->pos);
		} else if (ttype == MeshTransformType_uv) {
			F32 uvz = v->uv.z;
			v->uv = transform_v3f(tf, v->uv);
			v->uv.x = CLAMP(v->uv.x, 0.0, 1.0);
			v->uv.y = CLAMP(v->uv.y, 0.0, 1.0);
			v->uv.z = uvz;
		} else if (ttype == MeshTransformType_outline_uv) {
			V3f uv = {v->outline_uv.x, v->outline_uv.y, 0.0};
			v->outline_uv = v3f_to_v2f(transform_v3f(tf, uv));
			v->outline_uv.x = CLAMP(v->outline_uv.x, 0.0, 1.0);
			v->outline_uv.y = CLAMP(v->outline_uv.y, 0.0, 1.0);
		}
	}

	resource_modified(&mesh->res);
}

internal
void transform_poly(V2d *vertices, bool *selected, U32 count, T3f tf)
{
	for (U32 i = 0; i < count; ++i) {
		if (!selected[i])
			continue;

		vertices[i] = v3d_to_v2d(transform_v3d(t3f_to_t3d(tf), v2d_to_v3d(vertices[i])));
	}
}

internal
void gui_uvbox(GuiContext *gui, ModelEntity *m, bool outline_uv)
{
	const char *box_label = "uvbox_box";
	if (outline_uv)
		box_label = gui_str(gui, "outline_uvbox_box");
	UiContext *ctx = g_env.uicontext;

	V2i pix_pos, pix_size;
	EditorBoxState state = gui_begin_editorbox(gui, &pix_pos, &pix_size, box_label, false);

	if (!m)
		goto exit;

	if (state.pressed) {
		// Control vertex selection
		F64 closest_dist = 0;
		U32 closest_i = NULL_HANDLE;
		for (U32 i = 0; i < m->mesh_v_count; ++i) {
			TriMeshVertex *v = &m->vertices[i];
			V2f uv = outline_uv ? v->outline_uv : v3f_to_v2f(v->uv);
			V2i pos = uv_to_pix(uv, pix_pos, pix_size);

			F64 dist = dist_sqr_v2i(pos, ctx->dev.cursor_pos);
			if (	closest_i == NULL_HANDLE ||
					dist < closest_dist) {
				closest_i = i;
				closest_dist = dist;
			}
		}

		if (!ctx->dev.shift_down) {
			for (U32 i = 0; i < m->mesh_v_count; ++i)
				m->vertices[i].selected = false;
		}

		if (closest_dist < 100*100) {
			ensure(closest_i != NULL_HANDLE);
			toggle_bool(&m->vertices[closest_i].selected);
		}
	}

	V2i padding = {20, 20};
	pix_pos = add_v2i(pix_pos, padding);
	pix_size = sub_v2i(pix_size, scaled_v2i(2, padding));

	T3d coords = {
		{pix_size.x, -pix_size.y, 1},
		identity_qd(),
		{	pix_pos.x,
			pix_pos.y,
			0.0} // @todo
	};
	T3f delta;
	if (cursor_transform_delta_pixels(&delta, box_label, coords)) {
		transform_mesh(m, delta, outline_uv ? MeshTransformType_outline_uv : MeshTransformType_uv);
	}

	if (!outline_uv)
		drawcmd_px_model_image(pix_pos, pix_size, m, gui_layer(gui) + 1);

	for (U32 i = 0; i < m->mesh_v_count; ++i) {
		TriMeshVertex *v = &m->vertices[i];
		V2f uv = outline_uv ? v->outline_uv : v3f_to_v2f(v->uv);

		V2i pix_uv = uv_to_pix(uv, pix_pos, pix_size);
		V2d p = screen_to_world_point(pix_uv);
		draw_mesh_vertex(v2d_to_v3d(p), v->selected, gui_layer(gui) + 3);
	}

	Color fill_color = {0.6, 0.6, 0.8, 0.4};
	V3d poly[3];
	for (U32 i = 0; i < m->mesh_i_count; ++i) {
		U32 v_i = m->indices[i];
		TriMeshVertex *v = &m->vertices[v_i];
		V2f uv = outline_uv ? v->outline_uv : v3f_to_v2f(v->uv);
		V2i pix_uv = uv_to_pix(uv, pix_pos, pix_size);
		V2d p = screen_to_world_point(pix_uv);
		poly[i%3] = (V3d) {p.x, p.y, 0};

		if (i % 3 == 2)
			ddraw_poly(fill_color, poly, 3, gui_layer(gui) + 2);
	}
	
exit:
	gui_end_editorbox(gui);
}

// Mesh editing on world
internal
void gui_mesh_overlay(U32 *model_h, bool *is_edit_mode)
{
	UiContext *ctx = g_env.uicontext;
	V3d cur_wp = v2d_to_v3d(screen_to_world_point(ctx->dev.cursor_pos));

	const char *box_label = "editor_overlay_box";
	EditorBoxState state = gui_begin_editorbox(ctx->gui, NULL, NULL, box_label, true);

	if (!*is_edit_mode) { // Mesh select mode
		if (state.down)
			*model_h = find_modelentity_at_pixel(ctx->dev.cursor_pos);
	}

	ModelEntity *m = NULL;
	if (*model_h != NULL_HANDLE)
		m = get_modelentity(*model_h);
	else
		goto exit;

	if (ctx->dev.toggle_select_all) {
		if (*is_edit_mode) {
			bool some_selected = false;
			for (U32 i = 0; i < m->mesh_v_count; ++i) {
				if (m->vertices[i].selected)
					some_selected = true;
			}
			for (U32 i = 0; i < m->mesh_v_count; ++i) {
				m->vertices[i].selected = !some_selected;
			}
		} else {
			*model_h = NULL_HANDLE;
			goto exit;
		}
	}

	if (*is_edit_mode && g_env.device->key_pressed['e']) {
		ctx->dev.grabbing = gui_id(box_label);
		gui_set_active(ctx->gui, box_label);
		editor_store_res_state();

		// Extrude selected vertices
		Mesh *mesh = editable_model_mesh(m->model_name);
		U32 old_v_count = mesh->v_count;
		for (U32 i = 0; i < old_v_count; ++i) {
			TriMeshVertex v = mesh_vertices(mesh)[i];
			if (!v.selected)
				continue;
			add_rt_mesh_vertex(mesh, v); // Duplicate selected vertices for extrude
			mesh_vertices(mesh)[i].selected = false;
		}

		// @todo Create indices for extruded faces

		recache_ptrs_to_meshes();
	}

	if (*is_edit_mode && g_env.device->key_pressed['f']) {
		// Create face between three vertices
		Mesh *mesh = editable_model_mesh(m->model_name);
		U32 selected_count = 0;
		MeshIndexType indices[3];
		for (U32 i = 0; i < mesh->v_count; ++i) {
			TriMeshVertex v = mesh_vertices(mesh)[i];
			if (!v.selected)
				continue;
			if (selected_count < ARRAY_COUNT(indices))
				indices[selected_count++] = i;
		}
		if (selected_count == 3) {
			add_rt_mesh_index(mesh, indices[0]);
			add_rt_mesh_index(mesh, indices[1]);
			add_rt_mesh_index(mesh, indices[2]);
		} else {
			debug_print("Select 3 vertices to make a face");
		}
		recache_ptrs_to_meshes();
	}

	if (*is_edit_mode && g_env.device->key_pressed['x']) {
		// Delete selected vertices (and corresponding faces)
		Mesh *mesh = editable_model_mesh(m->model_name);
		for (U32 i = 0; i < mesh->v_count;) {
			TriMeshVertex v = mesh_vertices(mesh)[i];
			if (v.selected)
				remove_rt_mesh_vertex(mesh, i);
			else
				++i;
		}
		recache_ptrs_to_meshes();
	}

	T3f delta;
	if (*is_edit_mode && cursor_transform_delta_world(&delta, box_label, m->tf)) {
		transform_mesh(m, delta, MeshTransformType_pos);
	}

	if (*is_edit_mode && state.pressed) {
		// Control vertex selection
		F64 closest_dist = 0;
		U32 closest_i = NULL_HANDLE;
		for (U32 i = 0; i < m->mesh_v_count; ++i) {
			V3d pos = vertex_world_pos(m, i);

			F64 dist = dist_sqr_v3d(pos, cur_wp);
			if (	closest_i == NULL_HANDLE ||
					dist < closest_dist) {
				closest_i = i;
				closest_dist = dist;
			}
		}

		if (!ctx->dev.shift_down) {
			for (U32 i = 0; i < m->mesh_v_count; ++i)
				m->vertices[i].selected = false;
		}

		if (closest_i != NULL_HANDLE && closest_dist < 2.0)
			toggle_bool(&m->vertices[closest_i].selected);
	}

	// Draw vertices
	if (*is_edit_mode) {
		for (U32 i = 0; i < m->mesh_v_count; ++i) {
			TriMeshVertex *v = &m->vertices[i];
			V3d p = vertex_world_pos(m, i);
			draw_mesh_vertex(p, v->selected, WORLD_DEBUG_VISUAL_LAYER + 1);
		}
	}
	
exit:
	gui_end_editorbox(ctx->gui);
}

void do_mesh_editor(U32 *model_h, bool *is_edit_mode, bool active)
{
	GuiContext *ctx = g_env.uicontext->gui;
	bool changed = false;

	if (active) {
		gui_mesh_overlay(model_h, is_edit_mode);

		ModelEntity *m = NULL;
		if (*model_h != NULL_HANDLE)
			m = get_modelentity(*model_h);

		gui_uvbox(g_env.uicontext->gui, m, false);
		gui_uvbox(g_env.uicontext->gui, m, true);

		gui_begin_panel(ctx, "model_settings", "model_settings_content");
		if (m) {
			Model *model = (Model*)substitute_res(res_by_name(g_env.resblob, ResType_Model, m->model_name));
			Mesh *mesh = editable_model_mesh(m->model_name);

			bool col_changed = false;
			{ // Model settings
				gui_label(ctx, "model_setting+l1|Model settings");
				col_changed |= gui_slider(ctx, "model_setting+r|R", &model->color.r, 0.0, 1.0);
				col_changed |= gui_slider(ctx, "model_setting+g|G", &model->color.g, 0.0, 1.0);
				col_changed |= gui_slider(ctx, "model_setting+b|B", &model->color.b, 0.0, 1.0);
				col_changed |= gui_slider(ctx, "model_setting+a|A", &model->color.a, 0.0, 1.0);

				if (col_changed)
					resource_modified(&model->res);
			}

			{ // Vertex attributes
				V3f pos = {};
				Color col = white_color();
				Color outline_col = white_color();
				F32 col_exp = 1.0;
				F32 outline_exp = 1.0;
				F32 outline_width = 1.0;
				for (U32 i = 0; i < mesh->v_count; ++i) {
					TriMeshVertex *v = &mesh_vertices(mesh)[i];
					if (!v->selected)
						continue;
					pos = v->pos;
					col = v->color;
					outline_col = v->outline_color;
					col_exp = v->color_exp;
					outline_exp = v->outline_exp;
					outline_width = v->outline_width;
					break;
				}

				gui_label(ctx, "model_setting+l2|Vertex attributes");

				bool v_x_changed = gui_slider(ctx, "model_setting+vx|X", &pos.x, -1.0, 1.0);
				bool v_y_changed = gui_slider(ctx, "model_setting+vy|Y", &pos.y, -1.0, 1.0);
				bool v_z_changed = gui_slider(ctx, "model_setting+vz|Z", &pos.z, -1.0, 1.0);

				gui_label(ctx, "model_setting+l3|Color");
				bool v_col_changed = false;
				v_col_changed |= gui_slider(ctx, "model_setting+vr|R", &col.r, 0.0, 1.0);
				v_col_changed |= gui_slider(ctx, "model_setting+vg|G", &col.g, 0.0, 1.0);
				v_col_changed |= gui_slider(ctx, "model_setting+vb|B", &col.b, 0.0, 1.0);
				v_col_changed |= gui_slider(ctx, "model_setting+va|A", &col.a, 0.0, 1.0);

				bool col_exp_changed = gui_slider(ctx, "model_setting+vexp|Color exp", &col_exp, 0.0, 5.0);

				gui_label(ctx, "model_setting+l4|Outline color");
				bool v_out_col_changed = false;
				v_out_col_changed |= gui_slider(ctx, "model_setting+vor|R", &outline_col.r, 0.0, 1.0);
				v_out_col_changed |= gui_slider(ctx, "model_setting+vog|G", &outline_col.g, 0.0, 1.0);
				v_out_col_changed |= gui_slider(ctx, "model_setting+vob|B", &outline_col.b, 0.0, 1.0);
				v_out_col_changed |= gui_slider(ctx, "model_setting+voa|A", &outline_col.a, 0.0, 1.0);

				bool outline_exp_changed = gui_slider(ctx, "model_setting+voe|Outline exp", &outline_exp, 0.0, 5.0);
				bool outline_width_changed = gui_slider(ctx, "model_setting+vow|Outline width", &outline_width, 0.0, 50.0);

				for (U32 i = 0; i < mesh->v_count; ++i) {
					TriMeshVertex *v = &mesh_vertices(mesh)[i];
					if (!v->selected)
						continue;

					if (v_x_changed)
						v->pos.x = pos.x;
					if (v_y_changed)
						v->pos.y = pos.y;
					if (v_z_changed)
						v->pos.z = pos.z;
					if (v_col_changed)
						v->color = col;
					if (v_out_col_changed)
						v->outline_color = outline_col;
					if (col_exp_changed)
						v->color_exp = col_exp;
					if (outline_exp_changed)
						v->outline_exp = outline_exp;
					if (outline_width_changed)
						v->outline_width = outline_width;
				}

				changed |= col_changed || v_x_changed || v_y_changed || v_z_changed || v_col_changed || v_out_col_changed || col_exp_changed || outline_exp_changed || outline_width_changed;
			}

			if (changed) {
				resource_modified(&mesh->res);
				recache_ptrs_to_meshes();
			}
		}
		gui_end_panel(ctx);
	}

	// Draw mesh
	if (*model_h != NULL_HANDLE) {
		ModelEntity	*m = get_modelentity(*model_h);
		Color fill_color = {0.6, 0.6, 0.8, 0.4};
		if (!*is_edit_mode)
			fill_color = (Color) {1.0, 0.8, 0.5, 0.6};

		Color poly_color = fill_color;
		if (!active)
			poly_color = inactive_color();

		if (active)
			ddraw_circle((Color) {1, 1, 1, 1}, m->tf.pos, editor_vertex_size()*0.5, WORLD_DEBUG_VISUAL_LAYER);

		V3d poly[3];
		for (U32 i = 0; i < m->mesh_i_count; ++i) {
			U32 v_i = m->indices[i];
			V3d p = vertex_world_pos(m, v_i);
			poly[i%3] = p;

			if (i % 3 == 2 && !changed)
				ddraw_poly(poly_color, poly, 3, WORLD_DEBUG_VISUAL_LAYER);
		}
	}
}

// RigidBody editor

internal V2d *rigidbody_vertices(U32 *count, RigidBody *body)
{
	V2d *v = ALLOC(frame_ator(), MAX_BODY_VERTICES*sizeof(*v), "body_vertices");
	*count = 0;
	RigidBodyDef *def =
		(RigidBodyDef*)res_by_name(	g_env.resblob, ResType_RigidBodyDef, body->def_name);

	for (U32 i = 0; i < def->poly_count; ++i) {
		Poly p = def->polys[i];
		for (U32 k = 0; k < p.v_count; ++k) {
			v[(*count)++] = p.v[k];
		}
	}
	for (U32 i = 0; i < def->circle_count; ++i) {
		Circle c = def->circles[i];
		V2d pos = c.pos;
		v[(*count)++] = pos;
		pos.x += c.rad;
		v[(*count)++] = pos;
	}

	ensure(*count < MAX_BODY_VERTICES);

	return v;
}

// Only moving vertices between rigidbody_vertices() and apply_rigidbody_vertices() is allowed
internal void apply_rigidbody_vertices(RigidBody *body, V2d *vertices, U32 count)
{
	ensure(count < MAX_BODY_VERTICES);

	RigidBodyDef *def =
		(RigidBodyDef*)substitute_res(res_by_name(	g_env.resblob,
													ResType_RigidBodyDef, body->def_name));

	U32 v_ix = 0;
	for (U32 i = 0; i < def->poly_count; ++i) {
		Poly *p = &def->polys[i];
		for (U32 k = 0; k < p->v_count; ++k) {
			V2d pos = vertices[v_ix++];
			p->v[k] = pos;
		}
	}
	for (U32 i = 0; i < def->circle_count; ++i) {
		Circle *c = &def->circles[i];
		V2d pos = vertices[v_ix++];
		V2d arc = vertices[v_ix++];
		c->pos = pos;
		c->rad = dist_v2d(pos, arc);
	}

	ensure(v_ix == count);

	resource_modified(&def->res);
	recache_ptrs_to_rigidbodydef(def); // Apply changes to physics world
}

void do_body_editor(BodyEditor *editor, bool is_edit_mode, bool active)
{
	if (active) {
		UiContext *ctx = g_env.uicontext;

		const char *box_label = "editor_overlay_box";
		EditorBoxState state = gui_begin_editorbox(ctx->gui, NULL, NULL, box_label, true);

		if (!is_edit_mode && state.down) { // Body select mode
			V2d p = screen_to_world_point(ctx->dev.cursor_pos);
			U32 count;
			QueryInfo *q = query_bodies(&count, p, 0.0);
			if (count > 0) {
				Handle h = rigidbody_handle(q->body);
				if (h != editor->body_h)
					memset(editor->vertex_selected, 0, sizeof(editor->vertex_selected));

				editor->body_h = h;
			}
		}

		RigidBody *body = NULL;
		if (editor->body_h != NULL_HANDLE)
			body = get_rigidbody(editor->body_h);

		if (is_edit_mode && body && state.pressed) {
			// Control vertex selection
			F64 closest_dist = 0;
			U32 closest_i = NULL_HANDLE;
			U32 vertex_count;
			V2d *vertices = rigidbody_vertices(&vertex_count, body);
			for (U32 i = 0; i < vertex_count; ++i) {
				V2d pos = v3d_to_v2d(transform_v3d(body->tf, v2d_to_v3d(vertices[i])));
				F64 dist = dist_sqr_v2d(pos, screen_to_world_point(ctx->dev.cursor_pos));
				if (closest_i == NULL_HANDLE || dist < closest_dist) {
					closest_i = i;
					closest_dist = dist;
				}
			}

			if (!ctx->dev.shift_down) {
				for (U32 i = 0; i < vertex_count; ++i)
					editor->vertex_selected[i] = false;
			}

			if (closest_dist < 100*100) {
				ensure(closest_i != NULL_HANDLE);
				toggle_bool(&editor->vertex_selected[closest_i]);
			}
		}

		// Move vertices
		if (is_edit_mode && body) {
			T3f delta;
			if (cursor_transform_delta_world(&delta, box_label, body->tf)) {
				U32 count;
				V2d *vertices = rigidbody_vertices(&count, body);
				transform_poly(vertices, editor->vertex_selected, count, delta);
				apply_rigidbody_vertices(body, vertices, count);
			}
		}

		gui_end_editorbox(ctx->gui);
	}

	// Draw shapes and vertices
	if (editor->body_h != NULL_HANDLE) {
		RigidBody *body = get_rigidbody(editor->body_h);

		Color fill_color = {0.6, 0.6, 0.8, 0.4};
		if (!is_edit_mode)
			fill_color = (Color) {1.0, 0.8, 0.5, 0.6};

		Color poly_color = fill_color;
		if (!active)
			poly_color = inactive_color();

		for (U32 i = 0; i < body->poly_count; ++i) {
			Poly poly = body->polys[i];
			V3d v[poly.v_count];
			for (U32 k = 0; k < poly.v_count; ++k) {
				v[k] = transform_v3d(body->tf, v2d_to_v3d(poly.v[k]));
			}

			ddraw_poly(poly_color, v, poly.v_count, WORLD_DEBUG_VISUAL_LAYER);
		}

		for (U32 i = 0; i < body->circle_count; ++i) {
			Circle circle = body->circles[i];
			V3d pos = transform_v3d(body->tf, v2d_to_v3d(circle.pos));
			ddraw_circle(poly_color, pos, circle.rad, WORLD_DEBUG_VISUAL_LAYER);

		}

		if (is_edit_mode) {
			U32 vertex_count;
			V2d *vertices = rigidbody_vertices(&vertex_count, body);
			ensure(vertex_count <= MAX_BODY_VERTICES);
			for (U32 i = 0; i < vertex_count; ++i) {
				V3d pos = transform_v3d(body->tf, v2d_to_v3d(vertices[i]));
				draw_mesh_vertex(	pos, editor->vertex_selected[i],
									WORLD_DEBUG_VISUAL_LAYER + 1);
			}
		}
	}
}


// Armature editor

internal
Clip *get_modifiable_clip(const char *name)
{
	return (Clip*)substitute_res(res_by_name(	g_env.resblob,
												ResType_Clip,
												name));
}

// Armature editing on world
// Returns true if editing is actively happening 
internal
bool gui_armature_overlay(ArmatureEditor *state, bool is_edit_mode)
{
	bool editing_happening = false;
	UiContext *ctx = g_env.uicontext;
	V3d cur_wp = v2d_to_v3d(screen_to_world_point(ctx->dev.cursor_pos));

	const char *box_label = "editor_overlay_box";
	EditorBoxState bstate =
		gui_begin_editorbox(ctx->gui, NULL, NULL, box_label, true);

	if (!is_edit_mode) {
		if (bstate.down)
			state->comp_h = find_compentity_at_pixel(ctx->dev.cursor_pos);
	}

	CompEntity *entity = NULL;
	if (state->comp_h != NULL_HANDLE)
		entity = get_compentity(state->comp_h);
	else
		goto exit;
	Armature *a = (Armature*)substitute_res(&entity->armature->res);
	T3d global_pose[MAX_ARMATURE_JOINT_COUNT];
	calc_global_pose(global_pose, entity);

	if (ctx->dev.toggle_select_all) {
		if (is_edit_mode) {
			bool some_selected = false;
			for (U32 i = 0; i < a->joint_count; ++i) {
				if (a->joints[i].selected)
					some_selected = true;
			}
			for (U32 i = 0; i < a->joint_count; ++i)
				a->joints[i].selected = !some_selected;
		} else {
			state->comp_h = NULL_HANDLE;
			goto exit;
		}
	}

	if (is_edit_mode) {
		for (U32 i = 0; i < a->joint_count; ++i) {
			if (!a->joints[i].selected)
				continue;

			CursorDeltaMode m = cursor_delta_mode(box_label);
			if (!m)
				continue;

			editing_happening = true;

			if (state->clip_is_bind_pose) {
				resource_modified(&a->res);
				// Modify bind pose

				T3d coords = entity->tf;
				U32 super_i = a->joints[i].super_id;
				if (super_i != NULL_JOINT_ID) {
					coords = global_pose[super_i];
					coords.pos = global_pose[i].pos; 
				}

				T3f delta;
				cursor_transform_delta_world(&delta, box_label, coords);
				V3f translation = delta.pos;
				{ // `translation` from cur pose coords to bind pose coords
					T3f to_bind = inv_t3f(entity->pose.tf[i]);
					V3f a = transform_v3f(to_bind, (V3f) {0, 0, 0});
					V3f b = transform_v3f(to_bind, translation);
					translation = sub_v3f(b, a);
				}
				delta.pos = translation;

				T3f *bind_tf = &a->joints[i].bind_pose;
				bind_tf->pos = add_v3f(delta.pos, bind_tf->pos);
				bind_tf->rot = mul_qf(delta.rot, bind_tf->rot);
				bind_tf->scale = mul_v3f(delta.scale, bind_tf->scale);
			} else {
				// Modify/create keyframe

				T3d coords = global_pose[i];
				T3f delta;
				cursor_transform_delta_world(&delta, box_label, coords);
				V3f translation = delta.pos;
				{ // Not sure what happens here. It's almost the same as bind pose modifying, but with inverse transform...
					T3f from_bind = entity->pose.tf[i];
					V3f a = transform_v3f(from_bind, (V3f) {0, 0, 0});
					V3f b = transform_v3f(from_bind, translation);
					translation = sub_v3f(b, a);
				}
				delta.pos = translation;

				const T3f base = entity->pose.tf[i];

				Clip_Key key = { .time = state->clip_time };
				fmt_str(key.joint_name, sizeof(key.joint_name), "%s", a->joint_names[i]);

				switch (m) {
				case CursorDeltaMode_scale:
					key.type = Clip_Key_Type_scale;
					key.value.scale = mul_v3f(delta.scale, base.scale);
					entity->pose.tf[i].scale = key.value.scale;
				break;
				case CursorDeltaMode_rotate:
					key.type = Clip_Key_Type_rot;
					key.value.rot = mul_qf(delta.rot, base.rot);
					entity->pose.tf[i].rot = key.value.rot;
				break;
				case CursorDeltaMode_translate:
					key.type = Clip_Key_Type_pos;
					key.value.pos = add_v3f(delta.pos, base.pos);
					entity->pose.tf[i].pos = key.value.pos;
				break;
				default: fail("Unknown CursorDeltaMode: %i", m);
				}

				Clip *clip = get_modifiable_clip(state->clip_name);
				update_rt_clip_key(clip, key);
			}
		}
	}

	if (is_edit_mode && bstate.pressed) {
		// Control joint selection
		F64 closest_dist = 0;
		U32 closest_i = NULL_HANDLE;
		for (U32 i = 0; i < a->joint_count; ++i) {
			V3d pos = global_pose[i].pos;

			F64 dist = dist_sqr_v3d(pos, cur_wp);
			if (	closest_i == NULL_HANDLE ||
					dist < closest_dist) {
				closest_i = i;
				closest_dist = dist;
			}
		}

		if (!ctx->dev.shift_down) {
			for (U32 i = 0; i < a->joint_count; ++i)
				a->joints[i].selected = false;
		}

		if (closest_dist < 2.0) {
			ensure(closest_i != NULL_HANDLE);
			toggle_bool(&a->joints[closest_i].selected);
		}
	}
	
exit:
	gui_end_editorbox(ctx->gui);

	return editing_happening;
}

void do_armature_editor(	ArmatureEditor *state,
							bool is_edit_mode,
							bool active)
{
	UiContext *ctx = g_env.uicontext;
	GuiContext *gui = ctx->gui;

	if (active) {
		bool editing_happening = gui_armature_overlay(state, is_edit_mode);

		CompEntity *entity = NULL;
		Armature *a = NULL;
		if (state->comp_h != NULL_HANDLE) {
			entity = get_compentity(state->comp_h);
			a = entity->armature;	
		}

		{ // Timeline box
			gui_begin(gui, "timeline");
			V2i px_pos, px_size;
			gui_turtle_pos(gui, &px_pos.x, &px_pos.y);
			gui_turtle_size(gui, &px_size.x, &px_size.y);

			drawcmd_px_quad(px_pos, px_size, 0.0, panel_color(), outline_color(panel_color()), gui_layer(gui));

			if (strlen(state->clip_name) == 0)
				fmt_str(state->clip_name, RES_NAME_SIZE, "%s", "bind_pose");

			U32 clip_count;
			Clip **clips = (Clip **)all_res_by_type(&clip_count,
													g_env.resblob,
													ResType_Clip);

			// Listbox containing all animation clips
			if (gui_begin_combo(gui, gui_str(gui, "clip_button+list|Clip: %s", state->clip_name))) {
				for (U32 i = 0; i < clip_count + 1; ++i) {
					const char *name = "bind_pose";
					if (i < clip_count)
						name = clips[i]->res.name;
					if (gui_combo_item(gui, name))
						fmt_str(state->clip_name, RES_NAME_SIZE, "%s", name);
				}
				gui_end_combo(gui);
			}

			state->clip_is_bind_pose = !strcmp(state->clip_name, "bind_pose");

			// "Make looping", "Delete" and "Play" -buttons
			if (!state->clip_is_bind_pose) {
				if (gui_button(gui, "clip_button+looping|Make looping")) {
					Clip *clip = get_modifiable_clip(state->clip_name);
					make_rt_clip_looping(clip);
				}

				if (	gui_button(gui, "clip_button+del|Delete key <x>")
						|| ctx->dev.delete) {

					Clip *clip = get_modifiable_clip(state->clip_name);

					// Delete all keys of selected joints at current time
					bool key_deleted;
					do {
						key_deleted = false;
						for (U32 i = 0; i < clip->key_count; ++i) {
							Clip_Key key = clip_keys(clip)[i];
							if (	key.time == state->clip_time &&
									a->joints[joint_id_by_name(a, key.joint_name)].selected) {
								delete_rt_clip_key(clip, i);
								key_deleted = true;
								break;
							}
						}
					} while (key_deleted);
				}

				if (	gui_button(gui,	gui_str(gui, "clip_button+play|%s",
							state->is_playing ? "Pause <space>" : "Play <space>"))
						|| ctx->dev.toggle_play)
					toggle_bool(&state->is_playing);
			}

			// Selected joints
			if (a && is_edit_mode) {
				gui_label(gui, "clip_button+info| Selected joints: ");
				int count = 0;
				for (U32 i = 0; i < a->joint_count; ++i) {
					if (!a->joints[i].selected)
						continue;
					if (count > 0)
						gui_label(gui, gui_str(gui, "clip_button+info_%i|, ", i));
					gui_label(gui, gui_str(gui, "clip_button+joint|%s", a->joint_names[i]));
					++count;
				}
			}

			// Interior of timeline
			px_pos.x += 10;
			px_pos.y += 27;
			px_size.x -= 20;
			px_size.y -= 27;
			Color c = darken_color(panel_color());
			drawcmd_px_quad(px_pos, px_size, 0.0, c, outline_color(c), gui_layer(gui) + 1);
			const char *clip_timeline_label = "clip_timeline";
			EditorBoxState bstate =
				gui_begin_editorbox(g_env.uicontext->gui, NULL, NULL, clip_timeline_label, true);
			if (entity && a) {
				if (state->clip_is_bind_pose) {
					entity->pose = identity_pose();
					state->is_playing = false;
				} else {
					const Clip *clip =
						(Clip*)substitute_res(res_by_name(	g_env.resblob,
															ResType_Clip,
															state->clip_name));

					if (bstate.ldown) { // LMB
						// Set time
						F64 lerp = (F64)(g_env.uicontext->dev.cursor_pos.x -
										px_pos.x)/px_size.x;
						F64 t = lerp*clip->duration;
						if (ctx->dev.snap_to_closest) {
							F64 closest_t = -1000000;
							for (U32 i = 0; i < clip->key_count; ++i) {
								F64 key_t = clip_keys(clip)[i].time;
								if (	ABS(t - key_t) <
										ABS(t - closest_t))
									closest_t = key_t;
							}
							t = closest_t;
						}
						state->clip_time = CLAMP(t, 0, clip->duration);
					}

					// Move keys
					CursorDeltaMode m = cursor_delta_mode(clip_timeline_label);
					if (m == CursorDeltaMode_translate) {
						Clip *clip = get_modifiable_clip(state->clip_name);

						T3d coords = {{1, 1, 1}, identity_qd(), {0, 0, 0}};
						T3f delta;
						cursor_transform_delta_pixels(	&delta,
														clip_timeline_label,
														coords);
						const F64 dt = clip->duration*delta.pos.x/px_size.x;
						const F64 target_t =
							CLAMP(state->clip_time + dt, 0, clip->duration);
						move_rt_clip_keys(clip, state->clip_time, target_t);
						state->clip_time = target_t;
					}

					// Show keys
					for (U32 key_i = 0; key_i < clip->key_count; ++key_i) {
						Clip_Key key = clip_keys(clip)[key_i];
						JointId joint_id = joint_id_by_name(a, key.joint_name);

						F64 lerp_x = key.time/clip->duration;
						F64 lerp_y = (F64)joint_id/clip->joint_count;
						V2i pos = {
							px_pos.x + px_size.x*lerp_x - 3,
							px_pos.y + px_size.y*lerp_y - 6 + 4*key.type
						};
						V2i size = {6, 3};

						Color color = (Color [4]) {
							{}, // none
							{1.0, 0.0, 0.0, 1.0}, // scale
							{0.0, 1.0, 0.0, 1.0}, // rot
							{0.0, 0.0, 1.0, 1.0}, // pos
						}[key.type];
						if (key.time == state->clip_time && a->joints[joint_id].selected) {
							color = (Color) {1.0, 1.0, 1.0, 1.0};
							size.y += 5;
						}
						drawcmd_px_quad(pos, size, 0.0, color, color, gui_layer(gui) + 2);
					}

					// Update animation to CompEntity when not actively editing
					// This because calculated pose doesn't exactly match
					// with keys (discretization error) and causes feedback loop
					if (state->is_playing)
						state->clip_time += g_env.device->dt;
					while (state->clip_time > clip->duration)
						state->clip_time -= clip->duration;
					if (!editing_happening)
						entity->pose = calc_clip_pose(clip, state->clip_time);

					// Show timeline cursor
					F64 lerp = state->clip_time/clip->duration;
					V2i time_cursor_pos = {
						px_pos.x + px_size.x*lerp - 1, px_pos.y
					};
					Color c = {1, 1, 0, 0.8};
					drawcmd_px_quad(time_cursor_pos, (V2i){2, px_size.y}, 0.0,
									c, c, gui_layer(gui) + 3);
				}
			}

			gui_end_editorbox(gui);
			gui_end(gui);
		}
	}

	// Draw armature
	if (state->comp_h != NULL_HANDLE){
		CompEntity *entity = get_compentity(state->comp_h);
		Armature *a = entity->armature;
		T3d global_pose[MAX_ARMATURE_JOINT_COUNT];
		calc_global_pose(global_pose, entity);

		Color default_color = {0.6, 0.6, 0.8, 0.8};
		Color selected_color = {1.0, 0.8, 0.5, 0.7};
		Color line_color = {0.0, 0.0, 0.0, 1.0};
		Color orientation_color = {1.0, 1.0, 1.0, 0.8};
		if (!is_edit_mode)
			line_color = selected_color;
		if (!active) {
			default_color = inactive_color();
			selected_color = inactive_color();
			line_color = inactive_color();
		}

		F64 rad = editor_vertex_size()*3;
		for (U32 i = 0; i < a->joint_count; ++i) {
			V3d p = global_pose[i].pos;

			const U32 v_count = 15;
			V3d v[v_count];
			for (U32 i = 0; i < v_count; ++i) {
				F64 a = i*3.141*2.0/v_count;
				v[i].x = p.x + cos(a)*rad;
				v[i].y = p.y + sin(a)*rad;
				v[i].z = 0.0;
			}

			Color c = default_color;
			if (a->joints[i].selected || !is_edit_mode)
				c = selected_color;
			ddraw_poly(c, v, v_count, WORLD_DEBUG_VISUAL_LAYER);

			V3d end_p = transform_v3d(global_pose[i], (V3d) {rad, 0, 0});
			ddraw_line(orientation_color, p, end_p, 1, WORLD_DEBUG_VISUAL_LAYER);

			if (a->joints[i].super_id != NULL_JOINT_ID) {
				ddraw_line(	line_color,
							p,
							global_pose[a->joints[i].super_id].pos,
							1,
							WORLD_DEBUG_VISUAL_LAYER);
			}
		}
	}
}

