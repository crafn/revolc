#ifndef GUILIB_H
#define GUILIB_H

/*

Non-bloaty immediate mode gui library in C.
This should be self-contained, so include and use only standard library.

Common voluntary options to be defined before including this file, or in this file.

#define GUI_BOOL <type>
#define GUI_API <declspec>
#define GUI_MALLOC <func>
#define GUI_REALLOC <func>
#define GUI_FREE <func>
#define GUI_PRINTF <func>
#define GUI_ASSERT <func>

The library is designed to make zero memory allocations during normal runtime. It will allocate more memory in the following cases:
 - The number of gui elements increase -- this can be prevented by increasing
   GUI_DEFAULT_FRAME_MEMORY (temp memory used for e.g. strings) or
   GUI_DEFAULT_STORAGE_CAPACITY (persistent memory used for e.g. tree open/closed state) or
   GUI_DEFAULT_ELEMENT_CAPACITY (number of elements per frame)
 - New layout rules are added. This usually is not a problem,
   because gui layout is meant to be adjusted & saved before shipping.

Todo list
 - GuiContext_* -> Gui_*
 - C99 -> C89 (while keeping C++ compat)
 - header-only
 - gui demo
 - custom element example
 - name for the library
 - check/rename functions (prefix gui_)
 - check/rename datatypes (prefix Gui)
 - format comments of api to end of lines
*/

#define GUI_DEFAULT_FRAME_MEMORY (1024*5)
#define GUI_DEFAULT_STORAGE_CAPACITY (128)
#define GUI_DEFAULT_ELEMENT_CAPACITY (128)

#define MAX_GUI_LABEL_SIZE 256 // @todo Remove limit. Use single buffer for all stored strings.
#define MAX_GUI_STACK_SIZE 32 // @todo Remove limit
#define MAX_GUI_WINDOW_COUNT 128 // @todo Remove limit
#define GUI_FILENAME_SIZE MAX_PATH_SIZE // @todo Remove limit
#define MAX_LAYOUTS_PER_ELEMENT 8

#ifndef GUI_API
#	define GUI_API
#endif

#ifndef GUI_BOOL
#	if __cplusplus || _MSC_VER
#		define GUI_BOOL bool
#	else
#		define GUI_BOOL unsigned char
#	endif
#endif

#ifndef GUI_MALLOC
#	define GUI_MALLOC malloc
#endif 

#ifndef GUI_REALLOC
#	define GUI_REALLOC realloc
#endif

#ifndef GUI_FREE
#	define GUI_FREE free
#endif

#ifndef GUI_PRINTF
#	define GUI_PRINTF printf
#endif

#ifndef GUI_ASSERT
#	define GUI_ASSERT assert
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>

#if _MSC_VER
#	include <stdbool.h>
#endif

#if __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER) && _MSC_VER <= 1800 // MSVC 2013
size_t gui_v_sprintf_impl(char *buf, size_t count, const char *fmt, va_list args);
void gui_sprintf_impl(char *buf, size_t count, const char *fmt, ...);
#	define GUI_FMT_STR gui_sprintf_impl
#	define GUI_V_FMT_STR gui_v_sprintf_impl
#else
#	define GUI_FMT_STR snprintf
#	define GUI_V_FMT_STR vsnprintf
#endif

typedef uint32_t GuiId;
#define NULL_GUI_ID 0

typedef struct DragDropData {
	const char *tag; // Receiver checks this 
	int ix;
} DragDropData;

// Arrays like scissor[4] are indexed as [0] == x, [1] == y, [2] == w, [3] == h
// or like padding[4] as [0] == left, [1] == top, [2] == right, [3] == bottom

// @todo Rename to GuiContext_Frame or something
typedef struct GuiContext_Turtle {
	int manual_offset[2]; // Manual offset of child elements
	int pos[2]; // Copied from layout
	int size[2]; // Copied from layout
	char label[MAX_GUI_LABEL_SIZE]; // Label of the gui_begin
	int window_ix;
	int element_ix;
	int layer; // Graphical layer
	int scissor[4]; // Depends on window/panel/whatever pos and sizes. Given to draw commands. Zero == unused.

	int padding[4]; // Copied from layout
	int gap[2]; // Copied from layout
} GuiContext_Turtle;

// Stored data for window elements
// @todo Rename to GuiPanel
typedef struct GuiContext_Window {
	GuiId id;
	char label[MAX_GUI_LABEL_SIZE];
	GUI_BOOL used;
	GUI_BOOL used_in_last_frame;
	GUI_BOOL remove_when_not_used;
	GUI_BOOL minimized;

	GUI_BOOL has_bar; // GUI_FALSE is equivalent to panel-type window
	int bar_height;
	int recorded_pos[2]; // Last position of upper left corner @todo Redundant
	// Size on screen, not taking account title bar or borders or scroll bars
	// Depends on window size in layout
	int client_size[2];

	int slider_width[2];
} GuiContext_Window;

// @todo Maybe we should require only up/down status of buttons from the client?
#define GUI_KEYSTATE_DOWN_BIT 0x1
#define GUI_KEYSTATE_PRESSED_BIT 0x2
#define GUI_KEYSTATE_RELEASED_BIT 0x4

#define GUI_WRITTEN_TEXT_BUF_SIZE 32 // Max keypresses per frame
#define GUI_TEXTFIELD_BUF_SIZE 64 // For integer and float edit

#define GUI_KEY_COUNT 256
#define GUI_KEY_LMB 0
#define GUI_KEY_MMB 1
#define GUI_KEY_RMB 2
#define GUI_KEY_LCTRL 3

typedef enum GuiDrawInfo_Type {
	GuiDrawInfo_button,
	GuiDrawInfo_checkbox,
	GuiDrawInfo_radiobutton,
	GuiDrawInfo_textbox,
	GuiDrawInfo_text,
	GuiDrawInfo_panel,
	GuiDrawInfo_title_bar,
	GuiDrawInfo_resize_handle,
	GuiDrawInfo_slider,
	GuiDrawInfo_slider_handle
} GuiDrawInfo_Type;

typedef struct GuiDrawInfo {
	GuiDrawInfo_Type type;
	int pos[2];
	int size[2];
	GUI_BOOL hovered;
	GUI_BOOL held;
	GUI_BOOL selected; // Synonym for checked, focused and active
	const char *text;
	int layer;
	GUI_BOOL has_scissor;
	int scissor_pos[2];
	int scissor_size[2];

	// Internal

	int element_ix; // For hacking draw commands after solving the layout
} GuiDrawInfo;

typedef struct GuiContext_MemBucket {
	void *data;
	int size;
	int used;
} GuiContext_MemBucket;

// @todo Rename LayoutProperty -> Prop
typedef struct GuiContext_LayoutProperty {
	GuiId layout_id;
	GuiId key_id;
	int value;
	GUI_BOOL dont_save;

	// @todo These could be in separate arrays to minimize duplicated strings
	char layout_name[MAX_GUI_LABEL_SIZE];
	char key_name[MAX_GUI_LABEL_SIZE];
} GuiContext_LayoutProperty;

// @todo Replace with temp LayoutProperty
typedef struct GuiContext_Storage {
	GuiId id;
	GUI_BOOL bool_value;
} GuiContext_Storage;

typedef void (*CalcTextSizeFunc)(int ret[2], void *user_data, const char *text);

// Record of elements for post-frame layout solving
typedef struct GuiContext_Element {
	char label[MAX_GUI_LABEL_SIZE]; // @todo To separate array to minimize duplicated strings
	int manual_offset[2]; // Set by client code. Manual offset of this element.
	int depth;
	GUI_BOOL detached; // Doesn't affect parent size

	// Solver state
	GUI_BOOL min_solved;
	int solved_min_pos[2];
	int solved_min_size[2];
	int solved_min_content_size[2]; // Same as min_size if element is not forced to be smaller than content

	GUI_BOOL final_solved;
	int solved_pos[2];
	int solved_size[2];
	int solved_content_size[2]; // Can be smaller or larger than solved_size
	GUI_BOOL solved_needs_scroll[2];
	int solved_pos_delta[2]; // Compared to prev frame
} GuiContext_Element;

typedef struct Gui_Tbl_Entry {
	GuiId key;
	int value;
} Gui_Tbl_Entry;

typedef struct Gui_Tbl {
	struct Gui_Tbl_Entry *array;
	int array_size;
	int count;
	GuiId null_key;
	int null_value;
} Gui_Tbl;

GUI_API Gui_Tbl gui_create_tbl(int null_value, int expected_item_count);
GUI_API void gui_destroy_tbl(Gui_Tbl *tbl);
GUI_API int gui_get_tbl(Gui_Tbl *tbl, GuiId key);
GUI_API void gui_set_tbl(Gui_Tbl *tbl, GuiId key, int value);
GUI_API void gui_clear_tbl(Gui_Tbl *tbl);

// Handles the gui state
typedef struct GuiContext {
	// Write to these to make gui work
	int host_win_size[2];
	int cursor_pos[2]; // Screen position, pixel coordinates
	int mouse_scroll; // Typically +1 or -1
	uint8_t key_state[GUI_KEY_COUNT];
	int base_layer; // Smallest draw layer
	GUI_BOOL allow_next_window_outside;
	GUI_BOOL create_next_window_minimized;
	GUI_BOOL dont_save_next_window_layout;

	// Internals

	int drag_start_pos[2]; // Pixel coordinates
	GUI_BOOL dragging;
	float drag_start_value[2]; // Knob value, or xy position, or ...

	char written_text_buf[GUI_WRITTEN_TEXT_BUF_SIZE]; // Modified by gui_write_char
	int written_char_count;

	GuiContext_Turtle turtles[MAX_GUI_STACK_SIZE];
	int turtle_ix;

	GuiContext_Window windows[MAX_GUI_WINDOW_COUNT];
	int window_order[MAX_GUI_WINDOW_COUNT];
	int focused_win_ix; // Not necessarily window_order[window_count - 1] because nothing is focused when clicking background
	int window_count;

	// This is used at mouse input and render dimensions. All gui calculations are done in pt.
	float dpi_scale; // 1.0: pixels == points, 2.0: pixels == 2.0*points (gui size is doubled). 

	GuiId hot_id, last_hot_id;
	int hot_layer;
	GuiId active_id, last_active_id;
	char active_label[MAX_GUI_LABEL_SIZE];
	int active_win_ix;

	GuiId interacted_id; // Set for one frame on button release etc.

	GUI_BOOL has_input; // Writing in textfield or something
	GuiId open_combo_id;
	GuiId open_contextmenu_id;

	CalcTextSizeFunc calc_text_size;
	void *calc_text_size_user_data;

	GuiDrawInfo *draw_infos;
	int draw_info_capacity;
	int draw_info_count;

	Gui_Tbl prop_ix_tbl;
	GuiContext_LayoutProperty *layout_props;
	int layout_props_capacity;
	int layout_props_count;
	char layout_element_label[MAX_GUI_LABEL_SIZE];

	GuiContext_Element *elements;
	int element_capacity;
	int element_count;

	// Misc element state storage
	GuiContext_Storage *storage; // Kept in order
	int storage_capacity;
	int storage_count;

	// Used for int and float text fields
	char textfield_buf[GUI_TEXTFIELD_BUF_SIZE];
	GuiId textfield_buf_owner;

	// List of buffers which are invalidated every frame. Used for temp strings.
	GuiContext_MemBucket *framemem_buckets;
	int framemem_bucket_count; // It's best to have just one bucket, but sometimes memory usage can peak and more memory is allocated.
} GuiContext;

GUI_API const char *gui_label_text(const char *label);
GUI_API const char *gui_str(GuiContext *ctx, const char *fmt, ...); // Temporary string. These are cheap to make. Valid only this frame.

// Startup and shutdown of GUI
GUI_API GuiContext *create_gui(CalcTextSizeFunc calc_text, void *user_data_for_calc_text);
GUI_API void destroy_gui(GuiContext *ctx);
GUI_API void gui_pre_frame(GuiContext *ctx);
GUI_API void gui_post_frame(GuiContext *ctx);

GUI_API GUI_BOOL gui_has_input(GuiContext *ctx);

// Supply characters that should be written to e.g. text field.
// Use '\b' for erasing.
GUI_API void gui_write_char(GuiContext *ctx, char ch);

// Retrieves draw info for gui components used in the current frame
GUI_API void gui_draw_info(GuiContext *ctx, GuiDrawInfo **draw, int *draw_count);

GUI_API int gui_layer(GuiContext *ctx);

GUI_API void gui_set_scroll(GuiContext *ctx, int scroll_x, int scroll_y); // Move window contents
GUI_API void gui_scroll(GuiContext *ctx, int *x, int *y);

GUI_API void gui_begin_window(GuiContext *ctx, const char *win_label);
GUI_API void gui_end_window(GuiContext *ctx);
GUI_API void gui_window_client_size(GuiContext *ctx, int *w, int *h);
GUI_API void gui_window_pos(GuiContext *ctx, int *x, int *y);
GUI_API GUI_BOOL gui_is_window_open(GuiContext *ctx);

GUI_API void gui_begin_panel(GuiContext *ctx, const char *win_label);
GUI_API void gui_end_panel(GuiContext *ctx);

GUI_API GUI_BOOL gui_begin_contextmenu(GuiContext *ctx, const char *label, GuiId element_id);
GUI_API void gui_end_contextmenu(GuiContext *ctx);
GUI_API void gui_close_contextmenu(GuiContext *ctx);
GUI_API GUI_BOOL gui_contextmenu_item(GuiContext *ctx, const char *label);

// Secondary way to detect interactions with preceeding gui elements. True on button release etc.
// @todo Implement for all elements and/or think of stricter semantics
GUI_API GUI_BOOL gui_interacted(GuiContext *ctx, GuiId element_id);

GUI_API GUI_BOOL gui_button(GuiContext *ctx, const char *label);
GUI_API GUI_BOOL gui_selectable(GuiContext *ctx, const char *label, GUI_BOOL selected);
GUI_API GUI_BOOL gui_checkbox(GuiContext *ctx, const char *label, GUI_BOOL *value);
GUI_API GUI_BOOL gui_radiobutton(GuiContext *ctx, const char *label, GUI_BOOL value);
GUI_API GUI_BOOL gui_slider(GuiContext *ctx, const char *label, float *value, float min, float max);
GUI_API GUI_BOOL gui_slider_double(GuiContext *ctx, const char *label, double *value, double min, double max);
GUI_API GUI_BOOL gui_textfield(GuiContext *ctx, const char *label, char *buf, int buf_size);
// @todo Rename gui_property_int etc.
GUI_API GUI_BOOL gui_intfield(GuiContext *ctx, const char *label, int *value);
GUI_API GUI_BOOL gui_doublefield(GuiContext *ctx, const char *label, double *value);
GUI_API GUI_BOOL gui_floatfield(GuiContext *ctx, const char *label, float *value);
GUI_API void gui_label(GuiContext *ctx, const char *label);

GUI_API void gui_begin_listbox(GuiContext *ctx, const char *label);
GUI_API void gui_end_listbox(GuiContext *ctx);

/* Usage:
if (gui_begin_combo(ctx, "label")) {
	gui_combo_item(ctx, "foo");
	gui_combo_item(ctx, "bar");
	gui_end_combo(ctx);
} */
GUI_API GUI_BOOL gui_begin_combo(GuiContext *ctx, const char *label);
GUI_API GUI_BOOL gui_combo_item(GuiContext *ctx, const char *label);
GUI_API void gui_end_combo(GuiContext *ctx);

GUI_API GUI_BOOL gui_begin_tree(GuiContext *ctx, const char *label);
GUI_API void gui_end_tree(GuiContext *ctx);

// Shows a window which allows editing gui layout at runtime
GUI_API void gui_layout_editor(GuiContext *ctx, const char *save_path);


//
// Tools for creating custom gui elements
//

GUI_API void gui_begin(GuiContext *ctx, const char *label);
GUI_API void gui_begin_detached(GuiContext *ctx, const char *label);
GUI_API void gui_end(GuiContext *ctx);
GUI_API void gui_end_droppable(GuiContext *ctx, DragDropData *dropped);
GUI_API void gui_end_ex(GuiContext *ctx, DragDropData *dropped);

GUI_API GuiId gui_id(const char *label);

GUI_API void gui_set_hot(GuiContext *ctx, const char *label);
GUI_API GUI_BOOL gui_is_hot(GuiContext *ctx, const char *label);
GUI_API void gui_set_active(GuiContext *ctx, const char *label);
GUI_API void gui_set_inactive(GuiContext *ctx, GuiId id);
GUI_API GUI_BOOL gui_is_active(GuiContext *ctx, const char *label);

GUI_API void gui_set_turtle_pos(GuiContext *ctx, int x, int y); // @todo rename
GUI_API void gui_turtle_pos(GuiContext *ctx, int *x, int *y);
GUI_API void gui_turtle_size(GuiContext *ctx, int *x, int *y);
GUI_API int gui_turtle_layer(GuiContext *ctx);
GUI_API void gui_turtle_add_layer(GuiContext *ctx, int delta);

//
// Internal
//

void gui_update_layout_property(GuiContext *ctx, const char *label, const char *key, int value);
void gui_append_layout_property(GuiContext *ctx, const char *label, const char *key, int value, GUI_BOOL saved);

#if __cplusplus
}
#endif

#endif
