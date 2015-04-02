#ifndef REVOLC_EDITOR_EDITOR_UTIL_H
#define REVOLC_EDITOR_EDITOR_UTIL_H

#include "build.h"
#include "core/color.h"

/// @todo Remove unused fields
typedef struct EditorBoxState {
	bool hover;
	bool down;
	bool pressed;
	bool released;
} EditorBoxState;

// Creates a string which exists only this frame
REVOLC_API char * frame_str(const char *fmt, ...);

// @todo Move to ui/*
REVOLC_API void gui_wrap(V2i *p, V2i *s);
REVOLC_API Color gui_dev_panel_color();
REVOLC_API Color inactive_color();
REVOLC_API Color darken_color(Color c);
REVOLC_API void gui_text(const char *text);
REVOLC_API bool gui_button(const char *label, bool *is_down, bool *hovered);

REVOLC_API bool gui_begin_listbox(const char *label);
REVOLC_API bool gui_listbox_item(const char *label);
// void gui_end(); <- use this to end listbox

REVOLC_API F64 editor_vertex_size();
REVOLC_API bool cursor_transform_delta_world(	T3f *out,
												const char *label,
												T3d coords);
REVOLC_API bool cursor_transform_delta_pixels(	T3f *out,
												const char *label,
												T3d coords);
// Draws single-color quad
REVOLC_API void gui_quad(V2i px_pos, V2i px_size, Color c);

// Draws texture of a model
REVOLC_API void gui_model_image(	V2i px_pos,
									V2i px_size, ModelEntity *src_model);


// Top-left box
REVOLC_API void gui_res_info(ResType t, const Resource *res);

// Some common functionality, like scale/grab/rotate
REVOLC_API EditorBoxState gui_editorbox(	const char *label,
											V2i px_pos,
											V2i px_size,
											bool invisible);

#endif // REVOLC_EDITOR_EDITOR_UTIL_H
