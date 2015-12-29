#ifndef REVOLC_UI_UICONTEXT_H
#define REVOLC_UI_UICONTEXT_H

#include "build.h"
#include "gui.h"

typedef struct ButtonState {
	bool pressed, down, released;
} ButtonState;

struct Font;

Color panel_color();
Color inactive_color();
Color darken_color(Color c);
Color highlight_color(Color c);
Color outline_color(Color c);
const struct Font *gui_font();

typedef struct UiContext_Dev {
	// @todo Remove stuff duplicated respect to gui.h
	V2i cursor_pos;
	V2i prev_cursor_pos;
	V2i cursor_delta;
	ButtonState lmb;
	ButtonState rmb;
	bool shift_down;
	bool snap_to_closest;
	bool g_pressed;
	bool r_pressed;
	bool s_pressed;
	bool toggle_select_all;
	bool toggle_play;
	bool delete;

	GuiId grabbing; // Set by editor elements
	GuiId rotating; // Set by editor elements
	GuiId scaling; // Set by editor elements
} UiContext_Dev;

// @todo Rename to something like UiState -- gui is already *Context
typedef struct UiContext {
	UiContext_Dev dev;

	GuiContext *gui;
} UiContext;

// Sets g_env.uicontext
REVOLC_API void create_uicontext();
REVOLC_API void destroy_uicontext();

// Update cursor position etc.
REVOLC_API void begin_ui_frame();
REVOLC_API void end_ui_frame();

REVOLC_API bool world_has_input();

#endif // REVOLC_UI_UICONTEXT_H
