#ifndef REVOLC_EDITOR_EDITOR_UTIL_H
#define REVOLC_EDITOR_EDITOR_UTIL_H

#include "build.h"
#include "core/color.h"

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

// @todo Move to ui/*
REVOLC_API void ogui_wrap(V2i *p, V2i *s);
REVOLC_API Color ogui_dev_panel_color();
REVOLC_API Color inactive_color();
REVOLC_API Color darken_color(Color c);
REVOLC_API void ogui_text(const char *text);
REVOLC_API bool ogui_button(const char *label, bool *is_down, bool *hovered);

//	if (ogui_begin_listbox("foo")) {
//		ogui_listbox_item("bar");
//		ogui_end(); // Inside if!
//	}
REVOLC_API bool ogui_begin_listbox(const char *label);
REVOLC_API bool ogui_listbox_item(const char *label);

REVOLC_API F64 editor_vertex_size();

// @todo Not sure if this is a good idea. Could probably just have
//   if (ctx.dev.grabbing == ogui_id(label))
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

// Top-left box
REVOLC_API void ogui_res_info(ResType t, const Resource *res);

// Some common functionality, like scale/grab/rotate
REVOLC_API EditorBoxState ogui_editorbox(	const char *label,
											V2i px_pos,
											V2i px_size,
											bool invisible);

#endif // REVOLC_EDITOR_EDITOR_UTIL_H
