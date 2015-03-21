#ifndef REVOLC_UI_UICONTEXT_H
#define REVOLC_UI_UICONTEXT_H

#include "build.h"

typedef struct ButtonState {
	bool pressed, down, released;
} ButtonState;

typedef U32 GuiId;

// Immediate gui context
// (see Casey Muratori's video for more info: http://mollyrocket.com/861)
typedef struct UiContext {
	struct {
		V2i cursor_pos;
		V2i prev_cursor_pos;
		V2i cursor_delta;
		ButtonState lmb;
		ButtonState rmb;
		bool shift_down;
		bool g_pressed;
		bool r_pressed;
		bool s_pressed;
		bool toggle_select_all;

		GuiId grabbing; // Set by editor elements
		GuiId rotating; // Set by editor elements
		GuiId scaling; // Set b editor elements
	} dev;

	GuiId hot_id, last_hot_id;
	GuiId active_id;
} UiContext;

// Sets g_env.uicontext
REVOLC_API void create_uicontext();
REVOLC_API void destroy_uicontext();

// Update cursor position etc.
REVOLC_API void upd_uicontext();

REVOLC_API void gui_set_hot(const char *label);
REVOLC_API bool gui_is_hot(const char *label);

REVOLC_API void gui_set_active(const char *label);
REVOLC_API void gui_set_inactive(const char *label);
REVOLC_API bool gui_is_active(const char *label);

REVOLC_API GuiId gui_id(const char *label);

#endif // REVOLC_UI_UICONTEXT_H
