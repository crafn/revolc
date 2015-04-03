#include "editor_util.h"
#include "visual/font.h"

internal
const Font *gui_font()
{
	return (Font*)res_by_name(	g_env.resblob,
								ResType_Font,
								"dev");
}

char * frame_str(const char *fmt, ...)
{
	char *text= NULL;
	va_list args;
	va_list args_copy;

	va_start(args, fmt);
	va_copy(args_copy, args);
	U32 size= v_fmt_str(NULL, 0, fmt, args) + 1; 
	text= frame_alloc(size);
	v_fmt_str(text, size, fmt, args_copy);
	va_end(args_copy);
	va_end(args);
	return text;
}

void gui_wrap(V2i *p, V2i *s)
{
	const V2i win_size= g_env.device->win_size;
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

Color gui_dev_panel_color()
{ return (Color) {0.25, 0.25, 0.3, 0.9}; }

Color inactive_color()
{ return (Color) {0.5, 0.5, 0.5, 0.5}; }

Color darken_color(Color c)
{ return (Color) {c.r*0.6, c.g*0.6, c.b*0.6, c.a}; }

internal
Color highlight_color(Color c)
{ return (Color) {c.r + 0.2, c.g + 0.2, c.b + 0.1, c.a}; }

void gui_text(const char *text)
{
	V2i px_pos= gui_turtle_pos();
	gui_wrap(&px_pos, NULL);
	gui_begin((V2i) {1, 0});

	const U32 max_quad_count= strlen(text);
	const U32 max_vert_count= 4*max_quad_count;
	const U32 max_ind_count= 6*max_quad_count;
	TriMeshVertex *verts= frame_alloc(sizeof(*verts)*max_vert_count);
	MeshIndexType *inds= frame_alloc(sizeof(*inds)*max_ind_count);
	V2i size;
	U32 quad_count= text_mesh(&size, verts, inds, gui_font(), text);
	const U32 v_count= 4*quad_count;
	const U32 i_count= 6*quad_count;

	push_model(	px_tf(px_pos, (V2i) {1, 1}),
				verts, v_count,
				inds, i_count,
				(Color) {1, 1, 1, 1},
				gui_font()->atlas_uv,
				gui_next_draw_layer());


	gui_end();
	gui_advance_turtle(size);
}

bool gui_button(const char *label, bool *is_down, bool *is_hovered)
{
	V2i px_pos= gui_turtle_pos();
	V2i px_size= calc_text_mesh_size(gui_font(), label);
	px_size.x += 12;
	px_size.y += 5;

	gui_wrap(&px_pos, &px_size);

	gui_begin((V2i) {1, 0});
	UiContext *ctx= g_env.uicontext;
	const V2i c_p= ctx->dev.cursor_pos;

	bool pressed= false;
	bool down= false;
	bool hover= false;

	if (gui_is_active(label)) {
		if (ctx->dev.lmb.down) {
			down= true;
		} else {
			pressed= true;
			gui_set_inactive(label);
		}
	} else if (gui_is_hot(label)) {
		if (ctx->dev.lmb.pressed) {
			down= true;
			gui_set_active(label);
		}
	}

	if (	c_p.x >= px_pos.x &&
			c_p.y >= px_pos.y &&
			c_p.x < px_pos.x + px_size.x &&
			c_p.y < px_pos.y + px_size.y) {
		hover= true;
		gui_set_hot(label);
	}

	Color bg_color= darken_color(gui_dev_panel_color());
	if (down)
		bg_color= darken_color(bg_color);
	else if (hover)
		bg_color= highlight_color(bg_color);

	{ // Leave margin
		V2i p= add_v2i(px_pos, (V2i) {1, 1});
		V2i s= sub_v2i(px_size, (V2i) {2, 2});
		gui_quad(p, s, bg_color);
	}

	gui_set_turtle_pos(add_v2i(px_pos, (V2i) {5, 1}));
	gui_text(frame_str("%s", label));

	gui_end();

	gui_advance_turtle(px_size);

	if (is_down)
		*is_down= down;
	if (is_hovered)
		*is_hovered= hover;
	return pressed;
}

bool gui_begin_listbox(const char *label)
{
	UiContext *ctx= g_env.uicontext;

	bool btn_down;
	V2i listbox_pos= gui_turtle_pos();
	ctx->listbox_released=
		gui_button(	label, &btn_down, NULL);
	V2i list_start_pos= {
		listbox_pos.x, listbox_pos.y - gui_last_adv_size().y
	};
	const bool open= btn_down || ctx->listbox_released;

	if (open) {
		gui_begin((V2i) {0, -1}); // User calls gui_end()
		gui_set_turtle_pos(list_start_pos);
	}

	return open;
}

bool gui_listbox_item(const char *label)
{
	UiContext *ctx= g_env.uicontext;

	bool hovered;
	gui_button(label, NULL, &hovered);

	return ctx->listbox_released && hovered;
}

F64 editor_vertex_size()
{ return screen_to_world_size((V2i) {5, 0}).x; }

internal
V3f cursor_delta_in_tf_coords(T3d tf)
{
	UiContext *ctx= g_env.uicontext;
	V3d cur_wp= v2d_to_v3d(screen_to_world_point(ctx->dev.cursor_pos));
	V3d prev_wp= v2d_to_v3d(screen_to_world_point(ctx->dev.prev_cursor_pos));
	V3d cur= mul_t3d(	inv_t3d(tf),
						(T3d) {	{1, 1, 1},
								identity_qd(),
								cur_wp}).pos;
	V3d prev= mul_t3d(	inv_t3d(tf),
						(T3d) {	{1, 1, 1},
								identity_qd(),
								prev_wp}).pos;
	return v3d_to_v3f(sub_v3d(cur, prev));
}

internal
Qf cursor_rot_delta_in_tf_coords(T3d tf)
{
	V3d center= tf.pos;
	/// @todo Correct return with 3d rot

	UiContext *ctx= g_env.uicontext;
	V3d cur_wp= v2d_to_v3d(screen_to_world_point(ctx->dev.cursor_pos));
	V3d prev_wp= v2d_to_v3d(screen_to_world_point(ctx->dev.prev_cursor_pos));
	V3f v1= v3d_to_v3f(sub_v3d(prev_wp, center));
	V3f v2= v3d_to_v3f(sub_v3d(cur_wp, center));
	return qf_by_from_to(v1, v2);
}

internal
V3f cursor_scale_delta_in_tf_coords(T3d tf)
{
	V3d center= tf.pos;
	UiContext *ctx= g_env.uicontext;
	V3d cur_wp= v2d_to_v3d(screen_to_world_point(ctx->dev.cursor_pos));
	V3d prev_wp= v2d_to_v3d(screen_to_world_point(ctx->dev.prev_cursor_pos));
	V3f v1= v3d_to_v3f(sub_v3d(prev_wp, center));
	V3f v2= v3d_to_v3f(sub_v3d(cur_wp, center));

	F32 s= length_v3f(v2)/length_v3f(v1);
	return (V3f) {s, s, s};
}

CursorDeltaMode cursor_delta_mode(const char *label)
{
	UiContext *ctx= g_env.uicontext;
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
	UiContext *ctx= g_env.uicontext;
	*out= identity_t3f();

	if (ctx->dev.grabbing == gui_id(label)) {
		out->pos= cursor_delta_in_tf_coords(coords);
		return CursorDeltaMode_translate;
	}

	if (ctx->dev.rotating == gui_id(label)) {
		out->rot= cursor_rot_delta_in_tf_coords(coords);
		return CursorDeltaMode_rotate;
	}

	if (ctx->dev.scaling == gui_id(label)) {
		out->scale= cursor_scale_delta_in_tf_coords(coords);
		return CursorDeltaMode_scale;
	}

	return CursorDeltaMode_none;
}

CursorDeltaMode cursor_transform_delta_pixels(	T3f *out,
												const char *label,
												T3d coords)
{
	UiContext *ctx= g_env.uicontext;
	*out= identity_t3f();

	V3d cur_p= {ctx->dev.cursor_pos.x, ctx->dev.cursor_pos.y, 0};
	V3d prev_p= {ctx->dev.prev_cursor_pos.x, ctx->dev.prev_cursor_pos.y, 0};
	V3d center= coords.pos;

	if (ctx->dev.grabbing == gui_id(label)) {
		V3d cur= mul_t3d(	inv_t3d(coords),
							(T3d) {	{1, 1, 1},
									identity_qd(),
									cur_p}).pos;
		V3d prev= mul_t3d(	inv_t3d(coords),
							(T3d) {	{1, 1, 1},
									identity_qd(),
									prev_p}).pos;
		out->pos= v3d_to_v3f(sub_v3d(cur, prev));
		return CursorDeltaMode_translate;
	} 

	if (ctx->dev.rotating == gui_id(label)) {
		V3f v1= v3d_to_v3f(sub_v3d(prev_p, center));
		V3f v2= v3d_to_v3f(sub_v3d(cur_p, center));
		out->rot= qf_by_from_to(v1, v2);
		return CursorDeltaMode_rotate;
	}

	if (ctx->dev.scaling == gui_id(label)) {
		V3f w1= v3d_to_v3f(sub_v3d(prev_p, center));
		V3f w2= v3d_to_v3f(sub_v3d(cur_p, center));

		F32 s= length_v3f(w2)/length_v3f(w1);
		out->scale= (V3f) {s, s, s};
		return CursorDeltaMode_scale;
	}

	return CursorDeltaMode_translate;
}

void gui_quad(V2i px_pos, V2i px_size, Color c)
{
	gui_wrap(&px_pos, &px_size);

	ModelEntity init;
	init_modelentity(&init);
	init.tf= px_tf(px_pos, px_size);
	init.free_after_draw= true;
	fmt_str(init.model_name, sizeof(init.model_name), "guibox_singular");

	U32 handle= resurrect_modelentity(&init);
	ModelEntity *e= get_modelentity(handle);
	e->color= c;
	e->layer= gui_next_draw_layer();
}

void gui_model_image(V2i px_pos, V2i px_size, ModelEntity *src_model)
{
	ensure(src_model);

	V3d pos= v2d_to_v3d(screen_to_world_point(px_pos)); 
	V3d size= v2d_to_v3d(screen_to_world_size(px_size));

	ModelEntity init;
	init_modelentity(&init);
	init.tf.pos= pos;
	init.tf.scale= size;
	init.free_after_draw= true;
	fmt_str(init.model_name, sizeof(init.model_name), "guibox");

	U32 handle= resurrect_modelentity(&init);
	ModelEntity *e= get_modelentity(handle);
	e->atlas_uv= src_model->atlas_uv;
	e->scale_to_atlas_uv= src_model->scale_to_atlas_uv;
	e->layer= gui_next_draw_layer();
}

void gui_res_info(ResType t, const Resource *res)
{
	gui_set_turtle_pos((V2i) {0, 0});
	char *str= frame_str(	"%s: %s",
							restype_to_str(t),
							res ? res->name : "<none>");
	V2i size= calc_text_mesh_size(gui_font(), str);
	size.y += 3;
	gui_quad((V2i) {0, 0}, size, gui_dev_panel_color());
	gui_text(str);
}

EditorBoxState gui_editorbox(	const char *label,
								V2i px_pos,
								V2i px_size,
								bool invisible)
{
	gui_wrap(&px_pos, &px_size);
	UiContext *ctx= g_env.uicontext;
	const V2i c_p= ctx->dev.cursor_pos;
	const Color c= gui_dev_panel_color();

	EditorBoxState state= {};

	if (gui_is_active(label)) {
		state.pressed= false;
		if (	!ctx->dev.rmb.down &&
				!ctx->dev.grabbing && !ctx->dev.rotating && !ctx->dev.scaling) {
			state.released= true;
			gui_set_inactive(label);
		} else if (ctx->dev.rmb.down) {
			state.down= true;
		}

		if (	ctx->dev.rmb.pressed &&
				(ctx->dev.grabbing || ctx->dev.rotating || ctx->dev.scaling)) {
			// Cancel
			editor_revert_res_state();
			ctx->dev.grabbing= 0;
			ctx->dev.rotating= 0;
			ctx->dev.scaling= 0;
			gui_set_inactive(label);
		}

		if (ctx->dev.lmb.pressed) {
			ctx->dev.grabbing= 0;
			ctx->dev.rotating= 0;
			ctx->dev.scaling= 0;
			gui_set_inactive(label);
		}
	} else if (gui_is_hot(label)) {
		if (ctx->dev.rmb.pressed) {
			state.pressed= true;
			state.down= true;
			gui_set_active(label);
		} else if (ctx->dev.g_pressed) {
			ctx->dev.grabbing= gui_id(label);
			gui_set_active(label);
		} else if (ctx->dev.r_pressed) {
			ctx->dev.rotating= gui_id(label);
			gui_set_active(label);
		} else if (ctx->dev.s_pressed) {
			ctx->dev.scaling= gui_id(label);
			gui_set_active(label);
		}

		if (gui_is_active(label))
			editor_store_res_state();
	}

	if (	c_p.x >= px_pos.x &&
			c_p.y >= px_pos.y &&
			c_p.x < px_pos.x + px_size.x &&
			c_p.y < px_pos.y + px_size.y) {
		state.hover= true;
		gui_set_hot(label);
	}

	if (!invisible)
		gui_quad(px_pos, px_size, c);

	return state;
}
