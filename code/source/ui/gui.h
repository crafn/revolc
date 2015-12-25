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

The library is designed to make zero memory allocations during normal runtime. It will allocate more memory in the following cases:
 - The number of gui elements increase -- this can be prevented by increasing
   GUI_DEFAULT_FRAME_MEMORY.
 - New layout rules are added. This usually is not a problem,
   because gui layout is meant to be adjusted by developers only.

Todo list
 - layouting
 - C99 -> C89 (while keeping C++ compat)
 - header-only
 - simple example
 - advanced example
 - name for the library

*/

#define GUI_DEFAULT_FRAME_MEMORY (1024*10)

#define MAX_GUI_LABEL_SIZE 256 // @todo Change to MAX_GUI_ID_SIZE which can be like 64
#define MAX_GUI_STACK_SIZE 32 // @todo Remove limit
#define MAX_GUI_WINDOW_COUNT 64 // @todo Remove limit
#define MAX_GUI_FRAME_COUNT 64 // @todo Remove limit
#define GUI_FILENAME_SIZE MAX_PATH_SIZE // @todo Remove limit

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

#include <stdint.h>
#include <stddef.h>
#if _MSC_VER
#	include <stdbool.h>
#endif

#if __cplusplus
extern "C" {
#endif

typedef uint32_t GuiId;
typedef uint32_t LayoutId;

typedef struct DragDropData {
	const char *tag; // Receiver checks this 
	int ix;
} DragDropData;

typedef struct GuiContext_Turtle {
	int pos[2]; // Output "cursor
	int size[2];
	int start_pos[2];
	int bounding_max[2];
	int last_bounding_max[2]; // Most recently added gui element
	char label[MAX_GUI_LABEL_SIZE]; // Label of the gui_begin
	int frame_ix;
	int window_ix;
	int layer; // Graphical layer
	GUI_BOOL detached; // If true, moving of this turtle doesn't affect parent bounding boxes etc.
	DragDropData inactive_dragdropdata; // This is copied to gui context when actual dragging and dropping within this turtle starts

	int scissor[4]; // Depends on window/panel/whatever pos and sizes. Given to draw commands. Zero == unused.
} GuiContext_Turtle;

// Stored data for frame elements
typedef struct GuiContext_Frame {
	GuiId id;
	int last_bounding_size[2];
	int scroll[2]; // Translation in pt. Cannot be relative, because adding content shouldn't cause translation to change.
} GuiContext_Frame;

// Stored data for window elements
typedef struct GuiContext_Window {
	GuiId id;
	GUI_BOOL used;
	GUI_BOOL used_in_last_frame;

	int frame_ix; // Corresponding GuiContext_Frame

	int bar_height;
	int pos[2]; // Top-left position
	int client_size[2]; // Size, not taking account title bar or borders

	int total_size[2]; // Value depends on client_size
} GuiContext_Window;

#define GUI_KEYSTATE_DOWN_BIT 0x1
#define GUI_KEYSTATE_PRESSED_BIT 0x2
#define GUI_KEYSTATE_RELEASED_BIT 0x4

#define GUI_WRITTEN_TEXT_BUF_SIZE 32

#define GUI_KEY_COUNT 256
#define GUI_KEY_LMB 0
#define GUI_KEY_MMB 1
#define GUI_KEY_RMB 2
#define GUI_KEY_LCTRL 3
#define GUI_KEY_0 '0'
#define GUI_KEY_1 '1'
#define GUI_KEY_2 '2'
#define GUI_KEY_3 '3'
#define GUI_KEY_4 '4'
#define GUI_KEY_5 '5'
#define GUI_KEY_6 '6'
#define GUI_KEY_7 '7'
#define GUI_KEY_8 '8'
#define GUI_KEY_9 '9'

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
	GUI_BOOL selected; // Synonym to checked, focused and active
	const char *text;
	int layer;
	GUI_BOOL has_scissor;
	int scissor_pos[2];
	int scissor_size[2];
} GuiDrawInfo;

typedef struct GuiContext_MemBucket {
	void *data;
	int size;
	int used;
} GuiContext_MemBucket;

typedef struct GuiElementLayout {
	LayoutId id;
	char str[MAX_GUI_LABEL_SIZE]; // For debugging only

	GUI_BOOL align_left, align_right;

	GUI_BOOL has_default_size;
	int default_size[2];
} GuiElementLayout;

typedef void (*CalcTextSizeFunc)(int ret[2], void *user_data, const char *text);

// Handles the gui state
typedef struct GuiContext {
	// Write to these to make gui work
	int host_win_size[2];
	int cursor_pos[2]; // Screen position, pixel coordinates
	int mouse_scroll; // Typically +1 or -1
	uint8_t key_state[GUI_KEY_COUNT];

	// Internals

	int next_window_pos[2];

	int drag_start_pos[2]; // Pixel coordinates
	GUI_BOOL dragging;
	float drag_start_value[2]; // Knob value, or xy position, or ...
	DragDropData dragdropdata; // Data from gui component which is currently dragged

	char written_text_buf[GUI_WRITTEN_TEXT_BUF_SIZE]; // Modified by gui_write_char
	int written_char_count;

	GuiContext_Turtle turtles[MAX_GUI_STACK_SIZE];
	int turtle_ix;

	GuiContext_Frame frames[MAX_GUI_FRAME_COUNT];
	int frame_count;

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

	CalcTextSizeFunc calc_text_size;
	void *calc_text_size_user_data;

	GuiDrawInfo *draw_infos;
	int draw_info_capacity;
	int draw_info_count;

	GuiElementLayout *layouts;
	int layout_capacity;
	int layout_count;
	GUI_BOOL layouts_need_sorting;
	char layout_element_label[MAX_GUI_LABEL_SIZE];

	// List of buffers which are invalidated every frame. Used for temp strings.
	GuiContext_MemBucket *framemem_buckets;
	int framemem_bucket_count; // It's best to have just one bucket, but sometimes memory usage can peak and more memory is allocated.
} GuiContext;

GUI_API const char *gui_label_text(const char *label);
GUI_API const char *gui_str(GuiContext *ctx, const char *fmt, ...); // Temporary string. These are cheap to make. Valid only this frame.

// Startup and shutdown of GUI
// @todo Size should be defined in gui_begin_window()
// @note skin_source_file contents is not copied
GUI_API GuiContext *create_gui(CalcTextSizeFunc calc_text, void *user_data_for_calc_text);
GUI_API void destroy_gui(GuiContext *ctx);

// Supply characters that should be written to e.g. text field.
// Use '\b' for erasing.
GUI_API void gui_write_char(GuiContext *ctx, char ch);

// Retrieves draw info for gui components used in the current frame
GUI_API void gui_draw_info(GuiContext *ctx, GuiDrawInfo **draw, int *draw_count);

GUI_API int gui_layer(GuiContext *ctx);

// Scrolling area
GUI_API void gui_begin_frame(GuiContext *ctx, const char *label, int x, int y, int w, int h);
GUI_API void gui_end_frame(GuiContext *ctx);
GUI_API void gui_set_frame_scroll(GuiContext *ctx, int scroll_x, int scroll_y); // Move frame contents
GUI_API void gui_frame_scroll(GuiContext *ctx, int *x, int *y);

// @todo Remove default size when layout is ready
GUI_API void gui_begin_window(GuiContext *ctx, const char *label, int default_size_x, int default_size_y);
GUI_API void gui_end_window(GuiContext *ctx);
GUI_API void gui_window_client_size(GuiContext *ctx, int *w, int *h);

GUI_API void gui_begin_panel(GuiContext *ctx, const char *label);
GUI_API void gui_end_panel(GuiContext *ctx);

GUI_API void gui_begin_contextmenu(GuiContext *ctx, const char *label);
GUI_API void gui_end_contextmenu(GuiContext *ctx);
GUI_API GUI_BOOL gui_contextmenu_item(GuiContext *ctx, const char *label);

GUI_API void gui_begin_dragdrop_src(GuiContext *ctx, DragDropData data);
GUI_API void gui_end_dragdrop_src(GuiContext *ctx);

GUI_API GUI_BOOL gui_button(GuiContext *ctx, const char *label);
GUI_API GUI_BOOL gui_selectable(GuiContext *ctx, const char *label, GUI_BOOL selected);
GUI_API GUI_BOOL gui_checkbox(GuiContext *ctx, const char *label, GUI_BOOL *value);
GUI_API GUI_BOOL gui_radiobutton(GuiContext *ctx, const char *label, GUI_BOOL value);
GUI_API void gui_slider(GuiContext *ctx, const char *label, float *value, float min, float max);
GUI_API GUI_BOOL gui_textfield(GuiContext *ctx, const char *label, char *buf, int buf_size);
GUI_API GUI_BOOL gui_intfield(GuiContext *ctx, const char *label, int *value);
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

GUI_API void gui_begin(GuiContext *ctx, const char *label);
GUI_API void gui_end(GuiContext *ctx);
GUI_API void gui_end_droppable(GuiContext *ctx, DragDropData *dropped);
GUI_API void gui_end_ex(GuiContext *ctx, GUI_BOOL make_zero_size, DragDropData *dropped);

GUI_API void gui_set_next_window_pos(GuiContext *ctx, int x, int y);
GUI_API void gui_set_turtle_pos(GuiContext *ctx, int x, int y);
GUI_API void gui_turtle_pos(GuiContext *ctx, int *x, int *y);
GUI_API void gui_next_row(GuiContext *ctx); // @todo Remove
GUI_API void gui_next_col(GuiContext *ctx); // @todo Remove
GUI_API void gui_enlarge_bounding(GuiContext *ctx, int x, int y);

// @todo Remove
GUI_API void gui_ver_space(GuiContext *ctx);
GUI_API void gui_hor_space(GuiContext *ctx);

// Shows a window which allows manipulating gui layout at runtime
GUI_API void gui_layout_settings(GuiContext *ctx);

#if __cplusplus
}
#endif

#endif
