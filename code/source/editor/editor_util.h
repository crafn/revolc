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

REVOLC_API void gui_wrap(V2i *p, V2i *s);
REVOLC_API Color inactive_color();
REVOLC_API F64 editor_vertex_size();
REVOLC_API bool cursor_transform_delta_world(	T3f *out,
												const char *label,
												T3d coords);
REVOLC_API bool cursor_transform_delta_pixels(	T3f *out,
												const char *label,
												T3d coords);
// Draws single-color quad
REVOLC_API void gui_quad(V2i pix_pos, V2i pix_size, Color c);

// Draws texture of a model
REVOLC_API void gui_model_image(	V2i pix_pos,
									V2i pix_size, ModelEntity *src_model);

// Some common functionality, like scale/grab/rotate
EditorBoxState gui_editorbox(	const char *label,
								V2i pix_pos,
								V2i pix_size,
								bool invisible);

#endif // REVOLC_EDITOR_EDITOR_UTIL_H
