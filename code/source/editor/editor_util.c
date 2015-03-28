#include "editor_util.h"

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

Color inactive_color()
{
	return (Color) {0.5, 0.5, 0.5, 0.5};
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

bool cursor_transform_delta_world(T3f *out, const char *label, T3d coords)
{
	UiContext *ctx= g_env.uicontext;
	*out= identity_t3f();

	if (ctx->dev.grabbing == gui_id(label)) {
		out->pos= cursor_delta_in_tf_coords(coords);
		return true;
	}

	if (ctx->dev.rotating == gui_id(label)) {
		out->rot= cursor_rot_delta_in_tf_coords(coords);
		return true;
	}

	if (ctx->dev.scaling == gui_id(label)) {
		out->scale= cursor_scale_delta_in_tf_coords(coords);
		return true;
	}

	return false;
}

bool cursor_transform_delta_pixels(	T3f *out,
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
		return true;
	} 

	if (ctx->dev.rotating == gui_id(label)) {
		V3f v1= v3d_to_v3f(sub_v3d(prev_p, center));
		V3f v2= v3d_to_v3f(sub_v3d(cur_p, center));
		out->rot= qf_by_from_to(v1, v2);
		return true;
	}

	if (ctx->dev.scaling == gui_id(label)) {
		V3f w1= v3d_to_v3f(sub_v3d(prev_p, center));
		V3f w2= v3d_to_v3f(sub_v3d(cur_p, center));

		F32 s= length_v3f(w2)/length_v3f(w1);
		out->scale= (V3f) {s, s, s};
		return true;
	}

	return false;
}

void gui_quad(V2i pix_pos, V2i pix_size, Color c)
{
	V3d pos= v2d_to_v3d(screen_to_world_point(pix_pos)); 
	V3d size= v2d_to_v3d(screen_to_world_size(pix_size));

	ModelEntity init;
	init_modelentity(&init);
	init.tf.pos= pos;
	init.tf.scale= size;
	init.free_after_draw= true;
	fmt_str(init.model_name, sizeof(init.model_name), "guibox_singular");

	U32 handle= resurrect_modelentity(&init);
	ModelEntity *e= get_modelentity(handle);
	e->color= c;
}

void gui_model_image(V2i pix_pos, V2i pix_size, ModelEntity *src_model)
{
	ensure(src_model);

	V3d pos= v2d_to_v3d(screen_to_world_point(pix_pos)); 
	V3d size= v2d_to_v3d(screen_to_world_size(pix_size));

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
}

EditorBoxState gui_editorbox(	const char *label,
								V2i pix_pos,
								V2i pix_size,
								bool invisible)
{
	gui_wrap(&pix_pos, &pix_size);
	UiContext *ctx= g_env.uicontext;
	const V2i c_p= ctx->dev.cursor_pos;
	const Color c= (Color) {0.25, 0.25, 0.3, 0.9};

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

	if (	c_p.x >= pix_pos.x &&
			c_p.y >= pix_pos.y &&
			c_p.x < pix_pos.x + pix_size.x &&
			c_p.y < pix_pos.y + pix_size.y) {
		state.hover= true;
		gui_set_hot(label);
	}

	if (!invisible)
		gui_quad(pix_pos, pix_size, c);

	return state;
}
