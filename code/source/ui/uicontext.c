#include "core/device.h"
#include "global/env.h"
#include "uicontext.h"

internal Color panel_color()
{ return (Color) {0.1/2, 0.1/2, 0.15/2, 0.9}; }

internal Color inactive_color()
{ return (Color) {0.2, 0.2, 0.2, 0.5}; }

internal Color darken_color(Color c)
{ return (Color) {c.r*0.8, c.g*0.8, c.b*0.8, c.a}; }

internal Color highlight_color(Color c)
{ return (Color) {c.r + 0.2, c.g + 0.2, c.b + 0.1, c.a}; }

internal const Font *gui_font()
{
	return (Font*)res_by_name(	g_env.resblob,
								ResType_Font,
								"dev");
}

internal void limit_by_scissor(float *x, float *y, float *w, float *h, GuiScissor *s)
{
	if (!s)
		return;
	if (s->pos[0] >= *x &&
		s->pos[1] >= *y &&
		s->pos[0] + s->size[0] <= *x + *w &&
		s->pos[1] + s->size[1] <= *y + *h)
		return;
	
	float p1[2] = {*x, *y};
	float p2[2] = {*x + *w, *y + *h};
	p1[0] = MAX(p1[0], s->pos[0]);
	p1[1] = MAX(p1[1], s->pos[1]);
	p2[0] = MIN(p2[0], s->pos[0] + s->size[0]);
	p2[1] = MIN(p2[1], s->pos[1] + s->size[1]);

	*x = p1[0];
	*y = p1[1];
	*w = MAX(p2[0] - p1[0], 0);
	*h = MAX(p2[1] - p1[1], 0);
}

void draw_button(void *user_data, float x, float y, float w, float h, bool down, bool hover, int layer, GuiScissor *scissor)
{
	limit_by_scissor(&x, &y, &w, &h, scissor);

	Color bg_color = darken_color(panel_color());
	if (down)
		bg_color = darken_color(bg_color);
	else if (hover)
		bg_color = highlight_color(bg_color);

	V2f p = {x, y};
	V2f s = {w, h};
	drawcmd_px_quad(v2f_to_v2i(p), v2f_to_v2i(s), bg_color, layer);
}

void draw_checkbox(void *user_data, float x, float y, float w, bool checked, bool down, bool hover, int layer, GuiScissor *scissor)
{
	float h = w;
	limit_by_scissor(&x, &y, &w, &h, scissor);

	Color bg_color = darken_color(panel_color());
	if (checked)
		bg_color = (Color) {0.2, 1, 0.2, 1};

	if (down)
		bg_color = darken_color(bg_color);
	else if (hover)
		bg_color = highlight_color(bg_color);

	V2f p = {x, y};
	V2f s = {w, h};
	drawcmd_px_quad(v2f_to_v2i(p), v2f_to_v2i(s), bg_color, layer);
}

void draw_radiobutton(void *user_data, float x, float y, float w, bool checked, bool down, bool hover, int layer, GuiScissor *scissor)
{
	float h = w;
	limit_by_scissor(&x, &y, &w, &h, scissor);

	Color bg_color = darken_color(panel_color());
	if (checked)
		bg_color = (Color) {1, 0.2, 0.2, 1};

	if (down)
		bg_color = darken_color(bg_color);
	else if (hover)
		bg_color = highlight_color(bg_color);

	// @todo Rotate quad
	V2f p = {x, y};
	V2f s = {w, h};
	drawcmd_px_quad(v2f_to_v2i(p), v2f_to_v2i(s), bg_color, layer);
}

void draw_textbox(void *user_data, float x, float y, float w, float h, bool active, bool hover, int layer, GuiScissor *scissor)
{
	draw_button(user_data, x, y, w, h, false, active || hover, layer, scissor);
}

void draw_text(void *user_data, float x, float y, const char *text, int layer, GuiScissor *s)
{
	const U32 max_quad_count = strlen(text);
	const U32 max_vert_count = 4*max_quad_count;
	const U32 max_ind_count = 6*max_quad_count;
	TriMeshVertex *verts = frame_alloc(sizeof(*verts)*max_vert_count);
	MeshIndexType *inds = frame_alloc(sizeof(*inds)*max_ind_count);
	V2i size;
	U32 quad_count = text_mesh(&size, verts, inds, gui_font(), text);
	const U32 v_count = 4*quad_count;
	const U32 i_count = 6*quad_count;

	drawcmd(px_tf((V2i) {x, y}, (V2i) {1, 1}),
			verts, v_count,
			inds, i_count,
			ogui_font()->atlas_uv,
			(Color) {1, 1, 1, 1},
			layer,
			0.0,
			NULL_PATTERN);
}

void calc_text_size(float ret[2], void *user_data, const char *text, int layer)
{
	V2i px_size = calc_text_mesh_size(ogui_font(), text);
	ret[0] = px_size.x;
	ret[1] = px_size.y;
}

void draw_window(void *user_data, float x, float y, float w, float h, float title_bar_height, const char *title, bool focus, int layer)
{
	Color bg_color = panel_color();

	{ // Content bg
		V2f p = {x, y + title_bar_height};
		V2f s = {w, h - title_bar_height};
		drawcmd_px_quad(v2f_to_v2i(p), v2f_to_v2i(s), bg_color, layer);
	}

	bg_color = darken_color(bg_color);

	{ // Title
		V2f p = {x, y};
		V2f s = {w, title_bar_height};

		drawcmd_px_quad(v2f_to_v2i(p), v2f_to_v2i(s), bg_color, layer);

		draw_text(NULL, p.x + 5, p.y + 2, title, layer + 1, NULL);
	}
}

void create_uicontext()
{
	GuiCallbacks cb = {};
	cb.draw_button = draw_button;
	cb.draw_checkbox = draw_checkbox;
	cb.draw_radiobutton = draw_radiobutton;
	cb.draw_textbox = draw_textbox;
	cb.draw_text = draw_text;
	cb.calc_text_size = calc_text_size;
	cb.draw_window = draw_window;

	UiContext *ctx = ZERO_ALLOC(gen_ator(), sizeof(*ctx), "uicontext");
	ctx->gui = create_gui(cb);
	g_env.uicontext = ctx;
}

void destroy_uicontext()
{
	destroy_gui(g_env.uicontext->gui);
	FREE(gen_ator(), g_env.uicontext);
	g_env.uicontext = NULL;
}

internal
U8 gui_key_state(int engine_code)
{
	Device *d = g_env.device;

	U8 state = 0;
	if (d->key_down[engine_code])
		state |= GUI_KEYSTATE_DOWN_BIT;
	if (d->key_pressed[engine_code])
		state |= GUI_KEYSTATE_PRESSED_BIT;
	if (d->key_released[engine_code])
		state |= GUI_KEYSTATE_RELEASED_BIT;
	return state;
}

void upd_uicontext()
{
	UiContext *ui = g_env.uicontext;

	{ // New gui
		GuiContext *ctx = ui->gui;
		ctx->host_win_size[0] = g_env.device->win_size.x;
		ctx->host_win_size[1] = g_env.device->win_size.y;
		ctx->cursor_pos[0] = g_env.device->cursor_pos.x;
		ctx->cursor_pos[1] = g_env.device->cursor_pos.y;

		ctx->key_state[GUI_KEY_LMB] = gui_key_state(KEY_LMB);

		for (U32 i = 0; i < g_env.device->written_text_size; ++i)
			gui_write_char(ctx, g_env.device->written_text_buf[i]);
	}

	ui->dev.prev_cursor_pos = ui->dev.cursor_pos;
	ui->dev.cursor_pos = g_env.device->cursor_pos;
	ui->dev.cursor_delta = sub_v2i(ui->dev.cursor_pos, ui->dev.prev_cursor_pos);

	ui->dev.lmb.pressed = g_env.device->key_pressed[KEY_LMB];
	ui->dev.lmb.down = g_env.device->key_down[KEY_LMB];
	ui->dev.lmb.released = g_env.device->key_released[KEY_LMB];

	ui->dev.rmb.pressed = g_env.device->key_pressed[KEY_RMB];
	ui->dev.rmb.down = g_env.device->key_down[KEY_RMB];
	ui->dev.rmb.released = g_env.device->key_released[KEY_RMB];

	ui->dev.shift_down = g_env.device->key_down[KEY_LSHIFT];
	ui->dev.snap_to_closest = g_env.device->key_down[KEY_LCTRL];
	ui->dev.g_pressed = g_env.device->key_pressed['g'];
	ui->dev.r_pressed = g_env.device->key_pressed['r'];
	ui->dev.s_pressed = g_env.device->key_pressed['s'];
	ui->dev.toggle_select_all = g_env.device->key_pressed['a'];
	ui->dev.toggle_play = g_env.device->key_pressed[KEY_SPACE];
	ui->dev.delete = g_env.device->key_pressed['x'];

	ensure(ui->turtle_i == 0);
	ui->turtles[0] = (UiContext_Turtle) {
		.dir = (V2i) {0, 1},
	};
	ui->combobox_released = false;

	ui->last_hot_id = ui->hot_id;
	ui->hot_id = 0;
}

void ogui_set_turtle_pos(V2i pos)
{
	UiContext *ctx = g_env.uicontext;
	ctx->turtles[ctx->turtle_i].pos = pos;
}

V2i ogui_turtle_pos()
{
	UiContext *ctx = g_env.uicontext;
	return ctx->turtles[ctx->turtle_i].pos;
}

void ogui_advance_turtle(V2i elem_size)
{
	UiContext *ctx = g_env.uicontext;
	UiContext_Turtle *turtle = &ctx->turtles[ctx->turtle_i];

	const V2i adv = mul_v2i(turtle->dir, elem_size);
	turtle->last_adv_size = elem_size;
	turtle->pos = add_v2i(turtle->pos, adv);
}

V2i ogui_last_adv_size()
{
	UiContext *ctx = g_env.uicontext;
	UiContext_Turtle *turtle = &ctx->turtles[ctx->turtle_i];
	return turtle->last_adv_size;
}

S32 ogui_next_draw_layer()
{
	UiContext *ctx = g_env.uicontext;
	UiContext_Turtle *turtle = &ctx->turtles[ctx->turtle_i];

	// Should return something > 0, as zero is the world layer
	return (turtle->draw_i++) + 1337;
}

void ogui_begin(V2i turtle_dir)
{
	UiContext *ctx = g_env.uicontext;
	ensure(ctx->turtle_i < MAX_GUI_STACK_SIZE);

	UiContext_Turtle *prev = &ctx->turtles[ctx->turtle_i];
	UiContext_Turtle *cur = &ctx->turtles[ctx->turtle_i + 1];

	++ctx->turtle_i;
	*cur = (UiContext_Turtle) {
		.pos = prev->pos,
		.dir = turtle_dir,
		.draw_i = prev->draw_i,
	};
}

void ogui_end()
{
	UiContext *ctx = g_env.uicontext;

	ensure(ctx->turtle_i > 0);
	--ctx->turtle_i;
}

internal
U32 hash32(const U8* buf, U32 size)
{
	// Modified FNV-1a
	U32 hash = 2166136261;
	for (U32 i = 0; i < size; ++i)
		hash = ((hash ^ buf[i]) + 379721)*16777619;
	return hash;
}

GuiId ogui_id(const char *label)
{ return hash32((U8*)label, strlen(label)); }

void ogui_set_hot(const char *label)
{
	UiContext *ctx = g_env.uicontext;
	if (ctx->active_id == 0) {
		//debug_print("set_hot %s", label);
		ctx->hot_id = ogui_id(label);
	}
}

bool ogui_is_hot(const char *label)
{
	UiContext *ctx = g_env.uicontext;
	return ctx->last_hot_id == ogui_id(label);
}

void ogui_set_active(const char *label)
{
	//debug_print("set_active %s", label);
	UiContext *ctx = g_env.uicontext;
	ctx->active_id = ogui_id(label);
}

void ogui_set_inactive(const char *label)
{
	//debug_print("set_inactive %s", label);
	UiContext *ctx = g_env.uicontext;
	if (ctx->active_id == ogui_id(label))
		ctx->active_id = 0;
}

bool ogui_is_active(const char *label)
{
	UiContext *ctx = g_env.uicontext;
	return ctx->active_id == ogui_id(label);
}
