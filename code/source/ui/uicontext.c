#include "core/device.h"
#include "global/env.h"
#include "uicontext.h"

#include "gen_layout.c" // load_layout

Color panel_color()
{ return (Color) {0.05, 0.05, 0.07, 0.95}; }

Color inactive_color()
{ return (Color) {0.2, 0.2, 0.2, 0.5}; }

Color darken_color(Color c)
{ return (Color) {c.r*0.8, c.g*0.8, c.b*0.8, c.a}; }

Color highlight_color(Color c)
{ return (Color) {c.r + 0.2, c.g + 0.2, c.b + 0.1, c.a}; }

Color outline_color(Color c)
{ return (Color) {c.r*0.2, c.g*0.2, c.b*0.2, c.a}; }

const Font *gui_font()
{
	return (Font*)res_by_name(	g_env.resblob,
								ResType_Font,
								"dev");
}

internal void limit_by_scissor(int pos[2], int size[2], int s_pos[2], int s_size[2])
{
	if (s_pos[0] >= pos[0] &&
		s_pos[1] >= pos[1] &&
		s_pos[0] + s_size[0] <= pos[0] + size[0] &&
		s_pos[1] + s_size[1] <= pos[1] + size[1])
		return;
	
	float p1[2] = {pos[0], pos[1]};
	float p2[2] = {pos[0] + size[0], pos[1] + size[1]};
	p1[0] = MAX(p1[0], s_pos[0]);
	p1[1] = MAX(p1[1], s_pos[1]);
	p2[0] = MIN(p2[0], s_pos[0] + s_size[0]);
	p2[1] = MIN(p2[1], s_pos[1] + s_size[1]);

	pos[0] = p1[0];
	pos[1] = p1[1];
	size[0] = MAX(p2[0] - p1[0], 0);
	size[1] = MAX(p2[1] - p1[1], 0);
}

internal void draw_text(int x, int y, const char *text, int layer)
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
			gui_font()->atlas_uv,
			white_color(), white_color(),
			layer,
			0.0,
			NULL_PATTERN);
}

// Callback for gui
void calc_text_size(int ret[2], void *user_data, const char *text)
{
	V2i px_size = calc_text_mesh_size(gui_font(), text);
	ret[0] = px_size.x;
	ret[1] = px_size.y;
}

void create_uicontext()
{
	UiContext *ctx = ZERO_ALLOC(gen_ator(), sizeof(*ctx), "uicontext");
	ctx->gui = create_gui(calc_text_size, NULL);
	load_layout(ctx->gui);
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

void begin_ui_frame()
{
	UiContext *ui = g_env.uicontext;

	{ // New gui
		GuiContext *ctx = ui->gui;
		ctx->host_win_size[0] = g_env.device->win_size.x;
		ctx->host_win_size[1] = g_env.device->win_size.y;
		ctx->cursor_pos[0] = g_env.device->cursor_pos.x;
		ctx->cursor_pos[1] = g_env.device->cursor_pos.y;

		ctx->key_state[GUI_KEY_LMB] = gui_key_state(KEY_LMB);
		ctx->key_state[GUI_KEY_MMB] = gui_key_state(KEY_MMB);
		ctx->key_state[GUI_KEY_RMB] = gui_key_state(KEY_RMB);
		ctx->key_state[GUI_KEY_LCTRL] = gui_key_state(KEY_LCTRL);
		F64 delta = g_env.device->mwheel_delta;
		if (delta > 0.0)
			ctx->mouse_scroll = 1;
		else if (delta < 0.0)
			ctx->mouse_scroll = -1;

		for (U32 i = 0; i < g_env.device->written_text_size; ++i)
			gui_write_char(ctx, g_env.device->written_text_buf[i]);

		gui_pre_frame(ctx);
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
}

void end_ui_frame()
{
	UiContext *ui = g_env.uicontext;
	GuiContext *ctx = ui->gui;
	
	gui_post_frame(ctx);

	GuiDrawInfo *draw_infos;
	int count;
	gui_draw_info(ctx, &draw_infos, &count);
	for (int i = 0; i < count; ++i) {
		GuiDrawInfo d = draw_infos[i];
		if (d.has_scissor && d.type != GuiDrawInfo_text)
			limit_by_scissor(d.pos, d.size, d.scissor_pos, d.scissor_size);
		V2i p = {d.pos[0], d.pos[1]};
		V2i s = {d.size[0], d.size[1]};

		switch (d.type) {
		case GuiDrawInfo_button:
		case GuiDrawInfo_panel:
		case GuiDrawInfo_resize_handle:
		case GuiDrawInfo_slider:
		case GuiDrawInfo_slider_handle:
		case GuiDrawInfo_textbox: {
			Color bg_color = panel_color();
			if (d.type != GuiDrawInfo_panel) {
				darken_color(bg_color);
				if (d.held)
					bg_color = darken_color(bg_color);
				else if (d.hovered || d.selected)
					bg_color = highlight_color(bg_color);
			}

			drawcmd_px_quad(p, s, 0.0, bg_color, outline_color(bg_color), d.layer);
		} break;

		case GuiDrawInfo_checkbox: {
			Color bg_color = darken_color(panel_color());
			if (d.selected)
				bg_color = (Color) {0.2, 1, 0.2, 1};

			if (d.held)
				bg_color = darken_color(bg_color);
			else if (d.hovered)
				bg_color = highlight_color(bg_color);

			drawcmd_px_quad(p, s, 0.0, bg_color, outline_color(bg_color), d.layer);
		} break;

		case GuiDrawInfo_radiobutton: {
			Color bg_color = darken_color(panel_color());
			if (d.selected) // Checked
				bg_color = (Color) {1, 0.2, 0.2, 1};

			if (d.held)
				bg_color = darken_color(bg_color);
			else if (d.hovered)
				bg_color = highlight_color(bg_color);

			p.x += 2;
			p.y += 1;
			s.x *= 0.8;
			s.y *= 0.8;
			drawcmd_px_quad(p, s, TAU/8, bg_color, outline_color(bg_color), d.layer);
		} break;

		case GuiDrawInfo_text: {
			draw_text(d.pos[0], d.pos[1], d.text, d.layer);
		} break;

		case GuiDrawInfo_title_bar: {
			Color bg_color = darken_color(panel_color());
			drawcmd_px_quad(p, s, 0.0, bg_color, outline_color(bg_color), d.layer);
			draw_text(p.x + 5, p.y + 2, d.text, d.layer + 1);
		} break;

		default: fail("Unknown GuiDrawInfo: %i", d.type);
		}
	}
}

bool world_has_input()
{
	return g_env.uicontext->gui->focused_win_ix < 0;
}

