#include "armature_editor.h"
#include "core/malloc.h"
#include "core/misc.h"
#include "editor.h"
#include "editor_util.h"
#include "global/env.h"
#include "mesh_editor.h"
#include "platform/device.h"
#include "ui/uicontext.h"
#include "visual/renderer.h"

internal
void editor_free_res_state()
{
	Editor *e= g_env.editor;
	free(e->stored.vertices);
	free(e->stored.indices);
	e->stored.vertices= NULL;
	e->stored.indices= NULL;
}

// Store currently selected resources for cancellation of edit
internal
void editor_store_res_state()
{
	Editor *e= g_env.editor;
	if (e->stored.vertices)
		editor_free_res_state();

	if (e->cur_model_h != NULL_HANDLE) {
		ModelEntity *m= get_modelentity(e->cur_model_h);
		Mesh *mesh= model_mesh((Model*)res_by_name(	g_env.resblob,
													ResType_Model,
													m->model_name));

		const U32 v_size= sizeof(*e->stored.vertices)*mesh->v_count;
		e->stored.vertices= dev_malloc(v_size);
		memcpy(e->stored.vertices, mesh_vertices(mesh), v_size);
		e->stored.v_count= mesh->v_count;

		const U32 i_size= sizeof(*e->stored.indices)*mesh->i_count;
		e->stored.indices= dev_malloc(i_size);
		memcpy(e->stored.indices, mesh_indices(mesh), i_size);
		e->stored.i_count= mesh->i_count;
	}

	if (e->cur_comp_h != NULL_HANDLE) {
		CompEntity *c= get_compentity(e->cur_comp_h);
		Armature *a= c->armature;

		for (U32 i= 0; i < a->joint_count; ++i)
			e->stored.bind_pose.tf[i]= a->joints[i].bind_pose;
		e->stored.joint_count= a->joint_count;
	}
}

// Revert currenty select resources to state before edit action
internal
void editor_revert_res_state()
{
	Editor *e= g_env.editor;
	if (e->stored.vertices && e->cur_model_h != NULL_HANDLE) {
		ModelEntity *m= get_modelentity(e->cur_model_h);
		Mesh *mesh= model_mesh((Model*)res_by_name(	g_env.resblob,
													ResType_Model,
													m->model_name));
		if (mesh->res.is_runtime_res) {
			// Free old mesh
			dev_free(blob_ptr(&mesh->res, mesh->v_offset));
			dev_free(blob_ptr(&mesh->res, mesh->i_offset));

			// Move stored mesh
			mesh->v_offset= blob_offset(&mesh->res, e->stored.vertices);
			mesh->i_offset= blob_offset(&mesh->res, e->stored.indices);
			mesh->v_count= e->stored.v_count;
			mesh->i_count= e->stored.i_count;
			e->stored.vertices= NULL;
			e->stored.indices= NULL;

			recache_ptrs_to_meshes();
		}
	}

	if (e->cur_comp_h != NULL_HANDLE) {
		CompEntity *c= get_compentity(e->cur_comp_h);
		Armature *a= c->armature;

		for (U32 i= 0; i < a->joint_count; ++i)
			a->joints[i].bind_pose= e->stored.bind_pose.tf[i];
		a->joint_count= e->stored.joint_count;
	}
}


void create_editor()
{
	Editor* e= zero_malloc(sizeof(*e));
	e->cur_model_h= NULL_HANDLE;
	e->cur_comp_h= NULL_HANDLE;

	g_env.editor= e;
}

void destroy_editor()
{
	editor_free_res_state();
	free(g_env.editor);
	g_env.editor= NULL;
}

void upd_editor()
{
	Editor *e= g_env.editor;

	if (g_env.device->key_pressed[KEY_F1])
		e->state= EditorState_mesh;
	if (g_env.device->key_pressed[KEY_F2])
		e->state= EditorState_armature;
	if (g_env.device->key_pressed[KEY_ESC])
		e->state= EditorState_invisible;

	if (e->state == EditorState_invisible)
		return;

	bool tab_pressed= g_env.device->key_pressed[KEY_TAB];
	if (tab_pressed)
		toggle_bool(&e->is_edit_mode);
	if (e->state == EditorState_mesh && e->cur_model_h == NULL_HANDLE)
		e->is_edit_mode= false;
	if (e->state == EditorState_armature && e->cur_comp_h == NULL_HANDLE)
		e->is_edit_mode= false;

	bool mesh_editor_active= e->state == EditorState_mesh;
	bool armature_editor_active= e->state == EditorState_armature;

	do_mesh_editor(&e->cur_model_h, &e->is_edit_mode, mesh_editor_active);
	do_armature_editor(	&e->cur_comp_h,
						&e->is_edit_mode,
						armature_editor_active);

}
