#ifndef REVOLC_EDITOR_EDITOR_UTIL_H
#define REVOLC_EDITOR_EDITOR_UTIL_H

#include "build.h"
#include "core/color.h"
#include "ui/gui.h"

/// @todo Remove unused fields
typedef struct EditorBoxState {
	// RMB
	bool hover;
	bool down;
	bool pressed;
	bool released;

	// LMB
	bool ldown;
} EditorBoxState;

REVOLC_API F64 editor_vertex_size();

// @todo Not sure if this is a good idea. Could probably just have
//   if (ctx.dev.grabbing == gui_id(label))
// instead of
//   if (cursor_delta_mode(label) == CursorDeltaMode_translate)
typedef enum {
	CursorDeltaMode_none,
	CursorDeltaMode_scale,
	CursorDeltaMode_rotate,
	CursorDeltaMode_translate,
} CursorDeltaMode;
REVOLC_API CursorDeltaMode cursor_delta_mode(const char *label);
REVOLC_API CursorDeltaMode cursor_transform_delta_world(	T3f *out,
															const char *label,
															T3d coords);
REVOLC_API CursorDeltaMode cursor_transform_delta_pixels(	T3f *out,
															const char *label,
															T3d coords);

// Some common functionality, like scale/grab/rotate
// @todo Grab etc. functionality should be in the gui lib. Then this can be substituted with simple gui_begin/end.
REVOLC_API EditorBoxState gui_begin_editorbox(	GuiContext *ctx,
												V2i *p, V2i *s,
												const char *label,
												bool invisible);
REVOLC_API void gui_end_editorbox(GuiContext *ctx);

#endif // REVOLC_EDITOR_EDITOR_UTIL_H
