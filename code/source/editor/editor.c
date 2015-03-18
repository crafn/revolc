#include "core/malloc.h"
#include "editor.h"
#include "global/env.h"
#include "platform/device.h"
#include "ui/uicontext.h"
#include "visual/renderer.h"

typedef struct GuiBoxState {
	bool hover;
	bool down;
	bool pressed;
	bool released;
} GuiBoxState;

internal
void toggle_bool(bool *b)
{ *b = !*b; }

internal
V3d vertex_world_pos(ModelEntity *m, U32 i)
{
	TriMeshVertex *v= &m->vertices[i];
	T3d v_t= identity_t3d();
	v_t.pos= v3f_to_v3d(v->pos);

	T3d t= mul_t3d(m->tf, v_t);
	return t.pos;
}

internal
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

internal
GuiBoxState gui_editorbox(const char *label, V2i pix_pos, V2i pix_size)
{
	gui_wrap(&pix_pos, &pix_size);
	UiContext *ctx= g_env.uicontext;
	const V2i c_p= ctx->cursor_pos;
	const Color c= (Color) {0.25, 0.25, 0.3, 0.9};

	GuiBoxState state= {};

	if (gui_is_active(label)) {
		state.pressed= false;
		if (!ctx->rmb.down && !ctx->grabbing) {
			state.released= true;
			gui_set_inactive(label);
		} else if (ctx->rmb.down) {
			state.down= true;
		}

		if (ctx->lmb.pressed || ctx->g_pressed) {
			ctx->grabbing= false;
			gui_set_inactive(label);
		}
	} else if (gui_is_hot(label)) {
		if (ctx->rmb.pressed) {
			state.pressed= true;
			state.down= true;
			gui_set_active(label);
		} else if (ctx->g_pressed) {
			ctx->grabbing= true;
			gui_set_active(label);
		}
	}

	if (	c_p.x >= pix_pos.x &&
			c_p.y >= pix_pos.y &&
			c_p.x < pix_pos.x + pix_size.x &&
			c_p.y < pix_pos.y + pix_size.y) {
		state.hover= true;
		gui_set_hot(label);
	}

	const V3d pos= v2d_to_v3d(screen_to_world_point(pix_pos)); 
	const V3d size= v2d_to_v3d(screen_to_world_size(pix_size));

	V2d poly[4]= {
		{pos.x, pos.y},
		{pos.x + size.x, pos.y},
		{pos.x + size.x, pos.y + size.y},
		{pos.x, pos.y + size.y},
	};
	ddraw_poly(c, poly, 4);

	return state;
}

internal
V2i uv_to_pix(V3f uv, V2i pix_pos, V2i pix_size)
{ return (V2i) {uv.x*pix_size.x + pix_pos.x, uv.y*pix_size.y + pix_pos.y}; }

internal
V3d pix_to_uv(V2i p, V2i pix_pos, V2i pix_size)
{ return (V3d) {(F64)(p.x - pix_pos.x)/pix_size.x,
				(F64)(p.y - pix_pos.y)/pix_size.y,
				0.0}; }

internal
void draw_vert(V2i pix_pos, bool selected)
{
	gui_wrap(&pix_pos, NULL);
	V2d p= screen_to_world_point(pix_pos);
	const F64 v_size= 0.03;
	V2d poly[4]= {
		{-v_size + p.x, -v_size + p.y},
		{-v_size + p.x, +v_size + p.y},
		{+v_size + p.x, +v_size + p.y},
		{+v_size + p.x, -v_size + p.y},
	};
	if (selected)
		ddraw_poly((Color) {1.0, 0.7, 0.2, 0.8}, poly, 4);
	else
		ddraw_poly((Color) {0.0, 0.0, 0.0, 0.8}, poly, 4);
}

internal
void gui_uvbox(V2i pix_pos, V2i pix_size, ModelEntity *m)
{
	const char *box_label= "uvbox_box";
	UiContext *ctx= g_env.uicontext;
	gui_wrap(&pix_pos, &pix_size);
	GuiBoxState state= gui_editorbox(box_label, pix_pos, pix_size);

	if (!m)
		return;

	if (state.pressed) {
		// Control vertex selection
		F64 closest_dist= 0;
		U32 closest_i= NULL_HANDLE;
		for (U32 i= 0; i < m->mesh_v_count; ++i) {
			TriMeshVertex *v= &m->vertices[i];
			V2i pos= uv_to_pix(v->uv, pix_pos, pix_size);

			F64 dist= dist_sqr_v2i(pos, ctx->cursor_pos);
			if (	closest_i == NULL_HANDLE ||
					dist < closest_dist) {
				closest_i= i;
				closest_dist= dist;
			}
		}

		if (!ctx->shift_down) {
			for (U32 i= 0; i < m->mesh_v_count; ++i)
				m->vertices[i].selected= false;
		}

		if (closest_dist < 100*100) {
			ensure(closest_i != NULL_HANDLE);
			toggle_bool(&m->vertices[closest_i].selected);
		}
	}

	if (gui_is_active(box_label) && ctx->grabbing) {
		// Move selected uv coords
		V3d cur= pix_to_uv(ctx->cursor_pos, pix_pos, pix_size);
		V3d prev= pix_to_uv(ctx->prev_cursor_pos, pix_pos, pix_size);
		V3d delta= sub_v3d(cur, prev);
		for (U32 i= 0; i < m->mesh_v_count; ++i) {
			TriMeshVertex *v= &m->vertices[i];
			if (!v->selected)
				continue;
			v->uv= add_v3f(v3d_to_v3f(delta), v->uv);

			v->uv.x= CLAMP(v->uv.x, 0.0, 1.0);
			v->uv.y= CLAMP(v->uv.y, 0.0, 1.0);
		}
	}

	V2i padding= {2, 2};
	pix_pos= add_v2i(pix_pos, padding);
	pix_size= sub_v2i(pix_size, scaled_v2i(2, padding));

	for (U32 i= 0; i < m->mesh_v_count; ++i) {
		TriMeshVertex *v= &m->vertices[i];
		draw_vert(uv_to_pix(v->uv, pix_pos, pix_size), v->selected);
	}
}

// Mesh editing on world
internal
void gui_mesh_overlay(U32 *model_h, bool *is_edit_mode)
{
	UiContext *ctx= g_env.uicontext;
	const char *label= "mesh_overlay";
	bool lmb_pressed= g_env.device->key_pressed[KEY_LMB];
	bool rmb_pressed= g_env.device->key_pressed[KEY_RMB];
	bool g_pressed= g_env.device->key_pressed['g'];
	bool shift_down= g_env.device->key_down[KEY_LSHIFT];
	bool tab_pressed= g_env.device->key_pressed[KEY_TAB];
	bool rmb_down= g_env.device->key_down[KEY_RMB];
	bool rmb_released= g_env.device->key_released[KEY_RMB];
	V3d cur_wp= v2d_to_v3d(screen_to_world_point(ctx->cursor_pos));
	V3d prev_wp= v2d_to_v3d(screen_to_world_point(ctx->prev_cursor_pos));
	F64 v_size= *is_edit_mode ? 0.05 : 0.3;

	ModelEntity *m= NULL;
	if (*model_h != NULL_HANDLE)
		m= get_modelentity(*model_h);

	if (gui_is_active(label)) {
		if (!*is_edit_mode) { // Mesh select mode
			if (rmb_down) {
				/// @todo Bounds
				F64 closest_dist= 0;
				U32 closest_h= NULL_HANDLE;
				for (U32 i= 0; i < MAX_MODELENTITY_COUNT; ++i) {
					if (!g_env.renderer->m_entities[i].allocated)
						continue;

					V3d entity_pos= g_env.renderer->m_entities[i].tf.pos;
					F64 dist= dist_sqr_v3d(entity_pos, cur_wp);
					if (	closest_h == NULL_HANDLE ||
							dist < closest_dist) {
						closest_h= i;
						closest_dist= dist;
					}
				}
				*model_h= closest_h;
			}

			if (rmb_released) {
				gui_set_inactive(label);
			}
		} else { // Edit mode
			ensure(m);
			if (ctx->grabbing) {
				if (lmb_pressed || g_pressed) {
					ctx->grabbing= false;
					gui_set_inactive(label);
				} else {
					V3d cur= mul_t3d(	inv_t3d(m->tf),
										(T3d) {	{1, 1, 1},
												identity_qd(),
												cur_wp}).pos;
					V3d prev= mul_t3d(	inv_t3d(m->tf),
										(T3d) {	{1, 1, 1},
												identity_qd(),
												prev_wp}).pos;
					V3d delta= sub_v3d(cur, prev);

					for (U32 i= 0; i < m->mesh_v_count; ++i) {
						TriMeshVertex *v= &m->vertices[i];
						if (!v->selected)
							continue;
						// Delta to world coords
						v->pos= add_v3f(v3d_to_v3f(delta), v->pos);
					}
				}
			}
		}
	} else if (gui_is_hot(label)) {
		if (tab_pressed) {
			toggle_bool(is_edit_mode);
			if (*model_h == NULL_HANDLE)
				*is_edit_mode= false;
		} else if (g_pressed) {
			ctx->grabbing= true;
			gui_set_active(label);
		} else if (*is_edit_mode && rmb_pressed) {
			// Control vertex selection
			F64 closest_dist= 0;
			U32 closest_i= NULL_HANDLE;
			for (U32 i= 0; i < m->mesh_v_count; ++i) {
				V3d pos= vertex_world_pos(m, i);

				F64 dist= dist_sqr_v3d(pos, cur_wp);
				if (	closest_i == NULL_HANDLE ||
						dist < closest_dist) {
					closest_i= i;
					closest_dist= dist;
				}
			}

			if (!shift_down) {
				for (U32 i= 0; i < m->mesh_v_count; ++i)
					m->vertices[i].selected= false;
			}

			if (closest_dist < 1.0) {
				ensure(closest_i != NULL_HANDLE);
				toggle_bool(&m->vertices[closest_i].selected);
			}
		} else if (rmb_pressed || g_pressed) {
			gui_set_active(label);
		}
	}

	if (*model_h != NULL_HANDLE)
		m= get_modelentity(*model_h);
	if (m) {
		/// @todo Proper drawing
		for (U32 i= 0; i < m->mesh_v_count; ++i) {
			TriMeshVertex *v= &m->vertices[i];
			V3d p= vertex_world_pos(m, i);
			V2d poly[4]= {
				{-v_size + p.x, -v_size + p.y},
				{-v_size + p.x, +v_size + p.y},
				{+v_size + p.x, +v_size + p.y},
				{+v_size + p.x, -v_size + p.y},
			};

			if (!is_edit_mode)
				ddraw_poly((Color) {0.5, 0.5, 0.5, 0.8}, poly, 4);
			else if (v->selected)
				ddraw_poly((Color) {1.0, 0.7, 0.2, 0.8}, poly, 4);
			else
				ddraw_poly((Color) {0.0, 0.0, 0.0, 0.8}, poly, 4);
		}
	}

	gui_set_hot(label);
}

void create_editor()
{
	Editor* e= zero_malloc(sizeof(*e));
	e->cur_model_h= NULL_HANDLE;

	g_env.editor= e;
}

void destroy_editor()
{
	free(g_env.editor);
	g_env.editor= NULL;
}

void toggle_editor()
{ toggle_bool(&g_env.editor->visible); }

void upd_editor()
{
	Editor *e= g_env.editor;
	if (!e->visible) {
		e->is_edit_mode= false;
		return;
	}

	gui_mesh_overlay(&e->cur_model_h, &e->is_edit_mode);

	ModelEntity *m= NULL;
	if (e->cur_model_h != NULL_HANDLE)
		m= get_modelentity(e->cur_model_h);

	const S32 box_size= 400;
	gui_uvbox(	(V2i) {-box_size, 0},
				(V2i) {box_size, box_size},
				m);
}
