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
	dev_free(e->stored.vertices);
	dev_free(e->stored.indices);
	e->stored.vertices = NULL;
	e->stored.indices = NULL;

	dev_free(e->stored.keys);
	dev_free(e->stored.samples);
	e->stored.keys = NULL;
	e->stored.samples = NULL;
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

		const U32 v_size = sizeof(*e->stored.vertices)*mesh->v_count;
		e->stored.vertices = dev_malloc(v_size);
		memcpy(e->stored.vertices, mesh_vertices(mesh), v_size);
		e->stored.v_count = mesh->v_count;

		const U32 i_size = sizeof(*e->stored.indices)*mesh->i_count;
		e->stored.indices = dev_malloc(i_size);
		memcpy(e->stored.indices, mesh_indices(mesh), i_size);
		e->stored.i_count = mesh->i_count;
	}

	if (e->ae_state.comp_h != NULL_HANDLE) {
		CompEntity *c = get_compentity(e->ae_state.comp_h);
		Armature *a = c->armature;

		for (U32 i = 0; i < a->joint_count; ++i)
			e->stored.bind_pose.tf[i] = a->joints[i].bind_pose;
		e->stored.joint_count = a->joint_count;
	}

	if (	!e->ae_state.clip_is_bind_pose &&
			e->ae_state.clip_name[0] != '\0') {
		Clip *clip =
				(Clip*)res_by_name(	g_env.resblob,
									ResType_Clip,
									e->ae_state.clip_name);
		const U32 keys_size = sizeof(*e->stored.keys)*clip->key_count;
		e->stored.keys = dev_malloc(keys_size);
		memcpy(e->stored.keys, clip_keys(clip), keys_size);
		e->stored.key_count = clip->key_count;

		const U32 samples_size =
			sizeof(*e->stored.samples)*clip_sample_count(clip);
		e->stored.samples = dev_malloc(samples_size);
		memcpy(e->stored.samples, clip_local_samples(clip), samples_size);
	}
}

// Revert currenty select resources to state before edit action
internal
void editor_revert_res_state()
{
	debug_print("Trying to revert res state");
#if 0
	Editor *e = g_env.editor;
	if (e->stored.vertices && e->cur_model_h != NULL_HANDLE) {
		ModelEntity *m = get_modelentity(e->cur_model_h);
		Mesh *mesh = model_mesh((Model*)res_by_name(	g_env.resblob,
													ResType_Model,
													m->model_name));
		if (mesh->res.is_runtime_res) {
			// Free old mesh
			FREE(leakable_dev_ator(), mesh_vertices(mesh));
			FREE(leakable_dev_ator(), mesh_indices(mesh));

			// Move stored mesh
			set_rel_ptr(&mesh->vertices, e->stored.vertices);
			set_rel_ptr(&mesh->indices, e->stored.indices);
			mesh->v_count = e->stored.v_count;
			mesh->i_count = e->stored.i_count;
			e->stored.vertices = NULL;
			e->stored.indices = NULL;

			recache_ptrs_to_meshes();
		}
	}

	if (e->ae_state.comp_h != NULL_HANDLE) {
		CompEntity *c = get_compentity(e->ae_state.comp_h);
		Armature *a = c->armature;

		for (U32 i = 0; i < a->joint_count; ++i)
			a->joints[i].bind_pose = e->stored.bind_pose.tf[i];
		a->joint_count = e->stored.joint_count;
	}

	if (e->stored.keys && e->stored.samples) {
		Clip *clip =
				(Clip*)res_by_name(	g_env.resblob,
									ResType_Clip,
									e->ae_state.clip_name);
		// Free old
		// @todo Automate
		FREE(leakable_dev_ator(), rel_ptr(&clip->keys));
		FREE(leakable_dev_ator(), rel_ptr(&clip->local_samples));

		// Move stored
		set_rel_ptr(&clip->keys, e->stored.keys);
		clip->key_count = e->stored.key_count;
		e->stored.keys = NULL;

		set_rel_ptr(&clip->local_samples, e->stored.samples);
		e->stored.samples = NULL;

		recache_ptrs_to_clips();
	}
#endif
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
