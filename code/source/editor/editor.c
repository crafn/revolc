#include "armature_editor.h"
#include "core/basic.h"
#include "core/device.h"
#include "core/memory.h"
#include "editor.h"
#include "editor_util.h"
#include "global/env.h"
#include "mesh_editor.h"
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

void upd_editor()
{
	Editor *e = g_env.editor;

	if (g_env.device->key_pressed[KEY_F1])
		e->state = EditorState_mesh;
	if (g_env.device->key_pressed[KEY_F2])
		e->state = EditorState_armature;
	if (g_env.device->key_pressed[KEY_ESC])
		e->state = EditorState_invisible;

	if (e->state == EditorState_invisible)
		return;

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
