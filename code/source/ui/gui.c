#include "gui.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

static void *gui_frame_alloc(GuiContext *ctx, int size);

const int gui_zero_v2i[2] = {0};

#if defined(_MSC_VER) && _MSC_VER <= 1800 // MSVC 2013
size_t gui_v_sprintf_impl(char *buf, size_t count, const char *fmt, va_list args)
{
	size_t ret = _vsnprintf(buf, count, fmt, args);
	// Fix unsafeness of msvc _vsnprintf
	if (buf && count > 0)
		buf[count - 1] = '\0';
	return ret;
}

void gui_sprintf_impl(char *buf, size_t count, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	v_sprintf_impl(buf, count, fmt, args);
	va_end(args);
}
#endif

#define GUI_MAX(a, b) ((a > b) ? (a) : (b))
#define GUI_MIN(a, b) ((a < b) ? (a) : (b))
#define GUI_CLAMP(v, a, b) (GUI_MIN(GUI_MAX((v), (a)), (b)))
#define GUI_ABS(x) ((x) < 0 ? (-x) : x)
#define GUI_LAYERS_PER_WINDOW 10000 // Maybe 10k layers inside a window is enough
#define GUI_UNUSED(x) (void)(x) // For unused params. C doesn't allow omitting param name.
#define GUI_TRUE 1
#define GUI_FALSE 0

#define GUI_NONE_WINDOW_IX (-2)
#define GUI_BG_WINDOW_IX (-1)

static void *check_ptr(void *ptr)
{
	if (!ptr) {
		abort();
	}
	return ptr;
}

#define GUI_DECL_V2(type, name, x, y) type name[2]; name[0] = x; name[1] = y;
#define GUI_V2(stmt) do { int c = 0; stmt; c = 1; stmt; } while(0)
#define GUI_ASSIGN_V2(a, b) GUI_V2((a)[c] = (b)[c])
#define GUI_EQUALS_V2(a, b) ((a)[0] == (b)[0] && (a)[1] == (b)[1])
#define GUI_ZERO(var) memset(&var, 0, sizeof(var))

static GUI_BOOL v2i_in_rect(int v[2], int pos[2], int size[2])
{ return v[0] >= pos[0] && v[1] >= pos[1] && v[0] < pos[0] + size[0] && v[1] < pos[1] + size[1]; }

static GuiId gui_hash(const char *buf, int size)
{
	// Modified FNV-1a
	uint32_t hash = 2166136261;
	for (int i = 0; i < size; ++i)
		hash = ((hash ^ buf[i]) + 379721) * 16777619;
	return hash;
}

GuiId gui_id(const char *label)
{
	int id_size = 0;
	// gui_id("foo_button|Press this") == gui_id("foo_button|Don't press this")
	while (label[id_size] && label[id_size] != '|')
		++id_size;
	return gui_hash(label, id_size);
}

static LayoutId layout_id(const char *label)
{
	int layout_size = 0;
	while (	label[layout_size] &&
			label[layout_size] != '+' && label[layout_size] != '|')
		++layout_size;
	return gui_hash(label, layout_size);
}


static int layout_cmp(const void *void_a, const void *void_b)
{
	const GuiElementLayout *a = void_a;
	const GuiElementLayout *b = void_b;
	return (a->id > b->id) - (a->id < b->id);
}

static GuiElementLayout element_layout(GuiContext *ctx, const char *label)
{
	if (ctx->layouts_need_sorting) {
		qsort(ctx->layouts, ctx->layout_count, sizeof(*ctx->layouts), layout_cmp);
		ctx->layouts_need_sorting = GUI_FALSE;
	}

	// @todo Consider using hashmap
	GuiElementLayout key;
	key.id = layout_id(label);
	GuiElementLayout *found =
		bsearch(&key, ctx->layouts, ctx->layout_count, sizeof(*ctx->layouts), layout_cmp);
	if (found)
		return *found;

	// Default layout
	GuiElementLayout def;
	GUI_ZERO(def);
	def.id = layout_id(label);
	def.has_size = GUI_TRUE;
	// @todo Different defaults for different element types
	def.size[0] = 100;
	def.size[1] = 20;
	GUI_FMT_STR(def.str, sizeof(def.str), "%s", label);
	return def;
}

void append_element_layout(GuiContext *ctx, GuiElementLayout layout)
{
	if (ctx->layout_count == ctx->layout_capacity) {
		// Need more space
		ctx->layout_capacity *= 2;
		ctx->layouts =
			GUI_REALLOC(ctx->layouts, sizeof(*ctx->layouts)*ctx->layout_capacity);
	}

	ctx->layouts[ctx->layout_count++] = layout;
	ctx->layouts_need_sorting = GUI_TRUE;
}

static void update_element_layout(GuiContext *ctx, GuiElementLayout layout)
{
	// This is rare/dev-only operation, so linear search is ok
	for (int i = 0; i < ctx->layout_count; ++i) {
		GuiElementLayout l = ctx->layouts[i];
		if (l.id != layout.id)
			continue;
		ctx->layouts[i] = layout;
		return;
	}

	// New layout, append and mark unsorted
	append_element_layout(ctx, layout);
}

static void save_layout(GuiContext *ctx, const char *path)
{
	FILE *file = fopen(path, "wb");
	if (!file) {
		GUI_PRINTF("save_layout: can't open file '%s'\n", path);
		return;
	}
	fprintf(file, "void load_layout(GuiContext *ctx)\n");
	fprintf(file, "{\n");
	fprintf(file, "\tctx->layout_count = 0;\n");

	for (int i = 0; i < ctx->layout_count; ++i) {
		GuiElementLayout l = ctx->layouts[i];

		fprintf(file, "\t{\n");
		fprintf(file, "\t\tGuiElementLayout l = {0};\n");
		fprintf(file, "\t\tl.id = %u;\n", l.id);
		fprintf(file, "\t\tGUI_FMT_STR(l.str, sizeof(l.str), \"%%s\", \"%s\");\n",
			l.str);
		fprintf(file, "\t\tl.has_offset = %i;\n", l.has_offset);
		fprintf(file, "\t\tl.offset[0] = %i;\n", l.offset[0]);
		fprintf(file, "\t\tl.offset[1] = %i;\n", l.offset[1]);

		fprintf(file, "\t\tl.has_size = %i;\n", l.has_size);
		fprintf(file, "\t\tl.size[0] = %i;\n", l.size[0]);
		fprintf(file, "\t\tl.size[1] = %i;\n", l.size[1]);

		fprintf(file, "\t\tl.prevent_resizing = %i;\n", l.prevent_resizing);

		fprintf(file, "\t\tl.align_left = %i;\n", l.align_left);
		fprintf(file, "\t\tl.align_right = %i;\n", l.align_right);
		fprintf(file, "\t\tl.align_top = %i;\n", l.align_top);
		fprintf(file, "\t\tl.align_bottom = %i;\n", l.align_bottom);

		fprintf(file, "\t\tappend_element_layout(ctx, l);\n");

		fprintf(file, "\t}\n\n");
	}

	fprintf(file, "}\n");
	fclose(file);
}

static GuiContext_Turtle *gui_turtle(GuiContext *ctx)
{
	return &ctx->turtles[ctx->turtle_ix];
}

void gui_set_hot(GuiContext *ctx, const char *label)
{
	if (ctx->active_id == 0) {
		GUI_BOOL set_hot = GUI_FALSE;
		if (ctx->hot_id == 0) {
			set_hot = GUI_TRUE;
		} else {
			// Last overlapping element of the topmost window gets to be hot
			if (ctx->hot_layer <= gui_turtle(ctx)->layer)
				set_hot = GUI_TRUE;
		}
		if (set_hot) {
			ctx->hot_id = gui_id(label);
			ctx->hot_layer = gui_turtle(ctx)->layer;
		}
	}
}

GUI_BOOL gui_is_hot(GuiContext *ctx, const char *label)
{
	return ctx->last_hot_id == gui_id(label);
}

void gui_set_active(GuiContext *ctx, const char *label)
{
	ctx->active_id = gui_id(label);
	GUI_FMT_STR(ctx->active_label, sizeof(ctx->active_label), "%s", label);
	ctx->last_active_id = ctx->active_id;
	ctx->active_win_ix = gui_turtle(ctx)->window_ix;
	ctx->focused_win_ix = gui_turtle(ctx)->window_ix;
	ctx->hot_id = 0; // Prevent the case where hot becomes assigned some different (overlapping) element than active
}

void gui_set_inactive(GuiContext *ctx, GuiId id)
{
	if (ctx->active_id == id) {
		ctx->active_id = 0;
		ctx->active_win_ix = GUI_NONE_WINDOW_IX;
		ctx->has_input = GUI_FALSE;
	}
}

GUI_BOOL gui_is_active(GuiContext *ctx, const char *label)
{
	return ctx->active_id == gui_id(label);
}

static void destroy_window(GuiContext *ctx, int handle)
{
	GUI_BOOL found = GUI_FALSE;
	for (int i = 0; i < ctx->window_count; ++i) {
		if (!found) {
			if (ctx->window_order[i] == handle)
				found = GUI_TRUE;
		} else {
			ctx->window_order[i - 1] = ctx->window_order[i];
		}
	}
	--ctx->window_count;

	GuiContext_Window *win = &ctx->windows[handle];

	GUI_ZERO(*win);
}

int gui_window_order(GuiContext *ctx, int handle)
{
	for (int i = 0; i < ctx->window_count; ++i) {
		if (ctx->window_order[i] == handle)
			return i;
	}
	return -1;
}

void pt_to_px(int px[2], int pt[2], float dpi_scale)
{
	px[0] = (int)floorf(pt[0]*dpi_scale + 0.5f);
	px[1] = (int)floorf(pt[1]*dpi_scale + 0.5f);
}

float pt_to_px_f32(float pt, float dpi_scale)
{
	return pt*dpi_scale;
}

void px_to_pt(int pt[2], int px[2], float dpi_scale)
{
	pt[0] = (int)floorf(px[0] / dpi_scale + 0.5f);
	pt[1] = (int)floorf(px[1] / dpi_scale + 0.5f);
}

// Labels can contain prefixed stuff (to keep them unique for example).
// "hoopoa|asdfg" -> "asdfg".
const char *gui_label_text(const char *label)
{
	int i = 0;
	while (label[i]) {
		if (label[i] == '|')
			return label + i + 1;
		++i;
	}
	return label;
}

const char *gui_str(GuiContext *ctx, const char *fmt, ...)
{
	char *text = NULL;
	va_list args;
	va_list args_copy;

	va_start(args, fmt);
	va_copy(args_copy, args);
	int size = (int)GUI_V_FMT_STR(NULL, 0, fmt, args) + 1;
	text = (char*)gui_frame_alloc(ctx, size);
	GUI_V_FMT_STR(text, size, fmt, args_copy);
	va_end(args_copy);
	va_end(args);
	return text;
}

void gui_set_turtle_pos(GuiContext *ctx, int x, int y)
{
	ctx->turtles[ctx->turtle_ix].pos[0] = x;
	ctx->turtles[ctx->turtle_ix].pos[1] = y;
	gui_enlarge_bounding(ctx, x, y);
}

GuiContext_Window *gui_window(GuiContext *ctx)
{
	if (gui_turtle(ctx)->window_ix == GUI_NONE_WINDOW_IX ||
		gui_turtle(ctx)->window_ix == GUI_BG_WINDOW_IX)
		return NULL;
	assert(gui_turtle(ctx)->window_ix < ctx->window_count);
	return gui_turtle(ctx)->window_ix >= 0 ? &ctx->windows[gui_turtle(ctx)->window_ix] : NULL;
}

void gui_win_dimension(GuiContext *ctx, int pos[2], int size[2], GuiContext_Window *win)
{
	GuiElementLayout layout = element_layout(ctx, win->label);
	GUI_ASSIGN_V2(pos, layout.offset);
	GUI_ASSIGN_V2(size, layout.size);
}

// @note Change of focus is delayed by one frame (similar to activation)
GUI_BOOL gui_focused(GuiContext *ctx)
{
	if (ctx->window_count == 0)
		return GUI_TRUE;
	return gui_turtle(ctx)->window_ix == ctx->focused_win_ix;
}

void gui_turtle_pos(GuiContext *ctx, int *x, int *y)
{
	*x = gui_turtle(ctx)->pos[0];
	*y = gui_turtle(ctx)->pos[1];
}

void gui_turtle_size(GuiContext *ctx, int *w, int *h)
{
	*w = gui_turtle(ctx)->size[0];
	*h = gui_turtle(ctx)->size[1];
}

void gui_parent_turtle_start_pos(GuiContext *ctx, int pos[2])
{
	if (ctx->turtle_ix == 0) {
		pos[0] = pos[1] = 0;
	} else {
		GUI_ASSIGN_V2(pos, ctx->turtles[ctx->turtle_ix - 1].start_pos);
	}
}

int *gui_scissor(GuiContext *ctx)
{
	int *s = gui_turtle(ctx)->scissor;
	GUI_BOOL width_is_zero = (s[3] == 0);
	return width_is_zero ? NULL : s;
}

// Returns whether the current turtle with certain size is at least partially visible in the client area of the current window
GUI_BOOL gui_is_inside_window(GuiContext *ctx, int size[2])
{
	int pos[2];
	gui_turtle_pos(ctx, &pos[0], &pos[1]);
	GuiContext_Window *win = gui_window(ctx);
	int win_pos[2];
	int win_size[2];
	gui_win_dimension(ctx, win_pos, win_size, win);
	if (!win)
		return GUI_TRUE; // @todo Use background size
	if (pos[0] + size[0] <= win_pos[0] || pos[1] + size[1] <= win_pos[1] + win->bar_height)
		return GUI_FALSE;
	if (pos[0] >= win_pos[0] + win_size[0] || pos[1] >= win_pos[1] + win_size[1])
		return GUI_FALSE;
	return GUI_TRUE;
}

void gui_start_dragging(GuiContext *ctx, float start_value[2])
{
	assert(!ctx->dragging);
	ctx->dragging = GUI_TRUE;
	GUI_ASSIGN_V2(ctx->drag_start_pos, ctx->cursor_pos);
	GUI_ASSIGN_V2(ctx->drag_start_value, start_value);
	// Store dragdropdata
	ctx->dragdropdata = gui_turtle(ctx)->inactive_dragdropdata;
}

static void *gui_frame_alloc(GuiContext *ctx, int size)
{
	assert(ctx->framemem_bucket_count >= 1);
	GuiContext_MemBucket *bucket = &ctx->framemem_buckets[ctx->framemem_bucket_count - 1];
	if (bucket->used + size > bucket->size) {
		// Need a new bucket :(
		int new_bucket_count = ctx->framemem_bucket_count + 1;
		ctx->framemem_buckets = (GuiContext_MemBucket*)check_ptr(realloc(ctx->framemem_buckets, sizeof(*ctx->framemem_buckets)*new_bucket_count));

		int bucket_size = GUI_MAX(size, bucket->size * 2);
		bucket = &ctx->framemem_buckets[ctx->framemem_bucket_count++];
		bucket->data = check_ptr(malloc(bucket_size));
		bucket->size = bucket_size;
		bucket->used = 0;
	}

	char *mem = (char *)bucket->data + bucket->used; // @todo Alignment
	bucket->used += size;
	assert(bucket->used <= bucket->size);
	return (void*)mem;
}

// Resize and clean frame memory
static void refresh_framemem(GuiContext *ctx)
{
	if (ctx->framemem_bucket_count > 1) { // Merge buckets to one for next frame
		int memory_size = ctx->framemem_buckets[0].size;
		for (int i = 1; i < ctx->framemem_bucket_count; ++i) {
			memory_size += ctx->framemem_buckets[i].size;
			GUI_FREE(ctx->framemem_buckets[i].data);
		}

		ctx->framemem_buckets = (GuiContext_MemBucket*)check_ptr(realloc(ctx->framemem_buckets, sizeof(*ctx->framemem_buckets)));
		ctx->framemem_buckets[0].data = check_ptr(realloc(ctx->framemem_buckets[0].data, memory_size));
		ctx->framemem_buckets[0].size = memory_size;

		ctx->framemem_bucket_count = 1;
	}

	ctx->framemem_buckets[0].used = 0;
}

static void gui_draw(	GuiContext *ctx, GuiDrawInfo_Type type, int pos[2], int size[2],
						GUI_BOOL hovered, GUI_BOOL held, GUI_BOOL selected,
						const char *text,
						int layer,
						int scissor[4])
{
	GuiDrawInfo info;
	GUI_ZERO(info);
	info.type = type;
	GUI_ASSIGN_V2(info.pos, pos);
	GUI_ASSIGN_V2(info.size, size);
	info.hovered = hovered;
	info.held = held;
	info.selected = selected;
	if (text) {
		// Copy text -- don't take pointers to host data
		int text_size = strlen(text) + 1;
		char *text_copy = gui_frame_alloc(ctx, text_size);
		GUI_FMT_STR(text_copy, text_size, "%s", text);
		info.text = text_copy;
	}
	info.layer = layer;
	if (scissor != NULL) {
		info.has_scissor = GUI_TRUE;
		info.scissor_pos[0] = scissor[0];
		info.scissor_pos[1] = scissor[1];
		info.scissor_size[0] = scissor[2];
		info.scissor_size[1] = scissor[3];
	}

	if (ctx->draw_info_count == ctx->draw_info_capacity) {
		// Need more space
		ctx->draw_info_capacity *= 2;
		GuiDrawInfo *new_infos = gui_frame_alloc(ctx, sizeof(*new_infos)*ctx->draw_info_capacity);
		memcpy(new_infos, ctx->draw_infos, sizeof(*new_infos)*ctx->draw_info_count);
		ctx->draw_infos = new_infos;
	}

	ctx->draw_infos[ctx->draw_info_count++] = info;
}

GuiContext *create_gui(CalcTextSizeFunc calc_text, void *user_data_for_calc_text)
{
	GuiContext *ctx = (GuiContext*)check_ptr(GUI_MALLOC(sizeof(*ctx)));
	GUI_ZERO(*ctx);
	ctx->dpi_scale = 1.0f;
	ctx->calc_text_size = calc_text;
	ctx->calc_text_size_user_data = user_data_for_calc_text;
	ctx->hot_layer = -1;
	ctx->active_win_ix = GUI_NONE_WINDOW_IX;
	ctx->focused_win_ix = -1;

	// "Null" turtle
	gui_turtle(ctx)->window_ix = GUI_BG_WINDOW_IX;

	ctx->draw_info_capacity = 64;

	ctx->layout_capacity = 64;
	ctx->layouts = check_ptr(GUI_MALLOC(sizeof(*ctx->layouts)*ctx->layout_capacity));

	ctx->framemem_bucket_count = 1;
	ctx->framemem_buckets = (GuiContext_MemBucket*)check_ptr(GUI_MALLOC(sizeof(*ctx->framemem_buckets)));
	ctx->framemem_buckets[0].data = check_ptr(GUI_MALLOC(GUI_DEFAULT_FRAME_MEMORY));
	ctx->framemem_buckets[0].size = GUI_DEFAULT_FRAME_MEMORY;
	ctx->framemem_buckets[0].used = 0;

	{ // Default layout
		{
			GuiElementLayout layout = element_layout(ctx, "gui_slider");
			layout.has_size = GUI_TRUE;
			layout.size[0] = 15;
			layout.size[1] = 15;
			update_element_layout(ctx, layout);
		}

		{
			GuiElementLayout layout = element_layout(ctx, "gui_bar");
			layout.has_size = GUI_TRUE;
			layout.size[0] = 0;
			layout.size[1] = 25;
			update_element_layout(ctx, layout);
		}

		{
			GuiElementLayout layout = element_layout(ctx, "gui_layoutwin");
			layout.has_size = GUI_TRUE;
			layout.size[0] = 200;
			layout.size[1] = 500;
			update_element_layout(ctx, layout);
		}

		{
			GuiElementLayout layout = element_layout(ctx, "gui_layout_list");
			layout.align_left = GUI_TRUE;
			layout.align_right = GUI_TRUE;
			update_element_layout(ctx, layout);
		}
	}

	return ctx;
}

void destroy_gui(GuiContext *ctx)
{
	if (ctx)
	{
		for (int i = 0; i < ctx->framemem_bucket_count; ++i)
			GUI_FREE(ctx->framemem_buckets[i].data);
		GUI_FREE(ctx->framemem_buckets);

		GUI_FREE(ctx->layouts);

		for (int i = MAX_GUI_WINDOW_COUNT - 1; i >= 0; --i) {
			if (ctx->windows[i].id)
				destroy_window(ctx, i);
		}

		GUI_FREE(ctx);
	}
}

void gui_pre_frame(GuiContext *ctx)
{
	assert(ctx->turtle_ix == 0);

	refresh_framemem(ctx);

	ctx->draw_infos =
		gui_frame_alloc(ctx, sizeof(*ctx->draw_infos)*ctx->draw_info_capacity);
	ctx->draw_info_count = 0;

	GUI_ASSIGN_V2(gui_turtle(ctx)->size, ctx->host_win_size);
}

void gui_post_frame(GuiContext *ctx)
{
	assert(ctx->turtle_ix == 0);
	for (int i = 0; i < MAX_GUI_WINDOW_COUNT; ++i) {
		if (!ctx->windows[i].id)
			continue;

		if (!ctx->windows[i].used) {
			// Hide closed windows - don't destroy. Position etc. must be preserved.
			//destroy_window(ctx, i);

			if (ctx->active_win_ix == i) {
				// Stop interacting with an element in hidden window
				gui_set_inactive(ctx, ctx->active_id);
			}
		}

		ctx->windows[i].used_in_last_frame = ctx->windows[i].used;
		ctx->windows[i].used = GUI_FALSE;
	}

	ctx->mouse_scroll = 0;
	ctx->last_hot_id = ctx->hot_id;
	ctx->hot_id = 0;
	ctx->written_char_count = 0;
}

GUI_BOOL gui_has_input(GuiContext *ctx)
{ return ctx->has_input; }

void gui_write_char(GuiContext *ctx, char ch)
{
	if (ctx->written_char_count >= (int)sizeof(ctx->written_text_buf))
		return;
	ctx->written_text_buf[ctx->written_char_count++] = ch;
}

void gui_draw_info(GuiContext *ctx, GuiDrawInfo **draw, int *draw_count)
{
	*draw = ctx->draw_infos;
	*draw_count = ctx->draw_info_count;
}

int gui_layer(GuiContext *ctx) { return gui_turtle(ctx)->layer; }

void gui_button_logic(GuiContext *ctx, const char *label, int pos[2], int size[2], GUI_BOOL *went_up, GUI_BOOL *went_down, GUI_BOOL *down, GUI_BOOL *hover)
{
	if (went_up) *went_up = GUI_FALSE;
	if (went_down) *went_down = GUI_FALSE;
	if (down) *down = GUI_FALSE;
	if (hover) *hover = GUI_FALSE;

	GUI_BOOL was_released = GUI_FALSE;
	if (gui_is_active(ctx, label)) {
		if (ctx->key_state[GUI_KEY_LMB] & GUI_KEYSTATE_DOWN_BIT) {
			if (down) *down = GUI_TRUE;
		} else if (ctx->key_state[GUI_KEY_LMB] & GUI_KEYSTATE_RELEASED_BIT) {
			if (went_up) *went_up = GUI_TRUE;
			gui_set_inactive(ctx, gui_id(label));
			was_released = GUI_TRUE;
		}

		// For layout editor
		if (ctx->key_state[GUI_KEY_MMB] & GUI_KEYSTATE_RELEASED_BIT)
			gui_set_inactive(ctx, gui_id(label));
	} else if (gui_is_hot(ctx, label)) {
		if (ctx->key_state[GUI_KEY_LMB] & GUI_KEYSTATE_PRESSED_BIT) {
			if (down) *down = GUI_TRUE;
			if (went_down) *went_down = GUI_TRUE;
			gui_set_active(ctx, label);
		}

		// For layout editor
		if (ctx->key_state[GUI_KEY_MMB] & GUI_KEYSTATE_PRESSED_BIT)
			gui_set_active(ctx, label);
	}

	int c_p[2];
	px_to_pt(c_p, ctx->cursor_pos, ctx->dpi_scale);
	int *s = gui_scissor(ctx);
	if (	v2i_in_rect(c_p, pos, size) &&
			(!s || v2i_in_rect(c_p, &s[0], &s[2]))) {
		gui_set_hot(ctx, label);
		if (hover && (gui_is_hot(ctx, label) || gui_is_active(ctx, label) || was_released))
			*hover = GUI_TRUE;
	}
}

void gui_begin(GuiContext *ctx, const char *label)
{
	assert(ctx->turtle_ix < MAX_GUI_STACK_SIZE);
	if (ctx->turtle_ix >= MAX_GUI_STACK_SIZE)
		ctx->turtle_ix = 0; // Failsafe

	GuiElementLayout layout = element_layout(ctx, label);
	//GuiContext_Window *win = gui_window(ctx);

	GuiContext_Turtle *prev = &ctx->turtles[ctx->turtle_ix];
	GuiContext_Turtle *cur = &ctx->turtles[++ctx->turtle_ix];

	GuiContext_Turtle new_turtle;
	GUI_ZERO(new_turtle);
	{ // Init pos and size using layout
		GUI_ASSIGN_V2(new_turtle.pos, prev->pos);
		int prev_size[2];
		GUI_ASSIGN_V2(prev_size, prev->size);
		// @todo Make work
		//if (win) {
			// Make right-aligned go to right side of content, not right side of win
		//	GUI_V2(prev_size[c] = MAX(prev_size[c], win->last_bounding_size[c]));
		//}

		if (layout.has_offset)
			GUI_V2(new_turtle.pos[c] += layout.offset[c]);

		if (layout.has_size)
			GUI_ASSIGN_V2(new_turtle.size, layout.size);

		if (layout.align_left)
			new_turtle.pos[0] = prev->pos[0];
		if (layout.align_right) {
			if (layout.align_left) {
				// Stretch
				new_turtle.size[0] = prev_size[0];
			} else {
				// Move
				new_turtle.pos[0] = prev->pos[0] + prev_size[0] - new_turtle.size[0];
			}
		}

		if (layout.align_top)
			new_turtle.pos[1] = prev->pos[1];
		if (layout.align_bottom) {
			if (layout.align_top) {
				// Stretch
				new_turtle.size[1] = prev_size[1];
			} else {
				// Move
				new_turtle.pos[1] = prev->pos[1] + prev_size[1] - new_turtle.size[1];
			}
		}
	}
	GUI_ASSIGN_V2(new_turtle.start_pos, new_turtle.pos);
	// Element will report how much space it took, regardless of turtle.size
	GUI_V2(new_turtle.bounding_max[c] = new_turtle.start_pos[c]);
	GUI_ASSIGN_V2(new_turtle.last_bounding_max, new_turtle.bounding_max);
	new_turtle.window_ix = prev->window_ix;
	new_turtle.layer = prev->layer + 1;
	new_turtle.detached = GUI_FALSE;
	new_turtle.inactive_dragdropdata = prev->inactive_dragdropdata;
	memcpy(new_turtle.scissor, prev->scissor, sizeof(new_turtle.scissor));
	GUI_FMT_STR(new_turtle.label, MAX_GUI_LABEL_SIZE, "%s", label);
	*cur = new_turtle;
}

void gui_end(GuiContext *ctx)
{
	gui_end_ex(ctx, GUI_FALSE, NULL);
}

void gui_end_droppable(GuiContext *ctx, DragDropData *dropdata)
{
	gui_end_ex(ctx, GUI_FALSE, dropdata);
}

void gui_end_ex(GuiContext *ctx, GUI_BOOL make_zero_size, DragDropData *dropdata)
{
	// Initialize outputs
	if (dropdata) {
		GUI_ZERO(*dropdata);
	}

	{ // Dragging stop logic
		if (	!ctx->key_state[GUI_KEY_LMB] & GUI_KEYSTATE_DOWN_BIT || // Mouse released
				(ctx->key_state[GUI_KEY_LCTRL] & GUI_KEYSTATE_DOWN_BIT && ctx->mouse_scroll)) { // Scrolling when xy dragging
			if (ctx->dragdropdata.tag && dropdata) {
				int pos[2], size[2], c_p[2];
				GUI_ASSIGN_V2(pos, gui_turtle(ctx)->start_pos);
				GUI_V2(size[c] = gui_turtle(ctx)->bounding_max[c] - pos[c]);
				px_to_pt(c_p, ctx->cursor_pos, ctx->dpi_scale);
				if (v2i_in_rect(c_p, pos, size)) {
					// Release dragdropdata
					*dropdata = ctx->dragdropdata;
					GUI_ZERO(ctx->dragdropdata);
				}
			}

			// This doesn't need to be here. Once per frame would be enough.
			ctx->dragging = GUI_FALSE;
		}
	}

	GUI_BOOL detached = make_zero_size || gui_turtle(ctx)->detached;

	assert(ctx->turtle_ix > 0);
	--ctx->turtle_ix;

	if (!detached) {
		// Merge bounding boxes
		GuiContext_Turtle *parent = &ctx->turtles[ctx->turtle_ix];
		GuiContext_Turtle *child = &ctx->turtles[ctx->turtle_ix + 1];

		GUI_V2(parent->bounding_max[c] =
			GUI_MAX(parent->bounding_max[c], child->bounding_max[c]));
		GUI_ASSIGN_V2(parent->last_bounding_max, child->bounding_max);
	}
}

void gui_slider_ex(GuiContext *ctx, const char *label, float *value, float min, float max, float handle_rel_size, GUI_BOOL v, int length)
{
	gui_begin(ctx, label);

	int *pos = gui_turtle(ctx)->pos;
	int *size = gui_turtle(ctx)->size;

	if (length > 0)
		size[v] = length;

	const int scroll_handle_height = GUI_MAX((int)(handle_rel_size*size[v]), 10);

	GUI_BOOL went_down, down, hover;
	gui_button_logic(ctx, label, pos, size, NULL, &went_down, &down, &hover);

	if (went_down) {
		GUI_DECL_V2(float, tmp, *value, 0);
		gui_start_dragging(ctx, tmp);
	}

	if (down && ctx->dragging) {
		int px_delta = ctx->cursor_pos[v] - ctx->drag_start_pos[v];
		*value = ctx->drag_start_value[0] + 1.f*px_delta / (size[v] - scroll_handle_height) *(max - min);
	}
	*value = GUI_CLAMP(*value, min, max);

	{ // Draw
		{ // Bg
			int px_pos[2], px_size[2];
			pt_to_px(px_pos, pos, ctx->dpi_scale);
			pt_to_px(px_size, size, ctx->dpi_scale);
			gui_draw(	ctx, GuiDrawInfo_slider, px_pos, px_size, GUI_FALSE, GUI_FALSE, GUI_FALSE,
						NULL, gui_layer(ctx), gui_scissor(ctx));
		}

		{ // Handle
			float rel_scroll = (*value - min) / (max - min);
			int handle_pos[2];
			GUI_ASSIGN_V2(handle_pos, pos);
			handle_pos[v] += (int)(rel_scroll*(size[v] - scroll_handle_height));
			int handle_size[2];
			GUI_ASSIGN_V2(handle_size, size);
			handle_size[v] = scroll_handle_height;

			int px_pos[2], px_size[2];
			pt_to_px(px_pos, handle_pos, ctx->dpi_scale);
			pt_to_px(px_size, handle_size, ctx->dpi_scale);
			gui_draw(	ctx, GuiDrawInfo_slider_handle, px_pos, px_size, hover, down, GUI_FALSE,
						NULL, gui_layer(ctx) + 1, gui_scissor(ctx));
		}
	}

	gui_enlarge_bounding(ctx, pos[0] + size[0], pos[1] + size[1]);

	gui_end(ctx);
}

void gui_set_scroll(GuiContext *ctx, int scroll_x, int scroll_y)
{
	gui_window(ctx)->scroll[0] = scroll_x;
	gui_window(ctx)->scroll[1] = scroll_y;
}

void gui_scroll(GuiContext *ctx, int *x, int *y)
{
	*x = gui_window(ctx)->scroll[0];
	*y = gui_window(ctx)->scroll[1];
}

void gui_begin_window_ex(GuiContext *ctx, const char *label, GUI_BOOL panel)
{
	GuiElementLayout layout = element_layout(ctx, label);
	GuiElementLayout slider_layout = element_layout(ctx, "gui_slider");
	GuiElementLayout bar_layout = element_layout(ctx, "gui_bar");

	gui_begin(ctx, label);
	gui_turtle(ctx)->detached = GUI_TRUE;
	int *pos = gui_turtle(ctx)->pos;
	int *size = gui_turtle(ctx)->size;

	int min_size = panel ? 10 : 80;

	GUI_V2(size[c] = MAX(size[c], min_size));

	int win_handle = -1;
	{ // Find/create window
		int free_handle = -1;
		for (int i = 0; i < MAX_GUI_WINDOW_COUNT; ++i) {
			if (ctx->windows[i].id == 0 && free_handle == -1)
				free_handle = i;
			if (ctx->windows[i].id == gui_id(label)) {
				win_handle = i;
				break;
			}
		}
		if (win_handle == -1) {
			assert(free_handle >= 0);
			// Create new window
			assert(ctx->window_count < MAX_GUI_WINDOW_COUNT);

			GuiContext_Window *win = &ctx->windows[free_handle];
			win->id = gui_id(label);
			GUI_FMT_STR(win->label, sizeof(win->label), "%s", label);
			win->has_bar = !panel;
			ctx->window_order[ctx->window_count++] = free_handle;

			win_handle = free_handle;
		}
	}
	assert(win_handle >= 0);
	GuiContext_Window *win = &ctx->windows[win_handle];
	assert(!win->used && "Same window used twice in a frame");
	win->used = GUI_TRUE;
	if (!win->used_in_last_frame)
		ctx->focused_win_ix = win_handle; // Appearing window will be focused

	gui_turtle(ctx)->window_ix = win_handle;
	gui_turtle(ctx)->layer = 1337 + gui_window_order(ctx, win_handle)*GUI_LAYERS_PER_WINDOW;
	win->bar_height = win->has_bar ? bar_layout.size[1] : 0;

	{ // Ordinary gui element logic
		GUI_V2(win->client_size[c] = size[c] - c*win->bar_height);

		// Title bar logic
		char bar_label[MAX_GUI_LABEL_SIZE];
		GUI_FMT_STR(bar_label, sizeof(bar_label), "gui_bar+win_%s", label);
		GUI_BOOL went_down, down, hover;
		GUI_DECL_V2(int, btn_size, size[0], win->bar_height);
		gui_button_logic(ctx, bar_label, pos, btn_size, NULL, &went_down, &down, &hover);

		if (ctx->active_win_ix == win_handle) {
			// Lift window to top
			GUI_BOOL found = GUI_FALSE;
			for (int i = 0; i < ctx->window_count; ++i) {
				if (!found) {
					if (ctx->window_order[i] == win_handle)
						found = GUI_TRUE;
				} else {
					ctx->window_order[i - 1] = ctx->window_order[i];
				}
			}
			ctx->window_order[ctx->window_count - 1] = win_handle;
		}

		if (went_down) {
			float v[2];
			GUI_V2(v[c] = (float)pos[c]);
			gui_start_dragging(ctx, v);
		}

		if (down && ctx->dragging)
			GUI_V2(pos[c] = (int)ctx->drag_start_value[c] - ctx->drag_start_pos[c] + ctx->cursor_pos[c]);

		const int margin = 20;
		pos[0] = GUI_CLAMP(pos[0], margin - size[0], ctx->host_win_size[0] - margin);
		pos[1] = GUI_CLAMP(pos[1], 0, ctx->host_win_size[1] - margin);

		int px_pos[2], px_size[2];
		pt_to_px(px_pos, pos, ctx->dpi_scale);
		pt_to_px(px_size, size, ctx->dpi_scale);
		gui_draw(	ctx, GuiDrawInfo_panel, px_pos, px_size, GUI_FALSE, GUI_FALSE, gui_focused(ctx),
					NULL, gui_layer(ctx), gui_scissor(ctx));
		if (!panel) {
			px_size[1] = win->bar_height;
			gui_draw(	ctx, GuiDrawInfo_title_bar, px_pos, px_size, GUI_FALSE, GUI_FALSE, gui_focused(ctx),
						gui_label_text(label), gui_layer(ctx) + 1, gui_scissor(ctx));
		}
	}

	//
	// Client-area content
	//

	gui_begin(ctx, gui_str(ctx, "winclient_%s", label));

	int *c_pos = gui_turtle(ctx)->pos;
	int *c_size = gui_turtle(ctx)->size;
	GUI_V2(c_pos[c] = pos[c] + c*win->bar_height);
	GUI_V2(c_size[c] = size[c] - c*win->bar_height);
	GUI_V2(gui_turtle(ctx)->start_pos[c] = c_pos[c]);

	// Make clicking frame backgound change last active element, so that scrolling works
	gui_button_logic(ctx, label, c_pos, c_size, NULL, NULL, NULL, NULL);

	{ // Scrolling
		int max_scroll[2];
		GUI_V2(max_scroll[c] = win->last_bounding_size[c] - c_size[c]);
		GUI_V2(max_scroll[c] = GUI_MAX(max_scroll[c], 0));

		char scroll_panel_label[MAX_GUI_LABEL_SIZE];
		GUI_FMT_STR(scroll_panel_label, sizeof(scroll_panel_label), "winscrollpanel_%s", label);
		gui_begin(ctx, scroll_panel_label);
		gui_turtle(ctx)->detached = GUI_TRUE; // Detach so that the scroll doesn't take part in window contents size
		gui_turtle(ctx)->layer += GUI_LAYERS_PER_WINDOW/2; // Make topmost in window @todo Then should move this to end_window
		for (int d = 0; d < 2; ++d) {
			if (win->needs_scroll[d]) {
				char scroll_label[MAX_GUI_LABEL_SIZE];
				GUI_FMT_STR(scroll_label, sizeof(scroll_label), "gui_slider+win_%i_%s", d, label);
				GUI_ASSIGN_V2(gui_turtle(ctx)->pos, c_pos);
				gui_turtle(ctx)->pos[!d] += c_size[!d] - slider_layout.size[!d];

				if (	d == 1 && // Vertical
						gui_focused(ctx) &&
						ctx->mouse_scroll != 0 && // Scrolling mouse wheel
						!(ctx->key_state[GUI_KEY_LCTRL] & GUI_KEYSTATE_DOWN_BIT)) // Not holding ctrl
					win->scroll[d] -= ctx->mouse_scroll*64;

				// Scroll by dragging
				if (	gui_turtle(ctx)->window_ix == ctx->active_win_ix &&
						(ctx->key_state[GUI_KEY_LCTRL] & GUI_KEYSTATE_DOWN_BIT) &&
						(ctx->key_state[GUI_KEY_LMB] & GUI_KEYSTATE_DOWN_BIT)) {
					if (!ctx->dragging) {
						float v[2];
						GUI_V2(v[c] = (float)win->scroll[c]);
						gui_start_dragging(ctx, v);
					} else {
						int v[2];
						GUI_V2(v[c] = (int)ctx->drag_start_value[c]);
						win->scroll[d] = v[d] + ctx->drag_start_pos[d] - ctx->cursor_pos[d];
					}
				}

				float scroll = 1.f*win->scroll[d];
				float rel_shown_area = 1.f*c_size[d]/win->last_bounding_size[d];
				float max_comp_scroll = 1.f*max_scroll[d];
				gui_slider_ex(ctx, scroll_label, &scroll, 0, max_comp_scroll, rel_shown_area, !!d, c_size[d] - slider_layout.size[d]);
				win->scroll[d] = (int)scroll;
			}
		}
		gui_end(ctx);

		GUI_V2(win->scroll[c] = GUI_CLAMP(win->scroll[c], 0, max_scroll[c]));
	}

	// Corner resize handle
	if (!layout.prevent_resizing) {
		char resize_label[MAX_GUI_LABEL_SIZE];
		GUI_FMT_STR(resize_label, sizeof(resize_label), "gui_slider+resize_%s", label);
		gui_begin(ctx, resize_label);
		gui_turtle(ctx)->detached = GUI_TRUE; // Detach so that the handle doesn't take part in window contents size
		gui_turtle(ctx)->layer += GUI_LAYERS_PER_WINDOW/2; // Make topmost in window @todo Then should move this to end_window

		int handle_size[2] = {slider_layout.size[0], slider_layout.size[1]};
		int handle_pos[2];
		GUI_V2(handle_pos[c] = c_pos[c] + c_size[c] - handle_size[c]);
		GUI_BOOL went_down, down, hover;
		gui_button_logic(ctx, resize_label, handle_pos, handle_size, NULL, &went_down, &down, &hover);

		if (went_down) {
			float v[2];
			GUI_V2(v[c] = (float)size[c]);
			gui_start_dragging(ctx, v);
		}

		if (down)
			GUI_V2(size[c] = (int)ctx->drag_start_value[c] + ctx->cursor_pos[c] - ctx->drag_start_pos[c]);
		GUI_V2(size[c] = GUI_MAX(size[c], min_size));

		int px_pos[2], px_size[2];
		pt_to_px(px_pos, handle_pos, ctx->dpi_scale);
		pt_to_px(px_size, handle_size, ctx->dpi_scale);
		gui_draw(	ctx, GuiDrawInfo_resize_handle, px_pos, px_size, hover, down, GUI_FALSE,
					NULL, gui_layer(ctx), gui_scissor(ctx));
		gui_end(ctx);
	}	


	int scissor[4];
	scissor[0] = c_pos[0];
	scissor[1] = c_pos[1];
	scissor[2] = c_size[0];
	scissor[3] = c_size[1];
	memcpy(gui_turtle(ctx)->scissor, scissor, sizeof(scissor));

	{
		// Scroll client area
		int client_start_pos[2];
		GUI_V2(client_start_pos[c] = c_pos[c] - win->scroll[c]);
		GUI_ASSIGN_V2(gui_turtle(ctx)->start_pos, client_start_pos);
		GUI_ASSIGN_V2(gui_turtle(ctx)->pos, client_start_pos);
	}

	// Save window pos and size to layout
	layout.has_offset = GUI_TRUE;
	layout.has_size = GUI_TRUE;
	GUI_ASSIGN_V2(layout.offset, pos);
	GUI_ASSIGN_V2(layout.size, size);
	update_element_layout(ctx, layout);
}

void gui_end_window_ex(GuiContext *ctx)
{
	GuiContext_Turtle *turtle = gui_turtle(ctx);
	GuiContext_Window *win = gui_window(ctx);
	GUI_V2(win->last_bounding_size[c] = turtle->bounding_max[c] - turtle->start_pos[c]);
	GUI_V2(win->needs_scroll[c] = turtle->size[c] < win->last_bounding_size[c]);
	gui_end(ctx);
	gui_end(ctx);
}

void gui_begin_window(GuiContext *ctx, const char *label)
{
	gui_begin_window_ex(ctx, label, GUI_FALSE);
}

void gui_end_window(GuiContext *ctx)
{
	gui_end_window_ex(ctx);
}

void gui_begin_panel(GuiContext *ctx, const char *label)
{
	gui_begin_window_ex(ctx, label, GUI_TRUE);
}

void gui_end_panel(GuiContext *ctx)
{
	gui_end_window_ex(ctx);
}

void gui_window_client_size(GuiContext *ctx, int *w, int *h)
{
	*w = gui_window(ctx)->client_size[0];
	*h = gui_window(ctx)->client_size[1];
}

void gui_begin_contextmenu(GuiContext *ctx, const char *label)
{
	gui_begin_window_ex(ctx, label, GUI_FALSE);
}

void gui_end_contextmenu(GuiContext *ctx)
{
	gui_end_window_ex(ctx);
}

GUI_BOOL gui_contextmenu_item(GuiContext *ctx, const char *label)
{
	GUI_BOOL ret = gui_button(ctx, label);
	gui_next_row(ctx);
	return ret;
}

void gui_begin_dragdrop_src(GuiContext *ctx, DragDropData data)
{
	assert(gui_turtle(ctx)->inactive_dragdropdata.tag == NULL && "Nested dragdrop areas in single gui element not supported");
	gui_turtle(ctx)->inactive_dragdropdata = data;
}

void gui_end_dragdrop_src(GuiContext *ctx)
{
	assert(gui_turtle(ctx)->inactive_dragdropdata.tag);
	GUI_ZERO(gui_turtle(ctx)->inactive_dragdropdata);
}

GUI_BOOL gui_button_ex(GuiContext *ctx, const char *label, GUI_BOOL force_down)
{
	gui_begin(ctx, label);
	int margin[2] = {5, 3};
	int *pos = gui_turtle(ctx)->pos;
	int *size = gui_turtle(ctx)->size;

	// @todo Recalc size only when text changes
	int text_size[2] = {0};
	ctx->calc_text_size(text_size, ctx->calc_text_size_user_data, gui_label_text(label));
	GUI_V2(size[c] = GUI_MAX((int)text_size[c] + margin[c] * 2, size[c]));

	GUI_BOOL went_up = GUI_FALSE, hover = GUI_FALSE, down = GUI_FALSE;
	if (gui_is_inside_window(ctx, size)) {
		gui_button_logic(ctx, label, pos, size, &went_up, NULL, &down, &hover);

		int px_pos[2], px_size[2];
		pt_to_px(px_pos, pos, ctx->dpi_scale);
		pt_to_px(px_size, size, ctx->dpi_scale);
		gui_draw(	ctx, GuiDrawInfo_button, px_pos, px_size, hover, down || force_down, GUI_FALSE,
					NULL, gui_layer(ctx), gui_scissor(ctx));

		int px_margin[2];
		pt_to_px(px_margin, margin, ctx->dpi_scale);
		gui_draw(	ctx, GuiDrawInfo_text, px_pos, px_size, GUI_FALSE, GUI_FALSE, GUI_FALSE,
					gui_label_text(label), gui_layer(ctx) + 1, gui_scissor(ctx));
	}

	gui_enlarge_bounding(ctx, pos[0] + size[0], pos[1] + size[1]);
	gui_end(ctx);

	gui_next_row(ctx);

	return went_up && hover;
}

GUI_BOOL gui_button(GuiContext *ctx, const char *label)
{ return gui_button_ex(ctx, label, GUI_FALSE); }

GUI_BOOL gui_selectable(GuiContext *ctx, const char *label, GUI_BOOL selected)
{ return gui_button_ex(ctx, label, selected); }

GUI_BOOL gui_checkbox_ex(GuiContext *ctx, const char *label, GUI_BOOL *value, GUI_BOOL radio_button_visual)
{
	gui_begin(ctx, label);
	int margin[2] = {5, 3};
	int *pos = gui_turtle(ctx)->pos;
	int *size = gui_turtle(ctx)->size;

	int text_size[2];
	ctx->calc_text_size(text_size, ctx->calc_text_size_user_data, gui_label_text(label));
	GUI_V2(size[c] = GUI_MAX((int)text_size[c] + margin[c] * 2, size[c]));

	GUI_BOOL went_up = GUI_FALSE, hover = GUI_FALSE, down = GUI_FALSE;
	if (gui_is_inside_window(ctx, size)) {
		int px_margin[2];
		pt_to_px(px_margin, margin, ctx->dpi_scale);
		int box_width = size[1];
		GUI_DECL_V2(int, tmp, 0, box_width);
		pt_to_px(tmp, tmp, ctx->dpi_scale);
		float px_box_width = tmp[1] - px_margin[1]*2.f;

		size[0] += box_width + margin[0];

		gui_button_logic(ctx, label, pos, size, &went_up, NULL, &down, &hover);

		int px_pos[2];
		pt_to_px(px_pos, pos, ctx->dpi_scale);
		//V2i px_size = pt_to_px(size, ctx->dpi_scale);

		px_pos[0] += px_margin[0];
		px_pos[1] += px_margin[0]; // @todo Should be [1]?
		GUI_DECL_V2(int, px_size, px_box_width, px_box_width);
		if (radio_button_visual)
			gui_draw(	ctx, GuiDrawInfo_radiobutton, px_pos, px_size, hover, down, *value,
						NULL, gui_layer(ctx), gui_scissor(ctx));
		else
			gui_draw(	ctx, GuiDrawInfo_checkbox, px_pos, px_size, hover, down, *value,
						NULL, gui_layer(ctx), gui_scissor(ctx));
		
		px_pos[0] += px_box_width + 2*px_margin[0];
		gui_draw(	ctx, GuiDrawInfo_text, px_pos, px_size, GUI_FALSE, GUI_FALSE, GUI_FALSE,
					gui_label_text(label), gui_layer(ctx) + 1, gui_scissor(ctx));
	}

	gui_enlarge_bounding(ctx, pos[0] + size[0], pos[1] + size[1]);
	gui_end(ctx);

	gui_next_row(ctx); // @todo Layouting

	if (value && went_up && hover)
		*value = !*value;
	return went_up && hover;
}

GUI_BOOL gui_checkbox(GuiContext *ctx, const char *label, GUI_BOOL *value)
{ return gui_checkbox_ex(ctx, label, value, GUI_FALSE); }

GUI_BOOL gui_radiobutton(GuiContext *ctx, const char *label, GUI_BOOL value)
{
	GUI_BOOL v = value;
	return gui_checkbox_ex(ctx, label, &v, GUI_TRUE);
}

void gui_slider(GuiContext *ctx, const char *label, float *value, float min, float max)
{
	gui_slider_ex(ctx, label, value, min, max, 0.1f, GUI_FALSE, 0);
	gui_next_row(ctx); // @todo Layouting
}

static GUI_BOOL gui_textfield_ex(GuiContext *ctx, const char *label, char *buf, int buf_size, GUI_BOOL int_only)
{
	GUI_BOOL content_changed = GUI_FALSE;

	gui_begin(ctx, label);
	int margin[2] = {5, 3};
	int box_size[2];
	int label_size[2] = {0, 0};
	int *pos = gui_turtle(ctx)->pos;
	int *size = gui_turtle(ctx)->size;
	GUI_ASSIGN_V2(box_size, size);
	GUI_BOOL has_label = (strlen(gui_label_text(label)) > 0);
	if (has_label) {
		int label_size_f[2];
		ctx->calc_text_size(label_size_f, ctx->calc_text_size_user_data, gui_label_text(label));
		GUI_V2(label_size[c] = label_size_f[c] + margin[c]*2);

		box_size[0] -= label_size[0];
	}

	GUI_BOOL went_down = GUI_FALSE, hover = GUI_FALSE;
	if (gui_is_inside_window(ctx, size)) {
		gui_button_logic(ctx, label, pos, size, NULL, &went_down, NULL, &hover);
		GUI_BOOL active = (ctx->last_active_id == gui_id(label));

		if (active) {
			assert(buf && buf_size > 0);
			int char_count = (int)strlen(buf);
			for (int i = 0; i < ctx->written_char_count; ++i) {
				if (char_count >= buf_size)
					break;
				char ch = ctx->written_text_buf[i];

				if (int_only) {
					if (ch < '0' || ch > '9')
						continue;
				}

				if (ch == '\b') {
					if (char_count > 0)
						buf[--char_count] = '\0';
				} else {
					buf[char_count++] = ch;
				}
				content_changed = GUI_TRUE;
			}
			char_count = GUI_MIN(char_count, buf_size - 1);
			buf[char_count] = '\0';

			ctx->has_input = GUI_TRUE;
		}

		int px_margin[2];
		pt_to_px(px_margin, margin, ctx->dpi_scale);
		if (has_label) { // Draw label
			int px_pos[2];
			int px_size[2] = {0};
			pt_to_px(px_pos, pos, ctx->dpi_scale);
			px_pos[0] += px_margin[0];
			gui_draw(	ctx, GuiDrawInfo_text, px_pos, px_size, GUI_FALSE, GUI_FALSE, GUI_FALSE,
						gui_label_text(label), gui_layer(ctx) + 1, gui_scissor(ctx));
		}

		{ // Draw textbox
			GUI_DECL_V2(int, pt_pos, pos[0] + label_size[0], pos[1]);
			int px_pos[2], px_size[2];
			pt_to_px(px_pos, pt_pos, ctx->dpi_scale);
			pt_to_px(px_size, box_size, ctx->dpi_scale);
			// @todo down --> active
			gui_draw(	ctx, GuiDrawInfo_textbox, px_pos, px_size, hover, GUI_FALSE, active,
						NULL, gui_layer(ctx), gui_scissor(ctx));
			px_pos[0] += px_margin[0];
			gui_draw(	ctx, GuiDrawInfo_text, px_pos, px_size, GUI_FALSE, GUI_FALSE, GUI_FALSE,
						buf, gui_layer(ctx) + 1, gui_scissor(ctx));
		}

	}

	gui_enlarge_bounding(ctx, pos[0] + size[0], pos[1] + size[1]);
	gui_end(ctx);

	gui_next_row(ctx); // @todo Layouting

	return content_changed;
}

GUI_BOOL gui_textfield(GuiContext *ctx, const char *label, char *buf, int buf_size)
{ return gui_textfield_ex(ctx, label, buf, buf_size, GUI_FALSE); }

GUI_BOOL gui_intfield(GuiContext *ctx, const char *label, int *value)
{
	char buf[64] = {0};
	GUI_FMT_STR(buf, sizeof(buf), "%i", *value);
	GUI_BOOL ret = gui_textfield_ex(ctx, label, buf, sizeof(buf), GUI_FALSE);
	if (sscanf(buf, "%i", value) != 1)
		*value = 0;
	return ret;
}

void gui_label(GuiContext *ctx, const char *label)
{
	gui_begin(ctx, label);
	int *pos = gui_turtle(ctx)->pos;
	int *size = gui_turtle(ctx)->size;
	ctx->calc_text_size(size, ctx->calc_text_size_user_data, gui_label_text(label));

	if (gui_is_inside_window(ctx, size)) {
		gui_button_logic(ctx, label, pos, size, NULL, NULL, NULL, NULL);

		int px_pos[2], px_size[2];
		pt_to_px(px_pos, pos, ctx->dpi_scale);
		pt_to_px(px_size, size, ctx->dpi_scale);
		gui_draw(	ctx, GuiDrawInfo_text, px_pos, px_size, GUI_FALSE, GUI_FALSE, GUI_FALSE,
					gui_label_text(label), gui_layer(ctx) + 1, gui_scissor(ctx));
	}

	gui_enlarge_bounding(ctx, pos[0] + size[0], pos[1] + size[1]);
	gui_end(ctx);

	gui_next_row(ctx); // @todo Layouting
}


void gui_begin_listbox(GuiContext *ctx, const char *label)
{
	gui_begin(ctx, label);
	// @todo Clipping and scrollbar
}

void gui_end_listbox(GuiContext *ctx)
{
	gui_end(ctx);

	gui_next_row(ctx); // @todo Layouting
}

GUI_BOOL gui_begin_combo(GuiContext *ctx, const char *label)
{
	// @todo Use borderless window for list
	GUI_BOOL btn_down;
	int combo_pos[2];
	gui_turtle_pos(ctx, &combo_pos[0], &combo_pos[1]);
	btn_down = gui_button(ctx, label);
	GUI_DECL_V2(int, list_start_pos, combo_pos[0], combo_pos[1]);

	if (btn_down) {
		gui_begin(ctx, label); // User calls gui_end_combo()
		gui_turtle(ctx)->detached = GUI_TRUE;
		gui_set_turtle_pos(ctx, list_start_pos[0], list_start_pos[1]);
	}

	return btn_down;
}

GUI_BOOL gui_combo_item(GuiContext *ctx, const char *label)
{
	return gui_button(ctx, label);
}

void gui_end_combo(GuiContext *ctx)
{
	gui_end(ctx);
}

void gui_next_row(GuiContext *ctx)
{
	gui_set_turtle_pos(ctx, gui_turtle(ctx)->pos[0], gui_turtle(ctx)->last_bounding_max[1]);
}

void gui_next_col(GuiContext *ctx)
{
	gui_set_turtle_pos(ctx, gui_turtle(ctx)->last_bounding_max[0], gui_turtle(ctx)->pos[1]);
}

void gui_enlarge_bounding(GuiContext *ctx, int x, int y)
{
	GUI_DECL_V2(int, pos, x, y);

	GuiContext_Turtle *turtle = &ctx->turtles[ctx->turtle_ix];

	GUI_V2(turtle->bounding_max[c] = GUI_MAX(turtle->bounding_max[c], pos[c]));
	GUI_ASSIGN_V2(turtle->last_bounding_max, pos);
}

void gui_ver_space(GuiContext *ctx)
{
	int pos[2];
	GUI_ASSIGN_V2(pos, gui_turtle(ctx)->pos);
	gui_enlarge_bounding(ctx, pos[0] + 25, pos[1]);
	gui_next_col(ctx);
}

void gui_hor_space(GuiContext *ctx)
{
	int pos[2];
	GUI_ASSIGN_V2(pos, gui_turtle(ctx)->pos);
	gui_enlarge_bounding(ctx, pos[0], pos[1] + 25);
	gui_next_row(ctx);
}

void gui_layout_settings(GuiContext *ctx, const char *save_path)
{
	if (ctx->active_id && (ctx->key_state[KEY_MMB] & GUI_KEYSTATE_DOWN_BIT)) {
		GUI_FMT_STR(ctx->layout_element_label, sizeof(ctx->layout_element_label),
					"%s", ctx->active_label);
	}

	gui_begin_window(ctx, "gui_layoutwin|Layout settings");

		gui_label(ctx, "gui_layout_list|Selected element (mmb):");
		gui_textfield(ctx, "gui_layout_list+name|  name:", ctx->layout_element_label, sizeof(ctx->layout_element_label));

		GuiElementLayout layout = element_layout(ctx, ctx->layout_element_label);
		gui_label(ctx, gui_str(ctx, "gui_layout_list+id|  id: %u", layout.id));

		GUI_BOOL changed = GUI_FALSE;

		changed |= gui_checkbox(ctx, "gui_layout_list+has_offset|has_offset", &layout.has_offset);
		changed |= gui_intfield(ctx, "gui_layout_list+offset[0]|x", &layout.offset[0]); 
		changed |= gui_intfield(ctx, "gui_layout_list+offset[1]|y", &layout.offset[1]); 

		changed |= gui_checkbox(ctx, "gui_layout_list+b_size|has_size", &layout.has_size);
		changed |= gui_intfield(ctx, "gui_layout_list+size[0]|width", &layout.size[0]);
		changed |= gui_intfield(ctx, "gui_layout_list+size[1]|height", &layout.size[1]);

		changed |= gui_checkbox(ctx, "gui_layout_list+b_resize|prevent_resizing", &layout.prevent_resizing);

		changed |= gui_checkbox(ctx, "gui_layout_list+left|align_left", &layout.align_left);
		changed |= gui_checkbox(ctx, "gui_layout_list+right|align_right", &layout.align_right);
		changed |= gui_checkbox(ctx, "gui_layout_list+top|align_top", &layout.align_top);
		changed |= gui_checkbox(ctx, "gui_layout_list+bottom|align_bottom", &layout.align_bottom);

		if (gui_button(ctx, "gui_layout_list+save|Save layout")) {
			save_layout(ctx, save_path);
		}

		gui_label(ctx, "gui_layout_list+listlabel|All layouts");

		gui_begin_listbox(ctx, "gui_layout_list+list");
		for (int i = 0; i < ctx->layout_count; ++i) {
			GuiElementLayout l = ctx->layouts[i];
			const char *label = gui_str(ctx, "gui_layout_list+%i|%s", i, l.str);
			if (gui_selectable(ctx, label, l.id == layout.id)) {
				GUI_FMT_STR(ctx->layout_element_label,
							sizeof(ctx->layout_element_label),
							"%s", l.str);
			}
		}
		gui_end_listbox(ctx);

		if (changed) {
			update_element_layout(ctx, layout);
		}
	gui_end_window(ctx);
}

