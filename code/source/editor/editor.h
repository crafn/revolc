#ifndef REVOLC_EDITOR_H
#define REVOLC_EDITOR_H

#include "build.h"

typedef enum {
	EditorState_invisible,
	EditorState_mesh,
	EditorState_armature,
} EditorState;

typedef struct Editor {
	U32 cur_model_h;
	ArmatureEditor ae_state;

	// Values restored when cancelling current action
	struct {
		TriMeshVertex *vertices;
		MeshIndexType *indices;
		U32 v_count;
		U32 i_count;

		JointPoseArray bind_pose;
		U32 joint_count;

		Clip_Key *keys;
		T3f *samples; // Sample count doesn't change (?)
		U32 key_count;
	} stored;

	bool is_edit_mode; // Edit or object mode

	EditorState state;
} Editor;

// Sets g_env.editor
REVOLC_API void create_editor();
REVOLC_API void destroy_editor();

REVOLC_API void upd_editor();

#endif // REVOLC_EDITOR_H
