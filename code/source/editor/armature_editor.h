#ifndef REVOLC_EDITOR_ARMATURE_EDITOR_H
#define REVOLC_EDITOR_ARMATURE_EDITOR_H

#include "build.h"

typedef struct ArmatureEditor {
	U32 comp_h; // Initialize to NULL_HANDLE
	char clip_name[RES_NAME_SIZE];
	bool clip_is_bind_pose;
	bool is_playing;
	F64 clip_time;
} ArmatureEditor;

REVOLC_API void do_armature_editor(	ArmatureEditor *state,
									bool is_edit_mode,
									bool active);

#endif // REVOLC_EDITOR_ARMATURE_EDITOR_H
