#include "gui.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

//
// Misc util
//

static void *gui_frame_alloc(GuiContext *ctx, int size);

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
	gui_v_sprintf_impl(buf, count, fmt, args);
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

static void *gui_check_ptr(void *ptr)
{
	if (!ptr) {
		abort();
	}
	return ptr;
}

//
// Vector
//

#define GUI_DECL_V2(type, name, x, y) type name[2]; name[0] = x; name[1] = y;
#define GUI_V2(stmt) do { int c = 0; { stmt; } c = 1; { stmt; } } while(0)
#define GUI_ASSIGN_V2(a, b) GUI_V2((a)[c] = (b)[c])
#define GUI_EQUALS_V2(a, b) ((a)[0] == (b)[0] && (a)[1] == (b)[1])
#define GUI_ZERO(var) memset(&var, 0, sizeof(var))

static GUI_BOOL v2i_in_rect(int v[2], int pos[2], int size[2])
{ return v[0] >= pos[0] && v[1] >= pos[1] && v[0] < pos[0] + size[0] && v[1] < pos[1] + size[1]; }

//
// Gui_Tbl
//

#define GUI_TBL_LOAD_FACTOR 2

static Gui_Tbl_Entry gui_null_tbl_entry(Gui_Tbl *tbl)
{
	Gui_Tbl_Entry e = {0};
	e.key = tbl->null_key;
	e.value = tbl->null_value;
	return e;
}

Gui_Tbl gui_create_tbl(int null_value, int expected_item_count)
{
	Gui_Tbl tbl = {0};
	tbl.null_key = NULL_GUI_ID;
	tbl.null_value = null_value;
	tbl.array_size = expected_item_count*GUI_TBL_LOAD_FACTOR;
	tbl.array = GUI_MALLOC(sizeof(*tbl.array)*tbl.array_size);
	for (int i = 0; i < tbl.array_size; ++i)
		tbl.array[i] = gui_null_tbl_entry(&tbl);
	return tbl;
}

void gui_destroy_tbl(Gui_Tbl *tbl)
{
	GUI_FREE(tbl->array);
	tbl->array = NULL;
}

int gui_get_tbl(Gui_Tbl *tbl, GuiId key)
{
	int ix = key % tbl->array_size;
	/* Linear probing */
	/* Should not be infinite because set_id_handle_tbl asserts if table is full */
	while (tbl->array[ix].key != key && tbl->array[ix].key != tbl->null_key)
		ix = (ix + 1) % tbl->array_size;

	if (tbl->array[ix].key == tbl->null_key)
		GUI_ASSERT(tbl->array[ix].value == tbl->null_value);

	return tbl->array[ix].value;
}

void gui_set_tbl(Gui_Tbl *tbl, GuiId key, int value)
{
	GUI_ASSERT(key != tbl->null_key);
	if (tbl->count > tbl->array_size/HASHTABLE_LOAD_FACTOR) {
		/* Resize container */
		Gui_Tbl larger = gui_create_tbl(tbl->null_value, tbl->array_size);
		for (int i = 0; i < tbl->array_size; ++i) {
			if (tbl->array[i].key == tbl->null_key)
				continue;
			gui_set_tbl(&larger, tbl->array[i].key, tbl->array[i].value);
		}

		gui_destroy_tbl(tbl);
		*tbl = larger;
	}

	int ix = key % tbl->array_size;

	/* Linear probing */
	while (tbl->array[ix].key != key && tbl->array[ix].key != tbl->null_key)
		ix = (ix + 1) % tbl->array_size;

	Gui_Tbl_Entry *entry = &tbl->array[ix];
	bool modify_existing = 	value != tbl->null_value && entry->key != tbl->null_key;
	bool insert_new =		value != tbl->null_value && entry->key == tbl->null_key;
	bool remove_existing =	value == tbl->null_value && entry->key != tbl->null_key;
	bool remove_new =		value == tbl->null_value && entry->key == tbl->null_key;

	if (modify_existing) {
		entry->value = value;
	} else if (insert_new) {
		entry->key = key;
		entry->value = value;
		++tbl->count;
	} else if (remove_existing) {
		entry->key = key;
		entry->key = tbl->null_key;
		entry->value = tbl->null_value;
		GUI_ASSERT(tbl->count > 0);
		--tbl->count;

		/* Rehash */
		ix = (ix + 1) % tbl->array_size;
		while (tbl->array[ix].key != tbl->null_key) {
			Gui_Tbl_Entry e = tbl->array[ix];
			tbl->array[ix] = gui_null_tbl_entry(tbl);
			--tbl->count;
			gui_set_tbl(tbl, e.key, e.value);

			ix = (ix + 1) % tbl->array_size;
		}
	} else if (remove_new) {
		/* Nothing to be removed */
	} else {
		GUI_ASSERT(0 && "Hash table logic failed");
	}

	GUI_ASSERT(tbl->count < tbl->array_size);
}

void gui_clear_tbl(Gui_Tbl *tbl)
{
	tbl->count = 0;
	for (int i = 0; i < tbl->array_size; ++i)
		tbl->array[i] = gui_null_tbl_entry(tbl);
}


static GuiId gui_hash(const char *buf, int size)
{
	// Modified FNV-1a
	uint32_t hash = 2166136261;
	for (int i = 0; i < size; ++i)
		hash = ((hash ^ buf[i]) + 379721) * 16777619;
	return hash;
}

// gui_id("layout:foo_button+1|Press this") == gui_id("layout2:foo_button+1|Don't press this")
GuiId gui_id(const char *label)
{
	int begin = 0;
	int end = 0;
	GUI_BOOL id_token_reached = GUI_FALSE;
	while (label[end] && label[end] != '|') {
		if (label[end] == ':' && !id_token_reached)
			begin = end + 1;
		if (label[end] == '+')
			id_token_reached = GUI_TRUE;
		++end;
	}
	return gui_hash(label + begin, end - begin);
}

// "a:b:foo|Foo" + "_ext" -> "a:b:foo_ext"
static void gui_modified_id_label(char result[MAX_GUI_LABEL_SIZE], const char *label, const char *addition)
{
	int i = 0;
	while (label[i] && label[i] != '|') {
		result[i] = label[i];
		++i;
	}

	int k = 0;
	while (i < MAX_GUI_LABEL_SIZE && addition[k]) {
		result[i] = addition[k];
		++i;
		++k;
	}

	result[MIN(i, MAX_GUI_LABEL_SIZE - 1)] = '\0';
}

static GuiId gui_prop_hash(GuiId layout_id, GuiId key_id)
{ return layout_id ^ (key_id*2011); }

// "foo:bar+123|Button" -> {"", "foo", "bar", "bar+123"}
static void split_layout_str(	const char *strs[MAX_LAYOUTS_PER_ELEMENT],
								int sizes[MAX_LAYOUTS_PER_ELEMENT],
								int *count, const char *label)
{
	*count = 0;

	// There's always null-layout
	strs[*count] = label;
	sizes[*count] = 0;
	++(*count);

	GUI_BOOL plus_found = GUI_FALSE;
	int begin = 0;
	int end = 0;
	while (label[end] && label[end] != '|') {
		if (!plus_found && (label[end] == ':' || label[end] == '+')) {
			GUI_ASSERT(*count < MAX_LAYOUTS_PER_ELEMENT);
			strs[*count] = label + begin;
			sizes[*count] = end - begin;
			++(*count);

			if (label[end] != '+')
				begin = end + 1;
			else
				plus_found = GUI_TRUE;
		}
		++end;
	}

	if (begin != end) {
		GUI_ASSERT(*count < MAX_LAYOUTS_PER_ELEMENT);
		strs[*count] = label + begin;
		sizes[*count] = end - begin;
		++(*count);
	}
}

static GuiContext_LayoutProperty *find_layout_property(GuiContext *ctx, const char *label, const char *key, GUI_BOOL most_specific)
{
	const char *layout_names[MAX_LAYOUTS_PER_ELEMENT];
	int layout_name_sizes[MAX_LAYOUTS_PER_ELEMENT];
	int layout_count;
	split_layout_str(layout_names, layout_name_sizes, &layout_count, label);

	int i;
	GuiId key_id = gui_id(key);
	for (i = layout_count - 1; i >= 0; --i) {
		GuiId layout_id = gui_hash(layout_names[i], layout_name_sizes[i]);

		int ix = gui_get_tbl(&ctx->prop_ix_tbl, gui_prop_hash(layout_id, key_id));

		if (ix >= 0)
			return &ctx->layout_props[ix];

		if (most_specific)
			break;
	}

	return NULL;
}


static int layout_property(GuiContext *ctx, const char *label, const char *key)
{
	GuiContext_LayoutProperty *found = find_layout_property(ctx, label, key, GUI_FALSE);
	if (found)
		return found->value;
	else
		return 0;
}

static GUI_BOOL has_layout_property(GuiContext *ctx, const char *label, const char *key)
{ return find_layout_property(ctx, label, key, GUI_FALSE) != NULL; }

static GUI_BOOL has_own_layout_property(GuiContext *ctx, const char *label, const char *key)
{ return find_layout_property(ctx, label, key, GUI_TRUE) != NULL; }

void gui_update_layout_property_ex(GuiContext *ctx, const char *label, const char *key, int value, GUI_BOOL saved)
{
	GuiContext_LayoutProperty *found = find_layout_property(ctx, label, key, GUI_TRUE);

	if (found) {
		found->value = value;
		found->dont_save = !saved;
	} else {
		// New property
		gui_append_layout_property(ctx, label, key, value, saved);
	}
}

void gui_update_layout_property(GuiContext *ctx, const char *label, const char *key, int value)
{ gui_update_layout_property_ex(ctx, label, key, value, GUI_TRUE); }

static void remove_layout_property(GuiContext *ctx, const char *label, const char *key)
{
	GuiContext_LayoutProperty *found = find_layout_property(ctx, label, key, GUI_TRUE);
	if (!found)
		return;

	gui_set_tbl(&ctx->prop_ix_tbl, gui_prop_hash(found->layout_id, found->key_id), -1);

	if (ctx->layout_props_count > 1) {
		GuiContext_LayoutProperty *over = &ctx->layout_props[ctx->layout_props_count - 1];
		gui_set_tbl(&ctx->prop_ix_tbl, gui_prop_hash(over->layout_id, over->key_id), found - ctx->layout_props);

		*found = *over;
		--ctx->layout_props_count;
	}
}

static void set_layout_property_saved(GuiContext *ctx, const char *label, const char *key, GUI_BOOL saved)
{
	GuiContext_LayoutProperty *found = find_layout_property(ctx, label, key, GUI_TRUE);
	if (found)
		found->dont_save = !saved;
}

static void most_specific_layout(const char **str, int *size, const char *label)
{
	const char *layout_names[MAX_LAYOUTS_PER_ELEMENT];
	int layout_name_sizes[MAX_LAYOUTS_PER_ELEMENT];
	int layout_count;
	split_layout_str(layout_names, layout_name_sizes, &layout_count, label);
	GUI_ASSERT(layout_count > 0);
	const int most_specific = layout_count - 1;

	*str = layout_names[most_specific];
	*size = layout_name_sizes[most_specific];
}

void gui_append_layout_property(GuiContext *ctx, const char *label, const char *key, int value, GUI_BOOL saved)
{
	if (ctx->layout_props_count == ctx->layout_props_capacity) {
		ctx->layout_props_capacity *= 2;
		ctx->layout_props = (GuiContext_LayoutProperty*)GUI_REALLOC(
			ctx->layout_props, sizeof(*ctx->layout_props)*ctx->layout_props_capacity);
	}

	const char *layout_name;
	int layout_name_size;
	most_specific_layout(&layout_name, &layout_name_size, label);

	GuiContext_LayoutProperty prop = {0};
	prop.layout_id = gui_hash(layout_name, layout_name_size);
	prop.key_id = gui_id(key);
	GUI_FMT_STR(prop.key_name, sizeof(prop.key_name), "%s", key);
	GUI_FMT_STR(prop.layout_name, sizeof(prop.layout_name), "%.*s", layout_name_size, layout_name);
	prop.value = value;
	prop.dont_save = !saved;

	gui_set_tbl(&ctx->prop_ix_tbl, gui_prop_hash(prop.layout_id, prop.key_id), ctx->layout_props_count);
	ctx->layout_props[ctx->layout_props_count++] = prop;
}

void gui_append_element(GuiContext *ctx, GuiContext_Element elem)
{
	if (ctx->element_count == ctx->element_capacity) {
		ctx->element_capacity *= 2;
		ctx->elements = (GuiContext_Element*)GUI_REALLOC(
			ctx->elements, sizeof(*ctx->elements)*ctx->element_capacity);
	}

	ctx->elements[ctx->element_count++] = elem;
}

// Return index to found, or index where should be inserted
static int storage_bsearch(GuiContext *ctx, GuiId id)
{
	int left = ctx->storage_count;
	int cursor = 0;
	while (left > 0) {
		int jump = left / 2;
		int mid = cursor + jump;
		if (ctx->storage[mid].id < id) {
			cursor = mid + 1;
			left -= jump + 1;
		} else {
			left = jump;
		}
	}
	return cursor;
}

static GUI_BOOL element_storage_bool(GuiContext *ctx, const char *label, GUI_BOOL default_value)
{
	GuiId id = gui_id(label);
	int ix = storage_bsearch(ctx, id);
	if (ix == ctx->storage_count || ctx->storage[ix].id != id)
		return default_value;
	return ctx->storage[ix].bool_value;
}

static void set_element_storage_bool(GuiContext *ctx, const char *label, GUI_BOOL value)
{
	GuiId id = gui_id(label);
	int ix = storage_bsearch(ctx, id);
	if (ix == ctx->storage_count || ctx->storage[ix].id != id) {
		// New storage value
		if (ctx->storage_count == ctx->storage_capacity) {
			// Need more space
			ctx->storage_capacity *= 2;
			ctx->storage = (GuiContext_Storage*)GUI_REALLOC(ctx->storage, sizeof(*ctx->storage)*ctx->storage_capacity);
		}

		// Make room in the middle
		memmove(&ctx->storage[ix + 1], &ctx->storage[ix], sizeof(*ctx->storage)*ctx->storage_count - ix);
		++ctx->storage_count;
	}

	ctx->storage[ix].id = id;
	ctx->storage[ix].bool_value = value;
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
	//fprintf(file, "\tctx->layout_props_count = 0;\n\n");
	GuiContext_LayoutProperty last = {0};
	for (int i = 0; i < ctx->layout_props_count; ++i) {
		GuiContext_LayoutProperty prop = ctx->layout_props[i];
		if (prop.dont_save)
			continue;

		if (i > 0 && strcmp(prop.layout_name, last.layout_name))
			fprintf(file, "\n");

		fprintf(file,	"\tgui_update_layout_property(ctx, \"%s\", \"%s\", %i);\n",
						prop.layout_name, prop.key_name, prop.value);

		last = prop;
	}
	fprintf(file, "}\n");
	fclose(file);
}

static GuiContext_Turtle *gui_turtle(GuiContext *ctx)
{
	return &ctx->turtles[ctx->turtle_ix];
}

// Minimum size is a smallest size when all contained stuff fits inside the element.
// E.g. windows can often be smaller than their min size
static void gui_set_min_size(GuiContext *ctx, const char *label, int size[2])
{
	gui_update_layout_property_ex(ctx, label, "min_size_x", size[0], GUI_FALSE);
	gui_update_layout_property_ex(ctx, label, "min_size_y", size[1], GUI_FALSE);
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
	// @todo Destroy layout properties associated with the window, if win->remove_when_not_used.
	//       Currently "leaks" memory if windows are destroyed and new ones are introduced with different ids

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

	GUI_ZERO(ctx->windows[handle]);

	if (ctx->focused_win_ix == handle)
		ctx->focused_win_ix = -1;
	if (ctx->active_win_ix == handle)
		ctx->active_win_ix = GUI_NONE_WINDOW_IX;
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

// Labels can contain prefixed stuff (layouts and unique ids).
// "hoopoa|asdfg" -> "asdfg"
// "foo:bar" -> "bar"
// "layout1:layout2|Label" -> "label"
const char *gui_label_text(const char *label)
{
	int i = 0;
	int pos = 0;
	while (label[i]) {
		if (label[i] == ':')
			pos = i + 1;
		else if (label[i] == '|')
			return label + i + 1;
		++i;
	}
	return label + pos;
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

const char *gui_prepend_layout(GuiContext *ctx, const char *layout, const char *label)
{
	if (label)
		return gui_str(ctx, "%s:%s", layout, label);
	else
		return layout;
}

void gui_set_turtle_pos(GuiContext *ctx, int x, int y)
{
	ctx->turtles[ctx->turtle_ix].manual_offset[0] = x;
	ctx->turtles[ctx->turtle_ix].manual_offset[1] = y;
}

GuiContext_Window *gui_window(GuiContext *ctx)
{
	if (gui_turtle(ctx)->window_ix == GUI_NONE_WINDOW_IX ||
		gui_turtle(ctx)->window_ix == GUI_BG_WINDOW_IX)
		return NULL;
	GUI_ASSERT(gui_turtle(ctx)->window_ix < MAX_GUI_WINDOW_COUNT);
	return gui_turtle(ctx)->window_ix >= 0 ? &ctx->windows[gui_turtle(ctx)->window_ix] : NULL;
}

void gui_win_dimension(GuiContext *ctx, int pos[2], int size[2], GuiContext_Window *win)
{
	GUI_ASSIGN_V2(pos, win->recorded_pos);

	size[0] = layout_property(ctx, win->label, "size_x");
	size[1] = layout_property(ctx, win->label, "size_y");
	if (win->minimized)
		size[1] = win->bar_height;
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

int gui_turtle_layer(GuiContext *ctx)
{ return gui_turtle(ctx)->layer; }

void gui_turtle_add_layer(GuiContext *ctx, int delta)
{ gui_turtle(ctx)->layer += delta; }

static void gui_combine_scissor(int dst[4], int src[4])
{
	if (!src)
		return;

	int dp[4] = {dst[0], dst[1], dst[0] + dst[2], dst[1] + dst[3]};
	int sp[4] = {src[0], src[1], src[0] + src[2], src[1] + src[3]};

	GUI_V2(dp[c] = MAX(dp[c], sp[c]));
	GUI_V2(dp[c + 2] = MIN(dp[c + 2], sp[c + 2]));

	dst[0] = dp[0];
	dst[1] = dp[1];
	dst[2] = dp[2] - dp[0];
	dst[3] = dp[3] - dp[1];
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
	if (!win)
		return GUI_TRUE; // @todo Use background size

	if (win->minimized)
		return GUI_FALSE;

	int win_pos[2];
	int win_size[2];
	gui_win_dimension(ctx, win_pos, win_size, win);
	if (pos[0] + size[0] <= win_pos[0] || pos[1] + size[1] <= win_pos[1] + win->bar_height)
		return GUI_FALSE;
	if (pos[0] >= win_pos[0] + win_size[0] || pos[1] >= win_pos[1] + win_size[1])
		return GUI_FALSE;
	return GUI_TRUE;
}

void gui_start_dragging(GuiContext *ctx, float start_value[2])
{
	GUI_ASSERT(!ctx->dragging);
	ctx->dragging = GUI_TRUE;
	GUI_ASSIGN_V2(ctx->drag_start_pos, ctx->cursor_pos);
	GUI_ASSIGN_V2(ctx->drag_start_value, start_value);
}

static void *gui_frame_alloc(GuiContext *ctx, int size)
{
	GUI_ASSERT(ctx->framemem_bucket_count >= 1);
	GuiContext_MemBucket *bucket = &ctx->framemem_buckets[ctx->framemem_bucket_count - 1];
	if (bucket->used + size > bucket->size) {
		// Need a new bucket :(
		int new_bucket_count = ctx->framemem_bucket_count + 1;
		ctx->framemem_buckets = (GuiContext_MemBucket*)gui_check_ptr(realloc(ctx->framemem_buckets, sizeof(*ctx->framemem_buckets)*new_bucket_count));

		int bucket_size = GUI_MAX(size, bucket->size * 2);
		bucket = &ctx->framemem_buckets[ctx->framemem_bucket_count++];
		bucket->data = gui_check_ptr(malloc(bucket_size));
		bucket->size = bucket_size;
		bucket->used = 0;
	}

	char *mem = (char *)bucket->data + bucket->used; // @todo Alignment
	bucket->used += size;
	GUI_ASSERT(bucket->used <= bucket->size);
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

		ctx->framemem_buckets = (GuiContext_MemBucket*)gui_check_ptr(realloc(ctx->framemem_buckets, sizeof(*ctx->framemem_buckets)));
		ctx->framemem_buckets[0].data = gui_check_ptr(realloc(ctx->framemem_buckets[0].data, memory_size));
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
		int text_size = (int)strlen(text) + 1;
		char *text_copy = (char*)gui_frame_alloc(ctx, text_size);
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
	info.element_ix = gui_turtle(ctx)->element_ix;

	if (ctx->draw_info_count == ctx->draw_info_capacity) {
		// Need more space
		ctx->draw_info_capacity *= 2;
		GuiDrawInfo *new_infos = (GuiDrawInfo*)gui_frame_alloc(ctx, sizeof(*new_infos)*ctx->draw_info_capacity);
		memcpy(new_infos, ctx->draw_infos, sizeof(*new_infos)*ctx->draw_info_count);
		ctx->draw_infos = new_infos;
	}

	ctx->draw_infos[ctx->draw_info_count++] = info;
}

GuiContext *create_gui(CalcTextSizeFunc calc_text, void *user_data_for_calc_text)
{
	GuiContext *ctx = (GuiContext*)gui_check_ptr(GUI_MALLOC(sizeof(*ctx)));
	GUI_ZERO(*ctx);
	ctx->dpi_scale = 1.0f;
	ctx->calc_text_size = calc_text;
	ctx->calc_text_size_user_data = user_data_for_calc_text;
	ctx->hot_layer = -1;
	ctx->active_win_ix = GUI_NONE_WINDOW_IX;
	ctx->focused_win_ix = -1;

	ctx->draw_info_capacity = 64;

	ctx->layout_props_capacity = 64;
	ctx->prop_ix_tbl = gui_create_tbl(-1, ctx->layout_props_capacity);
	ctx->layout_props = (GuiContext_LayoutProperty*)gui_check_ptr(
		GUI_MALLOC(sizeof(*ctx->layout_props)*ctx->layout_props_capacity));

	ctx->element_capacity = GUI_DEFAULT_ELEMENT_CAPACITY;
	ctx->elements = (GuiContext_Element*)gui_check_ptr(
		GUI_MALLOC(sizeof(*ctx->elements)*ctx->element_capacity));

	ctx->storage_capacity = GUI_DEFAULT_STORAGE_CAPACITY;
	ctx->storage = (GuiContext_Storage*)gui_check_ptr(GUI_MALLOC(sizeof(*ctx->storage)*ctx->storage_capacity));

	ctx->framemem_bucket_count = 1;
	ctx->framemem_buckets = (GuiContext_MemBucket*)gui_check_ptr(GUI_MALLOC(sizeof(*ctx->framemem_buckets)));
	ctx->framemem_buckets[0].data = gui_check_ptr(GUI_MALLOC(GUI_DEFAULT_FRAME_MEMORY));
	ctx->framemem_buckets[0].size = GUI_DEFAULT_FRAME_MEMORY;
	ctx->framemem_buckets[0].used = 0;

	{ // Default layouts
		gui_update_layout_property(ctx, "gui_bar", "size_y", 25);

		gui_update_layout_property(ctx, "gui_checkbox", "size_x", 22);
		gui_update_layout_property(ctx, "gui_checkbox", "size_y", 22);
		gui_update_layout_property(ctx, "gui_checkbox", "gap_x", 4);

		gui_update_layout_property(ctx, "gui_contextmenu", "prevent_resizing", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_contextmenu", "resize_to_min_x", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_contextmenu", "resize_to_min_y", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_contextmenu", "padding_left", 0);
		gui_update_layout_property(ctx, "gui_contextmenu", "padding_top", 0);
		gui_update_layout_property(ctx, "gui_contextmenu", "padding_right", 0);
		gui_update_layout_property(ctx, "gui_contextmenu", "padding_bottom", 0);
		gui_update_layout_property(ctx, "gui_contextmenu", "gap_x", 0);
		gui_update_layout_property(ctx, "gui_contextmenu", "gap_y", 0);
		gui_update_layout_property(ctx, "gui_contextmenu_item", "align_left", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_contextmenu_item", "align_right", GUI_TRUE);

		gui_update_layout_property(ctx, "gui_textfield", "size_y", 22);
		gui_update_layout_property(ctx, "gui_textfield", "gap_x", 4);

		gui_update_layout_property(ctx, "gui_button", "padding_left", 5);
		gui_update_layout_property(ctx, "gui_button", "padding_top", 2);
		gui_update_layout_property(ctx, "gui_button", "padding_right", 5);
		gui_update_layout_property(ctx, "gui_button", "padding_bottom", 4);

		gui_update_layout_property(ctx, "gui_slider_x", "size_y", 25);
		gui_update_layout_property(ctx, "gui_slider_x", "gap_x", 4);
		gui_update_layout_property(ctx, "gui_slider_x", "padding_left", 0);
		gui_update_layout_property(ctx, "gui_slider_x", "padding_top", 2);
		gui_update_layout_property(ctx, "gui_slider_x", "padding_right", 0);
		gui_update_layout_property(ctx, "gui_slider_x", "padding_bottom", 2);

		gui_update_layout_property(ctx, "gui_slider_y", "size_x", 25);
		gui_update_layout_property(ctx, "gui_slider_y", "gap_y", 4);

		gui_update_layout_property(ctx, "gui_content_slider_x", "size_y", 15);
		gui_update_layout_property(ctx, "gui_content_slider_x", "align_left", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_content_slider_x", "align_right", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_content_slider_x", "align_bottom", GUI_TRUE);

		gui_update_layout_property(ctx, "gui_content_slider_y", "size_x", 15);
		gui_update_layout_property(ctx, "gui_content_slider_y", "align_top", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_content_slider_y", "align_bottom", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_content_slider_y", "align_right", GUI_TRUE);

		gui_update_layout_property(ctx, "gui_scroll_panel", "align_left", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_scroll_panel", "align_top", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_scroll_panel", "align_right", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_scroll_panel", "align_bottom", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_scroll_panel", "use_parent_size_instead_of_content_size", GUI_TRUE);

		gui_update_layout_property(ctx, "gui_treenode", "padding_left", 20);
		gui_update_layout_property(ctx, "gui_treenode", "gap_y", 2);

		gui_update_layout_property(ctx, "gui_bg_window", "align_left", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_bg_window", "align_top", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_bg_window", "align_right", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_bg_window", "align_bottom", GUI_TRUE);

		gui_update_layout_property(ctx, "gui_layoutwin", "size_x", 400);
		gui_update_layout_property(ctx, "gui_layoutwin", "size_y", 700);

		gui_update_layout_property(ctx, "gui_layout_list", "align_left", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_layout_list", "align_right", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_layout_list", "gap_x", 3);
		gui_update_layout_property(ctx, "gui_layout_list_prop", "on_same_row", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_layout_list_prop", "align_left", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_layout_list_prop", "align_right", GUI_TRUE);

		gui_update_layout_property(ctx, "gui_window", "size_x", 100);
		gui_update_layout_property(ctx, "gui_window", "size_y", 100);

		gui_update_layout_property(ctx, "gui_client", "offset_x", 0);
		gui_update_layout_property(ctx, "gui_client", "offset_y", 0);
		gui_update_layout_property(ctx, "gui_client", "padding_left", 10);
		gui_update_layout_property(ctx, "gui_client", "padding_top", 5);
		gui_update_layout_property(ctx, "gui_client", "padding_right", 10);
		gui_update_layout_property(ctx, "gui_client", "padding_bottom", 5);
		gui_update_layout_property(ctx, "gui_client", "align_left", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_client", "align_right", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_client", "align_top", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_client", "align_bottom", GUI_TRUE);
		gui_update_layout_property(ctx, "gui_client", "size_x", 0); // Make minimum size zero
		gui_update_layout_property(ctx, "gui_client", "size_y", 0);
		gui_update_layout_property(ctx, "gui_client", "gap_x", 0);
		gui_update_layout_property(ctx, "gui_client", "gap_y", 4);
		gui_update_layout_property(ctx, "gui_client", "scrollable", GUI_TRUE);
	}

	return ctx;
}

void destroy_gui(GuiContext *ctx)
{
	if (ctx) {
		for (int i = 0; i < ctx->framemem_bucket_count; ++i)
			GUI_FREE(ctx->framemem_buckets[i].data);
		GUI_FREE(ctx->framemem_buckets);

		gui_destroy_tbl(&ctx->prop_ix_tbl);
		GUI_FREE(ctx->layout_props);
		GUI_FREE(ctx->elements);
		GUI_FREE(ctx->storage);

		for (int i = 0; i < MAX_GUI_WINDOW_COUNT; ++i) {
			if (ctx->windows[i].id)
				destroy_window(ctx, i);
		}

		GUI_FREE(ctx);
	}
}

void gui_button_logic(GuiContext *ctx, const char *label, int pos[2], int size[2], GUI_BOOL *went_up, GUI_BOOL *went_down, GUI_BOOL *down, GUI_BOOL *hover);

void gui_pre_frame(GuiContext *ctx)
{
	GUI_ASSERT(ctx->turtle_ix == 0);

	refresh_framemem(ctx);

	ctx->draw_infos =
		(GuiDrawInfo*)gui_frame_alloc(ctx, sizeof(*ctx->draw_infos)*ctx->draw_info_capacity);
	ctx->draw_info_count = 0;

	ctx->interacted_id = 0;
	ctx->element_count = 0;

	// "Null" turtle
	GUI_ZERO(*gui_turtle(ctx));
	gui_turtle(ctx)->window_ix = GUI_BG_WINDOW_IX;
	GUI_ASSIGN_V2(gui_turtle(ctx)->size, ctx->host_win_size);
	gui_turtle(ctx)->layer = ctx->base_layer;

	{ // Clickable background
		int pos[2] = {};
		gui_button_logic(ctx, "gui_bg_window", pos, ctx->host_win_size, NULL, NULL, NULL, NULL);
	}

}

static int gui_solve_element_tree_min_layout(GuiContext *ctx, int ix, const int *pos)
{
	GuiContext_Element *elem = &ctx->elements[ix];

	const char *offset_str[2] = { "offset_x", "offset_y" };
	int bounding_begin[2] = {pos[0], pos[1]};
	GUI_V2(bounding_begin[c] += elem->manual_offset[c] + layout_property(ctx, elem->label, offset_str[c]));

	const char *padding_str[4] = { "padding_left", "padding_top", "padding_right", "padding_bottom" };
	int padded_content_begin[2] = { bounding_begin[0], bounding_begin[1] };
	GUI_V2(padded_content_begin[c] += layout_property(ctx, elem->label, padding_str[c]));
	int padded_content_end[2] = { padded_content_begin[0], padded_content_begin[1] };

	// Single pass is sufficient for solving the layout where every element is minimum-size.
	// Note that some elements can be forced to be smaller than content minimum, e.g. windows.
	int sub_i = ix + 1;
	int sub_depth = elem->depth + 1;
	GuiContext_Element *last_sub = NULL;
	while (sub_i < ctx->element_count && ctx->elements[sub_i].depth == sub_depth) {
		GuiContext_Element *sub = &ctx->elements[sub_i];

		int sub_pos[2] = { padded_content_begin[0], padded_content_begin[1] };
		if (last_sub) {
			GUI_BOOL comp = !layout_property(ctx, sub->label, "on_same_row");
			sub_pos[comp] = last_sub->solved_min_pos[comp] + last_sub->solved_min_size[comp];

			const char *gap_str[2] = { "gap_x", "gap_y" };
			sub_pos[comp] += layout_property(ctx, elem->label, gap_str[comp]);
		}

		if (sub->detached) {
			GUI_V2(sub_pos[c] = bounding_begin[c]);
		}

		int next_i = sub_i + gui_solve_element_tree_min_layout(ctx, sub_i, sub_pos);
		if (!sub->detached) {
			GUI_V2(padded_content_end[c] = MAX(padded_content_end[c], sub->solved_min_pos[c] + sub->solved_min_size[c]));
			last_sub = sub;
		}

		sub_i = next_i;
	}

	int bounding_end[2];
	GUI_V2(bounding_end[c] = padded_content_end[c]);
	GUI_V2(bounding_end[c] += layout_property(ctx, elem->label, padding_str[c + 2]));

	const char *min_size_str[2] = { "min_size_x", "min_size_y" };
	GUI_V2(bounding_end[c] =
			MAX(bounding_end[c],
				bounding_begin[c] + layout_property(ctx, elem->label, min_size_str[c])));

	GUI_V2(elem->solved_min_pos[c] = bounding_begin[c]);
	GUI_V2(elem->solved_min_size[c] = bounding_end[c] - bounding_begin[c]);
	GUI_V2(elem->solved_min_content_size[c] = bounding_end[c] - bounding_begin[c]);

	// Explicit size overrides calculated minimum size
	const char *size_str[2] = { "size_x", "size_y" };
	const char *resize_to_min_str[2] = { "resize_to_min_x", "resize_to_min_y" };
	GUI_V2(
		if (has_layout_property(ctx, elem->label, size_str[c]) &&
			!layout_property(ctx, elem->label, resize_to_min_str[c])) {
			elem->solved_min_size[c] = layout_property(ctx, elem->label, size_str[c]);
		}
	);

	elem->min_solved = GUI_TRUE;

	return sub_i - ix;
}

// Apply weaker layout conditions, those which don't affect minimum size calculations
static int gui_solve_element_tree_final_layout(GuiContext *ctx, int ix,
		int *parent_pos, int *parent_size, int *parent_padding,
		const int *offset_)
{
	GuiContext_Element *elem = &ctx->elements[ix];
	GUI_BOOL detached = elem->detached;
	int offset[2] = {offset_[0], offset_[1]};
	GUI_ASSERT(elem->min_solved == GUI_TRUE);

	GUI_V2(elem->solved_pos[c] = elem->solved_min_pos[c] + offset[c]);
	GUI_V2(elem->solved_size[c] = elem->solved_min_size[c]);

	const char *align_str[4] = { "align_left", "align_top", "align_right", "align_bottom" };
	GUI_V2(
		GUI_BOOL align_min = layout_property(ctx, elem->label, align_str[c]);
		GUI_BOOL align_max = layout_property(ctx, elem->label, align_str[c + 2]);
		if (align_max) {
			if (align_min) {
				// Stretch, take account previous elements on same row/column
				int free_space =	parent_pos[c] + parent_size[c]
									- (elem->solved_pos[c] + elem->solved_size[c])
									- parent_padding[c + 2]*(!detached);
				elem->solved_size[c] += MAX(free_space, 0);
			} else {
				// Move
				int target = 	parent_pos[c] + parent_size[c]
								- elem->solved_size[c]
								- parent_padding[c + 2]*(!detached);
				int dif = target - elem->solved_pos[c];
				elem->solved_pos[c] = target;
				offset[c] = dif;
			}
		}
	);

	GUI_V2(elem->solved_content_size[c] = elem->solved_min_content_size[c]);

	int elem_padding[4] = {
		layout_property(ctx, elem->label, "padding_left"),
		layout_property(ctx, elem->label, "padding_top"),
		layout_property(ctx, elem->label, "padding_right"),
		layout_property(ctx, elem->label, "padding_bottom"),
	};

	const char *scroll_str[2] = { "scroll_x", "scroll_y" };
	int content_scroll[2] = {0};
	GUI_V2(content_scroll[c] = layout_property(ctx, elem->label, scroll_str[c]));

	// @todo Don't requery for every element
	GUI_DECL_V2(int, slider_size,	layout_property(ctx, "gui_content_slider_y", "size_x"),
									layout_property(ctx, "gui_content_slider_x", "size_y"));
	const char *needs_scroll_str[2] = { "needs_scroll_x", "needs_scroll_y" };
	GUI_BOOL needs_scroll[2];
	GUI_V2(needs_scroll[c] = layout_property(ctx, elem->label, needs_scroll_str[c]));

	int sub_i = ix + 1;
	int sub_depth = elem->depth + 1;
	while (sub_i < ctx->element_count && ctx->elements[sub_i].depth == sub_depth) {
		GuiContext_Element *sub = &ctx->elements[sub_i];
		int sub_offset[2] = {0};
		if (!sub->detached)
			GUI_V2(sub_offset[c] = offset[c] - content_scroll[c]);

		int pos[2];
		GUI_V2(pos[c] = elem->solved_pos[c] + sub_offset[c]);

		int size[2];
		if (layout_property(ctx, sub->label, "use_parent_size_instead_of_content_size")) {
			GUI_V2(size[c] = elem->solved_size[c]);
		} else {
			GUI_V2(size[c] = MAX(elem->solved_content_size[c], elem->solved_size[c]));
			GUI_V2(size[c] = size[c] - needs_scroll[!c]*slider_size[c]);
			GUI_V2(size[c] = MAX(size[c], elem->solved_min_content_size[c]));
		}

		sub_i += gui_solve_element_tree_final_layout(ctx, sub_i,
					pos, size, elem_padding, sub_offset);
	}

	if (layout_property(ctx, elem->label, "scrollable")) {
		GUI_V2(elem->solved_needs_scroll[c] =	elem->solved_min_content_size[c] >
												elem->solved_size[c] - needs_scroll[!c]*slider_size[c]);
	} else {
		GUI_V2(elem->solved_needs_scroll[c] = GUI_FALSE);
	}

	elem->final_solved = GUI_TRUE;

	return sub_i - ix;
}

void gui_post_frame(GuiContext *ctx)
{
	GUI_ASSERT(ctx->turtle_ix == 0);

	{ // Layout solver
		int i = 0;
		while (i < ctx->element_count) {
			int pos[2] = {0};
			i += gui_solve_element_tree_min_layout(ctx, i, pos);
		}

		i = 0;
		while (i < ctx->element_count) {
			int pos[2] = {0};
			int offset[2] = {0};
			int padding[4] = {0};
			i += gui_solve_element_tree_final_layout(ctx, i, pos, ctx->host_win_size, padding, offset);
		}

		for (i = 0; i < ctx->element_count; ++i) {
			GuiContext_Element *elem = &ctx->elements[i];
			GUI_ASSERT(elem->final_solved);

			const char *solved_pos_str[2] = { "solved_pos_x", "solved_pos_y" };
			GUI_V2(elem->solved_pos_delta[c] = elem->solved_pos[c] - layout_property(ctx, elem->label, solved_pos_str[c]));
		}

#define PRINT_TREE 0
#if PRINT_TREE
		GUI_PRINTF("ELEMENTS\n");
#endif

		for (i = 0; i < ctx->element_count; ++i) {
			GuiContext_Element *elem = &ctx->elements[i];
			GUI_ASSERT(elem->min_solved);
			GUI_ASSERT(elem->final_solved);

#if PRINT_TREE
			if (i < 100) {
				for (int k = 0; k < elem->depth; ++k)
					GUI_PRINTF(" ");
				GUI_PRINTF("%s mc(%i, %i) s(%i, %i) scroll(%i, %i)\n", elem->label,
							elem->solved_min_content_size[0], elem->solved_min_content_size[1],
							elem->solved_size[0], elem->solved_size[1],
							elem->solved_needs_scroll[0], elem->solved_needs_scroll[1]);
			}
#endif
			const char *solved_pos_str[2] = { "solved_pos_x", "solved_pos_y" };
			const char *solved_size_str[2] = { "solved_size_x", "solved_size_y" };
			const char *solved_content_size_str[2] = { "solved_content_size_x", "solved_content_size_y" };
			const char *needs_scroll_str[2] = { "needs_scroll_x", "needs_scroll_y" };

			// Record new layout values for next frame
			GUI_V2(gui_update_layout_property_ex(	ctx, elem->label, solved_pos_str[c],
													elem->solved_pos[c], GUI_FALSE));
			GUI_V2(gui_update_layout_property_ex(	ctx, elem->label, solved_size_str[c],
													elem->solved_size[c], GUI_FALSE));
			GUI_V2(gui_update_layout_property_ex(	ctx, elem->label, solved_content_size_str[c],
													elem->solved_content_size[c], GUI_FALSE));
			GUI_V2(gui_update_layout_property_ex(	ctx, elem->label, needs_scroll_str[c],
													elem->solved_needs_scroll[c], GUI_FALSE));
		}
	}

#if 0
	// Hack: offset draw commands according to changes made in layout of the current frame
	for (int i = 0; i < ctx->draw_info_count; ++i) {
		GuiDrawInfo *info = &ctx->draw_infos[i];
		GuiContext_Element *elem = &ctx->elements[info->element_ix];
		GUI_V2(info->pos[c] += elem->solved_pos_delta[c]);
	}
#endif

	for (int i = 0; i < MAX_GUI_WINDOW_COUNT; ++i) {
		if (!ctx->windows[i].id)
			continue;

		if (!ctx->windows[i].used) {
			if (ctx->active_win_ix == i) {
				// Stop interacting with an element in hidden window
				gui_set_inactive(ctx, ctx->active_id);
			}

			if (ctx->windows[i].remove_when_not_used) {
				// Position etc. is not preserved if the window appears again
				destroy_window(ctx, i);
				continue;
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

	uint8_t lmb_state = ctx->key_state[GUI_KEY_LMB];
	uint8_t mmb_state = ctx->key_state[GUI_KEY_MMB];
	uint8_t rmb_state = ctx->key_state[GUI_KEY_RMB];

	GUI_BOOL was_released = GUI_FALSE;
	if (gui_is_active(ctx, label)) {
		if (lmb_state & GUI_KEYSTATE_DOWN_BIT) {
			if (down) *down = GUI_TRUE;
		} else if (lmb_state & GUI_KEYSTATE_RELEASED_BIT) {
			if (went_up) *went_up = GUI_TRUE;
			gui_set_inactive(ctx, gui_id(label));
			was_released = GUI_TRUE;
		}
		
		// Rmb should bring window to top
		if (rmb_state & GUI_KEYSTATE_RELEASED_BIT) {
			gui_set_inactive(ctx, gui_id(label));
		}

		// For layout editor
		if (mmb_state & GUI_KEYSTATE_RELEASED_BIT)
			gui_set_inactive(ctx, gui_id(label));

	} else if (gui_is_hot(ctx, label)) {
		if (lmb_state & GUI_KEYSTATE_PRESSED_BIT) {
			if (down) *down = GUI_TRUE;
			if (went_down) *went_down = GUI_TRUE;
			gui_set_active(ctx, label);
		}

		// Rmb should bring window to top
		if (rmb_state & GUI_KEYSTATE_PRESSED_BIT) {
			gui_set_active(ctx, label);
		}

		// For layout editor
		if (mmb_state & GUI_KEYSTATE_PRESSED_BIT)
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

GUI_BOOL gui_slider_ex(GuiContext *ctx, const char *label, float *value, float min, float max, float handle_rel_size, GUI_BOOL v, int length, GUI_BOOL show_text);

static void gui_begin_ex(GuiContext *ctx, const char *label, GUI_BOOL detached)
{
	GUI_ASSERT(ctx->turtle_ix < MAX_GUI_STACK_SIZE);
	if (ctx->turtle_ix >= MAX_GUI_STACK_SIZE)
		ctx->turtle_ix = 0; // Failsafe

	GuiContext_Turtle *prev = &ctx->turtles[ctx->turtle_ix];
	GuiContext_Turtle *cur = &ctx->turtles[++ctx->turtle_ix];

	GuiContext_Turtle new_turtle;
	GUI_ZERO(new_turtle);
	new_turtle.pos[0] = layout_property(ctx, label, "solved_pos_x");
	new_turtle.pos[1] = layout_property(ctx, label, "solved_pos_y");
	new_turtle.size[0] = layout_property(ctx, label, "solved_size_x");
	new_turtle.size[1] = layout_property(ctx, label, "solved_size_y");

	new_turtle.window_ix = prev->window_ix;
	new_turtle.layer = prev->layer + 1;
	memcpy(new_turtle.scissor, prev->scissor, sizeof(new_turtle.scissor));
	GUI_FMT_STR(new_turtle.label, MAX_GUI_LABEL_SIZE, "%s", label);
	new_turtle.padding[0] = layout_property(ctx, label, "padding_left");
	new_turtle.padding[1] = layout_property(ctx, label, "padding_top");
	new_turtle.padding[2] = layout_property(ctx, label, "padding_right");
	new_turtle.padding[3] = layout_property(ctx, label, "padding_bottom");
	new_turtle.gap[0] = layout_property(ctx, label, "gap_x");
	new_turtle.gap[1] = layout_property(ctx, label, "gap_y");
	*cur = new_turtle;

	{ // Record element
		GuiContext_Element elem;
		GUI_ZERO(elem);
		GUI_FMT_STR(elem.label, sizeof(elem.label), "%s", gui_turtle(ctx)->label);
		GUI_V2(elem.manual_offset[c] = prev->manual_offset[c]);
		elem.depth = ctx->turtle_ix;
		elem.detached = detached;
		gui_turtle(ctx)->element_ix = ctx->element_count;
		gui_append_element(ctx, elem);
	}

	if (layout_property(ctx, label, "scrollable")) { // Scrolling
		const char *needs_scroll_str[2] = { "needs_scroll_x", "needs_scroll_y" };
		GUI_BOOL needs_scroll[2];
		GUI_V2(needs_scroll[c] = layout_property(ctx, label, needs_scroll_str[c]));

		const char *scroll_str[2] = { "scroll_x", "scroll_y" };
		int scroll[2];
		GUI_V2(scroll[c] = layout_property(ctx, label, scroll_str[c]));

		const char *solved_content_size_str[2] = { "solved_content_size_x", "solved_content_size_y" };
		int content_size[2];
		GUI_V2(content_size[c] = layout_property(ctx, label, solved_content_size_str[c]));

		GUI_DECL_V2(int, slider_size,	layout_property(ctx, "gui_content_slider_y", "size_x"),
										layout_property(ctx, "gui_content_slider_x", "size_y"));

		int size[2];
		GUI_V2(size[c] = new_turtle.size[c]);

		int slider_cut[2];
		GUI_V2(slider_cut[c] = needs_scroll[!c]*slider_size[c]);

		int max_scroll[2];
		GUI_V2(max_scroll[c] = content_size[c] - size[c] + slider_cut[c]);
		GUI_V2(max_scroll[c] = GUI_MAX(max_scroll[c], 0));

		for (int d = 0; d < 2; ++d) {
			char scroll_panel_label[MAX_GUI_LABEL_SIZE];
			GUI_FMT_STR(scroll_panel_label, sizeof(scroll_panel_label), "gui_scroll_panel+%i+%s", d, label);
			gui_begin_detached(ctx, scroll_panel_label);

			if (needs_scroll[d]) {
				gui_turtle(ctx)->layer += GUI_LAYERS_PER_WINDOW/2; // Make topmost in window @todo Then should move this to end_window
				char scroll_label[MAX_GUI_LABEL_SIZE];
				GUI_FMT_STR(scroll_label, sizeof(scroll_label), "gui_content_slider_%c+%i+%s",
																d ? 'y' : 'x', d, label);
				if (	d == 1 && // Vertical
						gui_focused(ctx) &&
						ctx->mouse_scroll != 0 && // Scrolling mouse wheel
						!(ctx->key_state[GUI_KEY_LCTRL] & GUI_KEYSTATE_DOWN_BIT)) // Not holding ctrl
					scroll[d] -= ctx->mouse_scroll*64;

				// Scroll by dragging
				if (	gui_turtle(ctx)->window_ix == ctx->active_win_ix &&
						(ctx->key_state[GUI_KEY_LCTRL] & GUI_KEYSTATE_DOWN_BIT) &&
						(ctx->key_state[GUI_KEY_LMB] & GUI_KEYSTATE_DOWN_BIT)) {
					if (!ctx->dragging) {
						float v[2];
						GUI_V2(v[c] = (float)scroll[c]);
						gui_start_dragging(ctx, v);
					} else {
						int v[2];
						GUI_V2(v[c] = (int)ctx->drag_start_value[c]);
						scroll[d] = v[d] + ctx->drag_start_pos[d] - ctx->cursor_pos[d];
					}
				}

				float scroll_v = 1.f*scroll[d];
				float rel_shown_area = 1.f*(size[d] - slider_cut[d])/content_size[d];
				float max_comp_scroll = 1.f*max_scroll[d];
				gui_slider_ex(	ctx, scroll_label, &scroll_v, 0, max_comp_scroll, rel_shown_area, !!d,
								size[d] - slider_size[d], GUI_FALSE);
				scroll[d] = (int)scroll_v;
			}
			gui_end(ctx);
		}

		GUI_V2(scroll[c] = GUI_CLAMP(scroll[c], 0, max_scroll[c]));
		GUI_V2(gui_update_layout_property(ctx, label, scroll_str[c], scroll[c]));
	}
}

void gui_begin(GuiContext *ctx, const char *label)
{ gui_begin_ex(ctx, label, GUI_FALSE); }

void gui_begin_detached(GuiContext *ctx, const char *label)
{ gui_begin_ex(ctx, label, GUI_TRUE); }

void gui_end(GuiContext *ctx)
{ gui_end_ex(ctx, NULL); }

void gui_end_droppable(GuiContext *ctx, DragDropData *dropdata)
{ gui_end_ex(ctx, dropdata); }

void gui_end_ex(GuiContext *ctx, DragDropData *dropdata)
{
	// Initialize outputs
	if (dropdata) {
		GUI_ZERO(*dropdata);
	}

	{ // Dragging stop logic
		if (	!(ctx->key_state[GUI_KEY_LMB] & GUI_KEYSTATE_DOWN_BIT) || // Mouse released
				(ctx->key_state[GUI_KEY_LCTRL] & GUI_KEYSTATE_DOWN_BIT && ctx->mouse_scroll)) { // Scrolling when xy dragging

			// This doesn't need to be here. Once per frame would be enough.
			ctx->dragging = GUI_FALSE;
		}
	}

	//GUI_BOOL detached = gui_turtle(ctx)->detached;
	GUI_ASSERT(ctx->turtle_ix > 0);
	--ctx->turtle_ix;

}

GUI_BOOL gui_slider_ex(GuiContext *ctx, const char *label, float *value, float min, float max, float handle_rel_size, GUI_BOOL v, int length, GUI_BOOL show_text)
{
	GUI_BOOL ret = GUI_FALSE;

	gui_begin(ctx, label);

	int *pos = gui_turtle(ctx)->pos;
	int *size = gui_turtle(ctx)->size;
	int *padding = gui_turtle(ctx)->padding;
	int *gap = gui_turtle(ctx)->gap;

	if (length > 0)
		size[v] = length;

	int text_size[2] = {};
	if (show_text && strlen(gui_label_text(label)) > 0) {
		ctx->calc_text_size(text_size, ctx->calc_text_size_user_data, gui_label_text(label));
		//GUI_V2(size[c] = GUI_MAX(text_size[c] + padding[c] + (!c)*gap[0] + padding[c + 2], size[c]));
	}
	// @todo Correct padding[0] behavior
	int bar_size[2];
	GUI_BOOL has_text = (text_size[0] > 0);
	GUI_V2(bar_size[c] = size[c] - (!c)*(text_size[0] + gap[0] + padding[2])*has_text);

	// @todo Correct min_size[1]
	int min_size[2];
	GUI_V2(min_size[c] = (!c)*(text_size[0] + gap[0] + padding[0])*has_text + (!c)*50);
	min_size[1] = text_size[1];
	gui_set_min_size(ctx, label, min_size);

	const int scroll_handle_height = GUI_MAX((int)(handle_rel_size*bar_size[v]), 10);

	GUI_BOOL went_down = GUI_FALSE, down = GUI_FALSE, hover = GUI_FALSE;
	if (gui_is_inside_window(ctx, size)) {
		gui_button_logic(ctx, label, pos, bar_size, NULL, &went_down, &down, &hover);

		if (went_down) {
			GUI_DECL_V2(float, tmp, *value, 0);
			gui_start_dragging(ctx, tmp);
		}

		if (down && ctx->dragging) {
			int px_delta = ctx->cursor_pos[v] - ctx->drag_start_pos[v];
			*value = ctx->drag_start_value[0] + 1.f*px_delta / (bar_size[v] - scroll_handle_height) *(max - min);
			ret = GUI_TRUE;
		}
		*value = GUI_CLAMP(*value, min, max);

		{ // Draw
			{ // Bg
				int px_pos[2], px_size[2];
				pt_to_px(px_pos, pos, ctx->dpi_scale);
				pt_to_px(px_size, bar_size, ctx->dpi_scale);
				gui_draw(	ctx, GuiDrawInfo_slider, px_pos, px_size, GUI_FALSE, GUI_FALSE, GUI_FALSE,
							NULL, gui_layer(ctx), gui_scissor(ctx));
			}

			{ // Handle
				float rel_scroll = (*value - min) / (max - min);
				int handle_pos[2];
				GUI_ASSIGN_V2(handle_pos, pos);
				handle_pos[v] += (int)(rel_scroll*(bar_size[v] - scroll_handle_height));
				int handle_size[2];
				GUI_ASSIGN_V2(handle_size, bar_size);
				handle_size[v] = scroll_handle_height;

				int px_pos[2], px_size[2];
				pt_to_px(px_pos, handle_pos, ctx->dpi_scale);
				pt_to_px(px_size, handle_size, ctx->dpi_scale);
				gui_draw(	ctx, GuiDrawInfo_slider_handle, px_pos, px_size, hover, down, GUI_FALSE,
							NULL, gui_layer(ctx) + 1, gui_scissor(ctx));
			}

			if (show_text) { // Label
				int text_pos[2];
				text_pos[0] = pos[0] + bar_size[0] + gap[0];
				text_pos[1] = pos[1] + padding[1];
				int px_pos[2];
				pt_to_px(px_pos, text_pos, ctx->dpi_scale);
				gui_draw(	ctx, GuiDrawInfo_text, px_pos, px_pos, GUI_FALSE, GUI_FALSE, GUI_FALSE,
							gui_label_text(label), gui_layer(ctx) + 2, gui_scissor(ctx));
			}

			if (show_text) { // Value
				int value_text_pos[2], value_text_size[2];
				const char *value_text = gui_str(ctx, "%.3f", *value);
				ctx->calc_text_size(value_text_size, ctx->calc_text_size_user_data, value_text);
				value_text_pos[0] = pos[0] + (bar_size[0] - value_text_size[0])/2;
				value_text_pos[1] = pos[1];

				int px_pos[2], px_size[2];
				pt_to_px(px_pos, value_text_pos, ctx->dpi_scale);
				pt_to_px(px_size, value_text_size, ctx->dpi_scale);
				gui_draw(	ctx, GuiDrawInfo_text, px_pos, px_size, GUI_FALSE, GUI_FALSE, GUI_FALSE,
							value_text, gui_layer(ctx) + 2, gui_scissor(ctx));
			}
		}
	}

	gui_end(ctx);

	return ret;
}

static void gui_lift_window_to_top(GuiContext *ctx, int win_handle)
{
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

// order_style: -1 behind all, 0 normal, 1 in front of all
void gui_begin_window_ex(GuiContext *ctx, const char *base_label, GUI_BOOL has_bar, int order_style)
{
	const char *win_label = gui_prepend_layout(ctx, "gui_window", base_label);

	int *manual_offset = gui_turtle(ctx)->manual_offset;

	gui_begin_detached(ctx, win_label);
	int *pos = gui_turtle(ctx)->pos;
	int *size = gui_turtle(ctx)->size;

	int min_size[2];
	GUI_V2(min_size[c] = has_bar ? 80 : 10);
	gui_set_min_size(ctx, win_label, min_size);

	int win_handle = -1;
	{ // Find/create window
		int free_handle = -1;
		for (int i = 0; i < MAX_GUI_WINDOW_COUNT; ++i) {
			if (ctx->windows[i].id == 0 && free_handle == -1)
				free_handle = i;
			if (ctx->windows[i].id == gui_id(win_label)) {
				win_handle = i;
				break;
			}
		}
		if (win_handle == -1) {
			GUI_ASSERT(free_handle >= 0);
			// Create new window
			GUI_ASSERT(ctx->window_count < MAX_GUI_WINDOW_COUNT);

			GuiContext_Window *win = &ctx->windows[free_handle];
			win->id = gui_id(win_label);
			GUI_FMT_STR(win->label, sizeof(win->label), "%s", win_label);
			win->has_bar = has_bar;
			win->minimized = ctx->create_next_window_minimized;
			if (order_style == -1) {
				// e.g. panels are created behind every window
				memmove(&ctx->window_order[1], &ctx->window_order[0], sizeof(*ctx->window_order)*ctx->window_count);
				ctx->window_order[0] = free_handle;
				++ctx->window_count;
			} else {
				ctx->window_order[ctx->window_count++] = free_handle;
			}

			win_handle = free_handle;
		} else {
			if (order_style == 1)
				gui_lift_window_to_top(ctx, win_handle);
		}
	}
	GUI_ASSERT(win_handle >= 0);
	GuiContext_Window *win = &ctx->windows[win_handle];
	if (win->used) {
		GUI_PRINTF("Same window used twice in a frame: %s\n", win_label);
		GUI_ASSERT(0 && "See printed error message");
	}
	win->used = GUI_TRUE;
	win->remove_when_not_used = ctx->dont_save_next_window_layout;
	if (!win->used_in_last_frame && ctx->focused_win_ix != GUI_BG_WINDOW_IX)
		ctx->focused_win_ix = win_handle; // Appearing window will be focused

	gui_turtle(ctx)->window_ix = win_handle;
	gui_turtle(ctx)->layer = ctx->base_layer + 1337 + gui_window_order(ctx, win_handle)*GUI_LAYERS_PER_WINDOW;
	win->bar_height = win->has_bar ? layout_property(ctx, "gui_bar", "size_y") : 0;

	GUI_BOOL interacted = GUI_FALSE;
	GUI_BOOL minimized = win->minimized;
	if (minimized)
		size[1] = win->bar_height;

	char client_label[MAX_GUI_LABEL_SIZE];
	gui_modified_id_label(client_label, gui_prepend_layout(ctx, "gui_client", base_label), "_client");
	const char *solved_pos_str[2] = { "solved_pos_x", "solved_pos_y" };
	const char *solved_size_str[2] = { "solved_size_x", "solved_size_y" };
	// Client are size on screen - not content size which can go outside window
	int c_pos[2];
	int c_size[2];
	GUI_V2(c_pos[c] = layout_property(ctx, win_label, solved_pos_str[c]));
	GUI_V2(c_size[c] = layout_property(ctx, win_label, solved_size_str[c]));

	GUI_DECL_V2(int, slider_size,	layout_property(ctx, "gui_content_slider_y", "size_x"),
									layout_property(ctx, "gui_content_slider_x", "size_y"));
	GUI_ASSIGN_V2(win->slider_width, slider_size);
	c_pos[1] += win->has_bar*win->bar_height;
	GUI_V2(c_size[c] = size[c] - c*win->bar_height);
	GUI_V2(win->client_size[c] = c_size[c]);

	GUI_V2(win->recorded_pos[c] = pos[c]);

	{ // Bg panel
		int px_pos[2], px_size[2];
		pt_to_px(px_pos, pos, ctx->dpi_scale);
		pt_to_px(px_size, size, ctx->dpi_scale);
		gui_draw(	ctx, GuiDrawInfo_panel, px_pos, px_size, GUI_FALSE, GUI_FALSE, gui_focused(ctx),
					NULL, gui_layer(ctx), gui_scissor(ctx));
	}

	if (win->has_bar) { // Title bar logic
		char bar_label[MAX_GUI_LABEL_SIZE];
		gui_modified_id_label(bar_label, gui_prepend_layout(ctx, "gui_bar", base_label), "_bar");
		GUI_BOOL went_down, down, hover;
		GUI_DECL_V2(int, btn_size, size[0] - win->bar_height, win->bar_height);

		gui_begin(ctx, bar_label);
		gui_button_logic(ctx, bar_label, pos, btn_size, NULL, &went_down, &down, &hover);

		if (ctx->active_win_ix == win_handle && order_style >= 0)
			gui_lift_window_to_top(ctx, win_handle);

		if (went_down) {
			float v[2];
			GUI_V2(v[c] = (float)pos[c]);
			gui_start_dragging(ctx, v);
		}

		if (down && ctx->dragging) {
			GUI_V2(pos[c] = (int)ctx->drag_start_value[c] - ctx->drag_start_pos[c] + ctx->cursor_pos[c]);
			interacted = GUI_TRUE;
		}

		if (!ctx->allow_next_window_outside) {
			const int margin = 20;
			pos[0] = GUI_CLAMP(pos[0], margin - size[0], ctx->host_win_size[0] - margin);
			pos[1] = GUI_CLAMP(pos[1], 0, ctx->host_win_size[1] - margin);
		}

		int px_pos[2], px_size[2];
		pt_to_px(px_pos, pos, ctx->dpi_scale);
		pt_to_px(px_size, size, ctx->dpi_scale);
		px_size[1] = win->bar_height;
		gui_draw(	ctx, GuiDrawInfo_title_bar, px_pos, px_size, GUI_FALSE, GUI_FALSE, gui_focused(ctx),
					gui_label_text(base_label), gui_layer(ctx) + 1, gui_scissor(ctx));

		{ // Minimize button
			GUI_DECL_V2(int, px_pos, pos[0] + size[0] - win->bar_height, pos[1]);
			GUI_DECL_V2(int, px_box_size, win->bar_height, win->bar_height);

			char box_label[MAX_GUI_LABEL_SIZE];
			gui_modified_id_label(box_label, gui_prepend_layout(ctx, "gui_minimize", base_label), "+minimize");

			GUI_BOOL went_up, down, hover;
			gui_button_logic(ctx, box_label, px_pos, px_box_size, &went_up, NULL, &down, &hover);
			if (went_up)
				win->minimized = !win->minimized;

			gui_draw(	ctx, GuiDrawInfo_button, px_pos, px_box_size, hover, down, false,
						NULL, gui_layer(ctx) + 2, gui_scissor(ctx));
		}

		gui_end(ctx);
	}

	// Corner resize handle
	if (!layout_property(ctx, win_label, "prevent_resizing") && !minimized) {
		char resize_label[MAX_GUI_LABEL_SIZE];
		gui_modified_id_label(resize_label, 	gui_prepend_layout(ctx, "gui_resize", base_label),
													"_resize");
		gui_begin_detached(ctx, resize_label);
		gui_turtle(ctx)->layer += GUI_LAYERS_PER_WINDOW/2; // Make topmost in window @todo Then should move this to end_window

		int handle_size[2] = {slider_size[0], slider_size[1]};
		int handle_pos[2];
		GUI_V2(handle_pos[c] = pos[c] + size[c] - handle_size[c]);
		GUI_BOOL went_down, down, hover;
		gui_button_logic(ctx, resize_label, handle_pos, handle_size, NULL, &went_down, &down, &hover);

		if (went_down) {
			float v[2];
			GUI_V2(v[c] = (float)size[c]);
			gui_start_dragging(ctx, v);
		}

		if (down) {
			GUI_V2(size[c] = (int)ctx->drag_start_value[c] + ctx->cursor_pos[c] - ctx->drag_start_pos[c]);
			interacted = GUI_TRUE;
		}
		GUI_V2(size[c] = GUI_MAX(size[c], min_size[c]));

		int px_pos[2], px_size[2];
		pt_to_px(px_pos, handle_pos, ctx->dpi_scale);
		pt_to_px(px_size, handle_size, ctx->dpi_scale);
		gui_draw(	ctx, GuiDrawInfo_resize_handle, px_pos, px_size, hover, down, GUI_FALSE,
					NULL, gui_layer(ctx), gui_scissor(ctx));
		gui_end(ctx);
	}

	//
	// Client-area content
	//

	gui_begin(ctx, client_label);

	// Make clicking frame background change last active element, so that scrolling works
	gui_button_logic(ctx, base_label, c_pos, c_size, NULL, NULL, NULL, NULL);

	int scissor[4];
	scissor[0] = c_pos[0];
	scissor[1] = c_pos[1];
	scissor[2] = c_size[0];
	scissor[3] = c_size[1];
	memcpy(gui_turtle(ctx)->scissor, scissor, sizeof(scissor));

	if (interacted) {
		// Save window pos and size to layout
		GUI_BOOL save = !ctx->dont_save_next_window_layout;
		gui_update_layout_property_ex(ctx, win_label, "offset_x", pos[0] - manual_offset[0], save);
		gui_update_layout_property_ex(ctx, win_label, "offset_y", pos[1] - manual_offset[1], save);
		if (!minimized) {
			gui_update_layout_property_ex(ctx, win_label, "size_x", size[0], save);
			gui_update_layout_property_ex(ctx, win_label, "size_y", size[1], save);
		}
	}

	ctx->allow_next_window_outside = GUI_FALSE;
	ctx->create_next_window_minimized = GUI_FALSE;
	ctx->dont_save_next_window_layout = GUI_FALSE;
}

void gui_end_window_ex(GuiContext *ctx)
{
	gui_end(ctx); // client area
	gui_end(ctx); // window area
}

void gui_begin_window(GuiContext *ctx, const char *win_label)
{ gui_begin_window_ex(ctx, win_label, GUI_TRUE, 0); }

void gui_end_window(GuiContext *ctx)
{ gui_end_window_ex(ctx); }

void gui_begin_panel(GuiContext *ctx, const char *win_label)
{ gui_begin_window_ex(ctx, win_label, GUI_FALSE, -1); }

void gui_end_panel(GuiContext *ctx)
{ gui_end_window_ex(ctx); }

void gui_window_client_size(GuiContext *ctx, int *w, int *h)
{
	*w = gui_window(ctx)->client_size[0];
	*h = gui_window(ctx)->client_size[1];
}

void gui_window_pos(GuiContext *ctx, int *x, int *y)
{
	*x = gui_window(ctx)->recorded_pos[0];
	*y = gui_window(ctx)->recorded_pos[1];
}

GUI_BOOL gui_is_window_open(GuiContext *ctx)
{ return !gui_window(ctx)->minimized; }

static GUI_BOOL gui_button_ex(GuiContext *ctx, const char *label, GUI_BOOL force_down)
{
	gui_begin(ctx, label);
	int *pos = gui_turtle(ctx)->pos;
	int *size = gui_turtle(ctx)->size;
	int *padding = gui_turtle(ctx)->padding;

	// @todo Recalc size only when text changes
	int text_size[2] = {0};
	ctx->calc_text_size(text_size, ctx->calc_text_size_user_data, gui_label_text(label));

	int min_size[2];
	GUI_V2(min_size[c] = (int)text_size[c] + padding[c] + padding[c + 2]);
	gui_set_min_size(ctx, label, min_size);

	GUI_BOOL went_up = GUI_FALSE, hover = GUI_FALSE, down = GUI_FALSE;
	if (gui_is_inside_window(ctx, size)) {
		gui_button_logic(ctx, label, pos, size, &went_up, NULL, &down, &hover);

		if (went_up)
			ctx->interacted_id = gui_id(label);

		int px_pos[2], px_size[2];
		pt_to_px(px_pos, pos, ctx->dpi_scale);
		pt_to_px(px_size, size, ctx->dpi_scale);
		gui_draw(	ctx, GuiDrawInfo_button, px_pos, px_size, hover, down || force_down, GUI_FALSE,
					NULL, gui_layer(ctx), gui_scissor(ctx));

		int px_padding[2];
		pt_to_px(px_padding, padding, ctx->dpi_scale);
		GUI_V2(px_pos[c] += px_padding[c]);
		gui_draw(	ctx, GuiDrawInfo_text, px_pos, px_size, GUI_FALSE, GUI_FALSE, GUI_FALSE,
					gui_label_text(label), gui_layer(ctx) + 1, gui_scissor(ctx));
	}

	gui_end(ctx);

	return went_up && hover;
}

GUI_BOOL gui_begin_contextmenu(GuiContext *ctx, const char *label, GuiId element_id)
{
	if (	ctx->last_hot_id == element_id &&
			(ctx->key_state[GUI_KEY_RMB] & GUI_KEYSTATE_PRESSED_BIT)) {
		ctx->open_contextmenu_id = gui_id(label);
		gui_update_layout_property(ctx, label, "offset_x", ctx->cursor_pos[0]);
		gui_update_layout_property(ctx, label, "offset_y", ctx->cursor_pos[1]);
	}

	if (ctx->open_contextmenu_id == gui_id(label)) {
		// User calls gui_end_contextmenu
		ctx->dont_save_next_window_layout = GUI_TRUE;
		gui_begin_window_ex(ctx, gui_prepend_layout(ctx, "gui_contextmenu", label), GUI_FALSE, 1);
		if (!gui_focused(ctx))
			ctx->open_contextmenu_id = 0;

		return GUI_TRUE;
	}

	return GUI_FALSE;
}

void gui_end_contextmenu(GuiContext *ctx)
{ gui_end_window_ex(ctx); }

void gui_close_contextmenu(GuiContext *ctx)
{ ctx->open_contextmenu_id = 0; }

GUI_BOOL gui_contextmenu_item(GuiContext *ctx, const char *label)
{ return gui_button_ex(ctx, gui_prepend_layout(ctx, "gui_button:gui_contextmenu_item", label), GUI_FALSE); }

GUI_BOOL gui_interacted(GuiContext *ctx, GuiId element_id)
{ return ctx->interacted_id == element_id; }

GUI_BOOL gui_button(GuiContext *ctx, const char *label)
{ return gui_button_ex(ctx, gui_prepend_layout(ctx, "gui_button", label), GUI_FALSE); }

GUI_BOOL gui_selectable(GuiContext *ctx, const char *label, GUI_BOOL selected)
{ return gui_button_ex(ctx, gui_prepend_layout(ctx, "gui_button", label), selected); }

GUI_BOOL gui_checkbox_ex(GuiContext *ctx, const char *label, GUI_BOOL *value, GUI_BOOL radio_button_visual)
{
	label = gui_prepend_layout(ctx, "gui_checkbox", label);
	gui_begin(ctx, label);
	int *pos = gui_turtle(ctx)->pos;
	int *size = gui_turtle(ctx)->size;
	int *padding = gui_turtle(ctx)->padding;
	int *gap = gui_turtle(ctx)->gap;

	int text_size[2];
	ctx->calc_text_size(text_size, ctx->calc_text_size_user_data, gui_label_text(label));
	int min_size[2];
	GUI_V2(min_size[c] = text_size[c] + padding[c] + padding[c + 2]);
	GUI_V2(size[c] = GUI_MAX(size[c], min_size[c]));
	gui_set_min_size(ctx, label, min_size);

	GUI_BOOL went_up = GUI_FALSE, hover = GUI_FALSE, down = GUI_FALSE;
	if (gui_is_inside_window(ctx, size)) {
		int px_padding[4];
		pt_to_px(&px_padding[0], &padding[0], ctx->dpi_scale);
		pt_to_px(&px_padding[2], &padding[2], ctx->dpi_scale);
		int px_gap[2];
		pt_to_px(px_gap, gap, ctx->dpi_scale);
		GUI_DECL_V2(int, box_size, size[1], size[1]);
		int px_box_size[2];
		pt_to_px(px_box_size, box_size, ctx->dpi_scale);

		gui_button_logic(ctx, label, pos, size, &went_up, NULL, &down, &hover);

		int px_pos[2];
		pt_to_px(px_pos, pos, ctx->dpi_scale);

		if (radio_button_visual)
			gui_draw(	ctx, GuiDrawInfo_radiobutton, px_pos, px_box_size, hover, down, *value,
						NULL, gui_layer(ctx), gui_scissor(ctx));
		else
			gui_draw(	ctx, GuiDrawInfo_checkbox, px_pos, px_box_size, hover, down, *value,
						NULL, gui_layer(ctx), gui_scissor(ctx));

		// @todo Correct padding[0] behavior
		px_pos[0] += px_box_size[0] + px_gap[0];
		px_pos[1] += px_padding[1];
		gui_draw(	ctx, GuiDrawInfo_text, px_pos, px_box_size, GUI_FALSE, GUI_FALSE, GUI_FALSE,
					gui_label_text(label), gui_layer(ctx) + 1, gui_scissor(ctx));
	}

	gui_end(ctx);

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

GUI_BOOL gui_slider(GuiContext *ctx, const char *label, float *value, float min, float max)
{
	return gui_slider_ex(ctx, gui_prepend_layout(ctx, "gui_slider_x", label), value, min, max, 0.1f, GUI_FALSE, 0, GUI_TRUE);
}

GUI_BOOL gui_slider_double(GuiContext *ctx, const char *label, double *value, double min, double max)
{
	// @todo gui_slider_ex should use doubles instead of floats
	float val = *value;
	GUI_BOOL ret = gui_slider_ex(ctx, gui_prepend_layout(ctx, "gui_slider_x", label), &val, min, max, 0.1f, GUI_FALSE, 0, GUI_TRUE);
	*value = val;
	return ret;
}

static GUI_BOOL gui_textfield_ex(GuiContext *ctx, const char *label, char *buf, int buf_size, GUI_BOOL int_only, GUI_BOOL *input_completed)
{
	GUI_BOOL content_changed = GUI_FALSE;
	if (input_completed)
		*input_completed = GUI_FALSE;

	gui_begin(ctx, label);
	int label_pos[2], box_size[2];
	int label_size[2] = {0, 0};
	int *pos = gui_turtle(ctx)->pos;
	int *size = gui_turtle(ctx)->size;
	int *padding = gui_turtle(ctx)->padding;
	int *gap = gui_turtle(ctx)->gap;
	GUI_BOOL has_label = (strlen(gui_label_text(label)) > 0);
	if (has_label) {
		ctx->calc_text_size(label_size, ctx->calc_text_size_user_data, gui_label_text(label));

		// @todo Correct padding[0] behavior
		const int min_box_size[2] = {20, 0};
		GUI_V2(size[c] = GUI_MAX(size[c], min_box_size[c] + label_size[c] + padding[c] + padding[c + 2]));
		label_pos[0] = pos[0] + size[0] - (label_size[0] + padding[2]);
		label_pos[1] = pos[1] + padding[1];
		box_size[0] = size[0] - (label_size[0] + padding[0] + padding[2] + gap[0]);
		box_size[1] = size[1];
	} else {
		GUI_ASSIGN_V2(label_pos, pos);
		GUI_ASSIGN_V2(box_size, size);
	}

	GUI_BOOL went_down = GUI_FALSE, hover = GUI_FALSE;
	if (gui_is_inside_window(ctx, size)) {
		gui_button_logic(ctx, label, pos, size, NULL, &went_down, NULL, &hover);
		GUI_BOOL active = (ctx->last_active_id == gui_id(label));

		if (active) {
			GUI_ASSERT(buf && buf_size > 0);
			int char_count = (int)strlen(buf);
			for (int i = 0; i < ctx->written_char_count; ++i) {
				if (char_count >= buf_size)
					break;
				char ch = ctx->written_text_buf[i];
				content_changed = GUI_TRUE;

				if (ch == '\n') {
					gui_set_inactive(ctx, gui_id(label));
					if (input_completed)
						*input_completed = GUI_TRUE;
					break;
				}

				if (ch == '\b') {
					if (char_count > 0)
						buf[--char_count] = '\0';
				} else {
					if (int_only) {
						if ((ch < '0' || ch > '9') && ch != '.' && ch != '-')
							continue;
					}
					buf[char_count++] = ch;
				}
			}
			char_count = GUI_MIN(char_count, buf_size - 1);
			buf[char_count] = '\0';

			ctx->has_input = GUI_TRUE;
		}

		int px_padding[2];
		pt_to_px(px_padding, padding, ctx->dpi_scale);
		if (has_label) { // Draw label
			int px_pos[2];
			int px_size[2] = {0};
			pt_to_px(px_pos, label_pos, ctx->dpi_scale);
			gui_draw(	ctx, GuiDrawInfo_text, px_pos, px_size, GUI_FALSE, GUI_FALSE, GUI_FALSE,
						gui_label_text(label), gui_layer(ctx) + 1, gui_scissor(ctx));
		}

		{ // Draw textbox
			int px_pos[2], px_size[2];
			pt_to_px(px_pos, pos, ctx->dpi_scale);
			pt_to_px(px_size, box_size, ctx->dpi_scale);
			// @todo down --> active
			gui_draw(	ctx, GuiDrawInfo_textbox, px_pos, px_size, hover, GUI_FALSE, active,
						NULL, gui_layer(ctx), gui_scissor(ctx));
			int scissor[4] = {px_pos[0], px_pos[1], px_size[0], px_size[1]};
			gui_combine_scissor(scissor, gui_scissor(ctx));
			px_pos[0] += 2; // @todo To layout
			gui_draw(	ctx, GuiDrawInfo_text, px_pos, px_size, GUI_FALSE, GUI_FALSE, GUI_FALSE,
						buf, gui_layer(ctx) + 1, scissor);
		}

	}

	gui_end(ctx);

	return content_changed;
}

GUI_BOOL gui_textfield(GuiContext *ctx, const char *label, char *buf, int buf_size)
{ return gui_textfield_ex(ctx, gui_prepend_layout(ctx, "gui_textfield", label), buf, buf_size, GUI_FALSE, NULL); }

GUI_BOOL gui_intfield(GuiContext *ctx, const char *label, int *value)
{
	label = gui_prepend_layout(ctx, "gui_textfield", label);

	char local_buf[GUI_TEXTFIELD_BUF_SIZE] = {0};
	if (ctx->textfield_buf_owner != gui_id(label))
		GUI_FMT_STR(local_buf, sizeof(local_buf), "%i", *value);
	else
		GUI_FMT_STR(local_buf, sizeof(local_buf), "%s", ctx->textfield_buf);


	GUI_BOOL completed;
	GUI_BOOL ret = gui_textfield_ex(ctx, label, local_buf, sizeof(local_buf), GUI_TRUE, &completed);
	if (ret) {
		// Text changed
		ctx->textfield_buf_owner = gui_id(label);
		GUI_FMT_STR(ctx->textfield_buf, sizeof(ctx->textfield_buf), "%s", local_buf);

		// May fail
		if (completed)
			sscanf(local_buf, "%i", value);
	}

	if (ctx->textfield_buf_owner == gui_id(label) && ctx->last_active_id != gui_id(label))
		ctx->textfield_buf_owner = 0;

	return ret;
}

GUI_BOOL gui_doublefield(GuiContext *ctx, const char *label, double *value)
{
	label = gui_prepend_layout(ctx, "gui_textfield", label);

	char local_buf[GUI_TEXTFIELD_BUF_SIZE] = {0};
	if (ctx->textfield_buf_owner != gui_id(label))
		GUI_FMT_STR(local_buf, sizeof(local_buf), "%f", *value);
	else
		GUI_FMT_STR(local_buf, sizeof(local_buf), "%s", ctx->textfield_buf);

	GUI_BOOL completed;
	GUI_BOOL ret = gui_textfield_ex(ctx, label, local_buf, sizeof(local_buf), GUI_TRUE, &completed);
	if (ret) {
		// Text changed
		ctx->textfield_buf_owner = gui_id(label);
		GUI_FMT_STR(ctx->textfield_buf, sizeof(ctx->textfield_buf), "%s", local_buf);

		// May fail
		if (completed)
			sscanf(local_buf, "%lf", value);
	}

	if (ctx->textfield_buf_owner == gui_id(label) && ctx->last_active_id != gui_id(label))
		ctx->textfield_buf_owner = 0;

	return ret;
}

GUI_BOOL gui_floatfield(GuiContext *ctx, const char *label, float *value)
{
	label = gui_prepend_layout(ctx, "gui_textfield", label);

	double v = *value;
	GUI_BOOL ret = gui_doublefield(ctx, label, &v);
	*value = (float)v;
	return ret;
}

void gui_label(GuiContext *ctx, const char *label)
{
	gui_begin(ctx, label);
	int *pos = gui_turtle(ctx)->pos;
	int *size = gui_turtle(ctx)->size;
	ctx->calc_text_size(size, ctx->calc_text_size_user_data, gui_label_text(label));
	gui_set_min_size(ctx, label, size);

	if (gui_is_inside_window(ctx, size)) {
		gui_button_logic(ctx, label, pos, size, NULL, NULL, NULL, NULL);

		int px_pos[2], px_size[2];
		pt_to_px(px_pos, pos, ctx->dpi_scale);
		pt_to_px(px_size, size, ctx->dpi_scale);
		gui_draw(	ctx, GuiDrawInfo_text, px_pos, px_size, GUI_FALSE, GUI_FALSE, GUI_FALSE,
					gui_label_text(label), gui_layer(ctx) + 1, gui_scissor(ctx));
	}

	gui_end(ctx);
}


void gui_begin_listbox(GuiContext *ctx, const char *label)
{
	gui_begin(ctx, label);
	// @todo Clipping and scrollbar
}

void gui_end_listbox(GuiContext *ctx)
{
	gui_end(ctx);
}

GUI_BOOL gui_begin_combo(GuiContext *ctx, const char *label)
{
	// @todo Use borderless window for list
	int combo_pos[2];
	gui_turtle_pos(ctx, &combo_pos[0], &combo_pos[1]);
	if (gui_button(ctx, label))
		ctx->open_combo_id = gui_id(label);
	GUI_DECL_V2(int, list_start_pos, combo_pos[0], combo_pos[1]);

	if (ctx->open_combo_id == gui_id(label)) {
		gui_begin_detached(ctx, label); // User calls gui_end_combo()
		gui_set_turtle_pos(ctx, list_start_pos[0], list_start_pos[1]);
		return GUI_TRUE;
	}

	return GUI_FALSE;
}

GUI_BOOL gui_combo_item(GuiContext *ctx, const char *label)
{
	GUI_BOOL pressed = gui_button(ctx, label);
	if (pressed)
		ctx->open_combo_id = 0;
	return pressed;
}

void gui_end_combo(GuiContext *ctx)
{
	gui_end(ctx);
}

GUI_BOOL gui_begin_tree(GuiContext *ctx, const char *label)
{
	GUI_BOOL open = element_storage_bool(ctx, label, GUI_FALSE);
	if (gui_button(ctx, label)) {
		set_element_storage_bool(ctx, label, !open);
	}

	if (open) {
		char treenode_label[MAX_GUI_LABEL_SIZE];
		gui_modified_id_label(treenode_label, gui_prepend_layout(ctx, "gui_treenode", label), "+treenode");
		gui_begin(ctx, treenode_label);
		return GUI_TRUE;
	} else {
		return GUI_FALSE;
	}
}

void gui_end_tree(GuiContext *ctx)
{
	gui_end(ctx);
}

void gui_layout_editor(GuiContext *ctx, const char *save_path)
{
	if (ctx->active_id && (ctx->key_state[GUI_KEY_MMB] & GUI_KEYSTATE_DOWN_BIT)) {
		GUI_FMT_STR(ctx->layout_element_label, sizeof(ctx->layout_element_label),
					"%s", ctx->active_label);
	}

	{ gui_begin_window(ctx, "gui_layoutwin|Layout settings");

		gui_label(ctx, "gui_layout_list|Selected element (mmb):");
		gui_textfield(ctx, "gui_layout_list+name|name", ctx->layout_element_label, sizeof(ctx->layout_element_label));

		const char *layout_name;
		int layout_name_size;
		most_specific_layout(&layout_name, &layout_name_size, ctx->layout_element_label);
		layout_name = gui_str(ctx, "%.*s", layout_name_size, layout_name);
		gui_label(ctx, gui_str(ctx, "gui_layout_list+id|layout: %s, layout_id: %u",
									layout_name, gui_hash(layout_name, layout_name_size)));

		const char *props[] = {
			"on_same_row",
			"offset_x", "offset_y",
			"size_x", "size_y",
			"prevent_resizing",
			"resize_to_min_x", "resize_to_min_y",
			"align_left", "align_right", "align_top", "align_bottom",
			"use_parent_size_instead_of_content_size",
			"padding_left", "padding_top", "padding_right", "padding_bottom",
			"gap_x", "gap_y",
			"scrollable",
		};

		GUI_BOOL prop_is_bool[] = {
			GUI_TRUE,
			GUI_FALSE, GUI_FALSE,
			GUI_FALSE, GUI_FALSE,
			GUI_TRUE,
			GUI_TRUE, GUI_TRUE,
			GUI_TRUE, GUI_TRUE, GUI_TRUE, GUI_TRUE,
			GUI_TRUE,
			GUI_FALSE, GUI_FALSE, GUI_FALSE, GUI_FALSE,
			GUI_FALSE, GUI_FALSE,
			GUI_TRUE,
		};

		int prop_count = sizeof(props)/sizeof(*props);
		GUI_ASSERT(prop_count == sizeof(prop_is_bool)/sizeof(*prop_is_bool));

		int i;
		for (i = 0; i < prop_count; ++i) {
			const char *prop = props[i];

			gui_begin(ctx, gui_str(ctx, "gui_layout_list+prop_container_%i", i));

			GUI_BOOL own = has_own_layout_property(ctx, layout_name, prop);
			if (gui_checkbox(ctx, gui_str(ctx, "gui_layout_list_own+%s|", prop), &own)) {
				if (own)
					gui_update_layout_property(ctx, layout_name, prop, layout_property(ctx, layout_name, prop));
				else
					remove_layout_property(ctx, layout_name, prop);
			}

			if (prop_is_bool[i]) {
				GUI_BOOL value = layout_property(ctx, ctx->layout_element_label, prop);
				if (gui_checkbox(ctx, gui_str(ctx, "gui_layout_list_prop+%s|%s", prop, prop), &value))
					gui_update_layout_property(ctx, ctx->layout_element_label, prop, value);
			} else {
				int value = layout_property(ctx, ctx->layout_element_label, prop);
				if (gui_intfield(ctx, gui_str(ctx, "gui_layout_list_prop+%s|%s", prop, prop), &value))
					gui_update_layout_property(ctx, ctx->layout_element_label, prop, value);
			}

			gui_end(ctx);
		}

		if (gui_button(ctx, "gui_layout_list+save|Save layout")) {
			save_layout(ctx, save_path);
		}

	gui_end_window(ctx); }
}

