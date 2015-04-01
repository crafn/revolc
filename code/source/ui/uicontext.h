#ifndef REVOLC_UI_UICONTEXT_H
#define REVOLC_UI_UICONTEXT_H

#include "build.h"

typedef struct ButtonState {
	bool pressed, down, released;
} ButtonState;

typedef U32 GuiId;

typedef struct UiContext_Turtle {
	V2i pos; // Output "cursor
	V2i last_adv_size;
	V2i dir; // Direction which `turtle` will move
} UiContext_Turtle;

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

	// Stack of turtles
	UiContext_Turtle turtles[MAX_GUI_STACK_SIZE];
	U32 turtle_i;

	GuiId hot_id, last_hot_id;
	GuiId active_id;
} UiContext;

// Sets g_env.uicontext
REVOLC_API void create_uicontext();
REVOLC_API void destroy_uicontext();

// Update cursor position etc.
REVOLC_API void upd_uicontext();

// Next gui element will be put in this position
REVOLC_API void gui_set_turtle_pos(V2i pos);
REVOLC_API V2i gui_turtle_pos();
// Move turtle according to current layout
REVOLC_API void gui_advance_turtle(V2i element_size);
REVOLC_API V2i gui_last_adv_size();

REVOLC_API void gui_begin(V2i turtle_dir);
REVOLC_API void gui_end();

REVOLC_API void gui_set_hot(const char *label);
REVOLC_API bool gui_is_hot(const char *label);

REVOLC_API void gui_set_active(const char *label);
REVOLC_API void gui_set_inactive(const char *label);
REVOLC_API bool gui_is_active(const char *label);

REVOLC_API GuiId gui_id(const char *label);

#endif // REVOLC_UI_UICONTEXT_H
