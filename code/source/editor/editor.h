#ifndef REVOLC_EDITOR_H
#define REVOLC_EDITOR_H

#include "build.h"
#include "armature_editor.h"

typedef enum {
	EditorState_invisible,
	EditorState_mesh,
	EditorState_armature,
	EditorState_world,
	EditorState_gui_test,
} EditorState;

typedef struct Editor {
	U32 cur_model_h;
	ArmatureEditor ae_state;

	// Undo states
	void *mesh_state;
	void *armature_state;
	void *clip_state;

	bool is_edit_mode; // Edit or object mode

	EditorState state;
	bool edit_layout;

	bool show_prog_state;
	bool show_node_list;
	bool show_cmd_list;

	bool show_nodegroupdef_list;
	U32 selected_nodegroupdef;

	bool show_create_cmd;
	struct {
		bool select_src;
		bool select_dst;
		Id src_node;
		U32 src_offset;
		U32 src_size;
		Id dst_node;
		U32 dst_offset;
		U32 dst_size;
	} create_cmd;
	

	F32 world_time_mul;
} Editor;

// Sets g_env.editor
REVOLC_API void create_editor();
REVOLC_API void destroy_editor();

REVOLC_API void upd_editor(F64 *world_dt);

#endif // REVOLC_EDITOR_H
