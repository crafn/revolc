#ifndef REVOLC_EDITOR_H
#define REVOLC_EDITOR_H

#include "build.h"

typedef enum {
	EditorState_invisible,
	EditorState_res,
	EditorState_world,
	EditorState_gui_test,
} EditorState;

typedef enum {
	EditorState_Res_mesh,
	EditorState_Res_armature,
	EditorState_Res_body,
} EditorState_Res;

typedef struct CreateCmdEditor {
	bool select_src;
	bool select_dst;
	Id src_node;
	U32 src_offset;
	U32 src_size;
	Id dst_node;
	U32 dst_offset;
	U32 dst_size;
} CreateCmdEditor;

typedef struct ArmatureEditor {
	U32 comp_h; // Initialize to NULL_HANDLE
	char clip_name[RES_NAME_SIZE];
	bool clip_is_bind_pose;
	bool is_playing;
	F64 clip_time;
} ArmatureEditor;

#define MAX_BODY_VERTICES (MAX_SHAPES_PER_BODY*(MAX_POLY_VERTEX_COUNT + 2))
typedef struct BodyEditor {
	Handle body_h; // Initialize to NULL_HANDLE
	bool vertex_selected[MAX_BODY_VERTICES];
} BodyEditor;

typedef struct Editor {
	U32 cur_model_h;
	ArmatureEditor armature_editor;
	BodyEditor body_editor;

	// Undo states
	void *mesh_state;
	void *armature_state;
	void *clip_state;
	void *bodydef_state;

	bool is_edit_mode; // Edit or object mode

	EditorState state;
	EditorState_Res res_state; // When in EditorState_res
	bool edit_layout;

	bool show_prog_state;
	bool show_node_list;
	bool show_cmd_list;

	bool show_nodegroupdef_list;
	U32 selected_nodegroupdef;

	bool show_create_cmd;
	CreateCmdEditor create_cmd;

	F32 world_time_mul;
} Editor;

// Sets g_env.editor
REVOLC_API void create_editor();
REVOLC_API void destroy_editor();

REVOLC_API void upd_editor(F64 *world_dt);

// Individual editor views

REVOLC_API void do_mesh_editor(U32 *model_h, bool *is_edit_mode, bool active);
REVOLC_API void do_armature_editor(	ArmatureEditor *state,
									bool is_edit_mode,
									bool active);
REVOLC_API void do_body_editor(BodyEditor *editor, bool is_edit_mode, bool active);

#endif // REVOLC_EDITOR_H
