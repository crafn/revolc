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

void upd_editor()
{
	Editor *e= g_env.editor;
	if (!e->visible)
		return;

	if (g_env.device->lmb_down) {
		// Find model under cursor

		V2d cursor= {
			2.0*g_env.device->cursor_pos[0]/g_env.device->win_size[0] - 1.0,
			-2.0*g_env.device->cursor_pos[1]/g_env.device->win_size[1] + 1.0
		};
		V2d cursor_w= screen_to_world_point(cursor);

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

		for (U32 i= 0; i < m->mesh_v_count; ++i) {
			V3f v_p= m->vertices[i].pos;
			T3d v_t= identity_t3d();
			v_t.pos= v3f_to_v3d(v_p);

			T3d t= mul_t3d(m->tf, v_t);
			V2d p= {t.pos.x, t.pos.y};
			V2d poly[4]= {
				{-0.1 + p.x, -0.1 + p.y},
				{-0.1 + p.x, +0.1 + p.y},
				{+0.1 + p.x, +0.1 + p.y},
				{+0.1 + p.x, -0.1 + p.y},
			};
			ddraw_poly((Color) {1.0, 0.8, 0.4, 0.8}, poly, 4);
		}
	}
}
