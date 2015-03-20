#include "core/malloc.h"
#include "editor.h"
#include "global/env.h"
#include "platform/device.h"
#include "ui/uicontext.h"
#include "visual/renderer.h"

typedef struct EditorBoxState {
	bool hover;
	bool down;
	bool pressed;
	bool released;
} EditorBoxState;

internal
void toggle_bool(bool *b)
{ *b = !*b; }

internal
F64 editor_vertex_size()
{ return screen_to_world_size((V2i) {5, 0}).x; }

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

// Draws single-color quad
internal
void gui_quad(V2i pix_pos, V2i pix_size, Color c)
{
	V3d pos= v2d_to_v3d(screen_to_world_point(pix_pos)); 
	V3d size= v2d_to_v3d(screen_to_world_size(pix_size));

	ModelEntity init;
	init_modelentity(&init);
	init.tf.pos= pos;
	init.tf.scale= size;
	init.free_after_draw= true;
	snprintf(init.model_name, sizeof(init.model_name), "guibox_singular");

	U32 handle= resurrect_modelentity(&init);
	ModelEntity *e= get_modelentity(handle);
	e->color= c;
}

// Draws texture of a model
internal
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
	snprintf(init.model_name, sizeof(init.model_name), "guibox");

	U32 handle= resurrect_modelentity(&init);
	ModelEntity *e= get_modelentity(handle);
	e->atlas_uv= src_model->atlas_uv;
	e->scale_to_atlas_uv= src_model->scale_to_atlas_uv;
}

internal
EditorBoxState gui_editorbox(const char *label, V2i pix_pos, V2i pix_size, bool invisible)
{
	gui_wrap(&pix_pos, &pix_size);
	UiContext *ctx= g_env.uicontext;
	const V2i c_p= ctx->cursor_pos;
	const Color c= (Color) {0.25, 0.25, 0.3, 0.9};

	EditorBoxState state= {};

	if (gui_is_active(label)) {
		state.pressed= false;
		if (!ctx->rmb.down && !ctx->grabbing) {
			state.released= true;
			gui_set_inactive(label);
		} else if (ctx->rmb.down) {
			state.down= true;
		}

		if (ctx->lmb.pressed || ctx->g_pressed) {
			ctx->grabbing= 0;
			gui_set_inactive(label);
		}
	} else if (gui_is_hot(label)) {
		if (ctx->rmb.pressed) {
			state.pressed= true;
			state.down= true;
			gui_set_active(label);

		} else if (ctx->g_pressed) {
			ctx->grabbing= gui_id(label);
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

	if (!invisible)
		gui_quad(pix_pos, pix_size, c);

	return state;
}

internal
void destroy_rt_mesh(Resource *res);

// Creates modifiable substitute for static mesh resource
internal
Mesh *create_rt_mesh(Mesh *src)
{
	Mesh *rt_mesh= dev_malloc(sizeof(*rt_mesh));
	*rt_mesh= *src;
	substitute_res(&src->res, &rt_mesh->res, destroy_rt_mesh);

	TriMeshVertex *verts= dev_malloc(sizeof(*verts)*src->v_count);
	memcpy(verts, mesh_vertices(src), sizeof(*verts)*src->v_count);

	MeshIndexType *inds= dev_malloc(sizeof(*inds)*src->i_count);
	memcpy(inds, mesh_indices(src), sizeof(*inds)*src->i_count);

	rt_mesh->v_offset= blob_offset(&rt_mesh->res, verts);
	rt_mesh->i_offset= blob_offset(&rt_mesh->res, inds);

	return rt_mesh;
}

internal
void destroy_rt_mesh(Resource *res)
{
	Mesh *m= (Mesh*)res;
	dev_free(blob_ptr(res, m->v_offset));
	dev_free(blob_ptr(res, m->i_offset));
	dev_free(m);
}

internal
void modify_mesh(ModelEntity *m, V3d delta, bool uv)
{
	if (m->has_own_mesh) {
		debug_print("@todo Modify unique mesh");
		return;
	}

	Mesh *mesh= model_mesh((Model*)res_by_name(	g_env.resblob,
												ResType_Model,
												m->model_name));
	if (!mesh->res.is_runtime_res) {
		mesh= create_rt_mesh(mesh);
		// Requery pointers to the new mesh
		recache_modelentities();
	}

	for (U32 i= 0; i < mesh->v_count; ++i) {
		TriMeshVertex *v= &mesh_vertices(mesh)[i];
		if (!v->selected)
			continue;
		if (uv) {
			v->uv= add_v3f(v3d_to_v3f(delta), v->uv);
			v->uv.x= CLAMP(v->uv.x, 0.0, 1.0);
			v->uv.y= CLAMP(v->uv.y, 0.0, 1.0);
		} else {
			v->pos= add_v3f(v3d_to_v3f(delta), v->pos);
		}
	}

	mesh->res.needs_saving= true;
}

internal
V2i uv_to_pix(V3f uv, V2i pix_pos, V2i pix_size)
{ return (V2i) {uv.x*pix_size.x + pix_pos.x, (1 - uv.y)*pix_size.y + pix_pos.y}; }

internal
V3d pix_to_uv(V2i p, V2i pix_pos, V2i pix_size)
{ return (V3d) {(F64)(p.x - pix_pos.x)/pix_size.x,
				1 - (F64)(p.y - pix_pos.y)/pix_size.y,
				0.0};
}

internal
void gui_uvbox(V2i pix_pos, V2i pix_size, ModelEntity *m)
{
	const char *box_label= "uvbox_box";
	UiContext *ctx= g_env.uicontext;
	gui_wrap(&pix_pos, &pix_size);

	EditorBoxState state= gui_editorbox(box_label, pix_pos, pix_size, false);

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

	if (ctx->grabbing == gui_id(box_label)) {
		// Move selected uv coords
		V3d cur= pix_to_uv(ctx->cursor_pos, pix_pos, pix_size);
		V3d prev= pix_to_uv(ctx->prev_cursor_pos, pix_pos, pix_size);
		V3d delta= sub_v3d(cur, prev);
		modify_mesh(m, delta, true);
	}

	V2i padding= {20, 20};
	pix_pos= add_v2i(pix_pos, padding);
	pix_size= sub_v2i(pix_size, scaled_v2i(2, padding));

	gui_model_image(pix_pos, pix_size, m);

	for (U32 i= 0; i < m->mesh_v_count; ++i) {
		TriMeshVertex *v= &m->vertices[i];

		V2i pix_uv= uv_to_pix(v->uv, pix_pos, pix_size);
		gui_wrap(&pix_uv, NULL);
		V2d p= screen_to_world_point(pix_uv);
		const F64 v_size= editor_vertex_size();
		V3d poly[4]= {
			{-v_size + p.x, -v_size + p.y, 0},
			{-v_size + p.x, +v_size + p.y, 0},
			{+v_size + p.x, +v_size + p.y, 0},
			{+v_size + p.x, -v_size + p.y, 0},
		};
		if (v->selected)
			ddraw_poly((Color) {1.0, 0.7, 0.2, 0.8}, poly, 4);
		else
			ddraw_poly((Color) {0.0, 0.0, 0.0, 0.8}, poly, 4);
	}

	Color fill_color= {0.6, 0.6, 0.8, 0.4};
	V3d poly[3];
	for (U32 i= 0; i < m->mesh_i_count; ++i) {
		U32 v_i= m->indices[i];
		TriMeshVertex *v= &m->vertices[v_i];
		V2i pix_uv= uv_to_pix(v->uv, pix_pos, pix_size);
		V2d p= screen_to_world_point(pix_uv);
		poly[i%3]= (V3d) {p.x, p.y, 0};

		if (i % 3 == 2)
			ddraw_poly(fill_color, poly, 3);
	}
}

// Mesh editing on world
internal
void gui_mesh_overlay(U32 *model_h, bool *is_edit_mode)
{
	UiContext *ctx= g_env.uicontext;
	V3d cur_wp= v2d_to_v3d(screen_to_world_point(ctx->cursor_pos));
	V3d prev_wp= v2d_to_v3d(screen_to_world_point(ctx->prev_cursor_pos));
	F64 v_size= editor_vertex_size();

	ModelEntity *m= NULL;
	if (*model_h != NULL_HANDLE)
		m= get_modelentity(*model_h);

	const char *box_label= "mesh_editorbox";
	EditorBoxState state=
		gui_editorbox(box_label, (V2i) {0, 0}, g_env.device->win_size, true);

	if (!*is_edit_mode) { // Mesh select mode
		if (state.down) {
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
	} else { // Edit mode
		if (ctx->grabbing == gui_id(box_label)) {
			ensure(m);
			V3d cur= mul_t3d(	inv_t3d(m->tf),
								(T3d) {	{1, 1, 1},
										identity_qd(),
										cur_wp}).pos;
			V3d prev= mul_t3d(	inv_t3d(m->tf),
								(T3d) {	{1, 1, 1},
										identity_qd(),
										prev_wp}).pos;
			V3d delta= sub_v3d(cur, prev);

			modify_mesh(m, delta, false);
		}
	}

	if (*is_edit_mode && state.pressed) {
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

		if (!ctx->shift_down) {
			for (U32 i= 0; i < m->mesh_v_count; ++i)
				m->vertices[i].selected= false;
		}

		if (closest_dist < 2.0) {
			ensure(closest_i != NULL_HANDLE);
			toggle_bool(&m->vertices[closest_i].selected);
		}
	}

	if (*model_h != NULL_HANDLE)
		m= get_modelentity(*model_h);
	if (m) {
		if (*is_edit_mode) {
			for (U32 i= 0; i < m->mesh_v_count; ++i) {
				TriMeshVertex *v= &m->vertices[i];
				V3d p= vertex_world_pos(m, i);
				V3d poly[4]= {
					{-v_size + p.x, -v_size + p.y, p.z},
					{-v_size + p.x, +v_size + p.y, p.z},
					{+v_size + p.x, +v_size + p.y, p.z},
					{+v_size + p.x, -v_size + p.y, p.z},
				};

				if (v->selected)
					ddraw_poly((Color) {1.0, 0.7, 0.2, 0.8}, poly, 4);
				else
					ddraw_poly((Color) {0.0, 0.0, 0.0, 0.8}, poly, 4);
			}
		}

		Color fill_color= {0.6, 0.6, 0.8, 0.4};
		if (!*is_edit_mode)
			fill_color= (Color) {1.0, 0.8, 0.5, 0.6};

		V3d poly[3];
		for (U32 i= 0; i < m->mesh_i_count; ++i) {
			U32 v_i= m->indices[i];
			V3d p= vertex_world_pos(m, v_i);
			poly[i%3]= p;

			if (i % 3 == 2)
				ddraw_poly(fill_color, poly, 3);
		}
	}
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
	if (!e->visible)
		return;

/*	if (g_env.device->key_pressed['m']) {
		Mesh *mesh= (Mesh*)res_by_name(g_env.resblob, ResType_Mesh, "unitquad");

		ParsedJsonFile j_file= malloc_parsed_json_file("test.json");

		WJson *j_upd= wjson_create();
		mesh_to_json(j_upd, mesh);
		//wjson_dump(j_obj);

		wjson_write_updated("test_upd.json", j_file.root, j_upd);

		wjson_destroy(j_upd);
		free_parsed_json_file(j_file);
	}
	*/

	bool tab_pressed= g_env.device->key_pressed[KEY_TAB];
	if (tab_pressed) {
		toggle_bool(&e->is_edit_mode);
		if (e->cur_model_h == NULL_HANDLE)
			e->is_edit_mode= false;
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
