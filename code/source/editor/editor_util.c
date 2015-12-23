#include "editor_util.h"
#include "visual/font.h"

internal
const Font *ogui_font()
{
	return (Font*)res_by_name(	g_env.resblob,
								ResType_Font,
								"dev");
}

void ogui_wrap(V2i *p, V2i *s)
{
	const V2i win_size = g_env.device->win_size;
	// Wrap around screen
	while (p->x < 0)
		p->x += win_size.x;
	while (p->x > win_size.x)
		p->x -= win_size.x;
	while (p->y < 0)
		p->y += win_size.y;
	while (p->y > win_size.y)
		p->y -= win_size.y;
}

Color ogui_dev_panel_color()
{ return (Color) {0.1, 0.1, 0.15, 0.9}; }

Color ogui_inactive_color()
{ return (Color) {0.2, 0.2, 0.2, 0.5}; }

Color ogui_darken_color(Color c)
{ return (Color) {c.r*0.6, c.g*0.6, c.b*0.6, c.a}; }

internal
Color ogui_highlight_color(Color c)
{ return (Color) {c.r + 0.2, c.g + 0.2, c.b + 0.1, c.a}; }

void ogui_text(const char *text)
{
	V2i px_pos = ogui_turtle_pos();
	ogui_wrap(&px_pos, NULL);
	ogui_begin((V2i) {1, 0});

	const U32 max_quad_count = strlen(text);
	const U32 max_vert_count = 4*max_quad_count;
	const U32 max_ind_count = 6*max_quad_count;
	TriMeshVertex *verts = frame_alloc(sizeof(*verts)*max_vert_count);
	MeshIndexType *inds = frame_alloc(sizeof(*inds)*max_ind_count);
	V2i size;
	U32 quad_count = text_mesh(&size, verts, inds, ogui_font(), text);
	const U32 v_count = 4*quad_count;
	const U32 i_count = 6*quad_count;

	drawcmd(px_tf(px_pos, (V2i) {1, 1}),
			verts, v_count,
			inds, i_count,
			ogui_font()->atlas_uv,
			(Color) {1, 1, 1, 1},
			ogui_next_draw_layer(),
			0.0,
			NULL_PATTERN);

	ogui_end();
	ogui_advance_turtle(size);
}

bool ogui_button(const char *label, bool *is_down, bool *is_hovered)
{
	V2i px_pos = ogui_turtle_pos();
	V2i px_size = calc_text_mesh_size(ogui_font(), label);
	px_size.x += 12;
	px_size.y += 5;

	ogui_wrap(&px_pos, &px_size);

	ogui_begin((V2i) {1, 0});
	UiContext *ctx = g_env.uicontext;
	const V2i c_p = ctx->dev.cursor_pos;

	bool pressed = false;
	bool down = false;
	bool hover = false;

	if (ogui_is_active(label)) {
		if (ctx->dev.lmb.down) {
			down = true;
		} else {
			pressed = true;
			ogui_set_inactive(label);
		}
	} else if (ogui_is_hot(label)) {
		if (ctx->dev.lmb.pressed) {
			down = true;
			ogui_set_active(label);
		}
	}

	if (	c_p.x >= px_pos.x &&
			c_p.y >= px_pos.y &&
			c_p.x < px_pos.x + px_size.x &&
			c_p.y < px_pos.y + px_size.y) {
		hover = true;
		ogui_set_hot(label);
	}

	Color bg_color = ogui_darken_color(ogui_dev_panel_color());
	if (down)
		bg_color = ogui_darken_color(bg_color);
	else if (hover)
		bg_color = ogui_highlight_color(bg_color);

	{ // Leave margin
		V2i p = add_v2i(px_pos, (V2i) {1, 1});
		V2i s = sub_v2i(px_size, (V2i) {2, 2});
		drawcmd_px_quad(p, s, bg_color, ogui_next_draw_layer());
	}

	ogui_set_turtle_pos(add_v2i(px_pos, (V2i) {5, 1}));
	ogui_text(frame_str("%s", label));

	ogui_end();

	ogui_advance_turtle(px_size);

	if (is_down)
		*is_down = down;
	if (is_hovered)
		*is_hovered = hover;
	return pressed;
}

bool ogui_begin_combobox(const char *label)
{
	UiContext *ctx = g_env.uicontext;

	bool btn_down;
	V2i combobox_pos = ogui_turtle_pos();
	ctx->combobox_released =
		ogui_button(	label, &btn_down, NULL);
	V2i list_start_pos = {
		combobox_pos.x, combobox_pos.y - ogui_last_adv_size().y
	};
	const bool open = btn_down || ctx->combobox_released;

	if (open) {
		ogui_begin((V2i) {0, -1}); // User calls ogui_end()
		ogui_set_turtle_pos(list_start_pos);
	}

	return open;
}

bool ogui_combobox_item(const char *label)
{
	UiContext *ctx = g_env.uicontext;

	bool hovered;
	ogui_button(label, NULL, &hovered);

	return ctx->combobox_released && hovered;
}

void ogui_end_combobox()
{
	ogui_end();
}

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
	if (ctx->dev.grabbing == ogui_id(label))
		return CursorDeltaMode_translate;
	if (ctx->dev.rotating == ogui_id(label))
		return CursorDeltaMode_rotate;
	if (ctx->dev.scaling == ogui_id(label))
		return CursorDeltaMode_scale;
	return CursorDeltaMode_none;
}

CursorDeltaMode cursor_transform_delta_world(	T3f *out,
												const char *label,
												T3d coords)
{
	UiContext *ctx = g_env.uicontext;
	*out = identity_t3f();

	if (ctx->dev.grabbing == ogui_id(label)) {
		out->pos = cursor_delta_in_tf_coords(coords);
		return CursorDeltaMode_translate;
	}

	if (ctx->dev.rotating == ogui_id(label)) {
		out->rot = cursor_rot_delta_in_tf_coords(coords);
		return CursorDeltaMode_rotate;
	}

	if (ctx->dev.scaling == ogui_id(label)) {
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

	if (ctx->dev.grabbing == ogui_id(label)) {
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

	if (ctx->dev.rotating == ogui_id(label)) {
		V3f v1 = v3d_to_v3f(sub_v3d(prev_p, center));
		V3f v2 = v3d_to_v3f(sub_v3d(cur_p, center));
		out->rot = qf_by_from_to(v1, v2);
		return CursorDeltaMode_rotate;
	}

	if (ctx->dev.scaling == ogui_id(label)) {
		V3f w1 = v3d_to_v3f(sub_v3d(prev_p, center));
		V3f w2 = v3d_to_v3f(sub_v3d(cur_p, center));

		F32 s = length_v3f(w2)/length_v3f(w1);
		out->scale = (V3f) {s, s, s};
		return CursorDeltaMode_scale;
	}

	return CursorDeltaMode_translate;
}

void ogui_res_info(ResType t, const Resource *res)
{
	ogui_set_turtle_pos((V2i) {0, 0});
	char *str = frame_str(	"%s: %s",
							restype_to_str(t),
							res ? res->name : "<none>");
	V2i size = calc_text_mesh_size(ogui_font(), str);
	size.y += 3;
	drawcmd_px_quad((V2i) {0, 0}, size, ogui_dev_panel_color(), ogui_next_draw_layer());
	ogui_text(str);
}

EditorBoxState ogui_editorbox(	const char *label,
								V2i px_pos,
								V2i px_size,
								bool invisible)
{
	ogui_wrap(&px_pos, &px_size);
	UiContext *ctx = g_env.uicontext;
	const V2i c_p = ctx->dev.cursor_pos;
	const Color c = ogui_dev_panel_color();

	EditorBoxState state = {};

	if (ogui_is_active(label)) {
		state.pressed = false;
		if (	!ctx->dev.lmb.down &&
				!ctx->dev.rmb.down &&
				!ctx->dev.grabbing && !ctx->dev.rotating && !ctx->dev.scaling) {
			state.released = true;
			ogui_set_inactive(label);
		} else if (	ctx->dev.lmb.down &&
					!ctx->dev.grabbing && !ctx->dev.rotating && !ctx->dev.scaling) {
			// ldown == true if nothing else is going on
			state.ldown = true;
		} else if (ctx->dev.rmb.down) {
			state.down = true;
		}

		if (	ctx->dev.rmb.pressed &&
				(ctx->dev.grabbing || ctx->dev.rotating || ctx->dev.scaling)) {
			// Cancel
			editor_revert_res_state();
			ctx->dev.grabbing = 0;
			ctx->dev.rotating = 0;
			ctx->dev.scaling = 0;
			ogui_set_inactive(label);
		}

		if (ctx->dev.lmb.pressed) {
			ctx->dev.grabbing = 0;
			ctx->dev.rotating = 0;
			ctx->dev.scaling = 0;
			ogui_set_inactive(label);
		}
	} else if (ogui_is_hot(label)) {
		if (ctx->dev.lmb.pressed) {
			state.ldown = true;
			ogui_set_active(label);
		} else if (ctx->dev.rmb.pressed) {
			state.pressed = true;
			state.down = true;
			ogui_set_active(label);
		} else if (ctx->dev.g_pressed) {
			ctx->dev.grabbing = ogui_id(label);
			ogui_set_active(label);
		} else if (ctx->dev.r_pressed) {
			ctx->dev.rotating = ogui_id(label);
			ogui_set_active(label);
		} else if (ctx->dev.s_pressed) {
			ctx->dev.scaling = ogui_id(label);
			ogui_set_active(label);
		}

		if (ogui_is_active(label))
			editor_store_res_state();
	}

	if (	c_p.x >= px_pos.x &&
			c_p.y >= px_pos.y &&
			c_p.x < px_pos.x + px_size.x &&
			c_p.y < px_pos.y + px_size.y) {
		state.hover = true;
		ogui_set_hot(label);
	}

	if (!invisible)
		drawcmd_px_quad(px_pos, px_size, c, ogui_next_draw_layer());

	return state;
}
