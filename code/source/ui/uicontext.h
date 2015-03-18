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
	// Editor controls
	V2i cursor_pos;
	V2i prev_cursor_pos;
	V2i cursor_delta;
	ButtonState lmb;
	ButtonState rmb;
	bool shift_down;
	bool g_pressed;

	GuiId hot_id, last_hot_id;
	GuiId active_id;
} UiContext;

// Sets g_env.uicontext
void create_uicontext();
void destroy_uicontext();

// Update cursor position etc.
void upd_uicontext();

void gui_set_hot(const char *label);
bool gui_is_hot(const char *label);

void gui_set_active(const char *label);
void gui_set_inactive(const char *label);
bool gui_is_active(const char *label);

#endif // REVOLC_UI_UICONTEXT_H
