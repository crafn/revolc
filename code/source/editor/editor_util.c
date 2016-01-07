#include "editor_util.h"
#include "visual/font.h"
#include "ui/gui.h"

F64 editor_vertex_size()
{ return screen_to_world_size((V2i) {5, 0}).x; }

internal
V3f cursor_delta_in_tf_coords(T3d tf)
{
	UiContext *ctx = g_env.uicontext;
	V3d cur_wp = v2d_to_v3d(screen_to_world_point(ctx->dev.cursor_pos));
	V3d prev_wp = v2d_to_v3d(screen_to_world_point(ctx->dev.prev_cursor_pos));
	V3d cur = transform_v3d(inv_t3d(tf), cur_wp);
	V3d prev = transform_v3d(inv_t3d(tf), prev_wp);
	return v3d_to_v3f(sub_v3d(cur, prev));
}

internal
Qf cursor_rot_delta_in_tf_coords(T3d tf)
{
	V3d center = tf.pos;
	/// @todo Correct return with 3d rot

	UiContext *ctx = g_env.uicontext;
	V3d cur_wp = v2d_to_v3d(screen_to_world_point(ctx->dev.cursor_pos));
	V3d prev_wp = v2d_to_v3d(screen_to_world_point(ctx->dev.prev_cursor_pos));
	V3d a1 = sub_v3d(prev_wp, center);
	V3d a2 = sub_v3d(cur_wp, center);
	V3d b1 = rot_v3d(tf.rot, sub_v3d(prev_wp, center));
	V3d b2 = rot_v3d(tf.rot, sub_v3d(cur_wp, center));
	Qf rot = qf_by_from_to(v3d_to_v3f(a1), v3d_to_v3f(a2));

	if ((cross_v3d(a1, a2).z > 0) != (cross_v3d(b1, b2).z > 0))
		rot = neg_qf(rot); // Mirror
	return rot;
}

internal
V3f cursor_scale_delta_in_tf_coords(T3d tf)
{
	V3d center = tf.pos;
	UiContext *ctx = g_env.uicontext;
	V3d cur_wp = v2d_to_v3d(screen_to_world_point(ctx->dev.cursor_pos));
	V3d prev_wp = v2d_to_v3d(screen_to_world_point(ctx->dev.prev_cursor_pos));
	V3f v1 = v3d_to_v3f(sub_v3d(prev_wp, center));
	V3f v2 = v3d_to_v3f(sub_v3d(cur_wp, center));

	F32 s = length_v3f(v2)/length_v3f(v1);
	return (V3f) {s, s, s};
}

CursorDeltaMode cursor_delta_mode(const char *label)
{
	UiContext *ctx = g_env.uicontext;
	if (ctx->dev.grabbing == gui_id(label))
		return CursorDeltaMode_translate;
	if (ctx->dev.rotating == gui_id(label))
		return CursorDeltaMode_rotate;
	if (ctx->dev.scaling == gui_id(label))
		return CursorDeltaMode_scale;
	return CursorDeltaMode_none;
}

CursorDeltaMode cursor_transform_delta_world(	T3f *out,
												const char *label,
												T3d coords)
{
	UiContext *ctx = g_env.uicontext;
	*out = identity_t3f();

	if (ctx->dev.grabbing == gui_id(label)) {
		out->pos = cursor_delta_in_tf_coords(coords);
		return CursorDeltaMode_translate;
	}

	if (ctx->dev.rotating == gui_id(label)) {
		out->rot = cursor_rot_delta_in_tf_coords(coords);
		return CursorDeltaMode_rotate;
	}

	if (ctx->dev.scaling == gui_id(label)) {
		out->scale = cursor_scale_delta_in_tf_coords(coords);
		return CursorDeltaMode_scale;
	}

	return CursorDeltaMode_none;
}

CursorDeltaMode cursor_transform_delta_pixels(	T3f *out,
												const char *label,
												T3d coords)
{
	UiContext *ctx = g_env.uicontext;
	*out = identity_t3f();

	V3d cur_p = {ctx->dev.cursor_pos.x, ctx->dev.cursor_pos.y, 0};
	V3d prev_p = {ctx->dev.prev_cursor_pos.x, ctx->dev.prev_cursor_pos.y, 0};
	V3d center = coords.pos;

	if (ctx->dev.grabbing == gui_id(label)) {
		V3d cur = mul_t3d(	inv_t3d(coords),
							(T3d) {	{1, 1, 1},
									identity_qd(),
									cur_p}).pos;
		V3d prev = mul_t3d(	inv_t3d(coords),
							(T3d) {	{1, 1, 1},
									identity_qd(),
									prev_p}).pos;
		out->pos = v3d_to_v3f(sub_v3d(cur, prev));
		return CursorDeltaMode_translate;
	} 

	if (ctx->dev.rotating == gui_id(label)) {
		V3f v1 = v3d_to_v3f(sub_v3d(prev_p, center));
		V3f v2 = v3d_to_v3f(sub_v3d(cur_p, center));
		out->rot = qf_by_from_to(v1, v2);
		return CursorDeltaMode_rotate;
	}

	if (ctx->dev.scaling == gui_id(label)) {
		V3f w1 = v3d_to_v3f(sub_v3d(prev_p, center));
		V3f w2 = v3d_to_v3f(sub_v3d(cur_p, center));

		F32 s = length_v3f(w2)/length_v3f(w1);
		out->scale = (V3f) {s, s, s};
		return CursorDeltaMode_scale;
	}

	return CursorDeltaMode_none;
}

void gui_res_info(ResType t, const Resource *res)
{
	GuiContext *ctx = g_env.uicontext->gui;

	gui_begin_panel(ctx, "res_info");
	const char *str = gui_str(	ctx, "%s: %s",
								restype_to_str(t),
								res ? res->name : "<none>");
	gui_label(ctx, str);
	gui_end_panel(ctx);
}

EditorBoxState gui_editorbox(	GuiContext *ctx,
								V2i *p, V2i *s,
								const char *label,
								bool invisible)
{
	UiContext *ui = g_env.uicontext;
	const V2i c_p = ui->dev.cursor_pos;
	const Color c = panel_color();

	gui_begin(ctx, label);

	V2i px_pos, px_size;
	gui_turtle_pos(ctx, &px_pos.x, &px_pos.y);
	gui_turtle_size(ctx, &px_size.x, &px_size.y);

	if (p)
		*p = px_pos;
	if (s)
		*s = px_size;

	EditorBoxState state = {};

	if (gui_is_active(ctx, label)) {
		state.pressed = false;
		if (	!ui->dev.lmb.down &&
				!ui->dev.rmb.down &&
				!ui->dev.grabbing && !ui->dev.rotating && !ui->dev.scaling) {
			state.released = true;
			gui_set_inactive(ctx, gui_id(label));
		} else if (	ui->dev.lmb.down &&
					!ui->dev.grabbing && !ui->dev.rotating && !ui->dev.scaling) {
			// ldown == true if nothing else is going on
			state.ldown = true;
		} else if (ui->dev.rmb.down) {
			state.down = true;
		}

		if (	ui->dev.rmb.pressed &&
				(ui->dev.grabbing || ui->dev.rotating || ui->dev.scaling)) {
			// Cancel
			editor_revert_res_state();
			ui->dev.grabbing = 0;
			ui->dev.rotating = 0;
			ui->dev.scaling = 0;
			gui_set_inactive(ctx, gui_id(label));
		}

		if (ui->dev.lmb.pressed) {
			ui->dev.grabbing = 0;
			ui->dev.rotating = 0;
			ui->dev.scaling = 0;
			gui_set_inactive(ctx, gui_id(label));
		}
	} else if (gui_is_hot(ctx, label)) {
		if (ui->dev.lmb.pressed) {
			state.ldown = true;
			gui_set_active(ctx, label);
		} else if (ui->dev.rmb.pressed) {
			state.pressed = true;
			state.down = true;
			gui_set_active(ctx, label);
		} else if (ui->dev.g_pressed) {
			ui->dev.grabbing = gui_id(label);
			gui_set_active(ctx, label);
		} else if (ui->dev.r_pressed) {
			ui->dev.rotating = gui_id(label);
			gui_set_active(ctx, label);
		} else if (ui->dev.s_pressed) {
			ui->dev.scaling = gui_id(label);
			gui_set_active(ctx, label);
		}

		if (gui_is_active(ctx, label))
			editor_store_res_state();
	}

	if (	c_p.x >= px_pos.x &&
			c_p.y >= px_pos.y &&
			c_p.x < px_pos.x + px_size.x &&
			c_p.y < px_pos.y + px_size.y) {
		state.hover = true;
		gui_set_hot(ctx, label);
	}

	if (!invisible)
		drawcmd_px_quad(px_pos, px_size, 0.0, c, outline_color(c), gui_layer(ctx));

	gui_end(ctx);

	return state;
}
