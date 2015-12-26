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

// @todo Move to ui
REVOLC_API void ogui_text(const char *text);
REVOLC_API bool ogui_button(const char *label, bool *is_down, bool *hovered);

//	if (ogui_begin_listbox("foo")) {
//		ogui_listbox_item("bar");
//		ogui_end(); // Inside if!
//	}
REVOLC_API bool ogui_begin_combobox(const char *label);
REVOLC_API bool ogui_combobox_item(const char *label);
REVOLC_API void ogui_end_combobox();

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
REVOLC_API void gui_res_info(ResType t, const Resource *res);

// Some common functionality, like scale/grab/rotate
REVOLC_API EditorBoxState gui_editorbox(	GuiContext *ctx,
											V2i *p, V2i *s,
											const char *label,
											bool invisible);

#endif // REVOLC_EDITOR_EDITOR_UTIL_H
