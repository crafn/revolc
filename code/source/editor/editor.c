#include "core/malloc.h"
#include "editor.h"
#include "global/env.h"
#include "platform/device.h"
#include "visual/renderer.h"

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
{
	g_env.editor->visible= !g_env.editor->visible;
}

internal
V2d vertex_world_pos(ModelEntity *m, U32 i)
{
	TriMeshVertex *v= &m->vertices[i];
	T3d v_t= identity_t3d();
	v_t.pos= v3f_to_v3d(v->pos);

	T3d t= mul_t3d(m->tf, v_t);
	V2d p= {t.pos.x, t.pos.y};
	return p;
}

internal
void toggle_bool(bool *b)
{ *b = !*b; }

void upd_editor()
{
	Editor *e= g_env.editor;
	if (!e->visible)
		return;

	/// @todo Proper input layer

	V2d cursor_p= {
		2.0*g_env.device->cursor_pos[0]/g_env.device->win_size[0] - 1.0,
		-2.0*g_env.device->cursor_pos[1]/g_env.device->win_size[1] + 1.0
	};
	local_persist V2d last_cursor_w;
	V2d cursor_w= screen_to_world_point(cursor_p);
	V2d delta_cursor_w= sub_v2d(cursor_w, last_cursor_w);
	last_cursor_w= cursor_w;

	bool lmb_pressed= g_env.device->key_pressed[KEY_LMB];
	bool rmb_down= g_env.device->key_down[KEY_RMB];
	bool rmb_pressed= g_env.device->key_pressed[KEY_RMB];
	bool tab_pressed= g_env.device->key_pressed[KEY_TAB];
	bool g_pressed= g_env.device->key_pressed['g'];
	bool shift_down= g_env.device->key_down[KEY_LSHIFT];

	if (tab_pressed)
		toggle_bool(&e->is_edit_mode);

	if (!e->is_edit_mode && rmb_down) {
		// Find model under cursor

		/// @todo Bounds
		F64 closest_dist= 0;
		U32 closest_h= NULL_HANDLE;
		for (U32 i= 0; i < MAX_MODELENTITY_COUNT; ++i) {
			if (!g_env.renderer->m_entities[i].allocated)
				continue;

			V2d entity_pos= v3d_to_v2d(g_env.renderer->m_entities[i].tf.pos);
			F64 dist= dist_sqr_v2d(entity_pos, cursor_w);
			if (	closest_h == NULL_HANDLE ||
					dist < closest_dist) {
				closest_h= i;
				closest_dist= dist;
			}
		}

		e->cur_model_h= closest_h;
	}

	if (e->cur_model_h != NULL_HANDLE) {
		ModelEntity *m= &g_env.renderer->m_entities[e->cur_model_h];
		F64 v_size= e->is_edit_mode ? 0.05 : 0.3;

		// Mesh edit
		if (e->is_edit_mode) {
			if (rmb_pressed) {
				// Find closest vertex
				F64 closest_dist= 0;
				U32 closest_i= NULL_HANDLE;
				for (U32 i= 0; i < m->mesh_v_count; ++i) {
					V2d pos= vertex_world_pos(m, i);

					F64 dist= dist_sqr_v2d(pos, cursor_w);
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
			}

			if (g_pressed)
				toggle_bool(&e->grabbing);

			if (e->grabbing) {
				if (lmb_pressed) {
					e->grabbing= false;
				} else {
					for (U32 i= 0; i < m->mesh_v_count; ++i) {
						TriMeshVertex *v= &m->vertices[i];
						if (v->selected)
							v->pos= add_v3f((V3f) {delta_cursor_w.x, delta_cursor_w.y}, v->pos);
					}
				}
			}
		} else { // Mesh select mode
			e->grabbing= false;
		}

		// Draw
		/// @todo Proper drawing
		for (U32 i= 0; i < m->mesh_v_count; ++i) {
			TriMeshVertex *v= &m->vertices[i];
			V2d p= vertex_world_pos(m, i);
			V2d poly[4]= {
				{-v_size + p.x, -v_size + p.y},
				{-v_size + p.x, +v_size + p.y},
				{+v_size + p.x, +v_size + p.y},
				{+v_size + p.x, -v_size + p.y},
			};
			if (!e->is_edit_mode)
				ddraw_poly((Color) {1.0, 0.5, 0.2, 0.8}, poly, 4);
			else if (v->selected)
				ddraw_poly((Color) {1.0, 0.5, 0.2, 0.8}, poly, 4);
			else
				ddraw_poly((Color) {0.0, 0.0, 0.0, 0.8}, poly, 4);
		}
	}
}
