#ifndef REVOLC_EDITOR_H
#define REVOLC_EDITOR_H

#include "build.h"

typedef struct Editor {
	U32 cur_model_h;
	bool is_edit_mode; // Edit or object mode
	bool grabbing;

	bool visible; 
} Editor;

// Sets g_env.editor
REVOLC_API void create_editor();
REVOLC_API void destroy_editor();

REVOLC_API void toggle_editor();
REVOLC_API void upd_editor();

#endif // REVOLC_EDITOR_H
